# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
import numpy as np


# Returns the number of bits necessary to represent an integer in binary, 2-complement format
def bit_width(a, b):
    if a > b:
        min_value = b
        max_value = a
    else:
        min_value = a
        max_value = b

    i = 0
    while max_neg_nint[i] > min_value or max_pos_nint[i] < max_value:
        i += 1

    # value = max(abs(min_value), abs(max_value)+1)
    # width = int(math.ceil(math.log2(value))+1)
    # print("min: ", min_value, ", max: ", max_value, " -> req bit width: ", width)
    return i


max_precision = 24
# maximal number with only fraction bits
max_fractions = []
for i in range(max_precision + 1):
    if i == 0:
        max_fractions.append(0)
        continue
    max_fractions.append(max_fractions[i - 1] + 1 / (2 ** i))

# maximal pos number with only integer bits
max_pos = []
for i in range(max_precision + 1):
    max_pos.append(2 ** (i - 1) - 1)

# maximal neg number with only integer bits
max_neg = []
for i in range(max_precision + 1):
    max_neg.append(-2 ** (i - 1))

max_pos_nint = []
for i in range(max_precision + 1):
    max_pos_nint.append(max_pos[i] + max_fractions[max_precision - i])

max_neg_nint = max_neg


# print("Fixpoint Format and related Min-/Max-values:")
# for i in range(max_precision+1):
#     print("fpf", i, ".", (max_precision-i), ": \t[min: ", max_neg_nint[i], ", \t max: ", max_pos_nint[i], "\t]" )


def swap32(x):
    return (((x << 24) & 0xFF000000) |
            ((x << 8) & 0x00FF0000) |
            ((x >> 8) & 0x0000FF00) |
            ((x >> 24) & 0x000000FF))


def get_fpf(array, max_size=16):
    """Determines the fpf for a given array
    Args:
        array: the data (python's float/double)
        max_size: limits the fractional bit length (fixed point format)
    Returns:
        A touple of (integer, fractional) - bits
    """
    minv = np.min(array)
    maxv = np.max(array)
    width = bit_width(minv, maxv)
    return (width, max_size - width)


def float_to_fpf(array, fpf):
    """Converts a given array to a given fixed point format
    Args:
        array: the data (python's float/double)
        fpf: A touple of fixed point format (integer, fractional) - bits
    Returns:
        The converted array as np.int32
    """
    # if fpf[0] + fpf[1] == 16:
    #     print("Converting float to 16-bit fpf")
    # if fpf[0] + fpf[1] == 24:
    #     print("Converting float to 24-bit fpf")
    return np.int32(array * np.left_shift(1, fpf[1]))


def fpf_to_float(array, fpf):
    """Converts a given array in fixed point format back to np.float32
    Args:
        array: the data (np.int32 or similar)
        fpf: A touple of fixed point format (integer, fractional) - bits
    Returns:
        The converted array as np.float32
    """
    return np.divide(np.float32(array), np.left_shift(1, fpf[1]))


def quantize(array, width):
    """Converts a given array to fixed point format limited by the specified width
    Args:
        array: the data (np.float32 or similar)
        width: maximum fixed point bits
    Returns:
        The converted array as np.float32 (Float!)
    """
    fpf = get_fpf(array, width)
    data_fpf = float_to_fpf(array, fpf)
    return fpf_to_float(data_fpf, fpf)  # continue with fixpoint values
