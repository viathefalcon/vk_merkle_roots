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

    T result = 1;
    while (true) {
        const T nxt = (result << 1);
        if (nxt > limit){
            break;
        }
        result = nxt;
    }
    return result;
}

// Returns the lowest common multiple of the given values
template <typename T>
T lowest_common_multiple(T lhs, T rhs) {
    
    // Figure out the smaller of the args
    T smaller = lhs, larger = rhs;
    if (smaller > larger){
        smaller = rhs;
        larger = lhs;
    }

    // Now, loop until we have a product of the smaller which
    // can be divided by the larger without a remainder
    T result, multiplicand = 0;
    do {
        result = smaller * (++multiplicand);
        if ((result % larger) == 0){
            break;
        }
    } while (multiplicand < larger);
    return result;
}

// Returns the binary logarithm of the given input
// (more specifically: log2 of the largest power of 2
// less than or equal to the input)
uint32_t ln2(uint32_t);

} // namespace vkmr
#endif // _VKMR_UTILS_H_
