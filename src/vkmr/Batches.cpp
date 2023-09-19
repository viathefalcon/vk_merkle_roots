// Batches.cpp: defines the types, functions and classes that slices memory for inputs and outputs
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>

// Local Project Headers
#include "Debug.h"
#include "../common/SHA-256defs.h"

// Declarations
#include "Batches.h"

#if defined (VULKAN_SUPPORT)
// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkGetPhysicalDeviceMemoryProperties2KHR g_pVkGetPhysicalDeviceMemoryProperties2KHR;

namespace vkmr {

// Classes
//

class OneTierBatch
 : public Batch {
public:
    OneTierBatch(const ComputeDevice&, VkDeviceMemory, VkDeviceSize);
    ~OneTierBatch(void);

    virtual VkDeviceSize Size(void) const { return m_vkSize; };

    virtual bool Add(const char*, size_t);

    virtual void Reset(void) { m_metadata.clear( ); }

    VkResult Dispatch(VkQueue);

private:
    // Evaluates to the number of 32-bit words
    // needed to hold a string of the given (byte)
    // length
    static uint32_t WordCount(size_t len) {
        const size_t words = len / sizeof( uint32_t );
        return ((len % sizeof( uint32_t )) == 0)
            ? words
            : (words + 1);
    }

    typedef struct {

        VkDescriptorBufferInfo vkDescriptorBufferInputs;
        VkDescriptorBufferInfo vkDescriptorBufferMetdata;
        VkDescriptorBufferInfo vkDescriptorBufferResults;

    } VkBufferDescriptors;
    VkBufferDescriptors GenerateBufferDescriptors(size_t len = 0) const {

        const auto minStorageBufferOffsetAlignment = m_minStorageBufferOffsetAlignment;
        const auto alignStorageBufferOffset = [minStorageBufferOffsetAlignment](VkDeviceSize vkSize) {
            const auto delta = (vkSize % minStorageBufferOffsetAlignment);
            if (delta == 0){
                return vkSize;
            }
            return vkSize + (minStorageBufferOffsetAlignment - delta);
        };

        VkSha256Metadata back = { 0 };
        if (!m_metadata.empty( )){
            back = m_metadata.back( );
        }
        const auto count = (len == 0) ? m_metadata.size( ) : (m_metadata.size( ) + 1);

        // Generate
        VkDescriptorBufferInfo vkDescriptorBufferInputs = {};
        vkDescriptorBufferInputs.buffer = m_vkBuffer;
        vkDescriptorBufferInputs.offset = 0;
        vkDescriptorBufferInputs.range = ((back.start + OneTierBatch::WordCount( back.size ) + OneTierBatch::WordCount( len )) * sizeof( uint32_t ));
        VkDescriptorBufferInfo vkDescriptorBufferMetadata = {};
        vkDescriptorBufferMetadata.buffer = m_vkBuffer;
        vkDescriptorBufferMetadata.offset = alignStorageBufferOffset( vkDescriptorBufferInputs.range );
        vkDescriptorBufferMetadata.range = (sizeof( VkSha256Metadata ) * count);
        VkDescriptorBufferInfo vkDescriptorBufferResults = {};
        vkDescriptorBufferResults.buffer = m_vkBuffer;
        vkDescriptorBufferResults.offset = alignStorageBufferOffset( vkDescriptorBufferInputs.range + vkDescriptorBufferMetadata.range );
        vkDescriptorBufferResults.range = VK_WHOLE_SIZE;

        // Wrap up and return
        VkBufferDescriptors vkBufferInfos = { vkDescriptorBufferInputs, vkDescriptorBufferMetadata, vkDescriptorBufferResults };
        return vkBufferInfos;
    }

    const ComputeDevice& m_device;
    VkDeviceMemory m_vkDeviceMemory;
    VkDeviceSize m_vkSize;
    VkDeviceSize m_minStorageBufferOffsetAlignment;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;

    void* m_pData;
    VkFence m_vkFence;
    VkBuffer m_vkBuffer;
    std::vector<VkSha256Metadata> m_metadata;
};

OneTierBatch::OneTierBatch(const ComputeDevice& device, VkDeviceMemory vkDeviceMemory, VkDeviceSize vkSize):
    m_device( device ),
    m_vkSize( vkSize ),
    m_minStorageBufferOffsetAlignment( device.MinStorageBufferOffset( ) ),
    m_vkDeviceMemory( vkDeviceMemory ),
    m_descriptorSet( device.AllocateDescriptorSet( 1U ) ),
    m_commandBuffer( device.AllocateCommandBuffer( ) ),
    m_pData( nullptr ),
    m_vkFence( VK_NULL_HANDLE ),
    m_vkBuffer( VK_NULL_HANDLE ) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Map in the memory
    m_vkResult = ::vkMapMemory( *device, vkDeviceMemory, 0U, VK_WHOLE_SIZE, 0, &m_pData );
    if (m_vkResult != VK_SUCCESS){
        m_pData = nullptr;
        return;
    }

