// Devices.cpp: defines the types, functions and classes that encapsulate GPU devices
//

// Includes
//

// C++ Standard Library Headers
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

// Local Project Headers
#include "Debug.h"

// Declarations
#include "Devices.h"

// Externals
//

// Vulkan Extension Function Pointers
extern PFN_vkCmdPipelineBarrier2KHR g_VkCmdPipelineBarrier2KHR;
extern PFN_vkGetPhysicalDeviceProperties2KHR g_pVkGetPhysicalDeviceProperties2KHR;
extern PFN_vkGetPhysicalDeviceMemoryProperties2KHR g_pVkGetPhysicalDeviceMemoryProperties2KHR;

namespace vkmr {

// Classes
//

DescriptorSet::DescriptorSet(VkDevice vkDevice, VkDescriptorPool vkDescriptorPool, VkDescriptorSetLayout vkDescriptorSetLayout):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkDescriptorPool( vkDescriptorPool ),
    m_vkDescriptorSet( VK_NULL_HANDLE ) {
    
    VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
    vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    vkDescriptorSetAllocateInfo.descriptorPool = m_vkDescriptorPool;
    vkDescriptorSetAllocateInfo.descriptorSetCount = 1U;
    vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;
    m_vkResult = ::vkAllocateDescriptorSets( m_vkDevice, &vkDescriptorSetAllocateInfo, &m_vkDescriptorSet );
    if (m_vkResult != VK_SUCCESS){
        Release( );
    }
}

DescriptorSet::DescriptorSet(DescriptorSet&& descriptorSet):
    m_vkResult( descriptorSet.m_vkResult ),
    m_vkDevice( descriptorSet.m_vkDevice ),
    m_vkDescriptorPool( descriptorSet.m_vkDescriptorPool ),
    m_vkDescriptorSet( descriptorSet.m_vkDescriptorSet ) {

    descriptorSet.Reset( );
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& descriptorSet) {

    if (this != &descriptorSet){
        this->Release( );

        m_vkResult = descriptorSet.m_vkResult;
        m_vkDevice = descriptorSet.m_vkDevice;
        m_vkDescriptorPool = descriptorSet.m_vkDescriptorPool;
        m_vkDescriptorSet = descriptorSet.m_vkDescriptorSet;

        descriptorSet.Reset( );
    }
    return (*this);
}

void DescriptorSet::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkDescriptorPool = VK_NULL_HANDLE;
    m_vkDescriptorSet = VK_NULL_HANDLE;
}

void DescriptorSet::Release(void) {

    if (m_vkDescriptorSet != VK_NULL_HANDLE){
        ::vkFreeDescriptorSets( m_vkDevice, m_vkDescriptorPool, 1U, &m_vkDescriptorSet );
    }
    Reset( );
}

CommandBuffer::CommandBuffer(VkDevice vkDevice, VkCommandPool vkCommandPool):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkCommandPool( vkCommandPool ),
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
    m_vkCommandBuffer( commandBuffer.m_vkCommandBuffer ) {

    commandBuffer.Reset( );
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& commandBuffer) {

    if (this != &commandBuffer){
        Release( );

        m_vkResult = commandBuffer.m_vkResult;
        m_vkDevice = commandBuffer.m_vkDevice;
        m_vkCommandPool = commandBuffer.m_vkCommandPool;
        m_vkCommandBuffer = commandBuffer.m_vkCommandBuffer;

        commandBuffer.Reset( );
    }
    return (*this);
}

void CommandBuffer::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkCommandPool = VK_NULL_HANDLE;
    m_vkCommandBuffer = VK_NULL_HANDLE;
}

void CommandBuffer::Release(void) {

    if (m_vkCommandBuffer != VK_NULL_HANDLE){
        ::vkFreeCommandBuffers( m_vkDevice, m_vkCommandPool, 1, &m_vkCommandBuffer );
    }
    Reset( );
}

