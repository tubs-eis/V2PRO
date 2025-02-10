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

#include "yololite_net.h"


int main(int argc, char *argv[]) {
  YoloLiteNet *cnn = new YoloLiteNet();

  cnn->generateNet();

}
