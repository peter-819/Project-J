#pragma once
#include "VulkanDescs.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffers.h"
#include "VulkanPSO.h"
#include <optional>

namespace ProjectJ{
    class RHI;
    struct VulkanConfig{
        bool enableValidationLayer;
        J_WINDOW_HANDLE window;
    };
    class VulkanRHI{
        friend class VulkanBufferBase;
        friend class VulkanStagingBuffer;
    public:
        VulkanRHI(const VulkanConfig& config);
        ~VulkanRHI();
        void Draw();
    public:
        void Init();
        void Cleanup();

    private:
        void PCreateInstance();
        bool PCheckValidationLayer();
        void PSetupDebugMessenger();
        void PCreateSurface();
        void PPickPhysicalDevice();
        void PCreateLogicalDevice();
        void PCreateRenderPass();
        void PCreateDescriptorSetLayout();
        void PCreateGraphicsPipeline();
        void PCreateFramebuffers();
        void PCreateCommandPool();
        void PCreateVertexBuffer();
        void PCreateIndexBuffer();
        void PCreateUniformBuffer();
        void PCreateDescriptorPool();
        void PCreateDescriptorSet();
        void PCreateCommandBuffers();
        void PCreateSyncObjects();

        std::vector<const char*> HGetRequiredExtensions();
        VkDebugUtilsMessengerCreateInfoEXT HPopulateDebugMessengerCreateInfo() const;
    private:
        VulkanConfig mConfig;
        VkInstance mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;
        VkPhysicalDevice mPhysicalDevice;
        VkDevice mDevice;
        VkQueue mGraphicQueue;
        VkQueue mPresentQueue;
        VkSurfaceKHR mSurface;
        
        std::vector<VkFramebuffer> mSwapChainFramebuffers;
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkPipelineLayout mPipelineLayout;
        VkRenderPass mRenderPass;
        // VkPipeline mGraphicsPipeline;
        VkCommandPool mCommandPool;
        std::unique_ptr<VulkanVertexBuffer> mVertexBuffer;
        std::unique_ptr<VulkanIndexBuffer> mIndexBuffer;
        VkDescriptorPool mDescriptorPool;
        std::vector<VkDescriptorSet> mDescriptorSets;

        // std::vector<VkBuffer> mUniformBuffers;
        std::vector<std::unique_ptr<VulkanUniformBuffer<UniformBufferObject> > > mUniformBuffers;
        
        // std::vector<VkDeviceMemory> mUniformBufferMemorys;
        std::vector<VkDeviceMemory> mPSUniformBufferMemorys;

        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        std::vector<VkFence> mInFlightFences;
        std::vector<VkFence> mImagesInFlight;
        size_t mCurrentFrame = 0;

        std::vector<VkCommandBuffer> mCommandBuffers;
        const std::vector<const char*> mValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> mDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        const int MAX_FRAMES_IN_FLIGHT = 2;
        QueueFamilyIndices mQueueFamilyIndices;
        SwapChainSupportDetails mSwapChainSupportDetails;

        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };
        const std::vector<uint16_t> indices = {
            0,1,2,2,3,0
        };
    private:
        std::shared_ptr<VulkanSwapChain> mSwapChain;
        std::shared_ptr<VulkanPSO> mGraphicPipeline;
    };

}