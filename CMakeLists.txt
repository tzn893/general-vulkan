#general vulkan
project(gvulkan)

find_package(Vulkan REQUIRED)


if(Vulkan_FOUND)
    message(STATUS "vulkan has been found")
    message(STATUS "vulkan include directory ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "vulkan library ${Vulkan_LIBRARIES}")
else()
    message(STATUS "fail to find vulkan")
endif()