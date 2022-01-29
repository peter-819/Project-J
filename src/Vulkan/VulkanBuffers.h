#pragma once
#include "VulkanInclude.h"
#include "VulkanDescs.h"

namespace ProjectJ{
    class VulkanBufferBase{
    public:
        VulkanBufferBase() = delete;
        VulkanBufferBase(VkDevice device, VkPhysicalDevice physicalDevice, 
            VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~VulkanBufferBase();
    protected:
        VkBuffer mBuffer;
        VkDeviceMemory mMemory;
        VkDevice mDevice;
        VkPhysicalDevice mPhysicalDevice;
    };

    template<class TUniformBufferClass, uint32_t Size = sizeof TUniformBufferClass>
    class VulkanUniformBuffer : public VulkanBufferBase{
    public:
        VulkanUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice)
            :VulkanBufferBase(device, physicalDevice, Size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){
                
            }

        void Sync(){
            void *data;
            vkMapMemory(mDevice, mMemory, 0, Size, 0, &data);
            memcpy(data, &mCpuBuffer, Size);
            vkUnmapMemory(mDevice, mMemory);
        }
        void ModifyAndSync(std::function<void(TUniformBufferClass&)> modifyFunc){
            modifyFunc(mCpuBuffer);
            Sync();
        }
        VkDescriptorBufferInfo GetBufferInfo(){
            VkDescriptorBufferInfo info{};
            info.buffer = mBuffer;
            info.offset = 0;
            info.range = Size;
            return info;
        }
    private:
        TUniformBufferClass mCpuBuffer;
    };
}