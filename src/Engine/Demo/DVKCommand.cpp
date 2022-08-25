#include "DVKCommand.h"
#include "Vulkan/VulkanCommon.h"
#include "Vulkan/VulkanGlobals.h"
#include "vulkan/vulkan_core.h"
#include <cstddef>

namespace vk_demo 
{
    DVKCommandBuffer::~DVKCommandBuffer()
    {
        VkDevice device = vulkanDevice->GetInstanceHandle();
        if(cmdBuffer!=VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(device,commandPool,1,&cmdBuffer);
            cmdBuffer = VK_NULL_HANDLE;
        }

        if(fence!=VK_NULL_HANDLE)
        {
            vkDestroyFence(device,fence,nullptr);
            fence = VK_NULL_HANDLE;
        }
        queue = nullptr;
        vulkanDevice = nullptr;
    }

    DVKCommandBuffer::DVKCommandBuffer()
    {

    }

    void DVKCommandBuffer::Submit(VkSemaphore* signalSemaphore)
    {

    }

    void DVKCommandBuffer::Begin()
    {
        if(isBegun)
        {
            return;
        }
        isBegun = true;
        VkCommandBufferBeginInfo cmdBufferBeginInfo;
        ZeroVulkanStruct(cmdBufferBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmdBuffer,&cmdBufferBeginInfo);
    }

    void DVKCommandBuffer::End()
    {
        if(!isBegun)
        {
            return ;
        }
        isBegun = false;
        vkEndCommandBuffer(cmdBuffer);
    }
}