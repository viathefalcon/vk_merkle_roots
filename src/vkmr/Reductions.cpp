// Reductions.cpp: defines the types, functions and classes which encapsulate asynchronous on-device reduction operations 
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <cstring>
#include <iostream>
#include <utility>

// Declarations
#include "Ops.h"

// Local Project Headers
#include "Debug.h"
#include "Utils.h"

// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;

namespace vkmr {

// Types
//

struct alignas(uint) BySubgroupPushConstants {
    uint offset;
    uint pairs;
    uint delta;
    uint bound;
};

// Classes
//
class Reduction {
public:
    virtual ~Reduction() = default;

    operator VkResult() const { return m_vkResult; }
    operator VkSha256Result() const { return m_vkSha256Result; }

    virtual Reduction& Apply(Slice<VkSha256Result>&, ComputeDevice&, const vkmr::Pipeline&) = 0;
    virtual VkResult Dispatch(VkQueue) = 0;

protected:
    Reduction(): m_vkResult( VK_RESULT_MAX_ENUM ) { }
    Reduction(VkResult vkResult): m_vkResult( vkResult ) { }

    VkResult m_vkResult;
    VkSha256Result m_vkSha256Result;
};

class BasicReduction : public Reduction {
public:
    BasicReduction(BasicReduction&&);
    BasicReduction(VkDevice, DescriptorSet&&, CommandBuffer&&);
    BasicReduction(BasicReduction const&) = delete;

    BasicReduction(void) { Reset( ); }
    virtual ~BasicReduction(void) { Release( ); }

    BasicReduction& operator=(BasicReduction const&) = delete;
    BasicReduction& operator=(BasicReduction&&);

    virtual Reduction& Apply(Slice<VkSha256Result>&, ComputeDevice&, const vkmr::Pipeline&);
    virtual VkResult Dispatch(VkQueue);

private:
    void Free(void);
    void Reset(void);
    void Release(void);

    VkDevice m_vkDevice;
    VkDeviceSize m_vkSize;
    uint m_count;

