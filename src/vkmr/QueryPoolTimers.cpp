// QueryPoolTimers.cpp: defines the types, functions and classes for working with query pool-based timers.
//

// Includes
//

// Declarations
#include "QueryPoolTimers.h"

static const VkAllocationCallbacks *c_pAllocator = VK_NULL_HANDLE;

namespace vkmr {

// Classes
//
QueryPoolTimer::QueryPoolTimer(void):
    m_vkDevice( VK_NULL_HANDLE ),
    m_vkQueryPool( VK_NULL_HANDLE ),
    m_period( c_defaultPeriod ) { }

QueryPoolTimer::QueryPoolTimer(VkDevice vkDevice, VkQueryPool vkQueryPool, float period):
    m_vkDevice( vkDevice ),
    m_vkQueryPool( vkQueryPool ),
    m_period( period ) { }

QueryPoolTimer::QueryPoolTimer(QueryPoolTimer&& queryPoolTimer):
    m_vkDevice( queryPoolTimer.m_vkDevice ),
    m_vkQueryPool( queryPoolTimer.m_vkQueryPool ),
    m_period( queryPoolTimer.m_period ) {

    queryPoolTimer.Reset( );
}

QueryPoolTimer::~QueryPoolTimer(void) {
    Release( );
}

QueryPoolTimer& QueryPoolTimer::operator=(QueryPoolTimer&& queryPoolTimer) {

    if (this != &queryPoolTimer){
        this->Release( );

        m_vkDevice = queryPoolTimer.m_vkDevice;
        m_vkQueryPool = queryPoolTimer.m_vkQueryPool;
        m_period = queryPoolTimer.m_period;

        queryPoolTimer.Reset( );
    }
    return (*this);
}

void QueryPoolTimer::Start(VkCommandBuffer vkCommandBuffer) {

    if (m_vkQueryPool == VK_NULL_HANDLE){
        // Bail
        return;
    }

    ::vkCmdResetQueryPool( vkCommandBuffer, m_vkQueryPool, 0, 2 );
    ::vkCmdWriteTimestamp( vkCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_vkQueryPool, 0 );
}

void QueryPoolTimer::Finish(VkCommandBuffer vkCommandBuffer) {

    if (m_vkQueryPool == VK_NULL_HANDLE){
        // Bail
        return;
    }
    ::vkCmdWriteTimestamp( vkCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_vkQueryPool, 1 );
}

double QueryPoolTimer::ElapsedMillis(void) const {

    if (m_vkQueryPool != VK_NULL_HANDLE){
        uint64_t timestamps[2];
        VkResult vkResult = ::vkGetQueryPoolResults(
            m_vkDevice,
            m_vkQueryPool,
            0,
            c_queryCount,
            sizeof(timestamps),
            timestamps,
            sizeof( uint64_t ),
            VK_QUERY_RESULT_64_BIT);
        if (vkResult == VK_SUCCESS){
            const auto elapsedTime = timestamps[1] - timestamps[0];
            const auto nanos = elapsedTime * m_period;
            const auto millis = nanos / 1e6;
            return millis;
        }
    }
    return 0;
}

void QueryPoolTimer::Reset(void) {

    m_vkDevice = VK_NULL_HANDLE;
    m_vkQueryPool = VK_NULL_HANDLE;
    m_period = c_defaultPeriod;
}

void QueryPoolTimer::Release(void) {

    if (m_vkQueryPool != VK_NULL_HANDLE){
        ::vkDestroyQueryPool( m_vkDevice, m_vkQueryPool, c_pAllocator );
    }
    Reset( );
}

QueryPoolTimers::QueryPoolTimers(void):
    m_vkDevice( VK_NULL_HANDLE ),
    m_period( QueryPoolTimer::c_defaultPeriod ),
    m_supported( false ) { }

QueryPoolTimers::QueryPoolTimers(ComputeDevice& device):
    m_vkDevice( *device ),
    m_period( QueryPoolTimer::c_defaultPeriod ),
    m_supported( false ) {

    // Query for the device limits
    VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
    vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device.GetPhysicalDeviceProperties2KHR( &vkPhysicalDeviceProperties2 );
    
    // Setup
    m_supported = (vkPhysicalDeviceProperties2.properties.limits.timestampComputeAndGraphics == VK_TRUE);
    m_period = vkPhysicalDeviceProperties2.properties.limits.timestampPeriod;
}

QueryPoolTimer QueryPoolTimers::New(void) {

    if (m_supported){
        // Allocate the query pool
        VkQueryPool vkQueryPool = VK_NULL_HANDLE;
        VkQueryPoolCreateInfo queryPoolCreateInfo = {};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolCreateInfo.queryCount = QueryPoolTimer::c_queryCount;
        VkResult vkResult = ::vkCreateQueryPool( m_vkDevice, &queryPoolCreateInfo, c_pAllocator, &vkQueryPool );
        if (vkResult == VK_SUCCESS){
            // Wrap up and return
            return QueryPoolTimer( m_vkDevice, vkQueryPool, m_period );
        }
    }
    return QueryPoolTimer( ); // return an empty/inert instance
}

} // namespace vkmr
