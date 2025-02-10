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
// Created by gesper on 24.06.22.
//

#ifndef CONV2DADD_MATHHELPER_H
#define CONV2DADD_MATHHELPER_H

//#include <bits/stdc++.h>
#include <algorithm>

using namespace std;
// Function to find gcd multiple numbers
constexpr double findGCD(double a, double b, double c, double d, double e) {
    long at = long(a * 100);
    long bt = long(b * 100);
    long ct = long(c * 100);
    long dt = long(d * 100);
    long et = long(e * 100);
    auto result = std::gcd(std::gcd(std::gcd(std::gcd(at, bt), ct), dt), et);
    return double(result) / 100;
}

#endif  //CONV2DADD_MATHHELPER_H
