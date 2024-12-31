// Batches.h: declares the types, functions and classes that batch (user/caller) inputs
//

#ifndef __VKMR_BATCHES_H__
#define __VKMR_BATCHES_H__

// Includes
//

// Vulkan Headers
#include <vulkan/vulkan.h>

// C++ Standard Library Headers
#include <memory>

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
    typedef size_t size_type;

    Batch(Batch&&);
    Batch(Batch const&) = delete;
    Batch(void) { Reset( ); }

    ~Batch(void) { Release( ); }

    Batch& operator=(Batch&&);
    Batch& operator=(Batch const&) = delete;
    operator bool() const { return (static_cast<bool>( m_data ) && static_cast<bool>( m_metadata )); }

    // Returns the number of strings in the batch
    size_type Count(void) const { return m_count; }

    // Returns the size of the batch, in bytes
    size_type Size(void) const;

    // Returns true if the batch is empty; false otherwise
    bool Empty(void) const {
        if (!(*this)){
            return true;
        }
        return (this->Count( ) == 0U);
    }

    // Returns the batch number
    number_type Number(void) const { return m_number; }

    // Pushes the given strings onto the batch
    bool Push(const ::std::vector<::std::string>&);

    // Pops the a given number of strings off the back of the batch
    void Pop(size_t);

    // Returns the buffer descriptors
    VkBufferDescriptors BufferDescriptors(void) const;

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

    // Gives the batch number
    number_type m_number;
};

// Encapsulates a collection of batches
class Batches {
public:
    Batches(Batches&&) noexcept;
    Batches(Batches&) = delete;
    Batches(VkDeviceSize vkDataSize):
        m_vkDataSize( vkDataSize ),
        m_vkMetadataSize( (vkDataSize / sizeof( VkSha256Result ) ) * sizeof( VkSha256Metadata ) ),
        m_count( 0U ) { }

    Batches& operator=(Batches&&) noexcept;
    Batches& operator=(Batches const&) = delete;
    operator bool() const {
        return (m_vkDataSize > 0U) && (m_vkMetadataSize > 0U);
    }

    // Returns the maximum number of batches that can
    // be concurrently in flight for the given device
    uint32_t MaxBatchCount(const ComputeDevice&) const;

    // Instantiates and returns a new batch
    Batch New(ComputeDevice&);

private:
    VkDeviceSize m_vkDataSize, m_vkMetadataSize;
    uint32_t m_count;
};

} // namespace vkmr

#endif // __VKMR_BATCHES_H__