    VkFence m_vkFence;
    VkBuffer m_vkBufferHost;
    VkDeviceMemory m_vkHostMemory;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

BasicReduction::BasicReduction(BasicReduction&& reduction):
    Reduction( reduction.m_vkResult ),
    m_vkDevice( reduction.m_vkDevice ),
    m_vkFence( reduction.m_vkFence ),
    m_vkSize( reduction.m_vkSize ),
    m_count( reduction.m_count ),
    m_vkBufferHost( reduction.m_vkBufferHost ),
    m_vkHostMemory( reduction.m_vkHostMemory ),
    m_descriptorSet( ::std::move( reduction.m_descriptorSet ) ),
    m_commandBuffer( ::std::move( reduction.m_commandBuffer ) ) {
    reduction.Reset( );
}

BasicReduction::BasicReduction(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer):
    Reduction( ),
    m_vkDevice( vkDevice ),
    m_vkSize( 0U ),
    m_count( 0U ),
    m_vkFence( VK_NULL_HANDLE ),
    m_vkBufferHost( VK_NULL_HANDLE ),
    m_vkHostMemory( VK_NULL_HANDLE ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ) {

    // Create the fence
    VkFenceCreateInfo vkFenceCreateInfo = {};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    m_vkResult = ::vkCreateFence( m_vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &m_vkFence );
}

BasicReduction& BasicReduction::operator=(BasicReduction&& reduction) {

    if (this != &reduction){
        this->Release( );

        m_vkResult = reduction.m_vkResult;
        m_vkDevice = reduction.m_vkDevice;
        m_vkSize = reduction.m_vkSize;
        m_count = reduction.m_count;
        m_vkFence = reduction.m_vkFence;
        m_vkBufferHost = reduction.m_vkBufferHost;
        m_vkHostMemory = reduction.m_vkHostMemory;
        m_descriptorSet = ::std::move( reduction.m_descriptorSet );
        m_commandBuffer = ::std::move( reduction.m_commandBuffer );

        reduction.Reset( );
    }
    return (*this);
}

Reduction& BasicReduction::Apply(Slice<VkSha256Result>& slice, ComputeDevice& device, const vkmr::Pipeline& pipeline) {

    // Release any previously-held memory
    this->Free( );

    // Get the (approx) memory requirements
    m_count = slice.Count( );
    const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( sizeof( VkSha256Result ) * m_count );
    m_vkSize = vkMemoryRequirements.size;

    // Look for some corresponding memory types, and try to allocate
    const auto memoryBudgets = device.AvailableMemoryTypes(
        vkMemoryRequirements,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    );
    for (auto it = memoryBudgets.cbegin( ), end = memoryBudgets.cend( ); it != end; it++){
        const auto& deviceMemoryBudget = *it;
        if (deviceMemoryBudget.vkMemoryBudget < m_vkSize){
            // We're not interested, yet..?
            continue;
        }

        // Try and allocate
        m_vkHostMemory = device.Allocate( deviceMemoryBudget, m_vkSize );
        if (m_vkHostMemory == VK_NULL_HANDLE){
            continue;
        }
        break;
    }
    m_vkResult = (m_vkHostMemory == VK_NULL_HANDLE) ? VK_ERROR_UNKNOWN : VK_SUCCESS;
    if (m_vkResult == VK_SUCCESS){
        // Create a buffer
        VkBufferCreateInfo vkBufferCreateInfo = {};
        vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkBufferCreateInfo.size = m_vkSize;
        m_vkResult = ::vkCreateBuffer(
            m_vkDevice,
            &vkBufferCreateInfo,
            VK_NULL_HANDLE,
            &m_vkBufferHost
        );
    }
    if (m_vkResult == VK_SUCCESS){
        // Bind the buffer to the memory
        m_vkResult = ::vkBindBufferMemory( m_vkDevice, m_vkBufferHost, m_vkHostMemory, 0U );
    }
    if (m_vkResult == VK_SUCCESS){
        // Update the descriptor set (to point at the slice's buffer)
        const auto sliceBufferDescriptor = slice.BufferDescriptor( );
        VkWriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkWriteDescriptorSet.dstSet = *m_descriptorSet;
        vkWriteDescriptorSet.descriptorCount = 1;
        vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vkWriteDescriptorSet.pBufferInfo = &(sliceBufferDescriptor);
        VkWriteDescriptorSet vkWriteDescriptorSets[] = {
            vkWriteDescriptorSet
        };
        ::vkUpdateDescriptorSets( m_vkDevice, 1, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );

        // Get the command buffer
        auto vkCommandBuffer = *m_commandBuffer;

        // Start recording commands into it
        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        m_vkResult = ::vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
        if (m_vkResult == VK_SUCCESS){
            ::vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline );
            VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
            ::vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Layout( ), 0, 1, descriptorSets, 0, VK_NULL_HANDLE );

            // Loop until we've reduced the number of elements to 1
            for (uint pass = 0U, count = m_count; count > 1U; ){
                const uint delta = (1 << pass);

                // Shader only operates on pairs, so the group count
                // for every pass must be even
                if ((count % 2) != 0){
                    // If this is not the first iteration, then we need
                    // a barrier between the last shader invocation and
                    // the copy we're about to do
                    if (pass > 0U){
                        VkMemoryBarrier2KHR vkMemoryBarrier = {};
                        vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                        vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                        vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
                        vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
                        vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                        VkDependencyInfoKHR vkDependencyInfo = {};
                        vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                        vkDependencyInfo.memoryBarrierCount = 1;
                        vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
                        g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
                    }

                    // Duplicate the last element
                    VkBufferCopy vkBufferCopy = {};
                    vkBufferCopy.size = sizeof( Slice<VkSha256Result>::value_type );
                    vkBufferCopy.srcOffset = vkBufferCopy.size * (count - 1U) * delta;
                    vkBufferCopy.dstOffset = vkBufferCopy.srcOffset + (vkBufferCopy.size * delta);
                    if ((vkBufferCopy.dstOffset + vkBufferCopy.size) > slice.Size( )){
                        // The copy would overflow the slice's memory - bail
                        m_vkResult = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                        break;
                    }
                    ::vkCmdCopyBuffer( vkCommandBuffer, slice.Buffer( ), slice.Buffer( ), 1, &vkBufferCopy );
                    std::cout << "Duplicating item at " << int64_t(vkBufferCopy.srcOffset) << " to " << int64_t(vkBufferCopy.dstOffset) << "; count == " << count << ", delta == " << delta << std::endl;
                    count += 1U;

                    // Now we need a barrier between the copy and the shader invocation, below
                    VkMemoryBarrier2KHR vkMemoryBarrier = {};
                    vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                    vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
                    vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
                    vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                    vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;
                    VkDependencyInfoKHR vkDependencyInfo = {};
                    vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                    vkDependencyInfo.memoryBarrierCount = 1;
                    vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
                    g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
                }else if (pass > 0U){
                    // Inject a barrier between shader invocations
                    VkMemoryBarrier2KHR vkMemoryBarrier = {};
                    vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                    vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                    vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
                    vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                    vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;
                    VkDependencyInfoKHR vkDependencyInfo = {};
                    vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                    vkDependencyInfo.memoryBarrierCount = 1;
                    vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
                    g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
                }

                // Push the constants
                struct {
                    uint pass, delta, bound;
                } pc;
                pc.pass = (++pass);
                pc.delta = delta;
                pc.bound = m_count;
                ::vkCmdPushConstants( vkCommandBuffer, pipeline.Layout( ), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

                // Actually dispatch the shader invocations
                const auto pairs = (count >> 1);
                const auto& workgroupSize = pipeline.GetWorkGroupSize( );
                const auto x = workgroupSize.GetGroupCountX( pairs );
                ::std::cout << "Dispatching " << x << " workgroup(s) of size " << workgroupSize.x << " for " << pairs << " pair(s)" << ::std::endl;
                ::vkCmdDispatch( vkCommandBuffer, x, 1, 1 );

                count = pairs;
            }
        }
        if (m_vkResult == VK_SUCCESS){
            // Insert a barrier such that the writes from the shader complete
            // before we try and copy back to host-mappable memory
            VkMemoryBarrier2KHR vkMemoryBarrier = {};
            vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
            vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
            vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            VkDependencyInfoKHR vkDependencyInfo = {};
            vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            vkDependencyInfo.memoryBarrierCount = 1;
            vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
            g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );

            // Add the command to copy from the slice buffer to the one we've allocated
            VkBufferCopy vkBufferCopy = {};
            vkBufferCopy.size = sizeof( VkSha256Result );
            ::vkCmdCopyBuffer( vkCommandBuffer, slice.Buffer( ), m_vkBufferHost, 1, &vkBufferCopy );
            m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
        }
    }
    return (*this);
}

