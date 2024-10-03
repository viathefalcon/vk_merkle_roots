// Batches.cpp: defines the types, functions and classes that slices memory for inputs
//

// Includes
//

// C++ Standard Library Headers
#include <cstring>
#include <algorithm>
#include <unordered_map>

// Local Project Headers
#include "Debug.h"

// Declarations
#include "Batches.h"

static const VkAllocationCallbacks *c_pAllocator = VK_NULL_HANDLE;

namespace vkmr {

// Classes
//

Batch::Batch(Batch&& batch):
    m_data( ::std::move( batch.m_data ) ),
    m_metadata( ::std::move( batch.m_metadata ) ),
    m_count( batch.m_count ),
    m_number( batch.m_number ) {

    batch.Reset( );
}

Batch& Batch::operator=(Batch&& batch) {

    if (this != &batch){
        Release( );

        m_data = ::std::move( batch.m_data );
        m_metadata = ::std::move( batch.m_metadata );
        m_count = batch.m_count;
        m_number = batch.m_number;

        batch.Reset( );
    }
    return (*this);
}

bool Batch::Push(const ::std::vector<::std::string>& strings) {

    // Look for an early out
    if (!(*this)){
        return false;
    }
    if (strings.empty( )){
        return true;
    }

    // Is there space for the metadata
    const auto new_metadata_size = (sizeof( VkSha256Metadata ) * (m_count + strings.size( )));
    if (new_metadata_size > m_metadata.vkSize){
        // Nope
        return false;
    }

    // Is there space for the strings themselves
    auto back = this->Back( );
    uint32_t wc = 0;
    ::std::for_each( strings.cbegin( ), strings.cend( ), [&wc](const ::std::string& str) {
        wc += WordCount( str.size( ) );
    } );
    if (wc == 0){
        // Just pretend..
        return true;
    }
    const auto new_data_size = sizeof( uint ) * (back.start + WordCount( back.size ) + wc);
    if (new_data_size > m_data.vkSize){
        // Nope
        return false;
    }

    // If we get here, then we're good to go
    ::std::for_each( strings.cbegin( ), strings.cend( ), [&](const ::std::string& str) {
        const auto len = str.size( );

        // Append the metadata
        back.start = (back.start + WordCount( back.size ));
        back.size = static_cast<uint>( len );
        memcpy(
            reinterpret_cast<uint8_t*>( m_metadata.pData ) + (sizeof( VkSha256Metadata ) * m_count),
            &back,
            sizeof( VkSha256Metadata )
        );

        // Append the string
        memcpy(
            reinterpret_cast<uint8_t*>( m_data.pData ) + (back.start * sizeof( uint )),
            str.c_str( ),
            sizeof( char ) * len
        );

        // Update our state and give the caller the happy news
        m_count += 1;
    } );
    return true;
}

void Batch::Pop(size_t count) {
    m_count -= ::std::min( m_count, count );
}

Batch::VkBufferDescriptors Batch::BufferDescriptors(void) const {

    // Get the last metadata element, if any
    const auto back = this->Back( );

    // Generate
    VkDescriptorBufferInfo vkDescriptorBufferInputs = {};
    vkDescriptorBufferInputs.buffer = m_data.vkBuffer;
    vkDescriptorBufferInputs.offset = 0U;
    vkDescriptorBufferInputs.range = ((back.start + WordCount( back.size )) * sizeof( uint32_t ));
    VkDescriptorBufferInfo vkDescriptorBufferMetadata = {};
    vkDescriptorBufferMetadata.buffer = m_metadata.vkBuffer;
    vkDescriptorBufferMetadata.offset = 0U;
    vkDescriptorBufferMetadata.range = (sizeof( VkSha256Metadata ) * m_count);

    // Wrap up and return
    VkBufferDescriptors vkBufferDescriptors = {
        vkDescriptorBufferInputs,
        vkDescriptorBufferMetadata
    };
    return vkBufferDescriptors;
}

Batch::Batch(Batch::number_type number, Buffer&& data, Buffer&& metadata):
    m_data( ::std::move( data ) ),
    m_metadata( ::std::move( metadata ) ),
    m_count( 0U ),
    m_number( number ) {
}

void Batch::Reset(void) {
    m_count = 0U;
    m_number = 0xFFFFFFFF;
}

void Batch::Release(void) {
    m_data = Buffer( );
    m_metadata = Buffer( );
    Reset( );
}

VkSha256Metadata Batch::Back(void) const {

    VkSha256Metadata back = { 0 };
    if (m_count > 0){
        // Copy onto the stack
        ::std::memcpy(
            &back,
            static_cast<unsigned char*>( m_metadata.pData ) + ((m_count - 1) * sizeof( VkSha256Metadata )),
            sizeof( VkSha256Metadata )
        );
    }
    return back;
}

uint32_t Batch::WordCount(size_t len) {
    const size_t words = len / sizeof( uint32_t );
    return ((len % sizeof( uint32_t )) == 0)
        ? words
        : (words + 1);
}

Batch::Buffer::Buffer(Buffer&& buffer) noexcept:
    vkDevice( buffer.vkDevice ),
    vkBuffer( buffer.vkBuffer ),
    vkDeviceMemory( buffer.vkDeviceMemory ),
    pData( buffer.pData ),
    vkSize( buffer.vkSize) {

    buffer.Reset( );        
}

Batch::Buffer::Buffer(VkDevice p_vkDevice, VkDeviceMemory p_vkDeviceMemory, VkDeviceSize p_vkSize) noexcept:
    vkDevice( p_vkDevice ),
    vkDeviceMemory( p_vkDeviceMemory ),
    vkSize( p_vkSize ) {

    // Create a buffer
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkBufferCreateInfo.size = vkSize;
    VkResult vkResult = ::vkCreateBuffer(
        vkDevice,
        &vkBufferCreateInfo,
        VK_NULL_HANDLE,
        &vkBuffer
    );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }

