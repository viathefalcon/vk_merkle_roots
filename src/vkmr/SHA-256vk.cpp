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
#include <utility>
#include <vector>
#include <string>

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>
#endif

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"

// Class(es)
//
#if defined (VULKAN_SUPPORT)
class VkBufferSet {
private:
    VkDevice m_vkDevice;
    uint32_t m_queueFamilyIndex;
    VkDeviceSize m_size;
    VkBuffer m_vkBufferHost, m_vkBufferDevice;

    void reset() {
        m_vkBufferHost = VK_NULL_HANDLE;
        m_vkBufferDevice = VK_NULL_HANDLE;
        m_queueFamilyIndex = 0;
        m_size = 0U;
        m_vkDevice = VK_NULL_HANDLE;
    }

    void release() {
        if (m_vkBufferHost != VK_NULL_HANDLE){
            ::vkDestroyBuffer( m_vkDevice, m_vkBufferHost, VK_NULL_HANDLE );
        }
        if (m_vkBufferDevice != VK_NULL_HANDLE){
            ::vkDestroyBuffer( m_vkDevice, m_vkBufferDevice, VK_NULL_HANDLE );
        }
    }

public:
    VkBufferSet() noexcept {
        reset( );
    }

    VkBufferSet(VkDevice vkDevice, uint32_t queueFamilyIndex, VkDeviceSize size) noexcept:
        m_vkDevice( vkDevice ),
        m_queueFamilyIndex( queueFamilyIndex ),
        m_size( size ),
        m_vkBufferHost( VK_NULL_HANDLE ),
        m_vkBufferDevice( VK_NULL_HANDLE ) {

        // Create the host-side buffer
        VkBufferCreateInfo vkBufferCreateInfo = {};
        vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vkBufferCreateInfo.queueFamilyIndexCount = 1;
        vkBufferCreateInfo.pQueueFamilyIndices = &(m_queueFamilyIndex);
        vkBufferCreateInfo.size = size;
        VkResult vkResult = ::vkCreateBuffer(
            vkDevice,
            &vkBufferCreateInfo,
            VK_NULL_HANDLE,
            &m_vkBufferHost
        );
        if (vkResult != VK_SUCCESS){
            m_vkBufferHost = VK_NULL_HANDLE;
        }

        // The device-side buffer is similar
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkResult = ::vkCreateBuffer(
            vkDevice,
            &vkBufferCreateInfo,
            VK_NULL_HANDLE,
            &m_vkBufferDevice
        );
        if (vkResult != VK_SUCCESS){
            m_vkBufferDevice = VK_NULL_HANDLE;
        }
    }

    VkBufferSet(VkBufferSet&& pair) noexcept:
        m_vkDevice( pair.m_vkDevice ),
        m_queueFamilyIndex( pair.m_queueFamilyIndex ),
        m_size( pair.m_size ),
        m_vkBufferHost( pair.m_vkBufferHost ),
        m_vkBufferDevice( pair.m_vkBufferDevice ) {

        pair.reset( );
    }

    // We don't want a copy constructor
    VkBufferSet(const VkBufferSet&) = delete;

    ~VkBufferSet() {
        release( );
    }

    VkBufferSet& operator=(VkBufferSet&& pair) noexcept {
        if (this != &pair){
            release( );

            m_vkDevice = pair.m_vkDevice;
            m_queueFamilyIndex = pair.m_queueFamilyIndex;
            m_size = pair.m_size;
            m_vkBufferHost = pair.m_vkBufferHost;
            m_vkBufferDevice = pair.m_vkBufferDevice;

            pair.reset( );
        }
        return (*this);
    }

    VkBufferSet& operator=(VkBufferSet const&) = delete;

    operator bool() const {
        return !((m_vkBufferHost == VK_NULL_HANDLE) || (m_vkBufferDevice == VK_NULL_HANDLE));
    }

    VkBuffer host() const {
        return m_vkBufferHost;
    }

    VkBuffer device() const {
        return m_vkBufferDevice;
    }

