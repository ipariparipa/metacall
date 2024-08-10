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

#ifndef METACALL_CONNECTION_H
#define METACALL_CONNECTION_H

#include "common.h"

#include <memory>
#include <functional>
#include <any>
#include <tuple>
#include <array>

#include "assert.h"

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

class object;
class application;

class connection
{
public:
    using ptr = std::shared_ptr<connection>;

    virtual inline ~connection() = default;
    virtual std::size_t hash() const = 0;
    virtual const object * sender() const = 0;
    virtual const void * source() const = 0;
    virtual object * receiver() const = 0;
    virtual std::any invoke(const std::vector<std::any> & vargs) = 0;

    template<typename ... ArgT>
    static
    std::vector<std::any> to_any(const ArgT && ... args)
    {
        std::vector<std::any> _args(sizeof... (ArgT));
        size_t i = 0;
        using tmp = int[];
        (void)tmp { 0, (_args[i++] = std::make_any<ArgT>(args), 0)... };

        return _args;
    }
};

class cpp_connection final : public connection
{
    struct _private;
    std::unique_ptr<_private> p;

    class invoker
    {
    public:
        inline virtual ~invoker() = default;
        virtual std::any invoke(const std::vector<std::any> & args) = 0;
    };

    template<typename ObjT, typename FuncT>
    struct traits
    {
        using func = FuncT;
    };

    template<typename ObjT, typename RetT, typename ...ArgT>
    struct traits<ObjT, std::function<RetT(ArgT...)>>
    {
        static const size_t nargs = sizeof...(ArgT);

        using retval = RetT;

        using tuple = std::tuple<std::remove_const_t<std::remove_reference_t<ArgT>>...>;

        template <size_t i>
        struct arg
        { using type = typename std::tuple_element<i, tuple>::type; };
    };

    template<typename ObjT, typename RetT, typename ...ArgT>
    struct traits<ObjT, RetT(ObjT::*)(ArgT...)>
    {
        static const size_t nargs = sizeof...(ArgT);

        using retval = RetT;

        using tuple = std::tuple<std::remove_const_t<std::remove_reference_t<ArgT>>...>;

        template <size_t i>
        struct arg
        { using type = typename std::tuple_element<i, tuple>::type; };
    };

    template<typename traits>
    class base_function_invoker : public invoker
    {
    public:
        virtual ~base_function_invoker() override = default;
    protected:

        template<std::size_t I = 0>
        typename std::enable_if<I == traits::nargs, void>::type _cast_to(const std::array<const std::any*, traits::nargs> & , typename traits::tuple &)
        { }

        template<std::size_t I = 0>
        typename std::enable_if<I < traits::nargs, void>::type _cast_to(const std::array<const std::any*, traits::nargs> & from, typename traits::tuple & to)
        {
            std::get<I>(to) = std::any_cast<typename traits::template arg<I>::type>(*from[I]);
            _cast_to<I + 1>(from, to);
        }

        void cast_to(const std::array<const std::any*, traits::nargs> & from, typename traits::tuple & to)
        { _cast_to(from, to); }

        virtual std::any apply(typename traits::tuple & args) = 0;

    public:
        std::any invoke(const std::vector<std::any> & vargs) final override
        {
            assert(traits::nargs == vargs.size());

            std::array<const std::any*, traits::nargs> tmp;
            for(size_t  i = 0; i < traits::nargs; ++i)
                tmp[i] = &vargs[i];

            typename traits::tuple args;
            cast_to(tmp, args);

            return apply(args);
        }
    };

    template<typename ObjT, typename RetT, typename ...ArgT>
    class method_invoker final : public base_function_invoker<cpp_connection::traits<ObjT, RetT(ObjT::*)(ArgT...)>>
    {
    public:
        using MethT = RetT(ObjT::*)(ArgT...);
    private:
        ObjT * m_rec;
        MethT m_dst;

        using traits = cpp_connection::traits<ObjT, MethT>;
        using BaseT = base_function_invoker<traits>;

        template<std::size_t... I>
        RetT apply(typename traits::tuple & args, std::index_sequence<I...>)
        { return (m_rec->*m_dst)(std::get<I>(args)...); }

        std::any apply(typename traits::tuple & args) final override
        { return apply(args, std::make_index_sequence<traits::nargs>{}); }

    public:
        method_invoker(ObjT * rec, MethT dst)
            : m_rec(rec), m_dst(dst)
        { }

        ~method_invoker() final override = default;
    };

