#pragma once

#include <vector>
#include <string_view>
#include <optional>

std::vector<std::string_view> split(std::string_view str, std::string_view sep);
std::optional<long> parseLong(std::string_view str, int base = 10);
std::optional<int> parseInt(std::string_view str, int base = 10);

