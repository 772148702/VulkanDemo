#include "Common/Common.h"
#include "VulkanPlatform.h"
#include "VulkanRHI.h"
#include "VulkanDevice.h"
#include "Utils/StringUtils.h"

#include "vector"
#include "vulkan/vulkan_core.h"
#include <string.h>


struct VulkanLayerExtension
{
	VulkanLayerExtension();
	void AddUniqueExtensionNames(std::vector<std::string>& outExtensions);
	void AddUniqueExtensionNames(std::vector<const char*>& outExtensions);

	VkLayerProperties layerProps;
	std::vector<VkExtensionProperties> extensionProps;
};

static const char* G_ValidationLayersInstance[] =
{
#if PLATFORM_WINDOWS
	"VK_LAYER_KHRONOS_validation",
#elif PLATFORM_MAC
	"VK_LAYER_LUNARG_standard_validation",
    "VK_LAYER_GOOGLE_unique_objects",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
#elif PLATFORM_IOS
    "MoltenVK",
#elif PLATFORM_ANDROID
	"VK_LAYER_GOOGLE_threading",
	"VK_LAYER_LUNARG_parameter_validation",
	"VK_LAYER_LUNARG_object_tracker",
	"VK_LAYER_LUNARG_core_validation",
	"VK_LAYER_LUNARG_swapchain",
	"VK_LAYER_GOOGLE_unique_objects",
#elif PLATFORM_LINUX
	"VK_LAYER_KHRONOS_validation",
#endif
	nullptr
};



static const char* G_ValidationLayersDevice[] =
{
#if PLATFORM_WINDOWS
	"VK_LAYER_KHRONOS_validation",
#elif PLATFORM_IOS
    "MoltenVK",
#elif PLATFORM_MAC
    "VK_LAYER_LUNARG_standard_validation",
    "VK_LAYER_GOOGLE_unique_objects",
    "VK_LAYER_GOOGLE_threading",
    "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker",
#elif PLATFORM_ANDROID
	"VK_LAYER_GOOGLE_threading",
	"VK_LAYER_LUNARG_parameter_validation",
	"VK_LAYER_LUNARG_object_tracker",
	"VK_LAYER_LUNARG_core_validation",
	"VK_LAYER_GOOGLE_unique_objects",
#elif PLATFORM_LINUX
	"VK_LAYER_KHRONOS_validation",
#endif
	nullptr
};

static const char* G_InstanceExtensions[] =
{
#if PLATFORM_WINDOWS
	
#elif PLATFORM_MAC

#elif PLATFORM_IOS
    
#elif PLATFORM_LINUX

#elif PLATFORM_ANDROID
    
#endif
	nullptr
};


static const char* G_DeviceExtensions[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,

#if PLATFORM_WINDOWS

#elif PLATFORM_MAC

#elif PLATFORM_IOS

#elif PLATFORM_LINUX

#elif PLATFORM_ANDROID
	
#endif

	nullptr
};


static FORCE_INLINE void EnumerateInstanceExtensionProperties(const char* layerName,VulkanLayerExtension& outLayer)
{
    uint32 count=0;
    vkEnumerateInstanceExtensionProperties(layerName,&count,nullptr);
    outLayer.extensionProps.resize(count);
	vkEnumerateInstanceExtensionProperties(layerName, &count, outLayer.extensionProps.data());
}


static FORCE_INLINE void EnumerateDeviceExtensionProperties(VkPhysicalDevice device,const char * layerName,VulkanLayerExtension& outLayer)
{
    uint32 count = 0;
    vkEnumerateDeviceExtensionProperties(device, layerName, &count, nullptr);
    outLayer.extensionProps.resize(count);
    vkEnumerateDeviceExtensionProperties(device, layerName, &count, outLayer.extensionProps.data());
}


static FORCE_INLINE int32 FindLayerIndexInList(const std::vector<VulkanLayerExtension>&layers,const char* layerName)
{
    for(int32 i=0;i<layers.size();i++)
    {
        if(strcmp(layers[i].layerProps.layerName,layerName)==0)
        {
            return i;
        }
    }
    return -1;
}

static FORCE_INLINE bool FindLayerInList(const std::vector<VulkanLayerExtension>&layers,const char* layerName)
{
    return FindLayerIndexInList(layers, layerName) != -1;
}

static FORCE_INLINE bool FindLayerExtensionInList(const std::vector<VulkanLayerExtension>& layers, const char* extensionName, const char*& foundLayer)
{
    for(int32 i=0;i<layers.size();++i)
    {
        for(int32 j=0;j<layers[i].extensionProps.size();j++)
        {
            if(strcmp(layers[i].extensionProps[j].extensionName, extensionName)==0)
            {
                foundLayer = layers[i].extensionProps[j].extensionName;
                return true;
            }
        }
    }
    return false;
}


static FORCE_INLINE bool FindLayerExtensionInList(const std::vector<VulkanLayerExtension>& layers, const char* extensionName)
{
	const char* dummy = nullptr;
	return FindLayerExtensionInList(layers, extensionName, dummy);
}


static FORCE_INLINE void TrimDuplicates(std::vector<const char*>& arr)
{
	for (int32 i = (int32)arr.size() - 1; i >= 0; --i)
	{
		bool found = false;
		for (int32 j = i - 1; j >= 0; --j)
		{
			if (strcmp(arr[i], arr[j]) == 0) {
				found = true;
				break;
			}
		}
		if (found) {
			arr.erase(arr.begin() + i);
		}
	}
}


VulkanLayerExtension::VulkanLayerExtension()
{
    memset(&layerProps,0,sizeof(VkLayerProperties));
}


void VulkanLayerExtension::AddUniqueExtensionNames(std::vector<std::string>& outExtensions)
{
    for(int32 i=0;i<outExtensions.size();i++)
    {
        StringUtils::AddUnique(outExtensions,extensionProps[i].extensionName);
    }
}






void VulkanDevice::GetDeviceExtensionsAndLayers(std::vector<const char*>& outDeviceExtensions, std::vector<const char*>& outDeviceLayers, bool& bOutDebugMarkers)
{
    bOutDebugMarkers= false;
    uint32 count=0;
    vkEnumerateDeviceLayerProperties(m_PhysicalDevice,&count,nullptr);
    std::vector<VkLayerProperties> properties(count);
    vkEnumerateDeviceLayerProperties(m_PhysicalDevice,&count,properties.data());
    std::vector<VulkanLayerExtension> deviceLayerExtensions(count+1);
    //why we need to start from index 1
    for (int32 index = 1; index < deviceLayerExtensions.size(); ++index) 
    {
        deviceLayerExtensions[index].layerProps = properties[index - 1];
    }
    std::vector<std::string> foundUniqueLayers;
    std::vector<std::string> foundLayerExtensions;
    for(int32 index=0;index<deviceLayerExtensions.size();index++)
    {
        if(index==0)
        {
            //todo
        }
    }

 }