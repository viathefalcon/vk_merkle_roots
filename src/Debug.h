// Debug.h: declares the functions for debug output
//

#if !defined (__VK_MERKLE_ROOTS_DEBUG__)
#define __VK_MERKLE_ROOTS_DEBUG__

// Includes
//

// C++ Standard Library Headers
#include <string>
#include <sstream>

// Macros
//

#if defined (_DEBUG) && defined (_WIN32)
#define PrintOutString(str) OutputDebugStringA( (str).c_str( ) )
#else
#define PrintOutString(str) std::cout << str << std::endl;
#endif

#define OutputDebugStringStream(oss) PrintOutString( (oss).str( ) )

// Functions
//

std::ostringstream print_bytes(const std::string&);

void debug_print(const char*, unsigned);
void debug_print_bits(const unsigned char*, unsigned);
void debug_print_bits_and_bytes(const unsigned char*, size_t);
void debug_print_bits_and_bytes(const std::string&);
void debug_print_bytes(const std::string&);
void debug_print_label(const std::string&);

#endif // __VK_MERKLE_ROOTS_DEBUG__
