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
#ifndef SEGMENT_H
#define SEGMENT_H

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

#include "vpro_globals.h"

namespace CNN_LAYER {

  struct SEGMENT;
  typedef std::vector<std::vector<SEGMENT *>> SegmentBatch;

  // copied from cnn_struct_reduced.h
  struct SEGMENT {
    std::vector<uint32_t> in_MM_base{}; // byte address of top-left pixel of padded segment (i.e. outside of padded data)
    std::vector<int32_t> in_MM_y_stride{}; // MM distance of two pixels in vertical direction (in elements)

    uint32_t out_MM_base{}; // byte address
    int32_t out_MM_y_stride{};

    int32_t x_seg{};
    int32_t y_seg{};
    int32_t in_channel{}; // segment info about #in_channel to get correct kernel
    int32_t out_channel{};

    bool dummy{}; // if set, this segment is not written back into LM/MM
    bool isLast{}; // if set, this segment is last to be calculated in one lane
    bool isFirst{}; // if set, this segment is first to be calc (no accumulate of result)

    bool pad_top{};
    bool pad_right{};
    bool pad_bottom{};
    bool pad_left{};

    // multi-line
    std::string to_string() const {
      std::stringstream ss;
      ss << std::setfill('0');
      for (int i = 0; i < (int)in_MM_base.size(); i++)
        ss << "in_MM_base[" << i << "] 0x"   << std::hex << std::setw(8) << in_MM_base[i] << "\n";
      for (int i = 0; i < (int)in_MM_y_stride.size(); i++)
        ss << "in_MM_y_stride[" << i << "] " << std::dec                 << in_MM_y_stride[0] << "\n";
      ss << "out_MM_base 0x"     << std::hex << std::setw(8) << out_MM_base      << "\n"
         << "out_MM_y_stride "   << std::dec                 << out_MM_y_stride  << "\n"
         << "x_seg "                                         << x_seg            << "\n"
         << "y_seg "                                         << y_seg            << "\n"
         << "in_channel "                                    << in_channel       << "\n"
         << "out_channel "                                   << out_channel      << "\n"
         << "dummy "                                         << dummy            << "\n"
         << "isLast "                                        << isLast           << "\n"
         << "isFirst "                                       << isFirst          << "\n"
         << "pad_top "                                       << pad_top          << "\n"
         << "pad_right "                                     << pad_right        << "\n"
         << "pad_bottom "                                    << pad_bottom       << "\n"
         << "pad_left "                                      << pad_left         << "\n";
      return ss.str();
    }

    // single line
    std::string to_short_string() const {
      std::stringstream ss;
      if (dummy) {
          ss << "D";
      } else {
          ss << (dummy ? "D" : " ") << (isFirst ? "F" : " ") << (isLast ? "L" : " ") << " xy(" << std::setw(3) << x_seg << ", " << std::setw(3) << y_seg << "), in ch " << std::setw(2) << in_channel
             << " @ 0x" << std::setfill('0') << std::hex << std::setw(8) << in_MM_base[0] << " s " << std::setfill(' ') << std::dec << std::setw(4) << in_MM_y_stride[0];

          for (int i = 1; i < (int)in_MM_base.size(); i++)
            ss << ", 0x" << std::setfill('0') << std::hex << std::setw(8) << in_MM_base[i] << " s " << std::setfill(' ') << std::dec << std::setw(4) << in_MM_y_stride[i];

          ss << ", out ch " << std::setw(2) << out_channel
             << " @ 0x" << std::setfill('0') << std::hex << std::setw(8) << out_MM_base << " s " << std::setfill(' ') << std::dec << std::setw(4) << out_MM_y_stride;

          ss << ", pad trbl " << pad_top << pad_right << pad_bottom << pad_left;
      }
      return ss.str();
    }

    bool equals(const SEGMENT &ref) const {
      // this is ugly, but memcmp might fail due to padding
      bool equal = true;
      for (int i = 0; i < (int)in_MM_base.size(); i++)
        equal &= ref.in_MM_base[i]    == in_MM_base[i];
      for (int i = 0; i < (int)in_MM_y_stride.size(); i++)
        equal &= ref.in_MM_y_stride[i]== in_MM_y_stride[i];
      equal &= ref.out_MM_base      == out_MM_base     ;
      equal &= ref.out_MM_y_stride  == out_MM_y_stride ;
      equal &= ref.x_seg            == x_seg           ;
      equal &= ref.y_seg            == y_seg           ;
      equal &= ref.in_channel       == in_channel      ;
      equal &= ref.out_channel      == out_channel     ;
      equal &= ref.dummy            == dummy           ;
      equal &= ref.isLast           == isLast          ;
      equal &= ref.isFirst          == isFirst         ;
      equal &= ref.pad_top          == pad_top         ;
      equal &= ref.pad_right        == pad_right       ;
      equal &= ref.pad_bottom       == pad_bottom      ;
      equal &= ref.pad_left         == pad_left        ;
      return equal;
    }

    static bool push_segments_if_full_batch(std::vector<SEGMENT *> &segments, SegmentBatch &batch_list, int &lane) {
      if (lane == 0 || (lane % VPRO_CFG::parallel_Lanes) != 0)
        return false;

      for (unsigned int s = 0; s < batch_list.at(0).size(); s++) {
        for (auto &batch: batch_list) {
          segments.push_back(batch[s]);
        }
      }
      for (auto &batch: batch_list)
        batch.clear();
      lane = 0;
      return true;
    }

  };

  struct DUMMY_SEGMENT : public SEGMENT {
    // create copy of existing SEGMENT and set dummy
    DUMMY_SEGMENT(){
        dummy = true;

        pad_top = false;    // FIXME mimic old cnn converter. Required?
        pad_right = false;
        pad_bottom = false;
        pad_left = false;
    }
    DUMMY_SEGMENT(const SEGMENT &templ) : SEGMENT(templ) {
      dummy = true;

      pad_top = false;    // FIXME mimic old cnn converter. Required?
      pad_right = false;
      pad_bottom = false;
      pad_left = false;
    }

    static int insert_dummies(SegmentBatch &batch_list, const SEGMENT &templ, int lane) {
      // insert dummy segments to the current lane, such that it has the
      // same number of segments as the previous lane
      assert(lane > 0);
      int last_batch_size = batch_list[lane - 1].size();
      for (int i = 0; i < last_batch_size; i++) {
        batch_list[lane].push_back(new DUMMY_SEGMENT(templ));
      }
      return last_batch_size; // number of appended dummies
    }

  };

//void dump_segments(const std::string &fname, std::vector<SEGMENT *> segments);

//void save_segments(const std::string &fname, std::vector<SEGMENT *> segments);

//bool push_segments_if_full_batch(std::vector<SEGMENT *> &segments, SegmentBatch &batch_list, int &lane);

//int insert_dummies(SegmentBatch &batch_list, const SEGMENT &templ, int lane);

}; // namespace CNN_LAYER

#endif // SEGMENT_H
