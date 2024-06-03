// Batches.h: declares the types, functions and classes that batch (user/caller) inputs
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
    operator bool() const { return (static_cast<bool>( m_data ) && static_cast<bool>( m_metadata )); }

    // Returns the number of strings in the batch
    size_t Count(void) const { return m_count; }

    // Pushes the given string onto the batch
    bool Push(const char*, size_t);

    // Pops the last string off the batch
    void Pop(void);

    // Returns the buffer descriptors
    VkBufferDescriptors BufferDescriptors(void) const;

    // Allocates and returns a new batch
    static Batch New(ComputeDevice&, VkDeviceSize);

private:
    struct Buffer {
        VkDevice vkDevice;
        VkBuffer vkBuffer;
        VkDeviceMemory vkDeviceMemory;
        VkDeviceSize vkSize;
        void* pData;

        Buffer(Buffer const&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer(VkDevice, VkDeviceMemory, VkDeviceSize) noexcept;
        Buffer() noexcept { Reset( ); }
        ~Buffer() noexcept { Release( ); }

        operator bool() const { return (pData != nullptr); }
        Buffer& operator=(Buffer const&) = delete;
        Buffer& operator=(Buffer&&) noexcept;

        void Reset(void);
        void Release(void);
    };

    Batch(Buffer&&, Buffer&&);

    void Reset(void);
    void Release(void);

    // Returns the metadata of the last string in the batch
    VkSha256Metadata Back() const;

    // Counts the number of 32-bit words
    // needed to hold a string of the given (byte)
    // length
    static uint32_t WordCount(size_t);

    Buffer m_data, m_metadata;
    size_t m_count;
};

} // namespace vkmr

#endif // VULKAN_SUPPORT
#endif // __VKMR_BATCHES_H__
