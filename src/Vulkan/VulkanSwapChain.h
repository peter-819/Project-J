#pragma once
#include "VulkanDescs.h"
#include <map>

namespace ProjectJ{
    class VulkanSwapChain{
    public:
        VulkanSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, 
            QueueFamilyIndices queueFamilyIndices, const VulkanSwapChainDesc& desc);
        ~VulkanSwapChain();
        uint32_t GetImageCount() const {return mSwapChainImageViews.size();}
        uint32_t AcquireNextImage(VkSemaphore semaphore);
        void Present(VkQueue presentQueue, VkSemaphore waitSemaphore);
        //TODO: Remove these
        std::vector<VkImageView>& GetImageViews() {return mSwapChainImageViews;}
        VkFormat GetFormat() const {return mSwapChainImageFormat;}
        VkExtent2D GetExtent() const {return mSwapChainExtent;}
    private:
        void PCreateSwapChain();
        void PCreateImageViews();
        VkDevice mDevice;
        VkPhysicalDevice mPhysicalDevice;
        VkSurfaceKHR mSurface;

        VkSwapchainKHR mSwapChain;
        std::vector<VkImage> mSwapChainImages;
        VkFormat mSwapChainImageFormat;
        VkExtent2D mSwapChainExtent;
        std::vector<VkImageView> mSwapChainImageViews;
        struct SwapChainSupportDetails{
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        }mSwapChainSupportDetails;
        QueueFamilyIndices mQueueFamilyIndices;

        std::vector<VkFramebuffer> mSwapChainFramebuffers;
        VulkanSwapChainDesc mDesc;
        uint32_t mImageIndex = 0;
    };
}