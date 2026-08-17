#pragma once

#include <windows.h>
#include <commdlg.h>
#include <cstddef>
#include <cstdint>
#include <mmsystem.h>

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
struct DisplayPixelFormatDescriptor;
struct DisplaySceneDescriptor;
struct RuntimeGenericBackendChild;
struct DisplayRectangle;
struct DisplayRectangleTransform;
struct ScriptObjectState;

struct ApplicationState
{
    HINSTANCE instance;
    std::int32_t width;
    std::int32_t height;
    std::uint32_t validation_flags;
    void *archive_context;
    const char *message_table;
    HWND window;
    std::uint8_t unknown_001c[4];
    HWND capture_window;
    HMENU game_menu;
    HMENU options_menu;
    HMENU system_menu;
    std::uint32_t saved_flags;
    void *saved_memory;
    void *game_context;
    std::uint32_t script_state;
    std::int32_t content_left;
    std::int32_t content_top;
    std::int32_t content_right;
    std::int32_t content_bottom;
    char startup_config[0x104];
    char installed_version[0x104];
    char installation_path[0x104];
    char executable_directory[0x104];
    std::int32_t window_vertical_offset;
    std::int32_t window_top_adjustment;
    RECT desktop_window_rect;
    std::uint32_t flags;
    std::int32_t display_bits_per_pixel;
    std::int32_t display_width;
    std::int32_t display_height;
    DisplayMode *display_mode_iterator;
};

static_assert(sizeof(ApplicationState) == 0x48c);
static_assert(offsetof(ApplicationState, message_table) == 0x14);
static_assert(offsetof(ApplicationState, installed_version) == 0x154);
static_assert(offsetof(ApplicationState, window) == 0x18);
static_assert(offsetof(ApplicationState, game_menu) == 0x24);
static_assert(offsetof(ApplicationState, game_context) == 0x38);
static_assert(offsetof(ApplicationState, saved_flags) == 0x30);
static_assert(offsetof(ApplicationState, script_state) == 0x3c);
static_assert(offsetof(ApplicationState, capture_window) == 0x20);
static_assert(offsetof(ApplicationState, content_left) == 0x40);
static_assert(offsetof(ApplicationState, startup_config) == 0x50);
static_assert(offsetof(ApplicationState, installation_path) == 0x258);
static_assert(offsetof(ApplicationState, executable_directory) == 0x35c);
static_assert(offsetof(ApplicationState, flags) == 0x478);
static_assert(offsetof(ApplicationState, window_vertical_offset) == 0x460);
static_assert(offsetof(ApplicationState, desktop_window_rect) == 0x468);
static_assert(offsetof(ApplicationState, display_mode_iterator) == 0x488);
static_assert(offsetof(ApplicationState, display_bits_per_pixel) == 0x47c);

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
    std::uint32_t(__fastcall *load_registry)(ApplicationState *state);
    void(__fastcall *locate_drive)(ApplicationState *state, const char *requested_archive);
    std::uint32_t(__fastcall *measure_read_speed)(const char *archive_path, std::uint32_t bytes_to_measure);
    std::uint32_t(__fastcall *detect_alternate_mode)(ApplicationState *state);
};

// GAG.EXE: 0x0041F040
int __fastcall validate_startup_environment(ApplicationState *state, const char *requested_archive, std::uint32_t stages);

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
void __fastcall initialize_application_state_no_op(ApplicationState *state);

// GAG.EXE: 0x0041F3D0
bool __fastcall register_gag_window_classes(ApplicationState *state);

// GAG.EXE: 0x004174B0
std::uint32_t __fastcall register_custom_control_class(HINSTANCE instance);

void set_window_class_api_for_testing(const WindowClassApi &api);
void reset_custom_control_registration_for_testing();
void set_custom_control_registration_state_for_testing(std::uint32_t registered, HINSTANCE instance);

struct GraphicsHostInitializationResult
{
    std::uint32_t unknown_0000;
    HWND capture_window;
    std::uint8_t unknown_0008[0x458];
    std::uint32_t bits_per_pixel;
};

static_assert(sizeof(GraphicsHostInitializationResult) == 0x464);
static_assert(offsetof(GraphicsHostInitializationResult, capture_window) == 0x04);
static_assert(offsetof(GraphicsHostInitializationResult, bits_per_pixel) == 0x460);

struct RuntimeNamedNode;

struct GraphicsHostApi
{
    DWORD(WINAPI *gdi_set_batch_limit)(DWORD limit);
    std::uint32_t(__fastcall *initialize_media)(HINSTANCE instance);
    std::uint32_t(__fastcall *register_control)(HINSTANCE instance);
    std::uint32_t (*initialize_async)();
    std::uint32_t (*initialize_generic)();
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    HCURSOR(WINAPI *load_cursor)(HINSTANCE instance, LPCSTR name);
    ATOM(WINAPI *register_class)(const WNDCLASSA *window_class);
    HWND(WINAPI *create_window_ex)
    (DWORD extended_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter);
    BOOL(WINAPI *get_cursor_position)(LPPOINT point);
    BOOL(WINAPI *screen_to_client)(HWND window, LPPOINT point);
    std::uint32_t(__fastcall *initialize_display)(HWND window, std::uint32_t options);
    void(__fastcall *set_script_root)(ScriptRuntimeRoot *root);
    RuntimeNamedNode *(__fastcall *get_or_create_named_node)(const char *name);
    void(__fastcall *set_named_node_enabled)(void *identity, int enabled);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *show_window)(HWND window, int command);
};

// GAG.EXE: 0x0041FA00
GraphicsHostInitializationResult *__fastcall initialize_graphics_host(HINSTANCE instance, HWND parent, int x, int y, int width, int height, std::uint32_t flags);

struct GraphicsHostShutdownApi
{
    std::uint32_t (*shutdown_display)();
    std::uint32_t (*shutdown_generic_backend)();
    std::uint32_t (*shutdown_async_files)();
    std::uint32_t (*shutdown_media_backend)();
    void (*shutdown_display_modes)();
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *heap_destroy)(HANDLE heap);
    BOOL(WINAPI *destroy_window)(HWND window);
};

// GAG.EXE: 0x00420230
std::uint32_t shutdown_graphics_host();

void set_graphics_host_shutdown_api_for_testing(const GraphicsHostShutdownApi &api);
void set_graphics_host_shutdown_state_for_testing(HANDLE heap, HWND window);

void set_graphics_host_api_for_testing(const GraphicsHostApi &api);
void reset_graphics_host_state_for_testing(std::uint32_t scene_flags);
void get_graphics_host_observed_state_for_testing(RuntimeGameHostContext *context, void **callbacks, std::int32_t *pointer_x, std::int32_t *pointer_y, std::uint32_t *target_flags,
    HANDLE *resource_heap);
HWND get_runtime_display_window_for_testing();

struct RuntimeBootstrapApi
{
    DisplayMode *(*find_current_mode)();
    void *(__fastcall *create_surface)(std::int32_t width, std::int32_t height, const LegacyDisplayPixelFormat *format, std::uint32_t options);
    HDC (*get_palette_dc)();
    HDC (*get_palette_dib_dc)();
    HPALETTE (*get_palette_handle)();
    HBITMAP (*get_palette_bitmap)();
    PALETTEENTRY *(*get_palette_entries)();
    std::uint32_t *(__fastcall *initialize_scene_host)(std::int32_t primary_position, const DisplayPixelFormatDescriptor *format, std::int32_t width, std::int32_t height,
        int(__fastcall *synchronize)(void *context, void *payload, std::uint32_t mode), void *context, std::uint32_t worker_interval);
    DisplaySceneNode *(__fastcall *acquire_scene_node)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    DisplaySceneNode *(__fastcall *lock_scene_node)(std::int32_t identifier);
    void(__fastcall *unlock_scene_node)(std::int32_t identifier);
    std::uint32_t(__fastcall *acquire_display_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *rectangle_flags);
    std::uint32_t(__fastcall *set_clip_rectangle)(DisplayRectangle *rectangle);
    std::uint32_t (*release_display_lock)();
    void(__fastcall *operate_surface)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);
    void (*reset_display_state)();
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    LPTHREAD_START_ROUTINE script_thread_entry;
};

// GAG.EXE: 0x0041FEA0
GraphicsHostInitializationResult *__fastcall initialize_runtime_graphics(const LegacyDisplayPixelFormat *format);

void set_runtime_bootstrap_api_for_testing(const RuntimeBootstrapApi &api);
void get_runtime_bootstrap_state_for_testing(DisplayPixelFormatDescriptor *format, std::int32_t *scene_identifier, HANDLE *thread);

struct RuntimeScriptPropertySetApi
{
    void (*select_resource)(char *path, std::int32_t loop_animation);
    BOOL(__fastcall *release_memory_resource)(const char *path);
    void(__fastcall *set_property_value)(std::uint32_t value);
    void (*enter_state_1000)();
    void (*leave_state_1000)();
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(__fastcall *destroy_resource_tree)(void *root);
};

// GAG.EXE: 0x004202D0
void __fastcall set_runtime_script_property(std::uint32_t property, void *context, void *value);

void set_runtime_script_property_api_for_testing(const RuntimeScriptPropertySetApi &api);
void reset_runtime_script_property_state_for_testing(std::uint32_t value_1, std::uint32_t value_2, std::uint32_t value_3, std::uint32_t state_1000_count, std::uint32_t state_4_count);
void get_runtime_script_property_state_for_testing(std::uint32_t *value_1, std::uint32_t *value_2, std::uint32_t *value_3, std::uint32_t *state_1000_count, std::uint32_t *state_4_count,
    std::uint32_t *scene_flags, std::int32_t *host_mode);

struct RuntimeScriptPropertyGetApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    int(__fastcall *copy_string)(char *destination, const char *source);
    void(__fastcall *load_resource)(const char *path, void **data, std::uint32_t *size, std::int32_t *storage, std::uint32_t flags);
    std::uint32_t (*get_property_value)();
    std::uint16_t(__fastcall *query_frame_number)(void *identity);
};

// GAG.EXE: 0x004204B0
void __fastcall get_runtime_script_property(std::uint32_t property, void **value, void *result);

void set_runtime_script_property_get_api_for_testing(const RuntimeScriptPropertyGetApi &api);
void set_runtime_script_property_get_state_for_testing(const char *path, std::int32_t pointer_x, std::int32_t pointer_y);

struct ApplicationInitializationApi
{
    UINT(WINAPI *set_error_mode)(UINT mode);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    void(__fastcall *initialize_state)(ApplicationState *state);
    bool(__fastcall *register_window_classes)(ApplicationState *state);
    std::uint32_t(__fastcall *register_control_class)(HINSTANCE instance);
    int(__fastcall *copy_string)(char *destination, const char *source);
    int(__fastcall *validate_environment)(ApplicationState *state, const char *requested_archive, std::uint32_t stages);
    int(WINAPI *get_system_metrics)(int index);
    BOOL(WINAPI *adjust_window_rect)(LPRECT rectangle, DWORD style, BOOL menu);
    HWND(WINAPI *create_window_ex)
    (DWORD extended_style, LPCSTR class_name, LPCSTR window_name, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter);
    BOOL(WINAPI *show_window)(HWND window, int command);
    BOOL(WINAPI *set_window_position)(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags);
    BOOL(WINAPI *get_client_rect)(HWND window, LPRECT rectangle);
    GraphicsHostInitializationResult *(__fastcall *initialize_graphics_host)(HINSTANCE instance, HWND window, int x, int y, int width, int height, std::uint32_t flags);
    void(__fastcall *switch_display_mode)(ApplicationState *state, int restore_current);
    GraphicsHostInitializationResult *(__fastcall *initialize_runtime)(const LegacyDisplayPixelFormat *format);
    void(__fastcall *update_window_layout)(ApplicationState *state, SecondaryWindowLayout *secondary_layout);
    void (*enable_runtime)();
    void(__fastcall *set_active_object_field)(std::uint32_t value);
    std::uint32_t(__fastcall *detect_resource_type)(const char *data);
};

// GAG.EXE: 0x0041F4F0
ApplicationState *__fastcall initialize_gag_application(int width, int height, HINSTANCE instance, LPSTR command_line, int show_command);

void set_application_initialization_api_for_testing(const ApplicationInitializationApi &api);

struct RuntimeBackendInitializationApi
{
    HANDLE(WINAPI *heap_create)(DWORD options, SIZE_T initial_size, SIZE_T maximum_size);
    HANDLE(WINAPI *create_mutex)(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name);
    void(__fastcall *initialize_sound_class)(HINSTANCE instance);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x00429DF0
std::uint32_t __fastcall initialize_runtime_media_backend(HINSTANCE instance);

// GAG.EXE: 0x00410B70
std::uint32_t initialize_runtime_generic_backend();

struct RuntimeGenericBackendShutdownApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    std::uint32_t(__fastcall *destroy_backend)(void *identity);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};

// GAG.EXE: 0x00410BD0
std::uint32_t shutdown_runtime_generic_backend();

void set_runtime_generic_backend_shutdown_api_for_testing(const RuntimeGenericBackendShutdownApi &api);

// GAG.EXE: 0x00414E10
std::uint32_t initialize_async_file_subsystem();

void set_runtime_backend_initialization_api_for_testing(const RuntimeBackendInitializationApi &api);
void set_runtime_backend_initialization_state_for_testing(bool media_initialized, bool generic_initialized, bool async_initialized);
HANDLE get_runtime_media_backend_heap_for_testing();
HANDLE get_runtime_media_backend_mutex_for_testing();
HANDLE get_runtime_generic_backend_mutex_for_testing();

// GAG.EXE: 0x00404970
void __fastcall set_script_runtime_root_if_valid(ScriptRuntimeRoot *root);

// GAG.EXE: 0x00407EA0
void __fastcall set_runtime_named_node_enabled(void *identity, int enabled);

struct WindowProcedureApi
{
    LONG(WINAPI *get_window_long)(HWND window, int index);
    LONG(WINAPI *set_window_long)(HWND window, int index, LONG value);
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
    void(__fastcall *update_cursor_state)(ApplicationState *state, int active);
};

struct MainWindowProcedureApi
{
    LONG(WINAPI *get_window_long)(HWND window, int index);
    LONG(WINAPI *set_window_long)(HWND window, int index, LONG value);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(WINAPI *post_quit_message)(int exit_code);
    BOOL(WINAPI *reply_message)(LRESULT result);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *destroy_window)(HWND window);
    std::uint32_t (*get_script_state)();
    ScriptObjectState *(__fastcall *resolve_state_field)(const char *object_name, const char *field_name, const void *value, int value_type);
    void *(__fastcall *capture_bitmap)(void *game_context, std::uint32_t *size, int half_resolution);
    void(__fastcall *free_memory)(void *memory);
    void (*application_hook_1)();
    void(__fastcall *set_application_lock)(ApplicationState *state);
    void(__fastcall *clear_runtime_active)(ApplicationState *state);
    int(__fastcall *validate_startup)(ApplicationState *state, const char *requested_archive, std::uint32_t stages);
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
    std::uint8_t unknown_0000[0x48];
    HDC destination_context;
    HDC source_context;
    HPALETTE palette;
    HPALETTE previous_palette;
    HBITMAP bitmap;
    HGDIOBJ previous_bitmap;
    RECT client_rect;
    std::int32_t bits_per_pixel;
    std::int32_t source_width;
    std::int32_t source_height;
    const void *bitmap_identity;
    char *archive_path;
    char *comment_text;
};

static_assert(offsetof(CustomControlState, destination_context) == 0x48);
static_assert(offsetof(CustomControlState, palette) == 0x50);
static_assert(offsetof(CustomControlState, client_rect) == 0x60);
static_assert(offsetof(CustomControlState, bits_per_pixel) == 0x70);
static_assert(offsetof(CustomControlState, source_width) == 0x74);
static_assert(offsetof(CustomControlState, bitmap_identity) == 0x7c);
static_assert(offsetof(CustomControlState, archive_path) == 0x80);
static_assert(offsetof(CustomControlState, comment_text) == 0x84);
static_assert(sizeof(CustomControlState) == 0x88);

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
void __fastcall initialize_custom_control_gdi(HWND window, CustomControlState *state);

// GAG.EXE: 0x00417B60
void __fastcall set_custom_control_bitmap(CustomControlState *state, BITMAPINFO *bitmap, int present);

// GAG.EXE: 0x00417CB0
void __fastcall realize_and_present_custom_control(CustomControlState *state, BOOL background);

// GAG.EXE: 0x00417D10
void __fastcall destroy_custom_control_gdi(HWND window, CustomControlState *state);

// GAG.EXE: 0x00417D90
LRESULT CALLBACK gag_custom_control_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void set_custom_control_gdi_api_for_testing(const CustomControlGdiApi &api);

struct SettingsRegistryApi
{
    LSTATUS(WINAPI *open_key)(HKEY key, LPCSTR sub_key, DWORD options, REGSAM desired_access, PHKEY result);
    LSTATUS(WINAPI *set_value)(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type, const BYTE *data, DWORD data_size);
    LSTATUS(WINAPI *close_key)(HKEY key);
};

// GAG.EXE: 0x0041D060
void __fastcall save_runtime_settings(ApplicationState *state);

struct CursorVisibilityApi
{
    int(WINAPI *show_cursor)(BOOL show);
    void (*on_cursor_hidden)();
    void (*on_cursor_shown)();
};

// GAG.EXE: 0x0041D0D0
void __fastcall set_game_cursor_active(ApplicationState *state, int active);

void set_settings_registry_api_for_testing(const SettingsRegistryApi &api);
void set_cursor_visibility_api_for_testing(const CursorVisibilityApi &api);

struct StateFieldReference
{
    char name[0x24];
    std::int32_t activity;
    std::uint32_t unknown_0028;
    std::uint32_t flags;
};

static_assert(offsetof(StateFieldReference, activity) == 0x24);
static_assert(offsetof(StateFieldReference, flags) == 0x2c);

struct ApplicationStateFieldQuery
{
    char object_name[0x20];
    char field_name[0x20];
};

static_assert(sizeof(ApplicationStateFieldQuery) == 0x40);

// GAG.EXE: 0x0041D510
void __fastcall finish_credits_state(ApplicationState *state, StateFieldReference *reference);

void set_finish_credits_callback_for_testing(void (*callback)());

struct SecondaryWindowLayout
{
    std::uint32_t unknown_0000;
    std::uint32_t state;
    std::int32_t x;
    std::int32_t y;
    std::int32_t width;
    std::int32_t height;
    std::uint32_t flags;
};

static_assert(offsetof(SecondaryWindowLayout, state) == 4);
static_assert(offsetof(SecondaryWindowLayout, flags) == 0x18);
static_assert(sizeof(SecondaryWindowLayout) == 0x1c);

struct WindowLayoutApi
{
    int(WINAPI *get_system_metrics)(int index);
    BOOL(WINAPI *adjust_window_rect)(LPRECT rect, DWORD style, BOOL menu);
    BOOL(WINAPI *set_window_position)(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags);
    BOOL(WINAPI *get_client_rect)(HWND window, LPRECT rect);
    HWND(WINAPI *set_focus)(HWND window);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x0041CE60
void __fastcall update_application_window_layout(ApplicationState *state, SecondaryWindowLayout *secondary_layout);

void set_window_layout_api_for_testing(const WindowLayoutApi &api);

// GAG.EXE: 0x0041D120
void __fastcall restore_application_display(ApplicationState *state);

struct StateActivationApi
{
    std::uint32_t(__fastcall *query_status)(void *identity);
    std::uint32_t (*get_script_state)();
    void (*on_cursor_outside)();
};

// GAG.EXE: 0x0041D380
void __fastcall process_state_activation(ApplicationState *state, StateFieldReference *reference);

void set_state_activation_api_for_testing(const StateActivationApi &api);

struct SaveStateApi
{
    void *(__fastcall *capture_state)(void *game_context, std::uint32_t *size, int mode);
    std::uint32_t (*get_script_state)();
    void (*prepare_8bit_display)();
    int(__fastcall *show_dialog)(void *dialog_context, char *installation_path, const char *dialog_data, void *memory, char *first_path, char *second_path);
    void(__fastcall *save_state)(char *first_path, char *second_path, void *memory, std::uint32_t script_state);
    void (*restore_8bit_display)();
};

// GAG.EXE: 0x0041D280
void __fastcall save_application_state_interactive(ApplicationState *state, void *dialog_context);

void set_save_state_api_for_testing(const SaveStateApi &api);

struct OpenStateApi
{
    void (*prepare_8bit_display)();
    int(__fastcall *show_dialog)(void *dialog_context, char *installation_path, const char *dialog_data, char *installed_version);
    void(__fastcall *restore_8bit_display)(std::uint32_t mode);
};

// GAG.EXE: 0x0041D1C0
void __fastcall open_application_state_interactive(ApplicationState *state, void *dialog_context);

void set_open_state_api_for_testing(const OpenStateApi &api);

struct SynchronizedStateApi
{
    void (*enter_lock)();
    void (*leave_lock)();
    int(__fastcall *operation_17550)(void *first, void *second, void *third, void *fourth);
    int(__fastcall *operation_175f0)(void *first, void *second, void *third, void *fourth, void *fifth, void *sixth);
    int(__fastcall *operation_176a0)(void *first, void *second, void *third, void *fourth);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    HWND (*get_message_window)();
};

// GAG.EXE: 0x0041F7C0
bool __fastcall run_synchronized_state_operation_17550(void *first, void *second, void *third, void *fourth);

// GAG.EXE: 0x0041F830
bool __fastcall run_synchronized_state_operation_175f0(void *first, void *second, void *third, void *fourth, void *fifth, void *sixth);

// GAG.EXE: 0x0041F8F0
bool __fastcall run_synchronized_state_operation_176a0(void *first, void *second, void *third, void *fourth);

void set_synchronized_state_api_for_testing(const SynchronizedStateApi &api);

using InitializeApplication = ApplicationState *(__fastcall *)(int width, int height, HINSTANCE instance, LPSTR command_line, int show_command);

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
std::uint32_t __fastcall load_installation_registry_settings(ApplicationState *state, const RegistryApi &api);

RegistryApi make_win32_registry_api();

// GAG.EXE: 0x0040CF50
int __fastcall copy_string(char *destination, const char *source);

struct ScriptTextBuffer
{
    std::uint32_t length;
    std::uint32_t capacity;
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
std::int32_t __fastcall select_bounded_random_value(std::int32_t minimum, std::int32_t maximum);

// GAG.EXE: 0x0040D0B0
ScriptTextBuffer *create_script_text_buffer();

// GAG.EXE: 0x0040D0E0
void __fastcall clear_script_text_buffer(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D0F0
void __fastcall begin_script_text_document(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D140
void __fastcall end_script_text_document(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D180
void __fastcall append_script_text_property(ScriptTextBuffer *buffer, std::uint32_t property, const char *value);

// GAG.EXE: 0x0040D400
void __fastcall end_script_text_statement(ScriptTextBuffer *buffer);

// GAG.EXE: 0x0040D440
void __fastcall append_script_text_scope(ScriptTextBuffer *buffer, std::uint32_t scope);

// GAG.EXE: 0x0040D610
void __fastcall append_script_text_preload_directive(ScriptTextBuffer *buffer, std::uint32_t scope);

// GAG.EXE: 0x0040CE90
void __fastcall append_script_text_scoped_tokens(ScriptTextBuffer *buffer, std::uint32_t scope, const char *text);

// GAG.EXE: 0x0040D650
void __fastcall append_script_text_delimiter(ScriptTextBuffer *buffer, const char *text, char delimiter);

// GAG.EXE: 0x0040D690
void __fastcall append_script_text_integer(ScriptTextBuffer *buffer, std::uint32_t value, char delimiter);

// GAG.EXE: 0x0040D740
int __fastcall find_script_property_value(char *value, const char *property_name, const char *text, std::uint32_t text_length, std::uint32_t start_offset);

// GAG.EXE: 0x0040D830
int __fastcall find_script_section(const char *section_name, const char *text, int text_length);

// GAG.EXE: 0x00418230
std::int32_t __fastcall parse_path_numeric_identifier(const char *path);

struct RuntimeTreeNode;
struct RuntimeGenericResourceNode;
struct RuntimeTreeLink84;
struct RuntimeTreePrimaryResourceLink;

struct ScriptParserState
{
    RuntimeTreeNode *owner;
    char *name;
    char *creation_text;
    char *scratch_text;
    std::uint8_t unknown_0010[4];
    RuntimeGenericResourceNode *resource;
    const char *text;
    std::uint32_t text_length;
    std::uint32_t start_offset;
    std::uint32_t cursor;
};
static_assert(sizeof(ScriptParserState) == 0x28);
static_assert(offsetof(ScriptParserState, owner) == 0);
static_assert(offsetof(ScriptParserState, resource) == 0x14);
static_assert(offsetof(ScriptParserState, text) == 0x18);
static_assert(offsetof(ScriptParserState, text_length) == 0x1c);
static_assert(offsetof(ScriptParserState, cursor) == 0x24);

// GAG.EXE: 0x0040D8A0
std::uint32_t __fastcall parse_script_property_code(ScriptParserState *parser);

// GAG.EXE: 0x0040DC00
std::uint32_t __fastcall parse_script_scope_code(ScriptParserState *parser);

// GAG.EXE: 0x0040DFD0
std::uint32_t __fastcall parse_script_opcode(ScriptParserState *parser);

// GAG.EXE: 0x0040EA40
std::uint32_t __fastcall extract_script_property_name(ScriptParserState *parser, char *name);

// GAG.EXE: 0x0040EB70
std::uint32_t __fastcall extract_script_scope_name(ScriptParserState *parser, char *name);

// GAG.EXE: 0x0040ECB0
std::uint32_t __fastcall extract_script_parenthesized_text(ScriptParserState *parser, char *text, std::uint32_t text_capacity);

// GAG.EXE: 0x0040ED80
int __fastcall find_whitespace_token_index(const char *text, const char *token);

// GAG.EXE: 0x0040F0A0
std::uint32_t __fastcall extract_script_token(ScriptParserState *parser, char *token, std::uint32_t token_capacity);

struct ScriptValueParseApi
{
    std::uint32_t(__fastcall *evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, std::uint32_t *value_type);
};

struct ScriptTypedValueApi
{
    std::int32_t(__fastcall *parse_integer_expression)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_image_flag)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_value_token)(ScriptParserState *parser, char *value, std::uint32_t capacity);
};

struct RuntimeTreeCommandTargetApi
{
    std::uint32_t(__fastcall *parse_image_flag)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_value_token)(ScriptParserState *parser, char *value, std::uint32_t value_capacity);
};

struct ScriptIntegerExpressionApi
{
    std::uint32_t(__fastcall *evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, std::uint32_t *value_type);
    std::int32_t(__fastcall *select_random)(std::int32_t minimum, std::int32_t maximum);
    RuntimeTreeLink84 *(__fastcall *find_link_0084)(const void *name);
    RuntimeTreePrimaryResourceLink *(__fastcall *find_primary_link)(const void *name);
    std::int32_t(__fastcall *get_object_integer)(const char *object_name, const void *field_name);
    void(__fastcall *query_runtime)(std::uint32_t operation, const void *source, std::int32_t *value);
};

// GAG.EXE: 0x00408AA0
void __fastcall parse_script_typed_value(ScriptParserState *parser, void *value, std::uint32_t *value_type);

// GAG.EXE: 0x00408B20
void __fastcall append_natural_mouse_image_flag(ScriptTextBuffer *buffer, std::uint32_t flags);

// GAG.EXE: 0x0040A9D0
void __fastcall serialize_image_flag_overrides(ScriptTextBuffer *buffer, std::uint32_t flags);

// GAG.EXE: 0x0040EEB0
std::uint32_t __fastcall parse_script_parameter_token(const char *text, std::int32_t token_index, void *value, std::uint32_t *value_type);

// GAG.EXE: 0x0040F070
std::uint32_t __fastcall evaluate_script_parameter(ScriptParserState *parser, const char *name, void *value, std::uint32_t *value_type);

// GAG.EXE: 0x0040F4F0
std::int32_t __fastcall parse_script_integer_expression(ScriptParserState *parser);

// GAG.EXE: 0x0040F2C0
std::uint32_t __fastcall parse_script_value_token(ScriptParserState *parser, char *value, std::uint32_t value_capacity);

// GAG.EXE: 0x0040E580
std::uint32_t __fastcall parse_image_flag(ScriptParserState *parser);

// GAG.EXE: 0x00421440
std::uint32_t __fastcall parse_runtime_tree_command_target(ScriptParserState *parser, char *resource_name, char *tree_name, std::uint32_t *flags);
void set_runtime_tree_command_target_api_for_testing(const RuntimeTreeCommandTargetApi &api);

void set_script_value_parse_api_for_testing(const ScriptValueParseApi &api);
void set_script_typed_value_api_for_testing(const ScriptTypedValueApi &api);
void set_script_integer_expression_api_for_testing(const ScriptIntegerExpressionApi &api);

// GAG.EXE: 0x0040F380
std::int32_t __fastcall parse_script_integer_literal(ScriptParserState *parser);

void set_script_utility_api_for_testing(const ScriptUtilityApi &api);

// GAG.EXE: 0x0040D070
bool __fastcall fixed_dword_memory_equal(const void *left, const void *right, std::uint32_t byte_count);

struct RuntimeVisualObject;

struct ScriptObjectState
{
    char name[0x20];
    void *identity;
    ScriptObjectState *next;
    char field_names[32][0x20];
    std::uint32_t field_count;
    std::uint32_t flags_042c;
    char mouse_visual_name[0x20];
    char alternate_mouse_visual_name[0x20];
    RuntimeVisualObject *visual_object;
    RuntimeVisualObject *alternate_visual_object;
    std::uint32_t image_flags;
    std::uint32_t command_mask;
    std::uint32_t active_field_mask;
    std::int32_t integer_values[32];
    char string_values[32][0x20];
};

static_assert(offsetof(ScriptObjectState, next) == 0x24);
static_assert(offsetof(ScriptObjectState, identity) == 0x20);
static_assert(offsetof(ScriptObjectState, field_names) == 0x28);
static_assert(offsetof(ScriptObjectState, field_count) == 0x428);
static_assert(offsetof(ScriptObjectState, flags_042c) == 0x42c);
static_assert(offsetof(ScriptObjectState, visual_object) == 0x470);
static_assert(offsetof(ScriptObjectState, alternate_visual_object) == 0x474);
static_assert(offsetof(ScriptObjectState, mouse_visual_name) == 0x430);
static_assert(offsetof(ScriptObjectState, alternate_mouse_visual_name) == 0x450);
static_assert(offsetof(ScriptObjectState, image_flags) == 0x478);
static_assert(offsetof(ScriptObjectState, command_mask) == 0x47c);
static_assert(offsetof(ScriptObjectState, active_field_mask) == 0x480);
static_assert(offsetof(ScriptObjectState, integer_values) == 0x484);
static_assert(offsetof(ScriptObjectState, string_values) == 0x504);
static_assert(sizeof(ScriptObjectState) == 0x904);

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
    std::uint32_t active;
    std::int32_t integer_value;
    char string_value[0x20];
};

static_assert(offsetof(ScriptObjectFieldSnapshot, field_name) == 0x20);
static_assert(offsetof(ScriptObjectFieldSnapshot, active) == 0x40);
static_assert(offsetof(ScriptObjectFieldSnapshot, integer_value) == 0x44);
static_assert(offsetof(ScriptObjectFieldSnapshot, string_value) == 0x48);
static_assert(sizeof(ScriptObjectFieldSnapshot) == 0x68);

// GAG.EXE: 0x00406580
void __fastcall copy_runtime_tree_command_name(char *destination, std::uint32_t command);

// GAG.EXE: 0x00408340
ScriptObjectState *__fastcall create_script_object_state(const void *name);

// GAG.EXE: 0x00407FA0
std::uint32_t __fastcall parse_script_object_state(ScriptParserState *parser);

struct ScriptObjectParseApi
{
    std::uint32_t(__fastcall *parse_value)(ScriptParserState *parser, char *value, std::uint32_t capacity);
    std::uint32_t(__fastcall *parse_scope)(ScriptParserState *parser);
    std::int32_t(__fastcall *parse_integer)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_image_flag)(ScriptParserState *parser);
    bool(__fastcall *fixed_equal)(const void *left, const void *right, std::uint32_t bytes);
    RuntimeVisualObject *(__fastcall *find_visual)(const char *name);
    ScriptObjectState *(__fastcall *create_object)(const void *name);
};

