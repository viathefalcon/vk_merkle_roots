// Strm.cpp: A simple program to accept string arguments on the command line and write them as
//           a length-delimited stream to stdout
//

// Includes
//

// Standard Library Headers
#include <stdio.h>

// C++ Standard Library Headers
#include <cstring>
#include <iomanip>
#include <sstream>

// Functions
//

// Gives the entry-point
int main(int argc, const char* argv[]) {

    // Loop through the arguments
    for (int i = 1; i < argc; ++i){
        // Get the length
        const auto arg = *(argv + i);
        const auto len = std::strlen( arg );

        // Format into a fixed-length character string
        std::ostringstream oss;
        oss << std::setfill('0') << std::setw( 10 ) << static_cast<unsigned>( len );
        
        // Write it out
        const auto output = oss.str( );
        if (fwrite( output.c_str( ), sizeof( decltype( output )::value_type ), output.size( ), stdout ) < output.size( )){
            // Bail
            return 1;
        }

        // Write out the characters
        fwrite( arg, sizeof( char ), len, stdout );
    }
    fflush( stdout );
    return 0;
}
