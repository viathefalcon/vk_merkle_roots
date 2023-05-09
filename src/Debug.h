// Debug.h: declares the functions for debug output
//

#if !defined (__VK_MERKLE_ROOTS_DEBUG__)
#define __VK_MERKLE_ROOTS_DEBUG__

// Includes
//

// C++ Standard Library Headers
#include <string>
#include <sstream>
#if !defined (_WIN32)
#include <iostream>
#endif

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
std::ostringstream print_bits_to_oss(const unsigned char*, unsigned);

void debug_print(const char*, unsigned);
void debug_print_bits(const unsigned char*, unsigned);
void debug_print_bytes(const std::string&);
void debug_print_label(const std::string&);

// Templates
//

template <typename T>
void debug_print_bits_and_bytes(const T* data, size_t count) {

	std::ostringstream oss;

    // For each (data) element..
    for (decltype(count) c = 0; c < count; ++c){
        const auto ptr = reinterpret_cast<const unsigned char*>( data + c );
        // For each byte..
        for (size_t t = 0; t < sizeof( T ); ++t){
            const auto p = (ptr + t);
            const auto bits = print_bits_to_oss( p, 1 );
            oss << bits.str( );
            oss << " (";
            oss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<int>( *p ) << std::dec;
            oss << ") ";
        }
        oss << std::endl;
    }

	OutputDebugStringStream( oss );
}
#endif // __VK_MERKLE_ROOTS_DEBUG__