void set_script_object_parse_api_for_testing(const ScriptObjectParseApi &api);
void reset_script_object_parse_api_for_testing();

// GAG.EXE: 0x00408420
ScriptObjectState *__fastcall find_script_object_by_identity(void *identity);

// GAG.EXE: 0x00408480
std::int32_t __fastcall query_or_create_script_object_field(const char *object_name, const void *field_name, std::uint32_t *value, std::int32_t value_type);

// GAG.EXE: 0x004087A0
std::int32_t __fastcall get_script_object_integer(const char *object_name, const void *field_name);

// GAG.EXE: 0x00408800
std::uint32_t __fastcall get_script_object_string(const char *object_name, const void *field_name, void *destination);

// GAG.EXE: 0x00408870
std::int32_t __fastcall add_script_object_integer(const char *object_name, const void *field_name, std::int32_t delta);

// GAG.EXE: 0x00408900
bool __fastcall compare_script_object_field(const char *object_name, const void *field_name, const void *value, std::int32_t value_type);

// GAG.EXE: 0x004089E0
std::uint32_t __fastcall get_script_object_field_snapshot(const char *object_name, const void *field_name, ScriptObjectFieldSnapshot *snapshot);

void set_script_object_memory_api_for_testing(const ScriptObjectMemoryApi &api);
void set_script_object_release_api_for_testing(const ScriptObjectReleaseApi &api);

// GAG.EXE: 0x00408D80
void destroy_script_object_states();

// GAG.EXE: 0x00408B80
void __fastcall serialize_script_object_states(ScriptTextBuffer *buffer);

struct ScriptObjectSlot
{
    ScriptObjectState *object;
    std::uint32_t *active_field_mask;
    std::uint32_t field_mask;
};

struct ScriptObjectContainer
{
    char name[0x20];
    void *identity;
    ScriptObjectContainer *next;
    std::uint32_t current_mask;
    std::uint32_t required_mask;
    std::uint32_t slot_count;
    ScriptObjectSlot slots[32];
};

struct RuntimeVisualObject
{
    char name[0x20];
    void *identity;
    RuntimeVisualObject *next;
    char file_name[0x20];
    char serialized_file[0x104];
    std::int32_t position_x;
    std::int32_t position_y;
    void *previous_scene_identity;
    void *scene_identity;
    std::uint32_t flags;
    std::uint32_t palette_flags;
};

static_assert(sizeof(RuntimeVisualObject) == 0x164);
static_assert(offsetof(RuntimeVisualObject, identity) == 0x20);
static_assert(offsetof(RuntimeVisualObject, next) == 0x24);
static_assert(offsetof(RuntimeVisualObject, file_name) == 0x28);
static_assert(offsetof(RuntimeVisualObject, serialized_file) == 0x48);
static_assert(offsetof(RuntimeVisualObject, position_x) == 0x14c);
static_assert(offsetof(RuntimeVisualObject, previous_scene_identity) == 0x154);
static_assert(offsetof(RuntimeVisualObject, scene_identity) == 0x158);
static_assert(offsetof(RuntimeVisualObject, flags) == 0x15c);
static_assert(offsetof(RuntimeVisualObject, palette_flags) == 0x160);

// GAG.EXE: 0x00408DD0
std::uint32_t __fastcall parse_runtime_visual_object(ScriptParserState *parser);

// GAG.EXE: 0x00409060
void *__fastcall create_or_update_runtime_visual_object(const void *name, const void *file_name, std::int32_t position_x, std::int32_t position_y, std::uint32_t flags, std::uint32_t palette_flags);

// GAG.EXE: 0x00409210
void __fastcall serialize_runtime_visual_objects(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004091B0
BOOL __fastcall remove_runtime_visual_object(void *identity);

// GAG.EXE: 0x004092E0
void destroy_runtime_visual_objects();

struct RuntimePointerRegion
{
    std::uint8_t unknown_0000[0x24];
    RuntimePointerRegion *next;
    std::uint8_t unknown_0028[4];
    std::int32_t left;
    std::int32_t top;
    std::int32_t right;
    std::int32_t bottom;
    std::uint8_t unknown_003c[4];
    std::uint32_t scene_mask;
    std::uint32_t first_scene_bit;
    std::uint32_t current_scene_bit;
    std::uint32_t priority;
    RuntimeVisualObject *visual_override;
    void *owner_identity;
    ScriptObjectState *state_object;
    RuntimeTreePrimaryResourceLink *primary_resource;
    void *previous_owner_identity;
    void *previous_primary_resource_identity;
};

static_assert(sizeof(RuntimePointerRegion) == 0x68);
static_assert(offsetof(RuntimePointerRegion, next) == 0x24);
static_assert(offsetof(RuntimePointerRegion, left) == 0x2c);
static_assert(offsetof(RuntimePointerRegion, scene_mask) == 0x40);
static_assert(offsetof(RuntimePointerRegion, current_scene_bit) == 0x48);
static_assert(offsetof(RuntimePointerRegion, priority) == 0x4c);
static_assert(offsetof(RuntimePointerRegion, visual_override) == 0x50);
static_assert(offsetof(RuntimePointerRegion, state_object) == 0x58);
static_assert(offsetof(RuntimePointerRegion, primary_resource) == 0x5c);
static_assert(offsetof(RuntimePointerRegion, previous_owner_identity) == 0x60);

struct RuntimeSceneSlot
{
    RuntimeVisualObject *visual_object;
    std::uint16_t unknown_0004;
    std::uint8_t flags;
    std::uint8_t unknown_0007;
    char name[0x20];
};

static_assert(sizeof(RuntimeSceneSlot) == 0x28);
static_assert(offsetof(RuntimeSceneSlot, flags) == 6);
static_assert(offsetof(RuntimeSceneSlot, name) == 8);

struct RuntimeNamedNode
{
    char name[0x20];
    void *identity;
    std::uint32_t flags;
    std::uint32_t unknown_0028;
    RuntimeNamedNode *next;
    std::uint8_t unknown_0030[0x10];
    std::uint32_t status;
    RuntimeNamedNode *children;
    RuntimeNamedNode *child_sentinel;
    RuntimeNamedNode *child_cursor;
};

static_assert(sizeof(RuntimeNamedNode) == 0x50);
static_assert(offsetof(RuntimeNamedNode, identity) == 0x20);
static_assert(offsetof(RuntimeNamedNode, flags) == 0x24);
static_assert(offsetof(RuntimeNamedNode, next) == 0x2c);
static_assert(offsetof(RuntimeNamedNode, children) == 0x44);
static_assert(offsetof(RuntimeNamedNode, status) == 0x40);
static_assert(offsetof(RuntimeNamedNode, child_cursor) == 0x4c);

struct RuntimeResourceCacheEntry
{
    char name[0x20];
    void *data;
    std::uint32_t size;
    std::uint32_t flags_and_references;
    RuntimeResourceCacheEntry *next;
    RuntimeResourceCacheEntry *previous;
};

struct RuntimeGenericResourceNode
{
    char name[0x20];
    void *identity;
    void *resource_data;
    std::uint32_t current_position;
    void *resource_metadata;
    std::uint32_t active_references;
    std::uint8_t unknown_0034[4];
    RuntimeGenericResourceNode *next;
};

static_assert(sizeof(RuntimeGenericResourceNode) == 0x3c);
static_assert(offsetof(RuntimeGenericResourceNode, identity) == 0x20);
static_assert(offsetof(RuntimeGenericResourceNode, resource_data) == 0x24);
static_assert(offsetof(RuntimeGenericResourceNode, resource_metadata) == 0x2c);
static_assert(offsetof(RuntimeGenericResourceNode, active_references) == 0x30);
static_assert(offsetof(RuntimeGenericResourceNode, next) == 0x38);

static_assert(sizeof(RuntimeResourceCacheEntry) == 0x34);
static_assert(offsetof(RuntimeResourceCacheEntry, data) == 0x20);
static_assert(offsetof(RuntimeResourceCacheEntry, size) == 0x24);
static_assert(offsetof(RuntimeResourceCacheEntry, flags_and_references) == 0x28);
static_assert(offsetof(RuntimeResourceCacheEntry, next) == 0x2c);
static_assert(offsetof(RuntimeResourceCacheEntry, previous) == 0x30);

struct RuntimeNamedNodeMemoryApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeResourceReleaseApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(__fastcall *find_cache_entry)(void *parent_identity, const char *name);
    RuntimeNamedNode *(__fastcall *find_child)(void *parent_identity, void *child_identity);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    std::uint32_t(__fastcall *remove_cache_entry)(void *parent_identity, void *child_identity);
    std::uint32_t(__fastcall *close_async_record)(AsyncFileRecord *record);
    void(__fastcall *set_script_flags)(std::uint32_t flags, int enabled);
};

