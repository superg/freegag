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
#include "portable_path.h"
#include "portable_string.h"
#include "runtime_internal.h"
#include "runtime_services.h"

namespace freegag
{
uint32_t host_event_type{};
RuntimeHeap save_capture_heap;

bool validate_and_select_application_archive(ApplicationState *state, const char *requested_archive, bool report_missing_archive);

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
        tree = *value;
    else
        query = std::get_if<HostStateFieldQuery>(&event.payload);

    // Script identifiers are compared as complete zero-padded 32-byte fields, not as C strings.
    char query_object_name[0x20]{};
    char query_field_name[0x20]{};
    if(query != nullptr)
    {
        copy_string(query_object_name, query->object_name.c_str());
        copy_string(query_field_name, query->field_name.c_str());
    }

    uint32_t script_boolean_value;
    if(handle_scripted_save_load_message(event.command, state))
        return uint32_t{};
    const auto command = static_cast<HostApplicationCommand>(event.command);
    switch(command)
    {
    case HostApplicationCommand::INITIALIZE_RUNTIME_FLAGS:
        state->flags |= APPLICATION_CURSOR_OUTSIDE | APPLICATION_RUNTIME_ACTIVE | APPLICATION_INACTIVE;
        break;
    case HostApplicationCommand::QUERY_LOAD_DISABLED:
        if(query != nullptr)
        {
            script_boolean_value = (state->saved_flags & APPLICATION_LOAD_DISABLED) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
        }
        break;
    case HostApplicationCommand::QUERY_SAVE_DISABLED:
    case HostApplicationCommand::QUERY_RESUME_DISABLED:
    case HostApplicationCommand::QUERY_NEW_GAME_DISABLED:
        if(query != nullptr)
        {
            const uint32_t mask = command == HostApplicationCommand::QUERY_SAVE_DISABLED
                                    ? APPLICATION_SAVE_DISABLED
                                    : (command == HostApplicationCommand::QUERY_RESUME_DISABLED ? APPLICATION_RESUME_DISABLED : APPLICATION_NEW_GAME_DISABLED);
            script_boolean_value = (state->saved_flags & mask) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
        }
        break;
    case HostApplicationCommand::QUERY_FULLSCREEN_ENABLED:
        if((state->flags & APPLICATION_FULLSCREEN_AVAILABLE) != 0 && query != nullptr)
        {
            script_boolean_value = (state->flags & APPLICATION_FULLSCREEN_PREFERENCE) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
        }
        break;
    case HostApplicationCommand::QUERY_SOUND_MUTED:
    case HostApplicationCommand::QUERY_SUBTITLES_ENABLED:
        if(query != nullptr)
        {
            const uint32_t mask = command == HostApplicationCommand::QUERY_SOUND_MUTED ? APPLICATION_SOUND_MUTED : APPLICATION_SUBTITLES_ENABLED;
            script_boolean_value = (state->flags & mask) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
        }
        break;
    case HostApplicationCommand::QUERY_FULLSCREEN_UNAVAILABLE:
        if(query != nullptr)
        {
            script_boolean_value = (state->flags & APPLICATION_FULLSCREEN_AVAILABLE) != 0 ? SCRIPT_BOOLEAN_FALSE : SCRIPT_BOOLEAN_TRUE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
        }
        break;
    case HostApplicationCommand::OPEN_LOAD_SCREEN:
    case HostApplicationCommand::OPEN_SAVE_SCREEN:
        request_scripted_save_load_screen(command == HostApplicationCommand::OPEN_SAVE_SCREEN ? SaveLoadScreenMode::SAVE : SaveLoadScreenMode::LOAD, state);
        break;
    case HostApplicationCommand::START_NEW_GAME:
    case HostApplicationCommand::RESUME_SAVED_GAME:
    case HostApplicationCommand::TOGGLE_FULLSCREEN:
        acknowledge_host_event(completion, uint32_t{ 1 });
        if(command == HostApplicationCommand::START_NEW_GAME)
            dispatch_application_action(state, ApplicationAction::NEW_GAME);
        else if(command == HostApplicationCommand::RESUME_SAVED_GAME)
            dispatch_application_action(state, ApplicationAction::RESUME_SAVED_GAME);
        else
            dispatch_application_action(state, (state->flags & APPLICATION_FULLSCREEN_PREFERENCE) == 0 ? ApplicationAction::ENTER_FULLSCREEN : ApplicationAction::LEAVE_FULLSCREEN);
        break;
    case HostApplicationCommand::TOGGLE_MUTE:
        dispatch_application_action(state, ApplicationAction::TOGGLE_MUTE);
        break;
    case HostApplicationCommand::TOGGLE_COMMENTS:
        dispatch_application_action(state, ApplicationAction::TOGGLE_COMMENTS);
        break;
    case HostApplicationCommand::CAPTURE_STATE_SNAPSHOT:
        if((state->flags & APPLICATION_SNAPSHOT_ACTIVE) == 0)
        {
            if((state->flags & APPLICATION_SAVE_DISABLED) == 0)
            {
                state->saved_memory = capture_save_game_bitmap(nullptr, 1);
                state->script_state = reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
            }
            state->saved_flags = state->flags;
            state->flags = (state->flags & ~APPLICATION_PREFERENCES_CHANGED) | APPLICATION_SNAPSHOT_ACTIVE;
        }
        else
        {
            state->saved_flags = (state->saved_flags & ~(APPLICATION_LOAD_DISABLED | APPLICATION_RESUME_DISABLED)) | (state->flags & (APPLICATION_LOAD_DISABLED | APPLICATION_RESUME_DISABLED));
        }
        state->saved_flags &= ~APPLICATION_NEW_GAME_DISABLED;
        break;
    case HostApplicationCommand::RELEASE_STATE_SNAPSHOT:
        if((state->flags & APPLICATION_SNAPSHOT_ACTIVE) != 0)
        {
            if(state->saved_memory != nullptr)
            {
                free_heap_memory(state->saved_memory);
                state->saved_memory = nullptr;
            }
            state->flags &= ~APPLICATION_SNAPSHOT_ACTIVE;
            state->saved_flags = 0;
        }
        break;
    case HostApplicationCommand::QUERY_PREFERENCES_CHANGED:
        if(query != nullptr)
        {
            script_boolean_value = (state->flags & APPLICATION_PREFERENCES_CHANGED) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
            resolve_state_field_reference(query_object_name, query_field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
            state->flags &= ~APPLICATION_PREFERENCES_CHANGED;
        }
        break;
    case HostApplicationCommand::CLOSE_REQUESTED:
        dispatch_application_action(state, ApplicationAction::EXIT);
        break;
    case HostApplicationCommand::CREDITS_FINISHED:
        finish_credits_state(state, tree);
        break;
    case HostApplicationCommand::STATE_ACTIVATED:
        process_state_activation(state, tree);
        break;
    case HostApplicationCommand::COMMAND_COMPLETED:
        suspend_runtime_state();
        if((state->flags & APPLICATION_DISPLAY_RESTORE_PENDING) != 0)
        {
            state->flags &= ~APPLICATION_DISPLAY_RESTORE_PENDING;
            restore_application_display(state);
            state->flags |= APPLICATION_PREFERENCES_CHANGED;
        }
        runtime_command_state = 0;
        resume_runtime_state();
        break;
    case HostApplicationCommand::RUNTIME_SHUTDOWN:
        if((state->flags & APPLICATION_EXIT_REQUESTED) == 0)
        {
            clear_runtime_display();
            construct_runtime_resource(state->installed_version, 0, 0, 0, 0, 0, 0, 0x200);
            set_script_runtime_flags(SCRIPT_RUNTIME_COMMENTS_SUPPRESSED, (state->flags & APPLICATION_SUBTITLES_ENABLED) == 0);
            construct_runtime_resource(state->startup_config, 0, 0, 0, 0, 0, 0, 0);
            set_script_runtime_flags(SCRIPT_RUNTIME_COMMENTS_SUPPRESSED, (state->flags & APPLICATION_SUBTITLES_ENABLED) == 0);
            state->flags &= ~APPLICATION_SNAPSHOT_ACTIVE;
            runtime_command_state = 0;
            break;
        }
        if(state->script_state != 0)
        {
            append_string(state->installation_path, auto_save_file_name);
            const bool saved = write_synchronized_cdf_package(state->installation_path, nullptr, nullptr, reinterpret_cast<void *>(state->script_state));
            (void)saved;
        }
        save_runtime_settings(state);
        state->shutdown_complete = true;
        shutdown_graphics_host();
        close_host_events();
        runtime_command_state = 0;
        break;
    case HostApplicationCommand::VALIDATE_RESOURCE_PATH:
        if(const auto *path = std::get_if<std::string>(&event.payload))
        {
            char validated_path[0x104]{};
            copy_string(validated_path, path->c_str());
            validate_and_select_application_archive(state, validated_path, false);
            return std::string(state->installed_version);
        }
        break;
    case HostApplicationCommand::EXTRACT_FILE_NAME:
        if(const auto *path = std::get_if<std::string>(&event.payload))
        {
            char file_name[0x104]{};
            copy_string(file_name, path->c_str());
            copy_file_name_from_path(file_name, file_name);
            return std::string(file_name);
        }
        break;
    case HostApplicationCommand::STORAGE_FAILURE:
    case HostApplicationCommand::ANIMATION_FAILURE:
    case HostApplicationCommand::RUNTIME_FAILURE:
        if((state->flags & APPLICATION_FATAL_ERROR) == 0)
        {
            state->flags |= APPLICATION_FATAL_ERROR;
            dispatch_application_action(state, ApplicationAction::EXIT);
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
        return handle_application_host_event(*application_event, completion, state);
    if(std::holds_alternative<HostPresentPendingFramesEvent>(event))
        drain_sdl_presenter_frames();
    else if(const auto *xtet_event = std::get_if<HostXtEtEvent>(&event))
        handle_runtime_xtet_host_event(*xtet_event);
    return {};
}

uint32_t application_host_event_type()
{
    if(host_event_type == 0)
        host_event_type = SDL_RegisterEvents(1);
    return host_event_type;
}

void set_runtime_script_property(ScriptRuntimeProperty property, RuntimeGenericResourceNode *value)
{
    switch(property)
    {
    case ScriptRuntimeProperty::PALETTE_TRANSITION_STEP:
        runtime_palette_transition_step = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
        break;
    case ScriptRuntimeProperty::RECTANGLE_TRANSITION_STEP_SIZE:
        runtime_rectangle_transition_step_size = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
        break;
    case ScriptRuntimeProperty::AVAILABLE_SCENE_TRANSITIONS:
        runtime_available_scene_transitions = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)) & RUNTIME_SCENE_TRANSITION_AVAILABILITY_MASK;
        break;
    case ScriptRuntimeProperty::RESOURCE_PATH:
        select_runtime_resource(reinterpret_cast<char *>(value));
        break;
    case ScriptRuntimeProperty::RELEASE_RESOURCE:
        release_runtime_memory_resource(reinterpret_cast<char *>(value));
        break;
    case ScriptRuntimeProperty::SHARED_VALUE:
        runtime_property_value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value));
        break;
    case ScriptRuntimeProperty::RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND:
        runtime_resource_stream_rate_bytes_per_millisecond = static_cast<int32_t>(reinterpret_cast<intptr_t>(value));
        break;
    case ScriptRuntimeProperty::BEGIN_SUSPENDED_TRANSITION:
        if(suspended_runtime_state_count == 0)
        {
            suspend_runtime_state();
            runtime_scene_control_flags |= RUNTIME_HOST_SCENE_TRANSITION_GUARDED;
        }
        ++suspended_runtime_state_count;
        break;
    case ScriptRuntimeProperty::END_SUSPENDED_TRANSITION:
        if(suspended_runtime_state_count == 1)
        {
            runtime_scene_control_flags &= ~RUNTIME_HOST_SCENE_TRANSITION_GUARDED;
            resume_runtime_state();
        }
        if(suspended_runtime_state_count != 0)
            --suspended_runtime_state_count;
        break;
    case ScriptRuntimeProperty::BEGIN_PROPERTY_STATE:
        if(runtime_state_4_count == 0)
            runtime_scene_control_flags |= RUNTIME_HOST_PROPERTY_STATE_ACTIVE;
        ++runtime_state_4_count;
        break;
    case ScriptRuntimeProperty::END_PROPERTY_STATE:
        if(runtime_state_4_count == 1)
            runtime_scene_control_flags &= ~RUNTIME_HOST_PROPERTY_STATE_ACTIVE;
        if(runtime_state_4_count != 0)
            --runtime_state_4_count;
        break;
    case ScriptRuntimeProperty::DESTROY_TREE:
        destroy_runtime_tree_resources(value);
        break;
    case ScriptRuntimeProperty::MISSING_SOURCE:
    case ScriptRuntimeProperty::MISSING_SECTION:
        send_application_event(HostApplicationCommand::RUNTIME_FAILURE);
        break;
    }
}

