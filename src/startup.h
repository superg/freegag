#pragma once

#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <stddef.h>
#include <stdint.h>
namespace gag
{

struct DisplayMode;
struct CdfArchive;
struct AsyncFileHost;
struct AsyncFileRecord;
struct LegacyDisplayPixelFormat;
struct LegacyDirectDrawSurfaceDescriptor;
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
};

struct RuntimePresentationTarget
{
    HWND window;
    HDC destination_context;
    uint32_t bits_per_pixel;
    HPALETTE palette;
    uint32_t field_0944;
    HDC source_context;
    uint32_t tail[4];
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
    int32_t display_bits_per_pixel;
    int32_t display_width;
    int32_t display_height;
    DisplayMode *display_mode_iterator;
};


struct ValidationApi
{
    HWND(WINAPI *find_window)(LPCSTR class_name, LPCSTR window_name);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    HANDLE(WINAPI *find_first_file)(LPCSTR path, LPWIN32_FIND_DATAA find_data);
    BOOL(WINAPI *find_close)(HANDLE find);
    DWORD(WINAPI *get_module_file_name)(HMODULE module, LPSTR path, DWORD size);
    HDC(WINAPI *create_information_context)(LPCSTR driver, LPCSTR device, LPCSTR output, const DEVMODEA *mode);
    int(WINAPI *get_device_caps)(HDC context, int index);
    BOOL(WINAPI *delete_context)(HDC context);
    uint32_t (*load_registry)(ApplicationState *state);
    void (*locate_drive)(ApplicationState *state, const char *requested_archive);
    uint32_t (*measure_read_speed)(const char *archive_path, uint32_t bytes_to_measure);
    uint32_t (*detect_alternate_mode)(ApplicationState *state);
};

// GAG.EXE: 0x0041F040
int validate_startup_environment(ApplicationState *state, const char *requested_archive, uint32_t stages);

void set_validation_api_for_testing(const ValidationApi &api);

struct WindowClassApi
{
    HBRUSH(WINAPI *create_solid_brush)(COLORREF color);
    HICON(WINAPI *load_icon)(HINSTANCE instance, LPCSTR name);
    HCURSOR(WINAPI *load_cursor)(HINSTANCE instance, LPCSTR name);
    ATOM(WINAPI *register_class_ex)(const WNDCLASSEXA *window_class);
    ATOM(WINAPI *register_class)(const WNDCLASSA *window_class);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    WNDPROC primary_window_procedure;
    WNDPROC capture_window_procedure;
    WNDPROC custom_control_procedure;
};

// GAG.EXE: 0x0041F4E0
void initialize_application_state_no_op(ApplicationState *state);

// GAG.EXE: 0x0041F3D0
bool register_gag_window_classes(ApplicationState *state);

// GAG.EXE: 0x004174B0
uint32_t register_custom_control_class(HINSTANCE instance);

void set_window_class_api_for_testing(const WindowClassApi &api);
void reset_custom_control_registration_for_testing();
void set_custom_control_registration_state_for_testing(uint32_t registered, HINSTANCE instance);

struct GraphicsHostInitializationResult
{
    uintptr_t unknown_0000;
    HWND capture_window;
    uint8_t unknown_0008[0x458];
    uint32_t bits_per_pixel;
};


struct RuntimeNamedNode;

struct GraphicsHostApi
{
    DWORD(WINAPI *gdi_set_batch_limit)(DWORD limit);
    uint32_t (*initialize_media)(HINSTANCE instance);
    uint32_t (*register_control)(HINSTANCE instance);
    uint32_t (*initialize_async)();
    uint32_t (*initialize_generic)();
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    HCURSOR(WINAPI *load_cursor)(HINSTANCE instance, LPCSTR name);
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

// GAG.EXE: 0x0041FA00
GraphicsHostInitializationResult *initialize_graphics_host(HINSTANCE instance, HWND parent, int x, int y, int16_t width, uint16_t height, uint32_t flags);

struct GraphicsHostShutdownApi
{
    uint32_t (*shutdown_display)();
    uint32_t (*shutdown_generic_backend)();
    uint32_t (*shutdown_async_files)();
    uint32_t (*shutdown_media_backend)();
    void (*shutdown_display_modes)();
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *heap_destroy)(HANDLE heap);
    BOOL(WINAPI *destroy_window)(HWND window);
};

// GAG.EXE: 0x00420230
uint32_t shutdown_graphics_host();

void set_graphics_host_shutdown_api_for_testing(const GraphicsHostShutdownApi &api);
void set_graphics_host_shutdown_state_for_testing(HANDLE heap, HWND window);

void set_graphics_host_api_for_testing(const GraphicsHostApi &api);
void reset_graphics_host_state_for_testing(uint32_t scene_flags);
void get_graphics_host_observed_state_for_testing(RuntimeGameHostContext *context, void **callbacks, int32_t *pointer_x, int32_t *pointer_y, uint32_t *target_flags, HANDLE *resource_heap);
HWND get_runtime_display_window_for_testing();

struct RuntimeBootstrapApi
{
    DisplayMode *(*find_current_mode)();
    void *(*create_surface)(int32_t width, int32_t height, const LegacyDisplayPixelFormat *format, uint32_t options);
    HDC (*get_palette_dc)();
    HDC (*get_palette_dib_dc)();
    HPALETTE (*get_palette_handle)();
    HBITMAP (*get_palette_bitmap)();
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

// GAG.EXE: 0x0041FEA0
GraphicsHostInitializationResult *initialize_runtime_graphics(const LegacyDisplayPixelFormat *format);

void set_runtime_bootstrap_api_for_testing(const RuntimeBootstrapApi &api);
void get_runtime_bootstrap_state_for_testing(DisplayPixelFormatDescriptor *format, intptr_t *scene_identifier, HANDLE *thread);

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

// GAG.EXE: 0x004202D0
void set_runtime_script_property(uint32_t property, void *context, void *value);

void set_runtime_script_property_api_for_testing(const RuntimeScriptPropertySetApi &api);
void reset_runtime_script_property_state_for_testing(uint32_t value_1, uint32_t value_2, uint32_t value_3, uint32_t state_1000_count, uint32_t state_4_count);
void get_runtime_script_property_state_for_testing(uint32_t *value_1, uint32_t *value_2, uint32_t *value_3, uint32_t *state_1000_count, uint32_t *state_4_count, uint32_t *scene_flags,
    int32_t *host_mode);

struct RuntimeScriptPropertyGetApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    int (*copy_string)(char *destination, const char *source);
    void (*load_resource)(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);
    uint32_t (*get_property_value)();
    uint16_t (*query_frame_number)(void *identity);
};

// GAG.EXE: 0x004204B0
void get_runtime_script_property(uint32_t property, void **value, void *result);

void set_runtime_script_property_get_api_for_testing(const RuntimeScriptPropertyGetApi &api);
void set_runtime_script_property_get_state_for_testing(const char *path, int32_t pointer_x, int32_t pointer_y);

struct ApplicationInitializationApi
{
    UINT(WINAPI *set_error_mode)(UINT mode);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    void (*initialize_state)(ApplicationState *state);
    bool (*register_window_classes)(ApplicationState *state);
    uint32_t (*register_control_class)(HINSTANCE instance);
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
    void (*switch_display_mode)(ApplicationState *state, int restore_current);
    GraphicsHostInitializationResult *(*initialize_runtime)(const LegacyDisplayPixelFormat *format);
    void (*update_window_layout)(ApplicationState *state, SecondaryWindowLayout *secondary_layout);
    void (*enable_runtime)();
    void (*set_active_object_field)(uint32_t value);
    uint32_t (*detect_resource_type)(const char *data);
};

// GAG.EXE: 0x0041F4F0
ApplicationState *initialize_gag_application(int width, int height, HINSTANCE instance, LPSTR command_line, int show_command);

void set_application_initialization_api_for_testing(const ApplicationInitializationApi &api);
void set_gagboy_startup_mode_for_testing(bool enabled);

struct RuntimeBackendInitializationApi
{
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    HANDLE(WINAPI *create_mutex)(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name);
    void (*initialize_sound_class)(HINSTANCE instance);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x00429DF0
uint32_t initialize_runtime_media_backend(HINSTANCE instance);

// GAG.EXE: 0x00410B70
uint32_t initialize_runtime_generic_backend();

struct RuntimeGenericBackendShutdownApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint32_t (*destroy_backend)(void *identity);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};

// GAG.EXE: 0x00410BD0
uint32_t shutdown_runtime_generic_backend();

void set_runtime_generic_backend_shutdown_api_for_testing(const RuntimeGenericBackendShutdownApi &api);

// GAG.EXE: 0x00414E10
uint32_t initialize_async_file_subsystem();

void set_runtime_backend_initialization_api_for_testing(const RuntimeBackendInitializationApi &api);
void set_runtime_backend_initialization_state_for_testing(bool media_initialized, bool generic_initialized, bool async_initialized);
HANDLE get_runtime_media_backend_heap_for_testing();
HANDLE get_runtime_media_backend_mutex_for_testing();
HANDLE get_runtime_generic_backend_mutex_for_testing();

// GAG.EXE: 0x00404970
void set_script_runtime_root_if_valid(ScriptRuntimeRoot *root);

// GAG.EXE: 0x00407EA0
void set_runtime_named_node_enabled(void *identity, int enabled);

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
    void (*application_hook_1)();
    void (*set_application_lock)(ApplicationState *state);
    void (*clear_runtime_active)(ApplicationState *state);
    int (*validate_startup)(ApplicationState *state, const char *requested_archive, uint32_t stages);
    void (*set_runtime_flag_40)();
};

// GAG.EXE: 0x0041D560
LRESULT CALLBACK gag_main_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void set_main_window_procedure_api_for_testing(const MainWindowProcedureApi &api);

// GAG.EXE: 0x0041E680
LRESULT CALLBACK gag_capture_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void set_window_procedure_api_for_testing(const WindowProcedureApi &api);

struct CustomControlState
{
    uint8_t unknown_0000[0x48];
    HDC destination_context;
    HDC source_context;
    HPALETTE palette;
    HPALETTE previous_palette;
    HBITMAP bitmap;
    HGDIOBJ previous_bitmap;
    RECT client_rect;
    int32_t bits_per_pixel;
    int32_t source_width;
    int32_t source_height;
    const void *bitmap_identity;
    char *archive_path;
    char *comment_text;
    void *dialog_state;
};


struct CustomControlGdiApi
{
    HDC(WINAPI *get_context)(HWND window);
    HDC(WINAPI *create_compatible_context)(HDC context);
    int(WINAPI *get_device_caps)(HDC context, int index);
    BOOL(WINAPI *get_client_rect)(HWND window, LPRECT rect);
    UINT(WINAPI *set_system_palette_use)(HDC context, UINT use);
    HPALETTE(WINAPI *create_palette)(const LOGPALETTE *palette);
    HPALETTE(WINAPI *select_palette)(HDC context, HPALETTE palette, BOOL background);
    int(WINAPI *set_stretch_blt_mode)(HDC context, int mode);
    BOOL(WINAPI *unrealize_object)(HGDIOBJ object);
    UINT(WINAPI *realize_palette)(HDC context);
    BOOL(WINAPI *stretch_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD operation);
    HGDIOBJ(WINAPI *select_object)(HDC context, HGDIOBJ object);
    BOOL(WINAPI *delete_object)(HGDIOBJ object);
    int(WINAPI *release_context)(HWND window, HDC context);
    BOOL(WINAPI *delete_context)(HDC context);
    HBITMAP(WINAPI *create_dib_section)(HDC context, const BITMAPINFO *info, UINT usage, VOID **bits, HANDLE section, DWORD offset);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *set_dib_color_table)(HDC context, UINT start, UINT count, const RGBQUAD *colors);
};

// GAG.EXE: 0x00417AB0
void initialize_custom_control_gdi(HWND window, CustomControlState *state);

// GAG.EXE: 0x00417B60
void set_custom_control_bitmap(CustomControlState *state, BITMAPINFO *bitmap, int present);

// GAG.EXE: 0x00417CB0
void realize_and_present_custom_control(CustomControlState *state, BOOL background);

// GAG.EXE: 0x00417D10
void destroy_custom_control_gdi(HWND window, CustomControlState *state);

// GAG.EXE: 0x00417D90
LRESULT CALLBACK gag_custom_control_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void set_custom_control_gdi_api_for_testing(const CustomControlGdiApi &api);

struct CustomControlWindowApi
{
    BOOL(WINAPI *get_update_rect)(HWND window, LPRECT rect, BOOL erase);
    HDC(WINAPI *begin_paint)(HWND window, LPPAINTSTRUCT paint);
    BOOL(WINAPI *end_paint)(HWND window, const PAINTSTRUCT *paint);
    LONG_PTR(WINAPI *get_window_long)(HWND window, int index);
    LONG_PTR(WINAPI *set_window_long)(HWND window, int index, LONG_PTR value);
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *pattern_blt)(HDC context, int x, int y, int width, int height, DWORD operation);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    HRSRC(WINAPI *find_resource)(HMODULE module, LPCSTR name, LPCSTR type);
    HMODULE(WINAPI *get_module_handle)(LPCSTR name);
    HGLOBAL(WINAPI *load_resource)(HMODULE module, HRSRC resource);
    LPVOID(WINAPI *lock_resource)(HGLOBAL resource);
    BOOL(WINAPI *free_resource)(HGLOBAL resource);
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    uint32_t (*get_entry_size)(CdfArchive *archive, uint8_t selector, const char *name);
    int (*read_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    uint32_t (*close_archive)(CdfArchive *archive);
    bool (*strings_equal)(const char *left, const char *right);
    int (*copy_string)(char *destination, const char *source);
    void (*initialize_gdi)(HWND window, CustomControlState *state);
    void (*set_bitmap)(CustomControlState *state, BITMAPINFO *bitmap, int present);
    void (*realize_and_present)(CustomControlState *state, BOOL background);
    void (*destroy_gdi)(HWND window, CustomControlState *state);
};

void set_custom_control_window_api_for_testing(const CustomControlWindowApi &api);

struct SettingsRegistryApi
{
    LSTATUS(WINAPI *open_key)(HKEY key, LPCSTR sub_key, DWORD options, REGSAM desired_access, PHKEY result);
    LSTATUS(WINAPI *create_key)
    (HKEY key, LPCSTR sub_key, DWORD reserved, LPSTR class_name, DWORD options, REGSAM desired_access, const LPSECURITY_ATTRIBUTES security_attributes, PHKEY result, LPDWORD disposition);
    LSTATUS(WINAPI *set_value)(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type, const BYTE *data, DWORD data_size);
    LSTATUS(WINAPI *close_key)(HKEY key);
};

struct WindowPositionPersistenceApi
{
    LSTATUS(WINAPI *open_key)(HKEY key, LPCSTR sub_key, DWORD options, REGSAM desired_access, PHKEY result);
    LSTATUS(WINAPI *create_key)
    (HKEY key, LPCSTR sub_key, DWORD reserved, LPSTR class_name, DWORD options, REGSAM desired_access, const LPSECURITY_ATTRIBUTES security_attributes, PHKEY result, LPDWORD disposition);
    LSTATUS(WINAPI *query_value)(HKEY key, LPCSTR value_name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD data_size);
    LSTATUS(WINAPI *set_value)(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type, const BYTE *data, DWORD data_size);
    LSTATUS(WINAPI *close_key)(HKEY key);
    BOOL(WINAPI *get_window_rect)(HWND window, LPRECT rectangle);
    BOOL(WINAPI *get_window_placement)(HWND window, WINDOWPLACEMENT *placement);
    HMONITOR(WINAPI *monitor_from_rect)(LPCRECT rectangle, DWORD flags);
};

// GAG.EXE: 0x0041D060
void save_runtime_settings(ApplicationState *state);
bool load_saved_window_position(int32_t width, int32_t height, POINT *position);
bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, RECT *rectangle);
void save_window_position(ApplicationState *state);

struct CursorVisibilityApi
{
    int(WINAPI *show_cursor)(BOOL show);
    void (*on_cursor_hidden)();
    void (*on_cursor_shown)();
};

// GAG.EXE: 0x0041D0D0
void set_game_cursor_active(ApplicationState *state, int active);

void set_settings_registry_api_for_testing(const SettingsRegistryApi &api);
void set_window_position_persistence_api_for_testing(const WindowPositionPersistenceApi &api);
void set_cursor_visibility_api_for_testing(const CursorVisibilityApi &api);

struct RuntimeTreeNode;


struct ApplicationStateFieldQuery
{
    char object_name[0x20];
    char field_name[0x20];
};


// GAG.EXE: 0x0041D510
void finish_credits_state(ApplicationState *state, RuntimeTreeNode *tree);

void set_finish_credits_callback_for_testing(void (*callback)());

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
#if defined(FREEGAG_WINDOWS_FIXES)
    BOOL(WINAPI *get_window_rect)(HWND window, LPRECT rect);
    HMONITOR(WINAPI *monitor_from_window)(HWND window, DWORD flags);
    BOOL(WINAPI *get_monitor_info)(HMONITOR monitor, LPMONITORINFO info);
    BOOL(WINAPI *invalidate_rect)(HWND window, const RECT *rect, BOOL erase);
#endif
};

// GAG.EXE: 0x0041CE60
void update_modern_windows_windowed_viewport(ApplicationState *state);
void update_application_window_layout(ApplicationState *state, SecondaryWindowLayout *secondary_layout);

void set_window_layout_api_for_testing(const WindowLayoutApi &api);

// GAG.EXE: 0x0041D120
void restore_application_display(ApplicationState *state);

struct StateActivationApi
{
    uint32_t (*query_status)(void *identity);
    uintptr_t (*get_script_state)();
    void (*on_cursor_outside)();
};

// GAG.EXE: 0x0041D380
void process_state_activation(ApplicationState *state, RuntimeTreeNode *tree);

void set_state_activation_api_for_testing(const StateActivationApi &api);

struct SaveStateApi
{
    void *(*capture_state)(void *game_context, uint32_t *size, int mode);
    uintptr_t (*get_script_state)();
    void (*prepare_8bit_display)();
    int (*show_dialog)(void *dialog_context, char *installation_path, const char *dialog_data, void *memory, char *first_path, char *second_path);
    void (*save_state)(char *first_path, char *second_path, void *memory, uintptr_t script_state);
    void (*restore_8bit_display)();
};

// GAG.EXE: 0x0041D280
void save_application_state_interactive(ApplicationState *state, void *dialog_context);

void set_save_state_api_for_testing(const SaveStateApi &api);

struct OpenStateApi
{
    void (*prepare_8bit_display)();
    int (*show_dialog)(void *dialog_context, char *installation_path, const char *dialog_data, char *installed_version);
    void (*restore_8bit_display)(uint32_t mode);
};

// GAG.EXE: 0x0041D1C0
void open_application_state_interactive(ApplicationState *state, void *dialog_context);
void finish_application_state_load(ApplicationState *state, const char *path);

void set_open_state_api_for_testing(const OpenStateApi &api);

#if defined(GAG_TESTING)
bool uses_scripted_save_load_screens_for_testing();
#endif

struct SynchronizedStateApi
{
    void (*enter_lock)();
    void (*leave_lock)();
    int (*operation_17550)(void *first, void *second, void *third, void *fourth);
    int (*operation_175f0)(void *first, void *second, void *third, void *fourth, void *fifth, void *sixth);
    int (*operation_176a0)(void *first, void *second, void *third, void *fourth);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    HWND (*get_message_window)();
};

// GAG.EXE: 0x0041F7C0
bool run_synchronized_state_operation_17550(void *first, void *second, void *third, void *fourth);

// GAG.EXE: 0x0041F830
bool run_synchronized_state_operation_175f0(void *first, void *second, void *third, void *fourth, void *fifth, void *sixth);

// GAG.EXE: 0x0041F8F0
bool run_synchronized_state_operation_176a0(void *first, void *second, void *third, void *fourth);

void set_synchronized_state_api_for_testing(const SynchronizedStateApi &api);

using InitializeApplication = ApplicationState *(*)(int width, int height, HINSTANCE instance, LPSTR command_line, int show_command);

struct StartupApi
{
    InitializeApplication initialize_application;
    void (*set_runtime_flag_40)();
    BOOL(WINAPI *get_message)(LPMSG message, HWND window, UINT minimum, UINT maximum);
    BOOL(WINAPI *translate_message)(const MSG *message);
    LRESULT(WINAPI *dispatch_message)(const MSG *message);
    int(WINAPI *show_cursor)(BOOL show);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
};

// Non-original test seam. The call order and decisions are the body recovered at 0x0041CAE0.
int run_startup(HINSTANCE instance, LPSTR command_line, int show_command, const StartupApi &api);

StartupApi make_win32_startup_api();

struct RegistryApi
{
    LSTATUS(WINAPI *open_key)(HKEY key, LPCSTR sub_key, DWORD options, REGSAM desired_access, PHKEY result);
    LSTATUS(WINAPI *query_value)(HKEY key, LPCSTR value_name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD data_size);
    LSTATUS(WINAPI *close_key)(HKEY key);
};

// GAG.EXE: 0x0041EDF0
uint32_t load_installation_registry_settings(ApplicationState *state, const RegistryApi &api);

RegistryApi make_win32_registry_api();

// GAG.EXE: 0x0040CF50
int copy_string(char *destination, const char *source);

struct ScriptTextBuffer
{
    uint32_t length;
    uint32_t capacity;
    char *data;
};

struct ScriptUtilityApi
{
    LPVOID(WINAPI *virtual_alloc)(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protection);
    DWORD(WINAPI *get_tick_count)();
    void (*seed_random)(unsigned int seed);
    int (*random)();
};

