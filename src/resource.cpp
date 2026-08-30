#include "resource.h"
#include <cstring>
#include <fstream>
#include <mutex>
#include <vector>
#include "host_events.h"
#include "portable_path.h"
#include "runtime_internal.h"

namespace freegag
{
struct DeferredRuntimeResourceDestruction
{
    void *identity;
    bool decrement_wait_count;
};

std::mutex deferred_runtime_resource_destruction_mutex;
std::vector<DeferredRuntimeResourceDestruction> deferred_runtime_resource_destructions;

RuntimeResourceConstructionPlan prepare_runtime_resource_construction(uint32_t scene_identifier, int32_t x, int32_t y, uint32_t flags)
{
    RuntimeResourceConstructionPlan plan{ flags, scene_identifier, 0, x, y, RuntimeResourceSceneRole::XRGB_COMPOSITION };
    if((flags & RUNTIME_RESOURCE_INDEXED_SOURCE) == 0)
    {
        if((flags & RUNTIME_RESOURCE_PRIMARY) != 0)
        {
            plan.scene_identifier = 0;
            plan.scene_flags = DISPLAY_SCENE_PRIMARY;
            plan.flags |= RUNTIME_RESOURCE_INTERNAL_PRIMARY;
        }
        if((plan.flags & RUNTIME_RESOURCE_INDEPENDENT_SCENE) != 0 && plan.scene_identifier == 0)
        {
            plan.scene_identifier = find_available_display_scene_index(0x8000);
            plan.scene_flags |= DISPLAY_SCENE_INDEXED;
        }
        if((plan.flags & (RUNTIME_RESOURCE_HALF_SIZE | RUNTIME_RESOURCE_HIDDEN)) == 0)
            plan.scene_flags |= DISPLAY_SCENE_XRGB_COMPOSITION;
    }
    else
    {
        if(scene_identifier == 0)
            plan.scene_identifier = find_available_display_scene_index(1);
        plan.scene_flags = DISPLAY_SCENE_INDEXED;
    }
    if((plan.flags & RUNTIME_RESOURCE_HIDDEN) != 0)
    {
        plan.scene_identifier = find_available_display_scene_index(0x80000);
        plan.x = 10000;
        plan.y = 10000;
    }
    if((plan.flags & RUNTIME_RESOURCE_HALF_SIZE) != 0)
    {
        plan.flags |= RUNTIME_RESOURCE_NO_SKIP | RUNTIME_RESOURCE_ONE_STEP | RUNTIME_RESOURCE_LOOP;
        plan.scene_flags |= DISPLAY_SCENE_INDEXED;
        plan.scene_identifier = find_available_display_scene_index(0x100000);
        plan.x = 10000;
        plan.y = 10000;
    }
    if((plan.flags & RUNTIME_RESOURCE_PRIMARY) == 0 && ((plan.scene_flags & DISPLAY_SCENE_INDEXED) != 0 || (plan.flags & (RUNTIME_RESOURCE_HALF_SIZE | RUNTIME_RESOURCE_HIDDEN)) != 0))
        plan.scene_role = RuntimeResourceSceneRole::INDEXED_SOURCE;
    return plan;
}

void *construct_runtime_resource(char *path, uint32_t scene_identifier, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t scale_or_loop, uint32_t flags)
{
    const RuntimeResourceConstructionPlan plan = prepare_runtime_resource_construction(scene_identifier, x, y, flags);
    flags = plan.flags;
    scene_identifier = plan.scene_identifier;
    x = plan.x;
    y = plan.y;

    RuntimeResourceObject *resource = nullptr;
    void *result = nullptr;
    bool constructed = false;
    const uint32_t type = detect_runtime_resource_type(path);
    if(type == 1)
    {
        const uint32_t scene_flags = plan.scene_flags & ~1U;
        update_runtime_resource_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        load_runtime_resource(path, &data, &data_size, &storage, 0x20000000);
        if(data != nullptr)
        {
            RuntimeMediaBackend *backend = create_runtime_bitmap_backend(0, 0, data);
            if(backend != nullptr && backend->error_state == 0)
            {
                const auto *format = static_cast<const int32_t *>(backend->format_data);
                const uint32_t source_width = static_cast<uint32_t>(format[1]);
                const uint32_t source_height = static_cast<uint32_t>(format[2] < 0 ? -format[2] : format[2]);
                if((flags & RUNTIME_RESOURCE_HALF_SIZE) != 0)
                {
                    if(width == 0)
                        width = source_width >> 1;
                    if(height == 0)
                        height = source_height >> 1;
                }
                resource = static_cast<RuntimeResourceObject *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    backend->extension_data = resource;
                    resource->type_flags = (flags & RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK) | RUNTIME_RESOURCE_TYPE_BITMAP;
                    resource->backend = backend;
                    resource->data = data;
                    const uint32_t high_flags = flags & ~RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK;
                    resource->backend_flags = backend->media_flags | high_flags;
                    resource->x = x;
                    resource->y = y;
                    resource->previous_x = x;
                    resource->previous_y = y;
                    resource->requested_width = width;
                    resource->requested_height = height;
                    resource->output_width = source_width;
                    resource->output_height = source_height;
                    resource->scene_role = plan.scene_role;
                    const DisplayPixelFormatDescriptor *scene_format = plan.scene_role == RuntimeResourceSceneRole::XRGB_COMPOSITION ? &default_display_pixel_format : &indexed_source_pixel_format;
                    resource->scene_identifier = reinterpret_cast<intptr_t>(
                        acquire_display_scene_node(scene_identifier, x, y, source_width, source_height, scene_flags, reinterpret_cast<intptr_t>(resource), &resource->scene_descriptor, scene_format));
                    if(resource->scene_identifier != 0)
                    {
                        resource->callback_position = resource->scene_descriptor.pixels;
                        DisplayRectangle source_rectangle{ 0, 0, static_cast<int32_t>(source_width), static_cast<int32_t>(source_height) };
                        if((flags & (RUNTIME_RESOURCE_HALF_SIZE | RUNTIME_RESOURCE_HIDDEN)) == 0)
                            ++runtime_resource_count;
                        if((flags & RUNTIME_RESOURCE_PRIMARY) != 0)
                        {
                            configure_runtime_bitmap_backend(backend, &resource->scene_descriptor, nullptr, high_flags | RUNTIME_MEDIA_NO_PALETTE | RUNTIME_MEDIA_SKIP_PRESENTATION);
                            begin_display_scene_update(resource->scene_identifier);
                            finalize_runtime_media_backend(backend);
                            configure_runtime_resource_palette(resource);
                            end_display_scene_update(resource->scene_identifier, nullptr, nullptr);
                            current_runtime_resource = resource;
                        }
                        else
                        {
                            configure_runtime_bitmap_backend(backend, &resource->scene_descriptor, runtime_game_host_context.palette_entries,
                                high_flags | RUNTIME_MEDIA_NO_PALETTE | RUNTIME_MEDIA_SKIP_PRESENTATION);
                            if((flags & RUNTIME_RESOURCE_DEFERRED_LOAD) == 0)
                            {
                                begin_display_scene_update(resource->scene_identifier);
                                finalize_runtime_media_backend(backend);
                                configure_runtime_resource_palette(resource);
                                const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
                                end_display_scene_update(resource->scene_identifier, &transform, &source_rectangle);
                            }
                        }
                        constructed = true;
                        result = resource;
                    }
                }
            }
            if(!constructed)
            {
                if(backend != nullptr)
                    destroy_runtime_media_backend(backend);
                if(resource != nullptr)
                {
                    release_runtime_memory_resource(path);
                    free_runtime_heap(runtime_process_heap(), 0, resource);
                }
                resource = nullptr;
            }
        }
    }
    else if(type == 2)
    {
        update_runtime_resource_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        load_runtime_resource(path, &data, &data_size, &storage, 0x20000000);
        if(data != nullptr)
        {
            auto *wave_file = static_cast<RuntimePcmWaveFile *>(data);
            const uint32_t sound = create_runtime_sound_handle(&wave_file->format);
            if(sound != 0)
            {
                auto *wave = reinterpret_cast<RuntimeRiffChunk *>(wave_file + 1);
                constexpr char wave_data_chunk_id[4]{ 'd', 'a', 't', 'a' };
                while(!fixed_dword_memory_equal(wave->identifier, wave_data_chunk_id, sizeof(wave_data_chunk_id)))
                    wave = reinterpret_cast<RuntimeRiffChunk *>(reinterpret_cast<uint8_t *>(wave) + 1);
                resource = static_cast<RuntimeResourceObject *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    resource->type_flags = (flags & RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK) | RUNTIME_RESOURCE_TYPE_SOUND;
                    resource->backend = reinterpret_cast<void *>(static_cast<uintptr_t>(sound));
                    resource->data = data;
                    pause_runtime_sound(sound, 1);
                    queue_runtime_sound_data(sound, wave->data, wave->size, 1);
                    set_runtime_sound_loop_value(sound, (flags & RUNTIME_RESOURCE_LOOP) != 0 ? RUNTIME_SOUND_LOOP_INFINITE : (scale_or_loop == 0 ? 1 : scale_or_loop));
                    set_runtime_sound_playback_marker(sound, RUNTIME_SOUND_PLAYBACK_MARKER_PRESET);
                    if((flags & RUNTIME_RESOURCE_ONE_STEP) == 0)
                        resume_runtime_sound(sound, 1);
                    constructed = true;
                    result = resource;
                }
            }
            if(!constructed)
            {
                if(sound != 0)
                    destroy_runtime_sound_handle(sound);
                result = nullptr;
                release_runtime_memory_resource(path);
            }
        }
    }
    else if(type == 3)
    {
        update_runtime_resource_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        load_runtime_resource(path, &data, &data_size, &storage, 0);
        if(data != nullptr)
        {
            RuntimeAnimationBackend *backend = create_runtime_animation_backend(0, data, 0, storage);
            if(backend != nullptr && backend->base.error_state == 0)
            {
                if(scale_or_loop != 0)
                    flags |= RUNTIME_RESOURCE_LOOP;
                const bool half_size = (flags & RUNTIME_RESOURCE_HALF_SIZE) != 0;
                const auto *format = static_cast<const RuntimeAnimationFileHeader *>(backend->base.format_data);
                const uint32_t source_width = format->width;
                const uint32_t source_height = format->height;
                if(half_size)
                {
                    if(width == 0)
                        width = source_width >> 1;
                    if(height == 0)
                        height = source_height >> 1;
                }
                else
                {
                    if(width == 0)
                        width = 1;
                    if(height == 0)
                        height = 1;
                    backend->base.scale_x = width;
                    backend->base.scale_y = height;
                }
                const uint32_t output_width = half_size ? source_width : source_width * width;
                const uint32_t output_height = half_size ? source_height : source_height * height;
                resource = static_cast<RuntimeResourceObject *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    backend->base.extension_data = resource;
                    resource->type_flags = (flags & RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK) | RUNTIME_RESOURCE_TYPE_ANIMATION;
                    const uint32_t high_flags = flags & ~RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK;
                    resource->backend = backend;
                    resource->data = data;
                    resource->backend_flags = backend->base.media_flags | high_flags;
                    resource->x = x;
                    resource->y = y;
                    resource->previous_x = x;
                    resource->previous_y = y;
                    resource->frame_limit = scale_or_loop == 0 ? RUNTIME_RESOURCE_FRAME_LIMIT_UNBOUNDED : scale_or_loop - 1;
                    resource->frames_remaining = resource->frame_limit;
                    resource->requested_width = width;
                    resource->requested_height = height;
                    resource->output_width = output_width;
                    resource->output_height = output_height;
                    resource->scene_role = plan.scene_role;
                    const DisplayPixelFormatDescriptor *scene_format =
                        resource->scene_role == RuntimeResourceSceneRole::XRGB_COMPOSITION ? &default_display_pixel_format : &indexed_source_pixel_format;
                    resource->scene_identifier = reinterpret_cast<intptr_t>(acquire_display_scene_node(scene_identifier, x, y, output_width, output_height, plan.scene_flags,
                        reinterpret_cast<intptr_t>(resource), &resource->scene_descriptor, scene_format));
                    if(resource->scene_identifier != 0)
                    {
                        resource->callback_position = resource->scene_descriptor.pixels;
                        if((flags & RUNTIME_RESOURCE_PRIMARY) != 0)
                        {
                            // A std::jthread starts immediately, so release the construction-time pause before configuration creates the worker. Compute the primary wait target first because that
                            // worker may publish its first frame as soon as it starts.
                            const uint32_t count = runtime_resource_count + 1;
                            finalize_runtime_media_backend(backend);
                            configure_runtime_animation_backend(backend, &resource->scene_descriptor, nullptr, high_flags | RUNTIME_MEDIA_NO_PALETTE | RUNTIME_MEDIA_ONE_STEP,
                                update_runtime_resource_animation_backend);
                            wait_for_runtime_resource_count(count);
                            current_runtime_resource = resource;
                        }
                        else
                        {
                            // Explicitly stopped resources retain their initial pause until a script starts them; immediate resources must release it before their worker exists.
                            if((flags & RUNTIME_RESOURCE_DEFERRED_LOAD) == 0)
                                finalize_runtime_media_backend(backend);
                            configure_runtime_animation_backend(backend, &resource->scene_descriptor, nullptr, high_flags | RUNTIME_MEDIA_NO_PALETTE, update_runtime_resource_animation_backend);
                        }
                        constructed = true;
                        result = resource;
                    }
                }
            }
            if(!constructed)
            {
                if(backend != nullptr)
                    destroy_runtime_media_backend(backend);
                if(resource != nullptr)
                {
                    const uint32_t storage_flags = resource->backend_flags & RUNTIME_MEDIA_STORAGE_MASK;
                    if(storage_flags == RUNTIME_MEDIA_MEMORY_BACKED)
                        release_runtime_memory_resource(path);
                    else if(storage_flags == RUNTIME_MEDIA_STREAM_BACKED)
                        release_runtime_streamed_resource(static_cast<AsyncFileRecord *>(resource->data));
                    free_runtime_heap(runtime_process_heap(), 0, resource);
                }
                resource = nullptr;
            }
        }
    }
    else if(type == 4 && (flags & RUNTIME_RESOURCE_NATURAL_MOUSE) == 0)
    {
        update_runtime_resource_host(path, 0);
        result = find_or_load_runtime_generic_resource(path);
        if(result != nullptr && (flags & RUNTIME_RESOURCE_ONE_STEP) == 0)
            rebuild_runtime_tree_resources(activate_runtime_tree_with_notifications(path, "CFG", nullptr, nullptr));
    }
    else if(type == 0 || (type == 4 && (flags & RUNTIME_RESOURCE_NATURAL_MOUSE) != 0))
    {
        update_runtime_resource_host(path, 0);
        void *data = nullptr;
        uint32_t size = 0;
        int32_t storage = 0;
        load_runtime_resource(path, &data, &size, &storage, 0x20000000);
        if(data != nullptr)
        {
            RuntimeGenericBackend *backend = create_runtime_generic_backend(reinterpret_cast<uintptr_t>(data), size);
            result = backend;
            if(backend != nullptr)
            {
                resource = static_cast<RuntimeResourceObject *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    resource->type_flags = (flags & RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK) | RUNTIME_RESOURCE_TYPE_GENERIC;
                    resource->backend = backend;
                    resource->data = data;
                    constructed = true;
                    result = resource;
                }
            }
            if(!constructed)
            {
                if(backend != nullptr)
                    destroy_runtime_generic_backend(backend);
                result = nullptr;
                release_runtime_memory_resource(path);
            }
        }
    }
    else if(type == 5)
    {
        update_runtime_resource_host(path, 1);
        char full_path[260];
        build_runtime_resource_path(full_path, path);
        result = open_cdf_archive(full_path, runtime_resource_archive_alternate_stream);
        if(result != nullptr)
        {
            runtime_resource_archive = static_cast<CdfArchive *>(result);
            copy_string(runtime_graphics_resource_directory, path);
            runtime_display_context.flags |= RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN;
            if((flags & RUNTIME_RESOURCE_ONE_STEP) == 0)
            {
                result = activate_runtime_tree_with_notifications("Start.cfg", "CFG", nullptr, nullptr);
                rebuild_runtime_tree_resources(result);
            }
        }
    }

    if(resource != nullptr)
    {
        lock_runtime_mutex(&runtime_resource_mutex);
        get_or_create_runtime_child_by_data(runtime_media_objects_parent_identity, resource);
        if((resource->type_flags & RUNTIME_RESOURCE_HALF_SIZE) != 0)
        {
            RuntimeResourceVisibilityCallbackContext context{};
            context.resource_flags = resource->type_flags;
            copy_string(context.resource_name, path);
            add_display_scene_callback(resource->scene_identifier, reinterpret_cast<int (*)(DisplayTraversalState *)>(update_runtime_resource_visibility), &context, sizeof(context), 0);
        }
        runtime_display_context.flags &= ~RUNTIME_HOST_RESOURCE_LOAD_ACTIVE;
        unlock_runtime_mutex(&runtime_resource_mutex);
    }
    return result;
}

