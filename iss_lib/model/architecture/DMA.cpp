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
// Created by gesper on 14.03.19.
//

#include "DMA.h"
#include "../../simulator/ISS.h"
#include "../../simulator/helper/debugHelper.h"
#include "Cluster.h"
#include "unit/VectorUnit.h"

//## Integration
#include <iostream>

DMA::DMA(Cluster* cluster,
    std::vector<std::shared_ptr<Unit::IVectorUnit>> units,
    double& time,
    std::shared_ptr<ArchitectureState> architecture_state,
    DCMA* dcma)
    : cluster(cluster),
      units(units),
      time(time),
      architecture_state(architecture_state),
      dcma(dcma) {
    noneCmd = std::make_shared<CommandDMA>();
    command = noneCmd;

    cmd_queue = std::list<std::shared_ptr<CommandDMA>>();

    ext_addr_base_lst = -1;
    //    printf_info("SIM: \tInstanziated new DMA\n");

    if (DMA_gen_trace) {
        std::stringstream ss;
        ss << "DMA_" << cluster->cluster_id << ".trace";
        openTraceFile(ss.str().c_str());
        std::stringstream ss2;
        ss2 << "DMA_" << cluster->cluster_id << "_cmd.trace";
        cmd_trace = std::ofstream(ss2.str().c_str());
    }
}

DMA::~DMA() {
    if (DMA_gen_trace) {
        trace.close();
    }
}

std::shared_ptr<CommandDMA> DMA::getCmd() {
    return command;
}

bool DMA::isBusy() {
    return !command->is_done() || !cmd_queue.empty() || dcma->isBusy(DMA::cluster->cluster_id);
}

void DMA::execute_cmd(const std::shared_ptr<CommandDMA>& cmd) {
    cmd->id = id_counter;
    id_counter++;
    cmd_queue.push_back(cmd);

    auto command = cmd;

    if (DMA_gen_trace) {
        uint32_t cycle_stamp = cluster->core->getTime() / cluster->core->getDMAClockPeriod();
        cmd_trace << command->id << "," << cycle_stamp << "," << command->ext_base << ",";
        if (is_read_transfer(*command))
            cmd_trace << "r\n";
        else
            cmd_trace << "w\n";
        cmd_trace.flush();
    }

    if (cmd->x_size <= 0 || cmd->y_size <= 0) {
        cmd->print();
        printf_error(
            "[DMA] Command cannot transfer 0 elements! -> endless loop (hardware). At least one "
            "elements required!\n");
        exit(1);
    }
}

uint32_t DMA::io_read(uint32_t addr) {
    printf_warning("Inside NOT IMPLEMENTED DMA:io_read \n");

    if ((addr & 0xff00) >> 8 == (DMA::cluster->cluster_id)) {
        switch (addr & 0xff) {
            case (0x80):
            case (0x84):
            case (0x88):
            case (0x90):
            case (0x94):
            case (0x98): {
                printf_error("DMA IO Read. Command register are write only!\n");
            }
            case (0xbc): {
                return this->isBusy() ? 1 : 0;
                break;
            }
            case (0xa0):
            case (0xa4):
            case (0xa8):
            case (0xac):
            case (0xb0): {
                printf_error("DMA IO Read. Pad register are write only!\n");
            }
        }  // switch dma reg (0xFFFEyy80..0xFFFEyyFE, yy: 8-bit specifies cluster)
    }      // this cluster addressed
    // NOT YET IMPLEMENTED
    return 0;
}

void DMA::io_write(uint32_t addr, uint32_t value) {
    //std::cout << "DMA IO-WRITE CALLED" << std::endl;
    if ((addr & 0xf00) >> 8 == (DMA::cluster->cluster_id)) {
        switch (addr & 0xff) {
            case (0xbc): {
                printf_error("IO Write to dma busy register which is read only!\n");
                break;
            }
            case (0xa0): {
                architecture_state->dma_pad_top = value;
                break;
            }
            case (0xa4): {
                architecture_state->dma_pad_bottom = value;
                break;
            }
            case (0xa8): {
                architecture_state->dma_pad_left = value;
                break;
            }
            case (0xac): {
                architecture_state->dma_pad_right = value;
                break;
            }
            case (0xb0): {
                architecture_state->dma_pad_value = value;
                break;
            }
            default:
                printf_warning("IO Write to dma but not to a valid dma register!");
                break;
        }
    }
}

