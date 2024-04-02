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

VkSha256D::VkSha256D(): m_instance( VK_NULL_HANDLE ) {

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
    instanceExtNames.push_back( "VK_EXT_validation_features" );
    instanceExtNames.push_back( "VK_EXT_debug_utils" );
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

            std::cout << endl;
            std::cout << "Device #" << i << ": " << vkPhysicalDeviceProperties.deviceName << endl;
            std::cout << "maxComputeWorkGroupSize: " << vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0] << endl;
            std::cout << "Device type: " << int(vkPhysicalDeviceProperties.deviceType) << endl;

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
                ::std::cerr << "Failed to find a compute queue; skipping this device." << endl;
                continue;
            }
            std::cout << "Selected queue family #" << queueFamily << endl;

            // Create us a device to do the computation
            ComputeDevice device( vkPhysicalDevice, queueFamily, queueCount );
            if (static_cast<VkResult>( device ) == VK_SUCCESS){
                // Wrap up and accumulate
                const auto name = ::std::string( vkPhysicalDeviceProperties.deviceName );
                m_instances.push_back( Instance( name, ::std::move( device ) ) );
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

    m_mapping_pipeline = Mapping::Pipeline( m_device );
    m_mapping = Mapping( *m_device, m_device.AllocateDescriptorSet( m_mapping_pipeline ), m_device.AllocateCommandBuffer( ) );
    m_batch = Batch::New( m_device, 256 * 1024 * 1024 );
    m_slice = slice_type::New( m_device, 256 * 1024 * 1024 );
    m_reductions = ReductionFactory::New( m_device );
    m_reduction = m_reductions->NewReduction( );
}

VkSha256D::Instance::Instance(VkSha256D::Instance&& instance):
    IVkSha256DInstance( instance.Name( ) ),
    m_device( ::std::move( instance.m_device ) ),
    m_mapping_pipeline( ::std::move( instance.m_mapping_pipeline ) ),
    m_mapping( ::std::move( instance.m_mapping ) ),
    m_batch( ::std::move( instance.m_batch ) ),
    m_slice( ::std::move( instance.m_slice ) ),
    m_reductions( ::std::move( instance.m_reductions ) ),
    m_reduction( ::std::move( instance.m_reduction ) ) {
}

VkSha256D::Instance::~Instance() {

    m_mapping_pipeline = Pipeline( );
    m_mapping = Mapping( );
    m_batch = Batch( );
    m_slice = slice_type( );
    m_reductions.reset( );
    m_reduction.reset( );
}

VkSha256D::Instance& VkSha256D::Instance::operator=(VkSha256D::Instance&& instance) {

    m_name = instance.Name( );
    m_device = ::std::move( instance.m_device );
    m_mapping_pipeline = ::std::move( instance.m_mapping_pipeline );
    m_mapping = ::std::move( instance.m_mapping );
    m_batch = ::std::move( instance.m_batch );
    m_slice = ::std::move( instance.m_slice );
    m_reductions = ::std::move( instance.m_reductions );
    m_reduction = ::std::move( instance.m_reduction );
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

    // Apply the mapping from the batch to the slice
    m_mapping.Apply( m_batch, m_slice, m_mapping_pipeline );
    if (!m_mapping){
        // Ack
        cerr << "Failed to apply the mapping with error: " << static_cast<int64_t>( static_cast<VkResult>( m_mapping ) ) << endl;
        return "<mapping>";
    }
    auto vkResult = m_mapping.Dispatch( vkQueue );
    if (vkResult != VK_SUCCESS){
        cerr << "Failed to dispatch the mapping operation with error: " << static_cast<int64_t>( vkResult ) << endl;
        return "<dispatch1>";
    }

    // Apply the reduction
    const auto& applied = m_reduction->Apply( m_slice, m_device, m_reductions->Pipeline( ) );
    if (!applied){
        // Ack
        cerr << "Failed to apply the reduction with error: " << static_cast<int64_t>( static_cast<VkResult>( applied ) ) << endl;
        return "<reduction>";
    }
    vkResult = m_reduction->Dispatch( vkQueue );
    if (vkResult != VK_SUCCESS){
        cerr << "Failed to dispatch the reduction operation with error: " << static_cast<int64_t>( vkResult ) << endl;
        return "<dispatch2>";
    }
    return "";
}

bool VkSha256D::Instance::Add(const VkSha256D::Instance::arg_type& arg) {

    if (m_slice.Reserve( )){
        return m_batch.Push( arg.c_str( ), arg.size( ) );
    }
    return false;
}

} // namespace vkmr
#endif // defined (VULKAN_SUPPORT)
