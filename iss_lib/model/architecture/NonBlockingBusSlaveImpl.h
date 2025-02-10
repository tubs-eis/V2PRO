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
#pragma once
#include <stdint.h>
#include <cstdint>
#include <functional>
#include <iostream>
#include "../../simulator/helper/debugHelper.h"
#include "NonBlockingBusSlaveInterface.h"

using dbg_write_callback_function = std::function<void(intptr_t, uint8_t*)>;
using dbg_read_callback_function = std::function<void(intptr_t, uint8_t*)>;

using request_read_transfer_callback_function =
    std::function<bool(intptr_t, const uint32_t, const uint32_t)>;
using request_write_transfer_callback_function =
    std::function<bool(intptr_t, const uint8_t*, const uint32_t, const uint32_t)>;

using is_read_data_available_callback_function = std::function<bool(const uint32_t)>;
using is_write_data_ready_callback_function = std::function<bool(const uint32_t)>;

using read_data_callback_function = std::function<bool(uint8_t*, const uint32_t)>;

class NonBlockingBusSlaveImpl : public NonBlockingBusSlaveInterface {
   public:
    // Member
    read_data_callback_function m_read_data_callback;

    is_read_data_available_callback_function m_is_read_data_available_callback;
    is_write_data_ready_callback_function m_is_write_data_ready_callback;

    request_read_transfer_callback_function m_request_read_transfer_callback;
    request_write_transfer_callback_function m_request_write_transfer_callback;

    dbg_write_callback_function m_dbg_write_callback;
    dbg_read_callback_function m_dbg_read_callback;

    NonBlockingBusSlaveImpl();

    // Register
    void register_read_data_cb(read_data_callback_function cb);
    bool readData(uint8_t* data_ptr, const uint32_t initiator_id);

    // Register
    void register_is_read_data_available_cb(is_read_data_available_callback_function cb);
    void register_is_write_data_ready_cb(is_write_data_ready_callback_function cb);

    // Impl
    bool isReadDataAvailable(const uint32_t initiator_id);
    bool isWriteDataReady(const uint32_t initiator_id);

    // Register
    void register_request_read_transfer_cb(request_read_transfer_callback_function cb);
    void register_request_write_transfer_cb(request_write_transfer_callback_function cb);

    // Impl
    bool requestReadTransfer(
        intptr_t dst_addr_ptr, const uint32_t burst_length, const uint32_t initiator_id);

    bool requestWriteTransfer(intptr_t dst_addr_ptr,
        const uint8_t* data_ptr,
        const uint32_t burst_length,
        const uint32_t initiator_id);

    // DEBUG API

    // Register
    void register_dbg_write_cb(dbg_write_callback_function cb);
    void register_dbg_read_cb(dbg_read_callback_function cb);

    // Impl
    void dbgWrite(intptr_t dst_addr, uint8_t* data_ptr);
    void dbgRead(intptr_t dst_addr, uint8_t* data_ptr);
};
