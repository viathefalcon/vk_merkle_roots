// Devices.h: declares the types, functions and classes that encapsulate GPU devices
//

#ifndef __VKMR_DEVICES_H__
#define __VKMR_DEVICES_H__

// Includes
//

// C++ Standard Library Headers
#include <vector>

// Local Project Headers
#include "Shaders.h"

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

// Encapsulates a command pool
class CommandPool {
public:
    CommandPool(VkDevice, uint32_t);
    CommandPool(CommandPool&&);
    CommandPool(CommandPool const&) = delete;
    CommandPool(void) { Reset( ); }
    ~CommandPool(void) { Release( ); }

    CommandPool& operator=(CommandPool&&);
    CommandPool& operator=(CommandPool const&) = delete;
    operator bool() const { return (m_vkCommandPool != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }

    // Allocate a new command buffer from the pool
    CommandBuffer AllocateCommandBuffer(void);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;

    VkCommandPool m_vkCommandPool;
};

typedef struct {
    uint32_t heapIndex;
    uint32_t memoryTypeIndex;
    VkDeviceSize vkMemoryBudget, vkMemorySize;
    VkMemoryPropertyFlags vkMemoryPropertyFlags;
} MemoryTypeBudget;

struct WorkgroupSize {
    uint32_t x, y, z;
    bool bySubgroup = false;

    uint32_t GetGroupCountX(uint32_t) const;
};

// Encapsulates a pipeline
class Pipeline {
public:
    Pipeline(VkDevice, VkDescriptorSetLayout, VkPipelineLayout, ShaderModule&&, const WorkgroupSize* pWorkGroupSize = nullptr);
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
    const WorkgroupSize& GetWorkGroupSize(void) const { return m_workGroupSize; }

    static VkPipelineLayout NewSimpleLayout(VkDevice, VkDescriptorSetLayout, const VkPushConstantRange* pVkPushConstantRange = nullptr);

private:
    void Reset(void);
    void Release(void);

    VkDevice m_vkDevice;
    VkDescriptorSetLayout m_vkDescriptorSetLayout;
    VkPipelineLayout m_vkPipelineLayout;
    VkPipeline m_vkPipeline;

    ShaderModule m_shaderModule;
    WorkgroupSize m_workGroupSize;
};

// Encapsulates a descriptor pool
class DescriptorPool {
public:
    DescriptorPool(VkDevice, uint32_t, uint32_t);
    DescriptorPool(DescriptorPool&&);
    DescriptorPool(DescriptorPool const&) = delete;
    DescriptorPool(void) { Reset( ); }
    ~DescriptorPool(void) { Release( ); }

    DescriptorPool& operator=(DescriptorPool&&);
    DescriptorPool& operator=(DescriptorPool const&) = delete;
    operator bool() const { return (m_vkDescriptorPool != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }

    // Returns the underlying descriptor pool
    VkDescriptorPool operator * () const { return m_vkDescriptorPool; }

    // Allocate a new descriptor set from the pool for the given pipeline
    DescriptorSet AllocateDescriptorSet(Pipeline&) const;

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;

    VkDescriptorPool m_vkDescriptorPool;
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

    // Creates a new descriptor pool
    DescriptorPool CreateDescriptorPool(uint32_t, uint32_t) const;

    // Creates a new command pool
    CommandPool CreateCommandPool(void) const;

    // Returns a handle to a queue
    VkQueue Queue(void);

    // Returns the memory requirement of storage buffers
    // created with the device
    VkMemoryRequirements StorageBufferRequirements(VkDeviceSize) const;

    // Returns the requested physical device properties
    void GetPhysicalDeviceProperties2KHR(VkPhysicalDeviceProperties2KHR*) const;

    // Returns the minimum alignment of a storage buffer offset
    VkDeviceSize MinStorageBufferOffset(void) const;

    // Returns the smaller of the given size and the effective maximum buffer size
    // (we only deal in whole buffers, so the range becomes the limit)
    VkDeviceSize MaxStorageBufferSize(uint32_t) const;

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
    uint32_t m_queueFamily, m_queueCount, m_queueNext;

    VkResult m_vkResult;
    VkDevice m_vkDevice;
};

} // namespace vkmr

#endif // __VKMR_DEVICES_H__
