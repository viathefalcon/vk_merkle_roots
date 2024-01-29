// Rndm.cpp: A simple program to generate strings w/random content
//           and write them to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <cstdlib>
#include <iostream>
#include <algorithm>

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

    // Fail safe
    if (argc < 2){
        std::cerr << "Usage: rndm [number of outputs] [max output length]" << std::endl;
        return 1;
    }
    auto counter = std::atol( argv[1] );

    // Read in the max output length, or set a sensible default
    const auto max = (argc > 2) ? std::atol( argv[2] ) : 512U;

    // Start generating/writing
    std::srand( static_cast<unsigned>( std::time( NULL ) ) );
    do {
        // Get the length
        const auto len = std::rand( ) % max;

        // Write it out
        const size_t nitems = 1;
        const auto output = htonl( static_cast<netlong_t>( len ) );
        if (fwrite( &output, sizeof( output ), nitems, stdout ) < nitems){
            std::cerr << "Failed to write the length-delimiter of output #" << counter << std::endl;

            // Bail
            return 1;
        }
        
        const int extent = 126, offset = 32;
        for (auto written = 0U; written < len; ++written){
            const auto clamped = (offset + (std::rand( ) % (extent - offset)) );
            const auto output = static_cast<char>( clamped );
            if (fwrite( &output, sizeof( output ), nitems, stdout ) < nitems){
                std::cerr << "Failed to write " << output << " of " << counter << std::endl;

                // Bail
                return 1;
            }
        }
        fflush( stdout );
    } while (--counter > 0U);
    return 0;
}