    void xfer(VkCommandBuffer vkCommandBuffer) {
        VkBufferCopy vkBufferCopy = {};
        vkBufferCopy.size = m_size;
        vkCmdCopyBuffer( vkCommandBuffer, m_vkBufferHost, m_vkBufferDevice, 1, &vkBufferCopy );
    }
};

class VkMemoryMapping {
private:
    VkDevice m_vkDevice;
    VkDeviceMemory m_vkDeviceMemory;
    uint8_t* m_pMapped;

    void reset() {
        m_vkDevice = VK_NULL_HANDLE;
        m_vkDeviceMemory = VK_NULL_HANDLE;
        m_pMapped = NULL;
    }

    void release() {
        if (m_pMapped != NULL){
            ::vkUnmapMemory( m_vkDevice, m_vkDeviceMemory );
        }
        reset( );
    }

public:
    VkMemoryMapping() {
        reset( );
    }

    VkMemoryMapping(VkDevice vkDevice, VkDeviceMemory vkDeviceMemory) noexcept:
        m_vkDevice( vkDevice ),
        m_vkDeviceMemory( vkDeviceMemory ),
        m_pMapped( NULL ) {

        // Map the whole thing into memory
        VkResult vkResult = ::vkMapMemory(
            vkDevice,
            vkDeviceMemory,
            0U,
            VK_WHOLE_SIZE,
            0,
            reinterpret_cast<void**>( &m_pMapped )
        );
        if (vkResult != VK_SUCCESS){
            m_pMapped = NULL;
        }
    }

    VkMemoryMapping(VkMemoryMapping&& mapping) noexcept:
        m_vkDevice( mapping.m_vkDevice ),
        m_vkDeviceMemory( mapping.m_vkDeviceMemory ),
        m_pMapped( mapping.m_pMapped ) {

        mapping.reset( );
    }

    // We don't want a copy constructor
    VkMemoryMapping(const VkMemoryMapping&) = delete;

    ~VkMemoryMapping() {
        release( );
    }

    VkMemoryMapping& operator=(VkMemoryMapping&& mapping) {
        if (this != &mapping){
            release( );

            m_vkDevice = mapping.m_vkDevice;
            m_vkDeviceMemory = mapping.m_vkDeviceMemory;
            m_pMapped = mapping.m_pMapped;

            mapping.reset( );
        }
        return (*this);
    }

    // We also don't want an assignment operator
    VkMemoryMapping& operator=(VkMemoryMapping const&) = delete;

    operator bool() const {
        return m_pMapped != NULL;
    }

    operator uint8_t*() const {
        return m_pMapped;
    }
};

struct VkMemoryAllocation {
    VkDeviceSize vkSize;
    VkDeviceMemory vkDeviceMemory;
};

class VkMemoryAllocator {
private:
    VkPhysicalDevice m_vkPhysicalDevice;
    VkDevice m_vkDevice;
    VkMemoryPropertyFlags m_vkMemoryPropertyFlags;
    std::vector<VkDeviceMemory> m_allocations;

public:
    VkMemoryAllocator(VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkMemoryPropertyFlags vkMemoryPropertyFlags) noexcept:
        m_vkPhysicalDevice( vkPhysicalDevice ),
        m_vkDevice( vkDevice ),
        m_vkMemoryPropertyFlags( vkMemoryPropertyFlags ) { }

    ~VkMemoryAllocator() {
        // Free it all
        for (auto vkDeviceMemory : m_allocations){
            ::vkFreeMemory( m_vkDevice, vkDeviceMemory, VK_NULL_HANDLE );
        }
    }

    VkMemoryAllocation allocate(const std::vector<VkBuffer>& buffers) {
        // Setup
        VkMemoryAllocation vkMemoryAllocation = {};
        vkMemoryAllocation.vkSize = 0;
        VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties );

        // Scan the buffers to work out the required properties
        uint32_t uMemoryTypeFilter = 0;
        for (auto buffer : buffers) {
            VkMemoryRequirements vkMemoryRequirements = {};
            vkGetBufferMemoryRequirements( m_vkDevice, buffer, &vkMemoryRequirements );

            // Accumumlate
            vkMemoryAllocation.vkSize += vkMemoryRequirements.size;
            uMemoryTypeFilter |= vkMemoryRequirements.memoryTypeBits;

            // Accumulate the offset such that the memory is correctly aligned
            // for the buffer
            const VkDeviceSize vkOffset = (vkMemoryAllocation.vkSize % vkMemoryRequirements.alignment);
            vkMemoryAllocation.vkSize += vkOffset;
        }