struct RuntimePlanNode
{
    std::uint8_t unknown_0000[0x24];
    RuntimePlanNode *next;
    std::uint32_t flags;
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
    std::uint8_t unknown_0028[4];
    std::uint32_t flags;
    std::uint8_t unknown_0030[0x10];
    char class_name[0x20];
    std::uint8_t unknown_0060[4];
    RuntimeTreeNode *iterator_current;
    std::uint32_t iterator_ascending;
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

static_assert(sizeof(RuntimeTreeNode) == 0xbc);
static_assert(offsetof(RuntimeTreeNode, identity) == 0x20);
static_assert(offsetof(RuntimeTreeNode, parent) == 0x24);
static_assert(offsetof(RuntimeTreeNode, flags) == 0x2c);
static_assert(offsetof(RuntimeTreeNode, class_name) == 0x40);
static_assert(offsetof(RuntimeTreeNode, iterator_current) == 0x64);
static_assert(offsetof(RuntimeTreeNode, iterator_ascending) == 0x68);
static_assert(offsetof(RuntimeTreeNode, parser_contexts) == 0x6c);
static_assert(offsetof(RuntimeTreeNode, default_visual) == 0x70);

// GAG.EXE: 0x00406B40
std::uint32_t __fastcall apply_runtime_tree_image_flags(ScriptParserState *parser);
static_assert(offsetof(RuntimeTreeNode, scene_link_head) == 0x74);
static_assert(offsetof(RuntimeTreeNode, scene_link_tail) == 0x78);
static_assert(offsetof(RuntimeTreeNode, link_0084_head) == 0x84);
static_assert(offsetof(RuntimeTreeNode, link_0084_tail) == 0x88);
static_assert(offsetof(RuntimeTreeNode, link_008c_head) == 0x8c);
static_assert(offsetof(RuntimeTreeNode, link_008c_tail) == 0x90);
static_assert(offsetof(RuntimeTreeNode, primary_resource_link_head) == 0x9c);
static_assert(offsetof(RuntimeTreeNode, primary_resource_link_tail) == 0xa0);
static_assert(offsetof(RuntimeTreeNode, secondary_resource_link_head) == 0xa4);
static_assert(offsetof(RuntimeTreeNode, secondary_resource_link_tail) == 0xa8);
static_assert(offsetof(RuntimeTreeNode, child) == 0xac);
static_assert(offsetof(RuntimeTreeNode, auxiliary_head) == 0xb0);
static_assert(offsetof(RuntimeTreeNode, next) == 0xb4);
static_assert(offsetof(RuntimeTreeNode, previous) == 0xb8);

struct RuntimeTreeAuxiliaryNode
{
    char name[0x20];
    void *identity;
    RuntimeTreeAuxiliaryNode *next;
};

static_assert(sizeof(RuntimeTreeAuxiliaryNode) == 0x28);
static_assert(offsetof(RuntimeTreeAuxiliaryNode, identity) == 0x20);
static_assert(offsetof(RuntimeTreeAuxiliaryNode, next) == 0x24);

struct RuntimeTreeAuxiliaryReleaseApi
{
    void(__fastcall *notify)(std::uint32_t operation, std::uint32_t unused, RuntimeTreeAuxiliaryNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

// GAG.EXE: 0x004071E0
void __fastcall release_runtime_tree_auxiliary_nodes(RuntimeTreeNode *owner);

void set_runtime_tree_auxiliary_release_api_for_testing(const RuntimeTreeAuxiliaryReleaseApi &api);

struct RuntimeTreeAuxiliaryCreateApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(__fastcall *resolve)(std::uint32_t operation, void **identity, void **metadata);
};

// GAG.EXE: 0x00407040
void __fastcall add_runtime_tree_auxiliary_name(RuntimeTreeNode *owner, const char *name);

// GAG.EXE: 0x004070F0
std::uint32_t __fastcall parse_runtime_tree_auxiliary_names(ScriptParserState *parser);

// GAG.EXE: 0x00407130
std::uint32_t __fastcall add_default_runtime_tree_auxiliary_names(RuntimeTreeNode *owner);

void set_runtime_tree_auxiliary_create_api_for_testing(const RuntimeTreeAuxiliaryCreateApi &api);

struct RuntimeTreeDestructionCoreApi
{
    RuntimeTreeNode *(__fastcall *find_node)(void *identity);
    void(__fastcall *notify)(std::uint32_t operation, std::uint32_t unused, void *value);
    void(__fastcall *remove_scene_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_secondary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_primary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_links_7c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_links_84)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_links_8c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void(__fastcall *remove_containers)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL(__fastcall *destroy_container)(ScriptObjectContainer *container);
    void(__fastcall *release_auxiliary)(RuntimeTreeNode *owner);
    void(__fastcall *release_parsers)(RuntimeTreeNode *owner);
    RuntimeTreeParserContext *(__fastcall *find_parser)(RuntimeTreeNode *owner, const char *name);
    RuntimeTreeNode *(__fastcall *dispatch_parser)(RuntimeTreeParserContext *context);
    void(__fastcall *update_global_links)(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);
};

// GAG.EXE: 0x00405E50
RuntimeTreeNode *__fastcall destroy_runtime_tree_node(void *identity, void *replacement_identity);

// GAG.EXE: 0x00406360
void __fastcall update_runtime_tree_global_links(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);

// GAG.EXE: 0x00406190
void __fastcall publish_runtime_tree_global_links(RuntimeTreeNode *node);

void set_runtime_tree_destruction_core_api_for_testing(const RuntimeTreeDestructionCoreApi &api);

struct RuntimeFixedNameListNode
{
    char name[0x20];
    void *identity;
    std::uint32_t flags;
    char serialized_value[0x20];
    std::uint32_t resource_flags;
    void *resource_identity;
    void *previous_resource_identity;
    RuntimeFixedNameListNode *next;
};

static_assert(offsetof(RuntimeFixedNameListNode, next) == 0x54);
static_assert(offsetof(RuntimeFixedNameListNode, serialized_value) == 0x28);
static_assert(offsetof(RuntimeFixedNameListNode, resource_flags) == 0x48);
static_assert(offsetof(RuntimeFixedNameListNode, resource_identity) == 0x4c);
static_assert(offsetof(RuntimeFixedNameListNode, previous_resource_identity) == 0x50);
static_assert(sizeof(RuntimeFixedNameListNode) == 0x58);

// GAG.EXE: 0x0040CDA0
std::uint32_t __fastcall parse_script_file_value(ScriptParserState *parser, char *value, char *serialized_value);

// GAG.EXE: 0x00407240
std::uint32_t __fastcall create_or_update_runtime_fixed_name_node(ScriptParserState *parser);

// GAG.EXE: 0x004068F0
void __fastcall append_script_runtime_flags(ScriptTextBuffer *buffer, std::uint32_t flags);

// GAG.EXE: 0x004069D0
void __fastcall serialize_runtime_tree_sections(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00406BB0
void __fastcall serialize_runtime_language(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004073D0
void __fastcall serialize_runtime_fixed_name_nodes(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00404990
ScriptTextBuffer *serialize_current_runtime_state();

// GAG.EXE: 0x00406980
RuntimeTreeNode *get_runtime_tree_root();

// GAG.EXE: 0x004069A0
RuntimeTreeNode *find_runtime_tree_tail();

// GAG.EXE: 0x00406600
RuntimeTreeNode *__fastcall find_runtime_tree_ancestor_root(void *identity);

// GAG.EXE: 0x00407380
RuntimeFixedNameListNode *__fastcall find_runtime_fixed_name_list_node(const void *name);

// GAG.EXE: 0x00407440
void destroy_runtime_fixed_name_list_nodes();

static_assert(sizeof(RuntimePlanNode) == 0x2c);

struct RuntimeLockRecord
{
    std::uint8_t unknown_0000[4];
    void *identity_context;
    std::uint32_t unknown_0008;
    std::uint32_t flags;
    std::uint32_t unknown_0010;
    DWORD owner_thread;
    std::uint32_t recursion_count;
    std::int32_t scene_identifier;
};

static_assert(sizeof(RuntimeLockRecord) == 0x20);
static_assert(offsetof(RuntimeLockRecord, identity_context) == 0x04);
static_assert(offsetof(RuntimeLockRecord, flags) == 0x0c);
static_assert(offsetof(RuntimeLockRecord, scene_identifier) == 0x1c);

struct RuntimeSceneRecord
{
    std::uint8_t unknown_0000[4];
    void *context;
    std::uint32_t flags;
    std::uint8_t unknown_000c[8];
    DWORD owner_thread;
    std::uint32_t recursion_count;
    std::int32_t scene_identifier;
    std::uint8_t unknown_0020[0x18];
    std::int32_t x;
    std::int32_t y;
    std::uint8_t unknown_0040[8];
    std::int32_t x_offset;
    std::int32_t y_offset;
    std::uint8_t unknown_0050[8];
    std::int32_t previous_x;
    std::int32_t previous_y;
};

static_assert(sizeof(RuntimeSceneRecord) == 0x60);
static_assert(offsetof(RuntimeSceneRecord, owner_thread) == 0x14);
static_assert(offsetof(RuntimeSceneRecord, scene_identifier) == 0x1c);
static_assert(offsetof(RuntimeSceneRecord, x) == 0x38);
static_assert(offsetof(RuntimeSceneRecord, x_offset) == 0x48);
static_assert(offsetof(RuntimeSceneRecord, previous_x) == 0x58);

struct RuntimeResourceObject
{
    std::uint32_t state_flags;
    void *backend;
    std::uint32_t type_flags;
    std::uint32_t backend_flags;
    void *data;
    DWORD owner_thread;
    std::uint32_t recursion_count;
    std::int32_t scene_identifier;
    std::uint8_t scene_descriptor[0x10];
    std::uint32_t presentation_owner;
    std::uint32_t field_0034;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t output_width;
    std::uint32_t output_height;
    std::uint32_t requested_width;
    std::uint32_t requested_height;
    std::uint32_t frame_limit;
    std::uint32_t frames_remaining;
    std::int32_t previous_x;
    std::int32_t previous_y;
    std::uint8_t unknown_0060[4];
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    std::uint8_t unknown_006c[8];
    union
    {
        RuntimeGenericBackendChild *generic_backend_child;
        std::uint32_t field_0074;
    };
    std::uint8_t unknown_0078[0x11c];
    std::uint32_t callback_position;
};

struct DisplayTraversalState;

static_assert(sizeof(RuntimeResourceObject) == 0x198);
static_assert(offsetof(RuntimeResourceObject, backend) == 4);
static_assert(offsetof(RuntimeResourceObject, type_flags) == 8);
static_assert(offsetof(RuntimeResourceObject, backend_flags) == 0x0c);
static_assert(offsetof(RuntimeResourceObject, data) == 0x10);
static_assert(offsetof(RuntimeResourceObject, owner_thread) == 0x14);
static_assert(offsetof(RuntimeResourceObject, fixed_resource_identity) == 0x64);
static_assert(offsetof(RuntimeResourceObject, secondary_resource_identity) == 0x68);
static_assert(offsetof(RuntimeResourceObject, generic_backend_child) == 0x74);
static_assert(offsetof(RuntimeResourceObject, scene_identifier) == 0x1c);
static_assert(offsetof(RuntimeResourceObject, scene_descriptor) == 0x20);
static_assert(offsetof(RuntimeResourceObject, presentation_owner) == 0x30);
static_assert(offsetof(RuntimeResourceObject, x) == 0x38);
static_assert(offsetof(RuntimeResourceObject, output_width) == 0x40);
static_assert(offsetof(RuntimeResourceObject, requested_width) == 0x48);
static_assert(offsetof(RuntimeResourceObject, frame_limit) == 0x50);
static_assert(offsetof(RuntimeResourceObject, previous_x) == 0x58);
static_assert(offsetof(RuntimeResourceObject, field_0074) == 0x74);
static_assert(offsetof(RuntimeResourceObject, callback_position) == 0x194);

struct RuntimeResourceConstructionPlan
{
    std::uint32_t flags;
    std::uint32_t scene_identifier;
    std::uint32_t scene_flags;
    std::int32_t x;
    std::int32_t y;
};

struct RuntimeResourceConstructionPlanApi
{
    std::uint32_t(__fastcall *find_available_scene)(std::uint32_t flags);
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
    std::uint32_t(__fastcall *detect_type)(const char *path);
    void(__fastcall *update_host)(const char *path, std::int32_t mode);
    void(__fastcall *load)(const char *path, void **data, std::uint32_t *size, std::int32_t *storage, std::uint32_t flags);
    RuntimeMediaBackend *(__fastcall *create_bitmap)(std::uint32_t unused, std::uint32_t extension_bytes, void *data);
    RuntimeAnimationBackend *(__fastcall *create_animation)(std::uint32_t unused, void *data, std::uint32_t extension_bytes, std::uint32_t storage);
    std::uint32_t(__fastcall *create_sound)(WAVEFORMATEX *format);
    RuntimeSoundSlot *(__fastcall *get_sound_slot)(std::uint32_t handle);
    std::uint32_t(__fastcall *start_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *queue_sound)(std::uint32_t handle, void *data, std::uint32_t size, std::int32_t replace);
    void(__fastcall *set_sound_loop)(std::uint32_t handle, std::uint32_t value);
    std::uint32_t(__fastcall *stop_sound)(std::uint32_t handle, std::int32_t reset_timing);
    RuntimeGenericBackend *(__fastcall *create_generic)(std::uint32_t data, std::uint32_t size);
    RuntimeGenericResourceNode *(__fastcall *find_generic_resource)(const char *path);
    RuntimeTreeNode *(__fastcall *activate_tree)(const char *resource_name, const char *tree_name, void *creation_context, void *unused);
    void(__fastcall *rebuild_tree)(void *identity);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    std::uint32_t(__fastcall *configure_bitmap)(void *identity, const std::uint32_t *transform, const std::uint32_t *descriptor, void *callback, std::uint32_t flags);
    std::uint32_t(__fastcall *configure_animation)(void *identity, const std::uint32_t *transform, const std::uint32_t *descriptor, const void *comparison_palette, std::uint32_t flags,
        std::int32_t(__fastcall *callback)(RuntimeMediaBackend *backend));
    std::uint32_t(__fastcall *begin_scene)(std::int32_t identifier);
    void(__fastcall *finalize_media)(void *identity);
    void(__fastcall *configure_palette)(RuntimeResourceObject *resource);
    std::uint32_t(__fastcall *end_scene)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void(__fastcall *wait_for_count)(std::uint32_t count);
    std::uint32_t(__fastcall *destroy_media)(void *identity);
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    std::uint32_t(__fastcall *destroy_generic)(void *identity);
    BOOL(__fastcall *release_memory)(const char *path);
    std::uint32_t(__fastcall *release_stream)(AsyncFileRecord *record);
    void(__fastcall *build_path)(char *destination, const char *source);
    CdfArchive *(__fastcall *open_archive)(const char *path, int alternate_stream);
    RuntimeResourceCacheEntry *(__fastcall *register_resource)(void *parent_identity, void *data);
    std::uint32_t(
        __fastcall *add_scene_callback)(std::int32_t identifier, int(__fastcall *callback)(DisplayTraversalState *state), const void *context, std::uint32_t context_size, std::uint32_t flags);
};

// Non-original helper: exact pre-dispatch normalization from ConstructRuntimeResourceObject.
RuntimeResourceConstructionPlan prepare_runtime_resource_construction(std::uint32_t scene_identifier, std::int32_t x, std::int32_t y, std::uint32_t flags);

// GAG.EXE: 0x00424EC0
void *__fastcall construct_runtime_resource(char *path, std::uint32_t scene_identifier, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t scale_or_loop,
    std::uint32_t flags);

// Non-original deterministic entry used by constructor tests; the final legacy argument is ignored.
void *construct_runtime_resource_with_stack_value(char *path, std::uint32_t scene_identifier, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t scale_or_loop,
    std::uint32_t flags, std::int32_t animation_loop_stack_value);

void set_runtime_resource_construction_api_for_testing(const RuntimeResourceConstructionApi &api);

struct RuntimeResourceVisibilityCallbackContext
{
    std::uint32_t palette_state;
    std::uint32_t resource_flags;
    char resource_name[260];
};

static_assert(sizeof(RuntimeResourceVisibilityCallbackContext) == 0x10c);

// GAG.EXE: 0x00428160
std::uint32_t __fastcall update_runtime_resource_visibility(DisplayTraversalState *state);

struct RuntimeResourceDestroyApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeGenericResourceNode *(__fastcall *find_generic)(void *identity);
    void(__fastcall *remove_generic)(void *identity);
    std::uint32_t(__fastcall *destroy_media_backend)(void *backend);
    BOOL(__fastcall *release_memory_data)(void *data);
    std::uint32_t(__fastcall *release_stream)(AsyncFileRecord *record);
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    std::uint32_t(__fastcall *destroy_generic_backend)(void *backend);
    std::uint32_t(__fastcall *release_scene)(std::int32_t scene_identifier, std::int32_t owner);
    std::uint32_t(__fastcall *remove_runtime_child)(void *parent_identity, void *child_identity);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeSoundSlot;

struct RuntimeResourceControlApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
    std::uint32_t(__fastcall *destroy_resource)(void *identity);
    RuntimeSoundSlot *(__fastcall *get_sound_slot)(std::uint32_t handle);
};

// GAG.EXE: 0x00425BD0
void __fastcall request_runtime_resource_destruction(void *identity);

// GAG.EXE: 0x00425FB0
std::uint32_t __fastcall query_runtime_resource_frame_limit(void *identity);

// GAG.EXE: 0x00425FF0
std::uint32_t __fastcall query_runtime_resource_playback_flags(void *identity);

// GAG.EXE: 0x004258C0
void __fastcall set_runtime_property_value(std::uint32_t value);

// GAG.EXE: 0x00425F00
std::uint32_t get_runtime_property_value();

// GAG.EXE: 0x00426080
std::uint16_t __fastcall query_runtime_resource_frame_number(void *identity);

using RuntimeResourceConstructor = void *(
    __fastcall *)(char *path, std::uint32_t scene_identifier, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t scale_or_loop, std::uint32_t flags);

struct RuntimeResourceSelectionApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    std::uint32_t(__fastcall *close_archive)(CdfArchive *archive);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    RuntimeResourceConstructor construct_resource;
};

// GAG.EXE: 0x004244E0
void __fastcall select_runtime_resource(char *path);

// Non-original compatibility entry retained for focused caller tests.
void select_runtime_resource_with_loop_register(char *path, std::int32_t loop_animation);

void set_runtime_resource_selection_api_for_testing(const RuntimeResourceSelectionApi &api);

struct RuntimeGameDllUnloadApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    BOOL(WINAPI *free_library)(HMODULE module);
    void (*leave_runtime_state)();
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x004260B0
void unload_runtime_game_dll();

struct RuntimeGameHostContext
{
    HWND window;
    std::uint32_t unknown_0004;
    std::uint32_t bits_per_pixel;
    std::uint8_t unknown_000c[0x14];
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t unknown_0024;
    std::uint32_t unknown_0028;
    void *framebuffer;
    std::uint32_t unknown_0030;
    std::uint32_t unknown_0034;
    std::uint32_t unknown_0038;
    std::uint32_t unknown_003c;
};

static_assert(sizeof(RuntimeGameHostContext) == 0x40);
static_assert(offsetof(RuntimeGameHostContext, width) == 0x20);
static_assert(offsetof(RuntimeGameHostContext, framebuffer) == 0x2c);

using RuntimeGameDllInitialize = void(__fastcall *)(RuntimeGameHostContext *context, void **callbacks);
using RuntimeGameDllExecute = void(__fastcall *)(std::uint32_t command);
using RuntimeGameDllWindowProcedure = std::uint32_t(__fastcall *)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct RuntimeGameResultDescriptor
{
    std::uint32_t type;
    std::uint32_t reserved;
    std::uint32_t size;
    const void *data;
};

static_assert(sizeof(RuntimeGameResultDescriptor) == 0x10);

struct RuntimeGameDllLoadApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(__fastcall *update_resource_host)(const char *path, std::int32_t reset);
    void(__fastcall *build_resource_path)(char *destination, const char *source);
    std::int32_t(__fastcall *activate_comment_scene)(const char *name);
    HMODULE(WINAPI *load_library)(LPCSTR path);
    FARPROC(WINAPI *get_proc_address)(HMODULE module, LPCSTR name);
    void(__fastcall *deactivate_comment_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x00426110
bool __fastcall load_and_initialize_runtime_game_dll(const char *path);

struct RuntimeGameDllDispatchApi
{
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x00426210
std::uint32_t stop_runtime_game_dll();

// GAG.EXE: 0x00426270
std::uint32_t pause_runtime_game_dll();

// GAG.EXE: 0x00426290
std::uint32_t resume_runtime_game_dll();

struct DisplayRectangle;

struct RuntimeGameWindowApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *get_update_rect)(HWND window, LPRECT rectangle, BOOL erase);
    HDC(WINAPI *begin_paint)(HWND window, LPPAINTSTRUCT paint);
    std::uint32_t(__fastcall *queue_display_rectangle)(DisplayRectangle *rectangle);
    BOOL(WINAPI *end_paint)(HWND window, const PAINTSTRUCT *paint);
    void(__fastcall *update_pointer_position)(std::int32_t x, std::int32_t y);
    void(__fastcall *enqueue_byte)(std::uint8_t value);
    void(__fastcall *enqueue_pair)(std::uint32_t first, std::uint32_t second);
    void(__fastcall *enqueue_message)(std::uint32_t message);
    void (*clear_runtime_flag)();
    void (*unload_game_dll)();
    void (*enter_runtime_state)();
    void (*leave_runtime_state)();
    void (*set_runtime_flag)();
    LRESULT(WINAPI *default_window_procedure)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x004231E0
LRESULT CALLBACK runtime_game_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

struct RuntimePointerPositionApi
{
    DWORD(WINAPI *get_current_thread_id)();
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    RuntimeNamedNode *(__fastcall *find_child)(void *parent_identity, void *child_identity);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    std::uint32_t(__fastcall *offset_scene)(std::int32_t identifier, std::int32_t x, std::int32_t y);
};

// GAG.EXE: 0x004243F0
void __fastcall update_runtime_pointer_position(std::int32_t x, std::int32_t y);

// GAG.EXE: 0x00424D80
std::uint32_t __fastcall destroy_runtime_resource(void *identity);

struct RuntimeSoundSlot;
struct RuntimeMediaBackend;
struct DisplaySceneDescriptor;
using RuntimeAnimationCallback = std::int32_t(__fastcall *)(RuntimeMediaBackend *backend);

struct RuntimeMediaBackend
{
    std::uint32_t type;
    void *identity;
    DWORD owner_thread;
    std::uint32_t recursion_count;
    RuntimeMediaBackend *previous;
    RuntimeMediaBackend *next;
    const void *comparison_palette;
    std::uint16_t field_001c;
    std::uint16_t field_001e;
    PALETTEENTRY palette_entries[0x100];
    RGBQUAD dib_colors[0x100];
    std::uint32_t palette_padding;
    std::uint8_t palette_remap[0x100];
    std::uint16_t destination_x;
    std::uint16_t destination_y;
    std::uint16_t destination_stride;
    std::uint16_t destination_reserved;
    std::uint32_t descriptor_2;
    std::uint8_t *destination_pixels;
    HWND window;
    HDC destination_context;
    std::uint32_t destination_bits_per_pixel;
    HPALETTE destination_palette;
    std::uint32_t presentation_field_0944;
    HDC source_context;
    std::uint8_t presentation_tail[0x10];
    std::uint32_t media_flags;
    std::uint32_t error_state;
    std::uint32_t scale_x;
    std::uint32_t scale_y;
    void *extension_data;
    void *source_data;
    void *format_data;
    void *frame_header;
    void *chunk_header;
    RuntimeSoundSlot *sound_slot;
    void *audio_buffer;
    void *frame_buffer;
    std::int32_t dirty_left;
    std::int32_t dirty_top;
    std::int32_t dirty_right;
    std::int32_t dirty_bottom;
    std::uint16_t frame_number;
    std::uint16_t frame_reserved;
    std::uint32_t previous_frame_time;
    std::uint32_t next_frame_time;
    std::int32_t timing_correction;
    std::uint32_t synchronized_sound_frame;
    std::uint32_t timing_adjustment;
    std::uint32_t frame_duration;
    std::int32_t(__fastcall *animation_callback)(RuntimeMediaBackend *backend);
    std::uint32_t sound_handle;
    std::uint32_t allocation_1_active;
    AsyncFileRecord *stream_record;
    std::uint32_t allocation_2_active;
};

static_assert(sizeof(RuntimeMediaBackend) == 0x9cc);
static_assert(offsetof(RuntimeMediaBackend, identity) == 4);
static_assert(offsetof(RuntimeMediaBackend, owner_thread) == 8);
static_assert(offsetof(RuntimeMediaBackend, recursion_count) == 0x0c);
static_assert(offsetof(RuntimeMediaBackend, previous) == 0x10);
static_assert(offsetof(RuntimeMediaBackend, next) == 0x14);
static_assert(offsetof(RuntimeMediaBackend, field_001c) == 0x1c);
static_assert(offsetof(RuntimeMediaBackend, palette_entries) == 0x20);
static_assert(offsetof(RuntimeMediaBackend, dib_colors) == 0x420);
static_assert(offsetof(RuntimeMediaBackend, palette_remap) == 0x824);
static_assert(offsetof(RuntimeMediaBackend, destination_x) == 0x924);
static_assert(offsetof(RuntimeMediaBackend, destination_pixels) == 0x930);
static_assert(offsetof(RuntimeMediaBackend, window) == 0x934);
static_assert(offsetof(RuntimeMediaBackend, destination_context) == 0x938);
static_assert(offsetof(RuntimeMediaBackend, destination_bits_per_pixel) == 0x93c);
static_assert(offsetof(RuntimeMediaBackend, destination_palette) == 0x940);
static_assert(offsetof(RuntimeMediaBackend, source_context) == 0x948);
static_assert(offsetof(RuntimeMediaBackend, media_flags) == 0x95c);
static_assert(offsetof(RuntimeMediaBackend, error_state) == 0x960);
static_assert(offsetof(RuntimeMediaBackend, scale_x) == 0x964);
static_assert(offsetof(RuntimeMediaBackend, scale_y) == 0x968);
static_assert(offsetof(RuntimeMediaBackend, extension_data) == 0x96c);
static_assert(offsetof(RuntimeMediaBackend, source_data) == 0x970);
static_assert(offsetof(RuntimeMediaBackend, format_data) == 0x974);
static_assert(offsetof(RuntimeMediaBackend, frame_header) == 0x978);
static_assert(offsetof(RuntimeMediaBackend, chunk_header) == 0x97c);
static_assert(offsetof(RuntimeMediaBackend, sound_slot) == 0x980);
static_assert(offsetof(RuntimeMediaBackend, audio_buffer) == 0x984);
static_assert(offsetof(RuntimeMediaBackend, frame_buffer) == 0x988);
static_assert(offsetof(RuntimeMediaBackend, dirty_left) == 0x98c);
static_assert(offsetof(RuntimeMediaBackend, frame_number) == 0x99c);
static_assert(offsetof(RuntimeMediaBackend, previous_frame_time) == 0x9a0);
static_assert(offsetof(RuntimeMediaBackend, frame_duration) == 0x9b4);
static_assert(offsetof(RuntimeMediaBackend, animation_callback) == 0x9b8);
static_assert(offsetof(RuntimeMediaBackend, sound_handle) == 0x9bc);
static_assert(offsetof(RuntimeMediaBackend, stream_record) == 0x9c4);
static_assert(offsetof(RuntimeMediaBackend, audio_buffer) == 0x984);
static_assert(offsetof(RuntimeMediaBackend, frame_buffer) == 0x988);
static_assert(offsetof(RuntimeMediaBackend, allocation_1_active) == 0x9c0);
static_assert(offsetof(RuntimeMediaBackend, allocation_2_active) == 0x9c8);

struct RuntimeMediaBackendApi
{
    DWORD(WINAPI *get_current_thread_id)();
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct RuntimeBitmapBackendCreateApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
};

// GAG.EXE: 0x0042A1B0
RuntimeMediaBackend *__fastcall create_runtime_bitmap_backend(std::uint32_t unused, std::uint32_t extension_bytes, void *bitmap_data);

struct RuntimeAnimationBackend
{
    RuntimeMediaBackend base;
    void *source_cursor;
    void *data_start;
    void *data_end;
    std::uint8_t header[0x80];
    std::uint8_t streamed_tail[0x18];
};

struct DisplaySceneNode;

static_assert(sizeof(RuntimeAnimationBackend) == 0xa70);
static_assert(offsetof(RuntimeAnimationBackend, data_start) == 0x9d0);
static_assert(offsetof(RuntimeAnimationBackend, data_end) == 0x9d4);
static_assert(offsetof(RuntimeAnimationBackend, header) == 0x9d8);

struct RuntimeAnimationBackendCreateApi
{
    std::uint32_t(__fastcall *get_position)(AsyncFileRecord *record);
    std::uint32_t(__fastcall *read_record)(AsyncFileRecord *record, void *destination, std::uint32_t bytes, std::uint32_t *bytes_read, std::int32_t force_host_buffer);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    std::uint32_t(__fastcall *set_position)(AsyncFileRecord *record, std::uint32_t position);
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
void __fastcall render_runtime_generic_backend_child(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00427AB0
void __fastcall update_runtime_generic_backend_child(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00427EF0
std::int32_t __fastcall update_runtime_resource_animation_backend(RuntimeMediaBackend *backend);

// GAG.EXE: 0x0042A290
std::uint32_t __fastcall configure_runtime_bitmap_backend(void *identity, const std::uint32_t *transform, const std::uint32_t *descriptor, void *callback, std::uint32_t flags);

// GAG.EXE: 0x0042A340
std::uint32_t __fastcall configure_runtime_animation_backend(void *identity, const std::uint32_t *transform, const std::uint32_t *descriptor, const void *comparison_palette, std::uint32_t flags,
    RuntimeAnimationCallback callback);

struct RuntimeResourcePaletteConfigureApi
{
    bool(__fastcall *set_primary_owner)(std::int32_t identifier, std::int32_t owner, bool replace_existing);
    bool(__fastcall *configure_palette)(DisplaySceneNode *node, const std::uint32_t *palette, std::uint32_t count);
};

// GAG.EXE: 0x00427E60
void __fastcall configure_runtime_resource_palette(RuntimeResourceObject *resource);

// GAG.EXE: 0x00415D90
void __fastcall build_runtime_palette_index_remap(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00417260
std::uint8_t __fastcall convert_runtime_bitmap_to_surface(RuntimeMediaBackend *backend);

struct RuntimeMediaBackendFinalizeApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    std::uint8_t(__fastcall *convert_bitmap)(RuntimeMediaBackend *backend);
    UINT(WINAPI *set_palette_entries)(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
    UINT(WINAPI *realize_palette)(HDC context);
    UINT(WINAPI *set_dib_color_table)(HDC context, UINT start, UINT count, const RGBQUAD *colors);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
};

// GAG.EXE: 0x0042B300
void __fastcall finalize_runtime_media_backend(void *identity);

struct RuntimeAnimationFailureApi
{
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// GAG.EXE: 0x0042A4C0
void __fastcall fail_runtime_animation(RuntimeMediaBackend *backend, std::uint32_t error);

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
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    std::uint32_t(__fastcall *start_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *stop_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *set_stream_position)(AsyncFileRecord *record, std::uint32_t position);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

// Non-original helper: exact control phase of RunRuntimeAnimationThread.
RuntimeAnimationControlResult process_runtime_animation_control(RuntimeAnimationBackend *backend, std::uint32_t current_time, std::uint32_t *wait_milliseconds);

// Non-original helper: exact frame scheduling phase of RunRuntimeAnimationThread.
void schedule_runtime_animation_frame(RuntimeMediaBackend *backend, std::uint32_t current_time);

struct RuntimeAnimationFrameAcquireApi
{
    std::uint32_t(__fastcall *read_record)(AsyncFileRecord *record, void *destination, std::uint32_t bytes, std::uint32_t *bytes_read, std::int32_t force_host_buffer);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void(__fastcall *fail_animation)(RuntimeMediaBackend *backend, std::uint32_t error);
};

// Non-original helper: exact frame acquisition phase of RunRuntimeAnimationThread.
bool acquire_runtime_animation_frame(RuntimeAnimationBackend *backend);

struct RuntimeAnimationDecodeApi
{
    void(__fastcall *decode_palette)(RuntimeMediaBackend *backend);
    void(__fastcall *decode_mvz5)(RuntimeMediaBackend *backend);
    void(__fastcall *decode_delta_flc)(RuntimeMediaBackend *backend);
    void(__fastcall *decode_mvz8)(RuntimeMediaBackend *backend);
    void (*ignore_chunk_11)();
    void (*ignore_chunk_12)();
    void (*ignore_chunk_13)();
    void(__fastcall *decode_byte_run)(RuntimeMediaBackend *backend);
    void(__fastcall *decode_literal)(RuntimeMediaBackend *backend);
};

// Non-original helper: exact visual chunk dispatch phase of RunRuntimeAnimationThread.
void decode_runtime_animation_frame_chunks(RuntimeAnimationBackend *backend);

struct RuntimeAnimationCompletionApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    std::uint32_t(__fastcall *set_stream_position)(AsyncFileRecord *record, std::uint32_t position);
};

// Non-original helper: exact presentation/completion phase of RunRuntimeAnimationThread.
void complete_runtime_animation_frame(RuntimeAnimationBackend *backend);

struct RuntimeAnimationAudioApi
{
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    std::uint32_t(__fastcall *queue_sound_data)(std::uint32_t handle, void *data, std::uint32_t size, std::int32_t replace);
    std::uint32_t(__fastcall *stop_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *start_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *create_sound)(WAVEFORMATEX *format);
    RuntimeSoundSlot *(__fastcall *get_sound_slot)(std::uint32_t handle);
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
std::int32_t __fastcall present_runtime_animation_frame(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00415E60
void __fastcall decode_runtime_animation_palette(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00415EE0
void __fastcall decode_runtime_animation_mvz8(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416420
void __fastcall decode_runtime_animation_mvz5(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416900
void __fastcall decode_runtime_animation_literal(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416AD0
void __fastcall decode_runtime_animation_byte_run(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00416DA0
void __fastcall decode_runtime_animation_delta_flc(RuntimeMediaBackend *backend);


// GAG.EXE: 0x0042B820
void ignore_runtime_animation_chunk_11();

// GAG.EXE: 0x0042B830
void ignore_runtime_animation_chunk_12();

// GAG.EXE: 0x0042B840
void ignore_runtime_animation_chunk_13();

// GAG.EXE: 0x00429EB0
RuntimeAnimationBackend *__fastcall create_runtime_animation_backend(std::uint32_t unused, void *data, std::uint32_t extension_bytes, std::uint32_t storage);

// GAG.EXE: 0x0042B620
RuntimeMediaBackend *__fastcall acquire_runtime_media_backend(void *identity);

// GAG.EXE: 0x0042B5B0
std::uint32_t __fastcall get_runtime_media_backend_type(void *identity);

// GAG.EXE: 0x004299B0
std::uint8_t __fastcall classify_runtime_media_data(const void *data);

// GAG.EXE: 0x00404920
std::uint32_t __fastcall read_compressor_input(void *destination, std::uint32_t requested_size);

// Non-original test state accessors.
void set_compressor_input_state_for_testing(const void *input, std::uint32_t input_size, std::uint32_t input_position);
std::uint32_t get_compressor_input_position_for_testing();

// GAG.EXE: 0x0042B2A0
void __fastcall set_runtime_media_backend_scale(void *identity, std::uint32_t scale_x, std::uint32_t scale_y);

// GAG.EXE: 0x0042A440
std::uint32_t __fastcall stop_runtime_animation_backend(void *identity);

// GAG.EXE: 0x0042B600
void *__fastcall get_locked_runtime_media_extension(void *identity);

struct RuntimePaletteTarget
{
    std::uint32_t unknown_0000;
    HDC device_context;
    std::uint32_t unknown_0008;
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
UINT __fastcall apply_runtime_palette_entries(RuntimePaletteTarget *target, void *palette_data, std::uint32_t *flags, std::uint32_t force);

// GAG.EXE: 0x0042B4E0
std::uint32_t __fastcall destroy_runtime_media_backend(void *identity);

struct RuntimeGenericBackendChild;

struct RuntimeGenericBackend
{
    void *identity;
    std::uint32_t flags;
    RuntimeGenericBackend *next;
    std::uint32_t text_size;
    const char *text;
    std::uint8_t unknown_0014[8];
    std::uint32_t child_count;
    RuntimeGenericBackendChild *children;
};

struct RuntimeGenericBackendChild
{
    void *identity;
    RuntimeGenericBackend *parent;
    std::uint32_t flags;
    std::uint32_t context[2];
    std::uint32_t state[15];
    std::uint32_t computed_state[15];
    std::uint32_t state_end_position;
    std::uint32_t default_selection;
    std::uint32_t parser_position;
    std::uint32_t text_search_position;
    std::uint32_t descriptor[4];
    void *font_identity;
    RuntimeGenericBackendChild *next;
};

static_assert(sizeof(RuntimeGenericBackend) == 0x24);
static_assert(offsetof(RuntimeGenericBackend, text_size) == 0x0c);
static_assert(offsetof(RuntimeGenericBackend, text) == 0x10);
static_assert(offsetof(RuntimeGenericBackend, child_count) == 0x1c);
static_assert(offsetof(RuntimeGenericBackend, children) == 0x20);
static_assert(sizeof(RuntimeGenericBackendChild) == 0xb4);
static_assert(offsetof(RuntimeGenericBackendChild, flags) == 8);
static_assert(offsetof(RuntimeGenericBackendChild, context) == 0x0c);
static_assert(offsetof(RuntimeGenericBackendChild, state) == 0x14);
static_assert(offsetof(RuntimeGenericBackendChild, computed_state) == 0x50);
static_assert(offsetof(RuntimeGenericBackendChild, state_end_position) == 0x8c);
static_assert(offsetof(RuntimeGenericBackendChild, default_selection) == 0x90);
static_assert(offsetof(RuntimeGenericBackendChild, parser_position) == 0x94);
static_assert(offsetof(RuntimeGenericBackendChild, text_search_position) == 0x98);
static_assert(offsetof(RuntimeGenericBackendChild, descriptor) == 0x9c);
static_assert(offsetof(RuntimeGenericBackendChild, font_identity) == 0xac);
static_assert(offsetof(RuntimeGenericBackendChild, next) == 0xb0);

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
RuntimeGenericBackend *__fastcall create_runtime_generic_backend(std::uint32_t value_0010, std::uint32_t value_000c);

// GAG.EXE: 0x00410CC0
RuntimeGenericBackend *__fastcall acquire_runtime_generic_backend(void *identity);

// GAG.EXE: 0x00410D40
void __fastcall clear_runtime_generic_backend_ready(RuntimeGenericBackend *backend);

// GAG.EXE: 0x00410DE0
void *__fastcall find_available_runtime_generic_child(std::uint32_t maximum_end_position);

// GAG.EXE: 0x00410E50
std::int32_t __fastcall find_runtime_generic_text_entry(RuntimeGenericBackend *backend, std::int32_t category, const char *name);

struct RuntimeGenericChildCreateApi
{
    RuntimeGenericBackend *(__fastcall *acquire_backend)(void *identity);
    std::int32_t(__fastcall *find_text_entry)(RuntimeGenericBackend *backend, std::int32_t category, const char *name);
    std::int32_t(__fastcall *parse_integer)(const char *text, std::uint32_t *position, std::uint32_t end, std::uint32_t flags);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    std::uint32_t(__fastcall *build_child_state)(void *identity, std::uint32_t selection, std::uint32_t *state, std::uint32_t *descriptor, std::uint32_t *context);
    void(__fastcall *clear_backend_ready)(RuntimeGenericBackend *backend);
};

// GAG.EXE: 0x004110B0
RuntimeGenericBackendChild *__fastcall create_runtime_generic_backend_child(void *backend_identity, void *font_identity, const std::uint32_t *context, std::uintptr_t selection, std::uint32_t flags);

void set_runtime_generic_child_create_api_for_testing(const RuntimeGenericChildCreateApi &api);

// GAG.EXE: 0x00411220
RuntimeGenericBackendChild *__fastcall acquire_runtime_generic_backend_child(void *identity);

// GAG.EXE: 0x004118F0
std::int32_t __fastcall parse_runtime_generic_integer(const char *text, std::uint32_t *position, std::uint32_t end, std::uint32_t flags);

// GAG.EXE: 0x004119A0
std::int32_t __fastcall skip_runtime_generic_statement(const char *text, std::uint32_t *position, std::uint32_t end, std::uint32_t flags);

// GAG.EXE: 0x00411A20
std::int32_t __fastcall parse_runtime_generic_directive(const char *text, std::uint32_t *position, std::uint32_t end, std::uint32_t flags);

// GAG.EXE: 0x00411560
std::uint32_t __fastcall build_runtime_generic_backend_child_state(void *identity, std::uint32_t selection, std::uint32_t *state, std::uint32_t *descriptor, std::uint32_t *context);

// GAG.EXE: 0x00411420
void __fastcall publish_runtime_generic_backend_child_state(void *identity, const std::uint32_t *state, const std::uint32_t *descriptor, std::int32_t end_offset);

// GAG.EXE: 0x004122C0
std::uint32_t __fastcall measure_runtime_font_glyph(std::uint8_t character, const RuntimeMediaBackend *backend);

// GAG.EXE: 0x00412370
std::uint32_t __fastcall draw_runtime_font_glyph(DisplaySceneDescriptor *destination, std::uint8_t character, std::int32_t x, std::int32_t y, const RuntimeMediaBackend *font, std::uint32_t low_color,
    std::uint32_t high_color);

// GAG.EXE: 0x00411FF0
void __fastcall draw_runtime_generic_text(const char *text, std::uint32_t end, const std::uint32_t *state, void *font_identity, DisplaySceneDescriptor *destination, std::uint32_t flags);

struct RuntimeStandaloneTextState
{
    std::uint32_t unknown_0000[3];
    const char *text;
    void *font_identity;
    std::uint32_t value_0014;
    std::uint32_t value_0018;
    std::uint32_t low_color;
    std::uint32_t high_color;
    std::uint32_t unknown_0024[2];
    std::uint32_t bounds[4];
};
static_assert(sizeof(RuntimeStandaloneTextState) == 0x3c);
static_assert(offsetof(RuntimeStandaloneTextState, text) == 0x0c);
static_assert(offsetof(RuntimeStandaloneTextState, font_identity) == 0x10);
static_assert(offsetof(RuntimeStandaloneTextState, value_0014) == 0x14);
static_assert(offsetof(RuntimeStandaloneTextState, value_0018) == 0x18);
static_assert(offsetof(RuntimeStandaloneTextState, low_color) == 0x1c);
static_assert(offsetof(RuntimeStandaloneTextState, high_color) == 0x20);
static_assert(offsetof(RuntimeStandaloneTextState, bounds) == 0x2c);

// GAG.EXE: 0x00411800
std::uint32_t __fastcall initialize_runtime_standalone_text(const char *text, std::uint32_t value_0014, std::uint32_t value_0018, void *font_identity, std::uint32_t low_color,
    std::uint32_t high_color, RuntimeStandaloneTextState *state);

// GAG.EXE: 0x004118C0
void __fastcall draw_runtime_standalone_text(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);

// GAG.EXE: 0x00411CF0
void __fastcall measure_runtime_generic_text(std::uint32_t *bounds, const char *text, std::uint32_t *position, std::uint32_t end, void *font_identity, std::uint32_t flags);

// GAG.EXE: 0x00411BC0
std::uint32_t __fastcall select_runtime_generic_text(std::uint32_t *bounds, const char *text, std::uint32_t *position, std::uint32_t end, std::uint32_t search_position, void *font_identity,
    std::uint32_t flags);

// GAG.EXE: 0x004112B0
void __fastcall release_runtime_generic_backend_child_lock(RuntimeGenericBackendChild *child);

// GAG.EXE: 0x0042B6A0
void __fastcall release_runtime_media_backend_lock(RuntimeMediaBackend *backend);

// GAG.EXE: 0x00411340
std::uint32_t __fastcall get_runtime_generic_backend_child_flags(void *identity);

// GAG.EXE: 0x00411360
void __fastcall clear_runtime_generic_backend_child_ready(void *identity);

// GAG.EXE: 0x00411380
void __fastcall enable_runtime_generic_backend_child_mode_200(void *identity);

// GAG.EXE: 0x004113A0
void __fastcall disable_runtime_generic_backend_child_mode_200(void *identity);

// GAG.EXE: 0x004113C0
bool __fastcall get_runtime_generic_backend_child_context(void *identity, std::uint32_t *context);

// GAG.EXE: 0x004113F0
bool __fastcall set_runtime_generic_backend_child_context(void *identity, const std::uint32_t *context);

// GAG.EXE: 0x004114D0
std::uint32_t __fastcall query_runtime_generic_backend_child_state(void *identity, std::uint32_t *state, std::uint32_t *descriptor, std::uint32_t *context);

// GAG.EXE: 0x004112C0
void *__fastcall destroy_runtime_generic_backend_child(void *identity);

struct RuntimeGenericChildSceneApi
{
    void *(__fastcall *find_available_child)(std::uint32_t maximum_end_position);
    std::uint32_t(__fastcall *build_child_state)(void *identity, std::uint32_t selection, std::uint32_t *state, std::uint32_t *descriptor, std::uint32_t *context);
    std::uint32_t(__fastcall *find_scene_index)(std::uint32_t flags);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    std::uint32_t(__fastcall *begin_scene_update)(std::int32_t identifier);
    void(__fastcall *publish_child_state)(void *identity, const std::uint32_t *state, const std::uint32_t *descriptor, std::int32_t end_offset);
    std::uint32_t(__fastcall *end_scene_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    bool(__fastcall *set_child_context)(void *identity, const std::uint32_t *context);
    bool(__fastcall *get_child_context)(void *identity, std::uint32_t *context);
    void *(__fastcall *destroy_child)(void *identity);
    std::int32_t(__fastcall *query_scene)(std::int32_t index, DisplaySceneDescriptor *descriptor, std::uint32_t *flags);
    std::uint32_t(__fastcall *release_scene)(std::int32_t identifier, std::int32_t owner);
    void(__fastcall *enable_child_mode_200)(void *identity);
};

// GAG.EXE: 0x004212E0
void __fastcall process_available_runtime_generic_children(std::uint32_t maximum_end_position);

void set_runtime_generic_child_scene_api_for_testing(const RuntimeGenericChildSceneApi &api);
void set_runtime_pointer_event_flags_for_testing(std::uint32_t flags);

// GAG.EXE: 0x00410D50
std::uint32_t __fastcall destroy_runtime_generic_backend(void *identity);

struct RuntimeSoundBufferNode
{
    void *data;
    RuntimeSoundBufferNode *next;
    std::uint32_t offset;
    std::uint32_t unknown_000c;
    std::uint32_t size;
};

static_assert(sizeof(RuntimeSoundBufferNode) == 0x14);

struct RuntimeSoundSlot
{
    std::uint32_t active;
    std::uint32_t playing;
    std::uint32_t base_state;
    std::uint32_t playback_state;
    std::uint32_t schedule_state;
    std::uint32_t fade_block_index;
    std::uint32_t fade_current;
    std::uint8_t fade_step;
    std::uint8_t unknown_001d[3];
    std::uint32_t loop_value_1;
    std::uint32_t loop_value_2;
    std::uint8_t volume;
    std::uint8_t unknown_0029;
    std::uint16_t conversion_flags;
    std::uint32_t transition_flags;
    RuntimeSoundBufferNode *buffers;
};

static_assert(sizeof(RuntimeSoundSlot) == 0x34);
static_assert(offsetof(RuntimeSoundSlot, buffers) == 0x30);

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
    std::uint32_t(__fastcall *ensure_ready)(WAVEFORMATEX *format, std::uint32_t mixer_argument);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    std::uint32_t(__fastcall *formats_equal)(const WAVEFORMATEX *left, const WAVEFORMATEX *right);
    MMRESULT(WINAPI *wave_out_reset)(HWAVEOUT wave_out);
    MMRESULT(WINAPI *wave_out_unprepare_header)(HWAVEOUT wave_out, LPWAVEHDR header, UINT bytes);
    MMRESULT(WINAPI *wave_out_close)(HWAVEOUT wave_out);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    void (*cleanup_format_buffer)();
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    std::uint32_t(__fastcall *initialize_mixer)(WAVEFORMATEX *format, std::uint32_t mixer_argument);
    std::uint32_t(__fastcall *calculate_conversion)(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, std::uint16_t *conversion_flags);
};

// GAG.EXE: 0x00401820
std::uint32_t __fastcall create_runtime_sound_handle(WAVEFORMATEX *source_format);

// GAG.EXE: 0x00402040
void __fastcall destroy_runtime_sound_handle(std::uint32_t handle);

// GAG.EXE: 0x00401BB0
std::uint32_t __fastcall queue_runtime_sound_data(std::uint32_t handle, void *data, std::uint32_t size, std::int32_t replace);

// GAG.EXE: 0x00401CD0
std::uint32_t __fastcall start_runtime_sound(std::uint32_t handle, std::int32_t reset_timing);

// GAG.EXE: 0x00401D50
std::uint32_t __fastcall stop_runtime_sound(std::uint32_t handle, std::int32_t reset_timing);

// GAG.EXE: 0x00403380
void __fastcall set_runtime_sound_loop_value(std::uint32_t handle, std::uint32_t value);

// GAG.EXE: 0x004033E0
RuntimeSoundSlot *__fastcall get_runtime_sound_slot(std::uint32_t handle);

// GAG.EXE: 0x00401DE0
std::uint32_t __fastcall fade_out_runtime_sound(std::uint32_t handle, std::int32_t duration_ms, std::int32_t reset_timing);

// GAG.EXE: 0x00401F10
std::uint32_t __fastcall fade_in_runtime_sound(std::uint32_t handle, std::int32_t duration_ms, std::int32_t reset_timing);

// GAG.EXE: 0x00403310
std::uint32_t __fastcall set_runtime_sound_volume(std::uint32_t handle, std::uint8_t volume);

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
    void(__fastcall *destroy_sound)(std::uint32_t handle);
    void (*cleanup_format_buffer)();
};

// GAG.EXE: 0x00401190
std::uint32_t shutdown_runtime_sound();

struct RuntimeSoundReadinessApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    void(WINAPI *sleep)(DWORD milliseconds);
    std::uint32_t(__fastcall *initialize_mixer)(WAVEFORMATEX *format, std::uint32_t mixer_argument);
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
std::uint32_t __fastcall pause_runtime_sound_output(std::int32_t close_output);

// GAG.EXE: 0x004016D0
std::uint32_t resume_runtime_sound_output();

void set_runtime_sound_pause_resume_api_for_testing(const RuntimeSoundPauseResumeApi &api);
void set_runtime_sound_pause_resume_state_for_testing(std::uint32_t toggle_state, std::uint32_t mixing_suppressed, std::uint32_t output_initialized, std::uint32_t output_ready, HANDLE thread,
    DWORD thread_id, HWND window, std::uint32_t fault);
void get_runtime_sound_pause_resume_state_for_testing(std::uint32_t *toggle_state, std::uint32_t *mixing_suppressed, std::uint32_t *output_initialized, HANDLE *thread, DWORD *thread_id);

// GAG.EXE: 0x004010B0
std::uint32_t __fastcall ensure_runtime_sound_ready(WAVEFORMATEX *format, std::uint32_t mixer_argument);

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
    std::uint8_t *data;
};

static_assert(sizeof(RuntimeSoundOutputBlock) == 0x0c);

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
void __fastcall initialize_runtime_sound_class(HINSTANCE instance);

// GAG.EXE: 0x004012C0
std::uint32_t __fastcall runtime_wave_formats_equal(const WAVEFORMATEX *left, const WAVEFORMATEX *right);

// GAG.EXE: 0x00403410
std::uint32_t __fastcall calculate_runtime_wave_conversion(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, std::uint16_t *conversion_flags);

struct RuntimeSoundFormatCleanupApi
{
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

// GAG.EXE: 0x00401300
void cleanup_runtime_sound_format_buffer();


struct RuntimeSceneSwitchApi
{
    RuntimeLockRecord *(__fastcall *acquire)(void *identity);
    void(__fastcall *release)(RuntimeLockRecord *record);
    std::uint32_t(__fastcall *offset_scene)(std::int32_t identifier, std::int32_t x_delta, std::int32_t y_delta);
};

struct RuntimeDisplayResetApi
{
    void(__fastcall *switch_scene)(void *identity);
    void(__fastcall *set_script_flags)(std::uint32_t mask, int enabled);
    void (*reset_transient_indices)();
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    std::uint32_t(__fastcall *release_scene)(std::int32_t identifier, std::int32_t owner);
};

struct RuntimeDisplayShutdownApi
{
    RuntimeNamedNode *(__fastcall *get_named_node)(const char *name);
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    std::uint32_t(__fastcall *release_scene)(std::int32_t identifier, std::int32_t owner);
    std::uint32_t (*shutdown_host)();
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
    std::uint32_t(__fastcall *destroy_host)(AsyncFileHost *host);
    AsyncFileHost *(__fastcall *create_host)(const char *root, std::uint32_t requested_bytes, std::int32_t mode);
    void(__fastcall *set_host_mode)(AsyncFileHost *host, std::int32_t mode);
    std::uint32_t(__fastcall *close_archive)(CdfArchive *archive);
};

struct ArchiveReadSpeedApi
{
    std::uint32_t (*initialize_async)();
    std::uint32_t(__fastcall *extract_drive_prefix)(char *destination, const char *source);
    AsyncFileHost *(__fastcall *create_host)(const char *root, std::uint32_t requested_bytes, std::int32_t mode);
    AsyncFileRecord *(__fastcall *open_record)(AsyncFileHost *host, const char *path, std::uint32_t start_offset, std::uint32_t end_offset, std::uint32_t flags);
    std::uint32_t(__fastcall *get_size)(AsyncFileRecord *record);
    std::uint32_t(__fastcall *read_record)(AsyncFileRecord *record, void *destination, std::uint32_t bytes, std::uint32_t *bytes_read, std::int32_t force_host_buffer);
    DWORD(WINAPI *get_time)();
    std::uint32_t(__fastcall *close_record)(AsyncFileRecord *record);
    std::uint32_t(__fastcall *destroy_host)(AsyncFileHost *host);
};

struct RuntimeResourceTypeApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(__fastcall *find_cache_entry)(void *parent_identity, const char *name);
    void(__fastcall *update_host)(const char *path, std::int32_t reset);
    HANDLE(__fastcall *open_file)(const char *path);
    BOOL(WINAPI *read_file)(HANDLE file, LPVOID buffer, DWORD bytes, LPDWORD bytes_read, LPOVERLAPPED overlapped);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    std::uint8_t(__fastcall *get_archive_flags)(CdfArchive *archive, const char *name);
};

struct RuntimeCdfStreamApi
{
    int(WINAPI *compare_names)(LPCSTR left, LPCSTR right);
    AsyncFileRecord *(__fastcall *duplicate_record)(AsyncFileRecord *identity, std::uint32_t start_offset, std::uint32_t end_offset, std::uint32_t flags);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG high_distance, DWORD method);
};

#pragma pack(push, 1)
struct ArchiveCommentDialogState
{
    void *value_0000;
    const char *directory;
    char file_name[0x20];
    char extension[0x58];
    char *output_1;
    char *output_2;
    std::uint32_t maximum_identifier;
    std::uint32_t comment_count;
    std::uint32_t comment_capacity;
    char *archive_paths;
};
#pragma pack(pop)
static_assert(sizeof(ArchiveCommentDialogState) == 0x98);
static_assert(offsetof(ArchiveCommentDialogState, directory) == 0x04);
static_assert(offsetof(ArchiveCommentDialogState, extension) == 0x28);
static_assert(offsetof(ArchiveCommentDialogState, maximum_identifier) == 0x88);
static_assert(offsetof(ArchiveCommentDialogState, comment_count) == 0x8c);
static_assert(offsetof(ArchiveCommentDialogState, comment_capacity) == 0x90);
static_assert(offsetof(ArchiveCommentDialogState, archive_paths) == 0x94);

struct ArchiveCommentEnumerationApi
{
    HANDLE(WINAPI *find_first)(LPCSTR pattern, LPWIN32_FIND_DATAA data);
    BOOL(WINAPI *find_next)(HANDLE find, LPWIN32_FIND_DATAA data);
    BOOL(WINAPI *find_close)(HANDLE find);
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    CdfArchive *(__fastcall *open_archive)(const char *path, std::int32_t alternate_stream);
    std::uint32_t(__fastcall *get_error)(CdfArchive *archive);
    std::uint32_t(__fastcall *get_entry_size)(CdfArchive *archive, std::uint8_t selector, const char *name);
    int(__fastcall *read_entry)(CdfArchive *archive, std::uint8_t selector, const char *name, void *destination);
    std::uint32_t(__fastcall *close_archive)(CdfArchive *archive);
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
    std::uint32_t(__fastcall *enumerate_comments)(ArchiveCommentDialogState *state, HWND listbox);
    int(WINAPI *message_box)(HWND window, LPCSTR text, LPCSTR caption, UINT type);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct ArchiveCommentDialogLaunchApi
{
    void(__cdecl *split_path)(const char *path, char *drive, char *directory, char *file_name, char *extension);
    INT_PTR(WINAPI *dialog_box)(HINSTANCE instance, LPCSTR template_name, HWND parent, DLGPROC procedure, LPARAM parameter);
};

// GAG.EXE: 0x004182A0
std::uint32_t __fastcall enumerate_archive_comments(ArchiveCommentDialogState *state, HWND listbox);
void set_archive_comment_enumeration_api_for_testing(const ArchiveCommentEnumerationApi &api);

// GAG.EXE: 0x00418560
INT_PTR CALLBACK archive_comment_dialog_procedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

// GAG.EXE: 0x00417550
INT_PTR __fastcall run_archive_comment_dialog(HWND parent, const char *directory, const char *path, char *output);

// GAG.EXE: 0x004188A0
INT_PTR CALLBACK archive_selection_dialog_procedure(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam);

// GAG.EXE: 0x004175F0
INT_PTR __fastcall run_archive_selection_dialog(HWND parent, const char *directory, const char *path, void *initial_value, char *output_path, char *output_name);

void set_archive_comment_dialog_api_for_testing(const ArchiveCommentDialogApi &api);
void set_archive_comment_dialog_launch_api_for_testing(const ArchiveCommentDialogLaunchApi &api);

struct RuntimeResourceLoadApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(__fastcall *find_cache_entry)(void *parent_identity, const char *name);
    AsyncFileRecord *(__fastcall *open_async_record)(AsyncFileHost *host, const char *path, std::uint32_t start_offset, std::uint32_t end_offset, std::uint32_t flags);
    std::uint32_t(__fastcall *get_async_size)(AsyncFileRecord *record);
    std::int32_t(__fastcall *activate_loading_scene)(const char *name);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    std::uint32_t(__fastcall *read_async_record)(AsyncFileRecord *record, void *destination, std::uint32_t bytes, std::uint32_t *bytes_read, std::int32_t force_host_buffer);
    void(__fastcall *deactivate_loading_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    RuntimeResourceCacheEntry *(__fastcall *get_or_create_cache_entry)(void *parent_identity, const char *name);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    std::uint8_t(__fastcall *get_archive_flags)(CdfArchive *archive, const char *name);
    std::uint32_t(__fastcall *get_archive_size)(CdfArchive *archive, std::uint8_t selector, const char *name);
    void *(__fastcall *open_archive_stream)(CdfArchive *archive, const char *name);
    int(__fastcall *read_archive_entry)(CdfArchive *archive, std::uint8_t selector, const char *name, void *destination);
    void(__fastcall *set_script_flags)(std::uint32_t flags, int enabled);
    void(WINAPI *sleep)(DWORD milliseconds);
};

struct AsyncFileRecord;

struct AsyncFileHost
{
    AsyncFileHost *self;
    std::uint32_t flags;
    AsyncFileHost *next;
    std::int32_t mode;
    CRITICAL_SECTION primary_lock;
    CRITICAL_SECTION secondary_lock;
    HANDLE thread;
    DWORD bytes_per_sector;
    DWORD sectors_per_cluster;
    std::uint32_t file_offset;
    HANDLE file;
    std::uint32_t file_size;
    std::uint32_t remaining_size;
    std::uint32_t start_offset;
    std::uint32_t end_offset;
    std::uint32_t current_offset;
    void *buffer_start_cursor;
    std::uint32_t buffered_bytes;
    std::uint32_t available_bytes;
    std::uint32_t buffer_size;
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
    std::uint32_t flags;
    AsyncFileRecord *next;
    HANDLE file;
    std::uint32_t file_size;
    std::uint32_t remaining_size;
    std::uint32_t start_offset;
    std::uint32_t end_offset;
    std::uint32_t current_offset;
    DWORD timestamp;
    void *buffer;
    void *buffer_cursor;
    std::uint32_t buffered_bytes;
    std::uint32_t previous_offset;
    std::uint32_t next_offset;
    AsyncFileHost *host;
};

static_assert(sizeof(AsyncFileHost) == 0x90);
static_assert(offsetof(AsyncFileHost, bytes_per_sector) == 0x44);
static_assert(offsetof(AsyncFileHost, sectors_per_cluster) == 0x48);
static_assert(offsetof(AsyncFileHost, active_file) == 0x88);
static_assert(offsetof(AsyncFileHost, files) == 0x8c);
static_assert(sizeof(AsyncFileRecord) == 0x40);
static_assert(offsetof(AsyncFileRecord, file) == 0x0c);
static_assert(offsetof(AsyncFileRecord, host) == 0x3c);

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

static_assert(offsetof(ScriptObjectContainer, next) == 0x24);
static_assert(sizeof(ScriptObjectContainer) == 0x1b4);
static_assert(offsetof(ScriptObjectContainer, identity) == 0x20);
static_assert(offsetof(ScriptObjectContainer, current_mask) == 0x28);
static_assert(offsetof(ScriptObjectContainer, required_mask) == 0x2c);
static_assert(offsetof(ScriptObjectContainer, slot_count) == 0x30);
static_assert(offsetof(ScriptObjectContainer, slots) == 0x34);
static_assert(offsetof(ScriptObjectSlot, active_field_mask) == 4);
static_assert(offsetof(ScriptObjectSlot, field_mask) == 8);

struct RuntimeCommandDefinition
{
    char name[0x20];
    RuntimeVisualObject *visual_object;
    std::uint32_t flags;
};

static_assert(sizeof(RuntimeCommandDefinition) == 0x28);
static_assert(offsetof(RuntimeCommandDefinition, visual_object) == 0x20);
static_assert(offsetof(RuntimeCommandDefinition, flags) == 0x24);

struct ScriptRuntimeRoot
{
    ScriptRuntimeRoot *self;
    std::uint32_t flags;
    std::uint32_t palette_flags;
    std::uint32_t event_records[32][16];
    std::uint32_t transient_index_1;
    std::uint32_t transient_index_2;
    void(__fastcall *set_property)(std::uint32_t operation, std::int32_t argument, RuntimeGenericResourceNode *node);
    void(__fastcall *get_property)(std::uint32_t operation, void **resource_data, void **resource_metadata);
    HANDLE heap;
    std::uint32_t parser_integer_0820;
    std::uint32_t state_value_0824;
    char language[0x20];
    char parser_value_0848[0x20];
    char parser_text_0868[0x104];
    char default_auxiliary_names[0x104];
    std::uint32_t command_definition_count;
    RuntimeCommandDefinition command_definitions[32];
    RuntimeGenericResourceNode *generic_resources;
    RuntimeTreeNode *runtime_tree;
    ScriptObjectState *objects;
    RuntimeVisualObject *visual_objects;
    RuntimeNamedNode *runtime_nodes;
    RuntimeFixedNameListNode *fixed_name_nodes;
    RuntimePlanNode *plan_nodes;
    RuntimeTreeLink7C *global_link_007c_head;
    RuntimeTreeLink84 *global_link_0084_head;
    ScriptObjectContainer *containers;
    RuntimeTreeLink8C *global_link_008c_head;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_links;
    RuntimeTreeSceneLink *global_scene_links;
    RuntimePlanNode *plan_terminal;
    RuntimeTreeLink7C *global_link_007c_tail;
    RuntimeTreeLink84 *global_link_0084_tail;
    ScriptObjectContainer *container_tail;
    RuntimeTreeLink8C *global_link_008c_tail;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_link_tail;
    RuntimeTreeSceneLink *global_scene_link_tail;
    ScriptTextBuffer *serialized_script;
};

static_assert(offsetof(ScriptRuntimeRoot, objects) == 0xf7c);
static_assert(offsetof(ScriptRuntimeRoot, self) == 0);
static_assert(offsetof(ScriptRuntimeRoot, visual_objects) == 0xf80);
static_assert(offsetof(ScriptRuntimeRoot, containers) == 0xf98);
static_assert(offsetof(ScriptRuntimeRoot, flags) == 4);
static_assert(offsetof(ScriptRuntimeRoot, palette_flags) == 8);
static_assert(offsetof(ScriptRuntimeRoot, transient_index_1) == 0x80c);
static_assert(offsetof(ScriptRuntimeRoot, event_records) == 0x0c);
static_assert(offsetof(ScriptRuntimeRoot, transient_index_2) == 0x810);
static_assert(offsetof(ScriptRuntimeRoot, runtime_nodes) == 0xf84);
static_assert(offsetof(ScriptRuntimeRoot, fixed_name_nodes) == 0xf88);
static_assert(offsetof(ScriptRuntimeRoot, heap) == 0x81c);
static_assert(offsetof(ScriptRuntimeRoot, language) == 0x828);
static_assert(offsetof(ScriptRuntimeRoot, parser_integer_0820) == 0x820);
static_assert(offsetof(ScriptRuntimeRoot, state_value_0824) == 0x824);
static_assert(offsetof(ScriptRuntimeRoot, parser_value_0848) == 0x848);
static_assert(offsetof(ScriptRuntimeRoot, parser_text_0868) == 0x868);
static_assert(offsetof(ScriptRuntimeRoot, default_auxiliary_names) == 0x96c);
static_assert(offsetof(ScriptRuntimeRoot, plan_nodes) == 0xf8c);
static_assert(offsetof(ScriptRuntimeRoot, plan_terminal) == 0xfa8);
static_assert(offsetof(ScriptRuntimeRoot, global_link_007c_head) == 0xf90);
static_assert(offsetof(ScriptRuntimeRoot, global_link_007c_tail) == 0xfac);
static_assert(offsetof(ScriptRuntimeRoot, global_link_0084_head) == 0xf94);
static_assert(offsetof(ScriptRuntimeRoot, global_link_0084_tail) == 0xfb0);
static_assert(offsetof(ScriptRuntimeRoot, global_link_008c_head) == 0xf9c);
static_assert(offsetof(ScriptRuntimeRoot, global_link_008c_tail) == 0xfb8);
static_assert(offsetof(ScriptRuntimeRoot, container_tail) == 0xfb4);
static_assert(offsetof(ScriptRuntimeRoot, global_secondary_resource_links) == 0xfa0);
static_assert(offsetof(ScriptRuntimeRoot, global_scene_links) == 0xfa4);
static_assert(offsetof(ScriptRuntimeRoot, global_secondary_resource_link_tail) == 0xfbc);
static_assert(offsetof(ScriptRuntimeRoot, global_scene_link_tail) == 0xfc0);
static_assert(offsetof(ScriptRuntimeRoot, serialized_script) == 0xfc4);
static_assert(offsetof(ScriptRuntimeRoot, runtime_tree) == 0xf78);
static_assert(offsetof(ScriptRuntimeRoot, set_property) == 0x814);
static_assert(offsetof(ScriptRuntimeRoot, get_property) == 0x818);
static_assert(offsetof(ScriptRuntimeRoot, generic_resources) == 0xf74);
static_assert(offsetof(ScriptRuntimeRoot, command_definition_count) == 0xa70);
static_assert(offsetof(ScriptRuntimeRoot, command_definitions) == 0xa74);
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, command_definitions) + offsetof(RuntimeCommandDefinition, visual_object) == 0x1444 + offsetof(RuntimeSceneSlot, visual_object));
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, command_definitions) + offsetof(RuntimeCommandDefinition, flags) + 2 == 0x1444 + offsetof(RuntimeSceneSlot, flags));
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, command_definitions) + sizeof(RuntimeCommandDefinition) == 0x1444 + offsetof(RuntimeSceneSlot, name));
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, generic_resources) == 0x1444 + 31 * sizeof(RuntimeSceneSlot) + offsetof(RuntimeSceneSlot, name));
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, global_link_0084_head) == 0x1944);
static_assert(0x9b0 + offsetof(ScriptRuntimeRoot, serialized_script) + sizeof(ScriptTextBuffer *) == 0x1978);

// GAG.EXE: 0x004050B0
RuntimeGenericResourceNode *__fastcall find_runtime_generic_resource(void *identity);

// GAG.EXE: 0x00405080
void remove_all_runtime_generic_resources();

// GAG.EXE: 0x004050E0
void __fastcall set_runtime_generic_resource_position(void *identity, std::uint32_t position);

// GAG.EXE: 0x00405110
std::uint32_t __fastcall read_runtime_generic_resource_token(void *identity, char *output, std::uint32_t capacity, std::uint8_t delimiter);

struct RuntimeGenericResourceLoadApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

// GAG.EXE: 0x00404EE0
RuntimeGenericResourceNode *__fastcall find_or_load_runtime_generic_resource(const char *resource_name);

void set_runtime_generic_resource_load_api_for_testing(const RuntimeGenericResourceLoadApi &api);

struct RuntimeTreeParserContext
{
    RuntimeTreeNode *owner;
    char *name_pointer;
    char *creation_text_pointer;
    char *scratch_text_pointer;
    std::uint8_t unknown_0010[4];
    RuntimeGenericResourceNode *resource;
    void *resource_data;
    void *resource_metadata;
    std::uint32_t start_offset;
    std::uint32_t cursor;
    char creation_text[0x104];
    char scratch_text[0x104];
    char name[0x20];
    RuntimeTreeParserContext *next;
};

static_assert(sizeof(RuntimeTreeParserContext) == 0x254);
static_assert(offsetof(RuntimeTreeParserContext, resource) == 0x14);
static_assert(offsetof(RuntimeTreeParserContext, creation_text) == 0x28);
static_assert(offsetof(RuntimeTreeParserContext, scratch_text) == 0x12c);
static_assert(offsetof(RuntimeTreeParserContext, name) == 0x230);
static_assert(offsetof(RuntimeTreeParserContext, next) == 0x250);

struct RuntimeTreeParserContextApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

// GAG.EXE: 0x00405210
RuntimeTreeParserContext *__fastcall find_or_create_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, std::uint32_t start_offset,
    const char *creation_text);

void set_runtime_tree_parser_context_api_for_testing(const RuntimeTreeParserContextApi &api);

struct RuntimeTreeParserReleaseApi
{
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void(__fastcall *remove_resource)(void *identity);
};

// GAG.EXE: 0x004052F0
void __fastcall release_runtime_tree_parser_contexts(RuntimeTreeNode *owner);

// GAG.EXE: 0x00405350
RuntimeTreeParserContext *__fastcall find_existing_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name);

void set_runtime_tree_parser_release_api_for_testing(const RuntimeTreeParserReleaseApi &api);

struct RuntimeTreeCreationApi
{
    RuntimeTreeNode *(__fastcall *find_node)(void *identity);
    RuntimeGenericResourceNode *(__fastcall *find_resource)(void *identity);
    void *(__fastcall *find_root_by_name)(const void *name);
    RuntimeTreeNode *(__fastcall *find_ancestor_root)(void *identity);
    void *(__fastcall *find_descendant_by_name)(void *root_identity, const void *name);
    int(__fastcall *find_section)(const char *section_name, const char *text, int text_length);
    int(__fastcall *find_property)(char *value, const char *property_name, const char *text, std::uint32_t text_length, std::uint32_t start_offset);
    RuntimeTreeNode *(__fastcall *begin_enumeration)(void *identity);
    RuntimeTreeNode *(__fastcall *next_enumeration)(RuntimeTreeNode *root);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    RuntimeTreeParserContext *(
        __fastcall *create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, std::uint32_t start_offset, const char *creation_text);
    void(__fastcall *remove_resource)(void *identity);
    RuntimeTreeNode *(__fastcall *dispatch_parser)(RuntimeTreeParserContext *context);
    void(__fastcall *activate_node)(RuntimeTreeNode *node);
};

// GAG.EXE: 0x004056C0
RuntimeTreeNode *__fastcall dispatch_runtime_tree_parser(RuntimeTreeParserContext *context);

// GAG.EXE: 0x00405410
RuntimeTreeNode *__fastcall create_runtime_tree_node(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);

void set_runtime_tree_creation_api_for_testing(const RuntimeTreeCreationApi &api);

struct RuntimeTreeJumpApi
{
    std::uint32_t(__fastcall *parse_property)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_name)(ScriptParserState *parser, char *name, std::uint32_t capacity);
    void(__fastcall *synchronize_owner)(RuntimeTreeNode *owner);
    RuntimeGenericResourceNode *(__fastcall *find_or_load_resource)(const char *name);
    RuntimeTreeNode *(__fastcall *create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
};

// GAG.EXE: 0x00405D00
RuntimeTreeNode *__fastcall find_and_create_runtime_tree_jump(ScriptParserState *parser, const char *target, std::uint32_t success_cursor);

void set_runtime_tree_jump_api_for_testing(const RuntimeTreeJumpApi &api);

struct RuntimeTreeConditionalCreateApi
{
    bool(__fastcall *compare_field)(const char *object_name, const void *field_name, const void *value, std::int32_t value_type);
    bool(__fastcall *container_matches)(const void *name);
    RuntimeTreeNode *(__fastcall *find_descendant)(void *root_identity, const void *name);
    RuntimeGenericResourceNode *(__fastcall *load_resource)(const char *name);
    RuntimeTreeNode *(__fastcall *create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
    RuntimeTreeNode *(__fastcall *destroy_node)(void *identity, void *replacement_identity);
};

// GAG.EXE: 0x00406CB0
RuntimeTreeNode *__fastcall update_conditional_runtime_tree(ScriptParserState *parser);

// GAG.EXE: 0x00406EA0
RuntimeTreeNode *__fastcall create_conditional_runtime_tree(ScriptParserState *parser);

void set_runtime_tree_conditional_create_api_for_testing(const RuntimeTreeConditionalCreateApi &api);

struct RuntimeTreeParserResetApi
{
    std::uint32_t(__fastcall *parse_property)(ScriptParserState *parser);
    RuntimeTreeNode *(__fastcall *resolve_included_tree)(ScriptParserState *parser);
    RuntimeTreeNode *(__fastcall *find_node)(void *identity);
};

struct RuntimeTreeParserDirectDispatchApi
{
    std::uint32_t(__fastcall *parse_property)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_object)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_link_0084)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_link_007c)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_visual)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_primary)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_container)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_command)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_named)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_link_008c)(ScriptParserState *parser);
    RuntimeTreeNode *(__fastcall *create_conditional)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_auxiliary_names)(ScriptParserState *parser);
    std::uint32_t(__fastcall *create_fixed_name)(ScriptParserState *parser);
    bool(__fastcall *parse_language)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_secondary)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_value)(ScriptParserState *parser, char *value, std::uint32_t capacity);
    std::uint32_t(__fastcall *apply_image_flags)(ScriptParserState *parser);
    std::uint32_t(__fastcall *dispatch_section)(ScriptParserState *parser);
    void(__fastcall *set_resource_position)(void *identity, std::uint32_t position);
    std::uint32_t(__fastcall *read_resource_token)(void *identity, char *output, std::uint32_t capacity, std::uint8_t delimiter);
    std::uint32_t(__fastcall *parse_scene)(ScriptParserState *parser);
    void(__fastcall *add_auxiliary_name)(RuntimeTreeNode *owner, const char *name);
    void(__fastcall *publish_links)(RuntimeTreeNode *owner);
};

