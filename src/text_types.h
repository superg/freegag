#pragma once

#include "media_types.h"

namespace freegag
{
enum RuntimeGenericTextDirective : int32_t
{
    RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID = -1,
    RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT = 0x00010000,
    RUNTIME_GENERIC_TEXT_DIRECTIVE_JUMP = 0x00020000,
    RUNTIME_GENERIC_TEXT_DIRECTIVE_END = 0x00030000,
    RUNTIME_GENERIC_TEXT_DIRECTIVE_REFERENCE = 0x00040000
};

enum RuntimeGenericTextSentinel : uint32_t
{
    RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND = 0xffffffff,
    RUNTIME_GENERIC_TEXT_INTEGER_INVALID = 0x7fffffff
};

struct RuntimeGenericBackendChild;

struct RuntimeGenericBackend
{
    void *identity;
    uint32_t flags;
    RuntimeGenericBackend *next;
    uint32_t text_size;
    const char *text;
    uint32_t child_count;
    RuntimeGenericBackendChild *children;
};

struct RuntimeGenericBackendChild
{
    void *identity;
    RuntimeGenericBackend *parent;
    uint32_t flags;
    uintptr_t context[2];
    uint32_t state[15];
    uint32_t computed_state[15];
    uint32_t state_end_position;
    uint32_t default_selection;
    uint32_t parser_position;
    uint32_t text_search_position;
    DisplaySceneDescriptor descriptor;
    void *font_identity;
    RuntimeGenericBackendChild *next;
};


struct RuntimeStandaloneTextState
{
    uint32_t layout_state[3];
    const char *text;
    void *font_identity;
    uint32_t x;
    uint32_t y;
    uint32_t low_color;
    uint32_t high_color;
    union
    {
        uint32_t bounds[4];
        DisplayRectangle bounds_rectangle;
    };
};



} // namespace freegag
