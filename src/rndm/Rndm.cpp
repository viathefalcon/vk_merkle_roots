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

// Nearby Project Headers
#include "../common/Utils.h"

// Functions
//

// Gives the entry-point
int main(int argc, const char* argv[]) {

    // Figure out the maximum width of the length-delimiter
    typedef unsigned delim_type;
    const auto delim_width = vkmr::max_string_length<delim_type>( );

    // Fail safe
    if (argc < 2){
        std::cerr << "Usage: rndm [seed] [max total size] [max element size]" << std::endl;
        return 1;
    }

    // Get the upper bound for the output size
    const auto bound = std::atol( (argc > 2) ? argv[2] : argv[1] );

    // Set the seed
    const auto seed = (argc > 2) ? std::atol( argv[1] ) : std::time( NULL );
    std::srand( static_cast<unsigned>( seed ) );

    // Get the maximum length of any given output string
    const auto max = (argc > 3) ? std::atol( argv[3] ) : 0;

    // Start generating/writing until we've written just as much as asked for
    size_t count = 0U;
    for (long sum = 0U; sum < bound; ++count ){
        // Get the length of the next string
        const auto r = std::rand( );
        const auto len = (max == 0) ? r : (r % max);
        if (len == 0){
            continue;
        }
        if ((sum + len) > bound){
            // Can stop here
            break;
        }

        // Format into a fixed-length character string
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw( delim_width ) << static_cast<delim_type>( len );

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
    std::cerr << "Wrote " << count << " string(s)" << std::endl;
    return 0;
}
