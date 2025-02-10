# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#!/bin/bash
#
# installs dependecies for this project
# requires python, pip3, venv, git
#
# for pycharm:
# Environment: PYTHONUNBUFFERED=1;PYTHONPATH=./tensorflow/models/research:./tensorflow/models/research/slim

# create venv named venv first:
python3 -m venv ../exit/venv
# if not installed, try:
# virtualenv --python=python3 venv

# get tensorflow object_detection files
#git clone https://github.com/tensorflow/models.git tensorflow/

# install pip dependecies for tf,...
source ../exit/venv/bin/activate
pip3 install -r ../exit/requirements.txt

# compile object_detection
#cd tensorflow/models/research/
#protoc object_detection/protos/*.proto --python_out=.
#cd ../../..

echo "OBJECTDETECTLIB=`pwd`/tensorflow/models/research/" >> ../exit/venv/bin/activate
echo "export PYTHONPATH=\$PYTHONPATH:\$OBJECTDETECTLIB:\$OBJECTDETECTLIB/slim" >> ../exit/venv/bin/activate


