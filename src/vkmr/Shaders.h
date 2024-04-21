// Shaders.h: declares the types, functions and classes for working with shaders
//

#ifndef __VKMR_SHADERS_H__
#define __VKMR_SHADERS_H__

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

#if defined (VULKAN_SUPPORT)
// Includes
//

// C++ Standard Library Headers
#include <string>

// Vulkan Headers
#include <vulkan/vulkan.h>

namespace vkmr {

// Class(es)
//

// Encapsulates a shader module
class ShaderModule {
public:
    ShaderModule(VkDevice, const ::std::string&);
    ShaderModule(ShaderModule&&);
    ShaderModule(ShaderModule const&) = delete;
    ShaderModule(void) { Reset( ); }
    ~ShaderModule(void) { Release( ); }

    ShaderModule& operator=(ShaderModule&&);
    ShaderModule& operator=(ShaderModule const&) = delete;
    operator bool() const { return (m_vkShaderModule != VK_NULL_HANDLE); }
    operator VkResult() const { return m_vkResult; }
    VkShaderModule operator * () const { return m_vkShaderModule; }

private:
    void Reset(void);
    void Release(void);

    VkResult m_vkResult;
    VkDevice m_vkDevice;

    VkShaderModule m_vkShaderModule;

    static const VkAllocationCallbacks *pAllocator; 
};

} // namespace vkmr

#endif // VULKAN_SUPPORT
#endif // __VKMR_SHADERS_H__
