//implement vulkan memory allocator for project integration
#define VK_NO_PROTOTYPES

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>


#ifdef GVK_WINDOWS_PLATFORM
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include "Volk/volk.c"
