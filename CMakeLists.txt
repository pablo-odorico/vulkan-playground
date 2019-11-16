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

add_subdirectory(ResourceCompiler)

set(SOURCES
	src/main.cpp
)

add_executable(VulkanSample ${SOURCES})
target_include_directories(VulkanSample PRIVATE Vulkan::Vulkan)
target_link_libraries(VulkanSample Vulkan::Vulkan glfw)

function(target_shader TARGET SHADER_PATH RESOURCE_NAME)
    if (WIN32)
        set(GLSLC_BIN_PATH ${Vulkan_INCLUDE_DIR}/../bin/glslc.exe)
    else ()
        set(GLSLC_BIN_PATH glslc)
    endif()
    get_filename_component(FILE_NAME ${SHADER_PATH} NAME)
    set(GENERATED_FILE_DIR "${CMAKE_BINARY_DIR}/Generated/Shaders")
    set(GENERATED_FILE_PATH "${GENERATED_FILE_DIR}/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${GENERATED_FILE_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_FILE_DIR}
        COMMAND ${GLSLC_BIN_PATH} ${SHADER_PATH} -o ${GENERATED_FILE_PATH}
        DEPENDS ${SHADER_PATH}
    )
    target_resource(${TARGET} ${GENERATED_FILE_PATH} ${RESOURCE_NAME})
endfunction()

target_shader(VulkanSample ${CMAKE_SOURCE_DIR}/resources/shader.vert "vertex_shader")
target_shader(VulkanSample ${CMAKE_SOURCE_DIR}/resources/shader.frag "fragment_shader")
#target_resource(VulkanSample ${CMAKE_SOURCE_DIR}/resources/texture.png "texture")
