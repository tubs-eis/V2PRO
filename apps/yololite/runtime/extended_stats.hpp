//
// Created by gesper on 04.10.23.
//

#ifndef EXTENDEND_STATS_HPP
#define EXTENDEND_STATS_HPP

#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <eisv.h>
#include <vpro.h>
#include "calc_cnn.h"

#include "../bif/bif.h"
#include <list>
#include <utility>


uint64_t peak_ops{};

struct layer_stat_t {

public:
    explicit layer_stat_t()= default;
    explicit layer_stat_t(const char *label): label(label){};

    uint64_t any_lane{};
    uint64_t any_dma{};
    uint64_t both{};
    uint64_t only_dma{};
    uint64_t only_lane{};
    uint64_t only_risc{};
    uint64_t tot{};
    uint64_t tot2{};

    const BIF::LAYER *layer{};
    bool wasProcessed{false};

    uint64_t required_ops{};   // for efficiency -> minimum number of calc cycles

    float efficiency{0};
    float achieved_fps{0};
    float peak_fps{0};

    void print() const{
        printf("[layer_stat_t \n    any_lane %" PRIu64 "\n    any_dma %" PRIu64 "\n    both %" PRIu64 "\n    only_dma %" PRIu64 "\n    only_lane %" PRIu64 "\n    tot %" PRIu64 "\n    tot2 %" PRIu64 "]\n",
               any_lane, any_dma, both, only_dma, only_lane, tot, tot2);
    }

    const char *label{""};

    int accumulated{0};
    void operator +=(const layer_stat_t &other){
        any_lane += other.any_lane;
        any_dma += other.any_dma;
        both += other.both;
        only_dma += other.only_dma;
        only_lane += other.only_lane;
        only_risc += other.only_risc;
        tot += other.tot;
        tot2 += other.tot2;
        required_ops += other.required_ops;
        accumulated++;
    }

    void calcEfficiency(){
        peak_fps = float(peak_ops) / float(required_ops);
        achieved_fps = float(get_gpr_risc_freq()) / float(tot);
        efficiency = achieved_fps / peak_fps * 100;
    }
} pre_layer_stat;

std::list<layer_stat_t> stat_list(100, layer_stat_t()); // reserve 100 elements
std::list<layer_stat_t>::iterator stat_list_position = stat_list.begin();

layer_stat_t total_stat{};
layer_stat_t stat_maxpooling2d{"MaxPooling2D"};
layer_stat_t stat_depthwise{"Depthwise"}, stat_add{"Add"};
layer_stat_t stat_conv2d_3x3{"Conv2D (3x3)"}, stat_conv2d_1x1{"Conv2D (1x1)"}, stat_conv2d_3x3_stride{"Conv2D (3x3, stride)"}, stat_conv2d_1x1_stride{"Conv2D (1x1, stride)"};
layer_stat_t stat_other{"Others"};

std::list<layer_stat_t *> stats{&stat_add, &stat_depthwise,
                                &stat_conv2d_1x1, &stat_conv2d_1x1_stride, &stat_conv2d_3x3, &stat_conv2d_3x3_stride,
                                &stat_maxpooling2d,
                                &stat_other};

void pre_layer_stat_update(int layer_exec_idx, int total_layers, const BIF::LAYER *layer, bool details) {

    if (layer_exec_idx == 0 || peak_ops == 0){
        peak_ops = uint64_t(VPRO_CFG::parallel_Lanes) * get_gpr_vpro_freq() * 2;
//        printf("Layer start... first update -> reset...\n");
//        aux_reset_all_stats();
    }

    if (!details){  // else, keep 0 -> will get reset by detail reset
        pre_layer_stat.any_lane = uint64_t(aux_get_CNT_VPRO_LANE_ACT());
        pre_layer_stat.any_dma = aux_get_CNT_VPRO_DMA_ACT();
        pre_layer_stat.both = aux_get_CNT_VPRO_BOTH_ACT();
        pre_layer_stat.tot = aux_get_CNT_VPRO_TOTAL();
        pre_layer_stat.tot2 = aux_get_CNT_RISC_TOTAL();

//        pre_layer_stat.only_dma = pre_layer_stat.any_dma - pre_layer_stat.both;
//        pre_layer_stat.only_lane = pre_layer_stat.any_lane - pre_layer_stat.both;
//        pre_layer_stat.only_risc = pre_layer_stat.tot - pre_layer_stat.both - pre_layer_stat.only_dma - pre_layer_stat.only_lane;
    }
//    printf_info("Layer start... updated stats!\n");
//    pre_layer_stat.print();

    // after this, all stats will be reset if details, else only aux_clr_sys_time
}

