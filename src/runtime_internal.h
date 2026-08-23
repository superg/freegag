#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include "application.h"
#include "audio.h"
#include "cdf_archive.h"
#include "display_host.h"
#include "display_scene.h"
#include "extra/save_load.h"
#include "extra/synthesized_resource.h"
#include "game_host.h"
#include "media.h"
#include "resource.h"
#include "runtime.h"
#include "runtime_clock.h"
#include "runtime_model.h"
#include "runtime_tree.h"
#include "runtime_types.h"
#include "script.h"
#include "text.h"
#include "xtet/api.h"

namespace gag
{

bool find_virtual_runtime_script(const char *path, VirtualScriptResource *resource);
bool has_xtet_argument(int argc, char *argv[]);
extern bool gagboy_startup_mode;
extern RuntimePathApi runtime_path_api;

struct DisplayPresenterHostState
{
    uint32_t flags{};
    int32_t width;
    int32_t height;
    int32_t bits_per_pixel;
    void *pixels;
    CRITICAL_SECTION critical_section;
    PALETTEENTRY palette_entries[0x100]{};
};

struct RuntimeIndexedBitmapInfo
{
    BITMAPINFOHEADER header;
    RGBQUAD colors[0x100];
    uint8_t pixels[1];
};

struct RuntimeGenericChildStateFields
{
    uint32_t values[11];
    DisplayRectangle rectangle;
};

union RuntimeGenericChildState
{
    uint32_t words[15];
    RuntimeGenericChildStateFields fields;
};



inline DisplayPresenterHostState display_presenter_host_state;

inline const char *application_message(const ApplicationState *state, size_t index)
{
    constexpr size_t message_capacity = 0x104;
    return state->message_table + index * message_capacity;
}

inline DisplayRectangleTransform display_rectangle_transform(const DisplaySceneDescriptor &descriptor)
{
    return { descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
}
inline uint32_t &display_palette_flags = display_presenter_host_state.flags;
inline int32_t &display_palette_width = display_presenter_host_state.width;
inline int32_t &display_palette_height = display_presenter_host_state.height;
inline int32_t &display_palette_bits_per_pixel = display_presenter_host_state.bits_per_pixel;
inline void *&display_palette_pixels = display_presenter_host_state.pixels;
inline CRITICAL_SECTION &display_host_critical_section = display_presenter_host_state.critical_section;
inline PALETTEENTRY (&display_palette_entries)[0x100] = display_presenter_host_state.palette_entries;

// Selects the framebuffer color depth used for modern desktop presentation.
enum class ModernWindowsFullscreenScaling
{
    preserve_aspect,
    stretch,
    integer,
};

enum class ModernWindowsWindowedScaling
{
    preserve_aspect,
    integer,
};

constexpr ModernWindowsFullscreenScaling modern_windows_fullscreen_scaling = ModernWindowsFullscreenScaling::integer;
constexpr ModernWindowsWindowedScaling modern_windows_windowed_scaling = ModernWindowsWindowedScaling::integer;
constexpr DWORD modern_windows_windowed_style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN;

struct ModernWindowsPresentationState
{
    bool fullscreen;
    bool windowed_rectangle_valid;
    RECT windowed_rectangle;
    int32_t viewport_width;
    int32_t viewport_height;
};

inline ModernWindowsPresentationState modern_windows_presentation_state;
inline bool modern_windows_fullscreen_toggle_latched;
inline bool modern_windows_game_cursor_tracking;

inline RECT calculate_modern_windows_fullscreen_viewport(int32_t monitor_width, int32_t monitor_height, int32_t framebuffer_width, int32_t framebuffer_height, ModernWindowsFullscreenScaling scaling)
{
    RECT viewport{};
    if(monitor_width <= 0 || monitor_height <= 0 || framebuffer_width <= 0 || framebuffer_height <= 0)
    {
        return viewport;
    }

    int32_t width = monitor_width;
    int32_t height = monitor_height;
    if(scaling == ModernWindowsFullscreenScaling::integer)
    {
        const int32_t factor = std::min(monitor_width / framebuffer_width, monitor_height / framebuffer_height);
        if(factor > 0)
        {
            width = framebuffer_width * factor;
            height = framebuffer_height * factor;
        }
        else
        {
            scaling = ModernWindowsFullscreenScaling::preserve_aspect;
        }
    }
    if(scaling == ModernWindowsFullscreenScaling::preserve_aspect)
    {
        if(static_cast<int64_t>(monitor_width) * framebuffer_height <= static_cast<int64_t>(monitor_height) * framebuffer_width)
        {
            width = monitor_width;
            height = static_cast<int32_t>(static_cast<int64_t>(monitor_width) * framebuffer_height / framebuffer_width);
        }
        else
        {
            height = monitor_height;
            width = static_cast<int32_t>(static_cast<int64_t>(monitor_height) * framebuffer_width / framebuffer_height);
        }
    }

    viewport.left = (monitor_width - width) / 2;
    viewport.top = (monitor_height - height) / 2;
    viewport.right = viewport.left + width;
    viewport.bottom = viewport.top + height;
    return viewport;
}

inline RECT calculate_modern_windows_windowed_viewport(int32_t client_width, int32_t client_height, int32_t framebuffer_width, int32_t framebuffer_height, ModernWindowsWindowedScaling scaling)
{
    if(scaling == ModernWindowsWindowedScaling::preserve_aspect)
    {
        return calculate_modern_windows_fullscreen_viewport(client_width, client_height, framebuffer_width, framebuffer_height, ModernWindowsFullscreenScaling::preserve_aspect);
    }
    return calculate_modern_windows_fullscreen_viewport(client_width, client_height, framebuffer_width, framebuffer_height, ModernWindowsFullscreenScaling::integer);
}

inline bool modern_windows_presentation_is_scaled()
{
    return modern_windows_presentation_state.viewport_width > 0 && modern_windows_presentation_state.viewport_height > 0
        && (modern_windows_presentation_state.viewport_width != display_palette_width || modern_windows_presentation_state.viewport_height != display_palette_height);
}

inline int32_t map_modern_windows_presentation_coordinate(int32_t value, int32_t destination_extent, int32_t source_extent)
{
    if(destination_extent <= 0 || source_extent <= 0)
    {
        return 0;
    }
    value = std::max(0, std::min(value, destination_extent - 1));
    return static_cast<int32_t>(static_cast<int64_t>(value) * source_extent / destination_extent);
}

inline RECT map_modern_windows_presentation_rectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, int32_t source_width, int32_t source_height)
{
    RECT rectangle{};
    if(source_width <= 0 || source_height <= 0 || modern_windows_presentation_state.viewport_width <= 0 || modern_windows_presentation_state.viewport_height <= 0)
    {
        return rectangle;
    }
    rectangle.left = static_cast<int32_t>(static_cast<int64_t>(left) * modern_windows_presentation_state.viewport_width / source_width);
    rectangle.top = static_cast<int32_t>(static_cast<int64_t>(top) * modern_windows_presentation_state.viewport_height / source_height);
    rectangle.right = static_cast<int32_t>((static_cast<int64_t>(right) * modern_windows_presentation_state.viewport_width + source_width - 1) / source_width);
    rectangle.bottom = static_cast<int32_t>((static_cast<int64_t>(bottom) * modern_windows_presentation_state.viewport_height + source_height - 1) / source_height);
    return rectangle;
}

inline RuntimePlanModeSyncApi runtime_plan_mode_sync_api{ set_runtime_plans_inactive, clear_runtime_plans_inactive, rebuild_runtime_pointer_resources };

inline RuntimeTreeActivationApi runtime_tree_activation_api{ find_or_load_runtime_generic_resource, create_runtime_tree_node, set_script_runtime_flags, activate_runtime_tree_node_comment };
inline RuntimeGenericResourceLoadApi runtime_generic_resource_load_api{ HeapAlloc };
inline RuntimeTreeParserContextApi runtime_tree_parser_context_api{ HeapAlloc };
inline RuntimeTreeParserReleaseApi runtime_tree_parser_release_api{ HeapFree, remove_runtime_generic_resource };

inline RuntimePendingTreeSwitchApi runtime_pending_tree_switch_api{ destroy_runtime_tree_resources, activate_runtime_tree_with_notifications, reset_runtime_tree_parser_contexts,
    rebuild_runtime_tree_resources, update_runtime_pointer_region };

inline RuntimePairDispatchApi runtime_pair_dispatch_api{ dequeue_runtime_pair, update_runtime_pointer_region, handle_runtime_right_button_down, handle_runtime_left_button_up,
    handle_runtime_left_button_down };
inline ScriptUtilityApi script_utility_api{ VirtualAlloc, runtime_milliseconds, std::srand, std::rand };
inline bool script_random_seeded;

inline ScriptValueParseApi script_value_parse_api{ evaluate_script_parameter };
inline ScriptTypedValueApi script_typed_value_api{ parse_script_integer_expression, parse_image_flag, parse_script_value_token };
inline RuntimeTreeCommandTargetApi runtime_tree_command_target_api{ parse_image_flag, parse_script_value_token };
inline const uint8_t *compressor_input;
inline uint32_t compressor_input_size;
inline uint32_t compressor_input_position;

struct DisplaySceneHostState
{
    uint32_t lock_flags;
    CRITICAL_SECTION lock_critical_section;
    HANDLE lock_gate_event;
    uint32_t lock_busy;
    HANDLE lock_release_event;
    DWORD lock_owner_thread;
    uint32_t lock_recursion_count;
    intptr_t root_primary_position;
    int32_t width;
    int32_t height;
    int32_t root_secondary_position;
    DisplayRectangle clip_bounds;
    DisplayRectangle pending_rectangle;
    DisplayPixelFormatDescriptor *palette_source_state;
    HANDLE worker_thread;
    DisplaySceneSyncApi sync_api;
    void *sync_context;
    uint32_t worker_interval;
    uint32_t worker_rate;
    uint32_t scene_count;
    DisplaySceneNode *root;
    DisplaySceneNode *head;
};


inline DisplaySceneHostState display_scene_host_state;
inline uint32_t &display_lock_flags = display_scene_host_state.lock_flags;
inline CRITICAL_SECTION &display_lock_critical_section = display_scene_host_state.lock_critical_section;
inline HANDLE &display_lock_gate_event = display_scene_host_state.lock_gate_event;
inline uint32_t &display_lock_busy = display_scene_host_state.lock_busy;
inline HANDLE &display_lock_release_event = display_scene_host_state.lock_release_event;
inline DWORD &display_lock_owner_thread = display_scene_host_state.lock_owner_thread;
inline uint32_t &display_lock_recursion_count = display_scene_host_state.lock_recursion_count;
inline intptr_t &display_scene_root_primary_position = display_scene_host_state.root_primary_position;
inline int32_t &display_width = display_scene_host_state.width;
inline int32_t &display_height = display_scene_host_state.height;
inline int32_t &display_scene_root_secondary_position = display_scene_host_state.root_secondary_position;
inline DisplayRectangle &display_clip_bounds = display_scene_host_state.clip_bounds;
inline DisplayRectangle &display_pending_rectangle = display_scene_host_state.pending_rectangle;
inline DisplayPixelFormatDescriptor *&display_palette_source_state = display_scene_host_state.palette_source_state;
inline HANDLE &display_scene_worker_thread = display_scene_host_state.worker_thread;
inline DisplaySceneSyncApi &display_scene_sync_api = display_scene_host_state.sync_api;
inline void *&display_scene_sync_context = display_scene_host_state.sync_context;
inline uint32_t &display_scene_worker_interval = display_scene_host_state.worker_interval;
inline uint32_t &display_scene_worker_rate = display_scene_host_state.worker_rate;
inline uint32_t &display_scene_count = display_scene_host_state.scene_count;
inline DisplaySceneNode *&display_scene_root = display_scene_host_state.root;
inline DisplaySceneNode *&display_scene_head = display_scene_host_state.head;
inline DisplaySceneSurface display_scene_surface_state{};
inline DisplaySceneCallbackApi display_scene_callback_api{ runtime_milliseconds };
inline DisplayRootRegionApi display_root_region_api{ begin_display_scene_update, end_display_scene_update };
inline void switch_runtime_scene_value(uintptr_t value)
{
    switch_runtime_scene(reinterpret_cast<void *>(value));
}

inline void (*runtime_state_transition_callback)(uintptr_t) = switch_runtime_scene_value;
inline ScriptRuntimeRoot *script_runtime_root;

// Dispatches operation queries through the script runtime callback.
inline void query_script_runtime_value(uint32_t operation, const void *source, int32_t *value)
{
    using QueryCallback = void (*)(uint32_t operation, const void *source, int32_t *value);
    reinterpret_cast<QueryCallback>(script_runtime_root->get_property)(operation, source, value);
}

inline ScriptIntegerExpressionApi script_integer_expression_api{ evaluate_script_parameter, select_bounded_random_value, find_global_runtime_tree_link_0084_by_name,
    find_global_runtime_tree_primary_resource_link_by_name, get_script_object_integer, query_script_runtime_value };

inline void resolve_runtime_tree_auxiliary(uint32_t operation, void **identity, void **metadata)
{
    script_runtime_root->get_property(operation, identity, metadata);
}

inline RuntimeTreeAuxiliaryCreateApi runtime_tree_auxiliary_create_api{ HeapAlloc, HeapFree, resolve_runtime_tree_auxiliary };

// Invokes operation 0x30 through the script runtime callback.
inline void activate_created_runtime_tree_node(RuntimeTreeNode *node)
{
    script_runtime_root->set_property(0x30, 0, reinterpret_cast<RuntimeGenericResourceNode *>(node));
}

inline RuntimeTreeCreationApi runtime_tree_creation_api{ find_runtime_tree_node_by_identity, find_runtime_generic_resource, find_runtime_tree_root_identity_by_name, find_runtime_tree_ancestor_root,
    find_runtime_tree_descendant_identity_by_name, find_script_section, find_script_property_value, begin_runtime_tree_enumeration, get_next_runtime_tree_node, HeapAlloc, HeapFree,
    find_or_create_runtime_tree_parser_context, remove_runtime_generic_resource, dispatch_runtime_tree_parser, activate_created_runtime_tree_node };

inline void add_default_runtime_tree_auxiliary_names_discard_result(RuntimeTreeNode *owner)
{
    add_default_runtime_tree_auxiliary_names(owner);
}

inline RuntimeTreeJumpApi runtime_tree_jump_api{ parse_script_property_code, parse_script_value_token, add_default_runtime_tree_auxiliary_names_discard_result, find_or_load_runtime_generic_resource,
    create_runtime_tree_node };
inline RuntimeTreeConditionalCreateApi runtime_tree_conditional_create_api{ compare_script_object_field, script_object_container_state_matches_by_name,
    reinterpret_cast<RuntimeTreeNode *(*)(void *, const void *)>(find_runtime_tree_descendant_identity_by_name), find_or_load_runtime_generic_resource, create_runtime_tree_node,
    destroy_runtime_tree_node };

inline RuntimeTreeParserResetApi runtime_tree_parser_reset_api{ parse_script_property_code, update_conditional_runtime_tree, find_runtime_tree_node_by_identity };
bool strings_equal(const char *left, const char *right);
inline RuntimeTreeParserDirectDispatchApi runtime_tree_parser_direct_dispatch_api{ parse_script_property_code, parse_script_object_state, parse_runtime_tree_link_0084, parse_runtime_tree_link_007c,
    parse_runtime_visual_object, parse_runtime_tree_primary_resource_link, parse_script_object_container, parse_runtime_command_definition, parse_runtime_named_node, parse_runtime_tree_link_008c,
    create_conditional_runtime_tree, parse_runtime_tree_auxiliary_names, create_or_update_runtime_fixed_name_node, parse_runtime_language, parse_runtime_tree_secondary_resource_link,
    parse_script_value_token, apply_runtime_tree_image_flags, dispatch_runtime_tree_section_command, set_runtime_generic_resource_position, read_runtime_generic_resource_token,
    parse_runtime_tree_scene_link, add_runtime_tree_auxiliary_name, publish_runtime_tree_global_links };
inline RuntimeTreeParserSpecialDispatchApi runtime_tree_parser_special_dispatch_api{ parse_script_integer_expression, parse_image_flag, create_runtime_tree_command, find_and_create_runtime_tree_jump,
    strings_equal };
inline RuntimeTreeSectionDispatchApi runtime_tree_section_dispatch_api{ find_runtime_tree_node_by_identity, find_runtime_generic_resource, find_script_section,
    find_or_create_runtime_tree_parser_context, dispatch_runtime_tree_parser, remove_runtime_generic_resource };
inline RuntimeTreeBasicCommandApi runtime_tree_basic_command_api{ parse_script_value_token, extract_script_parenthesized_text, parse_script_scope_code, find_or_load_runtime_generic_resource,
    dispatch_runtime_tree_section, create_runtime_tree_node };

inline void notify_runtime_tree_auxiliary_release(uint32_t operation, uint32_t unused, RuntimeTreeAuxiliaryNode *node)
{
    script_runtime_root->set_property(operation, unused, reinterpret_cast<RuntimeGenericResourceNode *>(node));
}

inline RuntimeTreeAuxiliaryReleaseApi runtime_tree_auxiliary_release_api{ notify_runtime_tree_auxiliary_release, HeapFree };

inline void notify_runtime_tree_destruction(uint32_t operation, uint32_t unused, void *value)
{
    script_runtime_root->set_property(operation, unused, reinterpret_cast<RuntimeGenericResourceNode *>(value));
}

inline RuntimeTreeDestructionCoreApi runtime_tree_destruction_core_api{ find_runtime_tree_node_by_identity, notify_runtime_tree_destruction, remove_runtime_tree_scene_link_range,
    remove_runtime_tree_secondary_resource_link_range, remove_runtime_tree_primary_resource_link_range, remove_runtime_tree_link_007c_range, remove_runtime_tree_link_0084_range,
    remove_runtime_tree_link_008c_range, remove_script_object_container_range, HeapFree, destroy_script_object_container, release_runtime_tree_auxiliary_nodes, release_runtime_tree_parser_contexts,
    find_existing_runtime_tree_parser_context, dispatch_runtime_tree_parser, update_runtime_tree_global_links };
inline ScriptObjectMemoryApi script_object_memory_api{ HeapAlloc };
inline ScriptObjectReleaseApi script_object_release_api{ HeapFree };
inline ScriptObjectParseApi script_object_parse_api{ parse_script_value_token, parse_script_scope_code, parse_script_integer_expression, parse_image_flag, fixed_dword_memory_equal,
    find_runtime_visual_object, create_script_object_state };
inline RuntimeNamedNodeMemoryApi runtime_named_node_memory_api{ HeapAlloc, HeapFree };

inline RuntimeSceneSwitchApi runtime_scene_switch_api{ acquire_runtime_lock_record, release_runtime_lock_record, offset_display_scene_node };
inline RuntimePointerRefreshApi runtime_pointer_refresh_api{ update_runtime_pointer_region };
inline RuntimeCommentTreeCleanupApi runtime_comment_tree_cleanup_api{ begin_runtime_tree_enumeration, get_next_runtime_tree_node, destroy_runtime_tree_resources, deactivate_runtime_tree_and_visuals,
    reset_runtime_tree_parser_contexts, rebuild_runtime_pointer_resources };

inline intptr_t destroy_runtime_tree_for_deactivation(void *identity, void *replacement_identity)
{
    return reinterpret_cast<intptr_t>(destroy_runtime_tree_node(identity, replacement_identity));
}

inline RuntimeTreeDeactivateApi runtime_tree_deactivate_api{ find_runtime_tree_node_by_identity, request_runtime_resource_destruction, remove_runtime_visual_object, set_script_runtime_flags,
    deactivate_runtime_tree_node_comment, destroy_runtime_tree_for_deactivation };
inline RuntimeResourceLoopApi runtime_resource_loop_api{ acquire_runtime_lock_record, set_runtime_sound_loop_value, release_runtime_lock_record };
inline RuntimeCommandLoopState runtime_display_context{};
inline HINSTANCE runtime_graphics_instance;
inline char runtime_graphics_resource_directory[0x104]{};
inline ScriptRuntimeRoot graphics_script_runtime_root{};
inline RuntimeSceneSlot *runtime_scene_slots = graphics_script_runtime_root.command_definitions;
inline RuntimePointerRegion *&runtime_pointer_regions = graphics_script_runtime_root.global_pointer_regions;
inline uint32_t &graphics_host_flags = runtime_display_context.flags;
inline uint32_t &runtime_scene_control_flags = runtime_display_context.flags;
inline uint32_t &runtime_resource_transition_flags = runtime_display_context.accumulated_tree_flags;
inline uint32_t &runtime_command_state = runtime_display_context.external_command_pending;
inline uint32_t &runtime_target_flags = runtime_display_context.target_flags;
inline int32_t &runtime_scene_x = runtime_display_context.scene_x;
inline int32_t &runtime_scene_y = runtime_display_context.scene_y;
inline void *&deferred_runtime_scene_identity = runtime_display_context.deferred_scene_identity;
inline void *&current_runtime_scene_identity = runtime_display_context.current_scene_identity;
inline uintptr_t &runtime_state_value = runtime_display_context.deferred_state_value;
inline uintptr_t &saved_runtime_state_value = runtime_display_context.current_state_value;
inline void *&saved_default_comment_scene_identity = runtime_display_context.saved_default_comment_scene_identity;
inline void *&runtime_pointer_root_identity = runtime_display_context.runtime_tree_identity;
inline RuntimePointerRegion *&active_runtime_pointer_region = runtime_display_context.active_pointer_region;

inline uintptr_t runtime_pointer_event_record[16]{};
inline uint32_t runtime_pointer_state_mask;
inline void *runtime_pointer_state_owner;
inline void *runtime_pointer_event_state_object;

inline void enqueue_runtime_pointer_event()
{
    uintptr_t record[16]{};
    record[0] = runtime_pointer_state_mask;
    record[1] = reinterpret_cast<uintptr_t>(runtime_pointer_state_owner);
    record[2] = reinterpret_cast<uintptr_t>(runtime_pointer_event_state_object);
    std::memcpy(record + 3, runtime_pointer_event_record, 13 * sizeof(uintptr_t));
    enqueue_runtime_event_record(record);
}
inline RuntimeTextInputSceneRedrawApi runtime_text_input_scene_redraw_api{ acquire_display_scene_node, begin_display_scene_update, end_display_scene_update };
inline intptr_t runtime_text_input_guarded_scene;

inline void release_runtime_text_input_scene_guard()
{
    if(runtime_text_input_guarded_scene != 0)
    {
        runtime_text_input_scene_redraw_api.end_update(runtime_text_input_guarded_scene, nullptr, nullptr);
        runtime_text_input_guarded_scene = 0;
    }
}
inline RuntimeTextInputApi runtime_text_input_api{ dequeue_runtime_byte, runtime_milliseconds, initialize_runtime_standalone_text, acquire_runtime_text_input_scene,
    begin_runtime_text_input_scene_update, draw_runtime_standalone_text, end_runtime_text_input_scene_update, release_display_scene_node };
inline RuntimeScriptExecutorApi runtime_script_executor_api{ runtime_milliseconds, runtime_milliseconds, Sleep, process_available_runtime_generic_children, process_runtime_message,
    process_runtime_text_input, process_runtime_pair_message, run_runtime_command_loop, find_runtime_tree_node_by_identity, synchronize_runtime_plan_mode, process_pending_runtime_tree_switch,
    acknowledge_current_runtime_event_record, run_pending_runtime_external_command, activate_runtime_tree_link_007c, parse_script_opcode, execute_simple_runtime_script_opcode,
    select_bounded_random_value };
inline char &runtime_display_reset_byte = runtime_display_context.input_text[0];
inline RuntimeDisplayResetApi runtime_display_reset_api{ switch_runtime_scene, set_script_runtime_flags, reset_script_runtime_transient_indices, reset_runtime_byte_queue, reset_runtime_pair_queue,
    release_display_scene_node };
inline HANDLE &runtime_display_thread = runtime_display_context.script_thread;
inline intptr_t &runtime_display_scene_identifier = runtime_display_context.input_alternate_scene_identifier;
inline void *&runtime_display_host = runtime_display_context.display_scene_host;
inline RuntimeBootstrapApi runtime_bootstrap_api{ create_display_surface, get_display_palette_entries, initialize_display_scene_host, acquire_display_scene_node, lock_display_scene_node,
    unlock_display_scene_node, acquire_display_lock, set_display_clip_rectangle, release_display_lock, operate_display_surface, reset_runtime_display_state, CreateThread, execute_script_commands };
inline RuntimeDisplayShutdownApi runtime_display_shutdown_api{ get_or_create_runtime_named_node, WaitForSingleObject, CloseHandle, release_display_scene_node, shutdown_display_scene_host,
    teardown_display_palette_surface };
inline uint32_t &runtime_resource_count = runtime_display_context.resource_wait_count;
inline uint32_t &runtime_property_value = runtime_display_context.script_clock;
inline uint32_t &runtime_resource_presentation_owner = runtime_display_context.script_clock;
inline RuntimeResourceStateApi runtime_resource_state_api{ acquire_runtime_lock_record, begin_display_scene_update, finalize_runtime_media_backend, configure_runtime_resource_palette,
    end_display_scene_update, clear_runtime_generic_backend_child_ready, enable_runtime_generic_backend_child_mode_200, disable_runtime_generic_backend_child_mode_200, select_runtime_scene_transition,
    restart_runtime_sound_data, start_runtime_sound, stop_runtime_sound, release_runtime_lock_record };
inline RuntimeImmediateSceneTransitionApi runtime_immediate_scene_transition_api{ acquire_display_lock, set_display_clip_rectangle, release_display_lock, acquire_runtime_lock_record,
    dispatch_display_scene_update, Sleep, synchronize_display_region, apply_display_palette, release_runtime_lock_record };
inline RuntimeSceneTransitionSelectionApi runtime_scene_transition_selection_api{ std::rand, apply_immediate_runtime_scene_transition, apply_palette_runtime_scene_transition,
    apply_rectangle_runtime_scene_transition };
inline RuntimePaletteSceneTransitionApi runtime_palette_scene_transition_api{ acquire_runtime_lock_record, apply_immediate_runtime_scene_transition, acquire_display_lock, apply_display_palette,
    operate_display_surface, set_display_clip_rectangle, dispatch_display_scene_update, release_display_lock, release_runtime_lock_record, runtime_milliseconds, Sleep,
    invalidate_game_framebuffer_rect };
inline PALETTEENTRY runtime_transition_palette[0x101]{};
inline RuntimeRectangleSceneTransitionApi runtime_rectangle_scene_transition_api{ acquire_runtime_lock_record, apply_immediate_runtime_scene_transition, acquire_display_lock,
    set_display_clip_rectangle, release_display_lock, operate_display_surface, synchronize_display_region, apply_display_palette, dispatch_display_scene_update, runtime_milliseconds,
    runtime_milliseconds, Sleep, release_runtime_lock_record };
inline RuntimeResourceSceneRegionApi runtime_resource_scene_region_api{ lock_display_scene_node, acquire_runtime_lock_record, begin_display_scene_update, render_runtime_bitmap_backend_region,
    end_display_scene_update, update_display_root_region, release_runtime_lock_record, unlock_display_scene_node };
inline RuntimeResourceSceneDestructionApi runtime_resource_scene_destruction_api{ acquire_runtime_lock_record, destroy_runtime_resource, release_runtime_lock_record, Sleep,
    update_runtime_resource_scene_region };
inline RuntimeTreeDestructionApi runtime_tree_destruction_api{ find_runtime_tree_node_by_identity, set_runtime_resource_state, stop_runtime_game_dll, reset_runtime_display_state,
    find_last_runtime_primary_resource_link_by_identity, find_last_runtime_secondary_resource_link_by_identity, find_last_runtime_scene_link_by_identity, query_runtime_scene_flags,
    finalize_runtime_resource_destruction, request_runtime_resource_destruction, release_display_scene_node, set_runtime_tree_comment_mode, wait_for_runtime_resource_count };

inline RuntimeResourceSelectionApi runtime_resource_selection_api{ EnterCriticalSection, close_cdf_archive, LeaveCriticalSection, construct_runtime_resource };
inline RuntimePointerResourceRebuildApi runtime_pointer_resource_rebuild_api{ find_runtime_tree_node_by_identity, synchronize_runtime_pointer_owner_slots, query_runtime_scene_flags,
    finalize_runtime_resource_destruction, request_runtime_resource_destruction, construct_runtime_resource, update_runtime_scene_position, set_runtime_tree_comment_mode,
    wait_for_runtime_resource_count };
inline RuntimeGenericChildAttachmentApi runtime_generic_child_attachment_api{ acquire_runtime_lock_record, release_runtime_lock_record, find_available_display_scene_index,
    create_runtime_generic_backend_child, lock_display_scene_node, unlock_display_scene_node, acquire_display_scene_node, destroy_runtime_generic_backend_child };
inline RuntimeTreeResourceRebuildApi runtime_tree_resource_rebuild_api{ find_runtime_tree_node_by_identity, acquire_display_scene_node, construct_runtime_resource,
    synchronize_runtime_pointer_owner_slots, query_runtime_scene_flags, request_runtime_resource_destruction, attach_runtime_generic_backend_child, set_runtime_tree_comment_mode,
    wait_for_runtime_resource_count, reset_runtime_byte_queue, reset_runtime_pair_queue, reset_script_runtime_transient_indices, set_runtime_resource_state };
inline RuntimeResourceWaitApi runtime_resource_wait_api{ Sleep };
inline RuntimeResourceFileOpenApi runtime_resource_file_open_api{ GetVersionExA, CreateFileA };
inline CRITICAL_SECTION &runtime_resource_critical_section = runtime_display_context.resource_critical_section;
inline AsyncFileHost *&runtime_resource_host = runtime_display_context.async_file_host;
inline CdfArchive *&runtime_resource_archive = runtime_display_context.active_archive;
inline intptr_t &runtime_resource_archive_alternate_stream = runtime_display_context.archive_alternate_stream;
inline int32_t &runtime_resource_host_mode = runtime_display_context.resource_host_mode;
inline uint8_t &runtime_resource_archive_state = runtime_display_context.resource_archive_state;
inline RuntimeResourceHostApi runtime_resource_host_api{ EnterCriticalSection, LeaveCriticalSection, destroy_async_file_host, create_async_file_host, set_async_file_host_mode, close_cdf_archive };
inline void *&runtime_resource_cache_parent_identity = runtime_display_context.resource_cache_parent_identity;
inline RuntimeResourceTypeApi runtime_resource_type_api{ EnterCriticalSection, LeaveCriticalSection, find_runtime_resource_cache_entry, update_runtime_resource_host, open_runtime_resource_file,
    ReadFile, CloseHandle, get_cdf_entry_flags };
inline RuntimeCdfStreamApi runtime_cdf_stream_api{ lstrcmpiA, duplicate_async_file_record, CreateFileA, SetFilePointer };
inline ArchiveCommentEnumerationApi archive_comment_enumeration_api{ FindFirstFileA, FindNextFileA, FindClose, GetProcessHeap, HeapAlloc, HeapReAlloc, HeapFree, open_cdf_archive, get_cdf_error,
    get_cdf_entry_size, read_cdf_entry, close_cdf_archive, DeleteFileA };
inline HANDLE &runtime_resource_heap = runtime_display_context.resource_heap;
inline uint32_t &runtime_resource_streamed_count = runtime_display_context.resource_count;
inline RuntimeResourceLoadApi runtime_resource_load_api{ EnterCriticalSection, LeaveCriticalSection, find_runtime_resource_cache_entry, open_async_file_record, get_async_file_size,
    activate_default_comment_scene, HeapAlloc, HeapFree, read_async_file_record, deactivate_default_comment_scene, reset_runtime_byte_queue, reset_runtime_pair_queue,
    get_or_create_runtime_resource_cache_entry, get_cdf_entry_flags, get_cdf_entry_size, open_runtime_cdf_entry_stream, read_cdf_entry, set_script_runtime_flags, Sleep };
inline RuntimeResourceReleaseApi runtime_resource_release_api{ EnterCriticalSection, LeaveCriticalSection, find_runtime_resource_cache_entry, find_runtime_named_child, HeapFree,
    remove_runtime_named_child_by_identity, close_async_file_record, set_script_runtime_flags };
inline RuntimeMediaBackendApi runtime_media_backend_api{ GetCurrentThreadId, WaitForSingleObject, ReleaseMutex, HeapFree, Sleep };
inline HANDLE runtime_media_backend_heap;
inline HANDLE runtime_media_backend_mutex;
inline RuntimeBitmapRegionRenderApi runtime_bitmap_region_render_api{ WaitForSingleObject, ReleaseMutex, copy_runtime_bitmap_region };
inline bool runtime_media_backend_initialized;
inline RuntimeMediaBackend *runtime_media_backend_head;
inline RuntimeMediaBackend *runtime_media_backend_tail;
inline RuntimeMediaBackendShutdownApi runtime_media_backend_shutdown_api{ acquire_first_runtime_media_backend, release_runtime_media_backend_lock, HeapDestroy, CloseHandle, shutdown_runtime_sound };
inline RuntimeBitmapBackendCreateApi runtime_bitmap_backend_create_api{ HeapAlloc, WaitForSingleObject, ReleaseMutex };
inline RuntimeAnimationBackendCreateApi runtime_animation_backend_create_api{ get_async_file_position, read_async_file_record, HeapAlloc, set_async_file_position, WaitForSingleObject, ReleaseMutex };
inline RuntimeMediaBackendConfigureApi runtime_media_backend_configure_api{ WaitForSingleObject, ReleaseMutex };
inline RuntimeAnimationBackendConfigureApi runtime_animation_backend_configure_api{ WaitForSingleObject, ReleaseMutex, HeapAlloc, CreateThread, CloseHandle };
inline RuntimeResourcePaletteConfigureApi runtime_resource_palette_configure_api{ set_display_scene_primary_owner, configure_display_scene_palette };
inline RuntimeMediaBackendFinalizeApi runtime_media_backend_finalize_api{ WaitForSingleObject, ReleaseMutex, convert_runtime_bitmap_to_surface };
inline uint32_t runtime_animation_control_flags;
inline RuntimeAnimationControlApi runtime_animation_control_api{ destroy_runtime_sound_handle, start_runtime_sound, stop_runtime_sound, set_async_file_position };
inline RuntimeAnimationFrameAcquireApi runtime_animation_frame_acquire_api{ read_async_file_record, HeapAlloc, HeapReAlloc, fail_runtime_animation };
inline RuntimeAnimationDecodeApi runtime_animation_decode_api{ decode_runtime_animation_palette, decode_runtime_animation_mvz5, decode_runtime_animation_delta_flc, decode_runtime_animation_mvz8,
    ignore_runtime_animation_chunk_11, ignore_runtime_animation_chunk_12, ignore_runtime_animation_chunk_13, decode_runtime_animation_byte_run, decode_runtime_animation_literal };
inline RuntimeAnimationCompletionApi runtime_animation_completion_api{ Sleep, set_async_file_position };
inline RuntimeAnimationAudioApi runtime_animation_audio_api{ runtime_milliseconds, Sleep, HeapAlloc, HeapReAlloc, destroy_runtime_sound_handle, queue_runtime_sound_data, stop_runtime_sound,
    start_runtime_sound, create_runtime_sound_handle, query_runtime_sound_status, set_runtime_sound_playback_marker, set_runtime_sound_schedule_marker };
inline RuntimeAnimationWorkerApi runtime_animation_worker_api{ Sleep, runtime_milliseconds, ExitThread };
inline RuntimeResourceConstructionPlanApi runtime_resource_construction_plan_api{ find_available_display_scene_index };
inline RuntimeResourceConstructionApi runtime_resource_construction_api{
    GetProcessHeap,
    HeapAlloc,
    HeapFree,
    EnterCriticalSection,
    LeaveCriticalSection,
    detect_runtime_resource_type,
    update_runtime_resource_host,
    load_runtime_resource,
    create_runtime_bitmap_backend,
    create_runtime_animation_backend,
    create_runtime_sound_handle,
    set_runtime_sound_playback_marker,
    start_runtime_sound,
    queue_runtime_sound_data,
    set_runtime_sound_loop_value,
    stop_runtime_sound,
    create_runtime_generic_backend,
    find_or_load_runtime_generic_resource,
    activate_runtime_tree_with_notifications,
    rebuild_runtime_tree_resources,
    acquire_display_scene_node,
    configure_runtime_bitmap_backend,
    configure_runtime_animation_backend,
    begin_display_scene_update,
    finalize_runtime_media_backend,
    configure_runtime_resource_palette,
    end_display_scene_update,
    wait_for_runtime_resource_count,
    destroy_runtime_media_backend,
    destroy_runtime_sound_handle,
    destroy_runtime_generic_backend,
    release_runtime_memory_resource,
    release_runtime_streamed_resource,
    build_runtime_resource_path,
    open_cdf_archive,
    get_or_create_runtime_child_by_data,
    add_display_scene_callback,
};
inline RuntimeGenericBackendApi runtime_generic_backend_api{ WaitForSingleObject, ReleaseMutex, Sleep, GetProcessHeap, HeapFree };
inline RuntimeGenericBackendCreateApi runtime_generic_backend_create_api{ GetProcessHeap, HeapAlloc, HeapFree, WaitForSingleObject, ReleaseMutex };
inline RuntimeGenericChildCreateApi runtime_generic_child_create_api{ acquire_runtime_generic_backend, find_runtime_generic_text_entry, parse_runtime_generic_integer, GetProcessHeap, HeapAlloc,
    WaitForSingleObject, ReleaseMutex, build_runtime_generic_backend_child_state, clear_runtime_generic_backend_ready };
inline RuntimeGenericChildSceneApi runtime_generic_child_scene_api{ find_available_runtime_generic_child, build_runtime_generic_backend_child_state, find_available_display_scene_index,
    acquire_display_scene_node, begin_display_scene_update, publish_runtime_generic_backend_child_state, end_display_scene_update, set_runtime_generic_backend_child_context,
    get_runtime_generic_backend_child_context, destroy_runtime_generic_backend_child, query_display_scene_by_index, release_display_scene_node, enable_runtime_generic_backend_child_mode_200 };
inline HANDLE runtime_generic_backend_mutex;
inline RuntimeGenericBackend *runtime_generic_backend_head;
inline uint32_t runtime_generic_backend_enabled;
inline RuntimeGenericBackendShutdownApi runtime_generic_backend_shutdown_api{ WaitForSingleObject, ReleaseMutex, destroy_runtime_generic_backend, CloseHandle };
inline RuntimeBackendInitializationApi runtime_backend_initialization_api{ HeapCreate, CreateMutexA, initialize_runtime_sound, WaitForSingleObject, ReleaseMutex, InitializeCriticalSection };
inline RuntimeResourceDestroyApi runtime_resource_destroy_api{ acquire_runtime_lock_record, EnterCriticalSection, LeaveCriticalSection, find_runtime_generic_resource, remove_runtime_generic_resource,
    destroy_runtime_media_backend, release_runtime_memory_resource_by_data, release_runtime_streamed_resource, destroy_runtime_sound_handle, destroy_runtime_generic_backend,
    release_display_scene_node, remove_runtime_named_child_by_identity, GetProcessHeap, HeapFree };
inline RuntimeResourceControlApi runtime_resource_control_api{ acquire_runtime_lock_record, release_runtime_lock_record, destroy_runtime_resource, query_runtime_sound_status };
inline CRITICAL_SECTION &runtime_game_dll_critical_section = runtime_display_context.resource_critical_section;
inline RuntimeGameDllWindowProcedure &runtime_game_dll_window_procedure = runtime_display_context.game_dll_window_procedure;
inline RuntimeGameDllExecute &runtime_game_dll_execute = runtime_display_context.game_dll_execute;
inline GraphicsHostInitializationResult graphics_host_state{};
inline RuntimeGameHostContext runtime_game_host_context{};
inline uint32_t &runtime_resource_palette_bits_per_pixel = runtime_game_host_context.bits_per_pixel;
inline void *runtime_game_host_callbacks[35]{};
inline void *&runtime_media_objects_parent_identity = runtime_display_context.media_objects_parent_identity;
inline CRITICAL_SECTION &runtime_named_lock_critical_section = runtime_display_context.path_critical_section;
inline void *&runtime_named_lock_parent_identity = runtime_display_context.media_objects_parent_identity;
inline RuntimeNamedLockApi runtime_named_lock_api{ GetCurrentThreadId, EnterCriticalSection, LeaveCriticalSection, Sleep };
inline uint32_t &graphics_host_value_1 = runtime_display_context.reset_value_1;
inline uint32_t &graphics_host_value_2 = runtime_display_context.reset_value_2;
inline uint32_t &graphics_host_value_3 = runtime_display_context.reset_value_3;
inline uint32_t &runtime_state_1000_count = runtime_display_context.nested_runtime_state_count;
inline uint32_t &runtime_state_4_count = runtime_display_context.nested_runtime_state_4_count;
inline FramebufferInvalidateApi framebuffer_invalidate_api{ acquire_display_lock, dispatch_display_scene_update, release_display_lock };
inline ClearRuntimeDisplayApi clear_runtime_display_api{ acquire_display_lock, set_display_clip_rectangle, operate_display_surface, release_display_lock, update_display_root_region };
inline GraphicsHostApi graphics_host_api{ initialize_runtime_media_backend, initialize_async_file_subsystem, initialize_runtime_generic_backend, HeapCreate, RegisterClassA, CreateWindowExA,
    GetCursorPos, ScreenToClient, initialize_sdl_presenter, set_script_runtime_root_if_valid, get_or_create_runtime_named_node, set_runtime_named_node_enabled, InitializeCriticalSection, ShowWindow };
inline GraphicsHostShutdownApi graphics_host_shutdown_api{ shutdown_runtime_display, shutdown_runtime_generic_backend, shutdown_async_file_subsystem, shutdown_runtime_media_backend,
    shutdown_sdl_presenter, DeleteCriticalSection, HeapDestroy, DestroyWindow };
inline void initialize_linked_xtet(RuntimeGameHostContext *context, void **callbacks, const char *sfs_name)
{
    xtet::initialize_game(reinterpret_cast<xtet::GameHostContext *>(context), callbacks, sfs_name);
}

inline RuntimeGameLifecycleApi runtime_game_lifecycle_api{ EnterCriticalSection, update_runtime_resource_host, activate_default_comment_scene, deactivate_default_comment_scene,
    reset_runtime_byte_queue, reset_runtime_pair_queue, leave_runtime_state_1000, LeaveCriticalSection };
inline RuntimeGameIntegrationApi runtime_game_integration_api{ initialize_linked_xtet, xtet::dispatch_game_window_message, xtet::execute_game_command, xtet::shutdown_game };
inline RuntimeGameDllDispatchApi runtime_game_dll_dispatch_api{ runtime_milliseconds, Sleep };
inline HWND &runtime_game_main_window = runtime_display_context.window;
inline RuntimeGameWindowApi runtime_game_window_api{ SendMessageA, BeginPaint, EndPaint, update_runtime_pointer_position, enqueue_runtime_byte, enqueue_runtime_pair, enqueue_runtime_message,
    clear_runtime_flag_01000000, unload_runtime_game_dll, enter_runtime_state_1000, leave_runtime_state_1000, set_runtime_flag_01000000, TrackMouseEvent, SetCursor, DefWindowProcA };
inline RuntimePointerPositionApi runtime_pointer_position_api{ GetCurrentThreadId, EnterCriticalSection, find_runtime_named_child, LeaveCriticalSection, Sleep, offset_display_scene_node };
inline int32_t &runtime_pointer_x = runtime_display_context.scene_x;
inline int32_t &runtime_pointer_y = runtime_display_context.scene_y;
inline void *&current_runtime_resource = runtime_display_context.current_runtime_resource;
inline bool async_file_enabled;
inline AsyncFileShutdownApi async_file_shutdown_api{ destroy_async_file_host, EnterCriticalSection, LeaveCriticalSection, DeleteCriticalSection };
inline AsyncFileHost *async_file_hosts;
inline CRITICAL_SECTION async_file_global_lock;
inline AsyncFileLockApi async_file_lock_api{ EnterCriticalSection, LeaveCriticalSection, Sleep };
inline AsyncFileOpenApi async_file_open_api{ CreateFileA, GetProcessHeap, HeapAlloc, HeapFree, CloseHandle, VirtualAlloc, VirtualFree, GetFileSize };
inline AsyncFileHostApi async_file_host_api{ GetProcessHeap, HeapAlloc, HeapFree, GetDiskFreeSpaceA, InitializeCriticalSection, DeleteCriticalSection, VirtualAlloc, CreateThread, WaitForSingleObject,
    ReadFile, SetFilePointer, runtime_milliseconds };
inline BitmapCaptureApi bitmap_capture_api{ GetProcessHeap, HeapAlloc };

bool strings_equal(const char *left, const char *right);
void copy_directory_from_path(char *destination, const char *source);
void set_runtime_flag_40();

inline CursorStateApi cursor_state_api{ GetCursorPos, GetSystemMetrics };

inline uint32_t load_preferences_for_validation(ApplicationState *state)
{
    return load_local_preferences(state);
}

inline ValidationApi validation_api{ FindWindowA, FindFirstFileA, FindClose, load_preferences_for_validation, enable_borderless_fullscreen };
inline WindowClassApi window_class_api{ CreateSolidBrush, LoadIconA, LoadCursorA, RegisterClassExA, gag_main_window_procedure, gag_capture_window_procedure };

inline WindowProcedureApi window_procedure_api{ GetWindowLongPtrA, SetWindowLongPtrA, PostMessageA, GetSystemMenu, DeleteMenu, CreateMenu, CreatePopupMenu, AppendMenuA, CheckMenuItem, EnableMenuItem,
    SetMenu, DestroyWindow, DefWindowProcA, SendMessageA, set_game_cursor_active };
inline MainWindowProcedureApi main_window_procedure_api{ GetWindowLongPtrA, SetWindowLongPtrA, PostMessageA, PostQuitMessage, SendMessageA, DefWindowProcA, DestroyWindow, set_application_lock_flag,
    clear_runtime_active_flag, validate_startup_environment, set_runtime_flag_40 };
inline LocalPreferencesApi local_preferences_api{ GetFullPathNameA, GetPrivateProfileStringA, WritePrivateProfileStringA, GetWindowRect, GetWindowPlacement, MonitorFromRect };

inline void enter_runtime_byte_queue_lock()
{
    EnterCriticalSection(&runtime_display_context.byte_queue_critical_section);
}

inline void leave_runtime_byte_queue_lock()
{
    LeaveCriticalSection(&runtime_display_context.byte_queue_critical_section);
}

inline void enter_runtime_pair_queue_lock()
{
    EnterCriticalSection(&runtime_display_context.pair_queue_critical_section);
}

inline void leave_runtime_pair_queue_lock()
{
    LeaveCriticalSection(&runtime_display_context.pair_queue_critical_section);
}

inline void enter_runtime_message_queue_lock()
{
    EnterCriticalSection(&runtime_display_context.message_queue_critical_section);
}

inline void leave_runtime_message_queue_lock()
{
    LeaveCriticalSection(&runtime_display_context.message_queue_critical_section);
}

inline void enter_synchronized_resource_lock()
{
    EnterCriticalSection(&runtime_display_context.resource_critical_section);
}

inline void leave_synchronized_resource_lock()
{
    LeaveCriticalSection(&runtime_display_context.resource_critical_section);
}

inline void enter_runtime_path_lock()
{
    EnterCriticalSection(&runtime_display_context.path_critical_section);
}

inline void leave_runtime_path_lock()
{
    LeaveCriticalSection(&runtime_display_context.path_critical_section);
}

inline void pause_runtime_game_dll_discard_result()
{
    pause_runtime_game_dll();
}

inline void pause_runtime_sound_output_discard_result(int close_output)
{
    pause_runtime_sound_output(close_output);
}

inline void resume_runtime_sound_output_discard_result()
{
    resume_runtime_sound_output();
}

inline void resume_runtime_game_dll_discard_result()
{
    resume_runtime_game_dll();
}

// RuntimeCommandBounds fields are output pointers in mode 0x10000.
inline int begin_runtime_target_from_bounds_fields(uint32_t pixels, uint32_t rectangle, uint32_t pitch)
{
    return static_cast<int>(begin_display_target(reinterpret_cast<void **>(static_cast<uintptr_t>(pixels)), reinterpret_cast<DisplayRectangle *>(static_cast<uintptr_t>(rectangle)),
        reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(pitch))));
}

