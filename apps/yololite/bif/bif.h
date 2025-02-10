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
#ifndef BIF_H
#define BIF_H

#include <inttypes.h>
#include <bitset>
#include <cstdint>
#include <vector>
#include <cassert>
#include <cstddef>
#include "vpro/vpro_globals.h"

// Binary InterFace CNN description common to generator (host) and EISV runtime
// Stuff exclusive to host or EISV does NOT go here!

// Endianness
// Binary data generation relies on common endianness of host and EISV
// EISV is little endian (at least for data).
// [main.bin is swapped to 32-bit big endian (make all calls objcopy --reverse-bytes=4)]

// minimum number of wait cycles between instructions writing and reading to/from the same RF address
// FIXME this is a hardware property and does not belong into this file. Move to ISS HW struct?
#define W2R_BUBBLE_CYCLES 6

// reserved register file address for unwanted but unavoidable writes

#define RF_DISCARD_ADDR (VPRO_CFG::RF_SIZE-1)

typedef uint64_t mm_addr_type; // FIXME why 64?
typedef uint32_t mm_size_type;
typedef int16_t weight_t;
typedef int16_t qparam_t;
typedef int16_t data_t;

typedef std::vector<uint8_t> Blob;

// legacy char* instead of std::string for compatibility with embedded gcc.
// RISCV target: ld _sometimes_ says "undefined reference to `__dso_handle'" if anything related to string, new etc. is compiled

const char* mmAddrStr(mm_addr_type addr);
const char* to_bin(size_t const size, void const *const ptr, char *buf = NULL);


// enums defined outside BIF namespace, "enum" instead of "enum class" -> global constants
//  + human-readable code (avoid cluttering prefixes BIF::COMMAND_SEGMENT_TYPE::)
//  - all enum values must be globally unique

enum POOLTYPE : uint8_t {
  NO_POOLING = 0,
  MAX_POOLING = 1, // ideas: min pooling, avg pooling?
  AVG_POOLING = 2,
};
const char* to_char(POOLTYPE type);

/**
 * To identify buffers (double-buffering) inside the segment generation process
 */
enum BUFFER : uint8_t{
    A = 0,
    B = 1
};

/**
 * For VPRO Commands
 */
enum VPRO_TYPE : uint8_t  {
    conv_start = 0,
    conv_add = 1,
    maxpool2x2_fused = 2,
    activation_fused = 3,
    shift_store = 4,
    add = 5,
    conv_transpose_start = 6,
    conv_transpose_add = 7,
    conv1d_start = 8,
    conv1d_add = 9,
    concatenate = 10,
    depth_to_space = 11,
    dconv_deform_8x8 = 12,
    dconv_conv_start = 13,
    dconv_conv_add = 14,
    relu_pool_scatter = 15,
    max_pooling = 16,
    avgpool2d = 17,
    mul = 18,
    global_avgpool2d_start = 19,
    global_avgpool2d_add = 20,
    global_avgpool2d_store_intermediates = 21,
    global_avgpool2d_sum_intermediates = 22,
    global_avgpool2d_divide = 23,
    set_masks = 24,
    reset_indices = 25,
    shift_store_upsample = 26,
};
const char* to_char(VPRO_TYPE type);

enum ACTIVATION : uint8_t {
  LEAKY = 0,
  RECT = 1,
  RELU6 = 2,
  SIGMOID = 3,
  SWISH = 4,
  NO_ACTIVATION = 255
};
const char* to_char(ACTIVATION type);

enum LAYERTYPE : uint8_t {
  UNKNOWN_LAYERTYPE = 0,
  CONV2 = 1,
  ADD = 2,
  MUL = 3,
  CONV2_TRANSPOSE = 4,
  CONV1 = 5,
  CONCATENATE = 6,
  DEPTH_TO_SPACE = 7,
  SCATTER_TO_GRID = 8,
  DYNAMIC_AXIS = 9,
  DCONV_DEFORM = 10,
  DCONV_CONV = 11,
  POINTPILLARS=12,
  MAXPOOL2D = 13,
  AVGPOOL2D = 14,
  GLOBALAVGPOOL2D = 15,
  GLOBALMAXPOOL2D = 16,
};
const char* to_char(LAYERTYPE type);

