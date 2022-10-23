#include "gvk_window.h"

namespace gvk {

	static ptr<Window> g_window;

	static GVK_KEY convert_key_code(int glfw_key) {
		if (glfw_key <= GLFW_KEY_Z && glfw_key >= GLFW_KEY_A) {
			return GVK_KEY(glfw_key - GLFW_KEY_A + GVK_KEY_A);
		}
		if (glfw_key <= GLFW_KEY_9 && glfw_key >= GLFW_KEY_0) {
			return GVK_KEY(glfw_key - GLFW_KEY_0 + GVK_KEY_0);
		}
		return GVK_KEY_NUM;
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

	opt<ptr<Window>> Window::Create(uint32 width, uint32 height,const char* title) {
		gvk_assert(g_window == nullptr);
		
		if (title == nullptr) {
			title = "untitled vulkan application";
		}

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);


		GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (window == NULL) {
			return std::nullopt;
		}
		g_window = ptr<Window>(new Window(window, width, height));
		return { g_window };
	}

	GLFWwindow* Window::GetWindow() {
		return m_Window;
	}

	uint32 Window::GetWidth() {
		return m_Width;
	}

	uint32 Window::GetHeight() {
		return m_Height;
	}


	void Window::SetWindowTitle(const char* title)
	{
		gvk_assert(title != nullptr);
		glfwSetWindowTitle(m_Window, title);
	}

	Window::Window(GLFWwindow* window, uint32 width, uint32 height) :
		m_Window(window),m_Height(height),m_Width(width),m_OnResize(false) {
		memset(m_KeyState, 0xffffffff, sizeof(m_KeyState));
		glfwSetKeyCallback(m_Window, key_callback);
		glfwSetFramebufferSizeCallback(m_Window, frame_resize_callback);
	}

	void   Window::UpdateWindow() { 
		memset(m_KeyState, 0xffffffff, sizeof(m_KeyState));
		m_OnResize = false;
		glfwPollEvents(); 
	}

	Window::~Window() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	bool Window::KeyDown(GVK_KEY k) {
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GLFW_PRESS;
	}

	bool Window::KeyUp(GVK_KEY k) {
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GLFW_RELEASE;
	}

	bool Window::KeyHold(GVK_KEY k) {
		gvk_assert(k < GVK_KEY_NUM);
		return m_KeyState[k] == GLFW_REPEAT;
	}

	bool Window::OnResize()
	{
		return m_OnResize;
	}

}