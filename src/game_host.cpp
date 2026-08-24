#include "game_host.h"
#include <SDL3/SDL.h>
#include "host_events.h"
#include "portable_string.h"
#include "runtime_internal.h"
#include "xtet/asset_decoders.h"

namespace gag
{
namespace
{
bool portable_runtime_input_enabled{};

void drain_runtime_keyboard_input()
{
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_TEXT_INPUT);
}

void forward_xtet_host_event(xtet::HostEventType type, uint32_t result_type, const void *data, uint32_t size)
{
    HostXtEtEvent event;
    switch(type)
    {
    case xtet::HostEventType::terminate:
        event.type = HostXtEtEventType::terminate;
        break;
    case xtet::HostEventType::pause:
        event.type = HostXtEtEventType::pause;
        break;
    case xtet::HostEventType::resume:
        event.type = HostXtEtEventType::resume;
        break;
    case xtet::HostEventType::result:
        event.type = HostXtEtEventType::result;
        event.result_type = result_type;
        if(data != nullptr && size != 0)
        {
            const auto *bytes = static_cast<const uint8_t *>(data);
            event.result_data.assign(bytes, bytes + size);
        }
        send_host_event(std::move(event));
        return;
    }
    post_host_event(std::move(event));
}
} // namespace

void use_portable_runtime_input(bool enabled)
{
    portable_runtime_input_enabled = enabled;
}

void handle_runtime_xtet_host_event(const HostXtEtEvent &event)
{
    switch(event.type)
    {
    case HostXtEtEventType::terminate:
        clear_runtime_flag_01000000();
        unload_runtime_game_dll();
        break;
    case HostXtEtEventType::pause:
        if(!gagboy_startup_mode)
        {
            set_runtime_flag_01000000();
        }
        break;
    case HostXtEtEventType::resume:
        if(!gagboy_startup_mode)
        {
            clear_runtime_flag_01000000();
        }
        break;
    case HostXtEtEventType::result:
        if(event.result_data.size() < sizeof(runtime_display_context.game_result_data))
        {
            runtime_display_context.game_result_type = event.result_type;
            std::memcpy(runtime_display_context.game_result_data, event.result_data.data(), event.result_data.size());
        }
        break;
    }
}

uint32_t create_runtime_game_sound(const xtet::PcmFormat *format)
{
    if(format == nullptr)
    {
        return 0;
    }
    const RuntimePcmFormat runtime_format{ format->format_tag, format->channel_count, format->samples_per_second, format->average_bytes_per_second, format->block_alignment, format->bits_per_sample };
    return create_runtime_sound_handle(&runtime_format);
}

void unload_runtime_game_dll()
{
    runtime_game_lifecycle_api.enter_critical_section(&runtime_game_dll_mutex);
    if((runtime_scene_control_flags & 0x10) != 0)
    {
        runtime_game_integration_api.shutdown();
        if((runtime_scene_control_flags & 0x1000) != 0)
        {
            runtime_game_lifecycle_api.leave_runtime_state();
        }
        runtime_game_input_handler = nullptr;
        runtime_game_dll_execute = nullptr;
        runtime_scene_control_flags = (runtime_scene_control_flags & ~0x10u) | 0x20;
    }
    runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_mutex);
}

bool load_and_initialize_runtime_game_dll(const char *path)
{
    if(path == nullptr)
    {
        throw std::invalid_argument("Unsupported /GAME library: <null>");
    }
    char file_name[0x104]{};
    copy_file_name_from_path(file_name, path);
    if(compare_ascii_case_insensitive(file_name, "XTETDLL.DLL") != 0)
    {
        throw std::invalid_argument(std::string("Unsupported /GAME library: ") + path);
    }
    std::string sfs_name(file_name);
    sfs_name.replace(sfs_name.size() - 4, 4, ".SFS");

    bool result = false;
    runtime_game_lifecycle_api.enter_critical_section(&runtime_game_dll_mutex);
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        result = true;
        runtime_game_lifecycle_api.update_resource_host(path, 0);
        static constexpr char loading_scene[] = "m_DEF_LOAD";
        runtime_game_lifecycle_api.activate_comment_scene(loading_scene);
        runtime_game_input_handler = runtime_game_integration_api.input_handler;
        runtime_game_dll_execute = runtime_game_integration_api.execute;
        runtime_scene_control_flags = (runtime_scene_control_flags | 0x10) & ~0x20u;
        try
        {
            xtet::set_host_event_callback(forward_xtet_host_event);
            xtet::set_input_drain_callback(drain_runtime_keyboard_input);
            runtime_game_integration_api.initialize(&runtime_game_host_context, runtime_game_host_callbacks, sfs_name.c_str());
        }
        catch(...)
        {
            runtime_game_integration_api.shutdown();
            runtime_game_input_handler = nullptr;
            runtime_game_dll_execute = nullptr;
            runtime_scene_control_flags = (runtime_scene_control_flags & ~0x10u) | 0x20;
            runtime_game_lifecycle_api.deactivate_comment_scene(loading_scene);
            runtime_game_lifecycle_api.reset_byte_queue();
            runtime_game_lifecycle_api.reset_pair_queue();
            runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_mutex);
            throw;
        }
        runtime_game_lifecycle_api.deactivate_comment_scene(loading_scene);
        runtime_game_lifecycle_api.reset_byte_queue();
        runtime_game_lifecycle_api.reset_pair_queue();
    }
    runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_mutex);
    return result;
}

