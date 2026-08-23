#include "application.h"
#include "host_events.h"
#include "runtime_internal.h"

namespace gag
{
namespace
{
constexpr UINT host_event_wake_message = WM_APP + 0x41;

bool wake_host_event_loop(void *context)
{
    return PostMessageA(static_cast<HWND>(context), host_event_wake_message, 0, 0) != FALSE;
}

HostEventResult handle_application_host_event(const HostApplicationEvent &event, HostEventCompletion *completion, ApplicationState *state)
{
    RuntimeTreeNode *tree = nullptr;
    const HostStateFieldQuery *query = nullptr;
    if(const auto *value = std::get_if<RuntimeTreeNode *>(&event.payload))
    {
        tree = *value;
    }
    else
    {
        query = std::get_if<HostStateFieldQuery>(&event.payload);
    }

    uint32_t value;
    if(handle_scripted_save_load_message(event.command, state))
    {
        return uint32_t{};
    }
    switch(event.command)
    {
    case 1:
        set_application_inactive_flags(state);
        break;
    case 0x3ea:
        if(query != nullptr)
        {
            value = (state->saved_flags & 0x100000) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
        }
        break;
    case 0x3eb:
    case 0x3ec:
    case 0x3ed:
        if(query != nullptr)
        {
            const uint32_t mask = event.command == 0x3eb ? 0x800000 : (event.command == 0x3ec ? 0x200000 : 0x400000);
            value = (state->saved_flags & mask) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
        }
        break;
    case 0x3f2:
        if((state->flags & 0x4000) != 0 && query != nullptr)
        {
            value = (state->flags & 0x20) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
        }
        break;
    case 0x3fc:
    case 0x406:
        if(query != nullptr)
        {
            const uint32_t mask = event.command == 0x3fc ? 0x1000 : 0x02000000;
            value = (state->flags & mask) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
        }
        break;
    case 0x456:
        if(query != nullptr)
        {
            value = (state->flags & 0x4000) != 0 ? 0x7000000 : 0x3000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
        }
        break;
    case 0x7d2:
    case 0x7d3:
        request_scripted_save_load_screen(event.command == 0x7d3 ? SaveLoadScreenMode::save : SaveLoadScreenMode::load, state);
        break;
    case 0x7d1:
    case 0x7d4:
    case 0x7da:
        acknowledge_host_event(completion, uint32_t{ 1 });
        if(event.command == 0x7d1)
            SendMessageA(state->window, WM_COMMAND, 0x8860, 0);
        else if(event.command == 0x7d4)
            SendMessageA(state->window, WM_COMMAND, 0x8870, 0);
        else
        {
            SendMessageA(state->window, WM_COMMAND, (state->flags & 0x20) == 0 ? 0x8900 : 0x8910, 0);
        }
        break;
    case 0x7e4:
        SendMessageA(state->window, WM_COMMAND, 0x8850, 0);
        break;
    case 0x7ee:
        SendMessageA(state->window, WM_COMMAND, 0x8820, 0);
        break;
    case 0xbc2:
        if((state->flags & 0x80000) == 0)
        {
            if((state->flags & 0x800000) == 0)
            {
                state->saved_memory = capture_game_bitmap(state->game_context, nullptr, 1);
                state->script_state = reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
            }
            state->saved_flags = state->flags;
            state->flags = (state->flags & 0xfffbffff) | 0x80000;
        }
        else
        {
            state->saved_flags = (state->saved_flags & 0xffcfffff) | (state->flags & 0x300000);
        }
        state->saved_flags &= 0xffbfffff;
        break;
    case 0xbcc:
        if((state->flags & 0x80000) != 0)
        {
            if(state->saved_memory != nullptr)
            {
                free_heap_memory(state->saved_memory);
                state->saved_memory = nullptr;
            }
            state->flags &= 0xfff7ffff;
            state->saved_flags = 0;
        }
        break;
    case 0xbd6:
        if(query != nullptr)
        {
            value = (state->flags & 0x40000) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query->object_name.c_str(), query->field_name.c_str(), &value, 1);
            state->flags &= 0xfffbffff;
        }
        break;
    case 0x10000000:
        PostMessageA(state->window, WM_CLOSE, 0, 0);
        break;
    case 0x30000000:
        finish_credits_state(state, tree);
        break;
    case 0x40000000:
        process_state_activation(state, tree);
        break;
    case 0x60000000:
        enter_runtime_state_1000();
        if((state->flags & 0x10) != 0)
        {
            state->flags &= 0xffffffef;
            save_game_screenshot(state->window, state->game_context);
            clear_runtime_flag_01000000();
            clear_application_lock_flag(state);
        }
        if((state->flags & 0x40) != 0)
        {
            state->flags &= 0xffffffbf;
            restore_application_display(state);
            state->flags |= 0x40000;
        }
        clear_runtime_command_state();
        leave_runtime_state_1000();
        break;
    case 0x90000000:
        if((state->flags & 0x200) == 0)
        {
            clear_runtime_display();
            construct_runtime_resource(state->installed_version, 0, 0, 0, 0, 0, 0, 0x200);
            construct_runtime_resource(state->startup_config, 0, 0, 0, 0, 0, 0, 0);
            set_script_runtime_flags(1, (state->flags & 0x02000000) == 0);
            state->flags &= 0xfff7ffff;
            clear_runtime_command_state();
            break;
        }
        if((state->validation_flags & 0x100) != 0 && state->script_state != 0)
        {
            append_string(state->installation_path, auto_save_file_name);
            const bool saved = write_synchronized_cdf_package(state->installation_path, nullptr, nullptr, reinterpret_cast<void *>(state->script_state));
            (void)saved;
        }
        save_runtime_settings(state);
        shutdown_graphics_host();
        close_host_events();
        DestroyWindow(state->window);
        clear_runtime_command_state();
        break;
    case 0xa0000000:
        if(const auto *path = std::get_if<std::string>(&event.payload))
        {
            char validated_path[0x104]{};
            copy_string(validated_path, path->c_str());
            validate_startup_environment(state, validated_path, 4);
            return std::string(state->installed_version);
        }
        break;
    case 0xb0000000:
        if(const auto *path = std::get_if<std::string>(&event.payload))
        {
            char file_name[0x104]{};
            copy_string(file_name, path->c_str());
            copy_file_name_from_path(file_name, file_name);
            return std::string(file_name);
        }
        break;
    case 0xc0000000:
    case 0xd0000000:
    case 0x04000000:
        if((state->flags & 0x2000) == 0)
        {
            state->flags |= 0x2000;
            PostMessageA(state->window, WM_CLOSE, 0, 0);
        }
        break;
    default:
        break;
    }
    return uint32_t{};
}

HostEventResult handle_host_event(const HostEvent &event, HostEventCompletion *completion, void *context)
{
    auto *state = static_cast<ApplicationState *>(context);
    if(const auto *application_event = std::get_if<HostApplicationEvent>(&event))
    {
        return handle_application_host_event(*application_event, completion, state);
    }
    if(std::holds_alternative<HostPresentPendingFramesEvent>(event))
    {
        drain_sdl_presenter_frames();
    }
    else if(const auto *xtet_event = std::get_if<HostXtEtEvent>(&event))
    {
        handle_runtime_xtet_host_event(*xtet_event);
    }
    return {};
}
} // namespace

void set_runtime_script_property(uint32_t property, void *, void *value)
{
    switch(property)
    {
    case 1:
        graphics_host_value_1 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
        break;
    case 2:
        graphics_host_value_2 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
        break;
    case 4:
        graphics_host_value_3 = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)) & 0xf;
        break;
    case 5:
        runtime_script_property_set_api.select_resource(static_cast<char *>(value));
        break;
    case 7:
        runtime_script_property_set_api.release_memory_resource(static_cast<char *>(value));
        break;
    case 8:
        runtime_script_property_set_api.set_property_value(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)));
        break;
    case 12:
        runtime_resource_host_mode = static_cast<int32_t>(reinterpret_cast<intptr_t>(value));
        break;
    case 13:
        if(runtime_state_1000_count == 0)
        {
            runtime_script_property_set_api.enter_state_1000();
            runtime_scene_control_flags |= 0x4000;
        }
        ++runtime_state_1000_count;
        break;
    case 14:
        if(runtime_state_1000_count == 1)
        {
            runtime_scene_control_flags &= 0xffffbfff;
            runtime_script_property_set_api.leave_state_1000();
        }
        if(runtime_state_1000_count != 0)
        {
            --runtime_state_1000_count;
        }
        break;
    case 16:
        if(runtime_state_4_count == 0)
        {
            runtime_scene_control_flags |= 4;
        }
        ++runtime_state_4_count;
        break;
    case 32:
        if(runtime_state_4_count == 1)
        {
            runtime_scene_control_flags &= 0xfffffffb;
        }
        if(runtime_state_4_count != 0)
        {
            --runtime_state_4_count;
        }
        break;
    case 64:
        runtime_script_property_set_api.destroy_resource_tree(value);
        break;
    case 80:
    case 96:
        send_application_event(HostApplicationCommand::runtime_failure);
        break;
    }
}

void get_runtime_script_property(uint32_t property, void **value, void *result)
{
    switch(property)
    {
    case 1:
        *static_cast<uint32_t *>(result) = graphics_host_value_1;
        break;
    case 2:
        *static_cast<uint32_t *>(result) = graphics_host_value_2;
        break;
    case 4:
        *static_cast<uint32_t *>(result) = graphics_host_value_3;
        break;
    case 5:
    {
        char path[260];
        runtime_script_property_get_api.copy_string(path, runtime_graphics_resource_directory);
        HostEventResult event_result = send_application_event(HostApplicationCommand::extract_file_name, std::string(path));
        if(const auto *file_name = std::get_if<std::string>(&event_result))
        {
            runtime_script_property_get_api.copy_string(path, file_name->c_str());
        }
        runtime_script_property_get_api.copy_string(static_cast<char *>(result), path);
        break;
    }
    case 6:
    {
        void *data;
        int32_t storage;
        runtime_script_property_get_api.load_resource(static_cast<const char *>(*value), &data, static_cast<uint32_t *>(result), &storage, 0x20000000);
        *value = data;
        break;
    }
    case 8:
        *static_cast<uint32_t *>(result) = runtime_script_property_get_api.get_property_value();
        break;
    case 9:
        *static_cast<int32_t *>(result) = runtime_pointer_x;
        break;
    case 10:
        *static_cast<int32_t *>(result) = runtime_pointer_y;
        break;
    case 11:
        *static_cast<uint32_t *>(result) = runtime_script_property_get_api.query_frame_number(*value);
        break;
    case 12:
        *static_cast<int32_t *>(result) = runtime_resource_host_mode;
        break;
    }
}

