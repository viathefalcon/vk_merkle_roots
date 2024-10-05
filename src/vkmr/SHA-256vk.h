// SHA-256vk.h: declares the functions for computing SHA-256 hashes with Vulkan
//

#if !defined (__SHA_256_VK_H__)
#define __SHA_256_VK_H__

// Includes
//

// C++ Standard Library Headers
#include <functional>
#include <unordered_map>

// Local Project Headers
#include "ISha256D.h"
#include "Ops.h"

namespace vkmr {

// Class(es)
//

class IVkSha256DInstance : public ISha256D {
public:
    IVkSha256DInstance(const ISha256D::name_type& name): ISha256D( name ) { }
    virtual ~IVkSha256DInstance() = default;

    bool Reset(void) { return false; }
};

class VkSha256D {
public:
    class Instance;

    VkSha256D();
    ~VkSha256D();

    operator bool() const;
    operator VkResult() const { return m_vkResult; }

    bool Has(const ISha256D::name_type&) const;

    Instance Get(const ISha256D::name_type&);

    ::std::vector<ISha256D::name_type> Available(void) const;

private:
    VkResult m_vkResult;
    VkInstance m_instance;

    ::std::unordered_map<::std::string, ComputeDevice> m_devices;
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

    ISha256D::out_type Root(void);

    bool Add(const ISha256D::arg_type&);

private:
    // Flushes the contents of the buffer into
    // the current batch/slice as appropriate
    bool Flush(void);

    ComputeDevice m_device;
    Slices<VkSha256Result> m_slices;
    Batch m_batch;
    Batches m_batches;

    ::std::unique_ptr<Mappings> m_mappings;
    ::std::unique_ptr<Reductions> m_reductions;
    ::std::vector<arg_type> m_buffer;
};
#endif // defined (VULKAN_SUPPORT)

} // namespace vkmr