        // Scan for a memory to back our input and output buffers
        uint32_t uMemoryTypeIdx = vkPhysicalDeviceMemoryProperties.memoryTypeCount;
        for (decltype(uMemoryTypeIdx) j = 0; j < vkPhysicalDeviceMemoryProperties.memoryTypeCount; ++j){
            VkMemoryType* vkMemoryType = (vkPhysicalDeviceMemoryProperties.memoryTypes + j);
            if ((vkMemoryType->propertyFlags & m_vkMemoryPropertyFlags) != m_vkMemoryPropertyFlags){
                continue;
            }

            if (vkPhysicalDeviceMemoryProperties.memoryHeaps[vkMemoryType->heapIndex].size < vkMemoryAllocation.vkSize){
                continue;
            }

            decltype(uMemoryTypeFilter) uMemoryTypeMask = (1 << j);
            if ((uMemoryTypeFilter & uMemoryTypeMask) != uMemoryTypeMask){
                continue;
            }

            uMemoryTypeIdx = j;
            break;
        }

        // If we found one, then grab it
        VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
        if (uMemoryTypeIdx < vkPhysicalDeviceMemoryProperties.memoryTypeCount){
            // Allocate
            VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
            vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            vkMemoryAllocateInfo.allocationSize = vkMemoryAllocation.vkSize;
            vkMemoryAllocateInfo.memoryTypeIndex = uMemoryTypeIdx;
            VkResult vkResult = vkAllocateMemory( m_vkDevice, &vkMemoryAllocateInfo, VK_NULL_HANDLE, &vkDeviceMemory );
            if (vkResult == VK_SUCCESS){
                m_allocations.push_back( vkDeviceMemory );
            }else{
                vkDeviceMemory = VK_NULL_HANDLE;
            }
        }
        if (vkDeviceMemory != VK_NULL_HANDLE){
            // Bind the buffers
            VkResult vkResult = VK_SUCCESS;
            VkDeviceSize vkMemoryOffset = 0U;
            for (auto buffer : buffers){
                VkMemoryRequirements vkMemoryRequirements = {};
                vkGetBufferMemoryRequirements( m_vkDevice, buffer, &vkMemoryRequirements );
                vkMemoryOffset += (vkMemoryOffset % vkMemoryRequirements.alignment);

                vkResult = vkBindBufferMemory( m_vkDevice, buffer, vkDeviceMemory, vkMemoryOffset );
                if (vkResult != VK_SUCCESS){
                    break;
                }
                vkMemoryOffset += vkMemoryRequirements.size;
            }
            if (vkResult != VK_SUCCESS){
                vkDeviceMemory = VK_NULL_HANDLE;
            }
        }

        // Return
        vkMemoryAllocation.vkDeviceMemory = vkDeviceMemory;
        return vkMemoryAllocation;
    }

    VkMemoryMapping map(size_t index) const {
        if (index >= m_allocations.size( )){
            return VkMemoryMapping( );
        }

        // Map the whole of the indicated allocation
        return VkMemoryMapping( m_vkDevice, m_allocations[index] );
    }
};
#endif // defined (VULKAN_SUPPORT)

// Function(s)
//

