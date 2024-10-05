// Slices.h: declares the types, functions and classes for operating on slices of on-device GPU memory
//

#ifndef __VKMR_SLICES_H__
#define __VKMR_SLICES_H__

// Includes
//

// C++ Standard Library Headers
#include <unordered_map>

// Nearby Project Headers
#include "Utils.h"

// Local Project Headers
#include "Devices.h"

namespace vkmr {

// Forward Declarations
//

template <typename T>
class Slices;

// Templates
//

// Encapsulates a slice of on-device GPU memory
template <typename T>
class Slice {
public:
    friend class Slices<T>;

    typedef T value_type;
    typedef size_t size_type;
    typedef uint32_t number_type;

    Slice(void) { Reset( ); }
    Slice(Slice&& slice):
        m_vkDevice( slice.m_vkDevice ),
        m_vkBuffer( slice.m_vkBuffer ),
        m_vkDeviceMemory( slice.m_vkDeviceMemory ),
        m_vkSize( slice.m_vkSize ),
        m_sliced( slice.m_sliced ),
        m_reserved( slice.m_reserved ),
        m_capacity( slice.m_capacity ),
        m_filled( slice.m_filled ),
        m_alignedCount( slice.m_alignedCount ),
        m_number( slice.m_number ) {

        slice.Reset( );
    }
    Slice(Slice const&) = delete;

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
            m_filled = slice.m_filled;
            m_alignedCount = slice.m_alignedCount;
            m_number = slice.m_number;