CommandPool::CommandPool(VkDevice vkDevice, uint32_t queueFamily):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkCommandPool( VK_NULL_HANDLE ) {
    
    VkCommandPoolCreateInfo vkCommandPoolCreateInfo = {};
    vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vkCommandPoolCreateInfo.queueFamilyIndex = queueFamily;
    vkCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    m_vkResult = ::vkCreateCommandPool( m_vkDevice, &vkCommandPoolCreateInfo, VK_NULL_HANDLE, &m_vkCommandPool );
}

CommandPool::CommandPool(CommandPool&& commandPool):
    m_vkResult( commandPool.m_vkResult ),
    m_vkDevice( commandPool.m_vkDevice ),
    m_vkCommandPool( commandPool.m_vkCommandPool ) {

    commandPool.Reset( );
}

CommandPool& CommandPool::operator=(CommandPool&& commandPool) {

    if (this != &commandPool){
        Release( );

        m_vkResult = commandPool.m_vkResult;
        m_vkDevice = commandPool.m_vkDevice;
        m_vkCommandPool = commandPool.m_vkCommandPool;

        commandPool.Reset( );
    }
    return (*this);
}

CommandBuffer CommandPool::AllocateCommandBuffer(void) {
    return CommandBuffer( m_vkDevice, m_vkCommandPool );
}

void CommandPool::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkCommandPool = VK_NULL_HANDLE;
}

void CommandPool::Release(void) {

    if (m_vkCommandPool != VK_NULL_HANDLE){
        ::vkDestroyCommandPool( m_vkDevice, m_vkCommandPool, VK_NULL_HANDLE );
    }
    Reset( );
}

uint32_t WorkgroupSize::GetGroupCountX(uint32_t workItemCount) const {

    auto groupCount = (workItemCount / x);
    return ((workItemCount % x) == 0)
        ? groupCount
        : (groupCount + 1);
}

Pipeline::Pipeline(
    VkDevice vkDevice,
    VkDescriptorSetLayout vkDescriptorSetLayout,
    VkPipelineLayout vkPipelineLayout,
    ShaderModule&& shaderModule,
    const WorkgroupSize* pWorkGroupSize):
    m_vkDevice( vkDevice ),
    m_vkDescriptorSetLayout( vkDescriptorSetLayout ),
    m_vkPipelineLayout( vkPipelineLayout ),
    m_vkPipeline( VK_NULL_HANDLE ),
    m_shaderModule( ::std::move( shaderModule ) ) {

    // Check the pipeline layout
    if (m_vkPipelineLayout == VK_NULL_HANDLE){
        // Bail
        Release( );
        return;
    }

    // Initialise the compute pipeline
    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
    vkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vkPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    vkPipelineShaderStageCreateInfo.module = *m_shaderModule;
    vkPipelineShaderStageCreateInfo.pName = "main";

    // Capture the work group size, if provided
    VkSpecializationInfo specializationInfo = {};
    if (pWorkGroupSize == nullptr){
        m_workGroupSize.x = m_workGroupSize.y = m_workGroupSize.z = 1U;
    }else{
        m_workGroupSize = (*pWorkGroupSize);

        // Convert the workgroup size into a specialisation for the shader
        uint32_t workgroupSize[3] = { pWorkGroupSize->x, pWorkGroupSize->y, pWorkGroupSize->z };
        VkSpecializationMapEntry vkSpecializationMapEntries[3] = {};
        vkSpecializationMapEntries[0].constantID = 0;
        vkSpecializationMapEntries[0].offset = 0;
        vkSpecializationMapEntries[0].size = sizeof(uint32_t);

        vkSpecializationMapEntries[1].constantID = 1;
        vkSpecializationMapEntries[1].offset = vkSpecializationMapEntries[0].offset + sizeof(uint32_t);
        vkSpecializationMapEntries[1].size = sizeof(uint32_t);

        vkSpecializationMapEntries[2].constantID = 2;
        vkSpecializationMapEntries[2].offset = vkSpecializationMapEntries[1].offset + sizeof(uint32_t);
        vkSpecializationMapEntries[2].size = sizeof(uint32_t);
        specializationInfo.mapEntryCount = 3;
        specializationInfo.pMapEntries = vkSpecializationMapEntries;
        specializationInfo.dataSize = sizeof( workgroupSize );
        specializationInfo.pData = workgroupSize;

        // Apply to the pipeline
        vkPipelineShaderStageCreateInfo.pSpecializationInfo = &specializationInfo;
    }

    // Set the required workgroup size, if specified
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo requiredSubgroupSizeCreateInfo = {};
    const auto subgroupsRequired = (pWorkGroupSize == nullptr) ? false : pWorkGroupSize->bySubgroup;
    if (subgroupsRequired){
        requiredSubgroupSizeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO;
        requiredSubgroupSizeCreateInfo.requiredSubgroupSize = (pWorkGroupSize->x * pWorkGroupSize->y *pWorkGroupSize->z);
        vkPipelineShaderStageCreateInfo.pNext = &requiredSubgroupSizeCreateInfo;
    }

    // Create the compute pipeline
    VkComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
    vkComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    vkComputePipelineCreateInfo.stage = vkPipelineShaderStageCreateInfo;
    vkComputePipelineCreateInfo.layout = m_vkPipelineLayout;
    VkResult vkResult = ::vkCreateComputePipelines( m_vkDevice, 0, 1, &vkComputePipelineCreateInfo, VK_NULL_HANDLE, &m_vkPipeline );
    if (vkResult != VK_SUCCESS){
        Release( );
        return;
    }
}

