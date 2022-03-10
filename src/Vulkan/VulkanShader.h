#pragma once
#include "VulkanInclude.h"
#include "core/Reflection.hpp"
#include "VulkanResources.h"
#include "core/RHI.h"
#include <memory>

namespace ProjectJ{
    template<class TShader> struct ShaderParam;

    class VulkanShaderBase{
    public:
        virtual ~VulkanShaderBase(){}
        virtual const VkDescriptorSetLayout& GetDescriptorSetLayout() const = 0;
        virtual const VkDescriptorPool& GetDescriptorPool() const = 0;
    };

    template<class TShader>
    class VulkanShader : public VulkanShaderBase {
        using Param = typename ShaderParam<TShader>;
    public:        
        VulkanShader() 
        {
            PCreateDescriptorSetLayout();
            PCreateDescriptorPool();
        }
        virtual ~VulkanShader()
        {
            vkDestroyDescriptorPool(RHI::Get().mDevice,mDescriptorPool,nullptr);
            vkDestroyDescriptorSetLayout(RHI::Get().mDevice,mDescriptorSetLayout,nullptr);
        }
        virtual const VkDescriptorSetLayout& GetDescriptorSetLayout() const {return mDescriptorSetLayout;}
        virtual const VkDescriptorPool& GetDescriptorPool() const {return mDescriptorPool;}
    private:
        void PCreateDescriptorSetLayout(){
            std::vector<VkDescriptorSetLayoutBinding> bindings;// TODO: change to std::array
            for_each_member(Param{}, [&bindings](int index, const auto& val){
                if constexpr (is_uniform_buffer<std::decay_t<decltype(val)> >::value) {
                    VkDescriptorSetLayoutBinding uboLayoutBinding{};
                    uboLayoutBinding.binding = index;
                    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    uboLayoutBinding.descriptorCount = 1;
                    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                    uboLayoutBinding.pImmutableSamplers = nullptr;
                    bindings.push_back(uboLayoutBinding);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::shared_ptr<VulkanTextureSampler> >){       
                    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
                    samplerLayoutBinding.binding = index;
                    samplerLayoutBinding.descriptorCount = 1;
                    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    samplerLayoutBinding.pImmutableSamplers = nullptr;
                    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(samplerLayoutBinding);
                }
            });
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = bindings.size();
            layoutInfo.pBindings = bindings.data();
            VK_CHECK(vkCreateDescriptorSetLayout(RHI::Get().mDevice,&layoutInfo,nullptr,&mDescriptorSetLayout),"failed to create descriptor set layout.");
        }
        void PCreateDescriptorPool(){
            std::vector<VkDescriptorPoolSize> poolSizes;
            for_each_member(Param{}, [&poolSizes](int index, const auto& val){
                if constexpr (is_uniform_buffer<std::decay_t<decltype(val)> >::value) {
                    VkDescriptorPoolSize poolSize{};
                    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    poolSize.descriptorCount = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());
                    poolSizes.push_back(poolSize);
                }
                else if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::shared_ptr<VulkanTextureSampler> >){       
                    VkDescriptorPoolSize poolSize{};
                    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    poolSize.descriptorCount = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());
                    poolSizes.push_back(poolSize);
                }
            });
            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = poolSizes.size();
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = static_cast<uint32_t>(RHI::Get().mSwapChain->GetImageCount());   //TODO: fix image count
            VK_CHECK(vkCreateDescriptorPool(RHI::Get().mDevice,&poolInfo,nullptr,&mDescriptorPool),"failed to create descriptor pool.");
        }
    private:
        VkDescriptorSetLayout mDescriptorSetLayout;
        VkDescriptorPool mDescriptorPool;
    };
    
    std::false_type is_shader_impl(...);
    template<typename TShader> 
    std::true_type is_shader_impl(VulkanShader<TShader> const volatile&);
    template<typename T> 
    using is_shader = decltype(is_shader_impl(std::declval<T&>()));


    class TestShader : public VulkanShader<TestShader>{
    public:
    };
    
    template<> 
    struct ShaderParam<TestShader> {
        std::shared_ptr<VulkanDynamicUniformBuffer<UniformBufferObject> > uniformBuffer;
        std::shared_ptr<VulkanTextureSampler> textureSampler;
    };
}