// Vkmr.cpp: defines the entry point for the application
//

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

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
#if defined (VULKAN_SUPPORT)
    cout << "Initializing";
    std::string arg1;
    if (argc > 1){
        arg1.append( argv[1] );
        cout << " for " << arg1;
    }else{
        cout << "..";
    }
    cout << endl;;
    vkmr::VkSha256D instances( arg1 );
#endif

    // Loop over the inputs
    vkmr::Input input( stdin );
    while (input.Has( )){
        const auto arg = input.Get( );
        if (arg.empty( )){
            std::cerr << "Read an empty string?" << endl;
            continue;
        }

#if defined (VULKAN_SUPPORT)
        // Copy into each of the Vulkan-based instances
        instances.ForEach( [&](vkmr::VkSha256D::Instance& instance) {
            instance.Add( arg );
        } );
#endif

        /*
        const auto hashed = vkmr::cpu_sha256d( arg );
        cout << arg << " >> " << print_bytes( hashed ).str( ) << endl;
        */

        if (!mrc.Add( std::move( arg ) )){
            std::cerr << "Failed to accumulate \"" << arg << "\"" << std::endl;
            break;
        }
    }
    if (input.Count( ) > 0U){
        StopWatch sw;
        sw.Start( );
        const auto root = mrc.Root( );
        cout << "Root (of " << input.Count( ) << " item(s), " << input.Size( ) << " byte(s)) => " << print_bytes( root ).str( ) << " in " << sw.Elapsed( ) << endl;
#if defined (VULKAN_SUPPORT)
        instances.ForEach( [&](vkmr::VkSha256D::Instance& instance) {
            cout << instance.Name( ) << ":" << endl;
            StopWatch sw;
            sw.Start( );
            const auto gpu_root = instance.Root( );
            cout << gpu_root << " (" << sw.Elapsed( ) << ")" << endl;
        } );
#endif
    }
    return 0;
}
