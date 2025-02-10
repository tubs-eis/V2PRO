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
// Created by gesper on 20.07.23.
//

#include "base_layer.h"

using namespace CNN_LAYER;

void Layer::setWeights(std::vector<weight_t> &weights) {
    weights_packed = weights;
    weights_loaded = true;
}

// call after all parameters have been set and before using the layer
void Layer::processParams() {
    computeOutputDim();
    computeInputPadding();

    if (groups == GROUPS_UNSET)
        groups = 1; // default: each output channel depends on all input channels; following asserts are always okay
    assert(out_dim.ch % groups == 0);
    for (int src_idx = 0; src_idx < (int)src_layers.size(); src_idx++) {
        assert(in_dim(src_idx).ch % groups == 0);
    }

    if (lm_lane_stride == STRIDE_UNSET){
        lm_lane_stride = VPRO_CFG::RF_SIZE * 1;
    }
};

std::string Layer::getDefaultWeightsFilename() {
    std::stringstream ss;
    ss << "weights/l" <<  std::setfill('0') << std::setw(3) << std::right << number << "_weights.bin";
    return ss.str();
}

// specify layer inputs
void Layer::addSrcLayers(std::initializer_list<Layer *> new_layers) {
    src_layers.insert(src_layers.end(), new_layers.begin(), new_layers.end());

    for (auto l: new_layers) {
        l->addDest(this);
    }
}

// algorithm-view geometry: derive out_dim.(x|y) from inputs and parameters
// implementation-view out_dim.mm.* will be set by setOutputMMAddr()
void Layer::computeOutputDim() {
    assert(!src_layers.empty() && "can not compute output dim without src layers");
    out_dim.x = in_dim(0).x; // default: output dimension = dimension of 1st source
    out_dim.y = in_dim(0).y; // default: output dimension = dimension of 1st source
}

// algorithm-view
void Layer::computeInputPadding() {
    padding.algo.top = 0;
    padding.algo.right = 0;
    padding.algo.bottom = 0;
    padding.algo.left = 0;
}

void Layer::computeDmaPadding() {
    // FIXME DMA padding should be per input (potentially different memory layout)
    assert(padding.enabled || (padding.algo.top == 0 && padding.algo.right == 0 && padding.algo.bottom == 0 && padding.algo.left == 0));

    padding.dma = padding.algo;

    if (padding.enabled) {
        // padding.algo specifies additional pixels around in_dim.(x|y)
        int algo_conv_in_dim_x = in_dim(0).x + padding.algo.left + padding.algo.right;

        // input feature map size loaded by segmentation
        // (DMA applies padding to this input feature map):
        int impl_conv_in_dim_x = (seg.num.x-1)*seg.in.x_stride + seg.in.w;

        int extra_impl_padding_x = impl_conv_in_dim_x - algo_conv_in_dim_x;

        // - if segment input size and stride do not fit into algo_conv_in_dim, impl_conv_in_dim becomes larger (garbage pixels)
        // - stride may discard right/bottom pixels from the padded input feature map
        // --> impl input size may be larger or smaller than algo input size

        // negative padding means: padding ends outside of segmented input feature map
        // not handled as required by DMA -> clip
        padding.dma.right = padding.algo.right + extra_impl_padding_x;

        int algo_conv_in_dim_y = in_dim(0).y + padding.algo.top + padding.algo.bottom;
        int impl_conv_in_dim_y = (seg.num.y-1)*seg.in.y_stride + seg.in.h;
        int extra_impl_padding_y = impl_conv_in_dim_y - algo_conv_in_dim_y;
        padding.dma.bottom = padding.algo.bottom + extra_impl_padding_y;
    }
    padding.dma.top = std::max(0, padding.dma.top);
    padding.dma.right = std::max(0, padding.dma.right);
    padding.dma.bottom = std::max(0, padding.dma.bottom);
    padding.dma.left = std::max(0, padding.dma.left);
}

