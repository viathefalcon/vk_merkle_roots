// Utils.h: declares/defines utility functions and templates
//

#ifndef _VKMR_UTILS_H_
#define _VKMR_UTILS_H_

// Includes
//

// C++ Standard Library Headers
#include <stdint.h>

namespace vkmr {

// Templates
//

// Evaluates to true if the argument is a power-of-2
template <typename T>
bool is_pow2(T arg) {

    // Count the number of set bits
    T counter = 0;
    for (T u = arg; u > 0U; u >>= 1) {
        if ((u & 1) == 1){
            counter++;
        }
    }
    return (counter == 1);
}

// Returns the largest power-of-2 less than or equal to the given value
template <typename T>
T largest_pow2_le(T limit) {

    T result = 1U;
    while (true) {
        const T nxt = (result << 1);
        if (nxt > limit){
            break;
        }
        result = nxt;
    }
    return result;
}

// Returns the binary logarithm of the given input
// (more specifically: log2 of the largest power of 2
// less than or equal to the input)
uint32_t ln2(uint32_t);

} // namespace vkmr
#endif // _VKMR_UTILS_H_