// GAG.EXE: 0x0040CD00
int32_t select_bounded_random_value(int32_t minimum, int32_t maximum);

// GAG.EXE: 0x0040D0B0
ScriptTextBuffer *create_script_text_buffer();

// GAG.EXE: 0x0040D0E0
void clear_script_text_buffer(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D0F0
void begin_script_text_document(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D140
void end_script_text_document(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D180
void append_script_text_property(ScriptTextBuffer *buffer, uint32_t property, const char *value);

// GAG.EXE: 0x0040D400
void end_script_text_statement(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D440
void append_script_text_scope(ScriptTextBuffer *buffer, uint32_t scope);

// GAG.EXE: 0x0040D610
void append_script_text_preload_directive(ScriptTextBuffer *buffer, uint32_t scope);

// GAG.EXE: 0x0040CE90
void append_script_text_scoped_tokens(ScriptTextBuffer *buffer, uint32_t scope, const char *text);

// GAG.EXE: 0x0040D650
void append_script_text_delimiter(ScriptTextBuffer *buffer, const char *text, char delimiter);

// GAG.EXE: 0x0040D690
void append_script_text_integer(ScriptTextBuffer *buffer, uint32_t value, char delimiter);

// GAG.EXE: 0x0040D740
int find_script_property_value(char *value, const char *property_name, const char *text, uint32_t text_length, uint32_t start_offset);

// GAG.EXE: 0x0040D830
int find_script_section(const char *section_name, const char *text, int text_length);

// GAG.EXE: 0x00418230
int32_t parse_path_numeric_identifier(const char *path);

struct RuntimeTreeNode;
struct RuntimeGenericResourceNode;
struct RuntimeTreeLink84;
struct RuntimeTreePrimaryResourceLink;
struct RuntimeVisualObject;

struct ScriptParserState
{
    RuntimeTreeNode *owner;
    char *name;
    char *creation_text;
    char *scratch_text;
    uint8_t unknown_0010[4];
    RuntimeGenericResourceNode *resource;
    const char *text;
    uint32_t text_length;
    uint32_t start_offset;
    uint32_t cursor;
    RuntimeVisualObject *primary_visual;
};

// GAG.EXE: 0x0040D8A0
uint32_t parse_script_property_code(ScriptParserState *parser);

// GAG.EXE: 0x0040DC00
uint32_t parse_script_scope_code(ScriptParserState *parser);

// GAG.EXE: 0x0040DFD0
uint32_t parse_script_opcode(ScriptParserState *parser);

// GAG.EXE: 0x0040EA40
uint32_t extract_script_property_name(ScriptParserState *parser, char *name);

// GAG.EXE: 0x0040EB70
uint32_t extract_script_scope_name(ScriptParserState *parser, char *name);

// GAG.EXE: 0x0040ECB0
uint32_t extract_script_parenthesized_text(ScriptParserState *parser, char *text, uint32_t text_capacity);

// GAG.EXE: 0x0040ED80
int find_whitespace_token_index(const char *text, const char *token);

// GAG.EXE: 0x0040F0A0
uint32_t extract_script_token(ScriptParserState *parser, char *token, uint32_t token_capacity);

struct ScriptValueParseApi
{
    uint32_t (*evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);
};

struct ScriptTypedValueApi
{
    int32_t (*parse_integer_expression)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    uint32_t (*parse_value_token)(ScriptParserState *parser, char *value, uint32_t capacity);
};

struct RuntimeTreeCommandTargetApi
{
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    uint32_t (*parse_value_token)(ScriptParserState *parser, char *value, uint32_t value_capacity);
};

struct ScriptIntegerExpressionApi
{
    uint32_t (*evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);
    int32_t (*select_random)(int32_t minimum, int32_t maximum);
    RuntimeTreeLink84 *(*find_link_0084)(const void *name);
    RuntimeTreePrimaryResourceLink *(*find_primary_link)(const void *name);
    int32_t (*get_object_integer)(const char *object_name, const void *field_name);
    void (*query_runtime)(uint32_t operation, const void *source, int32_t *value);
};

// GAG.EXE: 0x00408AA0
void parse_script_typed_value(ScriptParserState *parser, void *value, uint32_t *value_type);

// GAG.EXE: 0x00408B20
void append_natural_mouse_image_flag(ScriptTextBuffer *buffer, uint32_t flags);

// GAG.EXE: 0x0040A9D0
void serialize_image_flag_overrides(ScriptTextBuffer *buffer, uint32_t flags);

// GAG.EXE: 0x0040EEB0
uint32_t parse_script_parameter_token(const char *text, int32_t token_index, void *value, uint32_t *value_type);

// GAG.EXE: 0x0040F070
uint32_t evaluate_script_parameter(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);

// GAG.EXE: 0x0040F4F0
int32_t parse_script_integer_expression(ScriptParserState *parser);

// GAG.EXE: 0x0040F2C0
uint32_t parse_script_value_token(ScriptParserState *parser, char *value, uint32_t value_capacity);

// GAG.EXE: 0x0040E580
uint32_t parse_image_flag(ScriptParserState *parser);

// GAG.EXE: 0x00421440
uint32_t parse_runtime_tree_command_target(ScriptParserState *parser, char *resource_name, char *tree_name, uint32_t *flags);
void set_runtime_tree_command_target_api_for_testing(const RuntimeTreeCommandTargetApi &api);

void set_script_value_parse_api_for_testing(const ScriptValueParseApi &api);
void set_script_typed_value_api_for_testing(const ScriptTypedValueApi &api);
void set_script_integer_expression_api_for_testing(const ScriptIntegerExpressionApi &api);

// GAG.EXE: 0x0040F380
int32_t parse_script_integer_literal(ScriptParserState *parser);

void set_script_utility_api_for_testing(const ScriptUtilityApi &api);

// GAG.EXE: 0x0040D070
bool fixed_dword_memory_equal(const void *left, const void *right, uint32_t byte_count);

struct ScriptObjectState
{
    char name[0x20];
    void *identity;
    ScriptObjectState *next;
    char field_names[32][0x20];
    uint32_t field_count;
    uint32_t flags_042c;
    char mouse_visual_name[0x20];
    char alternate_mouse_visual_name[0x20];
    RuntimeVisualObject *visual_object;
    RuntimeVisualObject *alternate_visual_object;
    uint32_t image_flags;
    uint32_t command_mask;
    uint32_t active_field_mask;
    int32_t integer_values[32];
    char string_values[32][0x20];
};


struct ScriptObjectMemoryApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

struct ScriptObjectReleaseApi
{
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct ScriptObjectFieldSnapshot
{
    char object_name[0x20];
    char field_name[0x20];
    uint32_t active;
    int32_t integer_value;
    char string_value[0x20];
};


// GAG.EXE: 0x00406580
void copy_runtime_tree_command_name(char *destination, uint32_t command);

// GAG.EXE: 0x00408340
ScriptObjectState *create_script_object_state(const void *name);

// GAG.EXE: 0x00407FA0
uint32_t parse_script_object_state(ScriptParserState *parser);

struct ScriptObjectParseApi
{
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*parse_scope)(ScriptParserState *parser);
    int32_t (*parse_integer)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    bool (*fixed_equal)(const void *left, const void *right, uint32_t bytes);
    RuntimeVisualObject *(*find_visual)(const char *name);
    ScriptObjectState *(*create_object)(const void *name);
};

void set_script_object_parse_api_for_testing(const ScriptObjectParseApi &api);
void reset_script_object_parse_api_for_testing();

// GAG.EXE: 0x00408420
ScriptObjectState *find_script_object_by_identity(void *identity);

// GAG.EXE: 0x00408480
int32_t query_or_create_script_object_field(const char *object_name, const void *field_name, uint32_t *value, int32_t value_type);

// GAG.EXE: 0x004087A0
int32_t get_script_object_integer(const char *object_name, const void *field_name);

// GAG.EXE: 0x00408800
uint32_t get_script_object_string(const char *object_name, const void *field_name, void *destination);

// GAG.EXE: 0x00408870
int32_t add_script_object_integer(const char *object_name, const void *field_name, int32_t delta);

// GAG.EXE: 0x00408900
bool compare_script_object_field(const char *object_name, const void *field_name, const void *value, int32_t value_type);

// GAG.EXE: 0x004089E0
uint32_t get_script_object_field_snapshot(const char *object_name, const void *field_name, ScriptObjectFieldSnapshot *snapshot);

void set_script_object_memory_api_for_testing(const ScriptObjectMemoryApi &api);
void set_script_object_release_api_for_testing(const ScriptObjectReleaseApi &api);

// GAG.EXE: 0x00408D80
void destroy_script_object_states();

// GAG.EXE: 0x00408B80
void serialize_script_object_states(ScriptTextBuffer *buffer);

struct ScriptObjectSlot
{
    ScriptObjectState *object;
    uint32_t *active_field_mask;
    uint32_t field_mask;
};

struct ScriptObjectContainer
{
    char name[0x20];
    void *identity;
    ScriptObjectContainer *next;
    uint32_t current_mask;
    uint32_t required_mask;
    uint32_t slot_count;
    ScriptObjectSlot slots[32];
};

struct RuntimeVisualObject
{
    char name[0x20];
    void *identity;
    RuntimeVisualObject *next;
    char file_name[0x20];
    char serialized_file[0x104];
    int32_t position_x;
    int32_t position_y;
    void *previous_scene_identity;
    void *scene_identity;
    uint32_t flags;
    uint32_t palette_flags;
};


// GAG.EXE: 0x00408DD0
uint32_t parse_runtime_visual_object(ScriptParserState *parser);

// GAG.EXE: 0x00409060
void *create_or_update_runtime_visual_object(const void *name, const void *file_name, int32_t position_x, int32_t position_y, uint32_t flags, uint32_t palette_flags);

// GAG.EXE: 0x00409210
void serialize_runtime_visual_objects(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004091B0
BOOL remove_runtime_visual_object(void *identity);

// GAG.EXE: 0x004092E0
void destroy_runtime_visual_objects();

struct RuntimePointerRegion
{
    char name[0x20];
    void *identity;
    RuntimePointerRegion *next;
    uint8_t unknown_0028[4];
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    uint8_t unknown_003c[4];
    uint32_t scene_mask;
    uint32_t first_scene_bit;
    uint32_t current_scene_bit;
    uint32_t priority;
    RuntimeVisualObject *visual_override;
    void *owner_identity;
    ScriptObjectState *state_object;
    RuntimeTreePrimaryResourceLink *primary_resource;
    void *previous_owner_identity;
    void *previous_primary_resource_identity;
};


struct RuntimeSceneSlot
{
    char name[0x20];
    RuntimeVisualObject *visual_object;
    uint32_t flags;
};

struct RuntimeResourceCacheEntry;

struct RuntimeNamedNode
{
    char name[0x20];
    void *identity;
    uint32_t flags;
    uint32_t unknown_0028;
    RuntimeNamedNode *next;
    int32_t zone_left;
    int32_t zone_top;
    int32_t zone_right;
    int32_t zone_bottom;
    uint32_t status;
    union
    {
        RuntimeNamedNode *children;
        RuntimeResourceCacheEntry *cache_entries;
    };
    union
    {
        RuntimeNamedNode *child_sentinel;
        RuntimeResourceCacheEntry *cache_entry_sentinel;
    };
    union
    {
        RuntimeNamedNode *child_cursor;
        RuntimeResourceCacheEntry *cache_entry_cursor;
    };
};


struct RuntimeResourceCacheEntry
{
    char name[0x20];
    void *data;
    uint32_t size;
    uint32_t flags_and_references;
    RuntimeResourceCacheEntry *next;
    RuntimeResourceCacheEntry *previous;
};

struct RuntimeExpandedListResource
{
    uint8_t unknown_0000[0x430];
    char primary_resource_name[0x4c];
    uint32_t link_value;
};

struct RuntimeGenericResourceNode
{
    char name[0x20];
    void *identity;
    void *resource_data;
    uint32_t current_position;
    uint32_t resource_metadata;
    uint32_t active_references;
    uint8_t unknown_0034[4];
    RuntimeGenericResourceNode *next;
};



struct RuntimeNamedNodeMemoryApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeResourceReleaseApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    RuntimeNamedNode *(*find_child)(void *parent_identity, void *child_identity);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    uint32_t (*remove_cache_entry)(void *parent_identity, void *child_identity);
    uint32_t (*close_async_record)(AsyncFileRecord *record);
    void (*set_script_flags)(uint32_t flags, int enabled);
};

struct RuntimePlanNode
{
    uint8_t unknown_0000[0x24];
    RuntimePlanNode *next;
    uint32_t flags;
};

struct RuntimeTreeSceneLink;
struct RuntimeTreeParserContext;
struct RuntimeTreePrimaryResourceLink;
struct RuntimeTreeSecondaryResourceLink;
struct RuntimeTreeLink7C;
struct RuntimeTreeLink84;
struct RuntimeTreeLink8C;
struct RuntimeTreeAuxiliaryNode;

struct RuntimeTreeNode
{
    char name[0x20];
    void *identity;
    RuntimeTreeNode *parent;
    uint8_t unknown_0028[4];
    uint32_t flags;
    uint8_t unknown_0030[0x10];
    char class_name[0x20];
    uint8_t unknown_0060[4];
    RuntimeTreeNode *iterator_current;
    uint32_t iterator_ascending;
    RuntimeTreeParserContext *parser_contexts;
    RuntimeVisualObject *default_visual;
    RuntimeTreeSceneLink *scene_link_head;
    RuntimeTreeSceneLink *scene_link_tail;
    RuntimeTreeLink7C *link_007c_head;
    RuntimeTreeLink7C *link_007c_tail;
    RuntimeTreeLink84 *link_0084_head;
    RuntimeTreeLink84 *link_0084_tail;
    RuntimeTreeLink8C *link_008c_head;
    RuntimeTreeLink8C *link_008c_tail;
    ScriptObjectContainer *container_head;
    ScriptObjectContainer *container_tail;
    RuntimeTreePrimaryResourceLink *primary_resource_link_head;
    RuntimeTreePrimaryResourceLink *primary_resource_link_tail;
    RuntimeTreeSecondaryResourceLink *secondary_resource_link_head;
    RuntimeTreeSecondaryResourceLink *secondary_resource_link_tail;
    RuntimeTreeNode *child;
    RuntimeTreeAuxiliaryNode *auxiliary_head;
    RuntimeTreeNode *next;
    RuntimeTreeNode *previous;
};


// GAG.EXE: 0x00406B40
uint32_t apply_runtime_tree_image_flags(ScriptParserState *parser);

struct RuntimeTreeAuxiliaryNode
{
    char name[0x20];
    void *identity;
    RuntimeTreeAuxiliaryNode *next;
};


struct RuntimeTreeAuxiliaryReleaseApi
{
    void (*notify)(uint32_t operation, uint32_t unused, RuntimeTreeAuxiliaryNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

// GAG.EXE: 0x004071E0
void release_runtime_tree_auxiliary_nodes(RuntimeTreeNode *owner);

void set_runtime_tree_auxiliary_release_api_for_testing(const RuntimeTreeAuxiliaryReleaseApi &api);

struct RuntimeTreeAuxiliaryCreateApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void (*resolve)(uint32_t operation, void **identity, void **metadata);
};

// GAG.EXE: 0x00407040
void add_runtime_tree_auxiliary_name(RuntimeTreeNode *owner, const char *name);

// GAG.EXE: 0x004070F0
uint32_t parse_runtime_tree_auxiliary_names(ScriptParserState *parser);

// GAG.EXE: 0x00407130
uint32_t add_default_runtime_tree_auxiliary_names(RuntimeTreeNode *owner);

void set_runtime_tree_auxiliary_create_api_for_testing(const RuntimeTreeAuxiliaryCreateApi &api);

struct RuntimeTreeDestructionCoreApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    void (*notify)(uint32_t operation, uint32_t unused, void *value);
    void (*remove_scene_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_secondary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_primary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_7c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_84)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_8c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_containers)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL (*destroy_container)(ScriptObjectContainer *container);
    void (*release_auxiliary)(RuntimeTreeNode *owner);
    void (*release_parsers)(RuntimeTreeNode *owner);
    RuntimeTreeParserContext *(*find_parser)(RuntimeTreeNode *owner, const char *name);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*update_global_links)(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);
};

// GAG.EXE: 0x00405E50
RuntimeTreeNode *destroy_runtime_tree_node(void *identity, void *replacement_identity);

// GAG.EXE: 0x00406360
void update_runtime_tree_global_links(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);

// GAG.EXE: 0x00406190
void publish_runtime_tree_global_links(RuntimeTreeNode *node);

void set_runtime_tree_destruction_core_api_for_testing(const RuntimeTreeDestructionCoreApi &api);

struct RuntimeFixedNameListNode
{
    char name[0x20];
    void *identity;
    uint32_t flags;
    char serialized_value[0x20];
    uint32_t resource_flags;
    void *resource_identity;
    void *previous_resource_identity;
    RuntimeFixedNameListNode *next;
};


// GAG.EXE: 0x0040CDA0
uint32_t parse_script_file_value(ScriptParserState *parser, char *value, char *serialized_value);

// GAG.EXE: 0x00407240
uint32_t create_or_update_runtime_fixed_name_node(ScriptParserState *parser);

// GAG.EXE: 0x004068F0
void append_script_runtime_flags(ScriptTextBuffer *buffer, uint32_t flags);

// GAG.EXE: 0x004069D0
void serialize_runtime_tree_sections(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00406BB0
void serialize_runtime_language(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004073D0
void serialize_runtime_fixed_name_nodes(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00404990
ScriptTextBuffer *serialize_current_runtime_state();

// GAG.EXE: 0x00406980
RuntimeTreeNode *get_runtime_tree_root();

// GAG.EXE: 0x004069A0
RuntimeTreeNode *find_runtime_tree_tail();

// GAG.EXE: 0x00406600
RuntimeTreeNode *find_runtime_tree_ancestor_root(void *identity);

// GAG.EXE: 0x00407380
RuntimeFixedNameListNode *find_runtime_fixed_name_list_node(const void *name);

// GAG.EXE: 0x00407440
void destroy_runtime_fixed_name_list_nodes();


struct RuntimeLockRecord
{
    uint32_t state_flags;
    void *identity_context;
    uint32_t type_flags;
    uint32_t flags;
    void *data;
    DWORD owner_thread;
    uint32_t recursion_count;
    intptr_t scene_identifier;
};


struct RuntimeResourceObject
{
    uint32_t state_flags;
    void *backend;
    uint32_t type_flags;
    uint32_t backend_flags;
    void *data;
    DWORD owner_thread;
    uint32_t recursion_count;
    intptr_t scene_identifier;
    DisplaySceneDescriptor scene_descriptor;
    uint32_t presentation_owner;
    uint32_t field_0034;
    int32_t x;
    int32_t y;
    uint32_t output_width;
    uint32_t output_height;
    uint32_t requested_width;
    uint32_t requested_height;
    uint32_t frame_limit;
    uint32_t frames_remaining;
    int32_t previous_x;
    int32_t previous_y;
    uint8_t unknown_0060[4];
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    uint8_t unknown_006c[8];
    union
    {
        RuntimeGenericBackendChild *generic_backend_child;
        uintptr_t field_0074;
    };
    uint8_t unknown_0078[0x11c];
    intptr_t callback_position;
};

struct DisplayTraversalState;


struct RuntimeResourceConstructionPlan
{
    uint32_t flags;
    uint32_t scene_identifier;
    uint32_t scene_flags;
    int32_t x;
    int32_t y;
};

struct RuntimeResourceConstructionPlanApi
{
    uint32_t (*find_available_scene)(uint32_t flags);
};

struct RuntimeMediaBackend;
struct RuntimeAnimationBackend;
struct RuntimeSoundSlot;
struct RuntimeGenericBackend;
struct RuntimeGenericResourceNode;
struct RuntimeTreeNode;
struct DisplaySceneNode;
struct DisplayPixelFormatDescriptor;
struct AsyncFileRecord;
struct RuntimeResourceCacheEntry;
struct DisplayTraversalState;

struct RuntimeResourceConstructionApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    uint32_t (*detect_type)(const char *path);
    void (*update_host)(const char *path, int32_t mode);
    void (*load)(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);
    RuntimeMediaBackend *(*create_bitmap)(uint32_t unused, uint32_t extension_bytes, void *data);
    RuntimeAnimationBackend *(*create_animation)(uint32_t unused, void *data, uint32_t extension_bytes, uint32_t storage);
    uint32_t (*create_sound)(WAVEFORMATEX *format);
    RuntimeSoundSlot *(*get_sound_slot)(uint32_t handle);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*queue_sound)(uint32_t handle, void *data, uint32_t size, int32_t replace);
    void (*set_sound_loop)(uint32_t handle, uint32_t value);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    RuntimeGenericBackend *(*create_generic)(uintptr_t data, uint32_t size);
    RuntimeGenericResourceNode *(*find_generic_resource)(const char *path);
    RuntimeTreeNode *(*activate_tree)(const char *resource_name, const char *tree_name, void *creation_context, void *unused);
    void (*rebuild_tree)(void *identity);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*configure_bitmap)(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, void *callback, uint32_t flags);
    uint32_t (*configure_animation)(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, const void *comparison_palette, uint32_t flags,
        int32_t (*callback)(RuntimeMediaBackend *backend));
    uint32_t (*begin_scene)(intptr_t identifier);
    void (*finalize_media)(void *identity);
    void (*configure_palette)(RuntimeResourceObject *resource);
    uint32_t (*end_scene)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void (*wait_for_count)(uint32_t count);
    uint32_t (*destroy_media)(void *identity);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*destroy_generic)(void *identity);
    BOOL (*release_memory)(const char *path);
    uint32_t (*release_stream)(AsyncFileRecord *record);
    void (*build_path)(char *destination, const char *source);
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    RuntimeResourceCacheEntry *(*register_resource)(void *parent_identity, void *data);
    uint32_t (*add_scene_callback)(intptr_t identifier, int (*callback)(DisplayTraversalState *state), const void *context, uint32_t context_size, uint32_t flags);
};

// Non-original helper: exact pre-dispatch normalization from ConstructRuntimeResourceObject.
RuntimeResourceConstructionPlan prepare_runtime_resource_construction(uint32_t scene_identifier, int32_t x, int32_t y, uint32_t flags);

// GAG.EXE: 0x00424EC0
void *construct_runtime_resource(char *path, uint32_t scene_identifier, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t scale_or_loop, uint32_t flags);

void set_runtime_resource_construction_api_for_testing(const RuntimeResourceConstructionApi &api);

struct RuntimeResourceVisibilityCallbackContext
{
    uint32_t palette_state;
    uint32_t resource_flags;
    char resource_name[260];
};


// GAG.EXE: 0x00428160
uint32_t update_runtime_resource_visibility(DisplayTraversalState *state);

struct RuntimeResourceDestroyApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeGenericResourceNode *(*find_generic)(void *identity);
    void (*remove_generic)(void *identity);
    uint32_t (*destroy_media_backend)(void *backend);
    BOOL (*release_memory_data)(void *data);
    uint32_t (*release_stream)(AsyncFileRecord *record);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*destroy_generic_backend)(void *backend);
    uint32_t (*release_scene)(intptr_t scene_identifier, intptr_t owner);
    uint32_t (*remove_runtime_child)(void *parent_identity, void *child_identity);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeSoundSlot;

