// Devices.cpp: defines the types, functions and classes that slices memory for inputs and outputs
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <cstring>
#include <iostream>

// Local Project Headers
#include "Debug.h"

// Declarations
#include "Devices.h"

#if defined (VULKAN_SUPPORT)
namespace vkmr {

// Classes
//

DescriptorSet::DescriptorSet(VkDevice vkDevice, VkDescriptorPool vkDescriptorPool, VkDescriptorSetLayout vkDescriptorSetLayout, uint32_t descriptorSetCount):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkDescriptorPool( vkDescriptorPool ),
    m_descriptorSetCount( descriptorSetCount ),
    m_vkDescriptorSet( VK_NULL_HANDLE ) {
    
    VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
    vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vkDescriptorSetAllocateInfo.descriptorPool = m_vkDescriptorPool;
    vkDescriptorSetAllocateInfo.descriptorSetCount = m_descriptorSetCount;
    vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;
    m_vkResult = ::vkAllocateDescriptorSets( m_vkDevice, &vkDescriptorSetAllocateInfo, &m_vkDescriptorSet );
}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet):
    m_vkResult( descriptorSet.m_vkResult ),
    m_vkDevice( descriptorSet.m_vkDevice ),
    m_vkDescriptorPool( descriptorSet.m_vkDescriptorPool ),
    m_descriptorSetCount( descriptorSet.m_descriptorSetCount ),
    m_vkDescriptorSet( descriptorSet.m_vkDescriptorSet ) {

    descriptorSet.Release( );
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& descriptorSet) {

    if (this != &descriptorSet){
        Release( );

        m_vkResult = descriptorSet.m_vkResult;
        m_vkDevice = descriptorSet.m_vkDevice;
        m_vkDescriptorPool = descriptorSet.m_vkDescriptorPool;
        m_descriptorSetCount = descriptorSet.m_descriptorSetCount;
        m_vkDescriptorSet = descriptorSet.m_vkDescriptorSet;

        descriptorSet.Reset( );
    }
    return (*this);
}

void DescriptorSet::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkDescriptorPool = VK_NULL_HANDLE;
    m_descriptorSetCount = uint32_t(-1);
    m_vkDescriptorSet = VK_NULL_HANDLE;
}

void DescriptorSet::Release(void) {

    if (m_vkDescriptorSet != VK_NULL_HANDLE){
        ::vkFreeDescriptorSets( m_vkDevice, m_vkDescriptorPool, m_descriptorSetCount, &m_vkDescriptorSet );
    }
    Reset( );
}

CommandBuffer::CommandBuffer(VkDevice vkDevice, VkCommandPool vkCommandPool, VkPipelineLayout vkPipelineLayout, VkPipeline vkPipeline):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkCommandPool( vkCommandPool ),
    m_vkPipelineLayout( vkPipelineLayout ),
    m_vkPipeline( vkPipeline ),
    m_vkCommandBuffer( VK_NULL_HANDLE ) {
    
    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vkCommandBufferAllocateInfo.commandPool = m_vkCommandPool;
    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkCommandBufferAllocateInfo.commandBufferCount = 1;
    m_vkResult = ::vkAllocateCommandBuffers( m_vkDevice, &vkCommandBufferAllocateInfo, &m_vkCommandBuffer );
}

CommandBuffer::CommandBuffer(CommandBuffer&& commandBuffer):
    m_vkResult( commandBuffer.m_vkResult ),
    m_vkDevice( commandBuffer.m_vkDevice ),
    m_vkCommandPool( commandBuffer.m_vkCommandPool ),
    m_vkPipelineLayout( commandBuffer.m_vkPipelineLayout ),
    m_vkPipeline( commandBuffer.m_vkPipeline ),
    m_vkCommandBuffer( commandBuffer.m_vkCommandBuffer ) {

    commandBuffer.Release( );
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& commandBuffer) {

    if (this != &commandBuffer){
        Release( );

        m_vkResult = commandBuffer.m_vkResult;
        m_vkDevice = commandBuffer.m_vkDevice;
        m_vkCommandPool = commandBuffer.m_vkCommandPool;
        m_vkPipelineLayout = commandBuffer.m_vkPipelineLayout;
        m_vkPipeline = commandBuffer.m_vkPipeline;
        m_vkCommandBuffer = commandBuffer.m_vkCommandBuffer;

        commandBuffer.Reset( );
    }
    return (*this);
}

