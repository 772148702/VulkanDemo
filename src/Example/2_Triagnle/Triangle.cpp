#include "Common/Common.h"
#include "Common/Log.h"

#include "Application/AppModuleBase.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"

#include "Demo/FileManager.h"

#include "Vulkan/VulkanCommon.h"
#include "Vulkan/VulkanGlobals.h"
#include "vulkan/vulkan_core.h"

#include <cstring>
#include <minwinbase.h>
#include <stdint.h>
#include <vector>



class TriangleModule : public AppModuleBase
{
public:
    TriangleModule(int32 width,int32 height,const char* title,const std::vector<std::string>&cmdLine):
    AppModuleBase(width,height,title),
    m_Ready(false),
    m_IndicesCount(0)
    {

    }

    virtual ~TriangleModule()
    {

    }

    virtual bool PreInit() override
    {
        return true;
    }

private:

    struct GPUBuffer
    {
        VkDeviceMemory  memory;
        VkBuffer        buffer;

        GPUBuffer()
            : memory(VK_NULL_HANDLE)
            , buffer(VK_NULL_HANDLE)
        {

        }
    };
    typedef GPUBuffer   IndexBuffer;
    typedef GPUBuffer   VertexBuffer;
    typedef GPUBuffer   UBOBuffer;

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    struct UBOData
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 projection;
    };

   VkShaderModule LoadSPIPVShader(const std::string& filepath)
   {
        VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

        uint8* dataPtr = nullptr;
        uint32 dataSize = 0;
        FileManager::ReadFile(filepath,dataPtr,dataSize);
        
        VkShaderModuleCreateInfo moduleCreateInfo;
        ZeroVulkanStruct(moduleCreateInfo,VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        moduleCreateInfo.codeSize = dataSize;
        moduleCreateInfo.pCode = (uint32_t*) dataPtr;

        VkShaderModule shaderModule;
        VERIFYVULKANRESULT(vkCreateShaderModule(device, &moduleCreateInfo, VULKAN_CPU_ALLOCATOR, &shaderModule));
        delete[] dataPtr;
        return shaderModule;
   }

  void Draw(float time,float delta)
  {
    
  }


  void UpdateUniformBuffer(float time,float delta)
  {
     VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
     m_MVPData.model.AppendRotation(90.0f*delta,Vector3::UpVector);
     uint8_t * pData = nullptr;
     VERIFYVULKANRESULT(vkMapMemory(device,m_MVPBuffer.memory,0,sizeof(UBOData),0,(void**)&pData));
     std::memcpy(pData, &m_MVPData, sizeof(UBOData));
     vkUnmapMemory(device, m_MVPBuffer.memory);
  }

void CreateMeshBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    VkQueue queue = GetVulkanRHI()->GetDevice()->GetPresentQueue()->GetHandle();

    // 顶点数据
    std::vector<Vertex> vertices = {
        {
            {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }
        },
        {
            { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
        },
        {
            {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
        }
    };

    // 索引数据
    std::vector<uint16> indices = { 0, 1, 2 };
    m_IndicesCount = (uint32)indices.size();


    // 顶点数据以及索引数据在整个生命周期中几乎不会发生改变，因此最佳的方式是将这些数据存储到GPU的内存中。
    // 存储到GPU内存也能加快GPU的访问。为了存储到GPU内存中，需要如下几个步骤。
    // 1、在主机端(Host)创建一个Buffer
    // 2、将数据拷贝至该Buffer
    // 3、在GPU端(Local Device)创建一个Buffer
    // 4、通过Transfer簇将数据从主机端拷贝至GPU端
    // 5、删除主基端(Host)的Buffer
    // 6、使用GPU端(Local Device)的Buffer进行渲染

    VertexBuffer tempVertexBuffer;
    IndexBuffer tempIndexBuffer;

    void* dataPtr = nullptr;
    VkMemoryRequirements memReqInfo;
    VkMemoryAllocateInfo memAllocInfo;
    ZeroVulkanStruct(memAllocInfo,VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);

    //vertex buffer
    VkBufferCreateInfo vertexBufferInfo;
    ZeroVulkanStruct(vertexBufferInfo,VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    vertexBufferInfo.size = vertices.size()* sizeof(Vertex);
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VERIFYVULKANRESULT( vkCreateBuffer(device,&vertexBufferInfo,VULKAN_CPU_ALLOCATOR,&tempIndexBuffer.buffer));

    vkGetBufferMemoryRequirements(device,tempVertexBuffer.buffer,&memReqInfo);
    uint32 memoryTypeIndex = 0;
    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    GetVulkanRHI()->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex);
    memAllocInfo.allocationSize = memReqInfo.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex;
    VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &tempVertexBuffer.memory));
    VERIFYVULKANRESULT(vkBindBufferMemory(device, tempVertexBuffer.buffer, tempVertexBuffer.memory, 0));
    VERIFYVULKANRESULT(vkMapMemory(device, tempVertexBuffer.memory, 0, memAllocInfo.allocationSize, 0, &dataPtr));
    std::memcpy(dataPtr, vertices.data(), vertexBufferInfo.size);
    vkUnmapMemory(device, tempVertexBuffer.memory);

    // local device vertex buffer
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VERIFYVULKANRESULT(vkCreateBuffer(device, &vertexBufferInfo, VULKAN_CPU_ALLOCATOR, &m_VertexBuffer.buffer));
    vkGetBufferMemoryRequirements(device, m_VertexBuffer.buffer, &memReqInfo);
    //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    GetVulkanRHI()->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
    memAllocInfo.allocationSize  = memReqInfo.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex;
    VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &m_VertexBuffer.memory));
    VERIFYVULKANRESULT(vkBindBufferMemory(device, m_VertexBuffer.buffer, m_VertexBuffer.memory, 0));

    // index buffer
    VkBufferCreateInfo indexBufferInfo;
    ZeroVulkanStruct(indexBufferInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    indexBufferInfo.size  = m_IndicesCount * sizeof(uint16);
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VERIFYVULKANRESULT(vkCreateBuffer(device, &indexBufferInfo, VULKAN_CPU_ALLOCATOR, &tempIndexBuffer.buffer));

    vkGetBufferMemoryRequirements(device, tempIndexBuffer.buffer, &memReqInfo);
    GetVulkanRHI()->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex);
    memAllocInfo.allocationSize  = memReqInfo.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex;
    VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &tempIndexBuffer.memory));
    VERIFYVULKANRESULT(vkBindBufferMemory(device, tempIndexBuffer.buffer, tempIndexBuffer.memory, 0));
    VERIFYVULKANRESULT(vkMapMemory(device, tempIndexBuffer.memory, 0, memAllocInfo.allocationSize, 0, &dataPtr));
    std::memcpy(dataPtr, indices.data(), indexBufferInfo.size);
    vkUnmapMemory(device, tempIndexBuffer.memory);

    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    VERIFYVULKANRESULT(vkCreateBuffer(device, &indexBufferInfo, VULKAN_CPU_ALLOCATOR, &m_IndicesBuffer.buffer));
    vkGetBufferMemoryRequirements(device, m_IndicesBuffer.buffer, &memReqInfo);
    GetVulkanRHI()->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
    memAllocInfo.allocationSize  = memReqInfo.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIndex;
    VERIFYVULKANRESULT(vkAllocateMemory(device, &memAllocInfo, VULKAN_CPU_ALLOCATOR, &m_IndicesBuffer.memory));
    VERIFYVULKANRESULT(vkBindBufferMemory(device, m_IndicesBuffer.buffer, m_IndicesBuffer.memory, 0));

    VkCommandBuffer xferCmdBuffer;
    // gfx queue自带transfer功能，为了优化需要使用专有的xfer queue。这里为了简单，先将就用。
    VkCommandBufferAllocateInfo xferCmdBufferInfo;
    ZeroVulkanStruct(xferCmdBufferInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    xferCmdBufferInfo.commandPool        = m_CommandPool;
    xferCmdBufferInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    xferCmdBufferInfo.commandBufferCount = 1;
    VERIFYVULKANRESULT(vkAllocateCommandBuffers(device, &xferCmdBufferInfo, &xferCmdBuffer));

    // 开始录制命令
    VkCommandBufferBeginInfo cmdBufferBeginInfo;
    ZeroVulkanStruct(cmdBufferBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    VERIFYVULKANRESULT(vkBeginCommandBuffer(xferCmdBuffer, &cmdBufferBeginInfo));

    VkBufferCopy copyRegion = {};
    copyRegion.size = vertices.size() * sizeof(Vertex);
    vkCmdCopyBuffer(xferCmdBuffer, tempVertexBuffer.buffer, m_VertexBuffer.buffer, 1, &copyRegion);

    copyRegion.size = indices.size() * sizeof(uint16);
    vkCmdCopyBuffer(xferCmdBuffer, tempIndexBuffer.buffer, m_IndicesBuffer.buffer, 1, &copyRegion);

    // 结束录制
    VERIFYVULKANRESULT(vkEndCommandBuffer(xferCmdBuffer));

    // 提交命令，并且等待命令执行完毕。
    VkSubmitInfo submitInfo;
    ZeroVulkanStruct(submitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &xferCmdBuffer;

    VkFenceCreateInfo fenceInfo;
    ZeroVulkanStruct(fenceInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    fenceInfo.flags = 0;

    VkFence fence = VK_NULL_HANDLE;
    VERIFYVULKANRESULT(vkCreateFence(device, &fenceInfo, VULKAN_CPU_ALLOCATOR, &fence));
    VERIFYVULKANRESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
    VERIFYVULKANRESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, MAX_int64));

    vkDestroyFence(device, fence, VULKAN_CPU_ALLOCATOR);
    vkFreeCommandBuffers(device, m_CommandPool, 1, &xferCmdBuffer);

    vkDestroyBuffer(device, tempVertexBuffer.buffer, VULKAN_CPU_ALLOCATOR);
    vkFreeMemory(device, tempVertexBuffer.memory, VULKAN_CPU_ALLOCATOR);
    vkDestroyBuffer(device, tempIndexBuffer.buffer, VULKAN_CPU_ALLOCATOR);
    vkFreeMemory(device, tempIndexBuffer.memory, VULKAN_CPU_ALLOCATOR);
}

void DestoryMeshBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    vkDestroyBuffer(device, m_VertexBuffer.buffer, VULKAN_CPU_ALLOCATOR);
    vkFreeMemory(device, m_VertexBuffer.memory, VULKAN_CPU_ALLOCATOR);
    vkDestroyBuffer(device, m_IndicesBuffer.buffer, VULKAN_CPU_ALLOCATOR);
    vkFreeMemory(device, m_IndicesBuffer.memory, VULKAN_CPU_ALLOCATOR);
}