struct RuntimeResourceControlApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*release_record)(RuntimeLockRecord *record);
    uint32_t (*destroy_resource)(void *identity);
    RuntimeSoundSlot *(*get_sound_slot)(uint32_t handle);
};

// GAG.EXE: 0x00425BD0
void request_runtime_resource_destruction(void *identity);

// GAG.EXE: 0x00425FB0
uint32_t query_runtime_resource_frame_limit(void *identity);

// GAG.EXE: 0x00425FF0
uint32_t query_runtime_resource_playback_flags(void *identity);

// GAG.EXE: 0x004258C0
void set_runtime_property_value(uint32_t value);

// GAG.EXE: 0x00425F00
uint32_t get_runtime_property_value();

// GAG.EXE: 0x00426080
uint16_t query_runtime_resource_frame_number(void *identity);

using RuntimeResourceConstructor = void *(*)(char *path, uint32_t scene_identifier, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t scale_or_loop, uint32_t flags);

struct RuntimeResourceSelectionApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    uint32_t (*close_archive)(CdfArchive *archive);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    RuntimeResourceConstructor construct_resource;
};

// GAG.EXE: 0x004244E0
void select_runtime_resource(char *path);

void set_runtime_resource_selection_api_for_testing(const RuntimeResourceSelectionApi &api);

// GAG.EXE: 0x004260B0
void unload_runtime_game_dll();

struct RuntimeGameHostContext
{
    HWND window;
    HDC palette_dc;
    uint32_t bits_per_pixel;
    HPALETTE palette;
    uint32_t unknown_0010;
    HDC palette_dib_dc;
    HBITMAP bitmap;
    HBITMAP selected_bitmap;
    uint16_t width;
    uint16_t height;
    void *display_surface;
    intptr_t unknown_0028;
    void *framebuffer;
    intptr_t unknown_0030;
    PALETTEENTRY *palette_entries;
    uint32_t unknown_0038;
    uint32_t unknown_003c;
};


using RuntimeGameDllInitialize = void (*)(RuntimeGameHostContext *context, void **callbacks, const char *sfs_name);
using RuntimeGameDllExecute = void (*)(uint32_t command);
using RuntimeGameDllWindowProcedure = uint32_t (*)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct RuntimeGameResultDescriptor
{
    uint32_t type;
    uint32_t reserved;
    uint32_t size;
    const void *data;
};


struct RuntimeGameLifecycleApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void (*update_resource_host)(const char *path, int32_t reset);
    int32_t (*activate_comment_scene)(const char *name);
    void (*deactivate_comment_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void (*leave_runtime_state)();
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
};

struct RuntimeGameIntegrationApi
{
    RuntimeGameDllInitialize initialize;
    RuntimeGameDllWindowProcedure window_procedure;
    RuntimeGameDllExecute execute;
    void (*shutdown)();
};

// GAG.EXE: 0x00426110
bool load_and_initialize_runtime_game_dll(const char *path);

struct RuntimeGameDllDispatchApi
{
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x00426210
uint32_t stop_runtime_game_dll();

// GAG.EXE: 0x00426270
uint32_t pause_runtime_game_dll();

// GAG.EXE: 0x00426290
uint32_t resume_runtime_game_dll();

struct DisplayRectangle;

struct RuntimeGameWindowApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *get_update_rect)(HWND window, LPRECT rectangle, BOOL erase);
    HDC(WINAPI *begin_paint)(HWND window, LPPAINTSTRUCT paint);
    uint32_t (*queue_display_rectangle)(DisplayRectangle *rectangle);
    BOOL(WINAPI *end_paint)(HWND window, const PAINTSTRUCT *paint);
    void (*update_pointer_position)(int32_t x, int32_t y);
    void (*enqueue_byte)(uint8_t value);
    void (*enqueue_pair)(uint32_t first, uint32_t second);
    void (*enqueue_message)(uint32_t message);
    void (*clear_runtime_flag)();
    void (*unload_game_dll)();
    void (*enter_runtime_state)();
    void (*leave_runtime_state)();
    void (*set_runtime_flag)();
    BOOL(WINAPI *track_mouse_event)(LPTRACKMOUSEEVENT event);
    HCURSOR(WINAPI *set_cursor)(HCURSOR cursor);
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x004231E0
LRESULT CALLBACK runtime_game_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct RuntimePointerPositionApi
{
    DWORD(WINAPI *get_current_thread_id)();
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    RuntimeNamedNode *(*find_child)(void *parent_identity, void *child_identity);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*offset_scene)(intptr_t identifier, int32_t x, int32_t y);
};

// GAG.EXE: 0x004243F0
void update_runtime_pointer_position(int32_t x, int32_t y);

// GAG.EXE: 0x00424D80
uint32_t destroy_runtime_resource(void *identity);

struct RuntimeSoundSlot;
struct RuntimeMediaBackend;
struct DisplaySceneDescriptor;
using RuntimeAnimationCallback = int32_t (*)(RuntimeMediaBackend *backend);

struct RuntimeMediaBackend
{
    uint32_t type;
    void *identity;
    DWORD owner_thread;
    uint32_t recursion_count;
    RuntimeMediaBackend *previous;
    RuntimeMediaBackend *next;
    const void *comparison_palette;
    uint16_t field_001c;
    uint16_t field_001e;
    PALETTEENTRY palette_entries[0x100];
    RGBQUAD dib_colors[0x100];
    uint32_t palette_padding;
    uint8_t palette_remap[0x100];
    uint16_t destination_x;
    uint16_t destination_y;
    uint16_t destination_stride;
    uint16_t destination_reserved;
    uint32_t descriptor_2;
    uint8_t *destination_pixels;
    HWND window;
    HDC destination_context;
    uint32_t destination_bits_per_pixel;
    HPALETTE destination_palette;
    uint32_t presentation_field_0944;
    HDC source_context;
    uint8_t presentation_tail[0x10];
    uint32_t media_flags;
    uint32_t error_state;
    uint32_t scale_x;
    uint32_t scale_y;
    void *extension_data;
    void *source_data;
    void *format_data;
    void *frame_header;
    void *chunk_header;
    RuntimeSoundSlot *sound_slot;
    void *audio_buffer;
    void *frame_buffer;
    int32_t dirty_left;
    int32_t dirty_top;
    int32_t dirty_right;
    int32_t dirty_bottom;
    uint16_t frame_number;
    uint16_t frame_reserved;
    uint32_t previous_frame_time;
    uint32_t next_frame_time;
    int32_t timing_correction;
    uint32_t synchronized_sound_frame;
    uint32_t timing_adjustment;
    uint32_t frame_duration;
    int32_t (*animation_callback)(RuntimeMediaBackend *backend);
    uint32_t sound_handle;
    uint32_t allocation_1_active;
    AsyncFileRecord *stream_record;
    uint32_t allocation_2_active;
};


struct RuntimeMediaBackendApi
{
    DWORD(WINAPI *get_current_thread_id)();
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(WINAPI *sleep)(DWORD milliseconds);
};

#pragma pack(push, 1)
struct RuntimeAnimationFileHeader
{
    uint32_t file_size;
    uint16_t signature;
    uint16_t frame_count;
    uint16_t width;
    uint16_t height;
    uint8_t unknown_000c[4];
    uint32_t frame_duration;
    uint8_t unknown_0014[0x3c];
    uint32_t data_start_offset;
    uint32_t data_end_offset;
    uint8_t unknown_0058[0x28];
};

struct RuntimeAnimationFrameHeader
{
    uint32_t size;
    uint16_t signature;
    uint16_t chunk_count;
    uint8_t unknown_0008[8];
};

struct RuntimeAnimationChunkHeader
{
    uint32_t size;
    uint16_t type;
};

struct RuntimeAnimationStreamHeaders
{
    RuntimeAnimationFrameHeader frame;
    RuntimeAnimationChunkHeader chunk;
    uint8_t unknown_0016[2];
};

struct RuntimeAnimationSoundFormatChunk
{
    RuntimeAnimationChunkHeader chunk;
    uint8_t unknown_0006[0x0c];
    WAVEFORMATEX format;
};

struct RuntimeFontFormat
{
    uint32_t unknown_0000;
    int32_t fixed_cell_width;
    int32_t fixed_cell_height;
};

struct RuntimePaletteData
{
    uint32_t unknown_0000;
    PALETTEENTRY entries[0x100];
};

struct RuntimePcmWaveFile
{
    uint8_t riff_and_format_headers[0x14];
    PCMWAVEFORMAT format;
};

struct RuntimeRiffChunk
{
    char identifier[4];
    uint32_t size;
    uint8_t data[1];
};
#pragma pack(pop)

struct RuntimeBitmapBackendCreateApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

// GAG.EXE: 0x0042A1B0
RuntimeMediaBackend *create_runtime_bitmap_backend(uint32_t unused, uint32_t extension_bytes, void *bitmap_data);

struct RuntimeAnimationBackend
{
    RuntimeMediaBackend base;
    void *source_cursor;
    void *data_start;
    void *data_end;
    RuntimeAnimationFileHeader header;
    RuntimeAnimationStreamHeaders streamed_headers;
};

struct DisplaySceneNode;


struct RuntimeAnimationBackendCreateApi
{
    uint32_t (*get_position)(AsyncFileRecord *record);
    uint32_t (*read_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    uint32_t (*set_position)(AsyncFileRecord *record, uint32_t position);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

struct RuntimeMediaBackendConfigureApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

struct RuntimeAnimationBackendConfigureApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};

// GAG.EXE: 0x00427A30
void render_runtime_generic_backend_child(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00427AB0
void update_runtime_generic_backend_child(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00427EF0
int32_t update_runtime_resource_animation_backend(RuntimeMediaBackend *backend);

// GAG.EXE: 0x0042A290
uint32_t configure_runtime_bitmap_backend(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, void *callback, uint32_t flags);

// GAG.EXE: 0x0042A340
uint32_t configure_runtime_animation_backend(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, const void *comparison_palette, uint32_t flags,
    RuntimeAnimationCallback callback);

struct RuntimeResourcePaletteConfigureApi
{
    bool (*set_primary_owner)(intptr_t identifier, intptr_t owner, bool replace_existing);
    bool (*configure_palette)(DisplaySceneNode *node, const uint32_t *palette, uint32_t count);
};

// GAG.EXE: 0x00427E60
void configure_runtime_resource_palette(RuntimeResourceObject *resource);

// GAG.EXE: 0x00415D90
void build_runtime_palette_index_remap(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00417260
uint8_t convert_runtime_bitmap_to_surface(RuntimeMediaBackend *backend);

struct RuntimeMediaBackendFinalizeApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint8_t (*convert_bitmap)(RuntimeMediaBackend *backend);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *realize_palette)(HDC context);
    UINT(WINAPI *set_dib_color_table)(HDC context, UINT start, UINT count, const RGBQUAD *colors);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
};

// GAG.EXE: 0x0042B300
void finalize_runtime_media_backend(void *identity);

struct RuntimeAnimationFailureApi
{
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x0042A4C0
void fail_runtime_animation(RuntimeMediaBackend *backend, uint32_t error);

// GAG.EXE: 0x0042A4A0
void pause_all_runtime_animations();

// GAG.EXE: 0x0042A4B0
void resume_all_runtime_animations();

enum class RuntimeAnimationControlResult
{
    DecodeFrame,
    Wait,
    Exit
};

struct RuntimeAnimationControlApi
{
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*set_stream_position)(AsyncFileRecord *record, uint32_t position);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// Non-original helper: exact control phase of RunRuntimeAnimationThread.
RuntimeAnimationControlResult process_runtime_animation_control(RuntimeAnimationBackend *backend, uint32_t current_time, uint32_t *wait_milliseconds);

// Non-original helper: exact frame scheduling phase of RunRuntimeAnimationThread.
void schedule_runtime_animation_frame(RuntimeMediaBackend *backend, uint32_t current_time);

struct RuntimeAnimationFrameAcquireApi
{
    uint32_t (*read_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void (*fail_animation)(RuntimeMediaBackend *backend, uint32_t error);
};

// Non-original helper: exact frame acquisition phase of RunRuntimeAnimationThread.
bool acquire_runtime_animation_frame(RuntimeAnimationBackend *backend);

struct RuntimeAnimationDecodeApi
{
    void (*decode_palette)(RuntimeMediaBackend *backend);
    void (*decode_mvz5)(RuntimeMediaBackend *backend);
    void (*decode_delta_flc)(RuntimeMediaBackend *backend);
    void (*decode_mvz8)(RuntimeMediaBackend *backend);
    void (*ignore_chunk_11)();
    void (*ignore_chunk_12)();
    void (*ignore_chunk_13)();
    void (*decode_byte_run)(RuntimeMediaBackend *backend);
    void (*decode_literal)(RuntimeMediaBackend *backend);
};

// Non-original helper: exact visual chunk dispatch phase of RunRuntimeAnimationThread.
void decode_runtime_animation_frame_chunks(RuntimeAnimationBackend *backend);

struct RuntimeAnimationCompletionApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    uint32_t (*set_stream_position)(AsyncFileRecord *record, uint32_t position);
};

// Non-original helper: exact presentation/completion phase of RunRuntimeAnimationThread.
void complete_runtime_animation_frame(RuntimeAnimationBackend *backend);

struct RuntimeAnimationAudioApi
{
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*queue_sound_data)(uint32_t handle, void *data, uint32_t size, int32_t replace);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*create_sound)(WAVEFORMATEX *format);
    RuntimeSoundSlot *(*get_sound_slot)(uint32_t handle);
};

// Non-original helper: exact audio chunk prepass of RunRuntimeAnimationThread.
void process_runtime_animation_audio_chunks(RuntimeAnimationBackend *backend);

struct RuntimeAnimationWorkerApi
{
    DWORD(WINAPI *gdi_set_batch_limit)(DWORD limit);
    void(WINAPI *sleep)(DWORD milliseconds);
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *exit_thread)(DWORD exit_code);
};

// GAG.EXE: 0x0042A520
DWORD WINAPI run_runtime_animation_thread(void *backend);

struct RuntimeAnimationPresentApi
{
    BOOL(WINAPI *animate_palette)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *realize_palette)(HDC context);
    UINT(WINAPI *set_dib_color_table)(HDC context, UINT start, UINT count, const RGBQUAD *colors);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
    BOOL(WINAPI *stretch_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD operation);
};

