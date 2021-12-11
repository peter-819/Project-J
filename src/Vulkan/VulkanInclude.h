
#include <core/PlatformInclude.h>
#include <Vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VK_CHECK(op,message) \
    do{\
        if((op) != VK_SUCCESS){ \
            JLOG_ERROR(message);\
            throw std::runtime_error(message);\
        }\
    }while(0);