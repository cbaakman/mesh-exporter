/* Copyright (C) 2018 Coos Baakman
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#ifndef ITER_H
#define ITER_H

#include <iterator>
#include <set>
#include <unordered_map>


namespace XMLMesh
{
    template<typename T>
    class ConstArrayIterable
    {
        private:
            const T *array;
            size_t length;
        public:
            ConstArrayIterable(const T *_array, const size_t _length)
            : array(_array), length(_length) {}
            ConstArrayIterable(const ConstArrayIterable &other)
            : array(other.array), length(other.length) {}

            const T *begin(void) const { return array; }
            const T *end(void) const { return array + length; }
    };

    template <typename T>
    class ConstSetIterator
    {
        private:
            typename std::set<T *>::const_iterator it;
        public:
            ConstSetIterator(typename std::set<T *>::const_iterator _it)
            : it(_it) {}
            ConstSetIterator(const ConstSetIterator<T> &other)
            : it(other.it) {}

            const T *operator*(void) const { return *it; }

            ConstSetIterator<T> &operator++(void) { it++; return *this; }

            bool operator==(const ConstSetIterator<T> &other) const { return it == other.it; }
            bool operator!=(const ConstSetIterator<T> &other) const { return it != other.it; }
    };

    template<typename T>
    class ConstSetIterable
    {
        private:
            ConstSetIterator<T> mBegin, mEnd;
        public:
            ConstSetIterable(const std::set<T *> &s)
            : mBegin(s.cbegin()), mEnd(s.cend()) {}

            ConstSetIterator<T> begin(void) { return mBegin; }
            ConstSetIterator<T> end(void) { return mEnd; }
    };

    template <typename V>
    class ConstMapValueIterator
    {
        private:
            typename std::unordered_map<std::string, V *>::const_iterator it;
        public:
            ConstMapValueIterator(typename std::unordered_map<std::string, V *>::const_iterator _it)
            : it(_it) {}
            ConstMapValueIterator(const ConstMapValueIterator<V> &other)
            : it(other.it) {}

            const V *operator*(void) const { return std::get<1>(*it); }

            ConstMapValueIterator<V> &operator++(void) { it++; return *this; }

            bool operator==(const ConstMapValueIterator<V> &other) const { return it == other.it; }
            bool operator!=(const ConstMapValueIterator<V> &other) const { return it != other.it; }
    };

    template<typename V>
    class ConstMapValueIterable
    {
        private:
            ConstMapValueIterator<V> mBegin, mEnd;
        public:
            ConstMapValueIterable(const std::unordered_map<std::string, V *> &m)
            : mBegin(m.cbegin()), mEnd(m.cend()) {}

            ConstMapValueIterator<V> begin(void) { return mBegin; }
            ConstMapValueIterator<V> end(void) { return mEnd; }
    };

    template <typename V>
    class MapValueIterator
    {
        private:
            typename std::unordered_map<std::string, V *>::const_iterator it;
        public:
            MapValueIterator(typename std::unordered_map<std::string, V *>::const_iterator _it)
            : it(_it) {}
            MapValueIterator(const MapValueIterator<V> &other)
            : it(other.it) {}

            V *operator*(void) { return std::get<1>(*it); }

            MapValueIterator<V> &operator++(void) { it++; return *this; }

            bool operator==(const MapValueIterator<V> &other) const { return it == other.it; }
            bool operator!=(const MapValueIterator<V> &other) const { return it != other.it; }
    };

    template<typename V>
    class MapValueIterable
    {
        private:
            MapValueIterator<V> mBegin, mEnd;
        public:
            MapValueIterable(const std::unordered_map<std::string, V *> &m)
            : mBegin(m.cbegin()), mEnd(m.cend()) {}

            MapValueIterator<V> begin(void) { return mBegin; }
            MapValueIterator<V> end(void) { return mEnd; }
    };
}

#endif  // ITER_H