void Layer::loadWeights(const std::string &path){
    if (!path.empty())
        weights_fname = path;
    if (weights_fname.empty())
        weights_fname = getDefaultWeightsFilename();

    std::ifstream fd(weights_fname, std::ios::ate | std::ios::binary);

    if (fd) { // weight file found, get number of weights from file size
        std::streamsize fsize = fd.tellg();
        int weights_count = fsize / sizeof(weight_t);
        weights_packed = std::vector<weight_t>(weights_count);

        fd.seekg(0, std::ios::beg);
        fd.read((char *) weights_packed.data(), weights_count*sizeof(weight_t));
        weights_loaded = true;
    }
    else { // if no weight file exists, assume the layer has zero weights
        weights_packed.clear();
        std::cout << "ERROR layer '" << getFullName() << "': loading weights from '" << weights_fname << "' failed!\n";
        // FIXME error handling
    }
    sanityCheckWeightsCount(weights_packed.size());
    fd.close();
}

// called by memory management
void Layer::setOutputMMAddr(mm_addr_type base_addr) {
    out_dim.mm.base = base_addr;

    // Set number of segments and their dimensions and main memory image of this layer's output
    setSegmentDimensions(); // seg_(out|in)_(w|h|num)

    // if setSegmentDimensions does not set seg.*_stride explicity, they default to segment width/height
    if (seg.in.x_stride == STRIDE_UNSET)
        seg.in.x_stride = seg.in.w;
    if (seg.in.y_stride == STRIDE_UNSET)
        seg.in.y_stride = seg.in.h;
    if (seg.out.x_stride == STRIDE_UNSET)
        seg.out.x_stride = seg.out.w;
    if (seg.out.y_stride == STRIDE_UNSET)
        seg.out.y_stride = seg.out.h;

    computeDmaPadding();
    setOutputMemDimensions(); // out_dim.mm.(x|y)
    calcOutputMemLayout(); // derive other out_dim.mm.* fields from out_dim.mm.(x|y)


    out_dim.mm.layout_known = true;
}

// tell memory management how much output space is required (may be larger than actual payload data)
mm_size_type Layer::getOutputMMSize() {
    assert(out_dim.mm.layout_known && "getOutputSize relies on the size determined by calcOutputMemLayout(). Call setOutputMMAddr() first!");
    return out_dim.mm.size;
}

mm_size_type Layer::getWeightsMMSize() { // in bytes
    return weights_packed.size() * sizeof(weight_t);
}

// return value: does this layer have a BIF::LAYER representation? Input layers do not.
void Layer::generateBifLayer(BIF::LAYER &bl) {
    assert("Layer must have at least one input layer!" && !src_layers.empty());

    bl.in_channels = in_dim(0).ch;
    bl.out_channels = out_dim.ch;
    bl.number = number;
    bl.type = getLayerType();
    bl.dynamic_shape = use_dynamic_shape;

    bl.seg_out_w = seg.out.w;
    bl.seg_out_h = seg.out.h;
    bl.seg_in_w = seg.in.w;
    bl.seg_in_h = seg.in.h;

    bl.pad = padding.dma;

    // Input
    //TODO: currently the input dimensions of the first layer are always used
    bl.input.mm_base = in_dim(0).mm.channel_base[0];
    bl.input.x = (uint32_t) in_dim(0).x;
    bl.input.y = (uint32_t) in_dim(0).y;
    bl.input.y_stride = (uint32_t) in_dim(0).mm.x;
    bl.input.channels = (uint32_t) in_dim(0).ch;

    // Output
    bl.output.mm_base = out_dim.mm.channel_base[0];
    bl.output.x = (uint32_t) out_dim.x;
    bl.output.y = (uint32_t) out_dim.y;
    bl.output.y_stride = (uint32_t) out_dim.mm.x;
    bl.output.channels = (uint32_t) out_dim.ch;

    // handshake with host processor: input no longer required, wait until we can overwrite output
    bl.last_layer_using_input = last_layer_using_input;
    bl.first_layer_producing_output = first_layer_producing_output;

    bl.parallel_outchannels_per_lane = parallel_outchannels_per_lane;
    bl.parallel_inchannels_per_lane = parallel_inchannels_per_lane;
}

