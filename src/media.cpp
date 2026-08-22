#include "media.h"
#include "runtime_internal.h"

namespace gag
{
RuntimeMediaBackend *create_runtime_bitmap_backend(uint32_t, uint32_t extension_bytes, void *bitmap_data)
{
    auto *backend = static_cast<RuntimeMediaBackend *>(runtime_bitmap_backend_create_api.heap_alloc(runtime_media_backend_heap, HEAP_ZERO_MEMORY, sizeof(RuntimeMediaBackend) + extension_bytes));
    if(backend == nullptr)
    {
        return nullptr;
    }
    if(extension_bytes != 0)
    {
        backend->extension_data = backend + 1;
    }
    backend->type = 0xac;
    backend->identity = backend;
    backend->source_data = bitmap_data;
    if(*static_cast<uint16_t *>(bitmap_data) != 0x4d42)
    {
        backend->error_state = 1;
        backend->media_flags |= 0x80000000;
    }
    backend->format_data = static_cast<uint8_t *>(bitmap_data) + sizeof(BITMAPFILEHEADER);
    backend->palette_version = 0x0300;
    backend->palette_entry_count = 0x00ec;
    backend->media_flags |= 0x20;
    runtime_bitmap_backend_create_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    if(runtime_media_backend_head == nullptr)
    {
        runtime_media_backend_head = backend;
        backend->previous = nullptr;
    }
    else
    {
        runtime_media_backend_tail->next = backend;
        backend->previous = runtime_media_backend_tail;
    }
    backend->next = nullptr;
    runtime_media_backend_tail = backend;
    runtime_bitmap_backend_create_api.release_mutex(runtime_media_backend_mutex);
    return backend;
}

RuntimeAnimationBackend *create_runtime_animation_backend(uint32_t, void *data, uint32_t extension_bytes, uint32_t storage)
{
    RuntimeAnimationBackend *backend = nullptr;
    if(storage == 0x01000000)
    {
        backend =
            static_cast<RuntimeAnimationBackend *>(runtime_animation_backend_create_api.heap_alloc(runtime_media_backend_heap, HEAP_ZERO_MEMORY, sizeof(RuntimeAnimationBackend) + extension_bytes));
        if(backend == nullptr)
        {
            return nullptr;
        }
        backend->base.stream_record = static_cast<AsyncFileRecord *>(data);
        backend->base.format_data = &backend->header;
        if(extension_bytes != 0)
        {
            backend->base.extension_data = backend + 1;
        }
        std::memcpy(&backend->header, data, sizeof(backend->header));
        const uint16_t signature = backend->header.signature;
        if(signature == 0xaf11)
        {
            backend->base.frame_duration = (backend->header.frame_duration * 5 + 5) * 2;
            backend->data_start = static_cast<uint8_t *>(data) + sizeof(RuntimeAnimationFileHeader);
            backend->base.frame_header = backend->data_start;
            backend->data_end = static_cast<uint8_t *>(backend->data_start) + static_cast<RuntimeAnimationFrameHeader *>(backend->data_start)->size;
            backend->base.error_state = 0;
        }
        else if(signature == 0xaf12)
        {
            backend->base.frame_duration = backend->header.frame_duration;
            backend->data_start = static_cast<uint8_t *>(data) + backend->header.data_start_offset;
            backend->data_end = static_cast<uint8_t *>(data) + backend->header.data_end_offset;
            backend->base.error_state = 0;
        }
        else
        {
            backend->base.error_state = 1;
            backend->base.media_flags |= 0x80000000;
        }
        backend->source_cursor = backend->data_start;
    }
    else if(storage == 0x02000000)
    {
        auto *record = static_cast<AsyncFileRecord *>(data);
        const uint32_t saved_position = runtime_animation_backend_create_api.get_position(record);
        RuntimeAnimationFileHeader header;
        uint32_t bytes_read;
        runtime_animation_backend_create_api.read_record(record, &header, sizeof(header), &bytes_read, 0);
        backend =
            static_cast<RuntimeAnimationBackend *>(runtime_animation_backend_create_api.heap_alloc(runtime_media_backend_heap, HEAP_ZERO_MEMORY, sizeof(RuntimeAnimationBackend) + extension_bytes));
        if(backend == nullptr)
        {
            return nullptr;
        }
        backend->base.format_data = &backend->header;
        backend->base.frame_header = &backend->streamed_headers.frame;
        backend->base.chunk_header = &backend->streamed_headers.chunk;
        if(extension_bytes != 0)
        {
            backend->base.extension_data = backend + 1;
        }
        backend->base.stream_record = record;
        backend->header = header;
        const uint16_t signature = backend->header.signature;
        if(signature == 0xaf12)
        {
            backend->base.frame_duration = backend->header.frame_duration;
            backend->data_start = reinterpret_cast<void *>(static_cast<uintptr_t>(saved_position) + backend->header.data_start_offset);
            backend->data_end = reinterpret_cast<void *>(static_cast<uintptr_t>(saved_position) + backend->header.data_end_offset);
            backend->base.error_state = 0;
        }
        else if(signature != 0xaf11)
        {
            backend->base.error_state = 1;
            backend->base.media_flags |= 0x80000000;
        }
        runtime_animation_backend_create_api.set_position(record, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(backend->data_start)));
    }
    else
    {
        return nullptr;
    }
    backend->base.identity = backend;
    backend->base.type = 0xaa;
    backend->base.media_flags |= storage | 0x21;
    backend->base.palette_version = 0x0300;
    backend->base.palette_entry_count = 0x0100;
    backend->base.sound_handle = 0xffffffff;
    backend->base.scale_x = 1;
    backend->base.scale_y = 1;
    runtime_animation_backend_create_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    if(runtime_media_backend_head == nullptr)
    {
        runtime_media_backend_head = &backend->base;
        backend->base.previous = nullptr;
    }
    else
    {
        runtime_media_backend_tail->next = &backend->base;
        backend->base.previous = runtime_media_backend_tail;
    }
    backend->base.next = nullptr;
    runtime_media_backend_tail = &backend->base;
    runtime_animation_backend_create_api.release_mutex(runtime_media_backend_mutex);
    return backend;
}

RuntimeMediaBackend *acquire_runtime_media_backend(void *identity)
{
    RuntimeMediaBackend *result = nullptr;
    const DWORD thread_id = runtime_media_backend_api.get_current_thread_id();
    for(;;)
    {
        bool contended = false;
        runtime_media_backend_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
        for(RuntimeMediaBackend *backend = runtime_media_backend_head; backend != nullptr; backend = backend->next)
        {
            if(backend->identity == identity)
            {
                if(backend->recursion_count == 0 || backend->owner_thread == thread_id)
                {
                    backend->owner_thread = thread_id;
                    ++backend->recursion_count;
                    result = backend;
                }
                else
                {
                    contended = true;
                }
                break;
            }
        }
        runtime_media_backend_api.release_mutex(runtime_media_backend_mutex);
        if(!contended)
        {
            return result;
        }
        runtime_media_backend_api.sleep(0);
    }
}

uint32_t get_runtime_media_backend_type(void *identity)
{
    uint32_t result = 0;
    runtime_media_backend_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    for(RuntimeMediaBackend *backend = runtime_media_backend_head; backend != nullptr; backend = backend->next)
    {
        if(backend->identity == identity)
        {
            result = backend->type;
            break;
        }
    }
    runtime_media_backend_api.release_mutex(runtime_media_backend_mutex);
    return result;
}

