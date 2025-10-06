#include <zorro/BufferView.hpp>

using namespace zorro;

BufferView::BufferView(const void* buffer, size_t size)
    : _buffer(buffer), _size(size)
{
}

std::optional<std::string_view> BufferView::readLine()
{
    if (!_size)
    {
        return std::nullopt;
    }

    const char* buffer = static_cast<const char*>(_buffer);
    const char* ptr = buffer;
    while (_size && *ptr != '\r' && *ptr != '\n')
    {
        ptr++;
        _size--;
    }

    std::string_view result(buffer, ptr - buffer);
    if (*ptr == '\r')
    {
        ptr++;
        _size--;
    }
    if (*ptr == '\n')
    {
        ptr++;
        _size--;
    }

    _buffer = ptr;
    return result;
}

const void* BufferView::data() const
{
    return _buffer;
}

size_t BufferView::size() const
{
    return _size;
}

