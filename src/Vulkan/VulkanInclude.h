
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
//TODO: This should be included in window manager
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define VK_CHECK(op,message) \
    do{\
        if((op) != VK_SUCCESS){ \
            JLOG_ERROR(message);\
            throw std::runtime_error(message);\
        }\
    }while(0);