enum COMMAND_SEGMENT_TYPE : uint8_t {
  DMA_CMD = 0,
  VPRO_CMD = 1,
  DMA_WAIT = 2,
  VPRO_WAIT = 3,
  DMA_BLOCK = 4,
  BOTH_SYNC = 5,
  SCATTER_CMD = 6,
  DMA_SET_PADDING = 7,
  UNKNOWN_COMMAND_SEGMENT_TYPE = 255
};
const char* to_char(COMMAND_SEGMENT_TYPE type);

// BEGIN based on ISS/common_lib/vpro/dma_cmd_struct.h
/**
 * DMA Commands can load data 2D, or 1D
 * DMA Commands can load data extern -> local, or local -> extern
 */
enum DMA_DIRECTION : uint8_t {
  /**
   * extern to local, 1D
   */
  e2l1D = 0,
  /**
   * extern to local, 2D
   */
  e2l2D = 1,
  /**
   * local to extern, 1D
   */
  l2e1D = 2,
  /**
   * local to extern, 2D
   */
  l2e2D = 3,
  /**
   * DMA Command Loop Generation
   */
  loop = 4  // may not be used in COMMAND_DMA directly!
};
const char* to_char(DMA_DIRECTION dir);

namespace BIF {

  struct  PAD_REDUCED {
    int32_t top{};
    int32_t left{};
    int32_t bottom{};
    int32_t right{};
    int32_t value{};

    const char* to_char(const char *prefix = "") const {
      static char buf[1024];
      sprintf(buf, "%stop    %" PRId32 "" "%sleft   %" PRId32 "" "%sbottom %" PRId32 "" "%sright  %" PRId32 "" "%svalue  %" PRId32 "",
              prefix, top,
              prefix, left,
              prefix, bottom,
              prefix, right,
              prefix, value);
      return buf;
    }

  };
  static_assert(sizeof(PAD_REDUCED) == 20, "Memory layout of packed struct");
  
  /**
   * DMA Commands can be created by DCache (Hardware) by loading one of this structs
   * These structs need to match 32-bytes (loaded parallel by dcache)
   * These elements need to be placed aligned to memory space
   *    in Instanciation: __attribute__ ((aligned (16)))
   */
  struct  COMMAND_DMA {
    /**
     * this Commands direction (e2l/l2e) and dimension (1D/2D)
     */
    DMA_DIRECTION direction{};
    bool isBiasOffset{false}; // no longer used by hardware but still useful for debug
    bool isKernelOffset{false}; // no longer used by hardware but still useful for debug
      // switch padding on/off per transfer; padding length is configured layer-wide via LAYER.pad
    uint8_t padding{}; // 7 downto 0 := '3 = left, '2 = bottom, '1 = right, '0 = top |  order see CommandDMA::PAD

    uint32_t cluster{}; // index when generated by layer, turned into a bit mask by DmaBlockExtension
    uint32_t unit_mask{};
    uint32_t mm_addr{}; // byte address of first non-padding element
    uint32_t mm_addr_64{};  // used in simulation (ISS) @ 02.06.2023 -> order of elements changed

    uint32_t lm_addr{}; // word address
    uint16_t y_leap{}; // distance of last transferred element in row n to first element of row n+1; =1 for gapless
                       // misleadingly called "x_stride" in ISS and HW
    uint16_t x_size{}; // in 16-bit words
    uint16_t y_size{}; // in 16-bit words

    uint8_t skipped_elements_at_end{0};    // used during CNN Conversion only
    COMMAND_SEGMENT_TYPE type{DMA_CMD};
//    uint16_t &word_count = x_stride;
//    uint8_t filler[6]{};    // to match 32-byte [6]

