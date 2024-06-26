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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>
#endif

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"

#if defined (VULKAN_SUPPORT)
// Globals
//

// Vulkan Extension Function Pointers
PFN_vkGetPhysicalDeviceProperties2KHR g_pVkGetPhysicalDeviceProperties2KHR;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR g_pVkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;

namespace vkmr {

// Class(es)
//

VkSha256D::VkSha256D(const ::std::string& name): m_instance( VK_NULL_HANDLE ) {

    using ::std::endl;

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
    instanceExtNames.push_back( (char*) "VK_EXT_validation_features" );
    instanceExtNames.push_back( (char*) "VK_EXT_debug_utils" );
#endif
    vkCreateInfo.enabledExtensionCount = instanceExtNames.size( );
    vkCreateInfo.ppEnabledExtensionNames = instanceExtNames.data( );

    m_vkResult = ::vkCreateInstance( &vkCreateInfo, VK_NULL_HANDLE, &m_instance );
    if (m_vkResult == VK_SUCCESS){
        // Load the extensions
        g_pVkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR)(
            ::vkGetInstanceProcAddr( m_instance, "vkGetPhysicalDeviceProperties2KHR" )
        );
        g_pVkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)(
            ::vkGetInstanceProcAddr( m_instance, "vkGetPhysicalDeviceMemoryProperties2" )
        );
        g_VkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)(
            ::vkGetInstanceProcAddr( m_instance, "vkCmdPipelineBarrier2KHR" )
        );

        // Count
        uint32_t vkPhysicalDeviceCount = 0;
        ::vkEnumeratePhysicalDevices( m_instance, &vkPhysicalDeviceCount, VK_NULL_HANDLE );

        // Retrieve
        VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
        ::vkEnumeratePhysicalDevices( m_instance, &vkPhysicalDeviceCount, vkPhysicalDevices );
        
        // Enumerate
        for (decltype(vkPhysicalDeviceCount) i = 0; i < vkPhysicalDeviceCount; ++i){
            VkPhysicalDevice vkPhysicalDevice = vkPhysicalDevices[i];
            VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
            ::vkGetPhysicalDeviceProperties( vkPhysicalDevice, &vkPhysicalDeviceProperties );

            ::std::ostringstream oss;
            oss << endl;
            oss << "Device #" << i << ": " << vkPhysicalDeviceProperties.deviceName << endl;
            oss << "maxComputeWorkGroupSize: (" << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0] << ", " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1] << ", " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[2] << ")" << endl;
            oss << "maxComputeWorkGroupInvocations: " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations << endl;
            oss << "maxComputeWorkGroupCount: (" << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0] << ", " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[1] << ", " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[2] << ")" << endl;
            oss << "Device type: " << int(vkPhysicalDeviceProperties.deviceType) << endl;

            // Count
            uint32_t vkQueueFamilyCount = 0;
            ::vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, VK_NULL_HANDLE );

            // Retrieve
            VkQueueFamilyProperties* vkQueueFamilyProperties = new VkQueueFamilyProperties[vkQueueFamilyCount];
            ::vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, vkQueueFamilyProperties );
            
            // Iterate
            uint32_t queueCount = 0, queueFamily = vkQueueFamilyCount;
            for (decltype(vkQueueFamilyCount) j = 0; j < vkQueueFamilyCount; ++j){
                VkQueueFamilyProperties*  vkQueueFamilyProps = (vkQueueFamilyProperties + j);
                oss << "Queue family #" << j << " supports ";
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    oss << "graphics ";
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_COMPUTE_BIT){
                    oss << "compute ";
                    if (vkQueueFamilyProps->queueCount > queueCount){
                        queueFamily = j;
                        queueCount = vkQueueFamilyProps->queueCount;
                    }
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_TRANSFER_BIT){
                    oss << "transfer ";
                }
                oss << "(0x" << ::std::hex << int(vkQueueFamilyProps->queueFlags) << ::std::dec << ") on " << vkQueueFamilyProps->queueCount << " queue(s)." << endl;
            }
            delete[] vkQueueFamilyProperties;

            // Look for an early out
            if (queueFamily >= vkQueueFamilyCount){
                ::std::cout << oss.str( );
                ::std::cerr << "Failed to find a compute queue; skipping this device." << endl;
                continue;
            }
            oss << "Selected queue family #" << queueFamily << endl;

            // If the caller specified a name, then check that it matches
            // the device's declared name
            const auto deviceName = ::std::string( vkPhysicalDeviceProperties.deviceName );
            if (!name.empty( )){
                if (deviceName != name){
                    continue;
                }
            }
            ::std::cout << oss.str( ) << endl;

            // Create us a device to do the computation
            ComputeDevice device( vkPhysicalDevice, queueFamily, queueCount );
            if (static_cast<VkResult>( device ) == VK_SUCCESS){
                // Wrap up and accumulate
                m_instances.push_back( Instance( deviceName, ::std::move( device ) ) );
                continue;
            }
            ::std::cerr << "Failed to create a logical compute device on Vulkan" << std::endl;
        }

        // Cleanup
        delete[] vkPhysicalDevices;
        return;
    }

    // Get the error name
    char* initError = NULL;
    switch (m_vkResult) {
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
    ::std::cerr << "Failed to initialise Vulkan w/error: " << initError << ::std::endl;
}

