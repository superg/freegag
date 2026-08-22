#pragma once

#include "text_types.h"

namespace gag
{
struct RuntimeSoundBufferNode
{
    void *data;
    RuntimeSoundBufferNode *next;
    uint32_t offset;
    uint32_t schedule_offset;
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



struct RuntimeWaveOutCallbackApi
{
    BOOL(WINAPI *post_message)(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    DWORD(WINAPI *time_get_time)();
};


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


struct RuntimeSoundClassApi
{
    ATOM(WINAPI *register_class)(const WNDCLASSA *window_class);
    HANDLE(WINAPI *create_mutex)(LPSECURITY_ATTRIBUTES attributes, BOOL initial_owner, LPCSTR name);
};



struct RuntimeSoundFormatCleanupApi
{
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};



} // namespace gag