// Mode 1 interprets the same four values as a DisplayRectangle.
inline void draw_runtime_bounds(RuntimeCommandBounds *bounds, int mode)
{
    synchronize_display_region(reinterpret_cast<DisplayRectangle *>(bounds), static_cast<uint32_t>(mode));
}

inline void present_runtime_message_target()
{
    release_display_lock();
}

inline CursorVisibilityApi cursor_visibility_api{ ShowCursor, leave_runtime_state_1000, enter_runtime_state_1000 };
inline void (*finish_credits_callback)() = clear_credits_runtime_flag;
inline RuntimeQueueApi runtime_queue_api{ enter_runtime_pair_queue_lock, leave_runtime_pair_queue_lock, enter_runtime_message_queue_lock, leave_runtime_message_queue_lock,
    enter_runtime_byte_queue_lock, leave_runtime_byte_queue_lock };
inline RuntimeInputSessionApi runtime_input_session_api{ reset_runtime_byte_queue, runtime_milliseconds, acquire_runtime_lock_record, initialize_runtime_standalone_text,
    find_available_display_scene_index, lock_display_scene_node, acquire_display_scene_node, begin_display_scene_update, draw_runtime_standalone_text, end_display_scene_update,
    unlock_display_scene_node, release_runtime_lock_record };