VkResult BasicReduction::Dispatch(VkQueue vkQueue) {

    // Look for an early out
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }
    if (m_count == 0){
        return VK_SUCCESS;
    }

    // Reset the fence
    m_vkResult = ::vkResetFences( m_vkDevice, 1, &m_vkFence );
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }
    VkCommandBuffer commandBuffers[] = { *m_commandBuffer };

    // Submit unto the queue
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = commandBuffers;
    m_vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence );
    if (m_vkResult == VK_SUCCESS){
        // Wait
        m_vkResult = ::vkWaitForFences( m_vkDevice, 1, &m_vkFence, true, uint64_t(-1) );
    }

    // Map in the memory, maybe
    void* pMapped = nullptr;
    if (m_vkResult == VK_SUCCESS){
        m_vkResult = ::vkMapMemory( m_vkDevice, m_vkHostMemory, 0U, VK_WHOLE_SIZE, 0, &pMapped );
    }
    if (m_vkResult == VK_SUCCESS){
        const auto pResults = static_cast<uint8_t*>( pMapped );
        /*
        // Iterate the results
        for (decltype(m_count) counter = 0U; counter < 1U; ++counter){
            const VkDeviceSize vkOffset = sizeof( VkSha256Result ) * counter;

            // Get the next result
            VkSha256Result result = {};
            ::std::memcpy( &result, pResults + vkOffset, sizeof( VkSha256Result ) );

            // The output is big-endian in nature; convert to little endiannes prior to output
            for (auto u = 0U; u < SHA256_WC; ++u){
                const uint w = result.data[u];
                result.data[u] = SWOP_ENDS_U32( w );
            }
            ::std::cout << print_bytes_ex( result.data, SHA256_WC ).str( ) << std::endl;
        }
        */
        ::std::memcpy( &m_vkSha256Result, pResults, sizeof( VkSha256Result ) );

        // The output is big-endian in nature; convert to little endiannes prior to output
        for (auto u = 0U; u < SHA256_WC; ++u){
            const uint w = m_vkSha256Result.data[u];
            m_vkSha256Result.data[u] = SWOP_ENDS_U32( w );
        }
    }

    // Cleanup & return
    if (pMapped){
        ::vkUnmapMemory( m_vkDevice, m_vkHostMemory );
    }
    return m_vkResult;
}

