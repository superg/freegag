#pragma once

#include <cstddef>
#include <cstdint>
#include "portable_types.h"
#include "runtime_services.h"

namespace gag
{

enum class ApplicationAction
{
    pause,
    resume,
    exit,
    save,
    load,
    new_game,
    resume_saved_game,
    credits,
    toggle_comments,
    toggle_mute,
    enter_fullscreen,
    leave_fullscreen
};

struct CdfArchive;
struct AsyncFileHost;
struct AsyncFileRecord;
struct SecondaryWindowLayout;
struct ScriptRuntimeRoot;
struct RuntimeGameHostContext;
struct DisplaySceneNode;
struct DisplayPixelFormatDescriptor
{
    uint32_t flags;
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t palette_count;
    const uint32_t *palette_source;
    const uint32_t *palette_entries;
};
struct RuntimeGenericBackendChild;
struct DisplayRectangle
{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
};
struct DisplayRectangleTransform;
struct ScriptObjectState;

struct DisplaySceneDescriptor
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    uint16_t present;
    uint16_t reserved;
    intptr_t pixels;
    uint32_t bits_per_pixel;
    uint32_t stride;
    intptr_t indexed_pixels;
    uint32_t indexed_stride;
};

struct ApplicationState
{
    int32_t width;
    int32_t height;
    uint32_t validation_flags;
    bool shutdown_complete;
    bool low_color_resources;
    void *archive_context;
    const char *message_table;
    uint32_t saved_flags;
    void *saved_memory;
    void *game_context;
    uintptr_t script_state;
    int32_t content_left;
    int32_t content_top;
    int32_t content_right;
    int32_t content_bottom;
    char startup_config[0x104];
    char installed_version[0x104];
    char installation_path[0x104];
    char executable_directory[0x104];
    uint32_t flags;
};


struct ValidationApi
{
    uint32_t (*load_preferences)(ApplicationState *state);
    uint32_t (*enable_borderless_fullscreen)(ApplicationState *state);
};



struct GraphicsHostInitializationResult
{
    uint32_t bits_per_pixel;
};


struct RuntimeNamedNode;

struct GraphicsHostApi
{
    uint32_t (*initialize_media)();
    uint32_t (*initialize_async)();
    uint32_t (*initialize_generic)();
    RuntimeHeap *(*heap_create)(uint32_t options, size_t initial_size, size_t maximum_size);
    uint32_t (*initialize_display)(int32_t width, int32_t height, uint32_t options);
    void (*set_script_root)(ScriptRuntimeRoot *root);
    RuntimeNamedNode *(*get_or_create_named_node)(const char *name);
    void (*set_named_node_enabled)(void *identity, int enabled);
};


struct GraphicsHostShutdownApi
{
    uint32_t (*shutdown_display)();
    uint32_t (*shutdown_generic_backend)();
    uint32_t (*shutdown_async_files)();
    uint32_t (*shutdown_media_backend)();
    void (*shutdown_presenter)();
    bool (*heap_destroy)(RuntimeHeap *heap);
};



struct RuntimeBootstrapApi
{
    void *(*create_surface)(int32_t width, int32_t height);
    PaletteEntry *(*get_palette_entries)();
    uint32_t *(*initialize_scene_host)(intptr_t primary_position, const DisplayPixelFormatDescriptor *format, int32_t width, int32_t height,
        int (*synchronize)(void *context, void *payload, uint32_t mode), void *context, uint32_t worker_interval);
    DisplaySceneNode *(*acquire_scene_node)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    DisplaySceneNode *(*lock_scene_node)(intptr_t identifier);
    void (*unlock_scene_node)(intptr_t identifier);
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*release_display_lock)();
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    void (*reset_display_state)();
};



struct RuntimeScriptPropertySetApi
{
    void (*select_resource)(char *path);
    bool (*release_memory_resource)(const char *path);
    void (*set_property_value)(uint32_t value);
    void (*enter_state_1000)();
    void (*leave_state_1000)();
    void (*destroy_resource_tree)(void *root);
};



struct RuntimeScriptPropertyGetApi
{
    int (*copy_string)(char *destination, const char *source);
    void (*load_resource)(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);
    uint32_t (*get_property_value)();
    uint16_t (*query_frame_number)(void *identity);
};



struct ApplicationInitializationApi
{
    int (*copy_string)(char *destination, const char *source);
    int (*validate_environment)(ApplicationState *state, const char *requested_archive, uint32_t stages);
    GraphicsHostInitializationResult *(*initialize_graphics_host)(int16_t width, uint16_t height, uint32_t flags);
    GraphicsHostInitializationResult *(*initialize_runtime)();
    void (*update_window_layout)(ApplicationState *state, SecondaryWindowLayout *secondary_layout);
    void (*enable_runtime)();
    void (*set_active_object_field)(uint32_t value);
    uint32_t (*detect_resource_type)(const char *data);
};



struct RuntimeBackendInitializationApi
{
    RuntimeHeap *(*heap_create)(uint32_t options, size_t initial_size, size_t maximum_size);
    void (*initialize_sound)();
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
};



struct RuntimeGenericBackendShutdownApi
{
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
    uint32_t (*destroy_backend)(void *identity);
};



struct RuntimeTreeNode;


struct SecondaryWindowLayout
{
    uint32_t unknown_0000;
    uint32_t state;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t flags;
};


struct StateActivationApi
{
    uint32_t (*query_status)(void *identity);
    uintptr_t (*get_script_state)();
    void (*on_cursor_outside)();
};



struct SynchronizedStateApi
{
    void (*enter_lock)();
    void (*leave_lock)();
    int (*write_cdf_package)(void *path, void *comment, void *unused, void *script_state);
};



} // namespace gag
