#pragma once

#include <thread>
#include "runtime_input.h"
#include "runtime_services.h"
#include "runtime_tree_types.h"

namespace freegag
{
struct DisplayBitmapCaptureSource
{
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    const uint8_t *pixels;
    const PaletteEntry *palette_entries;
};

enum RuntimeHostFlag : uint32_t
{
    RUNTIME_HOST_SHUTDOWN_REQUESTED = 0x00000001,
    RUNTIME_HOST_AUDIO_ENABLED = 0x00000002,
    RUNTIME_HOST_PROPERTY_STATE_ACTIVE = 0x00000004,
    RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE = 0x00000008,
    RUNTIME_HOST_EMBEDDED_GAME_ACTIVE = 0x00000010,
    RUNTIME_HOST_EMBEDDED_GAME_INACTIVE = 0x00000020,
    RUNTIME_HOST_COMMAND_STOP_REQUESTED = 0x00000040,
    RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED = 0x00000080,
    RUNTIME_HOST_TEXT_INPUT_COMMITTED = 0x00000100,
    RUNTIME_HOST_RESOURCE_LOAD_ACTIVE = 0x00000200,
    RUNTIME_HOST_MESSAGE_QUEUE_ENABLED = 0x00000400,
    RUNTIME_HOST_INITIALIZED = 0x00000800,
    RUNTIME_HOST_SCENE_SWITCH_DEFERRED = 0x00001000,
    RUNTIME_HOST_SCENE_TRANSITION_GUARDED = 0x00004000,
    RUNTIME_HOST_PALETTE_STATE = 0x00008000,
    RUNTIME_HOST_POINTER_MODE_PRIMARY = 0x00010000,
    RUNTIME_HOST_POINTER_MODE_SECONDARY = 0x00020000,
    RUNTIME_HOST_FORCE_PALETTE_REFRESH = 0x00040000,
    RUNTIME_HOST_NO_INVENTORY = 0x00080000,
    RUNTIME_HOST_SCRIPT_TREE_ACTIVE = 0x00100000,
    RUNTIME_HOST_EXTERNAL_COMMAND_PENDING = 0x00200000,
    RUNTIME_HOST_PAUSED = 0x01000000,
    RUNTIME_HOST_RESUME_PENDING = 0x02000000,
    RUNTIME_HOST_TREE_SWITCH_PENDING = 0x04000000,
    RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN = 0x10000000,
    RUNTIME_HOST_CREDITS_ACTIVE = 0x40000000,
    RUNTIME_HOST_PLAN_MODE = 0x80000000,

