# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
from fp_helper import get_fpf, float_to_fpf
import cv2
import numpy as np

setting_fpf_input = 14
output_file = "../input/l000.bin"

frame = cv2.imread('../init/image_in.png')

hsize = frame.shape[1] 
vsize = frame.shape[0] 

# cut rectangle ROI from image
frame = frame[0:vsize,int((hsize-vsize)/2):int((hsize-vsize)/2+vsize),0:3]

frame_small = cv2.resize(frame, (224, 224), interpolation=cv2.INTER_NEAREST)

inpImg = frame_small.copy().astype(np.float32)
image_bgr = cv2.cvtColor(inpImg, cv2.COLOR_RGB2BGR)
image_resized = cv2.resize(image_bgr, (224, 224), interpolation=cv2.INTER_NEAREST)
image_np_expanded = np.expand_dims(image_resized, axis=0)
image_np_expanded = np.array(image_np_expanded, dtype='f')
image_np_expanded -= np.min(image_np_expanded)  # 0 to max
image_np_expanded /= np.max(image_np_expanded)  # 0 to 1

input_fpf = get_fpf(image_np_expanded, setting_fpf_input)
data_input_fixp = float_to_fpf(image_np_expanded, input_fpf)
transfer_frame = data_input_fixp[0, :, :, :].astype(np.int16)

input_buffer = np.zeros((3*224*224), dtype=np.int16)
input_channel_0 = transfer_frame[0:224, 0:224, 0].flatten()
input_channel_1 = transfer_frame[0:224, 0:224, 1].flatten()
input_channel_2 = transfer_frame[0:224, 0:224, 2].flatten()

np.copyto(input_buffer[0:224*224], input_channel_0)
np.copyto(input_buffer[224*224:2*224*224], input_channel_1)
np.copyto(input_buffer[2*224*224:3*224*224], input_channel_1)
    
f = open(output_file, "wb")
f.write(input_buffer)
f.close()