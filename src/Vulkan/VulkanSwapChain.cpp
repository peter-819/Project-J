#include <Jpch.h>
#include "VulkanSwapChain.h"

namespace ProjectJ{
    VulkanSwapChain::VulkanSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices queueFamilyIndices, const VulkanSwapChainDesc& desc)
        : mDesc(desc), mDevice(device), mPhysicalDevice(physicalDevice), mSurface(surface), mQueueFamilyIndices(queueFamilyIndices){
        PCreateSwapChain();
        PCreateImageViews();
    }
    VulkanSwapChain::~VulkanSwapChain(){
        for(auto imageView : mSwapChainImageViews){
            vkDestroyImageView(mDevice,imageView,nullptr);
        }
        vkDestroySwapchainKHR(mDevice,mSwapChain,nullptr);
    }
    uint32_t VulkanSwapChain::AcquireNextImage(VkSemaphore semaphore){
        vkAcquireNextImageKHR(mDevice,mSwapChain,INT64_MAX,semaphore,VK_NULL_HANDLE,&mImageIndex);
        return mImageIndex;
    }
    void VulkanSwapChain::Present(VkQueue presentQueue, VkSemaphore waitSemaphore){
        VkSemaphore waitSemaphores[] = {waitSemaphore};
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;
        VkSwapchainKHR swapChains[] = {mSwapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &mImageIndex;
        presentInfo.pResults = nullptr; // Optional
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }
    void VulkanSwapChain::PCreateSwapChain(){
        auto querySwapChainSupport = [this](VkPhysicalDevice device){
            SwapChainSupportDetails details;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,mSurface,&details.capabilities);
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device,mSurface,&formatCount,nullptr);
            if(formatCount != 0){
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device,mSurface,&formatCount,details.formats.data());
            }
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device,mSurface,&presentModeCount,nullptr);
            if(presentModeCount != 0){
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device,mSurface,&presentModeCount,details.presentModes.data());
            }
            return details;
        };
        mSwapChainSupportDetails = querySwapChainSupport(mPhysicalDevice);
        
        auto chooseSwapSurfaceFormat = [](const std::vector<VkSurfaceFormatKHR>& availableFormats){
            for(const auto& aFormat : availableFormats){
                if(aFormat.format == VK_FORMAT_B8G8R8A8_SRGB && aFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
                    return aFormat;
                }
            }
            return availableFormats[0];
        };
        auto chooseSwapPresentMode = [](const std::vector<VkPresentModeKHR>& availablePresentModes){
            for(const auto& aMode : availablePresentModes){
                if(aMode == VK_PRESENT_MODE_MAILBOX_KHR){
                    return aMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        };
        auto chooseSwapExtent = [this](const VkSurfaceCapabilitiesKHR& capabilities){
            if(capabilities.currentExtent.width != UINT32_MAX){
                return capabilities.currentExtent;
            }
            else{
                int width,height;
                glfwGetFramebufferSize(mDesc.window,&width,&height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };
                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                return actualExtent;
            }
        };
        
        auto surfaceFormat = chooseSwapSurfaceFormat(mSwapChainSupportDetails.formats);
        auto presentMode = chooseSwapPresentMode(mSwapChainSupportDetails.presentModes);
        auto extent = chooseSwapExtent(mSwapChainSupportDetails.capabilities);

        uint32_t imageCount = mSwapChainSupportDetails.capabilities.minImageCount + 1;
        if (mSwapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > mSwapChainSupportDetails.capabilities.maxImageCount) {
            imageCount = mSwapChainSupportDetails.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = mSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        uint32_t queueFamilyIndices[] = {mQueueFamilyIndices.graphicsFamily.value(), mQueueFamilyIndices.presentFamily.value()};

        if (mQueueFamilyIndices.graphicsFamily != mQueueFamilyIndices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } 
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = mSwapChainSupportDetails.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSwapchainKHR(mDevice,&createInfo,nullptr,&mSwapChain),"failed to create swap chain.");

        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
        mSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

        mSwapChainImageFormat = surfaceFormat.format;
        mSwapChainExtent = extent;
    }
    void VulkanSwapChain::PCreateImageViews(){
        mSwapChainImageViews.resize(mSwapChainImages.size());
        for(size_t i = 0; i < mSwapChainImages.size(); i++){
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = mSwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = mSwapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            VK_CHECK(vkCreateImageView(mDevice,&createInfo,nullptr,&mSwapChainImageViews[i]),"failed to create image views.");
        }
    }
}