uint32_t update_runtime_resource_visibility(DisplayTraversalState *state)
{
    auto *context = static_cast<RuntimeResourceVisibilityCallbackContext *>(state->callback_context);
    constexpr uint32_t hidden_initializing_bitmap_flags = static_cast<uint32_t>(RUNTIME_MEDIA_INITIALIZING) | static_cast<uint32_t>(RUNTIME_RESOURCE_TYPE_BITMAP);
    uint32_t visible = (context->resource_flags & hidden_initializing_bitmap_flags) != hidden_initializing_bitmap_flags;
    if(visible != 0 && (context->resource_flags & RUNTIME_RESOURCE_TYPE_ANIMATION) != 0)
    {
        const auto *rectangle = static_cast<const DisplayRectangle *>(state->data);
        visible = rectangle->left < rectangle->right && rectangle->top < rectangle->bottom;
    }
    uint32_t result = 0;
    if((state->flags & DISPLAY_TRAVERSAL_QUERY) != 0)
    {
        visible |= context->palette_state != (runtime_scene_control_flags & RUNTIME_HOST_PALETTE_STATE);
        context->palette_state = runtime_scene_control_flags & RUNTIME_HOST_PALETTE_STATE;
        result = visible == 0;
    }
    if((state->flags & DISPLAY_TRAVERSAL_RENDER) != 0)
    {
        visible &= (runtime_scene_control_flags & RUNTIME_HOST_PALETTE_STATE) >> 15;
        if(visible != 0)
        {
            auto *source = static_cast<const uint8_t *>(reinterpret_cast<const void *>(state->first_position));
            auto *destination = static_cast<uint8_t *>(reinterpret_cast<void *>(state->current_position));
            uint8_t row_mask = 0xff;
            uint32_t rows = state->height;
            do
            {
                uint8_t mask = row_mask;
                uint32_t columns = state->width;
                do
                {
                    *destination++ = *source++ & mask;
                    mask = static_cast<uint8_t>(~mask);
                } while(--columns != 0);
                row_mask = static_cast<uint8_t>(~row_mask);
            } while(--rows != 0);
            context->resource_flags |= RUNTIME_MEDIA_INITIALIZING;
        }
        if(visible == 0 && (runtime_scene_control_flags & RUNTIME_HOST_PALETTE_STATE) == 0)
            return DISPLAY_TRAVERSAL_UNCHANGED;
        return DISPLAY_TRAVERSAL_BUFFER_UPDATED;
    }
    return result;
}

void request_runtime_resource_destruction(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(resource == nullptr)
        return;
    const uint32_t flags = resource->type_flags;
    const uint32_t type = flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x1000)
    {
        destroy_runtime_resource(identity);
        if((flags & (RUNTIME_RESOURCE_HALF_SIZE | RUNTIME_RESOURCE_HIDDEN)) == 0)
            --runtime_resource_count;
        return;
    }
    if(type != 0x2000)
    {
        destroy_runtime_resource(identity);
        return;
    }
    static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags |= RUNTIME_MEDIA_STOP_REQUESTED;
    release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(resource));
}

void queue_runtime_resource_destruction(void *identity, bool decrement_wait_count)
{
    std::lock_guard lock(deferred_runtime_resource_destruction_mutex);
    deferred_runtime_resource_destructions.push_back({ identity, decrement_wait_count });
}

void drain_runtime_resource_destructions()
{
    std::vector<DeferredRuntimeResourceDestruction> destructions;
    {
        std::lock_guard lock(deferred_runtime_resource_destruction_mutex);
        destructions.swap(deferred_runtime_resource_destructions);
    }
    for(const DeferredRuntimeResourceDestruction &destruction : destructions)
    {
        destroy_runtime_resource(destruction.identity);
        if(destruction.decrement_wait_count)
            --runtime_resource_count;
    }
}

uint32_t query_runtime_resource_frame_limit(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(resource == nullptr)
        return 0;
    const uint32_t result = resource->frame_limit;
    release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    return result;
}

uint16_t query_runtime_resource_frame_number(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    uint16_t result = 0;
    if(resource != nullptr)
    {
        if((resource->type_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == RUNTIME_RESOURCE_TYPE_ANIMATION && resource->backend != nullptr)
            result = static_cast<RuntimeMediaBackend *>(resource->backend)->frame_number;
        release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    }
    return result;
}

void select_runtime_resource(char *path)
{
    lock_runtime_mutex(&runtime_resource_mutex);
    if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) != 0)
    {
        close_cdf_archive(runtime_resource_archive);
        runtime_resource_archive = nullptr;
        runtime_scene_control_flags &= ~RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN;
        runtime_graphics_resource_directory[0] = '\0';
    }
    unlock_runtime_mutex(&runtime_resource_mutex);
    if(path != nullptr)
    {
        HostEventResult event_result = send_application_event(HostApplicationCommand::VALIDATE_RESOURCE_PATH, std::string(path));
        if(const auto *validated_path = std::get_if<std::string>(&event_result))
            copy_string(path, validated_path->c_str());
        construct_runtime_resource(path, 0, 0, 0, 0, 0, 0, 0x200);
    }
}

uint32_t query_runtime_resource_playback_flags(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(resource == nullptr)
        return 0;
    uint32_t result = 0;
    const uint32_t type = resource->type_flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x1000 || type == 0x2000)
    {
        result = static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags;
    }
    else if(type == 0x8000)
    {
        result = RUNTIME_MEDIA_MEMORY_BACKED;
        RuntimeSoundStatus status{};
        if(query_runtime_sound_status(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource->backend)), &status) == 0)
        {
            release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(resource));
            return 0;
        }
        if(status.control_state != 0)
            result |= RUNTIME_MEDIA_PAUSED;
        if(status.playback_marker != 0 || (status.schedule_marker == 0 && status.control_state == 0))
            result |= 0x2000;
        if(status.infinite_loop != 0)
            result |= 0x400;
    }
    release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    return result;
}

uint32_t destroy_runtime_resource(void *identity)
{
    auto *record = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    uint32_t result = 0;
    if(record == nullptr)
    {
        lock_runtime_mutex(&runtime_named_lock_mutex);
        RuntimeGenericResourceNode *generic = find_runtime_generic_resource(identity);
        if(generic != nullptr)
        {
            result = 1;
            remove_runtime_generic_resource(identity);
        }
        unlock_runtime_mutex(&runtime_named_lock_mutex);
        return result;
    }

    const uint32_t type = record->type_flags & RUNTIME_RESOURCE_TYPE_MASK;
    bool release_scene = true;
    if(type == 0x1000)
    {
        result = destroy_runtime_media_backend(record->backend);
        result = result != 0 && release_runtime_memory_resource_by_data(record->data);
    }
    else if(type == 0x2000)
    {
        result = destroy_runtime_media_backend(record->backend);
        const uint32_t storage = record->backend_flags & RUNTIME_MEDIA_STORAGE_MASK;
        if(storage == 0x01000000)
            result = result != 0 && release_runtime_memory_resource_by_data(record->data);
        else if(storage == 0x02000000)
            result &= release_runtime_streamed_resource(static_cast<AsyncFileRecord *>(record->data));
    }
    else if(type == 0x8000)
    {
        destroy_runtime_sound_handle(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->backend)));
        result = release_runtime_memory_resource_by_data(record->data);
    }
    else if(type == 0x10000)
    {
        result = destroy_runtime_generic_backend(record->backend);
        result = result != 0 && release_runtime_memory_resource_by_data(record->data);
    }
    else
    {
        release_scene = false;
    }
    if(release_scene)
        result &= release_display_scene_node(0, reinterpret_cast<intptr_t>(identity)) == 0;
    if(current_runtime_resource == identity)
        current_runtime_resource = nullptr;
    lock_runtime_mutex(&runtime_named_lock_mutex);
    remove_runtime_named_child_by_identity(runtime_named_lock_parent_identity, identity);
    result = result != 0 && free_runtime_heap(runtime_process_heap(), 0, record);
    unlock_runtime_mutex(&runtime_named_lock_mutex);
    return result;
}

