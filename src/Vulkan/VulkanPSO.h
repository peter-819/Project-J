#pragma once
#include "VulkanInclude.h"

namespace ProjectJ{
    struct VulkanPSODesc{
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        
        uint32_t attributeStride;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        
        VkExtent2D extent;
        VkPipelineLayout pipelineLayout;
    };
    class VulkanPSO{
    public:
        VulkanPSO(VkRenderPass renderPass, VkDevice device, const VulkanPSODesc& desc);
        ~VulkanPSO();
        void Bind(VkCommandBuffer& commandBuffer);
    private:
        VkDevice mDevice;
        VkPipeline mPipeline;
    };
}