// Debug.cpp: defines the functions for debug output
//

// Windows Headers
#if defined (_WIN32)
// Windows Headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

// C++ Standard Library Headers
#if !defined (_DEBUG)
#include <iostream>
#endif
#include <algorithm>

// Declarations
#include "Debug.h"

// Functions
//

std::ostringstream print_bits_to_oss(const unsigned char* ptr, unsigned count){

	std::ostringstream oss;
	auto p = const_cast<unsigned char*>( ptr );
	for (decltype( count ) x = 0; x < count; ++x){
		const auto u = *(p++);
		int i = 7;
		do {
			auto j = (1 << i--);
			oss << ((j & static_cast<decltype( j )>( u )) ? '1' : '0');
		} while (i >= 0);
	}
	return oss;
}

std::ostringstream print_bytes(const std::string& s) {

	std::ostringstream oss;
	std::for_each( s.cbegin( ), s.cend( ), [&oss](const std::string::value_type& v) {
		const auto u = static_cast<unsigned char>( v );
		oss << std::hex << std::setw( 2 ) << std::setfill( '0' ) << static_cast<int>( u ) << std::dec;
	} );
	return oss;
}

void debug_print(const char* ptr, unsigned size) {

	if (size){
		std::ostringstream oss;
		oss << "\"" << std::string( ptr, size ) << "\":" << std::endl;
		OutputDebugStringStream( oss );
	}
}

void debug_print_bits(const unsigned char* ptr, unsigned count){

	const auto oss = print_bits_to_oss( ptr, count );
	OutputDebugStringStream( oss );
}

void debug_print_bytes(const std::string& s) {

	auto oss = print_bytes( s );
	oss << std::endl;
	OutputDebugStringStream( oss );
}

void debug_print_label(const std::string& s) {
	PrintOutString( s );
}