// GAG.EXE: 0x0042B850
int32_t present_runtime_animation_frame(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00415E60
void decode_runtime_animation_palette(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00415EE0
void decode_runtime_animation_mvz8(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416420
void decode_runtime_animation_mvz5(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416900
void decode_runtime_animation_literal(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416AD0
void decode_runtime_animation_byte_run(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416DA0
void decode_runtime_animation_delta_flc(RuntimeMediaBackend *backend);


// GAG.EXE: 0x0042B820
void ignore_runtime_animation_chunk_11();

// GAG.EXE: 0x0042B830
void ignore_runtime_animation_chunk_12();

// GAG.EXE: 0x0042B840
void ignore_runtime_animation_chunk_13();

// GAG.EXE: 0x00429EB0
RuntimeAnimationBackend *create_runtime_animation_backend(uint32_t unused, void *data, uint32_t extension_bytes, uint32_t storage);

// GAG.EXE: 0x0042B620
RuntimeMediaBackend *acquire_runtime_media_backend(void *identity);

// GAG.EXE: 0x0042B5B0
uint32_t get_runtime_media_backend_type(void *identity);

// GAG.EXE: 0x004299B0
uint8_t classify_runtime_media_data(const void *data);

// GAG.EXE: 0x00404920
uint32_t read_compressor_input(void *destination, uint32_t requested_size);

// Non-original test state accessors.
void set_compressor_input_state_for_testing(const void *input, uint32_t input_size, uint32_t input_position);
uint32_t get_compressor_input_position_for_testing();

// GAG.EXE: 0x0042B2A0
void set_runtime_media_backend_scale(void *identity, uint32_t scale_x, uint32_t scale_y);

// GAG.EXE: 0x0042A440
uint32_t stop_runtime_animation_backend(void *identity);

// GAG.EXE: 0x0042B600
void *get_locked_runtime_media_extension(void *identity);

struct RuntimePaletteTarget
{
    uint32_t unknown_0000;
    HDC device_context;
    uint32_t unknown_0008;
    HPALETTE palette;
};

struct RuntimePaletteUpdateApi
{
    HPALETTE(WINAPI *select_palette)(HDC, HPALETTE, BOOL);
    BOOL(WINAPI *animate_palette)(HPALETTE, UINT, UINT, const PALETTEENTRY *);
    BOOL(WINAPI *unrealize_object)(HGDIOBJ);
    UINT(WINAPI *set_palette_entries)(HPALETTE, UINT, UINT, const PALETTEENTRY *);
    UINT(WINAPI *realize_palette)(HDC);
};

// GAG.EXE: 0x0042B720
UINT apply_runtime_palette_entries(RuntimePaletteTarget *target, void *palette_data, uint32_t *flags, uint32_t force);

// GAG.EXE: 0x0042B4E0
uint32_t destroy_runtime_media_backend(void *identity);

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
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void(WINAPI *sleep)(DWORD milliseconds);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeGenericBackendCreateApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

// GAG.EXE: 0x00410C40
RuntimeGenericBackend *create_runtime_generic_backend(uintptr_t value_0010, uint32_t value_000c);

// GAG.EXE: 0x00410CC0
RuntimeGenericBackend *acquire_runtime_generic_backend(void *identity);

// GAG.EXE: 0x00410D40
void clear_runtime_generic_backend_ready(RuntimeGenericBackend *backend);

// GAG.EXE: 0x00410DE0
void *find_available_runtime_generic_child(uint32_t maximum_end_position);

// GAG.EXE: 0x00410E50
int32_t find_runtime_generic_text_entry(RuntimeGenericBackend *backend, int32_t category, const char *name);

struct RuntimeGenericChildCreateApi
{
    RuntimeGenericBackend *(*acquire_backend)(void *identity);
    int32_t (*find_text_entry)(RuntimeGenericBackend *backend, int32_t category, const char *name);
    int32_t (*parse_integer)(const char *text, uint32_t *position, uint32_t end, uint32_t flags);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint32_t (*build_child_state)(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);
    void (*clear_backend_ready)(RuntimeGenericBackend *backend);
};

// GAG.EXE: 0x004110B0
RuntimeGenericBackendChild *create_runtime_generic_backend_child(void *backend_identity, void *font_identity, const uintptr_t *context, uintptr_t selection, uint32_t flags);

void set_runtime_generic_child_create_api_for_testing(const RuntimeGenericChildCreateApi &api);

// GAG.EXE: 0x00411220
RuntimeGenericBackendChild *acquire_runtime_generic_backend_child(void *identity);

// GAG.EXE: 0x004118F0
int32_t parse_runtime_generic_integer(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

// GAG.EXE: 0x004119A0
int32_t skip_runtime_generic_statement(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

// GAG.EXE: 0x00411A20
int32_t parse_runtime_generic_directive(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

// GAG.EXE: 0x00411560
uint32_t build_runtime_generic_backend_child_state(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);

// GAG.EXE: 0x00411420
void publish_runtime_generic_backend_child_state(void *identity, const uint32_t *state, const DisplaySceneDescriptor *descriptor, int32_t end_offset);

// GAG.EXE: 0x004122C0
uint32_t measure_runtime_font_glyph(uint8_t character, const RuntimeMediaBackend *backend);

// GAG.EXE: 0x00412370
uint32_t draw_runtime_font_glyph(DisplaySceneDescriptor *destination, uint8_t character, int32_t x, int32_t y, const RuntimeMediaBackend *font, uint32_t low_color, uint32_t high_color);

// GAG.EXE: 0x00411FF0
void draw_runtime_generic_text(const char *text, uint32_t end, const uint32_t *state, void *font_identity, DisplaySceneDescriptor *destination, uint32_t flags);

struct RuntimeStandaloneTextState
{
    uint32_t unknown_0000[3];
    const char *text;
    void *font_identity;
    uint32_t value_0014;
    uint32_t value_0018;
    uint32_t low_color;
    uint32_t high_color;
    uint32_t unknown_0024[2];
    union
    {
        uint32_t bounds[4];
        DisplayRectangle bounds_rectangle;
    };
};

// GAG.EXE: 0x00411800
uint32_t initialize_runtime_standalone_text(const char *text, uint32_t value_0014, uint32_t value_0018, void *font_identity, uint32_t low_color, uint32_t high_color,
    RuntimeStandaloneTextState *state);

// GAG.EXE: 0x004118C0
void draw_runtime_standalone_text(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);

// GAG.EXE: 0x00411CF0
void measure_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, void *font_identity, uint32_t flags);

// GAG.EXE: 0x00411BC0
uint32_t select_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, uint32_t search_position, void *font_identity, uint32_t flags);

// GAG.EXE: 0x004112B0
void release_runtime_generic_backend_child_lock(RuntimeGenericBackendChild *child);

// GAG.EXE: 0x0042B6A0
void release_runtime_media_backend_lock(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00411340
uint32_t get_runtime_generic_backend_child_flags(void *identity);

// GAG.EXE: 0x00411360
void clear_runtime_generic_backend_child_ready(void *identity);

// GAG.EXE: 0x00411380
void enable_runtime_generic_backend_child_mode_200(void *identity);

// GAG.EXE: 0x004113A0
void disable_runtime_generic_backend_child_mode_200(void *identity);

// GAG.EXE: 0x004113C0
bool get_runtime_generic_backend_child_context(void *identity, uintptr_t *context);

// GAG.EXE: 0x004113F0
bool set_runtime_generic_backend_child_context(void *identity, const uintptr_t *context);

// GAG.EXE: 0x004114D0
uint32_t query_runtime_generic_backend_child_state(void *identity, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);

// GAG.EXE: 0x004112C0
void *destroy_runtime_generic_backend_child(void *identity);

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

// GAG.EXE: 0x004212E0
void process_available_runtime_generic_children(uint32_t maximum_end_position);

void set_runtime_generic_child_scene_api_for_testing(const RuntimeGenericChildSceneApi &api);
void set_runtime_pointer_event_flags_for_testing(uint32_t flags);

// GAG.EXE: 0x00410D50
uint32_t destroy_runtime_generic_backend(void *identity);

struct RuntimeSoundBufferNode
{
    void *data;
    RuntimeSoundBufferNode *next;
    uint32_t offset;
    uint32_t unknown_000c;
    uint32_t size;
};


struct RuntimeSoundSlot
{
    uint32_t active;
    uint32_t playing;
    uint32_t base_state;
    uint32_t playback_state;
    uint32_t schedule_state;
    uint32_t fade_block_index;
    uint32_t fade_current;
    uint8_t fade_step;
    uint8_t unknown_001d[3];
    uint32_t loop_value_1;
    uint32_t loop_value_2;
    uint8_t volume;
    uint8_t unknown_0029;
    uint16_t conversion_flags;
    uint32_t transition_flags;
    RuntimeSoundBufferNode *buffers;
};


struct RuntimeSoundDestroyApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeSoundCreateApi
{
    uint32_t (*ensure_ready)(WAVEFORMATEX *format, uint32_t mixer_argument);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint32_t (*formats_equal)(const WAVEFORMATEX *left, const WAVEFORMATEX *right);
    MMRESULT(WINAPI *wave_out_reset)(HWAVEOUT wave_out);
    MMRESULT(WINAPI *wave_out_unprepare_header)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    MMRESULT(WINAPI *wave_out_close)(HWAVEOUT wave_out);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    void (*destroy_sound)(uint32_t handle);
    void (*cleanup_format_buffer)();
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    uint32_t (*initialize_mixer)(WAVEFORMATEX *format, uint32_t mixer_argument);
    uint32_t (*calculate_conversion)(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, uint16_t *conversion_flags);
};

// GAG.EXE: 0x00401820
uint32_t create_runtime_sound_handle(WAVEFORMATEX *source_format);

// GAG.EXE: 0x00402040
void destroy_runtime_sound_handle(uint32_t handle);

// GAG.EXE: 0x00401BB0
uint32_t queue_runtime_sound_data(uint32_t handle, void *data, uint32_t size, int32_t replace);

// GAG.EXE: 0x00401CD0
uint32_t start_runtime_sound(uint32_t handle, int32_t reset_timing);

// GAG.EXE: 0x00401D50
uint32_t stop_runtime_sound(uint32_t handle, int32_t reset_timing);

// GAG.EXE: 0x00403380
void set_runtime_sound_loop_value(uint32_t handle, uint32_t value);

// GAG.EXE: 0x004033E0
RuntimeSoundSlot *get_runtime_sound_slot(uint32_t handle);

// GAG.EXE: 0x00401DE0
uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

// GAG.EXE: 0x00401F10
uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

// GAG.EXE: 0x00403310
uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume);

struct RuntimeWaveOutCallbackApi
{
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    DWORD(WINAPI *time_get_time)();
};

// GAG.EXE: 0x004035C0
void CALLBACK runtime_wave_out_callback(HWAVEOUT wave_out, UINT message, DWORD_PTR instance, DWORD_PTR parameter_1, DWORD_PTR parameter_2);

struct RuntimeSoundShutdownApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    MMRESULT(WINAPI *wave_out_reset)(HWAVEOUT wave_out);
    MMRESULT(WINAPI *wave_out_unprepare_header)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    MMRESULT(WINAPI *wave_out_close)(HWAVEOUT wave_out);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    void (*destroy_sound)(uint32_t handle);
    void (*cleanup_format_buffer)();
};

// GAG.EXE: 0x00401190
uint32_t shutdown_runtime_sound();

struct RuntimeSoundReadinessApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*initialize_mixer)(WAVEFORMATEX *format, uint32_t mixer_argument);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

struct RuntimeSoundPauseResumeApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    MMRESULT(WINAPI *wave_out_reset)(HWAVEOUT wave_out);
    MMRESULT(WINAPI *wave_out_unprepare_header)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    MMRESULT(WINAPI *wave_out_close)(HWAVEOUT wave_out);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    void(WINAPI *sleep)(DWORD milliseconds);
    MMRESULT(WINAPI *wave_out_open)(LPHWAVEOUT wave_out, UINT device_id, LPCWAVEFORMATEX format, DWORD_PTR callback, DWORD_PTR instance, DWORD flags);
};

// GAG.EXE: 0x004010A0
void toggle_runtime_sound_state();

// GAG.EXE: 0x004015D0
uint32_t pause_runtime_sound_output(int32_t close_output);

// GAG.EXE: 0x004016D0
uint32_t resume_runtime_sound_output();

void set_runtime_sound_pause_resume_api_for_testing(const RuntimeSoundPauseResumeApi &api);
void set_runtime_sound_pause_resume_state_for_testing(uint32_t toggle_state, uint32_t mixing_suppressed, uint32_t output_initialized, uint32_t output_ready, HANDLE thread, DWORD thread_id,
    HWND window, uint32_t fault);
void get_runtime_sound_pause_resume_state_for_testing(uint32_t *toggle_state, uint32_t *mixing_suppressed, uint32_t *output_initialized, HANDLE *thread, DWORD *thread_id);

// GAG.EXE: 0x004010B0
uint32_t ensure_runtime_sound_ready(WAVEFORMATEX *format, uint32_t mixer_argument);

struct RuntimeSoundThreadApi
{
    HWND(WINAPI *create_window_ex)
    (DWORD extended_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter);
    BOOL(WINAPI *show_window)(HWND window, int command);
    HANDLE(WINAPI *get_current_thread)();
    BOOL(WINAPI *set_thread_priority)(HANDLE thread, int priority);
    BOOL(WINAPI *get_message)(LPMSG message, HWND window, UINT minimum, UINT maximum);
    LRESULT(WINAPI *dispatch_message)(const MSG *message);
    void(WINAPI *exit_thread)(DWORD exit_code);
};

// GAG.EXE: 0x00402100
DWORD WINAPI run_runtime_sound_thread(LPVOID parameter);

struct RuntimeSoundOutputBlock
{
    WAVEFORMATEX *format;
    WAVEHDR *header;
    uint8_t *data;
};


struct RuntimeSoundWindowApi
{
    MMRESULT(WINAPI *wave_out_prepare_header)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    MMRESULT(WINAPI *wave_out_write)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    void(WINAPI *post_quit_message)(int exit_code);
    BOOL(WINAPI *destroy_window)(HWND window);
    LRESULT(WINAPI *def_window_proc)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x004021E0
LRESULT CALLBACK runtime_sound_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct RuntimeSoundClassApi
{
    ATOM(WINAPI *register_class)(const WNDCLASSA *window_class);
    HANDLE(WINAPI *create_mutex)(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name);
};

// GAG.EXE: 0x00401000
void initialize_runtime_sound_class(HINSTANCE instance);

// GAG.EXE: 0x004012C0
uint32_t runtime_wave_formats_equal(const WAVEFORMATEX *left, const WAVEFORMATEX *right);

// GAG.EXE: 0x00403410
uint32_t calculate_runtime_wave_conversion(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, uint16_t *conversion_flags);

struct RuntimeSoundFormatCleanupApi
{
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

// GAG.EXE: 0x00401300
void cleanup_runtime_sound_format_buffer();


struct RuntimeSceneSwitchApi
{
    RuntimeLockRecord *(*acquire)(void *identity);
    void (*release)(RuntimeLockRecord *record);
    uint32_t (*offset_scene)(intptr_t identifier, int32_t x_delta, int32_t y_delta);
};

struct RuntimeDisplayResetApi
{
    void (*switch_scene)(void *identity);
    void (*set_script_flags)(uint32_t mask, int enabled);
    void (*reset_transient_indices)();
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
};

struct RuntimeDisplayShutdownApi
{
    RuntimeNamedNode *(*get_named_node)(const char *name);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
    uint32_t (*shutdown_host)();
    void (*teardown_surface)();
};

struct RuntimeResourceWaitApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct RuntimeResourceFileOpenApi
{
    BOOL(WINAPI *get_version)(LPOSVERSIONINFOA version);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file);
};

struct RuntimeResourceHostApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    uint32_t (*destroy_host)(AsyncFileHost *host);
    AsyncFileHost *(*create_host)(const char *root, uint32_t requested_bytes, int32_t mode);
    void (*set_host_mode)(AsyncFileHost *host, int32_t mode);
    uint32_t (*close_archive)(CdfArchive *archive);
};

struct ArchiveReadSpeedApi
{
    uint32_t (*initialize_async)();
    uint32_t (*extract_drive_prefix)(char *destination, const char *source);
    AsyncFileHost *(*create_host)(const char *root, uint32_t requested_bytes, int32_t mode);
    AsyncFileRecord *(*open_record)(AsyncFileHost *host, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags);
    uint32_t (*get_size)(AsyncFileRecord *record);
    uint32_t (*read_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    DWORD(WINAPI *get_time)();
    uint32_t (*close_record)(AsyncFileRecord *record);
    uint32_t (*destroy_host)(AsyncFileHost *host);
};

struct RuntimeResourceTypeApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    void (*update_host)(const char *path, int32_t reset);
    HANDLE (*open_file)(const char *path);
    BOOL(WINAPI *read_file)(HANDLE file, LPVOID buffer, DWORD bytes, LPDWORD bytes_read, LPOVERLAPPED overlapped);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    uint8_t (*get_archive_flags)(CdfArchive *archive, const char *name);
};

struct RuntimeCdfStreamApi
{
    int(WINAPI *compare_names)(LPCSTR left, LPCSTR right);
    AsyncFileRecord *(*duplicate_record)(AsyncFileRecord *identity, uint32_t start_offset, uint32_t end_offset, uint32_t flags);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG high_distance, DWORD method);
};

struct ArchiveCommentDialogState
{
    void *value_0000;
    const char *directory;
    char file_name[0x20];
    char extension[0x58];
    char *output_1;
    char *output_2;
    uint32_t maximum_identifier;
    uint32_t comment_count;
    uint32_t comment_capacity;
    char *archive_paths;
    CustomControlState custom_control;
};

struct ArchiveCommentEnumerationApi
{
    HANDLE(WINAPI *find_first)(LPCSTR pattern, LPWIN32_FIND_DATAA data);
    BOOL(WINAPI *find_next)(HANDLE find, LPWIN32_FIND_DATAA data);
    BOOL(WINAPI *find_close)(HANDLE find);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    uint32_t (*get_error)(CdfArchive *archive);
    uint32_t (*get_entry_size)(CdfArchive *archive, uint8_t selector, const char *name);
    int (*read_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    uint32_t (*close_archive)(CdfArchive *archive);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *delete_file)(LPCSTR path);
};

struct ArchiveCommentDialogApi
{
    HWND(WINAPI *get_dialog_item)(HWND dialog, int identifier);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    HWND(WINAPI *set_focus)(HWND window);
    BOOL(WINAPI *show_window)(HWND window, int command);
    BOOL(WINAPI *end_dialog)(HWND dialog, INT_PTR result);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    uint32_t (*enumerate_comments)(ArchiveCommentDialogState *state, HWND listbox);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct ArchiveCommentDialogLaunchApi
{
    void(__cdecl *split_path)(const char *path, char *drive, char *directory, char *file_name, char *extension);
    INT_PTR(WINAPI *dialog_box)(HINSTANCE instance, LPCSTR template_name, HWND parent, DLGPROC procedure, LPARAM parameter);
};

// GAG.EXE: 0x004182A0
uint32_t enumerate_archive_comments(ArchiveCommentDialogState *state, HWND listbox);
void set_archive_comment_enumeration_api_for_testing(const ArchiveCommentEnumerationApi &api);
const ArchiveCommentEnumerationApi &get_archive_comment_enumeration_api();

// GAG.EXE: 0x00418560
INT_PTR CALLBACK archive_comment_dialog_procedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

// GAG.EXE: 0x00417550
INT_PTR run_archive_comment_dialog(HWND parent, const char *directory, const char *path, char *output);

// GAG.EXE: 0x004188A0
INT_PTR CALLBACK archive_selection_dialog_procedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

// GAG.EXE: 0x004175F0
INT_PTR run_archive_selection_dialog(HWND parent, const char *directory, const char *path, void *initial_value, char *output_path, char *output_name);

void set_archive_comment_dialog_api_for_testing(const ArchiveCommentDialogApi &api);
void set_archive_comment_dialog_launch_api_for_testing(const ArchiveCommentDialogLaunchApi &api);

struct RuntimeResourceLoadApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    AsyncFileRecord *(*open_async_record)(AsyncFileHost *host, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags);
    uint32_t (*get_async_size)(AsyncFileRecord *record);
    int32_t (*activate_loading_scene)(const char *name);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    uint32_t (*read_async_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    void (*deactivate_loading_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    RuntimeResourceCacheEntry *(*get_or_create_cache_entry)(void *parent_identity, const char *name);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    uint8_t (*get_archive_flags)(CdfArchive *archive, const char *name);
    uint32_t (*get_archive_size)(CdfArchive *archive, uint8_t selector, const char *name);
    void *(*open_archive_stream)(CdfArchive *archive, const char *name);
    int (*read_archive_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct AsyncFileRecord;

struct AsyncFileHost
{
    AsyncFileHost *self;
    uint32_t flags;
    AsyncFileHost *next;
    int32_t mode;
    CRITICAL_SECTION primary_lock;
    CRITICAL_SECTION secondary_lock;
    HANDLE thread;
    DWORD bytes_per_sector;
    DWORD sectors_per_cluster;
    uint32_t file_offset;
    HANDLE file;
    uint32_t file_size;
    uint32_t remaining_size;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t current_offset;
    void *buffer_start_cursor;
    uint32_t buffered_bytes;
    uint32_t available_bytes;
    uint32_t buffer_size;
    void *read_cursor;
    void *write_cursor;
    void *secondary_cursor;
    void *buffer;
    AsyncFileRecord *active_file;
    AsyncFileRecord *files;
};

struct AsyncFileRecord
{
    AsyncFileRecord *self;
    uint32_t flags;
    AsyncFileRecord *next;
    HANDLE file;
    uint32_t file_size;
    uint32_t remaining_size;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t current_offset;
    DWORD timestamp;
    void *buffer;
    void *buffer_cursor;
    uint32_t buffered_bytes;
    uint32_t previous_offset;
    uint32_t next_offset;
    AsyncFileHost *host;
};


struct AsyncFileLockApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct AsyncFileOpenApi
{
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    LPVOID(WINAPI *virtual_alloc)(LPVOID address, SIZE_T bytes, DWORD allocation_type, DWORD protect);
    BOOL(WINAPI *virtual_free)(LPVOID address, SIZE_T bytes, DWORD free_type);
    DWORD(WINAPI *get_file_size)(HANDLE file, LPDWORD high_size);
};

struct AsyncFileHostApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL(WINAPI *get_disk_free_space)(LPCSTR root, LPDWORD sectors_per_cluster, LPDWORD bytes_per_sector, LPDWORD free_clusters, LPDWORD total_clusters);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    LPVOID(WINAPI *virtual_alloc)(LPVOID address, SIZE_T bytes, DWORD allocation_type, DWORD protect);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start, LPVOID parameter, DWORD flags, LPDWORD thread_id);
    DWORD(WINAPI *wait_for_single_object)(HANDLE object, DWORD milliseconds);
    BOOL(WINAPI *read_file)(HANDLE file, LPVOID buffer, DWORD bytes, LPDWORD bytes_read, LPOVERLAPPED overlapped);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG high_distance, DWORD method);
    DWORD(WINAPI *time_get_time)();
};

struct RuntimeNamedLockApi
{
    DWORD(WINAPI *get_current_thread_id)();
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
};


using RuntimeCommandDefinition = RuntimeSceneSlot;


struct ScriptRuntimeRoot
{
    ScriptRuntimeRoot *self;
    uint32_t flags;
    uint32_t palette_flags;
    uintptr_t event_records[32][16];
    uint32_t transient_index_1;
    uint32_t transient_index_2;
    void (*set_property)(uint32_t operation, int32_t argument, RuntimeGenericResourceNode *node);
    void (*get_property)(uint32_t operation, void **resource_data, void *result);
    HANDLE heap;
    uint32_t parser_integer_0820;
    uint32_t state_value_0824;
    char language[0x20];
    char parser_value_0848[0x20];
    char parser_text_0868[0x104];
    char default_auxiliary_names[0x104];
    uint32_t command_definition_count;
    RuntimeCommandDefinition command_definitions[32];
    RuntimeGenericResourceNode *generic_resources;
    RuntimeTreeNode *runtime_tree;
    ScriptObjectState *objects;
    RuntimeVisualObject *visual_objects;
    RuntimeNamedNode *runtime_nodes;
    RuntimeFixedNameListNode *fixed_name_nodes;
    union
    {
        RuntimePlanNode *plan_nodes;
        RuntimeTreePrimaryResourceLink *global_primary_resource_links;
    };
    RuntimeTreeLink7C *global_link_007c_head;
    union
    {
        RuntimeTreeLink84 *global_link_0084_head;
        RuntimePointerRegion *global_pointer_regions;
    };
    ScriptObjectContainer *containers;
    RuntimeTreeLink8C *global_link_008c_head;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_links;
    RuntimeTreeSceneLink *global_scene_links;
    union
    {
        RuntimePlanNode *plan_terminal;
        RuntimeTreePrimaryResourceLink *global_primary_resource_link_tail;
    };
    RuntimeTreeLink7C *global_link_007c_tail;
    union
    {
        RuntimeTreeLink84 *global_link_0084_tail;
        RuntimePointerRegion *global_pointer_region_tail;
    };
    ScriptObjectContainer *container_tail;
    RuntimeTreeLink8C *global_link_008c_tail;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_link_tail;
    RuntimeTreeSceneLink *global_scene_link_tail;
    ScriptTextBuffer *serialized_script;
};

// GAG.EXE: 0x004050B0
RuntimeGenericResourceNode *find_runtime_generic_resource(void *identity);

// GAG.EXE: 0x00405080
void remove_all_runtime_generic_resources();

// GAG.EXE: 0x004050E0
void set_runtime_generic_resource_position(void *identity, uint32_t position);

// GAG.EXE: 0x00405110
uint32_t read_runtime_generic_resource_token(void *identity, char *output, uint32_t capacity, uint8_t delimiter);

struct RuntimeGenericResourceLoadApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

// GAG.EXE: 0x00404EE0
RuntimeGenericResourceNode *find_or_load_runtime_generic_resource(const char *resource_name);

void set_runtime_generic_resource_load_api_for_testing(const RuntimeGenericResourceLoadApi &api);

struct RuntimeTreeParserContext
{
    RuntimeTreeNode *owner;
    char *name_pointer;
    char *creation_text_pointer;
    char *scratch_text_pointer;
    uint8_t unknown_0010[4];
    RuntimeGenericResourceNode *resource;
    void *resource_data;
    uint32_t resource_metadata;
    uint32_t start_offset;
    uint32_t cursor;
    RuntimeVisualObject *primary_visual;
    char creation_text[0x104];
    char scratch_text[0x104];
    char name[0x20];
    RuntimeTreeParserContext *next;
};


struct RuntimeTreeParserContextApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

// GAG.EXE: 0x00405210
RuntimeTreeParserContext *find_or_create_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);

void set_runtime_tree_parser_context_api_for_testing(const RuntimeTreeParserContextApi &api);

struct RuntimeTreeParserReleaseApi
{
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void (*remove_resource)(void *identity);
};

// GAG.EXE: 0x004052F0
void release_runtime_tree_parser_contexts(RuntimeTreeNode *owner);

// GAG.EXE: 0x00405350
RuntimeTreeParserContext *find_existing_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name);

void set_runtime_tree_parser_release_api_for_testing(const RuntimeTreeParserReleaseApi &api);

struct RuntimeTreeCreationApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    RuntimeGenericResourceNode *(*find_resource)(void *identity);
    void *(*find_root_by_name)(const void *name);
    RuntimeTreeNode *(*find_ancestor_root)(void *identity);
    void *(*find_descendant_by_name)(void *root_identity, const void *name);
    int (*find_section)(const char *section_name, const char *text, int text_length);
    int (*find_property)(char *value, const char *property_name, const char *text, uint32_t text_length, uint32_t start_offset);
    RuntimeTreeNode *(*begin_enumeration)(void *identity);
    RuntimeTreeNode *(*next_enumeration)(RuntimeTreeNode *root);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    RuntimeTreeParserContext *(*create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);
    void (*remove_resource)(void *identity);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*activate_node)(RuntimeTreeNode *node);
};

