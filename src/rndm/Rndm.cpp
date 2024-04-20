// Rndm.cpp: A simple program to generate strings w/random content
//           and write them to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <ctime>
#include <cstdlib>
#include <iostream>

// Functions
//

// Gives the entry-point
int main(int argc, const char* argv[]) {

    // Set the seed
    const auto seed = (argc > 1) ? std::atol( argv[1] ) : std::time( NULL );
    std::srand( static_cast<unsigned>( seed ) );
    std::cerr << "Using seed: " << seed << std::endl;

    // Fail safe
    if (argc < 3){
        std::cerr << "Usage: rndm [seed] [max total size] [max element size]" << std::endl;
        return 1;
    }

    // Get the upper bound for the output size
    const auto bound = std::atol( argv[2] );

    // Get the maximum length of any given output string
    const auto max = (argc > 3) ? std::atol( argv[3] ) : std::min( 16384L, bound );

    // Start generating/writing until we've written just as much as asked for
    size_t count = 0U;
    long sum = 0U;
    for ( ; sum < bound; ++count ){
        // Get a non-zero random value less than the set maximum
        const auto r = 1 + (std::rand( ) % (max - 1));

        // Use to get the length of the next string, capped to the remaining unfilled total
        const auto len = std::min( bound - sum, r );
        if (len == 0){
            break;
        }
        
        // Generate, write out the data itself
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

        // Write out a line separator
        const auto output = '\n';
        if (fwrite( &output, sizeof( output ), nchars, stdout ) < nchars){
            // Bail
            return 1;
        }

        // Flush and accumulate
        fflush( stdout );
        sum += len;
    }
    std::cerr << "Wrote " << count << " string(s) in a total of " << sum << " byte(s)." << std::endl << std::endl;
    return 0;
}
