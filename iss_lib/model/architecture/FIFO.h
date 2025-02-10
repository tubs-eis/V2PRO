/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
/**
 * @file FIFO.h
 * @author Alexander Köhne
 * @date 2021
 * @brief Stack like Storage for Data chain operations between different vector lanes
 */

#ifndef FIFO_HELPER_HEADER
#define FIFO_HELPER_HEADER

#include <deque>
#include <iostream>

#include <tuple>

template <class T>
class FIFO {
   public:
    FIFO() = delete;
    FIFO(unsigned int maxSize) : maxSize(maxSize) {}

    size_t _size() const {
        return q.size();
    }

    bool _push(T elem) {
        if (q.size() < maxSize) {
            q.push_back(elem);
            return true;
        }
        return false;
    }

    void _pop() {
        q.pop_front();
    }

    T _top() const {
        return q.at(0);
    }

    virtual bool _is_fillable() const {
        return q.size() < maxSize;
    }

    virtual bool _is_empty() const {
        return q.size() == 0;
    }

    template <class V>
    void _list(V& exe) const {
        if (q.size() == 0) {
            printf("NONE");
            return;
        }
        for (auto a : q) {
            exe(a);
        }
    }

   private:
    unsigned int maxSize;
    std::deque<T> q;
};

#endif  //FIFO_HELPER_HEADER