// GAG.EXE: 0x004056C0
RuntimeTreeNode *dispatch_runtime_tree_parser(RuntimeTreeParserContext *context);

// GAG.EXE: 0x00405410
RuntimeTreeNode *create_runtime_tree_node(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);

void set_runtime_tree_creation_api_for_testing(const RuntimeTreeCreationApi &api);

struct RuntimeTreeJumpApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    uint32_t (*parse_name)(ScriptParserState *parser, char *name, uint32_t capacity);
    void (*synchronize_owner)(RuntimeTreeNode *owner);
    RuntimeGenericResourceNode *(*find_or_load_resource)(const char *name);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
};

// GAG.EXE: 0x00405D00
RuntimeTreeNode *find_and_create_runtime_tree_jump(ScriptParserState *parser, const char *target, uint32_t success_cursor);

void set_runtime_tree_jump_api_for_testing(const RuntimeTreeJumpApi &api);

struct RuntimeTreeConditionalCreateApi
{
    bool (*compare_field)(const char *object_name, const void *field_name, const void *value, int32_t value_type);
    bool (*container_matches)(const void *name);
    RuntimeTreeNode *(*find_descendant)(void *root_identity, const void *name);
    RuntimeGenericResourceNode *(*load_resource)(const char *name);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
    RuntimeTreeNode *(*destroy_node)(void *identity, void *replacement_identity);
};

// GAG.EXE: 0x00406CB0
RuntimeTreeNode *update_conditional_runtime_tree(ScriptParserState *parser);

// GAG.EXE: 0x00406EA0
RuntimeTreeNode *create_conditional_runtime_tree(ScriptParserState *parser);

void set_runtime_tree_conditional_create_api_for_testing(const RuntimeTreeConditionalCreateApi &api);

struct RuntimeTreeParserResetApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    RuntimeTreeNode *(*resolve_included_tree)(ScriptParserState *parser);
    RuntimeTreeNode *(*find_node)(void *identity);
};

struct RuntimeTreeParserDirectDispatchApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    uint32_t (*parse_object)(ScriptParserState *parser);
    uint32_t (*parse_link_0084)(ScriptParserState *parser);
    uint32_t (*parse_link_007c)(ScriptParserState *parser);
    uint32_t (*parse_visual)(ScriptParserState *parser);
    uint32_t (*parse_primary)(ScriptParserState *parser);
    uint32_t (*parse_container)(ScriptParserState *parser);
    uint32_t (*parse_command)(ScriptParserState *parser);
    uint32_t (*parse_named)(ScriptParserState *parser);
    uint32_t (*parse_link_008c)(ScriptParserState *parser);
    RuntimeTreeNode *(*create_conditional)(ScriptParserState *parser);
    uint32_t (*parse_auxiliary_names)(ScriptParserState *parser);
    uint32_t (*create_fixed_name)(ScriptParserState *parser);
    bool (*parse_language)(ScriptParserState *parser);
    uint32_t (*parse_secondary)(ScriptParserState *parser);
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*apply_image_flags)(ScriptParserState *parser);
    uintptr_t (*dispatch_section)(ScriptParserState *parser);
    void (*set_resource_position)(void *identity, uint32_t position);
    uint32_t (*read_resource_token)(void *identity, char *output, uint32_t capacity, uint8_t delimiter);
    uint32_t (*parse_scene)(ScriptParserState *parser);
    void (*add_auxiliary_name)(RuntimeTreeNode *owner, const char *name);
    void (*publish_links)(RuntimeTreeNode *owner);
};

void set_runtime_tree_parser_direct_dispatch_api_for_testing(const RuntimeTreeParserDirectDispatchApi &api);
void reset_runtime_tree_parser_direct_dispatch_api_for_testing();

struct RuntimeTreeParserSpecialDispatchApi
{
    int32_t (*parse_integer)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    RuntimeTreeNode *(*create_command)(ScriptParserState *parser);
    RuntimeTreeNode *(*find_jump)(ScriptParserState *parser, const char *property_name, uint32_t cursor);
    bool (*strings_equal)(const char *left, const char *right);
};

void set_runtime_tree_parser_special_dispatch_api_for_testing(const RuntimeTreeParserSpecialDispatchApi &api);
void reset_runtime_tree_parser_special_dispatch_api_for_testing();

// GAG.EXE: 0x00405E00
void reset_runtime_tree_parser_context_recursive(ScriptParserState *parser);

// GAG.EXE: 0x00405DC0
void reset_runtime_tree_parser_contexts(void *identity);

void set_runtime_tree_parser_reset_api_for_testing(const RuntimeTreeParserResetApi &api);

struct RuntimeTreeSectionDispatchApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    RuntimeGenericResourceNode *(*find_resource)(void *identity);
    int (*find_section)(const char *section_name, const char *text, int text_length);
    RuntimeTreeParserContext *(*create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*remove_resource)(void *identity);
};

// GAG.EXE: 0x00405380
RuntimeTreeNode *dispatch_runtime_tree_section(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);

void set_runtime_tree_section_dispatch_api_for_testing(const RuntimeTreeSectionDispatchApi &api);

struct RuntimeTreeBasicCommandApi
{
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*extract_parenthesized)(ScriptParserState *parser, char *text, uint32_t capacity);
    uint32_t (*parse_scope)(ScriptParserState *parser);
    RuntimeGenericResourceNode *(*load_resource)(const char *name);
    RuntimeTreeNode *(*dispatch_section)(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
};

// GAG.EXE: 0x00406A70
uintptr_t dispatch_runtime_tree_section_command(ScriptParserState *parser);

// GAG.EXE: 0x00406B90
bool parse_runtime_language(ScriptParserState *parser);

// GAG.EXE: 0x00406C00
RuntimeTreeNode *create_runtime_tree_command(ScriptParserState *parser);

void set_runtime_tree_basic_command_api_for_testing(const RuntimeTreeBasicCommandApi &api);

// GAG.EXE: 0x00405000
void remove_runtime_generic_resource(void *identity);

// GAG.EXE: 0x004068C0
void set_script_runtime_flags(uint32_t mask, int enabled);

// GAG.EXE: 0x0040C370
void reset_script_runtime_transient_indices();

// GAG.EXE: 0x00407810
RuntimeNamedNode *find_runtime_named_child(void *parent_identity, void *child_identity);

// GAG.EXE: 0x00407720
RuntimeResourceCacheEntry *find_runtime_resource_cache_entry(void *parent_identity, const char *name);

// GAG.EXE: 0x00407780
RuntimeResourceCacheEntry *get_or_create_runtime_resource_cache_entry(void *parent_identity, const char *name);

// GAG.EXE: 0x00407860
RuntimeResourceCacheEntry *get_or_create_runtime_child_by_data(void *parent_identity, void *data);

// GAG.EXE: 0x004078D0
void add_script_object_to_runtime_named_node(const void *node_name, const char *object_name);

// GAG.EXE: 0x00407490
uint32_t parse_runtime_named_node(ScriptParserState *parser);

// GAG.EXE: 0x00407990
void remove_script_object_from_runtime_named_node(const void *node_name, const char *object_name);

// GAG.EXE: 0x00407C00
uint32_t rotate_runtime_named_node_cursor_previous(const void *node_name, int32_t count);

// GAG.EXE: 0x00407C60
uint32_t rotate_runtime_named_node_cursor_next(const void *node_name, int32_t count);

// GAG.EXE: 0x00407CC0
uint32_t clear_runtime_named_node_children(const void *node_name);

// GAG.EXE: 0x00407D50
void remove_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

// GAG.EXE: 0x00407A20
uint32_t remove_runtime_named_child_by_identity(void *parent_identity, void *child_identity);

// GAG.EXE: 0x00424C50
BOOL release_runtime_memory_resource(const char *name);

// GAG.EXE: 0x00424CC0
BOOL release_runtime_memory_resource_by_data(void *data);

// GAG.EXE: 0x00424D30
uint32_t release_runtime_streamed_resource(AsyncFileRecord *record);

// GAG.EXE: 0x00407D10
void append_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

// GAG.EXE: 0x00407DD0
void serialize_runtime_named_nodes(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00407EE0
void purge_disabled_runtime_named_nodes();

// GAG.EXE: 0x00409370
uint32_t parse_runtime_command_definition(ScriptParserState *parser);

// GAG.EXE: 0x004094C0
void append_dual_image_flag(ScriptTextBuffer *buffer, uint32_t flags);

// GAG.EXE: 0x00409510
void serialize_runtime_command_definitions(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004095E0
void clear_runtime_command_definitions();

// GAG.EXE: 0x00407690
RuntimeNamedNode *get_or_create_runtime_named_node(const char *name);

// GAG.EXE: 0x0040A7A0
bool set_runtime_plans_inactive();

// GAG.EXE: 0x0040A800
bool clear_runtime_plans_inactive();

// GAG.EXE: 0x0040CD60
RuntimeTreeNode *find_runtime_tree_node(RuntimeTreeNode *root, void *identity);

// GAG.EXE: 0x004065E0
RuntimeTreeNode *find_runtime_tree_node_by_identity(void *identity);

// GAG.EXE: 0x004097D0
void *find_last_runtime_tree_scene_link(RuntimeTreeNode *root);

// GAG.EXE: 0x00409B60
void *find_last_runtime_tree_secondary_resource_link(RuntimeTreeNode *root);

// GAG.EXE: 0x0040A500
void *find_last_runtime_tree_primary_resource_link(RuntimeTreeNode *root);

// GAG.EXE: 0x00406860
void *find_last_runtime_scene_link_by_identity(void *identity);

// GAG.EXE: 0x00406880
void *find_last_runtime_primary_resource_link_by_identity(void *identity);

// GAG.EXE: 0x004068A0
void *find_last_runtime_secondary_resource_link_by_identity(void *identity);

struct RuntimeTreeSceneLink
{
    char name[0x20];
    void *identity;
    uint32_t z;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    intptr_t scene_identifier;
    RuntimeTreeSceneLink *next;
};


struct RuntimeTreePrimaryResourceLink
{
    char identifier[0x20];
    void *identity;
    RuntimeTreePrimaryResourceLink *next;
    uint32_t flags;
    char file_name[0x20];
    void *resource_identity;
    uint32_t unknown_0050;
    uint32_t image_flags;
    uint32_t loop_count;
    int32_t x;
    int32_t y;
    uint32_t source_value;
    uint32_t width;
    uint32_t height;
    uint32_t ratio_x;
    uint32_t ratio_y;
    RuntimeTreeSecondaryResourceLink *secondary_link;
    RuntimeFixedNameListNode *fixed_name_node;
    void *previous_resource_identity;
    int32_t previous_x;
    int32_t previous_y;
};

struct RuntimeTreeSecondaryResourceLink
{
    char name[0x20];
    void *identity;
    char file_name[0x20];
    void *resource_identity;
    RuntimeTreeSecondaryResourceLink *next;
};


// GAG.EXE: 0x00409600
uint32_t parse_runtime_tree_scene_link(ScriptParserState *parser);

// GAG.EXE: 0x00409A80
uint32_t parse_runtime_tree_secondary_resource_link(ScriptParserState *parser);

struct RuntimeTreeLink84
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink84 *next;
    uint32_t unknown_0028;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t state_003c;
    union
    {
        uint32_t command_mask;
        uint32_t value_0040;
    };
    uint32_t primary_command_bit;
    uint8_t unknown_0048[4];
    union
    {
        uint32_t parameter;
        uint32_t value_004c;
    };
    union
    {
        RuntimeVisualObject *mouse_visual;
        uint32_t value_0050;
    };
    uintptr_t value_0054;
    union
    {
        ScriptObjectState *owner_object;
        void *identity_0058;
    };
    union
    {
        RuntimeTreePrimaryResourceLink *primary_resource;
        void *identity_005c;
    };
    void *previous_identity_0058;
    void *previous_identity_005c;
};

struct RuntimeTreeLink7C
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink7C *next;
    uint8_t unknown_0028[0x10];
    ScriptParserState parser;
    RuntimeGenericBackendChild *backend_child;
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    uint32_t wait_deadline;
    uint32_t owner_flags;
    uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    uint32_t unknown_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t unknown_00a8;
    uint32_t flags;
    uint32_t unknown_00b0;
};

struct RuntimeTreeLink8C
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink8C *next;
    uint32_t time;
    uint32_t flags;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t unknown_0040[0x0c];
    int32_t line_first;
    int32_t line_second;
};


// GAG.EXE: 0x00409A40
RuntimeTreeSceneLink *find_global_runtime_tree_scene_link_by_name(const void *name);

// GAG.EXE: 0x00409830
RuntimeTreeSceneLink *find_runtime_tree_scene_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x00409880
void insert_runtime_tree_scene_link(RuntimeTreeNode *node, RuntimeTreeSceneLink *link);

// GAG.EXE: 0x00409920
void remove_runtime_tree_scene_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x00409E00
RuntimeTreeSecondaryResourceLink *find_global_runtime_tree_secondary_resource_link_by_name(const void *name);

// GAG.EXE: 0x00409BC0
RuntimeTreeSecondaryResourceLink *find_runtime_tree_secondary_resource_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x00409C10
void insert_runtime_tree_secondary_resource_link(RuntimeTreeNode *node, RuntimeTreeSecondaryResourceLink *link);

// GAG.EXE: 0x00409CB0
void remove_runtime_tree_secondary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040A560
RuntimeTreePrimaryResourceLink *find_runtime_tree_primary_resource_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040A5B0
void insert_runtime_tree_primary_resource_link(RuntimeTreeNode *node, RuntimeTreePrimaryResourceLink *link);

// GAG.EXE: 0x0040A650
void remove_runtime_tree_primary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040A860
void update_runtime_tree_primary_resource_link(void *tree_identity, void *link_identity, const void *name, int32_t x_delta, int32_t y_delta, uint32_t value_0054);

// GAG.EXE: 0x0040A920
void append_three_digit_decimal_suffix(const char *prefix, uint32_t value, char *output);

// GAG.EXE: 0x0040A3C0
void *create_or_update_runtime_tree_primary_resource_link(void *tree_identity, const void *identifier, const void *file_name, int32_t source_value, int32_t x_delta, int32_t y_delta,
    uint32_t image_flags);

// GAG.EXE: 0x0040AE40
void *create_or_update_runtime_tree_link_0084(void *tree_identity, const void *name, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t value_0050, void *identity_0058,
    void *identity_005c, uintptr_t value_0054, uint32_t value_0040, uint32_t value_004c);

// GAG.EXE: 0x00409E50
uint32_t parse_runtime_tree_primary_resource_link(ScriptParserState *parser);

// GAG.EXE: 0x0040A990
RuntimeTreePrimaryResourceLink *find_global_runtime_tree_primary_resource_link_by_name(const void *name);

// GAG.EXE: 0x0040AAC0
uint32_t parse_runtime_tree_link_0084(ScriptParserState *parser);

// GAG.EXE: 0x0040AFE0
RuntimeTreeLink84 *find_last_runtime_tree_link_0084(RuntimeTreeNode *root);

// GAG.EXE: 0x0040B040
RuntimeTreeLink84 *find_runtime_tree_link_0084_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040B090
void insert_runtime_tree_link_0084(RuntimeTreeNode *node, RuntimeTreeLink84 *link);

// GAG.EXE: 0x0040B130
void remove_runtime_tree_link_0084_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040B280
void update_runtime_tree_link_0084(void *tree_identity, void *link_identity, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t value_0050, void *identity_0058, void *identity_005c,
    uintptr_t value_0054, uint32_t value_0040, uint32_t value_004c);

// GAG.EXE: 0x0040B380
RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_name(const void *name);

// GAG.EXE: 0x0040B3C0
RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_identity(void *identity);

// GAG.EXE: 0x0040B3E0
uint32_t parse_runtime_tree_link_008c(ScriptParserState *parser);

// GAG.EXE: 0x0040B850
uint32_t parse_runtime_tree_link_007c(ScriptParserState *parser);

// GAG.EXE: 0x0040C1E0
void seek_runtime_tree_link_007c_label(void *identity, const char *label);

// GAG.EXE: 0x0040C260
uint32_t find_runtime_tree_link_007c_opcode_value(void *identity, uint32_t opcode, const char *value, int restore_cursor);

// GAG.EXE: 0x0040C2F0
uint32_t scan_runtime_tree_link_007c_control_boundary(void *identity, uint32_t requested_boundary);

// GAG.EXE: 0x0040BF60
struct RuntimeTreeInteractionCriteria
{
    uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    uint32_t unknown_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t unknown_00a8;
    uint32_t flags;
    uint32_t unknown_00b0;
    const RuntimeTreeLink7C *source_link;
};

uint32_t match_runtime_tree_link_007c_interaction(uintptr_t *state, const RuntimeTreeInteractionCriteria *criteria);

// GAG.EXE: 0x0040C4B0
uint32_t activate_runtime_tree_link_007c(RuntimeTreeLink7C *link);

// GAG.EXE: 0x0040C570
uint32_t parse_script_object_container(ScriptParserState *parser);

// GAG.EXE: 0x0040BCD0
RuntimeTreeLink7C *find_last_runtime_tree_link_007c(RuntimeTreeNode *root);

// GAG.EXE: 0x0040BD30
RuntimeTreeLink7C *find_runtime_tree_link_007c_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040BD80
void insert_runtime_tree_link_007c(RuntimeTreeNode *node, RuntimeTreeLink7C *link);

// GAG.EXE: 0x0040BE20
void remove_runtime_tree_link_007c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040C8A0
ScriptObjectContainer *find_last_script_object_container(RuntimeTreeNode *root);

// GAG.EXE: 0x0040C900
ScriptObjectContainer *find_script_object_container_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040C950
void insert_script_object_container(RuntimeTreeNode *node, ScriptObjectContainer *container);

// GAG.EXE: 0x0040C9F0
void remove_script_object_container_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040CB40
BOOL destroy_script_object_container(ScriptObjectContainer *container);

// GAG.EXE: 0x0040CBA0
bool script_object_container_state_matches_by_identity(void *identity);

// GAG.EXE: 0x0040CC20
bool script_object_container_state_matches_by_name(const void *name);

// GAG.EXE: 0x0040CCB0
ScriptObjectContainer *find_script_condition_container_by_name(const void *name);

// GAG.EXE: 0x0040B560
RuntimeTreeLink8C *find_last_runtime_tree_link_008c(RuntimeTreeNode *root);

// GAG.EXE: 0x0040B5C0
RuntimeTreeLink8C *find_runtime_tree_link_008c_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040B610
void insert_runtime_tree_link_008c(RuntimeTreeNode *node, RuntimeTreeLink8C *link);

// GAG.EXE: 0x0040B6B0
void remove_runtime_tree_link_008c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040B800
RuntimeTreeLink8C *find_global_runtime_tree_link_008c_by_name(const void *name);

struct RuntimeTreeDestructionApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    void (*set_resource_state)(void *identity, uint32_t state);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    uint32_t (*stop_game_dll)();
    void (*reset_display_state)();
    void *(*find_primary_tail)(void *identity);
    void *(*find_secondary_tail)(void *identity);
    void *(*find_scene_tail)(void *identity);
    uint32_t (*query_scene_flags)(void *identity);
    void (*destroy_resource_and_scene)(void *identity);
    void (*request_resource_destruction)(void *identity);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void (*wait_for_resource_count)(uint32_t count);
};

// GAG.EXE: 0x00426BD0
void destroy_runtime_tree_resources(void *identity);

void set_runtime_tree_destruction_api_for_testing(const RuntimeTreeDestructionApi &api);
void set_runtime_tree_destruction_state_for_testing(void *pointer_root_identity, void *current_resource, uint32_t resource_count);

struct RuntimeResourceSceneDestructionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*destroy_resource)(void *identity);
    void (*release_record)(RuntimeLockRecord *record);
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*update_scene_region)(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height);
};

struct RuntimeResourceSceneRegionApi
{
    DisplaySceneNode *(*lock_scene)(intptr_t identifier);
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*begin_scene_update)(intptr_t identifier);
    uint32_t (*render_backend_region)(void *backend_identity, DisplayRectangle *rectangle);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    uint32_t (*update_root_scene_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);
    void (*release_record)(RuntimeLockRecord *record);
    void (*unlock_scene)(intptr_t identifier);
};

