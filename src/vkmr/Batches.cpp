// Batches.cpp: defines the types, functions and classes that slices memory for inputs
//

// Includes
//

// Local Project Headers
#include "Debug.h"

// Declarations
#include "Batches.h"

#if defined (VULKAN_SUPPORT)
namespace vkmr {

// Classes
//

Batch::Batch(Batch&& batch):
    m_vkDevice( batch.m_vkDevice ),
    m_vkBuffer( batch.m_vkBuffer ),
    m_vkDeviceMemory( batch.m_vkDeviceMemory ),
    m_vkSize( batch.m_vkSize ),
    m_minStorageBufferOffsetAlignment( batch.m_minStorageBufferOffsetAlignment ),
    m_pData( batch.m_pData ),
    m_count( batch.m_count ),
    m_metadata_offset( batch.m_metadata_offset ) {

    batch.Reset( );
}

Batch& Batch::operator=(Batch&& batch) {

    if (this != &batch){
        Release( );

        m_vkDevice = batch.m_vkDevice;
        m_vkBuffer = batch.m_vkBuffer;
        m_vkDeviceMemory = batch.m_vkDeviceMemory;
        m_vkSize = batch.m_vkSize;
        m_minStorageBufferOffsetAlignment = batch.m_minStorageBufferOffsetAlignment;
        m_pData = batch.m_pData;
        m_count = batch.m_count;
        m_metadata_offset = batch.m_metadata_offset;

        batch.Reset( );
    }
    return (*this);
}

void Batch::Reuse(void) {

    // Apparently important on Steam Deck..?
    ::std::memset( m_pData, 0, m_vkSize ); 

    // Set the initial position of the metadata relative to the start of the buffer
    m_metadata_offset = AlignOffset( m_vkSize / 2, sizeof( VkSha256Metadata ) );
}

bool Batch::Push(const char* str, size_t len) {

    // Look for an early out
    if (!(*this)){
        return false;
    }
    if (!str || (len == 0)){
        return true; // Silently skip
    }

    // Setup
    using ::std::memcpy;
    const auto wc = WordCount( len );
    const auto new_metadata_size = (sizeof( VkSha256Metadata ) * (m_count + 1));

    auto ptr = static_cast<unsigned char*>( m_pData );
    auto metadata_offset = m_metadata_offset;

    // Check if we can accomodate the string + metadata
    VkSha256Metadata back = { 0 };
    if (m_count > 0){
        // Copy onto the stack
        memcpy(
            &back,
            ptr + m_metadata_offset + ((m_count - 1) * sizeof( VkSha256Metadata )),
            sizeof( VkSha256Metadata )
        );

        // Calculate where we would need to put the metadata block,
        // and then check if the metadata would fit there (i.e. not
        // overrun the end of the buffer)
        metadata_offset = AlignOffset(
            back.start + WordCount( back.size ) + wc,
            sizeof( VkSha256Metadata )
        );
        if ((metadata_offset + new_metadata_size) > m_vkSize){
            // Nope
            return false;
        }
    }

    // If we get here, then we can accomodate the string, so start copying
    // things into place
    if (metadata_offset > m_metadata_offset){
        // To reduce the number of moves within the lifetime of the batch
        // try to bisect the available space
        const auto alt_offset = AlignOffset(
            (m_vkSize - (back.start + WordCount( back.size ) + wc + new_metadata_size)) / 2,
            sizeof( VkSha256Metadata )
        );
        if ((alt_offset + new_metadata_size) < m_vkSize){
            metadata_offset = alt_offset;
        }

        // Move the metadata block to allow us to append the string
        // without overrwriting the metadata
        ::std::memmove(
            ptr + metadata_offset,
            ptr + m_metadata_offset,
            sizeof( VkSha256Metadata ) * m_count
        );
        m_metadata_offset = metadata_offset;
    }

    // Append the metadata
    back.start = (back.start + WordCount( back.size ));
    back.size = static_cast<uint>( len );
    memcpy(
        ptr + m_metadata_offset + (sizeof( VkSha256Metadata ) * m_count),
        &back,
        sizeof( VkSha256Metadata )
    );

    // Append the string
    memcpy(
        ptr + (back.start * sizeof( uint )),
        str,
        sizeof( char ) * len
    );

    // Update our state and give the caller the happy news
    m_count += 1;
    return true;
}

void Batch::Pop(void) {

    if (m_count > 0){
        m_count -= 1U;
    }
}

Batch::VkBufferDescriptors Batch::BufferDescriptors(void) const {

    // Get the last metadata element, if any
    VkSha256Metadata back = { 0 };
    if (m_count > 0){
        // Copy onto the stack
        ::std::memcpy(
            &back,
            static_cast<unsigned char*>( m_pData ) + m_metadata_offset + ((m_count - 1) * sizeof( VkSha256Metadata )),
            sizeof( VkSha256Metadata )
        );
    }

    // Generate
    VkDescriptorBufferInfo vkDescriptorBufferInputs = {};
    vkDescriptorBufferInputs.buffer = m_vkBuffer;
    vkDescriptorBufferInputs.offset = 0;
    vkDescriptorBufferInputs.range = ((back.start + WordCount( back.size )) * sizeof( uint32_t ));
    VkDescriptorBufferInfo vkDescriptorBufferMetadata = {};
    vkDescriptorBufferMetadata.buffer = m_vkBuffer;
    vkDescriptorBufferMetadata.offset = static_cast<VkDeviceSize>( m_metadata_offset );
    vkDescriptorBufferMetadata.range = (sizeof( VkSha256Metadata ) * m_count);

    // Wrap up and return
    VkBufferDescriptors vkBufferDescriptors = {
        vkDescriptorBufferInputs,
        vkDescriptorBufferMetadata
    };
    return vkBufferDescriptors;
}

Batch Batch::New(ComputeDevice& device, VkDeviceSize vkSize) {

    // Start with an empty batch
    Batch batch;

    // Get the (approx) memory requirements and look fro some corresponding memory types
    const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( vkSize );
    const auto deviceMemoryBudgets = device.AvailableMemoryTypes(
        vkMemoryRequirements,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    );

    // Iterate, trying to allocate
    for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); it != end; it++){
        const auto& deviceMemoryBudget = *it;
        if (deviceMemoryBudget.vkMemoryBudget < vkSize){
            // We're not interested, yet..?
            continue;
        }

        // Try and allocate
        auto vkDeviceMemory = device.Allocate( deviceMemoryBudget, vkSize );
        if (vkDeviceMemory == VK_NULL_HANDLE){
            continue;
        }

        // Wrap it up
        batch = Batch( device, vkDeviceMemory, vkSize );
        break;
    }
    if (!batch){
        // Iterate again, but this time accept a potentially smaller size
        for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); it != end; it++){
            const auto& deviceMemoryBudget = *it;

            // Try and allocate
            const auto allocationSize = ::std::min( deviceMemoryBudget.vkMemoryBudget, vkSize );
            auto vkDeviceMemory = device.Allocate( deviceMemoryBudget, allocationSize );
            if (vkDeviceMemory == VK_NULL_HANDLE){
                continue;
            }

            // Wrap it up
            batch = Batch( device, vkDeviceMemory, allocationSize );
            break;
        }
    }
    return batch;
}

