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
}