void set_runtime_tree_parser_direct_dispatch_api_for_testing(const RuntimeTreeParserDirectDispatchApi &api);
void reset_runtime_tree_parser_direct_dispatch_api_for_testing();

struct RuntimeTreeParserSpecialDispatchApi
{
    std::int32_t(__fastcall *parse_integer)(ScriptParserState *parser);
    std::uint32_t(__fastcall *parse_image_flag)(ScriptParserState *parser);
    RuntimeTreeNode *(__fastcall *create_command)(ScriptParserState *parser);
    RuntimeTreeNode *(__fastcall *find_jump)(ScriptParserState *parser, const char *property_name, std::uint32_t cursor);
    bool(__fastcall *strings_equal)(const char *left, const char *right);
};

void set_runtime_tree_parser_special_dispatch_api_for_testing(const RuntimeTreeParserSpecialDispatchApi &api);
void reset_runtime_tree_parser_special_dispatch_api_for_testing();

// GAG.EXE: 0x00405E00
void __fastcall reset_runtime_tree_parser_context_recursive(ScriptParserState *parser);

// GAG.EXE: 0x00405DC0
void __fastcall reset_runtime_tree_parser_contexts(void *identity);

void set_runtime_tree_parser_reset_api_for_testing(const RuntimeTreeParserResetApi &api);

