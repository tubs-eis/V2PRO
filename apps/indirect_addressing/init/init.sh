# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
echo "[Init-Script] start"
# in dir: $PWD"

echo "empty"

#
# before loading data to mm this script is called (first action of simulator)
# can be used to convert data to binary ...
#
# e.g.:
#x="224"
#y="224"
#convert ../data/image_small.png -resize "$x"x"$y"\> ../data/image_small_crop.png
#python3 ../../../helper/main_memory_file_generator/img2bin.py ../data/image_small_crop.png "$x" "$y" ../data/input 2 0

echo "[Init-Script] done"
