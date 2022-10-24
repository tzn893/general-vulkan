#pragma once

#include <algorithm>
#include <stdint.h>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>



#define gvk_assert(expr) if(!(expr)) {__debugbreak();exit(-1);}
#define gvk_zero_mem(s) memset(&s,0,sizeof(s));
#define gvk_alloca(T,count) (T*)alloca(sizeof(T) * count);
#define gvk_count_of(arr) (sizeof(arr) / sizeof(arr[0]))


namespace gvk {
	using uint32 = uint32_t;
	using uint16 = uint16_t;
	using uint8 = uint8_t;
	using int32 = int32_t;
	using int16 = int16_t;
	using int8 = int8_t;

	template<typename T>
	using ptr = std::shared_ptr<T>;

	template<typename T>
	using opt = std::optional<T>;

	template<typename T>
	class View {
	public:
		View():data(nullptr),start(0),end(0) {}
		View(const T* data, uint32 start, uint32 end) :
			data(data), start(start), end(end) {
			gvk_assert(end >= start);
		}
		View(const std::vector<T>& arr) :data(arr.data()), start(0), end(arr.size()) {}
		View(const View<T>& v) {
			data = v.data; start = v.start; end = v.end;
		}

		const View& operator=(const View<T>& v) {
			data = v.data;
			start = v.start;
			end = v.end;
			return *this;
		}

		T& operator=(uint32 idx) {
			gvk_assert(data != nullptr);
			gvk_assert(start + idx < end);
			return data[idx + start];
		}

		const T& operator[](uint32 idx) const {
			gvk_assert(data != nullptr);
			gvk_assert(start + idx < end);
			return data[idx + start];
		}

		uint32 size() const {
			return end - start;
		}

	private:
		const T* data;
		uint32 start, end;
	};
}

#ifdef GVK_WINDOWS_PLATFORM
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>



