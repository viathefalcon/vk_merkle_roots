// Shaders.cpp: defines the types, functions and classes for working with shaders
//

// Includes
//

// C++ Standard Library Headers
#include <iostream>
#include <fstream>
#include <vector>

// Declarations
#include "Shaders.h"

#if defined (VULKAN_SUPPORT)

namespace vkmr {

const VkAllocationCallbacks* ShaderModule::pAllocator = VK_NULL_HANDLE;

ShaderModule::ShaderModule(VkDevice vkDevice, const ::std::string& path):
    m_vkResult( VK_RESULT_MAX_ENUM ),
    m_vkDevice( vkDevice ),
    m_vkShaderModule( VK_NULL_HANDLE ) {

    ::std::ifstream ifs( path, ::std::ios::binary | ::std::ios::ate );
    const auto g = ifs.tellg( );
    ifs.seekg( 0 );
    std::vector<uint32_t> shaderCode( g / sizeof( uint32_t ) );
    ifs.read( reinterpret_cast<char*>( shaderCode.data( ) ), g );
    ifs.close( );
    ::std::cout << "Loaded " << shaderCode.size() << " (32-bit) word(s) of shader code from " << path << ::std::endl;

    // Create the shader module
    VkShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
    vkShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderModuleCreateInfo.codeSize = shaderCode.size( ) * sizeof( uint32_t );
    vkShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>( shaderCode.data( ) );
    m_vkResult = ::vkCreateShaderModule( vkDevice, &vkShaderModuleCreateInfo, pAllocator, &m_vkShaderModule );
}

ShaderModule::ShaderModule(ShaderModule&& shaderModule):
    m_vkResult( shaderModule.m_vkResult ),
    m_vkDevice( shaderModule.m_vkDevice ),
    m_vkShaderModule( shaderModule.m_vkShaderModule ) {

    shaderModule.Reset( );
}

ShaderModule& ShaderModule::operator=(ShaderModule&& shaderModule) {

    if (this != &shaderModule){
        Release( );

        m_vkResult = shaderModule.m_vkResult;
        m_vkDevice = shaderModule.m_vkDevice;
        m_vkShaderModule = shaderModule.m_vkShaderModule;

        shaderModule.Reset( );
    }
    return (*this);
}

void ShaderModule::Reset(void) {

    m_vkResult = VK_RESULT_MAX_ENUM;
    m_vkDevice = VK_NULL_HANDLE;
    m_vkShaderModule = VK_NULL_HANDLE;
}

void ShaderModule::Release(void) {

    if (m_vkShaderModule != VK_NULL_HANDLE){
        ::vkDestroyShaderModule( m_vkDevice, m_vkShaderModule, pAllocator );
    }
    Reset( );
}

}
#endif // VULKAN_SUPPORT
