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

    private:
        VulkanConfig mConfig;
        VkInstance mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;
        VkPhysicalDevice mPhysicalDevice;
        VkDevice mDevice;
        VkQueue mGraphicQueue;
        VkQueue mPresentQueue;
        VkSurfaceKHR mSurface;
        const std::vector<const char*> mValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };
        struct QueueFamilyIndices{
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;
            inline bool IsComplete() const {
                return graphicsFamily.has_value()
                    && presentFamily.has_value();
            }
        }mQueueFamilyIndices;
        
    };
}