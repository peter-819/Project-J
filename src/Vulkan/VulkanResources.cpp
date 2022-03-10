#include <Jpch.h>
#include "VulkanResources.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"


namespace ProjectJ{
    VulkanBufferBase::VulkanBufferBase(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        mSize = size;
        mDevice = RHI::Get().mDevice;
        mPhysicalDevice = RHI::Get().mPhysicalDevice;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(mDevice,&bufferInfo,nullptr,&mBuffer),"failed to create vertex buffer.");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice,mBuffer,&memRequirements);
        auto findMemoryType = [this](uint32_t typeFilter, VkMemoryPropertyFlags properties){
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice,&memProperties);

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(mPhysicalDevice,&deviceProperties);
            JLOG_INFO("physical limits: {}", deviceProperties.limits.nonCoherentAtomSize);
            
            for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
                if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
                    return i;
                }
            }
            throw std::runtime_error("failed to find suitable memory type.");
        };

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            properties
        );
        
        JLOG_INFO("{} {}",size,allocInfo.allocationSize);
        VK_CHECK(vkAllocateMemory(mDevice,&allocInfo,nullptr,&mMemory),"failed to allocate vertex buffer memory.");
        vkBindBufferMemory(mDevice,mBuffer,mMemory,0);
    }
    VulkanBufferBase::~VulkanBufferBase(){
        vkDestroyBuffer(mDevice,mBuffer,nullptr);
        vkFreeMemory(mDevice,mMemory,nullptr);
    }
    VulkanStagingBuffer::VulkanStagingBuffer(void* data, size_t size) 
        : VulkanBufferBase(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
            void* p;
            vkMapMemory(mDevice,mMemory,0,size,0,&p);
            memcpy(p,data,(size_t)(size));
            vkUnmapMemory(mDevice,mMemory);
    }
    void VulkanStagingBuffer::CopyToBuffer(const VulkanBufferBase* dstBuffer){
        auto queue = RHI::Get().mQueue;
        queue->ExecuteDirectly([this, dstBuffer](VkCommandBuffer& commandBuffer){
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = mSize;
            vkCmdCopyBuffer(commandBuffer,mBuffer,dstBuffer->mBuffer,1,&copyRegion);
        });        
    }
    void VulkanStagingBuffer::CopyToTexture(const VulkanTexture* dstTex){
        auto queue = RHI::Get().mQueue;
        queue->ExecuteDirectly([this,dstTex](VkCommandBuffer& commandBuffer){
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                dstTex->Width,
                dstTex->Height,
                1
            };
            vkCmdCopyBufferToImage(
                commandBuffer,
                mBuffer,
                dstTex->mImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );
        });
    }
    VulkanVertexBuffer::VulkanVertexBuffer(void* data, size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanStagingBuffer stagingBuffer(data,size);
        stagingBuffer.CopyToBuffer(this);
    }

    VulkanVertexBuffer::VulkanVertexBuffer(size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
    }

    VulkanIndexBuffer::VulkanIndexBuffer(void* data, size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
        VulkanStagingBuffer stagingBuffer(data,size);
        stagingBuffer.CopyToBuffer(this);
    }

    VulkanIndexBuffer::VulkanIndexBuffer(size_t size)
        : VulkanBufferBase(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
    }

    VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, VkFormat format) 
        :Width(width), Height(height), Format(format)
    {
        auto& device = RHI::Get().mDevice;
        auto& physicalDevice = RHI::Get().mPhysicalDevice;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(Width);
        imageInfo.extent.height = static_cast<uint32_t>(Height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0; // Optional
        VK_CHECK(vkCreateImage(device,&imageInfo,nullptr,&mImage),"failed to create image!");
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, mImage, &memRequirements);
        
        auto findMemoryType = [physicalDevice](uint32_t typeFilter, VkMemoryPropertyFlags properties){
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice,&memProperties);
            for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
                if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
                    return i;
                }
            }
            throw std::runtime_error("failed to find suitable memory type.");
        };

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &mMemory),"failed to allocate image memory!");
        vkBindImageMemory(device, mImage, mMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = mImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &mView),"failed to create image view");

    }
    VulkanTexture::~VulkanTexture(){  
        vkDestroyImageView(RHI::Get().mDevice, mView, nullptr);
        vkDestroyImage(RHI::Get().mDevice, mImage, nullptr);
        vkFreeMemory(RHI::Get().mDevice, mMemory, nullptr);
    }
    void VulkanTexture::LayoutTransition(VkImageLayout oldLayout, VkImageLayout newLayout){
        auto queue = RHI::Get().mQueue;
        queue->ExecuteDirectly([=](VkCommandBuffer& commandBuffer){
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = mImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        });
    }


    VulkanSampler::VulkanSampler(const VulkanSamplerDesc& desc){
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = desc.magFilter;
        samplerInfo.minFilter = desc.minFilter;
        samplerInfo.addressModeU = desc.u;
        samplerInfo.addressModeV = desc.v;
        samplerInfo.addressModeW = desc.w;
        samplerInfo.anisotropyEnable = VK_TRUE;
        {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(RHI::Get().mPhysicalDevice, &properties);
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        }
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VK_CHECK(vkCreateSampler(RHI::Get().mDevice, &samplerInfo, nullptr, &mSampler),"failed to create sampler");
    }
    VulkanSampler::~VulkanSampler(){
        vkDestroySampler(RHI::Get().mDevice, mSampler, nullptr);
    }


    VulkanTextureSampler::VulkanTextureSampler(uint32_t width,uint32_t height, VkFormat format, const VulkanSamplerDesc& desc, VkShaderStageFlags stageBit)
        :VulkanTexture(width,height,format),VulkanSampler(desc), mStageBit(stageBit){

    }    
    VkDescriptorImageInfo VulkanTextureSampler::GetImageInfo() const {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = mView;
        imageInfo.sampler = mSampler;
        return imageInfo;
    }

    
    std::shared_ptr<VulkanTexture> TextureLoader::CreateTexFromPath(const std::string& path){
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        auto stagingBuffer = std::make_shared<VulkanStagingBuffer>(pixels,imageSize);
        
        auto texture = std::make_shared<VulkanTexture>(texWidth,texHeight, VK_FORMAT_R8G8B8A8_SRGB);// TODO
        
        texture->LayoutTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        stagingBuffer->CopyToTexture(texture.get());
        texture->LayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        stbi_image_free(pixels);
        return texture;
    }

    std::shared_ptr<VulkanTextureSampler> TextureLoader::CreateTexSamplerFromPath(const std::string& path, const VulkanSamplerDesc& desc, VkShaderStageFlags stageBit){
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        auto stagingBuffer = std::make_shared<VulkanStagingBuffer>(pixels,imageSize);
        
        auto texture = std::make_shared<VulkanTextureSampler>(texWidth,texHeight, VK_FORMAT_R8G8B8A8_SRGB, desc, stageBit);// TODO
        
        texture->LayoutTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        stagingBuffer->CopyToTexture(texture.get());
        texture->LayoutTransition(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        stbi_image_free(pixels);
        return texture;
    }
}