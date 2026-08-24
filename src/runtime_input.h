#pragma once

#include <cstdint>
#include <string>

namespace gag
{
enum class RuntimeInputType
{
    pointer_move,
    pointer_leave,
    button_down,
    button_up,
    key_down,
    key_up,
    text,
    close
};

enum class RuntimeMouseButton
{
    none,
    left,
    middle,
    right
};

struct RuntimeInputEvent
{
    RuntimeInputType type{};
    RuntimeMouseButton button{};
    int32_t x{};
    int32_t y{};
    uint32_t key{};
    bool repeat{};
    std::string text;
};

enum class RuntimeQueuedInputType : uint32_t
{
    pointer_move = 0x200,
    left_button_down = 0x201,
    left_button_up = 0x202,
    right_button_down = 0x204
};
} // namespace gag
