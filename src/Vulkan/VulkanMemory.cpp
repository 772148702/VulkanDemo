#include "Common/Common.h"
#include "Common/Log.h"
#include "Math/Math.h"
#include "Utils/Alignment.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include <algorithm>
#include <stdint.h>


enum
{
    GPU_ONLY_HEAP_PAGE_SIZE     = 256 * 1024 * 1024,
    STAGING_HEAP_PAGE_SIZE      = 32 * 1024 * 1024,
    ANDROID_MAX_HEAP_PAGE_SIZE  = 16 * 1024 * 1024,
};

   
// VulkanRange
void VulkanRange::JoinConsecutiveRanges(std::vector<VulkanRange>& ranges)
{
    if (ranges.size() == 0) {
        return;
    }
    
    std::sort(ranges.begin(), ranges.end(), [](const VulkanRange& a, const VulkanRange& b) {
        if (a.offset > b.offset) {
            return 1;
        }
        else if (a.offset == b.offset) {
            return 0;
        }
        else {
            return -1;
        }
    });
    
    for (int32 index = (int32)ranges.size() - 1; index > 0; --index)
    {
        VulkanRange& current = ranges[index + 0];
        VulkanRange& prev    = ranges[index - 1];
        if (prev.offset + prev.size == current.offset)
        {
            prev.size += current.size;
            ranges.erase(ranges.begin() + index);
        }
    }
}


VulkanResourceAllocation::VulkanResourceAllocation(VulkanResourceHeapPage* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 requestedSize, uint32 alignedOffset, uint32 allocationSize, uint32 allocationOffset, const char* file, uint32 line)
    : m_Owner(owner)
    , m_AllocationSize(allocationSize)
    , m_AllocationOffset(allocationOffset)
    , m_RequestedSize(requestedSize)
    , m_AlignedOffset(alignedOffset)
    , m_DeviceMemoryAllocation(deviceMemoryAllocation)
{


}
VulkanResourceAllocation::~VulkanResourceAllocation()
{
    m_Owner->ReleaseAllocation(this);
}


void VulkanResourceAllocation::BindBuffer(VulkanDevice* device, VkBuffer buffer)
{
    VkResult result = vkBindBufferMemory(device->GetInstanceHandle(), buffer, GetHandle(), GetOffset());
#if MONKEY_DEBUG
    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        device->GetMemoryManager().DumpMemory();
    }
    VERIFYVULKANRESULT(result);
#endif
}


void VulkanResourceAllocation::BindImage(VulkanDevice* device, VkImage image)
{
    VkResult result = vkBindImageMemory(device->GetInstanceHandle(), image, GetHandle(), GetOffset());
#if MONKEY_DEBUG
    if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        device->GetMemoryManager().DumpMemory();
    }
#endif
    VERIFYVULKANRESULT(result);
}


VulkanDeviceMemoryManager::VulkanDeviceMemoryManager()
    : m_Device(nullptr)
    , m_DeviceHandle(VK_NULL_HANDLE)
    , m_HasUnifiedMemory(false)
    , m_NumAllocations(0)
    , m_PeakNumAllocations(0)
{
    memset(&m_MemoryProperties, 0, sizeof(VkPhysicalDeviceMemoryProperties));
}



void VulkanDeviceMemoryManager::Init(VulkanDevice* device)
{
    m_Device             = device;
    m_NumAllocations     = 0;
    m_PeakNumAllocations = 0;
    m_DeviceHandle       = m_Device->GetInstanceHandle();

    vkGetPhysicalDeviceMemoryProperties(m_Device->GetPhysicalHandle(), &m_MemoryProperties);
    m_HeapInfos.resize(m_MemoryProperties.memoryHeapCount);

    SetupAndPrintMemInfo();
}


VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager()
{
    Destory();
}


void VulkanDeviceMemoryManager::Destory()
{
    for(int32 index = 0;index<m_HeapInfos.size();index++)
    {
        if(m_HeapInfos[index].allocations.size()>0)
        {
            if(m_HeapInfos[index].allocations.size()>0)
            {
                MLOG("Found %d freed allocations!",(int32)m_HeapInfos[index].allocations.size());
#if MONKEY_DEBUG
            DumpMemory();
#endif
            }
        }
    }
    m_NumAllocations = 0;
}




bool VulkanDeviceMemoryManager::SupportsMemoryType(VkMemoryPropertyFlags properties) const
{
    for(uint32 index=0;index<m_MemoryProperties.memoryTypeCount;++index)
    {
        if(m_MemoryProperties.memoryTypes[index].propertyFlags==properties)
        {
            return true;
        }   
    }
    return false;
}

