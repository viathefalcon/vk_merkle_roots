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
#include <vector>

// Local Project Headers
#include "Devices.h"
#include "../common/SHA-256defs.h"

namespace vkmr {

// Forward Declarations
//

class Batches;

// Class(es)
//

// Encapsulates a batch of inputs
class Batch {
    friend class Batches;

public:
    typedef struct {

        VkDescriptorBufferInfo vkDescriptorBufferInputs;
        VkDescriptorBufferInfo vkDescriptorBufferMetdata;

    } VkBufferDescriptors;

    typedef uint32_t number_type;

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

    // Locks the batch for further writing
    Batch& Lock(void) {
        m_locked = true;
        return (*this);
    }

    // Unlocks the batch for further writing
    Batch& Unlock(void) {
        m_locked = false;
        return (*this);
    }

    // Returns true if the batch is locked; false otherwise
    inline bool IsLocked(void) const {
        return m_locked;
    }

    // Enables the batch for re-use
    Batch& Reuse(void) {
        Unlock( );
        m_count = 0U;
        return (*this);
    }

    // Returns the batch number
    number_type Number(void) const { return m_number; }

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

    Batch(number_type, Buffer&&, Buffer&&);

    void Reset(void);
    void Release(void);

    // Returns the metadata of the last string in the batch
    VkSha256Metadata Back() const;

    // Counts the number of 32-bit words
    // needed to hold a string of the given (byte)
    // length
    static uint32_t WordCount(size_t);

    // Buffers to hold the batch data and metadata, respectively
    Buffer m_data, m_metadata;

    // Gives the number of inputs in the batch
    size_t m_count;

    // Indicates whether the batch is locked for further writing
    bool m_locked;

    // Gives the batch number
    number_type m_number;
};

// Encapsulates a collection of batches
class Batches {
public:
    Batches(Batches&&) noexcept;
    Batches(Batches&) = delete;
    Batches(VkDeviceSize vkDataSize, VkDeviceSize vkMetadataSize):
        m_vkDataSize( vkDataSize ),
        m_vkMetadataSize( vkMetadataSize ),
        m_last( -1 ) { }

    Batch& operator[](int);
    Batches& operator=(Batches&&) noexcept;
    Batches& operator=(Batches const&) = delete;
    operator bool() const {
        return (m_vkDataSize > 0U) && (m_vkMetadataSize > 0U);
    }

    // Returns a reference to a batch available for writing
    Batch& Get(ComputeDevice&);

    // Returns a reference to the most-recently-got batch
    Batch& Last(void);

    // Resets to an invalid state
    Batches& Reset(void);

private:
    VkDeviceSize m_vkDataSize, m_vkMetadataSize;

    int m_last;
    ::std::vector<Batch> m_batches;
    Batch m_empty;
};

} // namespace vkmr

#endif // VULKAN_SUPPORT
#endif // __VKMR_BATCHES_H__
