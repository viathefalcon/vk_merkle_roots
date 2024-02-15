// Rndm.cpp: A simple program to generate strings w/random content
//           and write them to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// Functions
//

// Gives the entry-point
int main(int argc, const char* argv[]) {

    // Fail safe
    if (argc < 2){
        std::cerr << "Usage: rndm [seed] [max output size]" << std::endl;
        return 1;
    }

    // Get the upper bound for the output size
    const auto bound = std::atol( (argc > 2) ? argv[2] : argv[1] );

    // Set the seed
    const auto seed = (argc > 2) ? std::atol( argv[1] ) : std::time( NULL );
    std::srand( static_cast<unsigned>( seed ) );

    // Start generating/writing until we've written just as much as asked for
    for (long sum = 0U; sum < bound; ){
        // Get the length of the next string
        const auto len = std::rand( );
        if (len == 0){
            continue;
        }
        if ((sum + len) > bound){
            // Can stop here
            break;
        }

        // Format into a fixed-length character string
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw( 10 ) << static_cast<unsigned>( len );
        
        // Write it out
        const auto output = oss.str( );
        if (fwrite( output.c_str( ), sizeof( decltype( output )::value_type ), output.size( ), stdout ) < output.size( )){
            // Bail
            return 1;
        }
        
        // Write out the data itself
        const size_t nchars = 1;
        const int extent = 126, offset = 32;
        for (auto written = 0U; written < len; ++written){
            const auto clamped = (offset + (std::rand( ) % (extent - offset)) );
            const auto output = static_cast<char>( clamped );
            if (fwrite( &output, sizeof( output ), nchars, stdout ) < nchars){
                // Bail
                return 1;
            }
        }

        // Flush and accumulate
        fflush( stdout );
        sum += len;
    }
    return 0;
}
