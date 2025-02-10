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
 * @file HFIFO.h
 * @author Alexander Köhne
 * @date 2021
 *
 * FIFO Wrapper to push and pop data asynchronously
 */

#ifndef HFIFO_HEADER_GUARD
#define HFIFO_HEADER_GUARD

#include <string>
#include "FIFO.h"

template <class T>
class HFIFO : public FIFO<T> {
   public:
    HFIFO() = delete;
    HFIFO(int maxSize) : FIFO<T>(maxSize) {}

    bool push(T elem) {
        pushing = true;
        current_push_elem = elem;
        return true;  //VOID ?
    }

    T pop() {
        poping = true;
        return FIFO<T>::_top();
    }

    void update() {
        if (poping) {
            data_last = FIFO<T>::_top();
            FIFO<T>::_pop();
        }
        if (pushing) {
            FIFO<T>::_push(current_push_elem);
        }
        poping_last = poping;
        poping = false;
        pushing = false;
    }

    bool wasRead() const {
        return poping_last;
    }

    T wasData() const {
        return data_last;
    }

    bool if_in() const {
        return pushing;
    }

    bool if_out() const {
        return poping;
    }

    T in() const {
        return current_push_elem;
    }

    T out() const {
        return FIFO<T>::_top();
    }

    template <class V>
    void list(V& exe) const {
        FIFO<T>::_list(exe);
    }

   private:
    T current_push_elem;
    bool pushing = false;
    bool poping = false;
    bool poping_last = false;
    T data_last;
};

#endif
