// Reductions.cpp: defines the types, functions and classes which encapsulate asynchronous on-device reduction operations 
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <cstring>
#include <iostream>
#include <utility>
#include <unordered_map>

// Declarations
#include "Ops.h"

// Local Project Headers
#include "Debug.h"
#include "Utils.h"
#include "SHA-256plus.h"
#include "QueryPoolTimers.h"

// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_pVkCmdPipelineBarrier2KHR;

namespace vkmr {

using ::std::vector;

// Types
//

struct alignas(uint) BySubgroupPushConstants {
    uint offset;
    uint pairs;
    uint delta;
    uint d2;
    uint bound;
};

// Classes
//

// An extension of the CPU-based implementation for which
// the input are already hashed
class CpuSha256DforReductions : public CpuSha256D {
public:
    bool Add(const VkSha256Result&);
};

bool CpuSha256DforReductions::Add(const VkSha256Result& vkSha256Result) {

    CpuSha256D::node_type leaf;
    for (uint32_t u = 0U; u < SHA256_WC; ++u){
        leaf.push_back( vkSha256Result.data[u] );
    }
    m_leaves.push_back( leaf );
    return true;
}

class Reduction {
public:
    virtual ~Reduction();

    operator VkResult() const { return m_vkResult; }
    operator VkFence() const { return m_vkFence; }

    Reductions::slice_type::number_type Number(void) const {
        return m_slice.Number( );
    }

    VkSha256Result Read(void);
    VkResult Apply(Reductions::slice_type&&, ComputeDevice&, const vkmr::Pipeline&);
    virtual double Elapsed(void);

protected:
    Reduction();
    Reduction(VkResult, VkDevice);

    virtual void Free(void);
    virtual CommandBuffer& GetCommandBuffer(ComputeDevice&, const vkmr::Pipeline&) = 0;

    VkResult m_vkResult;
    VkDevice m_vkDevice;

    VkFence m_vkFence;
    VkBuffer m_vkBufferHost;
    VkDeviceMemory m_vkHostMemory;

    Reductions::slice_type m_slice;
};

Reduction::Reduction(VkResult vkResult, VkDevice vkDevice):
    m_vkResult( vkResult ),
    m_vkDevice( vkDevice ),
    m_vkFence( VK_NULL_HANDLE ),
    m_vkBufferHost( VK_NULL_HANDLE ),
    m_vkHostMemory( VK_NULL_HANDLE ) { }

Reduction::Reduction(): Reduction( VK_RESULT_MAX_ENUM, VK_NULL_HANDLE ) { }

Reduction::~Reduction() {
    this->Free( );
}

VkSha256Result Reduction::Read(void) {

    // Default to nothing
    VkSha256Result result = { 0 };

    // Map in the memory, maybe
    void* pMapped = nullptr;
    if (m_vkResult == VK_SUCCESS){
        m_vkResult = ::vkMapMemory( m_vkDevice, m_vkHostMemory, 0U, VK_WHOLE_SIZE, 0, &pMapped );
    }
    if (m_vkResult == VK_SUCCESS){
        const auto pResults = static_cast<uint8_t*>( pMapped );
        ::std::memcpy( &result, pResults, sizeof( VkSha256Result ) );
    }

    // Cleanup & return
    if (pMapped){
        ::vkUnmapMemory( m_vkDevice, m_vkHostMemory );
    }
    return result;
}

VkResult Reduction::Apply(Reductions::slice_type&& slice, ComputeDevice& device, const vkmr::Pipeline& pipeline) {

    // Capture the slice internally
    m_slice = ::std::move( slice );

    // Release any previously-held memory
    this->Free( );

    // Get the (approx) memory requirements
    const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( sizeof( VkSha256Result ) );

    // Look for some corresponding memory types, and try to allocate
    const auto memoryBudgets = device.AvailableMemoryTypes(
        vkMemoryRequirements,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    );
    for (auto it = memoryBudgets.cbegin( ), end = memoryBudgets.cend( ); it != end; it++){
        const auto& deviceMemoryBudget = *it;
        if (deviceMemoryBudget.vkMemoryBudget < vkMemoryRequirements.size){
            // We're not interested, yet..?
            continue;
        }

        // Try and allocate
        m_vkHostMemory = device.Allocate( deviceMemoryBudget, vkMemoryRequirements.size );
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
        vkBufferCreateInfo.size = sizeof( VkSha256Result );
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
        // Create a fence
        VkFenceCreateInfo vkFenceCreateInfo = {};
        vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        m_vkResult = ::vkCreateFence( m_vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &m_vkFence );
    }
    if (m_vkResult == VK_SUCCESS){
        auto& commandBuffer = this->GetCommandBuffer( device, pipeline );
        if (m_vkResult == VK_SUCCESS){
            VkCommandBuffer commandBuffers[] = { *commandBuffer };

            // Submit unto the queue
            VkSubmitInfo vkSubmitInfo = {};
            vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            vkSubmitInfo.commandBufferCount = 1;
            vkSubmitInfo.pCommandBuffers = commandBuffers;
            m_vkResult = ::vkQueueSubmit( device.Queue( ), 1, &vkSubmitInfo, m_vkFence );
        }
    }
    return m_vkResult; 
}

double Reduction::Elapsed(void) {
    return 0;
}

void Reduction::Free(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkBufferHost != VK_NULL_HANDLE){
        ::vkDestroyBuffer( m_vkDevice, m_vkBufferHost, pAllocator );
        m_vkBufferHost = VK_NULL_HANDLE;
    }
    if (m_vkHostMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, m_vkHostMemory, pAllocator );
        m_vkHostMemory = VK_NULL_HANDLE;
    }
    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, pAllocator );
        m_vkFence = VK_NULL_HANDLE;
    }
}

