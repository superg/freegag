#pragma once

#include "runtime_tree_types.h"

namespace gag
{
struct RuntimePathApi
{
    void (*enter_lock)();
    void (*leave_lock)();
};



struct ScreenshotApi
{
    BOOL(WINAPI *get_save_file_name)(LPOPENFILENAMEA file_name);
    void *(*capture_bitmap)(void *snapshot_context, uint32_t *size, int mode);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD size, LPDWORD written, LPOVERLAPPED overlapped);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};



#pragma pack(push, 1)
struct BitmapCaptureSource
{
    uint8_t unknown_0000[8];
    uint32_t format_marker;
    uint8_t unknown_000c[0x14];
    uint16_t width;
    uint16_t height;
    const uint8_t *pixels;
};
#pragma pack(pop)

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
    const PALETTEENTRY *palette_entries;
};


struct BitmapCaptureApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};



struct RuntimeQueueApi
{
    void (*enter_pair_lock)();
    void (*leave_pair_lock)();
    void (*enter_queue_lock)();
    void (*leave_queue_lock)();
    void (*enter_byte_lock)();
    void (*leave_byte_lock)();
};



struct RuntimeMessagePair
{
    uint32_t first;
    uint32_t second;
};



struct RuntimePlanModeSyncApi
{
    bool (*set_inactive)();
    bool (*clear_inactive)();
    void (*rebuild)();
};


struct RuntimePendingTreeSwitchApi
{
    void (*destroy_resources)(void *identity);
    RuntimeTreeNode *(*activate_tree)(const char *first, const char *second, void *third, void *fourth);
    void (*finalize_current_tree)(void *identity);
    void (*rebuild_runtime_plans)(void *identity);
    uint32_t (*update_pointer)(int32_t x, int32_t y);
};



struct RuntimeTreeActivationApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    RuntimeGenericResourceNode *(*find_or_load_resource)(const char *name);
    RuntimeTreeNode *(*create_tree_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *name, void *creation_context);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void (*activate_comment)(RuntimeTreeNode *node);
};



struct RuntimePairDispatchApi
{
    int (*dequeue_pair)(RuntimeMessagePair *pair);
    uint32_t (*move_pointer)(int32_t x, int32_t y);
    uint32_t (*left_button_down)();
    uint32_t (*left_button_up)();
    uint32_t (*right_button_down)();
};



struct RuntimeInputSessionRecord
{
    uint32_t values[8];
};



struct RuntimeInputSessionApi
{
    void (*reset_byte_queue)();
    uint32_t (*get_time)();
    RuntimeLockRecord *(*acquire_record)(void *selector);
    uint32_t (*initialize_text)(const char *text, uint32_t x, uint32_t y, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state);
    uint32_t (*find_scene_index)(uint32_t flags);
    DisplaySceneNode *(*lock_scene)(intptr_t identifier);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_update)(intptr_t identifier);
    void (*draw_text)(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);
    uint32_t (*end_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void (*unlock_scene)(intptr_t identifier);
    void (*release_record)(RuntimeLockRecord *record);
};

struct RuntimeTextInputSceneRedrawApi
{
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_update)(intptr_t identifier);
    uint32_t (*end_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
};



