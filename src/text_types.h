#pragma once

#include "media_types.h"

namespace gag
{
struct RuntimeGenericBackendChild;

struct RuntimeGenericBackend
{
    void *identity;
    uint32_t flags;
    RuntimeGenericBackend *next;
    uint32_t text_size;
    const char *text;
    uint8_t unknown_0014[8];
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


struct RuntimeGenericBackendApi
{
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
    void (*sleep)(uint32_t milliseconds);
    RuntimeHeap *(*get_process_heap)();
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
};

struct RuntimeGenericBackendCreateApi
{
    RuntimeHeap *(*get_process_heap)();
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
};



struct RuntimeGenericChildCreateApi
{
    RuntimeGenericBackend *(*acquire_backend)(void *identity);
    int32_t (*find_text_entry)(RuntimeGenericBackend *backend, int32_t category, const char *name);
    int32_t (*parse_integer)(const char *text, uint32_t *position, uint32_t end, uint32_t flags);
    RuntimeHeap *(*get_process_heap)();
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
    uint32_t (*build_child_state)(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);
    void (*clear_backend_ready)(RuntimeGenericBackend *backend);
};



struct RuntimeStandaloneTextState
{
    uint32_t unknown_0000[3];
    const char *text;
    void *font_identity;
    uint32_t x;
    uint32_t y;
    uint32_t low_color;
    uint32_t high_color;
    uint32_t unknown_0024[2];
    union
    {
        uint32_t bounds[4];
        DisplayRectangle bounds_rectangle;
    };
};



struct RuntimeGenericChildSceneApi
{
    void *(*find_available_child)(uint32_t maximum_end_position);
    uint32_t (*build_child_state)(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);
    uint32_t (*find_scene_index)(uint32_t flags);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_scene_update)(intptr_t identifier);
    void (*publish_child_state)(void *identity, const uint32_t *state, const DisplaySceneDescriptor *descriptor, int32_t end_offset);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    bool (*set_child_context)(void *identity, const uintptr_t *context);
    bool (*get_child_context)(void *identity, uintptr_t *context);
    void *(*destroy_child)(void *identity);
    intptr_t (*query_scene)(int32_t index, DisplaySceneDescriptor *descriptor, DisplayPixelFormatDescriptor *format);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
    void (*enable_child_mode_200)(void *identity);
};



} // namespace gag
