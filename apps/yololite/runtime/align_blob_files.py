# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
import os

folder = "./bin/data/"

bin_files = [(folder + file) for file in os.listdir(folder) if
              os.path.isfile(os.path.join(folder, file)) and file.endswith(".bin")]

for file in bin_files:
    file_size = os.path.getsize(file)
    is_aligned = (file_size % 4) == 0
    if not is_aligned:
        while not ((file_size % 4) == 0):
            file_size += 1
        os.system("truncate -s " + str(file_size) + " " + file)