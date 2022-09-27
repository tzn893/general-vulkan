#pragma once

#include <algorithm>
#include <stdint.h>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_set>
#include <string>

using uint32 = uint32_t;
using uint16 = uint16_t;
using uint8  = uint8_t ;
using int32  = int32_t ;
using int16  = int16_t ;
using int8   = int8_t  ;

template<typename T>
using ptr = std::shared_ptr<T>;

template<typename T>
using opt = std::optional<T>;

#define gvk_assert(expr) if(!(expr)) {__debugbreak();exit(-1);}
#define gvk_zero_mem(s) memset(&s,0,sizeof(s));
#define gvk_alloca(T,count) (T*)alloca(sizeof(T) * count);


namespace gvk {
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

