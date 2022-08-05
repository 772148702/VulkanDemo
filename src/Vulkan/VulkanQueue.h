#pragma once
#include "Common/Common.h"
#include "Vulkan/VulkanRHI.h"
#include "VulkanPlatform.h"
#include "vulkan/vulkan_core.h"
#include <memory>

class VulkanDevice;

class VulkanQueue
{
public:
    VulkanQueue(VulkanDevice* device,uint32 familyIndex);
    virtual ~VulkanQueue();
    FORCE_INLINE uint32 GetFamilyIndex() const
    {
        return m_FamilyIndex;
    }
    FORCE_INLINE VkQueue GetHandle() const
    {
        return m_Queue;
    }
private:
    VkQueue m_Queue;
    uint32 m_FamilyIndex;
    VulkanDevice* m_Device;
};