std::vector<BIF::COMMAND_SEGMENT> &Layer::generateCommandSegments() {
    // fill commands vector
    //FIXME: this is done in generateCommands() -> refactor this
    generateSegments();
    generateCommands();
    compressCommands();

    return commands;
}

bool Layer::sanityCheckWeightsCount(int weights_count) {
    int expected_count = expectedWeightCount();
    bool check_passed = weights_count == expected_count;
    if (!check_passed) {
        std::cout << "Warning: got " << weights_count << " weights for layer " << getFullName()
                  << ", but expected " << expected_count << "!\n";
    }
    return check_passed;
}

// debug
std::string Layer::getFullName() {
    return "'" + name + "' (" + std::to_string(number) + ")";
}

// characters 'I' and/or 'O' if this layer is a CNN input and/or output
std::string Layer::getIoStr(bool pre_space, bool post_space) {
    std::string io = "";
    if (is_input_layer)
        io += "I";
    if (out_is_result)
        io += "O";
    if (!io.empty()) {
        if (pre_space)
            io = " " + io;
        if (post_space)
            io += " ";
    }
    return io;
}

std::string Layer::segDimStr() {
    return "wh " + std::to_string(seg.num.x) + " x " + std::to_string(seg.num.y) + " = " +
           std::to_string(seg.num.x*seg.num.y) + " segments\n" +
           "input segment shape:  wh " + std::to_string(seg.in.w)  + " x " + std::to_string(seg.in.h) + "\n"
                                                                                                        "output segment shape: wh " + std::to_string(seg.out.w) + " x " + std::to_string(seg.out.w);
}

std::string Layer::cmdCntStr() {
    return "Command counts for layer " + getFullName() + "\n" +
           "Total: " + std::to_string(commands.size()) + "\n" +
           "Sync commands: " + std::to_string(cmd_cnt.sync) + "\n" +
           "VPRO commands; " + std::to_string(cmd_cnt.vpro) + "\n" +
           "DMA commands; " + std::to_string(cmd_cnt.dma) + "\n";
}

std::string Layer::getLayerInfoText() {
    std::stringstream ss;
    ss << "== Layer " << getFullName() << getIoStr(true, false) << ", class " << getLayerTypeName() << "\n";

    if (dest_layers.empty() && !out_is_result)
        ss << "!! WARNING: this is a leaf layer but not a CNN output (result goes nowhere)!\n";

    if (last_layer_using_input)
        ss << "  Last layer reading CNN input\n";

    if (first_layer_producing_output)
        ss << "  First layer writing CNN output\n";

    ss << "  src :";
    std::string sep {" "};
    for (auto sl: src_layers) {
        ss << sep << "{ " << sl->getFullName() << ": " << sl->out_dim.algoMmStr() << " }";
        sep = ", ";
    }
    if (src_layers.empty()) {
        ss << " -";
    }
    ss << "\n";

    ss << "  dest:";
    sep = " ";
    for (auto l: dest_layers) {
        ss << sep << "{ " << l->getFullName() << " }";
        sep = ", ";
    }
    if (out_is_result)
        ss << sep << "<Result>";
    else if (dest_layers.empty()) {
        ss << " -";
    }
    ss << " : " << out_dim.detailStr() << "\n";

    ss << "  padding: algo trbl " << padding.algo.top << ", " << padding.algo.right << ", " << padding.algo.bottom << ", " << padding.algo.left
       << "; dma " << padding.dma.top << ", " << padding.dma.right << ", " << padding.dma.bottom << ", " << padding.dma.left << "\n";

    ss << "  segmentation: count wh " << seg.num.x << "x" << seg.num.y
       << ", in: size " << seg.in.w << "x" << seg.in.h << ", stride " << seg.in.x_stride << "x" << seg.in.y_stride
       << ", out: size " << seg.out.w << "x" << seg.out.h << ", stride " << seg.out.x_stride << "x" << seg.out.y_stride
       << "\n";

    ss << "  parallel channels per lane: in " << parallel_inchannels_per_lane << ", out " << parallel_outchannels_per_lane << "\n";
    
    ss << "  groups: " << groups << " ->";
    for (int src_idx = 0; src_idx < (int)src_layers.size(); src_idx++) {
        ss << " in_group_len(" << src_idx << "): " << in_dim(src_idx).ch / groups;
    }
    ss << ", out_group_len: " << (out_dim.ch / groups);      ss << "\n";

    if (!weights_fname.empty())
        ss << "  weights: '" << weights_fname << "'" << (weights_loaded ? " (loaded)" : " (NOT loaded)")
           << ", int16[" << weights_packed.size() << "]\n";
    return ss.str();
};

