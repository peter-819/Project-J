#pragma once
#include "VulkanInclude.h"
#include "VulkanDescs.h"

namespace ProjectJ{
    class VulkanQueue{
        friend class ScopedFrame;

    public:
        VulkanQueue();
        ~VulkanQueue();
    public:
        VkCommandBuffer AllocCommandBuffer();
        void AllocCommandBuffer(uint32_t count, std::vector<VkCommandBuffer>& commandBuffers);
        void ExecuteDirectly(std::function<void(VkCommandBuffer&)> func);
        void PrepareFrameCommands(std::function<void(uint32_t, VkCommandBuffer&)> func);
        void BeginFrame();
        void EndFrame();
    private:
        void PCreateSyncObjects();
        void PCreateCommandBuffers();
    private:
        VkCommandPool mCommandPool;
        VkQueue mGraphicQueue;
        VkQueue mPresentQueue;
        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        std::vector<VkCommandBuffer> mFrameCommandBuffers;
        std::vector<VkFence> mInFlightFences;
        const int MAX_FRAMES_IN_FLIGHT = 2;
        size_t mCurrentFrame = 0;
        size_t mImageIndex;
    };

    struct ScopedFrame{
        ScopedFrame() = delete;
        ScopedFrame(std::shared_ptr<VulkanQueue> queue);
        ~ScopedFrame();
        std::shared_ptr<VulkanQueue> Queue;
        size_t ImageIndex;
    };

    template<class T>
    class PipelineResources{
    public:
        PipelineResources() 
        {
            PCreateDescriptorSetLayout();
            PCreateDescriptorPool();
        }
        ~PipelineResources()
        {
            vkDestroyDescriptorPool(RHI::Get().mDevice,mDescriptorPool,nullptr);
            vkDestroyDescriptorSetLayout(RHI::Get().mDevice,mDescriptorSetLayout,nullptr);
        }
        const VkDescriptorSetLayout& GetDescriptorSetLayout() const {return mDescriptorSetLayout;}
        const VkDescriptorPool& GetDescriptorPool() const {return mDescriptorPool;}

    private:
        void PCreateDescriptorSetLayout(){
            std::vector<VkDescriptorSetLayoutBinding> bindings;// TODO: change to std::array
            for_each_member(T{}, [&bindings](int index, const auto& val){
                if constexpr (is_uniform_buffer<std::decay_t<decltype(val)> >::value) {
                    VkDescriptorSetLayoutBinding uboLayoutBinding{};
                    uboLayoutBinding.binding = index;
                    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    uboLayoutBinding.descriptorCount = 1;
                    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    uboLayoutBinding.pImmutableSamplers = nullptr;
                    bindings.push_back(uboLayoutBinding);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::shared_ptr<VulkanTextureSampler> >){       
                    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                    samplerLayoutBinding.binding = index;
                    samplerLayoutBinding.descriptorCount = 1;
                    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    samplerLayoutBinding.pImmutableSamplers = nullptr;
                    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(samplerLayoutBinding);
                }
            });
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();
            VK_CHECK(vkCreateDescriptorSetLayout(RHI::Get().mDevice,&layoutInfo,nullptr,&mDescriptorSetLayout),"failed to create descriptor set layout.");
        }
        void PCreateDescriptorPool(){
            std::vector<VkDescriptorPoolSize> poolSizes;
            for_each_member(T{}, [&poolSizes](int index, const auto& val){
                if constexpr (is_uniform_buffer<std::decay_t<decltype(val)> >::value) {
                    VkDescriptorPoolSize poolSize{};
                    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    poolSize.descriptorCount = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());
                    poolSizes.push_back(poolSize);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::shared_ptr<VulkanTextureSampler> >){       
                    VkDescriptorPoolSize poolSize{};
                    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    poolSize.descriptorCount = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());
                    poolSizes.push_back(poolSize);
                }
            });
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = poolSizes.size();
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());   //TODO: fix image count
            VK_CHECK(vkCreateDescriptorPool(RHI::Get().mDevice,&poolInfo,nullptr,&mDescriptorPool),"failed to create descriptor pool.");
    
        }
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkDescriptorPool mDescriptorPool;
    };
}