void destroy_runtime_tree_resources(void *identity)
{
    RuntimeTreeNode *root = find_runtime_tree_node_by_identity(identity);
    uint32_t count = runtime_resource_count;
    if(root == nullptr)
        return;
    on_scripted_save_load_tree_resources_destroyed(root);

    if(root->identity == runtime_pointer_root_identity)
    {
        set_runtime_resource_state(current_runtime_resource, 1);
        stop_runtime_game_dll();
        reset_runtime_display_state();
    }

    auto *primary_tail = static_cast<RuntimeTreePrimaryResourceLink *>(find_last_runtime_primary_resource_link_by_identity(identity));
    if(primary_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreePrimaryResourceLink *>(root->primary_resource_link_head);
        for(;;)
        {
            if(link->resource_identity != nullptr)
            {
                uint32_t flags = query_runtime_scene_flags(link->resource_identity);
                if(flags != 0)
                {
                    if((flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == 0)
                    {
                        request_runtime_resource_destruction(link->resource_identity);
                    }
                    else
                    {
                        --count;
                        if((link->flags & RUNTIME_RESOURCE_NO_CLOSE) == 0)
                            finalize_runtime_resource_destruction(link->resource_identity);
                        else
                            request_runtime_resource_destruction(link->resource_identity);
                    }
                }
                link->resource_identity = nullptr;
            }
            if(link == primary_tail)
                break;
            link = link->next;
        }
    }

    auto *secondary_tail = static_cast<RuntimeTreeSecondaryResourceLink *>(find_last_runtime_secondary_resource_link_by_identity(identity));
    if(secondary_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreeSecondaryResourceLink *>(root->secondary_resource_link_head);
        for(;;)
        {
            if(link->resource_identity != nullptr)
            {
                request_runtime_resource_destruction(link->resource_identity);
                link->resource_identity = nullptr;
            }
            if(link == secondary_tail)
                break;
            link = link->next;
        }
    }

    auto *scene_tail = static_cast<RuntimeTreeSceneLink *>(find_last_runtime_scene_link_by_identity(identity));
    if(scene_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreeSceneLink *>(root->scene_link_head);
        for(;;)
        {
            if(link->scene_identifier != 0)
            {
                release_display_scene_node(link->scene_identifier, 0);
                link->scene_identifier = 0;
            }
            if(link == scene_tail)
                break;
            link = link->next;
        }
    }

    set_runtime_tree_comment_mode(root, 0);
    wait_for_runtime_resource_count(count);
}

void finalize_runtime_resource_destruction(void *identity)
{
    intptr_t scene_identifier = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    RuntimeLockRecord *record = acquire_runtime_lock_record(identity);
    if(record != nullptr)
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        uint32_t type_flags = resource->type_flags;
        uint32_t type = type_flags & RUNTIME_RESOURCE_TYPE_MASK;
        if(type == 0x1000)
        {
            scene_identifier = resource->scene_identifier;
            x = resource->scene_descriptor.x;
            y = resource->scene_descriptor.y;
            width = static_cast<int32_t>(resource->output_width);
            height = static_cast<int32_t>(resource->output_height);
            destroy_runtime_resource(identity);
            if((type_flags & (RUNTIME_RESOURCE_HALF_SIZE | RUNTIME_RESOURCE_HIDDEN)) == 0)
                --runtime_resource_count;
        }
        else if(type == 0x2000)
        {
            scene_identifier = resource->scene_identifier;
            x = resource->scene_descriptor.x;
            y = resource->scene_descriptor.y;
            width = static_cast<int32_t>(resource->output_width);
            height = static_cast<int32_t>(resource->output_height);
            uint32_t target_count = runtime_resource_count - 1;
            static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags |= RUNTIME_MEDIA_STOP_REQUESTED;
            release_runtime_lock_record(record);
            if((type_flags & RUNTIME_RESOURCE_HALF_SIZE) == 0)
            {
                while(target_count < runtime_resource_count)
                {
                    drain_runtime_resource_destructions();
                    runtime_sleep(1);
                }
            }
        }
        else
        {
            destroy_runtime_resource(identity);
        }
    }
    update_runtime_resource_scene_region(scene_identifier, x, y, width, height);
}

void update_runtime_resource_scene_region(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height)
{
    DisplayRectangle rectangle{ x, y, x + width, y + height };
    if(scene_identifier == 0)
        scene_identifier = runtime_display_scene_identifier;
    DisplaySceneNode *scene = lock_display_scene_node(scene_identifier);
    if(scene == nullptr)
        return;
    RuntimeLockRecord *record = acquire_runtime_lock_record(reinterpret_cast<void *>(static_cast<uintptr_t>(scene->primary_owner)));
    if(record != nullptr)
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        if((resource->type_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == RUNTIME_RESOURCE_TYPE_BITMAP && (scene->flags & DISPLAY_SCENE_OPAQUE) != 0)
        {
            rectangle.left -= scene->x + resource->x;
            rectangle.top -= scene->y + resource->y;
            rectangle.right -= scene->x + resource->x;
            rectangle.bottom -= scene->y + resource->y;
            if(begin_display_scene_update(scene_identifier) == 0)
            {
                render_runtime_bitmap_backend_region(resource->backend, &rectangle);
                const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
                end_display_scene_update(scene_identifier, &transform, &rectangle);
            }
        }
        else
        {
            rectangle.left -= scene->x;
            rectangle.top -= scene->y;
            rectangle.right -= scene->x;
            rectangle.bottom -= scene->y;
            update_display_root_region(reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(scene_identifier)), &rectangle, 0);
        }
        release_runtime_lock_record(record);
    }
    unlock_display_scene_node(scene_identifier);
}

void copy_runtime_bitmap_region(RuntimeMediaBackend *backend, DisplayRectangle *rectangle)
{
    auto *format = static_cast<BitmapInfoHeader *>(backend->format_data);
    int32_t bitmap_width = format->biWidth;
    int32_t source_stride = (bitmap_width + 3) & ~3;
    if(backend->destination_bits_per_pixel == 32)
    {
        const uint8_t *source_pixels = static_cast<const uint8_t *>(backend->source_data) + backend->bitmap_file.bfOffBits;
        auto *destination_pixels = reinterpret_cast<uint32_t *>(backend->destination_pixels);
        const int32_t bitmap_height = format->biHeight < 0 ? -format->biHeight : format->biHeight;
        for(int32_t y = rectangle->top; y < rectangle->bottom; ++y)
        {
            const int32_t source_y = format->biHeight < 0 ? y : bitmap_height - y - 1;
            const uint8_t *source_row = source_pixels + static_cast<size_t>(source_y) * source_stride;
            uint32_t *destination_row = destination_pixels + static_cast<size_t>(backend->destination_y + y) * backend->destination_stride + backend->destination_x;
            for(int32_t x = rectangle->left; x < rectangle->right; ++x)
            {
                const uint8_t index = source_row[x];
                const PaletteEntry color = backend->palette_entries[index];
                destination_row[x] = (index == 0 ? 0u : 0xff000000u) | static_cast<uint32_t>(color.peRed) << 16 | static_cast<uint32_t>(color.peGreen) << 8 | color.peBlue;
                if(backend->indexed_pixels != nullptr)
                    backend->indexed_pixels[static_cast<size_t>(backend->indexed_origin_y + y) * backend->indexed_stride + backend->indexed_origin_x + x] = index;
            }
        }
        return;
    }
    int32_t copy_width = rectangle->right - rectangle->left;
    int32_t copy_height = rectangle->bottom - rectangle->top;
    int32_t source_skip = bitmap_width - source_stride + bitmap_width - copy_width;
    int32_t destination_stride = backend->destination_stride;
    int32_t destination_skip = destination_stride - copy_width;
    uint8_t *source = static_cast<uint8_t *>(backend->source_data) + backend->bitmap_file.bfOffBits + rectangle->top * source_stride + rectangle->left;
    uint8_t *destination = backend->destination_pixels + (static_cast<uint32_t>(backend->destination_y) + rectangle->top) * backend->destination_stride + backend->destination_x + rectangle->left;
    if(format->biHeight >= 0)
    {
        source += source_stride * (format->biHeight - rectangle->top - rectangle->top - copy_height);
        destination += destination_stride * (copy_height - 1);
        destination_skip = -(destination_stride + copy_width);
    }
    do
    {
        std::memcpy(destination, source, copy_width);
        source += copy_width + source_skip;
        destination += copy_width + destination_skip;
        --copy_height;
    } while(copy_height != 0);
}

uint32_t render_runtime_bitmap_backend_region(void *identity, DisplayRectangle *rectangle)
{
    uint32_t result = 0;
    lock_runtime_mutex_forever(runtime_media_backend_mutex, runtime_infinite_wait);
    try
    {
        for(RuntimeMediaBackend *backend = runtime_media_backend_head; backend != nullptr; backend = backend->next)
        {
            if(backend->identity == identity)
            {
                if(backend->type == 0xac)
                {
                    auto *format = static_cast<BitmapInfoHeader *>(backend->format_data);
                    int32_t height = format->biHeight < 0 ? -format->biHeight : format->biHeight;
                    if(rectangle->left < 0)
                        rectangle->left = 0;
                    if(rectangle->top < 0)
                        rectangle->top = 0;
                    if(format->biWidth < rectangle->right)
                        rectangle->right = format->biWidth;
                    if(height < rectangle->bottom)
                        rectangle->bottom = height;
                    if(rectangle->left < rectangle->right && rectangle->top < rectangle->bottom)
                    {
                        copy_runtime_bitmap_region(backend, rectangle);
                        result = 1;
                    }
                }
                break;
            }
        }
    }
    catch(...)
    {
        unlock_runtime_mutex(runtime_media_backend_mutex);
        throw;
    }
    unlock_runtime_mutex(runtime_media_backend_mutex);
    return result;
}

void select_runtime_scene_transition(uint32_t flags)
{
    uint32_t available;
    if((flags & RUNTIME_SCENE_TRANSITION_PRELOAD) != 0)
    {
        uint32_t depth_offset = runtime_game_host_context.bits_per_pixel - 8;
        available = (depth_offset < 1 ? 2U : 0U) + 0xffd;
    }
    else
    {
        available = runtime_available_scene_transitions;
        if(runtime_game_host_context.bits_per_pixel != 8)
            available &= ~RUNTIME_SCENE_TRANSITION_PALETTE;
    }
    uint32_t selected = available & flags & RUNTIME_SCENE_TRANSITION_OPTION_MASK;
    if(selected == 0 && available != 0 && (flags & RUNTIME_SCENE_TRANSITION_OPTION_MASK) != RUNTIME_SCENE_TRANSITION_IMMEDIATE && (flags & RUNTIME_SCENE_TRANSITION_PRELOAD) == 0)
    {
        selected = 1U << (std::rand() % 3);
        while((selected & available) == 0)
            selected = selected == 4 ? 1 : selected * 2;
    }
    switch(selected)
    {
    case 0:
    case 1:
        apply_immediate_runtime_scene_transition(0, flags);
        break;
    case 2:
        apply_palette_runtime_scene_transition(runtime_palette_transition_step, flags);
        break;
    case 4:
        apply_rectangle_runtime_scene_transition(static_cast<uint8_t>(runtime_rectangle_transition_step_size), flags);
        break;
    }
}

void apply_immediate_runtime_scene_transition(uint32_t, uint32_t flags)
{
    DisplayRectangle rectangle{ 0, 0, 0, 0 };
    uint32_t type = flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x1000)
    {
        if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            set_display_clip_rectangle(&rectangle);
            release_display_lock();
        }
        return;
    }
    if(type != 0x2000)
        return;
    rectangle.right = runtime_game_host_context.width;
    rectangle.bottom = runtime_game_host_context.height;
    RuntimeLockRecord *record = acquire_runtime_lock_record(current_runtime_resource);
    if(record == nullptr)
        return;
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
    if((resource->type_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == 0)
        return;
    if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        dispatch_display_scene_update(&rectangle, 0x200);
        runtime_sleep(0);
        synchronize_display_region(&rectangle, 1);
        if((flags & RUNTIME_RESOURCE_HOST_MEMORY) != 0)
        {
            auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
            apply_display_palette(backend->palette_entries, 0x10000);
            synchronize_display_region(&rectangle, 1);
        }
        set_display_clip_rectangle(&rectangle);
        release_display_lock();
    }
    release_runtime_lock_record(record);
}