GraphicsHostInitializationResult *initialize_graphics_host(HINSTANCE instance, HWND parent, int x, int y, int16_t width, uint16_t height, uint32_t flags)
{
    if((runtime_scene_control_flags & 0x800) != 0)
    {
        return &graphics_host_state;
    }

    runtime_display_context = {};
    runtime_graphics_instance = nullptr;
    std::memset(runtime_graphics_resource_directory, 0, sizeof(runtime_graphics_resource_directory));
    std::memset(runtime_transition_palette, 0, sizeof(runtime_transition_palette));
    std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
    graphics_host_state = {};
    runtime_game_host_context = {};
    graphics_script_runtime_root = {};
    std::memset(runtime_game_host_callbacks, 0, sizeof(runtime_game_host_callbacks));
    bool initialized = graphics_host_api.initialize_media() != 0;
    if(initialized)
    {
        initialized = graphics_host_api.initialize_async() != 0;
    }
    if(initialized)
    {
        initialized = graphics_host_api.initialize_generic() != 0;
    }
    if(initialized)
    {
        runtime_resource_heap = graphics_host_api.heap_create(0, 0, 0);
        initialized = runtime_resource_heap != nullptr;
    }

    HWND child = nullptr;
    if(initialized)
    {
        runtime_game_host_context.x_offset = static_cast<uint32_t>(x);
        runtime_game_host_context.y_offset = static_cast<uint32_t>(y);
        runtime_game_host_context.width = static_cast<uint16_t>(width + 3) & 0xfffc;
        runtime_game_host_context.height = static_cast<uint16_t>(height);
        graphics_host_state.message_window = reinterpret_cast<uintptr_t>(parent);
        runtime_display_context.window = parent;
        runtime_graphics_instance = instance;

        WNDCLASSA window_class{};
        window_class.style = CS_OWNDC;
        window_class.lpfnWndProc = runtime_game_window_procedure;
        window_class.hInstance = instance;
        window_class.lpszClassName = "Graphical System Child";
        if(graphics_host_api.register_class(&window_class) != 0)
        {
            child = graphics_host_api.create_window_ex(0, "Graphical System Child", nullptr, WS_CHILD, x, y, runtime_game_host_context.width, runtime_game_host_context.height, parent, nullptr,
                instance, nullptr);
            graphics_host_state.capture_window = child;
        }
        initialized = child != nullptr;
    }

    if(initialized)
    {
        POINT point;
        graphics_host_api.get_cursor_position(&point);
        graphics_host_api.screen_to_client(child, &point);
        if(static_cast<int32_t>(runtime_game_host_context.width) < point.x)
        {
            point.x = runtime_game_host_context.width - 1;
        }
        if(static_cast<int32_t>(runtime_game_host_context.height) < point.y)
        {
            point.y = runtime_game_host_context.height - 1;
        }
        if(point.x < 0)
        {
            point.x = 0;
        }
        if(point.y < 0)
        {
            point.y = 0;
        }
        runtime_pointer_x = point.x;
        runtime_pointer_y = point.y;
        initialized = graphics_host_api.initialize_display(child, flags & 0x300000) == 0;
    }

    runtime_target_flags = flags;
    if(!initialized)
    {
        return nullptr;
    }

    graphics_host_value_1 = 6;
    runtime_resource_host_mode = 0x6a4;
    graphics_host_value_2 = 5;
    graphics_host_value_3 = 5;
    graphics_script_runtime_root.self = &graphics_script_runtime_root;
    const auto set_property = &set_runtime_script_property;
    const auto get_property = &get_runtime_script_property;
    std::memcpy(&graphics_script_runtime_root.set_property, &set_property, sizeof(set_property));
    std::memcpy(&graphics_script_runtime_root.get_property, &get_property, sizeof(get_property));
    graphics_script_runtime_root.heap = graphics_host_api.heap_create(0, 0, 0);
    if(graphics_script_runtime_root.heap == nullptr)
    {
        return nullptr;
    }

    graphics_host_api.set_script_root(&graphics_script_runtime_root);
    runtime_resource_cache_parent_identity = graphics_host_api.get_or_create_named_node("OpenMemoryFilesList");
    graphics_host_api.set_named_node_enabled(runtime_resource_cache_parent_identity, 1);
    runtime_media_objects_parent_identity = graphics_host_api.get_or_create_named_node("MMediaObjectsList");
    graphics_host_api.set_named_node_enabled(runtime_media_objects_parent_identity, 1);
    CRITICAL_SECTION *critical_sections[]{ &runtime_display_context.byte_queue_critical_section, &runtime_display_context.pair_queue_critical_section,
        &runtime_display_context.message_queue_critical_section, &runtime_display_context.resource_critical_section, &runtime_display_context.path_critical_section };
    for(CRITICAL_SECTION *section : critical_sections)
    {
        graphics_host_api.initialize_critical_section(section);
    }

    runtime_game_host_callbacks[0] = reinterpret_cast<void *>(&invalidate_game_framebuffer_rect);
    runtime_game_host_callbacks[1] = reinterpret_cast<void *>(&create_runtime_game_sound);
    runtime_game_host_callbacks[2] = reinterpret_cast<void *>(&destroy_runtime_sound_handle);
    runtime_game_host_callbacks[3] = reinterpret_cast<void *>(&queue_runtime_sound_data);
    runtime_game_host_callbacks[4] = reinterpret_cast<void *>(&start_runtime_sound);
    runtime_game_host_callbacks[5] = reinterpret_cast<void *>(&stop_runtime_sound);
    runtime_game_host_callbacks[6] = reinterpret_cast<void *>(&set_runtime_sound_loop_value);
    runtime_game_host_callbacks[7] = reinterpret_cast<void *>(&create_runtime_animation_backend);
    runtime_game_host_callbacks[8] = reinterpret_cast<void *>(&configure_runtime_animation_backend);
    runtime_game_host_callbacks[9] = reinterpret_cast<void *>(&create_runtime_bitmap_backend);
    runtime_game_host_callbacks[10] = reinterpret_cast<void *>(&configure_runtime_bitmap_backend);
    runtime_game_host_callbacks[11] = reinterpret_cast<void *>(&finalize_runtime_media_backend);
    runtime_game_host_callbacks[12] = reinterpret_cast<void *>(&set_runtime_media_backend_scale);
    runtime_game_host_callbacks[14] = reinterpret_cast<void *>(&stop_runtime_animation_backend);
    runtime_game_host_callbacks[15] = reinterpret_cast<void *>(&destroy_runtime_media_backend);
    runtime_game_host_callbacks[16] = reinterpret_cast<void *>(&acquire_runtime_media_backend);
    runtime_game_host_callbacks[17] = reinterpret_cast<void *>(&get_locked_runtime_media_extension);
    runtime_game_host_callbacks[18] = reinterpret_cast<void *>(&release_runtime_media_backend_lock);
    runtime_game_host_callbacks[19] = &runtime_resource_host;
    runtime_game_host_callbacks[20] = reinterpret_cast<void *>(&open_async_file_record);
    runtime_game_host_callbacks[21] = reinterpret_cast<void *>(&close_async_file_record);
    runtime_game_host_callbacks[22] = reinterpret_cast<void *>(&set_async_file_position);
    runtime_game_host_callbacks[23] = reinterpret_cast<void *>(&read_async_file_record);
    runtime_game_host_callbacks[24] = reinterpret_cast<void *>(&get_async_file_position);
    runtime_game_host_callbacks[25] = reinterpret_cast<void *>(&get_async_file_size);
    runtime_game_host_callbacks[26] = reinterpret_cast<void *>(&open_cdf_archive);
    runtime_game_host_callbacks[27] = reinterpret_cast<void *>(&read_cdf_entry);
    runtime_game_host_callbacks[28] = reinterpret_cast<void *>(&close_cdf_archive);
    runtime_game_host_callbacks[29] = reinterpret_cast<void *>(&get_cdf_entry_name_by_index);
    runtime_game_host_callbacks[30] = reinterpret_cast<void *>(&get_cdf_entry_flags);
    runtime_game_host_callbacks[31] = reinterpret_cast<void *>(&get_cdf_entry_count);
    runtime_game_host_callbacks[32] = reinterpret_cast<void *>(&get_cdf_index_data_size);
    runtime_game_host_callbacks[33] = reinterpret_cast<void *>(&get_cdf_entry_size);
    runtime_game_host_callbacks[34] = reinterpret_cast<void *>(&open_runtime_cdf_entry_stream);
    graphics_host_api.show_window(child, SW_SHOWNORMAL);
    runtime_scene_control_flags |= 0x800;
    return &graphics_host_state;
}

uint32_t shutdown_graphics_host()
{
    const uint32_t display_result = graphics_host_shutdown_api.shutdown_display();
    uint32_t result = 0;
    if(display_result != 0)
    {
        const uint32_t generic_result = graphics_host_shutdown_api.shutdown_generic_backend();
        const uint32_t async_result = graphics_host_shutdown_api.shutdown_async_files();
        const uint32_t media_result = graphics_host_shutdown_api.shutdown_media_backend();
        const uint32_t subsystem_result = display_result & generic_result & async_result & media_result;
        graphics_host_shutdown_api.shutdown_presenter();
        if(subsystem_result != 0)
        {
            CRITICAL_SECTION *critical_sections[]{ &runtime_display_context.byte_queue_critical_section, &runtime_display_context.pair_queue_critical_section,
                &runtime_display_context.message_queue_critical_section, &runtime_display_context.resource_critical_section, &runtime_display_context.path_critical_section };
            for(CRITICAL_SECTION *section : critical_sections)
            {
                graphics_host_shutdown_api.delete_critical_section(section);
            }
            result = subsystem_result & static_cast<uint32_t>(graphics_host_shutdown_api.heap_destroy(runtime_resource_heap));
            graphics_host_shutdown_api.destroy_window(graphics_host_state.capture_window);
            if(result != 0)
            {
                runtime_display_context = {};
                runtime_graphics_instance = nullptr;
                std::memset(runtime_graphics_resource_directory, 0, sizeof(runtime_graphics_resource_directory));
                std::memset(runtime_transition_palette, 0, sizeof(runtime_transition_palette));
                std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
                graphics_host_state = {};
                runtime_game_host_context = {};
                graphics_script_runtime_root = {};
                std::memset(runtime_game_host_callbacks, 0, sizeof(runtime_game_host_callbacks));
                runtime_scene_control_flags &= 0xfffff7ff;
            }
        }
    }
    return result;
}



void clear_runtime_display()
{
    if(clear_runtime_display_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        clear_runtime_display_api.set_clip_rectangle(nullptr);
        clear_runtime_display_api.operate_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
        clear_runtime_display_api.release_display_lock();
        DisplayRectangle rectangle{ 0, 0, runtime_game_host_context.width, runtime_game_host_context.height };
        clear_runtime_display_api.update_root_region(nullptr, &rectangle, 0);
    }
}



uint32_t enable_borderless_fullscreen(ApplicationState *state)
{
    state->flags |= 0x4000;
    return state->flags & 0x4000;
}

