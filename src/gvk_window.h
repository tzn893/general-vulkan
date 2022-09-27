#pragma once

#include "gvk_common.h"

enum GVK_KEY {
	GVK_KEY_A,GVK_KEY_B,GVK_KEY_C,GVK_KEY_D,GVK_KEY_E,GVK_KEY_F,GVK_KEY_G,GVK_KEY_H,
	GVK_KEY_I,GVK_KEY_J,GVK_KEY_K,GVK_KEY_L,GVK_KEY_M,GVK_KEY_N,GVK_KEY_O,GVK_KEY_P,
	GVK_KEY_Q,GVK_KEY_R,GVK_KEY_S,GVK_KEY_T,GVK_KEY_U,GVK_KEY_V,GVK_KEY_W,GVK_KEY_X,
	GVK_KEY_Y,GVK_KEY_Z,
	GVK_KEY_1,GVK_KEY_2,GVK_KEY_3,GVK_KEY_4,GVK_KEY_5,GVK_KEY_6,GVK_KEY_7,GVK_KEY_8,
	GVK_KEY_9,GVK_KEY_0,
	GVK_KEY_NUM
};

namespace gvk {
	class Window {
	public:
		static opt<ptr<gvk::Window>> CreateWindow(uint32 width,uint32 height,const char* title);

		GLFWwindow* GetWindow();
		
		uint32 GetWidth();
		uint32 GetHeight();

		void   SetWindowTitle(const char* title);
		inline bool   ShouldClose() {return glfwWindowShouldClose(m_Window); }
		void   UpdateWindow();

		bool KeyDown(GVK_KEY k);
		bool KeyUp(GVK_KEY k);
		bool KeyHold(GVK_KEY k);

		~Window();
	private:
		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

		GLFWwindow* m_Window;
		uint32 m_Width, m_Height;

		Window(GLFWwindow*, uint32, uint32);
		int m_KeyState[GVK_KEY_NUM];
	};
}