void apply_palette_runtime_scene_transition(uint32_t palette_step, uint32_t flags)
{
    uint8_t channel_step = static_cast<uint8_t>(palette_step);
    uint8_t transition_active = 0;
    DisplayRectangle rectangle{};
    PaletteEntry temporary_palette[0xec];
    RuntimeLockRecord *record = acquire_runtime_lock_record(current_runtime_resource);
    if(record == nullptr || (reinterpret_cast<RuntimeResourceObject *>(record)->type_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == 0)
    {
        apply_immediate_runtime_scene_transition(0, flags);
    }
    else
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        std::memcpy(&runtime_transition_palette[0], &backend->palette_version, sizeof(PaletteEntry));
        std::memcpy(&runtime_transition_palette[1], backend->palette_entries, sizeof(backend->palette_entries));
        const uint32_t type = flags & RUNTIME_RESOURCE_TYPE_MASK;
        if(type == 0x1000)
        {
            ++transition_active;
        }
        else if(type == 0x2000)
        {
            rectangle.right = runtime_game_host_context.width;
            rectangle.bottom = runtime_game_host_context.height;
            if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                ++transition_active;
                if((flags & RUNTIME_RESOURCE_HOST_MEMORY) != 0)
                {
                    apply_display_palette(backend->palette_entries, 0x10000);
                    operate_display_surface(runtime_game_host_context.width >> 1, runtime_game_host_context.height >> 1, 4, 4, 1);
                }
                for(size_t index = 0; index != 0xec; ++index)
                {
                    const PaletteEntry source = runtime_transition_palette[index + 1];
                    temporary_palette[index].peRed = static_cast<uint8_t>(source.peRed + 1);
                    temporary_palette[index].peGreen = static_cast<uint8_t>(source.peGreen + 1);
                    temporary_palette[index].peBlue = static_cast<uint8_t>(source.peBlue + 1);
                    temporary_palette[index].peFlags = 0xff;
                    runtime_transition_palette[index + 1] = PaletteEntry{ 0, 0, 0, 1 };
                }
                apply_display_palette(runtime_transition_palette, 0);
                set_display_clip_rectangle(&rectangle);
                dispatch_display_scene_update(&rectangle, 0);
                release_display_lock();
            }
        }
        release_runtime_lock_record(record);
    }

    if(transition_active == 0)
        return;
    uint32_t completed = 0;
    uint32_t deadline = runtime_milliseconds();
    if((runtime_scene_control_flags & RUNTIME_HOST_FORCE_PALETTE_REFRESH) != 0)
        channel_step = 0xff;
    const uint32_t type = flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x2000)
    {
        while(completed < 0xec)
        {
            uint32_t now = runtime_milliseconds();
            if(now < deadline)
            {
                runtime_sleep(0);
            }
            else
            {
                deadline = runtime_milliseconds() + 2;
                uint8_t passes = channel_step;
                do
                {
                    completed = 0;
                    for(size_t reverse = 0xec; reverse != 0; --reverse)
                    {
                        PaletteEntry &temporary = temporary_palette[reverse - 1];
                        PaletteEntry &destination = runtime_transition_palette[reverse];
                        if(temporary.peFlags == 0)
                        {
                            ++completed;
                            continue;
                        }
                        auto *temporary_channels = reinterpret_cast<uint8_t *>(&temporary);
                        auto *destination_channels = reinterpret_cast<uint8_t *>(&destination);
                        for(size_t channel = 0; channel != 3; ++channel)
                            if(temporary_channels[channel] == 0)
                                ++destination_channels[channel];
                            else
                                ++temporary_channels[channel];
                        --temporary.peFlags;
                    }
                    --passes;
                } while(passes != 0);
                apply_display_palette(runtime_transition_palette, 0);
                if((runtime_scene_control_flags & RUNTIME_HOST_FORCE_PALETTE_REFRESH) != 0)
                    invalidate_game_framebuffer_rect(0, 0, runtime_game_host_context.width, runtime_game_host_context.height);
            }
        }
        return;
    }
    if(type != 0x1000)
        return;
    while(completed < 0x2c4)
    {
        uint32_t now = runtime_milliseconds();
        if(now < deadline)
        {
            runtime_sleep(0);
        }
        else
        {
            deadline = runtime_milliseconds() + 2;
            completed = 0;
            for(size_t reverse = 0xec; reverse != 0; --reverse)
            {
                auto *channels = reinterpret_cast<uint8_t *>(&runtime_transition_palette[reverse]);
                for(size_t channel = 0; channel != 3; ++channel)
                {
                    if(channels[channel] > channel_step)
                    {
                        channels[channel] = static_cast<uint8_t>(channels[channel] - channel_step);
                    }
                    else
                    {
                        channels[channel] = 0;
                        ++completed;
                    }
                }
            }
            apply_display_palette(runtime_transition_palette, 0);
            if((runtime_scene_control_flags & RUNTIME_HOST_FORCE_PALETTE_REFRESH) != 0)
                invalidate_game_framebuffer_rect(0, 0, runtime_game_host_context.width, runtime_game_host_context.height);
        }
    }
    if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        set_display_clip_rectangle(&rectangle);
        operate_display_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
        release_display_lock();
    }
}

void apply_rectangle_runtime_scene_transition(uint8_t step_size, uint32_t flags)
{
    RuntimeLockRecord *record = acquire_runtime_lock_record(current_runtime_resource);
    if(record == nullptr || (reinterpret_cast<RuntimeResourceObject *>(record)->type_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == 0)
    {
        apply_immediate_runtime_scene_transition(0, flags);
        return;
    }

    const uint32_t width = runtime_game_host_context.width;
    const uint32_t height = runtime_game_host_context.height;
    uint32_t horizontal_step;
    uint32_t vertical_step;
    if(step_size == 0xff)
    {
        horizontal_step = width;
        vertical_step = height;
    }
    else
    {
        horizontal_step = (step_size & 0xfcU) + 4;
        vertical_step = (horizontal_step * 15) / 20;
    }

    const uint32_t type = flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x1000)
    {
        DisplayRectangle clip{ 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) };
        if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            set_display_clip_rectangle(&clip);
            release_display_lock();
        }
        uint32_t deadline = runtime_milliseconds();
        while((clip.bottom - clip.top) > static_cast<int32_t>(vertical_step * 2) || (clip.right - clip.left) > static_cast<int32_t>(horizontal_step * 2))
        {
            uint32_t now = runtime_milliseconds();
            if(now < deadline)
            {
                runtime_sleep(0);
                continue;
            }
            deadline = runtime_milliseconds() + 2;

            const int32_t old_left = clip.left;
            const int32_t old_top = clip.top;
            const int32_t old_right = clip.right;
            const int32_t old_bottom = clip.bottom;
            const int32_t next_right = old_right - static_cast<int32_t>(horizontal_step);
            const int32_t next_bottom = old_bottom - static_cast<int32_t>(vertical_step);
            DisplayRectangle strips[4]{
                { old_left,   old_top,                                       next_right - old_left,                 static_cast<int32_t>(vertical_step)                            },
                { next_right, old_top,                                       static_cast<int32_t>(horizontal_step), old_bottom - old_top                                           },
                { old_left,   next_bottom,                                   next_right - old_left,                 static_cast<int32_t>(vertical_step)                            },
                { old_left,   old_top + static_cast<int32_t>(vertical_step), static_cast<int32_t>(horizontal_step), old_bottom - old_top - static_cast<int32_t>(vertical_step * 2) },
            };

            clip.left = std::min(old_left + static_cast<int32_t>(horizontal_step), static_cast<int32_t>((width >> 1) - horizontal_step));
            clip.top = std::min(old_top + static_cast<int32_t>(vertical_step), static_cast<int32_t>((height >> 1) - vertical_step));
            clip.right = std::max(next_right, static_cast<int32_t>((width >> 1) + horizontal_step));
            clip.bottom = std::max(next_bottom, static_cast<int32_t>((height >> 1) + vertical_step));
            if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                set_display_clip_rectangle(&clip);
                for(const DisplayRectangle &strip : strips)
                    operate_display_surface(strip.left, strip.top, strip.right, strip.bottom, 2);
                release_display_lock();
            }
        }
        if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            synchronize_display_region(&clip, 2);
            clip.left = clip.right;
            clip.top = clip.bottom;
            set_display_clip_rectangle(&clip);
            release_display_lock();
        }
    }
    else if(type == 0x2000)
    {
        DisplayRectangle clip;
        if(step_size == 0xff)
        {
            clip = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) };
        }
        else
        {
            clip = { static_cast<int32_t>((width >> 1) - horizontal_step), static_cast<int32_t>((height >> 1) - vertical_step), static_cast<int32_t>((width >> 1) + horizontal_step),
                static_cast<int32_t>((height >> 1) + vertical_step) };
        }
        if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            if((flags & RUNTIME_RESOURCE_HOST_MEMORY) != 0)
            {
                auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
                auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
                apply_display_palette(backend->palette_entries, 0x10000);
            }
            dispatch_display_scene_update(&clip, 0);
            set_display_clip_rectangle(&clip);
            release_display_lock();
        }
        uint32_t deadline = runtime_milliseconds();
        while((clip.right - clip.left) < static_cast<int32_t>(width) || (clip.bottom - clip.top) < static_cast<int32_t>(height))
        {
            uint32_t now = runtime_milliseconds();
            if(now < deadline)
            {
                runtime_sleep(0);
                continue;
            }
            deadline = runtime_milliseconds() + 2;
            const int32_t old_left = clip.left;
            const int32_t old_top = clip.top;
            const int32_t old_right = clip.right;
            const int32_t old_bottom = clip.bottom;
            clip.left = std::max(old_left - static_cast<int32_t>(horizontal_step), 0);
            clip.top = std::max(old_top - static_cast<int32_t>(vertical_step), 0);
            clip.right = std::min(old_right + static_cast<int32_t>(horizontal_step), static_cast<int32_t>(width));
            clip.bottom = std::min(old_bottom + static_cast<int32_t>(vertical_step), static_cast<int32_t>(height));
            DisplayRectangle strips[4]{
                { clip.left, clip.top,   old_right,  old_top     },
                { old_right, clip.top,   clip.right, clip.bottom },
                { clip.left, old_bottom, old_right,  clip.bottom },
                { clip.left, old_top,    old_left,   old_bottom  },
            };
            if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                set_display_clip_rectangle(&clip);
                for(DisplayRectangle &strip : strips)
                    dispatch_display_scene_update(&strip, 0);
                release_display_lock();
            }
        }
    }
    release_runtime_lock_record(record);
}

void set_runtime_resource_state(void *identity, uint32_t state)
{
    RuntimeLockRecord *record = acquire_runtime_lock_record(identity);
    if(record == nullptr)
    {
        if(current_runtime_resource == identity && state == 1)
            select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TYPE_BITMAP);
        return;
    }

    auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
    uint32_t type = resource->type_flags & RUNTIME_RESOURCE_TYPE_MASK;
    if(type == 0x1000)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        bool force_refresh = (state & 0x20000) != 0;
        if(force_refresh || (backend->media_flags & RUNTIME_MEDIA_RESOURCE_PENDING) != 0)
        {
            backend->media_flags |= RUNTIME_MEDIA_RESOURCE_PENDING;
            DisplayRectangle rectangle{ 0, 0, static_cast<int32_t>(resource->output_width), static_cast<int32_t>(resource->output_height) };
            begin_display_scene_update(resource->scene_identifier);
            finalize_runtime_media_backend(backend);
            configure_runtime_resource_palette(resource);
            const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
            end_display_scene_update(resource->scene_identifier, &transform, &rectangle);
            if(resource->generic_backend_child != nullptr)
                clear_runtime_generic_backend_child_ready(resource->generic_backend_child);
        }
        if(resource->generic_backend_child != nullptr)
            disable_runtime_generic_backend_child_mode_200(resource->generic_backend_child);
        if(current_runtime_resource == identity)
        {
            if(state == 1)
                select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_ACTIVATE);
            else if(!force_refresh)
                select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_SKIP_FADE | RUNTIME_RESOURCE_TRANSITION_DEACTIVATE);
        }
    }
    else if(type == 0x2000)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        uint32_t transition_flag = backend->frame_number == 1 ? 0x20000000 : 0;
        backend->media_flags = (backend->media_flags & ~(RUNTIME_MEDIA_PAUSED | RUNTIME_MEDIA_ONE_STEP)) | state;
        if(current_runtime_resource == identity)
        {
            if(state == 0)
                select_runtime_scene_transition(transition_flag | runtime_resource_transition_flags | RUNTIME_RESOURCE_TYPE_ANIMATION);
            if(state == 1)
                select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_ACTIVATE);
        }
    }
    else if(type == 0x8000)
    {
        bool force_refresh = (state & 0x20000) != 0;
        uint32_t handle = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource->backend));
        if(force_refresh)
        {
            restart_runtime_sound_data(handle);
            if(resource->generic_backend_child != nullptr)
                clear_runtime_generic_backend_child_ready(resource->generic_backend_child);
        }
        if((state & 1) != 0)
        {
            pause_runtime_sound(handle, 1);
            if(resource->generic_backend_child != nullptr)
                enable_runtime_generic_backend_child_mode_200(resource->generic_backend_child);
        }
        if(state == 0)
        {
            resume_runtime_sound(handle, 1);
            if(resource->generic_backend_child != nullptr)
                disable_runtime_generic_backend_child_mode_200(resource->generic_backend_child);
        }
        if(current_runtime_resource == identity)
        {
            if(state == 1)
                select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TYPE_BITMAP);
            else if(!force_refresh)
                select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_SKIP_FADE | RUNTIME_RESOURCE_TRANSITION_DEACTIVATE);
        }
    }
    else if(current_runtime_resource == identity)
    {
        if(state == 1)
            select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_ACTIVATE);
        else if((state & 0x20000) == 0)
            select_runtime_scene_transition(runtime_resource_transition_flags | RUNTIME_RESOURCE_TRANSITION_SKIP_FADE | RUNTIME_RESOURCE_TRANSITION_DEACTIVATE);
    }
    release_runtime_lock_record(record);
}

void release_runtime_lock_record(RuntimeLockRecord *record)
{
    if(record != nullptr && record->recursion_count != 0)
        --record->recursion_count;
}

RuntimeLockRecord *acquire_runtime_lock_record(void *child_identity)
{
    RuntimeThreadId thread_id = runtime_thread_id();
    while(true)
    {
        RuntimeLockRecord *record = nullptr;
        bool contended = false;
        lock_runtime_mutex(&runtime_named_lock_mutex);
        RuntimeNamedNode *node = find_runtime_named_child(runtime_named_lock_parent_identity, child_identity);
        if(node != nullptr)
        {
            record = static_cast<RuntimeLockRecord *>(node->identity);
            if(record->recursion_count == 0)
            {
                record->recursion_count = 1;
                record->owner_thread = thread_id;
            }
            else if(record->owner_thread == thread_id)
            {
                ++record->recursion_count;
            }
            else
            {
                contended = true;
            }
        }
        unlock_runtime_mutex(&runtime_named_lock_mutex);
        if(!contended)
            return record;
        runtime_sleep(5);
    }
}

