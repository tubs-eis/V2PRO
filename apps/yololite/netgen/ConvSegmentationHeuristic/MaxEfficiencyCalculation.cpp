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
// Created by gesper on 12.06.23.
//

#include "MaxEfficiencyCalculation.h"
#include <math.h>
#include <list>
#include <set>
#include <algorithm>
#include <cstdio>
#include <assert.h>
#include "misc.h"
#include "vpro_globals.h"

uint32_t MaxEfficiencyCalculation::getBlockSizeUpperBound(uint32_t insize, uint32_t n, uint32_t m) {
    // equation:
    // 1024 >= x*n+2*n  // x*n = outputs, kernel + bias in RF
    // 4096 >= x*n      // outputs in LM
    // 4096 >= x*m+2*n  // inputs + kernel + bias in LM

    int lm_max = int(std::min(floor(VPRO_CFG::LM_SIZE / 2. / n), floor(-2. * (float(n) - (VPRO_CFG::LM_SIZE / 4)) / m)));
    int rf_max = int(floor(-2. * (float(n) - (VPRO_CFG::RF_SIZE / 2)) / n));
    assert(lm_max >= 0);
    assert(lm_max < VPRO_CFG::LM_SIZE / 2);
    assert(rf_max >= 0);
    assert(rf_max < VPRO_CFG::RF_SIZE);

    uint32_t beta_max = 63; // to index another n, beta stores the jump across the input dimension! TODO: implementation fix; conv, bias/kernel load, store (also x_end)

    return std::min({(uint32_t)rf_max, insize, (uint32_t)lm_max, beta_max});
}

uint32_t MaxEfficiencyCalculation::getBlockSizeLowerBound(uint32_t insize, uint32_t n, uint32_t m) {
    return 1u;
}

int MaxEfficiencyCalculation::getBlockCount(uint32_t blocksize, uint32_t in_size) {
    return ceil_div(in_size, blocksize);
}

int MaxEfficiencyCalculation::getBlockOverlap(uint32_t blocksize, uint32_t blockcount, uint32_t in_size) {
    return int((blocksize * blockcount) - in_size);
}

void MaxEfficiencyCalculation::runCalculation() {
    in_size = inx * iny;

    if (not silent) {
        printf("Running Eval for:\n");
        printf("  inx = %i, iny = %i, [in size = %i]\n", inx, iny, in_size);
        printf("  inc = %i, outc = %i\n", inc, outc);
        printf("  HW parallel Lanes: %i\n\n", VPRO_CFG::parallel_Lanes);
    } else {
        printf("Eval for inx = %i, iny = %i, [in size = %i], inc = %i, outc = %i. HW parallel Lanes: %i\n", inx, iny, in_size, inc, outc, VPRO_CFG::parallel_Lanes);
    }

    auto bs = getBlockSizeUpperBound(in_size, 1, 1);
    auto bc = getBlockCount(bs, in_size);
    runCalculationSegmentation(1, 1, bs, bc);

    double overcalc_correction = 1. - (double(getBlockOverlap(bs, bc, in_size)) / (bs * bc));
    double mergeEff, originalEff = calc_eff * overcalc_correction * hw_eff * 100;
    final.efficiency = originalEff;
    final.n = 1;
    final.m = 1;
    final.block_count = bc;
    final.in_size = bs;
    final.overcalc = getBlockOverlap(bs, bc, in_size);

    int m = 1;  // TODO: multiple inputs...
    //    int bs_previous = 0;
    uint eval_count = 0;

    uint count = 0;
    for (uint32_t n = 2; n <= std::min(std::min(outc, in_size / 2), 62u); n += 2) {   // in_size/2 | n = 62 => bs = 14 | n = 31 => bs =  31
        for (bs = 1; bs <= getBlockSizeUpperBound(in_size, n, m); ++bs) {  //getBlockSizeUpperBound(in_size, n, m)
            count++;
        }
    }

    printf("\rRemaining: %5i Configuration for Evaluation\n", count);
    for (uint32_t n = 2; n <= std::min(std::min(outc, in_size / 2), 62u); n += 2) {   // in_size/2 | n = 62 => bs = 14 | n = 31 => bs =  31
        for (bs = 1; bs <= getBlockSizeUpperBound(in_size, n, m); ++bs) {  //getBlockSizeUpperBound(in_size, n, m)
            printf("\rRemaining: %5i", count);
            count--;
            auto bc = getBlockCount(bs, in_size);
            runCalculationSegmentation(n, m, bs, bc);

            overcalc_correction = 1. - (double(getBlockOverlap(bs, bc, in_size)) / (bs * bc));  // pow? -> more important?!
            mergeEff = calc_eff * overcalc_correction * hw_eff * 100;

            eval_count++;

            if (!silent) {
                if (PRINT_EVERY_TRY || (mergeEff > final.efficiency)) {
                    if (mergeEff > final.efficiency) {
                        printf("\r \n[MAX EFF] ");
                    } else {
                        printf("\r   current try ");
                    }
                    printf("Method [n %3i, m %3i]\tBlock Size: %4i,\tBlock Count: %4i,\tOverlap Elements:", n, m, bs, bc);
                    printf("%i", getBlockOverlap(bs, bc, in_size));
                    printf(",\tCalc Efficiency: %.03lf,\tHW Efficiency: %.03lf\t[Overcalc correction due to segment size: %.03lf]", calc_eff, hw_eff, overcalc_correction);
                    printf("\t Total Eff: %.03lf (max: %.03lf)", mergeEff, final.efficiency);
                    if (PRINT_EVERY_TRY_IN_NEW_LINE || (!PRINT_EVERY_TRY_IN_NEW_LINE && (mergeEff > final.efficiency))) {
                        printf("\n");
                    }
                }
            }
            if (mergeEff > final.efficiency) {
                final.efficiency = mergeEff;
                final.n = n;
                final.m = m;
                final.block_count = bc;
                final.in_size = bs;
                final.overcalc = getBlockOverlap(bs, bc, in_size);
            }
        }   // bs
    }   // n
    printf("\rAll Evaluations Done!\n");

    if (not silent)
        printf("\r \n\n#########################################\n Best Result: (n %i, m %i, bs %i, bc %i, eff %lf)\n", final.n, final.m, final.in_size, final.block_count,
               final.efficiency);
    else
        printf("  Best Result: (n %i, m %i, bs %i, bc %i, eff %lf)\n", final.n, final.m, final.in_size, final.block_count, final.efficiency);

    double improvement_vs_original = final.efficiency / originalEff - 1;
    printf("  Original Eff: %.2f\n", originalEff);
    printf("  Efficiency Improvement: +%.2f%% [After %u evaluations]\n", improvement_vs_original * 100, eval_count);
}

