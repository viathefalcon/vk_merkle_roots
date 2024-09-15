// Utils.cpp: defines utility functions
//

// Includes
//

// Declarations
#include "Utils.h"

namespace vkmr {

uint32_t ln2(uint32_t arg) {

    // Look for an early out
    if (arg == 0){
        return 0xFFFFFFFF;
    }

    uint32_t counter = 0;
    while ((arg >>= 1) > 0){
        counter++;
    }
    return counter;
}

} // namespace vkmr