    template<typename ObjT, typename ...ArgT>
    class method_invoker_void final : public base_function_invoker<cpp_connection::traits<ObjT, void(ObjT::*)(ArgT...)>>
    {
    public:
        using MethT = void(ObjT::*)(ArgT...);
    private:
        ObjT * m_rec;
        MethT m_dst;

        using traits = cpp_connection::traits<ObjT, MethT>;
        using BaseT = base_function_invoker<traits>;

        template<std::size_t... I>
        void apply(typename traits::tuple & args, std::index_sequence<I...>)
        {
            (m_rec->*m_dst)(std::get<I>(args)...);
        }

        std::any apply(typename traits::tuple & args) final override
        {
            apply(args, std::make_index_sequence<traits::nargs>{});
            return std::any();
        }

    public:
        method_invoker_void(ObjT * rec, MethT dst)
            : m_rec(rec), m_dst(dst)
        { }

        ~method_invoker_void() final override = default;
    };

    template<typename FuncT>
    class lambda_invoker final : public base_function_invoker<cpp_connection::traits<void, FuncT>>
    {
        using traits = cpp_connection::traits<void, FuncT>;
        using BaseT = base_function_invoker<cpp_connection::traits<void, FuncT>>;

        FuncT m_dst;

        template<std::size_t... I>
        auto apply(typename traits::tuple & args, std::index_sequence<I...>)
        { return std::invoke(m_dst, std::get<I>(args)...); }

        std::any apply(typename traits::tuple & args) final override
        { return apply(args, std::make_index_sequence<traits::nargs>{}); }

    public:
        lambda_invoker(const FuncT & dst)
            : m_dst(dst)
        { }

        ~lambda_invoker() final override = default;
    };

    template<typename FuncT>
    class lambda_invoker_void final : public base_function_invoker<cpp_connection::traits<void, FuncT>>
    {
        using traits = cpp_connection::traits<void, FuncT>;
        using BaseT = base_function_invoker<cpp_connection::traits<void, FuncT>>;

        FuncT m_dst;

        template<std::size_t... I>
        void apply(typename traits::tuple & args, std::index_sequence<I...>)
        { std::invoke(m_dst, std::get<I>(args)...); }

        std::any apply(typename traits::tuple & args) final override
        {
            apply(args, std::make_index_sequence<traits::nargs>{});
            return std::any();
        }

    public:
        lambda_invoker_void(const FuncT & dst)
            : m_dst(dst)
        { }

        ~lambda_invoker_void() final override = default;
    };
public:
    friend class application;

    METACALL_COPY_PROTECTOR(cpp_connection);

    ~cpp_connection() final override;

    std::size_t hash() const final override;
    const object * sender() const final override;
    const void * source() const final override;
    object * receiver() const final override;
    std::any invoke(const std::vector<std::any> & vargs) final override;

    template<typename RetT, typename ... ArgT>
    RetT call(const ArgT && ... args)
    { return std::any_cast<RetT>(std::forward(invoke(to_any(args...)))); }

    template<typename ... ArgT>
    void call(const ArgT && ... args)
    { invoke(std::forward(invoke(to_any(args...)))); }

    template<typename RecT, typename RetT, typename ...ArgT>
    static
    connection::ptr make(const object * sender, const void * src, RecT* receiver, RetT(RecT::*dst)(ArgT...))
    { return std::shared_ptr<cpp_connection>(new cpp_connection(sender, src, receiver, new method_invoker<RecT, RetT, ArgT...>(receiver, dst))); }

    template<typename RecT, typename ...ArgT>
    static
    connection::ptr make(const object * sender, const void * src, RecT* receiver, void(RecT::*dst)(ArgT...))
    { return std::shared_ptr<cpp_connection>(new cpp_connection(sender, src, receiver, new method_invoker_void<RecT, ArgT...>(receiver, dst))); }

    template<typename RetT, typename ... ArgT>
    static
    connection::ptr make(const object * sender, const void * src, object* receiver, const std::function<RetT(ArgT...)> & dst)
    { return std::shared_ptr<cpp_connection>(new cpp_connection(sender, src, receiver, new lambda_invoker<std::function<RetT(ArgT...)>>(dst))); }

    template<typename ... ArgT>
    static
    connection::ptr make(const object * sender, const void * src, object* receiver, const std::function<void(ArgT...)> & dst)
    { return std::shared_ptr<cpp_connection>(new cpp_connection(sender, src, receiver, new lambda_invoker_void<std::function<void(ArgT...)>>(dst))); }

private:
    cpp_connection(const object * sender, const void * source, object * receiver, invoker * invoker);
};

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_CONNECTION_H