    // Create a buffer
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkBufferCreateInfo.size = vkSize;
    m_vkResult = ::vkCreateBuffer(
        *m_device,
        &vkBufferCreateInfo,
        pAllocator,
        &m_vkBuffer
    );
    if (m_vkResult != VK_SUCCESS){
        m_vkBuffer = VK_NULL_HANDLE;
        return;
    }

    // Create a fence
    VkFenceCreateInfo vkFenceCreateInfo = {};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    m_vkResult = ::vkCreateFence( *device, &vkFenceCreateInfo, pAllocator, &m_vkFence );
    if (m_vkResult != VK_SUCCESS){
        // Stop here
        m_vkFence = VK_NULL_HANDLE;
        return;
    }

    // Bind the buffer to the device memory
    m_vkResult = ::vkBindBufferMemory( *m_device, m_vkBuffer, m_vkDeviceMemory, 0U );
}

OneTierBatch::~OneTierBatch(void) {

    // Cleanup
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( *m_device, m_vkFence, pAllocator );
    }
    if (m_vkBuffer != VK_NULL_HANDLE){
        ::vkDestroyBuffer( *m_device, m_vkBuffer, pAllocator );
    }
    if (m_pData){
        ::vkUnmapMemory( *m_device, m_vkDeviceMemory );
    }
    if (m_vkDeviceMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( *m_device, m_vkDeviceMemory, pAllocator );
        m_vkDeviceMemory = VK_NULL_HANDLE;
    }
}

bool OneTierBatch::Add(const char *str, size_t len) {

    // Look for an early out
    if (m_vkResult != VK_SUCCESS){
        return false;
    }
    if (!str || (len == 0)){
        return true; // Silently skip
    }

    // Calculate whether we can accomodate if correctly aligned
    const auto bufferDescriptors = this->GenerateBufferDescriptors( len );
    if (bufferDescriptors.vkDescriptorBufferResults.offset >= m_vkSize){
        // Nope
        return false;
    }

    // Figure out the offset to which to copy the string
    VkSha256Metadata back = { 0 };
    if (!m_metadata.empty( )){
        back = m_metadata.back( );
    }
    back.start = (back.start + OneTierBatch::WordCount( back.size ));
    back.size = static_cast<uint>( len );
    m_metadata.push_back( back );

    // Copy the string there and append the metadata
    ::std::memcpy( static_cast<char*>( m_pData ) + (back.start * sizeof( uint32_t )), str, len );
    return true;
}