VkSha256D::~VkSha256D() {

    m_instances.clear( );
    if (m_instance != VK_NULL_HANDLE){
        ::vkDestroyInstance( m_instance, VK_NULL_HANDLE );
    }
}

VkSha256D::operator bool() const {

    if (m_instance == VK_NULL_HANDLE){
        return false;
    }
    return !m_instances.empty( );
}

void VkSha256D::ForEach(::std::function<void(Instance&)> lambda) {
    ::std::for_each( m_instances.begin( ), m_instances.end( ), lambda );
}

VkSha256D::Instance::Instance(const ::std::string& name, ComputeDevice&& device):
    IVkSha256DInstance( name ),
    m_device( ::std::move( device ) ) {

    const VkDeviceSize Mega256 = (256 * 1024 * 1024);
    m_batch = Batch::New( m_device, Mega256 );
    m_slice = slice_type::New( m_device, Mega256 );
    m_mappings = Mappings::New( m_device );
    m_reductions = Reductions::New( m_device );
}

VkSha256D::Instance::Instance(VkSha256D::Instance&& instance):
    IVkSha256DInstance( instance.Name( ) ),
    m_device( ::std::move( instance.m_device ) ),
    m_batch( ::std::move( instance.m_batch ) ),
    m_slice( ::std::move( instance.m_slice ) ),
    m_mappings( ::std::move( instance.m_mappings ) ),
    m_reductions( ::std::move( instance.m_reductions ) ) {
}

VkSha256D::Instance::~Instance() {

    m_batch = Batch( );
    m_slice = slice_type( );
    m_mappings.reset( );
    m_reductions.reset( );
}

VkSha256D::Instance& VkSha256D::Instance::operator=(VkSha256D::Instance&& instance) {

    m_name = instance.Name( );
    m_device = ::std::move( instance.m_device );
    m_batch = ::std::move( instance.m_batch );
    m_slice = ::std::move( instance.m_slice );
    m_mappings = ::std::move( instance.m_mappings );
    m_reductions = ::std::move( instance.m_reductions );
    return (*this);
}

VkSha256D::Instance::out_type VkSha256D::Instance::Root(void) {

    using ::std::cerr;
    using ::std::endl;

    // Get a handle to a queue
    VkQueue vkQueue = m_device.Queue( 0 );
    if (vkQueue == VK_NULL_HANDLE){
        cerr << "Failed to retrieve handle to device queue!" << endl;
        return "<vkQueue>";
    }

    // Map the inputs into the slice
    auto mapping = m_mappings->Map( m_batch, m_slice, vkQueue );
    if (mapping == VK_NULL_HANDLE){
        return "<mapping>";
    }

    // Apply the reduction
    auto vkSha256Result = m_reductions->Reduce( mapping, vkQueue, m_slice, m_device );
    return print_bytes_ex( vkSha256Result.data, SHA256_WC ).str( );
}

bool VkSha256D::Instance::Add(const VkSha256D::Instance::arg_type& arg) {

    if (m_batch.Push( arg.c_str( ), arg.size( ) )){
        if (m_slice.Reserve( )){
            return true;
        }
        m_batch.Pop( );
    }
    return false;
}

void VkSha256D::Instance::Cap(size_t size) {

    if (m_slice.Reserved( ) > size){
        m_slice.Unreserve( m_slice.Reserved( ) - size );
    }
}

} // namespace vkmr
#endif // defined (VULKAN_SUPPORT)