VkResult CommandBuffer::BindDispatch(DescriptorSet& descriptorSet, uint32_t count) {

    // Look for an early out
    if (m_vkResult != VK_SUCCESS){
        return m_vkResult;
    }

    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    m_vkResult = vkBeginCommandBuffer( m_vkCommandBuffer, &vkCommandBufferBeginInfo );
    if (m_vkResult == VK_SUCCESS){
        VkDescriptorSet descriptorSets[] = { *descriptorSet };

        vkCmdBindPipeline( m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vkPipeline );
        vkCmdBindDescriptorSets( m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vkPipelineLayout, 0, 1, descriptorSets, 0, VK_NULL_HANDLE );
        vkCmdDispatch( m_vkCommandBuffer, count, 1, 1 );
        m_vkResult = vkEndCommandBuffer( m_vkCommandBuffer );
    }
    return m_vkResult;
}

void CommandBuffer::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkCommandPool = VK_NULL_HANDLE;
    m_vkPipelineLayout = VK_NULL_HANDLE;
    m_vkPipeline = VK_NULL_HANDLE;
    m_vkCommandBuffer = VK_NULL_HANDLE;
}

void CommandBuffer::Release(void) {

    if (m_vkCommandBuffer != VK_NULL_HANDLE){
        ::vkFreeCommandBuffers( m_vkDevice, m_vkCommandPool, 1, &m_vkCommandBuffer );
    }
    Reset( );
}

