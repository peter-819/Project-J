#pragma once
#include "VulkanInclude.h"

namespace ProjectJ{

    struct VulkanSwapChainDesc{
        J_WINDOW_HANDLE window;
        uint32_t width;
        uint32_t height;
    };
    struct SwapChainSupportDetails{
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    struct QueueFamilyIndices{
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        inline bool IsComplete() const {
            return graphicsFamily.has_value()
                && presentFamily.has_value();
        }
    };
    
    struct Vertex{
        glm::vec2 pos;
        glm::vec3 color;
    };
    struct UniformBufferObject{
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };    
    struct PSUniformBufferObject{
        glm::vec3 color;
    };
}