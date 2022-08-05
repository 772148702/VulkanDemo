#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "HAL/ThreadSafeCounter.h"

#include "Vulkan/VulkanRHI.h"
#include "VulkanPlatform.h"
#include "vulkan/vulkan_core.h"

#include <memory>
#include <vector>
#include <winnt.h>

class VulkanDevice;
class VulkanDeviceMemoryManager;
class VulkanResourceHeap;
class VulkanResourceHeapPage;
class VulkanResourceHeapManager;
class VulkanBufferSubAllocation;
class VulkanSubBufferAllocator;
class VulkanSubResourceAllocator;

class RefCount
{
public:
    RefCount()
    {

    }

    virtual ~RefCount()
    {
       if(m_Counter.GetValue()!=0)
       {
        MLOG("Ref>0");
       }
    }

    FORCE_INLINE int32 AddRef()
    {
        int32 newValue = m_Counter.Increment();
        return newValue;
    }

    FORCE_INLINE int32 Release()
    {
        int32 newValue = m_Counter.Decrement();
        if(newValue==0)
        {
            delete this;
        }
        return newValue;
    }

    FORCE_INLINE int32 GetRefCount() const 
    {
        int32 value= m_Counter.GetValue();
        return value;
    }

    private:
    ThreadSafeCounter m_Counter;
};


struct VulkanRange
{
    uint32 offset;
    uint32 size;
    static void JoinConsecutiveRanges(std::vector<VulkanRange>& ranges);
    FORCE_INLINE bool operator<(const VulkanRange& vulkanRange) const
    {
        return offset<vulkanRange.offset;
    }
};


class VulkanDeviceMemoryAllocation
{
public:
    VulkanDeviceMemoryAllocation();
    void* Unmap();
    void FlushMappedMemory(VkDeviceSize offset,VkDeviceSize size);
    void InvalidateMappedMemory(VkDeviceSize offset,VkDeviceSize size);
    
	FORCE_INLINE bool CanBeMapped() const
	{
		return m_CanBeMapped;
	}

    FORCE_INLINE bool IsMapped() const
	{
		return m_MappedPointer != nullptr;
	}

	FORCE_INLINE void* GetMappedPointer()
	{
		return m_MappedPointer;
	}

	FORCE_INLINE bool IsCoherent() const
	{
		return m_IsCoherent;
	}

	FORCE_INLINE VkDeviceMemory GetHandle() const
	{
		return m_Handle;
	}

	FORCE_INLINE VkDeviceSize GetSize() const
	{
		return m_Size;
	}

	FORCE_INLINE uint32 GetMemoryTypeIndex() const
	{
		return m_MemoryTypeIndex;
	}

protected:
	virtual ~VulkanDeviceMemoryAllocation();

    friend class VulkanDeviceMemoryManager;
protected:
	VkDeviceSize    m_Size;
	VkDevice        m_Device;
	VkDeviceMemory  m_Handle;
	void*           m_MappedPointer;
	uint32          m_MemoryTypeIndex;
	bool            m_CanBeMapped;
	bool            m_IsCoherent;
	bool            m_IsCached;
	bool            m_FreedBySystem;

};


class VulkanDeviceMemoryManager
{
    public:
    VulkanDeviceMemoryManager();
    virtual ~VulkanDeviceMemoryManager();
    void Init(VulkanDevice* device);
    void Destory();
    bool SupportsMemoryType(VkMemoryPropertyFlags properties) const;
    VulkanDeviceMemoryAllocation* Alloc(bool canFail,VkDeviceSize allocationSize,uint32 memoryTypeIndex,void* dedicatedAllocateInfo, const char* file,uint32 line);
    void Free(VulkanDeviceMemoryAllocation*& allocation);
#if MONKEY_DEBUG
    void DumpMemory();
#endif
    uint64 GetTotalMemory(bool gpu) const;

    FORCE_INLINE bool HasUnifiedMemory() const
    {
        return m_HasUnifiedMemory;
    }

    FORCE_INLINE uint32 GetNumMemoryTypes() const
    {
        return m_MemoryProperties.memoryTypeCount;
    }


    FORCE_INLINE VkResult GetMemoryTypeFromProperties(uint32 typeBits, VkMemoryPropertyFlags properties, uint32* outTypeIndex)
    {
        for (uint32 i = 0; i < m_MemoryProperties.memoryTypeCount && typeBits; ++i)
        {
            if ((typeBits & 1) == 1)
            {
                if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    *outTypeIndex = i;
                    return VK_SUCCESS;
                }
            }
            typeBits >>= 1;
        }
        
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    
    FORCE_INLINE VkResult GetMemoryTypeFromPropertiesExcluding(uint32 typeBits, VkMemoryPropertyFlags properties, uint32 excludeTypeIndex, uint32* outTypeIndex)
    {
        for (uint32 i = 0; i < m_MemoryProperties.memoryTypeCount && typeBits; ++i)
        {
            if ((typeBits & 1) == 1)
            {
                if ((m_MemoryProperties.memoryTypes[i].propertyFlags & properties) == properties && excludeTypeIndex != i)
                {
                    *outTypeIndex = i;
                    return VK_SUCCESS;
                }
            }
            typeBits >>= 1;
        }
        
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }
    
