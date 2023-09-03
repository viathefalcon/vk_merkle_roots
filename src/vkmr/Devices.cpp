// Devices.cpp: defines the types, functions and classes that slices memory for inputs and outputs
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <iostream>

// Local Project Headers
#include "Debug.h"

// Declarations
#include "Devices.h"

#if defined (VULKAN_SUPPORT)
namespace vkmr {

Device::Device(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily, uint32_t queueCount):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( VK_NULL_HANDLE),
    m_queueFamily( queueFamily ),
    m_queueCount( queueCount ) {

    using ::std::vector;
    using ::std::cout;
    using ::std::endl;
 
    // Enumerate the device extensions
    vector<char*> deviceExtNames;
    uint32_t uDeviceExtensionPropertyCount = 0;
    vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, VK_NULL_HANDLE, &uDeviceExtensionPropertyCount, VK_NULL_HANDLE );
    if (uDeviceExtensionPropertyCount > 0){
        // Allocate and query
        VkExtensionProperties* pVkExtensionProperties = new (std::nothrow) VkExtensionProperties[uDeviceExtensionPropertyCount];
        if (pVkExtensionProperties){
            cout << "Extensions: ";
            vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, VK_NULL_HANDLE, &uDeviceExtensionPropertyCount, pVkExtensionProperties );
            for (decltype(uDeviceExtensionPropertyCount) u = 0; u < uDeviceExtensionPropertyCount; ++u){
                VkExtensionProperties* vkExtensionProperties = (pVkExtensionProperties + u);
                const char* extensionName = vkExtensionProperties->extensionName;
                cout << extensionName << " ";

                if (strcmp( extensionName, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME );
                    continue;
                }
                if (strcmp( extensionName, VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME );
                    continue;
                }
                if (strcmp( extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );
                    continue;
                }
            }
            cout << endl;
            delete[] pVkExtensionProperties;
        }
    }

    // Try and create the device along w/all the queues..
    vector<float> queuePriorities(queueCount, 1.0f);
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {};
    vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vkDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
    vkDeviceQueueCreateInfo.queueCount = queuePriorities.size( );
    vkDeviceQueueCreateInfo.pQueuePriorities = queuePriorities.data( );
    VkDeviceCreateInfo vkDeviceCreateInfo = {};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.queueCreateInfoCount = 1;
    vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;
    vkDeviceCreateInfo.enabledExtensionCount = deviceExtNames.size( );
    vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtNames.data( );
    m_vkResult = vkCreateDevice( vkPhysicalDevice, &vkDeviceCreateInfo, VK_NULL_HANDLE, &m_vkDevice );
}

Device::Device(Device&& device):
    m_vkResult( device.m_vkResult ),
    m_vkDevice( device.m_vkDevice),
    m_queueFamily( device.m_queueFamily ),
    m_queueCount( device.m_queueCount ) {

    device.Release( );
}

Device& Device::operator=(Device&& device) {

    if (this != &device){
        Release( );

        m_vkResult = device.m_vkResult;
        m_vkDevice = device.m_vkDevice;
        m_queueFamily = device.m_queueFamily;
        m_queueCount = device.m_queueCount;

        device.Reset( );
    }
    return (*this);
}

VkQueue Device::Queue(uint32_t queueIndex) {

    VkQueue vkQueue = VK_NULL_HANDLE;
    ::vkGetDeviceQueue( m_vkDevice, m_queueFamily, queueIndex, &vkQueue );
    if (vkQueue == VK_NULL_HANDLE){
        ::std::cerr << "Failed to retrieve handle to device queue!" << ::std::endl;
    }
    return vkQueue;
}

VkMemoryRequirements Device::StorageBufferRequirements(VkDeviceSize vkSize) {

    // Setup
    VkMemoryRequirements vkMemoryRequirements = {};

    // Create the buffer
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkBufferCreateInfo.size = vkSize;
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    m_vkResult = ::vkCreateBuffer(
        m_vkDevice,
        &vkBufferCreateInfo,
        VK_NULL_HANDLE,
        &vkBuffer
    );
    if (m_vkResult == VK_SUCCESS){
        // Query the buffer
        ::vkGetBufferMemoryRequirements( m_vkDevice, vkBuffer, &vkMemoryRequirements );
        ::vkDestroyBuffer( m_vkDevice, vkBuffer, VK_NULL_HANDLE );
    }
    return vkMemoryRequirements;
}

void Device::Reset() {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_queueCount = 0;
    m_queueFamily = (uint32_t)(-1);
}

void Device::Release() {

    if (m_vkDevice != VK_NULL_HANDLE){
        ::vkDestroyDevice( m_vkDevice, VK_NULL_HANDLE );
    }
    Reset( );
}

}
#endif // VULKAN_SUPPORT
