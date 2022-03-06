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
    void VulkanStagingBuffer::CopyToBuffer(const VulkanBufferBase* dstBuffer){
        auto queue = RHI::Get().mQueue;
        queue->ExecuteDirectly([this, dstBuffer](VkCommandBuffer& commandBuffer){
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = mSize;
            vkCmdCopyBuffer(commandBuffer,mBuffer,dstBuffer->mBuffer,1,&copyRegion);
        });        
    }
    void VulkanStagingBuffer::CopyToTexture(const VulkanTexture* dstTex){
        auto queue = RHI::Get().mQueue;
        queue->ExecuteDirectly([this,dstTex](VkCommandBuffer& commandBuffer){
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                dstTex->Width,
                dstTex->Height,
                1
            };
            vkCmdCopyBufferToImage(
                commandBuffer,
                mBuffer,
                dstTex->mImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        });
    }
    VulkanVertexBuffer::VulkanVertexBuffer(void* data, size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanStagingBuffer stagingBuffer(data,size);
        stagingBuffer.CopyToBuffer(this);
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
        stagingBuffer.CopyToBuffer(this);
    }

    VulkanIndexBuffer::VulkanIndexBuffer(size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
    }
}