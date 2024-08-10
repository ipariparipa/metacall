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

#include "include/metacall/object.h"
#include "include/metacall/application.h"

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

struct object::_private
{
    _private(const std::shared_ptr<application> & app)
        : app(app)
    { }

    const std::shared_ptr<application> app;
    std::recursive_mutex mut;
    bool enabled = true;
    std::thread::id threadId;
};

object::object() : p(new _private(application::get()))
{
    move_to_thread();
}

object::object(const std::shared_ptr<application> & app) : p(new _private(app))
{
    move_to_thread();
}

object::~object()
{
    if(p->enabled)
        p->app->disconnect(this);
}

const std::shared_ptr<application> &object::app() const
{ return p->app; }

//virtual
void object::destroy_object()
{
    app()->call(application::calling_rule::queued, this, this, std::function([this](){ delete this; }));
}

std::thread::id object::object_thread_id() const
{
    std::unique_lock __locker(p->mut);
    return p->threadId;
}

void object::move_to_thread()
{ move_to_thread(std::this_thread::get_id()); }

void object::move_to_thread(std::thread::id id)
{
    std::unique_lock __locker(p->mut);
    p->threadId = id;
}

void object::_disable_object()
{
    std::unique_lock __locker(p->mut);
    p->enabled = false;
}

} METACALL_BASE_NAMESPACE_END
