#general vulkan
cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)

project(gvk)

if(WIN32)
    add_compile_definitions(GVK_WINDOWS_PLATFORM)
endif()

add_subdirectory(glfw)
set(GVK_GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include CACHE INTERNAL "GVK_GLFW_INCLUDE")

add_subdirectory(src)
add_subdirectory(test)