void CreateUniformBuffer()
{
    VkDevice device  = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

    VkBufferCreateInfo bufferInfo;
    bufferInfo.size = sizeof(UBOData);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    VERIFYVULKANRESULT(vkCreateBuffer(device,&bufferInfo,VULKAN_CPU_ALLOCATOR,&m_MVPBuffer.buffer));


    VkMemoryRequirements memReqInfo;
    vkGetBufferMemoryRequirements(device, m_MVPBuffer.buffer, &memReqInfo);
    uint32 memoryTypeIndex = 0;
    GetVulkanRHI()->GetDevice()->GetMemoryManager().GetMemoryTypeFromProperties(memReqInfo.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryTypeIndex);


    VkMemoryAllocateInfo allocInfo;
    ZeroVulkanStruct(allocInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
    allocInfo.allocationSize = memReqInfo.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    VERIFYVULKANRESULT( vkAllocateMemory(device,&allocInfo,VULKAN_CPU_ALLOCATOR,&m_MVPBuffer.memory));
    VERIFYVULKANRESULT( vkBindBufferMemory(device,m_MVPBuffer.buffer,m_MVPBuffer.memory,0));

    m_MVPDescriptor.buffer = m_MVPBuffer.buffer;
    m_MVPDescriptor.offset = 0;
    m_MVPDescriptor.range = sizeof(UBOData);

    m_MVPData.model.SetIdentity();
    m_MVPData.model.SetOrigin(Vector3(0,0,0));

    m_MVPData.view.SetIdentity();
    m_MVPData.view.SetOrigin(Vector4(0,0,-2.5f));
    m_MVPData.view.SetInverse();

    m_MVPData.projection.SetIdentity();
    m_MVPData.projection.Perspective(MMath::DegreesToRadians(75.0f), (float)GetWidth(), (float)GetHeight(), 0.01f, 3000.0f);
}

void DestoryUniformBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    vkDestroyBuffer(device,m_MVPBuffer.buffer,VULKAN_CPU_ALLOCATOR);
    vkFreeMemory(device,m_MVPBuffer.memory,VULKAN_CPU_ALLOCATOR);
}


