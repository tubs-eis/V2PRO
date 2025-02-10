# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
echo "[Exit-Script] start"

cd ../exit
RED='\033[0;31m'
NC='\033[0m' # No Color
source venv/bin/activate
python3 postprocessing.py
deactivate

echo "[Exit-Script] done"
