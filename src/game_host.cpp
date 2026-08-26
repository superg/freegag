#include "game_host.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include "host_events.h"
#include "portable_string.h"
#include "runtime_internal.h"
#include "xtet/asset_decoders.h"

namespace freegag
{
constexpr uint64_t runtime_animation_input_pump_interval_nanoseconds = 1'000'000;

bool portable_runtime_input_enabled{};
std::atomic_uint64_t runtime_keyboard_input_drain_timestamp{};
std::atomic_uint64_t runtime_keyboard_input_drain_request{};
std::atomic_uint64_t runtime_keyboard_input_drain_completion{};
std::atomic_uint64_t runtime_keyboard_input_drain_release{};
bool runtime_keyboard_input_drain_completion_pending{};

void drain_runtime_keyboard_input()
{
    ++runtime_keyboard_input_drain_request;
    if(SDL_IsMainThread())
        complete_runtime_keyboard_input_drain();
    else
        post_host_event(HostKeyboardInputDrainEvent{});
}

void complete_runtime_keyboard_input_drain()
{
    SDL_PumpEvents();
    runtime_keyboard_input_drain_timestamp = SDL_GetTicksNS();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_TEXT_INPUT);
    runtime_keyboard_input_drain_completion = runtime_keyboard_input_drain_request.load();
    runtime_keyboard_input_drain_completion_pending = true;
}

void finish_runtime_keyboard_input_drain()
{
    if(!runtime_keyboard_input_drain_completion_pending)
        return;
    SDL_PumpEvents();
    runtime_keyboard_input_drain_timestamp = SDL_GetTicksNS();
    SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_TEXT_INPUT);
    runtime_keyboard_input_drain_release = runtime_keyboard_input_drain_completion.load();
    runtime_keyboard_input_drain_completion_pending = false;
}

bool should_discard_runtime_keyboard_input(uint64_t timestamp)
{
    const uint64_t drain_timestamp = runtime_keyboard_input_drain_timestamp.load();
    const uint64_t request = runtime_keyboard_input_drain_request.load();
    if(runtime_keyboard_input_drain_release.load() < request)
        return true;
    return drain_timestamp != 0 && timestamp <= drain_timestamp;
}

void wait_for_runtime_game_animation(uint32_t milliseconds)
{
    if(!SDL_IsMainThread())
    {
        SDL_Delay(milliseconds);
        return;
    }

    const uint64_t deadline = SDL_GetTicksNS() + SDL_MS_TO_NS(milliseconds);
    while(true)
    {
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_TEXT_INPUT);
        const uint64_t current_time = SDL_GetTicksNS();
        if(current_time >= deadline)
            return;
        const uint64_t remaining = deadline - current_time;
        SDL_DelayNS(remaining < runtime_animation_input_pump_interval_nanoseconds ? remaining : runtime_animation_input_pump_interval_nanoseconds);
    }
}

