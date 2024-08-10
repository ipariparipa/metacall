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

#ifndef METACALL_OBJECT_H
#define METACALL_OBJECT_H

#include "common.h"

#include <thread>
#include <memory>

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

class application;
class connection;

class object
{
    METACALL_COPY_PROTECTOR(object);

    struct _private;
    std::unique_ptr<_private> p;
public:
    object();
    object(const std::shared_ptr<application> & app);
    virtual ~object();

    const std::shared_ptr<application> & app() const;

    virtual void destroy_object();

    template<typename T, typename ... ArgT>
    static
    std::shared_ptr<T> make(ArgT && ... args)
    {
        return std::shared_ptr<T>(new T(std::move(args)...), [](T * o) { o->destroy_object(); });
    }

    std::thread::id object_thread_id() const;

    void move_to_thread();
    void move_to_thread(std::thread::id);

private:
    friend class application;
    void _disable_object(); //called by `metacall::pplication` only
};

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_OBJECT_H
