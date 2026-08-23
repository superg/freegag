#pragma once

#include <windows.h>
#include <commdlg.h>
#include <stddef.h>
#include <stdint.h>

namespace gag
{

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
};

struct ApplicationState
{
    HINSTANCE instance;
    int32_t width;
    int32_t height;
    uint32_t validation_flags;
    void *archive_context;
    const char *message_table;
    HWND window;
    uint8_t unknown_001c[4];
    HWND capture_window;
    HMENU game_menu;
    HMENU options_menu;
    HMENU system_menu;
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
    int32_t window_vertical_offset;
    int32_t window_top_adjustment;
    RECT desktop_window_rect;
    uint32_t flags;
};


struct ValidationApi
{
    HWND(WINAPI *find_window)(LPCSTR class_name, LPCSTR window_name);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    HANDLE(WINAPI *find_first_file)(LPCSTR path, LPWIN32_FIND_DATAA find_data);
    BOOL(WINAPI *find_close)(HANDLE find);
    uint32_t (*load_preferences)(ApplicationState *state);
    uint32_t (*enable_borderless_fullscreen)(ApplicationState *state);
};



struct WindowClassApi
{
    HBRUSH(WINAPI *create_solid_brush)(COLORREF color);
    HICON(WINAPI *load_icon)(HINSTANCE instance, LPCSTR name);
    HCURSOR(WINAPI *load_cursor)(HINSTANCE instance, LPCSTR name);
    ATOM(WINAPI *register_class_ex)(const WNDCLASSEXA *window_class);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    WNDPROC primary_window_procedure;
    WNDPROC capture_window_procedure;
};


struct GraphicsHostInitializationResult
{
    uintptr_t message_window;
    HWND capture_window;
    uint8_t unknown_0008[0x458];
    uint32_t bits_per_pixel;
};


struct RuntimeNamedNode;

struct GraphicsHostApi
{
    uint32_t (*initialize_media)();
    uint32_t (*initialize_async)();
    uint32_t (*initialize_generic)();
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    ATOM(WINAPI *register_class)(const WNDCLASSA *window_class);
    HWND(WINAPI *create_window_ex)
    (DWORD extended_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter);
    BOOL(WINAPI *get_cursor_position)(LPPOINT point);
    BOOL(WINAPI *screen_to_client)(HWND window, LPPOINT point);
    uint32_t (*initialize_display)(HWND window, uint32_t options);
    void (*set_script_root)(ScriptRuntimeRoot *root);
    RuntimeNamedNode *(*get_or_create_named_node)(const char *name);
    void (*set_named_node_enabled)(void *identity, int enabled);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *show_window)(HWND window, int command);
};


struct GraphicsHostShutdownApi
{
    uint32_t (*shutdown_display)();
    uint32_t (*shutdown_generic_backend)();
    uint32_t (*shutdown_async_files)();
    uint32_t (*shutdown_media_backend)();
    void (*shutdown_presenter)();
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *heap_destroy)(HANDLE heap);
    BOOL(WINAPI *destroy_window)(HWND window);
};



struct RuntimeBootstrapApi
{
    void *(*create_surface)(int32_t width, int32_t height);
    PALETTEENTRY *(*get_palette_entries)();
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
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    LPTHREAD_START_ROUTINE script_thread_entry;
};



struct RuntimeScriptPropertySetApi
{
    void (*select_resource)(char *path);
    BOOL (*release_memory_resource)(const char *path);
    void (*set_property_value)(uint32_t value);
    void (*enter_state_1000)();
    void (*leave_state_1000)();
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*destroy_resource_tree)(void *root);
};



struct RuntimeScriptPropertyGetApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    int (*copy_string)(char *destination, const char *source);
    void (*load_resource)(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);
    uint32_t (*get_property_value)();
    uint16_t (*query_frame_number)(void *identity);
};



struct ApplicationInitializationApi
{
    UINT(WINAPI *set_error_mode)(UINT mode);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    bool (*register_window_classes)(ApplicationState *state);
    int (*copy_string)(char *destination, const char *source);
    int (*validate_environment)(ApplicationState *state, const char *requested_archive, uint32_t stages);
    int(WINAPI *get_system_metrics)(int index);
    BOOL(WINAPI *adjust_window_rect)(LPRECT rectangle, DWORD style, BOOL menu);
    HWND(WINAPI *create_window_ex)
    (DWORD extended_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter);
    BOOL(WINAPI *show_window)(HWND window, int command);
    BOOL(WINAPI *set_window_position)(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags);
    BOOL(WINAPI *get_client_rect)(HWND window, LPRECT rectangle);
    GraphicsHostInitializationResult *(*initialize_graphics_host)(HINSTANCE instance, HWND window, int x, int y, int16_t width, uint16_t height, uint32_t flags);
    GraphicsHostInitializationResult *(*initialize_runtime)();
    void (*update_window_layout)(ApplicationState *state, SecondaryWindowLayout *secondary_layout);
    void (*enable_runtime)();
    void (*set_active_object_field)(uint32_t value);
    uint32_t (*detect_resource_type)(const char *data);
};



