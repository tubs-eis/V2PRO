# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
echo "[Exit-Script] start"
# in dir: $PWD"WD"
echo "empty"

#
# after storing data from mm this script is called (last action of simulator)
# can be used to convert data from bin to png ...
#
x="224"
y="224"
#python3 ../exit/bin2img.py ../data/output.bin "$x" "$y" ../data/output_image 2 1

echo "[Exit-Script] done"
