#include "VulkanDevice.h"
#include "Common/Log.h"
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
   	bool debugMarkersFound = false;
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> validationLayers;
    GetDeviceExtensionsAndLayers(deviceExtensions, validationLayers, debugMarkersFound);
    if(m_AppDeviceExtensions.size()>0)
    {
      	MLOG("Using app device extensions");
        for (int32 i = 0; i < m_AppDeviceExtensions.size(); ++i)
		{
			deviceExtensions.push_back(m_AppDeviceExtensions[i]);
			MLOG("* %s", m_AppDeviceExtensions[i]);
		}
    }
}

void VulkanDevice::Destroy()
{
    m_FenceManager->Destory();
    delete m_FenceManager;

    m_MemoryManager->Destory();
    delete m_MemoryManager;

    vkDestroyDevice(m_Device,VULKAN_CPU_ALLOCATOR);
    m_Device = VK_NULL_HANDLE;

}


void VulkanDevice::InitGPU(int32 deviceIndex)
{
    vkGetPhysicalDeviceFeatures(m_PhysicalDevice,&m_PhysicalDeviceFeatures);
    MLOG("Using Device %d: Geometry %d Tessellation %d", deviceIndex, m_PhysicalDeviceFeatures.geometryShader, m_PhysicalDeviceFeatures.tessellationShader);

    CreateDevice();
	SetupFormats();
    
    m_MemoryManager = new VulkanDeviceMemoryManager();
    m_MemoryManager->Init(this);
    
    m_FenceManager = new VulkanFenceManager();
	m_FenceManager->Init(this);
}