void reset_runtime_session()
{
    stop_runtime_game_dll();
    RuntimeTreeNode *tree = get_runtime_tree_root();
    while(tree != nullptr)
    {
        destroy_runtime_tree_resources(tree);
        deactivate_runtime_tree_and_visuals(tree, nullptr);
        tree = get_runtime_tree_root();
    }

    reset_runtime_display_state();
    for(RuntimeVisualObject *visual = script_runtime_root->visual_objects; visual != nullptr; visual = visual->next)
        if(visual->scene_identity != nullptr)
            request_runtime_resource_destruction(visual->scene_identity);
    for(RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes; node != nullptr; node = node->next)
        if(node->resource_identity != nullptr)
            request_runtime_resource_destruction(node->resource_identity);
    destroy_runtime_fixed_name_list_nodes();
    purge_disabled_runtime_named_nodes();
    destroy_script_object_states();
    destroy_runtime_visual_objects();
    clear_runtime_command_definitions();
    remove_all_runtime_generic_resources();

    if((runtime_display_context.flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) != 0)
        close_cdf_archive(runtime_display_context.active_archive);
    runtime_display_context.active_archive = nullptr;
    runtime_display_context.flags &= ~RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN;
    destroy_async_file_host(runtime_display_context.async_file_host);
    runtime_display_context.async_file_host = nullptr;
    operate_display_surface(0, 0, runtime_display_context.width, runtime_display_context.height, 2);

    RuntimeNamedNode *media_objects = get_or_create_runtime_named_node("MMediaObjectsList");
    RuntimeNamedNode *open_memory_files = get_or_create_runtime_named_node("OpenMemoryFilesList");
    uint32_t start = runtime_milliseconds();
    while(media_objects->status != 0)
    {
        uint32_t current = runtime_milliseconds();
        if(current < start + 5000)
            break;
        runtime_sleep(10);
    }
    start = runtime_milliseconds();
    while(open_memory_files->status != 0)
    {
        uint32_t current = runtime_milliseconds();
        if(current < start + 5000)
            break;
        runtime_sleep(10);
    }
    if(media_objects->status == 0 && open_memory_files->status == 0)
        runtime_display_context.flags |= RUNTIME_HOST_RESOURCE_LOAD_ACTIVE;

    graphics_script_runtime_root.flags = 0;
    graphics_script_runtime_root.palette_flags = 0;
    std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
    runtime_palette_transition_step = RUNTIME_DEFAULT_PALETTE_TRANSITION_STEP;
    runtime_rectangle_transition_step_size = RUNTIME_DEFAULT_RECTANGLE_TRANSITION_STEP_SIZE;
    runtime_available_scene_transitions = RUNTIME_DEFAULT_AVAILABLE_SCENE_TRANSITIONS;
    runtime_resource_stream_rate_bytes_per_millisecond = RUNTIME_DEFAULT_RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND;
}

void switch_runtime_scene(void *identity)
{
    std::lock_guard lock(runtime_pointer_scene_mutex);
    if((graphics_host_flags & RUNTIME_HOST_SCENE_SWITCH_DEFERRED) != 0)
    {
        deferred_runtime_scene_identity = identity;
        return;
    }
    void *selected_identity = nullptr;
    auto *previous = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(current_runtime_scene_identity));
    if(previous != nullptr)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(previous->backend);
        backend->media_flags |= RUNTIME_MEDIA_PAUSED;
        offset_display_scene_node(previous->scene_identifier, 10000 - previous->x, 10000 - previous->y);
        previous->x = 10000;
        previous->y = 10000;
    }
    auto *selected = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(selected != nullptr)
    {
        uint32_t mode = selected->type_flags & RUNTIME_RESOURCE_TYPE_MASK;
        if(mode == 0x1000 || mode == 0x2000)
        {
            if(mode == 0x2000)
            {
                auto *backend = static_cast<RuntimeMediaBackend *>(selected->backend);
                backend->media_flags &= ~(RUNTIME_MEDIA_PAUSED | RUNTIME_MEDIA_ONE_STEP);
            }
            int32_t new_x = runtime_scene_x - static_cast<int32_t>(selected->requested_width);
            int32_t new_y = runtime_scene_y - static_cast<int32_t>(selected->requested_height);
            offset_display_scene_node(selected->scene_identifier, new_x - selected->x, new_y - selected->y);
            selected->x = new_x;
            selected->y = new_y;
            selected_identity = identity;
        }
    }
    current_runtime_scene_identity = selected_identity;
    if(selected != nullptr)
        release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(selected));
    if(previous != nullptr)
        release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(previous));
}

void reset_runtime_display_state()
{
    switch_runtime_scene(nullptr);
    graphics_host_flags &= ~RUNTIME_HOST_DISPLAY_RESET_MASK;
    set_script_runtime_flags(SCRIPT_RUNTIME_INVENTORY_OPEN, 0);
    set_script_runtime_flags(SCRIPT_RUNTIME_INVENTORY_CLOSE, 0);
    reset_script_runtime_transient_indices();
    reset_runtime_byte_queue();
    reset_runtime_input_queue();
    release_display_scene_node(0, reinterpret_cast<intptr_t>(&runtime_display_context));
    runtime_display_context.input_scene_identifier = 0;
    runtime_display_reset_byte = 0;
    saved_default_comment_scene_identity = nullptr;
    current_runtime_scene_identity = nullptr;
    current_runtime_resource = nullptr;
    runtime_pointer_root_identity = nullptr;
    runtime_display_context.active_script_link = nullptr;
    active_runtime_pointer_region = nullptr;
    runtime_pointer_state_mask = 0;
    runtime_pointer_state_owner = nullptr;
    runtime_pointer_event_state_object = nullptr;
    std::memset(runtime_pointer_event_record, 0, sizeof(runtime_pointer_event_record));
}

uint32_t shutdown_runtime_display()
{
    uint32_t result = 0;
    if((graphics_host_flags & RUNTIME_HOST_DISPLAY_READY) == RUNTIME_HOST_DISPLAY_READY)
    {
        begin_sdl_presenter_shutdown();
        auto *media_objects = static_cast<RuntimeNamedNode *>(get_or_create_runtime_named_node("MMediaObjectsList"));
        auto *open_memory_files = static_cast<RuntimeNamedNode *>(get_or_create_runtime_named_node("OpenMemoryFilesList"));
        if(open_memory_files->status == 0 && media_objects->status == 0)
        {
            graphics_host_flags |= RUNTIME_HOST_SHUTDOWN_REQUESTED;
            runtime_display_thread->join();
            delete runtime_display_thread;
            runtime_display_thread = nullptr;
            result = 1;
        }
        uint32_t cleaned = 0;
        if(result != 0)
        {
            cleaned = release_display_scene_node(runtime_display_scene_identifier, 0) == 0;
            cleaned &= shutdown_display_scene_host() == 0;
            teardown_display_palette_surface();
        }
        result = 0;
        if(cleaned != 0)
        {
            runtime_display_context.display_pixel_format = {};
            runtime_display_scene_identifier = 0;
            runtime_display_host = nullptr;
            graphics_host_flags &= ~RUNTIME_HOST_DISPLAY_READY;
            return cleaned;
        }
    }
    else if((graphics_host_flags & RUNTIME_HOST_MESSAGE_QUEUE_ENABLED) == 0)
    {
        result = 1;
    }
    return result;
}

void set_runtime_resource_loop_count(void *identity, uint32_t count)
{
    auto *record = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(record == nullptr)
        return;
    if((record->type_flags & RUNTIME_RESOURCE_TYPE_MASK) == RUNTIME_RESOURCE_TYPE_SOUND)
    {
        set_runtime_sound_loop_value(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->backend)), count);
    }
    else
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(record->backend);
        backend->media_flags |= RUNTIME_MEDIA_LOOP;
        record->frame_limit = count - 1;
        record->frames_remaining = count - 1;
    }
    release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(record));
}


uint32_t query_runtime_scene_flags(void *identity)
{
    RuntimeLockRecord *record = acquire_runtime_lock_record(identity);
    if(record == nullptr)
        return 0;
    uint32_t flags = reinterpret_cast<RuntimeResourceObject *>(record)->type_flags;
    release_runtime_lock_record(record);
    return flags;
}

void wait_for_runtime_resource_count(uint32_t count)
{
    while(runtime_resource_count != count)
    {
        drain_runtime_resource_destructions();
        runtime_sleep(0);
    }
}

void update_runtime_scene_position(void *identity, int32_t x, int32_t y)
{
    auto *record = reinterpret_cast<RuntimeResourceObject *>(acquire_runtime_lock_record(identity));
    if(record != nullptr)
    {
        record->previous_x = record->x;
        record->previous_y = record->y;
        record->x = x;
        record->y = y;
        offset_display_scene_node(record->scene_identifier, x - record->previous_x, y - record->previous_y);
        release_runtime_lock_record(reinterpret_cast<RuntimeLockRecord *>(record));
    }
}

void build_runtime_resource_path(char *destination, const char *source)
{
    char directory[0x80];
    char file_name[0x80];
    copy_directory_from_path(directory, source);
    copy_file_name_from_path(file_name, source);
    copy_string(destination, directory[0] != '\0' ? directory : runtime_display_context.resource_directory);
    append_string(destination, file_name);
}

void update_runtime_resource_host(const char *path, int32_t reset)
{
    char drive_prefix[32];
    char directory[260];
    directory[0] = '\0';
    lock_runtime_mutex(&runtime_resource_mutex);
    if(runtime_resource_host != nullptr)
    {
        if(reset != 0)
        {
            bool destroy_host = true;
            if(path != nullptr)
            {
                copy_directory_from_path(directory, path);
                if(directory[0] == '\0' || strings_equal(directory, runtime_display_context.resource_directory))
                    destroy_host = false;
            }
            if(destroy_host)
            {
                destroy_async_file_host(runtime_resource_host);
                runtime_resource_host = nullptr;
            }
            if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) != 0)
            {
                close_cdf_archive(runtime_resource_archive);
                runtime_scene_control_flags &= ~RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN;
                runtime_resource_archive = nullptr;
                runtime_resource_archive_state = 0;
            }
        }
    }
    if(runtime_resource_host == nullptr && path != nullptr)
    {
        copy_directory_from_path(directory, path);
        if(directory[0] != '\0')
            copy_string(runtime_display_context.resource_directory, directory);
        const char *root = extract_runtime_drive_prefix(drive_prefix, runtime_display_context.resource_directory) == 1 ? drive_prefix : nullptr;
        runtime_resource_host = create_async_file_host(root, 0x100000, runtime_resource_stream_rate_bytes_per_millisecond);
    }
    else
    {
        set_async_file_host_mode(runtime_resource_host, runtime_resource_stream_rate_bytes_per_millisecond);
    }
    unlock_runtime_mutex(&runtime_resource_mutex);
}

uint32_t detect_runtime_resource_type(const char *path)
{
    static constexpr uint8_t configuration_signature[5]{ '[', 'C', 'F', 'G', ']' };
    static constexpr uint8_t wave_signature[8]{ 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
    static constexpr uint8_t cdf_signature[6]{ 'C', 'D', 'F', '9', '6', 'a' };
    uint32_t type = RUNTIME_MEDIA_DATA_UNKNOWN;
    int32_t retry;
    do
    {
        retry = 0;
        lock_runtime_mutex(&runtime_resource_mutex);
        RuntimeResourceCacheEntry *entry = find_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, path);
        if(entry != nullptr)
        {
            type = entry->flags_and_references >> 16;
        }
        else if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) == 0)
        {
            update_runtime_resource_host(path, 0);
            char full_path[128];
            build_runtime_resource_path(full_path, path);
            std::filesystem::path resolved_path;
            std::ifstream file;
            if(resolve_existing_host_path_case_insensitive(full_path, &resolved_path))
                file.open(resolved_path, std::ios::binary);
            if(!file)
            {
                retry = 0;
            }
            else
            {
                uint8_t header[16];
                file.read(reinterpret_cast<char *>(header), sizeof(header));
                if(file.gcount() == sizeof(header))
                {
                    int16_t animation_marker;
                    std::memcpy(&animation_marker, header + 4, sizeof(animation_marker));
                    if(animation_marker == static_cast<int16_t>(0xaf12))
                        type = RUNTIME_MEDIA_DATA_ANIMATION;
                    uint16_t bitmap_marker;
                    std::memcpy(&bitmap_marker, header, sizeof(bitmap_marker));
                    if(bitmap_marker == 0x4d42)
                        type = RUNTIME_MEDIA_DATA_BITMAP;
                    if(fixed_dword_memory_equal(header, configuration_signature, sizeof(configuration_signature)))
                        type = RUNTIME_MEDIA_DATA_CONFIGURATION;
                    if(fixed_dword_memory_equal(header + 8, wave_signature, sizeof(wave_signature)))
                        type = RUNTIME_MEDIA_DATA_WAVE;
                    if(fixed_dword_memory_equal(header, cdf_signature, sizeof(cdf_signature)))
                        type = RUNTIME_MEDIA_DATA_ARCHIVE;
                }
            }
        }
        else if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) == RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN)
        {
            type = get_cdf_entry_flags(runtime_resource_archive, path) & ~CDF_ENTRY_COMPRESSED;
            if(type == 0)
                type = get_synthesized_resource_type(path);
        }
        unlock_runtime_mutex(&runtime_resource_mutex);
    } while(retry != 0);
    return type;
}

