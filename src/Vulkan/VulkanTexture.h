#pragma once
#include <string>

namespace ProjectJ{

    class VulkanTexture{
        friend class TextureLoader;
        friend class VulkanStagingBuffer;
        friend class VulkanSampler;
    public:
        VulkanTexture(uint32_t width,uint32_t height, VkFormat format);
        ~VulkanTexture();
        void LayoutTransition(VkImageLayout oldLayout, VkImageLayout newLayout);
    private:
        uint32_t Width;
        uint32_t Height;
        VkFormat Format;
    private:
        VkImage mImage;
        VkDeviceMemory mMemory;
        VkImageView mView;
    };

    class TextureLoader{
    public:
        static std::shared_ptr<VulkanTexture> CreateFromPath(const std::string& path);
    };

    struct VulkanSamplerDesc{
        VkFilter minFilter;
        VkFilter magFilter;
        VkSamplerAddressMode u,v,w;
    };
    class VulkanSampler{
    public:
        VulkanSampler(const VulkanSamplerDesc& desc);
        ~VulkanSampler();
        VkDescriptorImageInfo GetImageInfo(VulkanTexture* texture) const;
    private:
        VkSampler mSampler;
    };
}