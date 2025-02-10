/*
 *  * Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
 *                    Technische Universitaet Braunschweig, Germany
 *                    www.tu-braunschweig.de/en/eis
 * 
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 * 
 */
// This file forces "make libnetgen" to compile netgen/*.h (if included in layers.h)

// Why this file?
// -> useful to check if everything under netgen/ compiles without building sth from nets/

// Problem:
// cmake only compiles *.cpp into libnetgen; headers not included in any .cpp will not be compiled
// There is no other .cpp in netgen/ that includes layers.h -> cmake does not compile some header files

#include "layers.h"
#include "base_net.h"