void BasicReduction::Free(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkBufferHost != VK_NULL_HANDLE){
        ::vkDestroyBuffer( m_vkDevice, m_vkBufferHost, pAllocator );
    }
    if (m_vkHostMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, m_vkHostMemory, pAllocator );
    }
}

void BasicReduction::Reset(void) {
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkSize = 0U;
    m_count = 0U;
    m_vkFence = VK_NULL_HANDLE;
    m_vkBufferHost = VK_NULL_HANDLE;
    m_vkHostMemory = VK_NULL_HANDLE;
}

void BasicReduction::Release(void) {

    this->Free( );
    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, VK_NULL_HANDLE );
    }
    m_descriptorSet = DescriptorSet( );
    m_commandBuffer = CommandBuffer( );
    Reset( );
}

class ReductionBySubgroup : public Reduction {
public:
    ReductionBySubgroup(ReductionBySubgroup&&);
    ReductionBySubgroup(VkDevice, DescriptorSet&&, CommandBuffer&&);
    ReductionBySubgroup(ReductionBySubgroup const&) = delete;

    ReductionBySubgroup(void) { Reset( ); }
    virtual ~ReductionBySubgroup(void) { Release( ); }

    ReductionBySubgroup& operator=(ReductionBySubgroup const&) = delete;
    ReductionBySubgroup& operator=(ReductionBySubgroup&&);

    virtual Reduction& Apply(Slice<VkSha256Result>&, ComputeDevice&, const vkmr::Pipeline&);
    virtual VkResult Dispatch(VkQueue);

private:
    void Free(void);
    void Reset(void);
    void Release(void);

    VkDevice m_vkDevice;
    VkDeviceSize m_vkSize;
    uint m_count;