int validate_startup_environment(ApplicationState *state, const char *requested_archive, uint32_t stages)
{
    if((state->validation_flags & 0x80000000) == 0)
    {
        state->validation_flags |= stages;
    }

    if((stages & 1) != 0 && validation_api.find_window("FlcAppClassNT", nullptr) != nullptr)
    {
        return 0;
    }

    if((stages & 2) != 0)
    {
        validation_api.load_preferences(state);

        char path[MAX_PATH];
        WIN32_FIND_DATAA find_data;
        copy_string(path, state->installation_path);
        append_string(path, auto_save_file_name);
        HANDLE find = validation_api.find_first_file(path, &find_data);
        if(find == INVALID_HANDLE_VALUE)
        {
            state->flags |= 0x200000;
        }
        else
        {
            validation_api.find_close(find);
        }

        copy_string(path, state->installation_path);
        append_string(path, "*");
        append_string(path, ".GSF");
        find = validation_api.find_first_file(path, &find_data);
        if(find == INVALID_HANDLE_VALUE)
        {
            state->flags |= 0x100000;
        }
        else
        {
            validation_api.find_close(find);
        }
    }

    if((stages & 4) != 0 && state->archive_context == nullptr)
    {
        WIN32_FIND_DATAA find_data{};
        HANDLE find = validation_api.find_first_file(requested_archive, &find_data);
        if(find == INVALID_HANDLE_VALUE)
        {
            if((stages & 0x20) != 0)
            {
                validation_api.message_box(state->window, application_message(state, 15), state->message_table, MB_ICONERROR);
            }
            return 0;
        }
        validation_api.find_close(find);
        copy_string(state->installed_version, requested_archive);
    }
    else if((stages & 0x80) != 0 && state->archive_context == nullptr)
    {
        copy_string(state->installed_version, requested_archive);
    }
    else if(state->archive_context != nullptr)
    {
        state->installed_version[0] = '\0';
        copy_string(state->installed_version, requested_archive);
    }

    if((stages & 0x200) != 0)
    {
        validation_api.enable_borderless_fullscreen(state);
        if((stages & 0x400) != 0)
        {
            state->flags |= 0x20;
            state->flags &= 0xffffff7f;
        }
    }
    return 1;
}


// Application messages use fixed-capacity slots so existing table indexing remains stable.
char application_message_table[23][0x104] = { "GAG", "File", "Options", "View", "Load Game", "Save Game", "New Game", "Resume Game", "Credits", "Exit", "Comments", "Mute Sound", "Full Screen",
    "Window", "Application initialization error !", "Unable to open data file...\n\nMake sure you insert one of the CD's\ninto your CD drive!",
    "Internal application error...\n\nMake sure your CD disk is inserted into the drive\nis clean enough and not scratched!", "Registry problem...\n\nYou should run 'Setup' to install the game!",
    "Registry problem...\n\nInvalid registry information\nTry to run 'Setup' to reinstall the game!",
    "Registry problem...\n\nPrevious or demo version detected\nYou should run 'Setup' to reinstall the game!", "CD drive speed is not high enough for this application!\nRun anyway?",
    "This application requires minimum 256-color display mode...", "This application requires 256-color or HI-color display mode..." };

constexpr char gagboy_startup_script[] = R"([CFG]
event=e_START /PRELOAD:GAGBOY;

[GAGBOY]
flags=NOSAVE NOCOMMENT PAL_NOADJUST;
mouse=TET /FILE:K_ghand.256:0 /FILE:K_ukaz.bmp:1 /F:NOPAL /POS:7,11;
mouse=TNM /FILE:K_gnone.256:0 /FILE:K_none.bmp:1 /F:NOPAL;
command=Go;
object=GAGBoy Score::0;
image=Background /FILE:VE-GBNEW.BMP /F:NOPAL /INVERT_NOPAL /F:PRIMARY;
zone=Global /RECT:0,0,639,479 /COMM:Go /MOUSE:TNM /P:10;
zone=Start /POS:149,49,88,84 /COMM:Go /MOUSE:TET /P:20;
zone=Action1 /POS:180,185,45,200 /COMM:Go /MOUSE:TET /P:20;
zone=Action2 /POS:505,90,45,200 /COMM:Go /MOUSE:TET /P:20;
zone=Exit /POS:478,426,66,54 /COMM:Go /MOUSE:TET /P:20;
event=e_RUN /GAME:XTETDLL.DLL:GAGBoy::Score /QUIT;

[END]
)";
bool gagboy_startup_mode;

bool find_virtual_runtime_script(const char *path, VirtualScriptResource *resource)
{
    if(path == nullptr || resource == nullptr)
    {
        return false;
    }
    const char *name = path;
    for(const char *cursor = path; *cursor != '\0'; ++cursor)
    {
        if(*cursor == '\\' || *cursor == '/')
        {
            name = cursor + 1;
        }
    }
    if(gagboy_startup_mode && _stricmp(name, "GAGBOY.CFG") == 0)
    {
        resource->data = gagboy_startup_script;
        resource->size = static_cast<uint32_t>(sizeof(gagboy_startup_script) - 1);
        resource->resource_type = 4;
        return true;
    }
    return find_save_load_virtual_script(name, resource);
}

bool has_xtet_argument(int argc, char *argv[])
{
    if(argv == nullptr)
    {
        return false;
    }
    for(int index = 1; index < argc; ++index)
    {
        if(argv[index] != nullptr && std::strcmp(argv[index], "--xtet") == 0)
        {
            return true;
        }
    }
    return false;
}

ApplicationState *initialize_gag_application(int width, int height, HINSTANCE instance, bool start_xtet, int show_command)
{
    gagboy_startup_mode = start_xtet;
    application_initialization_api.set_error_mode(0x8001);
    ApplicationState *state = static_cast<ApplicationState *>(application_initialization_api.heap_alloc(application_initialization_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(ApplicationState)));
    if(state == nullptr)
    {
        return nullptr;
    }
    modern_windows_presentation_state = {};
    modern_windows_fullscreen_toggle_latched = false;
    modern_windows_game_cursor_tracking = false;

    state->instance = instance;
    state->message_table = application_message_table[0];
    if(!application_initialization_api.register_window_classes(state))
    {
        return nullptr;
    }
    state->width = width;
    state->height = height;
    state->flags |= 2;
    application_initialization_api.copy_string(state->executable_directory, "Gag01.cdf");
    if(application_initialization_api.validate_environment(state, state->executable_directory, 0xfffef9ff) == 0)
    {
        return nullptr;
    }

    {
        // Restore the complete framed window rectangle, using the saved point as a migration fallback.
        RECT windowed_rectangle{ 0, 0, width, height };
        application_initialization_api.adjust_window_rect(&windowed_rectangle, modern_windows_windowed_style, FALSE);
        const int32_t minimum_width = windowed_rectangle.right - windowed_rectangle.left;
        const int32_t minimum_height = windowed_rectangle.bottom - windowed_rectangle.top;
        windowed_rectangle = { 0, 0, minimum_width, minimum_height };
        if(!load_saved_window_rectangle(minimum_width, minimum_height, &windowed_rectangle))
        {
            const int32_t screen_width = application_initialization_api.get_system_metrics(SM_CXSCREEN);
            const int32_t screen_height = application_initialization_api.get_system_metrics(SM_CYSCREEN);
            const int32_t left = (screen_width - minimum_width) / 2;
            const int32_t top = (screen_height - minimum_height) / 2;
            windowed_rectangle = { left, top, left + minimum_width, top + minimum_height };
        }
        modern_windows_presentation_state.windowed_rectangle = windowed_rectangle;
        modern_windows_presentation_state.windowed_rectangle_valid = true;
    }

    state->desktop_window_rect.left = 0;
    state->desktop_window_rect.top = 0;
    if((state->flags & 0x80) != 0)
    {
        state->desktop_window_rect = modern_windows_presentation_state.windowed_rectangle;
        state->window_top_adjustment = 0;
    }
    else
    {
        state->desktop_window_rect.right = application_initialization_api.get_system_metrics(SM_CXSCREEN);
        state->desktop_window_rect.bottom = application_initialization_api.get_system_metrics(SM_CYSCREEN);
        application_initialization_api.adjust_window_rect(&state->desktop_window_rect, 0x80c00000, FALSE);
        state->desktop_window_rect.top -= application_initialization_api.get_system_metrics(SM_CYCAPTION);
        state->window_top_adjustment = 1 - state->desktop_window_rect.top;
    }

    DWORD window_style = 0x82000000;
    if((state->flags & 0x80) != 0)
    {
        // Provide a freely resizable framed window around the fixed-size game framebuffer.
        window_style = modern_windows_windowed_style;
    }
    int window_x = state->desktop_window_rect.left;
    int window_y = state->desktop_window_rect.top;
    if((state->flags & 0x80) != 0)
    {
        window_x = state->desktop_window_rect.left;
        window_y = state->desktop_window_rect.top;
    }
    state->window = application_initialization_api.create_window_ex(0, "FlcAppClassNT", "GAG", window_style, window_x, window_y, state->desktop_window_rect.right - state->desktop_window_rect.left,
        state->desktop_window_rect.bottom - state->desktop_window_rect.top, nullptr, nullptr, instance, state);
    if(state->window == nullptr)
    {
        return nullptr;
    }
    initialize_host_events(wake_host_event_loop, state->window, handle_host_event, state);
    application_initialization_api.show_window(state->window, show_command);
    if((state->flags & 0x80) == 0)
    {
        application_initialization_api.set_window_position(state->window, nullptr, 0, 0, 0, 0, 0x103);
    }

    RECT client_rectangle;
    application_initialization_api.get_client_rect(state->window, &client_rectangle);
    state->content_left = static_cast<int32_t>(static_cast<uint32_t>(client_rectangle.right - width) >> 1);
    if((state->flags & 0x80) != 0)
    {
        // Center the framebuffer within the window's client area.
        state->content_top = static_cast<int32_t>(static_cast<uint32_t>(client_rectangle.bottom - height) >> 1);
    }
    else
    {
        state->content_top = static_cast<int32_t>(static_cast<uint32_t>((client_rectangle.bottom - state->desktop_window_rect.top) - height) >> 1) - 1;
    }
    state->content_right = state->content_left + width;
    state->content_bottom = state->content_top + height;

    GraphicsHostInitializationResult *graphics =
        application_initialization_api.initialize_graphics_host(instance, state->window, state->content_left, state->content_top, static_cast<int16_t>(width), static_cast<uint16_t>(height), 0x300000);
    if(graphics == nullptr)
    {
        close_host_events();
        return nullptr;
    }
    state->capture_window = graphics->capture_window;
    state->game_context = graphics;
    application_initialization_api.validate_environment(state, state->executable_directory, 0x200);
    if(application_initialization_api.initialize_runtime() == nullptr)
    {
        shutdown_graphics_host();
        close_host_events();
        return nullptr;
    }

    application_initialization_api.update_window_layout(state, nullptr);
    if((state->flags & 0x1000) != 0)
    {
        application_initialization_api.enable_runtime();
    }
    if(graphics->bits_per_pixel > 8)
    {
        application_initialization_api.set_active_object_field(1);
    }
    if(gagboy_startup_mode)
    {
        application_initialization_api.copy_string(state->startup_config, "GAGBOY.CFG");
    }
    else
    {
        application_initialization_api.copy_string(state->startup_config, "Start.cfg");
        if(state->archive_context != nullptr && application_initialization_api.detect_resource_type(state->installed_version) == 4)
        {
            application_initialization_api.copy_string(state->startup_config, state->installed_version);
        }
    }
    return state;
}



