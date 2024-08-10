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

#ifndef METACALL_NOTIFIER_H
#define METACALL_NOTIFIER_H

#include "include/metacall/common.h"

#include <chrono>
#include <memory>

METACALL_BASE_NAMESPACE_BEGIN namespace metacall { namespace _internal {

class notifier
{
    METACALL_COPY_PROTECTOR(notifier)
    struct _private;
    std::unique_ptr<_private> p;
public:
    notifier();
    ~notifier();

    void notify();

    void notify_all();

    void wait();

    inline bool wait(long milliseconds)
    { return wait(std::chrono::milliseconds(milliseconds)); }

    bool wait(std::chrono::milliseconds timeout_ms);

    bool wait(std::chrono::time_point<std::chrono::system_clock> time);

    bool wait(std::chrono::time_point<std::chrono::steady_clock> time);

    bool try_wait();
};

}} METACALL_BASE_NAMESPACE_END

#endif // METACALL_NOTIFIER_H
