#general vulkan
cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 17)
option(GVK_ENABLE_SAMPLE "enable building samples" off)
option(GVK_ENABLE_IMGUI_SUPPORT "enable imgui support for gvk" off)

if(GVK_ENABLE_IMGUI_SUPPORT)
	add_compile_definitions(GVK_SUPPORT_IMGUI)
endif()

project(gvk)

if(WIN32)
    add_compile_definitions(GVK_WINDOWS_PLATFORM)
endif()

add_subdirectory(glfw)
set(GVK_GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include CACHE INTERNAL "GVK_GLFW_INCLUDE")

#for shader reflection
set(GVK_SPIRV_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect CACHE INTERNAL "GVK_SPIRV_INCLUDE")
set(GVK_SPIRV_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect/spirv_reflect.cpp CACHE INTERNAL GVK_SPIRV_SOURCE"")

add_subdirectory(src)

if(GVK_ENABLE_SAMPLE)
	add_subdirectory(sample)
endif()