uint32_t initialize_runtime_media_backend()
{
    if(runtime_media_backend_initialized)
    {
        return 1;
    }
    runtime_media_backend_heap = runtime_backend_initialization_api.heap_create(0, 0, 0);
    if(runtime_media_backend_heap == nullptr)
    {
        return 0;
    }
    runtime_media_backend_mutex = runtime_backend_initialization_api.create_mutex(nullptr, FALSE, nullptr);
    if(runtime_media_backend_mutex == nullptr)
    {
        return 0;
    }
    runtime_backend_initialization_api.initialize_sound();
    runtime_media_backend_initialized = true;
    return 1;
}

uint32_t initialize_runtime_generic_backend()
{
    if(runtime_generic_backend_enabled != 0)
    {
        return 1;
    }
    runtime_generic_backend_mutex = runtime_backend_initialization_api.create_mutex(nullptr, FALSE, nullptr);
    if(runtime_generic_backend_mutex == nullptr)
    {
        return 0;
    }
    runtime_backend_initialization_api.wait_for_single_object(runtime_generic_backend_mutex, INFINITE);
    runtime_generic_backend_enabled = 1;
    runtime_backend_initialization_api.release_mutex(runtime_generic_backend_mutex);
    return 1;
}

uint32_t shutdown_runtime_generic_backend()
{
    if(runtime_generic_backend_enabled != 0)
    {
        runtime_generic_backend_shutdown_api.wait_for_single_object(runtime_generic_backend_mutex, INFINITE);
        runtime_generic_backend_enabled = 0;
        runtime_generic_backend_shutdown_api.release_mutex(runtime_generic_backend_mutex);
        while(runtime_generic_backend_head != nullptr)
        {
            runtime_generic_backend_shutdown_api.destroy_backend(runtime_generic_backend_head);
        }
        runtime_generic_backend_shutdown_api.close_handle(runtime_generic_backend_mutex);
        runtime_generic_backend_mutex = nullptr;
    }
    return 1;
}


RuntimeMediaBackend *acquire_first_runtime_media_backend()
{
    return acquire_runtime_media_backend(runtime_media_backend_head);
}

uint32_t shutdown_runtime_media_backend()
{
    if(!runtime_media_backend_initialized)
    {
        return 0;
    }
    RuntimeMediaBackend *backend = runtime_media_backend_shutdown_api.acquire_first_backend();
    if(backend != nullptr)
    {
        runtime_media_backend_shutdown_api.release_backend_lock(backend);
        return 0;
    }
    runtime_media_backend_shutdown_api.heap_destroy(runtime_media_backend_heap);
    runtime_media_backend_shutdown_api.close_handle(runtime_media_backend_mutex);
    runtime_media_backend_initialized = false;
    runtime_media_backend_shutdown_api.shutdown_sound();
    return 1;
}


uint32_t initialize_async_file_subsystem()
{
    if(async_file_enabled)
    {
        return 1;
    }
    runtime_backend_initialization_api.initialize_critical_section(&async_file_global_lock);
    async_file_enabled = true;
    return 1;
}



void set_script_runtime_root_if_valid(ScriptRuntimeRoot *root)
{
    if(root != reinterpret_cast<ScriptRuntimeRoot *>(static_cast<intptr_t>(-1)))
    {
        script_runtime_root = root;
    }
}

void set_runtime_named_node_enabled(void *identity, int enabled)
{
    for(RuntimeNamedNode *node = script_runtime_root->runtime_nodes; node != nullptr; node = node->next)
    {
        if(node->identity == identity)
        {
            if(enabled != 0)
            {
                node->flags |= 1;
            }
            else
            {
                node->flags &= 0xfffffffe;
            }
            return;
        }
    }
}

bool register_gag_window_classes(ApplicationState *state)
{
    HBRUSH background = window_class_api.create_solid_brush(0);
    WNDCLASSEXA window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_class_api.primary_window_procedure;
    window_class.cbWndExtra = sizeof(LONG_PTR);
    window_class.hInstance = state->instance;
    window_class.hIcon = window_class_api.load_icon(state->instance, MAKEINTRESOURCEA(105));
    window_class.hCursor = window_class_api.load_cursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = background;
    window_class.lpszClassName = "FlcAppClassNT";
    ATOM primary_result = window_class_api.register_class_ex(&window_class);

    window_class = {};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_class_api.capture_window_procedure;
    window_class.cbWndExtra = sizeof(LONG_PTR);
    window_class.hInstance = state->instance;
    window_class.hCursor = window_class_api.load_cursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = background;
    window_class.lpszClassName = "FlcCapClassNT";
    window_class.hIconSm = window_class_api.load_icon(state->instance, MAKEINTRESOURCEA(105));
    ATOM capture_result = window_class_api.register_class_ex(&window_class);

    if(primary_result == 0 || capture_result == 0)
    {
        window_class_api.message_box(nullptr, application_message(state, 14), state->message_table, MB_ICONERROR);
    }
    return primary_result != 0 && capture_result != 0;
}

LRESULT CALLBACK gag_main_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto *state = reinterpret_cast<ApplicationState *>(main_window_procedure_api.get_window_long(window, 0));
    if(message == host_event_wake_message)
    {
        drain_host_events();
        return 0;
    }
    if(message == WM_LBUTTONDOWN)
    {
        modern_windows_fullscreen_toggle_latched = false;
    }
    if(state != nullptr && (state->flags & 0x80000000) != 0 && (message < 0x30f || message > 0x311))
    {
        return 0;
    }

    if(message == WM_CREATE)
    {
        main_window_procedure_api.set_window_long(window, 0, *reinterpret_cast<LONG_PTR *>(lparam));
        return 0;
    }
    if(message == WM_DESTROY)
    {
        close_host_events();
        main_window_procedure_api.post_quit_message(0);
        return 0;
    }
    if(message == WM_ACTIVATE)
    {
        if(state != nullptr && LOWORD(wparam) == 0 && HIWORD(wparam) == 0)
        {
            if((state->flags & 0x80000000) != 0)
            {
                return 0;
            }
            if((state->flags & 0x40000000) == 0)
            {
                // A normal window must not minimize merely because another application receives focus.
                if((state->flags & 0x80) == 0)
                {
                    main_window_procedure_api.post_message(window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                }
            }
        }
    }
    else if(message == WM_CLOSE)
    {
        if(state != nullptr)
        {
            set_application_inactive_flags(state);
            state->flags |= 0x200;
            main_window_procedure_api.set_runtime_flag_40();
            return 0;
        }
    }
    else if(message == WM_ACTIVATEAPP)
    {
        return 0;
    }
    else if(message == WM_GETMINMAXINFO)
    {
        if(state != nullptr && (state->flags & 0x80) != 0)
        {
            RECT minimum_rectangle{ 0, 0, state->width, state->height };
            window_layout_api.adjust_window_rect(&minimum_rectangle, modern_windows_windowed_style, FALSE);
            auto *minimum_information = reinterpret_cast<MINMAXINFO *>(lparam);
            minimum_information->ptMinTrackSize.x = minimum_rectangle.right - minimum_rectangle.left;
            minimum_information->ptMinTrackSize.y = minimum_rectangle.bottom - minimum_rectangle.top;
            return 0;
        }
    }
    else if(message == WM_SIZE)
    {
        if(state != nullptr && (state->flags & 0x80) != 0 && wparam != SIZE_MINIMIZED)
        {
            update_modern_windows_windowed_viewport(state);
            if(wparam == SIZE_RESTORED && window_layout_api.get_window_rect(state->window, &modern_windows_presentation_state.windowed_rectangle) != FALSE)
            {
                modern_windows_presentation_state.windowed_rectangle_valid = true;
            }
        }
    }
    else if(message == WM_WINDOWPOSCHANGING)
    {
        auto *position = reinterpret_cast<WINDOWPOS *>(lparam);
        if(state != nullptr && (position->flags & SWP_NOSIZE) == 0 && (state->flags & 0xb0000000) == 0x10000000)
        {
            SecondaryWindowLayout layout{ 0, 0, position->x, position->y, position->cx, position->cy, position->flags };
            update_application_window_layout(state, &layout);
            position->x = layout.x;
            position->y = layout.y;
            position->cx = layout.width;
            position->cy = layout.height;
            position->flags = layout.flags;
        }
        return main_window_procedure_api.default_window_procedure(window, message, wparam, lparam);
    }
    else if(message == WM_MOUSEMOVE)
    {
        const int32_t x = static_cast<uint16_t>(LOWORD(lparam));
        const int32_t y = static_cast<uint16_t>(HIWORD(lparam));
        if(x < state->content_left || y < state->content_top || x >= state->content_right || y >= state->content_bottom)
        {
            set_game_cursor_active(state, 1);
        }
        else if((state->flags & 0x40000001) == 0)
        {
            set_game_cursor_active(state, 0);
        }
        return 0;
    }
    else if(message == WM_SYSCOMMAND)
    {
        const uint32_t command = static_cast<uint32_t>(wparam) & 0xfffffff0;
        if(command == SC_MINIMIZE && state != nullptr)
        {
            state->flags |= 0x20000000;
            set_runtime_flag_01000000();
            const LRESULT result = main_window_procedure_api.default_window_procedure(window, message, wparam, lparam);
            state->flags = (state->flags & 0xdfffffff) | 0x10000000;
            return result;
        }
        if(command == SC_RESTORE && state != nullptr)
        {
            const LRESULT result = main_window_procedure_api.default_window_procedure(window, message, wparam, lparam);
            clear_runtime_flag_01000000();
            state->flags &= 0xefffffff;
            return result;
        }
        if(command == SC_SCREENSAVE)
        {
            return 0;
        }
    }
    else if(message == WM_KEYDOWN || message == WM_CHAR || (message >= 0x30f && message <= 0x311))
    {
        if(state->capture_window != nullptr)
        {
            return main_window_procedure_api.send_message(state->capture_window, message, wparam, lparam);
        }
        return 0;
    }
    else if(message == WM_COMMAND)
    {
        const uint32_t command = LOWORD(wparam);
        if(command == 0x8790)
        {
            set_runtime_flag_01000000();
            return 0;
        }
        if(command == 0x8800)
        {
            clear_runtime_flag_01000000();
            return 0;
        }
        if(command == 0x8890)
        {
            main_window_procedure_api.post_message(window, WM_CLOSE, 0, 0);
            return 0;
        }
        if(state == nullptr)
        {
            return 0;
        }
        if(command == 0x8780 || command == 0x8810)
        {
            const SaveLoadScreenMode mode = command == 0x8780 ? SaveLoadScreenMode::save : SaveLoadScreenMode::load;
            request_scripted_save_load_screen(mode, state);
            return 0;
        }
        if(command == 0x8840)
        {
            main_window_procedure_api.set_application_lock(state);
            main_window_procedure_api.clear_runtime_active(state);
            state->flags |= 0x10;
            set_runtime_flag_01000000();
            return 0;
        }
        if(command == 0x8860)
        {
            state->flags |= 0x200000;
            copy_string(state->startup_config, "NewGame.cfg");
            main_window_procedure_api.set_application_lock(state);
            main_window_procedure_api.clear_runtime_active(state);
            while(main_window_procedure_api.validate_startup(state, state->executable_directory, 0x24) == 0)
            {
            }
            main_window_procedure_api.set_runtime_flag_40();
            return 0;
        }
        if(command == 0x8870)
        {
            state->flags |= 0x200000;
            main_window_procedure_api.set_application_lock(state);
            main_window_procedure_api.clear_runtime_active(state);
            while(main_window_procedure_api.validate_startup(state, state->executable_directory, 0x24) == 0)
            {
            }
            copy_string(state->startup_config, "START.CFG");
            copy_string(state->installed_version, state->installation_path);
            append_string(state->installed_version, auto_save_file_name);
            main_window_procedure_api.set_runtime_flag_40();
            return 0;
        }
        if(command == 0x8880)
        {
            main_window_procedure_api.set_application_lock(state);
            main_window_procedure_api.clear_runtime_active(state);
            while(main_window_procedure_api.validate_startup(state, state->executable_directory, 0x24) == 0)
            {
            }
            set_runtime_paths_once("Credits.cfg", "CREDITS");
            return 0;
        }
        if(command == 0x8820)
        {
            // Intro trees update the live NOCOMMENT bit after the persisted preference is restored, while the menu graphic reads the application bit. Keep the persisted preference authoritative so
            // the first click takes effect immediately.
            const bool comments_enabled = (state->flags & 0x02000000) == 0;
            if(comments_enabled)
            {
                state->flags |= 0x02000000;
            }
            else
            {
                state->flags &= 0xfdffffff;
            }
            set_script_runtime_flags(1, !comments_enabled);
            state->flags |= 0x40000;
            return 0;
        }
        if(command == 0x8850)
        {
            if((state->flags & 0x1000) == 0)
            {
                state->flags |= 0x1000;
                enable_runtime_subsystem();
            }
            else
            {
                state->flags &= 0xffffefff;
                disable_runtime_subsystem();
            }
            state->flags |= 0x40000;
            return 0;
        }
        if(command == 0x8900)
        {
            const uint32_t flags = state->flags;
            if((flags & 0x80) != 0)
            {
                state->flags = (flags & 0xffffff7f) | 0x40;
                set_runtime_flag_01000000();
            }
            return 0;
        }
        if(command == 0x8910)
        {
            if((state->flags & 0x80) == 0)
            {
                state->flags |= 0xc0;
                set_runtime_flag_01000000();
            }
            return 0;
        }
        return 0;
    }
    return main_window_procedure_api.default_window_procedure(window, message, wparam, lparam);
}

