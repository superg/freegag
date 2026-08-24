#pragma once

#include "pcm_format.h"
#include "portable_types.h"
#include "runtime_input.h"
#include "runtime_services.h"
#include "script_types.h"

namespace gag
{
struct DisplayTraversalState;

struct RuntimeResourceConstructionPlan
{
    uint32_t flags;
    uint32_t scene_identifier;
    uint32_t scene_flags;
    int32_t x;
    int32_t y;
    RuntimeResourceSceneRole scene_role;
};

struct RuntimeResourceConstructionPlanApi
{
    uint32_t (*find_available_scene)(uint32_t flags);
};

struct RuntimeMediaBackend;
struct RuntimeAnimationBackend;
struct RuntimeSoundStatus;
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
    RuntimeHeap *(*get_process_heap)();
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    uint32_t (*detect_type)(const char *path);
    void (*update_host)(const char *path, int32_t mode);
    void (*load)(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);
    RuntimeMediaBackend *(*create_bitmap)(uint32_t unused, uint32_t extension_bytes, void *data);
    RuntimeAnimationBackend *(*create_animation)(uint32_t unused, void *data, uint32_t extension_bytes, uint32_t storage);
    uint32_t (*create_sound)(const RuntimePcmFormat *format);
    uint32_t (*set_sound_playback_marker)(uint32_t handle, uint32_t marker);
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
    uint32_t (*configure_bitmap)(void *identity, const DisplaySceneDescriptor *descriptor, void *callback, uint32_t flags);
    uint32_t (*configure_animation)(void *identity, const DisplaySceneDescriptor *descriptor, const void *comparison_palette, uint32_t flags, int32_t (*callback)(RuntimeMediaBackend *backend));
    uint32_t (*begin_scene)(intptr_t identifier);
    void (*finalize_media)(void *identity);
    void (*configure_palette)(RuntimeResourceObject *resource);
    uint32_t (*end_scene)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void (*wait_for_count)(uint32_t count);
    uint32_t (*destroy_media)(void *identity);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*destroy_generic)(void *identity);
    bool (*release_memory)(const char *path);
    uint32_t (*release_stream)(AsyncFileRecord *record);
    void (*build_path)(char *destination, const char *source);
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    RuntimeResourceCacheEntry *(*register_resource)(void *parent_identity, void *data);
    uint32_t (*add_scene_callback)(intptr_t identifier, int (*callback)(DisplayTraversalState *state), const void *context, uint32_t context_size, uint32_t flags);
};



struct RuntimeResourceVisibilityCallbackContext
{
    uint32_t palette_state;
    uint32_t resource_flags;
    char resource_name[260];
};



struct RuntimeResourceDestroyApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    RuntimeGenericResourceNode *(*find_generic)(void *identity);
    void (*remove_generic)(void *identity);
    uint32_t (*destroy_media_backend)(void *backend);
    bool (*release_memory_data)(void *data);
    uint32_t (*release_stream)(AsyncFileRecord *record);
    void (*destroy_sound)(uint32_t handle);
    uint32_t (*destroy_generic_backend)(void *backend);
    uint32_t (*release_scene)(intptr_t scene_identifier, intptr_t owner);
    uint32_t (*remove_runtime_child)(void *parent_identity, void *child_identity);
    RuntimeHeap *(*get_process_heap)();
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
};

struct RuntimeResourceControlApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*release_record)(RuntimeLockRecord *record);
    uint32_t (*destroy_resource)(void *identity);
    uint32_t (*query_sound)(uint32_t handle, RuntimeSoundStatus *status);
};



using RuntimeResourceConstructor = void *(*)(char *path, uint32_t scene_identifier, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t scale_or_loop, uint32_t flags);

struct RuntimeResourceSelectionApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    uint32_t (*close_archive)(CdfArchive *archive);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    RuntimeResourceConstructor construct_resource;
};



struct RuntimeGameHostContext
{
    uint32_t bits_per_pixel;
    uint32_t unknown_0010;
    uint16_t width;
    uint16_t height;
    void *display_surface;
    intptr_t unknown_0028;
    void *framebuffer;
    intptr_t unknown_0030;
    PaletteEntry *palette_entries;
    uint32_t x_offset;
    uint32_t y_offset;
};

using RuntimeGameDllInitialize = void (*)(RuntimeGameHostContext *context, void **callbacks, const char *sfs_name);
using RuntimeGameDllExecute = void (*)(uint32_t command);
using RuntimeGameInputHandler = uint32_t (*)(const RuntimeInputEvent &event);

struct RuntimeGameLifecycleApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*update_resource_host)(const char *path, int32_t reset);
    int32_t (*activate_comment_scene)(const char *name);
    void (*deactivate_comment_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void (*leave_runtime_state)();
    void (*leave_critical_section)(RuntimeMutex *mutex);
};

struct RuntimeGameIntegrationApi
{
    RuntimeGameDllInitialize initialize;
    RuntimeGameInputHandler input_handler;
    RuntimeGameDllExecute execute;
    void (*shutdown)();
};


struct RuntimeGameDllDispatchApi
{
    uint32_t (*time_get_time)();
    void (*sleep)(uint32_t milliseconds);
};



struct DisplayRectangle;

struct RuntimePointerPositionApi
{
    RuntimeThreadId (*get_current_thread_id)();
    void (*enter_critical_section)(RuntimeMutex *mutex);
    RuntimeNamedNode *(*find_child)(void *parent_identity, void *child_identity);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    void (*sleep)(uint32_t milliseconds);
    uint32_t (*offset_scene)(intptr_t identifier, int32_t x, int32_t y);
};



} // namespace gag
