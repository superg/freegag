#pragma once

#include "pcm_format.h"
#include "resource_types.h"

namespace gag
{
struct RuntimeSoundStatus;
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
    uint16_t palette_version;
    uint16_t palette_entry_count;
    PALETTEENTRY palette_entries[0x100];
    uint16_t destination_x;
    uint16_t destination_y;
    uint16_t destination_stride;
    uint16_t destination_reserved;
    uint32_t destination_bits_per_pixel;
    uint32_t descriptor_2;
    uint8_t *destination_pixels;
    uint8_t *indexed_pixels;
    uint32_t indexed_stride;
    uint32_t indexed_height;
    HWND window;
    uint32_t media_flags;
    uint32_t error_state;
    uint32_t scale_x;
    uint32_t scale_y;
    void *extension_data;
    void *source_data;
    void *format_data;
    void *frame_header;
    void *chunk_header;
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
    RuntimePcmFormat format;
    uint16_t extra_format_size;
};

struct RuntimeFontFormat
{
    uint32_t unknown_0000;
    int32_t fixed_cell_width;
    int32_t fixed_cell_height;
};

struct RuntimePcmWaveFile
{
    uint8_t riff_and_format_headers[0x14];
    RuntimePcmFormat format;
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
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    HANDLE(WINAPI *create_thread)(LPSECURITY_ATTRIBUTES attributes, SIZE_T stack_size, LPTHREAD_START_ROUTINE start_address, LPVOID parameter, DWORD creation_flags, LPDWORD thread_id);
    BOOL(WINAPI *close_handle)(HANDLE handle);
};



struct RuntimeResourcePaletteConfigureApi
{
    bool (*set_primary_owner)(intptr_t identifier, intptr_t owner, bool replace_existing);
    bool (*configure_palette)(DisplaySceneNode *node, const uint32_t *palette, uint32_t count);
};



struct RuntimeMediaBackendFinalizeApi
{
    DWORD(WINAPI *wait_for_single_object)(HANDLE handle, DWORD milliseconds);
    BOOL(WINAPI *release_mutex)(HANDLE mutex);
    uint8_t (*convert_bitmap)(RuntimeMediaBackend *backend);
};


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
};



struct RuntimeAnimationFrameAcquireApi
{
    uint32_t (*read_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void (*fail_animation)(RuntimeMediaBackend *backend, uint32_t error);
};


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


struct RuntimeAnimationCompletionApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*set_stream_position)(AsyncFileRecord *record, uint32_t position);
};


struct RuntimeAnimationAudioApi
{
    uint32_t (*time_get_time)();
    void(WINAPI *sleep)(DWORD milliseconds);
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    LPVOID(WINAPI *heap_realloc)(HANDLE heap, DWORD flags, LPVOID memory, SIZE_T bytes);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*queue_sound_data)(uint32_t handle, void *data, uint32_t size, int32_t replace);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*create_sound)(const RuntimePcmFormat *format);
    uint32_t (*query_sound)(uint32_t handle, RuntimeSoundStatus *status);
    uint32_t (*set_playback_marker)(uint32_t handle, uint32_t marker);
    uint32_t (*set_schedule_marker)(uint32_t handle, uint32_t marker);
};


struct RuntimeAnimationWorkerApi
{
    void(WINAPI *sleep)(DWORD milliseconds);
    uint32_t (*time_get_time)();
    void(WINAPI *exit_thread)(DWORD exit_code);
};


} // namespace gag
