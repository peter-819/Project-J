#pragma once
#include "VulkanDescs.h"
#include "VulkanSwapChain.h"
#include "VulkanBuffers.h"
#include "VulkanPSO.h"
#include "VulkanTexture.h"
#include "VulkanCommand.h"
#include "VulkanShader.h"
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
        friend class VulkanTexture;
        friend class VulkanQueue;
        friend class VulkanSampler;
        template<typename> friend class VulkanShader;
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
        void PCreateGraphicsPipeline();
        void PCreateFramebuffers();
        void PCreateCommandPool();
        void PCreateVertexBuffer();
        void PCreateIndexBuffer();
        void PCreateUniformBuffer();
        void PCreateTextureSampler();
        void PCreateDescriptorSet();
        void PPrepareCommandBuffers();

        std::vector<const char*> HGetRequiredExtensions();
        VkDebugUtilsMessengerCreateInfoEXT HPopulateDebugMessengerCreateInfo() const;
    private:
        VulkanConfig mConfig;
        VkInstance mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;
        VkPhysicalDevice mPhysicalDevice;
        VkDevice mDevice;
        VkSurfaceKHR mSurface;
        
        std::vector<VkFramebuffer> mSwapChainFramebuffers;
        VkPipelineLayout mPipelineLayout;
        VkRenderPass mRenderPass;
        std::unique_ptr<VulkanVertexBuffer> mVertexBuffer;
        std::unique_ptr<VulkanIndexBuffer> mIndexBuffer;
        std::vector<VkDescriptorSet> mDescriptorSets;

        std::vector<std::unique_ptr<VulkanUniformBuffer<UniformBufferObject> > > mUniformBuffers;
        std::shared_ptr<VulkanTextureSampler> mTextureSampler;
        std::unique_ptr<TestShader> mTestShader;

        const std::vector<const char*> mValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> mDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        QueueFamilyIndices mQueueFamilyIndices;
        SwapChainSupportDetails mSwapChainSupportDetails;

        const std::vector<Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
        };
        const std::vector<uint16_t> indices = {
            0,1,2,2,3,0
        };
    private:
        std::shared_ptr<VulkanSwapChain> mSwapChain;
        std::shared_ptr<VulkanPSO> mGraphicPipeline;
        std::shared_ptr<VulkanQueue> mQueue;
        std::shared_ptr<VulkanCommandBuffer> mTestCommandBuffer;
    };

}