void get_runtime_script_property(ScriptRuntimeProperty property, void **value, void *result)
{
    switch(property)
    {
    case ScriptRuntimeProperty::PALETTE_TRANSITION_STEP:
        *static_cast<uint32_t *>(result) = runtime_palette_transition_step;
        break;
    case ScriptRuntimeProperty::RECTANGLE_TRANSITION_STEP_SIZE:
        *static_cast<uint32_t *>(result) = runtime_rectangle_transition_step_size;
        break;
    case ScriptRuntimeProperty::AVAILABLE_SCENE_TRANSITIONS:
        *static_cast<uint32_t *>(result) = runtime_available_scene_transitions;
        break;
    case ScriptRuntimeProperty::RESOURCE_PATH:
    {
        char path[260];
        copy_string(path, runtime_graphics_resource_directory);
        HostEventResult event_result = send_application_event(HostApplicationCommand::EXTRACT_FILE_NAME, std::string(path));
        if(const auto *file_name = std::get_if<std::string>(&event_result))
            copy_string(path, file_name->c_str());
        copy_string(static_cast<char *>(result), path);
        break;
    }
    case ScriptRuntimeProperty::RESOURCE_DATA:
    {
        void *data;
        int32_t storage;
        load_runtime_resource(static_cast<const char *>(*value), &data, static_cast<uint32_t *>(result), &storage, 0x20000000);
        *value = data;
        break;
    }
    case ScriptRuntimeProperty::SHARED_VALUE:
        *static_cast<uint32_t *>(result) = runtime_property_value;
        break;
    case ScriptRuntimeProperty::POINTER_X:
        *static_cast<int32_t *>(result) = runtime_pointer_x;
        break;
    case ScriptRuntimeProperty::POINTER_Y:
        *static_cast<int32_t *>(result) = runtime_pointer_y;
        break;
    case ScriptRuntimeProperty::RESOURCE_FRAME:
        *static_cast<uint32_t *>(result) = query_runtime_resource_frame_number(*value);
        break;
    case ScriptRuntimeProperty::RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND:
        *static_cast<int32_t *>(result) = runtime_resource_stream_rate_bytes_per_millisecond;
        break;
    }
}

