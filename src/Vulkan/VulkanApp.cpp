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
    void VulkanRHI::Init(){
        PCreateInstance();
        PSetupDebugMessenger();
        PCreateSurface();
        PPickPhysicalDevice();
        PCreateLogicalDevice();
        PCreateImageViews();
    }
    void VulkanRHI::Cleanup(){
        vkDestroyPipelineLayout(mDevice,mPipelineLayout,nullptr);
        for(auto imageView : mSwapChainImageViews){
            vkDestroyImageView(mDevice,imageView,nullptr);
        }
        vkDestroySwapchainKHR(mDevice,mSwapChain,nullptr);
        vkDestroyDevice(mDevice,nullptr);
        if (mConfig.enableValidationLayer) {
            DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(mInstance,mSurface,nullptr);
        vkDestroyInstance(mInstance, nullptr);
    }
    
    void VulkanRHI::PCreateInstance(){
        if(mConfig.enableValidationLayer && !pCheckValidationLayer()) {
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

        auto requiredExtensions = PGetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        if(mConfig.enableValidationLayer){
            createInfo.enabledLayerCount = static_cast<uint32_t>(mValidationLayers.size());
            createInfo.ppEnabledLayerNames = mValidationLayers.data();

            auto dMessengerInfo = PPopulateDebugMessengerCreateInfo();
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

    bool VulkanRHI::pCheckValidationLayer(){
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
    std::vector<const char*> VulkanRHI::PGetRequiredExtensions(){
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions,glfwExtensions + glfwExtensionCount);
        if(mConfig.enableValidationLayer){
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }
    VkDebugUtilsMessengerCreateInfoEXT VulkanRHI::PPopulateDebugMessengerCreateInfo() const{
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
        auto createInfo = PPopulateDebugMessengerCreateInfo();
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
    void VulkanRHI::PCreateSwapChain(){
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
                glfwGetFramebufferSize(mConfig.window,&width,&height);

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
    void VulkanRHI::PCreateImageViews(){
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
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Op
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) mSwapChainExtent.width;
        viewport.height = (float) mSwapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = mSwapChainExtent;

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
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        VK_CHECK(vkCreatePipelineLayout(mDevice,&pipelineLayoutInfo,nullptr,&mPipelineLayout),"failed to create pipeline layout");

        
        vkDestroyShaderModule(mDevice,fragShaderModule,nullptr);
        vkDestroyShaderModule(mDevice,vertShaderModule,nullptr);

    }
}
