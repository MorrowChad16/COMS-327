This builds on HW 1.01 and 1.02

Added functionality is created with a "room heat map" which use dijstras algorithm
originating at the PC's location. The cost within the rooms is 1. We factor by 10 to 
get single digit values

The "whole heap map" is essentially the same except the hardness is / 85 to get a range between 1-3 for the cost and
the hardness check when adding cost to the map is eleminated

No special measures are needed to run the program. Simply run --save, --load, --load--save and it will print the heat maps

The print functions for both are similar they read in the arrays and print only the heat signatures, replacing empty space with ' ' and walls with 'X'