void CreateSemaphores()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    VkSemaphoreCreateInfo createInfo;
    ZeroVulkanStruct(createInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    vkCreateSemaphore(device, &createInfo,VULKAN_CPU_ALLOCATOR,&m_RenderComplete);
}


void DestorySemaphores()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    vkDestroySemaphore(device, m_RenderComplete, VULKAN_CPU_ALLOCATOR);
}

void CreateFences()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    int32 frameCount = GetVulkanRHI()->GetSwapChain()->GetBackBufferCount();

    VkFenceCreateInfo fenceCreateInfo;
    ZeroVulkanStruct(fenceCreateInfo,VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_Fences.resize(frameCount);
    for(int32 i=0;i<m_Fences.size();++i)
    {
        vkCreateFence(device,&fenceCreateInfo,VULKAN_CPU_ALLOCATOR,&m_Fences[i]);
    }
}

void DestoryFences()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    for(int32 i=0;i<m_Fences.size();i++)
    {
        vkDestroyFence(device,m_Fences[i],VULKAN_CPU_ALLOCATOR);
    }
}

void CreateDescriptorSetLayout()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

    VkDescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descSetLayoutInfo;
    ZeroVulkanStruct(descSetLayoutInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    descSetLayoutInfo.bindingCount = 1;
    descSetLayoutInfo.pBindings = &layoutBinding;
    VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(device,&descSetLayoutInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorSetLayout));

    VkPipelineLayoutCreateInfo pipeLayoutInfo;
    ZeroVulkanStruct(pipeLayoutInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    pipeLayoutInfo.setLayoutCount = 1;
    pipeLayoutInfo.pSetLayouts    = &m_DescriptorSetLayout;
    VERIFYVULKANRESULT(vkCreatePipelineLayout(device, &pipeLayoutInfo, VULKAN_CPU_ALLOCATOR, &m_PipelineLayout));
}

void CreateCommandBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    
    VkCommandPoolCreateInfo cmdPoolInfo;
    ZeroVulkanStruct(cmdPoolInfo,VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    cmdPoolInfo.queueFamilyIndex = GetVulkanRHI()->GetDevice()->GetPresentQueue()->GetFamilyIndex();
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VERIFYVULKANRESULT(vkCreateCommandPool(device, &cmdPoolInfo, VULKAN_CPU_ALLOCATOR, &m_CommandPool));
    
    VkCommandBufferAllocateInfo cmdBufferInfo;
    ZeroVulkanStruct(cmdBufferInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    cmdBufferInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = 1;
    cmdBufferInfo.commandPool        = m_CommandPool;

    m_CommandBuffers.resize(GetVulkanRHI()->GetSwapChain()->GetBackBufferCount());
    for (int32 i = 0; i < m_CommandBuffers.size(); ++i)
    {
        vkAllocateCommandBuffers(device, &cmdBufferInfo, &(m_CommandBuffers[i]));
    }
}

private:
bool                            m_Ready = false;

    std::vector<VkFramebuffer>      m_FrameBuffers;

    VkImage                         m_DepthStencilImage = VK_NULL_HANDLE;
    VkImageView                     m_DepthStencilView = VK_NULL_HANDLE;
    VkDeviceMemory                  m_DepthStencilMemory = VK_NULL_HANDLE;

    VkRenderPass                    m_RenderPass = VK_NULL_HANDLE;
    VkSampleCountFlagBits           m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
    PixelFormat                     m_DepthFormat = PF_D24;

    VkCommandPool                   m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer>    m_CommandBuffers;

    std::vector<VkFence>            m_Fences;
    VkSemaphore                     m_PresentComplete = VK_NULL_HANDLE;
    VkSemaphore                     m_RenderComplete = VK_NULL_HANDLE;

    VertexBuffer                    m_VertexBuffer;
    IndexBuffer                     m_IndicesBuffer;
    UBOBuffer                       m_MVPBuffer;
    UBOData                         m_MVPData;

    VkDescriptorBufferInfo          m_MVPDescriptor;

    VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;

    VkPipeline                      m_Pipeline = VK_NULL_HANDLE;
    VkPipelineCache                 m_PipelineCache = VK_NULL_HANDLE;

    uint32                          m_IndicesCount = 0;
};