void DMA::tick() {
    // check if cmd available
    // execute next iteration of cmd if not (already done and remaining_elements > 0)
    // if padding, write padding value to LM
    // else createDcmaRequest (burst_length, addr) and set remaining_elements to burst_length
    // increment iteration
    //

    if (command->is_done() && !cmd_queue.empty() && !dcma->isBusy(DMA::cluster->cluster_id)) {
        command = cmd_queue.front();
        cmd_queue.pop_front();
        command->done = false;

        Statistics::get().getDMAStat()->addExecutedCommand(command.get(), cluster->cluster_id);

        cur_iteration.x = 0;
        cur_iteration.y = 0;
        cur_iteration.loc_addr = command->loc_base & 0x000fffffu;
        cur_iteration.ext_addr = command->ext_base;
        cur_iteration.remaining_req_elements = 0;
        cur_iteration.ext_addr_index = command->ext_base;
        cur_iteration.total_remaining_elements = command->x_size * command->y_size;

        if (debug & DEBUG_DMA) {
            printf_info("[DMA] new Command: %s\n", command->get_string().toStdString().c_str());
            for (auto u : command->unit) {
                //                command->print();
                printf_info("[DMA C%iU%i] ", cluster->cluster_id, u);
                if (command->type == CommandDMA::EXT_1D_TO_LOC_1D ||
                    command->type == CommandDMA::EXT_2D_TO_LOC_1D) {
                    printf_info("Start E2L (DMA-Load): Ext Addr = 0x%" PRIx64
                                ", LM Addr = 0x%08X [size=%i elements]\n",
                        command->ext_base,
                        (command->loc_base & 0x000fffff),
                        (command->x_size * command->y_size));
                } else if (command->type == CommandDMA::LOC_1D_TO_EXT_1D ||
                           command->type == CommandDMA::LOC_1D_TO_EXT_2D) {
                    printf_info(
                        "Start L2E (DMA-Store): Ext Addr = 0x%08X, LM Addr = 0x%08X [size=%i "
                        "elements]\n",
                        command->ext_base,
                        (command->loc_base & 0x000fffff),
                        (command->x_size * command->y_size));
                }
            }
        }
    }

    // check if a transfer is ongoing
    if (cur_iteration.remaining_req_elements > 0) {
        // poll DCMA if req is finished
        // if read: write data to lm
        uint32_t dataword_counter = 0;
        while (dataword_counter < DCMA_DATA_WIDTH / DMA_DATA_WIDTH &&
               cur_iteration.remaining_req_elements > 0 && !command->done) {
            if (is_read_transfer(*command)) {
                if (dcma->isReadDataAvailable(cluster->cluster_id)) {
                    uint8_t readdata[DMA_DATA_WIDTH / 8];
                    dcma->readData(cluster->cluster_id, &readdata[0]);
                    write_to_LM(cur_iteration.loc_addr, readdata);

                    //debug
                    if (debug & DEBUG_DMA_DETAIL) {
                        for (auto u : command->unit) {
                            printf_info(
                                "[DMA C%iU%i] E2L (Load  %i/%i): Ext Addr = 0x%08X, LM Addr = "
                                "0x%08X, data: %i = 0x%04X \n",
                                cluster->cluster_id,
                                u,
                                command->x_size * command->y_size -
                                    cur_iteration.total_remaining_elements + 1,
                                command->x_size * command->y_size,
                                cur_iteration.ext_addr + cur_iteration.ext_addr_index * 2,
                                cur_iteration.loc_addr,
                                *((int16_t*)readdata),
                                *((int16_t*)readdata));
                        }
                    }

                    cur_iteration.loc_addr++;
                    cur_iteration.remaining_req_elements--;
                    cur_iteration.total_remaining_elements--;
                    cur_iteration.ext_addr_index++;
                    if (cur_iteration.remaining_req_elements == 0 &&
                        cur_iteration.total_remaining_elements == 0) {
                        command->done = true;
                        if (debug & DEBUG_DMA) {
                            for (auto u : command->unit) {
                                printf_info("[DMA C%iU%i] E2L Done\n", cluster->cluster_id, u);
                            }
                        }
                    }
                }

            } else {
                if (dcma->isWriteDataReady(cluster->cluster_id)) {
                    // write a 16bit word to dcma
                    // loc to ext dma commands have only 1 unit
                    auto data = units[command->unit.first()]->getLocalMemoryData(
                        cur_iteration.loc_addr, (DMA_DATA_WIDTH / 8));
                    dcma->writeData(cluster->cluster_id, reinterpret_cast<const uint8_t*>(&data));

                    //debug
                    if (debug & DEBUG_DMA_DETAIL) {
                        for (auto u : command->unit) {
                            printf_info(
                                "[DMA C%iU%i] L2E (Store  %i/%i): Ext Addr = 0x%08X, LM Addr = "
                                "0x%08X, data: %i = 0x%04X \n",
                                cluster->cluster_id,
                                u,
                                command->x_size * command->y_size -
                                    cur_iteration.total_remaining_elements + 1,
                                command->x_size * command->y_size,
                                cur_iteration.ext_addr + cur_iteration.ext_addr_index * 2,
                                cur_iteration.loc_addr,
                                int16_t(data),
                                int16_t(data));
                        }
                    }

                    cur_iteration.loc_addr++;
                    cur_iteration.remaining_req_elements--;
                    cur_iteration.total_remaining_elements--;
                    cur_iteration.ext_addr_index++;
                    if (cur_iteration.remaining_req_elements == 0 &&
                        cur_iteration.total_remaining_elements == 0) {
                        command->done = true;
                        if (debug & DEBUG_DMA) {
                            for (auto u : command->unit) {
                                printf_info("[DMA C%iU%i] L2E Done\n", cluster->cluster_id, u);
                            }
                        }
                    }
                }
            }
            dataword_counter++;
        }
    } else {  // request new block (from dcma)
        if (!command->is_done()) {
            if (is_padding_region(*command, cur_iteration)) {
                // transfer padding value to LM
                write_to_LM(cur_iteration.loc_addr, (uint8_t*)(&architecture_state->dma_pad_value));
                if (debug & DEBUG_DMA_DETAIL) {
                    for (auto u : command->unit) {
                        printf_info(
                            "[DMA C%iU%i] E2L (Load %i/%i): Ext Addr = ----------, LM Addr = "
                            "0x%08X, data: %i = 0x%04X [PAD Region]\n",
                            cluster->cluster_id,
                            u,
                            command->x_size * command->y_size -
                                cur_iteration.total_remaining_elements + 1,
                            command->x_size * command->y_size,
                            cur_iteration.loc_addr,
                            architecture_state->dma_pad_value,
                            architecture_state->dma_pad_value);
                    }
                }
                increment_iteration();
                cur_iteration.loc_addr++;
                cur_iteration.total_remaining_elements--;
                cur_iteration.ext_addr_index++;
                if (cur_iteration.total_remaining_elements == 0) command->done = true;
            } else {
                uint32_t nr_padding_pixels = 0;
                if (command->type == CommandDMA::EXT_2D_TO_LOC_1D) {
                    if (command->pad[CommandDMA::PAD::LEFT])
                        nr_padding_pixels += architecture_state->dma_pad_left;
                    if (command->pad[CommandDMA::PAD::RIGHT])
                        nr_padding_pixels += architecture_state->dma_pad_right;
                }
                auto burst_length = command->x_size - nr_padding_pixels;
                //            dcma->createRequest
                if (is_read_transfer(*command)) {
                    dcma->requestDmaReadTransfer(
                        cur_iteration.ext_addr, burst_length, cluster->cluster_id);
                } else {
                    dcma->requestDmaWriteTransfer(
                        cur_iteration.ext_addr, burst_length, cluster->cluster_id);
                }

                if (DMA_gen_trace) {
                    uint32_t cycle_stamp =
                        cluster->core->getTime() / cluster->core->getDMAClockPeriod();
                    trace << command->id << "," << cycle_stamp << "," << cur_iteration.ext_addr
                          << "," << burst_length * DMA_DATA_WIDTH / 8 << ",";
                    if (is_read_transfer(*command))
                        trace << "r\n";
                    else
                        trace << "w\n";
                    trace.flush();
                }

                increment_iteration(burst_length, false);
                cur_iteration.remaining_req_elements = burst_length;
            }
        }
    }
}

