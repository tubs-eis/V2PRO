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
// Created by gesper on 12.11.20.
//
#include <stddef.h>

//void __attribute__((noinline)) *memset_(void *dest, int32_t val, uint32_t n){
extern "C"
void *memset(void *dest, size_t val, size_t n) {
    char *ptr = (char *)dest;
    while (n-- > 0)
        *ptr++ = val;
    return dest;
}

//void __attribute__((noinline)) *memcpy_ (void *dest, const void *src, uint32_t len){
extern "C"
void *memcpy(void *dest, const void *src, size_t len){
    char *d = (char *)dest;
    const char *s = (char *)src;
    while (len--)
        *d++ = *s++;
    return dest;
}

// --------
// ERROR: requires cpu instruction "seb" sign extension of byte, which is currently not enabled!?
// --------
//extern "C"
//int memcmp(const void *str1, const void *str2, size_t count){
//    const char *s1 = (char *)str1;
//    const char *s2 = (char *)str2;
//    while (count-- > 0)
//    {
//        if (*s1++ != *s2++)
//            return s1[-1] < s2[-1] ? -1 : 1;
//    }
//    return 0;
//}

extern "C"
void *memmove (void *dest, const void *src, size_t len){
    char *d = (char *)dest;
    const char *s = (char *)src;
    if (d < s){
        while (len--)
            *d++ = *s++;
    } else {
        char *lasts = (char *)(s + (len-1));
        char *lastd = (char *)(d + (len-1));
        while (len--)
            *lastd-- = *lasts--;
    }
    return dest;
}
