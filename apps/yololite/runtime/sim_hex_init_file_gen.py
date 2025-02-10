# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#!/usr/bin/env python3
# coding: utf-8

import os, sys
import argparse
from datetime import date, datetime
import numpy as np


parser = argparse.ArgumentParser(description='Convert ISS input.cfg to ModelSim Required HW SIM Memory Init file',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-init', '--init_cfg', default='bin/data/input.cfg',
                    help='the init file from ISS')
parser.add_argument('-output', '--output_cfg', default="bin/main.mm_init.bin",
                    help='the generated init file for HW Sim')
args = parser.parse_args()

data_dir = os.getcwd() + "/bin/data"

with open(args.output_cfg, "w") as output:
    for line in open(args.init_cfg, "r"):
        line = line.strip()
        if line.startswith('#'):
            continue
        filename, address = line.split(' ')[:2]
        filename = data_dir + "/" + filename.split("/")[-1]
        address = str(int(int(address, 0) / 2))
        output.write(filename)
        output.write("\n")
        output.write(address)
        output.write("\n")

