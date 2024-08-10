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

#ifndef METACALL_DELEGATE_H
#define METACALL_DELEGATE_H

#include "common.h"
#include "object.h"
#include "application.h"

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

class delegate
{
protected:
    virtual std::list<connection::ptr> get_connections() const = 0;
    virtual const std::shared_ptr<application> & app() const = 0;

    inline delegate() = default;

public:
    using ptr = std::shared_ptr<delegate>;

    virtual inline ~delegate() = default;

    template<typename RetT, typename ...ArgT>
    RetT invoke(const ArgT && ... args)
    {
        return app()->emit<RetT, ArgT...>(get_connections(), args...);
    }

    template<typename ...ArgT>
    void invoke(const ArgT && ... args)
    {
        app()->emit<ArgT...>(application::calling_rule::automatic, get_connections(), args...);
    }

    template<typename ...ArgT>
    void invoke(application::calling_rule rule, const ArgT && ... args)
    {
        app()->emit<ArgT...>(rule, get_connections(), args...);
    }
};

class empty_delegate final : public delegate
{
    const std::shared_ptr<application> m_app;

    inline empty_delegate(const std::shared_ptr<application> & app)
        : delegate()
        , m_app(app)
    { }

protected:
    inline std::list<connection::ptr> get_connections() const final override
    { return { }; }

    inline const std::shared_ptr<application> & app() const final override
    { return m_app; }

public:
    inline ~empty_delegate() final override = default;

    static inline
    delegate::ptr make(const std::shared_ptr<application> & app)
    { return std::shared_ptr<delegate>(new empty_delegate(app)); }

    static inline
    delegate::ptr make()
    { return std::shared_ptr<delegate>(new empty_delegate(application::get())); }
};

class cpp_delegate : public delegate
{
    struct base_invoker : object
    {
        inline base_invoker(const std::shared_ptr<application> & app) : object(app) { };
        virtual inline ~base_invoker() = default;

        virtual std::list<connection::ptr> get_connections() const = 0;
    };

    template<typename FuncT>
    struct invoker { };

    template<typename RetT, typename ...ArgT>
    struct invoker<std::function<RetT(ArgT...)>> final : base_invoker
    {
        inline invoker(const std::shared_ptr<application> & app) : base_invoker(app) { }
        inline ~invoker() final override = default;

        inline RetT invoke(const ArgT && ...) { return {}; }

        inline std::list<connection::ptr> get_connections() const final override
        {
            return app()->get_connections(this, &invoker<std::function<RetT(ArgT...)>>::invoke);
        }
    };

    template<typename ...ArgT>
    struct invoker<std::function<void(ArgT...)>> final : base_invoker
    {
        inline invoker(const std::shared_ptr<application> & app) : base_invoker(app) { }
        inline ~invoker() final override = default;

        void invoke(const ArgT && ...) { }

        std::list<connection::ptr> get_connections() const final override
        {
             return app()->get_connections(this, &invoker<std::function<void(ArgT...)>>::invoke);
        }
    };

    template<typename ObjT, typename RetT, typename ...ArgT>
    struct invoker<RetT(ObjT::*)(ArgT...)> final : base_invoker
    {
        inline invoker(const std::shared_ptr<application> & app) : base_invoker(app) { }
        inline ~invoker() final override = default;

        RetT invoke(const ArgT && ...) { return {}; }

        std::list<connection::ptr> get_connections() const final override
        {
            return app()->get_connections(this, &invoker<RetT(ObjT::*)(ArgT...)>::invoke);
        }
    };

    template<typename ObjT, typename ...ArgT>
    struct invoker<void(ObjT::*)(ArgT...)> final : base_invoker
    {
        inline invoker(const std::shared_ptr<application> & app) : base_invoker(app) { }
        inline ~invoker() final override = default;

        void invoke(const ArgT && ...) { }

        std::list<connection::ptr> get_connections() const final override
        {
            return app()->get_connections(this, &invoker<void(ObjT::*)(ArgT...)>::invoke);
        }
    };

    std::shared_ptr<base_invoker> m_invoker;

    inline cpp_delegate(const std::shared_ptr<base_invoker> & invoker)
        : delegate()
        , m_invoker(invoker)
    { }

protected:
    virtual inline std::list<connection::ptr> get_connections() const override
    {
        return m_invoker->get_connections();
    }

    virtual inline const std::shared_ptr<application> & app() const override
    {
        return m_invoker->app();
    }

public:
    using Ptr = std::shared_ptr<delegate>;

    virtual inline ~cpp_delegate() override = default;

    template<typename RecT, typename DestinationFuncT>
    static
    delegate::ptr make(const std::shared_ptr<application> & app, RecT * receiver, const DestinationFuncT & destination)
    {
        auto invoker_ = object::make<invoker<DestinationFuncT>>(app);
        app->connect(invoker_.get(), &cpp_delegate::invoker<DestinationFuncT>::invoke, receiver, destination);
        return std::shared_ptr<delegate>(new cpp_delegate(invoker_));
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    static
    delegate::ptr make(const std::shared_ptr<application> & app, RecT * receiver, RetT(RecT::*destination)(ArgT...))
    {
        auto invoker_ = object::make<invoker<RetT(RecT::*)(ArgT...)>>(app);
        app->connect(invoker_.get(), &cpp_delegate::invoker<RetT(RecT::*)(ArgT...)>::invoke, receiver, destination);
        return delegate::ptr(new cpp_delegate(invoker_));
    }

    template<typename RecT, typename DestinationFuncT>
    static
    delegate::ptr make(RecT * receiver, const DestinationFuncT & destination)
    {
        assert(receiver);
        return make<RecT, DestinationFuncT>(receiver->app(), receiver, destination);
    }

    template<typename RecT, typename RetT, typename ...ArgT>
    static
    delegate::ptr make(RecT * receiver, RetT(RecT::*destination)(ArgT...))
    {
        assert(receiver);
        return make<RecT, RetT, ArgT...>(receiver->app(), receiver, destination);
    }
};

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_DELEGATE_H
