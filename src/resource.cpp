#include "resource.h"
#include <cstring>
#include "runtime_internal.h"

namespace gag
{
RuntimeResourceConstructionPlan prepare_runtime_resource_construction(uint32_t scene_identifier, int32_t x, int32_t y, uint32_t flags)
{
    RuntimeResourceConstructionPlan plan{ flags, scene_identifier, 0, x, y };
    if((flags & 0x40) == 0)
    {
        if((flags & 1) != 0)
        {
            plan.scene_identifier = 0;
            plan.scene_flags = 0x20000;
            plan.flags |= 0x80;
        }
        if((plan.flags & 0x20) != 0 && plan.scene_identifier == 0)
        {
            plan.scene_identifier = runtime_resource_construction_plan_api.find_available_scene(0x8000);
            plan.scene_flags |= 0x40;
        }
        if((plan.flags & 6) == 0)
        {
            plan.scene_flags |= 0x8000000;
        }
    }
    else
    {
        if(scene_identifier == 0)
        {
            plan.scene_identifier = runtime_resource_construction_plan_api.find_available_scene(1);
        }
        plan.scene_flags = 0x40;
    }
    if((plan.flags & 4) != 0)
    {
        plan.scene_identifier = runtime_resource_construction_plan_api.find_available_scene(0x80000);
        plan.x = 10000;
        plan.y = 10000;
    }
    if((plan.flags & 2) != 0)
    {
        plan.flags |= 0x100600;
        plan.scene_flags |= 0x40;
        plan.scene_identifier = runtime_resource_construction_plan_api.find_available_scene(0x100000);
        plan.x = 10000;
        plan.y = 10000;
    }
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
    const uint32_t type = runtime_resource_construction_api.detect_type(path);
    if(type == 1)
    {
        const uint32_t scene_flags = plan.scene_flags & ~1U;
        runtime_resource_construction_api.update_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        runtime_resource_construction_api.load(path, &data, &data_size, &storage, 0x20000000);
        if(data != nullptr)
        {
            RuntimeMediaBackend *backend = runtime_resource_construction_api.create_bitmap(0, 0, data);
            if(backend != nullptr && backend->error_state == 0)
            {
                const auto *format = static_cast<const int32_t *>(backend->format_data);
                const uint32_t source_width = static_cast<uint32_t>(format[1]);
                const uint32_t source_height = static_cast<uint32_t>(format[2] < 0 ? -format[2] : format[2]);
                if((flags & 2) != 0)
                {
                    if(width == 0)
                    {
                        width = source_width >> 1;
                    }
                    if(height == 0)
                    {
                        height = source_height >> 1;
                    }
                }
                resource = static_cast<RuntimeResourceObject *>(
                    runtime_resource_construction_api.heap_alloc(runtime_resource_construction_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    backend->extension_data = resource;
                    resource->type_flags = (flags & 0xff) | 0x1000;
                    resource->backend = backend;
                    resource->data = data;
                    resource->presentation_owner = runtime_resource_presentation_owner;
                    const uint32_t high_flags = flags & 0xffffff00;
                    resource->backend_flags = backend->media_flags | high_flags;
                    resource->x = x;
                    resource->y = y;
                    resource->previous_x = x;
                    resource->previous_y = y;
                    resource->requested_width = width;
                    resource->requested_height = height;
                    resource->output_width = source_width;
                    resource->output_height = source_height;
                    resource->scene_identifier = reinterpret_cast<intptr_t>(runtime_resource_construction_api.acquire_scene(scene_identifier, x, y, source_width, source_height, scene_flags,
                        reinterpret_cast<intptr_t>(resource), &resource->scene_descriptor, nullptr));
                    if(resource->scene_identifier != 0)
                    {
                        resource->callback_position = resource->scene_descriptor.pixels;
                        DisplayRectangle source_rectangle{ 0, 0, static_cast<int32_t>(source_width), static_cast<int32_t>(source_height) };
                        if((flags & 6) == 0)
                        {
                            ++runtime_resource_count;
                        }
                        if((flags & 1) != 0)
                        {
                            runtime_resource_construction_api.configure_bitmap(backend, &resource->scene_descriptor, nullptr, high_flags | 0x4000100);
                            runtime_resource_construction_api.begin_scene(resource->scene_identifier);
                            runtime_resource_construction_api.finalize_media(backend);
                            runtime_resource_construction_api.configure_palette(resource);
                            runtime_resource_construction_api.end_scene(resource->scene_identifier, nullptr, nullptr);
                            current_runtime_resource = resource;
                        }
                        else
                        {
                            runtime_resource_construction_api.configure_bitmap(backend, &resource->scene_descriptor, runtime_game_host_context.palette_entries, high_flags | 0x4000100);
                            if((flags & 0x10) == 0)
                            {
                                runtime_resource_construction_api.begin_scene(resource->scene_identifier);
                                runtime_resource_construction_api.finalize_media(backend);
                                runtime_resource_construction_api.configure_palette(resource);
                                const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
                                runtime_resource_construction_api.end_scene(resource->scene_identifier, &transform, &source_rectangle);
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
                {
                    runtime_resource_construction_api.destroy_media(backend);
                }
                if(resource != nullptr)
                {
                    runtime_resource_construction_api.release_memory(path);
                    runtime_resource_construction_api.heap_free(runtime_resource_construction_api.get_process_heap(), 0, resource);
                }
                resource = nullptr;
            }
        }
    }
    else if(type == 2)
    {
        runtime_resource_construction_api.update_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        runtime_resource_construction_api.load(path, &data, &data_size, &storage, 0x20000000);
        if(data != nullptr)
        {
            auto *wave_file = static_cast<RuntimePcmWaveFile *>(data);
            const uint32_t sound = runtime_resource_construction_api.create_sound(&wave_file->format);
            if(sound != 0)
            {
                auto *wave = reinterpret_cast<RuntimeRiffChunk *>(wave_file + 1);
                constexpr char wave_data_chunk_id[4]{ 'd', 'a', 't', 'a' };
                while(!fixed_dword_memory_equal(wave->identifier, wave_data_chunk_id, sizeof(wave_data_chunk_id)))
                {
                    wave = reinterpret_cast<RuntimeRiffChunk *>(reinterpret_cast<uint8_t *>(wave) + 1);
                }
                resource = static_cast<RuntimeResourceObject *>(
                    runtime_resource_construction_api.heap_alloc(runtime_resource_construction_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    resource->type_flags = (flags & 0xff) | 0x8000;
                    resource->backend = reinterpret_cast<void *>(static_cast<uintptr_t>(sound));
                    resource->data = data;
                    runtime_resource_construction_api.start_sound(sound, 1);
                    runtime_resource_construction_api.queue_sound(sound, wave->data, wave->size, 1);
                    runtime_resource_construction_api.set_sound_loop(sound, (flags & 0x400) != 0 ? 0xffffffff : (scale_or_loop == 0 ? 1 : scale_or_loop));
                    runtime_resource_construction_api.set_sound_playback_marker(sound, 0xffffffff);
                    if((flags & 0x200) == 0)
                    {
                        runtime_resource_construction_api.stop_sound(sound, 1);
                    }
                    constructed = true;
                    result = resource;
                }
            }
            if(!constructed)
            {
                if(sound != 0)
                {
                    runtime_resource_construction_api.destroy_sound(sound);
                }
                result = nullptr;
                runtime_resource_construction_api.release_memory(path);
            }
        }
    }
    else if(type == 3)
    {
        runtime_resource_construction_api.update_host(path, 0);
        void *data = nullptr;
        uint32_t data_size = 0;
        int32_t storage = 0;
        runtime_resource_construction_api.load(path, &data, &data_size, &storage, 0);
        if(data != nullptr)
        {
            RuntimeAnimationBackend *backend = runtime_resource_construction_api.create_animation(0, data, 0, storage);
            if(backend != nullptr && backend->base.error_state == 0)
            {
                if(scale_or_loop != 0)
                {
                    flags |= 0x400;
                }
                const bool half_size = (flags & 2) != 0;
                const auto *format = static_cast<const RuntimeAnimationFileHeader *>(backend->base.format_data);
                const uint32_t source_width = format->width;
                const uint32_t source_height = format->height;
                if(half_size)
                {
                    if(width == 0)
                    {
                        width = source_width >> 1;
                    }
                    if(height == 0)
                    {
                        height = source_height >> 1;
                    }
                }
                else
                {
                    if(width == 0)
                    {
                        width = 1;
                    }
                    if(height == 0)
                    {
                        height = 1;
                    }
                    backend->base.scale_x = width;
                    backend->base.scale_y = height;
                }
                const uint32_t output_width = half_size ? source_width : source_width * width;
                const uint32_t output_height = half_size ? source_height : source_height * height;
                resource = static_cast<RuntimeResourceObject *>(
                    runtime_resource_construction_api.heap_alloc(runtime_resource_construction_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    backend->base.extension_data = resource;
                    resource->type_flags = (flags & 0xff) | 0x2000;
                    const uint32_t high_flags = flags & 0xffffff00;
                    resource->backend = backend;
                    resource->data = data;
                    resource->presentation_owner = runtime_resource_presentation_owner;
                    resource->backend_flags = backend->base.media_flags | high_flags;
                    resource->x = x;
                    resource->y = y;
                    resource->previous_x = x;
                    resource->previous_y = y;
                    resource->frame_limit = scale_or_loop == 0 ? 0xffffffff : scale_or_loop - 1;
                    resource->frames_remaining = resource->frame_limit;
                    resource->requested_width = width;
                    resource->requested_height = height;
                    resource->output_width = output_width;
                    resource->output_height = output_height;
                    resource->scene_identifier = reinterpret_cast<intptr_t>(runtime_resource_construction_api.acquire_scene(scene_identifier, x, y, output_width, output_height, plan.scene_flags,
                        reinterpret_cast<intptr_t>(resource), &resource->scene_descriptor, nullptr));
                    if(resource->scene_identifier != 0)
                    {
                        resource->callback_position = resource->scene_descriptor.pixels;
                        if((flags & 1) != 0)
                        {
                            runtime_resource_construction_api.configure_animation(backend, &resource->scene_descriptor, nullptr, high_flags | 0x4000200, update_runtime_resource_animation_backend);
                            const uint32_t count = runtime_resource_count + 1;
                            runtime_resource_construction_api.finalize_media(backend);
                            runtime_resource_construction_api.wait_for_count(count);
                            current_runtime_resource = resource;
                        }
                        else
                        {
                            runtime_resource_construction_api.configure_animation(backend, &resource->scene_descriptor, nullptr, high_flags | 0x4000000, update_runtime_resource_animation_backend);
                            if((flags & 0x10) == 0)
                            {
                                runtime_resource_construction_api.finalize_media(backend);
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
                {
                    runtime_resource_construction_api.destroy_media(backend);
                }
                if(resource != nullptr)
                {
                    const uint32_t storage_flags = resource->backend_flags & 0x3000000;
                    if(storage_flags == 0x1000000)
                    {
                        runtime_resource_construction_api.release_memory(path);
                    }
                    else if(storage_flags == 0x2000000)
                    {
                        runtime_resource_construction_api.release_stream(static_cast<AsyncFileRecord *>(resource->data));
                    }
                    runtime_resource_construction_api.heap_free(runtime_resource_construction_api.get_process_heap(), 0, resource);
                }
                resource = nullptr;
            }
        }
    }
    else if(type == 4 && (flags & 0x10000) == 0)
    {
        runtime_resource_construction_api.update_host(path, 0);
        result = runtime_resource_construction_api.find_generic_resource(path);
        if(result != nullptr && (flags & 0x200) == 0)
        {
            runtime_resource_construction_api.rebuild_tree(runtime_resource_construction_api.activate_tree(path, "CFG", nullptr, nullptr));
        }
    }
    else if(type == 0 || (type == 4 && (flags & 0x10000) != 0))
    {
        runtime_resource_construction_api.update_host(path, 0);
        void *data = nullptr;
        uint32_t size = 0;
        int32_t storage = 0;
        runtime_resource_construction_api.load(path, &data, &size, &storage, 0x20000000);
        if(data != nullptr)
        {
            RuntimeGenericBackend *backend = runtime_resource_construction_api.create_generic(reinterpret_cast<uintptr_t>(data), size);
            result = backend;
            if(backend != nullptr)
            {
                resource = static_cast<RuntimeResourceObject *>(
                    runtime_resource_construction_api.heap_alloc(runtime_resource_construction_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(RuntimeResourceObject)));
                if(resource != nullptr)
                {
                    resource->type_flags = (flags & 0xff) | 0x10000;
                    resource->backend = backend;
                    resource->data = data;
                    constructed = true;
                    result = resource;
                }
            }
            if(!constructed)
            {
                if(backend != nullptr)
                {
                    runtime_resource_construction_api.destroy_generic(backend);
                }
                result = nullptr;
                runtime_resource_construction_api.release_memory(path);
            }
        }
    }
    else if(type == 5)
    {
        runtime_resource_construction_api.update_host(path, 1);
        char full_path[260];
        runtime_resource_construction_api.build_path(full_path, path);
        result = runtime_resource_construction_api.open_archive(full_path, runtime_resource_archive_alternate_stream);
        if(result != nullptr)
        {
            runtime_resource_archive = static_cast<CdfArchive *>(result);
            copy_string(runtime_graphics_resource_directory, path);
            runtime_display_context.flags |= 0x10000000;
            if((flags & 0x200) == 0)
            {
                result = runtime_resource_construction_api.activate_tree("Start.cfg", "CFG", nullptr, nullptr);
                runtime_resource_construction_api.rebuild_tree(result);
            }
        }
    }

    if(resource != nullptr)
    {
        runtime_resource_construction_api.enter_critical_section(&runtime_resource_critical_section);
        runtime_resource_construction_api.register_resource(runtime_media_objects_parent_identity, resource);
        if((resource->type_flags & 2) != 0)
        {
            RuntimeResourceVisibilityCallbackContext context{};
            context.resource_flags = resource->type_flags;
            copy_string(context.resource_name, path);
            runtime_resource_construction_api.add_scene_callback(resource->scene_identifier, reinterpret_cast<int (*)(DisplayTraversalState *)>(update_runtime_resource_visibility), &context,
                sizeof(context), 0);
        }
        runtime_display_context.flags &= ~0x200U;
        runtime_resource_construction_api.leave_critical_section(&runtime_resource_critical_section);
    }
    return result;
}

uint32_t update_runtime_resource_visibility(DisplayTraversalState *state)
{
    auto *context = static_cast<RuntimeResourceVisibilityCallbackContext *>(state->callback_context);
    uint32_t visible = (context->resource_flags & 0x80001000) != 0x80001000;
    if(visible != 0 && (context->resource_flags & 0x2000) != 0)
    {
        const auto *rectangle = static_cast<const DisplayRectangle *>(state->data);
        visible = rectangle->left < rectangle->right && rectangle->top < rectangle->bottom;
    }
    uint32_t result = 0;
    if((state->flags & 0x1000000) != 0)
    {
        visible |= context->palette_state != (runtime_scene_control_flags & 0x8000);
        context->palette_state = runtime_scene_control_flags & 0x8000;
        result = visible == 0;
    }
    if((state->flags & 0x2000000) != 0)
    {
        visible &= (runtime_scene_control_flags & 0x8000) >> 15;
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
            context->resource_flags |= 0x80000000;
        }
        if(visible == 0 && (runtime_scene_control_flags & 0x8000) == 0)
        {
            return 1;
        }
        return 0;
    }
    return result;
}

void request_runtime_resource_destruction(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_control_api.acquire_record(identity));
    if(resource == nullptr)
    {
        return;
    }
    const uint32_t flags = resource->type_flags;
    const uint32_t type = flags & 0xff000;
    if(type == 0x1000)
    {
        runtime_resource_control_api.destroy_resource(identity);
        if((flags & 6) == 0)
        {
            --runtime_resource_count;
        }
        return;
    }
    if(type != 0x2000)
    {
        runtime_resource_control_api.destroy_resource(identity);
        return;
    }
    static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags |= 0x10000;
    runtime_resource_control_api.release_record(reinterpret_cast<RuntimeLockRecord *>(resource));
}

uint32_t query_runtime_resource_frame_limit(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_control_api.acquire_record(identity));
    if(resource == nullptr)
    {
        return 0;
    }
    const uint32_t result = resource->frame_limit;
    runtime_resource_control_api.release_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    return result;
}

void set_runtime_property_value(uint32_t value)
{
    runtime_property_value = value;
}

uint32_t get_runtime_property_value()
{
    return runtime_property_value;
}

uint16_t query_runtime_resource_frame_number(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_control_api.acquire_record(identity));
    uint16_t result = 0;
    if(resource != nullptr)
    {
        if((resource->type_flags & 0x3000) == 0x2000 && resource->backend != nullptr)
        {
            result = static_cast<RuntimeMediaBackend *>(resource->backend)->frame_number;
        }
        runtime_resource_control_api.release_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    }
    return result;
}

void select_runtime_resource(char *path)
{
    runtime_resource_selection_api.enter_critical_section(&runtime_resource_critical_section);
    if((runtime_scene_control_flags & 0x10000000) != 0)
    {
        runtime_resource_selection_api.close_archive(runtime_resource_archive);
        runtime_resource_archive = nullptr;
        runtime_scene_control_flags &= 0xefffffff;
        runtime_graphics_resource_directory[0] = '\0';
    }
    runtime_resource_selection_api.leave_critical_section(&runtime_resource_critical_section);
    if(path != nullptr)
    {
        runtime_resource_selection_api.send_message(reinterpret_cast<HWND>(graphics_host_state.message_window), 0x7ffd, 0xa0000000, reinterpret_cast<LPARAM>(path));
        runtime_resource_selection_api.construct_resource(path, 0, 0, 0, 0, 0, 0, 0x200);
    }
}

uint32_t query_runtime_resource_playback_flags(void *identity)
{
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_control_api.acquire_record(identity));
    if(resource == nullptr)
    {
        return 0;
    }
    uint32_t result = 0;
    const uint32_t type = resource->type_flags & 0xff000;
    if(type == 0x1000 || type == 0x2000)
    {
        result = static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags;
    }
    else if(type == 0x8000)
    {
        result = 0x1000000;
        RuntimeSoundStatus status{};
        if(runtime_resource_control_api.query_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource->backend)), &status) == 0)
        {
            runtime_resource_control_api.release_record(reinterpret_cast<RuntimeLockRecord *>(resource));
            return 0;
        }
        if(status.control_state != 0)
        {
            result = 0x1000001;
        }
        if(status.playback_marker != 0 || (status.schedule_marker == 0 && status.control_state == 0))
        {
            result |= 0x2000;
        }
        if(status.infinite_loop != 0)
        {
            result |= 0x400;
        }
    }
    runtime_resource_control_api.release_record(reinterpret_cast<RuntimeLockRecord *>(resource));
    return result;
}

uint32_t destroy_runtime_resource(void *identity)
{
    auto *record = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_destroy_api.acquire_record(identity));
    uint32_t result = 0;
    if(record == nullptr)
    {
        runtime_resource_destroy_api.enter_critical_section(&runtime_named_lock_critical_section);
        RuntimeGenericResourceNode *generic = runtime_resource_destroy_api.find_generic(identity);
        if(generic != nullptr)
        {
            result = 1;
            runtime_resource_destroy_api.remove_generic(identity);
        }
        runtime_resource_destroy_api.leave_critical_section(&runtime_named_lock_critical_section);
        return result;
    }

    const uint32_t type = record->type_flags & 0xff000;
    bool release_scene = true;
    if(type == 0x1000)
    {
        result = runtime_resource_destroy_api.destroy_media_backend(record->backend);
        result &= runtime_resource_destroy_api.release_memory_data(record->data);
    }
    else if(type == 0x2000)
    {
        result = runtime_resource_destroy_api.destroy_media_backend(record->backend);
        const uint32_t storage = record->backend_flags & 0x03000000;
        if(storage == 0x01000000)
        {
            result &= runtime_resource_destroy_api.release_memory_data(record->data);
        }
        else if(storage == 0x02000000)
        {
            result &= runtime_resource_destroy_api.release_stream(static_cast<AsyncFileRecord *>(record->data));
        }
    }
    else if(type == 0x8000)
    {
        runtime_resource_destroy_api.destroy_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->backend)));
        result = runtime_resource_destroy_api.release_memory_data(record->data);
    }
    else if(type == 0x10000)
    {
        result = runtime_resource_destroy_api.destroy_generic_backend(record->backend);
        result &= runtime_resource_destroy_api.release_memory_data(record->data);
    }
    else
    {
        release_scene = false;
    }
    if(release_scene)
    {
        result &= runtime_resource_destroy_api.release_scene(0, reinterpret_cast<intptr_t>(identity)) == 0;
    }
    if(current_runtime_resource == identity)
    {
        current_runtime_resource = nullptr;
    }
    runtime_resource_destroy_api.enter_critical_section(&runtime_named_lock_critical_section);
    runtime_resource_destroy_api.remove_runtime_child(runtime_named_lock_parent_identity, identity);
    result &= runtime_resource_destroy_api.heap_free(runtime_resource_destroy_api.get_process_heap(), 0, record);
    runtime_resource_destroy_api.leave_critical_section(&runtime_named_lock_critical_section);
    return result;
}