Pipeline::Pipeline(Pipeline&& pipeline):
    m_vkDevice( pipeline.m_vkDevice ),
    m_vkDescriptorSetLayout( pipeline.m_vkDescriptorSetLayout ),
    m_vkPipelineLayout( pipeline.m_vkPipelineLayout ),
    m_vkPipeline( pipeline.m_vkPipeline ),
    m_shaderModule( ::std::move( pipeline.m_shaderModule ) ),
    m_workGroupSize( pipeline.m_workGroupSize ) {

    pipeline.Reset( );
}

Pipeline& Pipeline::operator=(Pipeline&& pipeline) {

    if (this != &pipeline){
        this->Release( );

        m_vkDevice = pipeline.m_vkDevice;
        m_vkDescriptorSetLayout = pipeline.m_vkDescriptorSetLayout;
        m_vkPipelineLayout = pipeline.m_vkPipelineLayout;
        m_vkPipeline = pipeline.m_vkPipeline;
        m_shaderModule = ::std::move( pipeline.m_shaderModule );
        m_workGroupSize = pipeline.m_workGroupSize;

        pipeline.Reset( );
    }
    return (*this);
}

VkPipelineLayout Pipeline::NewSimpleLayout(VkDevice vkDevice, VkDescriptorSetLayout vkDescriptorSetLayout, const VkPushConstantRange* pVkPushConstantRange) {

    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
    vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vkPipelineLayoutCreateInfo.setLayoutCount = 1;
    vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
    if (pVkPushConstantRange){
        vkPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        vkPipelineLayoutCreateInfo.pPushConstantRanges = pVkPushConstantRange;
    }
    VkResult vkResult = ::vkCreatePipelineLayout( vkDevice, &vkPipelineLayoutCreateInfo, VK_NULL_HANDLE, &vkPipelineLayout );
    return (vkResult == VK_SUCCESS) ? vkPipelineLayout : VK_NULL_HANDLE;
}

void Pipeline::Reset(void) {

    m_vkDevice = VK_NULL_HANDLE;
    m_vkDescriptorSetLayout = VK_NULL_HANDLE;
    m_vkPipelineLayout = VK_NULL_HANDLE;
    m_vkPipeline = VK_NULL_HANDLE;
    ::std::memset( &m_workGroupSize, 0, sizeof( WorkgroupSize ) );
}

