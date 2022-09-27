#include "gvk_window.h"
#include <stdio.h>

int main() {
	ptr<gvk::Window> window;
	if (auto v = gvk::Window::CreateWindow(500, 500, "new window test"); v.has_value()) {
		window = v.value();
	}
	else {
		return -1;
	}

	while (!window->ShouldClose()) {
		window->UpdateWindow();
	}
}