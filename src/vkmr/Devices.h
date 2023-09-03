// Devices.h: declares the types, functions and classes that encapsulate GPU devices
//

#ifndef __VKMR_DEVICES_H__
#define __VKMR_DEVICES_H__

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

#if defined (VULKAN_SUPPORT)
// Includes
//

// Vulkan Headers
#include <vulkan/vulkan.h>

namespace vkmr {

// Class(es)
//

// Encapsulates a (logical, Vulkan) device
class Device {
public:
    Device(VkPhysicalDevice, uint32_t, uint32_t);
    Device(Device&&);
    Device(const Device&) = delete;
    Device(void) { Reset( ); }
    ~Device(void) { Release( ); }

    Device& operator=(Device const&) = delete;
    Device& operator=(Device&& pair);
    operator bool() const { return (m_vkDevice != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }
    
    // Returns the underlying Vulkan device
    VkDevice& operator * () { return m_vkDevice; }

    // Returns the index of the device's queue family
    uint32_t QueueFamily(void) const { return m_queueFamily; }

    // Returns the queue with the given index
    VkQueue Queue(uint32_t);

    // Returns the memory requirement of storage buffers
    // created with the device
    VkMemoryRequirements StorageBufferRequirements(VkDeviceSize);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    uint32_t m_queueFamily, m_queueCount;
};

}

#endif // VULKAN_SUPPORT
#endif // __VKMR_DEVICES_H__
