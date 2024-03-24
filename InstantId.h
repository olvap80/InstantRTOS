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

class Id16{};
class Id32{};

//TODO: helpers for unique ID, ID allocation, etc
