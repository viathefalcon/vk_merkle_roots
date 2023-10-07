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
#include <cstring>
#include <vector>
#include <string>

// Other headers
#ifdef _WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"
#include "SHA-256plus.h"

// Functions
//

// Returns the next input string
std::string get(void) {

    // Read in the length
    u_long netlong = 0;
    std::vector<unsigned char> buffer;
    buffer.reserve( sizeof( netlong ) );
    do {
        const auto input = getchar( );
        if (input == EOF){
            break;
        }

        // Accumulate
        buffer.push_back( static_cast<unsigned char>( input ) );
    } while (buffer.size( ) < sizeof( netlong ));
    if (buffer.size( ) < sizeof( netlong )){
        // Bail
        return "";
    }

    // Get the bytes into the variable, and then convert
    // from network order
    std::memcpy( &netlong, buffer.data( ), sizeof( netlong ) );
    const auto length = static_cast<uint32_t>( ntohl( netlong ) );

    // Read in the string itself
    std::string str;
    str.reserve( static_cast<size_t>( length ) );
    do {
        const auto input = getchar( );
        if (input == EOF){
            break;
        }

        // Accumulate
        str.append( 1, static_cast<char>( input ) );
    } while (str.size( ) < static_cast<decltype(str)::size_type>( length ));
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
        const auto hashed = vkmr::cpu_sha256d( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;

        if (!mrc.Add( std::move( arg ) )){
            break;
        }
    }
    cout << "Root => " << print_bytes( mrc.Run( ) ).str( ) << endl;
#else
    for (int i = 1; i < argc; ++i) {
        const auto arg = std::string( argv[i] );
        const auto hashed = vkmr::cpu_sha256d( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;
    }
#endif

#if defined (VULKAN_SUPPORT)
    vkSha256( argc - 1, argv + 1 );
#endif
    return result;
}