    RUNTIME_HOST_POINTER_MODE_MASK = RUNTIME_HOST_POINTER_MODE_PRIMARY | RUNTIME_HOST_POINTER_MODE_SECONDARY,
    RUNTIME_HOST_INPUT_READY = RUNTIME_HOST_MESSAGE_QUEUE_ENABLED | RUNTIME_HOST_SCRIPT_TREE_ACTIVE,
    RUNTIME_HOST_DISPLAY_READY = RUNTIME_HOST_RESOURCE_LOAD_ACTIVE | RUNTIME_HOST_MESSAGE_QUEUE_ENABLED,
    RUNTIME_HOST_DISPLAY_RESET_MASK = 0x0083c1bc
};

enum RuntimeCommandFlag : uint32_t
{
    RUNTIME_COMMAND_SHUTDOWN_REQUESTED = 0x00000001,
    RUNTIME_COMMAND_GAME_BUSY = 0x00000010,
    RUNTIME_COMMAND_GAME_RESULT_READY = 0x00000020,
    RUNTIME_COMMAND_LOOP_STOP_REQUESTED = 0x00000040,
    RUNTIME_COMMAND_TEXT_INPUT_ACTIVE = 0x00000100,
    RUNTIME_COMMAND_NESTED_STATE_ACTIVE = 0x00004000,
    RUNTIME_COMMAND_SCRIPT_EXECUTION_ENABLED = 0x00100000,
    RUNTIME_COMMAND_LOOP_RUNNING = 0x01000000,
    RUNTIME_COMMAND_RESUME_REQUESTED = 0x02000000
};

inline constexpr uint32_t RUNTIME_TARGET_ACTIVE = 0x00100000;

enum RuntimeTextInputFlag : uint32_t
{
    RUNTIME_TEXT_INPUT_LOWERCASE = 0x00000010,
    RUNTIME_TEXT_INPUT_UPPERCASE = 0x00000020,
    RUNTIME_TEXT_INPUT_LETTER_CASE_MASK = RUNTIME_TEXT_INPUT_LOWERCASE | RUNTIME_TEXT_INPUT_UPPERCASE
};

enum DisplayPresenterFlag : uint32_t
{
    DISPLAY_TARGET_LOCKED = 0x40000000,
    DISPLAY_PRESENTER_INITIALIZED = 0x80000000
};

enum DisplayTargetResult : uint32_t
{
    DISPLAY_TARGET_SUCCESS = 0,
    DISPLAY_TARGET_UNAVAILABLE = 0x00200000
};

enum DisplaySceneHostFlag : uint32_t
{
    DISPLAY_SCENE_HOST_INITIALIZED = 0x00000001,
    DISPLAY_SCENE_PALETTE_CHANGED = 0x00000010,
    DISPLAY_SCENE_WORKER_READY = 0x00000020,
    DISPLAY_SCENE_LOCK_RELEASE_PENDING = 0x00001000,
    DISPLAY_SCENE_LOCK_ACQUIRED = 0x00002000,
    DISPLAY_SCENE_LOCK_MODE_MASK = DISPLAY_SCENE_LOCK_RELEASE_PENDING | DISPLAY_SCENE_LOCK_ACQUIRED,
    DISPLAY_SCENE_HOST_SHUTDOWN_REQUESTED = 0x40000000
};

enum DisplayOperationResult : uint32_t
{
    DISPLAY_OPERATION_SUCCESS = 0,
    DISPLAY_OPERATION_RELEASE_PENDING = DISPLAY_SCENE_LOCK_RELEASE_PENDING,
    DISPLAY_OPERATION_LOCK_NOT_OWNED = 0x20000000,
    DISPLAY_OPERATION_FAILED = 0x80000000
};

inline constexpr uint32_t DISPLAY_SCENE_ROOT_INDEX = 0x7fffffff;

enum DisplaySceneFlag : uint32_t
{
    DISPLAY_SCENE_DISABLED = 0x00000001,
    DISPLAY_SCENE_STATIC = 0x00000002,
    DISPLAY_SCENE_OPAQUE = 0x00000020,
    DISPLAY_SCENE_INDEXED = 0x00000040,
    DISPLAY_SCENE_PRIMARY_OWNER = 0x00010000,
    DISPLAY_SCENE_PRIMARY = 0x00020000,
    DISPLAY_SCENE_PRESERVE_POSITION = 0x00100000,
    DISPLAY_SCENE_PRESERVE_DIMENSIONS = 0x00200000,
    DISPLAY_SCENE_UPDATE_PENDING = 0x01000000,
    DISPLAY_SCENE_FIXED_SIZE = 0x02000000,
    DISPLAY_SCENE_FIXED_POSITION = 0x04000000,
    DISPLAY_SCENE_XRGB_COMPOSITION = 0x08000000
};

enum DisplayDirtyFlag : uint32_t
{
    DISPLAY_DIRTY_PRIMARY = 0x00010000,
    DISPLAY_DIRTY_SECONDARY = 0x00020000
};

inline constexpr uint32_t DISPLAY_SCENE_CALLBACK_NO_BUFFER_SWAP = 0x00010000;

enum DisplayTraversalFlag : uint32_t
{
    DISPLAY_TRAVERSAL_QUERY = 0x01000000,
    DISPLAY_TRAVERSAL_RENDER = 0x02000000
};

enum DisplayTraversalResult : int
{
    DISPLAY_TRAVERSAL_BUFFER_UPDATED = 0,
    DISPLAY_TRAVERSAL_UNCHANGED = 1,
    DISPLAY_TRAVERSAL_STOP = 0x10
};



struct RuntimeCommandLoopState
{
    uint8_t resource_archive_state;
    char resource_directory[0x104];
    void *display_scene_host;
    DisplayPixelFormatDescriptor display_pixel_format;
    intptr_t input_alternate_scene_identifier;
    char first_runtime_path[0x104];
    char second_runtime_path[0x104];
    uint16_t width;
    uint16_t height;
    RuntimeGameInputHandler game_input_handler;
    RuntimeGameDllExecute game_dll_execute;
    uint32_t game_result_type;
    uint8_t game_result_data[0x104];
    char input_text[runtime_input_text_capacity];
    RuntimeStandaloneTextState input_text_state;
    intptr_t input_scene_identifier;
    uint32_t input_text_flags;
    uint32_t input_scene_index;
    uint32_t input_caret_tick;
    uint32_t input_cursor;
    uint32_t input_end;
    uint32_t queued_input_available;
    RuntimeQueuedInput queued_inputs[0x20];
    uint32_t queued_input_read_index;
    uint32_t queued_input_write_index;
    uint32_t byte_available;
    uint8_t byte_queue[0x20];
    uint32_t byte_read_index;
    uint32_t byte_write_index;
    CdfArchive *active_archive;
    union
    {
        AsyncFileHost *async_file_host;
        intptr_t archive_alternate_stream;
    };
    void *resource_cache_parent_identity;
    RuntimeMutex byte_queue_mutex;
    RuntimeMutex queued_input_mutex;
    RuntimeMutex resource_mutex;
    RuntimeMutex path_mutex;
    RuntimeHeap *resource_heap;
    std::jthread *script_thread;
    void *media_objects_parent_identity;
    uint32_t resource_wait_count;
    uint32_t accumulated_tree_flags;
    uint32_t palette_transition_step;
    uint32_t rectangle_transition_step_size;
    uint32_t available_scene_transitions;
    uint32_t nested_runtime_state_count;
    uint32_t nested_runtime_state_4_count;
    uint32_t nested_no_inventory_state_count;
    uint32_t resource_count;
    uint32_t external_command_pending;
    uint32_t target_flags;
    uint32_t flags;
    int32_t resource_stream_rate_bytes_per_millisecond;
    uint32_t script_clock;
    int32_t scene_x;
    int32_t scene_y;
    void *saved_default_comment_scene_identity;
    union
    {
        void *deferred_scene_identity;
        uintptr_t deferred_state_value;
    };
    union
    {
        void *current_scene_identity;
        uintptr_t current_state_value;
    };
    void *current_runtime_resource;
    void *runtime_tree_identity;
    RuntimeTreeLink7C *active_script_link;
    RuntimePointerRegion *active_pointer_region;
};
struct RuntimeCommandBounds
{
    uint32_t first;
    uint32_t second;
    uint32_t width;
    uint32_t height;
};

struct DisplayRectangle;

struct DisplayRectangle;

struct DisplayRectangleTransform
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
};