class BasicReduction : public Reduction {
public:
    BasicReduction(VkDevice, DescriptorSet&&, CommandBuffer&&);
    virtual ~BasicReduction(void) { }

    virtual CommandBuffer& GetCommandBuffer(ComputeDevice&, const vkmr::Pipeline&);

private:
    VkDeviceSize m_vkSize;
    uint m_count;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

BasicReduction::BasicReduction(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer):
    Reduction( VK_RESULT_MAX_ENUM, vkDevice ),
    m_vkSize( 0U ),
    m_count( 0U ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ) { }

CommandBuffer& BasicReduction::GetCommandBuffer(ComputeDevice& device, const vkmr::Pipeline& pipeline) {

    // Update the descriptor set (to point at the slice's buffer)
    m_count = m_slice.Count( );
    const auto sliceBufferDescriptor = m_slice.BufferDescriptor( );
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
                    g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
                }

                // Duplicate the last element
                VkBufferCopy vkBufferCopy = {};
                vkBufferCopy.size = sizeof( Reductions::slice_type::value_type );
                vkBufferCopy.srcOffset = vkBufferCopy.size * (count - 1U) * delta;
                vkBufferCopy.dstOffset = vkBufferCopy.srcOffset + (vkBufferCopy.size * delta);
                if ((vkBufferCopy.dstOffset + vkBufferCopy.size) > m_slice.Size( )){
                    // The copy would overflow the slice's memory - bail
                    m_vkResult = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                    break;
                }
                ::vkCmdCopyBuffer( vkCommandBuffer, m_slice.Buffer( ), m_slice.Buffer( ), 1, &vkBufferCopy );
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
                g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
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
                g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
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
        g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );

        // Add the command to copy from the slice buffer to the one we've allocated
        VkBufferCopy vkBufferCopy = {};
        vkBufferCopy.size = sizeof( VkSha256Result );
        ::vkCmdCopyBuffer( vkCommandBuffer, m_slice.Buffer( ), m_vkBufferHost, 1, &vkBufferCopy );
        m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
    }
    return m_commandBuffer;
}

class ReductionBySubgroup : public Reduction {
public:
    ReductionBySubgroup(VkDevice, DescriptorSet&&, CommandBuffer&&, QueryPoolTimer&&);
    virtual ~ReductionBySubgroup(void) { }

    virtual CommandBuffer& GetCommandBuffer(ComputeDevice&, const vkmr::Pipeline&);

    double Elapsed(void) {
        return m_queryPoolTimer.ElapsedMillis( );
    }

private:
    VkDeviceSize m_vkSize;
    uint m_count;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
    QueryPoolTimer m_queryPoolTimer;
};

ReductionBySubgroup::ReductionBySubgroup(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer, QueryPoolTimer&& queryPoolTimer):
    Reduction( VK_RESULT_MAX_ENUM, vkDevice ),
    m_vkSize( 0U ),
    m_count( 0U ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ),
    m_queryPoolTimer( ::std::move( queryPoolTimer ) ) { }

