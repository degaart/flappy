#pragma once

#include <vector>
#include <string_view>
#include <optional>

namespace zorro
{
    std::vector<std::string_view> split(std::string_view str, std::string_view sep);
    std::optional<long> parseLong(std::string_view str, int base = 10);
    std::optional<int> parseInt(std::string_view str, int base = 10);
    void __panic(const char* file, int line, const char* format, ...);
    void __trace(const char* file, int line, const char* format, ...);
    std::vector<uint8_t> loadBmp(const char* filename, int* width, int* height);
}

#define panic(...) zorro::__panic(__FILE__,__LINE__, __VA_ARGS__)
#define trace(...) zorro::__trace(__FILE__,__LINE__, __VA_ARGS__)