void Pipeline::Release(void) {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkPipeline != VK_NULL_HANDLE){
        ::vkDestroyPipeline( m_vkDevice, m_vkPipeline, pAllocator );
    }
    if (m_vkPipelineLayout != VK_NULL_HANDLE){
        ::vkDestroyPipelineLayout( m_vkDevice, m_vkPipelineLayout, pAllocator );
    }
    if (m_vkDescriptorSetLayout != VK_NULL_HANDLE){
        ::vkDestroyDescriptorSetLayout( m_vkDevice, m_vkDescriptorSetLayout, pAllocator );
    }
    m_shaderModule = ShaderModule( );
    Reset( );
}

DescriptorPool::DescriptorPool(VkDevice vkDevice, uint32_t setCount, uint32_t descriptorCount):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkDescriptorPool( VK_NULL_HANDLE ) {
    
    VkDescriptorPoolSize vkDescriptorPoolSize = {};
    vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vkDescriptorPoolSize.descriptorCount = descriptorCount; // This is the total number, *across* descriptor sets allocated from the pool
    VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
    vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    vkDescriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    vkDescriptorPoolCreateInfo.maxSets = setCount;
    vkDescriptorPoolCreateInfo.poolSizeCount = 1;
    vkDescriptorPoolCreateInfo.pPoolSizes = &vkDescriptorPoolSize;
    m_vkResult = ::vkCreateDescriptorPool( m_vkDevice, &vkDescriptorPoolCreateInfo, VK_NULL_HANDLE, &m_vkDescriptorPool );
}

DescriptorPool::DescriptorPool(DescriptorPool&& descriptorPool):
    m_vkResult( descriptorPool.m_vkResult ),
    m_vkDevice( descriptorPool.m_vkDevice ),
    m_vkDescriptorPool( descriptorPool.m_vkDescriptorPool ) {

    descriptorPool.Reset( );
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& descriptorPool) {

    if (this != &descriptorPool){
        this->Release( );

        m_vkResult = descriptorPool.m_vkResult;
        m_vkDevice = descriptorPool.m_vkDevice;
        m_vkDescriptorPool = descriptorPool.m_vkDescriptorPool;
        m_vkDescriptorPool = descriptorPool.m_vkDescriptorPool;

        descriptorPool.Reset( );
    }
    return (*this);
}

DescriptorSet DescriptorPool::AllocateDescriptorSet(Pipeline& p) const {
    return DescriptorSet( m_vkDevice, m_vkDescriptorPool, p.DescriptorSetLayout( ) );   
}

void DescriptorPool::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkDescriptorPool = VK_NULL_HANDLE;
}

void DescriptorPool::Release(void) {

    if (m_vkDescriptorPool != VK_NULL_HANDLE){
        ::vkDestroyDescriptorPool( m_vkDevice, m_vkDescriptorPool, VK_NULL_HANDLE );
    }
    Reset( );
}

