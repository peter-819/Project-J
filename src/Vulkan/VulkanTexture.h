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
    protected:
        VkImage mImage;
        VkDeviceMemory mMemory;
        VkImageView mView;
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
    protected:
        VkSampler mSampler;
    };

    class VulkanTextureSampler : public VulkanTexture, public VulkanSampler {
    public:
        VulkanTextureSampler(uint32_t width,uint32_t height, VkFormat format, const VulkanSamplerDesc& desc);

        VkDescriptorImageInfo GetImageInfo() const;
    };

    
    class TextureLoader{
    public:
        static std::shared_ptr<VulkanTexture> CreateTexFromPath(const std::string& path);
        static std::shared_ptr<VulkanTextureSampler> CreateTexSamplerFromPath(const std::string& path, const VulkanSamplerDesc& desc);

    };
}