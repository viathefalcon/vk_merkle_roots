// SHA-256vk.h: declares the functions for computing SHA-256 hashes with Vulkan
//

#if !defined (__SHA_256_VK_H__)
#define __SHA_256_VK_H__

// Includes
//

// C++ Standard Library Headers
#include <functional>

// Local Project Headers
#include "Ops.h"

namespace vkmr {

// Class(es)
//

class IVkSha256DInstance {
public:
    typedef ::std::string arg_type;
    typedef ::std::string out_type;
    typedef ::std::string name_type;

    IVkSha256DInstance(const name_type& name): m_name( name ) { }
    virtual ~IVkSha256DInstance() = default;

    virtual out_type Root(void) = 0;
    virtual bool Add(const arg_type&) = 0;
    const name_type& Name(void) const { return m_name; }

protected:
    name_type m_name;
};

#if defined (VULKAN_SUPPORT)
class VkSha256D {
public:
    class Instance;

    VkSha256D(const ::std::string&);
    ~VkSha256D();

    operator bool() const;
    operator VkResult() const { return m_vkResult; }

    void ForEach(::std::function<void(Instance&)>);

private:
    VkResult m_vkResult;
    VkInstance m_instance;

    ::std::vector<Instance> m_instances;
};

class VkSha256D::Instance: public IVkSha256DInstance {
public:
    typedef ::std::string arg_type;
    typedef ::std::string out_type;

    Instance(const ::std::string&, ComputeDevice&&);
    Instance(Instance&&);
    Instance(Instance const&) = delete;
    virtual ~Instance();

    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&);

    out_type Root(void);

    bool Add(const arg_type&);
    void Cap(size_t);

private:
    typedef Slice<VkSha256Result> slice_type;

    ComputeDevice m_device;
    Batch m_batch;
    slice_type m_slice;
    ::std::unique_ptr<Mappings> m_mappings;
    ::std::unique_ptr<Reductions> m_reductions;
};
#endif // defined (VULKAN_SUPPORT)

} // namespace vkmr

#endif // __SHA_256_VK_H__