void destroy_runtime_tree_resources(void *identity)
{
    RuntimeTreeNode *root = runtime_tree_destruction_api.resolve_tree(identity);
    uint32_t count = runtime_resource_count;
    if(root == nullptr)
    {
        return;
    }
    on_scripted_save_load_tree_resources_destroyed(root);

    if(root->identity == runtime_pointer_root_identity)
    {
        runtime_tree_destruction_api.set_resource_state(current_runtime_resource, 1);
        runtime_tree_destruction_api.send_message(reinterpret_cast<HWND>(static_cast<uintptr_t>(graphics_host_state.message_window)), 0x7ffd, 0x50000000, reinterpret_cast<LPARAM>(root));
        runtime_tree_destruction_api.stop_game_dll();
        runtime_tree_destruction_api.reset_display_state();
    }
    else
    {
        runtime_tree_destruction_api.send_message(reinterpret_cast<HWND>(static_cast<uintptr_t>(graphics_host_state.message_window)), 0x7ffd, 0x50000000, reinterpret_cast<LPARAM>(root));
    }

    auto *primary_tail = static_cast<RuntimeTreePrimaryResourceLink *>(runtime_tree_destruction_api.find_primary_tail(identity));
    if(primary_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreePrimaryResourceLink *>(root->primary_resource_link_head);
        for(;;)
        {
            if(link->resource_identity != nullptr)
            {
                uint32_t flags = runtime_tree_destruction_api.query_scene_flags(link->resource_identity);
                if(flags != 0)
                {
                    if((flags & 0x3000) == 0)
                    {
                        runtime_tree_destruction_api.request_resource_destruction(link->resource_identity);
                    }
                    else
                    {
                        --count;
                        if((link->flags & 0x01000000) == 0)
                        {
                            runtime_tree_destruction_api.destroy_resource_and_scene(link->resource_identity);
                        }
                        else
                        {
                            runtime_tree_destruction_api.request_resource_destruction(link->resource_identity);
                        }
                    }
                }
                link->resource_identity = nullptr;
            }
            if(link == primary_tail)
            {
                break;
            }
            link = link->next;
        }
    }

    auto *secondary_tail = static_cast<RuntimeTreeSecondaryResourceLink *>(runtime_tree_destruction_api.find_secondary_tail(identity));
    if(secondary_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreeSecondaryResourceLink *>(root->secondary_resource_link_head);
        for(;;)
        {
            if(link->resource_identity != nullptr)
            {
                runtime_tree_destruction_api.request_resource_destruction(link->resource_identity);
                link->resource_identity = nullptr;
            }
            if(link == secondary_tail)
            {
                break;
            }
            link = link->next;
        }
    }

    auto *scene_tail = static_cast<RuntimeTreeSceneLink *>(runtime_tree_destruction_api.find_scene_tail(identity));
    if(scene_tail != nullptr)
    {
        auto *link = static_cast<RuntimeTreeSceneLink *>(root->scene_link_head);
        for(;;)
        {
            if(link->scene_identifier != 0)
            {
                runtime_tree_destruction_api.release_scene(link->scene_identifier, 0);
                link->scene_identifier = 0;
            }
            if(link == scene_tail)
            {
                break;
            }
            link = link->next;
        }
    }

    runtime_tree_destruction_api.set_comment_mode(root, 0);
    runtime_tree_destruction_api.wait_for_resource_count(count);
}

