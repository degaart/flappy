#include <optional>
#include <string_view>

namespace zorro
{
    class BufferView
    {
    public:
        BufferView(const void* buffer, size_t size);
        std::optional<std::string_view> readLine();
        const void* data() const;
        size_t size() const;
    private:
        const void* _buffer;
        size_t _size;
    };
}


