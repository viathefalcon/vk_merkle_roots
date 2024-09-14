// Mappings.cpp: defines the types, functions and classes which encapsulate asynchronous on-device mapping operations 
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <cstring>
#include <utility>
#include <iostream>

// Declarations
#include "Ops.h"

// Local Project Headers
#include "Debug.h"

// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;

namespace vkmr {

// Types
//

struct alignas(uint) MappingPushConstants {
    uint offset;
    uint bound;
};

// Classes
//
class Mapping {
public:
    Mapping(Mapping&&);
    Mapping(VkDevice, DescriptorSet&&, CommandBuffer&&, Batch&&, Slice<VkSha256Result>&&, uint32_t);
    Mapping(Mapping const&) = delete;

    Mapping(void) { Reset( ); }
    ~Mapping(void) { Release( ); }

    Mapping& operator=(Mapping const&) = delete;
    Mapping& operator=(Mapping&&);
    operator bool() const { return (m_vkResult == VK_SUCCESS); }
    operator VkResult() const { return m_vkResult; }
    operator VkFence() const { return m_vkFence; }

    VkResult Dispatch(VkQueue, vkmr::Pipeline&);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkFence m_vkFence;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
    Batch m_batch;
    Slice<VkSha256Result> m_slice;

    uint32_t m_maxComputeWorkGroupCount;
};

Mapping::Mapping(Mapping&& mapping):
    m_vkResult( mapping.m_vkResult ),
    m_vkDevice( mapping.m_vkDevice ),
    m_vkFence( mapping.m_vkFence ),
    m_descriptorSet( ::std::move( mapping.m_descriptorSet ) ),
    m_commandBuffer( ::std::move( mapping.m_commandBuffer ) ),
    m_batch( ::std::move( mapping.m_batch ) ),
    m_slice( ::std::move( mapping.m_slice ) ),
    m_maxComputeWorkGroupCount( mapping.m_maxComputeWorkGroupCount ) {
    
    mapping.Reset( );
}

Mapping::Mapping(VkDevice vkDevice, DescriptorSet&& descriptorSet, CommandBuffer&& commandBuffer, Batch&& batch, Slice<VkSha256Result>&& slice, uint32_t maxComputeWorkGroupCount):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkFence( VK_NULL_HANDLE ),
    m_descriptorSet( ::std::move( descriptorSet ) ),
    m_commandBuffer( ::std::move( commandBuffer ) ),
    m_batch( ::std::move( batch ) ),
    m_slice( ::std::move( slice ) ),
    m_maxComputeWorkGroupCount( maxComputeWorkGroupCount ) {

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
        m_batch = ::std::move( mapping.m_batch );
        m_slice = ::std::move( mapping.m_slice );
        m_maxComputeWorkGroupCount = mapping.m_maxComputeWorkGroupCount;

        mapping.Reset( );
    }
    return (*this);
}

VkResult Mapping::Dispatch(VkQueue vkQueue, vkmr::Pipeline& pipeline) {

    // Reset the fence
    m_vkResult = ::vkResetFences( m_vkDevice, 1, &m_vkFence );
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }

     // Update the descriptor set
    const auto batchBufferDescriptors = m_batch.BufferDescriptors( );
    const auto sliceBufferDescriptor = m_slice.BufferDescriptor( );
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

        // Split into as many dispatches as are needed
        const auto bound = static_cast<uint>( m_batch.Count( ) );
        const auto& workgroupSize = pipeline.GetWorkGroupSize( );
        const auto count = workgroupSize.GetGroupCountX( bound );
        for (auto remaining = count; remaining > 0U; ){
            const auto x = ::std::min(
                remaining,
                m_maxComputeWorkGroupCount
            );

            // Push the constants
            MappingPushConstants pc = { 0U };
            pc.offset = workgroupSize.x * (count - remaining);
            pc.bound = m_batch.Count( );
            ::vkCmdPushConstants( vkCommandBuffer, pipeline.Layout( ), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

            // Actually dispatch the shader invocations
            ::vkCmdDispatch( vkCommandBuffer, x, 1, 1 );

            // Advance
            remaining -= x;
        }
        m_vkResult = ::vkEndCommandBuffer( vkCommandBuffer );
    }   
    VkCommandBuffer commandBuffers[] = { vkCommandBuffer };

    // Submit the (presumably previously-recordded) commands onto the queue
    VkSubmitInfo vkSubmitInfo = {};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vkSubmitInfo.commandBufferCount = 1;
    vkSubmitInfo.pCommandBuffers = commandBuffers;
    return (m_vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence ));
}

void Mapping::Reset(void) {
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkFence = VK_NULL_HANDLE;
    m_maxComputeWorkGroupCount = 0U;
}

void Mapping::Release(void) {

    if (m_vkFence != VK_NULL_HANDLE){
        ::vkDestroyFence( m_vkDevice, m_vkFence, VK_NULL_HANDLE );
    }
    m_descriptorSet = DescriptorSet( );
    m_commandBuffer = CommandBuffer( );
    Reset( );
}