ComputeDevice::ComputeDevice(VkPhysicalDevice vkPhysicalDevice, uint32_t queueFamily, uint32_t queueCount):
    m_vkPhysicalDevice( vkPhysicalDevice ),
    m_queueFamily( queueFamily ),
    m_queueCount( queueCount ),
    m_queueNext( 0U ),
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( VK_NULL_HANDLE ) {

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
            decltype(deviceExtNames) requestedExtNames = {
                (char*) VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                (char*) VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                (char*) VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME
            };
            vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, VK_NULL_HANDLE, &uDeviceExtensionPropertyCount, pVkExtensionProperties );
            for (decltype(uDeviceExtensionPropertyCount) u = 0; (!requestedExtNames.empty( )) && (u < uDeviceExtensionPropertyCount); ++u){
                VkExtensionProperties* vkExtensionProperties = (pVkExtensionProperties + u);
                const char* extensionName = vkExtensionProperties->extensionName;

                const auto end = requestedExtNames.end( );
                auto it = ::std::find_if( requestedExtNames.begin( ), end, [=](const decltype(deviceExtNames)::value_type& value) {
                    return (strcmp( value, extensionName ) == 0);
                } );
                if (it == end){
                    continue;
                }
                deviceExtNames.push_back( *it );
                requestedExtNames.erase( it );
            }
            delete[] pVkExtensionProperties;
        }
    }
    cout << "Going to create a device with these extensions: ";
    for (auto it = deviceExtNames.cbegin( ), end = deviceExtNames.cend( ); it != end; ++it){
        cout << (*it) << " ";
    }
    cout << endl;
    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

    // Enable the synchronization2 feature
    VkPhysicalDeviceSynchronization2Features vkPhysicalDeviceSynchronization2Features = {};
    vkPhysicalDeviceSynchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    vkPhysicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;

    // Enable the subgroup size control features
    VkPhysicalDeviceSubgroupSizeControlFeatures subgroupSizeControlFeatures = {};
    subgroupSizeControlFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
    subgroupSizeControlFeatures.subgroupSizeControl = VK_TRUE;
    subgroupSizeControlFeatures.pNext = &vkPhysicalDeviceSynchronization2Features;

    // Try and create the device along w/all the queues..
    vector<float> queuePriorities(queueCount, 1.0f);
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {};
    vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vkDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
    vkDeviceQueueCreateInfo.queueCount = queuePriorities.size( );
    vkDeviceQueueCreateInfo.pQueuePriorities = queuePriorities.data( );
    VkDeviceCreateInfo vkDeviceCreateInfo = {};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.pNext = &subgroupSizeControlFeatures;
    vkDeviceCreateInfo.queueCreateInfoCount = 1;
    vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;
    if (!deviceExtNames.empty( )){
        vkDeviceCreateInfo.enabledExtensionCount = deviceExtNames.size( );
        vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtNames.data( );
    }
    m_vkResult = ::vkCreateDevice( vkPhysicalDevice, &vkDeviceCreateInfo, pAllocator, &m_vkDevice );
}

ComputeDevice::ComputeDevice(ComputeDevice&& device):
    m_vkPhysicalDevice( device.m_vkPhysicalDevice ),
    m_queueFamily( device.m_queueFamily ),
    m_queueCount( device.m_queueCount ),
    m_queueNext( device.m_queueNext ),
    m_vkResult( device.m_vkResult ),
    m_vkDevice( device.m_vkDevice) {

    device.Reset( );
}

ComputeDevice& ComputeDevice::operator=(ComputeDevice&& device) {

    if (this != &device){
        this->Release( );

        m_vkPhysicalDevice = device.m_vkPhysicalDevice;
        m_queueFamily = device.m_queueFamily;
        m_queueCount = device.m_queueCount;
        m_queueNext = device.m_queueNext;
        m_vkResult = device.m_vkResult;
        m_vkDevice = device.m_vkDevice;

        device.Reset( );
    }
    return (*this);
}

DescriptorPool ComputeDevice::CreateDescriptorPool(uint32_t setCount, uint32_t descriptorCount) const {
    return DescriptorPool( m_vkDevice, setCount, descriptorCount );
}

CommandPool ComputeDevice::CreateCommandPool(void) const {
    return CommandPool( m_vkDevice, m_queueFamily );
}