void finalize_runtime_resource_destruction(void *identity)
{
    intptr_t scene_identifier = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    RuntimeLockRecord *record = runtime_resource_scene_destruction_api.acquire_record(identity);
    if(record != nullptr)
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        uint32_t type_flags = resource->type_flags;
        uint32_t type = type_flags & 0xff000;
        if(type == 0x1000)
        {
            scene_identifier = resource->scene_identifier;
            x = resource->scene_descriptor.x;
            y = resource->scene_descriptor.y;
            width = static_cast<int32_t>(resource->output_width);
            height = static_cast<int32_t>(resource->output_height);
            runtime_resource_scene_destruction_api.destroy_resource(identity);
            if((type_flags & 6) == 0)
            {
                --runtime_resource_count;
            }
        }
        else if(type == 0x2000)
        {
            scene_identifier = resource->scene_identifier;
            x = resource->scene_descriptor.x;
            y = resource->scene_descriptor.y;
            width = static_cast<int32_t>(resource->output_width);
            height = static_cast<int32_t>(resource->output_height);
            uint32_t target_count = runtime_resource_count - 1;
            static_cast<RuntimeMediaBackend *>(resource->backend)->media_flags |= 0x10000;
            runtime_resource_scene_destruction_api.release_record(record);
            if((type_flags & 2) == 0)
            {
                while(target_count < runtime_resource_count)
                {
                    runtime_resource_scene_destruction_api.sleep(1);
                }
            }
        }
        else
        {
            runtime_resource_scene_destruction_api.destroy_resource(identity);
        }
    }
    runtime_resource_scene_destruction_api.update_scene_region(scene_identifier, x, y, width, height);
}

void update_runtime_resource_scene_region(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height)
{
    DisplayRectangle rectangle{ x, y, x + width, y + height };
    if(scene_identifier == 0)
    {
        scene_identifier = runtime_display_scene_identifier;
    }
    DisplaySceneNode *scene = runtime_resource_scene_region_api.lock_scene(scene_identifier);
    if(scene == nullptr)
    {
        return;
    }
    RuntimeLockRecord *record = runtime_resource_scene_region_api.acquire_record(reinterpret_cast<void *>(static_cast<uintptr_t>(scene->primary_owner)));
    if(record != nullptr)
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        if((resource->type_flags & 0x3000) == 0x1000 && (scene->flags & 0x20) != 0)
        {
            rectangle.left -= scene->x + resource->x;
            rectangle.top -= scene->y + resource->y;
            rectangle.right -= scene->x + resource->x;
            rectangle.bottom -= scene->y + resource->y;
            if(runtime_resource_scene_region_api.begin_scene_update(scene_identifier) == 0)
            {
                runtime_resource_scene_region_api.render_backend_region(resource->backend, &rectangle);
                const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
                runtime_resource_scene_region_api.end_scene_update(scene_identifier, &transform, &rectangle);
            }
        }
        else
        {
            rectangle.left -= scene->x;
            rectangle.top -= scene->y;
            rectangle.right -= scene->x;
            rectangle.bottom -= scene->y;
            runtime_resource_scene_region_api.update_root_scene_region(reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(scene_identifier)), &rectangle, 0);
        }
        runtime_resource_scene_region_api.release_record(record);
    }
    runtime_resource_scene_region_api.unlock_scene(scene_identifier);
}

void copy_runtime_bitmap_region(RuntimeMediaBackend *backend, DisplayRectangle *rectangle)
{
    auto *format = static_cast<BITMAPINFOHEADER *>(backend->format_data);
    int32_t bitmap_width = format->biWidth;
    int32_t source_stride = (bitmap_width + 3) & ~3;
    int32_t copy_width = rectangle->right - rectangle->left;
    int32_t copy_height = rectangle->bottom - rectangle->top;
    int32_t source_skip = bitmap_width - source_stride + bitmap_width - copy_width;
    int32_t destination_stride = backend->destination_stride;
    int32_t destination_skip = destination_stride - copy_width;
    const auto *file_header = static_cast<const BITMAPFILEHEADER *>(backend->source_data);
    uint8_t *source = static_cast<uint8_t *>(backend->source_data) + file_header->bfOffBits + rectangle->top * source_stride + rectangle->left;
    uint8_t *destination = backend->destination_pixels + (static_cast<uint32_t>(backend->destination_y) + rectangle->top) * backend->destination_stride + backend->destination_x + rectangle->left;
    if(format->biHeight >= 0)
    {
        source += source_stride * (format->biHeight - rectangle->top - rectangle->top - copy_height);
        destination += destination_stride * (copy_height - 1);
        destination_skip = -(destination_stride + copy_width);
    }
    if((backend->media_flags & 0x04000000) == 0)
    {
        do
        {
            int32_t remaining = copy_width;
            do
            {
                *destination++ = backend->palette_remap[*source++];
                --remaining;
            } while(remaining != 0);
            source += source_skip;
            destination += destination_skip;
            --copy_height;
        } while(copy_height != 0);
    }
    else
    {
        do
        {
            std::memcpy(destination, source, copy_width);
            source += copy_width + source_skip;
            destination += copy_width + destination_skip;
            --copy_height;
        } while(copy_height != 0);
    }
}

uint32_t render_runtime_bitmap_backend_region(void *identity, DisplayRectangle *rectangle)
{
    uint32_t result = 0;
    runtime_bitmap_region_render_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    try
    {
        for(RuntimeMediaBackend *backend = runtime_media_backend_head; backend != nullptr; backend = backend->next)
        {
            if(backend->identity == identity)
            {
                if(backend->type == 0xac)
                {
                    auto *format = static_cast<BITMAPINFOHEADER *>(backend->format_data);
                    int32_t height = format->biHeight < 0 ? -format->biHeight : format->biHeight;
                    if(rectangle->left < 0)
                    {
                        rectangle->left = 0;
                    }
                    if(rectangle->top < 0)
                    {
                        rectangle->top = 0;
                    }
                    if(format->biWidth < rectangle->right)
                    {
                        rectangle->right = format->biWidth;
                    }
                    if(height < rectangle->bottom)
                    {
                        rectangle->bottom = height;
                    }
                    if(rectangle->left < rectangle->right && rectangle->top < rectangle->bottom)
                    {
                        runtime_bitmap_region_render_api.copy_bitmap_region(backend, rectangle);
                        result = 1;
                    }
                }
                break;
            }
        }
    }
    catch(...)
    {
        runtime_bitmap_region_render_api.release_mutex(runtime_media_backend_mutex);
        throw;
    }
    runtime_bitmap_region_render_api.release_mutex(runtime_media_backend_mutex);
    return result;
}

void select_runtime_scene_transition(uint32_t flags)
{
    uint32_t available;
    if((flags & 0x10000000) != 0)
    {
        uint32_t depth_offset = runtime_game_host_context.bits_per_pixel - 8;
        available = (depth_offset < 1 ? 2U : 0U) + 0xffd;
    }
    else
    {
        available = graphics_host_value_3;
        if(runtime_game_host_context.bits_per_pixel != 8)
        {
            available &= 0xfffffffd;
        }
    }
    uint32_t selected = available & flags & 0xfff;
    if(selected == 0 && available != 0 && (flags & 0xfff) != 1 && (flags & 0x10000000) == 0)
    {
        selected = 1U << (runtime_scene_transition_selection_api.random() % 3);
        while((selected & available) == 0)
        {
            selected = selected == 4 ? 1 : selected * 2;
        }
    }
    switch(selected)
    {
    case 0:
    case 1:
        runtime_scene_transition_selection_api.apply_immediate(0, flags);
        break;
    case 2:
        runtime_scene_transition_selection_api.apply_palette(graphics_host_value_1, flags);
        break;
    case 4:
        runtime_scene_transition_selection_api.apply_rectangle(static_cast<uint8_t>(graphics_host_value_2), flags);
        break;
    }
}

void apply_immediate_runtime_scene_transition(uint32_t, uint32_t flags)
{
    DisplayRectangle rectangle{ 0, 0, 0, 0 };
    uint32_t type = flags & 0xff000;
    if(type == 0x1000)
    {
        if(runtime_immediate_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            runtime_immediate_scene_transition_api.set_clip_rectangle(&rectangle);
            runtime_immediate_scene_transition_api.release_display_lock();
        }
        return;
    }
    if(type != 0x2000)
    {
        return;
    }
    rectangle.right = runtime_game_host_context.width;
    rectangle.bottom = runtime_game_host_context.height;
    RuntimeLockRecord *record = runtime_immediate_scene_transition_api.acquire_record(current_runtime_resource);
    if(record == nullptr)
    {
        return;
    }
    auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
    if((resource->type_flags & 0x3000) == 0)
    {
        return;
    }
    if(runtime_immediate_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        runtime_immediate_scene_transition_api.dispatch_scene_update(&rectangle, 0x200);
        runtime_immediate_scene_transition_api.sleep(0);
        runtime_immediate_scene_transition_api.synchronize_region(&rectangle, 1);
        if((flags & 0x20000000) != 0)
        {
            auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
            runtime_immediate_scene_transition_api.apply_palette(backend->palette_entries, 0x10000);
            runtime_immediate_scene_transition_api.synchronize_region(&rectangle, 1);
        }
        runtime_immediate_scene_transition_api.set_clip_rectangle(&rectangle);
        runtime_immediate_scene_transition_api.release_display_lock();
    }
    runtime_immediate_scene_transition_api.release_record(record);
}

void apply_palette_runtime_scene_transition(uint32_t step, uint32_t flags)
{
    uint8_t palette_step = static_cast<uint8_t>(step);
    uint8_t transition_active = 0;
    DisplayRectangle rectangle{};
    PALETTEENTRY temporary_palette[0xec];
    RuntimeLockRecord *record = runtime_palette_scene_transition_api.acquire_record(current_runtime_resource);
    if(record == nullptr || (reinterpret_cast<RuntimeResourceObject *>(record)->type_flags & 0x3000) == 0)
    {
        runtime_palette_scene_transition_api.apply_immediate(0, flags);
    }
    else
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        std::memcpy(&runtime_transition_palette[0], &backend->palette_version, sizeof(PALETTEENTRY));
        std::memcpy(&runtime_transition_palette[1], backend->palette_entries, sizeof(backend->palette_entries));
        const uint32_t type = flags & 0xff000;
        if(type == 0x1000)
        {
            ++transition_active;
        }
        else if(type == 0x2000)
        {
            rectangle.right = runtime_game_host_context.width;
            rectangle.bottom = runtime_game_host_context.height;
            if(runtime_palette_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                ++transition_active;
                if((flags & 0x20000000) != 0)
                {
                    runtime_palette_scene_transition_api.apply_palette(backend->palette_entries, 0x10000);
                    runtime_palette_scene_transition_api.operate_surface(runtime_game_host_context.width >> 1, runtime_game_host_context.height >> 1, 4, 4, 1);
                }
                for(size_t index = 0; index != 0xec; ++index)
                {
                    const PALETTEENTRY source = runtime_transition_palette[index + 1];
                    temporary_palette[index].peRed = static_cast<BYTE>(source.peRed + 1);
                    temporary_palette[index].peGreen = static_cast<BYTE>(source.peGreen + 1);
                    temporary_palette[index].peBlue = static_cast<BYTE>(source.peBlue + 1);
                    temporary_palette[index].peFlags = 0xff;
                    runtime_transition_palette[index + 1] = PALETTEENTRY{ 0, 0, 0, 1 };
                }
                runtime_palette_scene_transition_api.apply_palette(runtime_transition_palette, 0);
                runtime_palette_scene_transition_api.set_clip_rectangle(&rectangle);
                runtime_palette_scene_transition_api.dispatch_scene_update(&rectangle, 0);
                runtime_palette_scene_transition_api.release_display_lock();
            }
        }
        runtime_palette_scene_transition_api.release_record(record);
    }

    if(transition_active == 0)
    {
        return;
    }
    uint32_t completed = 0;
    DWORD deadline = runtime_palette_scene_transition_api.time_get_time();
    if((runtime_scene_control_flags & 0x40000) != 0)
    {
        palette_step = 0xff;
    }
    const uint32_t type = flags & 0xff000;
    if(type == 0x2000)
    {
        while(completed < 0xec)
        {
            DWORD now = runtime_palette_scene_transition_api.time_get_time();
            if(now < deadline)
            {
                runtime_palette_scene_transition_api.sleep(0);
            }
            else
            {
                deadline = runtime_palette_scene_transition_api.time_get_time() + 2;
                uint8_t passes = palette_step;
                do
                {
                    completed = 0;
                    for(size_t reverse = 0xec; reverse != 0; --reverse)
                    {
                        PALETTEENTRY &temporary = temporary_palette[reverse - 1];
                        PALETTEENTRY &destination = runtime_transition_palette[reverse];
                        if(temporary.peFlags == 0)
                        {
                            ++completed;
                            continue;
                        }
                        auto *temporary_channels = reinterpret_cast<uint8_t *>(&temporary);
                        auto *destination_channels = reinterpret_cast<uint8_t *>(&destination);
                        for(size_t channel = 0; channel != 3; ++channel)
                        {
                            if(temporary_channels[channel] == 0)
                            {
                                ++destination_channels[channel];
                            }
                            else
                            {
                                ++temporary_channels[channel];
                            }
                        }
                        --temporary.peFlags;
                    }
                    --passes;
                } while(passes != 0);
                runtime_palette_scene_transition_api.apply_palette(runtime_transition_palette, 0);
                if((runtime_scene_control_flags & 0x40000) != 0)
                {
                    runtime_palette_scene_transition_api.invalidate_framebuffer(0, 0, runtime_game_host_context.width, runtime_game_host_context.height);
                }
            }
        }
        return;
    }
    if(type != 0x1000)
    {
        return;
    }
    while(completed < 0x2c4)
    {
        DWORD now = runtime_palette_scene_transition_api.time_get_time();
        if(now < deadline)
        {
            runtime_palette_scene_transition_api.sleep(0);
        }
        else
        {
            deadline = runtime_palette_scene_transition_api.time_get_time() + 2;
            completed = 0;
            for(size_t reverse = 0xec; reverse != 0; --reverse)
            {
                auto *channels = reinterpret_cast<uint8_t *>(&runtime_transition_palette[reverse]);
                for(size_t channel = 0; channel != 3; ++channel)
                {
                    if(channels[channel] > palette_step)
                    {
                        channels[channel] = static_cast<uint8_t>(channels[channel] - palette_step);
                    }
                    else
                    {
                        channels[channel] = 0;
                        ++completed;
                    }
                }
            }
            runtime_palette_scene_transition_api.apply_palette(runtime_transition_palette, 0);
            if((runtime_scene_control_flags & 0x40000) != 0)
            {
                runtime_palette_scene_transition_api.invalidate_framebuffer(0, 0, runtime_game_host_context.width, runtime_game_host_context.height);
            }
        }
    }
    if(runtime_palette_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        runtime_palette_scene_transition_api.set_clip_rectangle(&rectangle);
        runtime_palette_scene_transition_api.operate_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
        runtime_palette_scene_transition_api.release_display_lock();
    }
}

