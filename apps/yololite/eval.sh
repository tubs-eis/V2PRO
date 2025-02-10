# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 

#UNITS=8
#CLUSTERS=1

for CLUSTERS in {1..8}
do
  for UNITS in {1..8}
  do
    make CLUSTERS="${CLUSTERS}" UNITS="${UNITS}"

    cd nets/yololite/generated || exit
    scp *blob.bin aldec:/home/xilinx/EIS-V_bin/cnn_rework/data/
    cd - || exit

    cd runtime || exit
    make CLUSTERS="${CLUSTERS}" UNITS="${UNITS}" all

    cd bin || exit
    scp main.bin aldec:/home/xilinx/EIS-V_bin/cnn_rework/
    cd ../../ || exit

    ssh aldec "source /etc/profile; source .profile; source .bashrc; cd python_scripts/gesper; sudo -E python3 cnn.py; mv 8c8u.log ${CLUSTERS}c${UNITS}u.log"

    scp aldec:/home/xilinx/python_scripts/gesper/${CLUSTERS}c${UNITS}u.log /home/gesper/repositories/gesper-documents/papers/07.2022-PositionPaper_VPRO_EISV/scripts/logs/
  done
done