    const char* to_char() const {
      static char buf[1024];

#if 0 // cluster IS NOT one-hot in general -> only useful notation is a binary bit mask
      // sometimes cluster is an index, sometimes a one-hot bit mask; no way to distinguish here
      // convert one-hot bit mask to index if possible
      uint32_t c = cluster;
      int bits1 = 0;
      int log2_cluster = -1;
      while (c) {
        bits1 += c & 1;
        log2_cluster++;
        c >>= 1;
      }
      char bitidx = '?';
      if (bits1 == 1 && log2_cluster < 10)
        bitidx = '0' + log2_cluster;
#endif

      if (VPRO_CFG::CLUSTERS < 32) // compile will procude shift result = 0xffffffff instead 0x0
          assert(!(cluster >> VPRO_CFG::CLUSTERS) && "cluster mask has bit set beyond VPRO_CFG::CLUSTERS");
      if (VPRO_CFG::UNITS < 32)    // compile will procude shift result = 0xffffffff instead 0x0
          assert(!(unit_mask >> VPRO_CFG::UNITS) && "unit mask has bit set beyond VPRO_CFG::UNITS");
      char bufc[70];
      char bufu[70];
      sprintf(buf, "dir %s, " "isKernelOffset %d, " "isBiasOffset %d, " "cluster 0b%s, "
              "unit_mask 0b%s, " "mm_addr 0x%08" PRIx32 ", " "lm_addr 0x%04" PRIx32 ", " "y_leap %3d, " "x_size %4d, "
              "y_size %4d, padding trbl %d%d%d%d",
              ::to_char(direction), isKernelOffset, isBiasOffset, to_bin(VPRO_CFG::CLUSTERS, &cluster, bufc), to_bin(VPRO_CFG::UNITS, &unit_mask, bufu),
              mm_addr, lm_addr, y_leap, x_size, y_size, padding & 1, (padding >> 1) & 1, (padding >> 2) & 1, (padding >> 3) & 1);
      return buf;
    }
    const char* unit_mask_to_char() const {
      static char buf[32];
      sprintf(buf, "%d",
      		  unit_mask);
      return buf;
    }

    bool equals(const COMMAND_DMA &ref) const {
      bool equal = true;
      equal &= (ref.direction == direction);
//      equal &= (ref.isBiasOffset == isBiasOffset);
//      equal &= (ref.isKernelOffset == isKernelOffset);
      equal &= (ref.cluster == cluster);
      equal &= (ref.unit_mask == unit_mask);
      //equal &= (ref.mm_addr == mm_addr);//FIXME: uncomment this, once MM addresses are handled correctly
      equal &= (ref.lm_addr == lm_addr);
      equal &= (ref.y_leap == y_leap);
      equal &= (ref.x_size == x_size);
      equal &= (ref.y_size == y_size);
      equal &= (ref.padding == padding);
      return equal;
    }
  };
  static_assert(sizeof(COMMAND_DMA) == 32, "Memory layout of packed struct");
  // END bases on ISS/common_lib/vpro/dma_cmd_struct.h


    /**
     * COMMAND_DMA_LOOP usage requires a COMMAND_DMA afterwards (base)
     */
    struct COMMAND_DMA_LOOP {
        DMA_DIRECTION direction{loop};// '0.
        uint8_t cluster_loop_len{};
        int8_t cluster_loop_shift_incr{};
        uint8_t unit_loop_len{};
        int8_t unit_loop_shift_incr{};  // '1.
        uint8_t inter_unit_loop_len{};
        uint8_t struct_padding0[2]{};
        int16_t lm_incr{};  // 13-bit signed! // '2.
        uint8_t struct_padding1[2]{};
        int32_t mm_incr{}; // '3.
        uint16_t dma_cmd_count{};// '4.
        uint8_t struct_padding2[12]{};//pad structure to 32 byte
        COMMAND_SEGMENT_TYPE type{DMA_CMD};

