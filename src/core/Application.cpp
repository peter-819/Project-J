#include "Jpch.h"
#include "Application.h"
#include "RHI.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
namespace ProjectJ{
    RHIType* RHI::gInstance = nullptr;
    Application::Application(const AppInfo& appInfo){

    }
    void Application::Run(){ 
        Logger::InitGlobally();
        JLOG_INFO("HI, J-Project");
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

        {
            RHIConfig config;
            config.enableValidationLayer = true;
            config.window = window;
            RHI::Create(config);
            while(!glfwWindowShouldClose(window)) {
                glfwPollEvents();
                RHI::Get().Draw();
            }
        }
        RHI::Destroy();
        glfwDestroyWindow(window);

        glfwTerminate();
    }
}