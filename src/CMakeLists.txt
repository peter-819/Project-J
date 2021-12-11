cmake_minimum_required(VERSION 3.22.0)

set(SOURCE_LIST 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanSwapChain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanBuffers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/VulkanPSO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Logger.cpp
PARENT_SCOPE)