uint8_t classify_runtime_media_data(const void *data)
{
    const auto *bytes = static_cast<const uint8_t *>(data);
    uint16_t signature;
    std::memcpy(&signature, bytes + 4, sizeof(signature));
    if(signature == 0xaf12)
    {
        return 3;
    }
    if(bytes[0] == 'B' && bytes[1] == 'M')
    {
        return 1;
    }
    static constexpr uint8_t wave_format_signature[7]{ 'W', 'A', 'V', 'E', 'f', 'm', 't' };
    if(std::memcmp(bytes + 8, wave_format_signature, sizeof(wave_format_signature)) == 0)
    {
        return 2;
    }
    static constexpr uint8_t configuration_signature[5]{ '[', 'C', 'F', 'G', ']' };
    return std::memcmp(bytes, configuration_signature, sizeof(configuration_signature)) == 0 ? 4 : 0;
}

uint32_t read_compressor_input(void *destination, uint32_t requested_size)
{
    if(compressor_input_position >= compressor_input_size)
    {
        return 0;
    }
    uint32_t copied_size = compressor_input_size - compressor_input_position;
    if(requested_size < copied_size)
    {
        copied_size = requested_size;
    }
    std::memcpy(destination, compressor_input + compressor_input_position, copied_size);
    compressor_input_position += copied_size;
    return copied_size;
}

void set_runtime_media_backend_scale(void *identity, uint32_t scale_x, uint32_t scale_y)
{
    runtime_media_backend_configure_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    for(RuntimeMediaBackend *backend = runtime_media_backend_head; backend != nullptr; backend = backend->next)
    {
        if(backend->identity == identity)
        {
            backend->scale_x = scale_x;
            backend->scale_y = scale_y;
            break;
        }
    }
    runtime_media_backend_configure_api.release_mutex(runtime_media_backend_mutex);
}

uint32_t stop_runtime_animation_backend(void *identity)
{
    runtime_media_backend_configure_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    RuntimeMediaBackend *backend = runtime_media_backend_head;
    while(backend != nullptr && backend->identity != identity)
    {
        backend = backend->next;
    }
    if(backend == nullptr)
    {
        runtime_media_backend_configure_api.release_mutex(runtime_media_backend_mutex);
        return 0;
    }
    backend->media_flags |= 0x10000;
    runtime_media_backend_configure_api.release_mutex(runtime_media_backend_mutex);
    return 1;
}

void *get_locked_runtime_media_extension(void *identity)
{
    RuntimeMediaBackend *backend = acquire_runtime_media_backend(identity);
    return backend == nullptr ? nullptr : backend->extension_data;
}

UINT apply_runtime_palette_entries(RuntimePaletteTarget *target, void *palette_data, uint32_t *flags, uint32_t force)
{
    auto *entries = static_cast<RuntimePaletteData *>(palette_data)->entries;
    if((*flags & 0x40000) == 0)
    {
        if((*flags & 0x80000) != 0)
        {
            for(uint32_t index = 0; index < 236; ++index)
            {
                entries[index].peFlags = PC_EXPLICIT;
            }
            runtime_palette_update_api.select_palette(target->device_context, target->palette, FALSE);
            force = 1;
            *flags &= ~0x80000u;
        }
    }
    else
    {
        for(uint32_t index = 0; index < 236; ++index)
        {
            entries[index].peFlags = 0;
        }
        runtime_palette_update_api.select_palette(target->device_context, target->palette, TRUE);
        force = 1;
        *flags |= 0x80000;
    }

    if(force == 0)
    {
        return runtime_palette_update_api.animate_palette(target->palette, 0, 236, entries) != FALSE ? 236 : 0;
    }
    runtime_palette_update_api.unrealize_object(target->palette);
    const UINT result = runtime_palette_update_api.set_palette_entries(target->palette, 0, 236, entries);
    if(result != 0)
    {
        runtime_palette_update_api.realize_palette(target->device_context);
    }
    return result;
}

uint32_t configure_runtime_bitmap_backend(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, void *callback, uint32_t flags)
{
    runtime_media_backend_configure_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    RuntimeMediaBackend *backend = runtime_media_backend_head;
    while(backend != nullptr && backend->identity != identity)
    {
        backend = backend->next;
    }
    if(backend == nullptr)
    {
        runtime_media_backend_configure_api.release_mutex(runtime_media_backend_mutex);
        return 0;
    }
    backend->media_flags |= flags;
    if(callback == nullptr)
    {
        callback = &backend->palette_version;
    }
    backend->comparison_palette = callback;
    backend->window = target->window;
    backend->destination_context = target->destination_context;
    backend->destination_bits_per_pixel = target->bits_per_pixel;
    backend->destination_palette = target->palette;
    backend->presentation_field_0944 = target->field_0944;
    backend->source_context = target->source_context;
    std::memcpy(backend->presentation_tail, target->tail, sizeof(backend->presentation_tail));
    backend->destination_x = static_cast<uint16_t>(descriptor->x);
    backend->destination_y = static_cast<uint16_t>(descriptor->y);
    backend->destination_stride = static_cast<uint16_t>(descriptor->width);
    backend->destination_reserved = static_cast<uint16_t>(descriptor->height);
    backend->descriptor_2 = descriptor->present;
    backend->destination_pixels = reinterpret_cast<uint8_t *>(descriptor->pixels);
    runtime_media_backend_configure_api.release_mutex(runtime_media_backend_mutex);
    return 1;
}

uint32_t configure_runtime_animation_backend(void *identity, const RuntimePresentationTarget *target, const DisplaySceneDescriptor *descriptor, const void *comparison_palette, uint32_t flags,
    RuntimeAnimationCallback callback)
{
    runtime_animation_backend_configure_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    RuntimeMediaBackend *backend = runtime_media_backend_head;
    while(backend != nullptr && backend->identity != identity)
    {
        backend = backend->next;
    }
    if(backend == nullptr)
    {
        runtime_animation_backend_configure_api.release_mutex(runtime_media_backend_mutex);
        return 0;
    }
    if(comparison_palette == nullptr)
    {
        comparison_palette = &backend->palette_version;
    }
    backend->comparison_palette = comparison_palette;
    backend->animation_callback = callback == nullptr ? present_runtime_animation_frame : callback;
    backend->window = target->window;
    backend->destination_context = target->destination_context;
    backend->destination_bits_per_pixel = target->bits_per_pixel;
    backend->destination_palette = target->palette;
    backend->presentation_field_0944 = target->field_0944;
    backend->source_context = target->source_context;
    std::memcpy(backend->presentation_tail, target->tail, sizeof(backend->presentation_tail));
    backend->destination_x = static_cast<uint16_t>(descriptor->x);
    backend->destination_y = static_cast<uint16_t>(descriptor->y);
    backend->destination_stride = static_cast<uint16_t>(descriptor->width);
    backend->destination_reserved = static_cast<uint16_t>(descriptor->height);
    backend->descriptor_2 = descriptor->present;
    backend->destination_pixels = reinterpret_cast<uint8_t *>(descriptor->pixels);
    backend->dirty_left = 32000;
    backend->dirty_top = 32000;
    backend->dirty_right = 0;
    backend->dirty_bottom = 0;
    backend->media_flags |= flags;
    HANDLE thread = runtime_animation_backend_configure_api.create_thread(nullptr, 0, run_runtime_animation_thread, backend, 0, reinterpret_cast<LPDWORD>(&backend->source_data));
    runtime_animation_backend_configure_api.close_handle(thread);
    runtime_animation_backend_configure_api.release_mutex(runtime_media_backend_mutex);
    return 1;
}

