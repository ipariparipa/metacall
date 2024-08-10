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

#ifndef METACALL_COMMON_H
#define METACALL_COMMON_H

#define METACALL_COPY_PROTECTOR(class_name) class_name(const class_name &) = delete; class_name & operator = (const class_name &) = delete;

#ifdef METACALL_BASE_NAMESPACE
#  define METACALL_BASE_NAMESPACE_BEGIN namespace METACALL_BASE_NAMESPACE {
#  define METACALL_BASE_NAMESPACE_END }
#  define METACALL_NS METACALL_BASE_NAMESPACE::metacall
#else
#  define METACALL_BASE_NAMESPACE_BEGIN
#  define METACALL_BASE_NAMESPACE_END
#  define METACALL_NS metacall
#endif

METACALL_BASE_NAMESPACE_BEGIN namespace metacall {

template<typename ... ArgT>
struct overload_t
{
    template<typename ObjT, typename RetT>
    constexpr auto operator () (RetT(ObjT::*ptr)(ArgT...)) const noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename ... ArgT>
constexpr overload_t<ArgT...> overload = { };

} METACALL_BASE_NAMESPACE_END

#endif // METACALL_COMMON_H
