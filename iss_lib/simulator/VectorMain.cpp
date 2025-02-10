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
//
// Created by gesper on 27.06.19.
//

#include "VectorMain.h"

VectorMain::VectorMain(int (*main_fkt)(int, char**), int& argc, char** argv)
    : main_fkt(main_fkt), argc(argc), argv(argv) {}
int VectorMain::doWork() {
    return main_fkt(argc, argv);
}