CommandBuffer& ReductionBySubgroup::GetCommandBuffer(ComputeDevice& device, const vkmr::Pipeline& pipeline) {

    m_count = m_slice.Count( );

    // Update the descriptor set (to point at the slice's buffer)
    const auto sliceBufferDescriptor = m_slice.BufferDescriptor( );
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

        // Capture the timstamp
        m_queryPoolTimer.Start( vkCommandBuffer );

        ::vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline );
        VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
        ::vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Layout( ), 0, 1, descriptorSets, 0, VK_NULL_HANDLE );

        // Setup
        auto halveven = [](uint u) -> uint {
            return (((u % 2 == 0) ? u : (u+1)) >> 1);
        };
        const auto& workgroupSize = pipeline.GetWorkGroupSize( );

        // Loop until we will have reduced to 1 element
        uint applicable = m_slice.Number( ) > 1 ? m_slice.Capacity( ) : m_slice.Count( );
        for (uint delta = 1U, count = m_count; applicable > 1U; ){
            applicable = halveven( applicable );

            // Get the actual number of work groups needed for the current pass
            const auto elements = count;
            const auto pairs = halveven( count );
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
                pc.pairs = ::std::min( applicable, workgroupSize.x );
                pc.delta = delta;
                pc.d2 = (delta << 1);
                pc.bound = m_count;
                ::vkCmdPushConstants( vkCommandBuffer, pipeline.Layout( ), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

                // Actually dispatch the shader invocations
                ::std::cout << m_slice.Number( ) << ": dispatching " << x << " subgroup-ed workgroup(s) of size " << pc.pairs << " for " << pairs << " pair(s) (" << elements << " element(s))" << ::std::endl;
                ::vkCmdDispatch( vkCommandBuffer, x, 1, 1 );

                // Advance
                remaining -= x;
            }

            // Tee up the next iteration
            applicable = workgroupSize.GetGroupCountX( applicable );
            if (applicable > 1U){
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
                g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );
            }
            delta *= (workgroupSize.x << 1); // x2 because each invocation in the group has addressed two items
        }

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
        g_pVkCmdPipelineBarrier2KHR( vkCommandBuffer, &vkDependencyInfo );

        // Add the command to copy from the slice buffer to the one we've allocated
        VkBufferCopy vkBufferCopy = {};
        vkBufferCopy.size = sizeof( VkSha256Result );
        ::vkCmdCopyBuffer( vkCommandBuffer, m_slice.Buffer( ), m_vkBufferHost, 1, &vkBufferCopy );

        // Capture the timestamp and wrap it up
        m_queryPoolTimer.Finish( vkCommandBuffer );
        m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
    }
    return (m_commandBuffer);
}

class ReductionFactory {
public:
    typedef ::std::shared_ptr<Reduction> ProductType;

    ReductionFactory(ComputeDevice& device, bool subgroupSupportPreferred):
        m_subgroupSupportPreferred( subgroupSupportPreferred ),
        m_queryPoolTimers( device ) { }
    ~ReductionFactory() = default;

