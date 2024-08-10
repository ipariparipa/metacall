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

#include "notifier.h"

#include <condition_variable>
#include <atomic>

METACALL_BASE_NAMESPACE_BEGIN namespace metacall { namespace _internal {

struct notifier::_private
{
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic_ulong count = 0;
};

notifier::notifier() : p(new _private)
{}

notifier::~notifier() = default;

void notifier::notify()
{
    std::unique_lock<std::mutex> lock(p->mutex);
    p->condition.notify_one();
    ++p->count;
}

void notifier::notify_all()
{
    std::unique_lock<std::mutex> lock(p->mutex);
    p->condition.notify_all();
    ++p->count;
}

void notifier::wait()
{
    std::unique_lock<std::mutex> lock(p->mutex);
    while (!p->count)
         p->condition.wait(lock);

    --p->count;
}

bool notifier::wait(std::chrono::milliseconds msecs)
{
    std::unique_lock<std::mutex> lock(p->mutex);
    while (!p->count)
         if (p->condition.wait_for(lock, msecs) != std::cv_status::no_timeout)
            return false;

    --p->count;
    return true;
}

bool notifier::wait(std::chrono::time_point<std::chrono::system_clock> time)
{
    std::unique_lock<std::mutex> lock(p->mutex);
    while (!p->count)
         if (p->condition.wait_until(lock, time) != std::cv_status::no_timeout)
             return false;

    --p->count;
    return true;
}

bool notifier::wait(std::chrono::time_point<std::chrono::steady_clock> time)
{
    std::unique_lock<std::mutex> lock(p->mutex);
    while (!p->count)
         if (p->condition.wait_until(lock, time) != std::cv_status::no_timeout)
             return false;

    --p->count;
    return true;
}

bool notifier::try_wait()
{
    std::unique_lock<std::mutex> lock(p->mutex);
    if (p->count)
    {
         --p->count;
         return true;
    }
    return false;
}

}} METACALL_BASE_NAMESPACE_END