struct RuntimeBackendInitializationApi
{
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    HANDLE(WINAPI *create_mutex)(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name);
    void (*initialize_sound)();
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
};



struct RuntimeGenericBackendShutdownApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint32_t (*destroy_backend)(void *identity);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};



struct WindowProcedureApi
{
    LONG_PTR(WINAPI *get_window_long)(HWND window, int index);
    LONG_PTR(WINAPI *set_window_long)(HWND window, int index, LONG_PTR value);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    HMENU(WINAPI *get_system_menu)(HWND window, BOOL revert);
    BOOL(WINAPI *delete_menu)(HMENU menu, UINT position, UINT flags);
    HMENU(WINAPI *create_menu)();
    HMENU(WINAPI *create_popup_menu)();
    BOOL(WINAPI *append_menu)(HMENU menu, UINT flags, UINT_PTR item, LPCSTR text);
    DWORD(WINAPI *check_menu_item)(HMENU menu, UINT item, UINT check);
    BOOL(WINAPI *enable_menu_item)(HMENU menu, UINT item, UINT enable);
    BOOL(WINAPI *set_menu)(HWND window, HMENU menu);
    BOOL(WINAPI *destroy_window)(HWND window);
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*update_cursor_state)(ApplicationState *state, int active);
};

struct MainWindowProcedureApi
{
    LONG_PTR(WINAPI *get_window_long)(HWND window, int index);
    LONG_PTR(WINAPI *set_window_long)(HWND window, int index, LONG_PTR value);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(WINAPI *post_quit_message)(int exit_code);
    BOOL(WINAPI *reply_message)(LRESULT result);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *destroy_window)(HWND window);
    uintptr_t (*get_script_state)();
    ScriptObjectState *(*resolve_state_field)(const char *object_name, const char *field_name, const void *value, int value_type);
    void *(*capture_bitmap)(void *game_context, uint32_t *size, int half_resolution);
    void (*free_memory)(void *memory);
    void (*set_application_lock)(ApplicationState *state);
    void (*clear_runtime_active)(ApplicationState *state);
    int (*validate_startup)(ApplicationState *state, const char *requested_archive, uint32_t stages);
    void (*set_runtime_flag_40)();
};



struct LocalPreferencesApi
{
    DWORD(WINAPI *get_full_path_name)(LPCSTR file_name, DWORD size, LPSTR path, LPSTR *file_part);
    DWORD(WINAPI *read_value)(LPCSTR section, LPCSTR key, LPCSTR default_value, LPSTR value, DWORD size, LPCSTR file_name);
    BOOL(WINAPI *write_value)(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR file_name);
    BOOL(WINAPI *get_window_rect)(HWND window, LPRECT rectangle);
    BOOL(WINAPI *get_window_placement)(HWND window, WINDOWPLACEMENT *placement);
    HMONITOR(WINAPI *monitor_from_rect)(LPCRECT rectangle, DWORD flags);
};


struct CursorVisibilityApi
{
    int(WINAPI *show_cursor)(BOOL show);
    void (*on_cursor_hidden)();
    void (*on_cursor_shown)();
};



struct RuntimeTreeNode;


struct ApplicationStateFieldQuery
{
    char object_name[0x20];
    char field_name[0x20];
};



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


struct WindowLayoutApi
{
    int(WINAPI *get_system_metrics)(int index);
    BOOL(WINAPI *adjust_window_rect)(LPRECT rect, DWORD style, BOOL menu);
    LONG(WINAPI *set_window_long)(HWND window, int index, LONG value);
    BOOL(WINAPI *set_window_position)(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags);
    BOOL(WINAPI *get_client_rect)(HWND window, LPRECT rect);
    HWND(WINAPI *set_focus)(HWND window);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *get_window_rect)(HWND window, LPRECT rect);
    HMONITOR(WINAPI *monitor_from_window)(HWND window, DWORD flags);
    BOOL(WINAPI *get_monitor_info)(HMONITOR monitor, LPMONITORINFO info);
    BOOL(WINAPI *invalidate_rect)(HWND window, const RECT *rect, BOOL erase);
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
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    HWND (*get_message_window)();
};



} // namespace gag
