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

#if defined (VULKAN_SUPPORT)
// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;

namespace vkmr {

// Classes
//
class Mapping {
public:
    Mapping(Mapping&&);
    Mapping(VkDevice, DescriptorSet&&, CommandBuffer&&);
    Mapping(Mapping const&) = delete;

    Mapping(void) { Reset( ); }
    ~Mapping(void) { Release( ); }

    Mapping& operator=(Mapping const&) = delete;
    Mapping& operator=(Mapping&&);
    operator bool() const { return (m_vkResult == VK_SUCCESS); }
    operator VkResult() const { return m_vkResult; }
    operator VkFence() const { return m_vkFence; }

    Mapping& Apply(Batch&, Slice<VkSha256Result>&, vkmr::Pipeline&);
    VkResult Dispatch(VkQueue);

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;
    VkFence m_vkFence;

    DescriptorSet m_descriptorSet;
    CommandBuffer m_commandBuffer;
};

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

        // Push the constants
        struct {
            uint bound;
        } pc;
        pc.bound = batch.Count( );
        ::vkCmdPushConstants( vkCommandBuffer, pipeline.Layout( ), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( pc ), &pc );

        // Figure out the number of workgroups
        const auto x = pipeline.GetWorkGroupSize( ).GetGroupCountX( pc.bound );

        // Actually dispatch the shader invocations
        ::vkCmdDispatch( vkCommandBuffer, x, 1, 1 );
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
    return (m_vkResult = ::vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, m_vkFence ));
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

class MappingsImpl : public Mappings {
public:
    MappingsImpl(ComputeDevice& device, vkmr::Pipeline&& pipeline):
        m_vkDevice( *device ),
        m_descriptorPool( device.CreateDescriptorPool( 1, 3 ) ),
        m_commandPool( device.CreateCommandPool( ) ),
        m_pipeline( ::std::move( pipeline ) ) { }

    virtual ~MappingsImpl(void) {

        m_container.clear( );
        m_descriptorPool = DescriptorPool( );
        m_commandPool = CommandPool( );
        m_pipeline = Pipeline( );
    }

    VkFence Map(Batch&, Slice<VkSha256Result>&, VkQueue);

private:
    VkDevice m_vkDevice;

    DescriptorPool m_descriptorPool;
    CommandPool m_commandPool;
    vkmr::Pipeline m_pipeline;

    ::std::vector<Mapping> m_container;
};

VkFence MappingsImpl::Map(Batch& batch, Slice<VkSha256Result>& slice, VkQueue vkQueue) {

    using ::std::cerr;
    using ::std::endl;

    // Get us a reference to a mapping instance
    if (m_container.empty( )){
        m_container.push_back(
            Mapping(
                m_vkDevice,
                m_descriptorPool.AllocateDescriptorSet( m_pipeline ),
                m_commandPool.AllocateCommandBuffer( )
            )
        );
    }
    auto& mapping = m_container.front( );

    // Apply the mapping from the batch to the slice
    mapping.Apply( batch, slice, m_pipeline );
    auto vkResult = static_cast<VkResult>( mapping );
    if (vkResult != VK_SUCCESS){
        // Ack
        cerr << "Failed to apply the mapping with error: " << static_cast<int64_t>( vkResult ) << endl;
        return VK_NULL_HANDLE;
    }
    vkResult = mapping.Dispatch( vkQueue );
    if (vkResult != VK_SUCCESS){
        cerr << "Failed to dispatch the mapping operation with error: " << static_cast<int64_t>( vkResult ) << endl;
        return VK_NULL_HANDLE;
    }
    return static_cast<VkFence>( mapping );
}

::std::unique_ptr<Mappings> Mappings::New(ComputeDevice& device) {

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
        vkPushConstantRange.size = sizeof( uint );
        mappings.reset( new MappingsImpl(
            device,
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
#endif // defined (VULKAN_SUPPORT)