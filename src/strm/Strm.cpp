// Strm.cpp: A simple program to accept string arguments on the command line and write them as
//           a length-delimited stream to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <cstring>

// Other headers
#if defined(_WIN32)
#include <winsock.h>
typedef u_long netlong_t;
#else
#include <arpa/inet.h>
typedef socklen_t netlong_t;
#endif

// Functions
//

// Gives the entry-point
int main(int argc, const char* argv[]) {

    // Loop through the arguments
    for (int i = 1; i < argc; ++i){
        // Get the length
        const auto arg = *(argv + i);
        const auto len = std::strlen( arg );

        // Write out as a 32-bit unsigned int
        // (in network-order)
        const size_t nitems = 1;
        const auto output = htonl( static_cast<netlong_t>( len ) );
        if (fwrite( &output, sizeof( output ), nitems, stdout ) < nitems){
            // Bail
            return 1;
        }

        // Write out the characters
        fwrite( arg, sizeof( char ), len, stdout );
    }
    fflush( stdout );
    return 0;
}
