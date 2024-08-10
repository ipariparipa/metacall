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

#include "include/metacall/application.h"

#include "notifier.h"

#include <map>
#include <mutex>
#include <set>
#include <tuple>
#include <optional>

#ifdef _METACALL_DEBUG_APP
# include "tmp/debug_app.h"
#else
# define METACALL_DEBUG_PRINT(...) //NOOP
# define METACALL_DEBUG_PRINT_CALLS(...) //NOOP
# define METACALL_DEBUG_PRINT_CONNS(...) //NOOP
#endif

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

using namespace _internal;

namespace {
    constexpr std::chrono::steady_clock::time_point very_first_tp { std::chrono::steady_clock::duration::min() };

    template< typename ContainerT, typename PredicateT >
    void erase_if(ContainerT & cont, const PredicateT & predicate)
    {
        for(auto it = cont.begin(); it != cont.end(); )
            if(predicate(*it))
                it = cont.erase(it);
            else
                ++it;
    }
}

struct application::_private
{
    std::recursive_mutex connections_mut;
    std::map<const object *, std::map<const void *, std::map<const object *, std::set<connection::ptr>>>> connections;

    std::recursive_mutex calls_mut;
    std::map<std::chrono::steady_clock::time_point, std::list<std::tuple<std::set<connection::ptr>, std::vector<std::any> /*args*/>>> calls;
    std::optional<int> exit_code;
    notifier call_notif;

    bool in_deletion = false;
};

//static
const std::shared_ptr<application> & application::get()
{
    static auto o = object::make<application>();
    return o;
}

application::application()
    : object(std::shared_ptr<application>(this, [](application*){}))
    , p(new _private)
{ }

application::~application()
{
    _disable_object();
    this_app = nullptr;
}

//virtual
void application::destroy_object() //override
{
    delete this;
}

void application::disconnect(const object * o)
{
    std::scoped_lock __locker(p->connections_mut, p->calls_mut);
    if(p->in_deletion)
        return;

    if(auto it = p->connections.find(o); it != p->connections.end())
        p->connections.erase(it);

    erase_if(p->connections, [o](auto & sources){
        auto & receivers = sources.second;
        erase_if(receivers, [o](auto & receiver) {
            auto & conns = receiver.second;
            if(auto it = conns.find(o); it != conns.end())
                conns.erase(it);

            return !conns.size();
        });

        return !receivers.size();
    });

    erase_if(p->calls, [o](auto & items) {
        auto & calls = items.second;

        erase_if(calls, [o](auto & call) {
            auto & conns = std::get<0>(call);

            erase_if(conns, [o](const auto & c) {
                return c->sender() == o || c->receiver() == o;
            });

            return !conns.size();
        });

        return !calls.size();
    });

    METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);
    METACALL_DEBUG_PRINT_CONNS(__FUNCTION__, p->connections);
}

void application::disconnect(const connection::ptr & c)
{
    std::scoped_lock __locker(p->connections_mut, p->calls_mut);

    if(auto it = p->connections.find(c->sender()); it != p->connections.end())
        p->connections.erase(it);

    erase_if(p->connections, [c](auto & sender){
        auto & sources = sender.second;
        erase_if(sources, [c](auto & source) {
            auto & receivers = source.second;
            erase_if(receivers, [c](auto & receiver) {
                auto & connections = receiver.second;
                if(auto it = connections.find(c); it != connections.end())
                    connections.erase(it);

                return !connections.size();
            });

            return !receivers.size();
        });

        return !sources.size();
    });

    erase_if(p->calls, [&](auto & items) {
        auto & calls = items.second;

        erase_if(calls, [&](auto & call) {
            auto & conns = std::get<0>(call);

            if(auto it = conns.find(c); it != conns.end())
                conns.erase(it);

            return !conns.size();
        });

        if(calls.size())
            return false;

        if(items.first > very_first_tp)
            undefine_timer(&items.second, items.first);

        return true;
    });

    METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);
    METACALL_DEBUG_PRINT_CONNS(__FUNCTION__, p->connections);
}

void application::add_to_queue(application::calling_rule rule, const std::list<connection::ptr> & conns, const std::vector<std::any> & args)
{ add_to_queue(very_first_tp, rule, conns, args); }

void application::add_to_queue(std::chrono::steady_clock::time_point time, calling_rule rule, const std::list<connection::ptr> &conns, const std::vector<std::any> & args)
{
    std::unique_lock __locker(p->calls_mut);

    switch(rule)
    {
    case calling_rule::automatic: assert(rule != calling_rule::automatic); return;
    case calling_rule::direct: assert(rule != calling_rule::direct); return;
    case calling_rule::priority:
        p->calls[time].push_front(std::make_tuple(std::set<connection::ptr>(conns.begin(), conns.end()), args));
        break;
    case calling_rule::queued:
        p->calls[time].push_back(std::make_tuple(std::set<connection::ptr>(conns.begin(), conns.end()), args));
        break;
    }
    METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);

    if(time > std::chrono::steady_clock::now())
        define_timer(&p->calls[time], time, [this](){ run(); });

    call_notification();
}

//virtual
void application::call_notification()
{
    p->call_notif.notify();
}

//virtual
void application::call_exit(int c)
{
    (void)c;
    p->call_notif.notify();
}