class MappingsImpl : public Mappings {
public:
    MappingsImpl(ComputeDevice& device, uint32_t capacity, vkmr::Pipeline&& pipeline):
        m_vkDevice( *device ),
        m_capacity( capacity ),
        m_descriptorPool( device.CreateDescriptorPool( capacity, 3 * capacity ) ), // 1 set per potential concurrent mapping op
        m_commandPool( device.CreateCommandPool( ) ),
        m_pipeline( ::std::move( pipeline ) ) {

        VkPhysicalDeviceProperties vkPhysicalDeviceProperties = {};
        ::vkGetPhysicalDeviceProperties( device.PhysicalDevice( ), &vkPhysicalDeviceProperties );
        m_maxComputeWorkGroupCount = vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
    }

    virtual ~MappingsImpl(void) {

        m_container.clear( );
        m_descriptorPool = DescriptorPool( );
        m_commandPool = CommandPool( );
        m_pipeline = Pipeline( );
    }

    VkResult Map(Batch&&, Slice<VkSha256Result>&&, VkQueue);

    void Update(void);

    void WaitFor(void);

private:
    VkDevice m_vkDevice;
    uint32_t m_maxComputeWorkGroupCount, m_capacity;

    DescriptorPool m_descriptorPool;
    CommandPool m_commandPool;
    vkmr::Pipeline m_pipeline;

    ::std::vector<Mapping> m_container;
};

VkResult MappingsImpl::Map(Batch&& batch, Slice<VkSha256Result>&& slice, VkQueue queue) {

    // Descriptor sets is the limiting factor on the number of potential in-flight mappings
    const auto ok = (m_container.size( ) < m_capacity);
    auto descriptorSet = ok ? m_descriptorPool.AllocateDescriptorSet( m_pipeline ) : DescriptorSet( );

    // Create a new mapping
    m_container.push_back(
        Mapping(
            m_vkDevice,
            ::std::move( descriptorSet ),
            m_commandPool.AllocateCommandBuffer( ),
            ::std::move( batch ),
            ::std::move( slice ),
            m_maxComputeWorkGroupCount
        )
    );
    if (ok){
        // Dispatch the mapping onto the queue
        auto& mapping = m_container.back( );
        return mapping.Dispatch( queue, m_pipeline );
    }
    return VK_ERROR_OUT_OF_POOL_MEMORY;
}

void MappingsImpl::Update(void) {

    for (auto it = m_container.cbegin( ); it != m_container.cend( ); ) {
        // Get the current status
        auto status = ::vkGetFenceStatus( m_vkDevice, static_cast<VkFence>( *it ) );
        if (status == VK_SUCCESS){
            // Retire the mapping by removing from hte
            it = m_container.erase( it );
        }else{
            // Check again later; for now, just advance
            ++it;
        }
    }
}

void MappingsImpl::WaitFor(void) {

    // Get the in-flight mappings as a vector of fences
    ::std::vector<VkFence> fences;
    for (auto it = m_container.cbegin( ), end = m_container.cend( ); it != end; ++it) {
        fences.push_back( static_cast<VkFence>( *it ) );
    }
    if (fences.empty( )){
        return;
    }

    // Wait on them all
    ::vkWaitForFences( m_vkDevice, fences.size( ), fences.data( ), VK_TRUE, UINT64_MAX );

    // Retire them all
    m_container.clear( );
}

::std::unique_ptr<Mappings> Mappings::New(ComputeDevice& device, uint32_t capacity) {

    // Look for an early out
    ::std::unique_ptr<Mappings> mappings;
    auto vkDevice = *device;
    if (vkDevice == VK_NULL_HANDLE){
        return mappings;
    }

    // Prep
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Load the shader code, wrap it in a module, etc
    ShaderModule shaderModule( vkDevice, "SHA-256-n.spv" );
    auto vkResult = static_cast<VkResult>( shaderModule );

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
        vkResult = ::vkCreateDescriptorSetLayout(
            vkDevice,
            &vkDescriptorSetLayoutCreateInfo,
            pAllocator,
            &vkDescriptorSetLayout
        );
    }

    WorkgroupSize workgroupSize;
    workgroupSize.x = workgroupSize.y = workgroupSize.z = 1;
    if (vkResult == VK_SUCCESS){
        // Query for the device limits
        VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
        vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        device.GetPhysicalDeviceProperties2KHR( &vkPhysicalDeviceProperties2 );

        // Figure out how big the work group can be in the x-dimension
        workgroupSize.x = ::std::min(
            vkPhysicalDeviceProperties2.properties.limits.maxComputeWorkGroupSize[0],
            vkPhysicalDeviceProperties2.properties.limits.maxComputeWorkGroupInvocations
        );
    }

    // Wrap it all up, maybe
    if (vkResult == VK_SUCCESS){
        VkPushConstantRange vkPushConstantRange = {};
        vkPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        vkPushConstantRange.offset = 0;
        vkPushConstantRange.size = sizeof( MappingPushConstants );
        mappings.reset( new MappingsImpl(
            device,
            capacity,
            vkmr::Pipeline(
                vkDevice,
                vkDescriptorSetLayout,
                vkmr::Pipeline::NewSimpleLayout( vkDevice, vkDescriptorSetLayout, &vkPushConstantRange ),
                ::std::move( shaderModule ),
                &workgroupSize
            )
        ) );
    }
    return mappings;
}

} // namespace vkmr
