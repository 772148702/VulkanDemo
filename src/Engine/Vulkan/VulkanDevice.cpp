#include "VulkanDevice.h"
#include "Common/Common.h"
#include "Common/Log.h"
#include "Core/PixelFormat.h"
#include "VulkanPlatform.h"
#include "VulkanGlobals.h"
#include "VulkanFence.h"
#include "Application/Application.h"
#include "vulkan/vulkan_core.h"
#include <string>
#include <vector>


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



 void VulkanDevice::SetupFormats()
 {
    for(uint32 index=0;index<PF_MAX;++index)
    {
        const VkFormat format = (VkFormat) index;
        memset(&m_FormatProperties[index],0,sizeof(VkFormat));
        vkGetPhysicalDeviceFormatProperties( m_PhysicalDevice,  format,  &m_FormatProperties[index]);
    }
    for(int32 index=0;index<PF_MAX;++index)
    {
        G_PixelFormats[index].platformFormat = VK_FORMAT_UNDEFINED;
        G_PixelFormats[index].supported= false;
        VkComponentMapping& commponentMapping = m_PixelFormatComponentMapping[index];
        commponentMapping.r = VK_COMPONENT_SWIZZLE_R;
        commponentMapping.g = VK_COMPONENT_SWIZZLE_G;
        commponentMapping.b = VK_COMPONENT_SWIZZLE_B;
        commponentMapping.a = VK_COMPONENT_SWIZZLE_A;
    }

    MapFormatSupport(PF_B8G8R8A8,VK_FORMAT_B8G8R8G8_422_UNORM);
   	SetComponentMapping(PF_B8G8R8A8, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

    MapFormatSupport(PF_G8,VK_FORMAT_R8_UNORM);
    SetComponentMapping(PF_G8,VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

    MapFormatSupport(PF_G16, VK_FORMAT_R16_UNORM);
	SetComponentMapping(PF_G16, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_FloatRGB, VK_FORMAT_B10G11R11_UFLOAT_PACK32);
	SetComponentMapping(PF_FloatRGB, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_FloatRGBA, VK_FORMAT_R16G16B16A16_SFLOAT, 8);
	SetComponentMapping(PF_FloatRGBA, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);
    
    MapFormatSupport(PF_DepthStencil, VK_FORMAT_D32_SFLOAT_S8_UINT);
    if (!G_PixelFormats[PF_DepthStencil].supported)
    {
        MapFormatSupport(PF_DepthStencil, VK_FORMAT_D24_UNORM_S8_UINT);
        if (!G_PixelFormats[PF_DepthStencil].supported)
        {
            MapFormatSupport(PF_DepthStencil, VK_FORMAT_D16_UNORM_S8_UINT);
            if (!G_PixelFormats[PF_DepthStencil].supported) {
                MLOG("No stencil texture format supported!");
            }
        }
    }

    SetComponentMapping(PF_DepthStencil, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);

	MapFormatSupport(PF_ShadowDepth, VK_FORMAT_D16_UNORM);
	SetComponentMapping(PF_ShadowDepth, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY);

	MapFormatSupport(PF_G32R32F, VK_FORMAT_R32G32_SFLOAT, 8);
	SetComponentMapping(PF_G32R32F, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_A32B32G32R32F, VK_FORMAT_R32G32B32A32_SFLOAT, 16);
	SetComponentMapping(PF_A32B32G32R32F, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_G16R16, VK_FORMAT_R16G16_UNORM);
	SetComponentMapping(PF_G16R16, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_G16R16F, VK_FORMAT_R16G16_SFLOAT);
	SetComponentMapping(PF_G16R16F, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_G16R16F_FILTER, VK_FORMAT_R16G16_SFLOAT);
	SetComponentMapping(PF_G16R16F_FILTER, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R16_UINT, VK_FORMAT_R16_UINT);
	SetComponentMapping(PF_R16_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R16_SINT, VK_FORMAT_R16_SINT);
	SetComponentMapping(PF_R16_SINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R32_UINT, VK_FORMAT_R32_UINT);
	SetComponentMapping(PF_R32_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R32_SINT, VK_FORMAT_R32_SINT);
	SetComponentMapping(PF_R32_SINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R8_UINT, VK_FORMAT_R8_UINT);
	SetComponentMapping(PF_R8_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_D24, VK_FORMAT_D24_UNORM_S8_UINT);
    if (!G_PixelFormats[PF_D24].supported)
    {
        MapFormatSupport(PF_D24, VK_FORMAT_D16_UNORM_S8_UINT);
		if (!G_PixelFormats[PF_D24].supported)
		{
			MapFormatSupport(PF_D24, VK_FORMAT_D32_SFLOAT);
			if (!G_PixelFormats[PF_D24].supported)
			{
				MapFormatSupport(PF_D24, VK_FORMAT_D32_SFLOAT_S8_UINT);
				if (!G_PixelFormats[PF_D24].supported)
				{
					MapFormatSupport(PF_D24, VK_FORMAT_D16_UNORM);
                    if (!G_PixelFormats[PF_D24].supported) {
                        MLOG("%s", "No Depth texture format supported!");
                    }
				}
			}
		}
    }
	SetComponentMapping(PF_D24, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);
    
	MapFormatSupport(PF_R16F, VK_FORMAT_R16_SFLOAT);
	SetComponentMapping(PF_R16F, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R16F_FILTER, VK_FORMAT_R16_SFLOAT);
	SetComponentMapping(PF_R16F_FILTER, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_FloatR11G11B10, VK_FORMAT_B10G11R11_UFLOAT_PACK32, 4);
	SetComponentMapping(PF_FloatR11G11B10, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_A2B10G10R10, VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4);
	SetComponentMapping(PF_A2B10G10R10, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_A16B16G16R16, VK_FORMAT_R16G16B16A16_UNORM, 8);
	SetComponentMapping(PF_A16B16G16R16, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_A8, VK_FORMAT_R8_UNORM);
	SetComponentMapping(PF_A8, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_R);

	MapFormatSupport(PF_R5G6B5_UNORM, VK_FORMAT_R5G6B5_UNORM_PACK16);
	SetComponentMapping(PF_R5G6B5_UNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R8G8B8A8, VK_FORMAT_R8G8B8A8_UNORM);
	SetComponentMapping(PF_R8G8B8A8, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT);
	SetComponentMapping(PF_R8G8B8A8_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM);
	SetComponentMapping(PF_R8G8B8A8_SNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R16G16_UINT, VK_FORMAT_R16G16_UINT);
	SetComponentMapping(PF_R16G16_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT);
	SetComponentMapping(PF_R16G16B16A16_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT);
	SetComponentMapping(PF_R16G16B16A16_SINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT);
	SetComponentMapping(PF_R32G32B32A32_UINT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM);
	SetComponentMapping(PF_R16G16B16A16_SNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM);
	SetComponentMapping(PF_R16G16B16A16_UNORM, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A);

	MapFormatSupport(PF_R8G8, VK_FORMAT_R8G8_UNORM);
	SetComponentMapping(PF_R8G8, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_V8U8, VK_FORMAT_R8G8_UNORM);
	SetComponentMapping(PF_V8U8, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);

	MapFormatSupport(PF_R32_FLOAT, VK_FORMAT_R32_SFLOAT);
	SetComponentMapping(PF_R32_FLOAT, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ZERO);
 }


void VulkanDevice::MapFormatSupport(PixelFormat format, VkFormat vkFormat)
{
    PixelFormatInfo& formatInfo = G_PixelFormats[format];
    formatInfo.platformFormat = vkFormat;
    formatInfo.supported = IsFormatSupported(vkFormat);

    if(!formatInfo.supported)
    {
        MLOG("PixelFormat(%d) is not supported with Vk format %d", (int32)format, (int32)vkFormat);
    }
}

void VulkanDevice::SetComponentMapping(PixelFormat format, VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a)
{
    VkComponentMapping& componentMapping = m_PixelFormatComponentMapping[format];
    componentMapping.r  = r;
    componentMapping.g = g;
    componentMapping.b = b;
    componentMapping.a = a;
}

void VulkanDevice::MapFormatSupport(PixelFormat format, VkFormat vkFormat, int32 blockBytes)
{
    MapFormatSupport(format,vkFormat);
    PixelFormatInfo formatInfo = G_PixelFormats[format];
    formatInfo.blockBytes = blockBytes;
}

bool VulkanDevice::QueryGPU(int32 deviceIndex)
{
    bool discrete = false;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice,&m_PhysicalDeviceProperties);

    auto GetDeviceTypeString = [&]()->std::string
    {
        std::string info;
        switch (m_PhysicalDeviceProperties.deviceType) 
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                info = ("Other");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                info = ("Intergrate GPU");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                info=("Discrete GPU");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                info = ("Virutal GPU");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                info=("CPU");
                break;
            default:
                info=("Unkown");
                break;
        }
        return info;
    };
	MLOG("Device %d: %s", deviceIndex, m_PhysicalDeviceProperties.deviceName);
	MLOG("- API %d.%d.%d(0x%x) Driver 0x%x VendorId 0x%x", VK_VERSION_MAJOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_MINOR(m_PhysicalDeviceProperties.apiVersion), VK_VERSION_PATCH(m_PhysicalDeviceProperties.apiVersion), m_PhysicalDeviceProperties.apiVersion, m_PhysicalDeviceProperties.driverVersion, m_PhysicalDeviceProperties.vendorID);
	MLOG("- DeviceID 0x%x Type %s", m_PhysicalDeviceProperties.deviceID, GetDeviceTypeString().c_str());
	MLOG("- Max Descriptor Sets Bound %d Timestamps %d", m_PhysicalDeviceProperties.limits.maxBoundDescriptorSets, m_PhysicalDeviceProperties.limits.timestampComputeAndGraphics);

	uint32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, nullptr);
	m_QueueFamilyProps.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueCount, m_QueueFamilyProps.data());
    
    return discrete;
}


bool VulkanDevice::IsFormatSupported(VkFormat format)
{
  	auto ArePropertiesSupported = [](const VkFormatProperties& prop) -> bool 
	{
		return (prop.bufferFeatures != 0) || (prop.linearTilingFeatures != 0) || (prop.optimalTilingFeatures != 0);
	};
	if (format >= 0 && format < PF_MAX)
	{
		const VkFormatProperties& prop = m_FormatProperties[format];
		return ArePropertiesSupported(prop);
	}
    auto it = m_ExtensionFormatProperties.find(format);
	if (it != m_ExtensionFormatProperties.end())
	{
		const VkFormatProperties& foundProperties = it->second;
		return ArePropertiesSupported(foundProperties);
	}

    VkFormatProperties newProperties;
	memset(&newProperties, 0, sizeof(VkFormatProperties));
	
	vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &newProperties);
    m_ExtensionFormatProperties.insert(std::pair<VkFormat, VkFormatProperties>(format, newProperties));
    
	return ArePropertiesSupported(newProperties);
}



const VkComponentMapping& VulkanDevice::GetFormatComponentMapping(PixelFormat format) const
{
    return m_PixelFormatComponentMapping[format];
}