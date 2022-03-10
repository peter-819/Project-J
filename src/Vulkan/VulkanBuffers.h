#pragma once
#include "VulkanInclude.h"
#include "VulkanDescs.h"
namespace ProjectJ{
    class VulkanBufferBase{
    public:
        VulkanBufferBase() = delete;
        VulkanBufferBase(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        virtual ~VulkanBufferBase();
    protected:
        VkDeviceMemory mMemory;
        VkDevice mDevice;
        VkPhysicalDevice mPhysicalDevice;
        VkDeviceSize mSize;
    //TODO: remove public 
    public:
        VkBuffer mBuffer;
    };
    using VulkanBufferPtr = std::shared_ptr<VulkanBufferBase>;

    template<class TUniformBufferClass, size_t Size = sizeof TUniformBufferClass>
    class VulkanUniformBuffer : public VulkanBufferBase{
    public:
        VulkanUniformBuffer(VkShaderStageFlags stageBit)
            :VulkanBufferBase(Size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
            mStageBit(stageBit){
                
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
        void GetStageBit() const {return mStageBit;}
    private:
        TUniformBufferClass mCpuBuffer;
        VkShaderStageFlags mStageBit;
    };

    template<class T>
    class VulkanDynamicUniformBuffer : public VulkanUniformBuffer<T> {};

    class VulkanStagingBuffer : public VulkanBufferBase{
    public:
        VulkanStagingBuffer::VulkanStagingBuffer(void* data, size_t size);
        void CopyToBuffer(const VulkanBufferBase* dstBuffer);
        void CopyToTexture(const class VulkanTexture* dstTex);
    private:
        VkCommandPool mCommandPool;
        VkQueue mGraphicQueue;
    };

    class VulkanVertexBuffer : public VulkanBufferBase{
    public:
        VulkanVertexBuffer(void* data, size_t size);
        VulkanVertexBuffer(size_t size);
    };

    class VulkanIndexBuffer : public VulkanBufferBase{
    public:
        VulkanIndexBuffer(void* data, size_t size);
        VulkanIndexBuffer(size_t size);
    };

    template<typename>
    struct is_uniform_buffer : std::false_type {};
    
    template<typename UB, size_t Size>
    struct is_uniform_buffer<std::shared_ptr<VulkanUniformBuffer<UB, Size> > > : std::true_type {};
    template<typename UB>
    struct is_uniform_buffer<std::shared_ptr<VulkanDynamicUniformBuffer<UB> > > : std::true_type {};

}