void apply_rectangle_runtime_scene_transition(uint8_t size, uint32_t flags)
{
    RuntimeLockRecord *record = runtime_rectangle_scene_transition_api.acquire_record(current_runtime_resource);
    if(record == nullptr || (reinterpret_cast<RuntimeResourceObject *>(record)->type_flags & 0x3000) == 0)
    {
        runtime_rectangle_scene_transition_api.apply_immediate(0, flags);
        return;
    }

    const uint32_t width = runtime_game_host_context.width;
    const uint32_t height = runtime_game_host_context.height;
    uint32_t horizontal_step;
    uint32_t vertical_step;
    if(size == 0xff)
    {
        horizontal_step = width;
        vertical_step = height;
    }
    else
    {
        horizontal_step = (size & 0xfcU) + 4;
        vertical_step = (horizontal_step * 15) / 20;
    }

    const uint32_t type = flags & 0xff000;
    if(type == 0x1000)
    {
        DisplayRectangle clip{ 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) };
        if(runtime_rectangle_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            runtime_rectangle_scene_transition_api.set_clip_rectangle(&clip);
            runtime_rectangle_scene_transition_api.release_display_lock();
        }
        DWORD deadline = runtime_rectangle_scene_transition_api.time_get_time();
        while((clip.bottom - clip.top) > static_cast<int32_t>(vertical_step * 2) || (clip.right - clip.left) > static_cast<int32_t>(horizontal_step * 2))
        {
            DWORD now = runtime_rectangle_scene_transition_api.time_get_time();
            if(now < deadline)
            {
                runtime_rectangle_scene_transition_api.sleep(0);
                continue;
            }
            deadline = runtime_rectangle_scene_transition_api.time_get_time() + 2;

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
            if(runtime_rectangle_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                runtime_rectangle_scene_transition_api.set_clip_rectangle(&clip);
                for(const DisplayRectangle &strip : strips)
                {
                    runtime_rectangle_scene_transition_api.operate_surface(strip.left, strip.top, strip.right, strip.bottom, 2);
                }
                runtime_rectangle_scene_transition_api.release_display_lock();
            }
        }
        if(runtime_rectangle_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            runtime_rectangle_scene_transition_api.synchronize_region(&clip, 2);
            clip.left = clip.right;
            clip.top = clip.bottom;
            runtime_rectangle_scene_transition_api.set_clip_rectangle(&clip);
            runtime_rectangle_scene_transition_api.release_display_lock();
        }
    }
    else if(type == 0x2000)
    {
        DisplayRectangle clip;
        if(size == 0xff)
        {
            clip = { 0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height) };
        }
        else
        {
            clip = { static_cast<int32_t>((width >> 1) - horizontal_step), static_cast<int32_t>((height >> 1) - vertical_step), static_cast<int32_t>((width >> 1) + horizontal_step),
                static_cast<int32_t>((height >> 1) + vertical_step) };
        }
        if(runtime_rectangle_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
        {
            if((flags & 0x20000000) != 0)
            {
                auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
                auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
                runtime_rectangle_scene_transition_api.apply_palette(backend->palette_entries, 0x10000);
            }
            runtime_rectangle_scene_transition_api.dispatch_scene_update(&clip, 0);
            runtime_rectangle_scene_transition_api.set_clip_rectangle(&clip);
            runtime_rectangle_scene_transition_api.release_display_lock();
        }
        DWORD deadline = runtime_rectangle_scene_transition_api.get_tick_count();
        while((clip.right - clip.left) < static_cast<int32_t>(width) || (clip.bottom - clip.top) < static_cast<int32_t>(height))
        {
            DWORD now = runtime_rectangle_scene_transition_api.get_tick_count();
            if(now < deadline)
            {
                runtime_rectangle_scene_transition_api.sleep(0);
                continue;
            }
            deadline = runtime_rectangle_scene_transition_api.get_tick_count() + 2;
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
            if(runtime_rectangle_scene_transition_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
            {
                runtime_rectangle_scene_transition_api.set_clip_rectangle(&clip);
                for(DisplayRectangle &strip : strips)
                {
                    runtime_rectangle_scene_transition_api.dispatch_scene_update(&strip, 0);
                }
                runtime_rectangle_scene_transition_api.release_display_lock();
            }
        }
    }
    runtime_rectangle_scene_transition_api.release_record(record);
}

void set_runtime_resource_state(void *identity, uint32_t state)
{
    RuntimeLockRecord *record = runtime_resource_state_api.acquire_record(identity);
    if(record == nullptr)
    {
        if(current_runtime_resource == identity && state == 1)
        {
            runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x1000);
        }
        return;
    }

    auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
    uint32_t type = resource->type_flags & 0xff000;
    if(type == 0x1000)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        bool force_refresh = (state & 0x20000) != 0;
        if(force_refresh || (backend->media_flags & 0x20) != 0)
        {
            backend->media_flags |= 0x20;
            DisplayRectangle rectangle{ 0, 0, static_cast<int32_t>(resource->output_width), static_cast<int32_t>(resource->output_height) };
            runtime_resource_state_api.begin_scene_update(resource->scene_identifier);
            runtime_resource_state_api.finalize_backend(backend);
            runtime_resource_state_api.configure_palette(resource);
            const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
            runtime_resource_state_api.end_scene_update(resource->scene_identifier, &transform, &rectangle);
            if(resource->generic_backend_child != nullptr)
            {
                runtime_resource_state_api.clear_child_ready(resource->generic_backend_child);
            }
        }
        if(resource->generic_backend_child != nullptr)
        {
            runtime_resource_state_api.disable_child_mode(resource->generic_backend_child);
        }
        if(current_runtime_resource == identity)
        {
            if(state == 1)
            {
                runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x1000);
            }
            else if(!force_refresh)
            {
                runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x20002000);
            }
        }
    }
    else if(type == 0x2000)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
        uint32_t transition_flag = backend->frame_number == 1 ? 0x20000000 : 0;
        backend->media_flags = (backend->media_flags & (state ^ 0xfffffdfe)) | state;
        if(current_runtime_resource == identity)
        {
            if(state == 0)
            {
                runtime_resource_state_api.select_transition(transition_flag | runtime_resource_transition_flags | 0x2000);
            }
            if(state == 1)
            {
                runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x1000);
            }
        }
    }
    else if(type == 0x8000)
    {
        bool force_refresh = (state & 0x20000) != 0;
        uint32_t handle = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource->backend));
        if(force_refresh)
        {
            runtime_resource_state_api.restart_sound_data(handle);
            if(resource->generic_backend_child != nullptr)
            {
                runtime_resource_state_api.clear_child_ready(resource->generic_backend_child);
            }
        }
        if((state & 1) != 0)
        {
            runtime_resource_state_api.start_sound(handle, 1);
            if(resource->generic_backend_child != nullptr)
            {
                runtime_resource_state_api.enable_child_mode(resource->generic_backend_child);
            }
        }
        if(state == 0)
        {
            runtime_resource_state_api.stop_sound(handle, 1);
            if(resource->generic_backend_child != nullptr)
            {
                runtime_resource_state_api.disable_child_mode(resource->generic_backend_child);
            }
        }
        if(current_runtime_resource == identity)
        {
            if(state == 1)
            {
                runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x1000);
            }
            else if(!force_refresh)
            {
                runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x20002000);
            }
        }
    }
    else if(current_runtime_resource == identity)
    {
        if(state == 1)
        {
            runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x1000);
        }
        else if((state & 0x20000) == 0)
        {
            runtime_resource_state_api.select_transition(runtime_resource_transition_flags | 0x20002000);
        }
    }
    runtime_resource_state_api.release_record(record);
}

void release_runtime_lock_record(RuntimeLockRecord *record)
{
    if(record != nullptr && record->recursion_count != 0)
    {
        --record->recursion_count;
    }
}

RuntimeLockRecord *acquire_runtime_lock_record(void *child_identity)
{
    DWORD thread_id = runtime_named_lock_api.get_current_thread_id();
    while(true)
    {
        RuntimeLockRecord *record = nullptr;
        bool contended = false;
        runtime_named_lock_api.enter_critical_section(&runtime_named_lock_critical_section);
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
        runtime_named_lock_api.leave_critical_section(&runtime_named_lock_critical_section);
        if(!contended)
        {
            return record;
        }
        runtime_named_lock_api.sleep(5);
    }
}

void reset_runtime_session()
{
    runtime_session_reset_api.stop_game_dll();
    RuntimeTreeNode *tree = runtime_session_reset_api.get_tree_root();
    while(tree != nullptr)
    {
        runtime_session_reset_api.destroy_tree_resources(tree);
        runtime_session_reset_api.deactivate_tree(tree, nullptr);
        tree = runtime_session_reset_api.get_tree_root();
    }

    runtime_session_reset_api.reset_display_state();
    for(RuntimeVisualObject *visual = script_runtime_root->visual_objects; visual != nullptr; visual = visual->next)
    {
        if(visual->scene_identity != nullptr)
        {
            runtime_session_reset_api.request_resource_destruction(visual->scene_identity);
        }
    }
    for(RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes; node != nullptr; node = node->next)
    {
        if(node->resource_identity != nullptr)
        {
            runtime_session_reset_api.request_resource_destruction(node->resource_identity);
        }
    }
    runtime_session_reset_api.destroy_fixed_name_nodes();
    runtime_session_reset_api.purge_named_nodes();
    runtime_session_reset_api.destroy_object_states();
    runtime_session_reset_api.destroy_visual_objects();
    runtime_session_reset_api.clear_command_definitions();
    runtime_session_reset_api.remove_generic_resources();

    if((runtime_display_context.flags & 0x10000000) != 0)
    {
        runtime_session_reset_api.close_archive(runtime_display_context.active_archive);
    }
    runtime_display_context.active_archive = nullptr;
    runtime_display_context.flags &= 0xefffffff;
    runtime_session_reset_api.destroy_async_host(runtime_display_context.async_file_host);
    runtime_display_context.async_file_host = nullptr;
    runtime_session_reset_api.operate_surface(0, 0, runtime_display_context.width, runtime_display_context.height, 2);

    RuntimeNamedNode *media_objects = runtime_session_reset_api.get_named_node("MMediaObjectsList");
    RuntimeNamedNode *open_memory_files = runtime_session_reset_api.get_named_node("OpenMemoryFilesList");
    DWORD start = runtime_session_reset_api.get_time();
    while(media_objects->status != 0)
    {
        DWORD current = runtime_session_reset_api.get_time();
        if(current < start + 5000)
        {
            break;
        }
        runtime_session_reset_api.sleep(10);
    }
    start = runtime_session_reset_api.get_time();
    while(open_memory_files->status != 0)
    {
        DWORD current = runtime_session_reset_api.get_time();
        if(current < start + 5000)
        {
            break;
        }
        runtime_session_reset_api.sleep(10);
    }
    if(media_objects->status == 0 && open_memory_files->status == 0)
    {
        runtime_display_context.flags |= 0x200;
    }

    graphics_script_runtime_root.flags = 0;
    graphics_script_runtime_root.palette_flags = 0;
    std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
    runtime_display_context.reset_value_1 = 6;
    runtime_display_context.reset_value_2 = 5;
    runtime_display_context.reset_value_3 = 5;
    runtime_resource_host_mode = 0x6a4;
}

