// Batches.h: declares the types, functions and classes that slices memory for inputs
//

#ifndef __VKMR_BATCHES_H__
#define __VKMR_BATCHES_H__

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

// Includes
//

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>

// C++ Standard Library Headers
#include <memory>

// Local Project Headers
#include "Devices.h"
#include "../common/SHA-256defs.h"

namespace vkmr {

// Class(es)
//

// Encapsulates a batch of inputs
class Batch {
public:
    typedef struct {

        VkDescriptorBufferInfo vkDescriptorBufferInputs;
        VkDescriptorBufferInfo vkDescriptorBufferMetdata;

    } VkBufferDescriptors;

    Batch(Batch&&);
    Batch(Batch const&) = delete;
    Batch(void) { Reset( ); }

    ~Batch(void) { Release( ); }

    Batch& operator=(Batch&&);
    Batch& operator=(Batch const&) = delete;
    operator bool() const { return (m_pData != nullptr); }

    // Returns the underlying buffer
    VkBuffer Buffer(void) const { return m_vkBuffer; } 

    // Returns the size of the batch
    VkDeviceSize Size(void) const { return m_vkSize; }

    // Returns the number of strings in the batch
    size_t Count(void) const { return m_count; }

    // Prepares the batch for reuse
    void Reuse(void);

    // Pushes the given string onto the batch
    bool Push(const char*, size_t);

    // Pops the last string off the batch
    void Pop(void);

    // Returns the buffer descriptors
    VkBufferDescriptors BufferDescriptors(void) const;

    // Allocates and returns a new batch
    static Batch New(ComputeDevice&, VkDeviceSize);

private:
    Batch(const ComputeDevice&, VkDeviceMemory, VkDeviceSize);

    void Reset(void);
    void Release(void);

    // Adjusts the given offset to be aligned to the lowest common multiple
    // of the minimum such offset for the underlying device and a given type
    size_t AlignOffset(size_t, size_t);

    // Counts the number of 32-bit words
    // needed to hold a string of the given (byte)
    // length
    static uint32_t WordCount(size_t);

    VkDevice m_vkDevice;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkDeviceMemory;
    VkDeviceSize m_vkSize, m_minStorageBufferOffsetAlignment;

    void* m_pData;
    size_t m_count, m_metadata_offset;
};

} // namespace vkmr

#endif // VULKAN_SUPPORT
#endif // __VKMR_BATCHES_H__
