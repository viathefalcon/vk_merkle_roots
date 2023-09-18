// Batches.h: declares the types, functions and classes that slices memory for inputs and outputs
//

#ifndef __VKMR_BATCHES_H__
#define __VKMR_BATCHES_H__

// Macros
//

#if !defined (_MACOS_64_)
#define VULKAN_SUPPORT
#endif

// Includes
//

#if defined (VULKAN_SUPPORT)
// Vulkan Headers
#include <vulkan/vulkan.h>

// C++ Standard Library Headers
#include <memory>

// Local Project Headers
#include "Devices.h"

namespace vkmr {

// Class(es)
//

class Batch {
public:
    virtual ~Batch() = default;

    // Gives the current/most recent status 
    operator VkResult() const { return m_vkResult; }

    // Return the size of the batch
    virtual VkDeviceSize Size(void) const = 0;

    // Accumulate the given string in the batch
    virtual bool Add(const char*, size_t) = 0;

    // Prepares the batch for reuse
    virtual void Reset(void) = 0;

    // Dispatches the batch on the given queue for processing
    virtual VkResult Dispatch(VkQueue) = 0;

protected:
    VkResult m_vkResult;
};

}

// Function(s)
//

namespace vkmr {
// Creates a new batch, if possible, of the given size
std::unique_ptr<Batch> NewBatch(VkDeviceSize, const ComputeDevice&);
}

#endif // VULKAN_SUPPORT
#endif // __VKMR_BATCHES_H__