            slice.Reset( );
        }
        return (*this);
    }

    Slice& operator=(Slice const&) = delete;
    operator bool() const { return (m_vkDeviceMemory != VK_NULL_HANDLE); }

    Slice& operator+=(Slice&& sub) {
        if (sub.Number( ) == Number( )){
            m_filled += sub.m_vkSize / sizeof( T );
        }
        return (*this);
    }

    bool IsFilled(void) const {
        return m_filled == m_capacity;
    }

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

    // Returns the slice number
    number_type Number(void) const {
        return m_number;
    }

    // Returns the number of available slots in the slice
    inline size_type Available(void) const {
        return m_capacity - (m_sliced + m_reserved);
    }

    // Reserves a  given number of, er, slot(s) (?) within the slice
    bool Reserve(size_type count = 1U) {
        if (Available( ) > count){
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
    Slice Sub(void) {

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
                slice = Slice( m_number, m_vkDevice, vkBuffer, VK_NULL_HANDLE, vkBufferCreateInfo.size );

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

private:
    Slice(number_type number, VkDevice vkDevice, VkBuffer vkBuffer, VkDeviceMemory vkDeviceMemory, VkDeviceSize vkSize):
        m_vkDevice( vkDevice ),
        m_vkBuffer( vkBuffer ),
        m_vkDeviceMemory( vkDeviceMemory ),
        m_vkSize( vkSize ),
        m_sliced( 0U ),
        m_reserved( 0U ),
        m_capacity( vkSize / sizeof( T ) ),
        m_number( number ) {

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
        m_sliced = m_reserved = m_capacity = m_filled = m_alignedCount = 0U;
        m_number = 0U;
    }

    void Release(void) {

        if (m_vkBuffer != VK_NULL_HANDLE){
            ::vkDestroyBuffer( m_vkDevice, m_vkBuffer, VK_NULL_HANDLE );
        }
        if (m_vkDeviceMemory != VK_NULL_HANDLE){
            ::std::cout << "Deallocating memory for slice " << this->Number( ) << ".." << ::std::endl;
            ::vkFreeMemory( m_vkDevice, m_vkDeviceMemory, VK_NULL_HANDLE );
        }
        Reset( );
    }

    VkDevice m_vkDevice;
    VkBuffer m_vkBuffer;
    VkDeviceMemory m_vkDeviceMemory;
    VkDeviceSize m_vkSize;
    size_type m_sliced, m_reserved, m_capacity, m_filled, m_alignedCount;
    number_type m_number;

    static const VkBufferUsageFlags c_vkBufferUsageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
};

template <typename T>
class Slices {
public:
    typedef Slice<T> slice_type;
    typedef typename slice_type::number_type index_type;

    Slices(VkDeviceSize vkSliceSize): m_vkPreferredSliceSize( vkSliceSize ), m_current( 0U ) { }
    Slices(): Slices( 0U ) { }
    Slices(Slices const&) = delete;
    Slices(Slices&& slices):
        m_vkPreferredSliceSize( slices.m_vkPreferredSliceSize ),
        m_current( slices.m_current ),
        m_container( ::std::move( slices.m_container ) ),
        m_empty( ::std::move( slices.m_empty ) ) {

        slices.Reset( );
    }

    Slices& operator=(Slices const&) = delete;
    Slices& operator=(Slices&& slices) {

        if (this != &slices){
            m_container.clear( );

            m_vkPreferredSliceSize = slices.m_vkPreferredSliceSize;
            m_current = slices.m_current;
            m_container = ::std::move( slices.m_container );
            m_empty = ::std::move( slices.m_empty );

            slices.Reset( );
        }
        return (*this);
    }

    slice_type& operator[](index_type index) {
        const auto found = m_container.find( index );
        if (found == m_container.cend( )){
            return m_empty;
        }
        return found->second;
    }

    // Returns a reference to the currently-active slice,
    // or an empty slice if none
    slice_type& Current(void) { return (*this)[m_current]; }

    // Removes and returns the slice with the given number,
    // or an empty slice if none
    slice_type Remove(index_type index) {

        const auto found = m_container.find( m_current );
        if (found == m_container.cend( )){
            return slice_type( );
        }

        auto slice = ::std::move( found->second );
        m_container.erase( found );
        return slice;
    }

    // Allocates and returns a new slice of on-device memory from the
    // given device (GPU) which can hold some whole number of elements
    // not smaller than a given minimum
    slice_type& New(ComputeDevice& device) {

        // Look for an early out
        if (m_vkPreferredSliceSize == 0U){
            return m_empty;
        }

        // Get the target slice size
        auto vkSliceSize = this->SliceSize( device );

        // Loop until we've maybe allocated _something_
        VkDeviceMemory vkDeviceMemory = VK_NULL_HANDLE;
        do {
            // Adjust for whole element size
            vkSliceSize = vkSliceSize - (vkSliceSize % sizeof( T ));

            // Look for corresponding on-device memory
            const VkMemoryRequirements vkMemoryRequirements = device.StorageBufferRequirements( vkSliceSize );
            auto deviceMemoryBudgets = device.AvailableMemoryTypes(
                vkMemoryRequirements,
                c_vkMemoryPropertyFlags
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
            vkBufferCreateInfo.usage = slice_type::c_vkBufferUsageFlags;
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
                const auto number = ++m_current; 
                auto emplaced = m_container.emplace(
                    number,
                    slice_type( number, vkDevice, vkBuffer, vkDeviceMemory, vkSliceSize )
                );
                if (emplaced.second){
                    return (emplaced.first)->second;
                }
            }
            device.Free( vkDeviceMemory );
            break;
        } while (vkSliceSize > 1U);
        return m_empty;
    }

    // Returns the maximum number of concurrent slices that can be allocated
    // from the given (compute) device
    index_type MaxSliceCount(const ComputeDevice& device) const {

        // Get the memory budgets for compatible memory types
        const auto dataRequirements = device.StorageBufferRequirements( m_vkPreferredSliceSize );
        const auto deviceMemoryBudgets = device.AvailableMemoryTypes(
            dataRequirements,
            c_vkMemoryPropertyFlags
        );

        // Reduce down to the max per heap
        ::std::unordered_map<uint32_t, VkDeviceSize> heaped;
        for (auto it = deviceMemoryBudgets.cbegin( ), end = deviceMemoryBudgets.cend( ); it != end; it++){
            const auto& deviceMemoryBudget = *it;

            // Look for the heap
            const auto found = heaped.find( deviceMemoryBudget.heapIndex );
            if (found == heaped.end( )){
                heaped.insert( { deviceMemoryBudget.heapIndex, deviceMemoryBudget.vkMemorySize } );
                continue;
            }
            found->second = ::std::max( found->second, deviceMemoryBudget.vkMemorySize );
        }

        // Now iterate the heaps and sum up
        index_type result = 0U;
        for (auto it = heaped.cbegin( ), end = heaped.cend( ); it != end; ++it){
            const auto count = static_cast<uint32_t>( it->second / m_vkPreferredSliceSize );
            result += count;
        }
        return result;
    }

    // Calculates the actual size of a slice on the given compute device
    VkDeviceSize SliceSize(const ComputeDevice& device) const {

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
        // * the preferred slice size
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
        vkSliceSize = ::std::min( vkSliceSize, m_vkPreferredSliceSize );

        // Cap to the size needed to hold the largest possible
        // number of elements which is a power of 2
        const auto count = largest_pow2_le( vkSliceSize / sizeof( T ) );
        return count * sizeof( T );
    }

private:
    void Reset(void) {
        m_vkPreferredSliceSize = 0U;
        m_current = 0U;
        m_container.clear( );
    }

    VkDeviceSize m_vkPreferredSliceSize;
    index_type m_current;
    ::std::unordered_map<index_type, slice_type> m_container;
    slice_type m_empty;

    static const VkMemoryPropertyFlags c_vkMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
};

} // namespace vkmr
#endif // __VKMR_SLICES_H__