void configure_runtime_resource_palette(RuntimeResourceObject *resource)
{
    auto *backend = static_cast<RuntimeMediaBackend *>(resource->backend);
    const auto *palette = reinterpret_cast<const uint32_t *>(backend->palette_entries);
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(resource->scene_identifier));
    const intptr_t owner = reinterpret_cast<intptr_t>(resource);
    if((resource->type_flags & 1) == 0)
    {
        if(runtime_resource_palette_configure_api.set_primary_owner(resource->scene_identifier, owner, false)
            && ((resource->backend_flags & 0x4000000) == 0 || runtime_resource_palette_bits_per_pixel > 8))
        {
            runtime_resource_palette_configure_api.configure_palette(scene, palette, 0x100);
        }
    }
    else
    {
        runtime_resource_palette_configure_api.set_primary_owner(resource->scene_identifier, owner, false);
        runtime_resource_palette_configure_api.configure_palette(nullptr, palette, 0xec);
        if((resource->backend_flags & 0x4000000) == 0 || runtime_resource_palette_bits_per_pixel > 8)
        {
            runtime_resource_palette_configure_api.configure_palette(scene, palette, 0x100);
        }
    }
}

void build_runtime_palette_index_remap(RuntimeMediaBackend *backend)
{
    if((backend->media_flags & 0x4000000) != 0)
    {
        return;
    }
    const auto *comparison_palette = static_cast<const uint8_t *>(backend->comparison_palette);
    const auto *source_color = reinterpret_cast<const uint8_t *>(backend->palette_entries);
    uint8_t *remap = backend->palette_remap;
    const uint32_t bits_per_pixel = backend->destination_bits_per_pixel;
    const uint16_t comparison_count = bits_per_pixel == 8 ? 0xec : 0x100;
    for(uint16_t source_index = 0; source_index < 0x100; ++source_index)
    {
        uint8_t tolerance = 0;
        uint16_t comparison_index = comparison_count;
        do
        {
            comparison_index = 0;
            const uint8_t *comparison_color = comparison_palette;
            do
            {
                comparison_color += 4;
                uint32_t component = 0;
                for(; component < 3; ++component)
                {
                    const uint8_t left = comparison_color[component];
                    const uint8_t right = source_color[component];
                    const uint8_t difference = left < right ? static_cast<uint8_t>(right - left) : static_cast<uint8_t>(left - right);
                    if(tolerance < difference)
                    {
                        break;
                    }
                }
                if(component == 3)
                {
                    break;
                }
                ++comparison_index;
            } while(comparison_index < comparison_count);
            if(comparison_index < comparison_count)
            {
                break;
            }
            tolerance = static_cast<uint8_t>(tolerance + 10);
        } while(tolerance < 0xfa);
        *remap++ = static_cast<uint8_t>(comparison_index);
        source_color += 4;
    }
}

uint8_t convert_runtime_bitmap_to_surface(RuntimeMediaBackend *backend)
{
    auto *format = static_cast<BITMAPINFOHEADER *>(backend->format_data);
    const uint8_t *source_palette = reinterpret_cast<const uint8_t *>(format) + format->biSize;
    for(uint32_t index = 0; index < 0x100; ++index)
    {
        const uint8_t blue = source_palette[index * 4];
        const uint8_t green = source_palette[index * 4 + 1];
        const uint8_t red = source_palette[index * 4 + 2];
        backend->palette_entries[index] = { red, green, blue, 1 };
        std::memcpy(&backend->dib_colors[index], source_palette + index * 4, sizeof(uint32_t));
    }
    build_runtime_palette_index_remap(backend);
    auto *bitmap_file = static_cast<BITMAPFILEHEADER *>(backend->source_data);
    const uint8_t *source = reinterpret_cast<const uint8_t *>(bitmap_file) + bitmap_file->bfOffBits;
    const uint16_t destination_x = backend->destination_x;
    const uint16_t destination_y = backend->destination_y;
    const uint16_t destination_stride = backend->destination_stride;
    uint8_t *destination_base = backend->destination_pixels;
    uint8_t *destination = destination_base + destination_y * destination_stride + destination_x;
    const int32_t width = format->biWidth;
    const int32_t signed_height = format->biHeight;
    int32_t height = signed_height;
    int32_t destination_row_adjustment;
    if(height < 0)
    {
        height = -height;
        destination_row_adjustment = destination_stride - width;
    }
    else
    {
        destination += destination_stride * (height - 1);
        destination_row_adjustment = -(destination_stride + width);
    }
    const uint32_t source_row_padding = (static_cast<uint32_t>(width) + 3 & ~3u) - static_cast<uint32_t>(width);
    uint8_t result = 0;
    if((backend->media_flags & 0x4000000) == 0)
    {
        const uint8_t *remap = backend->palette_remap;
        do
        {
            int32_t remaining = width;
            do
            {
                result = remap[*source++];
                *destination++ = result;
                --remaining;
            } while(remaining != 0);
            destination += destination_row_adjustment;
            source += source_row_padding;
            --height;
        } while(height != 0);
    }
    else
    {
        do
        {
            std::memcpy(destination, source, static_cast<size_t>(width));
            source += width + source_row_padding;
            destination += width + destination_row_adjustment;
            --height;
        } while(height != 0);
    }
    return result;
}

void finalize_runtime_media_backend(void *identity)
{
    runtime_media_backend_finalize_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
    RuntimeMediaBackend *backend = runtime_media_backend_head;
    while(backend != nullptr)
    {
        if(backend->identity == identity)
        {
            if(backend->type == 0xaa)
            {
                backend->media_flags &= 0xffffdffe;
                break;
            }
            if(backend->type == 0xac)
            {
                if((backend->media_flags & 0x20) != 0)
                {
                    backend->media_flags &= ~0x20u;
                    runtime_media_backend_finalize_api.convert_bitmap(backend);
                    const uint32_t flags = backend->media_flags;
                    if((flags & 0x100) == 0)
                    {
                        if(backend->destination_bits_per_pixel == 8 && (flags & 0x10) == 0 && (flags & 0x20) != 0)
                        {
                            runtime_media_backend_finalize_api.set_palette_entries(backend->destination_palette, 0, 0xec, backend->palette_entries);
                            runtime_media_backend_finalize_api.realize_palette(backend->destination_context);
                            backend->media_flags &= ~0x20u;
                        }
                        runtime_media_backend_finalize_api.set_dib_color_table(backend->source_context, 0, 0x100, backend->dib_colors);
                        int32_t width = 0;
                        int32_t height = 0;
                        auto *format = static_cast<const uint8_t *>(backend->format_data);
                        std::memcpy(&width, format + 4, sizeof(width));
                        std::memcpy(&height, format + 8, sizeof(height));
                        runtime_media_backend_finalize_api.bit_blt(backend->destination_context, backend->destination_x, backend->destination_y, width, height, backend->source_context,
                            backend->destination_x, backend->destination_y, SRCCOPY);
                    }
                }
                break;
            }
        }
        backend = backend->next;
    }
    runtime_media_backend_finalize_api.release_mutex(runtime_media_backend_mutex);
}

void fail_runtime_animation(RuntimeMediaBackend *backend, uint32_t error)
{
    backend->error_state = error;
    backend->media_flags |= 0x80000000;
    if(backend->animation_callback(backend) == 0)
    {
        runtime_animation_failure_api.post_message(backend->window, 0x7ffe, reinterpret_cast<WPARAM>(backend->identity), static_cast<LPARAM>(0x80000000u));
    }
    backend->media_flags |= 1;
}

