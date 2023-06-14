#define STB_IMAGE_IMPLEMENTATION
#include "stbi.h"

#include "image.h"

std::tuple<void*, uint32_t, uint32_t> load_image(const std::string& path)
{
    int width, height, comp;
	void* image = stbi_load(path.c_str(), &width, &height, &comp, 4);
	return std::make_tuple(image, (uint32_t)width, (uint32_t)height);
}