struct RuntimeTreeSectionDispatchApi
{
    RuntimeTreeNode *(__fastcall *find_node)(void *identity);
    RuntimeGenericResourceNode *(__fastcall *find_resource)(void *identity);
    int(__fastcall *find_section)(const char *section_name, const char *text, int text_length);
    RuntimeTreeParserContext *(
        __fastcall *create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, std::uint32_t start_offset, const char *creation_text);
    RuntimeTreeNode *(__fastcall *dispatch_parser)(RuntimeTreeParserContext *context);
    void(__fastcall *remove_resource)(void *identity);
};

// GAG.EXE: 0x00405380
RuntimeTreeNode *__fastcall dispatch_runtime_tree_section(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);

void set_runtime_tree_section_dispatch_api_for_testing(const RuntimeTreeSectionDispatchApi &api);

struct RuntimeTreeBasicCommandApi
{
    std::uint32_t(__fastcall *parse_value)(ScriptParserState *parser, char *value, std::uint32_t capacity);
    std::uint32_t(__fastcall *extract_parenthesized)(ScriptParserState *parser, char *text, std::uint32_t capacity);
    std::uint32_t(__fastcall *parse_scope)(ScriptParserState *parser);
    RuntimeGenericResourceNode *(__fastcall *load_resource)(const char *name);
    RuntimeTreeNode *(__fastcall *dispatch_section)(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);
    RuntimeTreeNode *(__fastcall *create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
};

// GAG.EXE: 0x00406A70
std::uint32_t __fastcall dispatch_runtime_tree_section_command(ScriptParserState *parser);

// GAG.EXE: 0x00406B90
bool __fastcall parse_runtime_language(ScriptParserState *parser);

// GAG.EXE: 0x00406C00
RuntimeTreeNode *__fastcall create_runtime_tree_command(ScriptParserState *parser);

void set_runtime_tree_basic_command_api_for_testing(const RuntimeTreeBasicCommandApi &api);

// GAG.EXE: 0x00405000
void __fastcall remove_runtime_generic_resource(void *identity);

// GAG.EXE: 0x004068C0
void __fastcall set_script_runtime_flags(std::uint32_t mask, int enabled);

// GAG.EXE: 0x0040C370
void reset_script_runtime_transient_indices();

// GAG.EXE: 0x00407810
RuntimeNamedNode *__fastcall find_runtime_named_child(void *parent_identity, void *child_identity);

// GAG.EXE: 0x00407720
RuntimeResourceCacheEntry *__fastcall find_runtime_resource_cache_entry(void *parent_identity, const char *name);

// GAG.EXE: 0x00407780
RuntimeResourceCacheEntry *__fastcall get_or_create_runtime_resource_cache_entry(void *parent_identity, const char *name);

// GAG.EXE: 0x00407860
RuntimeResourceCacheEntry *__fastcall get_or_create_runtime_child_by_data(void *parent_identity, void *data);

// GAG.EXE: 0x004078D0
void __fastcall add_script_object_to_runtime_named_node(const void *node_name, const char *object_name);

// GAG.EXE: 0x00407490
std::uint32_t __fastcall parse_runtime_named_node(ScriptParserState *parser);

// GAG.EXE: 0x00407990
void __fastcall remove_script_object_from_runtime_named_node(const void *node_name, const char *object_name);

// GAG.EXE: 0x00407C00
std::uint32_t __fastcall rotate_runtime_named_node_cursor_previous(const void *node_name, std::int32_t count);

// GAG.EXE: 0x00407C60
std::uint32_t __fastcall rotate_runtime_named_node_cursor_next(const void *node_name, std::int32_t count);

// GAG.EXE: 0x00407CC0
std::uint32_t __fastcall clear_runtime_named_node_children(const void *node_name);

// GAG.EXE: 0x00407D50
void __fastcall remove_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

// GAG.EXE: 0x00407A20
std::uint32_t __fastcall remove_runtime_named_child_by_identity(void *parent_identity, void *child_identity);

// GAG.EXE: 0x00424C50
BOOL __fastcall release_runtime_memory_resource(const char *name);

// GAG.EXE: 0x00424CC0
BOOL __fastcall release_runtime_memory_resource_by_data(void *data);

// GAG.EXE: 0x00424D30
std::uint32_t __fastcall release_runtime_streamed_resource(AsyncFileRecord *record);

// GAG.EXE: 0x00407D10
void __fastcall append_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

// GAG.EXE: 0x00407DD0
void __fastcall serialize_runtime_named_nodes(ScriptTextBuffer *buffer);

// GAG.EXE: 0x00407EE0
void purge_disabled_runtime_named_nodes();

// GAG.EXE: 0x00409370
std::uint32_t __fastcall parse_runtime_command_definition(ScriptParserState *parser);

// GAG.EXE: 0x004094C0
void __fastcall append_dual_image_flag(ScriptTextBuffer *buffer, std::uint32_t flags);

// GAG.EXE: 0x00409510
void __fastcall serialize_runtime_command_definitions(ScriptTextBuffer *buffer);

// GAG.EXE: 0x004095E0
void clear_runtime_command_definitions();

// GAG.EXE: 0x00407690
RuntimeNamedNode *__fastcall get_or_create_runtime_named_node(const char *name);

// GAG.EXE: 0x0040A7A0
bool set_runtime_plans_inactive();

// GAG.EXE: 0x0040A800
bool clear_runtime_plans_inactive();

// GAG.EXE: 0x0040CD60
RuntimeTreeNode *__fastcall find_runtime_tree_node(RuntimeTreeNode *root, void *identity);

// GAG.EXE: 0x004065E0
RuntimeTreeNode *__fastcall find_runtime_tree_node_by_identity(void *identity);

// GAG.EXE: 0x004097D0
void *__fastcall find_last_runtime_tree_scene_link(RuntimeTreeNode *root);

// GAG.EXE: 0x00409B60
void *__fastcall find_last_runtime_tree_secondary_resource_link(RuntimeTreeNode *root);

// GAG.EXE: 0x0040A500
void *__fastcall find_last_runtime_tree_primary_resource_link(RuntimeTreeNode *root);

// GAG.EXE: 0x00406860
void *__fastcall find_last_runtime_scene_link_by_identity(void *identity);

// GAG.EXE: 0x00406880
void *__fastcall find_last_runtime_primary_resource_link_by_identity(void *identity);

// GAG.EXE: 0x004068A0
void *__fastcall find_last_runtime_secondary_resource_link_by_identity(void *identity);

struct RuntimeTreeSceneLink
{
    char name[0x20];
    void *identity;
    std::uint32_t z;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t flags;
    std::int32_t scene_identifier;
    RuntimeTreeSceneLink *next;
};

static_assert(sizeof(RuntimeTreeSceneLink) == 0x44);

struct RuntimeTreePrimaryResourceLink
{
    char identifier[0x20];
    void *identity;
    RuntimeTreePrimaryResourceLink *next;
    std::uint32_t flags;
    char file_name[0x20];
    void *resource_identity;
    std::uint32_t unknown_0050;
    std::uint32_t image_flags;
    std::uint32_t loop_count;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t source_value;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t ratio_x;
    std::uint32_t ratio_y;
    RuntimeTreeSecondaryResourceLink *secondary_link;
    RuntimeFixedNameListNode *fixed_name_node;
    void *previous_resource_identity;
    std::int32_t previous_x;
    std::int32_t previous_y;
};

struct RuntimeTreeSecondaryResourceLink
{
    char name[0x20];
    void *identity;
    char file_name[0x20];
    void *resource_identity;
    RuntimeTreeSecondaryResourceLink *next;
};

static_assert(sizeof(RuntimeTreeSecondaryResourceLink) == 0x4c);

// GAG.EXE: 0x00409600
std::uint32_t __fastcall parse_runtime_tree_scene_link(ScriptParserState *parser);

// GAG.EXE: 0x00409A80
std::uint32_t __fastcall parse_runtime_tree_secondary_resource_link(ScriptParserState *parser);

struct RuntimeTreeLink84
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink84 *next;
    std::uint32_t unknown_0028;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t state_003c;
    union
    {
        std::uint32_t command_mask;
        std::uint32_t value_0040;
    };
    std::uint32_t primary_command_bit;
    std::uint8_t unknown_0048[4];
    union
    {
        std::uint32_t parameter;
        std::uint32_t value_004c;
    };
    union
    {
        RuntimeVisualObject *mouse_visual;
        std::uint32_t value_0050;
    };
    std::uint32_t value_0054;
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
    std::uint8_t unknown_0028[0x10];
    ScriptParserState parser;
    RuntimeGenericBackendChild *backend_child;
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    std::uint32_t wait_deadline;
    std::uint32_t owner_flags;
    std::uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    std::uint32_t unknown_0084;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t width;
    std::uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    std::int32_t random_minimum;
    std::int32_t random_maximum;
    std::uint32_t unknown_00a8;
    std::uint32_t flags;
    std::uint32_t unknown_00b0;
};

struct RuntimeTreeLink8C
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink8C *next;
    std::uint32_t time;
    std::uint32_t flags;
    std::int32_t x;
    std::int32_t y;
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t unknown_0040[0x0c];
    std::int32_t line_first;
    std::int32_t line_second;
};

static_assert(sizeof(RuntimeTreeSceneLink) == 0x44);
static_assert(sizeof(RuntimeTreePrimaryResourceLink) == 0x8c);
static_assert(sizeof(RuntimeTreeSecondaryResourceLink) == 0x4c);
static_assert(sizeof(RuntimeTreeLink84) == 0x68);
static_assert(sizeof(RuntimeTreeLink7C) == 0xb4);
static_assert(offsetof(RuntimeTreeLink7C, next) == 0x24);
static_assert(offsetof(RuntimeTreeLink7C, parser) == 0x38);
static_assert(offsetof(RuntimeTreeLink7C, backend_child) == 0x60);
static_assert(offsetof(RuntimeTreeLink7C, fixed_resource_identity) == 0x64);
static_assert(offsetof(RuntimeTreeLink7C, secondary_resource_identity) == 0x68);
static_assert(offsetof(RuntimeTreeLink7C, wait_deadline) == 0x6c);
static_assert(offsetof(RuntimeTreeLink7C, owner_flags) == 0x70);
static_assert(offsetof(RuntimeTreeLink7C, command_bit) == 0x74);
static_assert(offsetof(RuntimeTreeLink7C, source_object) == 0x78);
static_assert(offsetof(RuntimeTreeLink7C, zone_link) == 0x80);
static_assert(offsetof(RuntimeTreeLink7C, x) == 0x88);
static_assert(offsetof(RuntimeTreeLink7C, primary_resource) == 0x98);
static_assert(offsetof(RuntimeTreeLink7C, random_minimum) == 0xa0);
static_assert(offsetof(RuntimeTreeLink7C, flags) == 0xac);
static_assert(sizeof(RuntimeTreeLink8C) == 0x54);
static_assert(offsetof(RuntimeTreeLink84, next) == 0x24);
static_assert(offsetof(RuntimeTreeLink84, identity) == 0x20);
static_assert(offsetof(RuntimeTreeLink84, x) == 0x2c);
static_assert(offsetof(RuntimeTreeLink84, command_mask) == 0x40);
static_assert(offsetof(RuntimeTreeLink84, parameter) == 0x4c);
static_assert(offsetof(RuntimeTreeLink84, owner_object) == 0x58);
static_assert(offsetof(RuntimeTreeLink84, previous_identity_005c) == 0x64);
static_assert(offsetof(RuntimeTreeLink8C, next) == 0x24);
static_assert(offsetof(RuntimeTreeLink8C, time) == 0x28);
static_assert(offsetof(RuntimeTreeLink8C, x) == 0x30);
static_assert(offsetof(RuntimeTreeLink8C, line_first) == 0x4c);
static_assert(offsetof(RuntimeTreeSceneLink, scene_identifier) == 0x3c);
static_assert(offsetof(RuntimeTreeSceneLink, next) == 0x40);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, next) == 0x24);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, flags) == 0x28);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, file_name) == 0x2c);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, resource_identity) == 0x4c);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, image_flags) == 0x54);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, loop_count) == 0x58);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, x) == 0x5c);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, y) == 0x60);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, source_value) == 0x64);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, ratio_x) == 0x70);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, secondary_link) == 0x78);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, previous_resource_identity) == 0x80);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, previous_x) == 0x84);
static_assert(offsetof(RuntimeTreePrimaryResourceLink, previous_y) == 0x88);
static_assert(offsetof(RuntimeTreeSecondaryResourceLink, resource_identity) == 0x44);
static_assert(offsetof(RuntimeTreeSecondaryResourceLink, next) == 0x48);

// GAG.EXE: 0x00409A40
RuntimeTreeSceneLink *__fastcall find_global_runtime_tree_scene_link_by_name(const void *name);

// GAG.EXE: 0x00409830
RuntimeTreeSceneLink *__fastcall find_runtime_tree_scene_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x00409880
void __fastcall insert_runtime_tree_scene_link(RuntimeTreeNode *node, RuntimeTreeSceneLink *link);

// GAG.EXE: 0x00409920
void __fastcall remove_runtime_tree_scene_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x00409E00
RuntimeTreeSecondaryResourceLink *__fastcall find_global_runtime_tree_secondary_resource_link_by_name(const void *name);

// GAG.EXE: 0x00409BC0
RuntimeTreeSecondaryResourceLink *__fastcall find_runtime_tree_secondary_resource_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x00409C10
void __fastcall insert_runtime_tree_secondary_resource_link(RuntimeTreeNode *node, RuntimeTreeSecondaryResourceLink *link);

// GAG.EXE: 0x00409CB0
void __fastcall remove_runtime_tree_secondary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040A560
RuntimeTreePrimaryResourceLink *__fastcall find_runtime_tree_primary_resource_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040A5B0
void __fastcall insert_runtime_tree_primary_resource_link(RuntimeTreeNode *node, RuntimeTreePrimaryResourceLink *link);

// GAG.EXE: 0x0040A650
void __fastcall remove_runtime_tree_primary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040A860
void __fastcall update_runtime_tree_primary_resource_link(void *tree_identity, void *link_identity, const void *name, std::int32_t x_delta, std::int32_t y_delta, std::uint32_t value_0054);

// GAG.EXE: 0x0040A920
void __fastcall append_three_digit_decimal_suffix(const char *prefix, std::uint32_t value, char *output);

// GAG.EXE: 0x0040A3C0
void *__fastcall create_or_update_runtime_tree_primary_resource_link(void *tree_identity, const void *identifier, const void *file_name, std::int32_t source_value, std::int32_t x_delta,
    std::int32_t y_delta, std::uint32_t image_flags);

// GAG.EXE: 0x0040AE40
void *__fastcall create_or_update_runtime_tree_link_0084(void *tree_identity, const void *name, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t value_0050,
    void *identity_0058, void *identity_005c, std::uint32_t value_0054, std::uint32_t value_0040, std::uint32_t value_004c);

// GAG.EXE: 0x00409E50
std::uint32_t __fastcall parse_runtime_tree_primary_resource_link(ScriptParserState *parser);

// GAG.EXE: 0x0040A990
RuntimeTreePrimaryResourceLink *__fastcall find_global_runtime_tree_primary_resource_link_by_name(const void *name);

// GAG.EXE: 0x0040AAC0
std::uint32_t __fastcall parse_runtime_tree_link_0084(ScriptParserState *parser);

// GAG.EXE: 0x0040AFE0
RuntimeTreeLink84 *__fastcall find_last_runtime_tree_link_0084(RuntimeTreeNode *root);

// GAG.EXE: 0x0040B040
RuntimeTreeLink84 *__fastcall find_runtime_tree_link_0084_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040B090
void __fastcall insert_runtime_tree_link_0084(RuntimeTreeNode *node, RuntimeTreeLink84 *link);

// GAG.EXE: 0x0040B130
void __fastcall remove_runtime_tree_link_0084_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040B280
void __fastcall update_runtime_tree_link_0084(void *tree_identity, void *link_identity, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t value_0050,
    void *identity_0058, void *identity_005c, std::uint32_t value_0054, std::uint32_t value_0040, std::uint32_t value_004c);

// GAG.EXE: 0x0040B380
RuntimeTreeLink84 *__fastcall find_global_runtime_tree_link_0084_by_name(const void *name);

// GAG.EXE: 0x0040B3C0
RuntimeTreeLink84 *__fastcall find_global_runtime_tree_link_0084_by_identity(void *identity);

// GAG.EXE: 0x0040B3E0
std::uint32_t __fastcall parse_runtime_tree_link_008c(ScriptParserState *parser);

// GAG.EXE: 0x0040B850
std::uint32_t __fastcall parse_runtime_tree_link_007c(ScriptParserState *parser);

// GAG.EXE: 0x0040C1E0
void __fastcall seek_runtime_tree_link_007c_label(void *identity, const char *label);

// GAG.EXE: 0x0040C260
std::uint32_t __fastcall find_runtime_tree_link_007c_opcode_value(void *identity, std::uint32_t opcode, const char *value, int restore_cursor);

// GAG.EXE: 0x0040C2F0
std::uint32_t __fastcall scan_runtime_tree_link_007c_control_boundary(void *identity, std::uint32_t requested_boundary);

// GAG.EXE: 0x0040BF60
std::uint32_t __fastcall match_runtime_tree_link_007c_interaction(std::uint32_t *state, const std::uint32_t *criteria);

// GAG.EXE: 0x0040C4B0
std::uint32_t __fastcall activate_runtime_tree_link_007c(RuntimeTreeLink7C *link);

// GAG.EXE: 0x0040C570
std::uint32_t __fastcall parse_script_object_container(ScriptParserState *parser);

// GAG.EXE: 0x0040BCD0
RuntimeTreeLink7C *__fastcall find_last_runtime_tree_link_007c(RuntimeTreeNode *root);

// GAG.EXE: 0x0040BD30
RuntimeTreeLink7C *__fastcall find_runtime_tree_link_007c_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040BD80
void __fastcall insert_runtime_tree_link_007c(RuntimeTreeNode *node, RuntimeTreeLink7C *link);

// GAG.EXE: 0x0040BE20
void __fastcall remove_runtime_tree_link_007c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040C8A0
ScriptObjectContainer *__fastcall find_last_script_object_container(RuntimeTreeNode *root);

// GAG.EXE: 0x0040C900
ScriptObjectContainer *__fastcall find_script_object_container_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040C950
void __fastcall insert_script_object_container(RuntimeTreeNode *node, ScriptObjectContainer *container);

// GAG.EXE: 0x0040C9F0
void __fastcall remove_script_object_container_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040CB40
BOOL __fastcall destroy_script_object_container(ScriptObjectContainer *container);

// GAG.EXE: 0x0040CBA0
bool __fastcall script_object_container_state_matches_by_identity(void *identity);

// GAG.EXE: 0x0040CC20
bool __fastcall script_object_container_state_matches_by_name(const void *name);

// GAG.EXE: 0x0040CCB0
ScriptObjectContainer *__fastcall find_script_condition_container_by_name(const void *name);

// GAG.EXE: 0x0040B560
RuntimeTreeLink8C *__fastcall find_last_runtime_tree_link_008c(RuntimeTreeNode *root);

// GAG.EXE: 0x0040B5C0
RuntimeTreeLink8C *__fastcall find_runtime_tree_link_008c_insertion_predecessor(RuntimeTreeNode *node);

// GAG.EXE: 0x0040B610
void __fastcall insert_runtime_tree_link_008c(RuntimeTreeNode *node, RuntimeTreeLink8C *link);

// GAG.EXE: 0x0040B6B0
void __fastcall remove_runtime_tree_link_008c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

// GAG.EXE: 0x0040B800
RuntimeTreeLink8C *__fastcall find_global_runtime_tree_link_008c_by_name(const void *name);