void *open_runtime_cdf_entry_stream(CdfArchive *archive, const char *name)
{
    return open_cdf_entry_async_record(archive, runtime_resource_host, name);
}

void load_runtime_resource(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags)
{
    static constexpr char loading_scene[] = "m_DEF_LOAD";
    static constexpr uint8_t configuration_signature[5]{ '[', 'C', 'F', 'G', ']' };
    static constexpr uint8_t wave_signature[8]{ 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
    static constexpr uint8_t cdf_signature[6]{ 'C', 'D', 'F', '9', '6', 'a' };
    uint32_t resource_size = 0;
    void *resource_data = nullptr;
    int32_t resource_storage = 0;
    while(true)
    {
        int32_t retry = 0;
        char name[128];
        char full_path[128];
        copy_file_name_from_path(name, path);
        lock_runtime_mutex(&runtime_resource_mutex);
        RuntimeResourceCacheEntry *entry = find_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, name);
        if(entry != nullptr)
        {
            resource_size = entry->size;
            resource_data = entry->data;
            resource_storage = 0x01000000;
            ++entry->flags_and_references;
        }
        else if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) == 0)
        {
            build_runtime_resource_path(full_path, path);
            AsyncFileRecord *record = open_async_file_record(runtime_resource_host, full_path, 0, 0, 0);
            if(record == nullptr)
            {
                retry = 0;
            }
            else
            {
                resource_size = get_async_file_size(record);
                if(resource_size < 0x100000 || (flags & RUNTIME_RESOURCE_HOST_MEMORY) != 0)
                {
                    activate_default_comment_scene(loading_scene);
                    resource_data = allocate_runtime_heap(runtime_resource_heap, 0, resource_size);
                    if(resource_data != nullptr)
                    {
                        uint32_t resource_type = RUNTIME_MEDIA_DATA_UNKNOWN;
                        if(read_async_file_record(record, resource_data, resource_size, &resource_type, 0) == 0)
                        {
                            resource_size = 0;
                            void *failed_data = resource_data;
                            resource_data = nullptr;
                            free_runtime_heap(runtime_resource_heap, 0, failed_data);
                            send_application_event(HostApplicationCommand::STORAGE_FAILURE, std::string(full_path));
                            retry = 0;
                        }
                        else
                        {
                            resource_storage = 0x01000000;
                        }
                        if(resource_data != nullptr)
                        {
                            RuntimeResourceCacheEntry *new_entry = nullptr;
                            deactivate_default_comment_scene(loading_scene);
                            reset_runtime_byte_queue();
                            reset_runtime_input_queue();
                            new_entry = get_or_create_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, name);
                            if(new_entry != nullptr)
                            {
                                new_entry->size = resource_size;
                                new_entry->data = resource_data;
                                const auto *bytes = static_cast<const uint8_t *>(resource_data);
                                int16_t animation_marker;
                                std::memcpy(&animation_marker, bytes + 4, sizeof(animation_marker));
                                if(animation_marker == static_cast<int16_t>(0xaf12))
                                    resource_type = RUNTIME_MEDIA_DATA_ANIMATION;
                                uint16_t bitmap_marker;
                                std::memcpy(&bitmap_marker, bytes, sizeof(bitmap_marker));
                                if(bitmap_marker == 0x4d42)
                                    resource_type = RUNTIME_MEDIA_DATA_BITMAP;
                                if(fixed_dword_memory_equal(bytes, configuration_signature, sizeof(configuration_signature)))
                                    resource_type = RUNTIME_MEDIA_DATA_CONFIGURATION;
                                if(fixed_dword_memory_equal(bytes + 8, wave_signature, sizeof(wave_signature)))
                                    resource_type = RUNTIME_MEDIA_DATA_WAVE;
                                if(fixed_dword_memory_equal(bytes, cdf_signature, sizeof(cdf_signature)))
                                    resource_type = RUNTIME_MEDIA_DATA_ARCHIVE;
                                new_entry->flags_and_references = (resource_type << 16) | 1;
                            }
                            goto resource_loaded;
                        }
                    }
                    deactivate_default_comment_scene(loading_scene);
                    reset_runtime_byte_queue();
                    reset_runtime_input_queue();
                }
                else
                {
                    resource_storage = 0x02000000;
                    resource_data = record;
                }
            }
        }
        else if((runtime_scene_control_flags & RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN) == RUNTIME_HOST_RESOURCE_ARCHIVE_OPEN)
        {
            const uint8_t selector = get_cdf_entry_flags(runtime_resource_archive, path);
            const uint32_t archive_size = get_cdf_entry_size(runtime_resource_archive, selector, path);
            if((selector & CDF_ENTRY_COMPRESSED) == 0 && (flags & RUNTIME_RESOURCE_HOST_MEMORY) == 0)
            {
                resource_size = archive_size;
                if(resource_size != 0)
                {
                    resource_data = open_runtime_cdf_entry_stream(runtime_resource_archive, path);
                    resource_storage = 0x02000000;
                }
            }
            else
            {
                activate_default_comment_scene(loading_scene);
                resource_size = archive_size;
                if(resource_size != 0)
                {
                    resource_data = allocate_runtime_heap(runtime_resource_heap, 0, resource_size);
                    if(resource_data == nullptr)
                    {
                        resource_size = 0;
                    }
                    else if(read_cdf_entry(runtime_resource_archive, selector, path, resource_data) == 0)
                    {
                        resource_size = 0;
                        free_runtime_heap(runtime_resource_heap, 0, resource_data);
                        send_application_event(HostApplicationCommand::STORAGE_FAILURE, std::string(path));
                        retry = 0;
                        resource_data = nullptr;
                    }
                    else
                    {
                        resource_storage = 0x01000000;
                    }
                }
                deactivate_default_comment_scene(loading_scene);
                reset_runtime_byte_queue();
                reset_runtime_input_queue();
                if(resource_data != nullptr)
                {
                    RuntimeResourceCacheEntry *new_entry = get_or_create_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, name);
                    if(new_entry != nullptr)
                    {
                        new_entry->size = resource_size;
                        new_entry->data = resource_data;
                        new_entry->flags_and_references = (static_cast<uint32_t>(selector & ~CDF_ENTRY_COMPRESSED) << 16) | 1;
                    }
                }
            }
            if(archive_size == 0)
            {
                const auto [synthesized_data, synthesized_size] = synthesize_resource(runtime_resource_heap, path);
                if(synthesized_size != 0)
                {
                    resource_size = synthesized_size;
                    resource_data = synthesized_data;
                    resource_storage = 0x01000000;
                    RuntimeResourceCacheEntry *new_entry = get_or_create_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, name);
                    if(new_entry != nullptr)
                    {
                        new_entry->size = resource_size;
                        new_entry->data = resource_data;
                        new_entry->flags_and_references = (get_synthesized_resource_type(path) << 16) | 1;
                    }
                }
            }
        }
resource_loaded:
        if(retry == 0 && resource_data != nullptr && resource_storage == 0x02000000)
        {
            if(runtime_resource_streamed_count == 0)
                set_script_runtime_flags(SCRIPT_RUNTIME_STREAMING_ACTIVE, 1);
            ++runtime_resource_streamed_count;
        }
        unlock_runtime_mutex(&runtime_resource_mutex);
        if(retry == 0)
        {
            *data = resource_data;
            *storage = resource_storage;
            *size = resource_size;
            return;
        }
        runtime_sleep(5);
    }
}

bool release_runtime_memory_resource(const char *name)
{
    bool result = false;
    lock_runtime_mutex(&runtime_resource_mutex);
    RuntimeResourceCacheEntry *entry = find_runtime_resource_cache_entry(runtime_resource_cache_parent_identity, name);
    if(entry != nullptr)
    {
        --entry->flags_and_references;
        if((entry->flags_and_references & RUNTIME_RESOURCE_REFERENCE_COUNT_MASK) == 0)
        {
            result = free_runtime_heap(runtime_resource_heap, 0, entry->data);
            remove_runtime_named_child_by_identity(runtime_resource_cache_parent_identity, entry->data);
        }
    }
    unlock_runtime_mutex(&runtime_resource_mutex);
    return result;
}

bool release_runtime_memory_resource_by_data(void *data)
{
    bool result = false;
    lock_runtime_mutex(&runtime_resource_mutex);
    auto *entry = reinterpret_cast<RuntimeResourceCacheEntry *>(find_runtime_named_child(runtime_resource_cache_parent_identity, data));
    if(entry != nullptr)
    {
        --entry->flags_and_references;
        if((entry->flags_and_references & RUNTIME_RESOURCE_REFERENCE_COUNT_MASK) == 0)
        {
            result = free_runtime_heap(runtime_resource_heap, 0, entry->data);
            remove_runtime_named_child_by_identity(runtime_resource_cache_parent_identity, entry->data);
        }
    }
    unlock_runtime_mutex(&runtime_resource_mutex);
    return result;
}

uint32_t release_runtime_streamed_resource(AsyncFileRecord *record)
{
    lock_runtime_mutex(&runtime_resource_mutex);
    uint32_t result = close_async_file_record(record);
    if(result != 0 && runtime_resource_streamed_count != 0)
    {
        --runtime_resource_streamed_count;
        if(runtime_resource_streamed_count == 0)
            set_script_runtime_flags(SCRIPT_RUNTIME_STREAMING_ACTIVE, 0);
    }
    unlock_runtime_mutex(&runtime_resource_mutex);
    return result;
}

uint32_t extract_runtime_drive_prefix(char *destination, const char *source)
{
    uint32_t index = 0;
    do
    {
        char value = source[index];
        if(value == '\0' || value == ':' || value == '\\' || value == '/')
            break;
        destination[index] = value;
        ++index;
    } while(index < 0x0c);
    if(source[index] == ':')
    {
        destination[index] = ':';
        destination[index + 1] = source[index + 1];
        destination[index + 2] = '\0';
        return 1;
    }
    return 0;
}

void advance_async_host_read(AsyncFileHost *host, uint32_t bytes)
{
    const uint32_t sector_size = host->bytes_per_sector;
    const uint32_t aligned = (static_cast<uint32_t>(static_cast<uint8_t *>(host->secondary_cursor) - static_cast<uint8_t *>(host->buffer)) % sector_size + bytes);
    host->available_bytes.fetch_add(aligned - aligned % sector_size, std::memory_order_release);
    host->secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor) + bytes;
    host->current_offset += bytes;
    if(static_cast<uint8_t *>(host->buffer) + host->buffer_size <= host->secondary_cursor)
        host->secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor) - host->buffer_size;
}

void advance_async_host_write(AsyncFileHost *host, uint32_t bytes)
{
    host->buffered_bytes += bytes;
    host->file_offset += bytes;
    host->write_cursor = static_cast<uint8_t *>(host->write_cursor) + bytes;
    host->available_bytes.fetch_sub(bytes, std::memory_order_release);
    if(static_cast<uint8_t *>(host->buffer) + host->buffer_size <= host->write_cursor)
        host->write_cursor = host->buffer;
}

void invalidate_shared_async_records(AsyncFileRecord *record)
{
    const std::shared_ptr<SharedBinaryFile> file = record->file;
    lock_runtime_mutex(&async_file_global_mutex);
    AsyncFileHost *host = record->host;
    lock_runtime_mutex(host->secondary_lock);
    for(AsyncFileRecord *sibling = host->files; sibling != nullptr; sibling = sibling->next)
    {
        if(sibling != record && sibling->file == file)
        {
            sibling->flags &= ~ASYNC_FILE_RECORD_BUFFER_INVALID;
            if(host->active_file == sibling)
            {
                lock_runtime_mutex(host->primary_lock);
                host->active_file = nullptr;
                unlock_runtime_mutex(host->primary_lock);
            }
        }
    }
    unlock_runtime_mutex(host->secondary_lock);
    unlock_runtime_mutex(&async_file_global_mutex);
}

void position_async_host(AsyncFileHost *host, uint32_t offset)
{
    AsyncFileRecord *record = host->active_file;
    if((record->flags & ASYNC_FILE_RECORD_BUFFER_INVALID) == 0)
    {
        const uint32_t sector_size = host->bytes_per_sector;
        host->file_offset = offset / sector_size * sector_size;
        host->write_cursor = host->buffer;
        host->secondary_cursor = static_cast<uint8_t *>(host->buffer) + offset % sector_size;
        host->buffer_start_cursor = host->secondary_cursor;
        host->buffered_bytes = 0;
        host->available_bytes.store(host->buffer_size, std::memory_order_release);
        runtime_sleep(0);
    }
    else
    {
        if((record->flags & ASYNC_FILE_RECORD_SHARED) != 0)
            invalidate_shared_async_records(record);
        const uint32_t previous_offset = record->previous_offset;
        uint32_t copied_bytes = record->next_offset - previous_offset;
        const uint32_t skipped_prefix = previous_offset < record->start_offset ? record->start_offset - previous_offset : 0;
        const uint32_t consumed_bytes = copied_bytes - record->buffered_bytes;
        auto *buffer = static_cast<uint8_t *>(host->buffer);
        host->file_offset = record->next_offset;
        host->secondary_cursor = buffer + consumed_bytes;
        host->buffer_start_cursor = buffer + skipped_prefix;
        host->buffered_bytes = copied_bytes;
        host->write_cursor = buffer + copied_bytes;
        std::memcpy(buffer, record->buffer, copied_bytes);
        host->available_bytes.store(host->buffer_size - copied_bytes + consumed_bytes / host->bytes_per_sector * host->bytes_per_sector, std::memory_order_release);
        record->flags &= ~ASYNC_FILE_RECORD_BUFFER_INVALID;
    }
    host->flags &= ~(ASYNC_FILE_HOST_REPOSITION_PENDING | ASYNC_FILE_HOST_END_REACHED);
}

