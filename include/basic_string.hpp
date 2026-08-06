#pragma once

#include <cstddef>
#include <cstdint>

namespace core
{
#pragma pack(push, 4)

    struct basic_string
    {
        enum class StorageMode : std::uint8_t
        {
            HeapOwned = 0,
            Inline = 1,
            External = 2,
        };

        union
        {
            struct
            {
                char data[0x18];               // 0x00
                std::uint8_t remaining;        // 0x18
                std::uint8_t unknown19[7];     // 0x19
            } embedded;

            struct
            {
                char* data;                    // 0x00
                std::uint64_t unknown08;        // 0x08
                std::uint64_t size;             // 0x10
                std::uint64_t unknown18;        // 0x18
            } allocated;
        };

        StorageMode mode;                      // 0x20
        std::uint8_t padding21[3];

        [[nodiscard]]
        bool is_inline() const noexcept
        {
            return mode == StorageMode::Inline;
        }

        [[nodiscard]]
        bool is_owned_heap() const noexcept
        {
            return mode == StorageMode::HeapOwned;
        }

        [[nodiscard]]
        std::size_t size() const noexcept
        {
            return is_inline()
                ? 0x18u - embedded.remaining
                : static_cast<std::size_t>(allocated.size);
        }

        [[nodiscard]]
        char const* data() const noexcept
        {
            return is_inline() ? embedded.data : allocated.data;
        }

        [[nodiscard]]
        char const* c_str() const noexcept
        {
            return data();
        }

        char* mutable_data_unsafe() noexcept
        {
            return is_inline() ? embedded.data : allocated.data;
        }

        void set_size_unsafe(std::size_t newSize) noexcept
        {
            if (is_inline()) {
                embedded.remaining =
                    static_cast<std::uint8_t>(0x18u - newSize);
            } else {
                allocated.size = newSize;
            }
        }
    };

#pragma pack(pop)

    static_assert(sizeof(basic_string) == 0x24);
    static_assert(offsetof(basic_string, embedded.remaining) == 0x18);
    static_assert(offsetof(basic_string, allocated.size) == 0x10);
    static_assert(offsetof(basic_string, mode) == 0x20);
}