bool initialize_graphics_host(int16_t width, uint16_t height)
{
    if((runtime_scene_control_flags & RUNTIME_HOST_INITIALIZED) != 0)
        return true;

    runtime_display_context = {};
    std::memset(runtime_graphics_resource_directory, 0, sizeof(runtime_graphics_resource_directory));
    std::memset(runtime_transition_palette, 0, sizeof(runtime_transition_palette));
    std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
    runtime_game_host_context = {};
    graphics_script_runtime_root = {};
    bool initialized = initialize_runtime_media_backend() != 0;
    if(initialized)
        initialized = initialize_async_file_subsystem() != 0;
    if(initialized)
        initialized = initialize_runtime_generic_backend() != 0;
    if(initialized)
    {
        runtime_resource_heap = create_runtime_heap(0, 0, 0);
        initialized = runtime_resource_heap != nullptr;
    }

    if(initialized)
    {
        runtime_game_host_context.width = static_cast<uint16_t>(width + 3) & 0xfffc;
        runtime_game_host_context.height = static_cast<uint16_t>(height);
    }

    if(initialized)
    {
        runtime_pointer_x = 0;
        runtime_pointer_y = 0;
        initialized = initialize_sdl_presenter(runtime_game_host_context.width, runtime_game_host_context.height) == 0;
    }

    runtime_target_flags = RUNTIME_TARGET_ACTIVE;
    if(!initialized)
        return false;

    runtime_palette_transition_step = RUNTIME_DEFAULT_PALETTE_TRANSITION_STEP;
    runtime_resource_stream_rate_bytes_per_millisecond = RUNTIME_DEFAULT_RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND;
    runtime_rectangle_transition_step_size = RUNTIME_DEFAULT_RECTANGLE_TRANSITION_STEP_SIZE;
    runtime_available_scene_transitions = RUNTIME_DEFAULT_AVAILABLE_SCENE_TRANSITIONS;
    graphics_script_runtime_root.self = &graphics_script_runtime_root;
    graphics_script_runtime_root.set_property = set_runtime_script_property;
    graphics_script_runtime_root.get_property = get_runtime_script_property;
    graphics_script_runtime_root.heap = create_runtime_heap(0, 0, 0);
    if(graphics_script_runtime_root.heap == nullptr)
        return false;

    set_script_runtime_root_if_valid(&graphics_script_runtime_root);
    runtime_resource_cache_parent_identity = get_or_create_runtime_named_node("OpenMemoryFilesList");
    set_runtime_named_node_enabled(runtime_resource_cache_parent_identity, 1);
    runtime_media_objects_parent_identity = get_or_create_runtime_named_node("MMediaObjectsList");
    set_runtime_named_node_enabled(runtime_media_objects_parent_identity, 1);
    runtime_scene_control_flags |= RUNTIME_HOST_INITIALIZED;
    return true;
}

