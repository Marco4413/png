#pragma once

#ifndef _PNG_UTILS_H
#define _PNG_UTILS_H

#include "png/base.h"

#include <iterator> // random_access_iterator_tag

namespace PNG
{
    namespace Utils
    {
        template<typename IotaT>
        class Iota
        {
        public:
            // A random access Iota iterator that is inteded to be used with std::for_each and std::execution::par/_unseq
            class Iterator
            {
            public:
                using iterator_category = std::random_access_iterator_tag;
                using value_type        = IotaT;
                using difference_type   = IotaT;
                using pointer           = IotaT*;
                using reference         = IotaT;
                using iterator          = Iterator;

                Iterator()
                    : m_Value(0) { }

                Iterator(IotaT value)
                    : m_Value(value) { }

                Iterator operator++(int)
                {
                    Iterator prevIt = *this;
                    ++(*this);
                    return prevIt;
                }

                Iterator operator--(int)
                {
                    Iterator prevIt = *this;
                    --(*this);
                    return prevIt;
                }

                Iterator& operator++() { m_Value++; return *this; }
                Iterator& operator--() { m_Value--; return *this; }

                Iterator& operator+=(IotaT n) { m_Value += n; return *this; }

                Iterator operator+(IotaT n) const { return Iterator(m_Value + n); }
                friend Iterator operator+(IotaT n, Iterator it) { return it + n; }

                Iterator& operator-=(IotaT n) { m_Value -= n; return *this; }

                Iterator operator-(IotaT n) const { return Iterator(m_Value - n); }
                friend Iterator operator-(IotaT n, Iterator it) { return it - n; }

                IotaT operator-(Iterator other) const { return m_Value - other.m_Value; }

                IotaT operator[](IotaT n) const { return *(*this + n); }

                bool operator==(iterator other) const { return m_Value == other.m_Value; }
                bool operator!=(iterator other) const { return !(*this == other); }

                bool operator<(Iterator other) const { return m_Value < other.m_Value; }
                bool operator<=(Iterator other) const { return m_Value <= other.m_Value; }
                bool operator>(Iterator other) const { return m_Value > other.m_Value; }
                bool operator>=(Iterator other) const { return m_Value >= other.m_Value; }

                IotaT operator*() const { return m_Value; }
            private:
                IotaT m_Value;
            };

        public:
            const IotaT From;
            const IotaT To;

        public:
            constexpr Iota(IotaT from, IotaT to)
                : From(to > from ? from : to), To(to > from ? to : from) { }

            constexpr Iota(IotaT to)
                : Iota(0, to) { }
            
            Iterator begin() const { return Iterator(From); }
            Iterator end() const { return Iterator(To); }
        };
    }
}

#endif // _PNG_UTILS_H
