#include "gvk_window.h"

#define GVK_KEY_HOLD (GLFW_REPEAT + GLFW_RELEASE + GLFW_PRESS)
#define GVK_KEY_NONE 0xffffffff

namespace gvk {

	static ptr<Window> g_window;

	static GVK_KEY convert_key_code(int glfw_key) 
	{
		if (glfw_key <= GLFW_KEY_Z && glfw_key >= GLFW_KEY_A) 
		{
			return GVK_KEY(glfw_key - GLFW_KEY_A + GVK_KEY_A);
		}
		if (glfw_key <= GLFW_KEY_9 && glfw_key >= GLFW_KEY_0) 
		{
			return GVK_KEY(glfw_key - GLFW_KEY_0 + GVK_KEY_0);
		}
		return GVK_KEY_NUM;
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) 
	{
		GVK_KEY gvk_key = convert_key_code(key);
		if (gvk_key >= GVK_KEY_NUM) {return;}
		
		g_window->m_KeyState[(uint32)gvk_key] = action;
	}

	static void frame_resize_callback(GLFWwindow* window,int width,int height) 
	{
		g_window->m_Width = width;
		g_window->m_Height = height;
		g_window->m_OnResize = true;
	}

	opt<ptr<Window>> Window::Create(uint32 width, uint32 height,const char* title) 
	{
		gvk_assert(g_window == nullptr);
		
		if (title == nullptr) {
			title = "untitled vulkan application";
		}

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);


		//set camera mode
		//glfwSetInputMode(m_Window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);


		GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (window == NULL) 
		{
			return std::nullopt;
		}
		g_window = ptr<Window>(new Window(window, width, height));
		return { g_window };
	}

	GLFWwindow* Window::GetWindow() 
	{
		return m_Window;
	}

	uint32 Window::GetWidth() 
	{
		return m_Width;
	}

	uint32 Window::GetHeight() 
	{
		return m_Height;
	}


	void Window::SetWindowTitle(const char* title)
	{
		gvk_assert(title != nullptr);
		glfwSetWindowTitle(m_Window, title);
	}

	static int convert_mouse_code(int button)
	{
		if (button >= GLFW_MOUSE_BUTTON_1 && button <= GLFW_MOUSE_BUTTON_3)
		{
			return button - GLFW_MOUSE_BUTTON_1 + GVK_MOUSE_1;
		}
		return GVK_KEY_NUM;
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		int code = convert_mouse_code(button);
		
		if (code < GVK_KEY_NUM)
		{
			g_window->m_KeyState[code] = action;
		}
	}

	void mouse_position_callback(GLFWwindow* window, double xposIn, double yposIn)
	{
		GvkVector2 curr_pos{ xposIn,yposIn };
		g_window->m_MouseOffset = GvkVector2{ curr_pos.x - g_window->m_LastMousePosition.x,curr_pos.y - g_window->m_LastMousePosition.y };
		g_window->m_LastMousePosition = curr_pos;
		g_window->m_MouseMove = true;
	}

	Window::Window(GLFWwindow* window, uint32 width, uint32 height) :
		m_Window(window),m_Height(height),m_Width(width),m_OnResize(false) 
	{
		memset(m_KeyState, 0xffffffff, sizeof(m_KeyState));
		glfwSetKeyCallback(m_Window, key_callback);
		glfwSetFramebufferSizeCallback(m_Window, frame_resize_callback);
		glfwSetCursorPosCallback(m_Window, mouse_position_callback);
		glfwSetMouseButtonCallback(m_Window, mouse_button_callback);

		m_MouseMove = false;
		m_MouseOffset = { 0,0 };
		
		double xpos, ypos;
		glfwGetCursorPos(m_Window, &xpos, &ypos);

		m_LastMousePosition = GvkVector2{ (float)xpos,(float)ypos };
	}

	void   Window::UpdateWindow() 
	{

		for (uint32 i = 0;i < GVK_KEY_NUM;i++)
		{
			if (m_KeyState[i] == GLFW_PRESS)
			{
				m_KeyState[i] = GVK_KEY_HOLD;
			}
			else if (m_KeyState[i] == GLFW_RELEASE)
			{
				m_KeyState[i] = GVK_KEY_NONE;
			}
		}
		m_MouseOffset = GvkVector2{ 0,0 };


		m_MouseMove = false;
		m_OnResize = false;

		glfwPollEvents();
	}

	Window::~Window() 
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	bool Window::KeyDown(GVK_KEY k) 
	{
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GLFW_PRESS;
	}

	bool Window::KeyUp(GVK_KEY k) 
	{
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GLFW_RELEASE;
	}

	bool Window::KeyHold(GVK_KEY k) 
	{
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GVK_KEY_HOLD;
	}


	bool Window::MouseMove()
	{
		return m_MouseMove;
	}

	GvkVector2 Window::GetMousePosition()
	{
		return m_LastMousePosition;
	}

	GvkVector2 Window::GetMouseOffset()
	{
		return m_MouseOffset;
	}

	bool Window::OnResize()
	{
		return m_OnResize;
	}

}