void MaxEfficiencyCalculation::runCalculationSegmentation(uint n, uint m, uint block_size, uint blockcount) {
    bool DEBUG = false;

    uint total_transfers = 0;
    uint total_calcs_ext_dummies = 0;
    uint total_executed_correct_segments = 0;
    uint total_executed_dummy_segments = 0;

    assert(inc > 0);

    struct Seg {
        Seg(bool dummy, uint c = 0, uint x = 0) : dummy(dummy), outc(c), x(x){};
        bool dummy{};
        uint outc{0};
        uint x{0};
    };

    std::vector<Seg> set; // for all lanes, use this initial set of segments

    uint appended_segs = 0, appended_dummies = 0;   // counters. final added segments for execution
    uint total_segs = blockcount * inc * outc;

    if (DEBUG) {
        printf("\n###################################################\n");
        printf("[n %i, m %i] block size: %i, block count: %i\n", n, m, block_size, blockcount);
        printf("Logical Segments to be processed on any lane (block * inc * outc): %u\n", total_segs);
    }


    // get paralellLanes * n segments [same inc, preferred is same block, diff outc]
    // loop ics when all lanes have a segment
    std::list<Seg> seg_blocks{};

    enum SET_FILL_METHOD {
        ITERATE_ALL_SORTED_OUTC,
        ITERATE_ALL_SORTED_X,
        ITERATE_ALL_SORTED_X2,
    } volatile fillMethod = ITERATE_ALL_SORTED_X2;       // TODO: is this a parameter generated here? -> use different strategy based on best result

//    if (outc > VPRO_CFG::parallel_Lanes){
//        fillMethod = ITERATE_ALL_SORTED_X2;
//    } else {
//        fillMethod = ITERATE_ALL_SORTED_X;
//    }

    if (fillMethod == ITERATE_ALL_SORTED_X) {
        for (uint c_start = 0; c_start < outc; c_start += n) {
            for (uint x = 0; x < blockcount; ++x) {
                for (uint c = c_start; c < c_start + n && c < outc; ++c) { // new x after every parallel outc set
                    seg_blocks.emplace_back(false, c, x);
                }
            }
        }
    } else if (fillMethod == ITERATE_ALL_SORTED_X2) {
        for (uint c_start = 0; c_start < outc; c_start += VPRO_CFG::LANES * n) {
            for (uint x = 0; x < blockcount; ++x) {
                for (uint c = c_start; c < c_start + VPRO_CFG::LANES * n && c < outc; ++c) { // new x after every parallel outc set
                    seg_blocks.emplace_back(false, c, x);
                }
            }
        }
    } else if (fillMethod == ITERATE_ALL_SORTED_OUTC) {
        for (uint x = 0; x < blockcount; ++x) {
            for (uint c = 0; c < outc; ++c) {
                seg_blocks.emplace_back(false, c, x);
            }
        }
    }
    assert(seg_blocks.size() == blockcount * outc);

    while (appended_segs < total_segs) {
        // new set for all lanes
        set.clear();

        for (uint lane = 0; lane < VPRO_CFG::parallel_Lanes; lane++) {
            bool dummy_lane = false;
            for (uint n_iteration = 0; n_iteration < n; n_iteration++) { // parallel outc per lane
                if (!set.empty() && !seg_blocks.empty()) {
                    if (seg_blocks.front().x != set.back().x) {
                        // fill current lane with dummies
                        if (n_iteration > 0)
                            dummy_lane = true;
                        // new x only if new unit (first segment)
                        if (lane % VPRO_CFG::LANES != 0)
                            dummy_lane = true;
                    }
                    if (dummy_lane) {
                        set.emplace_back(true);
                        continue;
                    }
                }
                if (seg_blocks.empty()) {
                    // no remaining segments
                    set.emplace_back(true);
                    continue;
                }
                set.push_back(seg_blocks.front());
                seg_blocks.pop_front();
            }
        }
        assert(set.size() == VPRO_CFG::parallel_Lanes * n);

        // add segments to final list for all ics
        for (const Seg &seg: set) {
            if (seg.dummy) {
                appended_dummies += inc;
                total_executed_dummy_segments += 1;  // * inc
            } else {
                appended_segs += inc;
                total_executed_correct_segments += 1;  // * inc
            }
        }   // all in_channels are processed

        // split to parallel clusters for broadcast elimination
        auto prev = set.begin();
        int cluster = 0;
        std::vector<std::list<Seg>> cluster_dma_list(VPRO_CFG::CLUSTERS);
        cluster_dma_list[cluster].push_back(*prev);
        int seg_proc = 1;
        assert(n * VPRO_CFG::UNITS * VPRO_CFG::LANES >= 2);   // as this is already the second!
        for (auto seg = ++set.begin(); seg != set.end(); seg++) {
            if (!seg->dummy) {
                cluster_dma_list[cluster].push_back(*seg);
            }
            prev++;
            seg_proc++;
            if (seg_proc % (n * VPRO_CFG::UNITS * VPRO_CFG::LANES) == 0) {
                cluster++;
                seg_proc = 0;
            }
        }

        // sort for each cluster/dma the segments
        auto sortRuleLambda = [](Seg const &s1, Seg const &s2) -> bool {
            if (s1.x != s2.x) return s1.x < s2.x;
            else return s1.outc < s2.outc;
        };
        for (auto &cluster_list: cluster_dma_list) {
            cluster_list.sort(sortRuleLambda);
            cluster_list.unique([](Seg const &s1, Seg const &s2) -> bool {
                return (s1.x == s2.x && s1.outc == s2.outc);
            });
        }

//        int c = 0;
//        for (auto &cluster_list: cluster_dma_list) {
//            printf("[K] Cluster %i [%zu]: ", c, cluster_list.size());
//            for (auto s: cluster_list) {
//                if (!s->dummy)
//                    printf(" (%i, %i)", s->x, s->outc);
//            }
//            printf("\n");
//            c++;
//        }
//        c = 0;

        // get the list for cluster with most transfers
        uint max_dma_length = 0;
        for (auto &cluster_list: cluster_dma_list) {
            uint this_dma_length = 0;

            // calculate the transfers for this cluster's segments (broadcasts are already merged)
            // all inc segments are required (inputs + kernel [n], if seg_blocks.empty() => store)
            // unique -> same x/outc -> kernel broadcast

            this_dma_length += cluster_list.size() * inc;  // kernel
            this_dma_length += cluster_list.size() * block_size;  // store
            this_dma_length += cluster_list.size();   // bias

            // unique without outc information -> same x -> input broadcast
            cluster_list.unique([](Seg const &s1, Seg const &s2) -> bool {
                return (s1.x == s2.x);
            });

//            printf("[IN] Cluster %i [%zu]: ", c, cluster_list.size());
//            for (auto s: cluster_list) {
//                if (!s->dummy)
//                    printf(" (%i, %i)", s->x, s->outc);
//            }
//            printf("\n");
//            c++;

            this_dma_length += cluster_list.size() * block_size * inc;   // input

            // TODO: DMA delay penalty by #nr of dma commands?

            // check for maximum
            if (this_dma_length >= max_dma_length) {
                max_dma_length = this_dma_length;
            }
        }
        uint calc_cycles = inc * block_size * n;

        if (DEBUG) {
            printf("For this Set (IN C times: %i): dma_cycles: %u, calc_cycles: %u\n", inc, max_dma_length, calc_cycles);
            printf("total_executed_correct_segments: %i, total_executed_dummy_segments: %i\n", total_executed_correct_segments, total_executed_dummy_segments);
        }

        total_transfers += max_dma_length;
        total_calcs_ext_dummies += calc_cycles;
    }   // all segments added

    assert(seg_blocks.empty());

    calc_eff = 100 * double(total_calcs_ext_dummies) / VPRO_CFG::parallel_Lanes / total_transfers;
    hw_eff = double(total_executed_correct_segments) / (total_executed_dummy_segments + total_executed_correct_segments);

    if (DEBUG) {
        printf(" Calculations (Excluding Dummy Lanes): %i\n", total_calcs_ext_dummies);
        printf(" Transfers (No Dummy Transfers): %i \n", int(total_transfers));

        printf(" Segments on Lanes: %i \n", int(total_executed_correct_segments));
        printf(" Dummy Segments on Lanes: %i \n", int(total_executed_dummy_segments));

        printf("\t Calc Eff: %.08lf [Calc/Transfer]\n", calc_eff);
        printf("\t HW Eff: %.08lf\n", hw_eff);
    }
}
