// QueryPoolTimers.h: declares the types, functions and classes for working with query pool-based timers.
//

#ifndef __VKMR_QUERY_POOL_TIMERS_H__
#define __VKMR_QUERY_POOL_TIMERS_H__

// Includes
//

// Vulkan Headers
#include <vulkan/vulkan.h>

// Local Project Headers
#include "Devices.h"

namespace vkmr {

// Forward Declarations
//

class QueryPoolTimers;

// Class(es)
//

// Implements a basic, one-shot timer based on a Vulkan query pool
class QueryPoolTimer {
    friend class QueryPoolTimers;

public:
    QueryPoolTimer(void);
    QueryPoolTimer(QueryPoolTimer&&);
    QueryPoolTimer(QueryPoolTimer const&) = delete;
    QueryPoolTimer(VkDevice, VkQueryPool, float);
    ~QueryPoolTimer(void);

    QueryPoolTimer& operator=(QueryPoolTimer const&) = delete;
    QueryPoolTimer& operator=(QueryPoolTimer&&);

    operator bool() const { return (m_vkQueryPool != VK_NULL_HANDLE); }

    void Start(VkCommandBuffer);
    void Finish(VkCommandBuffer);
    double ElapsedMillis(void) const;

private:
    void Reset();
    void Release();

    double m_period;
    VkDevice m_vkDevice;
    VkQueryPool m_vkQueryPool;

    static const int c_defaultPeriod = 1;
    static const uint32_t c_queryCount = 2; // Two timestamps: start and end
};

class QueryPoolTimers {
public:
    QueryPoolTimers(void);
    QueryPoolTimers(ComputeDevice&);
    ~QueryPoolTimers() = default;

    QueryPoolTimer New();

private:
    double m_period;
    bool m_supported;

    VkDevice m_vkDevice;
};

} // namespace vkmr

#endif // __VKMR_QUERY_POOL_TIMERS_H__
