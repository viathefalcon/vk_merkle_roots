// Ops.h: declares the types, functions and classes which encapsulate asynchronous on-device operations
//

#ifndef __VKMR_OPS_H__
#define __VKMR_OPS_H__

// Includes
//

// Local Project Headers
#include "Slices.h"
#include "Batches.h"

#if defined (VULKAN_SUPPORT)
namespace vkmr {
// Encapsulates a mapping of an input batch to a slice of device memory
class Mapping {
public:
    Mapping(Mapping&&);
    Mapping(VkDevice, DescriptorSet&&, CommandBuffer&&);
    Mapping(Mapping const&) = delete;

    Mapping(void) { Reset( ); }
    ~Mapping(void) { Release( ); }

    Mapping& operator=(Mapping const&) = delete;
    Mapping& operator=(Mapping&&);
    operator bool() const { return (m_vkResult == VK_SUCCESS); }
    operator VkResult() const { return m_vkResult; }

    Mapping& Apply(Batch&, Slice<VkSha256Result>&, vkmr::Pipeline&);
    VkResult Dispatch(VkQueue);

    static Pipeline Pipeline(ComputeDevice&);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkFence m_vkFence;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

// Encapsulates the reduction of a slice to a single value
class Reduction {
public:
    Reduction(Reduction&&);
    Reduction(VkDevice, DescriptorSet&&, CommandBuffer&&);
    Reduction(Reduction const&) = delete;

    Reduction(void) { Reset( ); }
    ~Reduction(void) { Release( ); }

    Reduction& operator=(Reduction const&) = delete;
    Reduction& operator=(Reduction&&);
    operator bool() const { return (m_vkResult == VK_SUCCESS); }
    operator VkResult() const { return m_vkResult; }

    Reduction& Apply(Slice<VkSha256Result>&, ComputeDevice&, vkmr::Pipeline&);
    VkResult Dispatch(VkQueue);

    static Pipeline Pipeline(ComputeDevice&);

private:
    void Free(void);
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkDeviceSize m_vkSize;
    size_t m_count;

    VkFence m_vkFence;
    VkBuffer m_vkBufferHost;
    VkDeviceMemory m_vkHostMemory;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

} // namespace vkmr
#endif // VULKAN_SUPPORT
#endif // __VKMR_OPS_H__