inline RuntimeCommandLoopApi runtime_command_loop_api{ pause_runtime_game_dll_discard_result, pause_all_runtime_animations, pause_runtime_sound_output_discard_result, process_runtime_message, Sleep,
    resume_runtime_sound_output_discard_result, resume_all_runtime_animations, resume_runtime_game_dll_discard_result, reset_runtime_session };
inline uint32_t runtime_session_reset_storage[0x1d3]{};
inline RuntimeSessionResetApi runtime_session_reset_api{ stop_runtime_game_dll, get_runtime_tree_root, destroy_runtime_tree_resources, deactivate_runtime_tree_and_visuals, reset_runtime_display_state,
    request_runtime_resource_destruction, destroy_runtime_fixed_name_list_nodes, purge_disabled_runtime_named_nodes, destroy_script_object_states, destroy_runtime_visual_objects,
    clear_runtime_command_definitions, remove_all_runtime_generic_resources, close_cdf_archive, destroy_async_file_host, operate_display_surface, get_or_create_runtime_named_node,
    runtime_milliseconds, Sleep };
inline RuntimeMessageProcessorApi runtime_message_processor_api{ dequeue_runtime_message, disable_display_palette_mode, enable_display_palette_mode, acquire_display_lock, update_runtime_target,
    present_runtime_message_target };
