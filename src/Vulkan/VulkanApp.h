#pragma once
#include "VulkanInclude.h"
#include <optional>

namespace ProjectJ{
    struct VulkanConfig{
        bool enableValidationLayer;
        GLFWwindow* window;
    };
    class VulkanRHI{
    public:
        VulkanRHI(const VulkanConfig& config);
        ~VulkanRHI();
        void Draw();
    private:
        void Init();
        void Cleanup();
    
    private:
        void PCreateInstance();
        bool PCheckValidationLayer();
        void PSetupDebugMessenger();
        void PCreateSurface();
        void PPickPhysicalDevice();
        void PCreateLogicalDevice();
        void PCreateSwapChain();
        void PCreateImageViews();
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
        void HCreateBuffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& bufferMemory
        );
        void HCopyBuffer(VkBuffer srcBuffer,VkBuffer dstBuffer,VkDeviceSize size);
    private:
        VulkanConfig mConfig;
        VkInstance mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;
        VkPhysicalDevice mPhysicalDevice;
        VkDevice mDevice;
        VkQueue mGraphicQueue;
        VkQueue mPresentQueue;
        VkSurfaceKHR mSurface;
        VkSwapchainKHR mSwapChain;
        std::vector<VkImage> mSwapChainImages;
        VkFormat mSwapChainImageFormat;
        VkExtent2D mSwapChainExtent;
        std::vector<VkImageView> mSwapChainImageViews;
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkPipelineLayout mPipelineLayout;
        VkRenderPass mRenderPass;
        VkPipeline mGraphicsPipeline;
        VkCommandPool mCommandPool;
        VkBuffer mVertexBuffer;
        VkDeviceMemory mVertexBufferMemory;
        VkBuffer mIndexBuffer;
        VkDeviceMemory mIndexBufferMemory;
        VkDescriptorPool mDescriptorPool;
        std::vector<VkDescriptorSet> mDescriptorSets;

        std::vector<VkBuffer> mUniformBuffers;
        std::vector<VkDeviceMemory> mUniformBufferMemorys;

        std::vector<VkSemaphore> mImageAvailableSemaphores;
        std::vector<VkSemaphore> mRenderFinishedSemaphores;
        std::vector<VkFence> mInFlightFences;
        std::vector<VkFence> mImagesInFlight;
        size_t mCurrentFrame = 0;
    
        std::vector<VkCommandBuffer> mCommandBuffers;
        std::vector<VkFramebuffer> mSwapChainFramebuffers;
        const std::vector<const char*> mValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> mDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        const int MAX_FRAMES_IN_FLIGHT = 2;
        struct QueueFamilyIndices{
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            inline bool IsComplete() const {
                return graphicsFamily.has_value()
                    && presentFamily.has_value();
            }
        }mQueueFamilyIndices;
        struct SwapChainSupportDetails{
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        }mSwapChainSupportDetails;

        struct Vertex{
            glm::vec2 pos;
            glm::vec3 color;
        };
        const std::vector<Vertex> vertices = {  
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };
        const std::vector<uint16_t> indices = {
            0,1,2,2,3,0
        };
        struct UniformBufferObject{
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };
    };
}