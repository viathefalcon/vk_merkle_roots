// Ops.h: declares the types, functions and classes which encapsulate asynchronous on-device operations
//

#ifndef __VKMR_OPS_H__
#define __VKMR_OPS_H__

// Includes
//

// C++ Standard Library Headers
#include <memory>

// Local Project Headers
#include "Slices.h"
#include "Batches.h"

namespace vkmr {
// Classes
//

// Encapsulates mappings of input batches to a slices of device memory
class Mappings {
public:
    virtual ~Mappings(void) = default;

    virtual VkResult Map(Batch&&, Slice<VkSha256Result>&&, VkQueue) = 0;

    // Updates the status of in-flight mappings
    virtual void Update(void) = 0;

    // Synchronously waits for all in-flight mappings to complete
    virtual void WaitFor(void) = 0;

    static ::std::unique_ptr<Mappings> New(ComputeDevice&, uint32_t);
};

// Encapsulates reductions of slices of device memory to a single value
class Reductions {
public:
    typedef Slice<VkSha256Result> slice_type;

    virtual ~Reductions(void) = default;

    // Initiates a new reduction of the given slice of on-device memory
    virtual VkSha256Result Reduce(slice_type&&, ComputeDevice&) = 0;

    static ::std::unique_ptr<Reductions> New(ComputeDevice&, typename slice_type::number_type);
};

} // namespace vkmr
#endif // __VKMR_OPS_H__
