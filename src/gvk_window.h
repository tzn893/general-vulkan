#pragma once
#include "gvk_common.h"
#include <glfw/glfw3.h>

namespace gvk {
	class Window {
	public:

		GLFWwindow* GetWindow();
		
		uint32 GetWidth();
		uint32 GetHeight();
	
	private:
		GLFWwindow* m_Window;
		uint32 m_Width, m_Height;

	};
}