        const char *to_char() const {
            static char buf[1024];
            sprintf(buf, " DMA LOOP, " "cluster_loop_len %d, " "cluster_loop_shift_incr %d, " "unit_loop_len %d, "
                         "unit_loop_shift_incr %d, " "inter_unit_loop_len %d, " "lm_incr 0x%04" PRIx32 ", " "mm_incr 0x%08" PRIx32 ", "
                         "dma_cmd_count %d",
                    cluster_loop_len, cluster_loop_shift_incr, unit_loop_len, unit_loop_shift_incr,
                    inter_unit_loop_len, lm_incr, mm_incr, dma_cmd_count);
            return buf;
        }

        bool equals(const COMMAND_DMA_LOOP &ref) const {
            bool equal = true;
            equal &= (ref.direction == direction);
            equal &= (ref.cluster_loop_len == cluster_loop_len);
            equal &= (ref.cluster_loop_shift_incr == cluster_loop_shift_incr);
            equal &= (ref.unit_loop_len == unit_loop_len);
            equal &= (ref.unit_loop_shift_incr == unit_loop_shift_incr);
            equal &= (ref.inter_unit_loop_len == inter_unit_loop_len);
            equal &= (ref.lm_incr == lm_incr);
            equal &= (ref.mm_incr == mm_incr);
            equal &= (ref.dma_cmd_count == dma_cmd_count);
            return equal;
        }
    };

    static_assert(sizeof(COMMAND_DMA_LOOP) == 32, "Memory layout of packed struct");


  // BEGIN based on cnn_struct_reduced.h g9114dd8
  struct  COMMAND_VPRO { // used by      shift_store maxpool2x2 activation elwise
    VPRO_TYPE command{};                   //  x         x          x         x
    union {
        uint8_t lane{};                    //
        uint8_t lane_mask;                 //  x         x          x
    };
    union {
        uint16_t buffer{};                 //
        uint16_t lm_base;                  //  x                    x         x
    };
    uint16_t xend{};                       //  x         x          x         x
    uint16_t yend{};                       //  x         x          x         x
    uint16_t zend{};                       //  x         x          x
    union {
        uint16_t offset{};                 //
        uint16_t rf_base;                  //  x         x          x         x
    };
    int16_t shift_right{};                 //  x                    x
    union {
        uint16_t kernel_load_buffer_l0{};  //
        uint16_t broadcast_map;            //                                 x
        uint16_t n_summands;               // pooling
        int16_t multiplier;                // global_pooling
    };
    union {
        uint16_t kernel_load_buffer_l1{};  //
        uint16_t rf_ch_stride;             //  x         x          x
        int16_t pre_shift_right;           // global_pooling
    };
    union {
        uint16_t bias_load_buffer_l0{};    //
        uint16_t lm_ch_stride;             //  x
        uint16_t cluster_mask;             //
    };
    union {
        uint16_t bias_load_buffer_l1{};    //
        int16_t rf_frac_bits;              //                       x
        uint16_t unit_mask;                //
    };
    union {
        uint16_t deform_offset_buffer{};   //
        uint16_t in_ch_offset;             //
        uint16_t pp_index_buffer;          //
    };

    uint16_t deform_output_buffer{};       //
    // FIXME VPRO_CFG::RF_SIZE
    int16_t lm_lane_stride                 //  x                    x
      {(int16_t)VPRO_CFG::RF_SIZE};
    // pad structure to 32 byte
    uint8_t struct_padding[2]{};
    // wait cycles before instruction
    // execution starts
    uint8_t nops{};                        //  x         x          x    
    COMMAND_SEGMENT_TYPE type{COMMAND_SEGMENT_TYPE::VPRO_CMD};