void forward_xtet_host_event(xtet::HostEventType type, uint32_t result_type, const void *data, uint32_t size)
{
    HostXtEtEvent event;
    switch(type)
    {
    case xtet::HostEventType::TERMINATE:
        event.type = HostXtEtEventType::TERMINATE;
        break;
    case xtet::HostEventType::PAUSE:
        event.type = HostXtEtEventType::PAUSE;
        break;
    case xtet::HostEventType::RESUME:
        event.type = HostXtEtEventType::RESUME;
        break;
    case xtet::HostEventType::RESULT:
        event.type = HostXtEtEventType::RESULT;
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

void use_portable_runtime_input(bool enabled)
{
    portable_runtime_input_enabled = enabled;
}

void handle_runtime_xtet_host_event(const HostXtEtEvent &event)
{
    switch(event.type)
    {
    case HostXtEtEventType::TERMINATE:
        graphics_host_flags &= ~RUNTIME_HOST_PAUSED;
        unload_runtime_game_dll();
        break;
    case HostXtEtEventType::PAUSE:
        graphics_host_flags |= RUNTIME_HOST_PAUSED;
        break;
    case HostXtEtEventType::RESUME:
        graphics_host_flags &= ~RUNTIME_HOST_PAUSED;
        break;
    case HostXtEtEventType::RESULT:
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
        return 0;
    const RuntimePcmFormat runtime_format{ format->format_tag, format->channel_count, format->samples_per_second, format->average_bytes_per_second, format->block_alignment, format->bits_per_sample };
    return create_runtime_sound_handle(&runtime_format);
}

void unload_runtime_game_dll()
{
    lock_runtime_mutex(&runtime_game_dll_mutex);
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) != 0)
    {
        xtet::shutdown_game();
        if((runtime_scene_control_flags & RUNTIME_HOST_SCENE_SWITCH_DEFERRED) != 0)
            resume_runtime_state();
        runtime_game_input_handler = nullptr;
        runtime_game_dll_execute = nullptr;
        runtime_scene_control_flags = (runtime_scene_control_flags & ~RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) | RUNTIME_HOST_EMBEDDED_GAME_INACTIVE;
    }
    unlock_runtime_mutex(&runtime_game_dll_mutex);
}

bool load_and_initialize_runtime_game_dll(const char *path)
{
    if(path == nullptr)
        throw std::invalid_argument("Unsupported /GAME library: <null>");
    char file_name[0x104]{};
    copy_file_name_from_path(file_name, path);
    if(compare_ascii_case_insensitive(file_name, "XTETDLL.DLL") != 0)
        throw std::invalid_argument(std::string("Unsupported /GAME library: ") + path);
    std::string sfs_name(file_name);
    sfs_name.replace(sfs_name.size() - 4, 4, ".SFS");

    bool result = false;
    lock_runtime_mutex(&runtime_game_dll_mutex);
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) == 0)
    {
        result = true;
        update_runtime_resource_host(path, 0);
        static constexpr char loading_scene[] = "m_DEF_LOAD";
        activate_default_comment_scene(loading_scene);
        runtime_game_input_handler = xtet::dispatch_game_input;
        runtime_game_dll_execute = xtet::execute_game_command;
        runtime_scene_control_flags = (runtime_scene_control_flags | RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) & ~RUNTIME_HOST_EMBEDDED_GAME_INACTIVE;
        try
        {
            xtet::set_host_event_callback(forward_xtet_host_event);
            xtet::set_input_drain_callback(drain_runtime_keyboard_input);
            const xtet::GameHostServices services{ invalidate_game_framebuffer_rect, create_runtime_game_sound, destroy_runtime_sound_handle, queue_runtime_sound_data, pause_runtime_sound,
                resume_runtime_sound, wait_for_runtime_game_animation };
            xtet::initialize_game(&runtime_game_host_context, services, sfs_name.c_str());
        }
        catch(...)
        {
            xtet::shutdown_game();
            runtime_game_input_handler = nullptr;
            runtime_game_dll_execute = nullptr;
            runtime_scene_control_flags = (runtime_scene_control_flags & ~RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) | RUNTIME_HOST_EMBEDDED_GAME_INACTIVE;
            deactivate_default_comment_scene(loading_scene);
            reset_runtime_byte_queue();
            reset_runtime_input_queue();
            unlock_runtime_mutex(&runtime_game_dll_mutex);
            throw;
        }
        deactivate_default_comment_scene(loading_scene);
        reset_runtime_byte_queue();
        reset_runtime_input_queue();
    }
    unlock_runtime_mutex(&runtime_game_dll_mutex);
    return result;
}

uint32_t stop_runtime_game_dll()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) == 0)
        return 0;
    runtime_game_dll_execute(1);
    const uint32_t start = runtime_milliseconds();
    while((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) != 0 && runtime_milliseconds() - start < 5000)
        runtime_sleep(10);
    return 1;
}