ComputeDevice::ComputeDevice(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily, uint32_t queueCount, const ::std::vector<uint32_t>& shaderCode):
    m_vkPhysicalDevice( vkPhysicalDevice ),
    m_queueFamily( queueFamily ),
    m_queueCount( queueCount ),
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( VK_NULL_HANDLE),
    m_vkShaderModule( VK_NULL_HANDLE ),
    m_vkCommandPool( VK_NULL_HANDLE),
    m_vkDescriptorSetLayout( VK_NULL_HANDLE),
    m_vkPipelineLayout( VK_NULL_HANDLE ),
    m_vkPipeline( VK_NULL_HANDLE ),
    m_vkDescriptorPool( VK_NULL_HANDLE ) {

    using ::std::strcmp;
    using ::std::vector;
    using ::std::cout;
    using ::std::endl;
 
    // Enumerate the device extensions
    vector<char*> deviceExtNames;
    uint32_t uDeviceExtensionPropertyCount = 0;
    vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, VK_NULL_HANDLE, &uDeviceExtensionPropertyCount, VK_NULL_HANDLE );
    if (uDeviceExtensionPropertyCount > 0){
        // Allocate and query
        VkExtensionProperties* pVkExtensionProperties = new (std::nothrow) VkExtensionProperties[uDeviceExtensionPropertyCount];
        if (pVkExtensionProperties){
            vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, VK_NULL_HANDLE, &uDeviceExtensionPropertyCount, pVkExtensionProperties );
            for (decltype(uDeviceExtensionPropertyCount) u = 0; u < uDeviceExtensionPropertyCount; ++u){
                VkExtensionProperties* vkExtensionProperties = (pVkExtensionProperties + u);
                const char* extensionName = vkExtensionProperties->extensionName;

                if (strcmp( extensionName, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( (char*) VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME );
                    continue;
                }
                if (strcmp( extensionName, VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( (char*) VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME );
                    continue;
                }
                if (strcmp( extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME ) == 0){
                    deviceExtNames.push_back( (char*) VK_EXT_MEMORY_BUDGET_EXTENSION_NAME );
                    continue;
                }
            }
            delete[] pVkExtensionProperties;
        }
    }
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Try and create the device along w/all the queues..
    vector<float> queuePriorities(queueCount, 1.0f);
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {};
    vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vkDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
    vkDeviceQueueCreateInfo.queueCount = queuePriorities.size( );
    vkDeviceQueueCreateInfo.pQueuePriorities = queuePriorities.data( );
    VkDeviceCreateInfo vkDeviceCreateInfo = {};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.queueCreateInfoCount = 1;
    vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;
    vkDeviceCreateInfo.enabledExtensionCount = deviceExtNames.size( );
    vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtNames.data( );
    m_vkResult = ::vkCreateDevice( vkPhysicalDevice, &vkDeviceCreateInfo, pAllocator, &m_vkDevice );
    if (m_vkResult == VK_SUCCESS){
        // Create the shader module
        VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
        vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vkShaderModuleCreateInfo.codeSize = shaderCode.size( ) * sizeof( uint32_t );
        vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( shaderCode.data( ) );
        m_vkResult = ::vkCreateShaderModule( m_vkDevice, &vkShaderModuleCreateInfo, pAllocator, &m_vkShaderModule );
    }
    if (m_vkResult == VK_SUCCESS){
        // Create the command pool
        VkCommandPoolCreateInfo vkCommandPoolCreateInfo = {};
        vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        vkCommandPoolCreateInfo.queueFamilyIndex = queueFamily;
        vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        m_vkResult = ::vkCreateCommandPool( m_vkDevice, &vkCommandPoolCreateInfo, pAllocator, &m_vkCommandPool );
    }
    if (m_vkResult == VK_SUCCESS){
        // Initialise the descriptor set layout
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
        m_vkResult = ::vkCreateDescriptorSetLayout( m_vkDevice, &vkDescriptorSetLayoutCreateInfo, pAllocator, &m_vkDescriptorSetLayout );
    }
    if (m_vkResult == VK_SUCCESS){
        // Create the pipeline layout
        VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
        vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        vkPipelineLayoutCreateInfo.setLayoutCount = 1;
        vkPipelineLayoutCreateInfo.pSetLayouts = &m_vkDescriptorSetLayout;
        m_vkResult = ::vkCreatePipelineLayout( m_vkDevice, &vkPipelineLayoutCreateInfo, pAllocator, &m_vkPipelineLayout );
    }
    if (m_vkResult == VK_SUCCESS){
        // Initialise the compute pipeline
        VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
        vkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vkPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        vkPipelineShaderStageCreateInfo.module = m_vkShaderModule;
        vkPipelineShaderStageCreateInfo.pName = "main";
        VkComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
        vkComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        vkComputePipelineCreateInfo.stage = vkPipelineShaderStageCreateInfo;
        vkComputePipelineCreateInfo.layout = m_vkPipelineLayout;
        m_vkResult = ::vkCreateComputePipelines( m_vkDevice, 0, 1, &vkComputePipelineCreateInfo, pAllocator, &m_vkPipeline );
    }
    if (m_vkResult == VK_SUCCESS){
        // Create the descriptor pool
        VkDescriptorPoolSize vkDescriptorPoolSize = {};
        vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        vkDescriptorPoolSize.descriptorCount = 3;
        VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
        vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        vkDescriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        vkDescriptorPoolCreateInfo.maxSets = 1;
        vkDescriptorPoolCreateInfo.poolSizeCount = 1;
        vkDescriptorPoolCreateInfo.pPoolSizes = &vkDescriptorPoolSize;
        m_vkResult = ::vkCreateDescriptorPool( m_vkDevice, &vkDescriptorPoolCreateInfo, pAllocator, &m_vkDescriptorPool );
    }
}

ComputeDevice::ComputeDevice(ComputeDevice&& device):
    m_vkPhysicalDevice( device.m_vkPhysicalDevice ),
    m_queueFamily( device.m_queueFamily ),
    m_queueCount( device.m_queueCount ),
    m_vkResult( device.m_vkResult ),
    m_vkDevice( device.m_vkDevice),
    m_vkShaderModule( device.m_vkShaderModule ),
    m_vkCommandPool( device.m_vkCommandPool),
    m_vkDescriptorSetLayout( device.m_vkDescriptorSetLayout ),
    m_vkPipelineLayout( device.m_vkPipelineLayout ),
    m_vkPipeline( device.m_vkPipeline ),
    m_vkDescriptorPool( device.m_vkDescriptorPool ) {

    device.Release( );
}

ComputeDevice& ComputeDevice::operator=(ComputeDevice&& device) {

    if (this != &device){
        Release( );

        m_vkPhysicalDevice = device.m_vkPhysicalDevice;
        m_queueFamily = device.m_queueFamily;
        m_queueCount = device.m_queueCount;
        m_vkResult = device.m_vkResult;
        m_vkDevice = device.m_vkDevice;
        m_vkShaderModule = device.m_vkShaderModule;
        m_vkCommandPool = device.m_vkCommandPool;
        m_vkDescriptorSetLayout = device.m_vkDescriptorSetLayout;
        m_vkPipelineLayout = device.m_vkPipelineLayout;
        m_vkPipeline = device.m_vkPipeline;
        m_vkDescriptorPool = device.m_vkDescriptorPool;

        device.Reset( );
    }
    return (*this);
}