uint32_t shutdown_graphics_host()
{
    const uint32_t display_result = shutdown_runtime_display();
    uint32_t result = 0;
    if(display_result != 0)
    {
        const uint32_t generic_result = shutdown_runtime_generic_backend();
        const uint32_t async_result = shutdown_async_file_subsystem();
        const uint32_t media_result = shutdown_runtime_media_backend();
        const uint32_t subsystem_result = display_result & generic_result & async_result & media_result;
        shutdown_sdl_presenter();
        if(subsystem_result != 0)
        {
            result = subsystem_result & static_cast<uint32_t>(destroy_runtime_heap(runtime_resource_heap));
            if(result != 0)
            {
                runtime_display_context = {};
                std::memset(runtime_graphics_resource_directory, 0, sizeof(runtime_graphics_resource_directory));
                std::memset(runtime_transition_palette, 0, sizeof(runtime_transition_palette));
                std::memset(runtime_session_reset_storage, 0, sizeof(runtime_session_reset_storage));
                runtime_game_host_context = {};
                graphics_script_runtime_root = {};
                runtime_scene_control_flags &= ~RUNTIME_HOST_INITIALIZED;
            }
        }
    }
    return result;
}



void clear_runtime_display()
{
    if(acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        set_display_clip_rectangle(nullptr);
        operate_display_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
        release_display_lock();
        DisplayRectangle rectangle{ 0, 0, runtime_game_host_context.width, runtime_game_host_context.height };
        update_display_root_region(nullptr, &rectangle, 0);
    }
}



void disable_unavailable_saved_game_actions(ApplicationState *state)
{
    std::error_code error;
    const std::filesystem::path installation_path = state->installation_path[0] == '\0' ? std::filesystem::path(".") : std::filesystem::path(state->installation_path);
    std::filesystem::path resolved_auto_save;
    if(!resolve_existing_host_path_case_insensitive(installation_path / auto_save_file_name, &resolved_auto_save))
        state->flags |= APPLICATION_RESUME_DISABLED;

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
        state->flags |= APPLICATION_LOAD_DISABLED;
}

bool validate_and_select_application_archive(ApplicationState *state, const char *requested_archive, bool report_missing_archive)
{
    std::filesystem::path resolved_archive;
    if(state->archive_context == nullptr)
    {
        if(!resolve_existing_host_path_case_insensitive(requested_archive, &resolved_archive))
        {
            if(report_missing_archive)
                std::fputs("GAG: Unable to open data file...\n\nMake sure you insert one of the CD's\ninto your CD drive!\n", stderr);
            return false;
        }
    }
    else
        state->installed_version[0] = '\0';

    const std::string selected_archive = resolved_archive.empty() ? requested_archive : resolved_archive.string();
    copy_string(state->installed_version, selected_archive.c_str());
    return true;
}


bool find_virtual_runtime_script(const char *path, VirtualScriptResource *resource)
{
    if(path == nullptr || resource == nullptr)
        return false;
    const char *name = path;
    for(const char *cursor = path; *cursor != '\0'; ++cursor)
        if(*cursor == '\\' || *cursor == '/')
            name = cursor + 1;
    return find_save_load_virtual_script(name, resource);
}