bool application::activate()
{
    std::unique_lock __locker(p->calls_mut);

    METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);

    if(!p->calls.size())
        return false;

    if(auto front = p->calls.begin(); front != p->calls.end() && front->first <= std::chrono::steady_clock::now())
    {
        auto call = front->second.front();
        front->second.pop_front();

        if(!front->second.size())
            p->calls.erase(front);

        for(auto & c : std::get<0>(call))
            c->invoke(std::get<1>(call));

        return p->calls.size();
    }

    return false;
}

void application::run()
{
    while(activate());
}

void application::lock()
{ p->connections_mut.lock(); }

void application::unlock()
{ p->connections_mut.unlock(); }

std::any application::call(const connection::ptr & conn, const std::vector<std::any> & args)
{ return conn->invoke(std::move(args)); }

void application::call(calling_rule rule, const connection::ptr & conn, const std::vector<std::any> & args)
{
    switch(rule)
    {
    case calling_rule::automatic:
        if(!conn->receiver() || conn->receiver()->object_thread_id() != std::this_thread::get_id())
        {
            add_to_queue(calling_rule::priority, { conn }, std::move(args));
            break;
        }
        [[fallthrough]];
    case calling_rule::direct:
        conn->invoke(std::move(args));
        break;
    case calling_rule::priority:
    case calling_rule::queued:
        add_to_queue(rule, { conn }, std::move(args));
        break;
    }
}

void application::call(std::chrono::steady_clock::time_point time, calling_rule rule, const connection::ptr & conn, const std::vector<std::any> & args)
{
    switch(rule)
    {
    case calling_rule::automatic:
        if(!conn->receiver() || conn->receiver()->object_thread_id() != std::this_thread::get_id())
        {
            add_to_queue(time, calling_rule::priority, { conn }, std::move(args));
            break;
        }
        [[fallthrough]];
    case calling_rule::direct:
        if(time <= std::chrono::steady_clock::now())
        {
            conn->invoke(std::move(args));
            break;
        }
        rule = calling_rule::priority;
        [[fallthrough]];
    case calling_rule::priority:
    case calling_rule::queued:
        add_to_queue(time, rule, { conn }, std::move(args));
        break;
    }
}

std::any application::_emit(const std::list<connection::ptr> & conns, const std::vector<std::any> & args)
{
    std::any ret;
    for(auto & c : conns)
        ret = c->invoke(args);

    return ret;
}

void application::_emit(calling_rule rule, const std::list<connection::ptr> & conns, const std::vector<std::any> & args)
{
    auto this_thread_id = std::this_thread::get_id();

    std::unique_lock __locker(*this);
    switch(rule)
    {
    case calling_rule::automatic:
    {
        std::list<connection::ptr> to_queue;
        for(auto & c : conns)
            if(c->receiver() && c->receiver()->object_thread_id() == this_thread_id)
                c->invoke(args);
            else
                to_queue.push_back(c);

        if(to_queue.size())
            add_to_queue(calling_rule::priority, to_queue, args);
    }
        break;
    case calling_rule::direct:
        for(auto & c : conns)
            c->invoke(args);
        break;
    case calling_rule::priority:
    case calling_rule::queued:
        add_to_queue(rule, conns, args);
        break;
    }
}

void application::_add_connection(const object * sender, const void * source, object * receiver, const connection::ptr & conn)
{
    std::unique_lock __locker(*this);
    p->connections[sender][source][receiver].insert(conn);
    METACALL_DEBUG_PRINT_CONNS(__FUNCTION__, p->connections);
}

std::list<connection::ptr> application::_get_connections(const object * sender, const void *source)
{
    std::list<connection::ptr> ret;

    if(auto it1 = p->connections.find(sender); it1 != p->connections.end())
        if(auto it2 = it1->second.find(source); it2 != it1->second.end())
            for(const auto & rec : it2->second)
                for(const auto & conn : rec.second)
                    ret.push_back(conn);

    return ret;
}

int application::exec()
{
    move_to_thread();
    p->exit_code.reset();
    while(!p->exit_code)
        if(p->calls.size())
        {
            {
                std::unique_lock __locker(p->calls_mut);
                METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);
                if(auto front = p->calls.begin(); front != p->calls.end() && front->first <= std::chrono::steady_clock::now())
                {
                    auto call = front->second.front();
                    front->second.pop_front();

                    if(!front->second.size())
                        p->calls.erase(front);

                    METACALL_DEBUG_PRINT_CALLS(__FUNCTION__, p->calls);

                    for(auto & c : std::get<0>(call))
                    {
                        METACALL_DEBUG_PRINT(__FUNCTION__, c);
                         __locker.unlock();
                        c->invoke(std::get<1>(call));
                    }

                    //`front` can be already invalid here
                }
            }

            {
                std::unique_lock __locker(p->calls_mut);
                if(auto front = p->calls.begin(); front != p->calls.end())
                {
                    if(front->first > std::chrono::steady_clock::now())
                    {
                        auto until = front->first;
                        __locker.unlock();
                        p->call_notif.wait(until);
                    }
                }
                else
                    goto WAIT;
            }
        }
        else
        { WAIT:
            p->call_notif.wait();
        }

    assert(p->exit_code);
    return *p->exit_code;
}

int application::final()
{
    run();
    return p->exit_code ? *p->exit_code : 0;
}

void application::exit(int c)
{
    std::unique_lock _locker(p->calls_mut);
    p->exit_code = c;
    call_exit(c);
}

} METACALL_BASE_NAMESPACE_END
