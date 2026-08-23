#include "game_host.h"
#include "host_events.h"
#include "runtime_internal.h"
#include "xtet/asset_decoders.h"

namespace gag
{
namespace
{
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
    runtime_game_lifecycle_api.enter_critical_section(&runtime_game_dll_critical_section);
    if((runtime_scene_control_flags & 0x10) != 0)
    {
        runtime_game_integration_api.shutdown();
        if((runtime_scene_control_flags & 0x1000) != 0)
        {
            runtime_game_lifecycle_api.leave_runtime_state();
        }
        runtime_game_dll_window_procedure = nullptr;
        runtime_game_dll_execute = nullptr;
        runtime_scene_control_flags = (runtime_scene_control_flags & ~0x10u) | 0x20;
    }
    runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_critical_section);
}

bool load_and_initialize_runtime_game_dll(const char *path)
{
    if(path == nullptr)
    {
        throw std::invalid_argument("Unsupported /GAME library: <null>");
    }
    char file_name[MAX_PATH]{};
    copy_file_name_from_path(file_name, path);
    if(_stricmp(file_name, "XTETDLL.DLL") != 0)
    {
        throw std::invalid_argument(std::string("Unsupported /GAME library: ") + path);
    }
    std::string sfs_name(file_name);
    sfs_name.replace(sfs_name.size() - 4, 4, ".SFS");

    bool result = false;
    runtime_game_lifecycle_api.enter_critical_section(&runtime_game_dll_critical_section);
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        result = true;
        runtime_game_lifecycle_api.update_resource_host(path, 0);
        static constexpr char loading_scene[] = "m_DEF_LOAD";
        runtime_game_lifecycle_api.activate_comment_scene(loading_scene);
        runtime_game_dll_window_procedure = runtime_game_integration_api.window_procedure;
        runtime_game_dll_execute = runtime_game_integration_api.execute;
        runtime_scene_control_flags = (runtime_scene_control_flags | 0x10) & ~0x20u;
        try
        {
            xtet::set_host_event_callback(forward_xtet_host_event);
            runtime_game_integration_api.initialize(&runtime_game_host_context, runtime_game_host_callbacks, sfs_name.c_str());
        }
        catch(...)
        {
            runtime_game_integration_api.shutdown();
            runtime_game_dll_window_procedure = nullptr;
            runtime_game_dll_execute = nullptr;
            runtime_scene_control_flags = (runtime_scene_control_flags & ~0x10u) | 0x20;
            runtime_game_lifecycle_api.deactivate_comment_scene(loading_scene);
            runtime_game_lifecycle_api.reset_byte_queue();
            runtime_game_lifecycle_api.reset_pair_queue();
            runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_critical_section);
            throw;
        }
        runtime_game_lifecycle_api.deactivate_comment_scene(loading_scene);
        runtime_game_lifecycle_api.reset_byte_queue();
        runtime_game_lifecycle_api.reset_pair_queue();
    }
    runtime_game_lifecycle_api.leave_critical_section(&runtime_game_dll_critical_section);
    return result;
}

