#pragma once

#include "runtime_types.h"

namespace gag
{

uint32_t end_display_target();

uint32_t acquire_display_lock(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);

uint32_t release_display_lock();

int process_scene_node_callbacks(DisplaySceneNode *node);

bool clip_display_rectangle(DisplayRectangle *rectangle);

bool constrain_display_rectangle_to_surface(DisplayRectangle *rectangle);

void trim_display_rectangle_overlap(DisplayRectangle *rectangle, DisplaySceneNode *node);

void accumulate_scene_node_rectangle(DisplayRectangle *rectangle, DisplaySceneNode *node);

void merge_display_rectangle(DisplayRectangle *destination, const DisplayRectangleTransform *transform, const DisplayRectangle *source);

uint32_t queue_display_rectangle(DisplayRectangle *rectangle);

bool contains_display_scene_node(intptr_t identifier);

int synchronize_display_scene_node(DisplaySceneNode *node, DisplayRectangle *output_rectangle);

void publish_display_scene_node(DisplaySceneNode *node);

uint32_t dispatch_display_scene_update(void *target, uint32_t options);

uint32_t find_available_display_scene_index(uint32_t candidate);

uint32_t wait_for_display_scene_ready(uint32_t timeout);

uint32_t set_display_clip_rectangle(DisplayRectangle *rectangle);

uint32_t release_display_lock_mode_1000();

DisplaySceneNode *lock_display_scene_node(intptr_t identifier);

void unlock_display_scene_node(intptr_t identifier);

bool set_display_scene_primary_owner(intptr_t identifier, intptr_t owner, bool replace_existing);

intptr_t query_display_scene_by_index(int32_t index, DisplaySceneDescriptor *descriptor, DisplayPixelFormatDescriptor *callback_format);

uint32_t blit_bitmap_with_optional_palette_remap(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, uint32_t flags);

uint32_t offset_display_scene_node(intptr_t identifier, int32_t x_delta, int32_t y_delta);

uint32_t begin_display_scene_update(intptr_t identifier);

bool activate_display_scene_node(intptr_t identifier);

uint32_t end_display_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);

uint32_t update_display_root_region(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);

uint32_t add_display_scene_callback(intptr_t identifier, int (*callback)(DisplayTraversalState *state), const void *context, uint32_t context_size, uint32_t flags);

void fill_display_scene_rectangle_8(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

void fill_display_scene_rectangle_16(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

void composite_transparent_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

void composite_opaque_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state, uint32_t mode);

void composite_transparent_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

void composite_opaque_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

void composite_transparent_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

void composite_opaque_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

uint32_t release_display_scene_node(intptr_t identifier, intptr_t owner);

void build_indexed_to_16_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state);

void build_indexed_to_indexed_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state);

void configure_display_scene_format(DisplaySceneNode *node, const DisplayPixelFormatDescriptor *format);

bool configure_display_scene_palette(DisplaySceneNode *node, const uint32_t *palette, uint32_t count);

DisplaySceneNode *acquire_display_scene_node(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format);

uint32_t *initialize_display_scene_host(intptr_t primary_position, const DisplayPixelFormatDescriptor *format, int32_t width, int32_t height,
    int (*synchronize)(void *context, void *payload, uint32_t mode), void *context, uint32_t worker_interval);

uint32_t shutdown_display_scene_host();

DWORD WINAPI run_display_scene_worker(uint32_t *flags);

} // namespace gag
