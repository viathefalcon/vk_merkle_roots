// Ops.cpp: defines the types, functions and classes which encapsulate asynchronous on-device operations 
//

// Includes
//

// C++ Standard Library Headers
#include <cstring>
#include <iostream>
#include <fstream>
#include <utility>

// Declarations
#include "Ops.h"

// Local Project Headers
#include "Debug.h"

#if defined (VULKAN_SUPPORT)
// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;

namespace vkmr {

// Classes
//
Mapping::Mapping(Mapping&& mapping):
    m_vkResult( mapping.m_vkResult ),
    m_vkDevice( mapping.m_vkDevice ),
    m_vkFence( mapping.m_vkFence ),
    m_descriptorSet( ::std::move( mapping.m_descriptorSet ) ),
    m_commandBuffer( ::std::move( mapping.m_commandBuffer ) ) {
    
    mapping.Reset( );
}

Mapping::Mapping(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkFence( VK_NULL_HANDLE ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ) {

    // Create the fence
    VkFenceCreateInfo vkFenceCreateInfo = {};
    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    m_vkResult = ::vkCreateFence( m_vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &m_vkFence );
}

Mapping& Mapping::operator=(Mapping&& mapping) {

    if (this != &mapping){
        Release( );

        m_vkResult = mapping.m_vkResult;
        m_vkDevice = mapping.m_vkDevice;
        m_vkFence = mapping.m_vkFence;
        m_descriptorSet = ::std::move( mapping.m_descriptorSet );
        m_commandBuffer = ::std::move( mapping.m_commandBuffer );

        mapping.Reset( );
    }
    return (*this);
}

Mapping& Mapping::Apply(Batch& batch, Slice<VkSha256Result>& slice, vkmr::Pipeline& pipeline) {

    // Update the descriptor set
    const auto batchBufferDescriptors = batch.BufferDescriptors( );
    const auto sliceBufferDescriptor = slice.BufferDescriptor( );
    VkWriteDescriptorSet vkWriteDescriptorSetInputs = {};
    vkWriteDescriptorSetInputs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetInputs.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetInputs.descriptorCount = 1;
    vkWriteDescriptorSetInputs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetInputs.pBufferInfo = &(batchBufferDescriptors.vkDescriptorBufferInputs);
    VkWriteDescriptorSet vkWriteDescriptorSetMetadata = {};
    vkWriteDescriptorSetMetadata.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetMetadata.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetMetadata.dstBinding = 1;
    vkWriteDescriptorSetMetadata.descriptorCount = 1;
    vkWriteDescriptorSetMetadata.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetMetadata.pBufferInfo = &(batchBufferDescriptors.vkDescriptorBufferMetdata);
    VkWriteDescriptorSet vkWriteDescriptorSetResults = {};
    vkWriteDescriptorSetResults.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vkWriteDescriptorSetResults.dstSet = *m_descriptorSet;
    vkWriteDescriptorSetResults.dstBinding = 2;
    vkWriteDescriptorSetResults.descriptorCount = 1;
    vkWriteDescriptorSetResults.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkWriteDescriptorSetResults.pBufferInfo = &(sliceBufferDescriptor);
    VkWriteDescriptorSet vkWriteDescriptorSets[] = {
        vkWriteDescriptorSetInputs,
        vkWriteDescriptorSetMetadata,
        vkWriteDescriptorSetResults
    };
    ::vkUpdateDescriptorSets( m_vkDevice, 3, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );

    // Update the command buffer
    auto vkCommandBuffer = *m_commandBuffer;
    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_vkResult = ::vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
    if (m_vkResult == VK_SUCCESS){
        ::vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline );

        VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
        ::vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.Layout( ), 0, 1, descriptorSets, 0, VK_NULL_HANDLE );

        // c.f. https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples#cpu-read-back-of-data-written-by-a-compute-shader
        VkMemoryBarrier2KHR host2ShaderMemB = {};
        host2ShaderMemB.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        host2ShaderMemB.srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
        host2ShaderMemB.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT_KHR;
        host2ShaderMemB.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
        host2ShaderMemB.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        VkDependencyInfoKHR host2ShaderDep = {};
        host2ShaderDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        host2ShaderDep.memoryBarrierCount = 1;
        host2ShaderDep.pMemoryBarriers = &host2ShaderMemB;
        g_VkCmdPipelineBarrier2KHR( vkCommandBuffer, &host2ShaderDep );

        // Actually dispatch the shader invocations
        ::vkCmdDispatch( vkCommandBuffer, static_cast<uint32_t>( batch.Count( ) ), 1, 1 );
        m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
    }
    return (*this);
}