void switch_runtime_scene(void *identity)
{
    if((graphics_host_flags & 0x1000) != 0)
    {
        deferred_runtime_scene_identity = identity;
        return;
    }
    void *selected_identity = nullptr;
    auto *previous = reinterpret_cast<RuntimeResourceObject *>(runtime_scene_switch_api.acquire(current_runtime_scene_identity));
    if(previous != nullptr)
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(previous->backend);
        backend->media_flags |= 1;
        runtime_scene_switch_api.offset_scene(previous->scene_identifier, 10000 - previous->x, 10000 - previous->y);
        previous->x = 10000;
        previous->y = 10000;
    }
    auto *selected = reinterpret_cast<RuntimeResourceObject *>(runtime_scene_switch_api.acquire(identity));
    if(selected != nullptr)
    {
        uint32_t mode = selected->type_flags & 0xff000;
        if(mode == 0x1000 || mode == 0x2000)
        {
            if(mode == 0x2000)
            {
                auto *backend = static_cast<RuntimeMediaBackend *>(selected->backend);
                backend->media_flags &= 0xfffffdfe;
            }
            int32_t new_x = runtime_scene_x - static_cast<int32_t>(selected->requested_width);
            int32_t new_y = runtime_scene_y - static_cast<int32_t>(selected->requested_height);
            runtime_scene_switch_api.offset_scene(selected->scene_identifier, new_x - selected->x, new_y - selected->y);
            selected->x = new_x;
            selected->y = new_y;
            selected_identity = identity;
        }
    }
    current_runtime_scene_identity = selected_identity;
    if(selected != nullptr)
    {
        runtime_scene_switch_api.release(reinterpret_cast<RuntimeLockRecord *>(selected));
    }
    if(previous != nullptr)
    {
        runtime_scene_switch_api.release(reinterpret_cast<RuntimeLockRecord *>(previous));
    }
}

void reset_runtime_display_state()
{
    runtime_display_reset_api.switch_scene(nullptr);
    graphics_host_flags &= 0xff7c3e43;
    runtime_display_reset_api.set_script_flags(2, 0);
    runtime_display_reset_api.set_script_flags(4, 0);
    runtime_display_reset_api.reset_transient_indices();
    runtime_display_reset_api.reset_byte_queue();
    runtime_display_reset_api.reset_pair_queue();
    runtime_display_reset_api.release_scene(0, reinterpret_cast<intptr_t>(&runtime_display_context));
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
    if((graphics_host_flags & 0x600) == 0x600)
    {
        begin_sdl_presenter_shutdown();
        auto *media_objects = static_cast<RuntimeNamedNode *>(runtime_display_shutdown_api.get_named_node("MMediaObjectsList"));
        auto *open_memory_files = static_cast<RuntimeNamedNode *>(runtime_display_shutdown_api.get_named_node("OpenMemoryFilesList"));
        if(open_memory_files->status == 0 && media_objects->status == 0)
        {
            graphics_host_flags |= 1;
            runtime_display_shutdown_api.wait_for_single_object(runtime_display_thread, INFINITE);
            result = runtime_display_shutdown_api.close_handle(runtime_display_thread);
        }
        uint32_t cleaned = 0;
        if(result != 0)
        {
            cleaned = runtime_display_shutdown_api.release_scene(runtime_display_scene_identifier, 0) == 0;
            cleaned &= runtime_display_shutdown_api.shutdown_host() == 0;
            runtime_display_shutdown_api.teardown_surface();
        }
        result = 0;
        if(cleaned != 0)
        {
            runtime_display_context.display_pixel_format = {};
            runtime_display_scene_identifier = 0;
            runtime_display_host = nullptr;
            runtime_display_thread = nullptr;
            graphics_host_flags &= 0xfffff9ff;
            return cleaned;
        }
    }
    else if((graphics_host_flags & 0x400) == 0)
    {
        result = 1;
    }
    return result;
}

void set_runtime_resource_loop_count(void *identity, uint32_t count)
{
    auto *record = reinterpret_cast<RuntimeResourceObject *>(runtime_resource_loop_api.acquire_record(identity));
    if(record == nullptr)
    {
        return;
    }
    if((record->type_flags & 0xff000) == 0x8000)
    {
        runtime_resource_loop_api.set_sound_loop(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->backend)), count);
    }
    else
    {
        auto *backend = static_cast<RuntimeMediaBackend *>(record->backend);
        backend->media_flags |= 0x400;
        record->frame_limit = count - 1;
        record->frames_remaining = count - 1;
    }
    runtime_resource_loop_api.release_record(reinterpret_cast<RuntimeLockRecord *>(record));
}


uint32_t query_runtime_scene_flags(void *identity)
{
    RuntimeLockRecord *record = acquire_runtime_lock_record(identity);
    if(record == nullptr)
    {
        return 0;
    }
    uint32_t flags = reinterpret_cast<RuntimeResourceObject *>(record)->type_flags;
    release_runtime_lock_record(record);
    return flags;
}

