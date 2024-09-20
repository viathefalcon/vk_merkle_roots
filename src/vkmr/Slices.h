// Slices.h: declares the types, functions and classes for operating on slices of on-device GPU memory
//

#ifndef __VKMR_SLICES_H__
#define __VKMR_SLICES_H__

// Includes
//

// Nearby Project Headers
#include "Utils.h"

// Local Project Headers
#include "Devices.h"

namespace vkmr {

// Templates
//

// Encapsulates a slice of on-device GPU memory
template <typename T>
class Slice {
public:
    typedef T value_type;
    typedef size_t size_type;

    Slice(void) { Reset( ); }
    Slice(Slice&& slice):
        m_vkDevice( slice.m_vkDevice ),
        m_vkBuffer( slice.m_vkBuffer ),
        m_vkDeviceMemory( slice.m_vkDeviceMemory ),
        m_vkSize( slice.m_vkSize ),
        m_sliced( slice.m_sliced ),
        m_reserved( slice.m_reserved ),
        m_capacity( slice.m_capacity ),
        m_alignedCount( slice.m_alignedCount ) {

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
            m_alignedCount = slice.m_alignedCount;

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

    // Returns the number of elements needed for the slice
    // to be correctly aligned at both ends
    size_type AlignedReservationSize(void) const {
        return m_alignedCount;
    }

    // Reserves a  given number of, er, slot(s) (?) within the slice
    bool Reserve(size_type count = 1U) {
        if ((m_capacity - (m_sliced + m_reserved)) > count){
            m_reserved += count;
            return true;
        }
        return false;
    }

    void Unreserve(size_type count = 1U) {
        m_reserved -= ::std::min( m_reserved, count );
    }

    // Return the number of reservations in the slice
    size_type Reserved(void) const { return m_reserved; }

    // Returns the number of elements in the slice
    size_type Count(void) const { return m_sliced; }

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
            vkBufferCreateInfo.usage = c_vkBufferUsageFlags;
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
                vkBuffer = VK_NULL_HANDLE;
            }
            if (vkResult == VK_SUCCESS){
                slice = Slice( m_vkDevice, vkBuffer, VK_NULL_HANDLE, vkBufferCreateInfo.size );

                // Update our own internal state
                m_sliced += m_reserved;
                m_reserved = 0U;
            }else if (vkBuffer != VK_NULL_HANDLE){
                ::vkDestroyBuffer( m_vkDevice, vkBuffer, pAllocator );
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
    // not smaller than a given minimum
    static Slice New(ComputeDevice& device, VkDeviceSize vkMinSize) {

        // Be pessimistic - start w/an empty slice
        Slice slice;

        // Query for the maximum number of concurrent invocations
        // and the per-device allocation limit
        VkPhysicalDeviceMaintenance3PropertiesKHR vkPhysicalDeviceMaintenance3Properties = {};
        vkPhysicalDeviceMaintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
        VkPhysicalDeviceProperties2KHR vkPhysicalDeviceProperties2 = {};
        vkPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        vkPhysicalDeviceProperties2.pNext = &vkPhysicalDeviceMaintenance3Properties;
        device.GetPhysicalDeviceProperties2KHR( &vkPhysicalDeviceProperties2 );

        // We want the smaller of: 
        // * the largest slice we can reduce in one pass
        // * the maximum the system (says it) will let us allocate.
        // * the maximum buffer range
        const VkPhysicalDeviceProperties& vkPhysicalDeviceProperties = vkPhysicalDeviceProperties2.properties;
        VkDeviceSize vkSliceSize = sizeof( T ) * vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0] * vkPhysicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
        if (vkPhysicalDeviceMaintenance3Properties.maxMemoryAllocationSize > 0U){
            vkSliceSize = ::std::min( vkSliceSize, vkPhysicalDeviceMaintenance3Properties.maxMemoryAllocationSize );
        }
        if (vkPhysicalDeviceProperties.limits.maxStorageBufferRange > 0U){
            vkSliceSize = ::std::min(
                vkSliceSize,
                static_cast<decltype( vkSliceSize )>( vkPhysicalDeviceProperties.limits.maxStorageBufferRange )
            );
        }

        // Cap to the size needed to hold the largest possible
        // number of elements which is a power of 2
        const auto count = largest_pow2_le( vkSliceSize / sizeof( T ) );
        vkSliceSize = count * sizeof( T );

        // Loop until we've maybe allocated _something_
        VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
        do {
            // Adjust for whole element size
            vkSliceSize = vkSliceSize - (vkSliceSize % sizeof( T ));

            // Look for corresponding on-device memory
            const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( vkSliceSize );
            auto deviceMemoryBudgets = device.AvailableMemoryTypes(
                vkMemoryRequirements,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            const auto vkAllocationSize = vkMemoryRequirements.size;
            std::cout << "Looking for " << vkAllocationSize << " bytes of sliced memory.." << std::endl;

            // Iterate and try to allocate
            for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); (vkDeviceMemory == VK_NULL_HANDLE) && (it != end); it++){
                const auto& deviceMemoryBudget = *it;
                if (deviceMemoryBudget.vkMemoryBudget < vkAllocationSize){
                    // Nah, not interested
                    continue;
                }
                vkDeviceMemory = device.Allocate( deviceMemoryBudget, vkAllocationSize );
            }

            if (vkDeviceMemory == VK_NULL_HANDLE){
                // Try again for half the size?
                vkSliceSize = (vkSliceSize >> 1);
                continue;
            }

            // Otherwise, try and wrap it all up
            const VkAllocationCallbacks *pAllocator = VK_NULL_HANDLE;
            auto vkDevice = *device;

            // Create the buffer
            VkBuffer vkBuffer = VK_NULL_HANDLE;
            VkBufferCreateInfo vkBufferCreateInfo = {};
            vkBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vkBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkBufferCreateInfo.usage = c_vkBufferUsageFlags;
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
                // Bail
                device.Free( vkDeviceMemory );
            }

            // Can stop here, regardless?
            break;
        } while (vkSliceSize >= vkMinSize);
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

        // Calcalate the number of elements needed for the slice to be correctly
        // aligned at both ends
        VkMemoryRequirements vkMemoryRequirements = {};
        ::vkGetBufferMemoryRequirements( m_vkDevice, m_vkBuffer, &vkMemoryRequirements );
        if (vkMemoryRequirements.alignment == 0){
            m_alignedCount = 1;
        }else{
            m_alignedCount = lowest_common_multiple( sizeof( T ), vkMemoryRequirements.alignment ) / sizeof( T );
        }
    }

    void Reset(void) {

        m_vkDevice = VK_NULL_HANDLE;
        m_vkBuffer = VK_NULL_HANDLE;
        m_vkDeviceMemory = VK_NULL_HANDLE;
        m_vkSize = 0U;
        m_sliced = m_reserved = m_capacity = m_alignedCount = 0U;
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
    size_type m_sliced, m_reserved, m_capacity, m_alignedCount;

    static const VkBufferUsageFlags c_vkBufferUsageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
};

} // namespace vkmr
#endif // __VKMR_SLICES_H__
