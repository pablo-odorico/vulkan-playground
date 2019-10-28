cmake_minimum_required(VERSION 3.12)

project(VulkanSample)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)

set(VULKAN_CPP_DIR ${CMAKE_SOURCE_DIR}/../Vulkan-Hpp)

#add_subdirectory(${VULKAN_CPP_DIR}/Vulkan-Headers ${CMAKE_BINARY_DIR}/Vulkan-Headers)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${VULKAN_CPP_DIR}/glfw ${CMAKE_BINARY_DIR}/glfw)
include_directories(${VULKAN_CPP_DIR}/glfw/include)

include_directories(${VULKAN_CPP_DIR}/glm)

set(SOURCES
	src/main.cpp
)

add_executable(VulkanSample ${SOURCES})
target_include_directories(VulkanSample PRIVATE Vulkan::Vulkan)
target_link_libraries(VulkanSample Vulkan::Vulkan glfw)
