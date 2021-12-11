#include "Jpch.h"
#include "VulkanApp.h"

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
        Init();
    }
    VulkanRHI::~VulkanRHI(){
        Cleanup();
    }
    void VulkanRHI::Draw(){
        vkWaitForFences(mDevice,1,&mInFlightFences[mCurrentFrame],VK_TRUE,UINT64_MAX);
        uint32_t imageIndex;
        //vkAcquireNextImageKHR(mDevice,mSwapChain,UINT64_MAX,mImageAvailableSemaphores[mCurrentFrame],VK_NULL_HANDLE,&imageIndex);
        imageIndex = mSwapChain->AcquireNextImage(mImageAvailableSemaphores[mCurrentFrame]);
        if(mImagesInFlight[imageIndex] != VK_NULL_HANDLE){
            vkWaitForFences(mDevice,1,&mImagesInFlight[imageIndex],VK_TRUE,UINT64_MAX);
        }
        mImagesInFlight[imageIndex] = mImagesInFlight[mCurrentFrame];
        
        auto updateUniformBuffer = [this](uint32_t currentImage){
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        
            UniformBufferObject ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), mSwapChain->GetExtent().width / (float) mSwapChain->GetExtent().height, 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;
            void* data;
            vkMapMemory(mDevice, mUniformBufferMemorys[currentImage], 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(mDevice, mUniformBufferMemorys[currentImage]);
        };
        updateUniformBuffer(imageIndex);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        vkResetFences(mDevice,1,&mInFlightFences[mCurrentFrame]);
        VK_CHECK(vkQueueSubmit(mGraphicQueue,1,&submitInfo,mInFlightFences[mCurrentFrame]),"failed to submit draw command buffer.");

        mSwapChain->Present(mPresentQueue,mRenderFinishedSemaphores[mCurrentFrame]);
        mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
        PCreateRenderPass();
        PCreateDescriptorSetLayout();
        PCreateGraphicsPipeline();
        PCreateFramebuffers();
        PCreateCommandPool();
        PCreateVertexBuffer();
        PCreateIndexBuffer();
        PCreateUniformBuffer();
        PCreateDescriptorPool();
        PCreateDescriptorSet();
        PCreateCommandBuffers();
        PCreateSyncObjects();
    }
    void VulkanRHI::Cleanup(){
        vkDeviceWaitIdle(mDevice);

        vkDestroyBuffer(mDevice,mIndexBuffer,nullptr);
        vkFreeMemory(mDevice,mIndexBufferMemory,nullptr);

        vkDestroyBuffer(mDevice,mVertexBuffer,nullptr);
        vkFreeMemory(mDevice,mVertexBufferMemory,nullptr);
        
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            vkDestroyBuffer(mDevice,mUniformBuffers[i],nullptr);
            vkFreeMemory(mDevice,mUniformBufferMemorys[i],nullptr);
        }
        vkDestroyDescriptorPool(mDevice,mDescriptorPool,nullptr);
        vkDestroyDescriptorSetLayout(mDevice,mDescriptorSetLayout,nullptr);
        
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            vkDestroySemaphore(mDevice,mRenderFinishedSemaphores[i],nullptr);
            vkDestroySemaphore(mDevice,mImageAvailableSemaphores[i],nullptr);
            vkDestroyFence(mDevice,mInFlightFences[i],nullptr);
        }
        vkDestroyCommandPool(mDevice,mCommandPool,nullptr);
        for(auto framebuffer : mSwapChainFramebuffers){
            vkDestroyFramebuffer(mDevice,framebuffer,nullptr);
        }
        vkDestroyPipeline(mDevice,mGraphicsPipeline,nullptr);
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
        vkGetDeviceQueue(mDevice,mQueueFamilyIndices.graphicsFamily.value(),0,&mGraphicQueue);
        vkGetDeviceQueue(mDevice,mQueueFamilyIndices.presentFamily.value(),0,&mPresentQueue);
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
    void VulkanRHI::PCreateDescriptorSetLayout(){
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;
        
        VK_CHECK(vkCreateDescriptorSetLayout(mDevice,&layoutInfo,nullptr,&mDescriptorSetLayout),"failed to create descriptor set layout.");
    }
    void VulkanRHI::PCreateGraphicsPipeline(){
        auto readFile = [](const std::string& filename) {
            std::ifstream file(filename,std::ios::ate | std::ios::binary);
            if(!file.is_open()){
                throw std::runtime_error("failed to open file!");
            }
            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(),fileSize);
            file.close();
            return buffer;
        };
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");
        auto createShaderModule = [this](const std::vector<char>& code){
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
            VkShaderModule shaderModule;
            VK_CHECK(vkCreateShaderModule(mDevice,&createInfo,nullptr,&shaderModule),"failed to create shader module.");
            return shaderModule;
        };
        auto vertShaderModule = createShaderModule(vertShaderCode);
        auto fragShaderModule = createShaderModule(fragShaderCode);
        
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        
        //Vertex Layout
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescription;
        attributeDescription[0].binding = 0;
        attributeDescription[0].location = 0;
        attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescription[0].offset = offsetof(Vertex,pos);

        attributeDescription[1].binding = 0;
        attributeDescription[1].location = 1;
        attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescription[1].offset = offsetof(Vertex,color);
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size()); // Optional
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data(); // Op
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;

        viewport.width = (float) mSwapChain->GetExtent().width;
        viewport.height = (float) mSwapChain->GetExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = mSwapChain->GetExtent();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        //TODO: figure out this state.
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &mDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        VK_CHECK(vkCreatePipelineLayout(mDevice,&pipelineLayoutInfo,nullptr,&mPipelineLayout),"failed to create pipeline layout");

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr; // Optional
        pipelineInfo.layout = mPipelineLayout;
        pipelineInfo.renderPass = mRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(mDevice,VK_NULL_HANDLE,1,&pipelineInfo,nullptr,&mGraphicsPipeline),"failed to create graphics pipeline.");
        
        vkDestroyShaderModule(mDevice,fragShaderModule,nullptr);
        vkDestroyShaderModule(mDevice,vertShaderModule,nullptr);

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
    void VulkanRHI::PCreateCommandPool(){
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily.value();
        poolInfo.flags = 0;
        VK_CHECK(vkCreateCommandPool(mDevice,&poolInfo,nullptr,&mCommandPool),"failed to create command pool.");
    }
    void VulkanRHI::PCreateVertexBuffer(){
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        HCreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(mDevice,stagingBufferMemory,0,bufferSize,0,&data);
        memcpy(data,vertices.data(),(size_t)(bufferSize));
        vkUnmapMemory(mDevice,stagingBufferMemory);

        HCreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            mVertexBuffer,
            mVertexBufferMemory
        );  
        HCopyBuffer(stagingBuffer,mVertexBuffer,bufferSize);
        vkDestroyBuffer(mDevice,stagingBuffer,nullptr);
        vkFreeMemory(mDevice,stagingBufferMemory,nullptr);
    }
    void VulkanRHI::PCreateIndexBuffer(){
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        HCreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(mDevice,stagingBufferMemory,0,bufferSize,0,&data);
        memcpy(data,indices.data(),(size_t)(bufferSize));
        vkUnmapMemory(mDevice,stagingBufferMemory);

        HCreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            mIndexBuffer,
            mIndexBufferMemory
        );  
        HCopyBuffer(stagingBuffer,mIndexBuffer,bufferSize);
        vkDestroyBuffer(mDevice,stagingBuffer,nullptr);
        vkFreeMemory(mDevice,stagingBufferMemory,nullptr);
    }
    void VulkanRHI::PCreateUniformBuffer(){
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        mUniformBuffers.resize(mSwapChain->GetImageCount());
        mUniformBufferMemorys.resize(mSwapChain->GetImageCount());
        
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            HCreateBuffer(
                bufferSize, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                mUniformBuffers[i],
                mUniformBufferMemorys[i]
            );
        }
    }
    void VulkanRHI::PCreateDescriptorPool(){
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(mSwapChain->GetImageCount());
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(mSwapChain->GetImageCount());
        VK_CHECK(vkCreateDescriptorPool(mDevice,&poolInfo,nullptr,&mDescriptorPool),"failed to create descriptor pool.");
    }
    void VulkanRHI::PCreateDescriptorSet(){
        std::vector<VkDescriptorSetLayout> layouts(mSwapChain->GetImageCount(),mDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = mDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapChain->GetImageCount());
        allocInfo.pSetLayouts = layouts.data();
        
        mDescriptorSets.resize(mSwapChain->GetImageCount());
        VK_CHECK(vkAllocateDescriptorSets(mDevice,&allocInfo,mDescriptorSets.data()),"failed to allocate descriptor sets");
        for(size_t i = 0; i < mSwapChain->GetImageCount(); i++){
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = mUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = mDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional
            vkUpdateDescriptorSets(mDevice, 1, &descriptorWrite, 0, nullptr);
        }
    }
    void VulkanRHI::PCreateCommandBuffers(){
        mCommandBuffers.resize(mSwapChainFramebuffers.size());
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(mDevice,&allocInfo,mCommandBuffers.data()),"failed to allocate command buffers.");
        for(size_t i = 0; i < mCommandBuffers.size(); i++){
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;
            VK_CHECK(vkBeginCommandBuffer(mCommandBuffers[i],&beginInfo),"failed to begin recoreding command buffer.");

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = mRenderPass;
            renderPassInfo.framebuffer = mSwapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = mSwapChain->GetExtent();
            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;
            vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
       
            vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);
            
            VkBuffer vertexBuffers[] = {mVertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(mCommandBuffers[i],0,1,vertexBuffers,offsets);
            vkCmdBindIndexBuffer(mCommandBuffers[i],mIndexBuffer,0,VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(mCommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,mPipelineLayout,0,1,&mDescriptorSets[i],0,nullptr);
            vkCmdDrawIndexed(mCommandBuffers[i],static_cast<uint32_t>(indices.size()),1,0,0,0);
            vkCmdEndRenderPass(mCommandBuffers[i]);

            VK_CHECK(vkEndCommandBuffer(mCommandBuffers[i]),"failed to record command buffer.");
        }
    }
    void VulkanRHI::PCreateSyncObjects(){
        mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        mImagesInFlight.resize(mSwapChain->GetImageCount(),VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            VK_CHECK(vkCreateSemaphore(mDevice,&semaphoreInfo,nullptr,&mImageAvailableSemaphores[i]),"failed to create semaphores.");
            VK_CHECK(vkCreateSemaphore(mDevice,&semaphoreInfo,nullptr,&mRenderFinishedSemaphores[i]),"failed to create semaphores.");    
            VK_CHECK(vkCreateFence(mDevice,&fenceInfo,nullptr,&mInFlightFences[i]),"failed to create fence.");
        }
    }
    void VulkanRHI::HCreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,VkMemoryPropertyFlags properties,VkBuffer& buffer,VkDeviceMemory& bufferMemory){
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(mDevice,&bufferInfo,nullptr,&buffer),"failed to create vertex buffer.");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice,buffer,&memRequirements);
        auto findMemoryType = [this](uint32_t typeFilter, VkMemoryPropertyFlags properties){
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice,&memProperties);
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
        VK_CHECK(vkAllocateMemory(mDevice,&allocInfo,nullptr,&bufferMemory),"failed to allocate vertex buffer memory.");
        vkBindBufferMemory(mDevice,buffer,bufferMemory,0);
    }
    void VulkanRHI::HCopyBuffer(VkBuffer srcBuffer,VkBuffer dstBuffer,VkDeviceSize size){
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = mCommandPool;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(mDevice,&allocInfo,&commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer,&beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,1,&copyRegion);
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(mGraphicQueue,1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(mGraphicQueue);
        vkFreeCommandBuffers(mDevice,mCommandPool,1,&commandBuffer);
    }
}
