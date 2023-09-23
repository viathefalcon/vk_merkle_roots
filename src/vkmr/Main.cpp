// Main.cpp: defines the entry point for the application
//

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

// Includes
//

// C Standard Library Headers
#include <stdio.h>

// C++ Standard Headers
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"
#include "SHA-256plus.h"

// Functions
//

// Returns the next input string
std::string get(void) {

    // Read in the length
    uint32_t length = 0;
    size_t counter = 0;
    do {
        const auto input = getchar( );
        if (input == EOF){
            break;
        }

        // Accumulate
        const auto byte = static_cast<unsigned char>( input );
        memcpy( reinterpret_cast<unsigned char*>( &length ) + counter, &byte, sizeof( byte ) );

        // Advance
        counter++;
    } while (counter < sizeof( length ));
    if (counter < sizeof( length )){
        // Bail
        return "";
    }

    // Read in the string itself
    char* buffer = new (std::nothrow) char[length + 1];
    counter = 0;
    do {
        const auto input = getchar( );
        if (input == EOF){
            break;
        }

        // Accumulate
        *(buffer + (counter++)) = static_cast<char>( input );
    } while (counter < static_cast<decltype(counter)>( length ));

    // Return
    std::string str( buffer, counter );
    delete[] buffer;
    return str;
}

// Gives the entry-point for the application
int main(int argc, const char* argv[]) {

    int result = 0;
#if defined (_MACOS_64_)
    while (true) {
        const auto arg = get( );
        if (arg.empty( )){
            break;
        }
        const auto hashed = cpu_sha256( arg );
        std::cout << arg << " >> " << print_bytes( hashed ).str( ) << std::endl;
    }
#else
    for (int i = 1; i < argc; ++i) {
        const auto arg = std::string( argv[i] );
        const auto hashed = cpu_sha256( arg );
        std::cout << arg << " >> " << print_bytes( hashed ).str( ) << std::endl;
    }
#endif

#if defined (VULKAN_SUPPORT)
    vkSha256( argc - 1, argv + 1 );
#endif
    return result;
}
