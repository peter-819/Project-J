#include <Jpch.h>
#include "VulkanBuffers.h"
namespace ProjectJ{
    VulkanBufferBase::VulkanBufferBase(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        mSize = size;
        mDevice = RHI::Get().mDevice;
        mPhysicalDevice = RHI::Get().mPhysicalDevice;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(mDevice,&bufferInfo,nullptr,&mBuffer),"failed to create vertex buffer.");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice,mBuffer,&memRequirements);
        auto findMemoryType = [this](uint32_t typeFilter, VkMemoryPropertyFlags properties){
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice,&memProperties);
            for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
                if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
                    return i;
                }
            }
            throw std::runtime_error("failed to find suitable memory type.");
        };

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            properties
        );
        VK_CHECK(vkAllocateMemory(mDevice,&allocInfo,nullptr,&mMemory),"failed to allocate vertex buffer memory.");
        vkBindBufferMemory(mDevice,mBuffer,mMemory,0);
    }
    VulkanBufferBase::~VulkanBufferBase(){
        vkDestroyBuffer(mDevice,mBuffer,nullptr);
        vkFreeMemory(mDevice,mMemory,nullptr);
    }
    VulkanStagingBuffer::VulkanStagingBuffer(void* data, size_t size) 
        : VulkanBufferBase(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            void* p;
            vkMapMemory(mDevice,mMemory,0,size,0,&p);
            memcpy(p,data,(size_t)(size));
            vkUnmapMemory(mDevice,mMemory);
    }
    void VulkanStagingBuffer::CopyToDst(const VulkanBufferBase* dstBuffer){
        mCommandPool = RHI::Get().mCommandPool;
        mGraphicQueue = RHI::Get().mGraphicQueue;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = mCommandPool;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(mDevice,&allocInfo,&commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer,&beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = mSize;
        vkCmdCopyBuffer(commandBuffer,mBuffer,dstBuffer->mBuffer,1,&copyRegion);
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(mGraphicQueue,1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(mGraphicQueue);
        vkFreeCommandBuffers(mDevice,mCommandPool,1,&commandBuffer);
    }

    VulkanVertexBuffer::VulkanVertexBuffer(void* data, size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanStagingBuffer stagingBuffer(data,size);
        stagingBuffer.CopyToDst(this);
    }

    VulkanVertexBuffer::VulkanVertexBuffer(size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
    }

    VulkanIndexBuffer::VulkanIndexBuffer(void* data, size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanStagingBuffer stagingBuffer(data,size);
        stagingBuffer.CopyToDst(this);
    }

    VulkanIndexBuffer::VulkanIndexBuffer(size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
    }
}