VkResult OneTierBatch::Dispatch(VkQueue vkQueue) {

    // Look for an early out
    if (m_metadata.empty( )){
        // Nothing to do
        return VK_SUCCESS;
    }

    // Figure out where everything should go and then copy the metadata into place
    auto bufferDescriptors = this->GenerateBufferDescriptors( );
    ::std::memcpy(
        static_cast<uint8_t*>( m_pData ) + (bufferDescriptors.vkDescriptorBufferMetdata.offset),
        m_metadata.data( ),
        bufferDescriptors.vkDescriptorBufferMetdata.range
    );

    // Update the descriptor set
    VkWriteDescriptorSet vkWriteDescriptorSetInputs = {};
    vkWriteDescriptorSetInputs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetInputs.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetInputs.descriptorCount = 1;
    vkWriteDescriptorSetInputs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetInputs.pBufferInfo = &(bufferDescriptors.vkDescriptorBufferInputs);
    VkWriteDescriptorSet vkWriteDescriptorSetMetadata = {};
    vkWriteDescriptorSetMetadata.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetMetadata.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetMetadata.dstBinding = 1;
    vkWriteDescriptorSetMetadata.descriptorCount = 1;
    vkWriteDescriptorSetMetadata.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetMetadata.pBufferInfo = &(bufferDescriptors.vkDescriptorBufferMetdata);
    VkWriteDescriptorSet vkWriteDescriptorSetResults = {};
    vkWriteDescriptorSetResults.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetResults.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetResults.dstBinding = 2;
    vkWriteDescriptorSetResults.descriptorCount = 1;
    vkWriteDescriptorSetResults.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetResults.pBufferInfo = &(bufferDescriptors.vkDescriptorBufferResults);
    VkWriteDescriptorSet vkWriteDescriptorSets[] = {
        vkWriteDescriptorSetInputs,
        vkWriteDescriptorSetMetadata,
        vkWriteDescriptorSetResults
    };
    ::vkUpdateDescriptorSets( *m_device, 3, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );

    // Update the command buffer
    VkResult vkResult = m_commandBuffer.BindDispatch( m_descriptorSet, m_metadata.size( ) );
    if (vkResult == VK_SUCCESS){
        VkCommandBuffer commandBuffers[] = { *m_commandBuffer };

        // Submit unto the queue
        VkSubmitInfo vkSubmitInfo = {};
        vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        vkSubmitInfo.commandBufferCount = 1;
        vkSubmitInfo.pCommandBuffers = commandBuffers;
        vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence );
    }
    if (vkResult == VK_SUCCESS){
        // Wait
        vkResult = ::vkWaitForFences( *m_device, 1, &m_vkFence, true, uint64_t(-1) );
    }
    if (vkResult == VK_SUCCESS){
        // Iterate the results (🤞)
        auto pResult = static_cast<uint8_t*>( m_pData ) + bufferDescriptors.vkDescriptorBufferResults.offset;
        for (auto& metadata : m_metadata){
            // Wrap the input as a string
            ::std::string str( static_cast<char*>( m_pData ) + (metadata.start * sizeof( uint32_t )), metadata.size );

            // Get the output
            VkSha256Result result = {};
            ::std::memcpy( &result, pResult, sizeof( VkSha256Result ) );
            pResult += sizeof( VkSha256Result );

            ::std::cout << "Vulkan:- " << str << " >> " << print_bytes_ex( result.data, SHA256_WC ).str( ) << std::endl;
        }
    }
    return vkResult;
}

// Functions
//