inline RuntimeTargetUpdateApi runtime_target_update_api{ draw_runtime_bounds, begin_runtime_target_from_bounds_fields, end_display_target };
inline DisplayLockReleaseApi display_lock_release_api{ GetCurrentThreadId, SetEvent };
inline DisplayLockAcquireApi display_lock_acquire_api{ GetCurrentThreadId, WaitForSingleObject, Sleep, EnterCriticalSection, LeaveCriticalSection, ResetEvent };
inline DisplaySceneMemoryApi display_scene_memory_api{ GetProcessHeap, HeapAlloc, HeapFree };
inline DisplaySceneHostApi display_scene_host_api{ InitializeCriticalSection, DeleteCriticalSection, CreateEventA, CloseHandle, CreateThread, WaitForSingleObject };
inline DisplaySceneWorkerApi display_scene_worker_api{ runtime_milliseconds, Sleep, acquire_display_lock, synchronize_display_scene_node, publish_display_scene_node, release_display_lock_mode_1000,
    release_display_lock };
inline const DisplayPixelFormatDescriptor default_display_pixel_format{ 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0, nullptr, nullptr };
inline const DisplayPixelFormatDescriptor indexed_source_pixel_format{ 0, 8, 0, 0, 0, 0, nullptr, nullptr };
inline WindowLayoutApi window_layout_api{ GetSystemMetrics, AdjustWindowRect, SetWindowLongA, SetWindowPos, GetClientRect, SetFocus, SendMessageA, GetWindowRect, MonitorFromWindow, GetMonitorInfoA,
    InvalidateRect };

