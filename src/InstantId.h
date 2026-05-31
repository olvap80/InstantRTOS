/** @file InstantId.h
 @brief Allow simple 3char and 6char compact readable identifiers to be packed
        as uint16_t or uint32_t, suitable for debugging in embedded
        to avoid creation of long strings and minimize CPU usage!
        There is no "build step" needed, everything fits into header!

(c) see https://github.com/olvap80/InstantRTOS


*/


//TODO: C++11 constexpr custom literals that turn strings to numbers
// _id16, _id32, _prefix32,

//NOTE: for uint16_t all the numbers up to 0x7FFF are treated as "just numbers"
//id16encode(uint16_t, char* buffer, int items_avail), 
//NOTE: for uint32_t all the numbers up to 0x3FFFFFFF are treated as "just numbers"
//id32encode(uint32_t, char* buffer, int items_avail)

/*
;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ 32 characters (consecutive ASCII)

Format                  Prefix   Bits Left    Contents
3 Letters               1        15 bits      3 letters (per user interpretation)
Number                  01       14 bits      Just number from 0 up to 2¹⁴ - 1 = 16,383 (user interpretation)
1 Letter + Num          001      13 bits      5-bit letter, 8-bit number 0–255 (user interpretation)
Reserved 1              0001     12 bits
Error code              000011   10 bits      Numerical error code (user interpretation)
Error as 2 Letters      000010   10 bits      5-bit letter + 5-bit letter (error for sure per user interpretation)  
2 Letters               000001	 10 bits      5-bit letter + 5-bit letter (small identifier per user interpretation)
Reserved 2              000000   10 bits  

Option 31 characters
<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ
and 31*31*31 + 31*31 + 256 = 31008 (everything fits into 15 bits! 2letters and 1 can be encoded)

59 + 31*31 = 1020 one or two letters fit into 10 bits 6 bits are for usage
int this way we can encode field name and type. !!!!!
Types to support uint8, 16, 32, int8, 16, 32, float, double

Format                  Prefix   Bits Left    Contents
1, 2, 3 Letters         1        15 bits      up to letters (per user interpretation)
Number                  01       14 bits      Just number from 0 up to 2¹⁴ - 1 = 16,383 (user interpretation)
1 Letter + Num          001      13 bits      5-bit letter, 8-bit number 0–255 (user interpretation)
Reserved 1              0001     12 bits
Error code              000011   10 bits      Numerical error code (user interpretation)
Error as 2 Letters      000010   10 bits      5-bit letter + 5-bit letter (error for sure per user interpretation)  
Reserved 2              000001   10 bits      
Reserved 3              000000   10 bits  

No numbers, just letters for identification
Format                  Prefix   Bits Left    Contents
1, 2, 3 Letters         1        15 bits      up to letters (per user interpretation) from 31008 + 1 to 32767 error codes
1 Letter + Num          01       14 bits      5-bit letter, 9-bit number 0–511 (user interpretation)
Error code              001      13 bits      Numerical error code (user interpretation)
???

=================
256+31*31+31*31*31+31*31*31*31+31*31*31*31*31+31*31*31*31*31*31 = 917087361
256+256*256+256*256*256+31*31*31*31+31*31*31*31*31+31*31*31*31*31*31 = 933899361

===================
Identifies only with type
128+128*128+128*128*128+64*31*31*31+31*31*31*31*31 = 32649439
2^32/32649439 = 131  (enough space for other 7 bits)

*/

class Id16{};
class Id32{};

//TODO: helpers for unique ID, ID allocation, etc
