#pragma once

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class Fixed16
{
public:
    int32_t value;

    constexpr Fixed16()
        : value(0)
    {
    }

    constexpr Fixed16(int i)
        : value(i << 16)
    {
    }

    constexpr explicit Fixed16(float f)
        : value(static_cast<int32_t>(f * 65536.0f))
    {
    }

    constexpr explicit Fixed16(double d)
        : value(static_cast<int32_t>(d * 65536.0))
    {
    }

    static constexpr Fixed16 fromRaw(int32_t raw)
    {
        Fixed16 f;
        f.value = raw;
        return f;
    }

    constexpr int toInt() const
    {
        return value >> 16;
    }

    constexpr float toFloat() const
    {
        return static_cast<float>(value) / 65536.0f;
    }

    constexpr double toDouble() const
    {
        return static_cast<double>(value) / 65536.0;
    }

    constexpr Fixed16 operator+(const Fixed16& other) const
    {
        return fromRaw(value + other.value);
    }

    constexpr Fixed16 operator-(const Fixed16& other) const
    {
        return fromRaw(value - other.value);
    }

    constexpr Fixed16 operator*(const Fixed16& other) const
    {
        int64_t tmp = static_cast<int64_t>(value) * static_cast<int64_t>(other.value);
        return fromRaw(static_cast<int32_t>(tmp >> 16));
    }

    constexpr Fixed16 operator/(const Fixed16& other) const
    {
        int64_t tmp = (static_cast<int64_t>(value) << 16) / other.value;
        return fromRaw(static_cast<int32_t>(tmp));
    }

    Fixed16& operator+=(const Fixed16& other)
    {
        value += other.value;
        return *this;
    }

    Fixed16& operator-=(const Fixed16& other)
    {
        value -= other.value;
        return *this;
    }

    Fixed16& operator*=(const Fixed16& other)
    {
        int64_t tmp = static_cast<int64_t>(value) * static_cast<int64_t>(other.value);
        value = static_cast<int32_t>(tmp >> 16);
        return *this;
    }

    Fixed16& operator/=(const Fixed16& other)
    {
        int64_t tmp = (static_cast<int64_t>(value) << 16) / other.value;
        value = static_cast<int32_t>(tmp);
        return *this;
    }

    constexpr bool operator==(const Fixed16& other) const
    {
        return value == other.value;
    }

    constexpr bool operator!=(const Fixed16& other) const
    {
        return value != other.value;
    }

    constexpr bool operator<(const Fixed16& other) const
    {
        return value < other.value;
    }

    constexpr bool operator<=(const Fixed16& other) const
    {
        return value <= other.value;
    }

    constexpr bool operator>(const Fixed16& other) const
    {
        return value > other.value;
    }

    constexpr bool operator>=(const Fixed16& other) const
    {
        return value >= other.value;
    }

    std::string toString(int precision = 4) const
    {
        int32_t raw = value;
        bool neg = raw < 0;
        if (neg)
            raw = -raw;

        int32_t integer = raw >> 16;
        int32_t frac = raw & 0xFFFF;

        std::string result;
        if (neg)
            result.push_back('-');
        result += std::to_string(integer);

        if (precision > 0)
        {
            result.push_back('.');
            for (int i = 0; i < precision; ++i)
            {
                frac = (frac * 10);
                int digit = frac >> 16;
                result.push_back('0' + digit);
                frac &= 0xFFFF;
            }
        }
        return result;
    }

    static std::optional<Fixed16> fromString(std::string_view str)
    {
        if (str.empty())
            return std::nullopt;

        bool neg = false;
        size_t i = 0;

        if (str[i] == '+' || str[i] == '-')
        {
            neg = (str[i] == '-');
            ++i;
        }

        if (i >= str.size() || !std::isdigit(static_cast<unsigned char>(str[i])))
            return std::nullopt;

        int64_t intPart = 0;
        while (i < str.size() && std::isdigit(static_cast<unsigned char>(str[i])))
        {
            intPart = intPart * 10 + (str[i] - '0');
            ++i;
        }

        int64_t fracPart = 0;
        int scale = 1;
        if (i < str.size() && str[i] == '.')
        {
            ++i;
            while (i < str.size() && std::isdigit(static_cast<unsigned char>(str[i])))
            {
                fracPart = fracPart * 10 + (str[i] - '0');
                scale *= 10;
                ++i;
            }
        }

        if (i != str.size())
            return std::nullopt;

        int64_t raw = (intPart << 16);
        if (scale > 1)
        {
            raw += (fracPart << 16) / scale;
        }

        if (neg)
            raw = -raw;

        if (raw < INT32_MIN || raw > INT32_MAX)
            return std::nullopt;
        return Fixed16::fromRaw(static_cast<int32_t>(raw));
    }
};