void wait_for_runtime_resource_count(uint32_t count)
{
    while(runtime_resource_count != count)
    {
        runtime_resource_wait_api.sleep(0);
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
        runtime_scene_switch_api.offset_scene(record->scene_identifier, x - record->previous_x, y - record->previous_y);
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
    runtime_resource_host_api.enter_critical_section(&runtime_resource_critical_section);
    if(runtime_resource_host != nullptr)
    {
        if(reset != 0)
        {
            bool destroy_host = true;
            if(path != nullptr)
            {
                copy_directory_from_path(directory, path);
                if(directory[0] == '\0' || strings_equal(directory, runtime_display_context.resource_directory))
                {
                    destroy_host = false;
                }
            }
            if(destroy_host)
            {
                runtime_resource_host_api.destroy_host(runtime_resource_host);
                runtime_resource_host = nullptr;
            }
            if((runtime_scene_control_flags & 0x10000000) != 0)
            {
                runtime_resource_host_api.close_archive(runtime_resource_archive);
                runtime_scene_control_flags &= ~0x10000000U;
                runtime_resource_archive = nullptr;
                runtime_resource_archive_state = 0;
            }
        }
    }
    if(runtime_resource_host == nullptr && path != nullptr)
    {
        copy_directory_from_path(directory, path);
        if(directory[0] != '\0')
        {
            copy_string(runtime_display_context.resource_directory, directory);
        }
        const char *root = extract_runtime_drive_prefix(drive_prefix, runtime_display_context.resource_directory) == 1 ? drive_prefix : nullptr;
        runtime_resource_host = runtime_resource_host_api.create_host(root, 0x100000, runtime_resource_host_mode);
    }
    else
    {
        runtime_resource_host_api.set_host_mode(runtime_resource_host, runtime_resource_host_mode);
    }
    runtime_resource_host_api.leave_critical_section(&runtime_resource_critical_section);
}

uint32_t detect_runtime_resource_type(const char *path)
{
    static constexpr uint8_t configuration_signature[5]{ '[', 'C', 'F', 'G', ']' };
    static constexpr uint8_t wave_signature[8]{ 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ' };
    static constexpr uint8_t cdf_signature[6]{ 'C', 'D', 'F', '9', '6', 'a' };
    uint32_t type = 0;
    LRESULT retry;
    do
    {
        retry = 0;
        runtime_resource_type_api.enter_critical_section(&runtime_resource_critical_section);
        RuntimeResourceCacheEntry *entry = runtime_resource_type_api.find_cache_entry(runtime_resource_cache_parent_identity, path);
        if(entry != nullptr)
        {
            type = entry->flags_and_references >> 16;
        }
        else if(VirtualScriptResource virtual_script{}; find_virtual_runtime_script(path, &virtual_script))
        {
            // Fixes-owned virtual content must be classified before archive/file probing. Resource construction dispatches configuration parsing from this result and only loads its bytes afterward.
            type = virtual_script.resource_type;
        }
        else if((runtime_scene_control_flags & 0x10000000) == 0)
        {
            runtime_resource_type_api.update_host(path, 0);
            char full_path[128];
            build_runtime_resource_path(full_path, path);
            HANDLE file = runtime_resource_type_api.open_file(full_path);
            if(file == nullptr)
            {
                retry = runtime_resource_type_api.send_message(runtime_resource_notification_window, 0x7ffd, 0x03000000, reinterpret_cast<LPARAM>(full_path));
            }
            else
            {
                uint8_t header[16];
                DWORD bytes_read = 0;
                runtime_resource_type_api.read_file(file, header, sizeof(header), &bytes_read, nullptr);
                runtime_resource_type_api.close_handle(file);
                if(bytes_read == sizeof(header))
                {
                    int16_t animation_marker;
                    std::memcpy(&animation_marker, header + 4, sizeof(animation_marker));
                    if(animation_marker == static_cast<int16_t>(0xaf12))
                    {
                        type = 3;
                    }
                    uint16_t bitmap_marker;
                    std::memcpy(&bitmap_marker, header, sizeof(bitmap_marker));
                    if(bitmap_marker == 0x4d42)
                    {
                        type = 1;
                    }
                    if(fixed_dword_memory_equal(header, configuration_signature, sizeof(configuration_signature)))
                    {
                        type = 4;
                    }
                    if(fixed_dword_memory_equal(header + 8, wave_signature, sizeof(wave_signature)))
                    {
                        type = 2;
                    }
                    if(fixed_dword_memory_equal(header, cdf_signature, sizeof(cdf_signature)))
                    {
                        type = 5;
                    }
                }
            }
        }
        else if((runtime_scene_control_flags & 0x10000000) == 0x10000000)
        {
            type = runtime_resource_type_api.get_archive_flags(runtime_resource_archive, path) & ~0x10U;
            if(type == 0)
            {
                type = get_synthesized_resource_type(path);
            }
        }
        runtime_resource_type_api.leave_critical_section(&runtime_resource_critical_section);
    } while(retry != 0);
    return type;
}

void *open_runtime_cdf_entry_stream(CdfArchive *archive, const char *name)
{
    if(archive == nullptr)
    {
        return nullptr;
    }
    for(uint32_t index = 0; index < archive->entry_count; ++index)
    {
        CdfEntry *entry = archive->entries[index];
        if(runtime_cdf_stream_api.compare_names(entry->name, name) == 0)
        {
            if(archive->alternate_stream != 0)
            {
                return runtime_cdf_stream_api.duplicate_record(static_cast<AsyncFileRecord *>(archive->second_handle), entry->file_offset, entry->file_offset + entry->uncompressed_size, 0);
            }
            HANDLE file = runtime_cdf_stream_api.create_file(archive->path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            runtime_cdf_stream_api.set_file_pointer(file, entry->file_offset, nullptr, FILE_BEGIN);
            return file;
        }
    }
    return nullptr;
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
        LRESULT retry = 0;
        char name[128];
        char full_path[128];
        copy_file_name_from_path(name, path);
        runtime_resource_load_api.enter_critical_section(&runtime_resource_critical_section);
        RuntimeResourceCacheEntry *entry = runtime_resource_load_api.find_cache_entry(runtime_resource_cache_parent_identity, name);
        if(entry != nullptr)
        {
            resource_size = entry->size;
            resource_data = entry->data;
            resource_storage = 0x01000000;
            ++entry->flags_and_references;
        }
        else if(VirtualScriptResource virtual_script{}; find_virtual_runtime_script(name, &virtual_script))
        {
            resource_size = virtual_script.size;
            resource_data = runtime_resource_load_api.heap_alloc(runtime_resource_heap, HEAP_ZERO_MEMORY, resource_size + 1);
            if(resource_data != nullptr)
            {
                std::memcpy(resource_data, virtual_script.data, resource_size);
                resource_storage = 0x01000000;
                RuntimeResourceCacheEntry *new_entry = runtime_resource_load_api.get_or_create_cache_entry(runtime_resource_cache_parent_identity, name);
                if(new_entry != nullptr)
                {
                    new_entry->size = resource_size;
                    new_entry->data = resource_data;
                    new_entry->flags_and_references = (4u << 16) | 1;
                }
                goto resource_loaded;
            }
        }
        else if((runtime_scene_control_flags & 0x10000000) == 0)
        {
            build_runtime_resource_path(full_path, path);
            AsyncFileRecord *record = runtime_resource_load_api.open_async_record(runtime_resource_host, full_path, 0, 0, 0);
            if(record == nullptr)
            {
                retry = runtime_resource_load_api.send_message(runtime_resource_notification_window, 0x7ffd, 0x03000000, reinterpret_cast<LPARAM>(full_path));
            }
            else
            {
                resource_size = runtime_resource_load_api.get_async_size(record);
                if(resource_size < 0x100000 || (flags & 0x20000000) != 0)
                {
                    runtime_resource_load_api.activate_loading_scene(loading_scene);
                    resource_data = runtime_resource_load_api.heap_alloc(runtime_resource_heap, 0, resource_size);
                    if(resource_data != nullptr)
                    {
                        uint32_t resource_type = 0;
                        if(runtime_resource_load_api.read_async_record(record, resource_data, resource_size, &resource_type, 0) == 0)
                        {
                            resource_size = 0;
                            void *failed_data = resource_data;
                            resource_data = nullptr;
                            runtime_resource_load_api.heap_free(runtime_resource_heap, 0, failed_data);
                            retry = runtime_resource_load_api.send_message(runtime_resource_notification_window, 0x7ffd, 0xc0000000, reinterpret_cast<LPARAM>(full_path));
                        }
                        else
                        {
                            resource_storage = 0x01000000;
                        }
                        if(resource_data != nullptr)
                        {
                            RuntimeResourceCacheEntry *new_entry = nullptr;
                            runtime_resource_load_api.deactivate_loading_scene(loading_scene);
                            runtime_resource_load_api.reset_byte_queue();
                            runtime_resource_load_api.reset_pair_queue();
                            new_entry = runtime_resource_load_api.get_or_create_cache_entry(runtime_resource_cache_parent_identity, name);
                            if(new_entry != nullptr)
                            {
                                new_entry->size = resource_size;
                                new_entry->data = resource_data;
                                const auto *bytes = static_cast<const uint8_t *>(resource_data);
                                int16_t animation_marker;
                                std::memcpy(&animation_marker, bytes + 4, sizeof(animation_marker));
                                if(animation_marker == static_cast<int16_t>(0xaf12))
                                {
                                    resource_type = 3;
                                }
                                uint16_t bitmap_marker;
                                std::memcpy(&bitmap_marker, bytes, sizeof(bitmap_marker));
                                if(bitmap_marker == 0x4d42)
                                {
                                    resource_type = 1;
                                }
                                if(fixed_dword_memory_equal(bytes, configuration_signature, sizeof(configuration_signature)))
                                {
                                    resource_type = 4;
                                }
                                if(fixed_dword_memory_equal(bytes + 8, wave_signature, sizeof(wave_signature)))
                                {
                                    resource_type = 2;
                                }
                                if(fixed_dword_memory_equal(bytes, cdf_signature, sizeof(cdf_signature)))
                                {
                                    resource_type = 5;
                                }
                                new_entry->flags_and_references = (resource_type << 16) | 1;
                            }
                            goto resource_loaded;
                        }
                    }
                    runtime_resource_load_api.deactivate_loading_scene(loading_scene);
                    runtime_resource_load_api.reset_byte_queue();
                    runtime_resource_load_api.reset_pair_queue();
                }
                else
                {
                    resource_storage = 0x02000000;
                    resource_data = record;
                }
            }
        }
        else if((runtime_scene_control_flags & 0x10000000) == 0x10000000)
        {
            const uint8_t selector = runtime_resource_load_api.get_archive_flags(runtime_resource_archive, path);
            const uint32_t archive_size = runtime_resource_load_api.get_archive_size(runtime_resource_archive, selector, path);
            if((selector & 0x10) == 0 && (flags & 0x20000000) == 0)
            {
                resource_size = archive_size;
                if(resource_size != 0)
                {
                    resource_data = runtime_resource_load_api.open_archive_stream(runtime_resource_archive, path);
                    resource_storage = 0x02000000;
                }
            }
            else
            {
                runtime_resource_load_api.activate_loading_scene(loading_scene);
                resource_size = archive_size;
                if(resource_size != 0)
                {
                    resource_data = runtime_resource_load_api.heap_alloc(runtime_resource_heap, 0, resource_size);
                    if(resource_data == nullptr)
                    {
                        resource_size = 0;
                    }
                    else if(runtime_resource_load_api.read_archive_entry(runtime_resource_archive, selector, path, resource_data) == 0)
                    {
                        resource_size = 0;
                        runtime_resource_load_api.heap_free(runtime_resource_heap, 0, resource_data);
                        retry = runtime_resource_load_api.send_message(runtime_resource_notification_window, 0x7ffd, 0xc0000000, reinterpret_cast<LPARAM>(path));
                        resource_data = nullptr;
                    }
                    else
                    {
                        resource_storage = 0x01000000;
                    }
                }
                runtime_resource_load_api.deactivate_loading_scene(loading_scene);
                runtime_resource_load_api.reset_byte_queue();
                runtime_resource_load_api.reset_pair_queue();
                if(resource_data != nullptr)
                {
                    RuntimeResourceCacheEntry *new_entry = runtime_resource_load_api.get_or_create_cache_entry(runtime_resource_cache_parent_identity, name);
                    if(new_entry != nullptr)
                    {
                        new_entry->size = resource_size;
                        new_entry->data = resource_data;
                        new_entry->flags_and_references = (static_cast<uint32_t>(selector & ~0x10U) << 16) | 1;
                    }
                }
            }
            if(archive_size == 0)
            {
                const SynthesizedResourceSourceApi source_api{ runtime_resource_load_api.get_archive_size, runtime_resource_load_api.read_archive_entry };
                SynthesizedResource synthesized;
                if(synthesize_resource(runtime_resource_archive, path, source_api, &synthesized))
                {
                    resource_size = static_cast<uint32_t>(synthesized.data.size());
                    resource_data = runtime_resource_load_api.heap_alloc(runtime_resource_heap, 0, resource_size);
                    if(resource_data != nullptr)
                    {
                        std::memcpy(resource_data, synthesized.data.data(), resource_size);
                        resource_storage = 0x01000000;
                        RuntimeResourceCacheEntry *new_entry = runtime_resource_load_api.get_or_create_cache_entry(runtime_resource_cache_parent_identity, name);
                        if(new_entry != nullptr)
                        {
                            new_entry->size = resource_size;
                            new_entry->data = resource_data;
                            new_entry->flags_and_references = (synthesized.resource_type << 16) | 1;
                        }
                    }
                }
            }
        }
resource_loaded:
        if(retry == 0 && resource_data != nullptr && resource_storage == 0x02000000)
        {
            if(runtime_resource_streamed_count == 0)
            {
                runtime_resource_load_api.set_script_flags(0x10, 1);
            }
            ++runtime_resource_streamed_count;
        }
        runtime_resource_load_api.leave_critical_section(&runtime_resource_critical_section);
        if(retry == 0)
        {
            *data = resource_data;
            *storage = resource_storage;
            *size = resource_size;
            return;
        }
        runtime_resource_load_api.sleep(5);
    }
}

BOOL release_runtime_memory_resource(const char *name)
{
    BOOL result = FALSE;
    runtime_resource_release_api.enter_critical_section(&runtime_resource_critical_section);
    RuntimeResourceCacheEntry *entry = runtime_resource_release_api.find_cache_entry(runtime_resource_cache_parent_identity, name);
    if(entry != nullptr)
    {
        --entry->flags_and_references;
        if((entry->flags_and_references & 0xffff) == 0)
        {
            result = runtime_resource_release_api.heap_free(runtime_resource_heap, 0, entry->data);
            runtime_resource_release_api.remove_cache_entry(runtime_resource_cache_parent_identity, entry->data);
        }
    }
    runtime_resource_release_api.leave_critical_section(&runtime_resource_critical_section);
    return result;
}

BOOL release_runtime_memory_resource_by_data(void *data)
{
    BOOL result = FALSE;
    runtime_resource_release_api.enter_critical_section(&runtime_resource_critical_section);
    auto *entry = reinterpret_cast<RuntimeResourceCacheEntry *>(runtime_resource_release_api.find_child(runtime_resource_cache_parent_identity, data));
    if(entry != nullptr)
    {
        --entry->flags_and_references;
        if((entry->flags_and_references & 0xffff) == 0)
        {
            result = runtime_resource_release_api.heap_free(runtime_resource_heap, 0, entry->data);
            runtime_resource_release_api.remove_cache_entry(runtime_resource_cache_parent_identity, entry->data);
        }
    }
    runtime_resource_release_api.leave_critical_section(&runtime_resource_critical_section);
    return result;
}

uint32_t release_runtime_streamed_resource(AsyncFileRecord *record)
{
    runtime_resource_release_api.enter_critical_section(&runtime_resource_critical_section);
    uint32_t result = runtime_resource_release_api.close_async_record(record);
    if(result != 0 && runtime_resource_streamed_count != 0)
    {
        --runtime_resource_streamed_count;
        if(runtime_resource_streamed_count == 0)
        {
            runtime_resource_release_api.set_script_flags(0x10, 0);
        }
    }
    runtime_resource_release_api.leave_critical_section(&runtime_resource_critical_section);
    return result;
}

uint32_t extract_runtime_drive_prefix(char *destination, const char *source)
{
    uint32_t index = 0;
    do
    {
        char value = source[index];
        if(value == '\0' || value == ':' || value == '\\')
        {
            break;
        }
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

HANDLE open_runtime_resource_file(const char *path)
{
    OSVERSIONINFOA version{};
    version.dwOSVersionInfoSize = sizeof(version);
    runtime_resource_file_open_api.get_version(&version);
    DWORD flags = 0x08000000 + (version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ? 0x20000000 : 0);
    HANDLE file = runtime_resource_file_open_api.create_file(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, flags, nullptr);
    return file == INVALID_HANDLE_VALUE ? nullptr : file;
}

void advance_async_host_read(AsyncFileHost *host, uint32_t bytes)
{
    const uint32_t sector_size = host->bytes_per_sector;
    const uint32_t aligned = (static_cast<uint32_t>(static_cast<uint8_t *>(host->secondary_cursor) - static_cast<uint8_t *>(host->buffer)) % sector_size + bytes);
    host->available_bytes += aligned - aligned % sector_size;
    host->secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor) + bytes;
    host->current_offset += bytes;
    if(static_cast<uint8_t *>(host->buffer) + host->buffer_size <= host->secondary_cursor)
    {
        host->secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor) - host->buffer_size;
    }
}

void advance_async_host_write(AsyncFileHost *host, uint32_t bytes)
{
    host->buffered_bytes += bytes;
    host->file_offset += bytes;
    host->write_cursor = static_cast<uint8_t *>(host->write_cursor) + bytes;
    host->available_bytes -= bytes;
    if(static_cast<uint8_t *>(host->buffer) + host->buffer_size <= host->write_cursor)
    {
        host->write_cursor = host->buffer;
    }
}

void invalidate_shared_async_records(AsyncFileRecord *record)
{
    HANDLE file = record->file;
    async_file_lock_api.enter_critical_section(&async_file_global_lock);
    AsyncFileHost *host = record->host;
    async_file_lock_api.enter_critical_section(&host->secondary_lock);
    for(AsyncFileRecord *sibling = host->files; sibling != nullptr; sibling = sibling->next)
    {
        if(sibling != record && sibling->file == file)
        {
            sibling->flags &= ~0x20U;
            if(host->active_file == sibling)
            {
                async_file_lock_api.enter_critical_section(&host->primary_lock);
                host->active_file = nullptr;
                async_file_lock_api.leave_critical_section(&host->primary_lock);
            }
        }
    }
    async_file_lock_api.leave_critical_section(&host->secondary_lock);
    async_file_lock_api.leave_critical_section(&async_file_global_lock);
}

void position_async_host(AsyncFileHost *host, uint32_t offset)
{
    AsyncFileRecord *record = host->active_file;
    if((record->flags & 0x20) == 0)
    {
        const uint32_t sector_size = host->bytes_per_sector;
        host->file_offset = offset / sector_size * sector_size;
        host->write_cursor = host->buffer;
        host->secondary_cursor = static_cast<uint8_t *>(host->buffer) + offset % sector_size;
        host->buffer_start_cursor = host->secondary_cursor;
        host->buffered_bytes = 0;
        host->available_bytes = host->buffer_size;
        async_file_lock_api.sleep(0);
        async_file_host_api.set_file_pointer(host->file, host->file_offset, nullptr, FILE_BEGIN);
    }
    else
    {
        if((record->flags & 2) != 0)
        {
            async_file_host_api.set_file_pointer(record->file, record->next_offset, nullptr, FILE_BEGIN);
            invalidate_shared_async_records(record);
        }
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
        host->available_bytes = host->buffer_size - copied_bytes + consumed_bytes / host->bytes_per_sector * host->bytes_per_sector;
        std::memcpy(buffer, record->buffer, copied_bytes);
        record->flags &= ~0x20U;
    }
    host->flags &= ~0x30U;
}

void seek_async_host(AsyncFileHost *host, uint32_t offset)
{
    const uint32_t current_offset = host->current_offset;
    if(offset == current_offset)
    {
        return;
    }
    if(current_offset < offset && offset - current_offset < host->buffer_size >> 2)
    {
        advance_async_host_read(host, offset - current_offset);
        host->current_offset = offset;
        return;
    }
    async_file_lock_api.enter_critical_section(&host->primary_lock);
    if((host->flags & 0x20) == 0)
    {
        if((host->flags & 0x10) != 0 && 0 < static_cast<int32_t>(host->file_offset - offset))
        {
            uint32_t bytes_to_advance = host->start_offset % host->bytes_per_sector + (offset - host->start_offset);
            auto *read_cursor = static_cast<uint8_t *>(host->read_cursor);
            auto *secondary_cursor = static_cast<uint8_t *>(host->secondary_cursor);
            if(secondary_cursor <= read_cursor)
            {
                bytes_to_advance += static_cast<uint32_t>(read_cursor - secondary_cursor);
            }
            else
            {
                bytes_to_advance += host->buffer_size - static_cast<uint32_t>(secondary_cursor - read_cursor);
            }
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
        {
            host->secondary_cursor = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(cursor) - reinterpret_cast<uintptr_t>(buffer_end));
        }
    }
    async_file_lock_api.leave_critical_section(&host->primary_lock);
    host->current_offset = offset;
    host->flags &= ~0x10U;
}

uint32_t copy_async_host_bytes(AsyncFileHost *host, void *destination, uint32_t bytes, uint32_t *total_bytes)
{
    if(host->file_size < host->current_offset + bytes)
    {
        bytes = host->file_size - host->current_offset;
    }
    auto *buffer_end = static_cast<uint8_t *>(host->buffer) + host->buffer_size;
    const uint32_t sector_size = host->bytes_per_sector;
    *total_bytes += bytes;
    if(bytes == 0)
    {
        return 0;
    }
    if((host->flags & 0x20) == 0)
    {
        uint32_t used_bytes = host->buffer_size - host->available_bytes;
        while(used_bytes <= bytes + sector_size * 2)
        {
            async_file_lock_api.sleep(0);
            used_bytes = host->buffer_size - host->available_bytes;
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
    {
        return;
    }
    async_file_lock_api.enter_critical_section(&host->primary_lock);
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
    if((flags & 1) != 0)
    {
        if((flags & 0x10) != 0)
        {
            const uint32_t limit = host->remaining_size < host->buffer_size ? host->remaining_size : host->buffer_size;
            initial_read_size = (limit / host->bytes_per_sector >> 2) * host->bytes_per_sector;
        }
        record->flags = flags & ~1U;
    }
    if(initial_read_size != 0)
    {
        uint32_t remaining = initial_read_size;
        auto *output = static_cast<uint8_t *>(host->write_cursor);
        do
        {
            uint32_t bytes_to_read = remaining;
            if(read_chunk_size <= remaining)
            {
                bytes_to_read = read_chunk_size;
            }
            remaining -= bytes_to_read;
            DWORD bytes_read = 0;
            async_file_host_api.read_file(host->file, output, bytes_to_read, &bytes_read, nullptr);
            async_file_lock_api.sleep(0);
            output += bytes_to_read;
        } while(remaining != 0);
        advance_async_host_write(host, initial_read_size);
    }
    async_file_lock_api.leave_critical_section(&host->primary_lock);
}

void handle_async_host_short_read(AsyncFileHost *host)
{
    const uint32_t sector_size = host->bytes_per_sector;
    const uint32_t value = host->end_offset - reinterpret_cast<uintptr_t>(host->buffer_start_cursor) % sector_size - host->file_offset + host->buffered_bytes;
    host->buffered_bytes = value;
    if(host->remaining_size <= host->buffer_size && host->remaining_size <= value)
    {
        const uint32_t flags = host->flags;
        host->flags = flags | 0x20;
        host->flags = (flags & ~0x10U) | 0x20;
        return;
    }
    host->flags |= 0x10;
    host->buffered_bytes = 0;
    host->buffer_start_cursor = static_cast<uint8_t *>(host->write_cursor) + host->start_offset % sector_size;
    host->read_cursor = host->write_cursor;
    host->file_offset = host->start_offset / sector_size * sector_size;
    async_file_lock_api.sleep(0);
    async_file_host_api.set_file_pointer(host->file, host->file_offset, nullptr, FILE_BEGIN);
}

DWORD WINAPI run_async_file_worker(LPVOID parameter)
{
    AsyncFileHost *host = static_cast<AsyncFileHost *>(parameter);
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
        if((flags & 1) != 0)
        {
            return 0;
        }
        if(host->active_file == nullptr)
        {
            async_file_lock_api.sleep(10);
            restart_timing = true;
        }
        else if((host->flags & 0x20) == 0)
        {
            if(restart_timing)
            {
                delay = 0;
                rate = host->mode - (host->mode >> 2);
                target_time = async_file_host_api.time_get_time();
                restart_timing = false;
            }
            uint32_t next_target = target_time;
            async_file_lock_api.enter_critical_section(&host->primary_lock);
            if(host->active_file == nullptr || host->available_bytes < minimum_available)
            {
                async_file_lock_api.leave_critical_section(&host->primary_lock);
                async_file_lock_api.sleep(0);
            }
            else
            {
                uint32_t bytes_to_read = maximum_read;
                if(host->available_bytes <= maximum_read)
                {
                    bytes_to_read = host->available_bytes;
                }
                next_target = bytes_to_read / rate + target_time;
                if(buffer_end < static_cast<uint8_t *>(host->write_cursor) + bytes_to_read)
                {
                    const uint32_t tail_bytes = static_cast<uint32_t>(buffer_end - static_cast<uint8_t *>(host->write_cursor));
                    DWORD bytes_read = 0;
                    async_file_lock_api.sleep(0);
                    async_file_host_api.read_file(host->file, host->write_cursor, tail_bytes, &bytes_read, nullptr);
                    advance_async_host_write(host, tail_bytes);
                    if(tail_bytes <= bytes_read && host->file_offset < host->end_offset)
                    {
                        bytes_to_read -= tail_bytes;
                        async_file_lock_api.sleep(0);
                        async_file_host_api.read_file(host->file, host->write_cursor, bytes_to_read, &bytes_read, nullptr);
                        advance_async_host_write(host, bytes_to_read);
                        if(bytes_read < bytes_to_read || host->end_offset <= host->file_offset)
                        {
                            handle_async_host_short_read(host);
                        }
                    }
                    else
                    {
                        handle_async_host_short_read(host);
                    }
                }
                else
                {
                    DWORD bytes_read = 0;
                    async_file_lock_api.sleep(0);
                    async_file_host_api.read_file(host->file, host->write_cursor, bytes_to_read, &bytes_read, nullptr);
                    advance_async_host_write(host, bytes_to_read);
                    if(bytes_read < bytes_to_read || host->end_offset <= host->file_offset)
                    {
                        handle_async_host_short_read(host);
                    }
                }
                async_file_lock_api.leave_critical_section(&host->primary_lock);
            }
            const DWORD now = async_file_host_api.time_get_time();
            const uint32_t requested_rate = host->mode;
            if(rate != requested_rate)
            {
                int32_t adjustment = static_cast<int32_t>(requested_rate - rate) >> 1;
                if(adjustment == 0)
                {
                    adjustment = 1;
                }
                rate += adjustment;
                if(static_cast<int32_t>(requested_rate) < static_cast<int32_t>(rate))
                {
                    rate = requested_rate;
                }
            }
            delay = delay - now + next_target;
            target_time = now;
            if(0 < static_cast<int32_t>(delay))
            {
                async_file_lock_api.sleep(delay);
                target_time = delay + now;
                delay = 0;
            }
        }
        else
        {
            async_file_lock_api.sleep(10);
            restart_timing = true;
        }
        flags = host->flags;
    }
}

AsyncFileHost *create_async_file_host(const char *root, uint32_t requested_bytes, int32_t mode)
{
    if(!async_file_enabled)
    {
        return nullptr;
    }
    HANDLE heap = async_file_host_api.get_process_heap();
    AsyncFileHost *host = static_cast<AsyncFileHost *>(async_file_host_api.heap_alloc(heap, HEAP_ZERO_MEMORY, sizeof(AsyncFileHost)));
    if(host == nullptr)
    {
        return nullptr;
    }
    DWORD cluster_count = 0;
    if(!async_file_host_api.get_disk_free_space(root, &host->sectors_per_cluster, &host->bytes_per_sector, &cluster_count, &cluster_count))
    {
        async_file_host_api.heap_free(async_file_host_api.get_process_heap(), 0, host);
        return nullptr;
    }
    async_file_host_api.initialize_critical_section(&host->primary_lock);
    async_file_host_api.initialize_critical_section(&host->secondary_lock);
    host->buffer_size = requested_bytes / 0xffff * 0xffff / host->bytes_per_sector * host->bytes_per_sector;
    host->available_bytes = host->buffer_size;
    host->buffer = async_file_host_api.virtual_alloc(nullptr, host->buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if(host->buffer == nullptr)
    {
        async_file_host_api.delete_critical_section(&host->primary_lock);
        async_file_host_api.delete_critical_section(&host->secondary_lock);
        async_file_host_api.heap_free(async_file_host_api.get_process_heap(), 0, host);
        return nullptr;
    }
    host->self = host;
    host->write_cursor = host->buffer;
    host->secondary_cursor = host->buffer;
    host->mode = mode == 0 ? -1 : mode;
    async_file_lock_api.enter_critical_section(&async_file_global_lock);
    host->next = async_file_hosts;
    async_file_hosts = host;
    async_file_lock_api.leave_critical_section(&async_file_global_lock);
    DWORD thread_id = 0;
    host->thread = async_file_host_api.create_thread(nullptr, 0, run_async_file_worker, host, 0, &thread_id);
    return host;
}

AsyncFileHost *acquire_async_file_host(AsyncFileHost *identity)
{
    if(!async_file_enabled)
    {
        return nullptr;
    }
    while(true)
    {
        uint32_t busy = 0;
        AsyncFileHost *result = nullptr;
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            if(host->self == identity)
            {
                busy = host->flags & 0x10000;
                if(busy == 0)
                {
                    host->flags |= 0x10000;
                    result = host;
                }
                break;
            }
        }
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
        if(busy == 0)
        {
            return result;
        }
        async_file_lock_api.sleep(0);
    }
}

void release_async_file_host(AsyncFileHost *identity)
{
    async_file_lock_api.enter_critical_section(&async_file_global_lock);
    for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
    {
        if(host->self == identity)
        {
            host->flags &= ~0x10000U;
            break;
        }
    }
    async_file_lock_api.leave_critical_section(&async_file_global_lock);
}

uint32_t destroy_async_file_host(AsyncFileHost *identity)
{
    if(!async_file_enabled)
    {
        return 0;
    }
    AsyncFileHost *host = acquire_async_file_host(identity);
    if(host == nullptr)
    {
        return 0;
    }
    while(host->files != nullptr)
    {
        close_async_file_record(host->files);
    }
    async_file_lock_api.enter_critical_section(&async_file_global_lock);
    AsyncFileHost *previous = nullptr;
    AsyncFileHost *current = async_file_hosts;
    while(current != nullptr && current->self != identity)
    {
        previous = current;
        current = current->next;
    }
    if(previous == nullptr)
    {
        async_file_hosts = current->next;
    }
    else
    {
        previous->next = current->next;
    }
    async_file_lock_api.leave_critical_section(&async_file_global_lock);
    current->flags |= 1;
    async_file_host_api.wait_for_single_object(current->thread, INFINITE);
    async_file_host_api.delete_critical_section(&current->primary_lock);
    async_file_host_api.delete_critical_section(&current->secondary_lock);
    async_file_open_api.close_handle(current->thread);
    async_file_open_api.virtual_free(current->buffer, 0, MEM_RELEASE);
    async_file_host_api.heap_free(async_file_host_api.get_process_heap(), 0, current);
    return 1;
}

uint32_t shutdown_async_file_subsystem()
{
    if(!async_file_enabled)
    {
        return 0;
    }
    while(true)
    {
        while(async_file_hosts != nullptr)
        {
            async_file_shutdown_api.destroy_host(async_file_hosts);
        }
        async_file_shutdown_api.enter_critical_section(&async_file_global_lock);
        if(async_file_hosts == nullptr)
        {
            break;
        }
        async_file_shutdown_api.leave_critical_section(&async_file_global_lock);
    }
    async_file_shutdown_api.delete_critical_section(&async_file_global_lock);
    async_file_enabled = false;
    async_file_hosts = nullptr;
    return 1;
}


AsyncFileRecord *acquire_async_file_record(AsyncFileRecord *identity)
{
    if(!async_file_enabled)
    {
        return nullptr;
    }
    while(true)
    {
        uint32_t busy = 0;
        AsyncFileRecord *result = nullptr;
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    uint32_t flags = record->flags;
                    busy = flags & 0x10000;
                    if(busy == 0)
                    {
                        record->flags = flags | 0x10000;
                        result = record;
                        if((flags & 2) != 0)
                        {
                            for(AsyncFileRecord *shared = host->files; shared != nullptr; shared = shared->next)
                            {
                                if(shared->file == record->file)
                                {
                                    shared->flags |= 0x10000;
                                }
                            }
                        }
                    }
                    host = nullptr;
                    break;
                }
            }
            if(host == nullptr)
            {
                break;
            }
        }
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
        if(busy == 0)
        {
            return result;
        }
        async_file_lock_api.sleep(0);
    }
}