struct RuntimeCommandLoopState
{
    HWND window;
    uint8_t unknown_004[8];
    uint8_t resource_archive_state;
    uint8_t unknown_00d[0x103];
    char resource_directory[0x104];
    void *display_scene_host;
    uint8_t unknown_218[0x10];
    DisplayPixelFormatDescriptor display_pixel_format;
    intptr_t input_alternate_scene_identifier;
    char first_runtime_path[0x104];
    char second_runtime_path[0x104];
    void *command_context;
    uint16_t width;
    uint16_t height;
    void *display_surface;
    uint32_t callback_first_position_1;
    uint32_t callback_first_position_2;
    uint32_t callback_first_position_3;
    PALETTEENTRY *palette_entries;
    uint8_t unknown_490[0x94];
    RuntimeGameDllWindowProcedure game_dll_window_procedure;
    RuntimeGameDllExecute game_dll_execute;
    uint32_t game_result_type;
    uint8_t game_result_data[0x104];
    char input_text[0x20];
    RuntimeStandaloneTextState input_text_state;
    intptr_t input_scene_identifier;
    uint32_t input_text_flags;
    uint32_t input_scene_index;
    uint32_t input_caret_tick;
    uint32_t input_cursor;
    uint32_t input_end;
    uint32_t pair_available;
    RuntimeMessagePair pair_queue[0x20];
    uint32_t pair_read_index;
    uint32_t pair_write_index;
    uint32_t byte_available;
    uint8_t byte_queue[0x20];
    uint32_t byte_read_index;
    uint32_t byte_write_index;
    uint32_t message_available;
    uint32_t message_queue[0x20];
    uint32_t message_read_index;
    uint32_t message_write_index;
    CdfArchive *active_archive;
    union
    {
        AsyncFileHost *async_file_host;
        intptr_t archive_alternate_stream;
    };
    void *resource_cache_parent_identity;
    CRITICAL_SECTION byte_queue_critical_section;
    CRITICAL_SECTION pair_queue_critical_section;
    CRITICAL_SECTION message_queue_critical_section;
    CRITICAL_SECTION resource_critical_section;
    CRITICAL_SECTION path_critical_section;
    HANDLE resource_heap;
    HANDLE script_thread;
    uint8_t unknown_900[4];
    void *media_objects_parent_identity;
    uint32_t resource_wait_count;
    uint32_t accumulated_tree_flags;
    uint32_t reset_value_1;
    uint32_t reset_value_2;
    uint32_t reset_value_3;
    uint32_t nested_runtime_state_count;
    uint32_t nested_runtime_state_4_count;
    uint32_t resource_count;
    uint32_t external_command_pending;
    uint32_t target_flags;
    uint32_t flags;
    int32_t resource_host_mode;
    uint32_t script_clock;
    int32_t scene_x;
    int32_t scene_y;
    uint8_t unknown_944[0x10];
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
struct RuntimeTextInputApi
{
    uint8_t (*dequeue_byte)();
    uint32_t (*time_get_time)();
    uint32_t (*initialize_text)(const char *text, uint32_t x, uint32_t y, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_update)(intptr_t identifier);
    void (*draw_text)(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);
    uint32_t (*end_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
};



struct RuntimeCommandBounds
{
    uint32_t first;
    uint32_t second;
    uint32_t width;
    uint32_t height;
};

struct DisplayRectangle;

struct RuntimeMessageProcessorApi
{
    uint32_t (*dequeue_message)();
    void (*handle_message_30f)();
    void (*handle_message_311)();
    uint32_t (*query_state)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    bool (*update_target)(void *target, RuntimeCommandBounds *bounds, int enabled);
    void (*present)();
};



struct RuntimeTargetUpdateApi
{
    void (*draw_bounds)(RuntimeCommandBounds *bounds, int mode);
    int (*begin_target)(uint32_t height, uint32_t second, uint32_t width);
    uint32_t (*end_target)();
};



struct DisplayLockReleaseApi
{
    DWORD(WINAPI *get_current_thread_id)();
    BOOL(WINAPI *set_event)(HANDLE event);
};

struct DisplayRectangle;

struct DisplayLockAcquireApi
{
    DWORD(WINAPI *get_current_thread_id)();
    DWORD(WINAPI *wait_for_single_object)(HANDLE object, DWORD milliseconds);
    void(WINAPI *sleep)(DWORD milliseconds);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *reset_event)(HANDLE event);
};



struct DisplayRectangleTransform
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
};

struct DisplaySceneSurface
{
    uint8_t unknown_00[0x34];
    int32_t width;
    int32_t height;
};

struct DisplayTraversalState;
struct DisplaySyncRequest;
struct DisplaySceneNode;

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
    DWORD timestamp;
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
    DWORD lock_owner_thread;
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
};


struct DisplaySceneCallbackApi
{
    uint32_t (*time_get_time)();
};

struct DisplaySyncRequest
{
    DisplaySceneNode *node;
    DisplayRectangle *geometry;
    int32_t *secondary_position;
    intptr_t *primary_position;
};