LRESULT CALLBACK gag_capture_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto *state = reinterpret_cast<ApplicationState *>(window_procedure_api.get_window_long(window, 0));
    if(state != nullptr && (state->flags & 0x80000000) != 0 && (message < 0x30f || message > 0x311))
    {
        return 0;
    }

    if(message == WM_CREATE)
    {
        auto *create = reinterpret_cast<CREATESTRUCTA *>(lparam);
        state = static_cast<ApplicationState *>(create->lpCreateParams);
        window_procedure_api.set_window_long(window, 0, reinterpret_cast<LONG_PTR>(state));
        HMENU menu = window_procedure_api.get_system_menu(window, FALSE);
        window_procedure_api.delete_menu(menu, SC_RESTORE, 0);
        window_procedure_api.delete_menu(menu, SC_MAXIMIZE, 0);
        window_procedure_api.delete_menu(menu, SC_SIZE, 0);
        if((state->validation_flags & 0x100) != 0)
        {
            HMENU menu_bar = window_procedure_api.create_menu();
            state->game_menu = window_procedure_api.create_popup_menu();
            state->options_menu = window_procedure_api.create_popup_menu();
            state->system_menu = window_procedure_api.create_popup_menu();
            window_procedure_api.append_menu(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(state->game_menu), application_message(state, 1));
            window_procedure_api.append_menu(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(state->options_menu), application_message(state, 2));
            window_procedure_api.append_menu(menu_bar, MF_POPUP, reinterpret_cast<UINT_PTR>(state->system_menu), application_message(state, 3));
            window_procedure_api.append_menu(state->options_menu, 0, 0x8820, application_message(state, 10));
            window_procedure_api.check_menu_item(state->options_menu, 0x8820, (state->flags & 0x2000000) != 0 ? MF_CHECKED : 0);
            window_procedure_api.append_menu(state->options_menu, 0, 0x8850, application_message(state, 11));
            window_procedure_api.check_menu_item(state->options_menu, 0x8850, (state->flags & 0x1000) != 0 ? MF_CHECKED : 0);
            window_procedure_api.append_menu(state->options_menu, 0, 0x8840, "Save Screen");
            window_procedure_api.append_menu(state->system_menu, 0, 0x8900, application_message(state, 12));
            window_procedure_api.append_menu(state->system_menu, 0, 0x8910, application_message(state, 13));
            window_procedure_api.append_menu(state->game_menu, 0, 0x8790, "Pause Game");
            window_procedure_api.append_menu(state->game_menu, 0, 0x8800, "Resume Game");
            window_procedure_api.append_menu(state->game_menu, MF_SEPARATOR, 0, nullptr);
            window_procedure_api.append_menu(state->game_menu, 0, 0x8810, application_message(state, 4));
            if((state->flags & 0x100000) != 0)
            {
                window_procedure_api.enable_menu_item(state->game_menu, 0x8810, MF_GRAYED);
            }
            window_procedure_api.append_menu(state->game_menu, MF_GRAYED, 0x8780, application_message(state, 5));
            window_procedure_api.append_menu(state->game_menu, MF_SEPARATOR, 0, nullptr);
            window_procedure_api.append_menu(state->game_menu, 0, 0x8860, application_message(state, 6));
            window_procedure_api.append_menu(state->game_menu, 0, 0x8870, application_message(state, 7));
            if((state->flags & 0x200000) != 0)
            {
                window_procedure_api.enable_menu_item(state->game_menu, 0x8870, MF_GRAYED);
            }
            window_procedure_api.append_menu(state->game_menu, MF_CHECKED, 0x8880, application_message(state, 8));
            window_procedure_api.append_menu(state->game_menu, MF_SEPARATOR, 0, nullptr);
            window_procedure_api.append_menu(state->game_menu, 0, 0x8890, application_message(state, 9));
            window_procedure_api.set_menu(window, menu_bar);
        }
        return 0;
    }

    if(message == WM_ACTIVATE && state != nullptr && LOWORD(wparam) == 0 && HIWORD(wparam) == 0 && reinterpret_cast<HWND>(lparam) != state->window)
    {
        if((state->flags & 0x80000000) != 0)
        {
            return 0;
        }
        if((state->flags & 0x40000000) == 0)
        {
            // Preserve an inactive windowed top-level window without minimizing it.
            if((state->flags & 0x80) == 0)
            {
                window_procedure_api.post_message(state->window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            }
        }
    }
    else if(message == WM_CLOSE)
    {
        window_procedure_api.destroy_window(state->window);
        return 0;
    }
    else if(message == WM_ACTIVATEAPP)
    {
        return 0;
    }
    else if(message == WM_NCHITTEST)
    {
        LRESULT result = window_procedure_api.default_window_procedure(window, message, wparam, lparam);
        if(result == HTCAPTION || result == HTMENU || result == HTSYSMENU || result == HTMINBUTTON)
        {
            window_procedure_api.update_cursor_state(state, 1);
        }
        return result;
    }
    else if(message == WM_NCACTIVATE)
    {
        return 1;
    }
    else if(message == WM_KEYDOWN || message == WM_CHAR)
    {
        if(state->capture_window != nullptr)
        {
            return window_procedure_api.send_message(state->capture_window, message, wparam, lparam);
        }
    }
    else if(message == WM_COMMAND || message == WM_SYSCOMMAND)
    {
        return window_procedure_api.send_message(state->window, message, wparam, lparam);
    }
    else if(message == WM_MENUSELECT)
    {
        if(HIWORD(wparam) == 0xffff && lparam == 0)
        {
            state->flags &= 0xfbffffff;
        }
        else
        {
            state->flags |= 0x4000000;
        }
    }

    return window_procedure_api.default_window_procedure(window, message, wparam, lparam);
}

constexpr char preferences_file_name[] = ".\\freegag.ini";
constexpr char game_preferences_section[] = "Game";
constexpr char window_preferences_section[] = "Window";

bool get_preferences_path(char *path, DWORD size)
{
    const DWORD length = local_preferences_api.get_full_path_name(preferences_file_name, size, path, nullptr);
    return length != 0 && length < size;
}

