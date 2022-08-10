#include "Common/Common.h"
#include "Common/Log.h"

#include "Core/PixelFormat.h"
#include "Engine.h"
#include "Application/Application.h"
#include "Application/GenericApplication.h"

#include "RHIDefinitions.h"
#include "Vulkan/VulkanGlobals.h"
#include "Vulkan/Windows/VulkanWindowsPlatform.h"
#include "VulkanRHI.h"
#include "VulkanPlatform.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanMemory.h"
#include "VulkanSwapChain.h"
#include "VulkanMemory.h"
#include "vulkan/vulkan_core.h"
#include <memory>


VulkanRHI::VulkanRHI():m_Instance(VK_NULL_HANDLE),m_Device(nullptr),m_SwapChain(nullptr),m_PixelFormat(PF_B8G8R8A8)
{

}

VulkanRHI::~VulkanRHI()
{
    
}


void VulkanRHI::PostInit()
{

}



void VulkanRHI::Shutdown()
{
    DestorySwapChain();
#if MONKEY_DEBUG
    RemoveDebugLayerCallback();
#endif
    m_Device->Destroy();
    m_Device=nullptr;
    vkDestroyInstance(m_Instance,VULKAN_CPU_ALLOCATOR);
}



void VulkanRHI::InitInstance()
{
    CreateInstance();
    if(!VulkanPlatform::LoadVulkanInstanceFunctions(m_Instance))
    {
        MLOG("%s\n","Failed load vulkan instance functions");
        return;
    }
#if MONKEY_DEBUG
    SetupDebugLayerCallback();
#endif
    SelectAndInitDevice();
    RecreateSwapChain();
}

void VulkanRHI::RecreateSwapChain()
{
    DestorySwapChain();
    uint32 desiredBackBufferNum = 3;
    int32 width = Engine::Get()->GetPlatformWindow()->GetWidth();
    int32 height = Engine::Get()->GetPlatformWindow()->GetHeight();
    m_SwapChain = std::shared_ptr<VulkanSwapChain>(new VulkanSwapChain(m_Instance,m_Device,m_PixelFormat,width,height,&desiredBackBufferNum,m_BackbufferImages,1));
    m_BackbufferViews.resize(m_BackbufferImages.size());

    for(int32 i=0;i<m_BackbufferImages.size();i++)
    {
        VkImageViewCreateInfo imageViewCreatInfo;
        ZeroVulkanStruct(imageViewCreatInfo,VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        imageViewCreatInfo.format = PixelFormatToVkFormat(m_PixelFormat,false);
        imageViewCreatInfo.components = m_Device->GetFormatComponentMapping(m_PixelFormat);
        imageViewCreatInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreatInfo.image = m_BackbufferImages[i];
        imageViewCreatInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreatInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreatInfo.subresourceRange.levelCount     = 1;
        imageViewCreatInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreatInfo.subresourceRange.layerCount     = 1;
         VERIFYVULKANRESULT(vkCreateImageView(m_Device->GetInstanceHandle(), &imageViewCreatInfo, VULKAN_CPU_ALLOCATOR, &(m_BackbufferViews[i])));
    }
}


void VulkanRHI::DestorySwapChain()
{
     m_SwapChain = nullptr;
     for(int32 i=0;i<m_BackbufferViews.size();i++)
     {
        vkDestroyImageView(m_Device->GetInstanceHandle(),m_BackbufferViews[i],VULKAN_CPU_ALLOCATOR);
     }
}