#include "VulkanDevice.h"
#include "VulkanPlatform.h"
#include "VulkanGlobals.h"
#include "VulkanFence.h"
#include "Application/Application.h"
#include "vulkan/vulkan_core.h"


VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice)
    : m_Device(VK_NULL_HANDLE)
    , m_PhysicalDevice(physicalDevice)
    , m_GfxQueue(nullptr)
    , m_ComputeQueue(nullptr)
    , m_TransferQueue(nullptr)
    , m_PresentQueue(nullptr)
    , m_FenceManager(nullptr)
    , m_MemoryManager(nullptr)
	, m_PhysicalDeviceFeatures2(nullptr)
{
    
}


VulkanDevice::~VulkanDevice()
{
    if(m_Device!=VK_NULL_HANDLE)
    {
        Destroy();
        m_Device=VK_NULL_HANDLE;
    }
}


void VulkanDevice::CreateDevice()
{
    
}