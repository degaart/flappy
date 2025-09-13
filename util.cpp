#include "util.hpp"

#include <errno.h>
#include <stdlib.h>

std::vector<std::string_view> split(std::string_view str, std::string_view sep)
{
    std::vector<std::string_view> result;
    size_t pos = 0;
    while (true)
    {
        size_t next = str.find(sep, pos);
        if (next == std::string_view::npos)
        {
            result.emplace_back(str.substr(pos));
            break;
        }
        result.emplace_back(str.substr(pos, next - pos));
        pos = next + sep.size();
    }
    return result;
}

std::optional<long> parseLong(std::string_view str, int base)
{
    if (str.empty())
        return std::nullopt;
    char* end = nullptr;
    errno = 0;
    long value = strtol(str.data(), &end, base);
    if (errno != 0 || end != str.data() + str.size())
    {
        return std::nullopt;
    }
    return value;
}

std::optional<int> parseInt(std::string_view str, int base)
{
    auto ret = parseLong(str, base);
    if (!ret)
    {
        return std::nullopt;
    }
    return *ret;
}


