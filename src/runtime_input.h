#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace freegag
{
enum class RuntimeInputType
{
    POINTER_MOVE,
    POINTER_LEAVE,
    BUTTON_DOWN,
    BUTTON_UP,
    KEY_DOWN,
    KEY_UP,
    TEXT,
    CLOSE
};

enum class RuntimeMouseButton
{
    NONE,
    LEFT,
    MIDDLE,
    RIGHT
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
    POINTER_MOVE = 0x200,
    LEFT_BUTTON_DOWN = 0x201,
    LEFT_BUTTON_UP = 0x202,
    RIGHT_BUTTON_DOWN = 0x204
};

struct RuntimeQueuedInput
{
    RuntimeQueuedInputType type;
    uint32_t packed_position;
};

inline constexpr size_t runtime_input_text_capacity = 0x20;
using RuntimeInputText = std::array<char, runtime_input_text_capacity>;

inline constexpr uint32_t RUNTIME_POINTER_POSITION_OUTSIDE = 0xffffffff;
} // namespace freegag
