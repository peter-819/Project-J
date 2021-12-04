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
    }
    void VulkanRHI::Cleanup(){
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
        auto isDeviceSuitable = [findQueueFamilies](VkPhysicalDevice device)->bool{
            auto indices = findQueueFamilies(device);
            return indices.IsComplete();
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
        createInfo.enabledExtensionCount = 0;
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
}
