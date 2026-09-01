#pragma once

#include <cstddef>
#include <cstdint>
#include "portable_types.h"
#include "runtime_services.h"

namespace freegag
{

enum class ApplicationAction
{
    PAUSE,
    RESUME,
    EXIT,
    SAVE,
    LOAD,
    NEW_GAME,
    RESUME_SAVED_GAME,
    CREDITS,
    TOGGLE_COMMENTS,
    TOGGLE_MUTE,
    ENTER_FULLSCREEN,
    LEAVE_FULLSCREEN
};

struct CdfArchive;
struct AsyncFileHost;
struct AsyncFileRecord;
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
    intptr_t pixels;
    uint32_t bits_per_pixel;
    uint32_t stride;
    intptr_t indexed_pixels;
    uint32_t indexed_stride;
};

enum ApplicationFlag : uint32_t
{
    APPLICATION_CURSOR_HIDDEN = 1u << 0,
    APPLICATION_CURSOR_OUTSIDE = 1u << 1,
    APPLICATION_FULLSCREEN_PREFERENCE = 1u << 5,
    APPLICATION_DISPLAY_RESTORE_PENDING = 1u << 6,
    APPLICATION_WINDOWED = 1u << 7,
    APPLICATION_EXIT_REQUESTED = 1u << 9,
    APPLICATION_SOUND_MUTED = 1u << 12,
    APPLICATION_FATAL_ERROR = 1u << 13,
    APPLICATION_FULLSCREEN_AVAILABLE = 1u << 14,
    APPLICATION_CREDITS_ACTIVE = 1u << 16,
    APPLICATION_PREFERENCES_CHANGED = 1u << 18,
    APPLICATION_SNAPSHOT_ACTIVE = 1u << 19,
    APPLICATION_LOAD_DISABLED = 1u << 20,
    APPLICATION_RESUME_DISABLED = 1u << 21,
    APPLICATION_NEW_GAME_DISABLED = 1u << 22,
    APPLICATION_SAVE_DISABLED = 1u << 23,
    APPLICATION_RUNTIME_ACTIVE = 1u << 24,
    APPLICATION_SUBTITLES_ENABLED = 1u << 25,
    APPLICATION_INACTIVE = 1u << 27,
    APPLICATION_LOCKED = 1u << 30
};

struct ApplicationState
{
    int32_t width;
    int32_t height;
    bool shutdown_complete;
    bool low_color_resources;
    bool pointer_inside_window;
    bool gary;
    bool host_pause_deferred_scene;
    bool host_pause_animation;
    bool host_pause_gagboy;
    void *archive_context;
    uint32_t saved_flags;
    void *saved_memory;
    uintptr_t script_state;
    char startup_config[0x104];
    char installed_version[0x104];
    char installation_path[0x104];
    char executable_directory[0x104];
    uint32_t flags;
};


struct RuntimeNamedNode;

struct RuntimeTreeNode;


} // namespace freegag
