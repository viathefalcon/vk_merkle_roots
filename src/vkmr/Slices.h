// Slices.h: declares the types, functions and classes for operating on slices of on-device GPU memory
//

#ifndef __VKMR_SLICES_H__
#define __VKMR_SLICES_H__

// Includes
//

// Local Project Headers
#include "Devices.h"

#if defined (VULKAN_SUPPORT)
namespace vkmr {

// Templates
//

// Encapsulates a slice of on-device GPU memory
template <typename T>
class Slice {
public:
    typedef T value_type;

    Slice(void) { Reset( ); }
    Slice(Slice&& slice):
        m_vkDevice( slice.m_vkDevice ),
        m_vkBuffer( slice.m_vkBuffer ),
        m_vkDeviceMemory( slice.m_vkDeviceMemory ),
        m_vkSize( slice.m_vkSize ),
        m_sliced( slice.m_sliced ),
        m_reserved( slice.m_reserved ),
        m_capacity( slice.m_capacity ) {

        slice.Reset( );
    }
    Slice(Slice&) = delete;

    ~Slice(void) { Release( ); }

    Slice& operator=(Slice&& slice) {

        if (this != &slice){
            Release( );

            m_vkDevice = slice.m_vkDevice;
            m_vkBuffer = slice.m_vkBuffer;
            m_vkDeviceMemory = slice.m_vkDeviceMemory;
            m_vkSize = slice.m_vkSize;
            m_sliced = slice.m_sliced;
            m_reserved = slice.m_reserved;
            m_capacity = slice.m_capacity;

            slice.Reset( );
        }
        return (*this);
    }

    Slice& operator=(Slice const&) = delete;
    operator bool() const { return (m_vkDeviceMemory != VK_NULL_HANDLE); }

    // Returns the underlying buffer
    VkBuffer Buffer(void) const {
        return m_vkBuffer;
    }

    // Returns the size, in bytes, of the memory slice
    VkDeviceSize Size() const {
        return m_vkSize;
    }

    // Reserves a, er, slot (?) within the slice
    bool Reserve(void) {
        if (m_capacity > (m_sliced + m_reserved)){
            m_reserved += 1U;
            return true;
        }
        return false;
    }

    // Return the number of reservations in the slice
    size_t Reserved(void) const { return m_reserved; }

    // Gets the sub slice encompassing the reservations since the last 
    // sub slice, if any
    Slice Get(void) {

        // Start with an empty slice
        Slice slice;
        if (m_reserved > 0U){
            const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;

            // Create the buffer
            VkBuffer vkBuffer = VK_NULL_HANDLE;
            VkBufferCreateInfo vkBufferCreateInfo = {};
            vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vkBufferCreateInfo.size = (m_reserved * sizeof( T ));
            VkResult vkResult = ::vkCreateBuffer(
                m_vkDevice,
                &vkBufferCreateInfo,
                pAllocator,
                &vkBuffer
            );
            if (vkResult == VK_SUCCESS){
                // Bind it to the memory
                vkResult = ::vkBindBufferMemory(
                    m_vkDevice,
                    vkBuffer,
                    m_vkDeviceMemory,
                    (m_sliced * sizeof( T ))
                );
            }else{
                ::vkDestroyBuffer( m_vkDevice, vkBuffer, pAllocator );
            }
            if (vkResult == VK_SUCCESS){
                slice = Slice( m_vkDevice, vkBuffer, VK_NULL_HANDLE, vkBufferCreateInfo.size );

                // Update our own internal state
                m_sliced += m_reserved;
                m_reserved = 0U;
            }
        }
        return slice;
    }

    VkDescriptorBufferInfo BufferDescriptor(void) const {

        VkDescriptorBufferInfo vkDescriptorBufferInfo = {};
        vkDescriptorBufferInfo.buffer = m_vkBuffer;
        vkDescriptorBufferInfo.offset = 0U;
        vkDescriptorBufferInfo.range = VK_WHOLE_SIZE;
        return vkDescriptorBufferInfo;
    }