VkQueue ComputeDevice::Queue(void) {

    VkQueue vkQueue = VK_NULL_HANDLE;
    ::vkGetDeviceQueue( m_vkDevice, m_queueFamily, m_queueNext, &vkQueue );
    if (vkQueue == VK_NULL_HANDLE){
        ::std::cerr << "Failed to retrieve handle to device queue!" << ::std::endl;
    }else{
        m_queueNext += 1U;
        if (m_queueNext >= m_queueCount){
            m_queueNext = 0U;
        }
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

void ComputeDevice::GetPhysicalDeviceProperties2KHR(VkPhysicalDeviceProperties2KHR* pVkPhysicalDeviceProperties2) const {
    g_pVkGetPhysicalDeviceProperties2KHR( m_vkPhysicalDevice, pVkPhysicalDeviceProperties2 );
}

VkDeviceSize ComputeDevice::MinStorageBufferOffset(void) const {

    VkPhysicalDeviceProperties properties;
    ::vkGetPhysicalDeviceProperties( m_vkPhysicalDevice, &properties );
    return properties.limits.minStorageBufferOffsetAlignment;
}

ComputeDevice::MemoryTypeBudgets ComputeDevice::AvailableMemoryTypes(
    const VkMemoryRequirements& vkMemoryRequirements,
    VkMemoryPropertyFlags vkMemoryPropertyFlags) const {

    // Query
    VkPhysicalDeviceMemoryProperties2 vkPhysicalDeviceMemoryProperties2 = {};
    VkPhysicalDeviceMemoryBudgetPropertiesEXT vkPhysicalDeviceMemoryBudgetProperties = {};
    if (g_pVkGetPhysicalDeviceMemoryProperties2KHR){
        vkPhysicalDeviceMemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

        vkPhysicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkPhysicalDeviceMemoryProperties2.pNext = &vkPhysicalDeviceMemoryBudgetProperties;
        g_pVkGetPhysicalDeviceMemoryProperties2KHR( m_vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties2 );
    }else{
        // Fall back
        vkGetPhysicalDeviceMemoryProperties( m_vkPhysicalDevice, &(vkPhysicalDeviceMemoryProperties2.memoryProperties) );
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
        MemoryTypeBudget memoryTypeBudget = {
            vkMemoryType->heapIndex,
            index,
            budget,
            pvkMemoryHeap->size,
            vkMemoryType->propertyFlags
        };
        memoryTypeBudgets.push_back( memoryTypeBudget );
    }

    // Sort by budget descending and return
    ::std::sort( memoryTypeBudgets.begin( ), memoryTypeBudgets.end( ), [](const MemoryTypeBudget& lhs, const MemoryTypeBudget& rhs) {
        return lhs.vkMemoryBudget > rhs.vkMemoryBudget;
    } );
    return memoryTypeBudgets;
}

VkDeviceMemory ComputeDevice::Allocate(const MemoryTypeBudget& deviceMemoryBudget, VkDeviceSize vkSize) {

    VkMemoryAllocateInfo vkDeviceMemoryAllocateInfo = {};
    vkDeviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vkDeviceMemoryAllocateInfo.allocationSize = vkSize;
    vkDeviceMemoryAllocateInfo.memoryTypeIndex = deviceMemoryBudget.memoryTypeIndex;
    VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
    VkResult vkResult = ::vkAllocateMemory( m_vkDevice, &vkDeviceMemoryAllocateInfo, VK_NULL_HANDLE, &vkDeviceMemory );
    if (vkResult == VK_SUCCESS){
        ::std::cout << "Allocated " << int64_t(vkSize) << " bytes of memory type " << vkDeviceMemoryAllocateInfo.memoryTypeIndex << ::std::endl;

        // TODO, maybe: keep track of allocations? But, would that ever be useful?
        return vkDeviceMemory;
    }
    return VK_NULL_HANDLE;
}

void ComputeDevice::Free(VkDeviceMemory vkDeviceMemory) const {

    if (vkDeviceMemory != VK_NULL_HANDLE){
        ::vkFreeMemory( m_vkDevice, vkDeviceMemory, VK_NULL_HANDLE );
    }
}

void ComputeDevice::Reset() {

    m_vkPhysicalDevice = VK_NULL_HANDLE;
    m_queueCount = m_queueNext = 0U;
    m_queueFamily = uint32_t(-1);
    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
}

void ComputeDevice::Release() {

    const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
    if (m_vkDevice != VK_NULL_HANDLE){
        ::vkDestroyDevice( m_vkDevice, pAllocator );
    }
    Reset( );
}

}