    FORCE_INLINE const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const
    {
        return m_MemoryProperties;
    }
    
    FORCE_INLINE VulkanDeviceMemoryAllocation* Alloc(bool canFail, VkDeviceSize allocationSize, uint32 memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags, void* dedicatedAllocateInfo, const char* file, uint32 line)
    {
        uint32 memoryTypeIndex = ~0;
        VERIFYVULKANRESULT(this->GetMemoryTypeFromProperties(memoryTypeBits, memoryPropertyFlags, &memoryTypeIndex));
        return Alloc(canFail, allocationSize, memoryTypeIndex, dedicatedAllocateInfo, file, line);
    }
    
protected:
    struct HeapInfo
    {
        HeapInfo()
            : totalSize(0)
            , usedSize(0)
            , peakSize(0)
        {
            
        }
        
        VkDeviceSize totalSize;
        VkDeviceSize usedSize;
        VkDeviceSize peakSize;
        std::vector<VulkanDeviceMemoryAllocation*> allocations;
    };
    
    void SetupAndPrintMemInfo();
    
protected:

    VkPhysicalDeviceMemoryProperties m_MemoryProperties;
    VulkanDevice*                    m_Device;
    VkDevice                         m_DeviceHandle;
    bool                             m_HasUnifiedMemory;
    uint32                           m_NumAllocations;
    uint32                           m_PeakNumAllocations;
    std::vector<HeapInfo>            m_HeapInfos;
};

class VulkanResourceAllocation: public RefCount
{
     VulkanResourceAllocation(VulkanResourceHeapPage* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 requestedSize, uint32 alignedOffset, uint32 allocationSize, uint32 allocationOffset, const char* file, uint32 line);
    
    virtual ~VulkanResourceAllocation();

    void BindBuffer(VulkanDevice* device,VkBuffer buffer);
    void BindImage(VulkanDevice* device,VkImage image);

    FORCE_INLINE uint32 GetSize() const
    {
        return m_RequestedSize;
    }
     FORCE_INLINE uint32 GetAllocationSize()
    {
        return m_AllocationSize;
    }
    
    FORCE_INLINE uint32 GetOffset() const
    {
        return m_AlignedOffset;
    }
    
    FORCE_INLINE VkDeviceMemory GetHandle() const
    {
        return m_DeviceMemoryAllocation->GetHandle();
    }
    
    FORCE_INLINE void* GetMappedPointer()
    {
        return (uint8*)m_DeviceMemoryAllocation->GetMappedPointer() + m_AlignedOffset;
    }
    
    FORCE_INLINE uint32 GetMemoryTypeIndex() const
    {
        return m_DeviceMemoryAllocation->GetMemoryTypeIndex();
    }
    
    FORCE_INLINE void FlushMappedMemory()
    {
        m_DeviceMemoryAllocation->FlushMappedMemory(m_AllocationOffset, m_AllocationSize);
    }
    
    FORCE_INLINE void InvalidateMappedMemory()
    {
        m_DeviceMemoryAllocation->InvalidateMappedMemory(m_AllocationOffset, m_AllocationSize);
    }
private:
    friend class VulkanResourceHeapPage;
    
private:
    VulkanResourceHeapPage*         m_Owner;
    uint32                          m_AllocationSize;
    uint32                          m_AllocationOffset;
    uint32                          m_RequestedSize;
    uint32                          m_AlignedOffset;
    VulkanDeviceMemoryAllocation*   m_DeviceMemoryAllocation;
};



class VulkanResourceHeapPage
{
public:
    VulkanResourceHeapPage(VulkanResourceHeap* owner, VulkanDeviceMemoryAllocation* deviceMemoryAllocation, uint32 id);
    
    virtual ~VulkanResourceHeapPage();
    
    void ReleaseAllocation(VulkanResourceAllocation* allocation);
    
    VulkanResourceAllocation* TryAllocate(uint32 size, uint32 alignment, const char* file, uint32 line);
    
    VulkanResourceAllocation* Allocate(uint32 size, uint32 alignment, const char* file, uint32 line)
    {
        VulkanResourceAllocation* resourceAllocation = TryAllocate(size, alignment, file, line);
        return resourceAllocation;
    }
    
    FORCE_INLINE VulkanResourceHeap* GetOwner()
    {
        return m_Owner;
    }
    
    FORCE_INLINE uint32 GetID() const
    {
        return m_ID;
    }
    
protected:
    bool JoinFreeBlocks();
    
    friend class VulkanResourceHeap;
protected:

    VulkanResourceHeap*                     m_Owner;
    VulkanDeviceMemoryAllocation*           m_DeviceMemoryAllocation;
    std::vector<VulkanResourceAllocation*>  m_ResourceAllocations;
    std::vector<VulkanRange>                m_FreeList;
    
    uint32                                  m_MaxSize;
    uint32                                  m_UsedSize;
    int32                                   m_PeakNumAllocations;
    uint32                                  m_FrameFreed;
    uint32                                  m_ID;
};