namespace gvk {
	inline uint32 GetFormatSize(VkFormat format) 
	{
		//TODO currently we only have formats usually used
		switch (format) {
		case VK_FORMAT_UNDEFINED:  return 0;
			case VK_FORMAT_R4G4_UNORM_PACK8: return 1;
			case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return 2;
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return 2;
			case VK_FORMAT_R5G6B5_UNORM_PACK16: return 2;
			case VK_FORMAT_B5G6R5_UNORM_PACK16: return 2;
			case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return 2;
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return 2;
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return 2;
			case VK_FORMAT_R8_UNORM: return 1;
			case VK_FORMAT_R8_SNORM: return 1;
			case VK_FORMAT_R8_USCALED: return 1;
			case VK_FORMAT_R8_SSCALED: return 1;
			case VK_FORMAT_R8_UINT: return 1;
			case VK_FORMAT_R8_SINT: return 1;
			case VK_FORMAT_R8_SRGB: return 1;
			case VK_FORMAT_R8G8_UNORM: return 2;
			case VK_FORMAT_R8G8_SNORM: return 2;
			case VK_FORMAT_R8G8_USCALED: return 2;
			case VK_FORMAT_R8G8_SSCALED: return 2;
			case VK_FORMAT_R8G8_UINT: return 2;
			case VK_FORMAT_R8G8_SINT: return 2;
			case VK_FORMAT_R8G8_SRGB: return 2;
			case VK_FORMAT_R8G8B8_UNORM: return 3;
			case VK_FORMAT_R8G8B8_SNORM: return 3;
			case VK_FORMAT_R8G8B8_USCALED: return 3;
			case VK_FORMAT_R8G8B8_SSCALED: return 3;
			case VK_FORMAT_R8G8B8_UINT: return 3;
			case VK_FORMAT_R8G8B8_SINT: return 3;
			case VK_FORMAT_R8G8B8_SRGB: return 3;
			case VK_FORMAT_B8G8R8_UNORM: return 3;
			case VK_FORMAT_B8G8R8_SNORM: return 3;
			case VK_FORMAT_B8G8R8_USCALED: return 3;
			case VK_FORMAT_B8G8R8_SSCALED: return 3;
			case VK_FORMAT_B8G8R8_UINT: return 3;
			case VK_FORMAT_B8G8R8_SINT: return 3;
			case VK_FORMAT_B8G8R8_SRGB: return 3;
			case VK_FORMAT_R8G8B8A8_UNORM: return 4;
			case VK_FORMAT_R8G8B8A8_SNORM: return 4;
			case VK_FORMAT_R8G8B8A8_USCALED: return 4;
			case VK_FORMAT_R8G8B8A8_SSCALED: return 4;
			case VK_FORMAT_R8G8B8A8_UINT: return 4;
			case VK_FORMAT_R8G8B8A8_SINT: return 4;
			case VK_FORMAT_R8G8B8A8_SRGB: return 4;
			case VK_FORMAT_B8G8R8A8_UNORM: return 4;
			case VK_FORMAT_B8G8R8A8_SNORM: return 4;
			case VK_FORMAT_B8G8R8A8_USCALED: return 4;
			case VK_FORMAT_B8G8R8A8_SSCALED: return 4;
			case VK_FORMAT_B8G8R8A8_UINT: return 4;
			case VK_FORMAT_B8G8R8A8_SINT: return 4;
			case VK_FORMAT_B8G8R8A8_SRGB: return 4;
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_UINT_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_SINT_PACK32: return 4;
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_UINT_PACK32: return 4;
			case VK_FORMAT_A2R10G10B10_SINT_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_UINT_PACK32: return 4;
			case VK_FORMAT_A2B10G10R10_SINT_PACK32: return 4;
			case VK_FORMAT_R16_UNORM: return 2;
			case VK_FORMAT_R16_SNORM: return 2;
			case VK_FORMAT_R16_USCALED: return 2;
			case VK_FORMAT_R16_SSCALED: return 2;
			case VK_FORMAT_R16_UINT: return 2;
			case VK_FORMAT_R16_SINT: return 2;
			case VK_FORMAT_R16_SFLOAT: return 2;
			case VK_FORMAT_R16G16_UNORM: return 4;
			case VK_FORMAT_R16G16_SNORM: return 4;
			case VK_FORMAT_R16G16_USCALED: return 4;
			case VK_FORMAT_R16G16_SSCALED: return 4;
			case VK_FORMAT_R16G16_UINT: return 4;
			case VK_FORMAT_R16G16_SINT: return 4;
			case VK_FORMAT_R16G16_SFLOAT: return 4;
			case VK_FORMAT_R16G16B16_UNORM: return 6;
			case VK_FORMAT_R16G16B16_SNORM: return 6;
			case VK_FORMAT_R16G16B16_USCALED: return 6;
			case VK_FORMAT_R16G16B16_SSCALED: return 6;
			case VK_FORMAT_R16G16B16_UINT: return 6;
			case VK_FORMAT_R16G16B16_SINT: return 6;
			case VK_FORMAT_R16G16B16_SFLOAT: return 6;
			case VK_FORMAT_R16G16B16A16_UNORM: return 6;
			case VK_FORMAT_R16G16B16A16_SNORM: return 6;
			case VK_FORMAT_R16G16B16A16_USCALED: return 6;
			case VK_FORMAT_R16G16B16A16_SSCALED: return 6;
			case VK_FORMAT_R16G16B16A16_UINT: return 6;
			case VK_FORMAT_R16G16B16A16_SINT: return 6;
			case VK_FORMAT_R16G16B16A16_SFLOAT: return 6;
			case VK_FORMAT_R32_UINT: return 4;
			case VK_FORMAT_R32_SINT: return 4;
			case VK_FORMAT_R32_SFLOAT: return 4;
			case VK_FORMAT_R32G32_UINT: return 8;
			case VK_FORMAT_R32G32_SINT: return 8;
			case VK_FORMAT_R32G32_SFLOAT: return 8;
			case VK_FORMAT_R32G32B32_UINT: return 12;
			case VK_FORMAT_R32G32B32_SINT: return 12;
			case VK_FORMAT_R32G32B32_SFLOAT: return 12;
			case VK_FORMAT_R32G32B32A32_UINT: return 16;
			case VK_FORMAT_R32G32B32A32_SINT: return 16;
			case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
			case VK_FORMAT_R64_UINT: return 8;
			case VK_FORMAT_R64_SINT: return 8;
			case VK_FORMAT_R64_SFLOAT: return 8;
			case VK_FORMAT_R64G64_UINT: return 16;
			case VK_FORMAT_R64G64_SINT: return 16;
			case VK_FORMAT_R64G64_SFLOAT: return 16;
			case VK_FORMAT_R64G64B64_UINT: return 24;
			case VK_FORMAT_R64G64B64_SINT: return 24;
			case VK_FORMAT_R64G64B64_SFLOAT: return 24;
			case VK_FORMAT_R64G64B64A64_UINT: return 32;
			case VK_FORMAT_R64G64B64A64_SINT: return 32;
			case VK_FORMAT_R64G64B64A64_SFLOAT: return 32;

			case VK_FORMAT_D16_UNORM : return 2;
			case VK_FORMAT_X8_D24_UNORM_PACK32: return 4;			
			case VK_FORMAT_D32_SFLOAT : return 4;
			case VK_FORMAT_S8_UINT: return 1;
			case VK_FORMAT_D16_UNORM_S8_UINT: return 3;
			case VK_FORMAT_D24_UNORM_S8_UINT: return 4;
			case VK_FORMAT_D32_SFLOAT_S8_UINT: return 5;
		}

		return 0;
	}