int vkSha256(int argc, const char* argv[]) {
#if defined (VULKAN_SUPPORT)
    using namespace std;

    // Returns a count of the 32-bit words required to store a string
    auto wc = [](const std::string& str) -> size_t {
        const auto size = str.size( );
        const size_t words = size / sizeof( uint32_t );
        return ((size % sizeof( uint32_t )) == 0)
            ? words
            : (words + 1);
    };

    // Agglomerate the inputs
    size_t sum = 0;
    vector<string> args;
    for (int i = 0; i < argc; ++i){
        const string arg( argv[i] );
        args.push_back( move( arg ) );

        sum += arg.size( );
    }
    if (sum == 0){
        // No inputs of any length, so just get out here
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
#if !defined(_ONDECK_)
    VkValidationFeatureEnableEXT vkValidationFeatureEnableEXT[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
    VkValidationFeaturesEXT vkValidationFeaturesEXT = {};
    vkValidationFeaturesEXT.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    vkValidationFeaturesEXT.enabledValidationFeatureCount = 1;
    vkValidationFeaturesEXT.pEnabledValidationFeatures = vkValidationFeatureEnableEXT;

    const char* szLayerName = "VK_LAYER_KHRONOS_validation";
    const char* pszExtensionNames[] = { "VK_EXT_validation_features", "VK_EXT_debug_utils" };
    vkCreateInfo.enabledLayerCount = 1;
    vkCreateInfo.ppEnabledLayerNames = &szLayerName;
    vkCreateInfo.enabledExtensionCount = 2;
    vkCreateInfo.ppEnabledExtensionNames = pszExtensionNames;
    vkCreateInfo.pNext = &vkValidationFeaturesEXT;
#endif

    VkInstance instance = VK_NULL_HANDLE;
    VkResult vkResult = vkCreateInstance( &vkCreateInfo, VK_NULL_HANDLE, &instance );
    if (vkResult == VK_SUCCESS){
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
            uint32_t uQueueFamilyIdx = vkQueueFamilyCount;
            for (decltype(vkQueueFamilyCount) j = 0; j < vkQueueFamilyCount; ++j){
                VkQueueFamilyProperties*  vkQueueFamilyProps = (vkQueueFamilyProperties + j);
                std::cout << "Queue family #" << j << " supports ";
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    std::cout << "graphics ";
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_COMPUTE_BIT){
                    std::cout << "compute ";
                    uQueueFamilyIdx = j;
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_TRANSFER_BIT){
                    std::cout << "transfer ";
                }
                std::cout << "on " << vkQueueFamilyProps->queueCount << " queue(s)." << endl;
            }
            delete[] vkQueueFamilyProperties;
            std::cout << endl;

            // Look for an early out
            if (uQueueFamilyIdx >= vkQueueFamilyCount){
                cerr << "Failed to find a compute queue; skipping this device." << endl;
                continue;
            }
            std::cout << "Selected queue family #" << uQueueFamilyIdx << endl;

            // Create a logical device w/the selected queue
            const float queuePrioritory = 1.0f;
            VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {};
            vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vkDeviceQueueCreateInfo.queueFamilyIndex = uQueueFamilyIdx;
            vkDeviceQueueCreateInfo.queueCount = 1;
            vkDeviceQueueCreateInfo.pQueuePriorities = &queuePrioritory;
            VkDeviceCreateInfo vkDeviceCreateInfo = {};
            vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            vkDeviceCreateInfo.queueCreateInfoCount = 1;
            vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;
            VkDevice vkDevice = VK_NULL_HANDLE;
            vkResult = vkCreateDevice( vkPhysicalDevice, &vkDeviceCreateInfo, VK_NULL_HANDLE, &vkDevice );
            if (vkResult != VK_SUCCESS){
                cerr << "Failed to create logical device on queue #" << uQueueFamilyIdx << endl;
            }

            // Get a handle the queue
            VkQueue vkQueue = VK_NULL_HANDLE;
            if (vkResult == VK_SUCCESS){
                vkGetDeviceQueue( vkDevice, uQueueFamilyIdx, 0, &vkQueue );
                if (vkQueue == VK_NULL_HANDLE){
                    cerr << "Failed to retrieve handle to device queue!" << endl;
                    vkResult = VK_ERROR_UNKNOWN;
                }
            }

            // Create the command pool
            VkCommandPool vkCommandPool = VK_NULL_HANDLE;
            if (vkResult == VK_SUCCESS){
                VkCommandPoolCreateInfo vkCommandPoolCreateInfo = {};
                vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                vkCommandPoolCreateInfo.queueFamilyIndex = uQueueFamilyIdx;
                vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                vkResult = vkCreateCommandPool( vkDevice, &vkCommandPoolCreateInfo, VK_NULL_HANDLE, &vkCommandPool );
            }

            // Allocate the command buffer
            VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
            if (vkResult == VK_SUCCESS){
                VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
                vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                vkCommandBufferAllocateInfo.commandPool = vkCommandPool;
                vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                vkCommandBufferAllocateInfo.commandBufferCount = 1;
                vkResult = vkAllocateCommandBuffers( vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer );
            }

            // Create some buffers
            if (vkResult == VK_SUCCESS){
                // Prep some allocators
                VkMemoryAllocator vkHostMemoryAllocator(
                    vkPhysicalDevice,
                    vkDevice,
                    (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                );
                VkMemoryAllocator vkDeviceMemoryAllocator(
                    vkPhysicalDevice,
                    vkDevice,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );

                // Calculate the number of words needed to hold the input
                VkDeviceSize vkInputWords = 0;
                for (auto& arg : args){
                    vkInputWords += wc( arg );
                }

                // Create us some buffers
                VkBuffer vkBufferResults = VK_NULL_HANDLE;
                VkBufferSet vkBufferSetInput( vkDevice, uQueueFamilyIdx, vkInputWords * sizeof( uint32_t ) );
                VkBufferSet vkBufferSetMetadata( vkDevice, uQueueFamilyIdx, args.size( ) * sizeof( VkSha256Metadata ) );
                if (vkBufferSetInput && vkBufferSetInput){
                    // A buffer for the outputs
                    VkBufferCreateInfo vkBufferCreateInfo = {};
                    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    vkBufferCreateInfo.queueFamilyIndexCount = 1;
                    vkBufferCreateInfo.pQueueFamilyIndices = &uQueueFamilyIdx;
                    vkBufferCreateInfo.size = args.size( ) * sizeof( VkSha256Result );
                    vkResult = vkCreateBuffer( vkDevice, &vkBufferCreateInfo, VK_NULL_HANDLE, &vkBufferResults );
                }
                if (vkResult == VK_SUCCESS){
                    // Allocate memory for the host-accessible buffers
                    VkBuffer vkBufferMetadata = vkBufferSetMetadata.host( );
                    VkBuffer vkBufferInputs = vkBufferSetInput.host( );
                    vector<VkBuffer> buffers = { vkBufferResults, vkBufferMetadata, vkBufferInputs };
                    VkMemoryAllocation allocated = vkHostMemoryAllocator.allocate( buffers );
                    if (allocated.vkDeviceMemory == VK_NULL_HANDLE){
                        vkResult = VK_ERROR_OUT_OF_HOST_MEMORY;
                    }else{
                        cout << "Allocated " << allocated.vkSize << " bytes of host-accessible memeory" << std::endl;
                        VkDeviceMemory vkDeviceMemory = allocated.vkDeviceMemory;

                        // Map the memory into our address space
                        uint8_t* pMapped = VK_NULL_HANDLE;
                        vkResult = vkMapMemory( vkDevice, vkDeviceMemory, 0, allocated.vkSize, 0, reinterpret_cast<void**>( &pMapped ) );
                        if (vkResult == VK_SUCCESS){
                            std::memset( pMapped, 0, allocated.vkSize );
                        }

                        // Populate the buffers
                        VkDeviceSize vkMemoryOffset = 0U;
                        if (vkResult == VK_SUCCESS){
                            VkMemoryRequirements vkMemoryRequirements = {};
                            vkGetBufferMemoryRequirements( vkDevice, vkBufferResults, &vkMemoryRequirements );
                            vkMemoryOffset += vkMemoryRequirements.size;
                        }
                        if (vkResult == VK_SUCCESS){
                            VkMemoryRequirements vkMemoryRequirements = {};
                            vkGetBufferMemoryRequirements( vkDevice, vkBufferMetadata, &vkMemoryRequirements );
                            vkMemoryOffset += (vkMemoryOffset % vkMemoryRequirements.alignment);

                            // Copy the sizes
                            uint32_t starting = 0;
                            auto pDst = pMapped + vkMemoryOffset;
                            for (auto it = args.cbegin( ), end = args.cend( ); it != end; ++it){
                                VkSha256Metadata metadata = {0};
                                metadata.start = starting;
                                metadata.size = static_cast<uint32_t>( it->size( ) );

                                std::memcpy( pDst, &metadata, sizeof( metadata ) );
                                pDst += sizeof( metadata );

                                starting += wc( *it );
                            }
                            vkMemoryOffset += vkMemoryRequirements.size;
                        }
                        if (vkResult == VK_SUCCESS){
                            VkMemoryRequirements vkMemoryRequirements = {};
                            vkGetBufferMemoryRequirements( vkDevice, vkBufferInputs, &vkMemoryRequirements );
                            vkMemoryOffset += (vkMemoryOffset % vkMemoryRequirements.alignment);

                            // Copy in the strings
                            auto pDst = pMapped + vkMemoryOffset;
                            for (auto it = args.cbegin( ), end = args.cend( ); it != end; ++it){
                                std::memcpy( pDst, it->data( ), it->size( ) );
                                pDst += (sizeof( uint32_t ) * wc( *it ));
                            }

                            // Unmap
                            vkUnmapMemory( vkDevice, vkDeviceMemory );
                        }
                    }
                }
                if (vkResult == VK_SUCCESS){
                    // Allocate memery for the device-accessible buffers
                    vector<VkBuffer> buffers = { vkBufferSetMetadata.device( ), vkBufferSetInput.device( ) };
                    VkMemoryAllocation allocated = vkDeviceMemoryAllocator.allocate( buffers );
                    if (allocated.vkDeviceMemory == VK_NULL_HANDLE){
                        vkResult = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                    }else{
                        std::cout << "Allocated " << allocated.vkSize << " byte(s) of GPU-accessible memory" << std::endl;

                        // Start issuing commands
                        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
                        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                        vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
                        vkBufferSetMetadata.xfer( vkCommandBuffer );
                        vkBufferSetInput.xfer( vkCommandBuffer );
                        vkEndCommandBuffer( vkCommandBuffer );

                        // Submit to the queue
                        VkSubmitInfo vkSubmitInfo = {};
                        vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        vkSubmitInfo.commandBufferCount = 1;
                        vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
                        vkResult = vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, VK_NULL_HANDLE );
                    }
                }
                if (vkResult == VK_SUCCESS){
                    // Create the shader module
                    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
                        vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                        vkShaderModuleCreateInfo.codeSize = code.size( ) * sizeof( uint32_t );
                        vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( code.data( ) );
                        vkResult = vkCreateShaderModule( vkDevice, &vkShaderModuleCreateInfo, VK_NULL_HANDLE, &vkShaderModule );
                    }

                    // Create the layout of the descriptor set
                    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding1 = {};
                        vkDescriptorSetLayoutBinding1.binding = 0;
                        vkDescriptorSetLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        vkDescriptorSetLayoutBinding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
                        vkDescriptorSetLayoutBinding1.descriptorCount = 1;
                        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding2 = vkDescriptorSetLayoutBinding1;
                        vkDescriptorSetLayoutBinding2.binding = 1;
                        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding3 = vkDescriptorSetLayoutBinding1;
                        vkDescriptorSetLayoutBinding3.binding = 2;
                        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBindings[] = {
                            vkDescriptorSetLayoutBinding1,
                            vkDescriptorSetLayoutBinding2,
                            vkDescriptorSetLayoutBinding3
                        };
                        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
                        vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                        vkDescriptorSetLayoutCreateInfo.bindingCount = 3;
                        vkDescriptorSetLayoutCreateInfo.pBindings = vkDescriptorSetLayoutBindings;
                        vkResult = vkCreateDescriptorSetLayout( vkDevice, &vkDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &vkDescriptorSetLayout );
                    }

                    // Create the pipeline layout
                    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
                        vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                        vkPipelineLayoutCreateInfo.setLayoutCount = 1;
                        vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
                        vkResult = vkCreatePipelineLayout( vkDevice, &vkPipelineLayoutCreateInfo, VK_NULL_HANDLE, &vkPipelineLayout );
                    }

                    // Initialise the compute pipeline
                    VkPipeline vkPipeline = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
                        vkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                        vkPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                        vkPipelineShaderStageCreateInfo.module = vkShaderModule;
                        vkPipelineShaderStageCreateInfo.pName = "main";
                        VkComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
                        vkComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                        vkComputePipelineCreateInfo.stage = vkPipelineShaderStageCreateInfo;
                        vkComputePipelineCreateInfo.layout = vkPipelineLayout;
                        vkResult = vkCreateComputePipelines( vkDevice, 0, 1, &vkComputePipelineCreateInfo, VK_NULL_HANDLE, &vkPipeline );
                    }

                    // Create the descriptor pool
                    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkDescriptorPoolSize vkDescriptorPoolSize = {};
                        vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        vkDescriptorPoolSize.descriptorCount = 3;
                        VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
                        vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                        vkDescriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                        vkDescriptorPoolCreateInfo.maxSets = 1;
                        vkDescriptorPoolCreateInfo.poolSizeCount = 1;
                        vkDescriptorPoolCreateInfo.pPoolSizes = &vkDescriptorPoolSize;
                        vkResult = vkCreateDescriptorPool( vkDevice, &vkDescriptorPoolCreateInfo, VK_NULL_HANDLE, &vkDescriptorPool );
                    }

                    // Allocate the descriptor set
                    VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
                        vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                        vkDescriptorSetAllocateInfo.descriptorPool = vkDescriptorPool;
                        vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
                        vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;
                        vkResult = vkAllocateDescriptorSets( vkDevice, &vkDescriptorSetAllocateInfo, &vkDescriptorSet );
                    }

                    // Update the descriptor set
                    if (vkResult == VK_SUCCESS){
                        VkDescriptorBufferInfo vkDescriptorBufferInputs = {};
                        vkDescriptorBufferInputs.buffer = vkBufferSetInput.device( );
                        vkDescriptorBufferInputs.range = VK_WHOLE_SIZE;
                        VkDescriptorBufferInfo vkDescriptorBufferMetadata = {};
                        vkDescriptorBufferMetadata.buffer = vkBufferSetMetadata.device( );
                        vkDescriptorBufferMetadata.range = VK_WHOLE_SIZE;
                        VkDescriptorBufferInfo vkDescriptorBufferResults = {};
                        vkDescriptorBufferResults.buffer = vkBufferResults;
                        vkDescriptorBufferResults.range = VK_WHOLE_SIZE;

                        VkWriteDescriptorSet vkWriteDescriptorSetInputs = {};
                        vkWriteDescriptorSetInputs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        vkWriteDescriptorSetInputs.dstSet = vkDescriptorSet;
                        vkWriteDescriptorSetInputs.descriptorCount = 1;
                        vkWriteDescriptorSetInputs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        vkWriteDescriptorSetInputs.pBufferInfo = &vkDescriptorBufferInputs;
                        VkWriteDescriptorSet vkWriteDescriptorSetMetadata = {};
                        vkWriteDescriptorSetMetadata.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        vkWriteDescriptorSetMetadata.dstSet = vkDescriptorSet;
                        vkWriteDescriptorSetMetadata.dstBinding = 1;
                        vkWriteDescriptorSetMetadata.descriptorCount = 1;
                        vkWriteDescriptorSetMetadata.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        vkWriteDescriptorSetMetadata.pBufferInfo = &vkDescriptorBufferMetadata;
                        VkWriteDescriptorSet vkWriteDescriptorSetResults = {};
                        vkWriteDescriptorSetResults.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        vkWriteDescriptorSetResults.dstSet = vkDescriptorSet;
                        vkWriteDescriptorSetResults.dstBinding = 2;
                        vkWriteDescriptorSetResults.descriptorCount = 1;
                        vkWriteDescriptorSetResults.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        vkWriteDescriptorSetResults.pBufferInfo = &vkDescriptorBufferResults;

                        VkWriteDescriptorSet vkWriteDescriptorSets[] = {
                            vkWriteDescriptorSetInputs,
                            vkWriteDescriptorSetMetadata,
                            vkWriteDescriptorSetResults
                        };
                        vkUpdateDescriptorSets( vkDevice, 3, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );
                    }

                    // Wait for the command buffer to become available, if it hasn't already
                    if (vkResult == VK_SUCCESS){
                        vkQueueWaitIdle( vkQueue );
                        vkResult = vkResetCommandBuffer( vkCommandBuffer, 0 );
                    }

                    // Start issuing commands
                    if (vkResult == VK_SUCCESS){
                        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
                        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                        vkResult = vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
                    }
                    if (vkResult == VK_SUCCESS){
                        vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline );
                        vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, VK_NULL_HANDLE );
                        vkCmdDispatch( vkCommandBuffer, args.size( ), 1, 1 );
                        vkResult = vkEndCommandBuffer( vkCommandBuffer );
                    }

                    // Create a fence
                    VkFence vkFence = VK_NULL_HANDLE;
                    if (vkResult == VK_SUCCESS){
                        VkFenceCreateInfo vkFenceCreateInfo = {};
                        vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                        vkResult = vkCreateFence( vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &vkFence );
                    }

                    // Submit to the queue
                    if (vkResult == VK_SUCCESS){
                        VkSubmitInfo vkSubmitInfo = {};
                        vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                        vkSubmitInfo.commandBufferCount = 1;
                        vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
                        vkResult = vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, vkFence );
                    }
                    if (vkResult == VK_SUCCESS){
                        vkResult = vkWaitForFences( vkDevice, 1, &vkFence, true, uint64_t(-1) );
                    }

                    // Get out the results
                    if (vkResult == VK_SUCCESS){
                        auto vkMemoryMapping = vkHostMemoryAllocator.map( 0 );
                        if (vkMemoryMapping){
                            auto pMapped = static_cast<uint8_t*>( vkMemoryMapping );
                            for (auto& arg : args){
                                VkSha256Result result = {};
                                std::memcpy( &result, pMapped, sizeof( VkSha256Result ) );
                                pMapped += sizeof( VkSha256Result );

                                std::cout << "Vulkan:- " << arg << " (" << wc( arg ) << ") >> " << print_bytes_ex( result.data, SHA256_WC ).str( ) << std::endl;
                            }
                        }else{
                            cerr << "Failed to retrieve the result" << std::endl;
                        }
                    }

                    // Cleanup
                    if (vkFence){
                        vkDestroyFence( vkDevice, vkFence, VK_NULL_HANDLE );
                    }
                    if (vkCommandBuffer){
                        vkFreeCommandBuffers( vkDevice, vkCommandPool, 1, &vkCommandBuffer );
                    }
                    if (vkDescriptorSet){
                        vkFreeDescriptorSets( vkDevice, vkDescriptorPool, 1, &vkDescriptorSet );
                    }
                    if (vkDescriptorPool){
                        vkDestroyDescriptorPool( vkDevice, vkDescriptorPool, VK_NULL_HANDLE );
                    }
                    if (vkPipeline){
                        vkDestroyPipeline( vkDevice, vkPipeline, VK_NULL_HANDLE );
                    }
                    if (vkPipelineLayout){
                        vkDestroyPipelineLayout( vkDevice, vkPipelineLayout, VK_NULL_HANDLE );
                    }
                    if (vkDescriptorSetLayout){
                        vkDestroyDescriptorSetLayout( vkDevice, vkDescriptorSetLayout, VK_NULL_HANDLE );
                    }
                    if (vkShaderModule){
                        vkDestroyShaderModule( vkDevice, vkShaderModule, VK_NULL_HANDLE );
                    }
                }
                if (vkBufferResults != VK_NULL_HANDLE){
                    vkDestroyBuffer( vkDevice, vkBufferResults, VK_NULL_HANDLE );
                }
            }
            if (vkCommandPool){
                vkDestroyCommandPool( vkDevice, vkCommandPool, VK_NULL_HANDLE );
            }
            if (vkDevice){
                vkDestroyDevice( vkDevice, VK_NULL_HANDLE );
            }
        }

        // Cleanup
        delete[] vkPhysicalDevices;
        vkDestroyInstance( instance, VK_NULL_HANDLE );
        return 0;
    }

    cerr << "Failed to initialise Vulkan" << endl;
#else
    std::cerr << "Vulkan not supported!" << std::endl;
#endif
    return 1;
}