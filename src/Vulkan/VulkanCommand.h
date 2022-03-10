#pragma once
#include "VulkanInclude.h"
#include "VulkanDescs.h"
#include "VulkanShader.h"
#include "core/Reflection.hpp"

namespace ProjectJ{
    class ResourceManager{
    public:
        template<class TUniformBuffer>
        void AllocDynamicUniformBuffer(const std::string& name);

        template<class TUniformBuffer>
        void AllocStaticUniformBuffer(const std::string& name);

        static std::shared_ptr<VulkanTexture> CreateTexFromPath(const std::string& path);
        static std::shared_ptr<VulkanTextureSampler> CreateTexSamplerFromPath(const std::string& path, const VulkanSamplerDesc& desc, VkShaderStageFlags stageBit);
        
    private:
    };

    class VulkanCommandBuffer{
    public:
        void BeginRenderPass();
        void BindShader(VulkanShaderBase* shader){
            mShader = shader;
        }
        template<class TShader>
        void SetShaderParam(ShaderParam<TShader> param){
            assert(dynamic_cast<TShader*>(mShader));
            
        }
        void BindVertexBuffer();
        void BindIndexBuffer();
        void DrawIndexed();
        void EndRenderPass();

    private:
        VkCommandBuffer mCommandBuffer;
        VulkanShaderBase* mShader;
    };
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
}