bool DMA::is_read_transfer(const CommandDMA& dma_command) const {
    return (dma_command.type == CommandDMA::EXT_1D_TO_LOC_1D ||
            dma_command.type == CommandDMA::EXT_2D_TO_LOC_1D);
}

bool DMA::is_padding_region(const CommandDMA& dma_command, const Iteration& iteration) const {
    if (dma_command.type == CommandDMA::EXT_2D_TO_LOC_1D) {
        if ((iteration.y < architecture_state->dma_pad_top) &&
                dma_command.pad[CommandDMA::PAD::TOP] ||
            (iteration.y >= dma_command.y_size - architecture_state->dma_pad_bottom) &&
                dma_command.pad[CommandDMA::PAD::BOTTOM] ||
            (iteration.x < architecture_state->dma_pad_left) &&
                dma_command.pad[CommandDMA::PAD::LEFT] ||
            (iteration.x >= dma_command.x_size - architecture_state->dma_pad_right) &&
                dma_command.pad[CommandDMA::PAD::RIGHT]) {
            return true;
        }
    }
    return false;
}

void DMA::increment_iteration(uint32_t burst_length, bool is_padding) {
    cur_iteration.x += burst_length;
    if (!is_padding) {
        cur_iteration.ext_addr += 2 * (burst_length + command->y_leap - 1);
        cur_iteration.ext_addr_index = -(burst_length + command->y_leap - 1);
    }

    if (cur_iteration.x > command->x_size - 1) {
        if (cur_iteration.y != command->y_size - 1) {
            cur_iteration.x = 0;
            cur_iteration.y++;
        }
    }
}

void DMA::write_to_LM(const uint32_t& element_addr, const uint8_t* byte_data) {
    // write to selected local memories
    for (auto u : command->unit) {
        units[u]->writeLocalMemoryData(element_addr, byte_data, 2);
    }
}

void DMA::openTraceFile(const char* tracefilename) {
    trace = std::ofstream(tracefilename);
}
