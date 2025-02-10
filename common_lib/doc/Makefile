# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 


.PHONY: doc clean

doc:
	@echo "generating HTML doc ..."
	@doxygen Doxyfile
	@cp example.html html/.
	@echo "generated HTML doc"
	ln -s html/namespaceVPRO_1_1DIM3.html dim3.html

clean:
	@echo "cleaning html and latex folder"
	@rm -rf html latex