uint32_t stop_runtime_game_dll()
{
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        return 0;
    }
    runtime_game_dll_execute(1);
    const uint32_t start = runtime_game_dll_dispatch_api.time_get_time();
    while((runtime_scene_control_flags & 0x10) != 0 && runtime_game_dll_dispatch_api.time_get_time() - start < 5000)
    {
        runtime_game_dll_dispatch_api.sleep(10);
    }
    return 1;
}

uint32_t pause_runtime_game_dll()
{
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        return 0;
    }
    runtime_game_dll_execute(2);
    return 1;
}

uint32_t resume_runtime_game_dll()
{
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        return 0;
    }
    runtime_game_dll_execute(4);
    return 1;
}

void update_runtime_pointer_position(int32_t x, int32_t y)
{
    RuntimeResourceObject *record = nullptr;
    RuntimeNamedNode *node = nullptr;
    if((runtime_scene_control_flags & 0x1000) == 0)
    {
        const RuntimeThreadId thread = runtime_pointer_position_api.get_current_thread_id();
        for(;;)
        {
            bool contended = false;
            runtime_pointer_position_api.enter_critical_section(&runtime_named_lock_mutex);
            node = runtime_pointer_position_api.find_child(runtime_named_lock_parent_identity, current_runtime_scene_identity);
            if(node != nullptr)
            {
                record = static_cast<RuntimeResourceObject *>(node->identity);
                if(record->recursion_count == 0 || record->owner_thread == thread)
                {
                    ++record->recursion_count;
                    record->owner_thread = thread;
                }
                else
                {
                    contended = true;
                }
            }
            runtime_pointer_position_api.leave_critical_section(&runtime_named_lock_mutex);
            if(!contended)
            {
                break;
            }
            runtime_pointer_position_api.sleep(5);
        }
        if(node != nullptr)
        {
            const int32_t hotspot_x = static_cast<int32_t>(record->requested_width);
            const int32_t hotspot_y = static_cast<int32_t>(record->requested_height);
            runtime_pointer_position_api.offset_scene(record->scene_identifier, (x - hotspot_x) - record->x, (y - hotspot_y) - record->y);
            record->x = x - hotspot_x;
            record->y = y - hotspot_y;
        }
    }
    runtime_pointer_x = x;
    runtime_pointer_y = y;
    if(node != nullptr)
    {
        --record->recursion_count;
    }
}

bool handle_runtime_input_event(const RuntimeInputEvent &event)
{
    if((runtime_scene_control_flags & 0x10) != 0)
    {
        const uint32_t result = runtime_game_input_handler(event);
        if(result == xtet::kDispatchBusy && event.type != RuntimeInputType::close)
        {
            return false;
        }
        if(result == 0x10000)
        {
            return true;
        }
    }

    const auto packed_position = [&]() { return static_cast<uint32_t>(static_cast<uint16_t>(event.x)) | static_cast<uint32_t>(static_cast<uint16_t>(event.y)) << 16; };
    switch(event.type)
    {
    case RuntimeInputType::pointer_move:
        update_runtime_pointer_position(event.x, event.y);
        enqueue_runtime_pair(static_cast<uint32_t>(RuntimeQueuedInputType::pointer_move), packed_position());
        break;
    case RuntimeInputType::pointer_leave:
        enter_runtime_state_1000();
        enqueue_runtime_pair(static_cast<uint32_t>(RuntimeQueuedInputType::pointer_move), 0xffffffff);
        break;
    case RuntimeInputType::button_down:
        if(event.button == RuntimeMouseButton::left)
        {
            enqueue_runtime_pair(static_cast<uint32_t>(RuntimeQueuedInputType::left_button_down), packed_position());
        }
        else if(event.button == RuntimeMouseButton::right)
        {
            enqueue_runtime_pair(static_cast<uint32_t>(RuntimeQueuedInputType::right_button_down), packed_position());
        }
        break;
    case RuntimeInputType::button_up:
        if(event.button == RuntimeMouseButton::left)
        {
            enqueue_runtime_pair(static_cast<uint32_t>(RuntimeQueuedInputType::left_button_up), packed_position());
        }
        break;
    case RuntimeInputType::text:
        for(unsigned char character : event.text)
        {
            enqueue_runtime_byte(character);
        }
        break;
    case RuntimeInputType::key_down:
        if(event.key == 0x08 || event.key == 0x09 || event.key == 0x0d || event.key == 0x1b)
        {
            enqueue_runtime_byte(static_cast<uint8_t>(event.key));
        }
        break;
    default:
        break;
    }
    return true;
}

} // namespace gag