VkResult Mapping::Dispatch(VkQueue vkQueue) {

    // Look for an early out
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }

    // Reset the fence
    m_vkResult = ::vkResetFences( m_vkDevice, 1, &m_vkFence );
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }
    VkCommandBuffer commandBuffers[] = { *m_commandBuffer };

    // Submit the (presumably previously-recordded) commands onto the queue
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = commandBuffers;
    m_vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence );
    if (m_vkResult == VK_SUCCESS){
        // Wait
        m_vkResult = ::vkWaitForFences( m_vkDevice, 1, &m_vkFence, true, uint64_t(-1) );
    }
    return m_vkResult;
}

void Mapping::Reset(void) {
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkFence = VK_NULL_HANDLE;
}

void Mapping::Release(void) {

    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, VK_NULL_HANDLE );
    }
    m_descriptorSet = DescriptorSet( );
    m_commandBuffer = CommandBuffer( );
    Reset( );
}

vkmr::Pipeline Mapping::Pipeline(ComputeDevice& device) {

    // Look for an early out
    auto vkDevice = *device;
    if (vkDevice == VK_NULL_HANDLE){
        return vkmr::Pipeline( );
    }

    // Prep
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Load the shader code
    ::std::ifstream ifs( "SHA-256-n.spv", ::std::ios::binary | ::std::ios::ate );
    const auto g = ifs.tellg( );
    ifs.seekg( 0 );
    std::vector<uint32_t> shaderCode( g / sizeof( uint32_t ) );
    ifs.read( reinterpret_cast<char*>( shaderCode.data( ) ), g );
    ifs.close( );
    ::std::cout << "Loaded " << shaderCode.size() << " (32-bit) word(s) of shader code." << ::std::endl;

    // Create the shader module
    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderModuleCreateInfo.codeSize = shaderCode.size( ) * sizeof( uint32_t );
    vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( shaderCode.data( ) );
    VkResult vkResult = ::vkCreateShaderModule( vkDevice, &vkShaderModuleCreateInfo, pAllocator, &vkShaderModule );

    // Create the descriptor set layout
    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
    if (vkResult == VK_SUCCESS){
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding1 = {};
        vkDescriptorSetLayoutBinding1.binding = 0;
        vkDescriptorSetLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vkDescriptorSetLayoutBinding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        vkDescriptorSetLayoutBinding1.descriptorCount = 1;
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding2 = vkDescriptorSetLayoutBinding1;
        vkDescriptorSetLayoutBinding2.binding = 1;
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding3 = vkDescriptorSetLayoutBinding1;
        vkDescriptorSetLayoutBinding3.binding = 2;
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBindings[] = {
            vkDescriptorSetLayoutBinding1,
            vkDescriptorSetLayoutBinding2,
            vkDescriptorSetLayoutBinding3
        };
        VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
        vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        vkDescriptorSetLayoutCreateInfo.bindingCount = 3;
        vkDescriptorSetLayoutCreateInfo.pBindings = vkDescriptorSetLayoutBindings;
        vkResult = ::vkCreateDescriptorSetLayout( vkDevice, &vkDescriptorSetLayoutCreateInfo, pAllocator, &vkDescriptorSetLayout );
    }

    // Wrap it all up, maybe
    return (vkResult == VK_SUCCESS)
        ? vkmr::Pipeline( vkDevice, vkShaderModule, vkDescriptorSetLayout, vkmr::Pipeline::DefaultLayout( vkDevice, vkDescriptorSetLayout ) )
        : vkmr::Pipeline( );
}

Reduction::Reduction(Reduction&& reduction):
    m_vkResult( reduction.m_vkResult ),
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

Reduction::Reduction(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer):
    m_vkResult( VK_RESULT_MAX_ENUM ),
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