struct RuntimeTreeDestructionApi
{
    RuntimeTreeNode *(__fastcall *resolve_tree)(void *identity);
    void(__fastcall *set_resource_state)(void *identity, std::uint32_t state);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    std::uint32_t (*stop_game_dll)();
    void (*reset_display_state)();
    void *(__fastcall *find_primary_tail)(void *identity);
    void *(__fastcall *find_secondary_tail)(void *identity);
    void *(__fastcall *find_scene_tail)(void *identity);
    std::uint32_t(__fastcall *query_scene_flags)(void *identity);
    void(__fastcall *destroy_resource_and_scene)(void *identity);
    void(__fastcall *request_resource_destruction)(void *identity);
    std::uint32_t(__fastcall *release_scene)(std::int32_t identifier, std::int32_t owner);
    void(__fastcall *set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void(__fastcall *wait_for_resource_count)(std::uint32_t count);
};

// GAG.EXE: 0x00426BD0
void __fastcall destroy_runtime_tree_resources(void *identity);

void set_runtime_tree_destruction_api_for_testing(const RuntimeTreeDestructionApi &api);
void set_runtime_tree_destruction_state_for_testing(void *pointer_root_identity, void *current_resource, std::uint32_t resource_count);

struct RuntimeResourceSceneDestructionApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    std::uint32_t(__fastcall *destroy_resource)(void *identity);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
    void(WINAPI *sleep)(DWORD milliseconds);
    void(__fastcall *update_scene_region)(std::int32_t scene_identifier, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
};

struct RuntimeResourceSceneRegionApi
{
    DisplaySceneNode *(__fastcall *lock_scene)(std::int32_t identifier);
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    std::uint32_t(__fastcall *begin_scene_update)(std::int32_t identifier);
    std::uint32_t(__fastcall *render_backend_region)(void *backend_identity, DisplayRectangle *rectangle);
    std::uint32_t(__fastcall *end_scene_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    std::uint32_t(__fastcall *update_root_scene_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, std::uint32_t callback_value);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
    void(__fastcall *unlock_scene)(std::int32_t identifier);
};

// GAG.EXE: 0x00427900
void __fastcall update_runtime_resource_scene_region(std::int32_t scene_identifier, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

void set_runtime_resource_scene_region_api_for_testing(const RuntimeResourceSceneRegionApi &api);
void set_runtime_resource_scene_region_default_for_testing(std::int32_t scene_identifier);

// GAG.EXE: 0x00417370
void __fastcall copy_runtime_bitmap_region(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);

struct RuntimeBitmapRegionRenderApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE object, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    void(__fastcall *copy_bitmap_region)(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);
};

// GAG.EXE: 0x0042B140
std::uint32_t __fastcall render_runtime_bitmap_backend_region(void *identity, DisplayRectangle *rectangle);

void set_runtime_bitmap_region_render_api_for_testing(const RuntimeBitmapRegionRenderApi &api);

struct RuntimeSceneTransitionSelectionApi
{
    int (*random)();
    void(__fastcall *apply_immediate)(std::uint32_t unused, std::uint32_t flags);
    void(__fastcall *apply_palette)(std::uint32_t value, std::uint32_t flags);
    void(__fastcall *apply_rectangle)(std::uint8_t value, std::uint32_t flags);
};

// GAG.EXE: 0x00426D50
void __fastcall select_runtime_scene_transition(std::uint32_t flags);

void set_runtime_scene_transition_selection_api_for_testing(const RuntimeSceneTransitionSelectionApi &api);
void set_runtime_scene_transition_selection_state_for_testing(std::uint32_t available_transitions, std::uint32_t palette_value, std::uint32_t rectangle_value, std::uint16_t bits_per_pixel);

struct RuntimeResourceStateApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    std::uint32_t(__fastcall *begin_scene_update)(std::int32_t identifier);
    void(__fastcall *finalize_backend)(void *identity);
    void(__fastcall *configure_palette)(RuntimeResourceObject *resource);
    std::uint32_t(__fastcall *end_scene_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void(__fastcall *clear_child_ready)(void *identity);
    void(__fastcall *enable_child_mode)(void *identity);
    void(__fastcall *disable_child_mode)(void *identity);
    void(__fastcall *select_transition)(std::uint32_t flags);
    RuntimeSoundSlot *(__fastcall *get_sound_slot)(std::uint32_t handle);
    std::uint32_t(__fastcall *queue_sound_data)(std::uint32_t handle, void *data, std::uint32_t size, std::int32_t replace);
    std::uint32_t(__fastcall *start_sound)(std::uint32_t handle, std::int32_t reset_timing);
    std::uint32_t(__fastcall *stop_sound)(std::uint32_t handle, std::int32_t reset_timing);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x00425930
void __fastcall set_runtime_resource_state(void *identity, std::uint32_t state);

void set_runtime_resource_state_api_for_testing(const RuntimeResourceStateApi &api);
void set_runtime_resource_state_globals_for_testing(void *current_resource, std::uint32_t scene_flags);

struct RuntimeImmediateSceneTransitionApi
{
    std::uint32_t(__fastcall *acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, std::uint32_t *flags);
    std::uint32_t(__fastcall *set_clip_rectangle)(DisplayRectangle *rectangle);
    std::uint32_t (*release_display_lock)();
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    std::uint32_t(__fastcall *dispatch_scene_update)(void *rectangle, std::uint32_t flags);
    void(WINAPI *sleep)(DWORD milliseconds);
    void(__fastcall *synchronize_region)(DisplayRectangle *rectangle, std::uint32_t mode);
    UINT(__fastcall *apply_palette)(const PALETTEENTRY *entries, std::uint32_t flags);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x00426E30
void __fastcall apply_immediate_runtime_scene_transition(std::uint32_t unused, std::uint32_t flags);

void set_runtime_immediate_scene_transition_api_for_testing(const RuntimeImmediateSceneTransitionApi &api);

struct RuntimePaletteSceneTransitionApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    void(__fastcall *apply_immediate)(std::uint32_t unused, std::uint32_t flags);
    std::uint32_t(__fastcall *acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, std::uint32_t *flags);
    UINT(__fastcall *apply_palette)(const PALETTEENTRY *entries, std::uint32_t flags);
    void(__fastcall *operate_surface)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);
    std::uint32_t(__fastcall *set_clip_rectangle)(DisplayRectangle *rectangle);
    std::uint32_t(__fastcall *dispatch_scene_update)(void *rectangle, std::uint32_t flags);
    std::uint32_t (*release_display_lock)();
    void(__fastcall *release_record)(RuntimeLockRecord *record);
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void(__fastcall *invalidate_framebuffer)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
};

// GAG.EXE: 0x00426F40
void __fastcall apply_palette_runtime_scene_transition(std::uint32_t step, std::uint32_t flags);

void set_runtime_palette_scene_transition_api_for_testing(const RuntimePaletteSceneTransitionApi &api);
void set_runtime_palette_scene_transition_state_for_testing(void *current_resource, std::uint32_t scene_flags, std::uint16_t width, std::uint16_t height);
const PALETTEENTRY *get_runtime_palette_scene_transition_entries_for_testing();

struct RuntimeRectangleSceneTransitionApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    void(__fastcall *apply_immediate)(std::uint32_t unused, std::uint32_t flags);
    std::uint32_t(__fastcall *acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, std::uint32_t *flags);
    std::uint32_t(__fastcall *set_clip_rectangle)(DisplayRectangle *rectangle);
    std::uint32_t (*release_display_lock)();
    void(__fastcall *operate_surface)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);
    void(__fastcall *synchronize_region)(DisplayRectangle *rectangle, std::uint32_t mode);
    UINT(__fastcall *apply_palette)(const PALETTEENTRY *entries, std::uint32_t flags);
    std::uint32_t(__fastcall *dispatch_scene_update)(void *rectangle, std::uint32_t flags);
    DWORD(WINAPI *time_get_time)();
    DWORD(WINAPI *get_tick_count)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x004272D0
void __fastcall apply_rectangle_runtime_scene_transition(std::uint8_t size, std::uint32_t flags);

void set_runtime_rectangle_scene_transition_api_for_testing(const RuntimeRectangleSceneTransitionApi &api);
void set_runtime_rectangle_scene_transition_state_for_testing(void *current_resource, std::uint16_t width, std::uint16_t height);

struct DisplayRegionSynchronizationApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *sleep)(DWORD milliseconds);
    HRESULT(WINAPI *blt_fast)(void *surface, DWORD x, DWORD y, void *source, RECT *source_rectangle, DWORD flags);
    HRESULT(WINAPI *blt)(void *surface, RECT *destination_rectangle, void *source, RECT *source_rectangle, DWORD flags, void *effects);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
    BOOL(WINAPI *pat_blt)(HDC destination, int x, int y, int width, int height, DWORD operation);
};

// GAG.EXE: 0x00414220
void __fastcall synchronize_display_region(DisplayRectangle *rectangle, std::uint32_t mode);

void set_display_region_synchronization_api_for_testing(const DisplayRegionSynchronizationApi &api);
void set_display_region_synchronization_state_for_testing(std::uint32_t flags, void *primary_surface, void *secondary_surface, HDC primary_context, HDC secondary_context);

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
std::uint32_t __fastcall begin_display_target(void **pixels, DisplayRectangle *rectangle, std::uint32_t *pitch);

void set_display_target_begin_api_for_testing(const DisplayTargetBeginApi &api);
void set_display_target_begin_state_for_testing(std::uint32_t flags, void *secondary_surface, void *pixels, std::int32_t width, std::int32_t height, DisplayMode *mode);

// GAG.EXE: 0x00425C40
void __fastcall finalize_runtime_resource_destruction(void *identity);

void set_runtime_resource_scene_destruction_api_for_testing(const RuntimeResourceSceneDestructionApi &api);

// GAG.EXE: 0x00409330
RuntimeVisualObject *__fastcall find_runtime_visual_object(const char *name);

// GAG.EXE: 0x0040C3D0
void __fastcall enqueue_runtime_event_record(const std::uint32_t *record);

// GAG.EXE: 0x0040C390
void acknowledge_current_runtime_event_record();

// GAG.EXE: 0x0040C440
std::uint32_t __fastcall read_runtime_event_record(std::uint32_t *record, std::int32_t advance);

// GAG.EXE: 0x004237F0
std::int32_t __fastcall select_pointer_region_scene(RuntimePointerRegion *region);

// GAG.EXE: 0x00407A80
std::uint32_t __fastcall synchronize_runtime_pointer_owner_slots(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);

struct RuntimePointerResourceRebuildApi
{
    RuntimeTreeNode *(__fastcall *resolve_tree)(void *identity);
    std::uint32_t(__fastcall *synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    std::uint32_t(__fastcall *query_scene_flags)(void *identity);
    void(__fastcall *finalize_destruction)(void *identity);
    void(__fastcall *request_destruction)(void *identity);
    RuntimeResourceConstructor construct_resource;
    void(__fastcall *update_position)(void *identity, std::int32_t x, std::int32_t y);
    void(__fastcall *set_comment_mode)(RuntimeTreeNode *root, int enabled);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(__fastcall *wait_for_count)(std::uint32_t count);
};

struct RuntimeTreeResourceRebuildApi
{
    RuntimeTreeNode *(__fastcall *resolve_tree)(void *identity);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    RuntimeResourceConstructor construct_resource;
    std::uint32_t(__fastcall *synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    std::uint32_t(__fastcall *query_scene_flags)(void *identity);
    void(__fastcall *request_destruction)(void *identity);
    RuntimeGenericBackendChild *(__fastcall *configure_resource)(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, std::uint32_t value, std::uint32_t flags);
    void(__fastcall *set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void(__fastcall *wait_for_count)(std::uint32_t count);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void (*reset_transient_indices)();
    void(__fastcall *set_resource_state)(void *identity, std::uint32_t state);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
};

struct RuntimeGenericChildAttachmentApi
{
    RuntimeLockRecord *(__fastcall *acquire_resource)(void *identity);
    void(__fastcall *release_resource)(RuntimeLockRecord *record);
    std::uint32_t(__fastcall *find_scene_index)(std::uint32_t candidate);
    RuntimeGenericBackendChild *(__fastcall *create_child)(void *backend_identity, void *font_identity, const std::uint32_t *context, std::uintptr_t selection, std::uint32_t flags);
    DisplaySceneNode *(__fastcall *lock_scene)(std::int32_t identifier);
    void(__fastcall *unlock_scene)(std::int32_t identifier);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    void *(__fastcall *destroy_child)(void *identity);
};

// GAG.EXE: 0x00425D50
RuntimeGenericBackendChild *__fastcall attach_runtime_generic_backend_child(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, std::uint32_t selection,
    std::uint32_t flags);

void set_runtime_generic_child_attachment_api_for_testing(const RuntimeGenericChildAttachmentApi &api);
void set_runtime_generic_child_attachment_scene_for_testing(std::int32_t identifier);

// GAG.EXE: 0x004268B0
void __fastcall rebuild_runtime_tree_resources(void *identity);

// Non-original compatibility entry retained for focused rebuild tests.
void __fastcall rebuild_runtime_tree_resources_with_loop_register(void *identity, std::int32_t loop_animation);

void set_runtime_tree_resource_rebuild_api_for_testing(const RuntimeTreeResourceRebuildApi &api);

// GAG.EXE: 0x00426700
void rebuild_runtime_pointer_resources();

// Non-original compatibility entry retained for focused rebuild tests.
void rebuild_runtime_pointer_resources_with_loop_register(std::int32_t loop_animation);

void set_runtime_pointer_resource_rebuild_api_for_testing(const RuntimePointerResourceRebuildApi &api);

// GAG.EXE: 0x00423BC0
std::uint32_t handle_runtime_left_button_up();

// GAG.EXE: 0x004238B0
std::uint32_t handle_runtime_left_button_down();

// GAG.EXE: 0x00423CA0
std::uint32_t handle_runtime_right_button_down();

// Non-original helper exposing the original hidden EDI input for deterministic testing.
std::uint32_t handle_runtime_right_button_down_with_loop_register(std::int32_t loop_animation);

// GAG.EXE: 0x00423FA0
std::uint32_t __fastcall update_runtime_pointer_region(std::int32_t x, std::int32_t y);

struct RuntimePointerRefreshApi
{
    std::uint32_t(__fastcall *update_region)(std::int32_t x, std::int32_t y);
};

// GAG.EXE: 0x004236C0
std::uint32_t refresh_runtime_pointer_region();

void set_runtime_pointer_refresh_api_for_testing(const RuntimePointerRefreshApi &api);

// GAG.EXE: 0x004235E0
std::int32_t __fastcall activate_default_comment_scene(const char *name);

// GAG.EXE: 0x004236E0
void __fastcall activate_runtime_tree_node_comment(RuntimeTreeNode *node);

// GAG.EXE: 0x00423660
void __fastcall deactivate_default_comment_scene(const char *name);

// GAG.EXE: 0x00423710
void __fastcall deactivate_runtime_tree_node_comment(RuntimeTreeNode *node);

// GAG.EXE: 0x00426320
void __fastcall set_runtime_tree_comment_mode(RuntimeTreeNode *root, int enabled);

// GAG.EXE: 0x00406770
RuntimeTreeNode *__fastcall begin_runtime_tree_enumeration(void *identity);

// GAG.EXE: 0x004067F0
RuntimeTreeNode *__fastcall get_next_runtime_tree_node(RuntimeTreeNode *root);

struct RuntimeCommentTreeCleanupApi
{
    RuntimeTreeNode *(__fastcall *begin_enumeration)(void *identity);
    RuntimeTreeNode *(__fastcall *next_node)(RuntimeTreeNode *root);
    void(__fastcall *destroy_resources)(void *identity);
    std::uint32_t(__fastcall *deactivate_node)(void *identity, void *second);
    void(__fastcall *finalize_destroyed_nodes)(void *identity);
    void (*rebuild_runtime_plans)();
};

// GAG.EXE: 0x00423740
int destroy_runtime_comment_trees();

struct RuntimeTreeDeactivateApi
{
    RuntimeTreeNode *(__fastcall *resolve_identity)(void *identity);
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(__fastcall *request_resource_destruction)(void *identity);
    BOOL(__fastcall *remove_visual_object)(void *identity);
    void(__fastcall *set_script_flags)(std::uint32_t flags, int enabled);
    void(__fastcall *deactivate_comment)(RuntimeTreeNode *node);
    std::uint32_t(__fastcall *destroy_tree)(void *first, void *second);
};

// GAG.EXE: 0x00426600
std::uint32_t __fastcall deactivate_runtime_tree_and_visuals(void *identity, void *second);

void set_runtime_comment_tree_cleanup_api_for_testing(const RuntimeCommentTreeCleanupApi &api);
void set_runtime_tree_deactivate_api_for_testing(const RuntimeTreeDeactivateApi &api);

// GAG.EXE: 0x00406640
void *__fastcall find_runtime_tree_identity_by_name_recursive(void *start_identity, const void *name);

// GAG.EXE: 0x004066C0
void *__fastcall find_runtime_tree_descendant_identity_by_name(void *root_identity, const void *name);

// GAG.EXE: 0x00406720
void *__fastcall find_runtime_tree_root_identity_by_name(const void *name);

// GAG.EXE: 0x004237B0
std::uint32_t has_runtime_pointer_tree_flag_1000();

// GAG.EXE: 0x00425FA0
void __fastcall release_runtime_lock_record(RuntimeLockRecord *record);

// GAG.EXE: 0x00425F10
RuntimeLockRecord *__fastcall acquire_runtime_lock_record(void *child_identity);

struct RuntimeResourceLoopApi
{
    RuntimeLockRecord *(__fastcall *acquire_record)(void *identity);
    void(__fastcall *set_sound_loop)(std::uint32_t handle, std::uint32_t value);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x004258D0
void __fastcall set_runtime_resource_loop_count(void *identity, std::uint32_t count);

void set_runtime_resource_loop_api_for_testing(const RuntimeResourceLoopApi &api);

// GAG.EXE: 0x004242C0
void __fastcall switch_runtime_scene(void *identity);

// GAG.EXE: 0x004262B0
void reset_runtime_display_state();

// GAG.EXE: 0x00420130
std::uint32_t shutdown_runtime_display();

// GAG.EXE: 0x00425FD0
std::uint32_t __fastcall query_runtime_scene_flags(void *identity);

// GAG.EXE: 0x00426D20
void __fastcall wait_for_runtime_resource_count(std::uint32_t count);

// GAG.EXE: 0x00425EB0
void __fastcall update_runtime_scene_position(void *identity, std::int32_t x, std::int32_t y);

// GAG.EXE: 0x004246B0
void __fastcall build_runtime_resource_path(char *destination, const char *source);

// GAG.EXE: 0x00424570
void __fastcall update_runtime_resource_host(const char *path, std::int32_t reset);

// GAG.EXE: 0x00424710
std::uint32_t __fastcall detect_runtime_resource_type(const char *path);

// GAG.EXE: 0x00428720
void *__fastcall open_runtime_cdf_entry_stream(CdfArchive *archive, const char *name);

// GAG.EXE: 0x00424870
void __fastcall load_runtime_resource(const char *path, void **data, std::uint32_t *size, std::int32_t *storage, std::uint32_t flags);

// GAG.EXE: 0x00414DD0
std::uint32_t __fastcall extract_runtime_drive_prefix(char *destination, const char *source);

// GAG.EXE: 0x00417990
std::uint32_t __fastcall measure_archive_read_speed(const char *archive_path, std::uint32_t bytes_to_measure);
void set_archive_read_speed_api_for_testing(const ArchiveReadSpeedApi &api);

// GAG.EXE: 0x0042B6B0
HANDLE __fastcall open_runtime_resource_file(const char *path);

// GAG.EXE: 0x00415040
AsyncFileHost *__fastcall acquire_async_file_host(AsyncFileHost *identity);

// GAG.EXE: 0x00414EC0
AsyncFileHost *__fastcall create_async_file_host(const char *root, std::uint32_t requested_bytes, std::int32_t mode);

// GAG.EXE: 0x00414900
void __fastcall advance_async_host_write(AsyncFileHost *host, std::uint32_t bytes);

// GAG.EXE: 0x004148B0
void __fastcall advance_async_host_read(AsyncFileHost *host, std::uint32_t bytes);

// GAG.EXE: 0x00414A50
void __fastcall invalidate_shared_async_records(AsyncFileRecord *record);

// GAG.EXE: 0x00414930
void __fastcall position_async_host(AsyncFileHost *host, std::uint32_t offset);

// GAG.EXE: 0x00414AE0
void __fastcall seek_async_host(AsyncFileHost *host, std::uint32_t offset);

// GAG.EXE: 0x00414BB0
std::uint32_t __fastcall copy_async_host_bytes(AsyncFileHost *host, void *destination, std::uint32_t bytes, std::uint32_t *total_bytes);

// GAG.EXE: 0x00414CB0
void __fastcall activate_async_file_record(AsyncFileRecord *record);

// GAG.EXE: 0x00415AE0
void __fastcall handle_async_host_short_read(AsyncFileHost *host);

// GAG.EXE: 0x00415B70
DWORD WINAPI run_async_file_worker(LPVOID parameter);

// GAG.EXE: 0x004150D0
void __fastcall release_async_file_host(AsyncFileHost *identity);

// GAG.EXE: 0x00415120
std::uint32_t __fastcall destroy_async_file_host(AsyncFileHost *identity);

struct AsyncFileShutdownApi
{
    std::uint32_t(__fastcall *destroy_host)(AsyncFileHost *identity);
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
};

// GAG.EXE: 0x00414E40
std::uint32_t shutdown_async_file_subsystem();

void set_async_file_shutdown_api_for_testing(const AsyncFileShutdownApi &api);

// GAG.EXE: 0x004155C0
AsyncFileRecord *__fastcall acquire_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415690
void __fastcall release_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415210
void __fastcall set_async_file_host_mode(AsyncFileHost *identity, std::int32_t mode);

// GAG.EXE: 0x00415AC0
std::uint32_t __fastcall get_async_file_size(AsyncFileRecord *identity);

// GAG.EXE: 0x00415AA0
std::uint32_t __fastcall get_async_file_position(AsyncFileRecord *identity);

// GAG.EXE: 0x00415A20
std::uint32_t __fastcall set_async_file_position(AsyncFileRecord *identity, std::uint32_t position);

// GAG.EXE: 0x00415230
AsyncFileRecord *__fastcall open_async_file_record(AsyncFileHost *host_identity, const char *path, std::uint32_t start_offset, std::uint32_t end_offset, std::uint32_t flags);

// GAG.EXE: 0x00415360
AsyncFileRecord *__fastcall duplicate_async_file_record(AsyncFileRecord *identity, std::uint32_t start_offset, std::uint32_t end_offset, std::uint32_t flags);

// GAG.EXE: 0x00415420
std::uint32_t __fastcall close_async_file_record(AsyncFileRecord *identity);

// GAG.EXE: 0x00415720
std::uint32_t __fastcall read_async_file_record(AsyncFileRecord *identity, void *destination, std::uint32_t bytes, std::uint32_t *bytes_read, std::int32_t force_host_buffer);

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
    void(__fastcall *release_backend_lock)(RuntimeMediaBackend *backend);
    BOOL(WINAPI *heap_destroy)(HANDLE heap);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    std::uint32_t (*shutdown_sound)();
};

// GAG.EXE: 0x00429E50
std::uint32_t shutdown_runtime_media_backend();

void set_runtime_media_backend_shutdown_api_for_testing(const RuntimeMediaBackendShutdownApi &api);
void set_runtime_palette_update_api_for_testing(const RuntimePaletteUpdateApi &api);
void set_runtime_bitmap_backend_create_api_for_testing(const RuntimeBitmapBackendCreateApi &api);
void set_runtime_animation_backend_create_api_for_testing(const RuntimeAnimationBackendCreateApi &api);
void set_runtime_media_backend_configure_api_for_testing(const RuntimeMediaBackendConfigureApi &api);
void set_runtime_animation_backend_configure_api_for_testing(const RuntimeAnimationBackendConfigureApi &api);
void set_runtime_resource_palette_configure_api_for_testing(const RuntimeResourcePaletteConfigureApi &api);
void set_runtime_resource_palette_bits_per_pixel_for_testing(std::uint32_t bits_per_pixel);
void set_runtime_media_backend_finalize_api_for_testing(const RuntimeMediaBackendFinalizeApi &api);
void set_runtime_animation_failure_api_for_testing(const RuntimeAnimationFailureApi &api);
void set_runtime_animation_control_flags_for_testing(std::uint32_t flags);
std::uint32_t get_runtime_animation_control_flags_for_testing();
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
void set_runtime_generic_backend_create_state_for_testing(std::uint32_t enabled);
RuntimeGenericBackend *get_runtime_generic_backend_head_for_testing();
void set_runtime_sound_destroy_api_for_testing(const RuntimeSoundDestroyApi &api);
void set_runtime_sound_destroy_state_for_testing(std::int32_t enabled, HANDLE mutex, RuntimeSoundSlot *slots, std::uint32_t maximum_handle);
std::uint32_t get_runtime_sound_maximum_handle_for_testing();
void set_runtime_sound_create_api_for_testing(const RuntimeSoundCreateApi &api);
void set_runtime_sound_create_state_for_testing(HANDLE lifecycle_mutex, HWAVEOUT wave_out, WAVEFORMATEX *output_format, WAVEHDR *header_1, WAVEHDR *header_2, HWND window, HANDLE thread,
    DWORD thread_id, std::uint32_t output_ready, std::uint32_t ready, std::uint32_t fault);
void get_runtime_sound_create_state_for_testing(HANDLE *thread, DWORD *thread_id, std::uint32_t *output_initialized, std::uint32_t *ready);
void set_runtime_sound_format_cleanup_api_for_testing(const RuntimeSoundFormatCleanupApi &api);
void set_runtime_sound_format_cleanup_state_for_testing(void *buffer, std::uint32_t base_state);
void get_runtime_sound_format_cleanup_state_for_testing(void **buffer, std::uint32_t *base_state);
void set_runtime_sound_fade_state_for_testing(WAVEFORMATEX *output_format, std::uint32_t mixer_data_size);
void set_runtime_wave_out_callback_api_for_testing(const RuntimeWaveOutCallbackApi &api);
void set_runtime_wave_out_callback_state_for_testing(HWND window, std::uint32_t output_ready);
std::uint32_t get_runtime_wave_out_callback_state_for_testing();
void set_runtime_sound_shutdown_api_for_testing(const RuntimeSoundShutdownApi &api);
void set_runtime_sound_shutdown_state_for_testing(HANDLE lifecycle_mutex, HWAVEOUT wave_out, WAVEHDR *header_1, WAVEHDR *header_2, HANDLE thread, DWORD thread_id, std::uint32_t output_ready,
    std::uint32_t output_initialized);
void get_runtime_sound_shutdown_state_for_testing(std::int32_t *enabled, HANDLE *thread, DWORD *thread_id, std::uint32_t *output_initialized);
void set_runtime_sound_readiness_api_for_testing(const RuntimeSoundReadinessApi &api);
void set_runtime_sound_readiness_state_for_testing(std::uint32_t ready);
std::uint32_t get_runtime_sound_readiness_state_for_testing();
void set_runtime_sound_thread_api_for_testing(const RuntimeSoundThreadApi &api);
void set_runtime_sound_thread_state_for_testing(HINSTANCE instance, HWND window, std::uint32_t creation_failed);
void get_runtime_sound_thread_state_for_testing(HWND *window, std::uint32_t *creation_failed);
void set_runtime_sound_class_api_for_testing(const RuntimeSoundClassApi &api);
void set_runtime_sound_window_api_for_testing(const RuntimeSoundWindowApi &api);
void set_runtime_sound_window_state_for_testing(RuntimeSoundOutputBlock *outputs, WAVEFORMATEX *output_format, std::uint32_t mixer_data_size, std::uint32_t output_initialized,
    std::uint32_t output_index, void(__fastcall *mixer)(std::uint32_t marker));
void set_runtime_sound_mixing_suppressed_for_testing(std::uint8_t suppressed);
void set_runtime_wave_mixer_initialize_api_for_testing(const struct RuntimeWaveMixerInitializeApi &api);
void set_runtime_wave_mixer_initialize_state_for_testing(std::uint32_t fault, std::uint32_t window_creation_failed);
void get_runtime_wave_mixer_initialize_state_for_testing(std::uint32_t *fault, void **buffer, std::uint32_t *mixer_data_size, void(__fastcall **mixer)(std::uint32_t marker));
void get_runtime_sound_window_state_for_testing(std::uint32_t *output_ready, std::uint32_t *output_initialized, std::uint32_t *output_index);

// GAG.EXE: 0x004023C0
void __fastcall mix_runtime_sound_8bit_mono(std::uint32_t marker);

// GAG.EXE: 0x00402770
void __fastcall mix_runtime_sound_8bit_stereo(std::uint32_t marker);

// GAG.EXE: 0x00402B10
void __fastcall mix_runtime_sound_16bit_mono(std::uint32_t marker);

// GAG.EXE: 0x00402F10
void __fastcall mix_runtime_sound_16bit_stereo(std::uint32_t marker);

struct RuntimeWaveMixerInitializeApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    void(WINAPI *sleep)(DWORD milliseconds);
    MMRESULT(WINAPI *wave_out_open)(LPHWAVEOUT wave_out, UINT device_id, LPCWAVEFORMATEX format, DWORD_PTR callback, DWORD_PTR instance, DWORD flags);
    void (*cleanup_format_buffer)();
};

// GAG.EXE: 0x00401330
std::uint32_t __fastcall initialize_runtime_wave_out_mixer(WAVEFORMATEX *format, std::uint32_t unused_argument);
void set_runtime_resource_destroy_api_for_testing(const RuntimeResourceDestroyApi &api);
void set_runtime_resource_control_api_for_testing(const RuntimeResourceControlApi &api);
void set_runtime_game_dll_unload_api_for_testing(const RuntimeGameDllUnloadApi &api);
void set_runtime_game_dll_load_api_for_testing(const RuntimeGameDllLoadApi &api);
void set_runtime_game_dll_dispatch_api_for_testing(const RuntimeGameDllDispatchApi &api);
void set_runtime_game_dll_state_for_testing(HMODULE module, std::uint32_t flags);
void set_runtime_game_dll_execute_for_testing(RuntimeGameDllExecute execute);
void set_runtime_game_window_api_for_testing(const RuntimeGameWindowApi &api);
void set_runtime_game_window_state_for_testing(HWND main_window, RuntimeGameDllWindowProcedure window_procedure, std::uint16_t x_offset, std::uint16_t y_offset);
void get_runtime_game_result_for_testing(std::uint32_t *type, void *data, std::uint32_t size);
void set_runtime_pointer_position_api_for_testing(const RuntimePointerPositionApi &api);
void get_runtime_pointer_position_for_testing(std::int32_t *x, std::int32_t *y);
void set_runtime_game_host_state_for_testing(const RuntimeGameHostContext &context, void *const *callbacks);
void get_runtime_game_dll_state_for_testing(HMODULE *module, FARPROC *initialize, FARPROC *window_procedure, FARPROC *execute);
void set_runtime_resource_destroy_state_for_testing(void *current_resource);
void *get_runtime_resource_destroy_state_for_testing();
void set_runtime_named_lock_state_for_testing(void *parent_identity);
void set_runtime_scene_switch_api_for_testing(const RuntimeSceneSwitchApi &api);
void set_runtime_scene_switch_state_for_testing(void *current_identity, std::int32_t x, std::int32_t y);
void *get_current_runtime_scene_identity_for_testing();
void set_runtime_scene_control_state_for_testing(std::uint32_t flags, void *saved_identity);
std::uint32_t get_runtime_scene_control_flags_for_testing();
void set_runtime_scene_slots_for_testing(const RuntimeSceneSlot *slots);
const RuntimeSceneSlot *get_runtime_scene_slots_for_testing();
void set_runtime_pointer_region_state_for_testing(void *root_identity, RuntimePointerRegion *regions, RuntimePointerRegion *active_region, std::uint32_t state_mask, void *state_owner);
RuntimePointerRegion *get_active_runtime_pointer_region_for_testing();
RuntimePointerRegion *get_runtime_pointer_regions_for_testing();
std::uint32_t get_runtime_pointer_event_flags_for_testing();
void set_runtime_display_reset_api_for_testing(const RuntimeDisplayResetApi &api);
void set_runtime_display_reset_state_for_testing(std::uint32_t value_1, std::uint8_t byte_value, std::uint32_t value_2, const std::uint32_t *scene_state);
void get_runtime_display_reset_state_for_testing(std::uint32_t *value_1, std::uint8_t *byte_value, std::uint32_t *value_2, std::uint32_t *scene_state);
void set_runtime_display_shutdown_api_for_testing(const RuntimeDisplayShutdownApi &api);
void set_runtime_display_shutdown_state_for_testing(HANDLE thread, std::int32_t scene_identifier, void *host, const std::uint32_t *backend_state, const std::uint32_t *pixel_format_state);
void get_runtime_display_shutdown_state_for_testing(HANDLE *thread, std::int32_t *scene_identifier, void **host, std::uint32_t *backend_state, std::uint32_t *pixel_format_state);
void set_runtime_resource_wait_api_for_testing(const RuntimeResourceWaitApi &api);
void set_runtime_resource_count_for_testing(std::uint32_t count);
std::uint32_t get_runtime_resource_count_for_testing();
void set_runtime_resource_directory_for_testing(const char *directory);
void set_runtime_resource_file_open_api_for_testing(const RuntimeResourceFileOpenApi &api);
void set_runtime_resource_host_api_for_testing(const RuntimeResourceHostApi &api);
void set_runtime_resource_host_state_for_testing(AsyncFileHost *host, CdfArchive *archive, std::int32_t mode, std::uint8_t archive_state);
void get_runtime_resource_host_state_for_testing(AsyncFileHost **host, CdfArchive **archive, std::int32_t *mode, std::uint8_t *archive_state);
void set_runtime_resource_type_api_for_testing(const RuntimeResourceTypeApi &api);
void set_runtime_resource_type_state_for_testing(void *cache_parent_identity, HWND notification_window);
void set_runtime_cdf_stream_api_for_testing(const RuntimeCdfStreamApi &api);
void set_runtime_resource_load_api_for_testing(const RuntimeResourceLoadApi &api);
void set_runtime_resource_load_state_for_testing(HANDLE heap, std::uint32_t streamed_count);
std::uint32_t get_runtime_resource_streamed_count_for_testing();
void set_async_file_lock_api_for_testing(const AsyncFileLockApi &api);
void set_async_file_state_for_testing(bool enabled, AsyncFileHost *hosts);
void set_async_file_open_api_for_testing(const AsyncFileOpenApi &api);
void set_async_file_host_api_for_testing(const AsyncFileHostApi &api);

// GAG.EXE: 0x00408380
ScriptObjectState *__fastcall find_script_object_by_name(const char *name);

// GAG.EXE: 0x00408660
ScriptObjectState *__fastcall resolve_state_field_reference(const char *object_name, const char *field_name, const void *value, int value_type);

void set_script_runtime_root_for_testing(ScriptRuntimeRoot *root);
void use_embedded_script_runtime_root_for_testing();
ScriptRuntimeRoot *get_embedded_script_runtime_root_for_testing();

// GAG.EXE: 0x0040D030
void __fastcall copy_file_name_from_path(char *destination, const char *source);

// GAG.EXE: 0x0040CFD0
int __fastcall append_string(char *destination, const char *source);

struct DisplayMode
{
    std::uint32_t flags;
    std::uint32_t unknown_0004;
    std::uint32_t unknown_0008;
    std::uint32_t surface_caps;
    std::uint32_t unknown_0010;
    std::uint32_t pixel_value_count;
    std::int32_t width;
    std::int32_t height;
    std::uint32_t pixel_format_flags;
    std::uint32_t unknown_0024;
    std::int32_t bits_per_pixel;
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    std::uint32_t alpha_mask;
    DisplayMode *next;
};

static_assert(sizeof(DisplayMode) == 0x40);
static_assert(offsetof(DisplayMode, width) == 0x18);
static_assert(offsetof(DisplayMode, height) == 0x1c);
static_assert(offsetof(DisplayMode, pixel_format_flags) == 0x20);
static_assert(offsetof(DisplayMode, bits_per_pixel) == 0x28);
static_assert(offsetof(DisplayMode, green_mask) == 0x30);
static_assert(offsetof(DisplayMode, next) == 0x3c);

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
    std::uint32_t(__fastcall *set_cooperative_mode)(std::uint32_t mode);
    HRESULT(WINAPI *enumerate_modes)(void *display, DirectDrawModeCallback callback);
};

// GAG.EXE: 0x00412F40
std::uint32_t initialize_direct_draw_runtime();

// GAG.EXE: 0x00412DB0
HRESULT WINAPI collect_direct_draw_display_mode(LegacyDirectDrawSurfaceDescriptor *descriptor, void *context);

// GAG.EXE: 0x00412FE0
std::uint32_t enumerate_direct_draw_display_modes();

void set_display_bootstrap_api_for_testing(const DisplayBootstrapApi &api);
void set_display_bootstrap_state_for_testing(std::uint32_t flags, DisplayMode *head, DisplayMode *tail, std::uint32_t count, void *display);
std::uint32_t get_display_bootstrap_error_for_testing();
std::uint32_t get_display_mode_count_for_testing();

struct DisplayHostInitializationApi
{
    void(WINAPI *initialize_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *delete_critical_section)(LPCRITICAL_SECTION section);
    std::uint32_t (*enumerate_windows_modes)();
    std::uint32_t (*initialize_direct_draw)();
    std::uint32_t (*enumerate_direct_draw_modes)();
    DisplayMode *(*find_current_mode)();
};

// GAG.EXE: 0x00413380
std::uint32_t __fastcall initialize_display_mode_host(HWND window, std::uint32_t options);

void set_display_host_initialization_api_for_testing(const DisplayHostInitializationApi &api);
void set_display_host_initialization_state_for_testing(std::uint32_t flags, HWND window);
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
std::uint32_t enumerate_windows_display_modes();

void set_windows_display_enumeration_api_for_testing(const WindowsDisplayEnumerationApi &api);

// GAG.EXE: 0x00413650
DisplayMode *__fastcall begin_display_mode_enumeration(std::uint32_t mask);

// GAG.EXE: 0x004136A0
DisplayMode *__fastcall get_next_display_mode(std::uint32_t mask);

// GAG.EXE: 0x004136F0
DisplayMode *find_current_display_mode();

// GAG.EXE: 0x0041F960
DisplayMode *get_current_display_mode();

// GAG.EXE: 0x0041F980
DisplayMode *__fastcall begin_available_display_modes(std::uint32_t mask);

// GAG.EXE: 0x0041F9A0
DisplayMode *__fastcall get_next_available_display_mode(std::uint32_t mask);

// GAG.EXE: 0x0041EFA0
std::uint32_t __fastcall detect_alternate_display_mode(ApplicationState *state);

void set_display_mode_list_for_testing(DisplayMode *head);
void set_graphics_host_flags_for_testing(std::uint32_t flags);

struct DisplaySwitchApi
{
    std::uint32_t(__fastcall *select_mode)(DisplayMode *mode);
    std::uint32_t (*restore_current_mode)();
};

// GAG.EXE: 0x0041D010
void __fastcall switch_display_mode_if_enabled(ApplicationState *state, int restore_current);

// GAG.EXE: 0x00420BC0
void enable_runtime_subsystem();

// GAG.EXE: 0x00420BE0
void disable_runtime_subsystem();

// GAG.EXE: 0x00404980
void __fastcall set_active_object_field_0824(std::uint32_t value);

void set_runtime_subsystem_callback_for_testing(void (*callback)());
void set_display_switch_api_for_testing(const DisplaySwitchApi &api);
void set_active_object_for_testing(void *object);
std::uint32_t get_graphics_host_flags_for_testing();

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
    CdfArchive *(__fastcall *open_archive)(const char *path, int alternate_stream);
    int(__fastcall *read_entry)(CdfArchive *archive, std::uint8_t selector, const char *name, void *destination);
    std::uint32_t(__fastcall *close_archive)(CdfArchive *archive);
    int(WINAPI *compare_case_insensitive)(LPCSTR left, LPCSTR right);
};

