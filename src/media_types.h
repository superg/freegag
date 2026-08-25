#pragma once

#include <thread>
#include "pcm_format.h"
#include "resource_types.h"

namespace freegag
{
enum RuntimeMediaDataType : uint8_t
{
    RUNTIME_MEDIA_DATA_UNKNOWN = 0,
    RUNTIME_MEDIA_DATA_BITMAP = 1,
    RUNTIME_MEDIA_DATA_WAVE = 2,
    RUNTIME_MEDIA_DATA_ANIMATION = 3,
    RUNTIME_MEDIA_DATA_CONFIGURATION = 4,
    RUNTIME_MEDIA_DATA_ARCHIVE = 5
};

enum RuntimeMediaFlag : uint32_t
{
    RUNTIME_MEDIA_PAUSED = 0x00000001,
    RUNTIME_MEDIA_RESOURCE_PENDING = 0x00000020,
    RUNTIME_MEDIA_FULL_REFRESH_PENDING = 0x00000040,
    RUNTIME_MEDIA_SKIP_PRESENTATION = 0x00000100,
    RUNTIME_MEDIA_ONE_STEP = 0x00000200,
    RUNTIME_MEDIA_LOOP = 0x00000400,
    RUNTIME_MEDIA_CLOSE_AFTER_PLAYBACK = 0x00000800,
    RUNTIME_MEDIA_CLOSE_REQUESTED = 0x00001000,
    RUNTIME_MEDIA_PAUSE_NOTIFIED = 0x00002000,
    RUNTIME_MEDIA_PALETTE_CHANGED = 0x00004000,
    RUNTIME_MEDIA_PIXELS_CHANGED = 0x00008000,
    RUNTIME_MEDIA_STOP_REQUESTED = 0x00010000,
    RUNTIME_MEDIA_RESTART_REQUESTED = 0x00020000,
    RUNTIME_MEDIA_SYNCHRONIZED_TIMING = 0x00100000,
    RUNTIME_MEDIA_AUDIO_STARTED = 0x00400000,
    RUNTIME_MEDIA_AUDIO_DISABLED = 0x00800000,
    RUNTIME_MEDIA_MEMORY_BACKED = 0x01000000,
    RUNTIME_MEDIA_STREAM_BACKED = 0x02000000,
    RUNTIME_MEDIA_STORAGE_MASK = RUNTIME_MEDIA_MEMORY_BACKED | RUNTIME_MEDIA_STREAM_BACKED,
    RUNTIME_MEDIA_NO_PALETTE = 0x04000000,
    RUNTIME_MEDIA_DECODE_STARTED = 0x10000000,
    RUNTIME_MEDIA_FRAME_DECODED = 0x20000000,
    RUNTIME_MEDIA_LOOP_BOUNDARY = 0x40000000,
    RUNTIME_MEDIA_INITIALIZING = 0x80000000
};

inline constexpr uint32_t RUNTIME_ANIMATION_PAUSED = 0x01000000;

struct RuntimeSoundStatus;
struct RuntimeMediaBackend;
struct DisplaySceneDescriptor;
using RuntimeAnimationCallback = int32_t (*)(RuntimeMediaBackend *backend);

struct RuntimeMediaBackend
{
    uint32_t type;
    void *identity;
    RuntimeThreadId owner_thread;
    uint32_t recursion_count;
    RuntimeMediaBackend *previous;
    RuntimeMediaBackend *next;
    const void *comparison_palette;
    uint16_t palette_version;
    PaletteEntry palette_entries[0x100];
    uint16_t destination_x;
    uint16_t destination_y;
    uint16_t destination_stride;
    uint16_t destination_reserved;
    uint32_t destination_bits_per_pixel;
    uint8_t *destination_pixels;
    uint8_t *indexed_pixels;
    uint32_t indexed_stride;
    uint32_t indexed_width;
    uint32_t indexed_height;
    uint16_t indexed_origin_x;
    uint16_t indexed_origin_y;
    bool owns_indexed_pixels;
    uint32_t media_flags;
    uint32_t error_state;
    uint32_t scale_x;
    uint32_t scale_y;
    void *extension_data;
    void *source_data;
    BitmapFileHeader bitmap_file;
    BitmapInfoHeader bitmap_format;
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
    std::jthread *worker_thread;
};


#pragma pack(push, 1)
struct RuntimeAnimationFileHeader
{
    uint32_t file_size;
    uint16_t signature;
    uint16_t frame_count;
    uint16_t width;
    uint16_t height;
    uint8_t reserved_000c[4];
    uint32_t frame_duration;
    uint8_t reserved_0014[0x3c];
    uint32_t data_start_offset;
    uint32_t data_end_offset;
    uint8_t reserved_0058[0x28];
};

struct RuntimeAnimationFrameHeader
{
    uint32_t size;
    uint16_t signature;
    uint16_t chunk_count;
    uint8_t reserved_0008[8];
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
};

struct RuntimeAnimationSoundFormatChunk
{
    RuntimeAnimationChunkHeader chunk;
    uint8_t reserved_0006[0x0c];
    RuntimePcmFormat format;
    uint16_t extra_format_size;
};

struct RuntimeFontFormat
{
    uint32_t reserved_0000;
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


enum class RuntimeAnimationControlResult
{
    DECODE_FRAME,
    WAIT,
    EXIT
};

} // namespace freegag