bool read_preference_number(const char *section, const char *key, int64_t minimum, int64_t maximum, int64_t *result)
{
    char path[MAX_PATH];
    if(!get_preferences_path(path, static_cast<DWORD>(std::size(path))))
    {
        return false;
    }
    char value[64]{};
    const DWORD length = local_preferences_api.read_value(section, key, "", value, static_cast<DWORD>(std::size(value)), path);
    if(length == 0 || length >= std::size(value) - 1)
    {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    const long long parsed = std::strtoll(value, &end, 0);
    while(end != nullptr && std::isspace(static_cast<unsigned char>(*end)))
    {
        ++end;
    }
    if(errno == ERANGE || end == value || end == nullptr || *end != '\0' || parsed < minimum || parsed > maximum)
    {
        return false;
    }
    *result = parsed;
    return true;
}

bool read_saved_window_rectangle(RECT *rectangle)
{
    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    if(!read_preference_number(window_preferences_section, "Left", INT32_MIN, INT32_MAX, &left) || !read_preference_number(window_preferences_section, "Top", INT32_MIN, INT32_MAX, &top)
        || !read_preference_number(window_preferences_section, "Right", INT32_MIN, INT32_MAX, &right) || !read_preference_number(window_preferences_section, "Bottom", INT32_MIN, INT32_MAX, &bottom))
    {
        return false;
    }
    *rectangle = { static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(right), static_cast<LONG>(bottom) };
    return true;
}

void write_preference_number(const char *section, const char *key, int64_t value)
{
    char path[MAX_PATH];
    if(!get_preferences_path(path, static_cast<DWORD>(std::size(path))))
    {
        return;
    }
    char text[32];
    std::snprintf(text, sizeof(text), "%lld", static_cast<long long>(value));
    local_preferences_api.write_value(section, key, text, path);
}

void save_runtime_settings(ApplicationState *state)
{
    if((state->flags & 0x80) != 0)
    {
        save_window_position(state);
    }
    if(state->archive_context == nullptr)
    {
        char path[MAX_PATH];
        if(get_preferences_path(path, static_cast<DWORD>(std::size(path))))
        {
            char settings[16];
            std::snprintf(settings, sizeof(settings), "0x%08X", state->flags & 0x02001020);
            local_preferences_api.write_value(game_preferences_section, "Settings", settings, path);
        }
    }
}

bool load_saved_window_position(int32_t width, int32_t height, POINT *position)
{
    if(position == nullptr)
    {
        return false;
    }
    RECT saved_rectangle{};
    if(!read_saved_window_rectangle(&saved_rectangle))
    {
        return false;
    }
    const int64_t right = static_cast<int64_t>(saved_rectangle.left) + width;
    const int64_t bottom = static_cast<int64_t>(saved_rectangle.top) + height;
    if(right < INT32_MIN || right > INT32_MAX || bottom < INT32_MIN || bottom > INT32_MAX)
    {
        return false;
    }
    saved_rectangle.right = static_cast<LONG>(right);
    saved_rectangle.bottom = static_cast<LONG>(bottom);
    if(local_preferences_api.monitor_from_rect(&saved_rectangle, MONITOR_DEFAULTTONULL) == nullptr)
    {
        return false;
    }
    *position = { saved_rectangle.left, saved_rectangle.top };
    return true;
}

bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, RECT *rectangle)
{
    if(rectangle == nullptr)
    {
        return false;
    }

    RECT saved_rectangle{};
    const bool loaded = read_saved_window_rectangle(&saved_rectangle);
    const int64_t width = static_cast<int64_t>(saved_rectangle.right) - saved_rectangle.left;
    const int64_t height = static_cast<int64_t>(saved_rectangle.bottom) - saved_rectangle.top;
    bool valid = loaded && width >= minimum_width && height >= minimum_height && local_preferences_api.monitor_from_rect(&saved_rectangle, MONITOR_DEFAULTTONULL) != nullptr;
    if(valid)
    {
        *rectangle = saved_rectangle;
    }
    return valid;
}

void save_window_position(ApplicationState *state)
{
    if(state == nullptr || state->window == nullptr)
    {
        return;
    }
    RECT rectangle{};
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    if(local_preferences_api.get_window_placement(state->window, &placement) != FALSE)
    {
        rectangle = placement.rcNormalPosition;
    }
    else if(local_preferences_api.get_window_rect(state->window, &rectangle) == FALSE)
    {
        return;
    }

    write_preference_number(window_preferences_section, "Left", rectangle.left);
    write_preference_number(window_preferences_section, "Top", rectangle.top);
    write_preference_number(window_preferences_section, "Right", rectangle.right);
    write_preference_number(window_preferences_section, "Bottom", rectangle.bottom);
    modern_windows_presentation_state.windowed_rectangle = rectangle;
    modern_windows_presentation_state.windowed_rectangle_valid = true;
}

void set_game_cursor_active(ApplicationState *state, int active)
{
    if(active == 0)
    {
        if((state->flags & 1) == 0)
        {
            cursor_visibility_api.show_cursor(FALSE);
            cursor_visibility_api.on_cursor_hidden();
            state->flags |= 1;
        }
    }
    else if((state->flags & 1) != 0)
    {
        cursor_visibility_api.show_cursor(TRUE);
        cursor_visibility_api.on_cursor_shown();
        state->flags &= 0xfffffffe;
    }
}

void finish_credits_state(ApplicationState *state, RuntimeTreeNode *tree)
{
    if(tree->parent == nullptr && (state->validation_flags & 0x100) != 0 && (state->flags & 0x10000) != 0 && strings_equal("CREDITS", tree->name))
    {
        state->flags &= 0xfffeffff;
        finish_credits_callback();
    }
}

void update_modern_windows_windowed_viewport(ApplicationState *state)
{
    if(state == nullptr || state->window == nullptr || (state->flags & 0x80) == 0)
    {
        return;
    }
    RECT client_rectangle{};
    if(window_layout_api.get_client_rect(state->window, &client_rectangle) == FALSE)
    {
        return;
    }
    const RECT viewport = calculate_modern_windows_windowed_viewport(client_rectangle.right - client_rectangle.left, client_rectangle.bottom - client_rectangle.top, state->width, state->height,
        modern_windows_windowed_scaling);
    state->content_left = viewport.left;
    state->content_top = viewport.top;
    state->content_right = viewport.right;
    state->content_bottom = viewport.bottom;
    modern_windows_presentation_state.viewport_width = viewport.right - viewport.left;
    modern_windows_presentation_state.viewport_height = viewport.bottom - viewport.top;
    if(state->capture_window != nullptr)
    {
        window_layout_api.set_window_position(state->capture_window, nullptr, viewport.left, viewport.top, modern_windows_presentation_state.viewport_width,
            modern_windows_presentation_state.viewport_height, SWP_NOZORDER | SWP_NOCOPYBITS);
        window_layout_api.invalidate_rect(state->capture_window, nullptr, TRUE);
    }
    window_layout_api.invalidate_rect(state->window, nullptr, TRUE);
    runtime_game_host_context.x_offset = static_cast<uint32_t>(state->content_left);
    runtime_game_host_context.y_offset = static_cast<uint32_t>(state->content_top);
}