// GAG.EXE: 0x0041EBD0
void __fastcall locate_game_data_drive(ApplicationState *state, const char *requested_archive);

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

void set_runtime_state_transition_for_testing(std::uint32_t current_value, std::uint32_t saved_value, void(__fastcall *callback)(std::uint32_t value));
std::uint32_t get_runtime_state_value_for_testing();

struct RuntimePathApi
{
    void (*enter_lock)();
    void (*leave_lock)();
};

// GAG.EXE: 0x00420C30
void __fastcall set_runtime_paths_once(const char *first_path, const char *second_path);

void set_runtime_path_api_for_testing(const RuntimePathApi &api);
const char *get_first_runtime_path_for_testing();
const char *get_second_runtime_path_for_testing();

struct ScreenshotApi
{
    BOOL(WINAPI *get_save_file_name)(LPOPENFILENAMEA file_name);
    void *(__fastcall *capture_bitmap)(void *snapshot_context, std::uint32_t *size, int mode);
    HANDLE(WINAPI *create_file)(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD size, LPDWORD written, LPOVERLAPPED overlapped);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};

// GAG.EXE: 0x0041CBE0
void __fastcall save_game_screenshot(void *snapshot_context, void *game_context);

void set_screenshot_api_for_testing(const ScreenshotApi &api);

#pragma pack(push, 1)
struct BitmapCaptureSource
{
    std::uint8_t unresolved_00[0x20];
    std::uint16_t width;
    std::uint16_t height;
    const std::uint8_t *pixels;
};
#pragma pack(pop)

static_assert(sizeof(BitmapCaptureSource) == 0x28);
static_assert(offsetof(BitmapCaptureSource, width) == 0x20);
static_assert(offsetof(BitmapCaptureSource, height) == 0x22);
static_assert(offsetof(BitmapCaptureSource, pixels) == 0x24);

struct BitmapCaptureApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

// GAG.EXE: 0x00417790
void *__fastcall create_indexed_bitmap(const BitmapCaptureSource *source, const std::uint8_t *palette, std::uint32_t *size, int half_resolution);

// GAG.EXE: 0x0041F8B0
void *__fastcall capture_bitmap_if_runtime_active(const BitmapCaptureSource *source, const std::uint8_t *palette, std::uint32_t *size, int half_resolution);

// GAG.EXE: 0x0041CB90
void *__fastcall capture_game_bitmap(void *game_context, std::uint32_t *size, int half_resolution);

void set_bitmap_capture_api_for_testing(const BitmapCaptureApi &api);

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
void __fastcall enqueue_runtime_byte(std::uint8_t value);

// GAG.EXE: 0x004206D0
std::uint8_t dequeue_runtime_byte();

// GAG.EXE: 0x00420750
void reset_runtime_byte_queue();

struct RuntimeMessagePair
{
    std::uint32_t first;
    std::uint32_t second;
};

// GAG.EXE: 0x00420910
void __fastcall enqueue_runtime_pair(std::uint32_t first, std::uint32_t second);

// GAG.EXE: 0x004209B0
int __fastcall dequeue_runtime_pair(RuntimeMessagePair *pair);

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
    void(__fastcall *destroy_resources)(void *identity);
    RuntimeTreeNode *(__fastcall *activate_tree)(const char *first, const char *second, void *third, void *fourth);
    void(__fastcall *finalize_current_tree)(void *identity);
    void(__fastcall *rebuild_runtime_plans)(void *identity);
    std::uint32_t(__fastcall *update_pointer)(std::int32_t x, std::int32_t y);
};

// GAG.EXE: 0x004210A0
bool process_pending_runtime_tree_switch(RuntimeTreeNode *node);

void set_runtime_pending_tree_switch_api_for_testing(const RuntimePendingTreeSwitchApi &api);

struct RuntimeTreeActivationApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    RuntimeGenericResourceNode *(__fastcall *find_or_load_resource)(const char *name);
    RuntimeTreeNode *(__fastcall *create_tree_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *name, void *creation_context);
    void(__fastcall *set_script_flags)(std::uint32_t flags, int enabled);
    void(__fastcall *activate_comment)(RuntimeTreeNode *node);
};

// GAG.EXE: 0x00426560
RuntimeTreeNode *__fastcall activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *creation_context, void *unused);

void set_runtime_tree_activation_api_for_testing(const RuntimeTreeActivationApi &api);

struct RuntimePairDispatchApi
{
    int(__fastcall *dequeue_pair)(RuntimeMessagePair *pair);
    std::uint32_t(__fastcall *move_pointer)(std::int32_t x, std::int32_t y);
    std::uint32_t (*left_button_down)();
    std::uint32_t (*left_button_up)();
    std::uint32_t (*right_button_down)();
};

// GAG.EXE: 0x004211A0
std::uint32_t process_runtime_pair_message();

void set_runtime_plan_mode_sync_api_for_testing(const RuntimePlanModeSyncApi &api);
void set_runtime_pair_dispatch_api_for_testing(const RuntimePairDispatchApi &api);

struct RuntimeInputSessionRecord
{
    std::uint32_t values[8];
};

static_assert(sizeof(RuntimeInputSessionRecord) == 0x20);

// GAG.EXE: 0x004208E0
std::uint32_t __fastcall copy_runtime_input_session_record(RuntimeInputSessionRecord *record);

struct RuntimeInputSessionApi
{
    void (*reset_byte_queue)();
    DWORD(WINAPI *get_time)();
    RuntimeLockRecord *(__fastcall *acquire_record)(void *selector);
    std::uint32_t(__fastcall *initialize_text)(const char *text, std::uint32_t value_0014, std::uint32_t value_0018, void *font_identity, std::uint32_t low_color, std::uint32_t high_color,
        RuntimeStandaloneTextState *state);
    std::uint32_t(__fastcall *find_scene_index)(std::uint32_t flags);
    DisplaySceneNode *(__fastcall *lock_scene)(std::int32_t identifier);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    std::uint32_t(__fastcall *begin_update)(std::int32_t identifier);
    void(__fastcall *draw_text)(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);
    std::uint32_t(__fastcall *end_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void(__fastcall *unlock_scene)(std::int32_t identifier);
    void(__fastcall *release_record)(RuntimeLockRecord *record);
};

// GAG.EXE: 0x00420790
void __fastcall initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, std::uint32_t character_width, void *session_value);

// GAG.EXE: 0x00420A90
void __fastcall enqueue_runtime_message(std::uint32_t message);

// GAG.EXE: 0x00420B50
std::uint32_t dequeue_runtime_message();

// GAG.EXE: 0x00420CB0
void clear_credits_runtime_flag();

void set_runtime_queue_api_for_testing(const RuntimeQueueApi &api);
void set_runtime_pair_indices_for_testing(std::uint32_t read_index, std::uint32_t write_index);
void get_runtime_pair_indices_for_testing(std::uint32_t *read_index, std::uint32_t *write_index);
void reset_runtime_message_queue_for_testing();
void reset_runtime_byte_queue_for_testing();
void reset_runtime_pair_queue_for_testing();
void set_runtime_input_session_record_for_testing(const RuntimeInputSessionRecord &record, std::uint32_t status);
void set_runtime_input_session_api_for_testing(const RuntimeInputSessionApi &api);
void set_runtime_input_alternate_scene_for_testing(std::int32_t identifier);

#pragma pack(push, 1)
struct RuntimeCommandLoopState
{
    HWND window;
    std::uint8_t unknown_004[8];
    std::uint8_t resource_archive_state;
    std::uint8_t unknown_00d[0x103];
    char resource_directory[0x104];
    void *display_scene_host;
    std::uint8_t unknown_218[0x10];
    std::uint32_t display_pixel_format[8];
    std::int32_t input_alternate_scene_identifier;
    char first_runtime_path[0x104];
    char second_runtime_path[0x104];
    void *command_context;
    std::uint8_t command_target[0x20];
    std::uint16_t width;
    std::uint16_t height;
    void *display_surface;
    std::uint32_t callback_first_position_1;
    std::uint32_t callback_first_position_2;
    std::uint32_t callback_first_position_3;
    PALETTEENTRY *palette_entries;
    std::uint8_t unknown_490[0x94];
    HMODULE game_dll_module;
    FARPROC game_dll_initialize;
    FARPROC game_dll_window_procedure;
    FARPROC game_dll_execute;
    std::uint32_t game_result_type;
    std::uint8_t game_result_data[0x104];
    char input_text[0x20];
    RuntimeStandaloneTextState input_text_state;
    std::int32_t input_scene_identifier;
    std::uint32_t input_text_flags;
    std::uint32_t input_scene_index;
    std::uint32_t input_caret_tick;
    std::uint32_t input_cursor;
    std::uint32_t input_end;
    std::uint32_t pair_available;
    RuntimeMessagePair pair_queue[0x20];
    std::uint32_t pair_read_index;
    std::uint32_t pair_write_index;
    std::uint32_t byte_available;
    std::uint8_t byte_queue[0x20];
    std::uint32_t byte_read_index;
    std::uint32_t byte_write_index;
    std::uint32_t message_available;
    std::uint32_t message_queue[0x20];
    std::uint32_t message_read_index;
    std::uint32_t message_write_index;
    CdfArchive *active_archive;
    AsyncFileHost *async_file_host;
    void *resource_cache_parent_identity;
    CRITICAL_SECTION byte_queue_critical_section;
    CRITICAL_SECTION pair_queue_critical_section;
    CRITICAL_SECTION message_queue_critical_section;
    CRITICAL_SECTION resource_critical_section;
    CRITICAL_SECTION path_critical_section;
    HANDLE resource_heap;
    HANDLE script_thread;
    std::uint8_t unknown_900[4];
    void *media_objects_parent_identity;
    std::uint32_t resource_wait_count;
    std::uint32_t accumulated_tree_flags;
    std::uint32_t reset_value_1;
    std::uint32_t reset_value_2;
    std::uint32_t reset_value_3;
    std::uint32_t nested_runtime_state_count;
    std::uint32_t nested_runtime_state_4_count;
    std::uint32_t resource_count;
    std::uint32_t external_command_pending;
    std::uint32_t target_flags;
    std::uint32_t flags;
    std::int32_t resource_host_mode;
    std::uint32_t script_clock;
    std::int32_t scene_x;
    std::int32_t scene_y;
    std::uint8_t unknown_944[0x10];
    void *saved_default_comment_scene_identity;
    void *deferred_scene_identity;
    void *current_scene_identity;
    void *current_runtime_resource;
    void *runtime_tree_identity;
    RuntimeTreeLink7C *active_script_link;
    RuntimePointerRegion *active_pointer_region;
};
#pragma pack(pop)

static_assert(offsetof(RuntimeCommandLoopState, flags) == 0x930);
static_assert(offsetof(RuntimeCommandLoopState, script_clock) == 0x938);
static_assert(offsetof(RuntimeCommandLoopState, scene_x) == 0x93c);
static_assert(offsetof(RuntimeCommandLoopState, scene_y) == 0x940);
static_assert(offsetof(RuntimeCommandLoopState, saved_default_comment_scene_identity) == 0x954);
static_assert(offsetof(RuntimeCommandLoopState, deferred_scene_identity) == 0x958);
static_assert(offsetof(RuntimeCommandLoopState, current_scene_identity) == 0x95c);
static_assert(offsetof(RuntimeCommandLoopState, current_runtime_resource) == 0x960);
static_assert(offsetof(RuntimeCommandLoopState, runtime_tree_identity) == 0x964);
static_assert(offsetof(RuntimeCommandLoopState, active_script_link) == 0x968);
static_assert(offsetof(RuntimeCommandLoopState, active_pointer_region) == 0x96c);
static_assert(sizeof(RuntimeCommandLoopState) == 0x970);
static_assert(offsetof(RuntimeCommandLoopState, input_alternate_scene_identifier) == 0x248);
static_assert(offsetof(RuntimeCommandLoopState, display_scene_host) == 0x214);
static_assert(offsetof(RuntimeCommandLoopState, resource_archive_state) == 0x0c);
static_assert(offsetof(RuntimeCommandLoopState, resource_directory) == 0x110);
static_assert(offsetof(RuntimeCommandLoopState, first_runtime_path) == 0x24c);
static_assert(offsetof(RuntimeCommandLoopState, second_runtime_path) == 0x350);
static_assert(offsetof(RuntimeCommandLoopState, display_pixel_format) == 0x228);
static_assert(offsetof(RuntimeCommandLoopState, command_target) == 0x458);
static_assert(offsetof(RuntimeCommandLoopState, width) == 0x478);
static_assert(offsetof(RuntimeCommandLoopState, accumulated_tree_flags) == 0x90c);
static_assert(offsetof(RuntimeCommandLoopState, resource_wait_count) == 0x908);
static_assert(offsetof(RuntimeCommandLoopState, height) == 0x47a);
static_assert(offsetof(RuntimeCommandLoopState, display_surface) == 0x47c);
static_assert(offsetof(RuntimeCommandLoopState, callback_first_position_1) == 0x480);
static_assert(offsetof(RuntimeCommandLoopState, palette_entries) == 0x48c);
static_assert(offsetof(RuntimeCommandLoopState, script_thread) == 0x8fc);
static_assert(offsetof(RuntimeCommandLoopState, resource_cache_parent_identity) == 0x87c);
static_assert(offsetof(RuntimeCommandLoopState, resource_heap) == 0x8f8);
static_assert(offsetof(RuntimeCommandLoopState, media_objects_parent_identity) == 0x904);
static_assert(offsetof(RuntimeCommandLoopState, target_flags) == 0x92c);
static_assert(offsetof(RuntimeCommandLoopState, resource_host_mode) == 0x934);
static_assert(offsetof(RuntimeCommandLoopState, game_result_type) == 0x534);
static_assert(offsetof(RuntimeCommandLoopState, game_dll_module) == 0x524);
static_assert(offsetof(RuntimeCommandLoopState, game_dll_initialize) == 0x528);
static_assert(offsetof(RuntimeCommandLoopState, game_dll_window_procedure) == 0x52c);
static_assert(offsetof(RuntimeCommandLoopState, game_dll_execute) == 0x530);
static_assert(offsetof(RuntimeCommandLoopState, game_result_data) == 0x538);
static_assert(offsetof(RuntimeCommandLoopState, input_text) == 0x63c);
static_assert(offsetof(RuntimeCommandLoopState, input_text_state) == 0x65c);
static_assert(offsetof(RuntimeCommandLoopState, input_scene_identifier) == 0x698);
static_assert(offsetof(RuntimeCommandLoopState, input_text_flags) == 0x69c);
static_assert(offsetof(RuntimeCommandLoopState, input_scene_index) == 0x6a0);
static_assert(offsetof(RuntimeCommandLoopState, input_caret_tick) == 0x6a4);
static_assert(offsetof(RuntimeCommandLoopState, input_cursor) == 0x6a8);
static_assert(offsetof(RuntimeCommandLoopState, input_end) == 0x6ac);
static_assert(offsetof(RuntimeCommandLoopState, pair_available) == 0x6b0);
static_assert(offsetof(RuntimeCommandLoopState, pair_queue) == 0x6b4);
static_assert(offsetof(RuntimeCommandLoopState, pair_read_index) == 0x7b4);
static_assert(offsetof(RuntimeCommandLoopState, pair_write_index) == 0x7b8);
static_assert(offsetof(RuntimeCommandLoopState, byte_available) == 0x7bc);
static_assert(offsetof(RuntimeCommandLoopState, byte_queue) == 0x7c0);
static_assert(offsetof(RuntimeCommandLoopState, byte_read_index) == 0x7e0);
static_assert(offsetof(RuntimeCommandLoopState, byte_write_index) == 0x7e4);
static_assert(offsetof(RuntimeCommandLoopState, message_available) == 0x7e8);
static_assert(offsetof(RuntimeCommandLoopState, message_queue) == 0x7ec);
static_assert(offsetof(RuntimeCommandLoopState, message_read_index) == 0x86c);
static_assert(offsetof(RuntimeCommandLoopState, message_write_index) == 0x870);
static_assert(offsetof(RuntimeCommandLoopState, active_archive) == 0x874);
static_assert(offsetof(RuntimeCommandLoopState, async_file_host) == 0x878);
static_assert(offsetof(RuntimeCommandLoopState, byte_queue_critical_section) == 0x880);
static_assert(offsetof(RuntimeCommandLoopState, pair_queue_critical_section) == 0x898);
static_assert(offsetof(RuntimeCommandLoopState, message_queue_critical_section) == 0x8b0);
static_assert(offsetof(RuntimeCommandLoopState, resource_critical_section) == 0x8c8);
static_assert(offsetof(RuntimeCommandLoopState, path_critical_section) == 0x8e0);
static_assert(offsetof(RuntimeCommandLoopState, reset_value_1) == 0x910);
static_assert(offsetof(RuntimeCommandLoopState, reset_value_2) == 0x914);
static_assert(offsetof(RuntimeCommandLoopState, reset_value_3) == 0x918);
static_assert(offsetof(RuntimeCommandLoopState, nested_runtime_state_count) == 0x91c);
static_assert(offsetof(RuntimeCommandLoopState, nested_runtime_state_4_count) == 0x920);
static_assert(offsetof(RuntimeCommandLoopState, resource_count) == 0x924);
struct RuntimeTextInputApi
{
    std::uint8_t (*dequeue_byte)();
    DWORD(WINAPI *time_get_time)();
    std::uint32_t(__fastcall *initialize_text)(const char *text, std::uint32_t value_0014, std::uint32_t value_0018, void *font_identity, std::uint32_t low_color, std::uint32_t high_color,
        RuntimeStandaloneTextState *state);
    DisplaySceneNode *(__fastcall *acquire_scene)(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
        DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);
    std::uint32_t(__fastcall *begin_update)(std::int32_t identifier);
    void(__fastcall *draw_text)(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);
    std::uint32_t(__fastcall *end_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    std::uint32_t(__fastcall *release_scene)(std::int32_t identifier, std::int32_t owner);
};

// GAG.EXE: 0x00420E10
void __fastcall process_runtime_text_input(RuntimeCommandLoopState *state);

void set_runtime_text_input_api_for_testing(const RuntimeTextInputApi &api);
RuntimeCommandLoopState *get_runtime_command_loop_state_for_testing();

struct RuntimeCommandBounds
{
    std::uint32_t first;
    std::uint32_t second;
    std::uint32_t width;
    std::uint32_t height;
};

struct DisplayRectangle;

struct RuntimeMessageProcessorApi
{
    std::uint32_t (*dequeue_message)();
    void (*handle_message_30f)();
    void (*handle_message_311)();
    std::uint32_t(__fastcall *query_state)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *rectangle_flags);
    bool(__fastcall *update_target)(void *target, RuntimeCommandBounds *bounds, int enabled);
    void (*present)();
};