void pause_all_runtime_animations()
{
    runtime_animation_control_flags |= 0x1000000;
}

void resume_all_runtime_animations()
{
    runtime_animation_control_flags &= ~0x1000000u;
}

RuntimeAnimationControlResult process_runtime_animation_control(RuntimeAnimationBackend *animation, uint32_t current_time, uint32_t *wait_milliseconds)
{
    RuntimeMediaBackend &backend = animation->base;
    *wait_milliseconds = 0;
    if((backend.media_flags & 0x10000) != 0)
    {
        backend.media_flags |= 0x1000;
        runtime_animation_control_api.destroy_sound(backend.sound_handle);
        if(backend.animation_callback(&backend) == 0)
        {
            runtime_animation_control_api.post_message(backend.window, 0x7ffe, reinterpret_cast<WPARAM>(backend.identity), 0x1000);
        }
        return RuntimeAnimationControlResult::Exit;
    }
    if((backend.media_flags & 0x80000000) != 0)
    {
        return RuntimeAnimationControlResult::Wait;
    }
    if((backend.media_flags & 0x40) != 0)
    {
        const auto *header = static_cast<const uint8_t *>(backend.format_data);
        uint16_t width = 0;
        uint16_t height = 0;
        std::memcpy(&width, header + 8, sizeof(width));
        std::memcpy(&height, header + 10, sizeof(height));
        backend.dirty_left = 0;
        backend.dirty_top = 0;
        backend.dirty_right = width;
        backend.dirty_bottom = height;
        backend.media_flags |= 0x8000;
        backend.animation_callback(&backend);
        backend.dirty_left = 32000;
        backend.dirty_top = 32000;
        backend.dirty_right = 0;
        backend.dirty_bottom = 0;
        backend.media_flags &= ~0x40u;
    }
    if((backend.media_flags & 0x20000) != 0)
    {
        backend.frame_number = 0;
        const uint32_t storage = backend.media_flags & 0x3000000;
        if(storage == 0x1000000)
        {
            animation->source_cursor = animation->data_start;
        }
        else if(storage == 0x2000000)
        {
            runtime_animation_control_api.set_stream_position(backend.stream_record, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(animation->data_start)));
        }
        if(backend.sound_handle != 0)
        {
            runtime_animation_control_api.destroy_sound(backend.sound_handle);
            backend.sound_handle = 0;
        }
        backend.timing_correction = 0;
        const uint32_t old_flags = backend.media_flags;
        backend.media_flags = old_flags & ~0x20000u;
        if((old_flags & 0x2201) == 0x201)
        {
            backend.media_flags = old_flags & 0xfffdfffeu;
        }
    }
    if((backend.media_flags & 1) != 0 || (runtime_animation_control_flags & 0x1000000) != 0)
    {
        if((backend.media_flags & 0x2000) == 0)
        {
            if(backend.sound_handle != 0)
            {
                runtime_animation_control_api.start_sound(backend.sound_handle, 0);
            }
            backend.media_flags |= 0x2000;
            backend.animation_callback(&backend);
            runtime_animation_control_api.post_message(backend.window, 0x7ffe, reinterpret_cast<WPARAM>(backend.identity), 0x2000);
        }
        return RuntimeAnimationControlResult::Wait;
    }
    if((backend.media_flags & 0x2000) != 0)
    {
        if(backend.sound_handle != 0)
        {
            runtime_animation_control_api.stop_sound(backend.sound_handle, 0);
        }
        backend.next_frame_time = 0;
        backend.previous_frame_time = current_time - backend.frame_duration;
        backend.media_flags &= ~0x2000u;
    }
    if(static_cast<int32_t>(current_time) < static_cast<int32_t>(backend.next_frame_time))
    {
        *wait_milliseconds = backend.next_frame_time - current_time;
        return RuntimeAnimationControlResult::Wait;
    }
    return RuntimeAnimationControlResult::DecodeFrame;
}

void schedule_runtime_animation_frame(RuntimeMediaBackend *backend, uint32_t current_time)
{
    if(backend->frame_number > 1)
    {
        backend->timing_correction += (current_time - backend->previous_frame_time) - backend->frame_duration;
    }
    backend->previous_frame_time = current_time;
    uint32_t next_frame_time = current_time;
    if(backend->timing_correction < static_cast<int32_t>(backend->frame_duration))
    {
        uint32_t duration = backend->frame_duration;
        if(backend->timing_correction < 0)
        {
            duration = static_cast<uint32_t>(static_cast<int32_t>(duration << 2) / 3);
        }
        else
        {
            next_frame_time -= backend->timing_correction;
        }
        next_frame_time += duration;
    }
    backend->next_frame_time = next_frame_time;
    if((backend->media_flags & 0x100000) != 0)
    {
        backend->next_frame_time = current_time + backend->frame_duration;
    }
    backend->previous_frame_time += backend->timing_adjustment;
    backend->next_frame_time += backend->timing_adjustment;
}

bool acquire_runtime_animation_frame(RuntimeAnimationBackend *animation)
{
    RuntimeMediaBackend &backend = animation->base;
    const uint32_t storage = backend.media_flags & 0x3000000;
    if(storage == 0x1000000)
    {
        backend.frame_header = animation->source_cursor;
        backend.frame_buffer = static_cast<RuntimeAnimationFrameHeader *>(animation->source_cursor) + 1;
    }
    else if(storage == 0x2000000)
    {
        uint32_t bytes_read = 0;
        runtime_animation_frame_acquire_api.read_record(backend.stream_record, backend.frame_header, 0x10, &bytes_read, 1);
        if(bytes_read != 0x10)
        {
            runtime_animation_frame_acquire_api.fail_animation(&backend, 100);
            return false;
        }
    }
    ++backend.frame_number;
    const auto *frame_header = static_cast<const RuntimeAnimationFrameHeader *>(backend.frame_header);
    if(frame_header->signature != 0xf1fa)
    {
        runtime_animation_frame_acquire_api.fail_animation(&backend, 1);
        return false;
    }
    if((backend.media_flags & 0x2000000) != 0)
    {
        uint32_t payload_size = frame_header->size - sizeof(RuntimeAnimationFrameHeader);
        if(payload_size > 899999)
        {
            payload_size = 0;
        }
        if(payload_size != 0)
        {
            if(backend.allocation_2_active < payload_size)
            {
                void *buffer = nullptr;
                if(backend.allocation_2_active == 0)
                {
                    buffer = runtime_animation_frame_acquire_api.heap_alloc(runtime_media_backend_heap, 0, payload_size);
                }
                else
                {
                    buffer = runtime_animation_frame_acquire_api.heap_realloc(runtime_media_backend_heap, 0, backend.frame_buffer, payload_size);
                }
                if(buffer == nullptr)
                {
                    runtime_animation_frame_acquire_api.fail_animation(&backend, 2);
                    return false;
                }
                backend.allocation_2_active = payload_size;
                backend.frame_buffer = buffer;
            }
            uint32_t bytes_read = 0;
            runtime_animation_frame_acquire_api.read_record(backend.stream_record, backend.frame_buffer, payload_size, &bytes_read, 1);
            if(bytes_read != payload_size)
            {
                runtime_animation_frame_acquire_api.fail_animation(&backend, 100);
                return false;
            }
        }
    }
    animation->source_cursor = backend.frame_buffer;
    return true;
}