    const char* to_char() const {
      static char buf[1024];
      if (
          command == VPRO_TYPE::maxpool2x2_fused ||
          command == VPRO_TYPE::activation_fused ||
          command == VPRO_TYPE::shift_store ||
          command == VPRO_TYPE::add ||
          command == VPRO_TYPE::mul ||
          command == VPRO_TYPE::global_avgpool2d_start ||
          command == VPRO_TYPE::global_avgpool2d_add          
          ) {
          char bin_buf[65];
          sprintf(buf, "%s, nops %d, lane_mask 0b%s, xend %d, yend %d, zend %d, rf_ch_stride %d, rf_base 0x%03x, lm_ch_stride %d, lm_base 0x%04x, shift_right %d, rf_frac_bits %d",
                  ::to_char(command), nops, to_bin(VPRO_CFG::LANES, &lane_mask, bin_buf),
                  xend, yend, zend, rf_ch_stride, rf_base, lm_ch_stride, lm_base,
                  shift_right, rf_frac_bits);
      } else {
          sprintf(buf, "%s, " "lane %d, " "buffer %d, " "xend %d, " "yend %d, "
                  "zend %d, " "offset %d, " "kernel_load_buffer_l0 %d, "
                  "kernel_load_buffer_l1 %d, " "bias_load_buffer_l0 %d, " "bias_load_buffer_l1 %d, "
                  "deform_offset_buffer %d, " "deform_output_buffer %d",
                  ::to_char(command), lane, buffer, xend, yend, zend, offset,
                  kernel_load_buffer_l0, kernel_load_buffer_l1, bias_load_buffer_l0, bias_load_buffer_l1,
                  deform_offset_buffer, deform_output_buffer);
      }
      return buf;
    }

    bool equals(const COMMAND_VPRO &ref) const {
      bool equal = true;
      equal &= (ref.command == command);
      equal &= (ref.lane == lane);
      equal &= (ref.buffer == buffer);
      equal &= (ref.xend == xend);
      equal &= (ref.yend == yend);
      equal &= (ref.zend == zend);
      equal &= (ref.offset == offset);
      equal &= (ref.shift_right == shift_right);
      equal &= (ref.kernel_load_buffer_l0 == kernel_load_buffer_l0);
      equal &= (ref.kernel_load_buffer_l1 == kernel_load_buffer_l1);
      equal &= (ref.bias_load_buffer_l0 == bias_load_buffer_l0);
      equal &= (ref.bias_load_buffer_l1 == bias_load_buffer_l1);
      equal &= (ref.deform_offset_buffer == deform_offset_buffer);
      equal &= (ref.deform_output_buffer == deform_output_buffer);
      equal &= (ref.nops == nops);
      return equal;
    }
  };
  static_assert(sizeof(COMMAND_VPRO) == 32, "Memory layout of packed struct");


  struct  COMMAND_DMA_PADDING {
    PAD_REDUCED pad{};
    uint8_t struct_padding[11]{}; //pad structure to 32 byte
    COMMAND_SEGMENT_TYPE type{COMMAND_SEGMENT_TYPE::DMA_SET_PADDING};
  };
  static_assert(sizeof(COMMAND_DMA_PADDING) == 32, "Memory layout of packed struct");

struct  COMMAND_SCATTER {
  int16_t index_shift{};
  int16_t xmin_fixed{};
  int16_t ymin_fixed{};
  uint32_t mm_addr_coords{};
  uint32_t mm_addr_features{};
  uint32_t mm_addr_grid{};
  uint16_t memcopy_size{};
  uint16_t use_vpro_dma{};
  uint8_t struct_padding[6]{}; // pad structure to 32 byte
  COMMAND_SEGMENT_TYPE type{COMMAND_SEGMENT_TYPE::SCATTER_CMD};

    const char* to_char() const {
      static char buf[1024];
      sprintf(buf, "index_shift %i, " "xmin_fixed %i, " "ymin_fixed %i"
              "mm_addr_coords %x" "mm_addr_features %x" "memcopy_size %i",
             index_shift, xmin_fixed, ymin_fixed, mm_addr_coords, mm_addr_features, memcopy_size);
      return buf;
    }

