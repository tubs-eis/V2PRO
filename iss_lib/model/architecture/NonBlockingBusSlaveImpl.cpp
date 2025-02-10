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

#include "NonBlockingBusSlaveImpl.h"

NonBlockingBusSlaveImpl::NonBlockingBusSlaveImpl() {}

// Register
void NonBlockingBusSlaveImpl::register_read_data_cb(read_data_callback_function cb) {
    m_read_data_callback = cb;
}
// Impl
bool NonBlockingBusSlaveImpl::readData(uint8_t* data_ptr, const uint32_t initiator_id) {
    return m_read_data_callback(data_ptr, initiator_id);
}

// Register
void NonBlockingBusSlaveImpl::register_is_read_data_available_cb(
    is_read_data_available_callback_function cb) {
    m_is_read_data_available_callback = cb;
}

void NonBlockingBusSlaveImpl::register_is_write_data_ready_cb(
    is_write_data_ready_callback_function cb) {
    m_is_write_data_ready_callback = cb;
}

// Impl
bool NonBlockingBusSlaveImpl::isReadDataAvailable(const uint32_t initiator_id) {
    return m_is_read_data_available_callback(initiator_id);
}
bool NonBlockingBusSlaveImpl::isWriteDataReady(const uint32_t initiator_id) {
    return m_is_write_data_ready_callback(initiator_id);
}

// Register
void NonBlockingBusSlaveImpl::register_request_read_transfer_cb(
    request_read_transfer_callback_function cb) {
    m_request_read_transfer_callback = cb;
}
void NonBlockingBusSlaveImpl::register_request_write_transfer_cb(
    request_write_transfer_callback_function cb) {
    m_request_write_transfer_callback = cb;
}

// Impl
bool NonBlockingBusSlaveImpl::requestReadTransfer(
    intptr_t dst_addr_ptr, const uint32_t burst_length, const uint32_t initiator_id) {
    return m_request_read_transfer_callback(dst_addr_ptr, burst_length, initiator_id);
}
bool NonBlockingBusSlaveImpl::requestWriteTransfer(intptr_t dst_addr_ptr,
    const uint8_t* data_ptr,
    const uint32_t burst_length,
    const uint32_t initiator_id) {
    return m_request_write_transfer_callback(dst_addr_ptr, data_ptr, burst_length, initiator_id);
}

// DEBUG API

// Register
void NonBlockingBusSlaveImpl::register_dbg_write_cb(dbg_write_callback_function cb) {
    m_dbg_write_callback = cb;
}
void NonBlockingBusSlaveImpl::register_dbg_read_cb(dbg_read_callback_function cb) {
    m_dbg_read_callback = cb;
}
// Impl
void NonBlockingBusSlaveImpl::dbgWrite(intptr_t dst_addr, uint8_t* data_ptr) {
    m_dbg_write_callback(dst_addr, data_ptr);
}
void NonBlockingBusSlaveImpl::dbgRead(intptr_t dst_addr, uint8_t* data_ptr) {
    m_dbg_read_callback(dst_addr, data_ptr);
}