ApplicationState *initialize_gag_application(int width, int height, bool use_xtet_startup_script)
{
    std::unique_ptr<ApplicationState> owned_state(new (std::nothrow) ApplicationState{});
    ApplicationState *state = owned_state.get();
    if(state == nullptr)
        return nullptr;
    desktop_presentation_state = {};
    desktop_fullscreen_toggle_latched = false;

    state->width = width;
    state->height = height;
    state->flags |= APPLICATION_CURSOR_OUTSIDE;
    copy_string(state->executable_directory, "Gag01.cdf");
    load_local_preferences(state);
    disable_unavailable_saved_game_actions(state);
    if(!validate_and_select_application_archive(state, state->executable_directory, true))
        return nullptr;

    if(!initialize_graphics_host(static_cast<int16_t>(width), static_cast<uint16_t>(height)))
    {
        close_host_events();
        return nullptr;
    }
    if((state->flags & APPLICATION_WINDOWED) != 0)
    {
        PortableRectangle saved_rectangle{};
        if(load_saved_window_rectangle(width, height, &saved_rectangle))
            set_sdl_presenter_window_rectangle({ saved_rectangle.left, saved_rectangle.top, saved_rectangle.right, saved_rectangle.bottom });
        else
            center_sdl_presenter_window();
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
    state->flags |= APPLICATION_FULLSCREEN_AVAILABLE;
    if(!initialize_runtime_graphics())
    {
        shutdown_graphics_host();
        close_host_events();
        return nullptr;
    }

    update_application_window_layout(state);
    if((state->flags & APPLICATION_SOUND_MUTED) != 0)
        enable_runtime_subsystem();
    if(!state->low_color_resources)
        set_runtime_resource_variant(1);
    if(use_xtet_startup_script)
    {
        copy_string(state->startup_config, "FGGAGBOY.CFG");
    }
    else
    {
        copy_string(state->startup_config, "Start.cfg");
        if(state->archive_context != nullptr && detect_runtime_resource_type(state->installed_version) == RUNTIME_MEDIA_DATA_CONFIGURATION)
            copy_string(state->startup_config, state->installed_version);
    }
    return owned_state.release();
}



uint32_t initialize_runtime_media_backend()
{
    if(runtime_media_backend_initialized)
        return 1;
    runtime_media_backend_heap = create_runtime_heap(0, 0, 0);
    if(runtime_media_backend_heap == nullptr)
        return 0;
    runtime_media_backend_mutex = new (std::nothrow) RuntimeMutex;
    if(runtime_media_backend_mutex == nullptr)
        return 0;
    initialize_runtime_sound();
    runtime_media_backend_initialized = true;
    return 1;
}

uint32_t initialize_runtime_generic_backend()
{
    if(runtime_generic_backend_enabled != 0)
        return 1;
    runtime_generic_backend_mutex = new (std::nothrow) RuntimeMutex;
    if(runtime_generic_backend_mutex == nullptr)
        return 0;
    lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
    runtime_generic_backend_enabled = 1;
    unlock_runtime_mutex(runtime_generic_backend_mutex);
    return 1;
}

uint32_t shutdown_runtime_generic_backend()
{
    if(runtime_generic_backend_enabled != 0)
    {
        lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
        runtime_generic_backend_enabled = 0;
        unlock_runtime_mutex(runtime_generic_backend_mutex);
        while(runtime_generic_backend_head != nullptr)
            destroy_runtime_generic_backend(runtime_generic_backend_head);
        delete runtime_generic_backend_mutex;
        runtime_generic_backend_mutex = nullptr;
    }
    return 1;
}


uint32_t shutdown_runtime_media_backend()
{
    if(!runtime_media_backend_initialized)
        return 0;
    RuntimeMediaBackend *backend = acquire_runtime_media_backend(runtime_media_backend_head);
    if(backend != nullptr)
    {
        release_runtime_media_backend_lock(backend);
        return 0;
    }
    destroy_runtime_heap(runtime_media_backend_heap);
    delete runtime_media_backend_mutex;
    runtime_media_backend_mutex = nullptr;
    runtime_media_backend_initialized = false;
    shutdown_runtime_sound();
    return 1;
}


uint32_t initialize_async_file_subsystem()
{
    if(async_file_enabled)
        return 1;
    async_file_enabled = true;
    return 1;
}



void set_script_runtime_root_if_valid(ScriptRuntimeRoot *root)
{
    if(root != reinterpret_cast<ScriptRuntimeRoot *>(static_cast<intptr_t>(-1)))
        script_runtime_root = root;
}

void set_runtime_named_node_enabled(void *identity, int enabled)
{
    for(RuntimeNamedNode *node = script_runtime_root->runtime_nodes; node != nullptr; node = node->next)
    {
        if(node->identity == identity)
        {
            if(enabled != 0)
                node->flags |= RUNTIME_NAMED_NODE_ENABLED;
            else
                node->flags &= ~RUNTIME_NAMED_NODE_ENABLED;
            return;
        }
    }
}

void dispatch_application_action(ApplicationState *state, ApplicationAction action)
{
    if(action == ApplicationAction::PAUSE)
    {
        graphics_host_flags |= RUNTIME_HOST_PAUSED;
        return;
    }
    if(action == ApplicationAction::RESUME)
    {
        graphics_host_flags &= ~RUNTIME_HOST_PAUSED;
        return;
    }
    if(state == nullptr)
        return;
    if(action == ApplicationAction::EXIT)
    {
        state->flags |= APPLICATION_CURSOR_OUTSIDE | APPLICATION_RUNTIME_ACTIVE | APPLICATION_INACTIVE;
        state->flags |= APPLICATION_EXIT_REQUESTED;
        graphics_host_flags |= RUNTIME_HOST_COMMAND_STOP_REQUESTED;
        return;
    }
    if(action == ApplicationAction::SAVE || action == ApplicationAction::LOAD)
    {
        request_scripted_save_load_screen(action == ApplicationAction::SAVE ? SaveLoadScreenMode::SAVE : SaveLoadScreenMode::LOAD, state);
        return;
    }
    if(action == ApplicationAction::NEW_GAME)
    {
        state->flags |= APPLICATION_RESUME_DISABLED;
        copy_string(state->startup_config, "NewGame.cfg");
        state->flags |= APPLICATION_LOCKED;
        clear_runtime_active_flag(state);
        while(!validate_and_select_application_archive(state, state->executable_directory, true))
        {
        }
        graphics_host_flags |= RUNTIME_HOST_COMMAND_STOP_REQUESTED;
        return;
    }
    if(action == ApplicationAction::RESUME_SAVED_GAME)
    {
        state->flags |= APPLICATION_RESUME_DISABLED;
        state->flags |= APPLICATION_LOCKED;
        clear_runtime_active_flag(state);
        while(!validate_and_select_application_archive(state, state->executable_directory, true))
        {
        }
        copy_string(state->startup_config, "START.CFG");
        copy_string(state->installed_version, state->installation_path);
        append_string(state->installed_version, auto_save_file_name);
        graphics_host_flags |= RUNTIME_HOST_COMMAND_STOP_REQUESTED;
        return;
    }
    if(action == ApplicationAction::CREDITS)
    {
        state->flags |= APPLICATION_LOCKED;
        clear_runtime_active_flag(state);
        while(!validate_and_select_application_archive(state, state->executable_directory, true))
        {
        }
        set_runtime_paths_once("Credits.cfg", "CREDITS");
        return;
    }
    if(action == ApplicationAction::TOGGLE_COMMENTS)
    {
        const bool subtitles_enabled = (state->flags & APPLICATION_SUBTITLES_ENABLED) != 0;
        if(subtitles_enabled)
            state->flags &= ~APPLICATION_SUBTITLES_ENABLED;
        else
            state->flags |= APPLICATION_SUBTITLES_ENABLED;
        set_script_runtime_flags(SCRIPT_RUNTIME_COMMENTS_SUPPRESSED, subtitles_enabled);
        state->flags |= APPLICATION_PREFERENCES_CHANGED;
        return;
    }
    if(action == ApplicationAction::TOGGLE_MUTE)
    {
        if((state->flags & APPLICATION_SOUND_MUTED) == 0)
        {
            state->flags |= APPLICATION_SOUND_MUTED;
            enable_runtime_subsystem();
        }
        else
        {
            state->flags &= ~APPLICATION_SOUND_MUTED;
            disable_runtime_subsystem();
        }
        state->flags |= APPLICATION_PREFERENCES_CHANGED;
        return;
    }
    if(action == ApplicationAction::ENTER_FULLSCREEN && (state->flags & APPLICATION_WINDOWED) != 0)
    {
        state->flags = (state->flags & ~APPLICATION_WINDOWED) | APPLICATION_DISPLAY_RESTORE_PENDING;
        graphics_host_flags |= RUNTIME_HOST_PAUSED;
    }
    else if(action == ApplicationAction::LEAVE_FULLSCREEN && (state->flags & APPLICATION_WINDOWED) == 0)
    {
        state->flags |= APPLICATION_DISPLAY_RESTORE_PENDING | APPLICATION_WINDOWED;
        graphics_host_flags |= RUNTIME_HOST_PAUSED;
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
    const size_t first = value.find_first_not_of(" \t\n");
    if(first == std::string::npos)
        return {};
    const size_t last = value.find_last_not_of(" \t\n");
    return value.substr(first, last - first + 1);
}

Preferences parse_preferences()
{
    Preferences preferences;
    std::filesystem::path preferences_path;
    if(!resolve_existing_host_path_case_insensitive(preferences_file_name, &preferences_path))
        preferences_path = preferences_file_name;
    std::ifstream stream(preferences_path);
    std::string section;
    std::string line;
    while(std::getline(stream, line))
    {
        line = trim_preference_text(std::move(line));
        if(line.empty() || line.front() == ';' || line.front() == '#')
            continue;
        if(line.front() == '[' && line.back() == ']')
        {
            section = trim_preference_text(line.substr(1, line.size() - 2));
            continue;
        }
        const size_t separator = line.find('=');
        if(separator != std::string::npos && !section.empty())
            preferences[{ section, trim_preference_text(line.substr(0, separator)) }] = trim_preference_text(line.substr(separator + 1));
    }
    return preferences;
}

bool read_preference_bool(const Preferences &preferences, const char *key, bool default_value = false)
{
    const auto found = preferences.find({ game_preferences_section, key });
    if(found == preferences.end())
        return default_value;
    if(compare_ascii_case_insensitive(found->second.c_str(), "true") == 0)
        return true;
    if(compare_ascii_case_insensitive(found->second.c_str(), "false") == 0)
        return false;
    return default_value;
}

bool read_preference_number(const Preferences &preferences, const char *section, const char *key, int64_t minimum, int64_t maximum, int64_t *result)
{
    const auto found = preferences.find({ section, key });
    if(found == preferences.end() || found->second.empty())
        return false;

    errno = 0;
    char *end = nullptr;
    const char *value = found->second.c_str();
    const long long parsed = std::strtoll(value, &end, 0);
    while(end != nullptr && std::isspace(static_cast<unsigned char>(*end)))
        ++end;
    if(errno == ERANGE || end == value || end == nullptr || *end != '\0' || parsed < minimum || parsed > maximum)
        return false;
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
    std::filesystem::path preferences_path;
    if(!resolve_existing_host_path_case_insensitive(preferences_file_name, &preferences_path))
        preferences_path = preferences_file_name;
    std::ofstream stream(preferences_path, std::ios::trunc);
    if(!stream)
        return;
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
        return false;
    *rectangle = *preferences.window_rectangle;
    return true;
}

void update_preferences_from_state(ApplicationPreferences *preferences, const ApplicationState *state)
{
    preferences->fullscreen = (state->flags & APPLICATION_FULLSCREEN_PREFERENCE) != 0;
    preferences->low_color_resources = state->low_color_resources;
    preferences->sound = (state->flags & APPLICATION_SOUND_MUTED) == 0;
    preferences->subtitles = (state->flags & APPLICATION_SUBTITLES_ENABLED) != 0;
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
    if((state->flags & APPLICATION_WINDOWED) != 0)
    {
        DisplayRectangle rectangle{};
        if(get_sdl_presenter_window_rectangle(&rectangle))
            preferences.window_rectangle = PortableRectangle{ rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
    }
    else if(preferences.window_rectangle.has_value() && !window_rectangle_is_valid(*preferences.window_rectangle, state->width, state->height))
    {
        preferences.window_rectangle.reset();
    }
    if(state->archive_context == nullptr)
        write_preferences(preferences);
}

bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, PortableRectangle *rectangle)
{
    if(rectangle == nullptr)
        return false;

    PortableRectangle saved_rectangle{};
    const bool loaded = read_saved_window_rectangle(&saved_rectangle);
    const bool valid = loaded && window_rectangle_is_valid(saved_rectangle, minimum_width, minimum_height);
    if(valid)
        *rectangle = saved_rectangle;
    return valid;
}

void save_window_position(ApplicationState *state)
{
    if(state == nullptr)
        return;
    DisplayRectangle rectangle{};
    if(!get_sdl_presenter_window_rectangle(&rectangle))
        return;

    ApplicationPreferences preferences = read_preferences();
    update_preferences_from_state(&preferences, state);
    preferences.window_rectangle = PortableRectangle{ rectangle.left, rectangle.top, rectangle.right, rectangle.bottom };
    write_preferences(preferences);
}

void set_game_cursor_active(ApplicationState *state, int active)
{
    if(active == 0)
    {
        if((state->flags & APPLICATION_CURSOR_HIDDEN) == 0)
        {
            SDL_HideCursor();
            resume_runtime_state();
            state->flags |= APPLICATION_CURSOR_HIDDEN;
        }
    }
    else if((state->flags & APPLICATION_CURSOR_HIDDEN) != 0)
    {
        SDL_ShowCursor();
        suspend_runtime_state();
        state->flags &= ~APPLICATION_CURSOR_HIDDEN;
    }
}

void finish_credits_state(ApplicationState *state, RuntimeTreeNode *tree)
{
    if(tree->parent == nullptr && (state->flags & APPLICATION_CREDITS_ACTIVE) != 0 && strings_equal("CREDITS", tree->name))
    {
        state->flags &= ~APPLICATION_CREDITS_ACTIVE;
        graphics_host_flags &= ~RUNTIME_HOST_CREDITS_ACTIVE;
    }
}

void update_application_window_layout(ApplicationState *state)
{
    const bool fullscreen = (state->flags & APPLICATION_WINDOWED) == 0;
    set_sdl_presenter_fullscreen(fullscreen);
    desktop_presentation_state.fullscreen = fullscreen;
}


void restore_application_display(ApplicationState *state)
{
    if((state->flags & APPLICATION_WINDOWED) == 0)
    {
        state->flags |= APPLICATION_FULLSCREEN_PREFERENCE;
        if(!desktop_presentation_state.fullscreen)
        {
            // Preserve the latest framed position even when the application is later closed while fullscreen is active.
            save_window_position(state);
        }
    }
    set_sdl_presenter_fullscreen((state->flags & APPLICATION_WINDOWED) == 0);
    update_application_window_layout(state);
    if((state->flags & APPLICATION_WINDOWED) != 0)
        state->flags &= ~APPLICATION_FULLSCREEN_PREFERENCE;
    graphics_host_flags &= ~RUNTIME_HOST_PAUSED;
}

void process_state_activation(ApplicationState *state, RuntimeTreeNode *tree)
{
    // Process the RuntimeTreeNode directly; treating its prefix as a separate state record is unsafe when pointers widen.
    if(tree->parent != nullptr || runtime_display_context.runtime_tree_identity != tree)
        return;
    if(current_runtime_scene_identity != nullptr)
    {
        clear_application_lock_flag(state);
        if((state->flags & APPLICATION_CURSOR_HIDDEN) == 0)
        {
            int32_t x;
            int32_t y;
            if(get_sdl_presenter_mouse_position(&x, &y))
            {
                if(x < 0 || y < 0 || x >= state->width || y >= state->height)
                    suspend_runtime_state();
                else
                    set_game_cursor_active(state, 0);
            }
        }
    }
    if(strings_equal("CREDITS", tree->name))
    {
        graphics_host_flags |= RUNTIME_HOST_CREDITS_ACTIVE;
        state->flags |= APPLICATION_CREDITS_ACTIVE;
    }
    void *scene_identity = current_runtime_resource;
    uint32_t status = query_runtime_scene_flags(scene_identity);
    if(status == 0)
    {
        uint32_t previous_flags = state->flags;
        if((previous_flags & APPLICATION_SNAPSHOT_ACTIVE) == 0)
            state->flags = previous_flags | APPLICATION_NEW_GAME_DISABLED | APPLICATION_SAVE_DISABLED;
        return;
    }
    uint32_t previous_flags = state->flags;
    if((previous_flags & APPLICATION_SNAPSHOT_ACTIVE) == 0)
    {
        if((tree->flags & RUNTIME_TREE_NO_SAVE) == 0 && (status & RUNTIME_RESOURCE_TYPE_ANIMATION) == 0)
        {
            state->flags = previous_flags & ~(APPLICATION_NEW_GAME_DISABLED | APPLICATION_SAVE_DISABLED);
            if((status & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) != 0)
                state->script_state = reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
        }
        else
        {
            state->flags = previous_flags | APPLICATION_NEW_GAME_DISABLED | APPLICATION_SAVE_DISABLED;
        }
    }
    else
    {
        state->flags = previous_flags & ~APPLICATION_NEW_GAME_DISABLED;
    }
}


void finish_application_state_load(ApplicationState *state, const char *path)
{
    if(state == nullptr || path == nullptr)
        return;
    copy_string(state->installed_version, path);
    state->flags |= APPLICATION_RESUME_DISABLED;
    copy_string(state->startup_config, "START.CFG");
    switch_runtime_scene(nullptr);
    graphics_host_flags |= RUNTIME_HOST_COMMAND_STOP_REQUESTED;
}


bool finish_synchronized_state_operation(int result)
{
    unlock_runtime_mutex(&runtime_display_context.resource_mutex);
    if(result == CDF_ERROR_STORAGE_FAILURE)
        send_application_event(HostApplicationCommand::STORAGE_FAILURE);
    return result == 0;
}

bool write_synchronized_cdf_package(void *path, void *comment, void *unused, void *script_state)
{
    if((graphics_host_flags & RUNTIME_HOST_INITIALIZED) == 0)
        return false;
    lock_runtime_mutex(&runtime_display_context.resource_mutex);
    return finish_synchronized_state_operation(write_comment_cdf_package(static_cast<const char *>(path), comment, unused, static_cast<const ScriptTextBuffer *>(script_state)));
}


void set_runtime_paths_once(const char *first_path, const char *second_path)
{
    if((graphics_host_flags & RUNTIME_HOST_TREE_SWITCH_PENDING) == 0)
    {
        lock_runtime_mutex(&runtime_display_context.path_mutex);
        copy_string(runtime_display_context.first_runtime_path, first_path);
        copy_string(runtime_display_context.second_runtime_path, second_path);
        graphics_host_flags |= RUNTIME_HOST_TREE_SWITCH_PENDING;
        unlock_runtime_mutex(&runtime_display_context.path_mutex);
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
                break;
        }
    }
    return best_index;
}

void *create_indexed_display_bitmap(const DisplayBitmapCaptureSource *source, uint32_t *size, int half_resolution)
{
    if(size != nullptr)
        *size = 0;
    if(source == nullptr || source->pixels == nullptr || source->palette_entries == nullptr || source->width == 0 || source->height == 0)
        return nullptr;
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
        return nullptr;
    const uint32_t destination_stride = (width + 3) & ~3u;
    constexpr uint32_t palette_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    constexpr uint32_t pixel_offset = palette_offset + 256 * sizeof(BitmapColor);
    if(height > (UINT32_MAX - pixel_offset) / destination_stride)
        return nullptr;
    const uint32_t bitmap_size = pixel_offset + destination_stride * height;
    auto *bitmap = static_cast<uint8_t *>(save_capture_heap.allocate(bitmap_size, true));
    if(bitmap == nullptr)
        return nullptr;
    const BitmapFileHeader file_header{ 0x4d42, bitmap_size, 0, 0, pixel_offset };
    const BitmapInfoHeader info_header{ sizeof(BitmapInfoHeader), static_cast<int32_t>(width), static_cast<int32_t>(height), 1, 8, 0, destination_stride * height, 0, 0, 256, 256 };
    encode_bitmap_file_header(bitmap, file_header);
    encode_bitmap_info_header(bitmap + sizeof(BitmapFileHeader), info_header);
    auto *colors = reinterpret_cast<BitmapColor *>(bitmap + palette_offset);
    uint8_t *pixels = bitmap + pixel_offset;
    for(uint32_t index = 0; index < 256; ++index)
        colors[index] = { source->palette_entries[index].peBlue, source->palette_entries[index].peGreen, source->palette_entries[index].peRed, 0 };
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
        *size = bitmap_size;
    return bitmap;
}

void *capture_save_game_bitmap(uint32_t *size, int half_resolution)
{
    if(runtime_display_scene_identifier == 0)
        return nullptr;
    const auto *scene = reinterpret_cast<const DisplaySceneNode *>(static_cast<uintptr_t>(runtime_display_scene_identifier));
    if(scene->width <= 0 || scene->height <= 0 || scene->sync_secondary_position <= 0)
        return nullptr;
    const DisplayBitmapCaptureSource source{ static_cast<uint32_t>(scene->width), static_cast<uint32_t>(scene->height), static_cast<uint32_t>(scene->sync_secondary_position),
        scene->rectangle_callback_format.bits_per_pixel, scene->rectangle_callback_format.red_mask, scene->rectangle_callback_format.green_mask, scene->rectangle_callback_format.blue_mask,
        reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position)), display_palette_entries };
    return create_indexed_display_bitmap(&source, size, half_resolution);
}


