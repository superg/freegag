#include "application.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include "host_events.h"
#include "portable_string.h"
#include "runtime_internal.h"
#include "runtime_services.h"

namespace gag
{
namespace
{
uint32_t host_event_type{};
RuntimeHeap save_capture_heap;

bool wake_host_event_loop(void *)
{
    SDL_Event event{};
    event.type = application_host_event_type();
    return event.type != UINT32_MAX && SDL_PushEvent(&event);
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

    // Script identifiers are compared as complete zero-padded 32-byte fields, not as C strings.
    char query_object_name[0x20]{};
    char query_field_name[0x20]{};
    if(query != nullptr)
    {
        copy_string(query_object_name, query->object_name.c_str());
        copy_string(query_field_name, query->field_name.c_str());
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
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
        }
        break;
    case 0x3eb:
    case 0x3ec:
    case 0x3ed:
        if(query != nullptr)
        {
            const uint32_t mask = event.command == 0x3eb ? 0x800000 : (event.command == 0x3ec ? 0x200000 : 0x400000);
            value = (state->saved_flags & mask) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
        }
        break;
    case 0x3f2:
        if((state->flags & 0x4000) != 0 && query != nullptr)
        {
            value = (state->flags & 0x20) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
        }
        break;
    case 0x3fc:
    case 0x406:
        if(query != nullptr)
        {
            const uint32_t mask = event.command == 0x3fc ? 0x1000 : 0x02000000;
            value = (state->flags & mask) != 0 ? 0x3000000 : 0x7000000;
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
        }
        break;
    case 0x456:
        if(query != nullptr)
        {
            value = (state->flags & 0x4000) != 0 ? 0x7000000 : 0x3000000;
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
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
            dispatch_application_action(state, ApplicationAction::new_game);
        else if(event.command == 0x7d4)
            dispatch_application_action(state, ApplicationAction::resume_saved_game);
        else
        {
            dispatch_application_action(state, (state->flags & 0x20) == 0 ? ApplicationAction::enter_fullscreen : ApplicationAction::leave_fullscreen);
        }
        break;
    case 0x7e4:
        dispatch_application_action(state, ApplicationAction::toggle_mute);
        break;
    case 0x7ee:
        dispatch_application_action(state, ApplicationAction::toggle_comments);
        break;
    case 0xbc2:
        if((state->flags & 0x80000) == 0)
        {
            if((state->flags & 0x800000) == 0)
            {
                state->saved_memory = capture_save_game_bitmap(state->game_context, nullptr, 1);
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
            resolve_state_field_reference(query_object_name, query_field_name, &value, 1);
            state->flags &= 0xfffbffff;
        }
        break;
    case 0x10000000:
        dispatch_application_action(state, ApplicationAction::exit);
        break;
    case 0x30000000:
        finish_credits_state(state, tree);
        break;
    case 0x40000000:
        process_state_activation(state, tree);
        break;
    case 0x60000000:
        enter_runtime_state_1000();
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
            set_script_runtime_flags(1, (state->flags & 0x02000000) == 0);
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
        state->shutdown_complete = true;
        shutdown_graphics_host();
        close_host_events();
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
            dispatch_application_action(state, ApplicationAction::exit);
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

uint32_t application_host_event_type()
{
    if(host_event_type == 0)
    {
        host_event_type = SDL_RegisterEvents(1);
    }
    return host_event_type;
}

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

GraphicsHostInitializationResult *initialize_graphics_host(int16_t width, uint16_t height, uint32_t flags)
{
    if((runtime_scene_control_flags & 0x800) != 0)
    {
        return &graphics_host_state;
    }

    runtime_display_context = {};
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

    if(initialized)
    {
        runtime_game_host_context.x_offset = 0;
        runtime_game_host_context.y_offset = 0;
        runtime_game_host_context.width = static_cast<uint16_t>(width + 3) & 0xfffc;
        runtime_game_host_context.height = static_cast<uint16_t>(height);
    }

    if(initialized)
    {
        runtime_pointer_x = 0;
        runtime_pointer_y = 0;
        initialized = graphics_host_api.initialize_display(runtime_game_host_context.width, runtime_game_host_context.height, flags & 0x300000) == 0;
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
    runtime_game_host_callbacks[20] = reinterpret_cast<void *>(static_cast<AsyncFileRecord *(*)(AsyncFileHost *, const char *, uint32_t, uint32_t, uint32_t)>(&open_async_file_record));
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
            result = subsystem_result & static_cast<uint32_t>(graphics_host_shutdown_api.heap_destroy(runtime_resource_heap));
            if(result != 0)
            {
                runtime_display_context = {};
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

    if((stages & 2) != 0)
    {
        validation_api.load_preferences(state);

        std::error_code error;
        const std::filesystem::path installation_path = state->installation_path[0] == '\0' ? std::filesystem::path(".") : std::filesystem::path(state->installation_path);
        if(!std::filesystem::exists(installation_path / auto_save_file_name, error))
        {
            state->flags |= 0x200000;
        }

        bool found_save = false;
        error.clear();
        for(std::filesystem::directory_iterator entry(installation_path, error), end; !error && entry != end; entry.increment(error))
        {
            std::string extension = entry->path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            if(entry->is_regular_file(error) && extension == ".gsf")
            {
                found_save = true;
                break;
            }
        }
        if(!found_save)
        {
            state->flags |= 0x100000;
        }
    }

    if((stages & 4) != 0 && state->archive_context == nullptr)
    {
        std::error_code error;
        if(!std::filesystem::exists(requested_archive, error))
        {
            if((stages & 0x20) != 0)
            {
                std::fprintf(stderr, "%s: %s\n", state->message_table, application_message(state, 15));
            }
            return 0;
        }
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
    if(gagboy_startup_mode && compare_ascii_case_insensitive(name, "GAGBOY.CFG") == 0)
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

ApplicationState *initialize_gag_application(int width, int height, bool start_xtet)
{
    gagboy_startup_mode = start_xtet;
    std::unique_ptr<ApplicationState> owned_state(new (std::nothrow) ApplicationState{});
    ApplicationState *state = owned_state.get();
    if(state == nullptr)
    {
        return nullptr;
    }
    desktop_presentation_state = {};
    desktop_fullscreen_toggle_latched = false;

    state->message_table = application_message_table[0];
    state->width = width;
    state->height = height;
    state->flags |= 2;
    application_initialization_api.copy_string(state->executable_directory, "Gag01.cdf");
    if(application_initialization_api.validate_environment(state, state->executable_directory, 0xfffef9ff) == 0)
    {
        return nullptr;
    }

    state->content_left = 0;
    state->content_top = 0;
    state->content_right = state->content_left + width;
    state->content_bottom = state->content_top + height;

    GraphicsHostInitializationResult *graphics = application_initialization_api.initialize_graphics_host(static_cast<int16_t>(width), static_cast<uint16_t>(height), 0x300000);
    if(graphics == nullptr)
    {
        close_host_events();
        return nullptr;
    }
    state->game_context = graphics;
    if((state->flags & 0x80) != 0)
    {
        PortableRectangle saved_rectangle{};
        if(load_saved_window_rectangle(width, height, &saved_rectangle))
        {
            set_sdl_presenter_window_rectangle({ saved_rectangle.left, saved_rectangle.top, saved_rectangle.right, saved_rectangle.bottom });
        }
        else
        {
            center_sdl_presenter_window();
        }
    }
    else
    {
        set_sdl_presenter_fullscreen(true);
    }
    if(!show_sdl_presenter())
    {
        shutdown_graphics_host();
        close_host_events();
        return nullptr;
    }
    application_host_event_type();
    initialize_host_events(wake_host_event_loop, nullptr, handle_host_event, state);
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
    if(!state->low_color_resources)
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
    return owned_state.release();
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
    runtime_media_backend_mutex = new (std::nothrow) RuntimeMutex;
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
    runtime_generic_backend_mutex = new (std::nothrow) RuntimeMutex;
    if(runtime_generic_backend_mutex == nullptr)
    {
        return 0;
    }
    runtime_backend_initialization_api.wait_for_single_object(runtime_generic_backend_mutex, runtime_infinite_wait);
    runtime_generic_backend_enabled = 1;
    runtime_backend_initialization_api.release_mutex(runtime_generic_backend_mutex);
    return 1;
}

uint32_t shutdown_runtime_generic_backend()
{
    if(runtime_generic_backend_enabled != 0)
    {
        runtime_generic_backend_shutdown_api.wait_for_single_object(runtime_generic_backend_mutex, runtime_infinite_wait);
        runtime_generic_backend_enabled = 0;
        runtime_generic_backend_shutdown_api.release_mutex(runtime_generic_backend_mutex);
        while(runtime_generic_backend_head != nullptr)
        {
            runtime_generic_backend_shutdown_api.destroy_backend(runtime_generic_backend_head);
        }
        delete runtime_generic_backend_mutex;
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
    delete runtime_media_backend_mutex;
    runtime_media_backend_mutex = nullptr;
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

void dispatch_application_action(ApplicationState *state, ApplicationAction action)
{
    if(action == ApplicationAction::pause)
    {
        set_runtime_flag_01000000();
        return;
    }
    if(action == ApplicationAction::resume)
    {
        clear_runtime_flag_01000000();
        return;
    }
    if(state == nullptr)
    {
        return;
    }
    if(action == ApplicationAction::exit)
    {
        set_application_inactive_flags(state);
        state->flags |= 0x200;
        set_runtime_flag_40();
        return;
    }
    if(action == ApplicationAction::save || action == ApplicationAction::load)
    {
        request_scripted_save_load_screen(action == ApplicationAction::save ? SaveLoadScreenMode::save : SaveLoadScreenMode::load, state);
        return;
    }
    if(action == ApplicationAction::new_game)
    {
        state->flags |= 0x200000;
        copy_string(state->startup_config, "NewGame.cfg");
        set_application_lock_flag(state);
        clear_runtime_active_flag(state);
        while(validate_startup_environment(state, state->executable_directory, 0x24) == 0)
        {
        }
        set_runtime_flag_40();
        return;
    }
    if(action == ApplicationAction::resume_saved_game)
    {
        state->flags |= 0x200000;
        set_application_lock_flag(state);
        clear_runtime_active_flag(state);
        while(validate_startup_environment(state, state->executable_directory, 0x24) == 0)
        {
        }
        copy_string(state->startup_config, "START.CFG");
        copy_string(state->installed_version, state->installation_path);
        append_string(state->installed_version, auto_save_file_name);
        set_runtime_flag_40();
        return;
    }
    if(action == ApplicationAction::credits)
    {
        set_application_lock_flag(state);
        clear_runtime_active_flag(state);
        while(validate_startup_environment(state, state->executable_directory, 0x24) == 0)
        {
        }
        set_runtime_paths_once("Credits.cfg", "CREDITS");
        return;
    }
    if(action == ApplicationAction::toggle_comments)
    {
        const bool subtitles_enabled = (state->flags & 0x02000000) != 0;
        if(subtitles_enabled)
        {
            state->flags &= 0xfdffffff;
        }
        else
        {
            state->flags |= 0x02000000;
        }
        set_script_runtime_flags(1, subtitles_enabled);
        state->flags |= 0x40000;
        return;
    }
    if(action == ApplicationAction::toggle_mute)
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
        return;
    }
    if(action == ApplicationAction::enter_fullscreen && (state->flags & 0x80) != 0)
    {
        state->flags = (state->flags & 0xffffff7f) | 0x40;
        set_runtime_flag_01000000();
    }
    else if(action == ApplicationAction::leave_fullscreen && (state->flags & 0x80) == 0)
    {
        state->flags |= 0xc0;
        set_runtime_flag_01000000();
    }
}

constexpr char preferences_file_name[] = "freegag.ini";
constexpr char game_preferences_section[] = "Game";
constexpr char window_preferences_section[] = "Window";

using PreferenceKey = std::pair<std::string, std::string>;
using Preferences = std::map<PreferenceKey, std::string>;

struct ApplicationPreferences
{
    bool fullscreen{};
    bool integer_scaling{ true };
    bool low_color_resources{};
    bool sound{ true };
    bool subtitles{};
    std::optional<PortableRectangle> window_rectangle;
};

std::string trim_preference_text(std::string value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if(first == std::string::npos)
    {
        return {};
    }
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

Preferences parse_preferences()
{
    Preferences preferences;
    std::ifstream stream(preferences_file_name);
    std::string section;
    std::string line;
    while(std::getline(stream, line))
    {
        line = trim_preference_text(std::move(line));
        if(line.empty() || line.front() == ';' || line.front() == '#')
        {
            continue;
        }
        if(line.front() == '[' && line.back() == ']')
        {
            section = trim_preference_text(line.substr(1, line.size() - 2));
            continue;
        }
        const size_t separator = line.find('=');
        if(separator != std::string::npos && !section.empty())
        {
            preferences[{ section, trim_preference_text(line.substr(0, separator)) }] = trim_preference_text(line.substr(separator + 1));
        }
    }
    return preferences;
}

bool read_preference_bool(const Preferences &preferences, const char *key, bool default_value = false)
{
    const auto found = preferences.find({ game_preferences_section, key });
    if(found == preferences.end())
    {
        return default_value;
    }
    if(compare_ascii_case_insensitive(found->second.c_str(), "true") == 0)
    {
        return true;
    }
    if(compare_ascii_case_insensitive(found->second.c_str(), "false") == 0)
    {
        return false;
    }
    return default_value;
}

bool read_preference_number(const Preferences &preferences, const char *section, const char *key, int64_t minimum, int64_t maximum, int64_t *result)
{
    const auto found = preferences.find({ section, key });
    if(found == preferences.end() || found->second.empty())
    {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    const char *value = found->second.c_str();
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

ApplicationPreferences read_preferences()
{
    const Preferences parsed = parse_preferences();
    ApplicationPreferences preferences;
    preferences.fullscreen = read_preference_bool(parsed, "Fullscreen");
    preferences.integer_scaling = read_preference_bool(parsed, "IntegerScaling", true);
    preferences.low_color_resources = read_preference_bool(parsed, "LowColorResources");
    preferences.sound = read_preference_bool(parsed, "Sound", true);
    preferences.subtitles = read_preference_bool(parsed, "Subtitles");

    int64_t left;
    int64_t top;
    int64_t right;
    int64_t bottom;
    if(read_preference_number(parsed, window_preferences_section, "Left", INT32_MIN, INT32_MAX, &left) && read_preference_number(parsed, window_preferences_section, "Top", INT32_MIN, INT32_MAX, &top)
        && read_preference_number(parsed, window_preferences_section, "Right", INT32_MIN, INT32_MAX, &right)
        && read_preference_number(parsed, window_preferences_section, "Bottom", INT32_MIN, INT32_MAX, &bottom))
    {
        preferences.window_rectangle = PortableRectangle{ static_cast<int32_t>(left), static_cast<int32_t>(top), static_cast<int32_t>(right), static_cast<int32_t>(bottom) };
    }
    return preferences;
}

void write_preferences(const ApplicationPreferences &preferences)
{
    std::ofstream stream(preferences_file_name, std::ios::trunc);
    if(!stream)
    {
        return;
    }
    stream << "[Game]\n";
    stream << "Fullscreen=" << (preferences.fullscreen ? "true" : "false") << '\n';
    stream << "IntegerScaling=" << (preferences.integer_scaling ? "true" : "false") << '\n';
    stream << "LowColorResources=" << (preferences.low_color_resources ? "true" : "false") << '\n';
    stream << "Sound=" << (preferences.sound ? "true" : "false") << '\n';
    stream << "Subtitles=" << (preferences.subtitles ? "true" : "false") << '\n';
    if(preferences.window_rectangle.has_value())
    {
        const PortableRectangle &rectangle = *preferences.window_rectangle;
        stream << "\n[Window]\n";
        stream << "Bottom=" << rectangle.bottom << '\n';
        stream << "Left=" << rectangle.left << '\n';
        stream << "Right=" << rectangle.right << '\n';
        stream << "Top=" << rectangle.top << '\n';
    }
}

bool read_saved_window_rectangle(PortableRectangle *rectangle)
{
    const ApplicationPreferences preferences = read_preferences();
    if(!preferences.window_rectangle.has_value())
    {
        return false;
    }
    *rectangle = *preferences.window_rectangle;
    return true;
}

void update_preferences_from_state(ApplicationPreferences *preferences, const ApplicationState *state)
{
    preferences->fullscreen = (state->flags & 0x20) != 0;
    preferences->low_color_resources = state->low_color_resources;
    preferences->sound = (state->flags & 0x1000) == 0;
    preferences->subtitles = (state->flags & 0x02000000) != 0;
}

bool window_rectangle_is_valid(const PortableRectangle &rectangle, int32_t minimum_width, int32_t minimum_height)
{
    const int64_t width = static_cast<int64_t>(rectangle.right) - rectangle.left;
    const int64_t height = static_cast<int64_t>(rectangle.bottom) - rectangle.top;
    return width >= minimum_width && height >= minimum_height && is_sdl_presenter_rectangle_visible({ rectangle.left, rectangle.top, rectangle.right, rectangle.bottom });
}

void save_runtime_settings(ApplicationState *state)
{
    ApplicationPreferences preferences = read_preferences();
    update_preferences_from_state(&preferences, state);
    if((state->flags & 0x80) != 0)
    {
        DisplayRectangle rectangle{};
        if(get_sdl_presenter_window_rectangle(&rectangle))
        {
            preferences.window_rectangle = PortableRectangle{ rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
            desktop_presentation_state.windowed_rectangle = { rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
            desktop_presentation_state.windowed_rectangle_valid = true;
        }
    }
    else if(preferences.window_rectangle.has_value() && !window_rectangle_is_valid(*preferences.window_rectangle, state->width, state->height))
    {
        preferences.window_rectangle.reset();
    }
    if(state->archive_context == nullptr)
    {
        write_preferences(preferences);
    }
}

bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, PortableRectangle *rectangle)
{
    if(rectangle == nullptr)
    {
        return false;
    }

    PortableRectangle saved_rectangle{};
    const bool loaded = read_saved_window_rectangle(&saved_rectangle);
    const bool valid = loaded && window_rectangle_is_valid(saved_rectangle, minimum_width, minimum_height);
    if(valid)
    {
        *rectangle = saved_rectangle;
    }
    return valid;
}

void save_window_position(ApplicationState *state)
{
    if(state == nullptr)
    {
        return;
    }
    DisplayRectangle rectangle{};
    if(!get_sdl_presenter_window_rectangle(&rectangle))
    {
        return;
    }

    ApplicationPreferences preferences = read_preferences();
    update_preferences_from_state(&preferences, state);
    preferences.window_rectangle = PortableRectangle{ rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
    write_preferences(preferences);
    desktop_presentation_state.windowed_rectangle = { rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
    desktop_presentation_state.windowed_rectangle_valid = true;
}

void set_game_cursor_active(ApplicationState *state, int active)
{
    if(active == 0)
    {
        if((state->flags & 1) == 0)
        {
            SDL_HideCursor();
            leave_runtime_state_1000();
            state->flags |= 1;
        }
    }
    else if((state->flags & 1) != 0)
    {
        SDL_ShowCursor();
        enter_runtime_state_1000();
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

void update_desktop_windowed_viewport(ApplicationState *state)
{
    if(state == nullptr || (state->flags & 0x80) == 0)
    {
        return;
    }
    state->content_left = 0;
    state->content_top = 0;
    state->content_right = state->width;
    state->content_bottom = state->height;
    runtime_game_host_context.x_offset = 0;
    runtime_game_host_context.y_offset = 0;
}

void update_application_window_layout(ApplicationState *state, SecondaryWindowLayout *secondary_layout)
{
    if(secondary_layout != nullptr)
    {
        secondary_layout->state = 0;
        return;
    }
    const bool fullscreen = (state->flags & 0x80) == 0;
    set_sdl_presenter_fullscreen(fullscreen);
    desktop_presentation_state.fullscreen = fullscreen;
    update_desktop_windowed_viewport(state);
}


void restore_application_display(ApplicationState *state)
{
    if((state->flags & 0x80) == 0)
    {
        state->flags |= 0x20;
        if(!desktop_presentation_state.fullscreen)
        {
            // Preserve the latest framed position even when the application is later closed while fullscreen is active.
            save_window_position(state);
        }
    }
    set_sdl_presenter_fullscreen((state->flags & 0x80) == 0);
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
            int32_t x;
            int32_t y;
            if(get_sdl_presenter_mouse_position(&x, &y))
            {
                if(x < 0 || y < 0 || x >= state->width || y >= state->height)
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

uint8_t find_nearest_palette_entry(uint8_t red, uint8_t green, uint8_t blue, const PaletteEntry *palette)
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
    constexpr uint32_t palette_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    constexpr uint32_t pixel_offset = palette_offset + 256 * sizeof(BitmapColor);
    if(height > (UINT32_MAX - pixel_offset) / destination_stride)
    {
        return nullptr;
    }
    const uint32_t bitmap_size = pixel_offset + destination_stride * height;
    auto *bitmap = static_cast<uint8_t *>(save_capture_heap.allocate(bitmap_size, true));
    if(bitmap == nullptr)
    {
        return nullptr;
    }
    const BitmapFileHeader file_header{ 0x4d42, bitmap_size, 0, 0, pixel_offset };
    const BitmapInfoHeader info_header{ sizeof(BitmapInfoHeader), static_cast<int32_t>(width), static_cast<int32_t>(height), 1, 8, 0, destination_stride * height, 0, 0, 256, 256 };
    encode_bitmap_file_header(bitmap, file_header);
    encode_bitmap_info_header(bitmap + sizeof(BitmapFileHeader), info_header);
    auto *colors = reinterpret_cast<BitmapColor *>(bitmap + palette_offset);
    uint8_t *pixels = bitmap + pixel_offset;
    for(uint32_t index = 0; index < 256; ++index)
    {
        colors[index] = { source->palette_entries[index].peBlue, source->palette_entries[index].peGreen, source->palette_entries[index].peRed, 0 };
    }
    for(uint32_t destination_y = 0; destination_y < height; ++destination_y)
    {
        const uint32_t source_y = source->height - 1 - destination_y * sample_step;
        const uint8_t *source_row = source->pixels + static_cast<size_t>(source_y) * source->stride;
        uint8_t *destination_row = pixels + static_cast<size_t>(destination_y) * destination_stride;
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
        save_capture_heap.release(memory);
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
    std::error_code error;
    const bool preferences_missing = !std::filesystem::exists(preferences_file_name, error) && !error;
    const ApplicationPreferences preferences = read_preferences();
    set_sdl_presenter_integer_scaling(preferences.integer_scaling);
    state->low_color_resources = preferences.low_color_resources;
    if(preferences.fullscreen)
    {
        state->flags |= 0x20;
    }
    if(!preferences.sound)
    {
        state->flags |= 0x1000;
    }
    if(preferences.subtitles)
    {
        state->flags |= 0x02000000;
    }
    if(preferences_missing)
    {
        write_preferences(preferences);
    }

    state->flags |= (~state->flags & 0x20) << 2;
    return 2;
}


} // namespace gag
