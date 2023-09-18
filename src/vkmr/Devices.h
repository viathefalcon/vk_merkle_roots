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

// C++ Standard Library Headers
#include <vector>

// Vulkan Headers
#include <vulkan/vulkan.h>

namespace vkmr {

// Class(es)
//

// Encapsulates a descriptor set
class DescriptorSet {
public:
    DescriptorSet(VkDevice, VkDescriptorPool, VkDescriptorSetLayout, uint32_t);
    DescriptorSet(DescriptorSet&&);
    DescriptorSet(DescriptorSet&) = delete;
    DescriptorSet(void)  { Reset( ); }
    ~DescriptorSet(void) { Release( ); }

    DescriptorSet& operator=(DescriptorSet&&);
    DescriptorSet& operator=(DescriptorSet const&) = delete;
    operator bool() const { return (m_vkDescriptorSet != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }

    // Returns the underlying descriptor set
    VkDescriptorSet operator * () const { return m_vkDescriptorSet; }

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkDescriptorPool m_vkDescriptorPool;
    uint32_t m_descriptorSetCount;

    VkDescriptorSet m_vkDescriptorSet;
};

// Encapsulates a command buffer
class CommandBuffer {
public:
    CommandBuffer(VkDevice, VkCommandPool, VkPipelineLayout, VkPipeline);
    CommandBuffer(CommandBuffer&&);
    CommandBuffer(CommandBuffer&) = delete;
    CommandBuffer(void)  { Reset( ); }
    ~CommandBuffer(void) { Release( ); }

    CommandBuffer& operator=(CommandBuffer&&);
    CommandBuffer& operator=(CommandBuffer const&) = delete;
    operator bool() const { return (m_vkCommandBuffer != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }

    // Returns the underlying command buffer
    VkCommandBuffer operator * () const { return m_vkCommandBuffer; }

    // Updates the command buffer
    VkResult BindDispatch(DescriptorSet&, uint32_t);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkCommandPool m_vkCommandPool;
    VkPipelineLayout m_vkPipelineLayout;
    VkPipeline m_vkPipeline;

    VkCommandBuffer m_vkCommandBuffer;
};

// Encapsulates a (logical, Vulkan) compute device
class ComputeDevice {
public:
    ComputeDevice(VkPhysicalDevice, uint32_t, uint32_t, const ::std::vector<uint32_t>&);
    ComputeDevice(ComputeDevice&&);
    ComputeDevice(const ComputeDevice&) = delete;
    ComputeDevice(void) { Reset( ); }
    ~ComputeDevice(void) { Release( ); }

    ComputeDevice& operator=(ComputeDevice const&) = delete;
    ComputeDevice& operator=(ComputeDevice&&);
    operator bool() const { return (m_vkDevice != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }
    
    // Returns the underlying Vulkan device
    VkDevice operator * () const { return m_vkDevice; }

    // Returns a handle to the physical device
    VkPhysicalDevice PhysicalDevice(void) const { return m_vkPhysicalDevice; }

    // Returns the queue with the given index
    VkQueue Queue(uint32_t);

    // Returns the memory requirement of storage buffers
    // created with the device
    VkMemoryRequirements StorageBufferRequirements(VkDeviceSize) const;

    // Returns the minimum alignment of a storage buffer offset
    VkDeviceSize MinStorageBufferOffset(void) const;

    // Allocates and returns a descriptor set
    DescriptorSet AllocateDescriptorSet(uint32_t) const;

    // Allocates and returns a command buffer
    CommandBuffer AllocateCommandBuffer(void) const;

private:
    void Reset(void);
    void Release(void);

    VkPhysicalDevice m_vkPhysicalDevice;
    uint32_t m_queueFamily, m_queueCount;

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkShaderModule m_vkShaderModule;
    VkCommandPool m_vkCommandPool;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
    VkPipelineLayout m_vkPipelineLayout;
    VkPipeline m_vkPipeline;
    VkDescriptorPool m_vkDescriptorPool;
};

}

#endif // VULKAN_SUPPORT
#endif // __VKMR_DEVICES_H__