void clear_runtime_active_flag(ApplicationState *state)
{
    uint32_t previous_flags = state->flags;
    state->flags = previous_flags & ~APPLICATION_RUNTIME_ACTIVE;
    if((previous_flags & APPLICATION_LOCKED) == 0)
        state->flags &= ~APPLICATION_CURSOR_OUTSIDE;
}

void clear_application_lock_flag(ApplicationState *state)
{
    uint32_t previous_flags = state->flags;
    state->flags = previous_flags & ~APPLICATION_LOCKED;
    if((previous_flags & APPLICATION_RUNTIME_ACTIVE) == 0)
        state->flags &= ~APPLICATION_CURSOR_OUTSIDE;
}

void free_heap_memory(void *memory)
{
    if(memory != nullptr)
        save_capture_heap.release(memory);
}



int append_string(char *destination, const char *source)
{
    int length = 0;
    while(destination[length] != '\0')
        ++length;
    return length + copy_string(destination + length, source);
}

bool strings_equal(const char *left, const char *right)
{
    for(;;)
    {
        if(*left != *right)
            return false;
        if(*left == '\0')
            return true;
        ++left;
        ++right;
    }
}

void copy_directory_from_path(char *destination, const char *source)
{
    int index = 0;
    while(source[index] != '\0')
        ++index;
    while(index >= 0 && source[index] != '\\' && source[index] != '/')
        --index;
    destination[index + 1] = '\0';
    while(index >= 0)
    {
        destination[index] = source[index];
        --index;
    }
}

void load_local_preferences(ApplicationState *state)
{
    state->installation_path[0] = '\0';
    std::filesystem::path preferences_path;
    const bool preferences_missing = !resolve_existing_host_path_case_insensitive(preferences_file_name, &preferences_path);
    const ApplicationPreferences preferences = read_preferences();
    set_sdl_presenter_integer_scaling(preferences.integer_scaling);
    state->low_color_resources = preferences.low_color_resources;
    if(preferences.fullscreen)
        state->flags |= APPLICATION_FULLSCREEN_PREFERENCE;
    if(!preferences.sound)
        state->flags |= APPLICATION_SOUND_MUTED;
    if(preferences.subtitles)
        state->flags |= APPLICATION_SUBTITLES_ENABLED;
    if(preferences_missing)
        write_preferences(preferences);

    if((state->flags & APPLICATION_FULLSCREEN_PREFERENCE) == 0)
        state->flags |= APPLICATION_WINDOWED;
}


} // namespace freegag