void release_async_file_record(AsyncFileRecord *identity)
{
    if(async_file_enabled)
    {
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        for(AsyncFileHost *host = async_file_hosts; host != nullptr; host = host->next)
        {
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    uint32_t flags = record->flags;
                    if((flags & 0x10000) != 0)
                    {
                        record->flags = flags & ~0x10000U;
                        if((flags & 2) != 0)
                        {
                            for(AsyncFileRecord *shared = host->files; shared != nullptr; shared = shared->next)
                            {
                                if(shared->file == record->file)
                                {
                                    shared->flags &= ~0x10000U;
                                }
                            }
                        }
                    }
                    host = nullptr;
                    break;
                }
            }
            if(host == nullptr)
            {
                break;
            }
        }
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
    }
}

void set_async_file_host_mode(AsyncFileHost *identity, int32_t mode)
{
    AsyncFileHost *host = acquire_async_file_host(identity);
    if(host != nullptr)
    {
        if(mode != 0)
        {
            host->mode = mode;
        }
        release_async_file_host(identity);
    }
}

uint32_t get_async_file_size(AsyncFileRecord *identity)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
    {
        return 0;
    }
    uint32_t size = record->file_size;
    release_async_file_record(identity);
    return size;
}

uint32_t get_async_file_position(AsyncFileRecord *identity)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
    {
        return 0;
    }
    const uint32_t position = record->current_offset;
    release_async_file_record(identity);
    return position;
}