// GAG.EXE: 0x00427900
void update_runtime_resource_scene_region(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height);

void set_runtime_resource_scene_region_api_for_testing(const RuntimeResourceSceneRegionApi &api);
void set_runtime_resource_scene_region_default_for_testing(intptr_t scene_identifier);

// GAG.EXE: 0x00417370
void copy_runtime_bitmap_region(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);

struct RuntimeBitmapRegionRenderApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE object, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void (*copy_bitmap_region)(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);
};

// GAG.EXE: 0x0042B140
uint32_t render_runtime_bitmap_backend_region(void *identity, DisplayRectangle *rectangle);

void set_runtime_bitmap_region_render_api_for_testing(const RuntimeBitmapRegionRenderApi &api);

struct RuntimeSceneTransitionSelectionApi
{
    int (*random)();
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    void (*apply_palette)(uint32_t value, uint32_t flags);
    void (*apply_rectangle)(uint8_t value, uint32_t flags);
};

// GAG.EXE: 0x00426D50
void select_runtime_scene_transition(uint32_t flags);

void set_runtime_scene_transition_selection_api_for_testing(const RuntimeSceneTransitionSelectionApi &api);
void set_runtime_scene_transition_selection_state_for_testing(uint32_t available_transitions, uint32_t palette_value, uint32_t rectangle_value, uint16_t bits_per_pixel);

struct RuntimeResourceStateApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*begin_scene_update)(intptr_t identifier);
    void (*finalize_backend)(void *identity);
    void (*configure_palette)(RuntimeResourceObject *resource);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void (*clear_child_ready)(void *identity);
    void (*enable_child_mode)(void *identity);
    void (*disable_child_mode)(void *identity);
    void (*select_transition)(uint32_t flags);
    RuntimeSoundSlot *(*get_sound_slot)(uint32_t handle);
    uint32_t (*queue_sound_data)(uint32_t handle, void *data, uint32_t size, int32_t replace);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    void (*release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x00425930
void set_runtime_resource_state(void *identity, uint32_t state);

void set_runtime_resource_state_api_for_testing(const RuntimeResourceStateApi &api);
void set_runtime_resource_state_globals_for_testing(void *current_resource, uint32_t scene_flags);

struct RuntimeImmediateSceneTransitionApi
{
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*release_display_lock)();
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*synchronize_region)(DisplayRectangle *rectangle, uint32_t mode);
    UINT (*apply_palette)(const PALETTEENTRY *entries, uint32_t flags);
    void (*release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x00426E30
void apply_immediate_runtime_scene_transition(uint32_t unused, uint32_t flags);

void set_runtime_immediate_scene_transition_api_for_testing(const RuntimeImmediateSceneTransitionApi &api);

struct RuntimePaletteSceneTransitionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    UINT (*apply_palette)(const PALETTEENTRY *entries, uint32_t flags);
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    uint32_t (*release_display_lock)();
    void (*release_record)(RuntimeLockRecord *record);
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*invalidate_framebuffer)(int32_t x, int32_t y, int32_t width, int32_t height);
};

// GAG.EXE: 0x00426F40
void apply_palette_runtime_scene_transition(uint32_t step, uint32_t flags);

void set_runtime_palette_scene_transition_api_for_testing(const RuntimePaletteSceneTransitionApi &api);
void set_runtime_palette_scene_transition_state_for_testing(void *current_resource, uint32_t scene_flags, uint16_t width, uint16_t height);
const PALETTEENTRY *get_runtime_palette_scene_transition_entries_for_testing();

struct RuntimeRectangleSceneTransitionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*release_display_lock)();
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    void (*synchronize_region)(DisplayRectangle *rectangle, uint32_t mode);
    UINT (*apply_palette)(const PALETTEENTRY *entries, uint32_t flags);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    DWORD(WINAPI *time_get_time)();
    DWORD(WINAPI *get_tick_count)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x004272D0
void apply_rectangle_runtime_scene_transition(uint8_t size, uint32_t flags);

void set_runtime_rectangle_scene_transition_api_for_testing(const RuntimeRectangleSceneTransitionApi &api);
void set_runtime_rectangle_scene_transition_state_for_testing(void *current_resource, uint16_t width, uint16_t height);

struct DisplayRegionSynchronizationApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    HRESULT(WINAPI *blt_fast)(void *surface, DWORD x, DWORD y, void *source, RECT *source_rectangle, DWORD flags);
    HRESULT(WINAPI *blt)(void *surface, RECT *destination_rectangle, void *source, RECT *source_rectangle, DWORD flags, void *effects);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
    BOOL(WINAPI *stretch_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD operation);
    BOOL(WINAPI *pat_blt)(HDC destination, int x, int y, int width, int height, DWORD operation);
};

// GAG.EXE: 0x00414220
void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode);

void set_display_region_synchronization_api_for_testing(const DisplayRegionSynchronizationApi &api);
void set_display_region_synchronization_state_for_testing(uint32_t flags, void *primary_surface, void *secondary_surface, HDC primary_context, HDC secondary_context);

struct LegacyDirectDrawSurfaceDescriptor;

struct DisplayTargetBeginApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    HRESULT(WINAPI *is_surface_lost)(void *surface);
    HRESULT(WINAPI *restore_surface)(void *surface);
    HRESULT(WINAPI *lock_surface)(void *surface, RECT *rectangle, LegacyDirectDrawSurfaceDescriptor *descriptor, DWORD flags, HANDLE event);
};

// GAG.EXE: 0x00414360
uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch);

void set_display_target_begin_api_for_testing(const DisplayTargetBeginApi &api);
void set_display_target_begin_state_for_testing(uint32_t flags, void *secondary_surface, void *pixels, int32_t width, int32_t height, DisplayMode *mode);

// GAG.EXE: 0x00425C40
void finalize_runtime_resource_destruction(void *identity);

void set_runtime_resource_scene_destruction_api_for_testing(const RuntimeResourceSceneDestructionApi &api);

// GAG.EXE: 0x00409330
RuntimeVisualObject *find_runtime_visual_object(const char *name);

// GAG.EXE: 0x0040C3D0
void enqueue_runtime_event_record(const uintptr_t *record);

// GAG.EXE: 0x0040C390
void acknowledge_current_runtime_event_record();

// GAG.EXE: 0x0040C440
uint32_t read_runtime_event_record(uintptr_t *record, int32_t advance);

// GAG.EXE: 0x004237F0
int32_t select_pointer_region_scene(RuntimePointerRegion *region);

// GAG.EXE: 0x00407A80
uint32_t synchronize_runtime_pointer_owner_slots(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);

struct RuntimePointerResourceRebuildApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    uint32_t (*synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    uint32_t (*query_scene_flags)(void *identity);
    void (*finalize_destruction)(void *identity);
    void (*request_destruction)(void *identity);
    RuntimeResourceConstructor construct_resource;
    void (*update_position)(void *identity, int32_t x, int32_t y);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*wait_for_count)(uint32_t count);
};

struct RuntimeTreeResourceRebuildApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    RuntimeResourceConstructor construct_resource;
    uint32_t (*synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    uint32_t (*query_scene_flags)(void *identity);
    void (*request_destruction)(void *identity);
    RuntimeGenericBackendChild *(*configure_resource)(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, uintptr_t value, uint32_t flags);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void (*wait_for_count)(uint32_t count);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void (*reset_transient_indices)();
    void (*set_resource_state)(void *identity, uint32_t state);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

struct RuntimeGenericChildAttachmentApi
{
    RuntimeLockRecord *(*acquire_resource)(void *identity);
    void (*release_resource)(RuntimeLockRecord *record);
    uint32_t (*find_scene_index)(uint32_t candidate);
    RuntimeGenericBackendChild *(*create_child)(void *backend_identity, void *font_identity, const uintptr_t *context, uintptr_t selection, uint32_t flags);
    DisplaySceneNode *(*lock_scene)(intptr_t identifier);
    void (*unlock_scene)(intptr_t identifier);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    void *(*destroy_child)(void *identity);
};

// GAG.EXE: 0x00425D50
RuntimeGenericBackendChild *attach_runtime_generic_backend_child(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, uintptr_t selection, uint32_t flags);

void set_runtime_generic_child_attachment_api_for_testing(const RuntimeGenericChildAttachmentApi &api);
void set_runtime_generic_child_attachment_scene_for_testing(intptr_t identifier);

// GAG.EXE: 0x004268B0
void rebuild_runtime_tree_resources(void *identity);

void set_runtime_tree_resource_rebuild_api_for_testing(const RuntimeTreeResourceRebuildApi &api);

// GAG.EXE: 0x00426700
void rebuild_runtime_pointer_resources();

void set_runtime_pointer_resource_rebuild_api_for_testing(const RuntimePointerResourceRebuildApi &api);

// GAG.EXE: 0x00423BC0
uint32_t handle_runtime_left_button_up();

// GAG.EXE: 0x004238B0
uint32_t handle_runtime_left_button_down();

// GAG.EXE: 0x00423CA0
uint32_t handle_runtime_right_button_down();

// GAG.EXE: 0x00423FA0
uint32_t update_runtime_pointer_region(int32_t x, int32_t y);

struct RuntimePointerRefreshApi
{
    uint32_t (*update_region)(int32_t x, int32_t y);
};

// GAG.EXE: 0x004236C0
uint32_t refresh_runtime_pointer_region();

void set_runtime_pointer_refresh_api_for_testing(const RuntimePointerRefreshApi &api);

// GAG.EXE: 0x004235E0
int32_t activate_default_comment_scene(const char *name);

// GAG.EXE: 0x004236E0
void activate_runtime_tree_node_comment(RuntimeTreeNode *node);

// GAG.EXE: 0x00423660
void deactivate_default_comment_scene(const char *name);

// GAG.EXE: 0x00423710
void deactivate_runtime_tree_node_comment(RuntimeTreeNode *node);

// GAG.EXE: 0x00426320
void set_runtime_tree_comment_mode(RuntimeTreeNode *root, int enabled);

// GAG.EXE: 0x00406770
RuntimeTreeNode *begin_runtime_tree_enumeration(void *identity);

// GAG.EXE: 0x004067F0
RuntimeTreeNode *get_next_runtime_tree_node(RuntimeTreeNode *root);

struct RuntimeCommentTreeCleanupApi
{
    RuntimeTreeNode *(*begin_enumeration)(void *identity);
    RuntimeTreeNode *(*next_node)(RuntimeTreeNode *root);
    void (*destroy_resources)(void *identity);
    intptr_t (*deactivate_node)(void *identity, void *second);
    void (*finalize_destroyed_nodes)(void *identity);
    void (*rebuild_runtime_plans)();
};

// GAG.EXE: 0x00423740
int destroy_runtime_comment_trees();

struct RuntimeTreeDeactivateApi
{
    RuntimeTreeNode *(*resolve_identity)(void *identity);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*request_resource_destruction)(void *identity);
    BOOL (*remove_visual_object)(void *identity);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void (*deactivate_comment)(RuntimeTreeNode *node);
    intptr_t (*destroy_tree)(void *first, void *second);
};

// GAG.EXE: 0x00426600
intptr_t deactivate_runtime_tree_and_visuals(void *identity, void *second);

void set_runtime_comment_tree_cleanup_api_for_testing(const RuntimeCommentTreeCleanupApi &api);
void set_runtime_tree_deactivate_api_for_testing(const RuntimeTreeDeactivateApi &api);

// GAG.EXE: 0x00406640
void *find_runtime_tree_identity_by_name_recursive(void *start_identity, const void *name);

// GAG.EXE: 0x004066C0
void *find_runtime_tree_descendant_identity_by_name(void *root_identity, const void *name);
void *find_runtime_drag_cleanup_descendant();

// GAG.EXE: 0x00406720
void *find_runtime_tree_root_identity_by_name(const void *name);

// GAG.EXE: 0x004237B0
uint32_t has_runtime_pointer_tree_flag_1000();

// GAG.EXE: 0x00425FA0
void release_runtime_lock_record(RuntimeLockRecord *record);

// GAG.EXE: 0x00425F10
RuntimeLockRecord *acquire_runtime_lock_record(void *child_identity);

struct RuntimeResourceLoopApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*set_sound_loop)(uint32_t handle, uint32_t value);
    void (*release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x004258D0
void set_runtime_resource_loop_count(void *identity, uint32_t count);

void set_runtime_resource_loop_api_for_testing(const RuntimeResourceLoopApi &api);

// GAG.EXE: 0x004242C0
void switch_runtime_scene(void *identity);

// GAG.EXE: 0x004262B0
void reset_runtime_display_state();

// GAG.EXE: 0x00420130
uint32_t shutdown_runtime_display();

// GAG.EXE: 0x00425FD0
uint32_t query_runtime_scene_flags(void *identity);

// GAG.EXE: 0x00426D20
void wait_for_runtime_resource_count(uint32_t count);

// GAG.EXE: 0x00425EB0
void update_runtime_scene_position(void *identity, int32_t x, int32_t y);

// GAG.EXE: 0x004246B0
void build_runtime_resource_path(char *destination, const char *source);

// GAG.EXE: 0x00424570
void update_runtime_resource_host(const char *path, int32_t reset);

// GAG.EXE: 0x00424710
uint32_t detect_runtime_resource_type(const char *path);

// GAG.EXE: 0x00428720
void *open_runtime_cdf_entry_stream(CdfArchive *archive, const char *name);

// GAG.EXE: 0x00424870
void load_runtime_resource(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);

// GAG.EXE: 0x00414DD0
uint32_t extract_runtime_drive_prefix(char *destination, const char *source);

// GAG.EXE: 0x00417990
uint32_t measure_archive_read_speed(const char *archive_path, uint32_t bytes_to_measure);
void set_archive_read_speed_api_for_testing(const ArchiveReadSpeedApi &api);

// GAG.EXE: 0x0042B6B0
HANDLE open_runtime_resource_file(const char *path);

// GAG.EXE: 0x00415040
AsyncFileHost *acquire_async_file_host(AsyncFileHost *identity);

// GAG.EXE: 0x00414EC0
AsyncFileHost *create_async_file_host(const char *root, uint32_t requested_bytes, int32_t mode);

// GAG.EXE: 0x00414900
void advance_async_host_write(AsyncFileHost *host, uint32_t bytes);

// GAG.EXE: 0x004148B0
void advance_async_host_read(AsyncFileHost *host, uint32_t bytes);

// GAG.EXE: 0x00414A50
void invalidate_shared_async_records(AsyncFileRecord *record);

// GAG.EXE: 0x00414930
void position_async_host(AsyncFileHost *host, uint32_t offset);

// GAG.EXE: 0x00414AE0
void seek_async_host(AsyncFileHost *host, uint32_t offset);

// GAG.EXE: 0x00414BB0
uint32_t copy_async_host_bytes(AsyncFileHost *host, void *destination, uint32_t bytes, uint32_t *total_bytes);

// GAG.EXE: 0x00414CB0
void activate_async_file_record(AsyncFileRecord *record);

// GAG.EXE: 0x00415AE0
void handle_async_host_short_read(AsyncFileHost *host);

// GAG.EXE: 0x00415B70
DWORD WINAPI run_async_file_worker(LPVOID parameter);

// GAG.EXE: 0x004150D0
void release_async_file_host(AsyncFileHost *identity);

// GAG.EXE: 0x00415120
uint32_t destroy_async_file_host(AsyncFileHost *identity);

struct AsyncFileShutdownApi
{
    uint32_t (*destroy_host)(AsyncFileHost *identity);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x00414E40
uint32_t shutdown_async_file_subsystem();

void set_async_file_shutdown_api_for_testing(const AsyncFileShutdownApi &api);

// GAG.EXE: 0x004155C0
AsyncFileRecord *acquire_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415690
void release_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415210
void set_async_file_host_mode(AsyncFileHost *identity, int32_t mode);

// GAG.EXE: 0x00415AC0
uint32_t get_async_file_size(AsyncFileRecord *identity);

// GAG.EXE: 0x00415AA0
uint32_t get_async_file_position(AsyncFileRecord *identity);

// GAG.EXE: 0x00415A20
uint32_t set_async_file_position(AsyncFileRecord *identity, uint32_t position);

// GAG.EXE: 0x00415230
AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags);

// GAG.EXE: 0x00415360
AsyncFileRecord *duplicate_async_file_record(AsyncFileRecord *identity, uint32_t start_offset, uint32_t end_offset, uint32_t flags);

// GAG.EXE: 0x00415420
uint32_t close_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415720
uint32_t read_async_file_record(AsyncFileRecord *identity, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);

void set_runtime_named_lock_api_for_testing(const RuntimeNamedLockApi &api);
void set_runtime_named_node_memory_api_for_testing(const RuntimeNamedNodeMemoryApi &api);
void set_runtime_resource_release_api_for_testing(const RuntimeResourceReleaseApi &api);
void set_runtime_media_backend_api_for_testing(const RuntimeMediaBackendApi &api);
void set_runtime_media_backend_state_for_testing(HANDLE heap, HANDLE mutex, RuntimeMediaBackend *head, RuntimeMediaBackend *tail);

// GAG.EXE: 0x0042B290
RuntimeMediaBackend *acquire_first_runtime_media_backend();

struct RuntimeMediaBackendShutdownApi
{
    RuntimeMediaBackend *(*acquire_first_backend)();
    void (*release_backend_lock)(RuntimeMediaBackend *backend);
    BOOL(WINAPI *heap_destroy)(HANDLE heap);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    uint32_t (*shutdown_sound)();
};

// GAG.EXE: 0x00429E50
uint32_t shutdown_runtime_media_backend();

void set_runtime_media_backend_shutdown_api_for_testing(const RuntimeMediaBackendShutdownApi &api);
void set_runtime_palette_update_api_for_testing(const RuntimePaletteUpdateApi &api);
void set_runtime_bitmap_backend_create_api_for_testing(const RuntimeBitmapBackendCreateApi &api);
void set_runtime_animation_backend_create_api_for_testing(const RuntimeAnimationBackendCreateApi &api);
void set_runtime_media_backend_configure_api_for_testing(const RuntimeMediaBackendConfigureApi &api);
void set_runtime_animation_backend_configure_api_for_testing(const RuntimeAnimationBackendConfigureApi &api);
void set_runtime_resource_palette_configure_api_for_testing(const RuntimeResourcePaletteConfigureApi &api);
void set_runtime_resource_palette_bits_per_pixel_for_testing(uint32_t bits_per_pixel);
void set_runtime_media_backend_finalize_api_for_testing(const RuntimeMediaBackendFinalizeApi &api);
void set_runtime_animation_failure_api_for_testing(const RuntimeAnimationFailureApi &api);
void set_runtime_animation_control_flags_for_testing(uint32_t flags);
uint32_t get_runtime_animation_control_flags_for_testing();
void set_runtime_animation_control_api_for_testing(const RuntimeAnimationControlApi &api);
void set_runtime_animation_frame_acquire_api_for_testing(const RuntimeAnimationFrameAcquireApi &api);
void set_runtime_animation_decode_api_for_testing(const RuntimeAnimationDecodeApi &api);
void set_runtime_animation_completion_api_for_testing(const RuntimeAnimationCompletionApi &api);
void set_runtime_animation_audio_api_for_testing(const RuntimeAnimationAudioApi &api);
void set_runtime_animation_worker_api_for_testing(const RuntimeAnimationWorkerApi &api);
void set_runtime_resource_construction_plan_api_for_testing(const RuntimeResourceConstructionPlanApi &api);
void set_runtime_animation_present_api_for_testing(const RuntimeAnimationPresentApi &api);
void set_runtime_generic_backend_api_for_testing(const RuntimeGenericBackendApi &api);
void set_runtime_generic_backend_state_for_testing(HANDLE mutex, RuntimeGenericBackend *head);
void set_runtime_generic_backend_create_api_for_testing(const RuntimeGenericBackendCreateApi &api);
void set_runtime_generic_backend_create_state_for_testing(uint32_t enabled);
RuntimeGenericBackend *get_runtime_generic_backend_head_for_testing();
void set_runtime_sound_destroy_api_for_testing(const RuntimeSoundDestroyApi &api);
void set_runtime_sound_destroy_state_for_testing(int32_t enabled, HANDLE mutex, RuntimeSoundSlot *slots, uint32_t maximum_handle);
RuntimeSoundSlot *use_runtime_sound_backing_storage_for_testing();
uint32_t get_runtime_sound_maximum_handle_for_testing();
void set_runtime_sound_create_api_for_testing(const RuntimeSoundCreateApi &api);
void set_runtime_sound_create_state_for_testing(HANDLE lifecycle_mutex, HWAVEOUT wave_out, WAVEFORMATEX *output_format, WAVEHDR *header_1, WAVEHDR *header_2, HWND window, HANDLE thread,
    DWORD thread_id, uint32_t output_ready, uint32_t ready, uint32_t fault);
void get_runtime_sound_create_state_for_testing(HANDLE *thread, DWORD *thread_id, uint32_t *output_initialized, uint32_t *ready);
void set_runtime_sound_format_cleanup_api_for_testing(const RuntimeSoundFormatCleanupApi &api);
void set_runtime_sound_format_cleanup_state_for_testing(void *buffer, uint32_t base_state);
void get_runtime_sound_format_cleanup_state_for_testing(void **buffer, uint32_t *base_state);
void set_runtime_sound_fade_state_for_testing(WAVEFORMATEX *output_format, uint32_t mixer_data_size);
void set_runtime_wave_out_callback_api_for_testing(const RuntimeWaveOutCallbackApi &api);
void set_runtime_wave_out_callback_state_for_testing(HWND window, uint32_t output_ready);
uint32_t get_runtime_wave_out_callback_state_for_testing();
void set_runtime_sound_shutdown_api_for_testing(const RuntimeSoundShutdownApi &api);
void set_runtime_sound_shutdown_state_for_testing(HANDLE lifecycle_mutex, HWAVEOUT wave_out, WAVEHDR *header_1, WAVEHDR *header_2, HANDLE thread, DWORD thread_id, uint32_t output_ready,
    uint32_t output_initialized);
void get_runtime_sound_shutdown_state_for_testing(int32_t *enabled, HANDLE *thread, DWORD *thread_id, uint32_t *output_initialized);
void set_runtime_sound_readiness_api_for_testing(const RuntimeSoundReadinessApi &api);
void set_runtime_sound_readiness_state_for_testing(uint32_t ready);
uint32_t get_runtime_sound_readiness_state_for_testing();
void set_runtime_sound_thread_api_for_testing(const RuntimeSoundThreadApi &api);
void set_runtime_sound_thread_state_for_testing(HINSTANCE instance, HWND window, uint32_t creation_failed);
void get_runtime_sound_thread_state_for_testing(HWND *window, uint32_t *creation_failed);
void set_runtime_sound_class_api_for_testing(const RuntimeSoundClassApi &api);
void set_runtime_sound_window_api_for_testing(const RuntimeSoundWindowApi &api);
void set_runtime_sound_window_state_for_testing(RuntimeSoundOutputBlock *outputs, WAVEFORMATEX *output_format, uint32_t mixer_data_size, uint32_t output_initialized, uint32_t output_index,
    void (*mixer)(uint32_t marker));
void set_runtime_sound_mixing_suppressed_for_testing(uint8_t suppressed);
void set_runtime_wave_mixer_initialize_api_for_testing(const struct RuntimeWaveMixerInitializeApi &api);
void set_runtime_wave_mixer_initialize_state_for_testing(uint32_t fault, uint32_t window_creation_failed);
void get_runtime_wave_mixer_initialize_state_for_testing(uint32_t *fault, void **buffer, uint32_t *mixer_data_size, void (**mixer)(uint32_t marker));
void get_runtime_sound_window_state_for_testing(uint32_t *output_ready, uint32_t *output_initialized, uint32_t *output_index);

// GAG.EXE: 0x004023C0
void mix_runtime_sound_8bit_mono(uint32_t marker);

// GAG.EXE: 0x00402770
void mix_runtime_sound_8bit_stereo(uint32_t marker);

// GAG.EXE: 0x00402B10
void mix_runtime_sound_16bit_mono(uint32_t marker);

// GAG.EXE: 0x00402F10
void mix_runtime_sound_16bit_stereo(uint32_t marker);

struct RuntimeWaveMixerInitializeApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    void(WINAPI *sleep)(DWORD milliseconds);
    MMRESULT(WINAPI *wave_out_open)(LPHWAVEOUT wave_out, UINT device_id, LPCWAVEFORMATEX format, DWORD_PTR callback, DWORD_PTR instance, DWORD flags);
    void (*cleanup_format_buffer)();
};

