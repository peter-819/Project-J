#include "Jpch.h"
#include "Application.h"
#include "../Vulkan/VulkanApp.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace ProjectJ{
    Application::Application(const AppInfo& appInfo){

    }

    void Application::Run(){ 
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

        {
            VulkanConfig config;
            config.enableValidationLayer = true;
            config.window = window;
            VulkanRHI vkRHI(config);

            while(!glfwWindowShouldClose(window)) {
                glfwPollEvents();
            }
        }
        glfwDestroyWindow(window);

        glfwTerminate();
    }
}