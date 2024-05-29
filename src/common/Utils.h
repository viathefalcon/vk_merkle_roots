// Utils.h: declares/defines utility functions
//

#ifndef _VKMR_UTILS_H_
#define _VKMR_UTILS_H_

// Includes
//

// Local Project Headers
#include "SHA-256defs.h"

#ifndef GL_core_profile
namespace vkmr {
#endif

// Functions
//

#ifndef GL_core_profile
// Evaluates to true if the argument is a power-of-2
bool is_pow2(uint arg) {

    // Count the number of set bits
    uint counter = 0;
    for (uint u = arg; u > 0U; u >>= 1) {
        if ((u & 1) == 1){
            counter++;
        }
    }
    return (counter == 1);
}
#endif

// Returns the largest power-of-2 less than or equal to the given value
uint largest_pow2_le(uint limit) {

    uint result = 1U;
    while (true) {
        const uint nxt = (result << 1);
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
uint ln2(uint arg) {

    // Look for an early out
    if (arg == 0){
        return 0xFFFFFFFF;
    }

    uint counter = 0;
    while ((arg >>= 1) > 0){
        counter++;
    }
    return counter;
}

#ifndef GL_core_profile
} // namespace vkmr
#endif
#endif // _VKMR_UTILS_H_
