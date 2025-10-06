#include "util.hpp"

#include <algorithm>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

std::vector<std::string_view> zorro::split(std::string_view str, std::string_view sep)
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

std::optional<long> zorro::parseLong(std::string_view str, int base)
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

std::optional<int> zorro::parseInt(std::string_view str, int base)
{
    auto ret = parseLong(str, base);
    if (!ret)
    {
        return std::nullopt;
    }
    return *ret;
}

#pragma pack(push, 1)
struct BMPFileHeader
{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

std::vector<uint8_t> zorro::loadBmp(const char* tag, const void* buffer, size_t size, int* width, int* height)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(buffer);
    const BMPFileHeader* fileHeader = reinterpret_cast<const BMPFileHeader*>(ptr);
    ptr += sizeof(BMPFileHeader);

    const BMPInfoHeader* infoHeader = reinterpret_cast<const BMPInfoHeader*>(ptr);
    ptr += sizeof(BMPInfoHeader);

    if (fileHeader->bfType != 0x4D42 || infoHeader->biBitCount != 8)
    {
        panic("Invalid pixel format: %s", tag);
    }

    *width = infoHeader->biWidth;
    *height = infoHeader->biHeight;
    size_t rowSize = ((*width + 3) & ~3);
    size_t dataSize = rowSize * *height;

    size_t paletteSize = (infoHeader->biClrUsed ? infoHeader->biClrUsed : 256) * 4;

    ptr = reinterpret_cast<const uint8_t*>(buffer) + sizeof(BMPFileHeader) + infoHeader->biSize + paletteSize;

    std::vector<uint8_t> pixels(*width * *height);
    for (int y = 0; y < *height; ++y)
    {
        std::copy_n(&ptr[y * rowSize], *width, &pixels[(*height - 1 - y) * *width]);
    }
    return pixels;
}

