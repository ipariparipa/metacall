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

#include "include/metacall/connection.h"

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

struct cpp_connection::_private
{
    _private(const object * sender, const void * source, object * receiver, invoker * invoker)
        : sender(sender)
        , source(source)
        , receiver(receiver)
        , invoker(invoker)
    { }

    const object * sender;
    const void * source;
    object * receiver;

    std::unique_ptr<cpp_connection::invoker> invoker;
};

cpp_connection::~cpp_connection() = default;

cpp_connection::cpp_connection(const object * sender, const void * source, object * receiver, invoker * invoker)
    : p(new _private(sender, source, receiver, invoker))
{ }

std::size_t cpp_connection::hash() const
{
#define _hash_combine(h1, h2) ((h1) ^ ((h2) << 1))
    return _hash_combine(std::hash<const void*>{}(p->sender),
                         _hash_combine(std::hash<const void*>{}(p->source),
                                       _hash_combine(std::hash<const void*>{}(p->receiver), std::hash<const void*>{}(p->invoker.get()))));
#undef _hash_combine
}

const object * cpp_connection::sender() const
{ return p->sender; }

const void * cpp_connection::source() const
{ return p->source; }

object * cpp_connection::receiver() const
{ return p->receiver; }

std::any cpp_connection::invoke(const std::vector<std::any> & vargs)
{ return p->invoker->invoke(vargs); }

} METACALL_BASE_NAMESPACE_END