    bool equals(const COMMAND_SCATTER &ref) const {
      bool equal = true;
      equal &= (ref.index_shift == index_shift);
      equal &= (ref.xmin_fixed == xmin_fixed);
      equal &= (ref.ymin_fixed == ymin_fixed);
      equal &= (ref.mm_addr_coords == mm_addr_coords);
      equal &= (ref.mm_addr_features == mm_addr_features);
      equal &= (ref.mm_addr_grid == mm_addr_grid);
      equal &= (ref.memcopy_size == memcopy_size);
      return equal;
    }
  };
  static_assert(sizeof(COMMAND_SCATTER) == 32, "Memory layout of packed struct");


  /**
   * COMMAND (Segment) storage
   *  either DMA or VPRO Command
   */

  struct  TYPE {
    uint8_t struct_padding[31]{};
    COMMAND_SEGMENT_TYPE type{};
  };

  // C++ does not allow polymorphism and inheritance for packed structs
  struct  COMMAND_SEGMENT {
    COMMAND_SEGMENT() : type{} {}; // no auto-generated constructor for unions
    union {
      TYPE type;
      COMMAND_VPRO vpro;
      COMMAND_DMA dma;
      COMMAND_DMA_LOOP dma_loop;
      COMMAND_SCATTER scatter;
      COMMAND_DMA_PADDING dma_padding;
    } ;

    // alignment in risc !
    // if COMMAND_SEGMENT size is 29 [28 data + 1 type]
    //      elements inside (accessed by LH/LW, as well for uint8_t)
    //      cause MEM-stage exception due to misaligned access
    //      array of segments need to align those to word-boundarys (COMMAND_Segment size multiple of 4-Byte)
    // dma_direct_command aligned 32 byte to reduce dcache complexity

    //    dma: 28 x 8-bit
    //    vpro: 20 x 8-bit

    const char* to_char() const {
      static char buf[4096];
      switch(type.type) {
      case COMMAND_SEGMENT_TYPE::DMA_CMD:
        if (dma.direction != loop)
            sprintf(buf, "%s, %s", ::to_char(type.type), dma.to_char() );
        else
            sprintf(buf, "%s, %s", ::to_char(type.type), dma_loop.to_char() );
        break;
      case COMMAND_SEGMENT_TYPE::VPRO_CMD:
        sprintf(buf, "%s, %s", ::to_char(type.type), vpro.to_char()); break;
      case COMMAND_SEGMENT_TYPE::DMA_BLOCK:
        sprintf(buf, "%s, size %s", ::to_char(type.type), dma.unit_mask_to_char()); break;
      case COMMAND_SEGMENT_TYPE::DMA_SET_PADDING:
        sprintf(buf, "%s, %s", ::to_char(type.type), dma_padding.pad.to_char()); break;
      default:
        sprintf(buf, "%s"  , ::to_char(type.type)                ); break;
      }
      return buf;
    }

    bool equals(const COMMAND_SEGMENT &ref) const {
      // this is ugly, but memcmp might fail due to padding
      bool equal = (ref.type.type == type.type);
      if (type.type == COMMAND_SEGMENT_TYPE::DMA_CMD)
        return equal && dma.equals(ref.dma);
      if (type.type == COMMAND_SEGMENT_TYPE::VPRO_CMD)
        return equal && vpro.equals(ref.vpro);
      return equal;
    }
  };
  static_assert(sizeof(COMMAND_SEGMENT) == 32, "Memory layout of packed struct");


  /**
   * LAYER storage
   */
  struct  MM_IMAGE {
    uint32_t mm_base{}; // byte address
    uint32_t x{}; // number of payload elements per row
    uint32_t y{}; // number of payload elements per column
    uint32_t y_stride{}; // memory distance of two elements along dimension y
    uint32_t channels{};