    // Bind the buffer to the device memory
    vkResult = ::vkBindBufferMemory( vkDevice, vkBuffer, vkDeviceMemory, 0U );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }
    
    // Map in the memory
    vkResult = ::vkMapMemory( vkDevice, vkDeviceMemory, 0U, VK_WHOLE_SIZE, 0, &pData );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }

    // Apparently important on Steam Deck..?
    ::std::memset( pData, 0, vkSize ); 
}

Batch::Buffer& Batch::Buffer::operator=(Buffer&& buffer) noexcept {

    if (this != &buffer){
        Release( );

        vkDevice = buffer.vkDevice;
        vkBuffer = buffer.vkBuffer;
        vkDeviceMemory = buffer.vkDeviceMemory;
        vkSize = buffer.vkSize;
        pData = buffer.pData;

        buffer.Reset( );
    }
    return (*this);
}

void Batch::Buffer::Reset(void) {

    vkDevice = VK_NULL_HANDLE;
    vkBuffer = VK_NULL_HANDLE;
    vkDeviceMemory = VK_NULL_HANDLE;
    vkSize = 0U;
    pData = nullptr;
}

void Batch::Buffer::Release(void) {

    if (vkBuffer != VK_NULL_HANDLE){
        ::vkDestroyBuffer( vkDevice, vkBuffer, c_pAllocator );
    }
    if (pData){
        ::vkUnmapMemory( vkDevice, vkDeviceMemory );
    }
    if (vkDeviceMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( vkDevice, vkDeviceMemory, c_pAllocator );
    }
    Reset( );
}

Batches::Batches(Batches&& batches) noexcept:
    m_vkDataSize( batches.m_vkDataSize ),
    m_vkMetadataSize( batches.m_vkMetadataSize ),
    m_count( batches.m_count ) {    
}

Batches& Batches::operator=(Batches&& batches) noexcept {

    if (this != &batches){
        m_vkDataSize = batches.m_vkDataSize;
        m_vkMetadataSize = batches.m_vkMetadataSize;
        m_count = batches.m_count;
    }
    return (*this);
}

uint32_t Batches::MaxBatchCount(const ComputeDevice& device) const {

    // Look for an early out
    if ((m_vkDataSize + m_vkMetadataSize) == 0U){
        return 0U;
    }
    const auto dataRequirements = device.StorageBufferRequirements( m_vkDataSize );
    const auto metadataRequirements = device.StorageBufferRequirements( m_vkMetadataSize );

    // Get the memory budgets for compatible memory types
    const auto deviceMemoryBudgets = device.AvailableMemoryTypes(
        dataRequirements,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    );

    // Reduce down to the max per heap
    ::std::unordered_map<uint32_t, VkDeviceSize> heaped;
    for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); it != end; it++){
        const auto& deviceMemoryBudget = *it;

        // Look for the heap
        const auto found = heaped.find( deviceMemoryBudget.heapIndex );
        if (found == heaped.end( )){
            heaped.insert( { deviceMemoryBudget.heapIndex, deviceMemoryBudget.vkMemoryBudget } );
            continue;
        }
        found->second = ::std::max( found->second, deviceMemoryBudget.vkMemoryBudget );
    }

    // Now iterate the heaps and sum up
    uint32_t result = 0U;
    const auto vkSize = dataRequirements.size + metadataRequirements.size;
    for (auto it = heaped.cbegin( ), end = heaped.cend( ); it != end; ++it){
        const auto count = static_cast<uint32_t>( it->second / vkSize );
        result += count;
    }
    return result;
}

Batch Batches::New(ComputeDevice& device) {

    // Look for an early out
    if (!(*this)){
        return Batch( );
    }
    
    const auto allocateBuffer = [&](VkDeviceSize vkSize) -> Batch::Buffer {
        // Start with an empty buffer
        Batch::Buffer buffer;

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
            buffer = Batch::Buffer( *device, vkDeviceMemory, vkSize );
            break;
        }
        if (!buffer){
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
                buffer = Batch::Buffer( *device, vkDeviceMemory, allocationSize );
                break;
            }
        }
        return buffer;
    };
    return Batch(
        ++m_count, 
        allocateBuffer( m_vkDataSize ),
        allocateBuffer( m_vkMetadataSize )
    );
}

} // namespace vkmr