void decode_runtime_animation_frame_chunks(RuntimeAnimationBackend *animation)
{
    RuntimeMediaBackend &backend = animation->base;
    animation->source_cursor = backend.frame_buffer;
    backend.media_flags |= 0x10000000;
    backend.animation_callback(&backend);
    const uint16_t chunk_count = static_cast<const RuntimeAnimationFrameHeader *>(backend.frame_header)->chunk_count;
    for(uint32_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
    {
        backend.chunk_header = animation->source_cursor;
        const auto *chunk = static_cast<const RuntimeAnimationChunkHeader *>(backend.chunk_header);
        const uint16_t chunk_type = chunk->type;
        uint32_t payload_size = 0;
        if(chunk_type == 0x10)
        {
            const auto *format = static_cast<const RuntimeAnimationFileHeader *>(backend.format_data);
            payload_size = static_cast<uint32_t>(format->width) * format->height;
        }
        else
        {
            payload_size = chunk->size - sizeof(RuntimeAnimationChunkHeader);
        }
        animation->source_cursor = static_cast<uint8_t *>(animation->source_cursor) + sizeof(RuntimeAnimationChunkHeader);
        bool marks_pixels = true;
        switch(chunk_type)
        {
        case 4:
            runtime_animation_decode_api.decode_palette(&backend);
            backend.media_flags |= 0x4000;
            marks_pixels = false;
            break;
        case 5:
            runtime_animation_decode_api.decode_mvz5(&backend);
            break;
        case 7:
            runtime_animation_decode_api.decode_delta_flc(&backend);
            break;
        case 8:
            runtime_animation_decode_api.decode_mvz8(&backend);
            break;
        case 0xb:
            runtime_animation_decode_api.ignore_chunk_11();
            backend.media_flags |= 0x4000;
            marks_pixels = false;
            break;
        case 0xc:
            runtime_animation_decode_api.ignore_chunk_12();
            break;
        case 0xd:
            runtime_animation_decode_api.ignore_chunk_13();
            break;
        case 0xf:
            runtime_animation_decode_api.decode_byte_run(&backend);
            break;
        case 0x10:
            runtime_animation_decode_api.decode_literal(&backend);
            break;
        default:
            marks_pixels = false;
            break;
        }
        if(marks_pixels)
        {
            backend.media_flags |= 0x8000;
        }
        animation->source_cursor = static_cast<uint8_t *>(animation->source_cursor) + payload_size;
    }
    backend.media_flags |= 0x20000000;
    backend.animation_callback(&backend);
}

void complete_runtime_animation_frame(RuntimeAnimationBackend *animation)
{
    RuntimeMediaBackend &backend = animation->base;
    const int32_t presentation_threshold = static_cast<int32_t>(backend.frame_duration * 4u);
    if((backend.media_flags & 0x100) == 0 && (backend.timing_correction <= presentation_threshold || (backend.media_flags & 0x100000) != 0))
    {
        if(backend.animation_callback(&backend) != 0)
        {
            backend.dirty_left = 32000;
            backend.dirty_top = 32000;
            backend.dirty_right = 0;
            backend.dirty_bottom = 0;
        }
        runtime_animation_completion_api.sleep(0);
    }
    if((backend.media_flags & 0x200) != 0)
    {
        backend.media_flags |= 1;
    }
    const uint16_t total_frames = static_cast<const RuntimeAnimationFileHeader *>(backend.format_data)->frame_count;
    if((backend.media_flags & 0x400) == 0 && total_frames == backend.frame_number)
    {
        backend.media_flags |= 1;
        backend.timing_correction = 0;
        if((backend.media_flags & 0x800) != 0)
        {
            backend.media_flags |= 0x10000;
        }
    }
    if(total_frames < backend.frame_number)
    {
        backend.frame_number = 1;
        backend.timing_correction = 0;
        backend.timing_adjustment = 0;
        backend.media_flags |= 0x40000000;
        backend.animation_callback(&backend);
        runtime_animation_completion_api.post_message(backend.window, 0x7ffe, reinterpret_cast<WPARAM>(backend.identity), 0x40000000);
        const uint32_t storage = backend.media_flags & 0x3000000;
        if(storage == 0x1000000)
        {
            animation->source_cursor = animation->data_end;
        }
        else if(storage == 0x2000000)
        {
            runtime_animation_completion_api.set_stream_position(backend.stream_record, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(animation->data_end)));
        }
    }
}

void process_runtime_animation_audio_chunks(RuntimeAnimationBackend *animation)
{
    RuntimeMediaBackend &backend = animation->base;
    animation->source_cursor = backend.frame_buffer;
    const uint16_t chunk_count = static_cast<const RuntimeAnimationFrameHeader *>(backend.frame_header)->chunk_count;
    for(uint32_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index)
    {
        backend.chunk_header = animation->source_cursor;
        auto *chunk = static_cast<RuntimeAnimationChunkHeader *>(backend.chunk_header);
        const uint16_t chunk_type = chunk->type;
        uint32_t payload_size = 0;
        if(chunk_type == 0x10)
        {
            const auto *format = static_cast<const RuntimeAnimationFileHeader *>(backend.format_data);
            payload_size = static_cast<uint32_t>(format->width) * format->height;
        }
        else
        {
            payload_size = chunk->size - sizeof(RuntimeAnimationChunkHeader);
        }
        animation->source_cursor = chunk + 1;
        if(chunk_type == 0x300)
        {
            RuntimeSoundStatus sound_status{};
            if(backend.sound_handle != 0 && runtime_animation_audio_api.query_sound(backend.sound_handle, &sound_status) != 0 && (backend.media_flags & 0x800000) == 0)
            {
                const uint32_t wait_start = runtime_animation_audio_api.time_get_time();
                bool sound_available = true;
                while(sound_status.playback_marker == 0 && (backend.media_flags & 0x10000) == 0)
                {
                    runtime_animation_audio_api.sleep(0);
                    if(runtime_animation_audio_api.query_sound(backend.sound_handle, &sound_status) == 0)
                    {
                        sound_available = false;
                        break;
                    }
                }
                if(sound_available && sound_status.playback_marker != 0)
                {
                    backend.timing_adjustment = ((wait_start - backend.frame_duration * 5u) - sound_status.playback_marker) / (backend.synchronized_sound_frame - backend.frame_number);
                }
                const uint32_t wait_end = runtime_animation_audio_api.time_get_time();
                backend.previous_frame_time += wait_end - wait_start;
                backend.next_frame_time += wait_end - wait_start;
                const uint32_t storage = backend.media_flags & 0x3000000;
                if(storage == 0x1000000)
                {
                    runtime_animation_audio_api.queue_sound_data(backend.sound_handle, animation->source_cursor, payload_size, 0);
                }
                else if(storage == 0x2000000)
                {
                    uint8_t *destination = static_cast<uint8_t *>(backend.audio_buffer);
                    if((backend.media_flags & 0x400000) == 0)
                    {
                        backend.media_flags |= 0x400000;
                        destination += payload_size;
                    }
                    else
                    {
                        backend.media_flags &= ~0x400000u;
                    }
                    std::memcpy(destination, animation->source_cursor, payload_size);
                    runtime_animation_audio_api.queue_sound_data(backend.sound_handle, destination, payload_size, 0);
                }
                runtime_animation_audio_api.set_playback_marker(backend.sound_handle, 0);
                backend.synchronized_sound_frame = backend.frame_number;
            }
        }
        else if(chunk_type == 0x200)
        {
            RuntimeSoundStatus sound_status{};
            if(backend.sound_handle != 0 && runtime_animation_audio_api.query_sound(backend.sound_handle, &sound_status) != 0 && (backend.media_flags & 0x800000) == 0)
            {
                runtime_animation_audio_api.set_schedule_marker(backend.sound_handle, 0);
                runtime_animation_audio_api.set_playback_marker(backend.sound_handle, 0);
                backend.media_flags |= 0x400000;
                const uint32_t wait_start = runtime_animation_audio_api.time_get_time();
                bool available = true;
                const uint32_t storage = backend.media_flags & 0x3000000;
                if(storage == 0x1000000)
                {
                    backend.audio_buffer = animation->source_cursor;
                }
                else if(storage == 0x2000000)
                {
                    if(backend.allocation_1_active < payload_size)
                    {
                        void *buffer = backend.allocation_1_active == 0 ? runtime_animation_audio_api.heap_alloc(runtime_media_backend_heap, 0, payload_size)
                                                                        : runtime_animation_audio_api.heap_realloc(runtime_media_backend_heap, 0, backend.audio_buffer, payload_size);
                        if(buffer == nullptr)
                        {
                            available = false;
                            runtime_animation_audio_api.destroy_sound(backend.sound_handle);
                            backend.sound_handle = 0;
                        }
                        else
                        {
                            backend.audio_buffer = buffer;
                            backend.allocation_1_active = payload_size;
                        }
                    }
                    if(available)
                    {
                        std::memcpy(backend.audio_buffer, animation->source_cursor, payload_size);
                    }
                }
                if(available)
                {
                    const uint32_t half_size = payload_size >> 1;
                    runtime_animation_audio_api.queue_sound_data(backend.sound_handle, backend.audio_buffer, half_size, 1);
                    runtime_animation_audio_api.queue_sound_data(backend.sound_handle, static_cast<uint8_t *>(backend.audio_buffer) + half_size, half_size, 0);
                    runtime_animation_audio_api.stop_sound(backend.sound_handle, 0);
                    bool sound_available = true;
                    while(sound_status.schedule_marker == 0 && (backend.media_flags & 0x10000) == 0)
                    {
                        runtime_animation_audio_api.sleep(0);
                        if(runtime_animation_audio_api.query_sound(backend.sound_handle, &sound_status) == 0)
                        {
                            sound_available = false;
                            break;
                        }
                    }
                    if(sound_available && sound_status.schedule_marker != 0)
                    {
                        const uint32_t elapsed = sound_status.schedule_marker - wait_start;
                        backend.previous_frame_time += elapsed;
                        backend.next_frame_time += elapsed;
                    }
                }
                backend.synchronized_sound_frame = backend.frame_number;
            }
        }
        else if(chunk_type == 0x400)
        {
            backend.timing_adjustment = 0;
            runtime_animation_audio_api.start_sound(backend.sound_handle, 0);
        }
        else if(chunk_type == 0x600 && (backend.media_flags & 0x800000) == 0)
        {
            auto *sound_chunk = reinterpret_cast<RuntimeAnimationSoundFormatChunk *>(chunk);
            backend.sound_handle = runtime_animation_audio_api.create_sound(&sound_chunk->format);
        }
        animation->source_cursor = static_cast<uint8_t *>(animation->source_cursor) + payload_size;
    }
}

