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
//
// Created by thieu on 23.03.22.
//

#ifndef TEMPLATE_NONBLOCKINGBUSSLAVEINTERFACE_H
#define TEMPLATE_NONBLOCKINGBUSSLAVEINTERFACE_H

#include <stdint.h>
#include <bitset>
#include <iostream>

class NonBlockingBusSlaveInterface {
   public:
    const static int dataword_length_byte = 512 / 8;
    const static int dma_length_byte = 16 / 8;

    // uint8_t* byte_addr
    virtual bool requestReadTransfer(
        intptr_t dst_addr_ptr, const uint32_t burst_length, const uint32_t initiator_id) = 0;

    virtual bool isReadDataAvailable(const uint32_t initiator_id) = 0;

    virtual bool readData(uint8_t* data_ptr, const uint32_t initiator_id) = 0;

    virtual bool requestWriteTransfer(intptr_t dst_addr_ptr,
        const uint8_t* data_ptr,
        const uint32_t burst_length,
        const uint32_t initiator_id) = 0;

    virtual bool isWriteDataReady(const uint32_t initiator_id) = 0;

    virtual void dbgWrite(intptr_t dst_addr, uint8_t* data_ptr) = 0;

    virtual void dbgRead(intptr_t dst_addr, uint8_t* data_ptr) = 0;
};

#endif  //TEMPLATE_NONBLOCKINGBUSSLAVEINTERFACE_H