uint32_t pause_runtime_game_dll()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) == 0)
        return 0;
    runtime_game_dll_execute(2);
    return 1;
}

uint32_t resume_runtime_game_dll()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) == 0)
        return 0;
    runtime_game_dll_execute(4);
    return 1;
}

void update_runtime_pointer_position(int32_t x, int32_t y)
{
    std::lock_guard lock(runtime_pointer_scene_mutex);
    RuntimeResourceObject *record = nullptr;
    RuntimeNamedNode *node = nullptr;
    if((runtime_scene_control_flags & RUNTIME_HOST_SCENE_SWITCH_DEFERRED) == 0)
    {
        const RuntimeThreadId thread = runtime_thread_id();
        for(;;)
        {
            bool contended = false;
            lock_runtime_mutex(&runtime_named_lock_mutex);
            node = find_runtime_named_child(runtime_named_lock_parent_identity, current_runtime_scene_identity);
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
            unlock_runtime_mutex(&runtime_named_lock_mutex);
            if(!contended)
                break;
            runtime_sleep(5);
        }
        if(node != nullptr)
        {
            const int32_t hotspot_x = static_cast<int32_t>(record->requested_width);
            const int32_t hotspot_y = static_cast<int32_t>(record->requested_height);
            offset_display_scene_node(record->scene_identifier, (x - hotspot_x) - record->x, (y - hotspot_y) - record->y);
            record->x = x - hotspot_x;
            record->y = y - hotspot_y;
        }
    }
    runtime_pointer_x = x;
    runtime_pointer_y = y;
    if(node != nullptr)
        --record->recursion_count;
}

void handle_runtime_input_event(const RuntimeInputEvent &event)
{
    if((runtime_scene_control_flags & RUNTIME_HOST_EMBEDDED_GAME_ACTIVE) != 0)
    {
        const uint32_t result = runtime_game_input_handler(event);
        if(result == xtet::kDispatchBusy && event.type != RuntimeInputType::CLOSE)
            return;
        if(result == CDF_ERROR_STORAGE_FAILURE)
            return;
    }

    const auto packed_position = [&]() { return static_cast<uint32_t>(static_cast<uint16_t>(event.x)) | static_cast<uint32_t>(static_cast<uint16_t>(event.y)) << 16; };
    switch(event.type)
    {
    case RuntimeInputType::POINTER_MOVE:
        update_runtime_pointer_position(event.x, event.y);
        enqueue_runtime_input(RuntimeQueuedInputType::POINTER_MOVE, packed_position());
        break;
    case RuntimeInputType::POINTER_LEAVE:
        suspend_runtime_state();
        enqueue_runtime_input(RuntimeQueuedInputType::POINTER_MOVE, RUNTIME_POINTER_POSITION_OUTSIDE);
        break;
    case RuntimeInputType::BUTTON_DOWN:
        if(event.button == RuntimeMouseButton::LEFT)
            enqueue_runtime_input(RuntimeQueuedInputType::LEFT_BUTTON_DOWN, packed_position());
        else if(event.button == RuntimeMouseButton::RIGHT)
            enqueue_runtime_input(RuntimeQueuedInputType::RIGHT_BUTTON_DOWN, packed_position());
        break;
    case RuntimeInputType::BUTTON_UP:
        if(event.button == RuntimeMouseButton::LEFT)
            enqueue_runtime_input(RuntimeQueuedInputType::LEFT_BUTTON_UP, packed_position());
        break;
    case RuntimeInputType::TEXT:
        for(unsigned char character : event.text)
            enqueue_runtime_byte(character);
        break;
    case RuntimeInputType::KEY_DOWN:
        if(event.key == 0x08 || event.key == 0x09 || event.key == 0x0d || event.key == 0x1b)
            enqueue_runtime_byte(static_cast<uint8_t>(event.key));
        break;
    default:
        break;
    }
}

} // namespace freegag