DWORD WINAPI run_runtime_animation_thread(void *backend_pointer)
{
    auto *animation = static_cast<RuntimeAnimationBackend *>(backend_pointer);
    runtime_animation_worker_api.gdi_set_batch_limit(1);
    uint32_t wait_milliseconds = 0;
    for(;;)
    {
        runtime_animation_worker_api.sleep(wait_milliseconds);
        for(;;)
        {
            const uint32_t current_time = runtime_animation_worker_api.time_get_time();
            const RuntimeAnimationControlResult control = process_runtime_animation_control(animation, current_time, &wait_milliseconds);
            if(control == RuntimeAnimationControlResult::Exit)
            {
                runtime_animation_worker_api.exit_thread(0);
                return 0;
            }
            if(control == RuntimeAnimationControlResult::Wait)
            {
                break;
            }
            schedule_runtime_animation_frame(&animation->base, current_time);
            if(!acquire_runtime_animation_frame(animation))
            {
                wait_milliseconds = 0;
                break;
            }
            process_runtime_animation_audio_chunks(animation);
            decode_runtime_animation_frame_chunks(animation);
            complete_runtime_animation_frame(animation);
        }
    }
}

int32_t present_runtime_animation_frame(RuntimeMediaBackend *backend)
{
    uint32_t flags = backend->media_flags;
    if((flags & 0x10000000) != 0)
    {
        backend->media_flags = flags & ~0x10000000u;
        return 1;
    }
    if((flags & 0x20000000) != 0)
    {
        backend->media_flags = flags & ~0x20000000u;
        return 1;
    }
    if((flags & 0x4000) != 0)
    {
        if(backend->destination_bits_per_pixel == 8)
        {
            if((flags & 0x10) == 0)
            {
                if((flags & 0x20) == 0)
                {
                    runtime_animation_present_api.animate_palette(backend->destination_palette, 0, 0xec, backend->palette_entries);
                }
                else
                {
                    runtime_animation_present_api.set_palette_entries(backend->destination_palette, 0, 0xec, backend->palette_entries);
                    runtime_animation_present_api.realize_palette(backend->destination_context);
                    backend->media_flags &= ~0x20u;
                }
                runtime_animation_present_api.set_dib_color_table(backend->source_context, 0, 0x100, backend->dib_colors);
            }
        }
        else if((flags & 0x10) == 0)
        {
            runtime_animation_present_api.set_dib_color_table(backend->source_context, 0, 0x100, backend->dib_colors);
        }
    }
    if((backend->media_flags & 0x8000) != 0)
    {
        const int x = backend->destination_x + backend->dirty_left;
        const int y = backend->destination_y + backend->dirty_top;
        const int width = backend->dirty_right - backend->dirty_left;
        const int height = backend->dirty_bottom - backend->dirty_top;
        if((backend->media_flags & 0x200000) == 0)
        {
            runtime_animation_present_api.bit_blt(backend->destination_context, x, y, width, height, backend->source_context, x, y, SRCCOPY);
        }
        else if((backend->media_flags & 0x200000) == 0x200000)
        {
            runtime_animation_present_api.stretch_blt(backend->destination_context, backend->destination_x + backend->dirty_left * 2, backend->destination_y + backend->dirty_top * 2, width * 2,
                height * 2, backend->source_context, x, y, width, height, SRCCOPY);
        }
    }
    backend->media_flags &= 0xffff3fffu;
    return 1;
}

void decode_runtime_animation_palette(RuntimeMediaBackend *backend)
{
    auto *source = static_cast<const uint8_t *>(reinterpret_cast<RuntimeAnimationBackend *>(backend)->source_cursor);
    uint16_t packet_count = 0;
    std::memcpy(&packet_count, source, sizeof(packet_count));
    source += 2;
    auto *palette_entries = reinterpret_cast<uint8_t *>(backend->palette_entries);
    auto *dib_colors = reinterpret_cast<uint8_t *>(backend->dib_colors);
    do
    {
        const uint8_t skip = *source++;
        uint16_t color_count = *source++;
        if(color_count == 0)
        {
            color_count = 0x100;
        }
        else
        {
            palette_entries += skip * 4;
            dib_colors += skip * 4;
        }
        do
        {
            const uint8_t red = *source++;
            const uint8_t green = *source++;
            const uint8_t blue = *source++;
            palette_entries[0] = red;
            palette_entries[1] = green;
            palette_entries[2] = blue;
            palette_entries[3] = 1;
            dib_colors[0] = blue;
            dib_colors[1] = green;
            dib_colors[2] = red;
            dib_colors[3] = 0;
            palette_entries += 4;
            dib_colors += 4;
            --color_count;
        } while(color_count != 0);
        --packet_count;
    } while(packet_count != 0);
    build_runtime_palette_index_remap(backend);
}

