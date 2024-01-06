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
    DescriptorSet(VkDevice, VkDescriptorPool, VkDescriptorSetLayout);
    DescriptorSet(DescriptorSet&&);
    DescriptorSet(DescriptorSet const&) = delete;
    DescriptorSet(void) { Reset( ); }
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

    VkDescriptorSet m_vkDescriptorSet;
};

// Encapsulates a command buffer
class CommandBuffer {
public:
    CommandBuffer(VkDevice, VkCommandPool);
    CommandBuffer(CommandBuffer&&);
    CommandBuffer(CommandBuffer const&) = delete;
    CommandBuffer(void) { Reset( ); }
    ~CommandBuffer(void) { Release( ); }

    CommandBuffer& operator=(CommandBuffer&&);
    CommandBuffer& operator=(CommandBuffer const&) = delete;
    operator bool() const { return (m_vkCommandBuffer != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }

    // Returns the underlying command buffer
    VkCommandBuffer operator * () const { return m_vkCommandBuffer; }

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkCommandPool m_vkCommandPool;

    VkCommandBuffer m_vkCommandBuffer;
};

typedef struct {
    uint32_t memoryTypeIndex;
    VkDeviceSize vkMemoryBudget;
    VkMemoryPropertyFlags vkMemoryPropertyFlags;
} MemoryTypeBudget;

// Encapsulates a pipeline
class Pipeline {
public:
    Pipeline(VkDevice, VkShaderModule, VkDescriptorSetLayout);
    Pipeline(Pipeline&&);
    Pipeline(Pipeline const&) = delete;

    Pipeline(void) { Reset( ); }
    ~Pipeline(void) { Release( ); }

    Pipeline& operator=(Pipeline const&) = delete;
    Pipeline& operator=(Pipeline&&);

    operator bool() const { return (m_vkPipeline != VK_NULL_HANDLE); }
    VkPipeline operator * () const { return m_vkPipeline; }

    VkDescriptorSetLayout DescriptorSetLayout(void) const { return m_vkDescriptorSetLayout; }
    VkPipelineLayout Layout(void) const { return m_vkPipelineLayout; }

private:
    void Reset(void);
    void Release(void);

    VkDevice m_vkDevice;
    VkShaderModule m_vkShaderModule;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
    VkPipelineLayout m_vkPipelineLayout;
    VkPipeline m_vkPipeline;
};

// Encapsulates a (logical, Vulkan) compute device
class ComputeDevice {
public:
    ComputeDevice(VkPhysicalDevice, uint32_t, uint32_t);
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
    VkQueue Queue(uint32_t) const;

    // Returns the memory requirement of storage buffers
    // created with the device
    VkMemoryRequirements StorageBufferRequirements(VkDeviceSize) const;

    // Returns the requested physical device properties
    void GetPhysicalDeviceProperties2KHR(VkPhysicalDeviceProperties2KHR*) const;

    // Returns the minimum alignment of a storage buffer offset
    VkDeviceSize MinStorageBufferOffset(void) const;

    // Allocates and returns a descriptor set
    DescriptorSet AllocateDescriptorSet(void) const;

    // Allocates and returns a descriptor set for the given pipeline
    DescriptorSet AllocateDescriptorSet(Pipeline&) const;

    // Allocates and returns a command buffer
    CommandBuffer AllocateCommandBuffer() const;

    // Returns the available memory types corresponding to the given requirements
    typedef ::std::vector<MemoryTypeBudget> MemoryTypeBudgets;
    MemoryTypeBudgets AvailableMemoryTypes(const VkMemoryRequirements&, VkMemoryPropertyFlags) const;

    // Allocates up to the given amount of device memory
    VkDeviceMemory Allocate(const MemoryTypeBudget&, VkDeviceSize vkSize);

    // Frees the given, previously-allocated bit of memory
    void Free(VkDeviceMemory) const;    

private:
    void Reset(void);
    void Release(void);

    VkPhysicalDevice m_vkPhysicalDevice;
    uint32_t m_queueFamily, m_queueCount;

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkCommandPool m_vkCommandPool;
    VkDescriptorPool m_vkDescriptorPool;
};

} // namespace vkmr

#endif // VULKAN_SUPPORT
#endif // __VKMR_DEVICES_H__
