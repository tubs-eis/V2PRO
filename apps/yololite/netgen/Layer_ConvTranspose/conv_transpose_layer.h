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
#ifndef CONV_TRANSPOSE_H
#define CONV_TRANSPOSE_H

#include "Base/base_layer.h"
#include "Layer_Conv/conv_layer.h"

namespace CNN_LAYER {

class  Conv2DTranspose: public Conv2D 
{
//member
public:
    // subpixel offset padding
    BIF::PAD_REDUCED subpixel_padding; // algorithm-view: offset subpixel padding for stride
    uint16_t input_pixels_w;    // Number of actual input pixels per segment in w
    uint16_t input_pixels_h;    // Number of actual input pixels per segment in h

// methods
public:
    virtual std::string getLayerTypeName() { return "Conv2DTranspose"; }
    virtual LAYERTYPE getLayerType() { return LAYERTYPE::CONV2_TRANSPOSE; }

    virtual void computeOutputDim()
    {
        assert(stride > 1 && "Conv2DTranspose not designed for stride == 1; use Conv2D instead");
        assert(kernel_length >= stride); // kernel_length < stride does not really make sense because some pixels in the output does not depend on any inputs
        assert(padding_mode == PADDING_MODE::SAME || padding_mode == PADDING_MODE::VALID); // transpose_conv2d only working for same and valid padding
        assert(pre_zp.top == 0 && pre_zp.right == 0 && pre_zp.bottom == 0 && pre_zp.left == 0 && "fused ZeroPadding2D layer notimplemented for transposed conv");

        if (padding_mode == PADDING_MODE::VALID)
        {
            conv_out_dim.x = in_dim(0).x * stride + kernel_length - stride;
            conv_out_dim.y = in_dim(0).y * stride + kernel_length - stride;
            conv_out_dim.ch = out_dim.ch;
        }
        else if (padding_mode == PADDING_MODE::SAME)
        {
            conv_out_dim.x = in_dim(0).x * stride;
            conv_out_dim.y = in_dim(0).y * stride;
            conv_out_dim.ch = out_dim.ch;
        }
    }

    virtual void computeInputPadding() 
    {
        assert(padding_mode == PADDING_MODE::SAME || padding_mode == PADDING_MODE::VALID);
        
        unsigned int pad_x, pad_y;
        if (padding_mode == PADDING_MODE::VALID && kernel_length > 1)
        {
            //VALID PADDING
            // pad_x/pad_y are subpixel
            pad_x = (kernel_length - 1) * 2;
            pad_y = (kernel_length - 1) * 2;
        }
        else
        {
            //SAME PADDING
            // pad_x/pad_y are subpixel
            pad_x = kernel_length + stride - 2;
            pad_y = kernel_length + stride - 2;
        }

        // Splitting padding left/right and top/bottom
        // Here padding is actual pixels that needs to be added
        padding.algo.right = (pad_x / 2) / stride;
        padding.algo.left = (pad_x - (pad_x / 2)) / stride;
        padding.algo.bottom = (pad_y / 2) / stride;
        padding.algo.top = (pad_y - (pad_y / 2)) / stride;

        // Splitting padding left/right and top/bottom
        // Here padding is on subpixel level that needs to be added after padded whole pixels
        subpixel_padding.right = (pad_x / 2) % stride;
        subpixel_padding.left = (pad_x - (pad_x / 2)) % stride;
        subpixel_padding.bottom = (pad_y / 2) % stride;
        subpixel_padding.top = (pad_y - (pad_y / 2)) % stride;

        // printf("################### DEBUG ##################### \n");
        // printf("PADDING\n:");
        // printf("pad_x: %d\n", pad_x);
        // printf("pad_y: %d\n", pad_y);
        // printf("PIXEL: left: %d, top: %d, right: %d, bot: %d\n", padding.algo.left, padding.algo.top, padding.algo.right, padding.algo.bottom);
        // printf("SUBPIXEL: left: %d, top: %d, right: %d, bot: %d\n", subpixel_padding.left, subpixel_padding.top, subpixel_padding.right, subpixel_padding.bottom);
        // printf("\n############################################### \n");

        // crop padded feature map from convolution output -> algorithm-view output geometry
        out_dim.x = conv_out_dim.x - out_padding.left - out_padding.right; // PADDING_MODE::SAME: == in_dim(0).x * stride;
        out_dim.y = conv_out_dim.y - out_padding.top - out_padding.bottom; // PADDING_MODE::SAME: == in_dim(0).y * stride;
    }

    virtual void setSegmentDimensions();
    virtual void calcOutputMemLayout();

    virtual BIF::COMMAND_SEGMENT convVPRO(const CNN_LAYER::SEGMENT &segment, BUFFER &buffer, uint32_t lane_mask, BIF::COMMAND_VPRO &mem_layout);

    virtual void generateBifLayer(BIF::LAYER &bl) {
        Conv2D::generateBifLayer(bl);
        
        bl.subpixel_pad.left = subpixel_padding.left;
        bl.subpixel_pad.top = subpixel_padding.top;
        bl.subpixel_pad.right = subpixel_padding.right;
        bl.subpixel_pad.bottom = subpixel_padding.bottom;

        bl.input_pixels_w = input_pixels_w;
        bl.input_pixels_h = input_pixels_h;
    }

    virtual std::string getLayerInfoText() {
      std::stringstream ss;
      ss << Conv2D::getLayerInfoText();
      ss << "  subpixel_padding trbl " << subpixel_padding.top << ", " << subpixel_padding.right << ", " << subpixel_padding.bottom << ", " << subpixel_padding.left << "\n";
      return ss.str();
    }

protected:
  BIF::PAD_REDUCED out_padding{}; // FIXME this is never set! Does that mean there are broken cases or can it safely be removed?

}; // class Conv2DTransposed

} // namespace CNN_LAYER

#endif