// Shared implementation for the two MVZ chunk decoders.
void decode_runtime_animation_mvz(RuntimeMediaBackend *backend, bool packet_counted)
{
    const auto *source = static_cast<const uint8_t *>(reinterpret_cast<RuntimeAnimationBackend *>(backend)->source_cursor);
    const auto read_word = [](const uint8_t *value)
    {
        uint16_t result = 0;
        std::memcpy(&result, value, sizeof(result));
        return result;
    };
    const uint16_t area_width = read_word(source);
    const uint16_t area_height = read_word(source + 2);
    const uint16_t area_x = read_word(source + 4);
    const uint16_t area_y = read_word(source + 6);
    source += 0x10;
    const uint16_t origin_x = backend->destination_x;
    const uint16_t origin_y = backend->destination_y;
    const uint16_t destination_stride = backend->destination_stride;
    uint8_t *destination_base = backend->destination_pixels;
    const uint32_t scale_x = backend->scale_x;
    const uint32_t scale_y = backend->scale_y;
    const uint32_t left = area_x * scale_x;
    const uint32_t top = area_y * scale_y;
    const uint32_t output_width = area_width * scale_x;
    const uint32_t output_height = area_height * scale_y;
    backend->dirty_left = static_cast<int32_t>(left);
    backend->dirty_top = static_cast<int32_t>(top);
    const uint32_t right = left + output_width;
    const uint32_t bottom = top + output_height;
    backend->dirty_right = static_cast<int32_t>(right);
    backend->dirty_bottom = static_cast<int32_t>(bottom);
    const bool copy_indices = (backend->media_flags & 0x4000000) != 0;
    const auto map_value = [backend, copy_indices](uint8_t value) { return copy_indices ? value : backend->palette_remap[value]; };
    const uint32_t destination_x = origin_x + left;
    const uint32_t destination_y = origin_y + top;
    for(uint32_t y = 0; y < area_height; ++y)
    {
        uint32_t packet_count = 0;
        if(packet_counted)
        {
            packet_count = read_word(source);
            source += 2;
            if(packet_count == 0)
            {
                continue;
            }
        }
        uint32_t x = 0;
        do
        {
            if(packet_counted)
            {
                x += *source++;
                --packet_count;
            }
            const uint8_t control_byte = *source++;
            const uint8_t type = control_byte >> 6;
            const uint8_t control = control_byte & 0x3f;
            uint32_t count = 0;
            if(type == 0)
            {
                count = control;
                const uint8_t value = map_value(*source++);
                for(uint32_t repeat_y = 0; repeat_y < scale_y; ++repeat_y)
                {
                    uint8_t *destination = destination_base + (destination_y + y * scale_y + repeat_y) * destination_stride + destination_x + x * scale_x;
                    std::memset(destination, value, count * scale_x);
                }
            }
            else if(type == 1)
            {
                count = control;
                for(uint32_t repeat_y = 0; repeat_y < scale_y; ++repeat_y)
                {
                    uint8_t *destination = destination_base + (destination_y + y * scale_y + repeat_y) * destination_stride + destination_x + x * scale_x;
                    for(uint32_t index = 0; index < count; ++index)
                    {
                        const uint8_t value = map_value(source[index]);
                        std::memset(destination, value, scale_x);
                        destination += scale_x;
                    }
                }
                source += count;
            }
            else
            {
                uint32_t source_x = 0;
                uint32_t source_y = 0;
                if(type == 2)
                {
                    const uint32_t value = static_cast<uint32_t>(control) << 8 | *source++;
                    count = 3;
                    source_x = value & 0x1ff;
                    source_y = y - ((value >> 9) & 0x1f);
                }
                else
                {
                    const uint32_t value = static_cast<uint32_t>(control) << 16 | static_cast<uint32_t>(source[0]) << 8 | source[1];
                    source += 2;
                    count = value & 0x1f;
                    source_x = (value >> 5) & 0x3ff;
                    source_y = y - ((value >> 15) & 0x7f);
                }
                for(uint32_t repeat_y = 0; repeat_y < scale_y; ++repeat_y)
                {
                    uint8_t *destination = destination_base + (destination_y + y * scale_y + repeat_y) * destination_stride + destination_x + x * scale_x;
                    const uint8_t *copy_source = destination_base + (destination_y + source_y * scale_y + repeat_y) * destination_stride + destination_x + source_x * scale_x;
                    for(uint32_t index = 0; index < count * scale_x; ++index)
                    {
                        *destination++ = *copy_source++;
                    }
                }
            }
            x += count;
        } while(packet_counted ? packet_count != 0 : x < area_width);
    }
}

void decode_runtime_animation_mvz8(RuntimeMediaBackend *backend)
{
    decode_runtime_animation_mvz(backend, true);
}

void decode_runtime_animation_mvz5(RuntimeMediaBackend *backend)
{
    decode_runtime_animation_mvz(backend, false);
}

void decode_runtime_animation_literal(RuntimeMediaBackend *backend)
{
    const auto *header = static_cast<const uint8_t *>(backend->format_data);
    uint16_t source_width = 0;
    uint16_t source_height = 0;
    std::memcpy(&source_width, header + 8, sizeof(source_width));
    std::memcpy(&source_height, header + 10, sizeof(source_height));
    const uint16_t origin_x = backend->destination_x;
    const uint16_t origin_y = backend->destination_y;
    const uint16_t destination_stride = backend->destination_stride;
    uint8_t *destination_base = backend->destination_pixels;
    const uint32_t horizontal_scale = backend->scale_x;
    const uint32_t effective_horizontal_scale = horizontal_scale < 2 ? 1 : horizontal_scale;
    const uint32_t output_width = source_width * effective_horizontal_scale;
    const uint32_t output_height = source_height * backend->scale_y;
    const bool copy_indices = (backend->media_flags & 0x4000000) != 0;
    const auto *source = static_cast<const uint8_t *>(reinterpret_cast<RuntimeAnimationBackend *>(backend)->source_cursor);
    uint8_t *destination = destination_base + origin_y * destination_stride + origin_x;
    const uint32_t destination_row_adjustment = destination_stride - output_width;
    backend->dirty_left = 0;
    backend->dirty_top = 0;
    backend->dirty_right = static_cast<int32_t>(output_width);
    backend->dirty_bottom = static_cast<int32_t>(output_height);
    for(uint16_t row = 0; row < source_height; ++row)
    {
        const uint8_t *source_row = source;
        for(uint32_t repeat_y = 0; repeat_y < backend->scale_y; ++repeat_y)
        {
            source = source_row;
            for(uint16_t column = 0; column < source_width; ++column)
            {
                const uint8_t value = copy_indices ? *source++ : backend->palette_remap[*source++];
                for(uint32_t repeat_x = 0; repeat_x < effective_horizontal_scale; ++repeat_x)
                {
                    *destination++ = value;
                }
            }
            destination += destination_row_adjustment;
        }
    }
}

