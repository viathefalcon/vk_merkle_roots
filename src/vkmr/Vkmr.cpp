// Vkmr.cpp: defines the entry point for the application
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
    std::string str;
    str.reserve( static_cast<size_t>( length ) );
    counter = 0;
    do {
        const auto input = getchar( );
        if (input == EOF){
            break;
        }

        // Accumulate
        str.append( 1, static_cast<char>( input ) );
    } while (counter < static_cast<decltype(counter)>( length ));
    return str;
}

// Gives the entry-point for the application
int main(int argc, const char* argv[]) {

    using std::cout;
    using std::endl;

    int result = 0;
#if defined (_MACOS_64_)
    vkmr::CpuSha256D mrc;
    while (true) {
        const auto arg = get( );
        if (arg.empty( )){
            break;
        }
        const auto hashed = vkmr::cpu_sha256( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;

        if (!mrc.Add( std::move( arg ) )){
            break;
        }
    }
    cout << "Root => " << print_bytes( mrc.Run( ) ).str( ) << endl;
#else
    for (int i = 1; i < argc; ++i) {
        const auto arg = std::string( argv[i] );
        const auto hashed = cpu_sha256( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;
    }
#endif

#if defined (VULKAN_SUPPORT)
    vkSha256( argc - 1, argv + 1 );
#endif
    return result;
}
