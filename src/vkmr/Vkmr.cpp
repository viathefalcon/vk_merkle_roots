// Vkmr.cpp: defines the entry point for the application
//

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

// Local Project Headers
#include "Debug.h"
#include "Inputs.h"
#include "StopWatch.h"
#include "SHA-256vk.h"
#include "SHA-256plus.h"

// Functions
//

// Gives the main loop for the application
int run(vkmr::ISha256D& sha256D) {

    using std::cout;
    using std::endl;

    // Loop over the inputs
    vkmr::Input input( stdin );
    size_t size = 0U, count = 0U;
    while (input.Has( )){
        const auto arg = input.Get( );
        if (arg.empty( )){
            std::cerr << "Read an empty string?" << endl;
            continue;
        }
        bool ok = sha256D.Add( arg );
        if (!ok){
            break;
        }

        size += arg.size( );
        count++;
    }
    if (count > 0U){
        StopWatch sw;
        sw.Start( );
        const auto root = sha256D.Root( );
        cout << sha256D.Name( ) << ": computed root (of " << count << " item(s), " << size << " byte(s)) => " << root << " in " << sw.Elapsed( ) << endl;
    }
    return 0;
}

// Gives the entry-point for the application
int main(int argc, const char* argv[]) {

    using std::cout;
    using std::endl;

    vkmr::CpuSha256D mrc;
    std::string arg1;
    vkmr::VkSha256D instances;
    if (argc > 1){
        arg1.append( argv[1] );
    }else{
        auto available = instances.Available( );
        available.insert( available.begin( ), mrc.Name( ) );
        if (available.size( ) == 1){
            // Pick the only one available by default
            arg1 = available.front( );
        }else{
            std::cerr << "Usage: " << std::string( argv[0] ) << " <name of compute device>" << endl;
            std::cerr << "Available: " << endl;
            for (auto it = available.cbegin( ), end = available.cend( ); it != end; ++it){
                std::cerr << "* " << *it << endl;
            }
            return 1;
        }
    }
    cout << "Initializing for: " << arg1 << endl;

    // Look for the named instance
    if (instances.Has( arg1 )){
        auto vkSha256D = instances.Get( arg1 );
        return run( vkSha256D );
    }else if (mrc.Name( ) == arg1){
        return run( mrc );
    }
    std::cerr << "No device selected; aborting." << endl;
    return 1;
}
