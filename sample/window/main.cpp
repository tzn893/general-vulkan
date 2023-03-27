#include "gvk_window.h"
#include <stdio.h>
using namespace gvk;

int main() {
	ptr<gvk::Window> window;
	if (auto v = gvk::Window::Create(1000, 1000, "new window test"); v.has_value()) {
		window = v.value();
	}
	else {
		return -1;
	}

	while (!window->ShouldClose()) {

		if (window->KeyHold(GVK_MOUSE_1) && window->MouseMove())
		{
			GvkVector2 pos = window->GetMousePosition();
			GvkVector2 offset = window->GetMouseOffset();
			printf("position : (%f,%f), offset: (%f,%f)\n", pos.x, pos.y, offset.x, offset.y);
		}

		


		window->UpdateWindow();
	}
}