// GAG.EXE: 0x00421230
void __fastcall process_runtime_message(RuntimeCommandLoopState *state);

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
    void(__fastcall *present)(std::int32_t width, std::int32_t height, int enabled);
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
    std::uint32_t(__fastcall *set_cooperative_mode)(std::uint32_t mode);
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
std::uint32_t __fastcall set_active_display_mode(DisplayMode *mode);

// GAG.EXE: 0x004138D0
std::uint32_t restore_active_display_mode();

// GAG.EXE: 0x0041F9C0
std::uint32_t __fastcall set_active_display_mode_if_graphics_ready(DisplayMode *mode);

// GAG.EXE: 0x0041F9E0
std::uint32_t restore_active_display_mode_if_graphics_ready();

void set_display_mode_change_api_for_testing(const DisplayModeChangeApi &api);
void set_display_mode_change_state_for_testing(std::uint32_t flags, void *display, DisplayMode *current_mode);

struct DisplaySurfaceOperationApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    HRESULT(WINAPI *blt_fast)(void *surface, DWORD x, DWORD y, void *source, RECT *source_rectangle, DWORD flags);
    HRESULT(WINAPI *blt)(void *surface, RECT *destination_rectangle, void *source, RECT *source_rectangle, DWORD flags, void *effects);
    BOOL(WINAPI *bit_blt)(HDC destination, int x, int y, int width, int height, HDC source, int source_x, int source_y, DWORD operation);
    BOOL(WINAPI *pat_blt)(HDC destination, int x, int y, int width, int height, DWORD operation);
};

struct LegacyDisplayPixelFormat
{
    std::uint32_t flags;
    std::uint32_t reserved;
    std::uint32_t bits_per_pixel;
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
};

static_assert(sizeof(LegacyDisplayPixelFormat) == 0x18);

struct LegacyDirectDrawPixelFormat
{
    std::uint32_t size;
    std::uint32_t flags;
    std::uint32_t four_cc;
    std::uint32_t bits_per_pixel;
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    std::uint32_t alpha_mask;
};

struct LegacyDirectDrawSurfaceDescriptor
{
    std::uint32_t size;
    std::uint32_t flags;
    std::uint32_t height;
    std::uint32_t width;
    std::uint32_t pitch;
    std::uint32_t back_buffer_count;
    std::uint32_t mip_map_count;
    std::uint32_t alpha_bit_depth;
    std::uint32_t reserved;
    void *surface;
    std::uint32_t color_keys[8];
    LegacyDirectDrawPixelFormat pixel_format;
    std::uint32_t caps;
};

static_assert(sizeof(LegacyDirectDrawSurfaceDescriptor) == 0x6c);
static_assert(offsetof(LegacyDirectDrawSurfaceDescriptor, pixel_format) == 0x48);
static_assert(offsetof(LegacyDirectDrawSurfaceDescriptor, caps) == 0x68);

struct DisplaySurfaceCreationApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*teardown)();
    std::uint32_t(__fastcall *set_cooperative_mode)(std::uint32_t mode);
    HRESULT(WINAPI *create_direct_draw_surface)(void *display, LegacyDirectDrawSurfaceDescriptor *descriptor, void **surface, void *outer);
    HRESULT(WINAPI *get_attached_surface)(void *surface, std::uint32_t *caps, void **attached_surface);
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
};

// GAG.EXE: 0x00413340
HWND __fastcall find_top_level_display_window(HWND window);

// GAG.EXE: 0x00413590
std::uint32_t __fastcall set_display_cooperative_mode(std::uint32_t mode);

// GAG.EXE: 0x004140B0
void __fastcall operate_display_surface(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);

// GAG.EXE: 0x004139B0
void *__fastcall create_display_surface(std::int32_t width, std::int32_t height, const LegacyDisplayPixelFormat *format, std::uint32_t options);

// GAG.EXE: 0x00413F80
void teardown_display_palette_surface();

// GAG.EXE: 0x00414610
UINT __fastcall apply_display_palette(const PALETTEENTRY *palette, std::uint32_t update_flags);

// GAG.EXE: 0x00414590
void enable_display_palette_mode();

// GAG.EXE: 0x004145D0
void disable_display_palette_mode();

void set_display_palette_api_for_testing(const DisplayPaletteApi &api);
void set_display_palette_teardown_api_for_testing(const DisplayPaletteTeardownApi &api);
void set_display_cooperative_level_api_for_testing(const DisplayCooperativeLevelApi &api);
void set_display_surface_operation_api_for_testing(const DisplaySurfaceOperationApi &api);
void set_display_surface_creation_api_for_testing(const DisplaySurfaceCreationApi &api);
void set_display_palette_state_for_testing(std::uint32_t flags, std::int32_t display_bits_per_pixel, std::int32_t surface_bits_per_pixel, HDC palette_dc, HDC dib_dc, HPALETTE palette,
    std::int32_t width, std::int32_t height);
void set_display_palette_bitmap_for_testing(HBITMAP bitmap);
void set_display_palette_teardown_state_for_testing(HWND window, HPALETTE previous_palette, HBITMAP previous_bitmap);
void set_display_cooperative_state_for_testing(HWND window, void *display);
void set_display_surface_operation_state_for_testing(void *primary_surface, void *secondary_surface);
std::uint32_t get_display_palette_flags_for_testing();
const PALETTEENTRY *get_display_palette_entries_for_testing();

struct DisplayTargetApi
{
    void(__fastcall *release_backend_target)(void *backend, void *target);
};

// GAG.EXE: 0x00414540
std::uint32_t end_display_target();

void set_display_target_api_for_testing(const DisplayTargetApi &api);
void set_display_target_state_for_testing(void *backend, void *target);

struct RuntimeTargetUpdateApi
{
    void(__fastcall *draw_bounds)(RuntimeCommandBounds *bounds, int mode);
    int(__fastcall *begin_target)(std::uint32_t height, std::uint32_t second, std::uint32_t width);
    std::uint32_t (*end_target)();
};

// GAG.EXE: 0x004280D0
bool __fastcall update_runtime_target(void *unused, RuntimeCommandBounds *bounds, int mode);

void set_runtime_target_update_api_for_testing(const RuntimeTargetUpdateApi &api);
void set_runtime_target_flags_for_testing(std::uint32_t flags);

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
std::uint32_t __fastcall acquire_display_lock(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *rectangle_flags);

// GAG.EXE: 0x00419AF0
std::uint32_t release_display_lock();

void set_display_lock_release_api_for_testing(const DisplayLockReleaseApi &api);
void set_display_lock_acquire_api_for_testing(const DisplayLockAcquireApi &api);
void set_display_lock_state_for_testing(std::uint32_t flags, DWORD owner_thread, std::uint32_t recursion_count, HANDLE release_event);
void get_display_lock_state_for_testing(std::uint32_t *flags, DWORD *owner_thread, std::uint32_t *recursion_count);

struct DisplayRectangle
{
    std::int32_t left;
    std::int32_t top;
    std::int32_t right;
    std::int32_t bottom;
};

struct DisplayRectangleTransform
{
    std::int16_t x;
    std::int16_t y;
    std::uint16_t width;
    std::uint16_t height;
};

struct DisplaySceneSurface
{
    std::uint8_t unknown_00[0x34];
    std::int32_t width;
    std::int32_t height;
};

struct DisplayTraversalState;
struct DisplaySyncRequest;
struct DisplaySceneNode;

using DisplayRootRectangleCallback = void(__fastcall *)(DisplaySceneNode *root, DisplayRectangle *rectangle, int value);
using DisplayNodeRectangleCallback = void(
    __fastcall *)(DisplaySceneNode *root, int unused_register, int zero, DisplaySceneNode *node, DisplayRectangle *rectangle, void *node_state, std::uint32_t mode);

struct DisplaySceneCallbackNode
{
    std::uint32_t unknown_00;
    DisplaySceneCallbackNode *next;
    std::uint32_t flags;
    void *context;
    int(__fastcall *callback)(DisplayTraversalState *state);
};

struct DisplayTraversalState
{
    std::uint32_t flags;
    DWORD timestamp;
    std::uint32_t value_08;
    std::uint32_t value_0c;
    std::int32_t first_position;
    std::int32_t current_position;
    void *data;
    DisplayRectangle *clip_bounds;
    void *callback_context;
};

struct DisplaySceneNode
{
    std::int32_t identifier;
    std::uint32_t flags;
    std::uint32_t reference_count;
    std::uint32_t lock_count;
    DWORD lock_owner_thread;
    DisplaySceneSurface *surface;
    DisplaySceneNode *next;
    std::int32_t callback_first_position;
    std::int32_t callback_current_position;
    std::int32_t callback_alternate_position;
    std::int32_t sync_secondary_position;
    std::uint32_t unknown_2c;
    std::int32_t x;
    std::int32_t y;
    std::int32_t previous_x;
    std::int32_t previous_y;
    std::int32_t x_offset;
    std::int32_t y_offset;
    std::int32_t width;
    std::int32_t height;
    std::int32_t previous_width;
    std::int32_t previous_height;
    std::int32_t extra_width;
    std::int32_t extra_height;
    std::uint32_t state_60;
    DisplaySceneCallbackNode *callbacks;
    std::uint32_t owner_count;
    std::int32_t primary_owner;
    std::int32_t owners[128];
    DisplayNodeRectangleCallback rectangle_callback;
    DisplayRootRectangleCallback root_rectangle_callback;
    std::int32_t callback_position;
    std::uint32_t rectangle_callback_state;
    std::uint32_t rectangle_callback_metadata[7];
    std::uint32_t palette_source[256];
    std::uint32_t palette_mapping[256];
};

static_assert(sizeof(DisplayRectangle) == 0x10);
static_assert(sizeof(DisplayRectangleTransform) == 0x08);
static_assert(offsetof(DisplaySceneNode, surface) == 0x14);
static_assert(offsetof(DisplaySceneNode, lock_count) == 0x0c);
static_assert(offsetof(DisplaySceneNode, lock_owner_thread) == 0x10);
static_assert(offsetof(DisplaySceneNode, next) == 0x18);
static_assert(offsetof(DisplaySceneNode, x) == 0x30);
static_assert(offsetof(DisplaySceneNode, state_60) == 0x60);
static_assert(offsetof(DisplaySceneNode, callbacks) == 0x64);
static_assert(offsetof(DisplaySceneNode, owner_count) == 0x68);
static_assert(offsetof(DisplaySceneNode, primary_owner) == 0x6c);
static_assert(offsetof(DisplaySceneNode, owners) == 0x70);
static_assert(offsetof(DisplaySceneNode, callback_position) == 0x278);
static_assert(offsetof(DisplaySceneNode, rectangle_callback) == 0x270);
static_assert(offsetof(DisplaySceneNode, root_rectangle_callback) == 0x274);
static_assert(offsetof(DisplaySceneNode, rectangle_callback_state) == 0x27c);
static_assert(offsetof(DisplaySceneNode, palette_source) == 0x29c);
static_assert(offsetof(DisplaySceneNode, palette_mapping) == 0x69c);
static_assert(sizeof(DisplaySceneNode) == 0xa9c);
static_assert(sizeof(DisplayTraversalState) == 0x24);

struct DisplaySceneCallbackApi
{
    DWORD(WINAPI *time_get_time)();
};

struct DisplaySyncRequest
{
    DisplaySceneNode *node;
    DisplayRectangle *geometry;
    std::int32_t *secondary_position;
    std::int32_t *primary_position;
};

struct DisplaySceneSyncApi
{
    int(__fastcall *synchronize)(void *context, void *payload, std::uint32_t mode);
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
    std::uint32_t(__fastcall *acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *dirty_flags);
    int(__fastcall *synchronize_node)(DisplaySceneNode *node, DisplayRectangle *rectangle);
    void(__fastcall *publish_node)(DisplaySceneNode *node);
    std::uint32_t (*release_mode_1000)();
    std::uint32_t (*release_lock)();
};

struct DisplayPixelFormatDescriptor
{
    std::uint32_t flags;
    std::uint32_t bits_per_pixel;
    std::uint32_t red_mask;
    std::uint32_t green_mask;
    std::uint32_t blue_mask;
    std::uint32_t palette_count;
    const std::uint32_t *palette_source;
    const std::uint32_t *palette_entries;
};

static_assert(sizeof(DisplayPixelFormatDescriptor) == 0x20);

static_assert(sizeof(DisplaySyncRequest) == 0x10);

// GAG.EXE: 0x0041B560
int __fastcall process_scene_node_callbacks(DisplaySceneNode *node);

// GAG.EXE: 0x0041B690
bool __fastcall clip_display_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041B640
bool __fastcall constrain_display_rectangle_to_surface(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041B790
void __fastcall trim_display_rectangle_overlap(DisplayRectangle *rectangle, DisplaySceneNode *node);

// GAG.EXE: 0x0041B860
void __fastcall accumulate_scene_node_rectangle(DisplayRectangle *rectangle, DisplaySceneNode *node);

// GAG.EXE: 0x0041B6F0
void __fastcall merge_display_rectangle(DisplayRectangle *destination, const DisplayRectangleTransform *transform, const DisplayRectangle *source);

// GAG.EXE: 0x004195B0
std::uint32_t __fastcall queue_display_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x0041AC70
bool __fastcall contains_display_scene_node(std::int32_t identifier);

// GAG.EXE: 0x004190D0
int __fastcall synchronize_display_scene_node(DisplaySceneNode *node, DisplayRectangle *output_rectangle);

// GAG.EXE: 0x00419230
void __fastcall publish_display_scene_node(DisplaySceneNode *node);

// GAG.EXE: 0x00419710
std::uint32_t __fastcall dispatch_display_scene_update(void *target, std::uint32_t options);

struct FramebufferInvalidateApi
{
    std::uint32_t(__fastcall *acquire_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *rectangle_flags);
    std::uint32_t(__fastcall *dispatch_update)(void *target, std::uint32_t options);
    std::uint32_t (*release_lock)();
};

// GAG.EXE: 0x00427830
void __fastcall invalidate_game_framebuffer_rect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

void set_framebuffer_invalidate_api_for_testing(const FramebufferInvalidateApi &api);

// GAG.EXE: 0x00419550
std::uint32_t __fastcall find_available_display_scene_index(std::uint32_t candidate);

// GAG.EXE: 0x00419600
std::uint32_t __fastcall wait_for_display_scene_ready(std::uint32_t timeout);

// GAG.EXE: 0x00419660
std::uint32_t __fastcall set_display_clip_rectangle(DisplayRectangle *rectangle);

// GAG.EXE: 0x00419B60
std::uint32_t release_display_lock_mode_1000();

// GAG.EXE: 0x0041ACC0
DisplaySceneNode *__fastcall lock_display_scene_node(std::int32_t identifier);

// GAG.EXE: 0x0041AD50
void __fastcall unlock_display_scene_node(std::int32_t identifier);

// GAG.EXE: 0x0041ADC0
bool __fastcall set_display_scene_primary_owner(std::int32_t identifier, std::int32_t owner, bool replace_existing);

struct DisplaySceneDescriptor
{
    std::int16_t x;
    std::int16_t y;
    std::int16_t width;
    std::int16_t height;
    std::uint16_t present;
    std::uint16_t reserved;
    std::int32_t pixels;
};

static_assert(sizeof(DisplaySceneDescriptor) == 0x10);

// GAG.EXE: 0x0041AE60
std::int32_t __fastcall query_display_scene_by_index(std::int32_t index, DisplaySceneDescriptor *descriptor, std::uint32_t *callback_metadata);

// GAG.EXE: 0x0041AFA0
std::uint32_t __fastcall blit_bitmap_with_optional_palette_remap(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source,
    DisplayRectangle *rectangle, std::uint32_t flags);

// GAG.EXE: 0x0041AF20
std::uint32_t __fastcall offset_display_scene_node(std::int32_t identifier, std::int32_t x_delta, std::int32_t y_delta);

// GAG.EXE: 0x0041B280
std::uint32_t __fastcall begin_display_scene_update(std::int32_t identifier);

// GAG.EXE: 0x0041B360
std::uint32_t __fastcall end_display_scene_update(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);

struct DisplayRootRegionApi
{
    std::uint32_t(__fastcall *begin_scene_update)(std::int32_t identifier);
    std::uint32_t(__fastcall *end_scene_update)(std::int32_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
};

// GAG.EXE: 0x0041B1F0
std::uint32_t __fastcall update_display_root_region(DisplaySceneNode *scene, DisplayRectangle *rectangle, std::uint32_t callback_value);

void set_display_root_region_api_for_testing(const DisplayRootRegionApi &api);
void set_display_root_region_state_for_testing(std::uint32_t lock_flags, DisplaySceneNode *root);

struct ClearRuntimeDisplayApi
{
    std::uint32_t(__fastcall *acquire_display_lock)(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, std::uint32_t *rectangle_flags);
    std::uint32_t(__fastcall *set_clip_rectangle)(DisplayRectangle *rectangle);
    void(__fastcall *operate_surface)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);
    std::uint32_t (*release_display_lock)();
    std::uint32_t(__fastcall *update_root_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, std::uint32_t callback_value);
};

// GAG.EXE: 0x00427880
void clear_runtime_display();

void set_clear_runtime_display_api_for_testing(const ClearRuntimeDisplayApi &api);
void set_clear_runtime_display_size_for_testing(std::uint16_t width, std::uint16_t height);

// GAG.EXE: 0x0041A830
std::uint32_t __fastcall add_display_scene_callback(std::int32_t identifier, int(__fastcall *callback)(DisplayTraversalState *state), const void *context, std::uint32_t context_size,
    std::uint32_t flags);

// GAG.EXE: 0x0041B950
void __fastcall fill_display_scene_rectangle_8(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

// GAG.EXE: 0x0041BE60
void __fastcall fill_display_scene_rectangle_16(DisplaySceneNode *node, DisplayRectangle *rectangle, int value);

// GAG.EXE: 0x0041B9D0
void __fastcall composite_transparent_8_to_8(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041BC40
void __fastcall composite_opaque_8_to_8(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041BEE0
void __fastcall composite_transparent_indexed_to_8(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041C180
void __fastcall composite_opaque_indexed_to_8(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041C400
void __fastcall composite_transparent_indexed_to_16(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041C660
void __fastcall composite_opaque_indexed_to_16(DisplaySceneNode *destination, std::int32_t destination_x, std::int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle,
    void *source_state, std::uint32_t mode);

// GAG.EXE: 0x0041A480
std::uint32_t __fastcall release_display_scene_node(std::int32_t identifier, std::int32_t owner);

// GAG.EXE: 0x0041C8C0
void __fastcall build_indexed_to_16_palette(std::uint32_t *source_state, const std::uint32_t *destination_state);

// GAG.EXE: 0x0041CA00
void __fastcall build_indexed_to_indexed_palette(std::uint32_t *source_state, const std::uint32_t *destination_state);

// GAG.EXE: 0x00418EE0
void __fastcall configure_display_scene_format(DisplaySceneNode *node, const DisplayPixelFormatDescriptor *format);

// GAG.EXE: 0x0041AA10
bool __fastcall configure_display_scene_palette(DisplaySceneNode *node, const std::uint32_t *palette, std::uint32_t count);

// GAG.EXE: 0x00419BC0
DisplaySceneNode *__fastcall acquire_display_scene_node(std::uint32_t index, std::int32_t x, std::int32_t y, std::uint32_t width, std::uint32_t height, std::uint32_t flags, std::int32_t owner,
    DisplaySceneDescriptor *descriptor, const DisplayPixelFormatDescriptor *format);

// GAG.EXE: 0x004192B0
std::uint32_t *__fastcall initialize_display_scene_host(std::int32_t primary_position, const DisplayPixelFormatDescriptor *format, std::int32_t width, std::int32_t height,
    int(__fastcall *synchronize)(void *context, void *payload, std::uint32_t mode), void *context, std::uint32_t worker_interval);

// GAG.EXE: 0x004194B0
std::uint32_t shutdown_display_scene_host();

// GAG.EXE: 0x0041B3F0
DWORD WINAPI run_display_scene_worker(std::uint32_t *flags);

void set_display_clip_bounds_for_testing(const DisplayRectangle &bounds);
void set_display_scene_callback_api_for_testing(const DisplaySceneCallbackApi &api);
void set_display_scene_sync_api_for_testing(const DisplaySceneSyncApi &api);
void set_display_scene_memory_api_for_testing(const DisplaySceneMemoryApi &api);
void set_display_scene_host_api_for_testing(const DisplaySceneHostApi &api);
void set_display_scene_worker_api_for_testing(const DisplaySceneWorkerApi &api);
void set_display_scene_sync_state_for_testing(void *context, DisplaySceneNode *root_node);
void set_display_scene_worker_state_for_testing(std::uint32_t interval, std::uint32_t *palette_source_state);
std::uint32_t get_display_scene_worker_rate_for_testing();
void set_display_scene_root_primary_position_for_testing(std::int32_t primary_position);
void set_display_lock_acquire_state_for_testing(HANDLE gate_event, std::uint32_t busy, const DisplayRectangle &pending_rectangle, std::int32_t width, std::int32_t height,
    DisplaySceneNode *scene_head);
DisplayRectangle get_display_pending_rectangle_for_testing();

struct RuntimeCommandLoopApi
{
    void (*begin_first)();
    void (*begin_second)();
    void(__fastcall *begin_third)(int value);
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(__fastcall *process)(RuntimeCommandLoopState *state);
    LPARAM (*get_script_state)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void (*cancel_first)();
    void (*cancel_second)();
    void (*cancel_third)();
    void (*complete_first)();
};

struct RuntimeSessionResetApi
{
    std::uint32_t (*stop_game_dll)();
    RuntimeTreeNode *(*get_tree_root)();
    void(__fastcall *destroy_tree_resources)(void *identity);
    std::uint32_t(__fastcall *deactivate_tree)(void *identity, void *replacement_identity);
    void (*reset_display_state)();
    void(__fastcall *request_resource_destruction)(void *identity);
    void (*destroy_fixed_name_nodes)();
    void (*purge_named_nodes)();
    void (*destroy_object_states)();
    void (*destroy_visual_objects)();
    void (*clear_command_definitions)();
    void (*remove_generic_resources)();
    std::uint32_t(__fastcall *close_archive)(CdfArchive *archive);
    std::uint32_t(__fastcall *destroy_async_host)(AsyncFileHost *host);
    void(__fastcall *operate_surface)(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::int32_t mode);
    RuntimeNamedNode *(__fastcall *get_named_node)(const char *name);
    DWORD(WINAPI *get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x004263A0
void reset_runtime_session();

void set_runtime_session_reset_api_for_testing(const RuntimeSessionResetApi &api);
void set_runtime_session_reset_storage_for_testing(std::uint32_t value);
std::uint32_t get_runtime_session_reset_storage_for_testing(std::uint32_t index);
std::uint32_t get_runtime_pointer_event_record_for_testing(std::uint32_t index);
void set_embedded_script_runtime_flags_for_testing(std::uint32_t flags, std::uint32_t palette_flags);

// GAG.EXE: 0x00420CE0
int __fastcall run_runtime_command_loop(RuntimeCommandLoopState *state);

void set_runtime_command_loop_api_for_testing(const RuntimeCommandLoopApi &api);

struct RuntimeExternalCommandApi
{
    LRESULT(WINAPI *send_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    void(__fastcall *process_message)(RuntimeCommandLoopState *state);
    int(__fastcall *run_command_loop)(RuntimeCommandLoopState *state);
    void(WINAPI *sleep)(DWORD milliseconds);
};

// GAG.EXE: 0x00421010
std::uint32_t run_pending_runtime_external_command();

void set_runtime_external_command_api_for_testing(const RuntimeExternalCommandApi &api);
void set_runtime_external_command_state_for_testing(const RuntimeCommandLoopState &state);
const RuntimeCommandLoopState &get_runtime_external_command_state_for_testing();

// GAG.EXE: 0x00421530
DWORD WINAPI execute_script_commands(LPVOID parameter);

enum class RuntimeScriptOpcodeDisposition : std::uint32_t
{
    unhandled,
    complete,
    pause,
    commit_cursor,
    finish_link,
    restart_outer
};

// Non-original dispatcher slice used to compose and test GAG.EXE:0x00421530.
RuntimeScriptOpcodeDisposition execute_simple_runtime_script_opcode_for_testing(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, std::uint32_t opcode,
    std::int32_t random_value = 0, std::uint32_t saved_cursor = 0xffffffff);

struct RuntimeScriptExecutorApi
{
    DWORD(WINAPI *set_batch_limit)(DWORD limit);
    DWORD(WINAPI *get_tick_count)();
    DWORD(WINAPI *time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    void(__fastcall *process_children)(std::uint32_t maximum_end_position);
    void(__fastcall *process_message)(RuntimeCommandLoopState *state);
    void(__fastcall *process_text_input)(RuntimeCommandLoopState *state);
    std::uint32_t (*process_pair_message)();
    int(__fastcall *run_command_loop)(RuntimeCommandLoopState *state);
    RuntimeTreeNode *(__fastcall *resolve_tree)(void *identity);
    bool (*synchronize_plan_mode)();
    bool (*process_pending_tree_switch)(RuntimeTreeNode *node);
    void (*acknowledge_event)();
    std::uint32_t (*run_external_command)();
    std::uint32_t(__fastcall *activate_link)(RuntimeTreeLink7C *link);
    std::uint32_t(__fastcall *parse_opcode)(ScriptParserState *parser);
    RuntimeScriptOpcodeDisposition (
        *dispatch_opcode)(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, std::uint32_t opcode, std::int32_t random_value, std::uint32_t saved_cursor);
    std::int32_t(__fastcall *select_random)(std::int32_t minimum, std::int32_t maximum);
};

void set_runtime_script_executor_api_for_testing(const RuntimeScriptExecutorApi &api);

// GAG.EXE: 0x0041CE40
void application_hook_no_op_1();

// GAG.EXE: 0x0041CE50
void application_hook_no_op_2();

// GAG.EXE: 0x0041CDC0
void __fastcall set_application_lock_flag(ApplicationState *state);

// GAG.EXE: 0x0041CD30
void __fastcall set_application_inactive_flags(ApplicationState *state);

struct CursorStateApi
{
    BOOL(WINAPI *get_cursor_position)(LPPOINT point);
    int(WINAPI *get_system_metrics)(int index);
};

// GAG.EXE: 0x0041CD50
void __fastcall clear_runtime_active_flag(ApplicationState *state);

// GAG.EXE: 0x0041CDD0
void __fastcall clear_application_lock_flag(ApplicationState *state);

// GAG.EXE: 0x00417970
void __fastcall free_heap_memory(void *memory);

void set_cursor_state_api_for_testing(const CursorStateApi &api);

} // namespace gag
