#pragma once

#include <cstdint>

struct StackAllocated {
    char data[31];
    char flags;
};

struct HeapAllocated {
public:
    const char* data; // 0x0
    __int64_t capacity; // 0x8
    __int64_t size; // 0x10
    char padding[7]; // padding
    char flags; // 31
};

namespace core
{
    class basic_string
    {
    public:
        basic_string() = default;

        union
        {
            StackAllocated embedded;
            HeapAllocated heap;
        };

        char const* c_str() const {
            if (this->heap.flags < 0x40) {
                return this->embedded.data;
            }

            return this->heap.data;
        }
    };
}