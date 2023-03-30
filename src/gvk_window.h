#pragma once

#include "gvk_common.h"

enum GVK_KEY 
{
	GVK_KEY_A,GVK_KEY_B,GVK_KEY_C,GVK_KEY_D,GVK_KEY_E,GVK_KEY_F,GVK_KEY_G,GVK_KEY_H,
	GVK_KEY_I,GVK_KEY_J,GVK_KEY_K,GVK_KEY_L,GVK_KEY_M,GVK_KEY_N,GVK_KEY_O,GVK_KEY_P,
	GVK_KEY_Q,GVK_KEY_R,GVK_KEY_S,GVK_KEY_T,GVK_KEY_U,GVK_KEY_V,GVK_KEY_W,GVK_KEY_X,
	GVK_KEY_Y,GVK_KEY_Z,
	GVK_KEY_1,GVK_KEY_2,GVK_KEY_3,GVK_KEY_4,GVK_KEY_5,GVK_KEY_6,GVK_KEY_7,GVK_KEY_8,
	GVK_KEY_9,GVK_KEY_0,
	GVK_MOUSE_1, GVK_MOUSE_2, GVK_MOUSE_3,
	GVK_KEY_SPACE,GVK_KEY_SHIFT,GVK_KEY_ESCAPE,GVK_KEY_ENTER,GVK_KEY_TAB,GVK_KEY_BACKSPACE,GVK_KEY_PRINTSCREEN,GVK_KEY_CONTROL,
	GVK_KEY_RIGHT,GVK_KEY_LEFT,GVK_KEY_DOWN,GVK_KEY_UP,
	GVK_KEY_NUM
};

struct GvkVector2
{
	float x, y;
};

namespace gvk {
	class Window {
	public:
		static opt<ptr<gvk::Window>> Create(uint32 width,uint32 height,const char* title);

		GLFWwindow* GetWindow();
		
		uint32 GetWidth();
		uint32 GetHeight();

		void   SetWindowTitle(const char* title);
		inline bool   ShouldClose() {return glfwWindowShouldClose(m_Window); }
		void   UpdateWindow();

		bool KeyDown(GVK_KEY k);
		bool KeyUp(GVK_KEY k);
		bool KeyHold(GVK_KEY k);

		bool MouseMove();

		//mouse's position on screen
		GvkVector2 GetMousePosition();
		
		//mouse's offset compared to last frame
		GvkVector2 GetMouseOffset();

		bool OnResize();

		~Window();
	private:
		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void frame_resize_callback(GLFWwindow* window, int width, int height);
		friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		friend void mouse_position_callback(GLFWwindow* window, double xposIn, double yposIn);

		GLFWwindow* m_Window;
		uint32 m_Width, m_Height;

		Window(GLFWwindow*, uint32, uint32);

		GvkVector2	m_LastMousePosition;
		GvkVector2  m_MouseOffset;

		bool		m_MouseMove;

		int m_KeyState[GVK_KEY_NUM];
		bool m_OnResize;
	};
}