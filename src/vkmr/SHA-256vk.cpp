// SHA-256vk.cpp: defines the functions for computing SHA-256 hashes with Vulkan
//

// Includes
//

// C++ Standard Headers
#include <algorithm>
#include <iostream>
#include <sstream>
#include <utility>

// Vulkan Headers
#include <vulkan/vulkan.h>

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"

// Constants
//

static const VkDeviceSize MegaX = (256 * 1024 * 1024);

// Globals
//

// Vulkan Extension Function Pointers
PFN_vkGetPhysicalDeviceProperties2KHR g_pVkGetPhysicalDeviceProperties2KHR;
PFN_vkGetPhysicalDeviceMemoryProperties2KHR g_pVkGetPhysicalDeviceMemoryProperties2KHR;
PFN_vkCmdPipelineBarrier2KHR g_pVkCmdPipelineBarrier2KHR;

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
#if defined(_WIN32)
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
#if defined(_MACOS_64_)
    // To enable MoltenVK
    instanceExtNames.push_back( (char*) VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME );
    vkCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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
        g_pVkCmdPipelineBarrier2KHR = (PFN_vkCmdPipelineBarrier2KHR)(
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
            ::std::cout << oss.str( ) << endl;

            // Create us a device to do the computation
            ComputeDevice device( vkPhysicalDevice, queueFamily, queueCount );
            if (static_cast<VkResult>( device ) == VK_SUCCESS){
                // Wrap up and accumulate
                const auto deviceName = ::std::string( vkPhysicalDeviceProperties.deviceName );
                m_devices.insert( { deviceName, ::std::move( device ) } );
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

    m_devices.clear( );
    if (m_instance != VK_NULL_HANDLE){
        ::vkDestroyInstance( m_instance, VK_NULL_HANDLE );
    }
}

VkSha256D::operator bool() const {

    if (m_instance == VK_NULL_HANDLE){
        return false;
    }
    return !m_devices.empty( );
}

bool VkSha256D::Has(const ISha256D::name_type& name) const {
    const auto found = m_devices.find( name );
    return (found != m_devices.end( ));
}

VkSha256D::Instance VkSha256D::Get(const ISha256D::name_type& name) {
    const auto found = m_devices.find( name );
    auto instance = VkSha256D::Instance( found->first, ::std::move( found->second ) );
    m_devices.erase( found );
    return instance;
}

::std::vector<ISha256D::name_type> VkSha256D::Available(void) const {

    ::std::vector<ISha256D::name_type> names;
    ::std::for_each(
        m_devices.cbegin( ),
        m_devices.cend( ),
        [&names](const decltype(m_devices)::value_type& pair) {
            names.push_back( pair.first );
        }
    );
    return names;
}

VkSha256D::Instance::Instance(const ::std::string& name, ComputeDevice&& device):
    IVkSha256DInstance( name ),
    m_device( ::std::move( device ) ),
    m_slices( MegaX ),
    m_batches( MegaX, (MegaX / sizeof( VkSha256Result ) ) * sizeof( VkSha256Metadata ) ) {

    m_slices.New( m_device );
    m_mappings = Mappings::New( m_device, m_batches.MaxBatchCount( m_device ) );
    m_reductions = Reductions::New( m_device, m_slices.MaxSliceCount( m_device ) );
}

VkSha256D::Instance::Instance(VkSha256D::Instance&& instance):
    IVkSha256DInstance( instance.Name( ) ),
    m_device( ::std::move( instance.m_device ) ),
    m_slices( ::std::move( instance.m_slices ) ),
    m_batch( ::std::move( instance.m_batch ) ),
    m_batches( ::std::move( instance.m_batches ) ),
    m_mappings( ::std::move( instance.m_mappings ) ),
    m_reductions( ::std::move( instance.m_reductions ) ),
    m_buffer( ::std::move( instance.m_buffer) ) {
}

VkSha256D::Instance::~Instance() {

    m_slices = Slices<VkSha256Result>( );
    m_batch = Batch( );
    m_mappings.reset( );
    m_reductions.reset( );
    m_buffer.clear( );
}

VkSha256D::Instance& VkSha256D::Instance::operator=(VkSha256D::Instance&& instance) {

    m_name = instance.Name( );
    m_device = ::std::move( instance.m_device );
    m_slices = ::std::move( instance.m_slices );
    m_batch = ::std::move( instance.m_batch );
    m_batches = ::std::move( instance.m_batches );
    m_mappings = ::std::move( instance.m_mappings );
    m_reductions = ::std::move( instance.m_reductions );
    m_buffer = ::std::move( instance.m_buffer );
    return (*this);
}

ISha256D::out_type VkSha256D::Instance::Root(void) {

    // Flush the buffer
    this->Flush( );

    // If the current batch is not empty, then send it off for mapping
    // And then wait for all such mappings to finish
    auto& current = m_slices.Current( );
    if (!m_batch.Empty( )){
        m_mappings->Map( ::std::move( m_batch ), current.Sub( ), m_device.Queue( ) );
    }
    m_mappings->WaitFor( );

    // Apply the reduction to all of the slices and wait for all to finish
    while (m_slices.Has( )){
        const auto& slice = m_slices.Any( );
        if (slice){
            auto number = slice.Number( );
            m_reductions->Reduce(
                m_slices.Remove( number ),
                m_device
            );
        }
    }
    return m_reductions->WaitFor( );
}

bool VkSha256D::Instance::Add(const ISha256D::arg_type& arg) {

    // Update the state of any in-flight reductions
    m_reductions->Update( );

    // Update the state of any in-flight mappings
    auto mapped = m_mappings->Update( );
    while (!mapped.empty( )){
        auto sub = ::std::move( mapped.back( ) );
        mapped.pop_back( );

        const auto number = sub.Number( );
        auto& slice = m_slices[number];
        slice += ::std::move( sub );
        if (slice.IsFilled( )){
            std::cout << "Slice #" << slice.Number( ) << " has been filled." << ::std::endl;

            // Kick off a new reduction
            m_reductions->Reduce( m_slices.Remove( number ), m_device );
        }
    }

    // Add to the buffer, and check it if can be flushed
    m_buffer.push_back( arg );
    auto& slice = m_slices.Current( );
    if (m_buffer.size( ) == slice.AlignedReservationSize( )){
        return this->Flush( );
    }

    // We presume it will be able to..
    return true;
}

bool VkSha256D::Instance::Flush(void) {

    // Look for an early out
    if (m_buffer.empty( )){
        return true;
    }

    auto flush = [&]() -> bool {
        // Setup
        const auto count = m_buffer.size( );
        auto& slice = m_slices.Current( );
        auto reserve = [&](void) -> bool {
            if (slice.Reserve( count )){
                // Happy days
                m_buffer.clear( );
                return true;
            }else{
                m_batch.Pop( count );
                return false;
            }
        };

        // Try and add the strings to the current batch
        if (m_batch.Push( m_buffer )){
            return reserve( );
        }

        // If we get here, then the batch may be full,
        // in which case, immediately send it off for mapping..
        if (m_batch){
            m_mappings->Map( ::std::move( m_batch ), slice.Sub( ), m_device.Queue( ) );
        }

        // And, try again with a new batch
        m_batch = m_batches.New( m_device );
        if (m_batch.Push( m_buffer )){
            return reserve( );
        }
        return false;
    };

    const auto available = m_slices.Current( ).Available( );
    if (available == 0U){
        // Need to kick off a new mapping op and then create new slice + batch
        auto& current = m_slices.Current( );
        m_mappings->Map( ::std::move( m_batch ), current.Sub( ), m_device.Queue( ) );

        // Allocate a new slice
        auto& slice = m_slices.New( m_device );
        if (!slice){
            return false;
        }

        // And a new batch
        m_batch = m_batches.New( m_device );
        return flush( );
    }else if (available >= m_buffer.size( )){
        // Don't need to make any special accomodations..
        return flush( );
    }else{
        // Capture the overflow
        decltype( m_buffer ) overflow;
        overflow.reserve( m_buffer.size( ) );
        do {
            overflow.push_back( ::std::move( m_buffer.back( ) ) );
            m_buffer.pop_back( );
        } while (m_buffer.size( ) > available);
        ::std::cout << "Overflow: " << overflow.size( ) << "." << ::std::endl;

        // Now, try and flush
        auto result = flush( );

        // Regardless of the outcome, put back what we removed from the buffer
        while (!overflow.empty( )){
            m_buffer.push_back( ::std::move( overflow.back( ) ) );
            overflow.pop_back( );
        }
        return result;
    }

    return false;
}

} // namespace vkmr