uint32_t stop_runtime_game_dll()
{
    if((runtime_scene_control_flags & 0x10) == 0)
    {
        return 0;
    }
    runtime_game_dll_execute(1);
    const DWORD start = runtime_game_dll_dispatch_api.time_get_time();
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
        const DWORD thread = runtime_pointer_position_api.get_current_thread_id();
        for(;;)
        {
            bool contended = false;
            runtime_pointer_position_api.enter_critical_section(&runtime_named_lock_critical_section);
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
            runtime_pointer_position_api.leave_critical_section(&runtime_named_lock_critical_section);
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

LRESULT CALLBACK runtime_game_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    bool modern_windows_cursor_reentry = false;
    if(message == WM_LBUTTONDOWN)
    {
        modern_windows_fullscreen_toggle_latched = false;
    }
    if(message == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT)
    {
        runtime_game_window_api.set_cursor(nullptr);
        return TRUE;
    }
    if(message == WM_MOUSEMOVE)
    {
        runtime_game_window_api.set_cursor(nullptr);
        if(!modern_windows_game_cursor_tracking)
        {
            modern_windows_cursor_reentry = true;
            TRACKMOUSEEVENT tracking{ sizeof(tracking), TME_LEAVE, window, 0 };
            modern_windows_game_cursor_tracking = runtime_game_window_api.track_mouse_event(&tracking) != FALSE;
        }
    }
    if(message == WM_MOUSELEAVE)
    {
        modern_windows_game_cursor_tracking = false;
        runtime_game_window_api.enter_runtime_state();
        runtime_game_window_api.enqueue_pair(WM_MOUSEMOVE, static_cast<uint32_t>(MAKELPARAM(0xffff, 0xffff)));
        return 0;
    }
    if(message == WM_DESTROY)
    {
        modern_windows_game_cursor_tracking = false;
    }
    const auto input_lparam = [&]() -> LPARAM
    {
        if(modern_windows_presentation_is_scaled()
            && (message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_MBUTTONDOWN
                || message == WM_MBUTTONUP))
        {
            const int32_t x = map_modern_windows_presentation_coordinate(static_cast<uint16_t>(LOWORD(lparam)), modern_windows_presentation_state.viewport_width, runtime_game_host_context.width);
            const int32_t y = map_modern_windows_presentation_coordinate(static_cast<uint16_t>(HIWORD(lparam)), modern_windows_presentation_state.viewport_height, runtime_game_host_context.height);
            return static_cast<LPARAM>(MAKELONG(x, y));
        }
        return lparam;
    };
    const auto translated_lparam = [&]() -> LPARAM
    {
        const LPARAM input = input_lparam();
        const uint16_t x = static_cast<uint16_t>(static_cast<uint16_t>(LOWORD(input)) + static_cast<uint16_t>(runtime_game_host_context.x_offset));
        const uint16_t y = static_cast<uint16_t>(static_cast<uint16_t>(HIWORD(input)) + static_cast<uint16_t>(runtime_game_host_context.y_offset));
        return static_cast<LPARAM>(MAKELONG(x, y));
    };

    uint32_t game_dispatch_result = 0;
    if((runtime_scene_control_flags & 0x10) != 0)
    {
        game_dispatch_result = runtime_game_dll_window_procedure(window, message, wparam, input_lparam());
        if(game_dispatch_result == xtet::kDispatchBusy && message != WM_DESTROY)
        {
            PostMessageA(window, message, wparam, lparam);
            return 0;
        }
    }
    if(game_dispatch_result == 0x10000)
    {
        if(message == 0x30f)
        {
            return 1;
        }
        if(message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
        {
            runtime_game_window_api.send_message(runtime_game_main_window, message, wparam, translated_lparam());
            if(modern_windows_cursor_reentry)
            {
                leave_runtime_state_1000();
            }
        }
        return 0;
    }

    if(message == WM_ERASEBKGND)
    {
        return 1;
    }
    if(message == WM_PAINT)
    {
        PAINTSTRUCT paint;
        runtime_game_window_api.begin_paint(window, &paint);
        runtime_game_window_api.end_paint(window, &paint);
        repaint_sdl_presenter();
        return 0;
    }
    else if(message == WM_CHAR)
    {
        runtime_game_window_api.enqueue_byte(static_cast<uint8_t>(wparam));
    }
    else if(message == WM_MOUSEMOVE || message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
    {
        if(message == WM_MOUSEMOVE)
        {
            const LPARAM input = input_lparam();
            runtime_game_window_api.update_pointer_position(static_cast<uint16_t>(LOWORD(input)), static_cast<uint16_t>(HIWORD(input)));
        }
        runtime_game_window_api.send_message(runtime_game_main_window, message, wparam, translated_lparam());
        if(modern_windows_cursor_reentry)
        {
            leave_runtime_state_1000();
        }
        runtime_game_window_api.enqueue_pair(message, static_cast<uint32_t>(input_lparam()));
        return 0;
    }
    else if(message == 0x30f)
    {
        runtime_game_window_api.enqueue_message(0x30f);
        return 1;
    }
    else if(message == 0x310)
    {
        return 0;
    }
    else if(message == 0x311)
    {
        if(reinterpret_cast<HWND>(wparam) != window)
        {
            runtime_game_window_api.enqueue_message(0x311);
        }
        return 0;
    }
    return runtime_game_window_api.default_window_procedure(window, message, wparam, lparam);
}


} // namespace gag