void update_application_window_layout(ApplicationState *state, SecondaryWindowLayout *secondary_layout)
{
    if(secondary_layout != nullptr)
    {
        secondary_layout->state = 0;
        if((state->flags & 0x80) != 0)
        {
            // Windows owns the freely resizable framed window dimensions.
            return;
        }
        secondary_layout->flags &= ~(SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
        secondary_layout->x = state->desktop_window_rect.left;
        secondary_layout->y = state->desktop_window_rect.top;
        secondary_layout->width = state->desktop_window_rect.right - state->desktop_window_rect.left;
        secondary_layout->height = state->desktop_window_rect.bottom - state->desktop_window_rect.top;
        window_layout_api.set_window_position(state->capture_window, nullptr, state->content_left, state->content_top, modern_windows_presentation_state.viewport_width,
            modern_windows_presentation_state.viewport_height, SWP_NOZORDER | SWP_NOCOPYBITS);
        return;
    }

    state->desktop_window_rect.left = 0;
    state->desktop_window_rect.top = 0;
    if((state->flags & 0x80) != 0)
    {
        if(state->window != nullptr)
        {
            // Replacing GWL_STYLE after ShowWindow must retain visibility so Windows keeps the frame in hit testing.
            window_layout_api.set_window_long(state->window, GWL_STYLE, modern_windows_windowed_style | WS_VISIBLE);
        }
        state->window_top_adjustment = 0;

        if(modern_windows_presentation_state.fullscreen && modern_windows_presentation_state.windowed_rectangle_valid && state->window != nullptr)
        {
            const RECT &rectangle = modern_windows_presentation_state.windowed_rectangle;
            window_layout_api.set_window_position(state->window, nullptr, rectangle.left, rectangle.top, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
                SWP_FRAMECHANGED | SWP_NOCOPYBITS);
            state->desktop_window_rect = rectangle;
        }
        else if(state->window != nullptr)
        {
            window_layout_api.get_window_rect(state->window, &state->desktop_window_rect);
        }
        modern_windows_presentation_state.fullscreen = false;
    }
    else
    {
        if(!modern_windows_presentation_state.fullscreen && !modern_windows_presentation_state.windowed_rectangle_valid && state->window != nullptr)
        {
            modern_windows_presentation_state.windowed_rectangle_valid = window_layout_api.get_window_rect(state->window, &modern_windows_presentation_state.windowed_rectangle) != FALSE;
        }
        if(state->window != nullptr)
        {
            window_layout_api.set_window_long(state->window, GWL_STYLE, 0x82000000 | WS_VISIBLE);
        }

        MONITORINFO monitor_info{};
        monitor_info.cbSize = sizeof(monitor_info);
        const HMONITOR monitor = state->window != nullptr ? window_layout_api.monitor_from_window(state->window, MONITOR_DEFAULTTONEAREST) : nullptr;
        if(monitor == nullptr || window_layout_api.get_monitor_info(monitor, &monitor_info) == FALSE)
        {
            monitor_info.rcMonitor = { 0, 0, window_layout_api.get_system_metrics(SM_CXSCREEN), window_layout_api.get_system_metrics(SM_CYSCREEN) };
        }
        state->desktop_window_rect = monitor_info.rcMonitor;
        state->window_top_adjustment = 0;

        const int32_t monitor_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
        const int32_t monitor_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
        const RECT viewport = calculate_modern_windows_fullscreen_viewport(monitor_width, monitor_height, state->width, state->height, modern_windows_fullscreen_scaling);
        state->content_left = viewport.left;
        state->content_top = viewport.top;
        state->content_right = viewport.right;
        state->content_bottom = viewport.bottom;
        modern_windows_presentation_state.fullscreen = true;
        modern_windows_presentation_state.viewport_width = viewport.right - viewport.left;
        modern_windows_presentation_state.viewport_height = viewport.bottom - viewport.top;
    }
    if(state->window != nullptr && state->capture_window != nullptr)
    {
        if((state->flags & 0x80) == 0)
        {
            window_layout_api.set_window_position(state->window, nullptr, state->desktop_window_rect.left, state->desktop_window_rect.top,
                state->desktop_window_rect.right - state->desktop_window_rect.left, state->desktop_window_rect.bottom - state->desktop_window_rect.top, SWP_FRAMECHANGED | SWP_NOCOPYBITS);
        }
        if((state->flags & 0x80) != 0)
        {
            update_modern_windows_windowed_viewport(state);
        }
        else
        {
            window_layout_api.set_window_position(state->capture_window, nullptr, state->content_left, state->content_top, modern_windows_presentation_state.viewport_width,
                modern_windows_presentation_state.viewport_height, SWP_NOZORDER | SWP_NOCOPYBITS);
        }
        if((state->flags & 0x80) == 0)
        {
            window_layout_api.invalidate_rect(state->window, nullptr, TRUE);
            window_layout_api.invalidate_rect(state->capture_window, nullptr, TRUE);
        }
        window_layout_api.set_focus(state->capture_window);
        window_layout_api.send_message(state->window, WM_QUERYNEWPALETTE, 0, 0);
    }
    runtime_game_host_context.x_offset = static_cast<uint32_t>(state->content_left);
    runtime_game_host_context.y_offset = static_cast<uint32_t>(state->content_top);
}


void restore_application_display(ApplicationState *state)
{
    if((state->flags & 0x80) == 0)
    {
        state->flags |= 0x20;
        if(!modern_windows_presentation_state.fullscreen)
        {
            // Preserve the latest framed position even when the application is later closed while fullscreen is active.
            save_window_position(state);
        }
    }
    window_layout_api.set_window_position(state->capture_window, nullptr, -state->width, -state->height, 0, 0, 0x105);
    update_application_window_layout(state, nullptr);
    if((state->flags & 0x80) != 0)
    {
        state->flags &= 0xffffffdf;
    }
    clear_runtime_flag_01000000();
}

void process_state_activation(ApplicationState *state, RuntimeTreeNode *tree)
{
    // Process the RuntimeTreeNode directly; treating its prefix as a separate state record is unsafe when pointers widen.
    if(tree->parent != nullptr || runtime_display_context.runtime_tree_identity != tree || (state->validation_flags & 0x100) == 0)
    {
        return;
    }
    if(current_runtime_scene_identity != nullptr)
    {
        clear_application_lock_flag(state);
        if((state->flags & 1) == 0)
        {
            POINT point;
            if(cursor_state_api.get_cursor_position(&point) != FALSE)
            {
                point.x -= state->desktop_window_rect.left;
                point.y -= state->desktop_window_rect.top;
                if(point.x < state->content_left || point.y < state->content_top || state->content_right < point.x || state->content_bottom < point.y)
                {
                    state_activation_api.on_cursor_outside();
                }
                else
                {
                    set_game_cursor_active(state, 0);
                }
            }
        }
    }
    if(strings_equal("CREDITS", tree->name))
    {
        set_credits_runtime_flag();
        state->flags |= 0x10000;
    }
    void *scene_identity = current_runtime_resource;
    uint32_t status = state_activation_api.query_status(scene_identity);
    if(status == 0)
    {
        uint32_t previous_flags = state->flags;
        if((previous_flags & 0x80000) == 0)
        {
            state->flags = previous_flags | 0xc00000;
        }
        return;
    }
    uint32_t previous_flags = state->flags;
    if((previous_flags & 0x80000) == 0)
    {
        if((tree->flags & 0x100) == 0 && (status & 0x2000) == 0)
        {
            state->flags = previous_flags & 0xff3fffff;
            if((status & 0x3000) != 0)
            {
                state->script_state = state_activation_api.get_script_state();
            }
        }
        else
        {
            state->flags = previous_flags | 0xc00000;
        }
    }
    else
    {
        state->flags = previous_flags & 0xffbfffff;
    }
}


void finish_application_state_load(ApplicationState *state, const char *path)
{
    if(state == nullptr || path == nullptr)
    {
        return;
    }
    copy_string(state->installed_version, path);
    state->flags |= 0x200000;
    copy_string(state->startup_config, "START.CFG");
    runtime_state_transition_callback(0);
    graphics_host_flags |= 0x40;
}


bool finish_synchronized_state_operation(int result)
{
    synchronized_state_api.leave_lock();
    if(result == 0x10000)
    {
        send_application_event(HostApplicationCommand::storage_failure);
    }
    return result == 0;
}

bool write_synchronized_cdf_package(void *path, void *comment, void *unused, void *script_state)
{
    if((graphics_host_flags & 0x800) == 0)
    {
        return false;
    }
    synchronized_state_api.enter_lock();
    return finish_synchronized_state_operation(synchronized_state_api.write_cdf_package(path, comment, unused, script_state));
}


void set_runtime_paths_once(const char *first_path, const char *second_path)
{
    if((graphics_host_flags & 0x04000000) == 0)
    {
        runtime_path_api.enter_lock();
        copy_string(runtime_display_context.first_runtime_path, first_path);
        copy_string(runtime_display_context.second_runtime_path, second_path);
        graphics_host_flags |= 0x04000000;
        runtime_path_api.leave_lock();
    }
}



void save_game_screenshot(void *snapshot_context, void *game_context)
{
    (void)game_context;
    char file_path[0x100]{};
    char file_title[0x100]{};
    char unused[0x100]{};
    (void)unused;
    const char *state_name = active_runtime_pointer_region->name;
    std::sprintf(file_path, "%s_", state_name);

    static const char filter[] = "Bmp Files\0*.bmp\0\0";
    OPENFILENAMEA file_name{};
    file_name.lStructSize = 0x4c;
    file_name.lpstrFilter = filter;
    file_name.nFilterIndex = 1;
    file_name.lpstrFile = file_path;
    file_name.nMaxFile = 0x100;
    file_name.lpstrFileTitle = file_title;
    file_name.nMaxFileTitle = 0x100;
    file_name.Flags = 0x802;
    file_name.lpstrDefExt = "bmp";
    if(screenshot_api.get_save_file_name(&file_name) != FALSE)
    {
        uint32_t bitmap_size;
        void *bitmap = screenshot_api.capture_bitmap(snapshot_context, &bitmap_size, 0);
        if(bitmap != nullptr)
        {
            HANDLE file = screenshot_api.create_file(file_name.lpstrFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if(file != INVALID_HANDLE_VALUE)
            {
                DWORD written;
                screenshot_api.write_file(file, bitmap, bitmap_size, &written, nullptr);
                screenshot_api.close_handle(file);
            }
            free_heap_memory(bitmap);
        }
    }
}


void *create_indexed_bitmap(const BitmapCaptureSource *source, const uint8_t *palette, uint32_t *size, int half_resolution)
{
    if(size != nullptr)
    {
        *size = 0;
    }
    if(palette == nullptr)
    {
        return nullptr;
    }

    const uint32_t width = half_resolution == 0 ? source->width : source->width >> 1;
    uint32_t remaining_height = half_resolution == 0 ? source->height : source->height >> 1;
    const uint32_t pixel_offset = sizeof(BITMAPFILEHEADER) + offsetof(RuntimeIndexedBitmapInfo, pixels);
    const uint32_t bitmap_size = width * remaining_height + pixel_offset;
    auto *bitmap = static_cast<uint8_t *>(bitmap_capture_api.heap_alloc(bitmap_capture_api.get_process_heap(), HEAP_ZERO_MEMORY, bitmap_size));
    if(bitmap == nullptr)
    {
        return nullptr;
    }

    auto *file_header = reinterpret_cast<BITMAPFILEHEADER *>(bitmap);
    file_header->bfType = 0x4d42;
    file_header->bfSize = bitmap_size;
    file_header->bfReserved1 = 0;
    file_header->bfReserved2 = 0;
    file_header->bfOffBits = pixel_offset;
    auto *indexed_bitmap = reinterpret_cast<RuntimeIndexedBitmapInfo *>(bitmap + sizeof(BITMAPFILEHEADER));
    BITMAPINFOHEADER *info = &indexed_bitmap->header;
    info->biSize = sizeof(BITMAPINFOHEADER);
    info->biWidth = width;
    info->biHeight = remaining_height;
    info->biPlanes = 1;
    info->biBitCount = 8;
    info->biCompression = BI_RGB;
    info->biSizeImage = width * remaining_height;
    info->biXPelsPerMeter = 0;
    info->biYPelsPerMeter = 0;
    info->biClrUsed = 0x100;
    info->biClrImportant = 0x100;
    for(uint32_t index = 0; index < 0x100; ++index)
    {
        indexed_bitmap->colors[index].rgbBlue = palette[index * 4 + 6];
        indexed_bitmap->colors[index].rgbGreen = palette[index * 4 + 5];
        indexed_bitmap->colors[index].rgbRed = palette[index * 4 + 4];
    }

    uint32_t destination = 0;
    int32_t source_offset = (source->height - 1) * source->width;
    const int32_t horizontal_step = half_resolution == 0 ? 1 : 2;
    while(remaining_height != 0)
    {
        uint32_t remaining_width = width;
        while(remaining_width != 0)
        {
            indexed_bitmap->pixels[destination] = source->pixels[source_offset];
            ++destination;
            source_offset += horizontal_step;
            --remaining_width;
        }
        source_offset += source->width * (-1 - horizontal_step);
        --remaining_height;
    }
    if(size != nullptr)
    {
        *size = bitmap_size;
    }
    return bitmap;
}

uint8_t expand_masked_channel(uint32_t pixel, uint32_t mask)
{
    uint32_t shift = 0;
    while((mask & 1) == 0)
    {
        mask >>= 1;
        ++shift;
    }
    const uint32_t value = (pixel >> shift) & mask;
    return static_cast<uint8_t>((value * 255 + mask / 2) / mask);
}

void *create_display_bitmap(const DisplayBitmapCaptureSource *source, uint32_t *size, int half_resolution)
{
    if(size != nullptr)
    {
        *size = 0;
    }
    if(source == nullptr || source->pixels == nullptr || source->width == 0 || source->height == 0)
    {
        return nullptr;
    }
    const uint32_t bytes_per_pixel = source->bits_per_pixel >> 3;
    if((source->bits_per_pixel != 8 && source->bits_per_pixel != 16 && source->bits_per_pixel != 24 && source->bits_per_pixel != 32) || source->width > UINT32_MAX / bytes_per_pixel
        || source->stride < source->width * bytes_per_pixel || (source->bits_per_pixel == 8 && source->palette_entries == nullptr)
        || (source->bits_per_pixel != 8 && (source->red_mask == 0 || source->green_mask == 0 || source->blue_mask == 0)))
    {
        return nullptr;
    }

    const uint32_t sample_step = half_resolution == 0 ? 1 : 2;
    const uint32_t width = source->width / sample_step;
    const uint32_t height = source->height / sample_step;
    if(width == 0 || height == 0 || width > UINT32_MAX / sizeof(uint32_t))
    {
        return nullptr;
    }
    const uint32_t destination_stride = width * sizeof(uint32_t);
    constexpr uint32_t pixel_offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    if(height > (UINT32_MAX - pixel_offset) / destination_stride)
    {
        return nullptr;
    }
    const uint32_t bitmap_size = pixel_offset + destination_stride * height;
    auto *bitmap = static_cast<uint8_t *>(bitmap_capture_api.heap_alloc(bitmap_capture_api.get_process_heap(), HEAP_ZERO_MEMORY, bitmap_size));
    if(bitmap == nullptr)
    {
        return nullptr;
    }

    auto *file_header = reinterpret_cast<BITMAPFILEHEADER *>(bitmap);
    file_header->bfType = 0x4d42;
    file_header->bfSize = bitmap_size;
    file_header->bfOffBits = pixel_offset;
    auto *header = reinterpret_cast<BITMAPINFOHEADER *>(bitmap + sizeof(BITMAPFILEHEADER));
    header->biSize = sizeof(BITMAPINFOHEADER);
    header->biWidth = static_cast<LONG>(width);
    header->biHeight = static_cast<LONG>(height);
    header->biPlanes = 1;
    header->biBitCount = 32;
    header->biCompression = BI_RGB;
    header->biSizeImage = destination_stride * height;
    auto *pixels = reinterpret_cast<uint32_t *>(bitmap + pixel_offset);

    for(uint32_t destination_y = 0; destination_y < height; ++destination_y)
    {
        const uint32_t source_y = source->height - 1 - destination_y * sample_step;
        const uint8_t *source_row = source->pixels + static_cast<size_t>(source_y) * source->stride;
        uint32_t *destination_row = pixels + static_cast<size_t>(destination_y) * width;
        for(uint32_t destination_x = 0; destination_x < width; ++destination_x)
        {
            const uint8_t *source_pixel = source_row + static_cast<size_t>(destination_x * sample_step) * bytes_per_pixel;
            if(source->bits_per_pixel == 8)
            {
                const PALETTEENTRY color = source->palette_entries[*source_pixel];
                destination_row[destination_x] = static_cast<uint32_t>(color.peRed) << 16 | static_cast<uint32_t>(color.peGreen) << 8 | color.peBlue;
                continue;
            }
            uint32_t pixel = 0;
            std::memcpy(&pixel, source_pixel, bytes_per_pixel);
            const uint8_t red = expand_masked_channel(pixel, source->red_mask);
            const uint8_t green = expand_masked_channel(pixel, source->green_mask);
            const uint8_t blue = expand_masked_channel(pixel, source->blue_mask);
            destination_row[destination_x] = static_cast<uint32_t>(red) << 16 | static_cast<uint32_t>(green) << 8 | blue;
        }
    }
    if(size != nullptr)
    {
        *size = bitmap_size;
    }
    return bitmap;
}

uint8_t find_nearest_palette_entry(uint8_t red, uint8_t green, uint8_t blue, const PALETTEENTRY *palette)
{
    uint32_t best_distance = UINT32_MAX;
    uint8_t best_index = 0;
    for(uint32_t index = 0; index < 256; ++index)
    {
        const int32_t red_difference = static_cast<int32_t>(red) - palette[index].peRed;
        const int32_t green_difference = static_cast<int32_t>(green) - palette[index].peGreen;
        const int32_t blue_difference = static_cast<int32_t>(blue) - palette[index].peBlue;
        const uint32_t distance = static_cast<uint32_t>(red_difference * red_difference + green_difference * green_difference + blue_difference * blue_difference);
        if(distance < best_distance)
        {
            best_distance = distance;
            best_index = static_cast<uint8_t>(index);
            if(distance == 0)
            {
                break;
            }
        }
    }
    return best_index;
}

void *create_indexed_display_bitmap(const DisplayBitmapCaptureSource *source, uint32_t *size, int half_resolution)
{
    if(size != nullptr)
    {
        *size = 0;
    }
    if(source == nullptr || source->pixels == nullptr || source->palette_entries == nullptr || source->width == 0 || source->height == 0)
    {
        return nullptr;
    }
    const uint32_t bytes_per_pixel = source->bits_per_pixel >> 3;
    if((source->bits_per_pixel != 8 && source->bits_per_pixel != 16 && source->bits_per_pixel != 24 && source->bits_per_pixel != 32) || source->width > UINT32_MAX / bytes_per_pixel
        || source->stride < source->width * bytes_per_pixel || (source->bits_per_pixel != 8 && (source->red_mask == 0 || source->green_mask == 0 || source->blue_mask == 0)))
    {
        return nullptr;
    }
    const uint32_t sample_step = half_resolution == 0 ? 1 : 2;
    const uint32_t width = source->width / sample_step;
    const uint32_t height = source->height / sample_step;
    if(width == 0 || height == 0 || width > UINT32_MAX - 3)
    {
        return nullptr;
    }
    const uint32_t destination_stride = (width + 3) & ~3u;
    constexpr uint32_t pixel_offset = sizeof(BITMAPFILEHEADER) + offsetof(RuntimeIndexedBitmapInfo, pixels);
    if(height > (UINT32_MAX - pixel_offset) / destination_stride)
    {
        return nullptr;
    }
    const uint32_t bitmap_size = pixel_offset + destination_stride * height;
    auto *bitmap = static_cast<uint8_t *>(bitmap_capture_api.heap_alloc(bitmap_capture_api.get_process_heap(), HEAP_ZERO_MEMORY, bitmap_size));
    if(bitmap == nullptr)
    {
        return nullptr;
    }
    auto *file_header = reinterpret_cast<BITMAPFILEHEADER *>(bitmap);
    file_header->bfType = 0x4d42;
    file_header->bfSize = bitmap_size;
    file_header->bfOffBits = pixel_offset;
    auto *indexed_bitmap = reinterpret_cast<RuntimeIndexedBitmapInfo *>(bitmap + sizeof(BITMAPFILEHEADER));
    indexed_bitmap->header.biSize = sizeof(BITMAPINFOHEADER);
    indexed_bitmap->header.biWidth = static_cast<LONG>(width);
    indexed_bitmap->header.biHeight = static_cast<LONG>(height);
    indexed_bitmap->header.biPlanes = 1;
    indexed_bitmap->header.biBitCount = 8;
    indexed_bitmap->header.biCompression = BI_RGB;
    indexed_bitmap->header.biSizeImage = destination_stride * height;
    indexed_bitmap->header.biClrUsed = 256;
    indexed_bitmap->header.biClrImportant = 256;
    for(uint32_t index = 0; index < 256; ++index)
    {
        indexed_bitmap->colors[index] = { source->palette_entries[index].peBlue, source->palette_entries[index].peGreen, source->palette_entries[index].peRed, 0 };
    }
    for(uint32_t destination_y = 0; destination_y < height; ++destination_y)
    {
        const uint32_t source_y = source->height - 1 - destination_y * sample_step;
        const uint8_t *source_row = source->pixels + static_cast<size_t>(source_y) * source->stride;
        uint8_t *destination_row = indexed_bitmap->pixels + static_cast<size_t>(destination_y) * destination_stride;
        for(uint32_t destination_x = 0; destination_x < width; ++destination_x)
        {
            const uint8_t *source_pixel = source_row + static_cast<size_t>(destination_x * sample_step) * bytes_per_pixel;
            if(source->bits_per_pixel == 8)
            {
                destination_row[destination_x] = *source_pixel;
                continue;
            }
            uint32_t pixel = 0;
            std::memcpy(&pixel, source_pixel, bytes_per_pixel);
            destination_row[destination_x] = find_nearest_palette_entry(expand_masked_channel(pixel, source->red_mask), expand_masked_channel(pixel, source->green_mask),
                expand_masked_channel(pixel, source->blue_mask), source->palette_entries);
        }
    }
    if(size != nullptr)
    {
        *size = bitmap_size;
    }
    return bitmap;
}

void *capture_bitmap_if_runtime_active(const BitmapCaptureSource *source, const uint8_t *palette, uint32_t *size, int half_resolution)
{
    if((graphics_host_flags & 0x800) == 0)
    {
        return nullptr;
    }
    return create_indexed_bitmap(source, palette, size, half_resolution);
}

void *capture_game_bitmap(void *game_context, uint32_t *size, int half_resolution)
{
    (void)game_context;
    if(runtime_display_scene_identifier == 0)
    {
        return nullptr;
    }
    const auto *scene = reinterpret_cast<const DisplaySceneNode *>(static_cast<uintptr_t>(runtime_display_scene_identifier));
    if(scene->width <= 0 || scene->height <= 0 || scene->sync_secondary_position <= 0)
    {
        return nullptr;
    }
    DisplayBitmapCaptureSource source{};
    source.width = static_cast<uint32_t>(scene->width);
    source.height = static_cast<uint32_t>(scene->height);
    source.stride = static_cast<uint32_t>(scene->sync_secondary_position);
    source.bits_per_pixel = scene->rectangle_callback_format.bits_per_pixel;
    source.red_mask = scene->rectangle_callback_format.red_mask;
    source.green_mask = scene->rectangle_callback_format.green_mask;
    source.blue_mask = scene->rectangle_callback_format.blue_mask;
    source.pixels = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position));
    source.palette_entries = display_palette_entries;
    return create_display_bitmap(&source, size, half_resolution);
}

