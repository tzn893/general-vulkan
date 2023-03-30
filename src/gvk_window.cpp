#include "gvk_window.h"

#define GVK_KEY_HOLD (GLFW_REPEAT + GLFW_RELEASE + GLFW_PRESS)
#define GVK_KEY_NONE 0xffffffff

namespace gvk {

	static ptr<Window> g_window;

	static GVK_KEY g_key_table[GLFW_KEY_LAST + 1];

	static void	   InitializeKeyTable()
	{
		for (uint32 i = 0; i < GLFW_KEY_LAST + 1; i++)
		{
			g_key_table[i] = GVK_KEY_NUM;
		}
		g_key_table[GLFW_KEY_SPACE] = GVK_KEY_SPACE;
		g_key_table[GLFW_KEY_APOSTROPHE] = GVK_KEY_NUM;  /* ' */
		g_key_table[GLFW_KEY_COMMA] = GVK_KEY_NUM;  /* , */
		g_key_table[GLFW_KEY_MINUS] = GVK_KEY_NUM;  /* - */
		g_key_table[GLFW_KEY_PERIOD] = GVK_KEY_NUM;  /* . */
		g_key_table[GLFW_KEY_SLASH] = GVK_KEY_NUM;  /* / */
		g_key_table[GLFW_KEY_0] = GVK_KEY_0;
		g_key_table[GLFW_KEY_1] = GVK_KEY_1;
		g_key_table[GLFW_KEY_2] = GVK_KEY_2;
		g_key_table[GLFW_KEY_3] = GVK_KEY_3;
		g_key_table[GLFW_KEY_4] = GVK_KEY_4;
		g_key_table[GLFW_KEY_5] = GVK_KEY_5;
		g_key_table[GLFW_KEY_6] = GVK_KEY_6;
		g_key_table[GLFW_KEY_7] = GVK_KEY_7;
		g_key_table[GLFW_KEY_8] = GVK_KEY_8;
		g_key_table[GLFW_KEY_9] = GVK_KEY_9;
		g_key_table[GLFW_KEY_SEMICOLON] = GVK_KEY_NUM;  /* ; */
		g_key_table[GLFW_KEY_EQUAL] = GVK_KEY_NUM;  /* = */
		g_key_table[GLFW_KEY_A] = GVK_KEY_A;
		g_key_table[GLFW_KEY_B] = GVK_KEY_B;
		g_key_table[GLFW_KEY_C] = GVK_KEY_C;
		g_key_table[GLFW_KEY_D] = GVK_KEY_D;
		g_key_table[GLFW_KEY_E] = GVK_KEY_E;
		g_key_table[GLFW_KEY_F] = GVK_KEY_F;
		g_key_table[GLFW_KEY_G] = GVK_KEY_G;
		g_key_table[GLFW_KEY_H] = GVK_KEY_H;
		g_key_table[GLFW_KEY_I] = GVK_KEY_I;
		g_key_table[GLFW_KEY_J] = GVK_KEY_J;
		g_key_table[GLFW_KEY_K] = GVK_KEY_K;
		g_key_table[GLFW_KEY_L] = GVK_KEY_L;
		g_key_table[GLFW_KEY_M] = GVK_KEY_M;
		g_key_table[GLFW_KEY_N] = GVK_KEY_N;
		g_key_table[GLFW_KEY_O] = GVK_KEY_O;
		g_key_table[GLFW_KEY_P] = GVK_KEY_P;
		g_key_table[GLFW_KEY_Q] = GVK_KEY_Q;
		g_key_table[GLFW_KEY_R] = GVK_KEY_R;
		g_key_table[GLFW_KEY_S] = GVK_KEY_S;
		g_key_table[GLFW_KEY_T] = GVK_KEY_T;
		g_key_table[GLFW_KEY_U] = GVK_KEY_U;
		g_key_table[GLFW_KEY_V] = GVK_KEY_V;
		g_key_table[GLFW_KEY_W] = GVK_KEY_W;
		g_key_table[GLFW_KEY_X] = GVK_KEY_X;
		g_key_table[GLFW_KEY_Y] = GVK_KEY_Y;
		g_key_table[GLFW_KEY_Z] = GVK_KEY_Z;
		g_key_table[GLFW_KEY_LEFT_BRACKET] = GVK_KEY_NUM;  /* [ */
		g_key_table[GLFW_KEY_BACKSLASH] = GVK_KEY_NUM;  /* \ */
		g_key_table[GLFW_KEY_RIGHT_BRACKET] = GVK_KEY_NUM;  /* ] */
		g_key_table[GLFW_KEY_GRAVE_ACCENT] = GVK_KEY_NUM;  /* ` */
		g_key_table[GLFW_KEY_WORLD_1] = GVK_KEY_NUM; /* non-US #1 */
		g_key_table[GLFW_KEY_WORLD_2] = GVK_KEY_NUM; /* non-US #2 */

				/* Function keys */
		g_key_table[GLFW_KEY_ESCAPE] = GVK_KEY_ESCAPE;
		g_key_table[GLFW_KEY_ENTER] = GVK_KEY_ENTER;
		g_key_table[GLFW_KEY_TAB] = GVK_KEY_TAB;
		g_key_table[GLFW_KEY_BACKSPACE] = GVK_KEY_BACKSPACE;
		g_key_table[GLFW_KEY_INSERT] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_DELETE] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_RIGHT] = GVK_KEY_RIGHT;
		g_key_table[GLFW_KEY_LEFT] = GVK_KEY_LEFT;
		g_key_table[GLFW_KEY_DOWN] = GVK_KEY_DOWN;
		g_key_table[GLFW_KEY_UP] = GVK_KEY_UP;
		g_key_table[GLFW_KEY_PAGE_UP] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_PAGE_DOWN] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_HOME] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_END] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_CAPS_LOCK] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_SCROLL_LOCK] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_NUM_LOCK] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_PRINT_SCREEN] = GVK_KEY_PRINTSCREEN;
		g_key_table[GLFW_KEY_PAUSE] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F1] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F2] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F3] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F4] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F5] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F6] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F7] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F8] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F9] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F10] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F11] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F12] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F13] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F14] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F15] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F16] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F17] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F18] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F19] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F20] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F21] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F22] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F23] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F24] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_F25] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_0] = GVK_KEY_0;
		g_key_table[GLFW_KEY_KP_1] = GVK_KEY_1;
		g_key_table[GLFW_KEY_KP_2] = GVK_KEY_2;
		g_key_table[GLFW_KEY_KP_3] = GVK_KEY_3;
		g_key_table[GLFW_KEY_KP_4] = GVK_KEY_4;
		g_key_table[GLFW_KEY_KP_5] = GVK_KEY_5;
		g_key_table[GLFW_KEY_KP_6] = GVK_KEY_6;
		g_key_table[GLFW_KEY_KP_7] = GVK_KEY_7;
		g_key_table[GLFW_KEY_KP_8] = GVK_KEY_8;
		g_key_table[GLFW_KEY_KP_9] = GVK_KEY_9;
		g_key_table[GLFW_KEY_KP_DECIMAL] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_DIVIDE] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_MULTIPLY] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_SUBTRACT] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_ADD] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_KP_ENTER] = GVK_KEY_ENTER;
		g_key_table[GLFW_KEY_KP_EQUAL] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_LEFT_SHIFT] = GVK_KEY_SHIFT;
		g_key_table[GLFW_KEY_LEFT_CONTROL] = GVK_KEY_CONTROL;
		g_key_table[GLFW_KEY_LEFT_ALT] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_LEFT_SUPER] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_RIGHT_SHIFT] = GVK_KEY_SHIFT;
		g_key_table[GLFW_KEY_RIGHT_CONTROL] = GVK_KEY_CONTROL;
		g_key_table[GLFW_KEY_RIGHT_ALT] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_RIGHT_SUPER] = GVK_KEY_NUM;
		g_key_table[GLFW_KEY_MENU] = GVK_KEY_NUM;
	}

	static GVK_KEY convert_key_code(int glfw_key) 
	{
		return g_key_table[glfw_key];
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		GVK_KEY gvk_key = convert_key_code(key);
		if (gvk_key >= GVK_KEY_NUM) { return; }

		if (action == GLFW_PRESS || action == GLFW_RELEASE)
		{
			g_window->m_KeyState[(uint32)gvk_key] = action;
		}
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
		
		InitializeKeyTable();

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
		
		if (code < GVK_KEY_NUM && (action == GLFW_PRESS || action == GLFW_RELEASE))
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