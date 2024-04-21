// Strm.cpp: A simple program to accept string arguments on the command line and write them as
//           a line-separated stream to stdout
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

        // Write out the characters
        fwrite( arg, sizeof( char ), len, stdout );

        // Write out a line separator
        const auto output = '\n';
        fwrite( &output, sizeof( output ), 1, stdout );
    }
    fflush( stdout );
    return 0;
}