    const char* to_char(const char *prefix = "") const {
      static char buf[1024];
      sprintf(buf, "%smm_base  0x%08" PRIx32 "" "%sx        %10" PRId32 "" "%sy        %10" PRId32 "" "%sy_stride %10" PRId32 "" "%schannels %10" PRId32 "",
              prefix, mm_base,
              prefix, x,
              prefix, y,
              prefix, y_stride, //x_postgap,
              prefix, channels);
      return buf;
    }

  };
  static_assert(sizeof(MM_IMAGE) == 20, "Memory layout of packed struct");

  // variable length list of COMMAND_SEGMENTS included at end of struct
  struct  LAYER {
    uint16_t in_channels{};
    uint16_t out_channels{};
    uint16_t number{};
    LAYERTYPE type{}; // 8 bit
    uint8_t dummy1{}; // align following uint16 to 16 bit

    uint16_t stride{}; // convolution stride
    uint16_t kernel_length{};
    uint32_t conv_groups{}; // groups of convolution (e.g. depthwise -> == in_channels)
    uint16_t dilation_rate_w{};
    uint16_t dilation_rate_h{};
    uint16_t seg_out_w{}; // segment output width/height of core operation, e.g. convolution for conv layers 
    uint16_t seg_out_h{};
    uint16_t seg_in_w{};
    uint16_t seg_in_h{};

    int16_t conv_result_shift_right{};
    int16_t relu_6_shift_left{};
    int16_t alpha_mulh_shift_right{}; // 2 bytes
    int16_t bias_shift_right{};
    int16_t store_shift_right{};
    int16_t elwise_1_left_shift{};
    int16_t elwise_0_left_shift{};
    int16_t elwise_1_right_shift{}; // FIXME unused
    int16_t elwise_0_right_shift{}; // FIXME unused
    uint16_t pool_stride{};

    ACTIVATION activation{NO_ACTIVATION}; // 8 bit
    uint8_t dummy2{}; // align following uint16 to 16 bit
    uint16_t alpha{};
    // avoid compiler-generated padding: shall be 32-bit aligned here
    PAD_REDUCED pad{}; // configure DMA unit per layer; COMMAND_DMA.pad_X switches padding on/off per transfer
    MM_IMAGE input{}; // debug only, not used by runtime
    MM_IMAGE output{}; // debug only, not used by runtime
    // 32-bit aligned here

    uint16_t axis{};
    uint16_t dynamic_shape{};
    uint16_t block_size{};

    // handshake with host processor
    uint8_t last_layer_using_input{false}; // set rv_input_parsed post-layer
    uint8_t first_layer_producing_output{false}; // set rv_output_ready post-layer

    uint16_t pool_size_w{};
    uint16_t pool_size_h{};
    uint16_t pool_size_ch{};

    uint16_t pool_stride_w{};
    uint16_t pool_stride_h{};
    uint16_t pool_stride_ch{};

    uint16_t pool_avg_shiftr{};

    // deformable conv
    uint16_t deform_static_offsets{};

    uint16_t deform_max_offset_x{};
    uint16_t deform_max_offset_y{};
 
    uint16_t parallel_outchannels_per_lane{1};   // for kernel_size == 1, added to improve performance
    uint16_t parallel_inchannels_per_lane{1};

    // Conv2d Transpose
    PAD_REDUCED subpixel_pad{}; // subpixel offset padding for conv2d_transpose kernel
    uint16_t input_pixels_w{};  // input pixels in w needed for computation
    uint16_t input_pixels_h{};  // input pixels in h needed for computation


    // < insert new fields in front of this comment / align_filler[] >
    uint8_t align_filler[20]; // shall occupy all space up to 32-bit aligned command_segments_count
    int32_t command_segments_count{};
    COMMAND_SEGMENT command_segments[];

