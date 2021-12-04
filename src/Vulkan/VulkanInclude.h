
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
//TODO: This should be included in window manager
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Vulkan/vulkan.h>

#define VK_CHECK(op,message) \
    do{\
        if((op) != VK_SUCCESS){ \
            std::cout<<message<<std::endl;\
            throw std::runtime_error(message);\
        }\
    }while(0);