std::string Layer::getSegmentsString() {
    std::stringstream ss;
    for (unsigned int i = 0; i < segments.size(); i++) {
        ss << "SEGMENT " << i << "\n";
        ss << segments[i]->to_string();
    }
    return ss.str();
}

std::string Layer::getSegmentsShortString() {
    if (segments.size() > 10000 && !layercfg.force_segment_dump)
        return "<details disabled for > 10000 segments; can be forced via layercfg.force_segment_dump>\n";
    std::stringstream ss;
    for (unsigned int si = 0; si < segments.size(); si++) {
        int set     = (si / (VPRO_CFG::CLUSTERS*VPRO_CFG::UNITS*VPRO_CFG::LANES*parallel_outchannels_per_lane));
        int cluster = (si / (            VPRO_CFG::UNITS*VPRO_CFG::LANES*parallel_outchannels_per_lane)) % VPRO_CFG::CLUSTERS;
        int unit    = (si / (                     VPRO_CFG::LANES*parallel_outchannels_per_lane)) % VPRO_CFG::UNITS;
        int lane    = (si / (                              parallel_outchannels_per_lane)) % VPRO_CFG::LANES;
        int ch      = (si                                                                ) % parallel_outchannels_per_lane;
        ss << "SEGMENT " << std::setw(5) << si << " (s" << std::setw(2) << set << "c" << cluster << "u" << unit << "l" << lane << "." << std::setw(2) << ch << "): " << segments[si]->to_short_string() << "\n";
    }
    return ss.str();
}

std::string Layer::getLaneUsageString() {
    std::stringstream ss;
    int segs_per_set = VPRO_CFG::parallel_Lanes * parallel_outchannels_per_lane;
    ss << "  cluster ";
    for (int si = 0; si < segs_per_set; si++) {
        ss << (si / (            VPRO_CFG::UNITS*VPRO_CFG::LANES*parallel_outchannels_per_lane)) % VPRO_CFG::CLUSTERS;
    }
    ss << "\n     unit ";
    for (int si = 0; si < segs_per_set; si++) {
        ss << (si / (                     VPRO_CFG::LANES*parallel_outchannels_per_lane)) % VPRO_CFG::UNITS;
    }
    ss << "\n     lane ";
    for (int si = 0; si < segs_per_set; si++) {
        ss << (si / (                              parallel_outchannels_per_lane)) % VPRO_CFG::LANES;
    }
    ss << "\n  channel ";
    for (int si = 0; si < segs_per_set; si++) {
        ss << (si                                                                ) % parallel_outchannels_per_lane % 10;
    }
    ss << "\n";
    for (unsigned int i = 0; i < segments.size(); i++) {
        if (!(i % segs_per_set))
            ss << "set " << std::setw(4) << i / segs_per_set << " [";
        SEGMENT *s = segments[i];
        if (s->dummy)
            ss << "-";
        else if (s->isFirst && s->isLast)
            ss << "1";
        else if (s->isFirst)
            ss << "F";
        else if (s->isLast)
            ss << "L";
        else
            ss << "x";
        if (!((i+1) % segs_per_set))
            ss << "]\n";
    }
    return ss.str();
}  

std::string Layer::getCommandsString() {
    std::stringstream ss;
    for (unsigned int i = 0; i < commands.size(); i++) {
        //ss << "COMMAND " << i << "\n";
      ss << "[" << std::setw(5) << i << "] "<< commands[i].to_char() << "\n";
    }
    return ss.str();
}
