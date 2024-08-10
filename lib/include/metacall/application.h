/*
    This file is part of metacall.

    sasCore is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    sasCore is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with sasCore.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef METACALL_APPLICATION_H
#define METACALL_APPLICATION_H

#include "common.h"
#include "connection.h"
#include "object.h"

#include <memory>
#include <list>
#include <any>
#include <mutex>
#include <functional>

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

class application;

namespace _internal {

template<typename FuncT>
union _func_cast_helper
{
    FuncT func_ptr;
    void* void_ptr;
};

template<typename FuncT>
constexpr void * func_to_void_ptr(FuncT func)
{
    return _func_cast_helper<FuncT> { func }.void_ptr;
}

}

class application final : public object
{
    METACALL_COPY_PROTECTOR(application)

    struct _private;
    std::unique_ptr<_private> p;
public:
    static const std::shared_ptr<application> & get();

    application();
    ~application() final override;

    virtual void destroy_object() override;

    void disconnect(const object *);
    void disconnect(const connection::ptr &);

    template<typename SourceT, typename RecT, typename RetT, typename ...ArgT>
    connection::ptr
    connect(object * sender, SourceT source, RecT * receiver, RetT(RecT::*destination)(ArgT...))
    {
        assert(sender && sender->app().get() == this && (!receiver || receiver->app().get() == this));
        auto conn = cpp_connection::make(sender, _internal::func_to_void_ptr(source), receiver, destination);
        add_connection(sender, source, receiver, conn);
        return conn;
    }

    template<typename SourceFuncT, typename RecT, typename DestinationFuncT>
    connection::ptr
    connect(object * sender, SourceFuncT source, RecT * receiver, const DestinationFuncT & destination)
    {
        assert(sender && sender->app().get() == this && (!receiver || receiver->app().get() == this));
        auto conn = cpp_connection::make(sender, _internal::func_to_void_ptr(source), receiver, std::function(destination));
        add_connection(sender, source, receiver, conn);
        return conn;
    }

    template<typename SourceFuncT, typename DestinationFuncT>
    connection::ptr
    connect(object * sender, SourceFuncT source, const DestinationFuncT & destination)
    {
        return connect<SourceFuncT, object, DestinationFuncT>(sender, source, nullptr, destination);
    }

    template<typename ObjT, typename SourceFuncT>
    std::list<connection::ptr> get_connections(const ObjT * sender, SourceFuncT source)
    {
        std::unique_lock __locker(*this);
        return _get_connections(sender, _internal::func_to_void_ptr(source));
    }

    template<typename ObjT, typename SourceFuncT>
    void add_connection(const ObjT * sender, SourceFuncT source, object * receiver, const connection::ptr & conn)
    {
        _add_connection(sender, _internal::func_to_void_ptr(source), receiver, conn);
    }

    enum class calling_rule
    {
        automatic,
        direct,
        priority,
        queued,
    };

    template<typename ObjT, typename RetT, typename ...ArgT>
    RetT emit(ObjT * sender, RetT(ObjT::*source)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        return std::any_cast<RetT>(_emit(get_connections(sender, source), cpp_connection::to_any(std::move(args)...)));
    }

    template<typename ObjT, typename ...ArgT>
    void emit(calling_rule rule, ObjT * sender, void(ObjT::*source)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        _emit(rule, get_connections(sender, source), cpp_connection::to_any(std::move(args)...));
    }

    template<typename ObjT, typename ...ArgT>
    void emit(ObjT * sender, void(ObjT::*source)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        emit<ObjT, ArgT...>(calling_rule::direct, sender, source, std::move(args)...);
    }

    template<typename RetT, typename ...ArgT>
    RetT emit(const std::list<connection::ptr> & conns, const typename std::remove_reference<ArgT>::type & ... args)
    {
        return std::any_cast<RetT>(_emit(conns, cpp_connection::to_any(std::move(args)...)));
    }

    template<typename ...ArgT>
    void emit(calling_rule rule, const std::list<connection::ptr> & conns, const typename std::remove_reference<ArgT>::type & ... args)
    {
        _emit(rule, conns, cpp_connection::to_any(std::move(args)...));
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    RetT call(const object * sender, RecT * receiver, RetT(RecT::*destination)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        return std::any_cast<RetT>(call(cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...)));
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    void call(calling_rule rule, const object * sender, RecT * receiver, RetT(RecT::*destination)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        call(rule, cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...));
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    void call(std::chrono::steady_clock::time_point time, calling_rule rule, const object * sender, RecT * receiver, RetT(RecT::*destination)(ArgT...), const typename std::remove_reference<ArgT>::type & ... args)
    {
        call(time, rule, cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...));
    }

    template<typename RetT, typename ...ArgT>
    RetT call(const object * sender, object * receiver, const std::function<RetT(ArgT...)> & destination, const typename std::remove_reference<ArgT>::type & ... args)
    {
        return std::any_cast<RetT>(call(cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...)));
    }

    template<typename RetT, typename ...ArgT>
    void call(calling_rule rule, const object * sender, object * receiver, const std::function<RetT(ArgT...)> & destination, const typename std::remove_reference<ArgT>::type & ... args)
    {
        call(rule, cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...));
    }

    template<typename RetT, typename ...ArgT>
    void call(std::chrono::steady_clock::time_point time, calling_rule rule, const object * sender, object * receiver, const std::function<RetT(ArgT...)> & destination, const typename std::remove_reference<ArgT>::type & ... args)
    {
        call(time, rule, cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination), connection::to_any(std::move(args)...));
    }

    template<typename ...ArgT>
    void call(std::chrono::steady_clock::time_point time, calling_rule rule, const connection::ptr & conn, const typename std::remove_reference<ArgT>::type & ... args)
    {
        call(time, rule, conn, connection::to_any(std::move(args)...));
    }

    template<typename RetT, typename ...ArgT>
    void call(calling_rule rule, const object * sender, const std::function<RetT(ArgT...)> & destination, const typename std::remove_reference<ArgT>::type & ... args)
    {
        call<RetT, ArgT...>(rule, sender, nullptr, destination, std::move(args)...);
    }

    template<typename RetT, typename ...ArgT>
    static connection::ptr make_connection(const object * sender, object * receiver, const std::function<RetT(ArgT...)> & destination)
    {
        return cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination);
    }

    template<typename SourceT, typename RetT, typename ...ArgT>
    static connection::ptr make_connection(const object * sender, SourceT source, object * receiver, const std::function<RetT(ArgT...)> & destination)
    {
        return cpp_connection::make(sender, _internal::func_to_void_ptr(source), receiver, destination);
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    static connection::ptr make_connection(object * sender, RecT * receiver, RetT(RecT::*destination)(ArgT...))
    {
        return cpp_connection::make(sender, static_cast<const void*>(nullptr), receiver, destination);
    }

    template<typename SourceT, typename RecT, typename RetT, typename ...ArgT>
    static connection::ptr make_connection(object * sender, SourceT source, RecT * receiver, RetT(RecT::*destination)(ArgT...))
    {
        return cpp_connection::make(sender, _internal::func_to_void_ptr(source), receiver, destination);
    }

    bool activate();
    void run();

    int exec();
    int final();

    void exit(int);

protected:
    virtual void call_notification();
    virtual void call_exit(int);
    virtual inline void define_timer(const void* internal_object, std::chrono::steady_clock::time_point, std::function<void()> call) { (void)internal_object, (void)call; };
    virtual inline void undefine_timer(const void* internal_object, std::chrono::steady_clock::time_point) { (void)internal_object; };

private:
    friend std::unique_lock<application>;
    void lock();
    void unlock();

    std::any call(const connection::ptr & conn, const std::vector<std::any> & args);
    void call(calling_rule rule, const connection::ptr & conn, const std::vector<std::any> & args);
    void call(std::chrono::steady_clock::time_point time, calling_rule rule, const connection::ptr & conn, const std::vector<std::any> & args);

    std::any _emit(const std::list<connection::ptr> & conns, const std::vector<std::any> & args);
    void _emit(calling_rule rule, const std::list<connection::ptr> & conns, const std::vector<std::any> & args);

    void _add_connection(const object * sender, const void * source, object * receiver, const connection::ptr & conn);
    std::list<connection::ptr> _get_connections(const object * sender, const void * source);

    void add_to_queue(calling_rule rule, const std::list<connection::ptr> &, const std::vector<std::any> & args);
    void add_to_queue(std::chrono::steady_clock::time_point time, calling_rule rule, const std::list<connection::ptr> &, const std::vector<std::any> & args);

    mutable application * this_app = this;
};

template<typename SourceT, typename RecT, typename RetT, typename ...ArgT>
connection::ptr
connect(object * sender, SourceT source, RecT * receiver, RetT(RecT::*destination)(ArgT...))
{
    return sender->app()->connect<SourceT, RecT, RetT, ArgT...>(sender, source, receiver, destination);
}

template<typename SourceFuncT, typename RecT, typename DestinationFuncT>
connection::ptr
connect(object * sender, SourceFuncT source, RecT * receiver, const DestinationFuncT & destination)
{
    return sender->app()->connect<SourceFuncT, RecT, DestinationFuncT>(sender, source, receiver, destination);
}

template<typename SourceFuncT, typename DestinationFuncT>
connection::ptr
connect(object * sender, SourceFuncT source, const DestinationFuncT & destination)
{
    return sender->app()->connect<SourceFuncT, DestinationFuncT>(sender, source, destination);
}

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_APPLICATION_H