void *capture_save_game_bitmap(void *game_context, uint32_t *size, int half_resolution)
{
    (void)game_context;
    if(runtime_display_scene_identifier == 0)
    {
        return nullptr;
    }
    const auto *scene = reinterpret_cast<const DisplaySceneNode *>(static_cast<uintptr_t>(runtime_display_scene_identifier));
    if(scene->width <= 0 || scene->height <= 0 || scene->sync_secondary_position <= 0)
    {
        return nullptr;
    }
    const DisplayBitmapCaptureSource source{ static_cast<uint32_t>(scene->width), static_cast<uint32_t>(scene->height), static_cast<uint32_t>(scene->sync_secondary_position),
        scene->rectangle_callback_format.bits_per_pixel, scene->rectangle_callback_format.red_mask, scene->rectangle_callback_format.green_mask, scene->rectangle_callback_format.blue_mask,
        reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position)), display_palette_entries };
    return create_indexed_display_bitmap(&source, size, half_resolution);
}


void clear_credits_runtime_flag()
{
    if((graphics_host_flags & 0x40000000) != 0)
    {
        graphics_host_flags &= 0xbfffffff;
    }
}



void set_application_lock_flag(ApplicationState *state)
{
    state->flags |= 0x40000000;
}

void set_application_inactive_flags(ApplicationState *state)
{
    uint32_t previous_flags = state->flags;
    state->flags = previous_flags | 0x1000002;
    state->flags = previous_flags | 0x9000002;
}

void clear_runtime_active_flag(ApplicationState *state)
{
    uint32_t previous_flags = state->flags;
    state->flags = previous_flags & 0xfeffffff;
    if((previous_flags & 0x40000000) == 0)
    {
        clear_cursor_flag_above_client(state);
    }
}

void clear_application_lock_flag(ApplicationState *state)
{
    uint32_t previous_flags = state->flags;
    state->flags = previous_flags & 0xbfffffff;
    if((previous_flags & 0x01000000) == 0)
    {
        clear_cursor_flag_above_client(state);
    }
}

void free_heap_memory(void *memory)
{
    if(memory != nullptr)
    {
        HeapFree(GetProcessHeap(), 0, memory);
    }
}



int append_string(char *destination, const char *source)
{
    int length = 0;
    while(destination[length] != '\0')
    {
        ++length;
    }
    return length + copy_string(destination + length, source);
}

bool strings_equal(const char *left, const char *right)
{
    for(;;)
    {
        if(*left != *right)
        {
            return false;
        }
        if(*left == '\0')
        {
            return true;
        }
        ++left;
        ++right;
    }
}

void copy_directory_from_path(char *destination, const char *source)
{
    int index = 0;
    while(source[index] != '\0')
    {
        ++index;
    }
    while(index >= 0 && source[index] != '\\')
    {
        --index;
    }
    destination[index + 1] = '\0';
    while(index >= 0)
    {
        destination[index] = source[index];
        --index;
    }
}

uint32_t load_local_preferences(ApplicationState *state)
{
    state->installation_path[0] = '\0';
    int64_t settings = 0;
    if(read_preference_number(game_preferences_section, "Settings", 0, UINT32_MAX, &settings))
    {
        state->flags |= static_cast<uint32_t>(settings) & 0x02001020;
    }

    state->flags |= (~state->flags & 0x20) << 2;
    return 2;
}


} // namespace gag
