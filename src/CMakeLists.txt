file(GLOB GVK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB GVK_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(gvk ${GVK_HEADER} ${GVK_SOURCE} 
    ${GVK_SPIRV_SOURCE} 
    ${GVK_SPIRV_INCLUDE}/spirv_reflect.h
)

find_package(Vulkan REQUIRED)

target_include_directories(gvk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${GVK_GLFW_INCLUDE}
    ${GVK_SPIRV_INCLUDE}
    ${Vulkan_INCLUDE_DIRS})

target_link_libraries(gvk glfw ${Vulkan_LIBRARIES})

# if(not Vulkan_glslc_FOUND)
#     message(FATAL_ERROR "GLSLC is required")
# endif()

# message(STATUS "GLSLC excutable path ${Vulkan_GLSLC_EXECUTABLE}")