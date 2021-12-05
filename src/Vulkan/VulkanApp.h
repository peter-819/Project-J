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
    private:
        void Init();
        void Cleanup();
    
    private:
        void PCreateInstance();
        bool pCheckValidationLayer();
        std::vector<const char*> PGetRequiredExtensions();
        VkDebugUtilsMessengerCreateInfoEXT PPopulateDebugMessengerCreateInfo() const;
        void PSetupDebugMessenger();
        void PCreateSurface();
        void PPickPhysicalDevice();
        void PCreateLogicalDevice();
        void PCreateSwapChain();
        void PCreateImageViews();

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
        const std::vector<const char*> mValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> mDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
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
    };
}