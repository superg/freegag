#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include "application.h"
#include "audio.h"
#include "cdf_archive.h"
#include "display_host.h"
#include "display_scene.h"
#include "extra/save_load.h"
#include "extra/synthesized_resources.h"
#include "game_host.h"
#include "media.h"
#include "portable_string.h"
#include "resource.h"
#include "runtime.h"
#include "runtime_clock.h"
#include "runtime_model.h"
#include "runtime_services.h"
#include "runtime_tree.h"
#include "runtime_types.h"
#include "script.h"
#include "text.h"
#include "xtet/api.h"

namespace freegag
{

struct DisplayPresenterHostState
{
    uint32_t flags{};
    int32_t width;
    int32_t height;
    int32_t bits_per_pixel;
    void *pixels;
    std::recursive_mutex mutex;
    PaletteEntry palette_entries[0x100]{};
};

struct RuntimeGenericChildStateFields
{
    uint32_t values[11];
    DisplayRectangle rectangle;
};

union RuntimeGenericChildState
{
    uint32_t words[15];
    RuntimeGenericChildStateFields fields;
};

inline constexpr uint32_t RUNTIME_DEFAULT_PALETTE_TRANSITION_STEP = 6;
inline constexpr uint32_t RUNTIME_DEFAULT_RECTANGLE_TRANSITION_STEP_SIZE = 5;
inline constexpr uint32_t RUNTIME_DEFAULT_AVAILABLE_SCENE_TRANSITIONS = RUNTIME_SCENE_TRANSITION_IMMEDIATE | RUNTIME_SCENE_TRANSITION_RECTANGLE;
inline constexpr int32_t RUNTIME_DEFAULT_RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND = 1700;

inline DisplayPresenterHostState display_presenter_host_state;

inline DisplayRectangleTransform display_rectangle_transform(const DisplaySceneDescriptor &descriptor)
{
    return { descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
}
inline uint32_t &display_palette_flags = display_presenter_host_state.flags;
inline int32_t &display_palette_width = display_presenter_host_state.width;
inline int32_t &display_palette_height = display_presenter_host_state.height;
inline int32_t &display_palette_bits_per_pixel = display_presenter_host_state.bits_per_pixel;
inline void *&display_palette_pixels = display_presenter_host_state.pixels;
inline std::recursive_mutex &display_host_mutex = display_presenter_host_state.mutex;
inline PaletteEntry (&display_palette_entries)[0x100] = display_presenter_host_state.palette_entries;

struct DesktopPresentationState
{
    bool fullscreen;
};

inline DesktopPresentationState desktop_presentation_state;
inline bool desktop_fullscreen_toggle_latched;

inline bool script_random_seeded;


struct DisplaySceneHostState
{
    uint32_t lock_flags;
    RuntimeMutex *lock_mutex;
    RuntimeManualResetEvent *lock_gate_event;
    uint32_t lock_busy;
    RuntimeManualResetEvent *lock_release_event;
    RuntimeThreadId lock_owner_thread;
    uint32_t lock_recursion_count;
    intptr_t root_primary_position;
    int32_t width;
    int32_t height;
    int32_t root_secondary_position;
    DisplayRectangle clip_bounds;
    DisplayRectangle pending_rectangle;
    DisplayPixelFormatDescriptor *palette_source_state;
    std::jthread *worker_thread;
    DisplaySceneSynchronizeCallback synchronize;
    void *sync_context;
    uint32_t worker_interval;
    uint32_t worker_rate;
    uint32_t scene_count;
    DisplaySceneNode *root;
    DisplaySceneNode *head;
};


inline DisplaySceneHostState display_scene_host_state;
inline uint32_t &display_lock_flags = display_scene_host_state.lock_flags;
inline RuntimeMutex *&display_lock_mutex = display_scene_host_state.lock_mutex;
inline RuntimeManualResetEvent *&display_lock_gate_event = display_scene_host_state.lock_gate_event;
inline uint32_t &display_lock_busy = display_scene_host_state.lock_busy;
inline RuntimeManualResetEvent *&display_lock_release_event = display_scene_host_state.lock_release_event;
inline RuntimeThreadId &display_lock_owner_thread = display_scene_host_state.lock_owner_thread;
inline uint32_t &display_lock_recursion_count = display_scene_host_state.lock_recursion_count;
inline intptr_t &display_scene_root_primary_position = display_scene_host_state.root_primary_position;
inline int32_t &display_width = display_scene_host_state.width;
inline int32_t &display_height = display_scene_host_state.height;
inline int32_t &display_scene_root_secondary_position = display_scene_host_state.root_secondary_position;
inline DisplayRectangle &display_clip_bounds = display_scene_host_state.clip_bounds;
inline DisplayRectangle &display_pending_rectangle = display_scene_host_state.pending_rectangle;
inline DisplayPixelFormatDescriptor *&display_palette_source_state = display_scene_host_state.palette_source_state;
inline std::jthread *&display_scene_worker_thread = display_scene_host_state.worker_thread;
inline DisplaySceneSynchronizeCallback &display_scene_synchronize = display_scene_host_state.synchronize;
inline void *&display_scene_sync_context = display_scene_host_state.sync_context;
inline uint32_t &display_scene_worker_interval = display_scene_host_state.worker_interval;
inline uint32_t &display_scene_worker_rate = display_scene_host_state.worker_rate;
inline uint32_t &display_scene_count = display_scene_host_state.scene_count;
inline DisplaySceneNode *&display_scene_root = display_scene_host_state.root;
inline DisplaySceneNode *&display_scene_head = display_scene_host_state.head;
inline DisplaySceneSurface display_scene_surface_state{};
inline ScriptRuntimeRoot *script_runtime_root;

bool strings_equal(const char *left, const char *right);

inline RuntimeCommandLoopState runtime_display_context{};
inline char runtime_graphics_resource_directory[0x104]{};
inline ScriptRuntimeRoot graphics_script_runtime_root{};
inline RuntimeSceneSlot *runtime_scene_slots = graphics_script_runtime_root.command_definitions;
inline RuntimePointerRegion *&runtime_pointer_regions = graphics_script_runtime_root.global_pointer_regions;
inline uint32_t &graphics_host_flags = runtime_display_context.flags;
inline uint32_t &runtime_scene_control_flags = runtime_display_context.flags;
inline uint32_t &runtime_resource_transition_flags = runtime_display_context.accumulated_tree_flags;
inline uint32_t &runtime_command_state = runtime_display_context.external_command_pending;
inline uint32_t &runtime_target_flags = runtime_display_context.target_flags;
inline int32_t &runtime_scene_x = runtime_display_context.scene_x;
inline int32_t &runtime_scene_y = runtime_display_context.scene_y;
inline void *&deferred_runtime_scene_identity = runtime_display_context.deferred_scene_identity;
inline void *&current_runtime_scene_identity = runtime_display_context.current_scene_identity;
inline uintptr_t &runtime_state_value = runtime_display_context.deferred_state_value;
inline uintptr_t &saved_runtime_state_value = runtime_display_context.current_state_value;
inline void *&saved_default_comment_scene_identity = runtime_display_context.saved_default_comment_scene_identity;
inline void *&runtime_pointer_root_identity = runtime_display_context.runtime_tree_identity;
inline RuntimePointerRegion *&active_runtime_pointer_region = runtime_display_context.active_pointer_region;

inline uintptr_t runtime_pointer_event_record[16]{};
inline uint32_t runtime_pointer_state_mask;
inline void *runtime_pointer_state_owner;
inline void *runtime_pointer_event_state_object;

inline void enqueue_runtime_pointer_event()
{
    uintptr_t record[16]{};
    record[0] = runtime_pointer_state_mask;
    record[1] = reinterpret_cast<uintptr_t>(runtime_pointer_state_owner);
    record[2] = reinterpret_cast<uintptr_t>(runtime_pointer_event_state_object);
    std::memcpy(record + 3, runtime_pointer_event_record, 13 * sizeof(uintptr_t));
    enqueue_runtime_event_record(record);
}
inline intptr_t runtime_text_input_guarded_scene;

inline void release_runtime_text_input_scene_guard()
{
    if(runtime_text_input_guarded_scene != 0)
    {
        end_display_scene_update(runtime_text_input_guarded_scene, nullptr, nullptr);
        runtime_text_input_guarded_scene = 0;
    }
}
inline char &runtime_display_reset_byte = runtime_display_context.input_text[0];
inline std::jthread *&runtime_display_thread = runtime_display_context.script_thread;
inline intptr_t &runtime_display_scene_identifier = runtime_display_context.input_alternate_scene_identifier;
inline void *&runtime_display_host = runtime_display_context.display_scene_host;
inline uint32_t &runtime_resource_count = runtime_display_context.resource_wait_count;
inline uint32_t &runtime_property_value = runtime_display_context.script_clock;
inline PaletteEntry runtime_transition_palette[0x101]{};
inline RuntimeMutex &runtime_resource_mutex = runtime_display_context.resource_mutex;
inline AsyncFileHost *&runtime_resource_host = runtime_display_context.async_file_host;
inline CdfArchive *&runtime_resource_archive = runtime_display_context.active_archive;
inline intptr_t &runtime_resource_archive_alternate_stream = runtime_display_context.archive_alternate_stream;
inline int32_t &runtime_resource_stream_rate_bytes_per_millisecond = runtime_display_context.resource_stream_rate_bytes_per_millisecond;
inline uint8_t &runtime_resource_archive_state = runtime_display_context.resource_archive_state;
inline void *&runtime_resource_cache_parent_identity = runtime_display_context.resource_cache_parent_identity;
inline RuntimeHeap *&runtime_resource_heap = runtime_display_context.resource_heap;
inline uint32_t &runtime_resource_streamed_count = runtime_display_context.resource_count;
inline RuntimeHeap *runtime_media_backend_heap;
inline RuntimeMutex *runtime_media_backend_mutex;
inline bool runtime_media_backend_initialized;
inline RuntimeMediaBackend *runtime_media_backend_head;
inline RuntimeMediaBackend *runtime_media_backend_tail;
inline uint32_t runtime_animation_control_flags;
inline RuntimeMutex *runtime_generic_backend_mutex;
inline RuntimeGenericBackend *runtime_generic_backend_head;
inline uint32_t runtime_generic_backend_enabled;
inline RuntimeMutex &runtime_game_dll_mutex = runtime_display_context.resource_mutex;
inline RuntimeGameInputHandler &runtime_game_input_handler = runtime_display_context.game_input_handler;
inline RuntimeGameDllExecute &runtime_game_dll_execute = runtime_display_context.game_dll_execute;
inline RuntimeGameHostContext runtime_game_host_context{};
inline uint32_t &runtime_resource_palette_bits_per_pixel = runtime_game_host_context.bits_per_pixel;
inline void *&runtime_media_objects_parent_identity = runtime_display_context.media_objects_parent_identity;
inline RuntimeMutex &runtime_named_lock_mutex = runtime_display_context.path_mutex;
inline void *&runtime_named_lock_parent_identity = runtime_display_context.media_objects_parent_identity;
inline uint32_t &runtime_palette_transition_step = runtime_display_context.palette_transition_step;
inline uint32_t &runtime_rectangle_transition_step_size = runtime_display_context.rectangle_transition_step_size;
inline uint32_t &runtime_available_scene_transitions = runtime_display_context.available_scene_transitions;
inline uint32_t &suspended_runtime_state_count = runtime_display_context.nested_runtime_state_count;
inline uint32_t &runtime_state_4_count = runtime_display_context.nested_runtime_state_4_count;
inline uint32_t &no_inventory_runtime_state_count = runtime_display_context.nested_no_inventory_state_count;
inline RuntimeMutex runtime_pointer_scene_mutex;
inline int32_t &runtime_pointer_x = runtime_display_context.scene_x;
inline int32_t &runtime_pointer_y = runtime_display_context.scene_y;
inline void *&current_runtime_resource = runtime_display_context.current_runtime_resource;
inline bool async_file_enabled;
inline AsyncFileHost *async_file_hosts;
inline RuntimeMutex async_file_global_mutex;
bool strings_equal(const char *left, const char *right);
void copy_directory_from_path(char *destination, const char *source);
inline void enter_runtime_byte_queue_lock()
{
    lock_runtime_mutex(&runtime_display_context.byte_queue_mutex);
}

inline void leave_runtime_byte_queue_lock()
{
    unlock_runtime_mutex(&runtime_display_context.byte_queue_mutex);
}

inline void enter_runtime_input_queue_lock()
{
    lock_runtime_mutex(&runtime_display_context.queued_input_mutex);
}

inline void leave_runtime_input_queue_lock()
{
    unlock_runtime_mutex(&runtime_display_context.queued_input_mutex);
}

// RuntimeCommandBounds fields are output pointers in mode 0x10000.
inline int begin_runtime_target_from_bounds_fields(uint32_t pixels, uint32_t rectangle, uint32_t pitch)
{
    return static_cast<int>(begin_display_target(reinterpret_cast<void **>(static_cast<uintptr_t>(pixels)), reinterpret_cast<DisplayRectangle *>(static_cast<uintptr_t>(rectangle)),
        reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(pitch))));
}

inline uint32_t runtime_session_reset_storage[0x1d3]{};
inline const DisplayPixelFormatDescriptor default_display_pixel_format{ 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0, nullptr, nullptr };
inline const DisplayPixelFormatDescriptor indexed_source_pixel_format{ 0, 8, 0, 0, 0, 0, nullptr, nullptr };
} // namespace freegag