    const char* to_char(bool legacy_compatibility = false) const {
      static char buf[4096];
      int offs = sprintf(buf,
              "in_channels             %d\n"
              "out_channels            %d\n"
              "number                  %d\n"
              "type                    %s\n"
              "stride                  %d\n"
              "kernel_length           %d\n"
              "dilation_rate_w         %d\n"
              "dilation_rate_h         %d\n"
              "seg_out_w               %d\n"
              "seg_out_h               %d\n"
              "seg_in_w                %d\n"
              "seg_in_h                %d\n"
              "conv_result_shift_right %d\n"
              "relu_6_shift_left       %d\n"
              "bias_shift_right        %d\n"
              "store_shift_right       %d\n"
              "elwise_1_left_shift     %d\n"
              "elwise_0_left_shift     %d\n"
              "pool_stride             %d\n"
              "pool_size_w             %d\n"  
              "pool_size_h             %d\n"  
              "pool_size_ch            %d\n"   
              "pool_stride_w           %d\n"  
              "pool_stride_h           %d\n"  
              "pool_stride_ch          %d\n"
              "pool_avg_shiftr         %d\n" 
              "deform_max_offset_x     %d\n"
              "deform_max_offset_y     %d\n"
              "deform_static_offsets   %d\n"
              "parallel_outchannels_per_lane   %d\n"
              "parallel_inchannels_per_lane    %d\n"
              "activation              %s\n"
              "pad   : %s\n"
              "input : %s\n",
              in_channels,
              out_channels,
              number,
              ::to_char(type),
              stride,
              kernel_length,
              dilation_rate_w,
              dilation_rate_h,
              seg_out_w,
              seg_out_h,
              seg_in_w,
              seg_in_h,
              conv_result_shift_right,
              relu_6_shift_left,
              bias_shift_right,
              store_shift_right,
              elwise_1_left_shift,
              elwise_0_left_shift,
              pool_stride,
              pool_size_w, 
              pool_size_h,  
              pool_size_ch,  
              pool_stride_w,  
              pool_stride_h,  
              pool_stride_ch,
              pool_avg_shiftr,
              deform_max_offset_x,
              deform_max_offset_y,
              deform_static_offsets,
              parallel_outchannels_per_lane,
              parallel_inchannels_per_lane,
              ::to_char(activation),
              pad.to_char("  "),
              input.to_char("  "));
      assert(offs >= 0);
      offs += sprintf(buf+offs, "output: %s\n", output.to_char("  "));
      assert(offs >= 0);
      offs += sprintf(buf+offs,
                      "axis %d\n"
                      "dynamic_shape %d\n",
                      axis,
                      dynamic_shape);
      assert(offs >= 0);
      if (!legacy_compatibility) {
        offs += sprintf(buf+offs, "command_segments_count  %d\n", command_segments_count);
        assert(offs >= 0);
      }
      return buf;
    }

  };
  static_assert(sizeof(LAYER) % 32 == 0, "Memory layout of packed struct needs to be 32-byte aligned!");
  static_assert(offsetof(LAYER, align_filler) + sizeof(LAYER::align_filler) == offsetof(LAYER, command_segments_count), "Compiler-generate padding between align_filler and command_segments_count. Please increase align_filler.");
  // END based on cnn_struct_reduced.h g9114dd8

  constexpr static uint32_t net_magicword = 0xf3f67a81;

  // policy: store offsets instead of absolute pointers: relative addressing enables data relocation and is host-independent
  // avoid pointers/offsets when possible, instantiate instead
  // FIXME add VPRO config to NET for sanity checking
  struct  NET {
    uint32_t magicword{};
    uint32_t blobsize{};
    uint32_t reserved{};
    uint32_t layer_execlist_count{}; // number of entries in layer_execlist_offs
    uint32_t layer_execlist_offs{}; // ptr (offset relative to BIF_NET) to linear array of 32 bit layer indices to be executed
    uint32_t layer_count{};
    uint32_t bif_layer_offs[]; // variable number of elements at end of struct
  };

} // namespace BIF


#endif // BIF_H
