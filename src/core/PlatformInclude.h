#define J_WINDOWS //TODO: move to cmake

#ifdef J_WINDOWS
    #define J_GLFW
#endif

#ifdef J_GLFW
    #define VK_USE_PLATFORM_WIN32_KHR
    #define GLFW_INCLUDE_VULKAN
    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    typedef GLFWwindow* J_WINDOW_HANDLE;
#else
#endif