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