void decode_runtime_animation_byte_run(RuntimeMediaBackend *backend)
{
    const auto *header = static_cast<const uint8_t *>(backend->format_data);
    uint16_t source_width = 0;
    uint16_t source_height = 0;
    std::memcpy(&source_width, header + 8, sizeof(source_width));
    std::memcpy(&source_height, header + 10, sizeof(source_height));
    const uint16_t origin_x = backend->destination_x;
    const uint16_t origin_y = backend->destination_y;
    const uint16_t destination_stride = backend->destination_stride;
    uint8_t *destination_base = backend->destination_pixels;
    const uint32_t effective_horizontal_scale = backend->scale_x < 2 ? 1 : backend->scale_x;
    const uint32_t output_width = source_width * effective_horizontal_scale;
    const uint32_t output_height = source_height * backend->scale_y;
    const bool copy_indices = (backend->media_flags & 0x4000000) != 0;
    const auto map_value = [backend, copy_indices](uint8_t value) { return copy_indices ? value : backend->palette_remap[value]; };
    const auto *source = static_cast<const uint8_t *>(reinterpret_cast<RuntimeAnimationBackend *>(backend)->source_cursor);
    uint8_t *destination = destination_base + origin_y * destination_stride + origin_x;
    const uint32_t destination_row_adjustment = destination_stride - output_width;
    backend->dirty_left = 0;
    backend->dirty_top = 0;
    backend->dirty_right = static_cast<int32_t>(output_width);
    backend->dirty_bottom = static_cast<int32_t>(output_height);
    for(uint16_t row = 0; row < source_height; ++row)
    {
        const uint8_t *source_row = source;
        for(uint32_t repeat_y = 0; repeat_y < backend->scale_y; ++repeat_y)
        {
            source = source_row + 1;
            uint32_t produced = 0;
            do
            {
                const int8_t encoded_count = static_cast<int8_t>(*source++);
                const uint32_t count = encoded_count < 0 ? static_cast<uint32_t>(-encoded_count) : static_cast<uint32_t>(encoded_count);
                if(encoded_count < 0)
                {
                    for(uint32_t index = 0; index < count; ++index)
                    {
                        const uint8_t value = map_value(*source++);
                        for(uint32_t repeat_x = 0; repeat_x < effective_horizontal_scale; ++repeat_x)
                        {
                            *destination++ = value;
                        }
                    }
                }
                else
                {
                    const uint8_t value = map_value(*source++);
                    for(uint32_t index = 0; index < count * effective_horizontal_scale; ++index)
                    {
                        *destination++ = value;
                    }
                }
                produced += count;
            } while(produced < source_width);
            destination += destination_row_adjustment;
        }
    }
}

void decode_runtime_animation_delta_flc(RuntimeMediaBackend *backend)
{
    const auto *header = static_cast<const uint8_t *>(backend->format_data);
    uint16_t source_width = 0;
    std::memcpy(&source_width, header + 8, sizeof(source_width));
    const uint16_t origin_x = backend->destination_x;
    const uint16_t origin_y = backend->destination_y;
    const uint16_t destination_stride = backend->destination_stride;
    uint8_t *destination_base = backend->destination_pixels;
    const uint32_t scale_x = backend->scale_x < 2 ? 1 : backend->scale_x;
    const uint32_t scale_y = backend->scale_y;
    const uint32_t output_width = source_width * scale_x;
    const bool copy_indices = (backend->media_flags & 0x4000000) != 0;
    const auto map_value = [backend, copy_indices](uint8_t value) { return copy_indices ? value : backend->palette_remap[value]; };
    const auto read_signed_word = [](const uint8_t *value)
    {
        int16_t result = 0;
        std::memcpy(&result, value, sizeof(result));
        return result;
    };
    const auto *source = static_cast<const uint8_t *>(reinterpret_cast<RuntimeAnimationBackend *>(backend)->source_cursor);
    uint16_t remaining_lines = 0;
    std::memcpy(&remaining_lines, source, sizeof(remaining_lines));
    source += 2;
    int32_t current_y = 0;
    do
    {
        for(;;)
        {
            const int16_t control = read_signed_word(source);
            if(control >= 0 || (static_cast<uint16_t>(control) & 0x4000) == 0)
            {
                break;
            }
            source += 2;
            current_y += -static_cast<int32_t>(control) * static_cast<int32_t>(scale_y);
        }
        const uint8_t *line_source = source;
        const uint8_t *line_end = source;
        for(uint32_t repeat_y = 0; repeat_y < scale_y; ++repeat_y)
        {
            source = line_source;
            int16_t packet_count = read_signed_word(source);
            source += 2;
            uint8_t *destination_row = destination_base + (origin_y + current_y) * destination_stride + origin_x;
            if(packet_count < 0)
            {
                const uint8_t value = map_value(static_cast<uint8_t>(packet_count));
                std::memset(destination_row + output_width - scale_x, value, scale_x);
                backend->dirty_right = static_cast<int32_t>(output_width);
                packet_count = read_signed_word(source);
                source += 2;
            }
            if(current_y < backend->dirty_top)
            {
                backend->dirty_top = current_y;
            }
            int32_t current_x = 0;
            for(int16_t packet = 0; packet < packet_count; ++packet)
            {
                current_x += static_cast<uint32_t>(*source++) * scale_x;
                if(current_x < backend->dirty_left)
                {
                    backend->dirty_left = current_x;
                }
                const int8_t encoded_count = static_cast<int8_t>(*source++);
                if(encoded_count >= 0)
                {
                    const uint32_t count = static_cast<uint32_t>(encoded_count) * 2;
                    for(uint32_t index = 0; index < count; ++index)
                    {
                        const uint8_t value = map_value(*source++);
                        std::memset(destination_row + current_x, value, scale_x);
                        current_x += scale_x;
                    }
                }
                else
                {
                    const uint32_t count = static_cast<uint32_t>(-encoded_count);
                    const uint8_t first = map_value(*source++);
                    const uint8_t second = map_value(*source++);
                    for(uint32_t index = 0; index < count; ++index)
                    {
                        std::memset(destination_row + current_x, first, scale_x);
                        current_x += scale_x;
                        std::memset(destination_row + current_x, second, scale_x);
                        current_x += scale_x;
                    }
                }
            }
            if(backend->dirty_right < current_x)
            {
                backend->dirty_right = current_x;
            }
            ++current_y;
            line_end = source;
        }
        source = line_end;
        --remaining_lines;
    } while(remaining_lines != 0);
    if(backend->dirty_bottom < current_y)
    {
        backend->dirty_bottom = current_y;
    }
}

void ignore_runtime_animation_chunk_11() {}

void ignore_runtime_animation_chunk_12() {}

void ignore_runtime_animation_chunk_13() {}

uint32_t destroy_runtime_media_backend(void *identity)
{
    RuntimeMediaBackend *backend = acquire_runtime_media_backend(identity);
    uint32_t result = 0;
    if(backend != nullptr)
    {
        runtime_media_backend_api.wait_for_single_object(runtime_media_backend_mutex, INFINITE);
        if(backend->previous == nullptr)
        {
            runtime_media_backend_head = backend->next;
        }
        else
        {
            backend->previous->next = backend->next;
        }
        if(backend->next == nullptr)
        {
            runtime_media_backend_tail = backend->previous;
        }
        else
        {
            backend->next->previous = backend->previous;
        }
        result = 1;
        runtime_media_backend_api.release_mutex(runtime_media_backend_mutex);
        if(backend->type == 0xaa)
        {
            if(backend->allocation_1_active != 0)
            {
                result = runtime_media_backend_api.heap_free(runtime_media_backend_heap, 0, backend->audio_buffer) & 1;
            }
            if(backend->allocation_2_active != 0)
            {
                result &= runtime_media_backend_api.heap_free(runtime_media_backend_heap, 0, backend->frame_buffer);
            }
        }
        result &= runtime_media_backend_api.heap_free(runtime_media_backend_heap, 0, backend);
    }
    return result;
}


} // namespace gag