// Returns serialized script state as a pointer-sized value.
inline uintptr_t get_serialized_script_state_for_application()
{
    return reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
}

inline StateActivationApi state_activation_api{ query_runtime_scene_flags, get_serialized_script_state_for_application, enter_runtime_state_1000 };

constexpr char auto_save_file_name[] = "AutoSave.cdf";


inline int write_comment_cdf_package_adapter(void *first, void *second, void *third, void *fourth)
{
    return static_cast<int>(write_comment_cdf_package(static_cast<const char *>(first), second, third, static_cast<const ScriptTextBuffer *>(fourth)));
}

inline SynchronizedStateApi synchronized_state_api{ enter_synchronized_resource_lock, leave_synchronized_resource_lock, write_comment_cdf_package_adapter };

inline void clear_cursor_flag_above_client(ApplicationState *state)
{
    POINT point;
    if(cursor_state_api.get_cursor_position(&point) != FALSE)
    {
        int threshold =
            cursor_state_api.get_system_metrics(SM_CYCAPTION) + cursor_state_api.get_system_metrics(SM_CYFIXEDFRAME) + cursor_state_api.get_system_metrics(SM_CYMENU) + state->window_vertical_offset;
        if(threshold < point.y)
        {
            state->flags &= 0xfffffffd;
        }
    }
}

inline ApplicationInitializationApi application_initialization_api{ SetErrorMode, GetProcessHeap, HeapAlloc, register_gag_window_classes, copy_string, validate_startup_environment, GetSystemMetrics,
    AdjustWindowRect, CreateWindowExA, ShowWindow, SetWindowPos, GetClientRect, initialize_graphics_host, initialize_runtime_graphics, update_application_window_layout, enable_runtime_subsystem,
    set_active_object_field_0824, detect_runtime_resource_type };

inline void set_runtime_flag_40()
{
    graphics_host_flags |= 0x40;
}


inline RuntimeScriptPropertySetApi runtime_script_property_set_api{ select_runtime_resource, release_runtime_memory_resource, set_runtime_property_value, enter_runtime_state_1000,
    leave_runtime_state_1000, destroy_runtime_tree_resources };
inline RuntimeScriptPropertyGetApi runtime_script_property_get_api{ copy_string, load_runtime_resource, get_runtime_property_value, query_runtime_resource_frame_number };

} // namespace gag
