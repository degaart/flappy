#include <stdio.h>

#include <vector>
#include <optional>
#include <string_view>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

extern "C" const char* __asan_default_options()
{
    return "detect_leaks=0";
}

#define emit(ptr, size)                                                                                                                                        \
    if (fwrite(ptr, size, 1, f) != 1)                                                                                                                       \
    {                                                                                                                                                          \
        fprintf(stderr, "Write failed\n");                                                                                                                     \
        return 1;                                                                                                                                              \
    }

uint8_t palette[][3] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

struct Color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t unused;
};

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

std::optional<long> parseLong(std::string_view str)
{
    if (str.empty())
        return std::nullopt;
    char* end = nullptr;
    errno = 0;
    long value = strtol(str.data(), &end, 10);
    if (errno != 0 || end != str.data() + str.size())
    {
        return std::nullopt;
    }
    return value;
}

std::optional<int> parseInt(std::string_view str)
{
    auto ret = parseLong(str);
    if (!ret)
    {
        return std::nullopt;
    }
    return *ret;
}

static std::optional<std::vector<Color>> loadPalette(const char* filename)
{
    FILE* f = fopen(filename, "rt");
    if (!f)
    {
        fprintf(stderr, "Failed to load file %s", filename);
        return std::nullopt;
    }

    auto getLine = [f]() -> std::optional<std::string>
    {
        char buffer[512];
        if (!fgets(buffer, sizeof(buffer), f))
        {
            return std::nullopt;
        }

        std::string result(buffer);
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
        {
            result.pop_back();
        }

        return result;
    };

    if (auto header = getLine(); !header || *header != "JASC-PAL")
    {
        fprintf(stderr, "Invalid header (magic) for %s", filename);
        return std::nullopt;
    }
    else if (auto version = getLine(); !version || *version != "0100")
    {
        fprintf(stderr, "Invalid header (version) for %s", filename);
        return std::nullopt;
    }
    else if (auto version = getLine(); !version || *version != "256")
    {
        fprintf(stderr, "Invalid header (colorcount) for %s", filename);
        return std::nullopt;
    }

    std::vector<Color> result;
    result.reserve(256);
    for (int i = 0; i < 256; i++)
    {
        auto line = getLine();
        if (!line)
        {
            fprintf(stderr, "Failed to read entry %d in %s", i, filename);
            return std::nullopt;
        }

        auto tokens = split(*line, " ");
        if (tokens.size() != 3)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s", line->c_str(), filename);
            return std::nullopt;
        }

        auto r = parseInt(tokens[0]);
        if (!r)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s", line->c_str(), filename);
            return std::nullopt;
        }

        auto g = parseInt(tokens[1]);
        if (!g)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s", line->c_str(), filename);
            return std::nullopt;
        }

        auto b = parseInt(tokens[2]);
        if (!b)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s", line->c_str(), filename);
            return std::nullopt;
        }

        Color c;
        c.r = *r;
        c.g = *g;
        c.b = *b;
        result.emplace_back(c);
    }

    return result;
}

int main(int argc, char** argv)
{
    assert(sizeof(Color) == 4);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <infile> <outfile> <palettefile>\n", argv[0]);
        return 1;
    }

    const char* inFile = argv[1];
    const char* outFile = argv[2];
    const char* paletteFile = argv[3];

    std::optional<std::vector<Color>> palette = loadPalette(paletteFile);

    int width, height, bpp;
    auto data = stbi_load(inFile, &width, &height, &bpp, 0);
    if (!data)
    {
        fprintf(stderr, "Failed to load file %s\n", inFile);
        return 1;
    }
    else if (bpp != 3)
    {
        fprintf(stderr, "Unsupported bpp: %d\n", bpp);
        return 1;
    }

    FILE* f = fopen(outFile, "wb");
    if (!f)
    {
        fprintf(stderr, "Failed to create file %s\n", outFile);
        return 1;
    }

    char debugInfo[16];
    memset(debugInfo, 0, sizeof(debugInfo));
    snprintf(debugInfo, sizeof(debugInfo), "%dx%d", width, height);

    emit(debugInfo, sizeof(debugInfo));
    emit(&width, sizeof(width));
    emit(&height, sizeof(height));

    uint8_t* srcPtr = data;
    for (int y = 0; y < height; y++)
    {
        uint8_t* scanline = srcPtr;
        for (int x = 0; x < width; x++)
        {
            uint8_t r = *scanline++;
            uint8_t g = *scanline++;
            uint8_t b = *scanline++;

            uint8_t closest = 0;
            int minDist = INT_MAX;
            for (int col = 0; col < palette->size(); col++)
            {
                int rDist = r - (*palette)[col].r;
                int gDist = g - (*palette)[col].g;
                int bDist = b - (*palette)[col].b;
                int dist = rDist*rDist + gDist*gDist + bDist*bDist;
                if (dist < minDist)
                {
                    minDist = dist;
                    closest = col;
                }
            }
            emit(&closest, sizeof(closest));
        }
        srcPtr += width * 3;
    }

    fclose(f);
    stbi_image_free(data);
    return 0;
}

