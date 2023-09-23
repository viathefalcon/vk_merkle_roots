// Strm.cpp: A simple program to accept string arguments on the command line and write them as
//           a length-delimited stream to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <cstring>

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
        const auto output = static_cast<unsigned int>( len );
        fwrite( &output, sizeof( output ), 1, stdout );

        // Write out the characters
        fwrite( arg, sizeof( char ), len, stdout );
    }
    fflush( stdout );
    return 0;
}
