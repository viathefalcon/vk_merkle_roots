// SHA-256vk.cpp: defines the functions for computing SHA-256 hashes with Vulkan
//

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

// Includes
//

// C++ Standard Headers
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>
#endif

// Local Project Headers
#include "Debug.h"
#include "SHA-256vk.h"

// Functions
//

int vkSha256(const char* arg, size_t size) {
#if defined (VULKAN_SUPPORT)
    using namespace std;

    // Read in the shader bytes
    std::ifstream ifs( "SHA-256.spv", std::ios::binary | std::ios::ate );
    const auto g = ifs.tellg( );
    ifs.seekg( 0 );
    std::vector<uint32_t> code( g / sizeof( uint32_t ) );
    ifs.read( reinterpret_cast<char*>( code.data( ) ), g );
    ifs.close( );

    VkApplicationInfo vkAppInfo = {};
    vkAppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkAppInfo.pApplicationName = "vkMerkleRoots";
    vkAppInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
    vkAppInfo.pEngineName = "n/a";
    vkAppInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0);
    vkAppInfo.apiVersion = VK_API_VERSION_1_3;

    VkValidationFeatureEnableEXT vkValidationFeatureEnableEXT[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
    VkValidationFeaturesEXT vkValidationFeaturesEXT = {};
    vkValidationFeaturesEXT.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    vkValidationFeaturesEXT.enabledValidationFeatureCount = 1;
    vkValidationFeaturesEXT.pEnabledValidationFeatures = vkValidationFeatureEnableEXT;

    const char* szLayerName = "VK_LAYER_KHRONOS_validation";
    const char* pszExtensionNames[] = { "VK_EXT_validation_features", "VK_EXT_debug_utils" };
    VkInstanceCreateInfo vkCreateInfo = {};
    vkCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkCreateInfo.pApplicationInfo = &vkAppInfo;
    vkCreateInfo.enabledLayerCount = 1;
    vkCreateInfo.ppEnabledLayerNames = &szLayerName;
    vkCreateInfo.enabledExtensionCount = 2;
    vkCreateInfo.ppEnabledExtensionNames = pszExtensionNames;
    vkCreateInfo.pNext = &vkValidationFeaturesEXT;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult vkResult = vkCreateInstance( &vkCreateInfo, VK_NULL_HANDLE, &instance );
    if (vkResult == VK_SUCCESS){
        // Count
        uint32_t vkPhysicalDeviceCount = 0;
        vkEnumeratePhysicalDevices( instance, &vkPhysicalDeviceCount, VK_NULL_HANDLE );

        // Retrieve
        VkPhysicalDevice* vkPhysicalDevices = new VkPhysicalDevice[vkPhysicalDeviceCount];
        vkEnumeratePhysicalDevices( instance, &vkPhysicalDeviceCount, vkPhysicalDevices );
        
        // Enumerate
        for (decltype(vkPhysicalDeviceCount) i = 0; i < vkPhysicalDeviceCount; ++i){
            VkPhysicalDevice vkPhysicalDevice = vkPhysicalDevices[i];
            VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
            vkGetPhysicalDeviceProperties( vkPhysicalDevice, &vkPhysicalDeviceProperties );

            std::cout << "Device #" << i << ": " << vkPhysicalDeviceProperties.deviceName << endl;
            std::cout << "maxComputeWorkGroupSize: " << vkPhysicalDeviceProperties.limits.maxComputeSharedMemorySize << endl;

            // Count
            uint32_t vkQueueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, VK_NULL_HANDLE );

            // Retrieve
            VkQueueFamilyProperties* vkQueueFamilyProperties = new VkQueueFamilyProperties[vkQueueFamilyCount];
            vkGetPhysicalDeviceQueueFamilyProperties( vkPhysicalDevice, &vkQueueFamilyCount, vkQueueFamilyProperties );
            
            // Iterate
            uint32_t uQueueFamilyIdx = vkQueueFamilyCount;
            for (decltype(vkQueueFamilyCount) j = 0; j < vkQueueFamilyCount; ++j){
                VkQueueFamilyProperties*  vkQueueFamilyProps = (vkQueueFamilyProperties + j);
                std::cout << "Queue family #" << j << " supports ";
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    std::cout << "graphics ";
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_COMPUTE_BIT){
                    std::cout << "compute ";
                    uQueueFamilyIdx = j;
                }
                if (vkQueueFamilyProps->queueFlags & VK_QUEUE_TRANSFER_BIT){
                    std::cout << "transfer ";
                }
                std::cout << "on " << vkQueueFamilyProps->queueCount << " queue(s)." << endl;
            }
            delete[] vkQueueFamilyProperties;
            std::cout << endl;

            // Look for an early out
            if (uQueueFamilyIdx >= vkQueueFamilyCount){
                cerr << "Failed to find a compute queue; skipping this device." << endl;
                continue;
            }
            std::cout << "Selected queue family #" << uQueueFamilyIdx << endl;

            // Create a logical device w/the selected queue
            const float queuePrioritory = 1.0f;
            VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {};
            vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vkDeviceQueueCreateInfo.queueFamilyIndex = uQueueFamilyIdx;
            vkDeviceQueueCreateInfo.queueCount = 1;
            vkDeviceQueueCreateInfo.pQueuePriorities = &queuePrioritory;
            VkDeviceCreateInfo vkDeviceCreateInfo = {};
            vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            vkDeviceCreateInfo.queueCreateInfoCount = 1;
            vkDeviceCreateInfo.pQueueCreateInfos = &vkDeviceQueueCreateInfo;
            VkDevice vkDevice = VK_NULL_HANDLE;
            vkResult = vkCreateDevice( vkPhysicalDevice, &vkDeviceCreateInfo, VK_NULL_HANDLE, &vkDevice );
            if (vkResult != VK_SUCCESS){
                cerr << "Failed to create logical device on queue #" << uQueueFamilyIdx << endl;
            }

            // Create some buffers
            VkBuffer vkBufferInputs = VK_NULL_HANDLE, vkBufferStarts = VK_NULL_HANDLE, vkBufferSizes = VK_NULL_HANDLE, vkBufferResults = VK_NULL_HANDLE;
            if (vkResult == VK_SUCCESS){
                VkBufferCreateInfo vkBufferCreateInfo = {};
                vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                vkBufferCreateInfo.queueFamilyIndexCount = 1;
                vkBufferCreateInfo.pQueueFamilyIndices = &uQueueFamilyIdx;

                // A buffer for the argument
                uint32_t wc = size / sizeof( uint32_t );
                if ((size % sizeof( uint32_t )) > 0){
                    wc += 1;
                }
                vkBufferCreateInfo.size = wc * sizeof( uint32_t );
                vkResult = vkCreateBuffer( vkDevice, &vkBufferCreateInfo, VK_NULL_HANDLE, &vkBufferInputs );
                if (vkResult == VK_SUCCESS){
                    // A buffer for the starting positions (offsets)
                    vkBufferCreateInfo.size = sizeof( uint32_t );
                    vkResult = vkCreateBuffer( vkDevice, &vkBufferCreateInfo, VK_NULL_HANDLE, &vkBufferStarts );
                }
                if (vkResult == VK_SUCCESS){
                    // A buffer for the input sizes
                    vkBufferCreateInfo.size = sizeof( uint32_t );
                    vkResult = vkCreateBuffer( vkDevice, &vkBufferCreateInfo, VK_NULL_HANDLE, &vkBufferSizes );
                }
                if (vkResult == VK_SUCCESS){
                    // A buffer for the outputs
                    vkBufferCreateInfo.size = sizeof( VkSha256Result );
                    vkResult = vkCreateBuffer( vkDevice, &vkBufferCreateInfo, VK_NULL_HANDLE, &vkBufferResults );
                }
            }
            vector<VkBuffer> buffers = { vkBufferInputs, vkBufferStarts, vkBufferSizes, vkBufferResults };

            // Get the properties of the device's physical memory
            VkPhysicalDeviceMemoryProperties vkPhysicalDeviceMemoryProperties;
            vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &vkPhysicalDeviceMemoryProperties );
            uint32_t uMemoryTypeIdx = vkPhysicalDeviceMemoryProperties.memoryTypeCount;
            if (vkResult == VK_SUCCESS){
                // Get the properties of the memory we want to back our input and output buffers
                VkDeviceSize vkRequiredMemorySize = 0;
                uint32_t uMemoryTypeFilter = 0;
                for (auto it = buffers.begin( ), end = buffers.end( ); it != end; ++it){
                    VkMemoryRequirements vkMemoryRequirements = {};
                    vkGetBufferMemoryRequirements( vkDevice, (*it), &vkMemoryRequirements );

                    // Accumumlate
                    vkRequiredMemorySize += vkMemoryRequirements.size;
                    uMemoryTypeFilter |= vkMemoryRequirements.memoryTypeBits;
                }

                // Scan for a memory to back our input and output buffers
                VkMemoryPropertyFlags vkMemoryPropertyFlags = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                for (decltype(uMemoryTypeIdx) j = 0; j < vkPhysicalDeviceMemoryProperties.memoryTypeCount; ++j){
                    VkMemoryType* vkMemoryType = (vkPhysicalDeviceMemoryProperties.memoryTypes + j);
                    if ((vkMemoryType->propertyFlags & vkMemoryPropertyFlags) != vkMemoryPropertyFlags){
                        continue;
                    }

                    if (vkPhysicalDeviceMemoryProperties.memoryHeaps[vkMemoryType->heapIndex].size < vkRequiredMemorySize){
                        continue;
                    }

                    decltype(uMemoryTypeFilter) uMemoryTypeMask = (1 << j);
                    if ((uMemoryTypeFilter & uMemoryTypeMask) != uMemoryTypeMask){
                        continue;
                    }

                    uMemoryTypeIdx = j;
                    break;
                }
            }
            if (uMemoryTypeIdx < vkPhysicalDeviceMemoryProperties.memoryTypeCount){
                uint32_t* ptr = VK_NULL_HANDLE;

                // Sum up the required memeory
                VkDeviceSize vkRequiredMemorySize = 0;
                for (auto it = buffers.begin( ), end = buffers.end( ); it != end; ++it){
                    VkMemoryRequirements vkMemoryRequirements = {};
                    vkGetBufferMemoryRequirements( vkDevice, (*it), &vkMemoryRequirements );

                    // Accumumlate
                    vkRequiredMemorySize += vkMemoryRequirements.size;
                }

                // Allocate
                VkMemoryAllocateInfo vkMemoryAllocateInfo = {};
                vkMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                vkMemoryAllocateInfo.allocationSize = vkRequiredMemorySize;
                vkMemoryAllocateInfo.memoryTypeIndex = uMemoryTypeIdx;
                VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
                vkResult = vkAllocateMemory( vkDevice, &vkMemoryAllocateInfo, VK_NULL_HANDLE, &vkDeviceMemory );

                // Bind the buffers (backwards for some reason)
                VkDeviceSize vkMemoryOffset = 0U;
                if (vkResult == VK_SUCCESS){
                    vkResult = vkBindBufferMemory( vkDevice, vkBufferResults, vkDeviceMemory, vkMemoryOffset );
                }
                if (vkResult == VK_SUCCESS){
                    vkMemoryOffset += sizeof( VkSha256Result );
                    vkResult = vkBindBufferMemory( vkDevice, vkBufferSizes, vkDeviceMemory, vkMemoryOffset );
                }
                if (vkResult == VK_SUCCESS){
                    vkMemoryOffset += sizeof( uint32_t );
                    vkResult = vkBindBufferMemory( vkDevice, vkBufferStarts, vkDeviceMemory, vkMemoryOffset );
                }
                if (vkResult == VK_SUCCESS){
                    vkMemoryOffset += sizeof( uint32_t );
                    vkResult = vkBindBufferMemory( vkDevice, vkBufferInputs, vkDeviceMemory, vkMemoryOffset );
                }

                // Populate the memory
                uint8_t* pMapped = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    vkResult = vkMapMemory( vkDevice, vkDeviceMemory, 0, vkRequiredMemorySize, 0, reinterpret_cast<void**>( &pMapped ) );
                }
                if (vkResult == VK_SUCCESS){
                    std::memset( pMapped, 0, vkRequiredMemorySize );

                    // Copy the string into the end of the buffer
                    std::memcpy( pMapped + vkMemoryOffset, arg, size );

                    // Then the size
                    vkMemoryOffset -= (2 * sizeof( uint32_t ));
                    const uint32_t u = static_cast<uint32_t>( size );
                    std::memcpy( pMapped + vkMemoryOffset, &u, sizeof( u ) );

                    // Unap
                    vkUnmapMemory( vkDevice, vkDeviceMemory );
                }

                // Create the shader module
                VkShaderModule vkShaderModule = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
                    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    vkShaderModuleCreateInfo.codeSize = code.size( ) * sizeof( uint32_t );
                    vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( code.data( ) );
                    vkResult = vkCreateShaderModule( vkDevice, &vkShaderModuleCreateInfo, VK_NULL_HANDLE, &vkShaderModule );
                }

                // Create the layout of the descriptor set
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
                    VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding4 = vkDescriptorSetLayoutBinding1;
                    vkDescriptorSetLayoutBinding4.binding = 3;
                    VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBindings[] = {
                        vkDescriptorSetLayoutBinding1,
                        vkDescriptorSetLayoutBinding2,
                        vkDescriptorSetLayoutBinding3,
                        vkDescriptorSetLayoutBinding4
                    };
                    VkDescriptorSetLayoutCreateInfo vkDescriptorSetLayoutCreateInfo = {};
                    vkDescriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    vkDescriptorSetLayoutCreateInfo.bindingCount = 4;
                    vkDescriptorSetLayoutCreateInfo.pBindings = vkDescriptorSetLayoutBindings;
                    vkResult = vkCreateDescriptorSetLayout( vkDevice, &vkDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &vkDescriptorSetLayout );
                }

                // Create the pipeline layout
                VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkPipelineLayoutCreateInfo vkPipelineLayoutCreateInfo = {};
                    vkPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                    vkPipelineLayoutCreateInfo.setLayoutCount = 1;
                    vkPipelineLayoutCreateInfo.pSetLayouts = &vkDescriptorSetLayout;
                    vkResult = vkCreatePipelineLayout( vkDevice, &vkPipelineLayoutCreateInfo, VK_NULL_HANDLE, &vkPipelineLayout );
                }

                // Initialise the compute pipeline
                VkPipeline vkPipeline = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
                    vkPipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    vkPipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                    vkPipelineShaderStageCreateInfo.module = vkShaderModule;
                    vkPipelineShaderStageCreateInfo.pName = "main";
                    VkComputePipelineCreateInfo vkComputePipelineCreateInfo = {};
                    vkComputePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                    vkComputePipelineCreateInfo.stage = vkPipelineShaderStageCreateInfo;
                    vkComputePipelineCreateInfo.layout = vkPipelineLayout;
                    vkResult = vkCreateComputePipelines( vkDevice, 0, 1, &vkComputePipelineCreateInfo, VK_NULL_HANDLE, &vkPipeline );
                }

                // Create the descriptor pool
                VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkDescriptorPoolSize vkDescriptorPoolSize = {};
                    vkDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    vkDescriptorPoolSize.descriptorCount = 4;
                    VkDescriptorPoolCreateInfo vkDescriptorPoolCreateInfo = {};
                    vkDescriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                    vkDescriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                    vkDescriptorPoolCreateInfo.maxSets = 1;
                    vkDescriptorPoolCreateInfo.poolSizeCount = 1;
                    vkDescriptorPoolCreateInfo.pPoolSizes = &vkDescriptorPoolSize;
                    vkResult = vkCreateDescriptorPool( vkDevice, &vkDescriptorPoolCreateInfo, VK_NULL_HANDLE, &vkDescriptorPool );
                }

                // Allocate the descriptor set
                VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkDescriptorSetAllocateInfo vkDescriptorSetAllocateInfo = {};
                    vkDescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    vkDescriptorSetAllocateInfo.descriptorPool = vkDescriptorPool;
                    vkDescriptorSetAllocateInfo.descriptorSetCount = 1;
                    vkDescriptorSetAllocateInfo.pSetLayouts = &vkDescriptorSetLayout;
                    vkResult = vkAllocateDescriptorSets( vkDevice, &vkDescriptorSetAllocateInfo, &vkDescriptorSet );
                }

                // Update the descriptor set
                if (vkResult == VK_SUCCESS){
                    VkDescriptorBufferInfo vkDescriptorBufferInputs = {};
                    vkDescriptorBufferInputs.buffer = vkBufferInputs;
                    vkDescriptorBufferInputs.range = VK_WHOLE_SIZE;
                    VkDescriptorBufferInfo vkDescriptorBufferStarts = {};
                    vkDescriptorBufferStarts.buffer = vkBufferStarts;
                    vkDescriptorBufferStarts.range = VK_WHOLE_SIZE;
                    VkDescriptorBufferInfo vkDescriptorBufferSizes = {};
                    vkDescriptorBufferSizes.buffer = vkBufferSizes;
                    vkDescriptorBufferSizes.range = VK_WHOLE_SIZE;
                    VkDescriptorBufferInfo vkDescriptorBufferResults = {};
                    vkDescriptorBufferResults.buffer = vkBufferResults;
                    vkDescriptorBufferResults.range = VK_WHOLE_SIZE;

                    VkWriteDescriptorSet vkWriteDescriptorSetInputs = {};
                    vkWriteDescriptorSetInputs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    vkWriteDescriptorSetInputs.dstSet = vkDescriptorSet;
                    vkWriteDescriptorSetInputs.descriptorCount = 1;
                    vkWriteDescriptorSetInputs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    vkWriteDescriptorSetInputs.pBufferInfo = &vkDescriptorBufferInputs;
                    VkWriteDescriptorSet vkWriteDescriptorSetStarts = {};
                    vkWriteDescriptorSetStarts.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    vkWriteDescriptorSetStarts.dstSet = vkDescriptorSet;
                    vkWriteDescriptorSetStarts.dstBinding = 1;
                    vkWriteDescriptorSetStarts.descriptorCount = 1;
                    vkWriteDescriptorSetStarts.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    vkWriteDescriptorSetStarts.pBufferInfo = &vkDescriptorBufferStarts;
                    VkWriteDescriptorSet vkWriteDescriptorSetSizes = {};
                    vkWriteDescriptorSetSizes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    vkWriteDescriptorSetSizes.dstSet = vkDescriptorSet;
                    vkWriteDescriptorSetSizes.dstBinding = 2;
                    vkWriteDescriptorSetSizes.descriptorCount = 1;
                    vkWriteDescriptorSetSizes.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    vkWriteDescriptorSetSizes.pBufferInfo = &vkDescriptorBufferSizes;
                    VkWriteDescriptorSet vkWriteDescriptorSetResults = {};
                    vkWriteDescriptorSetResults.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    vkWriteDescriptorSetResults.dstSet = vkDescriptorSet;
                    vkWriteDescriptorSetResults.dstBinding = 3;
                    vkWriteDescriptorSetResults.descriptorCount = 1;
                    vkWriteDescriptorSetResults.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    vkWriteDescriptorSetResults.pBufferInfo = &vkDescriptorBufferResults;


                    VkWriteDescriptorSet vkWriteDescriptorSets[] = {
                        vkWriteDescriptorSetInputs,
                        vkWriteDescriptorSetStarts,
                        vkWriteDescriptorSetSizes,
                        vkWriteDescriptorSetResults
                    };
                    vkUpdateDescriptorSets( vkDevice, 4, vkWriteDescriptorSets, 0, VK_NULL_HANDLE );
                }

                // Create the command pool
                VkCommandPool vkCommandPool = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkCommandPoolCreateInfo vkCommandPoolCreateInfo = {};
                    vkCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    vkCommandPoolCreateInfo.queueFamilyIndex = uQueueFamilyIdx;
                    vkResult = vkCreateCommandPool( vkDevice, &vkCommandPoolCreateInfo, VK_NULL_HANDLE, &vkCommandPool );
                }

                // Allocate the command buffer
                VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkCommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
                    vkCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    vkCommandBufferAllocateInfo.commandPool = vkCommandPool;
                    vkCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    vkCommandBufferAllocateInfo.commandBufferCount = 1;
                    vkResult = vkAllocateCommandBuffers( vkDevice, &vkCommandBufferAllocateInfo, &vkCommandBuffer );
                }

                // Start issuing commands
                if (vkResult == VK_SUCCESS){
                    VkCommandBufferBeginInfo vkCommandBufferBeginInfo = {};
                    vkCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    vkCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    vkResult = vkBeginCommandBuffer( vkCommandBuffer, &vkCommandBufferBeginInfo );
                }
                if (vkResult == VK_SUCCESS){
                    vkCmdBindPipeline( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline );
                    vkCmdBindDescriptorSets( vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, VK_NULL_HANDLE );
                    vkCmdDispatch( vkCommandBuffer, 1, 1, 1 );
                    vkResult = vkEndCommandBuffer( vkCommandBuffer );
                }

                // Create a fence
                VkFence vkFence = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    VkFenceCreateInfo vkFenceCreateInfo = {};
                    vkFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    vkResult = vkCreateFence( vkDevice, &vkFenceCreateInfo, VK_NULL_HANDLE, &vkFence );
                }

                // Submit to the queue
                VkQueue vkQueue = VK_NULL_HANDLE;
                if (vkResult == VK_SUCCESS){
                    vkGetDeviceQueue( vkDevice, uQueueFamilyIdx, 0, &vkQueue );
                    if (vkQueue == VK_NULL_HANDLE){
                        cerr << "No queue!" << endl;
                        vkResult = VK_ERROR_UNKNOWN;
                    }
                }
                if (vkResult == VK_SUCCESS){
                    VkSubmitInfo vkSubmitInfo = {};
                    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    vkSubmitInfo.commandBufferCount = 1;
                    vkSubmitInfo.pCommandBuffers = &vkCommandBuffer;
                    vkResult = vkQueueSubmit( vkQueue, 1, &vkSubmitInfo, vkFence );
                }
                if (vkResult == VK_SUCCESS){
                    vkResult = vkWaitForFences( vkDevice, 1, &vkFence, true, uint64_t(-1) );
                }
                if (vkResult == VK_SUCCESS){
                    const VkDeviceSize vkMemorySize = sizeof( VkSha256Result );
                    vkResult = vkMapMemory( vkDevice, vkDeviceMemory, 0, vkMemorySize, 0, reinterpret_cast<void**>( &pMapped ) );
                }
                if (vkResult == VK_SUCCESS){
                    // Copy out the result
                    VkSha256Result result = {};
                    std::memcpy( &result, pMapped, sizeof( VkSha256Result ) );

                    vkUnmapMemory( vkDevice, vkDeviceMemory );

                    std::cout << "Vulkan:- " << arg << " >> " << print_bytes_ex( result.data, SHA256_WC ).str( ) << std::endl;
                }else{
                    cerr << "Failed to retrieve the result" << std::endl;
                }

                // Cleanup
                if (vkFence){
                    vkDestroyFence( vkDevice, vkFence, VK_NULL_HANDLE );
                }
                if (vkCommandBuffer){
                    vkFreeCommandBuffers( vkDevice, vkCommandPool, 1, &vkCommandBuffer );
                }
                if (vkCommandPool){
                    vkDestroyCommandPool( vkDevice, vkCommandPool, VK_NULL_HANDLE );
                }
                if (vkDescriptorSet){
                    vkFreeDescriptorSets( vkDevice, vkDescriptorPool, 1, &vkDescriptorSet );
                }
                if (vkDescriptorPool){
                    vkDestroyDescriptorPool( vkDevice, vkDescriptorPool, VK_NULL_HANDLE );
                }
                if (vkPipeline){
                    vkDestroyPipeline( vkDevice, vkPipeline, VK_NULL_HANDLE );
                }
                if (vkPipelineLayout){
                    vkDestroyPipelineLayout( vkDevice, vkPipelineLayout, VK_NULL_HANDLE );
                }
                if (vkDescriptorSetLayout){
                    vkDestroyDescriptorSetLayout( vkDevice, vkDescriptorSetLayout, VK_NULL_HANDLE );
                }
                if (vkShaderModule){
                    vkDestroyShaderModule( vkDevice, vkShaderModule, VK_NULL_HANDLE );
                }
                if (vkDeviceMemory){
                    vkFreeMemory( vkDevice, vkDeviceMemory, VK_NULL_HANDLE );
                }
            }
            for (auto it = buffers.begin( ), end = buffers.end( ); it != end; ++it){
                auto vkBuffer = (*it);
                if (vkBuffer == VK_NULL_HANDLE){
                    continue;
                }                
                vkDestroyBuffer( vkDevice, vkBuffer, VK_NULL_HANDLE );
            }
            if (vkDevice){
                vkDestroyDevice( vkDevice, VK_NULL_HANDLE );
            }
        }
        delete[] vkPhysicalDevices;

        // Cleanup
        vkDestroyInstance( instance, VK_NULL_HANDLE );
        return 0;
    }

    cerr << "Failed to initialise Vulkan" << endl;
    return 1;
#else
    std::cerr << "Vulkan not supported!" << std::endl;
#endif
}