VkQueue ComputeDevice::Queue(uint32_t queueIndex) {

    VkQueue vkQueue = VK_NULL_HANDLE;
    ::vkGetDeviceQueue( m_vkDevice, m_queueFamily, queueIndex, &vkQueue );
    if (vkQueue == VK_NULL_HANDLE){
        ::std::cerr << "Failed to retrieve handle to device queue!" << ::std::endl;
    }
    return vkQueue;
}

VkMemoryRequirements ComputeDevice::StorageBufferRequirements(VkDeviceSize vkSize) const {

    // Setup
    VkMemoryRequirements vkMemoryRequirements = {};

    // Create the buffer
    VkBufferCreateInfo vkBufferCreateInfo = {};
    vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    vkBufferCreateInfo.size = vkSize;
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    const VkResult vkResult = ::vkCreateBuffer(
        m_vkDevice,
        &vkBufferCreateInfo,
        VK_NULL_HANDLE,
        &vkBuffer
    );
    if (vkResult == VK_SUCCESS){
        // Query the buffer
        ::vkGetBufferMemoryRequirements( m_vkDevice, vkBuffer, &vkMemoryRequirements );
        ::vkDestroyBuffer( m_vkDevice, vkBuffer, VK_NULL_HANDLE );
    }
    return vkMemoryRequirements;
}

VkDeviceSize ComputeDevice::MinStorageBufferOffset(void) const {

    VkPhysicalDeviceProperties properties;
    ::vkGetPhysicalDeviceProperties( m_vkPhysicalDevice, &properties );
    return properties.limits.minStorageBufferOffsetAlignment;
}

DescriptorSet ComputeDevice::AllocateDescriptorSet(uint32_t count) const {
    return DescriptorSet( m_vkDevice, m_vkDescriptorPool, m_vkDescriptorSetLayout, count );
}

CommandBuffer ComputeDevice::AllocateCommandBuffer(void) const {
    return CommandBuffer( m_vkDevice, m_vkCommandPool, m_vkPipelineLayout, m_vkPipeline );
}

void ComputeDevice::Reset() {

    m_vkPhysicalDevice = VK_NULL_HANDLE;
    m_queueCount = 0;
    m_queueFamily = uint32_t(-1);
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkShaderModule = VK_NULL_HANDLE;
    m_vkCommandPool = VK_NULL_HANDLE;
    m_vkDescriptorSetLayout = VK_NULL_HANDLE;
    m_vkPipelineLayout = VK_NULL_HANDLE;
    m_vkPipeline = VK_NULL_HANDLE;
    m_vkDescriptorPool = VK_NULL_HANDLE;
}

void ComputeDevice::Release() {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkDescriptorPool != VK_NULL_HANDLE){
        ::vkDestroyDescriptorPool( m_vkDevice, m_vkDescriptorPool, pAllocator );
    }
    if (m_vkPipeline != VK_NULL_HANDLE){
        ::vkDestroyPipeline( m_vkDevice, m_vkPipeline, pAllocator );
    }
    if (m_vkPipelineLayout != VK_NULL_HANDLE){
        ::vkDestroyPipelineLayout( m_vkDevice, m_vkPipelineLayout, pAllocator );
    }
    if (m_vkDescriptorSetLayout != VK_NULL_HANDLE){
        ::vkDestroyDescriptorSetLayout( m_vkDevice, m_vkDescriptorSetLayout, pAllocator );
    }
    if (m_vkCommandPool != VK_NULL_HANDLE){
        ::vkDestroyCommandPool( m_vkDevice, m_vkCommandPool, pAllocator );
    }
    if (m_vkShaderModule != VK_NULL_HANDLE){
        ::vkDestroyShaderModule( m_vkDevice, m_vkShaderModule, pAllocator );
    }
    if (m_vkDevice != VK_NULL_HANDLE){
        ::vkDestroyDevice( m_vkDevice, pAllocator );
    }
    Reset( );
}

}
#endif // VULKAN_SUPPORT