    VkFence m_vkFence;
    VkBuffer m_vkBufferHost;
    VkDeviceMemory m_vkHostMemory;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

ReductionBySubgroup::ReductionBySubgroup(ReductionBySubgroup&& reduction):
    Reduction( reduction.m_vkResult ),
    m_vkDevice( reduction.m_vkDevice ),
    m_vkFence( reduction.m_vkFence ),
    m_vkSize( reduction.m_vkSize ),
    m_count( reduction.m_count ),
    m_vkBufferHost( reduction.m_vkBufferHost ),
    m_vkHostMemory( reduction.m_vkHostMemory ),
    m_descriptorSet( ::std::move( reduction.m_descriptorSet ) ),
    m_commandBuffer( ::std::move( reduction.m_commandBuffer ) ) {
    reduction.Reset( );
}

ReductionBySubgroup::ReductionBySubgroup(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer):
    Reduction( ),
    m_vkDevice( vkDevice ),
    m_vkSize( 0U ),
    m_count( 0U ),
    m_vkFence( VK_NULL_HANDLE ),
    m_vkBufferHost( VK_NULL_HANDLE ),
    m_vkHostMemory( VK_NULL_HANDLE ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ) {

    // Create the fence
    VkFenceCreateInfo vkFenceCreateInfo = {};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    m_vkResult = ::vkCreateFence( m_vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &m_vkFence );
}

ReductionBySubgroup& ReductionBySubgroup::operator=(ReductionBySubgroup&& reduction) {

    if (this != &reduction){
        this->Release( );

        m_vkResult = reduction.m_vkResult;
        m_vkDevice = reduction.m_vkDevice;
        m_vkSize = reduction.m_vkSize;
        m_count = reduction.m_count;
        m_vkFence = reduction.m_vkFence;
        m_vkBufferHost = reduction.m_vkBufferHost;
        m_vkHostMemory = reduction.m_vkHostMemory;
        m_descriptorSet = ::std::move( reduction.m_descriptorSet );
        m_commandBuffer = ::std::move( reduction.m_commandBuffer );

        reduction.Reset( );
    }
    return (*this);
}

Reduction& ReductionBySubgroup::Apply(Slice<VkSha256Result>& slice, ComputeDevice& device, const vkmr::Pipeline& pipeline) {

    // Release any previously-held memory
    this->Free( );

    // Get the (approx) memory requirements
    m_count = slice.Count( );
    const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( sizeof( VkSha256Result ) );
    m_vkSize = vkMemoryRequirements.size;

    // Look for some corresponding memory types, and try to allocate
    const auto memoryBudgets = device.AvailableMemoryTypes(
        vkMemoryRequirements,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    );
    for (auto it = memoryBudgets.cbegin( ), end = memoryBudgets.cend( ); it != end; it++){
        const auto& deviceMemoryBudget = *it;
        if (deviceMemoryBudget.vkMemoryBudget < m_vkSize){
            // We're not interested, yet..?
            continue;
        }

        // Try and allocate
        m_vkHostMemory = device.Allocate( deviceMemoryBudget, m_vkSize );
        if (m_vkHostMemory == VK_NULL_HANDLE){
            continue;
        }
        break;
    }
    m_vkResult = (m_vkHostMemory == VK_NULL_HANDLE) ? VK_ERROR_UNKNOWN : VK_SUCCESS;
    if (m_vkResult == VK_SUCCESS){
        // Create a buffer
        VkBufferCreateInfo vkBufferCreateInfo = {};
        vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkBufferCreateInfo.size = m_vkSize;
        m_vkResult = ::vkCreateBuffer(
            m_vkDevice,
            &vkBufferCreateInfo,
            VK_NULL_HANDLE,
            &m_vkBufferHost
        );
    }
    if (m_vkResult == VK_SUCCESS){
        // Bind the buffer to the memory
        m_vkResult = ::vkBindBufferMemory( m_vkDevice, m_vkBufferHost, m_vkHostMemory, 0U );
    }
    if (m_vkResult == VK_SUCCESS){
        // Update the descriptor set (to point at the slice's buffer)
        const auto sliceBufferDescriptor = slice.BufferDescriptor( );
        VkWriteDescriptorSet vkWriteDescriptorSet = {};
        vkWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vkWriteDescriptorSet.dstSet = *m_descriptorSet;
        vkWriteDescriptorSet.descriptorCount = 1;
        vkWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vkWriteDescriptorSet.pBufferInfo = &(sliceBufferDescriptor);
        VkWriteDescriptorSet vkWriteDescriptorSets[] = {
            vkWriteDescriptorSet
        };
        ::vkUpdateDescriptorSets( m_vkDevice, 1, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );

        // Get the command buffer
        auto vkCommandBuffer = *m_commandBuffer;

        // Start recording commands into it
        VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
        vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        m_vkResult = ::vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
        if (m_vkResult == VK_SUCCESS){
            VkPhysicalDeviceProperties vkPhysicalDeviceProperties = {};
            ::vkGetPhysicalDeviceProperties( device.PhysicalDevice( ), &vkPhysicalDeviceProperties );

            ::vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline );
            VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
            ::vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Layout( ), 0, 1, descriptorSets, 0, VK_NULL_HANDLE );

            // Loop until we will have reduced the number of elements to 1
            for (uint delta = 1U, count = m_count; count > 1U; ){
                const auto pairs = (((count % 2 == 0) ? count : (count+1)) >> 1);
                const auto& workgroupSize = pipeline.GetWorkGroupSize( );
                count = workgroupSize.GetGroupCountX( pairs );

                // Split into as many dispatches as are needed
                for (auto remaining = count; remaining > 0U; ){
                    const auto x = ::std::min(
                        remaining,
                        vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0]
                    );

                    // Push the constants
                    BySubgroupPushConstants pc = { 0U };
                    pc.offset = workgroupSize.x * (count - remaining);
                    pc.pairs = pairs;
                    pc.delta = delta;
                    pc.bound = m_count;
                    ::vkCmdPushConstants( vkCommandBuffer, pipeline.Layout( ), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

                    // Actually dispatch the shader invocations
                    ::std::cout << "Dispatching " << x << " subgroup-ed workgroup(s) of size " << workgroupSize.x << " for " << pairs << " pair(s)" << ::std::endl;
                    ::vkCmdDispatch( vkCommandBuffer, x, 1, 1 );

                    // Advance
                    remaining -= x;
                }

                // Tee up the next iteration
                delta *= (workgroupSize.x << 1); // x2 because each invocation in the group has addressed two items
                if (count > 1U){
                    // Inject a barrier between passes
                    VkMemoryBarrier2KHR vkMemoryBarrier = {};
                    vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                    vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                    vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
                    vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
                    vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT_KHR;
                    VkDependencyInfoKHR vkDependencyInfo = {};
                    vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
                    vkDependencyInfo.memoryBarrierCount = 1;
                    vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
                    g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
                }
            }
        }
        if (m_vkResult == VK_SUCCESS){
            // Insert a barrier such that the writes from the shader complete
            // before we try and copy back to host-mappable memory
            VkMemoryBarrier2KHR vkMemoryBarrier = {};
            vkMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
            vkMemoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
            vkMemoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT_KHR;
            vkMemoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
            vkMemoryBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            VkDependencyInfoKHR vkDependencyInfo = {};
            vkDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            vkDependencyInfo.memoryBarrierCount = 1;
            vkDependencyInfo.pMemoryBarriers = &vkMemoryBarrier;
            g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );

            // Add the command to copy from the slice buffer to the one we've allocated
            VkBufferCopy vkBufferCopy = {};
            vkBufferCopy.size = sizeof( VkSha256Result );
            ::vkCmdCopyBuffer( vkCommandBuffer, slice.Buffer( ), m_vkBufferHost, 1, &vkBufferCopy );
            m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
        }
    }
    return (*this);
}

VkResult ReductionBySubgroup::Dispatch(VkQueue vkQueue) {

    // Look for an early out
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }
    if (m_count == 0){
        return VK_SUCCESS;
    }

    // Reset the fence
    m_vkResult = ::vkResetFences( m_vkDevice, 1, &m_vkFence );
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }
    VkCommandBuffer commandBuffers[] = { *m_commandBuffer };

    // Submit unto the queue
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = commandBuffers;
    m_vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence );
    if (m_vkResult == VK_SUCCESS){
        // Wait
        m_vkResult = ::vkWaitForFences( m_vkDevice, 1, &m_vkFence, true, uint64_t(-1) );
    }

    // Map in the memory, maybe
    void* pMapped = nullptr;
    if (m_vkResult == VK_SUCCESS){
        m_vkResult = ::vkMapMemory( m_vkDevice, m_vkHostMemory, 0U, VK_WHOLE_SIZE, 0, &pMapped );
    }
    if (m_vkResult == VK_SUCCESS){
        const auto pResults = static_cast<uint8_t*>( pMapped );
        ::std::memcpy( &m_vkSha256Result, pResults, sizeof( VkSha256Result ) );

        // The output is big-endian in nature; convert to little endianess prior to output
        for (auto u = 0U; u < SHA256_WC; ++u){
            const uint w = m_vkSha256Result.data[u];
            m_vkSha256Result.data[u] = SWOP_ENDS_U32( w );
        }
    }

    // Cleanup & return
    if (pMapped){
        ::vkUnmapMemory( m_vkDevice, m_vkHostMemory );
    }
    return m_vkResult;
}

void ReductionBySubgroup::Free(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkBufferHost != VK_NULL_HANDLE){
        ::vkDestroyBuffer( m_vkDevice, m_vkBufferHost, pAllocator );
    }
    if (m_vkHostMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, m_vkHostMemory, pAllocator );
    }
}

void ReductionBySubgroup::Reset(void) {
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkSize = 0U;
    m_count = 0U;
    m_vkFence = VK_NULL_HANDLE;
    m_vkBufferHost = VK_NULL_HANDLE;
    m_vkHostMemory = VK_NULL_HANDLE;
}

