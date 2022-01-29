#include <Jpch.h>
#include "VulkanBuffers.h"

namespace ProjectJ{
    VulkanBufferBase::VulkanBufferBase(VkDevice device, VkPhysicalDevice physicalDevice, 
            VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        : mDevice(device), mPhysicalDevice(physicalDevice) 
    {
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
}