typedef struct {
    uint32_t memoryTypeIndex;
    VkDeviceSize vkMemoryBudget;
    VkMemoryPropertyFlags vkMemoryPropertyFlags;
} MemoryTypeBudget;
typedef ::std::vector<MemoryTypeBudget> MemoryTypeBudgets;
static MemoryTypeBudgets FindMemoryTypes(
    VkPhysicalDevice vkPhysicalDevice,
    const VkMemoryRequirements& vkMemoryRequirements,
    VkMemoryPropertyFlags vkMemoryPropertyFlags) {

    // Query
    VkPhysicalDeviceMemoryProperties2 vkPhysicalDeviceMemoryProperties2 = {};
    VkPhysicalDeviceMemoryBudgetPropertiesEXT vkPhysicalDeviceMemoryBudgetProperties = {};
    if (g_pVkGetPhysicalDeviceMemoryProperties2KHR){
        vkPhysicalDeviceMemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

        vkPhysicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkPhysicalDeviceMemoryProperties2.pNext = &vkPhysicalDeviceMemoryBudgetProperties;
        g_pVkGetPhysicalDeviceMemoryProperties2KHR( vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties2 );
    }else{
        // Fall back
        vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &(vkPhysicalDeviceMemoryProperties2.memoryProperties) );
    }

    // Scan
    MemoryTypeBudgets memoryTypeBudgets;
    const auto pVkPhysicalDeviceMemoryProperties = &(vkPhysicalDeviceMemoryProperties2.memoryProperties);
    for (decltype(pVkPhysicalDeviceMemoryProperties->memoryTypeCount) index = 0; index < pVkPhysicalDeviceMemoryProperties->memoryTypeCount; ++index){
        VkMemoryType* vkMemoryType = (pVkPhysicalDeviceMemoryProperties->memoryTypes + index);
        if ((vkMemoryType->propertyFlags & vkMemoryPropertyFlags) != vkMemoryPropertyFlags){
            continue;
        }

        decltype(vkMemoryRequirements.memoryTypeBits) memoryTypeMask = (1 << index);
        if ((vkMemoryRequirements.memoryTypeBits & memoryTypeMask) != memoryTypeMask){
            continue;
        }

        VkMemoryHeap* pvkMemoryHeap = (pVkPhysicalDeviceMemoryProperties->memoryHeaps + vkMemoryType->heapIndex);
        auto budget = vkPhysicalDeviceMemoryBudgetProperties.heapBudget[vkMemoryType->heapIndex];
        if (budget == 0){
            // Fallback
            budget = pvkMemoryHeap->size;
        }

        // Map the index to the budget and accumulate
        MemoryTypeBudget memoryTypeBudget = { index, budget, vkMemoryType->propertyFlags };
        memoryTypeBudgets.push_back( memoryTypeBudget );
    }

    // Sort by budget descending and return
    ::std::sort( memoryTypeBudgets.begin( ), memoryTypeBudgets.end( ), [](const MemoryTypeBudget& lhs, const MemoryTypeBudget& rhs) {
        return lhs.vkMemoryBudget > rhs.vkMemoryBudget;
    } );
    return memoryTypeBudgets;
}

::std::unique_ptr<Batch> NewBatch(
    VkDeviceSize vkSize,
    const ComputeDevice& device) {

    // Start with an empty slice
    ::std::unique_ptr<Batch> slice;

    // Get the (approx) memory requirements
    const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( vkSize );
    const VkMemoryPropertyFlags vkHostMemoryFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // First, look for device memory which is host mappable
    // (this should be available on our preferred platform: i.e. integrated GPUs)
    auto deviceMemoryBudgets = FindMemoryTypes(
        device.PhysicalDevice( ),
        vkMemoryRequirements,
        (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | vkHostMemoryFlags)
    );
    if (deviceMemoryBudgets.empty( )){
        // No joy, so just look for device memory on its own
        deviceMemoryBudgets = FindMemoryTypes(
            device.PhysicalDevice( ),
            vkMemoryRequirements,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }
    if (deviceMemoryBudgets.empty( )){
        // Done-zo
        return slice;
    }

    // Take the front of the list and ask for as much as we can from it
    const auto& deviceMemoryBudget = deviceMemoryBudgets.front( );
    VkMemoryAllocateInfo vkDeviceMemoryAllocateInfo = {};
    vkDeviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkDeviceMemoryAllocateInfo.allocationSize = ::std::min( deviceMemoryBudget.vkMemoryBudget, vkSize );
    vkDeviceMemoryAllocateInfo.memoryTypeIndex = deviceMemoryBudget.memoryTypeIndex;
    VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
    VkResult vkResult = ::vkAllocateMemory( *device, &vkDeviceMemoryAllocateInfo, VK_NULL_HANDLE, &vkDeviceMemory );
    if (vkResult == VK_SUCCESS){
        // Now, if the allocate memory isn't host-mappable, then we need to try and allocate the same again
        // from system memory
        if ((deviceMemoryBudget.vkMemoryPropertyFlags & vkHostMemoryFlags) == vkHostMemoryFlags){
            // Wrap it up (takes clear of cleanup)
            auto onetier = ::std::unique_ptr<Batch>(
                new OneTierBatch( device, vkDeviceMemory, vkDeviceMemoryAllocateInfo.allocationSize )
            );
            if (static_cast<VkResult>( *onetier ) == VK_SUCCESS){
                slice = ::std::move( onetier );
            }
        }else{
            // TODO
            ;
        }
    }
    return slice;
}

} // namespace vkmr

#endif // VULKAN_SUPPORT