void ReductionBySubgroup::Release(void) {

    this->Free( );
    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, VK_NULL_HANDLE );
    }
    m_descriptorSet = DescriptorSet( );
    m_commandBuffer = CommandBuffer( );
    Reset( );
}

class ReductionFactory {
public:
    typedef ::std::shared_ptr<Reduction> ProductType;

    ReductionFactory(bool subgroupSupportPreferred): m_subgroupSupportPreferred( subgroupSupportPreferred ) { }
    ~ReductionFactory() = default;

    ProductType CreateReduction(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer) {

        using ::std::make_shared;

        ProductType product;
        if (m_subgroupSupportPreferred){
            product = make_shared<ReductionBySubgroup>(
                vkDevice,
                ::std::move( descriptorSet ),
                ::std::move( commandBuffer )
            );
        }else{
            product = make_shared<BasicReduction>(
                vkDevice,
                ::std::move( descriptorSet ),
                ::std::move( commandBuffer )
            );
        }
        return product;
    }

private:
    bool m_subgroupSupportPreferred;
};
class ReductionsImpl : public Reductions {
public:
    ReductionsImpl(ComputeDevice& device, vkmr::Pipeline&& pipeline, bool subgroupSupportPreferred):
        m_vkDevice( *device ),
        m_descriptorPool( device.CreateDescriptorPool( 1, 1 ) ),
        m_commandPool( device.CreateCommandPool( ) ),
        m_pipeline( ::std::move( pipeline ) ),
        m_factory( ReductionFactory( subgroupSupportPreferred ) ) { }

    virtual ~ReductionsImpl() {

        m_container.clear( );
        m_descriptorPool = DescriptorPool( );
        m_commandPool = CommandPool( );
        m_pipeline = vkmr::Pipeline( );
    }

    VkSha256Result Reduce(Slice<VkSha256Result>&, ComputeDevice&);

private:
    VkDevice m_vkDevice;

    DescriptorPool m_descriptorPool;
    CommandPool m_commandPool;
    vkmr::Pipeline m_pipeline;

    ReductionFactory m_factory;
    ::std::vector<ReductionFactory::ProductType> m_container;
};

VkSha256Result ReductionsImpl::Reduce(Slice<VkSha256Result>& slice, ComputeDevice& device) {

    using ::std::cerr;
    using ::std::endl;

    // Get a reference to a reduction instance
    if (m_container.empty( )){
        m_container.push_back(
            m_factory.CreateReduction(
                m_vkDevice,
                m_descriptorPool.AllocateDescriptorSet( m_pipeline ),
                m_commandPool.AllocateCommandBuffer( )
            )
        );
    }
    auto reduction = m_container.front( );

    // Apply the reduction
    VkSha256Result vkSha256Result = {};
    reduction->Apply( slice, device, m_pipeline );
    auto vkResult = static_cast<VkResult>( *reduction );
    if (vkResult != VK_SUCCESS){
        // Ack
        cerr << "Failed to apply the reduction with error: " << static_cast<int64_t>( vkResult ) << endl;
        return vkSha256Result;
    }

    // Dispatch it
    auto vkQueue = device.Queue( );
    vkResult = reduction->Dispatch( vkQueue );
    if (vkResult != VK_SUCCESS){
        cerr << "Failed to dispatch the reduction operation with error: " << static_cast<int64_t>( vkResult ) << endl;
        return vkSha256Result;
    }
    return static_cast<VkSha256Result>( *reduction );
}