void post_layer_stat_update(int layer_exec_idx, int total_layers, const BIF::LAYER *layer, bool details) {

    auto &stat = *stat_list_position;

    stat.any_lane = uint64_t(aux_get_CNT_VPRO_LANE_ACT()) - pre_layer_stat.any_lane;
    stat.any_dma = uint64_t(aux_get_CNT_VPRO_DMA_ACT()) - pre_layer_stat.any_dma;
    stat.both = uint64_t(aux_get_CNT_VPRO_BOTH_ACT()) - pre_layer_stat.both;
    stat.tot = uint64_t(aux_get_CNT_VPRO_TOTAL()) - pre_layer_stat.tot;
    stat.tot2 = uint64_t(aux_get_CNT_RISC_TOTAL()) - pre_layer_stat.tot2;

    stat.only_dma = stat.any_dma - stat.both;
    stat.only_lane = stat.any_lane - stat.both;
    stat.only_risc = stat.tot - stat.both - stat.only_dma - stat.only_lane;
//    printf_info("Layer end... updated stats!\n");
//    stat.print();

    stat.layer = layer;
    stat.wasProcessed = true;
    stat.required_ops = 0;

    switch (layer->type) {
        case UNKNOWN_LAYERTYPE:
            break;
        case CONV2_TRANSPOSE:
        case CONV1:
        case CONV2:
            // 2D Conv:
            assert(layer->conv_groups >= 1);
            stat.required_ops = uint64_t(layer->in_channels / layer->conv_groups) * layer->input.x * layer->input.y * layer->output.channels * layer->kernel_length * layer->kernel_length; // mul
            stat.required_ops *= 2; //uint64_t(layer->input.channels) * layer->input.x * layer->input.y * layer->output.channels * layer->kernel_length * layer->kernel_length; // add

            // stride
            if (layer->stride != 1)
                stat.required_ops /= (layer->stride * layer->stride);

            // bias (always -> init)
            // stat.required_ops += layer->input.x * layer->input.y * layer->output.channels;

            // pooling
            if (layer->pool_stride == 2){   // maxpooling2x2 fused in conv2!
                stat.required_ops += layer->output.x * layer->output.y * layer->output.channels * 3;    // 3 compares for each out pixel
            }

            if(layer->dilation_rate_w != 1 || layer->dilation_rate_h != 1)
                printf_warning("[Extended Stats] Warning: dilation_rate has no required_op reference!\n");

            switch (layer->activation) {
                case LEAKY:
                    stat.required_ops += layer->output.x * layer->output.y * layer->output.channels;    // 1 mul for each out pixel
                    break;
                case RELU6:
                    stat.required_ops += layer->output.x * layer->output.y * layer->output.channels;    // 1 compare/min for each out pixel
                    break;
                case SIGMOID:
                    stat.required_ops += layer->output.x * layer->output.y * layer->output.channels * 3;    // 1 exp, 1 add, 1 div for each out pixel
                    break;
                case SWISH:
                    stat.required_ops += layer->output.x * layer->output.y * layer->output.channels * 4;    // 1 mul, 1 exp, 1 add, 1 div for each out pixel
                    break;
                case RECT:
                case NO_ACTIVATION:
                    break;
            }

            // upsample
            // more store, same number of mul-adds

            break;
        case ADD:
        case MUL:
            // Add 2 input feature maps
            // Mul 2 input feature maps
            stat.required_ops = layer->output.x * layer->output.y * layer->output.channels; // add/mul two inputs
            break;
        case CONCATENATE:
            stat.required_ops = 0; // only memory transfers
            break;
        case DEPTH_TO_SPACE:
            printf_warning("[Extended Stats] Warning: DEPTH_TO_SPACE has no required_op reference!\n");
            break;
        case SCATTER_TO_GRID:
            printf_warning("[Extended Stats] Warning: SCATTER_TO_GRID has no required_op reference!\n");
            break;
        case DYNAMIC_AXIS:
            printf_warning("[Extended Stats] Warning: DYNAMIC_AXIS has no required_op reference!\n");
            break;
        case DCONV_DEFORM:
            printf_warning("[Extended Stats] Warning: DCONV_DEFORM has no required_op reference!\n");
            break;
        case DCONV_CONV:
            printf_warning("[Extended Stats] Warning: DCONV_CONV has no required_op reference!\n");
            break;
        case POINTPILLARS:
            printf_warning("[Extended Stats] Warning: POINTPILLARS has no required_op reference!\n");
            break;
        case AVGPOOL2D:
        case MAXPOOL2D:
            // layer->input.channels *
            stat.required_ops = layer->input.x * layer->input.y * layer->output.channels * layer->kernel_length * layer->kernel_length;
            if (layer->stride != 1)
                stat.required_ops /= (layer->stride * layer->stride);
        case GLOBALAVGPOOL2D:
            stat.required_ops = (1 + layer->input.x * layer->input.y) * layer->output.channels; // x x y additionen, 1 div -> 1 pixel result per out channel
            printf_warning("[Extended Stats] Warning: GLOBALAVGPOOL2D. TODO check required_op reference!!\n");
            break;
        case GLOBALMAXPOOL2D:
            printf_warning("[Extended Stats] Warning: GLOBALMAXPOOL2D has no required_op reference!\n");
            break;
        default:
            printf_warning("[Extended Stats] Warning: default has no required_op reference!\n");
    }

    if (layer->type == CONV2){
        if (layer->conv_groups == layer->out_channels){
            stat_depthwise += stat;
        } else if (layer->stride == 1) {
            if (layer->kernel_length == 3) {
                stat_conv2d_3x3 += stat;
            } else if (layer->kernel_length == 1){
                stat_conv2d_1x1 += stat;
            }
        } else {
            if (layer->kernel_length == 3) {
                stat_conv2d_3x3_stride += stat;
            } else if (layer->kernel_length == 1){
                stat_conv2d_1x1_stride += stat;
            }
        }
    } else if (layer->type == ADD) {
        stat_add += stat;
    } else if (layer->type == MAXPOOL2D){
        stat_maxpooling2d += stat;
    } else {
        stat_other += stat;
    }
    total_stat += stat;

    stat.calcEfficiency();
    stat_list_position++;

    // final layer? -> print stat
    if (layer_exec_idx == total_layers - 1){

        // total stat calc
        total_stat.calcEfficiency();

        // remove unused
        while(!stat_list.empty() && !stat_list.back().wasProcessed)
            stat_list.pop_back();

        // sort by efficiency [low to high]
        // sort by execution cycles [low to high]
        stat_list.sort(
              []( const layer_stat_t& a, const layer_stat_t&b ){
//                  return a.efficiency < b.efficiency;
                  return a.tot < b.tot;
              } );

        // print
        printf("\n######################[Extended Stats Begin]######################\n\n");

        for (const auto &s: stat_list) {

//            printf("s.only_dma: %"PRIu64"\n", s.only_dma);
//            printf("s.both: %"PRIu64"\n", s.both);
//            printf("s.only_lane: %"PRIu64"\n", s.only_lane);

            printf("Layer (%i): (%s%s%s%s%s), ", s.layer->number, to_char(s.layer->type),
                   ((s.layer->conv_groups > 1)? ("Depthwise(Groups:"+std::to_string(s.layer->conv_groups)+")").c_str():""), // FIXME groups > 1 does not imply depthwise, groups is more general
                   ((s.layer->type == CONV2) ? (", "+std::to_string(s.layer->kernel_length)+"x"+std::to_string(s.layer->kernel_length)).c_str(): ""),
                   ((s.layer->stride > 1) ? (", stride: "+std::to_string(s.layer->stride)).c_str() : ""),
                   ((s.layer->pool_stride == 2) ? " maxpool2x2" : ""));
//            printf_info("\n%s\n", s.layer->to_char());
            if (s.efficiency <= total_stat.efficiency)  // smaller than average
                printf("Efficiency \e[1m\e[91m%03.2f%%\e[0m, ", s.efficiency);  // 1 -> bold, 91 -> light red
            else
                printf("Efficiency %03.2f%%, ", s.efficiency);
            printf("Required OPs %10" PRIu64 " = %3" PRIu64 "M, ", s.required_ops, s.required_ops/1000000);
            printf("Cycles %10" PRIu64 " (%3.2f%% of total)\n", s.tot, float(s.tot)/float(total_stat.tot)*100);

#define INT_PERCENT(A, REF) uint32_t((uint64_t(100)*(A))/(REF)), uint32_t((uint64_t(10000)*(A))/(REF) - ((uint64_t(100)*(A))/(REF))*100)
//            printf("achieved_fps: %f\n", stat.achieved_fps);
//            printf("peak_fps: %f\n", stat.peak_fps);
            if (((100*s.only_dma)/s.tot) >= 40)  // only dma percentage >= 40 %
                printf("     only DMA: \e[1m\e[91m%3u.%02u%%\e[0m \n", INT_PERCENT(s.only_dma, s.tot)); // 1 -> bold, 91 -> light red
            else
                printf("     only DMA: %3u.%02u%% \n", INT_PERCENT(s.only_dma, s.tot));
            printf("         Both: %3u.%02u%% \n", INT_PERCENT(s.both, s.tot));
            if (((100*s.only_lane)/s.tot) >= 40)  // only vpro percentage >= 40 %
                printf("    only VPRO: \e[1m\e[91m%3u.%02u%%\e[0m \n", INT_PERCENT(s.only_lane, s.tot)); // 1 -> bold, 91 -> light red
            else
                printf("    only VPRO: %3u.%02u%% \n", INT_PERCENT(s.only_lane, s.tot));
            if (((100*s.only_risc)/s.tot) >= 10)  // only risc-v percentage >= 10 %
                printf("  only Risc-V: \e[1m\e[91m%3u.%02u%%\e[0m \n", INT_PERCENT(s.only_risc, s.tot)); // 1 -> bold, 91 -> light red
            else
                printf("  only Risc-V: %3u.%02u%% \n", INT_PERCENT(s.only_risc, s.tot));
        }

        printf("Color Legend: Red - only DMA (> 40%%), VPRO (> 40%%), Risc-V (> 10%%)\n");
        printf("            : Red - Efficiency (< average%%)\n");
        printf("\n######################[Extended Stats Types]######################\n\n");

        for (auto s: stats) {
            if (s->accumulated > 0){
                s->calcEfficiency();
                printf_info("Layertype: %s", s->label);
                printf(" (Count: %i)\n", s->accumulated);
                printf("Efficiency %03.2f%%, ", s->efficiency);
                printf("Required OPs %10" PRIu64 " = %3" PRIu64 "M (%3u.%02u%%), ", s->required_ops, s->required_ops/1000000,
                       INT_PERCENT(s->required_ops, total_stat.required_ops));
                printf("Cycles %10" PRIu64 " (%3u.%02u%%)\n", s->tot,
                       INT_PERCENT(s->tot, total_stat.tot));
                printf("     only DMA: %3u.%02u%% \n", INT_PERCENT(s->only_dma, s->tot));
                printf("         Both: %3u.%02u%% \n", INT_PERCENT(s->both, s->tot));
                printf("    only VPRO: %3u.%02u%% \n", INT_PERCENT(s->only_lane, s->tot));
                printf("  only Risc-V: %3u.%02u%% \n\n", INT_PERCENT(s->only_risc, s->tot));
            }
        }

        printf("######################[Extended Stats Total]######################\n\n");

        printf("Peak Performance: %" PRIu64 " GOP/s %" PRIu64 " MOP/s (parallel Lanes: %i)\n\n", peak_ops / 1000/1000/1000, peak_ops / 1000/1000, VPRO_CFG::parallel_Lanes);

        printf("Efficiency %03.2f%%, ", total_stat.efficiency);
        printf("Required OPs %10" PRIu64 " = %3" PRIu64 "M , ", total_stat.required_ops, total_stat.required_ops/1000000);
        printf("Cycles %10" PRIu64 "\n", total_stat.tot);
        printf("     only DMA: %3u.%02u%% \n", INT_PERCENT(total_stat.only_dma, total_stat.tot));
        printf("         Both: %3u.%02u%% \n", INT_PERCENT(total_stat.both, total_stat.tot));
        printf("    only VPRO: %3u.%02u%% \n", INT_PERCENT(total_stat.only_lane, total_stat.tot));
        printf("  only Risc-V: %3u.%02u%% \n", INT_PERCENT(total_stat.only_risc, total_stat.tot));

        printf("\nAchieved FPS: %f [Peak FPS: %f]\n", total_stat.achieved_fps, total_stat.peak_fps);

        uint64_t totals = total_stat.only_dma + total_stat.only_lane + total_stat.only_risc + total_stat.both;
        if (totals != total_stat.tot)
            printf(" Sum : %" PRIu64 " vs. total accumulator: %" PRIu64 "!!!\n", totals, total_stat.tot);

        printf("\n######################[Extended Stats End  ]######################\n\n");
    }
}

#endif // EXTENDEND_STATS_HPP