void seek_async_host(AsyncFileHost *host, uint32_t offset)
{
    const uint32_t current_offset = host->current_offset;
    if(offset == current_offset)
        return;
    if(current_offset < offset && offset - current_offset < host->buffer_size >> 2)
    {
        advance_async_host_read(host, offset - current_offset);
        host->current_offset = offset;
        return;
    }
    lock_runtime_mutex(host->primary_lock);
    if((host->flags & ASYNC_FILE_HOST_END_REACHED) == 0)
    {
        if((host->flags & ASYNC_FILE_HOST_REPOSITION_PENDING) != 0 && 0 < static_cast<int32_t>(host->file_offset - offset))
        {
            uint32_t bytes_to_advance = host->start_offset % host->bytes_per_sector + (offset - host->start_offset);
            auto *read_cursor = static_cast<uint8_t *>(host->read_cursor);
            auto *secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor);
            if(secondary_cursor <= read_cursor)
                bytes_to_advance += static_cast<uint32_t>(read_cursor - secondary_cursor);
            else
                bytes_to_advance += host->buffer_size - static_cast<uint32_t>(secondary_cursor - read_cursor);
            advance_async_host_read(host, bytes_to_advance);
        }
        else
        {
            position_async_host(host, offset);
        }
    }
    else
    {
        auto *cursor = static_cast<uint8_t *>(host->buffer_start_cursor) + (offset - host->start_offset);
        auto *buffer_end = static_cast<uint8_t *>(host->buffer) + host->buffer_size;
        host->secondary_cursor = cursor;
        if(buffer_end < cursor)
            host->secondary_cursor = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(cursor) - reinterpret_cast<uintptr_t>(buffer_end));
    }
    unlock_runtime_mutex(host->primary_lock);
    host->current_offset = offset;
    host->flags &= ~ASYNC_FILE_HOST_REPOSITION_PENDING;
}

uint32_t copy_async_host_bytes(AsyncFileHost *host, void *destination, uint32_t bytes, uint32_t *total_bytes)
{
    if(host->file_size < host->current_offset + bytes)
        bytes = host->file_size - host->current_offset;
    auto *buffer_end = static_cast<uint8_t *>(host->buffer) + host->buffer_size;
    const uint32_t sector_size = host->bytes_per_sector;
    *total_bytes += bytes;
    if(bytes == 0)
        return 0;
    if((host->flags & ASYNC_FILE_HOST_END_REACHED) == 0)
    {
        uint32_t used_bytes = host->buffer_size - host->available_bytes.load(std::memory_order_acquire);
        while(used_bytes <= bytes + sector_size * 2)
        {
            runtime_sleep(0);
            used_bytes = host->buffer_size - host->available_bytes.load(std::memory_order_acquire);
        }
    }
    auto *source = static_cast<uint8_t *>(host->secondary_cursor);
    auto *output = static_cast<uint8_t *>(destination);
    if(buffer_end < source + bytes)
    {
        const uint32_t tail_bytes = static_cast<uint32_t>(buffer_end - source);
        std::memcpy(output, source, tail_bytes);
        advance_async_host_read(host, tail_bytes);
        bytes -= tail_bytes;
        output += tail_bytes;
    }
    std::memcpy(output, host->secondary_cursor, bytes);
    advance_async_host_read(host, bytes);
    return 1;
}

void activate_async_file_record(AsyncFileRecord *record)
{
    AsyncFileHost *host = record->host;
    if(host->active_file == record)
        return;
    lock_runtime_mutex(host->primary_lock);
    const uint32_t read_chunk_size = 0x8000 / host->bytes_per_sector * host->bytes_per_sector;
    host->active_file = record;
    host->file = record->file;
    host->file_size = record->file_size;
    host->remaining_size = record->remaining_size;
    host->start_offset = record->start_offset;
    host->end_offset = record->end_offset;
    host->current_offset = record->current_offset;
    position_async_host(host, host->current_offset);
    uint32_t initial_read_size = 0;
    const uint32_t flags = record->flags;
    if((flags & ASYNC_FILE_RECORD_INITIAL_READ) != 0)
    {
        if((flags & ASYNC_FILE_RECORD_PREFETCH) != 0)
        {
            const uint32_t limit = host->remaining_size < host->buffer_size ? host->remaining_size : host->buffer_size;
            initial_read_size = (limit / host->bytes_per_sector >> 2) * host->bytes_per_sector;
        }
        record->flags = flags & ~ASYNC_FILE_RECORD_INITIAL_READ;
    }
    if(initial_read_size != 0)
    {
        uint32_t remaining = initial_read_size;
        uint32_t transferred = 0;
        auto *output = static_cast<uint8_t *>(host->write_cursor);
        do
        {
            uint32_t bytes_to_read = remaining;
            if(read_chunk_size <= remaining)
                bytes_to_read = read_chunk_size;
            remaining -= bytes_to_read;
            const uint32_t bytes_read = host->file->read_at(host->file_offset, output, bytes_to_read);
            runtime_sleep(0);
            output += bytes_read;
            transferred += bytes_read;
            if(bytes_read != bytes_to_read)
                remaining = 0;
        } while(remaining != 0);
        advance_async_host_write(host, transferred);
    }
    unlock_runtime_mutex(host->primary_lock);
}

void handle_async_host_short_read(AsyncFileHost *host)
{
    const uint32_t sector_size = host->bytes_per_sector;
    const uint32_t value = host->end_offset - reinterpret_cast<uintptr_t>(host->buffer_start_cursor) % sector_size - host->file_offset + host->buffered_bytes;
    host->buffered_bytes = value;
    if(host->remaining_size <= host->buffer_size && host->remaining_size <= value)
    {
        const uint32_t flags = host->flags;
        host->flags = (flags & ~ASYNC_FILE_HOST_REPOSITION_PENDING) | ASYNC_FILE_HOST_END_REACHED;
        return;
    }
    host->flags |= ASYNC_FILE_HOST_REPOSITION_PENDING;
    host->buffered_bytes = 0;
    host->buffer_start_cursor = static_cast<uint8_t *>(host->write_cursor) + host->start_offset % sector_size;
    host->read_cursor = host->write_cursor;
    host->file_offset = host->start_offset / sector_size * sector_size;
    runtime_sleep(0);
}

void run_async_file_worker(AsyncFileHost *host)
{
    uint8_t *buffer_end = static_cast<uint8_t *>(host->buffer) + host->buffer_size;
    const uint32_t sector_size = host->bytes_per_sector;
    const uint32_t maximum_read = 0xc000 / sector_size * sector_size;
    const uint32_t minimum_available = 0x4000 / sector_size * sector_size;
    bool restart_timing = true;
    uint32_t delay = minimum_available;
    uint32_t target_time = 0;
    uint32_t rate = 0;
    uint32_t flags = host->flags;
    while(true)
    {
        if((flags & ASYNC_FILE_HOST_SHUTDOWN_REQUESTED) != 0)
            return;
        if(host->active_file == nullptr)
        {
            runtime_sleep(10);
            restart_timing = true;
        }
        else if((host->flags & ASYNC_FILE_HOST_END_REACHED) == 0)
        {
            if(restart_timing)
            {
                delay = 0;
                rate = host->mode - (host->mode >> 2);
                target_time = runtime_milliseconds();
                restart_timing = false;
            }
            uint32_t next_target = target_time;
            lock_runtime_mutex(host->primary_lock);
            const uint32_t available_bytes = host->available_bytes.load(std::memory_order_acquire);
            if(host->active_file == nullptr || available_bytes < minimum_available)
            {
                unlock_runtime_mutex(host->primary_lock);
                runtime_sleep(0);
            }
            else
            {
                uint32_t bytes_to_read = maximum_read;
                if(available_bytes <= maximum_read)
                    bytes_to_read = available_bytes;
                next_target = bytes_to_read / rate + target_time;
                if(buffer_end < static_cast<uint8_t *>(host->write_cursor) + bytes_to_read)
                {
                    const uint32_t tail_bytes = static_cast<uint32_t>(buffer_end - static_cast<uint8_t *>(host->write_cursor));
                    runtime_sleep(0);
                    const uint32_t bytes_read = host->file->read_at(host->file_offset, host->write_cursor, tail_bytes);
                    advance_async_host_write(host, bytes_read);
                    if(tail_bytes <= bytes_read && host->file_offset < host->end_offset)
                    {
                        bytes_to_read -= tail_bytes;
                        runtime_sleep(0);
                        const uint32_t head_bytes = host->file->read_at(host->file_offset, host->write_cursor, bytes_to_read);
                        advance_async_host_write(host, head_bytes);
                        if(head_bytes < bytes_to_read || host->end_offset <= host->file_offset)
                            handle_async_host_short_read(host);
                    }
                    else
                    {
                        handle_async_host_short_read(host);
                    }
                }
                else
                {
                    runtime_sleep(0);
                    const uint32_t bytes_read = host->file->read_at(host->file_offset, host->write_cursor, bytes_to_read);
                    advance_async_host_write(host, bytes_read);
                    if(bytes_read < bytes_to_read || host->end_offset <= host->file_offset)
                        handle_async_host_short_read(host);
                }
                unlock_runtime_mutex(host->primary_lock);
            }
            const uint32_t now = runtime_milliseconds();
            const uint32_t requested_rate = host->mode;
            if(rate != requested_rate)
            {
                int32_t adjustment = static_cast<int32_t>(requested_rate - rate) >> 1;
                if(adjustment == 0)
                    adjustment = 1;
                rate += adjustment;
                if(static_cast<int32_t>(requested_rate) < static_cast<int32_t>(rate))
                    rate = requested_rate;
            }
            delay = delay - now + next_target;
            target_time = now;
            if(0 < static_cast<int32_t>(delay))
            {
                runtime_sleep(delay);
                target_time = delay + now;
                delay = 0;
            }
        }
        else
        {
            runtime_sleep(10);
            restart_timing = true;
        }
        flags = host->flags;
    }
}

AsyncFileHost *create_async_file_host(const char *root, uint32_t requested_bytes, int32_t mode)
{
    (void)root;
    if(!async_file_enabled)
        return nullptr;
    AsyncFileHost *host = new (std::nothrow) AsyncFileHost{};
    if(host == nullptr)
        return nullptr;
    host->bytes_per_sector = 512;
    host->primary_lock = new (std::nothrow) RuntimeMutex;
    host->secondary_lock = new (std::nothrow) RuntimeMutex;
    if(host->primary_lock == nullptr || host->secondary_lock == nullptr)
    {
        delete host->primary_lock;
        delete host->secondary_lock;
        delete host;
        return nullptr;
    }
    host->buffer_size = requested_bytes / 0xffff * 0xffff / host->bytes_per_sector * host->bytes_per_sector;
    host->available_bytes.store(host->buffer_size, std::memory_order_relaxed);
    host->buffer = new (std::nothrow) uint8_t[host->buffer_size]{};
    if(host->buffer == nullptr)
    {
        delete host->primary_lock;
        delete host->secondary_lock;
        delete host;
        return nullptr;
    }
    host->self = host;
    host->write_cursor = host->buffer;
    host->secondary_cursor = host->buffer;
    host->mode = mode == 0 ? -1 : mode;
    lock_runtime_mutex(&async_file_global_mutex);
    host->next = async_file_hosts;
    async_file_hosts = host;
    unlock_runtime_mutex(&async_file_global_mutex);
    try
    {
        host->thread = new std::jthread([host]() { run_async_file_worker(host); });
    }
    catch(...)
    {
        lock_runtime_mutex(&async_file_global_mutex);
        async_file_hosts = host->next;
        unlock_runtime_mutex(&async_file_global_mutex);
        delete[] static_cast<uint8_t *>(host->buffer);
        delete host->primary_lock;
        delete host->secondary_lock;
        delete host;
        return nullptr;
    }
    return host;
}

AsyncFileHost *acquire_async_file_host(AsyncFileHost *identity)
{
    if(!async_file_enabled)
        return nullptr;
    while(true)
    {
        uint32_t busy = 0;
        AsyncFileHost *result = nullptr;
        lock_runtime_mutex(&async_file_global_mutex);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            if(host->self == identity)
            {
                busy = host->flags & ASYNC_FILE_HOST_LOCKED;
                if(busy == 0)
                {
                    host->flags |= ASYNC_FILE_HOST_LOCKED;
                    result = host;
                }
                break;
            }
        }
        unlock_runtime_mutex(&async_file_global_mutex);
        if(busy == 0)
            return result;
        runtime_sleep(0);
    }
}

