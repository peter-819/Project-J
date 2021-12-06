cmake_minimum_required(VERSION 3.22.0)
project(Project-J VERSION 0.1.0)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(3rdparty)
add_subdirectory(src)

add_executable(Project-J ${SOURCE_LIST})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
# ----------------------THIRD_PARTY--------------------------#
set(THIRDPARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

#VULKAN
set(VULKAN_DIR "C:\\VulkanSDK\\1.2.189.2")
set(VULKAN_INCLUDE_DIR ${VULKAN_DIR}/Include)
set(VULKAN_LIB_DIR ${VULKAN_DIR}/Lib)
find_library(VULKAN_LIBS
    NAMES vulkan-1
    HINTS ${VULKAN_LIB_DIR}
)

target_link_libraries(${PROJECT_NAME} 
    glfw ${GLFW_LIBRARIES}
    ${VULKAN_LIBS}
    spdlog
)

target_include_directories(${PROJECT_NAME} 
    PUBLIC ${VULKAN_INCLUDE_DIR}
    PUBLIC ${THIRDPARTY_DIR}/glm
)


#PCH
target_precompile_headers(${PROJECT_NAME} PRIVATE src/Jpch.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)