uint32_t set_async_file_position(AsyncFileRecord *identity, uint32_t position)
{
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
    {
        return 0;
    }
    uint32_t result = 0;
    if(record->start_offset <= position && position <= record->end_offset)
    {
        if((record->flags & 0x20) != 0)
        {
            if(position < record->previous_offset || record->next_offset < position)
            {
                record->flags &= ~0x20u;
            }
            else
            {
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer) + (position - record->previous_offset);
                record->buffered_bytes = record->next_offset - position;
            }
        }
        record->current_offset = position;
        if((record->flags & 2) != 0)
        {
            invalidate_shared_async_records(record);
        }
        result = 1;
    }
    release_async_file_record(identity);
    return result;
}

AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags)
{
    AsyncFileHost *host = acquire_async_file_host(host_identity);
    if(host == nullptr)
    {
        return nullptr;
    }
    AsyncFileRecord *record = nullptr;
    HANDLE file = async_file_open_api.create_file(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0x20000000, nullptr);
    bool valid = file != INVALID_HANDLE_VALUE;
    if(valid)
    {
        HANDLE heap = async_file_open_api.get_process_heap();
        record = static_cast<AsyncFileRecord *>(async_file_open_api.heap_alloc(heap, HEAP_ZERO_MEMORY, sizeof(AsyncFileRecord)));
        if(record == nullptr)
        {
            valid = false;
            async_file_open_api.close_handle(file);
        }
        if(valid)
        {
            record->buffer = async_file_open_api.virtual_alloc(nullptr, 0x8000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if(record->buffer == nullptr)
            {
                HANDLE cleanup_heap = async_file_open_api.get_process_heap();
                AsyncFileRecord *failed_record = record;
                record = nullptr;
                valid = false;
                async_file_open_api.heap_free(cleanup_heap, 0, failed_record);
                async_file_open_api.close_handle(file);
            }
        }
    }
    if(valid)
    {
        record->self = record;
        record->flags = flags | 1;
        record->file = file;
        record->file_size = async_file_open_api.get_file_size(file, nullptr);
        record->start_offset = start_offset;
        if(end_offset == 0)
        {
            end_offset = record->file_size;
        }
        record->end_offset = end_offset;
        record->remaining_size = end_offset - start_offset;
        record->current_offset = start_offset;
        record->host = host;
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        record->next = host->files;
        host->files = record;
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
    }
    release_async_file_host(host_identity);
    return record;
}

AsyncFileRecord *duplicate_async_file_record(AsyncFileRecord *identity, uint32_t start_offset, uint32_t end_offset, uint32_t flags)
{
    AsyncFileRecord *source = acquire_async_file_record(identity);
    if(source == nullptr)
    {
        return nullptr;
    }
    AsyncFileHost *host = source->host;
    AsyncFileRecord *record = static_cast<AsyncFileRecord *>(async_file_open_api.heap_alloc(async_file_open_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(AsyncFileRecord)));
    if(record != nullptr)
    {
        record->self = record;
        record->flags = flags | 3;
        source->flags |= 2;
        record->file = source->file;
        record->buffer = source->buffer;
        record->file_size = source->file_size;
        record->start_offset = start_offset;
        if(end_offset == 0)
        {
            end_offset = record->file_size;
        }
        record->end_offset = end_offset;
        record->remaining_size = end_offset - start_offset;
        record->current_offset = start_offset;
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        record->next = host->files;
        host->files = record;
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
        record->host = host;
    }
    release_async_file_record(identity);
    return record;
}

uint32_t close_async_file_record(AsyncFileRecord *identity)
{
    if(!async_file_enabled)
    {
        return 0;
    }
    uint32_t result = 0;
    while(true)
    {
        uint32_t busy = 0;
        async_file_lock_api.enter_critical_section(&async_file_global_lock);
        bool finished = false;
        for(AsyncFileHost *host = async_file_hosts; host != nullptr && !finished; host = host->next)
        {
            AsyncFileRecord *previous = nullptr;
            for(AsyncFileRecord *record = host->files; record != nullptr; record = record->next)
            {
                if(record->self == identity)
                {
                    busy = record->flags & 0x10000;
                    if(busy == 0)
                    {
                        ++result;
                        if(previous == nullptr)
                        {
                            host->files = record->next;
                        }
                        else
                        {
                            previous->next = record->next;
                        }
                        async_file_lock_api.enter_critical_section(&host->secondary_lock);
                        if(host->active_file == record)
                        {
                            async_file_lock_api.enter_critical_section(&host->primary_lock);
                            host->active_file = nullptr;
                            async_file_lock_api.leave_critical_section(&host->primary_lock);
                        }
                        async_file_lock_api.leave_critical_section(&host->secondary_lock);
                        int32_t shared_count = 0;
                        AsyncFileRecord *single_shared = nullptr;
                        if((record->flags & 2) != 0)
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
                            {
                                single_shared->flags &= ~2U;
                            }
                        }
                        if(shared_count == 0)
                        {
                            BOOL closed = async_file_open_api.close_handle(record->file);
                            BOOL freed_buffer = async_file_open_api.virtual_free(record->buffer, 0, MEM_RELEASE);
                            HANDLE heap = async_file_open_api.get_process_heap();
                            BOOL freed_record = async_file_open_api.heap_free(heap, 0, record);
                            result = result & static_cast<uint32_t>(closed) & static_cast<uint32_t>(freed_buffer) & static_cast<uint32_t>(freed_record);
                        }
                    }
                    finished = true;
                    break;
                }
                previous = record;
            }
        }
        async_file_lock_api.leave_critical_section(&async_file_global_lock);
        if(busy == 0)
        {
            return result;
        }
        async_file_lock_api.sleep(0);
    }
}

uint32_t read_async_file_record(AsyncFileRecord *identity, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer)
{
    *bytes_read = 0;
    AsyncFileRecord *record = acquire_async_file_record(identity);
    if(record == nullptr)
    {
        return 0;
    }
    AsyncFileHost *host = record->host;
    uint32_t result = 1;
    if(force_host_buffer != 0 || host->active_file == record)
    {
        while(true)
        {
            async_file_lock_api.enter_critical_section(&host->secondary_lock);
            if(force_host_buffer != 0 || host->active_file == record)
            {
                break;
            }
            async_file_lock_api.leave_critical_section(&host->secondary_lock);
        }
        if(host->active_file == record)
        {
            seek_async_host(host, record->current_offset);
        }
        else
        {
            activate_async_file_record(record);
        }
        const uint32_t chunk_size = (host->buffer_size / host->bytes_per_sector >> 2) * host->bytes_per_sector;
        auto *output = static_cast<uint8_t *>(destination);
        while(bytes != 0)
        {
            uint32_t chunk = chunk_size;
            if(bytes <= chunk_size)
            {
                chunk = bytes;
            }
            if(copy_async_host_bytes(host, output, chunk, bytes_read) == 0)
            {
                result = 0;
                break;
            }
            output += chunk;
            bytes -= chunk;
        }
        record->current_offset = host->current_offset;
        async_file_lock_api.leave_critical_section(&host->secondary_lock);
    }
    else
    {
        uint32_t copied = 0;
        const uint32_t chunk_size = 0x8000 / host->bytes_per_sector * host->bytes_per_sector;
        auto *output = static_cast<uint8_t *>(destination);
        while(bytes != 0)
        {
            uint32_t chunk = record->buffered_bytes;
            if(chunk != 0 && (record->flags & 0x20) != 0)
            {
                if(bytes < chunk)
                {
                    chunk = bytes;
                }
                std::memcpy(output, record->buffer_cursor, chunk);
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer_cursor) + chunk;
                record->buffered_bytes -= chunk;
                output += chunk;
                bytes -= chunk;
                copied += chunk;
            }
            if(bytes == 0)
            {
                break;
            }
            const DWORD target_time = record->timestamp + chunk_size / host->mode;
            async_file_lock_api.sleep(0);
            DWORD file_bytes = 0;
            if((record->flags & 0x20) == 0)
            {
                const uint32_t aligned_offset = record->current_offset / host->bytes_per_sector * host->bytes_per_sector;
                record->next_offset = aligned_offset;
                record->previous_offset = aligned_offset;
                async_file_host_api.set_file_pointer(record->file, aligned_offset, nullptr, FILE_BEGIN);
                if(!async_file_host_api.read_file(record->file, record->buffer, chunk_size, &file_bytes, nullptr))
                {
                    result = 0;
                    break;
                }
                if(file_bytes == 0)
                {
                    break;
                }
                if((record->flags & 2) != 0)
                {
                    invalidate_shared_async_records(record);
                }
                record->flags |= 0x20;
                const uint32_t prefix = record->current_offset % host->bytes_per_sector;
                record->buffer_cursor = static_cast<uint8_t *>(record->buffer) + prefix;
                record->buffered_bytes = file_bytes - prefix;
            }
            else
            {
                if(!async_file_host_api.read_file(record->file, record->buffer, chunk_size, &file_bytes, nullptr))
                {
                    result = 0;
                    break;
                }
                if(file_bytes == 0)
                {
                    break;
                }
                record->buffer_cursor = record->buffer;
                record->buffered_bytes = file_bytes;
            }
            record->previous_offset = record->next_offset;
            record->next_offset += file_bytes;
            const DWORD now = async_file_host_api.time_get_time();
            record->timestamp = now;
            if(now < target_time)
            {
                async_file_lock_api.sleep(target_time - now);
            }
        }
        record->current_offset += copied;
        *bytes_read = copied;
    }
    release_async_file_record(identity);
    return result;
}



} // namespace gag