    ProductType CreateReduction(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer) {

        using ::std::make_shared;

        ProductType product;
        if (m_subgroupSupportPreferred){
            product = make_shared<ReductionBySubgroup>(
                vkDevice,
                ::std::move( descriptorSet ),
                ::std::move( commandBuffer ),
                m_queryPoolTimers.New( )
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
    QueryPoolTimers m_queryPoolTimers;
};

class ReductionsImpl : public Reductions {
public:
    ReductionsImpl(ComputeDevice& device, vkmr::Pipeline&& pipeline, DescriptorPool&& descriptorPool, bool subgroupSupportPreferred):
        m_vkDevice( *device ),
        m_descriptorPool( ::std::move( descriptorPool ) ),
        m_commandPool( device.CreateCommandPool( ) ),
        m_pipeline( ::std::move( pipeline ) ),
        m_factory( ReductionFactory( device, subgroupSupportPreferred ) ) { }

    virtual ~ReductionsImpl() {

        m_container.clear( );
        m_descriptorPool = DescriptorPool( );
        m_commandPool = CommandPool( );
        m_pipeline = vkmr::Pipeline( );
    }

    VkResult Reduce(Reductions::slice_type&&, ComputeDevice&);

    void Update(void);

    ISha256D::out_type WaitFor(void);

private:
    VkDevice m_vkDevice;

    DescriptorPool m_descriptorPool;
    CommandPool m_commandPool;
    vkmr::Pipeline m_pipeline;

    ReductionFactory m_factory;
    vector<ReductionFactory::ProductType> m_container;
    ::std::unordered_map<slice_type::number_type, VkSha256Result> m_results;
};

VkResult ReductionsImpl::Reduce(Reductions::slice_type&& slice, ComputeDevice& device) {

    // Allocate and apply a new reduction
    auto reduction = m_factory.CreateReduction(
        m_vkDevice,
        m_descriptorPool.AllocateDescriptorSet( m_pipeline ),
        m_commandPool.AllocateCommandBuffer( )
    );
    auto vkResult = reduction->Apply( ::std::move( slice ), device, m_pipeline );
    if (vkResult == VK_SUCCESS){
        // Accumulate it
        m_container.push_back( ::std::move( reduction ) );
    }
    return vkResult;
}

void ReductionsImpl::Update(void) {

    for (auto it = m_container.begin( ); it != m_container.end( ); ) {
        auto reduction = (*it);
        auto vkFence = static_cast<VkFence>( *reduction );
        auto vkResult = ::vkGetFenceStatus( m_vkDevice, vkFence );
        if (vkResult == VK_SUCCESS){
            ::std::cout << "Reduction #" << reduction->Number( ) << " finished";
            auto elapsed = reduction->Elapsed( );
            if (elapsed != 0){
                ::std::cout << " in " << elapsed << "ms";
            }
            ::std::cout << "." << ::std::endl;

            VkSha256Result vkSha256Result = reduction->Read( );
            for (auto u = 0U; u < SHA256_WC; ++u){
                const uint w = vkSha256Result.data[u];
                vkSha256Result.data[u] = SWOP_ENDS_U32( w );
            }
            ::std::cout << "#" << reduction->Number( ) << ":" << print_bytes_ex( vkSha256Result.data, SHA256_WC ).str( ) << ::std::endl;

            // Done-zo..
            m_results.emplace(
                reduction->Number( ),
                reduction->Read( )
            );
            it = m_container.erase( it );
        }else{
            ++it;
        }
    }
}

ISha256D::out_type ReductionsImpl::WaitFor(void) {

    // Look for an early out
    if (m_container.empty( )){
        return "";
    }

    // Wait on all of the fences
    vector<VkFence> fences;
    for (auto it = m_container.cbegin( ), end = m_container.cend( ); it != end; ++it) {
        auto reduction = (*it);
        auto vkFence = static_cast<VkFence>( *reduction );
        fences.push_back( vkFence );
    }
    if (!fences.empty( )){
        ::vkWaitForFences( m_vkDevice, fences.size( ), fences.data( ), VK_TRUE, UINT64_MAX );
    }

    // Update to get out the results
    this->Update( );

    // Return
    if (m_results.size( ) == 1U){
        auto vkSha256Result = m_results.begin( )->second;

        // The output is big-endian in nature; convert to little endianess prior to output
        for (auto u = 0U; u < SHA256_WC; ++u){
            const uint w = vkSha256Result.data[u];
            vkSha256Result.data[u] = SWOP_ENDS_U32( w );
        }
        return print_bytes_ex( vkSha256Result.data, SHA256_WC ).str( );
    }

    CpuSha256DforReductions sha256D;
    for (slice_type::number_type u = 1U, bound = (u + m_results.size( )); u < bound; ++u ){
        const auto found = m_results.find( u );
        if (found == m_results.end( )){
            // Bail
            return "";
        }
        sha256D.Add( found->second );
    }
    return sha256D.Root( );
}

::std::unique_ptr<Reductions> Reductions::New(ComputeDevice& device, typename slice_type::number_type number) {

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

    // Allocate a descriptor pool
    ::std::cout << "Allocating for up to " << number << " concurrent reduction(s).." << ::std::endl;
    auto descriptorPool = DescriptorPool( vkDevice, number, number );
    if (!descriptorPool){
        vkResult = descriptorPool;
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
            ::std::move( descriptorPool ),
            subgroupsSupported
        ) );
    }
    return reductions;
}

} // namespace vkmr