Reduction& Reduction::operator=(Reduction&& reduction) {

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

Reduction& Reduction::Apply(Slice<VkSha256Result>& slice, ComputeDevice& device, vkmr::Pipeline& pipeline) {

    // Release any previously-held memory
    this->Free( );

    // Get the (approx) memory requirements
    m_count = slice.Reserved( );
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
                    //std::cout << "Duplicating item at " << int64_t(vkBufferCopy.srcOffset) << " to " << int64_t(vkBufferCopy.dstOffset) << "; count == " << count << ", delta == " << delta << std::endl;
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
                count >>= 1;
                ::vkCmdDispatch( vkCommandBuffer, count, 1, 1 );
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

VkResult Reduction::Dispatch(VkQueue vkQueue) {

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
        // Iterate the results (🤞)
        const auto pResults = static_cast<uint8_t*>( pMapped );
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
    }

    // Cleanup & return
    if (pMapped){
        ::vkUnmapMemory( m_vkDevice, m_vkHostMemory );
    }
    return m_vkResult;
}

vkmr::Pipeline Reduction::Pipeline(ComputeDevice& device) {

    // Look for an early out
    auto vkDevice = *device;
    if (vkDevice == VK_NULL_HANDLE){
        return vkmr::Pipeline( );
    }

    // Prep
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Load the shader code
    ::std::ifstream ifs( "SHA-256-2-be.spv", ::std::ios::binary | ::std::ios::ate );
    const auto g = ifs.tellg( );
    ifs.seekg( 0 );
    std::vector<uint32_t> shaderCode( g / sizeof( uint32_t ) );
    ifs.read( reinterpret_cast<char*>( shaderCode.data( ) ), g );
    ifs.close( );
    ::std::cout << "Loaded " << shaderCode.size() << " (32-bit) word(s) of shader code." << ::std::endl;

    // Create the shader module
    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderModuleCreateInfo.codeSize = shaderCode.size( ) * sizeof( uint32_t );
    vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( shaderCode.data( ) );
    VkResult vkResult = ::vkCreateShaderModule( vkDevice, &vkShaderModuleCreateInfo, pAllocator, &vkShaderModule );

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

    // Create the pipeline layout
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    if (vkResult == VK_SUCCESS){
        // c.f. https://docs.vulkan.org/guide/latest/push_constants.html
        VkPushConstantRange vkPushConstantRange = {};
        vkPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        vkPushConstantRange.offset = 0;
        vkPushConstantRange.size = (sizeof( uint ) * 3);

        VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
        vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vkPipelineLayoutCreateInfo.setLayoutCount = 1;
        vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
        vkPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        vkPipelineLayoutCreateInfo.pPushConstantRanges = &vkPushConstantRange;
        vkResult = ::vkCreatePipelineLayout( vkDevice, &vkPipelineLayoutCreateInfo, VK_NULL_HANDLE, &vkPipelineLayout );
    }

    // Query for subgroup support
    VkPhysicalDeviceSubgroupProperties subgroupProperties = {};
    subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
    VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
    vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkPhysicalDeviceProperties2.pNext = &subgroupProperties;
    device.GetPhysicalDeviceProperties2KHR( &vkPhysicalDeviceProperties2 );

    // Use to make the workgroup size the same as the subgroup size  
    uint32_t subgroupSize = 1U;
    if ((subgroupProperties.supportedOperations & VK_SHADER_STAGE_COMPUTE_BIT) != 0){
        subgroupSize = subgroupProperties.subgroupSize;
        ::std::cout << "Subgroups of size " << subgroupSize << " are supported." << ::std::endl;
    }
    uint32_t workgroupSize[3] = {subgroupSize, 1U, 1U};
    VkSpecializationMapEntry vkSpecializationMapEntries[3] = {};
    vkSpecializationMapEntries[0].constantID = 0;
    vkSpecializationMapEntries[0].offset = 0;
    vkSpecializationMapEntries[0].size = sizeof(uint32_t);
    vkSpecializationMapEntries[1].constantID = 1;
    vkSpecializationMapEntries[1].offset = vkSpecializationMapEntries[0].size + sizeof(uint32_t);
    vkSpecializationMapEntries[1].size = sizeof(uint32_t);
    vkSpecializationMapEntries[2].constantID = 2;
    vkSpecializationMapEntries[2].offset = vkSpecializationMapEntries[1].size + sizeof(uint32_t);
    vkSpecializationMapEntries[2].size = sizeof(uint32_t);
    VkSpecializationInfo specializationInfo = {};
    specializationInfo.mapEntryCount = 3;
    specializationInfo.pMapEntries = vkSpecializationMapEntries;
    specializationInfo.dataSize = sizeof( workgroupSize );
    specializationInfo.pData = workgroupSize;

    // Wrap it all up, maybe
    return (vkResult == VK_SUCCESS)
        ? vkmr::Pipeline( vkDevice, vkShaderModule, vkDescriptorSetLayout, vkPipelineLayout, &specializationInfo )
        : vkmr::Pipeline( );
}

void Reduction::Free(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkBufferHost != VK_NULL_HANDLE){
        ::vkDestroyBuffer( m_vkDevice, m_vkBufferHost, pAllocator );
    }
    if (m_vkHostMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, m_vkHostMemory, pAllocator );
    }
}

void Reduction::Reset(void) {
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkSize = 0U;
    m_count = 0U;
    m_vkFence = VK_NULL_HANDLE;
    m_vkBufferHost = VK_NULL_HANDLE;
    m_vkHostMemory = VK_NULL_HANDLE;
}

void Reduction::Release(void) {

    this->Free( );
    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, VK_NULL_HANDLE );
    }
    m_descriptorSet = DescriptorSet( );
    m_commandBuffer = CommandBuffer( );
    Reset( );
}

} // namespace vkmr
#endif // defined (VULKAN_SUPPORT)