struct DisplaySceneSyncApi
{
    int (*synchronize)(void *context, void *payload, uint32_t mode);
};

struct DisplaySceneMemoryApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};



struct DisplaySceneHostApi
{
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION critical_section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION critical_section);
    HANDLE(WINAPI *create_event)(LPSECURITY_ATTRIBUTES attributes, BOOL manual_reset, BOOL initial_state, LPCSTR name);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_routine, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
};

struct DisplaySceneWorkerApi
{
    uint32_t (*time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *dirty_flags);
    int (*synchronize_node)(DisplaySceneNode *node, DisplayRectangle *rectangle);
    void (*publish_node)(DisplaySceneNode *node);
    uint32_t (*release_mode_1000)();
    uint32_t (*release_lock)();
};



struct FramebufferInvalidateApi
{
    uint32_t (*acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    uint32_t (*dispatch_update)(void *target, uint32_t options);
    uint32_t (*release_lock)();
};



// Provides fallback layer storage for ownerless script declarations.


struct DisplayRootRegionApi
{
    uint32_t (*begin_scene_update)(intptr_t identifier);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
};



struct ClearRuntimeDisplayApi
{
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    uint32_t (*release_display_lock)();
    uint32_t (*update_root_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);
};



struct RuntimeCommandLoopApi
{
    void (*begin_first)();
    void (*begin_second)();
    void (*begin_third)(int value);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*process)(RuntimeCommandLoopState *state);
    LPARAM (*get_script_state)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*cancel_first)();
    void (*cancel_second)();
    void (*cancel_third)();
    void (*complete_first)();
};

struct RuntimeSessionResetApi
{
    uint32_t (*stop_game_dll)();
    RuntimeTreeNode *(*get_tree_root)();
    void (*destroy_tree_resources)(void *identity);
    intptr_t (*deactivate_tree)(void *identity, void *replacement_identity);
    void (*reset_display_state)();
    void (*request_resource_destruction)(void *identity);
    void (*destroy_fixed_name_nodes)();
    void (*purge_named_nodes)();
    void (*destroy_object_states)();
    void (*destroy_visual_objects)();
    void (*clear_command_definitions)();
    void (*remove_generic_resources)();
    uint32_t (*close_archive)(CdfArchive *archive);
    uint32_t (*destroy_async_host)(AsyncFileHost *host);
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    RuntimeNamedNode *(*get_named_node)(const char *name);
    uint32_t (*get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
};



struct RuntimeExternalCommandApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*process_message)(RuntimeCommandLoopState *state);
    int (*run_command_loop)(RuntimeCommandLoopState *state);
    void(WINAPI *sleep)(DWORD milliseconds);
};



enum class RuntimeScriptOpcodeDisposition : uint32_t
{
    unhandled,
    complete,
    pause,
    commit_cursor,
    finish_link,
    restart_outer,
    restart_outer_commit_cursor
};


struct RuntimeScriptExecutorApi
{
    uint32_t (*get_tick_count)();
    uint32_t (*time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*process_children)(uint32_t maximum_end_position);
    void (*process_message)(RuntimeCommandLoopState *state);
    void (*process_text_input)(RuntimeCommandLoopState *state);
    uint32_t (*process_pair_message)();
    int (*run_command_loop)(RuntimeCommandLoopState *state);
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    bool (*synchronize_plan_mode)();
    bool (*process_pending_tree_switch)(RuntimeTreeNode *node);
    void (*acknowledge_event)();
    uint32_t (*run_external_command)();
    uint32_t (*activate_link)(RuntimeTreeLink7C *link);
    uint32_t (*parse_opcode)(ScriptParserState *parser);
    RuntimeScriptOpcodeDisposition (*dispatch_opcode)(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, uint32_t opcode, int32_t random_value, uint32_t saved_cursor);
    int32_t (*select_random)(int32_t minimum, int32_t maximum);
};



struct CursorStateApi
{
    BOOL(WINAPI *get_cursor_position)(LPPOINT point);
    int(WINAPI *get_system_metrics)(int index);
};



} // namespace gag
