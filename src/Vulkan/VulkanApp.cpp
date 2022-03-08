#include "Jpch.h"
#include "VulkanApp.h"
#include "stb_image.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        // Message is important enough to show
    }
    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
namespace ProjectJ{
    VulkanRHI::VulkanRHI(const VulkanConfig& config)
        :mConfig(config) {
    }
    VulkanRHI::~VulkanRHI(){
    }
    void VulkanRHI::Draw(){
        ScopedFrame frame(mQueue);

        auto updateUniformBuffer = [this](UniformBufferObject& ubo) {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), mSwapChain->GetExtent().width / (float) mSwapChain->GetExtent().height, 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;
        };
        mUniformBuffers[frame.ImageIndex]->ModifyAndSync(updateUniformBuffer);
    }
    void VulkanRHI::Init(){
        PCreateInstance();
        PSetupDebugMessenger();
        PCreateSurface();
        PPickPhysicalDevice();
        PCreateLogicalDevice();
        VulkanSwapChainDesc desc{};
        desc.window = mConfig.window;
        mSwapChain = std::make_shared<VulkanSwapChain>(mDevice,mPhysicalDevice,mSurface,mQueueFamilyIndices,desc);
        mPipelineResource = std::make_unique<PipelineResources<Shader_Param> >();
        PCreateRenderPass();
        PCreateGraphicsPipeline();
        PCreateFramebuffers();
        mQueue = std::make_shared<VulkanQueue>();
        PCreateVertexBuffer();
        PCreateIndexBuffer();
        PCreateUniformBuffer();
        PCreateTextureSampler();
        PCreateDescriptorSet();
        PPrepareCommandBuffers();
    }
    void VulkanRHI::Cleanup(){
        vkDeviceWaitIdle(mDevice);

        mIndexBuffer.reset();
        mVertexBuffer.reset();
        mTextureSampler.reset();
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            mUniformBuffers[i].reset();
        }
        mPipelineResource.reset();
        // vkDestroyDescriptorPool(mDevice,mDescriptorPool,nullptr);
        // vkDestroyDescriptorSetLayout(mDevice,mDescriptorSetLayout,nullptr);
        
        mQueue.reset();
        for(auto framebuffer : mSwapChainFramebuffers){
            vkDestroyFramebuffer(mDevice,framebuffer,nullptr);
        }
        mGraphicPipeline.reset();
        vkDestroyPipelineLayout(mDevice,mPipelineLayout,nullptr);
        vkDestroyRenderPass(mDevice,mRenderPass,nullptr);
        mSwapChain.reset();
        vkDestroyDevice(mDevice,nullptr);
        if (mConfig.enableValidationLayer) {
            DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(mInstance,mSurface,nullptr);
        vkDestroyInstance(mInstance, nullptr);
    }
    
    void VulkanRHI::PCreateInstance(){
        if(mConfig.enableValidationLayer && !PCheckValidationLayer()) {
            throw std::runtime_error("validation layer not available");
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Project-J";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto requiredExtensions = HGetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        if(mConfig.enableValidationLayer){
            createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
            createInfo.ppEnabledLayerNames = mValidationLayers.data();

            auto dMessengerInfo = HPopulateDebugMessengerCreateInfo();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&dMessengerInfo;
        }
        else{
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        VK_CHECK(vkCreateInstance(&createInfo,nullptr,&mInstance),"failed to create instance.");

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr,&extensionCount,extensions.data());
        std::cout << "available extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }
    bool VulkanRHI::PCheckValidationLayer(){
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount,nullptr);
        std::vector<VkLayerProperties> avaliableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount,avaliableLayers.data());
        for(const char* name : mValidationLayers){
            bool found = false;
            for(const auto layer : avaliableLayers){
                if(strcmp(name,layer.layerName) == 0){
                    found = true;
                    break;
                }
            }
            if(!found) {
                return false;
            }
        }
        return true;
    }
    std::vector<const char*> VulkanRHI::HGetRequiredExtensions(){
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions,glfwExtensions + glfwExtensionCount);
        if(mConfig.enableValidationLayer){
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }
    VkDebugUtilsMessengerCreateInfoEXT VulkanRHI::HPopulateDebugMessengerCreateInfo() const{
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional
        return createInfo;
    }
    void VulkanRHI::PSetupDebugMessenger(){
        if(!mConfig.enableValidationLayer) return;
        auto createInfo = HPopulateDebugMessengerCreateInfo();
        VK_CHECK(CreateDebugUtilsMessengerEXT(mInstance,&createInfo,nullptr,&mDebugMessenger),"failed to setup debug messenger.");
    }
    void VulkanRHI::PCreateSurface(){
        // VkWin32SurfaceCreateInfoKHR createInfo{};
        // createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        // createInfo.hwnd = glfwGetWin32Window(mConfig.window);
        // createInfo.hinstance = GetModuleHandle(nullptr);
        // VK_CHECK(vkCreateWin32SurfaceKHR(mInstance,&createInfo,nullptr,&mSurface),"failed to create window surface.");
        VK_CHECK(glfwCreateWindowSurface(mInstance,mConfig.window,nullptr,&mSurface),"failed to create window surface.");
    }
    void VulkanRHI::PPickPhysicalDevice(){
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(mInstance,&deviceCount,nullptr);
        if(deviceCount == 0){
            throw std::runtime_error("failed to find GPUs with vulkan support.");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mInstance,&deviceCount,devices.data());

        auto findQueueFamilies = [this](VkPhysicalDevice device){
            QueueFamilyIndices indices;
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount,queueFamilies.data());
            for(uint32_t i = 0;i<queueFamilyCount;i++){
                const auto& queueFamily = queueFamilies[i];
                if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    indices.graphicsFamily = i;
                }
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device,i,mSurface,&presentSupport);
                if(presentSupport){ 
                    indices.presentFamily = i;
                }
                if(indices.IsComplete()){
                    break;
                }
            }
            return indices;
        };
        auto checkDeviceExtensions = [this](VkPhysicalDevice device){
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,nullptr);
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device,nullptr,&extensionCount,availableExtensions.data());
            std::set<std::string> requiredExtensions(mDeviceExtensions.begin(),mDeviceExtensions.end());
            for(const auto& extension : availableExtensions){
                requiredExtensions.erase(extension.extensionName);
            }
            return requiredExtensions.empty();
        };
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
        auto isDeviceSuitable = [querySwapChainSupport, checkDeviceExtensions, findQueueFamilies](VkPhysicalDevice device)->bool{
            auto indices = findQueueFamilies(device);
            if(!indices.IsComplete()) {
                return false;
            }
            if(!checkDeviceExtensions(device)){
                return false;
            }
            auto details = querySwapChainSupport(device);
            if(details.formats.empty() || details.presentModes.empty()) {
                return false;
            }
            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
            if(!supportedFeatures.samplerAnisotropy){
                return false;
            }
            return true;
        };

        auto rateDeviceSuitability = [isDeviceSuitable](VkPhysicalDevice device)->uint32_t{
            if(!isDeviceSuitable(device)){
                return 0;
            }
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device,&deviceProperties);
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceFeatures(device,&deviceFeatures);
            uint32_t score = 0;
            if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                score += 1000;
            }
            
            score += deviceProperties.limits.maxImageDimension2D;    
            if (!deviceFeatures.geometryShader) {
                return 0;
            }
            return score;
        };
        std::multimap<int, VkPhysicalDevice> candidates;
        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }
    
        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0) {
            mPhysicalDevice = candidates.rbegin()->second;
        } else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
        mQueueFamilyIndices = findQueueFamilies(mPhysicalDevice);
        mSwapChainSupportDetails = querySwapChainSupport(mPhysicalDevice);
    }
    void VulkanRHI::PCreateLogicalDevice(){
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            mQueueFamilyIndices.graphicsFamily.value(),
            mQueueFamilyIndices.presentFamily.value()
        };
        float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies){
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily.value();
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(mDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = mDeviceExtensions.data();

        if(mConfig.enableValidationLayer){
            createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
            createInfo.ppEnabledLayerNames = mValidationLayers.data();
        }
        else{
            createInfo.enabledLayerCount = 0;
        }
        VK_CHECK(vkCreateDevice(mPhysicalDevice,&createInfo,nullptr,&mDevice),"failed to create logical device.");
    }
    void VulkanRHI::PCreateRenderPass(){
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = mSwapChain->GetFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        VK_CHECK(vkCreateRenderPass(mDevice,&renderPassInfo,nullptr,&mRenderPass),"failed to create render pass.");
    }
    void VulkanRHI::PCreateGraphicsPipeline(){
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mPipelineResource->GetDescriptorSetLayout();
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        VK_CHECK(vkCreatePipelineLayout(mDevice,&pipelineLayoutInfo,nullptr,&mPipelineLayout),"failed to create pipeline layout");

        VulkanPSODesc desc{};
        desc.vertexShaderPath = "shaders/vert.spv";
        desc.fragmentShaderPath = "shaders/frag.spv";
        desc.attributeStride = sizeof(Vertex);
        desc.attributeDescriptions.resize(3);
        desc.attributeDescriptions[0].binding = 0;
        desc.attributeDescriptions[0].location = 0;
        desc.attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        desc.attributeDescriptions[0].offset = offsetof(Vertex,pos);

        desc.attributeDescriptions[1].binding = 0;
        desc.attributeDescriptions[1].location = 1;
        desc.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc.attributeDescriptions[1].offset = offsetof(Vertex,color);
        
        desc.attributeDescriptions[2].binding = 0;
        desc.attributeDescriptions[2].location = 2;
        desc.attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        desc.attributeDescriptions[2].offset = offsetof(Vertex,texCoord);

        desc.extent = mSwapChain->GetExtent();
        desc.pipelineLayout = mPipelineLayout;
        mGraphicPipeline = std::make_shared<VulkanPSO>(mRenderPass,mDevice,desc);
    }
    void VulkanRHI::PCreateFramebuffers(){
        mSwapChainFramebuffers.resize(mSwapChain->GetImageCount());
        for(size_t i = 0; i< mSwapChain->GetImageCount(); i++){
            VkImageView attachments[] = {
                mSwapChain->GetImageViews()[i]
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = mRenderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = mSwapChain->GetExtent().width;
            framebufferInfo.height = mSwapChain->GetExtent().height;
            framebufferInfo.layers = 1;
            VK_CHECK(vkCreateFramebuffer(mDevice,&framebufferInfo,nullptr,&mSwapChainFramebuffers[i]),"failed to create framebuffer.");
            
        }
    }
    void VulkanRHI::PCreateVertexBuffer(){
        mVertexBuffer = std::make_unique<VulkanVertexBuffer>((void*)vertices.data(), sizeof(vertices[0]) * vertices.size());
    }
    void VulkanRHI::PCreateIndexBuffer(){
        mIndexBuffer = std::make_unique<VulkanIndexBuffer>((void*)indices.data(), sizeof(indices[0]) * indices.size());
    }
    void VulkanRHI::PCreateUniformBuffer(){
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        mUniformBuffers.resize(mSwapChain->GetImageCount());
        
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            mUniformBuffers[i] = std::make_unique<VulkanUniformBuffer<UniformBufferObject> >(VK_SHADER_STAGE_VERTEX_BIT);
        }
    }
    void VulkanRHI::PCreateTextureSampler(){
        VulkanSamplerDesc desc{};
        desc.magFilter = VK_FILTER_LINEAR;
        desc.minFilter = VK_FILTER_LINEAR;
        desc.u = desc.v = desc.w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        mTextureSampler = TextureLoader::CreateTexSamplerFromPath("textures/texture.jpg", desc, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    void VulkanRHI::PCreateDescriptorSet(){
        std::vector<VkDescriptorSetLayout> layouts(mSwapChain->GetImageCount(),mPipelineResource->GetDescriptorSetLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mPipelineResource->GetDescriptorPool();
        allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapChain->GetImageCount());
        allocInfo.pSetLayouts = layouts.data();
        
        mDescriptorSets.resize(mSwapChain->GetImageCount());
        VK_CHECK(vkAllocateDescriptorSets(mDevice,&allocInfo,mDescriptorSets.data()),"failed to allocate descriptor sets");
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            VkDescriptorBufferInfo bufferInfo = mUniformBuffers[i]->GetBufferInfo();
            VkDescriptorImageInfo imageInfo = mTextureSampler->GetImageInfo();

            std::array<VkWriteDescriptorSet, 2> descriptorWrite{};
            
            descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[0].dstSet = mDescriptorSets[i];
            descriptorWrite[0].dstBinding = 0;
            descriptorWrite[0].dstArrayElement = 0;
            descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite[0].descriptorCount = 1;
            descriptorWrite[0].pBufferInfo = &bufferInfo;
            descriptorWrite[0].pImageInfo = nullptr; // Optional
            descriptorWrite[0].pTexelBufferView = nullptr; // Optional
            
            descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[1].dstSet = mDescriptorSets[i];
            descriptorWrite[1].dstBinding = 1;
            descriptorWrite[1].dstArrayElement = 0;
            descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite[1].descriptorCount = 1;
            descriptorWrite[1].pImageInfo = &imageInfo; // Optional
            vkUpdateDescriptorSets(mDevice, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
        }
    }
    void VulkanRHI::PPrepareCommandBuffers(){
        auto prepareFunc = [this](uint32_t index, VkCommandBuffer& commandBuffer){
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;
            VK_CHECK(vkBeginCommandBuffer(commandBuffer,&beginInfo),"failed to begin recoreding command buffer.");

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = mRenderPass;
            renderPassInfo.framebuffer = mSwapChainFramebuffers[index];
            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = mSwapChain->GetExtent();
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
       
            //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
            mGraphicPipeline->Bind(commandBuffer);

            VkBuffer vertexBuffers[] = {mVertexBuffer->mBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer,0,1,vertexBuffers,offsets);
            vkCmdBindIndexBuffer(commandBuffer,mIndexBuffer->mBuffer,0,VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,mPipelineLayout,0,1,&mDescriptorSets[index],0,nullptr);
            vkCmdDrawIndexed(commandBuffer,static_cast<uint32_t>(indices.size()),1,0,0,0);
            vkCmdEndRenderPass(commandBuffer);

            VK_CHECK(vkEndCommandBuffer(commandBuffer),"failed to record command buffer.");
        };
        mQueue->PrepareFrameCommands(prepareFunc);
    }
}
