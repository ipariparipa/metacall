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

#include "include/metacall/timer.h"

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

struct timer::_private : public object
{
    _private(timer * that) : that(that)
    { }

    void triggered()
    {
        that->trigger();
        assert(remaining > 0);
        if(--remaining > 0)
            that->app()->call(last += timeout, application::calling_rule::priority,
                              conn = application::make_connection(that, this, &timer::_private::triggered));
        else
            that->stopped();
    }

    timer * that;
    std::chrono::steady_clock::duration timeout;
    std::chrono::steady_clock::time_point last;
    size_t remaining = 0;
    connection::ptr conn;
};

timer::timer()
    : object()
    , p(new _private(this))
{ }

timer::timer(const std::shared_ptr<application> & app)
    : object(app)
    , p(new _private(this))
{ }

timer::~timer() = default;

//virtual
void timer::destroy_object() //override
{
    stop();
    object::destroy_object();
}

void timer::start(std::chrono::steady_clock::duration timeout, size_t num)
{
    p->timeout = timeout;
    if((p->remaining = num) > 0)
        app()->call(p->last = std::chrono::steady_clock::now() + timeout, application::calling_rule::priority,
                    p->conn = application::make_connection(this, p.get(), &timer::_private::triggered));
}

void timer::stop()
{
    if(p->conn)
    {
        app()->disconnect(p->conn);
        p->conn.reset();
    }
    p->remaining = 0;
    stopped();
}

//signals
void timer::trigger()
{ app()->emit(this, &timer::trigger); }

void timer::stopped()
{ app()->emit(this, &timer::stopped); }

bool timer::active() const
{ return p->remaining > 0; }

} METACALL_BASE_NAMESPACE_END
