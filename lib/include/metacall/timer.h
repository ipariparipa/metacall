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

#ifndef METACALL_TIMER_H
#define METACALL_TIMER_H

#include "application.h"

#include <memory>

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

class timer : public object
{
    struct _private;
    std::unique_ptr<_private> p;
public:
    timer();
    timer(const std::shared_ptr<application> & app);
    virtual ~timer() override;

    virtual void destroy_object() override;

    void start(std::chrono::steady_clock::duration timeout, size_t num = std::numeric_limits<size_t>::max());
    void stop();

    //signals
    void trigger();
    void stopped();

    bool active() const;
};

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_TIMER_H
