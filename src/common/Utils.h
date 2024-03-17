// Utils.h: declares utility types, functions, etc.
//

#if !defined (__VKMR_UTILS_H__)
#define __VKMR_UTILS_H__

// Includes
//

// C++ Standard Library Headers
#include <limits>
#include <string>
#include <sstream>

namespace vkmr {

// Templates
//

// Returns the maximum string length of a given (numeric) type
template <typename T>
::std::string::size_type max_string_length(void) {

    // Get the maximuim value and emit into a string
    ::std::ostringstream oss;
    oss << ::std::numeric_limits<T>::max();

    // Return the length of the string
    return oss.str( ).size( );
}

} // namespace vkmr

#endif // __VKMR_UTILS_H__