struct DisplaySceneSurface
{
    int32_t width;
    int32_t height;
};

struct DisplayTraversalState;
struct DisplaySyncRequest;
struct DisplaySceneNode;

enum class DisplaySceneStorage : uint32_t
{
    XRGB_COMPOSITION,
    INDEXED_SOURCE,
};

using DisplayRootRectangleCallback = void (*)(DisplaySceneNode *root, DisplayRectangle *rectangle, int value);
using DisplayNodeRectangleCallback = void (*)(DisplaySceneNode *root, int unused_register, int zero, DisplaySceneNode *node, DisplayRectangle *rectangle, void *node_state, uint32_t mode);

struct DisplaySceneCallbackNode
{
    uintptr_t identity;
    DisplaySceneCallbackNode *next;
    uint32_t flags;
    void *context;
    int (*callback)(DisplayTraversalState *state);
};

struct DisplayTraversalState
{
    uint32_t flags;
    uint32_t timestamp;
    uint32_t width;
    uint32_t height;
    intptr_t first_position;
    intptr_t current_position;
    void *data;
    DisplayRectangle *clip_bounds;
    void *callback_context;
};

struct DisplaySceneNode
{
    intptr_t identifier;
    uint32_t flags;
    uint32_t reference_count;
    uint32_t lock_count;
    RuntimeThreadId lock_owner_thread;
    DisplaySceneSurface *surface;
    DisplaySceneNode *next;
    intptr_t callback_first_position;
    intptr_t callback_current_position;
    intptr_t callback_alternate_position;
    int32_t sync_secondary_position;
    uint32_t scene_index;
    int32_t x;
    int32_t y;
    int32_t previous_x;
    int32_t previous_y;
    int32_t x_offset;
    int32_t y_offset;
    int32_t width;
    int32_t height;
    DisplayRectangle accumulated_rectangle;
    uint32_t state_60;
    DisplaySceneCallbackNode *callbacks;
    uint32_t owner_count;
    intptr_t primary_owner;
    intptr_t owners[128];
    DisplayNodeRectangleCallback rectangle_callback;
    DisplayRootRectangleCallback root_rectangle_callback;
    intptr_t callback_position;
    DisplayPixelFormatDescriptor rectangle_callback_format;
    uint32_t palette_source[256];
    uint32_t palette_mapping[256];
    DisplaySceneStorage storage;
    intptr_t indexed_backing;
};


struct DisplaySyncRequest
{
    DisplaySceneNode *node;
    DisplayRectangle *geometry;
    int32_t *secondary_position;
    intptr_t *primary_position;
};

using DisplaySceneSynchronizeCallback = int (*)(void *context, void *payload, uint32_t mode);

// Provides fallback layer storage for ownerless script declarations.


enum class RuntimeScriptOpcodeDisposition : uint32_t
{
    UNHANDLED,
    COMPLETE,
    PAUSE,
    COMMIT_CURSOR,
    FINISH_LINK,
    RESTART_OUTER,
    RESTART_OUTER_COMMIT_CURSOR
};


} // namespace freegag
