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

    virtual VkFence Map(Batch&, Slice<VkSha256Result>&, VkQueue) = 0;

    static ::std::unique_ptr<Mappings> New(ComputeDevice&);
};

// Encapsulates reductions of slices of device memory to a single value
class Reductions {
public:
    virtual ~Reductions(void) = default;

    virtual VkSha256Result Reduce(VkFence, VkQueue, Slice<VkSha256Result>&, ComputeDevice&) = 0;

    static ::std::unique_ptr<Reductions> New(ComputeDevice&);
};

} // namespace vkmr
#endif // __VKMR_OPS_H__