Batch::Batch(const ComputeDevice& device, VkDeviceMemory vkDeviceMemory, VkDeviceSize vkSize):
    m_vkDevice( *device ),
    m_vkBuffer( VK_NULL_HANDLE ),
    m_vkDeviceMemory( vkDeviceMemory ),
    m_vkSize( vkSize ),
    m_minStorageBufferOffsetAlignment( device.MinStorageBufferOffset( ) ),
    m_pData( nullptr ),
    m_count( 0U ),
    m_metadata_offset( 0U ) {

    // Create a buffer
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkBufferCreateInfo.size = vkSize;
    VkResult vkResult = ::vkCreateBuffer(
        m_vkDevice,
        &vkBufferCreateInfo,
        VK_NULL_HANDLE,
        &m_vkBuffer
    );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }

    // Bind the buffer to the device memory
    vkResult = ::vkBindBufferMemory( m_vkDevice, m_vkBuffer, m_vkDeviceMemory, 0U );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }
    
    // Map in the memory
    vkResult = ::vkMapMemory( m_vkDevice, vkDeviceMemory, 0U, VK_WHOLE_SIZE, 0, &m_pData );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }
    Reuse( );
}

void Batch::Reset(void) {

    m_vkDevice = VK_NULL_HANDLE;
    m_vkDeviceMemory = VK_NULL_HANDLE;
    m_vkSize = m_minStorageBufferOffsetAlignment = 0U;
    m_vkBuffer = VK_NULL_HANDLE;
    m_pData = nullptr;
    m_count = 0U;
    m_metadata_offset = 0U;
}

void Batch::Release(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkBuffer != VK_NULL_HANDLE){
        ::vkDestroyBuffer( m_vkDevice, m_vkBuffer, pAllocator );
    }
    if (m_pData){
        ::vkUnmapMemory( m_vkDevice, m_vkDeviceMemory );
    }
    if (m_vkDeviceMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, m_vkDeviceMemory, pAllocator );
    }
    Reset( );
}

size_t Batch::AlignOffset(size_t offset, size_t size) {

    const auto lcm = [](size_t s1, size_t s2) -> size_t {
        // Figure out the smaller of the two inputs
        auto lhs = s1, rhs = s2;
        if (s1 > s2){
            lhs = s2;
            rhs = s1;
        }

        // Loop until we find some multiple of the larger value that is
        // disivible by the smaller value with no remainder
        decltype(rhs) lcm = 0;
        for (decltype(lhs) multiplier = 1; true; ++multiplier) {
            const auto tmp = (rhs * multiplier);
            if ((tmp % lhs) == 0){
                lcm = tmp;
                break;
            }
        }
        return lcm;
    };

    // The target alignment is the lowest common multiple of the minimum offset
    // (provided to us by our Creator) and the required alignment of the given
    // type
    const auto alignment = lcm( m_minStorageBufferOffsetAlignment, size );
    const auto delta = (offset % alignment);
    if (delta == 0){
        return offset;
    }
    return offset + (alignment - delta);
}

uint32_t Batch::WordCount(size_t len) {
    const size_t words = len / sizeof( uint32_t );
    return ((len % sizeof( uint32_t )) == 0)
        ? words
        : (words + 1);
}

} // namespace vkmr

#endif // VULKAN_SUPPORT
