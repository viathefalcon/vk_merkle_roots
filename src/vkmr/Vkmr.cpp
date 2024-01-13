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
#if defined(_WIN32)
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
#if defined(_WIN32)
    u_long netlong = 0;
#else
    socklen_t netlong = 0;
#endif
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

    vkmr::CpuSha256D mrc;
#if defined (VULKAN_SUPPORT)
    cout << "Initializing.." << endl;
    vkmr::VkSha256D instances;
#endif

    // Loop over the inputs
    while (true) {
        const auto arg = get( );
        if (arg.empty( )){
            break;
        }

#if defined (VULKAN_SUPPORT)
        // Copy into each of the Vulkan-based instances
        instances.ForEach( [&](vkmr::VkSha256D::Instance& instance) {
            instance.Add( arg );
        } );
#endif

        const auto hashed = vkmr::cpu_sha256d( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;

        if (!mrc.Add( std::move( arg ) )){
            break;
        }
    }
    cout << "Root => " << print_bytes( mrc.Root( ) ).str( ) << endl;
#if defined (VULKAN_SUPPORT)
    instances.ForEach( [&](vkmr::VkSha256D::Instance& instance) {
        cout << instance.Name( ) << ":" << endl;
        instance.Root( );
    } );
#endif
    return 0;
}
