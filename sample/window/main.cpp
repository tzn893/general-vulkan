#include "gvk_window.h"
#include <stdio.h>
using namespace gvk;

int main() {
	ptr<gvk::Window> window;
	if (auto v = gvk::Window::Create(500, 500, "new window test"); v.has_value()) {
		window = v.value();
	}
	else {
		return -1;
	}

	while (!window->ShouldClose()) {
		window->UpdateWindow();
	}
}