void VulkanDeviceMemoryManager::SetupAndPrintMemInfo()
{
    const uint32 maxAllocations = m_Device->GetLimits().maxMemoryAllocationCount;
    MLOG("%d Device Memory Heaps; Max memory allocations %d", m_MemoryProperties.memoryHeapCount, maxAllocations);
    for(uint32 index=0;index<m_MemoryProperties.memoryHeapCount;++index)
    {
        bool isGPUHeap = ((m_MemoryProperties.memoryHeaps[index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
           MLOG(
            "%d: Flags 0x%x Size %llu (%.2f MB) %s",
            index,
            m_MemoryProperties.memoryHeaps[index].flags,
            (uint64)(m_MemoryProperties.memoryHeaps[index].size),
            (float)((double)m_MemoryProperties.memoryHeaps[index].size / 1024.0 / 1024.0),
            isGPUHeap ? "GPU" : ""
        );
        m_HeapInfos[index].totalSize = m_MemoryProperties.memoryHeaps[index].size;
    }

    m_HasUnifiedMemory = false;
    MLOG("%d Device Memory Types (%sunified)", m_MemoryProperties.memoryTypeCount, m_HasUnifiedMemory ? "" : "Not");
    for (uint32 index = 0; index < m_MemoryProperties.memoryTypeCount; ++index)
    {
        auto GetFlagsString = [](VkMemoryPropertyFlags flags)
        {
            std::string str;
            if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                str += " Local";
            }
            if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                str += " HostVisible";
            }
            if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                str += " HostCoherent";
            }
            if ((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) == VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                str += " HostCached";
            }
            if ((flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) == VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                str += " Lazy";
            }
            return str;
        };
        MLOG(
            "%d: Flags 0x%x Heap %d %s",
            index,
            m_MemoryProperties.memoryTypes[index].propertyFlags,
            m_MemoryProperties.memoryTypes[index].heapIndex,
            GetFlagsString(m_MemoryProperties.memoryTypes[index].propertyFlags).c_str()
        );
    }

    for (uint32 index = 0; index < m_MemoryProperties.memoryHeapCount; ++index)
    {
        bool isGPUHeap = ((m_MemoryProperties.memoryHeaps[index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
        if (isGPUHeap) {
            m_HeapInfos[index].totalSize = (uint64)((float)m_HeapInfos[index].totalSize * 0.95f);
        }
    }
}

VulkanBufferSubAllocation* VulkanResourceHeapManager::AllocateBuffer(uint32 size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryPropertyFlags, const char* file, uint32 line)
{
    const VkPhysicalDeviceLimits& limits = m_VulkanDevice->GetLimits();
    uint32 alignment = 1;
    bool isStorageOrTexel = (bufferUsageFlags & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) != 0;
    if(isStorageOrTexel)
    {
        //what?
        alignment = MMath::Max(alignment, ((bufferUsageFlags & (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) != 0) ? (uint32)limits.minTexelBufferOffsetAlignment : 1u);
        alignment = MMath::Max(alignment, ((bufferUsageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0) ? (uint32)limits.minStorageBufferOffsetAlignment : 1u);
    }
    else 
    {
        alignment = (uint32)limits.minUniformBufferOffsetAlignment;
        bufferUsageFlags|= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;    
    }

    int32 poolSize = (int32)GetPoolTypeForAlloc(size, alignment);
    if (poolSize != (int32)PoolSizes::SizesCount) {
        size = m_PoolSizes[poolSize];
    }

    for(int32 index=0;index<m_UsedBufferAllocations[poolSize].size();index++)
    {
         VulkanSubBufferAllocator* bufferAllocation = m_UsedBufferAllocations[poolSize][index];
         if((bufferAllocation->m_BufferUsageFlags&bufferUsageFlags==bufferUsageFlags)&&
          (bufferAllocation->m_MemoryPropertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
          {
            VulkanBufferSubAllocation* subAllocation = (VulkanBufferSubAllocation*)bufferAllocation->TryAllocateNoLocking(size, alignment, file, line);
            if (subAllocation) {
                return subAllocation;
            }
          }
    }

    for (int32 index = 0; index < m_FreeBufferAllocations[poolSize].size(); ++index)
    {
        VulkanSubBufferAllocator* bufferAllocation = m_FreeBufferAllocations[poolSize][index];
        if ((bufferAllocation->m_BufferUsageFlags & bufferUsageFlags) == bufferUsageFlags &&
            (bufferAllocation->m_MemoryPropertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
        {
            VulkanBufferSubAllocation* subAllocation = (VulkanBufferSubAllocation*)bufferAllocation->TryAllocateNoLocking(size, alignment, file, line);
            if (subAllocation)
            {
                m_FreeBufferAllocations[poolSize].erase(m_FreeBufferAllocations[poolSize].begin() + index);
                m_UsedBufferAllocations[poolSize].push_back(bufferAllocation);
                return subAllocation;
            }
        }
    }
    
    uint32 bufferSize = MMath::Max(size, m_BufferSizes[poolSize]);
    
    VkBuffer buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo bufferCreateInfo;
    ZeroVulkanStruct(bufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    bufferCreateInfo.size  = bufferSize;
    bufferCreateInfo.usage = bufferUsageFlags;
    VERIFYVULKANRESULT(vkCreateBuffer(m_VulkanDevice->GetInstanceHandle(), &bufferCreateInfo, VULKAN_CPU_ALLOCATOR, &buffer));
    
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_VulkanDevice->GetInstanceHandle(), buffer, &memReqs);
    uint32 memoryTypeIndex = 0;
    VERIFYVULKANRESULT(m_VulkanDevice->GetMemoryManager().GetMemoryTypeFromProperties(memReqs.memoryTypeBits, memoryPropertyFlags, &memoryTypeIndex));
    alignment = MMath::Max((uint32)memReqs.alignment, alignment);
    
    VulkanDeviceMemoryAllocation* deviceMemoryAllocation = m_VulkanDevice->GetMemoryManager().Alloc(false, memReqs.size, memoryTypeIndex, nullptr, file, line);
    VERIFYVULKANRESULT(vkBindBufferMemory(m_VulkanDevice->GetInstanceHandle(), buffer, deviceMemoryAllocation->GetHandle(), 0));

    if (deviceMemoryAllocation->CanBeMapped()) {
        deviceMemoryAllocation->Map(bufferSize, 0);
    }
    
    VulkanSubBufferAllocator* bufferAllocation = new VulkanSubBufferAllocator(this, deviceMemoryAllocation, memoryTypeIndex, memoryPropertyFlags, uint32(memReqs.alignment), buffer, bufferUsageFlags, poolSize);
    m_UsedBufferAllocations[poolSize].push_back(bufferAllocation);
    
    return (VulkanBufferSubAllocation*)bufferAllocation->TryAllocateNoLocking(size, alignment, file, line);
}


VulkanSubResourceAllocator::VulkanSubResourceAllocator(VulkanResourceHeapManager* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 memoryTypeIndex, VkMemoryPropertyFlags memoryPropertyFlags, uint32 alignment)
    : m_Owner(owner)
    , m_MemoryTypeIndex(memoryTypeIndex)
    , m_MemoryPropertyFlags(memoryPropertyFlags)
    , m_DeviceMemoryAllocation(deviceMemoryAllocation)
    , m_Alignment(alignment)
    , m_FrameFreed(0)
    , m_UsedSize(0)
{
    m_MaxSize = (uint32)deviceMemoryAllocation->GetSize();

    VulkanRange fullRange;
    fullRange.offset = 0;
    fullRange.size   = m_MaxSize;
    m_FreeList.push_back(fullRange);
}


VulkanSubResourceAllocator::~VulkanSubResourceAllocator()
{
    
}


VulkanResourceSubAllocation* VulkanSubResourceAllocator::TryAllocateNoLocking(uint32 size, uint32 alignment, const char* file, uint32 line)
{
    m_Alignment = MMath::Max(m_Alignment, alignment);
    for (int32 index = 0; index < m_FreeList.size(); ++index)
    {
        VulkanRange& entry         = m_FreeList[index];
        uint32 allocatedOffset     = entry.offset;
        uint32 alignedOffset       = Align(entry.offset, m_Alignment);
        uint32 alignmentAdjustment = alignedOffset - entry.offset;
        uint32 allocatedSize       = alignmentAdjustment + size;

        if (allocatedSize <= entry.size)
        {
            if (allocatedSize < entry.size)
            {
                entry.size   -= allocatedSize;
                entry.offset += allocatedSize;
            }
            else {
                m_FreeList.erase(m_FreeList.begin() + index);
            }
            m_UsedSize += allocatedSize;
            VulkanResourceSubAllocation* newSubAllocation = CreateSubAllocation(size, alignedOffset, allocatedSize, allocatedOffset);
            m_SubAllocations.push_back(newSubAllocation);
            return newSubAllocation;
        }
    }
    
    return nullptr;
}

bool VulkanSubResourceAllocator::JoinFreeBlocks()
{
    VulkanRange::JoinConsecutiveRanges(m_FreeList);
    if (m_FreeList.size() == 1)
    {
        if (m_SubAllocations.size() == 0)
        {
            if (m_UsedSize != 0 || m_FreeList[0].offset != 0 || m_FreeList[0].size != m_MaxSize) {
                MLOG("Resource Suballocation leak, should have %d free, only have %d; missing %d bytes", m_MaxSize, m_FreeList[0].size, m_MaxSize - m_FreeList[0].size);
            }
            return true;
        }
    }
    
    return false;
}


VulkanSubBufferAllocator::VulkanSubBufferAllocator(VulkanResourceHeapManager* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 memoryTypeIndex, VkMemoryPropertyFlags memoryPropertyFlags, uint32 alignment, VkBuffer buffer, VkBufferUsageFlags bufferUsageFlags, int32 poolSizeIndex)
    : VulkanSubResourceAllocator(owner, deviceMemoryAllocation, memoryTypeIndex, memoryPropertyFlags, alignment)
    , m_BufferUsageFlags(bufferUsageFlags)
    , m_Buffer(buffer)
    , m_PoolSizeIndex(poolSizeIndex)
{
    
}


VulkanSubBufferAllocator::~VulkanSubBufferAllocator()
{
    if (m_Buffer != VK_NULL_HANDLE) {
        MLOGE("Failed destory VulkanSubBufferAllocator, buffer not null.")
    }
}

void VulkanSubBufferAllocator::Destroy(VulkanDevice* device)
{
    vkDestroyBuffer(device->GetInstanceHandle(), m_Buffer, VULKAN_CPU_ALLOCATOR);
	m_Buffer = VK_NULL_HANDLE;
}



VulkanResourceSubAllocation* VulkanSubBufferAllocator::CreateSubAllocation(uint32 size, uint32 alignedOffset, uint32 allocatedSize, uint32 allocatedOffset)
{
    return new VulkanBufferSubAllocation(this, m_Buffer, size, alignedOffset, allocatedSize, allocatedOffset);
}


void VulkanSubBufferAllocator::Release(VulkanBufferSubAllocation* subAllocation)
{
    bool released = false;
    for (int32 index = 0; index < m_SubAllocations.size(); ++index)
    {
        if (m_SubAllocations[index] == subAllocation)
        {
            released = true;
            m_SubAllocations.erase(m_SubAllocations.begin() + index);
            break;
        }
    }
    
    if (released)
    {
        VulkanRange newFree;
        newFree.offset = subAllocation->m_AllocationOffset;
        newFree.size   = subAllocation->m_AllocationSize;
        m_FreeList.push_back(newFree);
        m_UsedSize -= subAllocation->m_AllocationSize;
    }
    
    if (JoinFreeBlocks()) {
        m_Owner->ReleaseBuffer(this);
    }
}



VulkanDeviceMemoryAllocation::VulkanDeviceMemoryAllocation()
	: m_Size(0)
	, m_Device(VK_NULL_HANDLE)
	, m_Handle(VK_NULL_HANDLE)
	, m_MappedPointer(nullptr)
	, m_MemoryTypeIndex(0)
	, m_CanBeMapped(false)
	, m_IsCoherent(false)
	, m_IsCached(false)
	, m_FreedBySystem(false)
{

}

VulkanDeviceMemoryAllocation::~VulkanDeviceMemoryAllocation()
{

}

void* VulkanDeviceMemoryAllocation::Map(VkDeviceSize size, VkDeviceSize offset)
{
	vkMapMemory(m_Device, m_Handle, offset, size, 0, &m_MappedPointer);
	return m_MappedPointer;
}

void VulkanDeviceMemoryAllocation::Unmap()
{
	vkUnmapMemory(m_Device, m_Handle);
}


VulkanResourceHeapPage::VulkanResourceHeapPage(VulkanResourceHeap* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 id):
m_Owner(owner),m_DeviceMemoryAllocation(deviceMemoryAllocation),m_MaxSize(0),m_UsedSize(0),m_PeakNumAllocations(0),m_FrameFreed(0),m_ID(id)
{
    m_MaxSize = (uint32)m_DeviceMemoryAllocation->GetSize();
    VulkanRange fullRange;
    fullRange.offset = 0;
    fullRange.size = m_MaxSize;
    m_FreeList.push_back(fullRange);
}

VulkanResourceHeapPage::~VulkanResourceHeapPage()
{
    if(m_DeviceMemoryAllocation==nullptr)
    {
        MLOGE("Device memory allocation is null.");
    }
}

void VulkanResourceHeapPage::ReleaseAllocation(VulkanResourceAllocation* allocation)
{
    auto it = std::find(m_ResourceAllocations.begin(),m_ResourceAllocations.end(),allocation);
    if(it!=m_ResourceAllocations.end())
    {
        m_ResourceAllocations.erase(it);
        VulkanRange newFree;
        newFree.offset = allocation->m_AllocationOffset;
        newFree.size = allocation->m_AllocationSize;
        m_FreeList.push_back(newFree);
    }

    m_UsedSize -=   allocation->m_AllocationSize;
    if(m_UsedSize<0)
    {
        MLOGE("Used size less than zero.");
    }

    if (JoinFreeBlocks()) 
    {
        m_Owner->FreePage(this);
    }
}


VulkanResourceAllocation* VulkanResourceHeapPage::TryAllocate(uint32 size, uint32 alignment, const char* file, uint32 line)
{
    
}

