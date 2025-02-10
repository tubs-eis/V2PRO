# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#1
num = int(input("Enter a number : "))
largest_divisor = 0

#2
for i in range(2, num):
    #3
    if num % i == 0:
        #4
        print("Divider: ", i)
        largest_divisor = i

#5
print("Largest divisor of {} is {}".format(num,largest_divisor))