    // Allocates and returns a new slice of on-device memory from the
    // given device (GPU) which can hold some whole number of elements
    // up to a given maximum
    static Slice New(ComputeDevice& device, VkDeviceSize vkMaxSize) {

        // Be pessimistic - start w/an empty slice
        Slice slice;

        // Query for the maximum number of concurrent invocations
        // and the per-device allocation limit
        VkPhysicalDeviceMaintenance3PropertiesKHR vkPhysicalDeviceMaintenance3Properties = {};
        vkPhysicalDeviceMaintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
        VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
        vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceMaintenance3Properties;
        g_pVkGetPhysicalDeviceProperties2KHR( device.PhysicalDevice( ), &vkPhysicalDeviceProperties2 );

        // The size of a slice is the smaller of:
        // 1. the most we can use (in one whack)
        // 2. the maximum allowed allocation size, provided to us by the caller
        // 3. the available on-device memory
        const VkPhysicalDeviceProperties& vkPhysicalDeviceProperties = vkPhysicalDeviceProperties2.properties;
        auto vkSliceSize = ::std::min(
            sizeof( T ) * vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0] * vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0],
            vkMaxSize
        );
        if (vkPhysicalDeviceMaintenance3Properties.maxMemoryAllocationSize > 0U){
            vkSliceSize = ::std::min( vkSliceSize, vkPhysicalDeviceMaintenance3Properties.maxMemoryAllocationSize );
        }
        vkSliceSize = vkSliceSize - (vkSliceSize % sizeof( T ));

        // Look for corresponding on-device memory
        const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( vkSliceSize );
        auto deviceMemoryBudgets = device.AvailableMemoryTypes(
            vkMemoryRequirements,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        // Iterate and try to allocate
        for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); it != end; it++){
            const auto& deviceMemoryBudget = *it;
            if (deviceMemoryBudget.vkMemoryBudget < vkSliceSize){
                // Nah, not interested
                continue;
            }

            auto vkDeviceMemory = device.Allocate( deviceMemoryBudget, vkSliceSize );
            if (vkDeviceMemory == VK_NULL_HANDLE){
                // Keep going
                continue;
            }

            const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
            auto vkDevice = *device;

            // Create the buffer
            VkBuffer vkBuffer = VK_NULL_HANDLE;
            VkBufferCreateInfo vkBufferCreateInfo = {};
            vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkBufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            vkBufferCreateInfo.size = vkSliceSize;
            VkResult vkResult = ::vkCreateBuffer(
                vkDevice,
                &vkBufferCreateInfo,
                pAllocator,
                &vkBuffer
            );
            if (vkResult == VK_SUCCESS){
                // Bind it to the memory
                vkResult = ::vkBindBufferMemory( vkDevice, vkBuffer, vkDeviceMemory, 0U );
            }else{
                ::vkDestroyBuffer( vkDevice, vkBuffer, pAllocator );
            }
            if (vkResult == VK_SUCCESS){
                // Bingo
                slice = Slice( vkDevice, vkBuffer, vkDeviceMemory, vkSliceSize );
            }else{
                device.Free( vkDeviceMemory );
            }

            // Can stop here, regardless
            break;
        }
        return slice;
    }

private:
    Slice(VkDevice vkDevice, VkBuffer vkBuffer, VkDeviceMemory vkDeviceMemory, VkDeviceSize vkSize):
        m_vkDevice( vkDevice ),
        m_vkBuffer( vkBuffer ),
        m_vkDeviceMemory( vkDeviceMemory ),
        m_vkSize( vkSize ),
        m_sliced( 0U ),
        m_reserved( 0U ),
        m_capacity( vkSize / sizeof( T ) ) {
    }

    void Reset(void) {

        m_vkDevice = VK_NULL_HANDLE;
        m_vkBuffer = VK_NULL_HANDLE;
        m_vkDeviceMemory = VK_NULL_HANDLE;
        m_vkSize = 0U;
        m_sliced = m_reserved = m_capacity = 0U;
    }

    void Release(void) {

        if (m_vkBuffer != VK_NULL_HANDLE){
            ::vkDestroyBuffer( m_vkDevice, m_vkBuffer, VK_NULL_HANDLE );
        }
        if (m_vkDeviceMemory != VK_NULL_HANDLE){
            ::std::cout << "Deallocating slice memory.." << ::std::endl;
            ::vkFreeMemory( m_vkDevice, m_vkDeviceMemory, VK_NULL_HANDLE );
        }
        Reset( );
    }

    VkDevice m_vkDevice;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkDeviceMemory;
    VkDeviceSize m_vkSize;
    size_t m_sliced, m_reserved, m_capacity;
};

} // namespace vkmr
#endif // VULKAN_SUPPORT
#endif // __VKMR_SLICES_H__
