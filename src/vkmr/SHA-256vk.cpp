// SHA-256vk.cpp: defines the functions for computing SHA-256 hashes with Vulkan
//

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

// Includes
//

// C++ Standard Headers
#include <iostream>
#include <fstream>
#include <cstring>

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>
#endif

// Local Project Headers
#include "Debug.h"
#include "Batches.h"
#include "SHA-256vk.h"

#if defined (VULKAN_SUPPORT)
// Globals
//

// Vulkan Extension Function Pointers
PFN_vkGetPhysicalDeviceProperties2KHR g_pVkGetPhysicalDeviceProperties2KHR;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR g_pVkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;
#endif // defined (VULKAN_SUPPORT)

// Function(s)
//

int vkSha256(int argc, const char* argv[]) {
#if defined (VULKAN_SUPPORT)
    using namespace std;

    // Look for an early out
    size_t sum = 0;
    for (int i = 0; i < argc; ++i){
        sum += std::strlen( argv[i] );
    }
    if (sum == 0){
        return 0;
    }

    // Read in the shader bytes
    std::ifstream ifs( "SHA-256-n.spv", std::ios::binary | std::ios::ate );
    const auto g = ifs.tellg( );
    ifs.seekg( 0 );
    std::vector<uint32_t> code( g / sizeof( uint32_t ) );
    ifs.read( reinterpret_cast<char*>( code.data( ) ), g );
    ifs.close( );
    std::cout << "Loaded " << code.size() << " (32-bit) word(s) of shader code." << endl;

    VkApplicationInfo vkAppInfo = {};
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pApplicationName = "vkMerkleRoots";
    vkAppInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    vkAppInfo.pEngineName = "n/a";
    vkAppInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);
    vkAppInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo vkCreateInfo = {};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &vkAppInfo;

    // Tee up the initial set of instance-level extensions we want
    std::vector<char*> instanceExtNames;
    instanceExtNames.push_back( (char*) VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME );
    instanceExtNames.push_back( (char*) VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME );
#if !defined(_ONDECK_)
    VkValidationFeatureEnableEXT vkValidationFeatureEnableEXT[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
    VkValidationFeaturesEXT vkValidationFeaturesEXT = {};
    vkValidationFeaturesEXT.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    vkValidationFeaturesEXT.enabledValidationFeatureCount = 1;
    vkValidationFeaturesEXT.pEnabledValidationFeatures = vkValidationFeatureEnableEXT;
    vkCreateInfo.pNext = &vkValidationFeaturesEXT;

    // Enable the validation layer
    const char* szLayerName = "VK_LAYER_KHRONOS_validation";
    vkCreateInfo.enabledLayerCount = 1;
    vkCreateInfo.ppEnabledLayerNames = &szLayerName;

    // Add the validation extensions
    instanceExtNames.push_back( "VK_EXT_validation_features" );
    instanceExtNames.push_back( "VK_EXT_debug_utils" );
#endif
    vkCreateInfo.enabledExtensionCount = instanceExtNames.size( );
    vkCreateInfo.ppEnabledExtensionNames = instanceExtNames.data( );

    VkInstance instance = VK_NULL_HANDLE;
    VkResult vkResult = vkCreateInstance( &vkCreateInfo, VK_NULL_HANDLE, &instance );
    if (vkResult == VK_SUCCESS){
        // Load the extension
        g_pVkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)(
            vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2KHR" )
        );
        g_pVkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)(
            vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties2" )
        );
        g_VkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)(
            vkGetInstanceProcAddr( instance, "vkCmdPipelineBarrier2KHR" )
        );

        // Count
        uint32_t vkPhysicalDeviceCount = 0;
        vkEnumeratePhysicalDevices( instance, &vkPhysicalDeviceCount, VK_NULL_HANDLE );

        // Retrieve
        VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
        vkEnumeratePhysicalDevices( instance, &vkPhysicalDeviceCount, vkPhysicalDevices );
        
        // Enumerate
        for (decltype(vkPhysicalDeviceCount) i = 0; i < vkPhysicalDeviceCount; ++i){
            VkPhysicalDevice vkPhysicalDevice = vkPhysicalDevices[i];
            VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
            vkGetPhysicalDeviceProperties( vkPhysicalDevice, &vkPhysicalDeviceProperties );

            std::cout << endl;
            std::cout << "Device #" << i << ": " << vkPhysicalDeviceProperties.deviceName << endl;
            std::cout << "maxComputeWorkGroupSize: " << vkPhysicalDeviceProperties.limits.maxComputeSharedMemorySize << endl;
            std::cout << "Device type: " << int(vkPhysicalDeviceProperties.deviceType) << endl;

            // Count
            uint32_t vkQueueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, VK_NULL_HANDLE );

            // Retrieve
            VkQueueFamilyProperties* vkQueueFamilyProperties = new VkQueueFamilyProperties[vkQueueFamilyCount];
            vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, vkQueueFamilyProperties );
            
            // Iterate
            uint32_t queueCount = 0, queueFamily = vkQueueFamilyCount;
            for (decltype(vkQueueFamilyCount) j = 0; j < vkQueueFamilyCount; ++j){
                VkQueueFamilyProperties*  vkQueueFamilyProps = (vkQueueFamilyProperties + j);
                std::cout << "Queue family #" << j << " supports ";
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    std::cout << "graphics ";
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_COMPUTE_BIT){
                    std::cout << "compute ";
                    if (vkQueueFamilyProps->queueCount > queueCount){
                        queueFamily = j;
                        queueCount = vkQueueFamilyProps->queueCount;
                    }
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_TRANSFER_BIT){
                    std::cout << "transfer ";
                }
                std::cout << "on " << vkQueueFamilyProps->queueCount << " queue(s)." << endl;
            }
            delete[] vkQueueFamilyProperties;
            std::cout << endl;

            // Look for an early out
            if (queueFamily >= vkQueueFamilyCount){
                cerr << "Failed to find a compute queue; skipping this device." << endl;
                continue;
            }
            std::cout << "Selected queue family #" << queueFamily << endl;

            // Query for the extended physical properties
            if (g_pVkGetPhysicalDeviceProperties2KHR){
                VkPhysicalDeviceExternalMemoryHostPropertiesEXT vkPhysicalDeviceExternalMemoryHostProperties = {};
                vkPhysicalDeviceExternalMemoryHostProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT;
                VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2 = {};
                vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceExternalMemoryHostProperties;
                g_pVkGetPhysicalDeviceProperties2KHR( vkPhysicalDevice, &vkPhysicalDeviceProperties2 );
                std::cout << "vkPhysicalDeviceExternalMemoryHostProperties.minImportedHostPointerAlignment == " << vkPhysicalDeviceExternalMemoryHostProperties.minImportedHostPointerAlignment << endl << endl;
            }else{
                std::cerr << "'vkGetPhysicalDeviceProperties2KHR' could not be resolved." << endl; 
            }

            // Create us a device to do the computation
            vkmr::ComputeDevice device( vkPhysicalDevice, queueFamily, queueCount, code );
            vkResult = static_cast<VkResult>( device );
            if (vkResult != VK_SUCCESS){
                cerr << "Failed to create a logical compute device on Vulkan" << std::endl;
                continue;
            }

            // Get a handle to a queue
            VkQueue vkQueue = device.Queue( 0 );
            if (vkQueue == VK_NULL_HANDLE){
                cerr << "Failed to retrieve handle to device queue!" << endl;
                continue;
            }

            // Create a new batch
            auto batch = vkmr::NewBatch( (1024 * 1024), device );
            if (!batch){
                std::cerr << "Failed to create a new batch; skipping." << std::endl;
                continue;
            }

            // Add the inputs to the batch
            for (int i = 0; i < argc; ++i){
                const auto arg = argv[i];
                const auto added = batch->Add( arg, std::strlen( arg ) );
                if (!added){
                    break;
                }
            }

            // Dispatch the batch..
            vkResult = batch->Dispatch( vkQueue );
            if (vkResult != VK_SUCCESS){
                std::cerr << "Failed to dispatch the batch..?" << std::endl;
            }
            batch.reset( nullptr ); // Force the cleanup rather than relying on going out-of-scope in any particular order
        }

        // Cleanup
        delete[] vkPhysicalDevices;
        vkDestroyInstance( instance, VK_NULL_HANDLE );
        return 0;
    }


    // Get the error name
    char* initError = NULL;
    switch (vkResult) {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            initError = (char*) "VK_ERROR_OUT_OF_HOST_MEMORY";
            break;

        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            initError = (char*) "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            break;

        case VK_ERROR_LAYER_NOT_PRESENT:
            initError = (char*) "VK_ERROR_LAYER_NOT_PRESENT";
            break;

        case VK_ERROR_EXTENSION_NOT_PRESENT:
            initError = (char*) "VK_ERROR_EXTENSION_NOT_PRESENT";
            break;

        case VK_ERROR_INCOMPATIBLE_DRIVER:
            initError = (char*) "VK_ERROR_INCOMPATIBLE_DRIVER";
            break;

        default:
            initError = (char*) "(some other, unidentified error)";
            break;       
    }
    cerr << "Failed to initialise Vulkan w/error: " << initError << endl;
#else
    std::cerr << "Vulkan not supported!" << std::endl;
#endif
    return 1;
}