#pragma once
#include <tuple>
#include <stdint.h>
#include <string>

std::tuple<void*, uint32_t, uint32_t> load_image(const std::string& path);