// GAG.EXE: 0x00401330
uint32_t initialize_runtime_wave_out_mixer(WAVEFORMATEX *format, uint32_t unused_argument);
void set_runtime_resource_destroy_api_for_testing(const RuntimeResourceDestroyApi &api);
void set_runtime_resource_control_api_for_testing(const RuntimeResourceControlApi &api);
void set_runtime_game_lifecycle_api_for_testing(const RuntimeGameLifecycleApi &api);
void set_runtime_game_integration_api_for_testing(const RuntimeGameIntegrationApi &api);
void set_runtime_game_dll_dispatch_api_for_testing(const RuntimeGameDllDispatchApi &api);
void set_runtime_game_dll_state_for_testing(uint32_t flags);
void set_runtime_game_dll_execute_for_testing(RuntimeGameDllExecute execute);
void set_runtime_game_window_api_for_testing(const RuntimeGameWindowApi &api);
void set_runtime_game_window_state_for_testing(HWND main_window, RuntimeGameDllWindowProcedure window_procedure, uint16_t x_offset, uint16_t y_offset);
void get_runtime_game_result_for_testing(uint32_t *type, void *data, uint32_t size);
void set_runtime_pointer_position_api_for_testing(const RuntimePointerPositionApi &api);
void get_runtime_pointer_position_for_testing(int32_t *x, int32_t *y);
void set_runtime_game_host_state_for_testing(const RuntimeGameHostContext &context, void *const *callbacks);
RuntimeGameHostContext get_runtime_game_host_state_for_testing();
void get_runtime_game_dll_state_for_testing(RuntimeGameDllWindowProcedure *window_procedure, RuntimeGameDllExecute *execute);
void set_runtime_resource_destroy_state_for_testing(void *current_resource);
void *get_runtime_resource_destroy_state_for_testing();
void set_runtime_named_lock_state_for_testing(void *parent_identity);
CRITICAL_SECTION *get_runtime_named_lock_critical_section_for_testing();
CRITICAL_SECTION *get_runtime_resource_critical_section_for_testing();
CRITICAL_SECTION *get_runtime_game_dll_critical_section_for_testing();
void *get_runtime_named_lock_parent_identity_for_testing();
HWND get_runtime_resource_notification_window_for_testing();
void set_runtime_scene_switch_api_for_testing(const RuntimeSceneSwitchApi &api);
void set_runtime_scene_switch_state_for_testing(void *current_identity, int32_t x, int32_t y);
void *get_current_runtime_scene_identity_for_testing();
void set_runtime_scene_control_state_for_testing(uint32_t flags, void *saved_identity);
uint32_t get_runtime_scene_control_flags_for_testing();
void set_runtime_scene_slots_for_testing(const RuntimeSceneSlot *slots);
const RuntimeSceneSlot *get_runtime_scene_slots_for_testing();
void set_runtime_pointer_region_state_for_testing(void *root_identity, RuntimePointerRegion *regions, RuntimePointerRegion *active_region, uint32_t state_mask, void *state_owner);
RuntimePointerRegion *get_active_runtime_pointer_region_for_testing();
RuntimePointerRegion *get_runtime_pointer_regions_for_testing();
uint32_t get_runtime_pointer_event_flags_for_testing();
void set_runtime_display_reset_api_for_testing(const RuntimeDisplayResetApi &api);
void set_runtime_display_reset_state_for_testing(uint32_t value_1, uint8_t byte_value, uint32_t value_2, const uint32_t *scene_state);
void get_runtime_display_reset_state_for_testing(uint32_t *value_1, uint8_t *byte_value, uint32_t *value_2, uint32_t *scene_state);
void set_runtime_display_shutdown_api_for_testing(const RuntimeDisplayShutdownApi &api);
void set_runtime_display_shutdown_state_for_testing(HANDLE thread, intptr_t scene_identifier, void *host, const RuntimePresentationTarget *backend_state,
    const DisplayPixelFormatDescriptor *pixel_format_state);
void get_runtime_display_shutdown_state_for_testing(HANDLE *thread, intptr_t *scene_identifier, void **host, RuntimePresentationTarget *backend_state,
    DisplayPixelFormatDescriptor *pixel_format_state);
void set_runtime_resource_wait_api_for_testing(const RuntimeResourceWaitApi &api);
void set_runtime_resource_count_for_testing(uint32_t count);
uint32_t get_runtime_resource_count_for_testing();
void set_runtime_resource_directory_for_testing(const char *directory);
void set_runtime_resource_file_open_api_for_testing(const RuntimeResourceFileOpenApi &api);
void set_runtime_resource_host_api_for_testing(const RuntimeResourceHostApi &api);
void set_runtime_resource_host_state_for_testing(AsyncFileHost *host, CdfArchive *archive, int32_t mode, uint8_t archive_state);
void get_runtime_resource_host_state_for_testing(AsyncFileHost **host, CdfArchive **archive, int32_t *mode, uint8_t *archive_state);
void set_runtime_resource_type_api_for_testing(const RuntimeResourceTypeApi &api);
void set_runtime_resource_type_state_for_testing(void *cache_parent_identity, HWND notification_window);
void set_runtime_cdf_stream_api_for_testing(const RuntimeCdfStreamApi &api);
void set_runtime_resource_load_api_for_testing(const RuntimeResourceLoadApi &api);
void set_runtime_resource_load_state_for_testing(HANDLE heap, uint32_t streamed_count);
uint32_t get_runtime_resource_streamed_count_for_testing();
void set_async_file_lock_api_for_testing(const AsyncFileLockApi &api);
void set_async_file_state_for_testing(bool enabled, AsyncFileHost *hosts);
void set_async_file_open_api_for_testing(const AsyncFileOpenApi &api);
void set_async_file_host_api_for_testing(const AsyncFileHostApi &api);

// GAG.EXE: 0x00408380
ScriptObjectState *find_script_object_by_name(const char *name);

// GAG.EXE: 0x00408660
ScriptObjectState *resolve_state_field_reference(const char *object_name, const char *field_name, const void *value, int value_type);

void set_script_runtime_root_for_testing(ScriptRuntimeRoot *root);
void use_embedded_script_runtime_root_for_testing();
ScriptRuntimeRoot *get_embedded_script_runtime_root_for_testing();

// GAG.EXE: 0x0040D030
void copy_file_name_from_path(char *destination, const char *source);

// GAG.EXE: 0x0040CFD0
int append_string(char *destination, const char *source);

struct DisplayMode
{
    uint32_t flags;
    uint32_t unknown_0004;
    uint32_t unknown_0008;
    uint32_t surface_caps;
    uint32_t unknown_0010;
    uint32_t pixel_value_count;
    int32_t width;
    int32_t height;
    uint32_t pixel_format_flags;
    uint32_t unknown_0024;
    int32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t alpha_mask;
    DisplayMode *next;
};


using DirectDrawCreateProcedure = HRESULT(WINAPI *)(GUID *driver, void **display, void *outer);
using DirectDrawModeCallback = HRESULT(WINAPI *)(LegacyDirectDrawSurfaceDescriptor *descriptor, void *context);

struct DisplayBootstrapApi
{
    BOOL(WINAPI *get_version)(LPOSVERSIONINFOA information);
    HMODULE(WINAPI *load_library)(LPCSTR name);
    FARPROC(WINAPI *get_proc_address)(HMODULE module, LPCSTR name);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    uint32_t (*set_cooperative_mode)(uint32_t mode);
    HRESULT(WINAPI *enumerate_modes)(void *display, DirectDrawModeCallback callback);
};

// GAG.EXE: 0x00412F40
uint32_t initialize_direct_draw_runtime();

// GAG.EXE: 0x00412DB0
HRESULT WINAPI collect_direct_draw_display_mode(LegacyDirectDrawSurfaceDescriptor *descriptor, void *context);

// GAG.EXE: 0x00412FE0
uint32_t enumerate_direct_draw_display_modes();

void set_display_bootstrap_api_for_testing(const DisplayBootstrapApi &api);
void set_display_bootstrap_state_for_testing(uint32_t flags, DisplayMode *head, DisplayMode *tail, uint32_t count, void *display);
uint32_t get_display_bootstrap_error_for_testing();
uint32_t get_display_mode_count_for_testing();
DisplayMode *get_display_mode_tail_for_testing();

struct DisplayHostInitializationApi
{
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    uint32_t (*enumerate_windows_modes)();
    uint32_t (*initialize_direct_draw)();
    uint32_t (*enumerate_direct_draw_modes)();
    DisplayMode *(*find_current_mode)();
};

// GAG.EXE: 0x00413380
uint32_t initialize_display_mode_host(HWND window, uint32_t options);

void set_display_host_initialization_api_for_testing(const DisplayHostInitializationApi &api);
void set_display_host_initialization_state_for_testing(uint32_t flags, HWND window);
HWND get_display_host_window_for_testing();

struct WindowsDisplayEnumerationApi
{
    HDC(WINAPI *get_dc)(HWND window);
    BOOL(WINAPI *enum_display_settings)(LPCSTR device, DWORD mode, LPDEVMODEA settings);
    LONG(WINAPI *change_display_settings)(LPDEVMODEA settings, DWORD flags);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    HBITMAP(WINAPI *create_dib_section)(HDC dc, const BITMAPINFO *info, UINT usage, VOID **bits, HANDLE section, DWORD offset);
    BOOL(WINAPI *delete_object)(HGDIOBJ object);
    int(WINAPI *release_dc)(HWND window, HDC dc);
};

// GAG.EXE: 0x00413030
uint32_t enumerate_windows_display_modes();

void set_windows_display_enumeration_api_for_testing(const WindowsDisplayEnumerationApi &api);

// GAG.EXE: 0x00413650
DisplayMode *begin_display_mode_enumeration(uint32_t mask);

// GAG.EXE: 0x004136A0
DisplayMode *get_next_display_mode(uint32_t mask);

// GAG.EXE: 0x004136F0
DisplayMode *find_current_display_mode();

#if defined(GAG_TESTING) && defined(FREEGAG_WINDOWS_FIXES)
void build_modern_windows_virtual_display_mode_for_testing(DisplayMode *mode, int32_t width, int32_t height, bool indexed);
int32_t get_modern_windows_color_depth_for_testing();
#endif

// GAG.EXE: 0x0041F960
DisplayMode *get_current_display_mode();

// GAG.EXE: 0x0041F980
DisplayMode *begin_available_display_modes(uint32_t mask);

// GAG.EXE: 0x0041F9A0
DisplayMode *get_next_available_display_mode(uint32_t mask);

// GAG.EXE: 0x0041EFA0
uint32_t detect_alternate_display_mode(ApplicationState *state);

void set_display_mode_list_for_testing(DisplayMode *head);
void set_graphics_host_flags_for_testing(uint32_t flags);

struct DisplaySwitchApi
{
    uint32_t (*select_mode)(DisplayMode *mode);
    uint32_t (*restore_current_mode)();
};

// GAG.EXE: 0x0041D010
void switch_display_mode_if_enabled(ApplicationState *state, int restore_current);

// GAG.EXE: 0x00420BC0
void enable_runtime_subsystem();

// GAG.EXE: 0x00420BE0
void disable_runtime_subsystem();

// GAG.EXE: 0x00404980
void set_active_object_field_0824(uint32_t value);

void set_display_switch_api_for_testing(const DisplaySwitchApi &api);
uint32_t get_graphics_host_flags_for_testing();

struct DriveDiscoveryApi
{
    DWORD(WINAPI *get_logical_drive_strings)(DWORD buffer_length, LPSTR buffer);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    UINT(WINAPI *get_drive_type)(LPCSTR root_path);
    HANDLE(WINAPI *find_first_file)(LPCSTR pattern, LPWIN32_FIND_DATAA find_data);
    BOOL(WINAPI *find_next_file)(HANDLE find, LPWIN32_FIND_DATAA find_data);
    BOOL(WINAPI *find_close)(HANDLE find);
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    int (*read_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    uint32_t (*close_archive)(CdfArchive *archive);
    int(WINAPI *compare_case_insensitive)(LPCSTR left, LPCSTR right);
};

// GAG.EXE: 0x0041EBD0
void locate_game_data_drive(ApplicationState *state, const char *requested_archive);

void set_drive_discovery_api_for_testing(const DriveDiscoveryApi &api);

// GAG.EXE: 0x00420C00
void set_runtime_flag_01000000();

// GAG.EXE: 0x00420C10
void clear_runtime_flag_01000000();

// GAG.EXE: 0x00420CD0
void clear_runtime_command_state();

// GAG.EXE: 0x00420C90
void set_credits_runtime_flag();

// GAG.EXE: 0x00424260
void enter_runtime_state_1000();

// GAG.EXE: 0x00424290
void leave_runtime_state_1000();

void set_runtime_state_transition_for_testing(uintptr_t current_value, uintptr_t saved_value, void (*callback)(uintptr_t value));
uintptr_t get_runtime_state_value_for_testing();

struct RuntimePathApi
{
    void (*enter_lock)();
    void (*leave_lock)();
};

// GAG.EXE: 0x00420C30
void set_runtime_paths_once(const char *first_path, const char *second_path);

void set_runtime_path_api_for_testing(const RuntimePathApi &api);
const char *get_first_runtime_path_for_testing();
const char *get_second_runtime_path_for_testing();

struct ScreenshotApi
{
    BOOL(WINAPI *get_save_file_name)(LPOPENFILENAMEA file_name);
    void *(*capture_bitmap)(void *snapshot_context, uint32_t *size, int mode);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD size, LPDWORD written, LPOVERLAPPED overlapped);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};

// GAG.EXE: 0x0041CBE0
void save_game_screenshot(void *snapshot_context, void *game_context);

void set_screenshot_api_for_testing(const ScreenshotApi &api);

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

// GAG.EXE: 0x00417790
void *create_indexed_bitmap(const BitmapCaptureSource *source, const uint8_t *palette, uint32_t *size, int half_resolution);
void *create_display_bitmap(const DisplayBitmapCaptureSource *source, uint32_t *size, int half_resolution);

// GAG.EXE: 0x0041F8B0
void *capture_bitmap_if_runtime_active(const BitmapCaptureSource *source, const uint8_t *palette, uint32_t *size, int half_resolution);

// GAG.EXE: 0x0041CB90
void *capture_game_bitmap(void *game_context, uint32_t *size, int half_resolution);

void set_bitmap_capture_api_for_testing(const BitmapCaptureApi &api);
#if defined(GAG_TESTING)
void set_runtime_display_scene_for_bitmap_capture_testing(DisplaySceneNode *scene);
#endif

struct RuntimeQueueApi
{
    void (*enter_pair_lock)();
    void (*leave_pair_lock)();
    void (*enter_queue_lock)();
    void (*leave_queue_lock)();
    void (*enter_byte_lock)();
    void (*leave_byte_lock)();
};

// GAG.EXE: 0x00420A50
void reset_runtime_pair_queue();

// GAG.EXE: 0x00420640
void enqueue_runtime_byte(uint8_t value);

// GAG.EXE: 0x004206D0
uint8_t dequeue_runtime_byte();

// GAG.EXE: 0x00420750
void reset_runtime_byte_queue();

struct RuntimeMessagePair
{
    uint32_t first;
    uint32_t second;
};

// GAG.EXE: 0x00420910
void enqueue_runtime_pair(uint32_t first, uint32_t second);

// GAG.EXE: 0x004209B0
int dequeue_runtime_pair(RuntimeMessagePair *pair);

struct RuntimePlanModeSyncApi
{
    bool (*set_inactive)();
    bool (*clear_inactive)();
    void (*rebuild)();
};

// GAG.EXE: 0x00421130
bool synchronize_runtime_plan_mode();

struct RuntimePendingTreeSwitchApi
{
    void (*destroy_resources)(void *identity);
    RuntimeTreeNode *(*activate_tree)(const char *first, const char *second, void *third, void *fourth);
    void (*finalize_current_tree)(void *identity);
    void (*rebuild_runtime_plans)(void *identity);
    uint32_t (*update_pointer)(int32_t x, int32_t y);
};

// GAG.EXE: 0x004210A0
bool process_pending_runtime_tree_switch(RuntimeTreeNode *node);

void set_runtime_pending_tree_switch_api_for_testing(const RuntimePendingTreeSwitchApi &api);

struct RuntimeTreeActivationApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    RuntimeGenericResourceNode *(*find_or_load_resource)(const char *name);
    RuntimeTreeNode *(*create_tree_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *name, void *creation_context);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void (*activate_comment)(RuntimeTreeNode *node);
};

// GAG.EXE: 0x00426560
RuntimeTreeNode *activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *parent_selector, void *creation_context);

void set_runtime_tree_activation_api_for_testing(const RuntimeTreeActivationApi &api);

struct RuntimePairDispatchApi
{
    int (*dequeue_pair)(RuntimeMessagePair *pair);
    uint32_t (*move_pointer)(int32_t x, int32_t y);
    uint32_t (*left_button_down)();
    uint32_t (*left_button_up)();
    uint32_t (*right_button_down)();
};

// GAG.EXE: 0x004211A0
uint32_t process_runtime_pair_message();

void set_runtime_plan_mode_sync_api_for_testing(const RuntimePlanModeSyncApi &api);
void set_runtime_pair_dispatch_api_for_testing(const RuntimePairDispatchApi &api);
RuntimePairDispatchApi get_runtime_pair_dispatch_api_for_testing();

struct RuntimeInputSessionRecord
{
    uint32_t values[8];
};


// GAG.EXE: 0x004208E0
uint32_t copy_runtime_input_session_record(RuntimeInputSessionRecord *record);

struct RuntimeInputSessionApi
{
    void (*reset_byte_queue)();
    DWORD(WINAPI *get_time)();
    RuntimeLockRecord *(*acquire_record)(void *selector);
    uint32_t (*initialize_text)(const char *text, uint32_t value_0014, uint32_t value_0018, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state);
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

#if defined(FREEGAG_WINDOWS_FIXES)
struct RuntimeTextInputSceneRedrawApi
{
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_update)(intptr_t identifier);
    uint32_t (*end_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
};

DisplaySceneNode *acquire_runtime_text_input_scene(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format);
uint32_t begin_runtime_text_input_scene_update(intptr_t identifier);
uint32_t end_runtime_text_input_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
#endif

// GAG.EXE: 0x00420790
void initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, uint32_t character_width, void *session_value);

// GAG.EXE: 0x00420A90
void enqueue_runtime_message(uint32_t message);

// GAG.EXE: 0x00420B50
uint32_t dequeue_runtime_message();

// GAG.EXE: 0x00420CB0
void clear_credits_runtime_flag();

