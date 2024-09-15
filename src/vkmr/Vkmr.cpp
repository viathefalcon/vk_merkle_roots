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
        const auto available = instances.Available( );
        if (available.empty( )){
            std::cerr << "No Vulkan GPU compute device available on this host." << endl;
            return 1;
        }else if (available.size( ) == 1){
            // Pick the only one available by default
            arg1 = available.front( );
        }else{
            std::cerr << "Usage: " << std::string( argv[0] ) << " <name of GPU compute device>" << endl;
            std::cerr << "Available: " << endl;
            for (auto it = available.cbegin( ), end = available.cend( ); it != end; ++it){
                std::cerr << "* " << *it << endl;
            }
            return 1;
        }
    }
    cout << "Initializing for: " << arg1 << endl;

    // Look for the named instance
    if (!instances.Has( arg1 )){
        std::cerr << "Failed to find \"" << arg1 << "\". Aborting.." << endl;
        return 1;
    }
    auto vkSha256D = instances.Get( arg1 );

    // Loop over the inputs
    vkmr::Input input( stdin );
    size_t size = 0U, count = 0U;
    while (input.Has( )){
        const auto arg = input.Get( );
        if (arg.empty( )){
            std::cerr << "Read an empty string?" << endl;
            continue;
        }

        // Copy into the Vulkan-based instance
        bool ok = vkSha256D.Add( arg );
        if (!ok){
            break;
        }

        if (!mrc.Add( std::move( arg ) )){
            std::cerr << "Failed to accumulate \"" << arg << "\"" << std::endl;
            break;
        }

        size += arg.size( );
        count++;
    }
    if (count > 0U){
        {
            StopWatch sw;
            sw.Start( );
            const auto root = mrc.Root( );
            cout << "Root (of " << count << " item(s), " << size << " byte(s)) => " << print_bytes( root ).str( ) << " in " << sw.Elapsed( ) << endl;
        }
        {
            cout << vkSha256D.Name( ) << ":" << endl;
            StopWatch sw;
            sw.Start( );
            const auto gpu_root = vkSha256D.Root( );
            cout << gpu_root << " (" << sw.Elapsed( ) << ")" << endl;
        }
    }
    return 0;
}