::std::unique_ptr<Reductions> Reductions::New(ComputeDevice& device) {

    // Look for an early out
    ::std::unique_ptr<Reductions> reductions;
    auto vkDevice = *device;
    if (vkDevice == VK_NULL_HANDLE){
        return reductions;
    }

    // Prep
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    const auto subgroupFeatureFlagMask = int(VK_SUBGROUP_FEATURE_BASIC_BIT) | int(VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT);

    // Query for subgroup support/size
    VkPhysicalDeviceSubgroupSizeControlPropertiesEXT vkPhysicalDeviceSubgroupSizeControlProperties = {};
    vkPhysicalDeviceSubgroupSizeControlProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
    VkPhysicalDeviceSubgroupProperties subgroupProperties = {};
    subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    subgroupProperties.pNext = &vkPhysicalDeviceSubgroupSizeControlProperties;
    VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
    vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkPhysicalDeviceProperties2.pNext = &subgroupProperties;
    device.GetPhysicalDeviceProperties2KHR( &vkPhysicalDeviceProperties2 );
    const auto subgroupFeatureFlags = subgroupProperties.supportedOperations & subgroupFeatureFlagMask;

    // Check whether subgroups are supported and that they are of a suitable size
    decltype(subgroupProperties.subgroupSize) subgroupSize = 1U;
#if defined(_MACOS_64_)
    // Further work required for subgroups under MoltenVK..
    auto subgroupsSupported = false;
#else
    auto subgroupsSupported = 
        (subgroupFeatureFlags != 0) && ((subgroupProperties.supportedStages & VK_SHADER_STAGE_COMPUTE_BIT) != 0);
#endif
    if (subgroupsSupported){
        // Determine our target subgroup size, preferring the minimum declared for the device
        // as (anecdotally) using it with our shader produces predictable + consistent results
        // on Intel integrated GPUs (e.g. "Intel(R) Iris(R) Xe Graphics")
        subgroupSize = subgroupProperties.subgroupSize;
        if (vkPhysicalDeviceSubgroupSizeControlProperties.minSubgroupSize > 1U){
            subgroupSize = vkPhysicalDeviceSubgroupSizeControlProperties.minSubgroupSize;
        }
    }
    subgroupsSupported = (subgroupSize > 1U);

    // If suitably-sized subgroups are supported,
    // then make the workgroup size the same as the subgroup size
    WorkgroupSize workgroupSize = {};
    WorkgroupSize *pWorkgroupSize = nullptr;
    if (subgroupsSupported){
        ::std::cout << "Subgroup feature flags = 0x" << std::hex << subgroupFeatureFlags << ::std::endl;
        ::std::cout << "Subgroups, with relative shuffle support, of size " << std::dec << subgroupSize << " are supported." << ::std::endl;

        workgroupSize.x = vkmr::largest_pow2_le( subgroupSize );
        workgroupSize.y = 1U;
        workgroupSize.z = 1U;
        workgroupSize.bySubgroup = true;
        pWorkgroupSize = &workgroupSize;
    }

    // Load the shader code, wrap it in a module, etc
    const auto shaderCodePath = subgroupsSupported ? "SHA-256-2-be-subgroups.spv" : "SHA-256-2-be.spv";
    ShaderModule shaderModule( vkDevice, shaderCodePath );
    auto vkResult = static_cast<VkResult>( shaderModule );

    // Create the descriptor set layout
    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
    if (vkResult == VK_SUCCESS){
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding1 = {};
        vkDescriptorSetLayoutBinding1.binding = 0;
        vkDescriptorSetLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vkDescriptorSetLayoutBinding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        vkDescriptorSetLayoutBinding1.descriptorCount = 1;
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBindings[] = {
            vkDescriptorSetLayoutBinding1
        };
        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
        vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        vkDescriptorSetLayoutCreateInfo.bindingCount = 1;
        vkDescriptorSetLayoutCreateInfo.pBindings = vkDescriptorSetLayoutBindings;
        vkResult = ::vkCreateDescriptorSetLayout( vkDevice, &vkDescriptorSetLayoutCreateInfo, pAllocator, &vkDescriptorSetLayout );
    }

    // Wrap it all up, maybe
    if (vkResult == VK_SUCCESS){
        // c.f. https://docs.vulkan.org/guide/latest/push_constants.html
        VkPushConstantRange vkPushConstantRange = {};
        vkPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        vkPushConstantRange.offset = 0;
        vkPushConstantRange.size = subgroupsSupported
            ? sizeof( BySubgroupPushConstants )
            : (sizeof( uint ) * 3);
        reductions.reset( new ReductionsImpl(
            device,
            vkmr::Pipeline(
                vkDevice,
                vkDescriptorSetLayout,
                vkmr::Pipeline::NewSimpleLayout( vkDevice, vkDescriptorSetLayout, &vkPushConstantRange ),
                ::std::move( shaderModule ),
                pWorkgroupSize
            ),
            subgroupsSupported
        ) );
    }
    return reductions;
}

} // namespace vkmr