void set_runtime_queue_api_for_testing(const RuntimeQueueApi &api);
void set_runtime_pair_indices_for_testing(uint32_t read_index, uint32_t write_index);
void get_runtime_pair_indices_for_testing(uint32_t *read_index, uint32_t *write_index);
void reset_runtime_message_queue_for_testing();
void reset_runtime_byte_queue_for_testing();
void reset_runtime_pair_queue_for_testing();
void set_runtime_input_session_record_for_testing(const RuntimeInputSessionRecord &record, uint32_t status);
void set_runtime_input_session_api_for_testing(const RuntimeInputSessionApi &api);
void set_runtime_input_alternate_scene_for_testing(intptr_t identifier);
#if defined(FREEGAG_WINDOWS_FIXES)
void set_runtime_text_input_scene_redraw_api_for_testing(const RuntimeTextInputSceneRedrawApi &api);
#endif

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
    RuntimePresentationTarget command_target;
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
    DWORD(WINAPI *time_get_time)();
    uint32_t (*initialize_text)(const char *text, uint32_t value_0014, uint32_t value_0018, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    uint32_t (*begin_update)(intptr_t identifier);
    void (*draw_text)(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);
    uint32_t (*end_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
};

// GAG.EXE: 0x00420E10
void process_runtime_text_input(RuntimeCommandLoopState *state);

void set_runtime_text_input_api_for_testing(const RuntimeTextInputApi &api);
RuntimeCommandLoopState *get_runtime_command_loop_state_for_testing();

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

// GAG.EXE: 0x00421230
void process_runtime_message(RuntimeCommandLoopState *state);

void set_runtime_message_processor_api_for_testing(const RuntimeMessageProcessorApi &api);

struct DisplayPaletteApi
{
    void (*enter_lock)();
    void (*leave_lock)();
    HWND (*get_host_window)();
    HWND(WINAPI *get_foreground_window)();
    DWORD(WINAPI *get_window_thread_process_id)(HWND window, LPDWORD process_id);
    BOOL(WINAPI *unrealize_object)(HGDIOBJ object);
    HPALETTE(WINAPI *select_palette)(HDC dc, HPALETTE palette, BOOL force_background);
    BOOL(WINAPI *animate_palette)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *realize_palette)(HDC dc);
    UINT(WINAPI *set_dib_color_table)(HDC dc, UINT start, UINT count, const RGBQUAD *colors);
    void (*present)(int32_t width, int32_t height, int enabled);
};

struct DisplayPaletteTeardownApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    HPALETTE(WINAPI *select_palette)(HDC dc, HPALETTE palette, BOOL force_background);
    HGDIOBJ(WINAPI *select_object)(HDC dc, HGDIOBJ object);
    BOOL(WINAPI *delete_object)(HGDIOBJ object);
    BOOL(WINAPI *delete_dc)(HDC dc);
    int(WINAPI *release_dc)(HWND window, HDC dc);
};

struct DisplayCooperativeLevelApi
{
    LONG(WINAPI *get_window_long)(HWND window, int index);
    HWND(WINAPI *get_parent)(HWND window);
    HRESULT(WINAPI *set_cooperative_level)(void *display, HWND window, DWORD flags);
};

struct DisplayModeChangeApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*set_cooperative_mode)(uint32_t mode);
    HRESULT(WINAPI *set_direct_draw_mode)(void *display, DWORD width, DWORD height, DWORD bits_per_pixel);
    HRESULT(WINAPI *restore_direct_draw_mode)(void *display);
    LONG(WINAPI *change_display_settings)(LPDEVMODEA settings, DWORD flags);
    DisplayMode *(*find_current_mode)();
};

struct DisplayModeHostShutdownApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*teardown_palette_surface)();
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x004134D0
void shutdown_display_mode_host();

void set_display_mode_host_shutdown_api_for_testing(const DisplayModeHostShutdownApi &api);

// GAG.EXE: 0x00413780
uint32_t set_active_display_mode(DisplayMode *mode);

// GAG.EXE: 0x004138D0
uint32_t restore_active_display_mode();

// GAG.EXE: 0x0041F9C0
uint32_t set_active_display_mode_if_graphics_ready(DisplayMode *mode);

// GAG.EXE: 0x0041F9E0
uint32_t restore_active_display_mode_if_graphics_ready();

void set_display_mode_change_api_for_testing(const DisplayModeChangeApi &api);
void set_display_mode_change_state_for_testing(uint32_t flags, void *display, DisplayMode *current_mode);

struct DisplaySurfaceOperationApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    HRESULT(WINAPI *blt_fast)(void *surface, DWORD x, DWORD y, void *source, RECT *source_rectangle, DWORD flags);
    HRESULT(WINAPI *blt)(void *surface, RECT *destination_rectangle, void *source, RECT *source_rectangle, DWORD flags, void *effects);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
    BOOL(WINAPI *stretch_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, int source_width, int source_height, DWORD operation);
    BOOL(WINAPI *pat_blt)(HDC destination, int x, int y, int width, int height, DWORD operation);
};

#if defined(GAG_TESTING) && defined(FREEGAG_WINDOWS_FIXES)
RECT calculate_modern_windows_fullscreen_viewport_for_testing(int32_t monitor_width, int32_t monitor_height, int32_t framebuffer_width, int32_t framebuffer_height, int32_t scaling);
RECT calculate_modern_windows_windowed_viewport_for_testing(int32_t client_width, int32_t client_height, int32_t framebuffer_width, int32_t framebuffer_height, int32_t scaling);
int32_t map_modern_windows_fullscreen_coordinate_for_testing(int32_t value, int32_t destination_extent, int32_t source_extent);
void set_modern_windows_fullscreen_presentation_for_testing(bool fullscreen, int32_t viewport_width, int32_t viewport_height);
void reset_modern_windows_presentation_for_testing();
bool get_modern_windows_windowed_rectangle_for_testing(RECT *rectangle);
#endif

struct LegacyDisplayPixelFormat
{
    uint32_t flags;
    uint32_t reserved;
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
};


struct LegacyDirectDrawPixelFormat
{
    uint32_t size;
    uint32_t flags;
    uint32_t four_cc;
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t alpha_mask;
};

struct LegacyDirectDrawSurfaceDescriptor
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitch;
    uint32_t back_buffer_count;
    uint32_t mip_map_count;
    uint32_t alpha_bit_depth;
    uint32_t reserved;
    void *surface;
    uint32_t color_keys[8];
    LegacyDirectDrawPixelFormat pixel_format;
    uint32_t caps;
};


struct DisplaySurfaceCreationApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*teardown)();
    uint32_t (*set_cooperative_mode)(uint32_t mode);
    HRESULT(WINAPI *create_direct_draw_surface)(void *display, LegacyDirectDrawSurfaceDescriptor *descriptor, void **surface, void *outer);
    HRESULT(WINAPI *get_attached_surface)(void *surface, uint32_t *caps, void **attached_surface);
    ULONG(WINAPI *release_surface)(void *surface);
    HDC(WINAPI *get_dc)(HWND window);
    HDC(WINAPI *create_compatible_dc)(HDC dc);
    int(WINAPI *release_dc)(HWND window, HDC dc);
    BOOL(WINAPI *delete_dc)(HDC dc);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    HPALETTE(WINAPI *create_palette)(const LOGPALETTE *palette);
    HPALETTE(WINAPI *select_palette)(HDC dc, HPALETTE palette, BOOL force_background);
    BOOL(WINAPI *delete_object)(HGDIOBJ object);
    HBITMAP(WINAPI *create_dib_section)(HDC dc, const BITMAPINFO *info, UINT usage, VOID **pixels, HANDLE section, DWORD offset);
    HGDIOBJ(WINAPI *select_object)(HDC dc, HGDIOBJ object);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *realize_palette)(HDC dc);
    UINT(WINAPI *set_dib_color_table)(HDC dc, UINT start, UINT count, const RGBQUAD *colors);
    int(WINAPI *set_stretch_blt_mode)(HDC dc, int mode);
};

// GAG.EXE: 0x00413340
HWND find_top_level_display_window(HWND window);

// GAG.EXE: 0x00413590
uint32_t set_display_cooperative_mode(uint32_t mode);

// GAG.EXE: 0x004140B0
void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);

// GAG.EXE: 0x004139B0
void *create_display_surface(int32_t width, int32_t height, const LegacyDisplayPixelFormat *format, uint32_t options);

// GAG.EXE: 0x00413F80
void teardown_display_palette_surface();

// GAG.EXE: 0x00414610
UINT apply_display_palette(const PALETTEENTRY *palette, uint32_t update_flags);

// GAG.EXE: 0x00414590
void enable_display_palette_mode();

// GAG.EXE: 0x004145D0
void disable_display_palette_mode();

void set_display_palette_api_for_testing(const DisplayPaletteApi &api);
void set_display_palette_teardown_api_for_testing(const DisplayPaletteTeardownApi &api);
void set_display_cooperative_level_api_for_testing(const DisplayCooperativeLevelApi &api);
void set_display_surface_operation_api_for_testing(const DisplaySurfaceOperationApi &api);
void set_display_surface_creation_api_for_testing(const DisplaySurfaceCreationApi &api);
void set_display_palette_state_for_testing(uint32_t flags, int32_t display_bits_per_pixel, int32_t surface_bits_per_pixel, HDC palette_dc, HDC dib_dc, HPALETTE palette, int32_t width, int32_t height);
void set_display_palette_bitmap_for_testing(HBITMAP bitmap);
void set_display_palette_teardown_state_for_testing(HWND window, HPALETTE previous_palette, HBITMAP previous_bitmap);
void set_display_cooperative_state_for_testing(HWND window, void *display);
void set_display_surface_operation_state_for_testing(void *primary_surface, void *secondary_surface);
uint32_t get_display_palette_flags_for_testing();
const PALETTEENTRY *get_display_palette_entries_for_testing();

struct DisplayTargetApi
{
    void (*release_backend_target)(void *backend, void *target);
};

// GAG.EXE: 0x00414540
uint32_t end_display_target();

void set_display_target_api_for_testing(const DisplayTargetApi &api);
void set_display_target_state_for_testing(void *backend, void *target);

struct RuntimeTargetUpdateApi
{
    void (*draw_bounds)(RuntimeCommandBounds *bounds, int mode);
    int (*begin_target)(uint32_t height, uint32_t second, uint32_t width);
    uint32_t (*end_target)();
};

// GAG.EXE: 0x004280D0
bool update_runtime_target(void *unused, RuntimeCommandBounds *bounds, int mode);

void set_runtime_target_update_api_for_testing(const RuntimeTargetUpdateApi &api);
void set_runtime_target_flags_for_testing(uint32_t flags);

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

// GAG.EXE: 0x004198E0
uint32_t acquire_display_lock(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);

// GAG.EXE: 0x00419AF0
uint32_t release_display_lock();

void set_display_lock_release_api_for_testing(const DisplayLockReleaseApi &api);
void set_display_lock_acquire_api_for_testing(const DisplayLockAcquireApi &api);
void set_display_lock_state_for_testing(uint32_t flags, DWORD owner_thread, uint32_t recursion_count, HANDLE release_event);
void get_display_lock_state_for_testing(uint32_t *flags, DWORD *owner_thread, uint32_t *recursion_count);

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
    uint32_t value_08;
    uint32_t value_0c;
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
    uint32_t unknown_2c;
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
    DWORD(WINAPI *time_get_time)();
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

// GAG.EXE: 0x00413480
HDC get_display_palette_dc();

// GAG.EXE: 0x00413490
HDC get_display_palette_dib_dc();

// GAG.EXE: 0x004134A0
HBITMAP get_display_palette_bitmap();

// GAG.EXE: 0x004134B0
HPALETTE get_display_palette_handle();

// GAG.EXE: 0x004134C0
PALETTEENTRY *get_display_palette_entries();

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
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *dirty_flags);
    int (*synchronize_node)(DisplaySceneNode *node, DisplayRectangle *rectangle);
    void (*publish_node)(DisplaySceneNode *node);
    uint32_t (*release_mode_1000)();
    uint32_t (*release_lock)();
};

// GAG.EXE: 0x0041B560
int process_scene_node_callbacks(DisplaySceneNode *node);

// GAG.EXE: 0x0041B690
bool clip_display_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041B640
bool constrain_display_rectangle_to_surface(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041B790
void trim_display_rectangle_overlap(DisplayRectangle *rectangle, DisplaySceneNode *node);

// GAG.EXE: 0x0041B860
void accumulate_scene_node_rectangle(DisplayRectangle *rectangle, DisplaySceneNode *node);

// GAG.EXE: 0x0041B6F0
void merge_display_rectangle(DisplayRectangle *destination, const DisplayRectangleTransform *transform, const DisplayRectangle *source);

// GAG.EXE: 0x004195B0
uint32_t queue_display_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041AC70
bool contains_display_scene_node(intptr_t identifier);

// GAG.EXE: 0x004190D0
int synchronize_display_scene_node(DisplaySceneNode *node, DisplayRectangle *output_rectangle);

// GAG.EXE: 0x00419230
void publish_display_scene_node(DisplaySceneNode *node);

// GAG.EXE: 0x00419710
uint32_t dispatch_display_scene_update(void *target, uint32_t options);

struct FramebufferInvalidateApi
{
    uint32_t (*acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    uint32_t (*dispatch_update)(void *target, uint32_t options);
    uint32_t (*release_lock)();
};

// GAG.EXE: 0x00427830
void invalidate_game_framebuffer_rect(int32_t x, int32_t y, int32_t width, int32_t height);

void set_framebuffer_invalidate_api_for_testing(const FramebufferInvalidateApi &api);

// GAG.EXE: 0x00419550
uint32_t find_available_display_scene_index(uint32_t candidate);

// GAG.EXE: 0x00419600
uint32_t wait_for_display_scene_ready(uint32_t timeout);

// GAG.EXE: 0x00419660
uint32_t set_display_clip_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x00419B60
uint32_t release_display_lock_mode_1000();

// GAG.EXE: 0x0041ACC0
DisplaySceneNode *lock_display_scene_node(intptr_t identifier);

// GAG.EXE: 0x0041AD50
void unlock_display_scene_node(intptr_t identifier);

// GAG.EXE: 0x0041ADC0
bool set_display_scene_primary_owner(intptr_t identifier, intptr_t owner, bool replace_existing);

// GAG.EXE: 0x0041AE60
intptr_t query_display_scene_by_index(int32_t index, DisplaySceneDescriptor *descriptor, DisplayPixelFormatDescriptor *callback_format);

// GAG.EXE: 0x0041AFA0
uint32_t blit_bitmap_with_optional_palette_remap(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, uint32_t flags);

// GAG.EXE: 0x0041AF20
uint32_t offset_display_scene_node(intptr_t identifier, int32_t x_delta, int32_t y_delta);

// GAG.EXE: 0x0041B280
uint32_t begin_display_scene_update(intptr_t identifier);

// Non-original compatibility service for ownerless script-declared layers.
bool activate_display_scene_node(intptr_t identifier);

// GAG.EXE: 0x0041B360
uint32_t end_display_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);

struct DisplayRootRegionApi
{
    uint32_t (*begin_scene_update)(intptr_t identifier);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
};

// GAG.EXE: 0x0041B1F0
uint32_t update_display_root_region(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);

void set_display_root_region_api_for_testing(const DisplayRootRegionApi &api);
void set_display_root_region_state_for_testing(uint32_t lock_flags, DisplaySceneNode *root);

struct ClearRuntimeDisplayApi
{
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    uint32_t (*release_display_lock)();
    uint32_t (*update_root_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);
};

// GAG.EXE: 0x00427880
void clear_runtime_display();

void set_clear_runtime_display_api_for_testing(const ClearRuntimeDisplayApi &api);
void set_clear_runtime_display_size_for_testing(uint16_t width, uint16_t height);

// GAG.EXE: 0x0041A830
uint32_t add_display_scene_callback(intptr_t identifier, int (*callback)(DisplayTraversalState *state), const void *context, uint32_t context_size, uint32_t flags);

// GAG.EXE: 0x0041B950
void fill_display_scene_rectangle_8(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

// GAG.EXE: 0x0041BE60
void fill_display_scene_rectangle_16(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

// GAG.EXE: 0x0041B9D0
void composite_transparent_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

// GAG.EXE: 0x0041BC40
void composite_opaque_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state, uint32_t mode);

// GAG.EXE: 0x0041BEE0
void composite_transparent_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

// GAG.EXE: 0x0041C180
void composite_opaque_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

// GAG.EXE: 0x0041C400
void composite_transparent_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

// GAG.EXE: 0x0041C660
void composite_opaque_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode);

// GAG.EXE: 0x0041A480
uint32_t release_display_scene_node(intptr_t identifier, intptr_t owner);

// GAG.EXE: 0x0041C8C0
void build_indexed_to_16_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state);

// GAG.EXE: 0x0041CA00
void build_indexed_to_indexed_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state);

// GAG.EXE: 0x00418EE0
void configure_display_scene_format(DisplaySceneNode *node, const DisplayPixelFormatDescriptor *format);

// GAG.EXE: 0x0041AA10
bool configure_display_scene_palette(DisplaySceneNode *node, const uint32_t *palette, uint32_t count);

// GAG.EXE: 0x00419BC0
DisplaySceneNode *acquire_display_scene_node(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format);

// GAG.EXE: 0x004192B0
uint32_t *initialize_display_scene_host(intptr_t primary_position, const DisplayPixelFormatDescriptor *format, int32_t width, int32_t height,
    int (*synchronize)(void *context, void *payload, uint32_t mode), void *context, uint32_t worker_interval);

// GAG.EXE: 0x004194B0
uint32_t shutdown_display_scene_host();

// GAG.EXE: 0x0041B3F0
DWORD WINAPI run_display_scene_worker(uint32_t *flags);

void set_display_clip_bounds_for_testing(const DisplayRectangle &bounds);
void set_display_scene_callback_api_for_testing(const DisplaySceneCallbackApi &api);
void set_display_scene_sync_api_for_testing(const DisplaySceneSyncApi &api);
void set_display_scene_memory_api_for_testing(const DisplaySceneMemoryApi &api);
void set_display_scene_host_api_for_testing(const DisplaySceneHostApi &api);
void set_display_scene_worker_api_for_testing(const DisplaySceneWorkerApi &api);
void set_display_scene_sync_state_for_testing(void *context, DisplaySceneNode *root_node);
void set_display_scene_worker_state_for_testing(uint32_t interval, DisplayPixelFormatDescriptor *palette_source_state);
uint32_t get_display_scene_worker_rate_for_testing();
void set_display_scene_root_primary_position_for_testing(intptr_t primary_position);
void set_display_lock_acquire_state_for_testing(HANDLE gate_event, uint32_t busy, const DisplayRectangle &pending_rectangle, int32_t width, int32_t height, DisplaySceneNode *scene_head);
DisplayRectangle get_display_pending_rectangle_for_testing();

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
    DWORD(WINAPI *get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x004263A0
void reset_runtime_session();

void set_runtime_session_reset_api_for_testing(const RuntimeSessionResetApi &api);
void set_runtime_session_reset_storage_for_testing(uint32_t value);
uint32_t get_runtime_session_reset_storage_for_testing(uint32_t index);
uintptr_t get_runtime_pointer_event_record_for_testing(uint32_t index);
void set_embedded_script_runtime_flags_for_testing(uint32_t flags, uint32_t palette_flags);

// GAG.EXE: 0x00420CE0
int run_runtime_command_loop(RuntimeCommandLoopState *state);

void set_runtime_command_loop_api_for_testing(const RuntimeCommandLoopApi &api);

struct RuntimeExternalCommandApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void (*process_message)(RuntimeCommandLoopState *state);
    int (*run_command_loop)(RuntimeCommandLoopState *state);
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x00421010
uint32_t run_pending_runtime_external_command();

void set_runtime_external_command_api_for_testing(const RuntimeExternalCommandApi &api);
void set_runtime_external_command_state_for_testing(const RuntimeCommandLoopState &state);
const RuntimeCommandLoopState &get_runtime_external_command_state_for_testing();

// GAG.EXE: 0x00421530
DWORD WINAPI execute_script_commands(LPVOID parameter);

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

// Non-original dispatcher slice used to compose and test GAG.EXE:0x00421530.
RuntimeScriptOpcodeDisposition execute_simple_runtime_script_opcode_for_testing(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, uint32_t opcode,
    int32_t random_value = 0, uint32_t saved_cursor = 0xffffffff);
bool should_send_runtime_script_message(int32_t command);

struct RuntimeScriptExecutorApi
{
    DWORD(WINAPI *set_batch_limit)(DWORD limit);
    DWORD(WINAPI *get_tick_count)();
    DWORD(WINAPI *time_get_time)();
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

void set_runtime_script_executor_api_for_testing(const RuntimeScriptExecutorApi &api);

// GAG.EXE: 0x0041CE40
void application_hook_no_op_1();

// GAG.EXE: 0x0041CE50
void application_hook_no_op_2();

// GAG.EXE: 0x0041CDC0
void set_application_lock_flag(ApplicationState *state);

// GAG.EXE: 0x0041CD30
void set_application_inactive_flags(ApplicationState *state);

struct CursorStateApi
{
    BOOL(WINAPI *get_cursor_position)(LPPOINT point);
    int(WINAPI *get_system_metrics)(int index);
};

// GAG.EXE: 0x0041CD50
void clear_runtime_active_flag(ApplicationState *state);

// GAG.EXE: 0x0041CDD0
void clear_application_lock_flag(ApplicationState *state);

// GAG.EXE: 0x00417970
void free_heap_memory(void *memory);

void set_cursor_state_api_for_testing(const CursorStateApi &api);

} // namespace gag
