# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
echo "[Init-Script] start"

cp ../init/example.png ../init/image_in.png

# x="224"
# y="224"

# convert ../init/example.png -resize "$x"x"$y"\! -quality 100 ../init/image_in.png

if [ ! -d "../exit/venv" ]; then
    ../exit/install.sh
fi

source ../exit/venv/bin/activate
python3 ../init/preprocessing.py
deactivate

# x="224"
# y="224"

# convert ../init/example.png -resize "$x"x"$y"\! -quality 100 ../init/image_in.png

# python3 ../init/img2bin.py ../init/image_in.png "$x" "$y" ../input/input 2 0
# python3 ../init/img2bin.py ../init/image_in.png "$x" "$y" ../input/input 2 1
# python3 ../init/img2bin.py ../init/image_in.png "$x" "$y" ../input/input 2 2

# cat ../input/input_0.bin ../input/input_1.bin ../input/input_2.bin > ../input/l000.bin

echo "[Init-Script] done"