void release_async_file_host(AsyncFileHost *identity)
{
    lock_runtime_mutex(&async_file_global_mutex);
    for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
    {
        if(host->self == identity)
        {
            host->flags &= ~ASYNC_FILE_HOST_LOCKED;
            break;
        }
    }
    unlock_runtime_mutex(&async_file_global_mutex);
}

uint32_t destroy_async_file_host(AsyncFileHost *identity)
{
    if(!async_file_enabled)
        return 0;
    AsyncFileHost *host = acquire_async_file_host(identity);
    if(host == nullptr)
        return 0;
    while(host->files != nullptr)
        close_async_file_record(host->files);
    lock_runtime_mutex(&async_file_global_mutex);
    AsyncFileHost *previous = nullptr;
    AsyncFileHost *current = async_file_hosts;
    while(current != nullptr && current->self != identity)
    {
        previous = current;
        current = current->next;
    }
    if(previous == nullptr)
        async_file_hosts = current->next;
    else
        previous->next = current->next;
    unlock_runtime_mutex(&async_file_global_mutex);
    current->flags |= ASYNC_FILE_HOST_SHUTDOWN_REQUESTED;
    current->thread->join();
    delete current->primary_lock;
    delete current->secondary_lock;
    delete current->thread;
    delete[] static_cast<uint8_t *>(current->buffer);
    delete current;
    return 1;
}

uint32_t shutdown_async_file_subsystem()
{
    if(!async_file_enabled)
        return 0;
    while(true)
    {
        while(async_file_hosts != nullptr)
            destroy_async_file_host(async_file_hosts);
        lock_runtime_mutex(&async_file_global_mutex);
        if(async_file_hosts == nullptr)
            break;
        unlock_runtime_mutex(&async_file_global_mutex);
    }
    async_file_enabled = false;
    async_file_hosts = nullptr;
    return 1;
}


AsyncFileRecord *acquire_async_file_record(AsyncFileRecord *identity)
{
    if(!async_file_enabled)
        return nullptr;
    while(true)
    {
        uint32_t busy = 0;
        AsyncFileRecord *result = nullptr;
        lock_runtime_mutex(&async_file_global_mutex);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    uint32_t flags = record->flags;
                    busy = flags & ASYNC_FILE_RECORD_LOCKED;
                    if(busy == 0)
                    {
                        record->flags = flags | ASYNC_FILE_RECORD_LOCKED;
                        result = record;
                        if((flags & ASYNC_FILE_RECORD_SHARED) != 0)
                        {
                            for(AsyncFileRecord *shared = host->files; shared != nullptr; shared = shared->next)
                                if(shared->file == record->file)
                                    shared->flags |= ASYNC_FILE_RECORD_LOCKED;
                        }
                    }
                    host = nullptr;
                    break;
                }
            }
            if(host == nullptr)
                break;
        }
        unlock_runtime_mutex(&async_file_global_mutex);
        if(busy == 0)
            return result;
        runtime_sleep(0);
    }
}

void release_async_file_record(AsyncFileRecord *identity)
{
    if(async_file_enabled)
    {
        lock_runtime_mutex(&async_file_global_mutex);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    uint32_t flags = record->flags;
                    if((flags & ASYNC_FILE_RECORD_LOCKED) != 0)
                    {
                        record->flags = flags & ~ASYNC_FILE_RECORD_LOCKED;
                        if((flags & ASYNC_FILE_RECORD_SHARED) != 0)
                        {
                            for(AsyncFileRecord *shared = host->files; shared != nullptr; shared = shared->next)
                                if(shared->file == record->file)
                                    shared->flags &= ~ASYNC_FILE_RECORD_LOCKED;
                        }
                    }
                    host = nullptr;
                    break;
                }
            }
            if(host == nullptr)
                break;
        }
        unlock_runtime_mutex(&async_file_global_mutex);
    }
}

void set_async_file_host_mode(AsyncFileHost *identity, int32_t mode)
{
    AsyncFileHost *host = acquire_async_file_host(identity);
    if(host != nullptr)
    {
        if(mode != 0)
            host->mode = mode;
        release_async_file_host(identity);
    }
}

uint32_t get_async_file_size(AsyncFileRecord *identity)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
        return 0;
    uint32_t size = record->file_size;
    release_async_file_record(identity);
    return size;
}

uint32_t get_async_file_position(AsyncFileRecord *identity)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
        return 0;
    const uint32_t position = record->current_offset;
    release_async_file_record(identity);
    return position;
}

uint32_t set_async_file_position(AsyncFileRecord *identity, uint32_t position)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
        return 0;
    uint32_t result = 0;
    if(record->start_offset <= position && position <= record->end_offset)
    {
        if((record->flags & ASYNC_FILE_RECORD_BUFFER_INVALID) != 0)
        {
            if(position < record->previous_offset || record->next_offset < position)
            {
                record->flags &= ~ASYNC_FILE_RECORD_BUFFER_INVALID;
            }
            else
            {
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer) + (position - record->previous_offset);
                record->buffered_bytes = record->next_offset - position;
            }
        }
        record->current_offset = position;
        if((record->flags & ASYNC_FILE_RECORD_SHARED) != 0)
            invalidate_shared_async_records(record);
        result = 1;
    }
    release_async_file_record(identity);
    return result;
}

AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags)
{
    auto file = std::make_shared<SharedBinaryFile>(path);
    if(!file->is_open())
        return nullptr;
    return open_async_file_record(host_identity, std::move(file), start_offset, end_offset, flags);
}

AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, std::shared_ptr<SharedBinaryFile> file, uint32_t start_offset, uint32_t end_offset, uint32_t flags)
{
    AsyncFileHost *host = acquire_async_file_host(host_identity);
    if(host == nullptr)
        return nullptr;
    AsyncFileRecord *record = nullptr;
    bool valid = file->is_open();
    if(valid)
    {
        record = new (std::nothrow) AsyncFileRecord{};
        if(record == nullptr)
            valid = false;
        if(valid)
        {
            record->buffer = new (std::nothrow) uint8_t[0x8000]{};
            if(record->buffer == nullptr)
            {
                AsyncFileRecord *failed_record = record;
                record = nullptr;
                valid = false;
                delete failed_record;
            }
        }
    }
    if(valid)
    {
        record->self = record;
        record->flags = flags | ASYNC_FILE_RECORD_INITIAL_READ;
        record->file = file;
        record->file_size = file->size();
        record->start_offset = start_offset;
        if(end_offset == 0)
            end_offset = record->file_size;
        record->end_offset = end_offset;
        record->remaining_size = end_offset - start_offset;
        record->current_offset = start_offset;
        record->host = host;
        lock_runtime_mutex(&async_file_global_mutex);
        record->next = host->files;
        host->files = record;
        unlock_runtime_mutex(&async_file_global_mutex);
    }
    release_async_file_host(host_identity);
    return record;
}

AsyncFileRecord *duplicate_async_file_record(AsyncFileRecord *identity, uint32_t start_offset, uint32_t end_offset, uint32_t flags)
{
    AsyncFileRecord *source = acquire_async_file_record(identity);
    if(source == nullptr)
        return nullptr;
    AsyncFileHost *host = source->host;
    AsyncFileRecord *record = new (std::nothrow) AsyncFileRecord{};
    if(record != nullptr)
    {
        record->self = record;
        record->flags = flags | ASYNC_FILE_RECORD_INITIAL_READ | ASYNC_FILE_RECORD_SHARED;
        source->flags |= ASYNC_FILE_RECORD_SHARED;
        record->file = source->file;
        record->buffer = source->buffer;
        record->file_size = source->file_size;
        record->start_offset = start_offset;
        if(end_offset == 0)
            end_offset = record->file_size;
        record->end_offset = end_offset;
        record->remaining_size = end_offset - start_offset;
        record->current_offset = start_offset;
        lock_runtime_mutex(&async_file_global_mutex);
        record->next = host->files;
        host->files = record;
        unlock_runtime_mutex(&async_file_global_mutex);
        record->host = host;
    }
    release_async_file_record(identity);
    return record;
}

uint32_t close_async_file_record(AsyncFileRecord *identity)
{
    if(!async_file_enabled)
        return 0;
    uint32_t result = 0;
    while(true)
    {
        uint32_t busy = 0;
        lock_runtime_mutex(&async_file_global_mutex);
        bool finished = false;
        for(AsyncFileHost *host = async_file_hosts; host != nullptr && !finished; host = host->next)
        {
            AsyncFileRecord *previous = nullptr;
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    busy = record->flags & ASYNC_FILE_RECORD_LOCKED;
                    if(busy == 0)
                    {
                        ++result;
                        if(previous == nullptr)
                            host->files = record->next;
                        else
                            previous->next = record->next;
                        lock_runtime_mutex(host->secondary_lock);
                        if(host->active_file == record)
                        {
                            lock_runtime_mutex(host->primary_lock);
                            host->active_file = nullptr;
                            unlock_runtime_mutex(host->primary_lock);
                        }
                        unlock_runtime_mutex(host->secondary_lock);
                        int32_t shared_count = 0;
                        AsyncFileRecord *single_shared = nullptr;
                        if((record->flags & ASYNC_FILE_RECORD_SHARED) != 0)
                        {
                            for(AsyncFileRecord *shared = host->files; shared != nullptr; shared = shared->next)
                            {
                                if(shared->file == record->file)
                                {
                                    ++shared_count;
                                    single_shared = shared;
                                }
                            }
                            if(shared_count == 1)
                                single_shared->flags &= ~ASYNC_FILE_RECORD_SHARED;
                        }
                        if(shared_count == 0)
                            delete[] static_cast<uint8_t *>(record->buffer);
                        delete record;
                    }
                    finished = true;
                    break;
                }
                previous = record;
            }
        }
        unlock_runtime_mutex(&async_file_global_mutex);
        if(busy == 0)
            return result;
        runtime_sleep(0);
    }
}

uint32_t read_async_file_record(AsyncFileRecord *identity, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer)
{
    *bytes_read = 0;
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
        return 0;
    AsyncFileHost *host = record->host;
    uint32_t result = 1;
    if(force_host_buffer != 0 || host->active_file == record)
    {
        while(true)
        {
            lock_runtime_mutex(host->secondary_lock);
            if(force_host_buffer != 0 || host->active_file == record)
                break;
            unlock_runtime_mutex(host->secondary_lock);
        }
        if(host->active_file == record)
            seek_async_host(host, record->current_offset);
        else
            activate_async_file_record(record);
        const uint32_t chunk_size = (host->buffer_size / host->bytes_per_sector >> 2) * host->bytes_per_sector;
        auto *output = static_cast<uint8_t *>(destination);
        while(bytes != 0)
        {
            uint32_t chunk = chunk_size;
            if(bytes <= chunk_size)
                chunk = bytes;
            if(copy_async_host_bytes(host, output, chunk, bytes_read) == 0)
            {
                result = 0;
                break;
            }
            output += chunk;
            bytes -= chunk;
        }
        record->current_offset = host->current_offset;
        unlock_runtime_mutex(host->secondary_lock);
    }
    else
    {
        uint32_t copied = 0;
        const uint32_t chunk_size = 0x8000 / host->bytes_per_sector * host->bytes_per_sector;
        auto *output = static_cast<uint8_t *>(destination);
        while(bytes != 0)
        {
            uint32_t chunk = record->buffered_bytes;
            if(chunk != 0 && (record->flags & ASYNC_FILE_RECORD_BUFFER_INVALID) != 0)
            {
                if(bytes < chunk)
                    chunk = bytes;
                std::memcpy(output, record->buffer_cursor, chunk);
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer_cursor) + chunk;
                record->buffered_bytes -= chunk;
                output += chunk;
                bytes -= chunk;
                copied += chunk;
            }
            if(bytes == 0)
                break;
            const uint32_t target_time = record->timestamp + chunk_size / host->mode;
            runtime_sleep(0);
            uint32_t file_bytes = 0;
            if((record->flags & ASYNC_FILE_RECORD_BUFFER_INVALID) == 0)
            {
                const uint32_t aligned_offset = record->current_offset / host->bytes_per_sector * host->bytes_per_sector;
                record->next_offset = aligned_offset;
                record->previous_offset = aligned_offset;
                file_bytes = record->file->read_at(aligned_offset, record->buffer, chunk_size);
                if(file_bytes == 0)
                    break;
                if((record->flags & ASYNC_FILE_RECORD_SHARED) != 0)
                    invalidate_shared_async_records(record);
                record->flags |= ASYNC_FILE_RECORD_BUFFER_INVALID;
                const uint32_t prefix = record->current_offset % host->bytes_per_sector;
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer) + prefix;
                record->buffered_bytes = file_bytes - prefix;
            }
            else
            {
                file_bytes = record->file->read_at(record->next_offset, record->buffer, chunk_size);
                if(file_bytes == 0)
                    break;
                record->buffer_cursor = record->buffer;
                record->buffered_bytes = file_bytes;
            }
            record->previous_offset = record->next_offset;
            record->next_offset += file_bytes;
            const uint32_t now = runtime_milliseconds();
            record->timestamp = now;
            if(now < target_time)
                runtime_sleep(target_time - now);
        }
        record->current_offset += copied;
        *bytes_read = copied;
    }
    release_async_file_record(identity);
    return result;
}



} // namespace freegag