	inline VkImageAspectFlags GetAllAspects(VkFormat format)
	{
		switch (format) {
			case VK_FORMAT_S8_UINT: return VK_IMAGE_ASPECT_STENCIL_BIT;
			
			case VK_FORMAT_D16_UNORM: 
			case VK_FORMAT_X8_D24_UNORM_PACK32: 
			case VK_FORMAT_D32_SFLOAT:
				return VK_IMAGE_ASPECT_DEPTH_BIT;

			case VK_FORMAT_D16_UNORM_S8_UINT: 
			case VK_FORMAT_D24_UNORM_S8_UINT: 
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;


			case VK_FORMAT_R4G4_UNORM_PACK8:  
			case VK_FORMAT_R4G4B4A4_UNORM_PACK16:  
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16:  
			case VK_FORMAT_R5G6B5_UNORM_PACK16:  
			case VK_FORMAT_B5G6R5_UNORM_PACK16:  
			case VK_FORMAT_R5G5B5A1_UNORM_PACK16:  
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16:  
			case VK_FORMAT_A1R5G5B5_UNORM_PACK16:  
			case VK_FORMAT_R8_UNORM:  
			case VK_FORMAT_R8_SNORM:  
			case VK_FORMAT_R8_USCALED:  
			case VK_FORMAT_R8_SSCALED:  
			case VK_FORMAT_R8_UINT:  
			case VK_FORMAT_R8_SINT:  
			case VK_FORMAT_R8_SRGB:  
			case VK_FORMAT_R8G8_UNORM:  
			case VK_FORMAT_R8G8_SNORM:  
			case VK_FORMAT_R8G8_USCALED:  
			case VK_FORMAT_R8G8_SSCALED:  
			case VK_FORMAT_R8G8_UINT:  
			case VK_FORMAT_R8G8_SINT:  
			case VK_FORMAT_R8G8_SRGB:  
			case VK_FORMAT_R8G8B8_UNORM:  
			case VK_FORMAT_R8G8B8_SNORM:  
			case VK_FORMAT_R8G8B8_USCALED:  
			case VK_FORMAT_R8G8B8_SSCALED:  
			case VK_FORMAT_R8G8B8_UINT:  
			case VK_FORMAT_R8G8B8_SINT:  
			case VK_FORMAT_R8G8B8_SRGB:  
			case VK_FORMAT_B8G8R8_UNORM:  
			case VK_FORMAT_B8G8R8_SNORM:  
			case VK_FORMAT_B8G8R8_USCALED:  
			case VK_FORMAT_B8G8R8_SSCALED:  
			case VK_FORMAT_B8G8R8_UINT:  
			case VK_FORMAT_B8G8R8_SINT:  
			case VK_FORMAT_B8G8R8_SRGB:  
			case VK_FORMAT_R8G8B8A8_UNORM:  
			case VK_FORMAT_R8G8B8A8_SNORM:  
			case VK_FORMAT_R8G8B8A8_USCALED:  
			case VK_FORMAT_R8G8B8A8_SSCALED:  
			case VK_FORMAT_R8G8B8A8_UINT:  
			case VK_FORMAT_R8G8B8A8_SINT:  
			case VK_FORMAT_R8G8B8A8_SRGB:  
			case VK_FORMAT_B8G8R8A8_UNORM:  
			case VK_FORMAT_B8G8R8A8_SNORM:  
			case VK_FORMAT_B8G8R8A8_USCALED:  
			case VK_FORMAT_B8G8R8A8_SSCALED:  
			case VK_FORMAT_B8G8R8A8_UINT:  
			case VK_FORMAT_B8G8R8A8_SINT:  
			case VK_FORMAT_B8G8R8A8_SRGB:  
			case VK_FORMAT_A8B8G8R8_UNORM_PACK32:  
			case VK_FORMAT_A8B8G8R8_SNORM_PACK32:  
			case VK_FORMAT_A8B8G8R8_USCALED_PACK32:  
			case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:  
			case VK_FORMAT_A8B8G8R8_UINT_PACK32:  
			case VK_FORMAT_A8B8G8R8_SINT_PACK32:  
			case VK_FORMAT_A8B8G8R8_SRGB_PACK32:  
			case VK_FORMAT_A2R10G10B10_UNORM_PACK32:  
			case VK_FORMAT_A2R10G10B10_SNORM_PACK32:  
			case VK_FORMAT_A2R10G10B10_USCALED_PACK32:  
			case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:  
			case VK_FORMAT_A2R10G10B10_UINT_PACK32:  
			case VK_FORMAT_A2R10G10B10_SINT_PACK32:  
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32:  
			case VK_FORMAT_A2B10G10R10_SNORM_PACK32:  
			case VK_FORMAT_A2B10G10R10_USCALED_PACK32:  
			case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:  
			case VK_FORMAT_A2B10G10R10_UINT_PACK32:  
			case VK_FORMAT_A2B10G10R10_SINT_PACK32:  
			case VK_FORMAT_R16_UNORM:  
			case VK_FORMAT_R16_SNORM:  
			case VK_FORMAT_R16_USCALED:  
			case VK_FORMAT_R16_SSCALED:  
			case VK_FORMAT_R16_UINT:  
			case VK_FORMAT_R16_SINT:  
			case VK_FORMAT_R16_SFLOAT:  
			case VK_FORMAT_R16G16_UNORM:  
			case VK_FORMAT_R16G16_SNORM:  
			case VK_FORMAT_R16G16_USCALED:  
			case VK_FORMAT_R16G16_SSCALED:  
			case VK_FORMAT_R16G16_UINT:  
			case VK_FORMAT_R16G16_SINT:  
			case VK_FORMAT_R16G16_SFLOAT:  
			case VK_FORMAT_R16G16B16_UNORM:  
			case VK_FORMAT_R16G16B16_SNORM:  
			case VK_FORMAT_R16G16B16_USCALED:  
			case VK_FORMAT_R16G16B16_SSCALED:  
			case VK_FORMAT_R16G16B16_UINT:  
			case VK_FORMAT_R16G16B16_SINT:  
			case VK_FORMAT_R16G16B16_SFLOAT:  
			case VK_FORMAT_R16G16B16A16_UNORM:  
			case VK_FORMAT_R16G16B16A16_SNORM:  
			case VK_FORMAT_R16G16B16A16_USCALED:  
			case VK_FORMAT_R16G16B16A16_SSCALED:  
			case VK_FORMAT_R16G16B16A16_UINT:  
			case VK_FORMAT_R16G16B16A16_SINT:  
			case VK_FORMAT_R16G16B16A16_SFLOAT:  
			case VK_FORMAT_R32_UINT:  
			case VK_FORMAT_R32_SINT:  
			case VK_FORMAT_R32_SFLOAT:  
			case VK_FORMAT_R32G32_UINT:  
			case VK_FORMAT_R32G32_SINT:  
			case VK_FORMAT_R32G32_SFLOAT:  
			case VK_FORMAT_R32G32B32_UINT:  
			case VK_FORMAT_R32G32B32_SINT:  
			case VK_FORMAT_R32G32B32_SFLOAT:  
			case VK_FORMAT_R32G32B32A32_UINT:  
			case VK_FORMAT_R32G32B32A32_SINT:  
			case VK_FORMAT_R32G32B32A32_SFLOAT:  
			case VK_FORMAT_R64_UINT:  
			case VK_FORMAT_R64_SINT:  
			case VK_FORMAT_R64_SFLOAT:  
			case VK_FORMAT_R64G64_UINT:  
			case VK_FORMAT_R64G64_SINT:  
			case VK_FORMAT_R64G64_SFLOAT:  
			case VK_FORMAT_R64G64B64_UINT:  
			case VK_FORMAT_R64G64B64_SINT:  
			case VK_FORMAT_R64G64B64_SFLOAT:  
			case VK_FORMAT_R64G64B64A64_UINT:  
			case VK_FORMAT_R64G64B64A64_SINT:  
			case VK_FORMAT_R64G64B64A64_SFLOAT:  
				return VK_IMAGE_ASPECT_COLOR_BIT;

			//TODO currently we don't support other formats
		}
		return 0;
	}
}