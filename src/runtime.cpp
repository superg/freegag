#include "runtime.h"
#include "runtime_internal.h"

namespace gag
{
void enable_runtime_subsystem()
{
    if((graphics_host_flags & 2) == 0)
    {
        graphics_host_flags |= 2;
        toggle_runtime_sound_state();
    }
}

void disable_runtime_subsystem()
{
    if((graphics_host_flags & 2) != 0)
    {
        graphics_host_flags &= 0xfffffffd;
        toggle_runtime_sound_state();
    }
}

void set_active_object_field_0824(uint32_t value)
{
    if(script_runtime_root != nullptr)
    {
        script_runtime_root->state_value_0824 = value;
    }
}

void set_runtime_flag_01000000()
{
    graphics_host_flags |= 0x01000000;
}

void clear_runtime_flag_01000000()
{
    graphics_host_flags &= 0xfeffffff;
}

void clear_runtime_command_state()
{
    runtime_command_state = 0;
}

void set_credits_runtime_flag()
{
    if((graphics_host_flags & 0x40000000) == 0)
    {
        graphics_host_flags |= 0x40000000;
    }
}

void enter_runtime_state_1000()
{
    if((graphics_host_flags & 0x1000) == 0)
    {
        runtime_state_value = saved_runtime_state_value;
        runtime_state_transition_callback(0);
        graphics_host_flags |= 0x1000;
    }
}

void leave_runtime_state_1000()
{
    if((graphics_host_flags & 0x4000) == 0 && (graphics_host_flags & 0x1000) != 0)
    {
        graphics_host_flags &= 0xffffefff;
        runtime_state_transition_callback(runtime_state_value);
    }
}

RuntimePathApi runtime_path_api{ enter_runtime_path_lock, leave_runtime_path_lock };

void reset_runtime_pair_queue()
{
    if((graphics_host_flags & 0x100400) == 0x100400)
    {
        runtime_queue_api.enter_pair_lock();
        runtime_display_context.pair_write_index = 0;
        runtime_display_context.pair_read_index = 0;
        runtime_queue_api.leave_pair_lock();
    }
}

void enqueue_runtime_byte(uint8_t value)
{
    if((graphics_host_flags & 0x100400) == 0x100400)
    {
        runtime_queue_api.enter_byte_lock();
        runtime_display_context.byte_available = 1;
        runtime_display_context.byte_queue[runtime_display_context.byte_write_index] = value;
        ++runtime_display_context.byte_write_index;
        if(runtime_display_context.byte_write_index == 0x20)
        {
            runtime_display_context.byte_write_index = 0;
        }
        if(runtime_display_context.byte_read_index == runtime_display_context.byte_write_index)
        {
            ++runtime_display_context.byte_read_index;
            if(runtime_display_context.byte_read_index == 0x20)
            {
                runtime_display_context.byte_read_index = 0;
            }
        }
        runtime_queue_api.leave_byte_lock();
    }
}

uint8_t dequeue_runtime_byte()
{
    if(runtime_display_context.byte_available == 0)
    {
        return 0;
    }
    uint8_t value = 0;
    if((graphics_host_flags & 0x100400) == 0x100400)
    {
        runtime_queue_api.enter_byte_lock();
        if(runtime_display_context.byte_read_index != runtime_display_context.byte_write_index)
        {
            value = runtime_display_context.byte_queue[runtime_display_context.byte_read_index];
            ++runtime_display_context.byte_read_index;
            if(runtime_display_context.byte_read_index == 0x20)
            {
                runtime_display_context.byte_read_index = 0;
            }
            if(runtime_display_context.byte_read_index == runtime_display_context.byte_write_index)
            {
                runtime_display_context.byte_available = 0;
            }
        }
        runtime_queue_api.leave_byte_lock();
    }
    return value;
}

void reset_runtime_byte_queue()
{
    if((graphics_host_flags & 0x100400) == 0x100400)
    {
        runtime_queue_api.enter_byte_lock();
        runtime_display_context.byte_write_index = 0;
        runtime_display_context.byte_read_index = 0;
        runtime_queue_api.leave_byte_lock();
    }
}

DisplaySceneNode *acquire_runtime_text_input_scene(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format)
{
    release_runtime_text_input_scene_guard();
    const intptr_t current_identifier = runtime_display_context.input_scene_identifier;
    if(current_identifier != 0 && runtime_text_input_scene_redraw_api.begin_update(current_identifier) == 0)
    {
        auto *current = reinterpret_cast<DisplaySceneNode *>(current_identifier);
        if(width <= static_cast<uint32_t>(current->width) && height <= static_cast<uint32_t>(current->height))
        {
            runtime_text_input_guarded_scene = current_identifier;
            flags |= 0x200000;
        }
        else
        {
            runtime_text_input_scene_redraw_api.end_update(current_identifier, nullptr, nullptr);
        }
    }

    DisplaySceneNode *scene = runtime_text_input_scene_redraw_api.acquire_scene(index, x, y, width, height, flags, owner, descriptor, format);
    if(scene == nullptr)
    {
        release_runtime_text_input_scene_guard();
    }
    return scene;
}

uint32_t begin_runtime_text_input_scene_update(intptr_t identifier)
{
    const uint32_t result = runtime_text_input_scene_redraw_api.begin_update(identifier);
    if(result != 0)
    {
        release_runtime_text_input_scene_guard();
    }
    return result;
}

uint32_t end_runtime_text_input_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle)
{
    const uint32_t result = runtime_text_input_scene_redraw_api.end_update(identifier, transform, rectangle);
    release_runtime_text_input_scene_guard();
    return result;
}

void process_runtime_text_input(RuntimeCommandLoopState *state)
{
    if((state->flags & 0x100) == 0)
    {
        return;
    }

    bool changed = false;
    uint8_t value = runtime_text_input_api.dequeue_byte();
    uint32_t cursor = state->input_cursor;
    if(state->input_end - cursor == 1 || value == 0x0d)
    {
        state->input_text[cursor] = '\0';
        ++state->input_cursor;
        state->flags &= 0xfffffeff;
        if(state->input_scene_identifier != 0)
        {
            runtime_text_input_api.release_scene(state->input_scene_identifier, reinterpret_cast<intptr_t>(state));
            state->input_scene_identifier = 0;
        }
        return;
    }

    if(value != 0)
    {
        if(value == 8)
        {
            if(cursor != 0)
            {
                state->input_text[cursor] = '\0';
                --state->input_cursor;
                changed = true;
            }
        }
        else
        {
            if(static_cast<int8_t>(value) > '@')
            {
                uint32_t letter_mode = state->input_text_flags & 0x30;
                if(letter_mode == 0x10)
                {
                    value |= 0x20;
                }
                else if(letter_mode == 0x20)
                {
                    value &= 0xdf;
                }
            }
            state->input_text[cursor] = static_cast<char>(value);
            cursor = state->input_cursor++;
            state->input_text[cursor + 1] = '\0';
            changed = true;
        }
    }

    if(state->input_scene_identifier == 0)
    {
        return;
    }

    DWORD current_tick = runtime_text_input_api.time_get_time();
    char &caret = state->input_text[state->input_cursor];
    if(state->input_caret_tick + 250 < current_tick)
    {
        if(caret != '-')
        {
            caret = '-';
            changed = true;
            state->input_text[state->input_cursor + 1] = '\0';
        }
    }
    else if(caret != '\0')
    {
        caret = ' ';
        changed = true;
        state->input_text[state->input_cursor + 1] = '\0';
    }

    if(state->input_caret_tick + 500 < current_tick)
    {
        state->input_caret_tick = current_tick;
    }

    RuntimeStandaloneTextState &text_state = runtime_display_context.input_text_state;
    if(changed
        && runtime_text_input_api.initialize_text(runtime_display_context.input_text, text_state.x, text_state.y, text_state.font_identity, text_state.low_color, text_state.high_color, &text_state)
               != 0)
    {
        DisplaySceneDescriptor descriptor;
        runtime_display_context.input_scene_identifier = reinterpret_cast<intptr_t>(runtime_text_input_api.acquire_scene(runtime_display_context.input_scene_index, 0, 0, text_state.bounds[2],
            text_state.bounds[3], 0x120000, reinterpret_cast<intptr_t>(&runtime_display_context), &descriptor, nullptr));
        if(runtime_text_input_api.begin_update(runtime_display_context.input_scene_identifier) == 0)
        {
            runtime_text_input_api.draw_text(&text_state, &descriptor);
            const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
            runtime_text_input_api.end_update(runtime_display_context.input_scene_identifier, &transform, &text_state.bounds_rectangle);
        }
    }
}

void enqueue_runtime_pair(uint32_t first, uint32_t second)
{
    bool input_enabled = (graphics_host_flags & 0x100400) == 0x100400;
    // A borderless transition temporarily clears the queue-enable bit while the capture child remains interactive. Retain the physical release, but not resize-generated moves that would
    // change the active game region before that release is applied.
    if(modern_windows_presentation_state.fullscreen && (graphics_host_flags & 0x400) != 0 && first == WM_LBUTTONUP)
    {
        input_enabled = true;
    }
    if(input_enabled)
    {
        runtime_queue_api.enter_pair_lock();
        runtime_display_context.pair_available = 1;
        runtime_display_context.pair_queue[runtime_display_context.pair_write_index].first = first;
        runtime_display_context.pair_queue[runtime_display_context.pair_write_index].second = second;
        ++runtime_display_context.pair_write_index;
        if(runtime_display_context.pair_write_index == 0x20)
        {
            runtime_display_context.pair_write_index = 0;
        }
        if(runtime_display_context.pair_read_index == runtime_display_context.pair_write_index)
        {
            ++runtime_display_context.pair_read_index;
            if(runtime_display_context.pair_read_index == 0x20)
            {
                runtime_display_context.pair_read_index = 0;
            }
        }
        runtime_queue_api.leave_pair_lock();
    }
}

int dequeue_runtime_pair(RuntimeMessagePair *pair)
{
    if(runtime_display_context.pair_available == 0)
    {
        return 0;
    }
    int result = 0;
    if((graphics_host_flags & 0x100400) == 0x100400)
    {
        runtime_queue_api.enter_pair_lock();
        if(runtime_display_context.pair_read_index != runtime_display_context.pair_write_index)
        {
            *pair = runtime_display_context.pair_queue[runtime_display_context.pair_read_index];
            ++runtime_display_context.pair_read_index;
            if(runtime_display_context.pair_read_index == 0x20)
            {
                runtime_display_context.pair_read_index = 0;
            }
            if(runtime_display_context.pair_read_index == runtime_display_context.pair_write_index)
            {
                runtime_display_context.pair_available = 0;
            }
            result = 1;
        }
        runtime_queue_api.leave_pair_lock();
    }
    return result;
}

bool synchronize_runtime_plan_mode()
{
    bool changed = false;
    if((runtime_display_context.flags & 0x40000000) == 0)
    {
        if((runtime_display_context.flags & 0x80000000) != 0)
        {
            changed = runtime_plan_mode_sync_api.clear_inactive();
            if(changed)
            {
                runtime_plan_mode_sync_api.rebuild();
            }
            runtime_display_context.flags &= 0x7fffffff;
        }
    }
    else if((runtime_display_context.flags & 0x80000000) == 0)
    {
        changed = runtime_plan_mode_sync_api.set_inactive();
        if(changed)
        {
            runtime_plan_mode_sync_api.rebuild();
        }
        runtime_display_context.flags |= 0x80000000;
    }
    return changed;
}

bool process_pending_runtime_tree_switch(RuntimeTreeNode *node)
{
    bool changed = false;
    if((runtime_display_context.flags & 0x4000000) != 0)
    {
        runtime_display_context.accumulated_tree_flags = 0;
        runtime_pending_tree_switch_api.destroy_resources(node);
        RuntimeTreeNode *activated = runtime_pending_tree_switch_api.activate_tree(runtime_display_context.first_runtime_path, runtime_display_context.second_runtime_path, nullptr, nullptr);
        if(activated == nullptr)
        {
            runtime_pending_tree_switch_api.rebuild_runtime_plans(node);
        }
        else
        {
            changed = node->identity != activated;
            if(!changed)
            {
                runtime_pending_tree_switch_api.finalize_current_tree(activated);
            }
            runtime_pending_tree_switch_api.rebuild_runtime_plans(activated);
            activated->flags |= runtime_display_context.accumulated_tree_flags;
        }
        runtime_pending_tree_switch_api.update_pointer(runtime_scene_x, runtime_scene_y);
        runtime_display_context.flags &= 0xfbffffff;
    }
    return changed;
}



RuntimeTreeNode *activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *parent_selector, void *creation_context)
{
    runtime_tree_activation_api.send_message(runtime_display_context.window, 0x7ffd, 0xf0000000, 0);
    RuntimeGenericResourceNode *resource = runtime_tree_activation_api.find_or_load_resource(resource_name);
    RuntimeTreeNode *node = runtime_tree_activation_api.create_tree_node(resource, parent_selector, tree_name, creation_context);
    if(node != nullptr)
    {
        if((node->flags & 0x1000) != 0 || node->name[0] == 0)
        {
            runtime_tree_activation_api.set_script_flags(2, 0);
            runtime_tree_activation_api.set_script_flags(4, 0);
        }
        if((node->flags & 0x800) != 0)
        {
            runtime_tree_activation_api.activate_comment(node);
        }
        runtime_tree_activation_api.send_message(runtime_display_context.window, 0x7ffd, 0x20000000, reinterpret_cast<LPARAM>(node));
    }
    return node;
}


uint32_t process_runtime_pair_message()
{
    RuntimeMessagePair pair;
    if(runtime_pair_dispatch_api.dequeue_pair(&pair) != 0 && (runtime_display_context.flags & 4) == 0)
    {
        switch(pair.first)
        {
        case 0x200:
            return runtime_pair_dispatch_api.move_pointer(static_cast<int32_t>(pair.second & 0xffff), static_cast<int32_t>(pair.second >> 16));
        case 0x201:
            return runtime_pair_dispatch_api.left_button_down();
        case 0x202:
            return runtime_pair_dispatch_api.left_button_up();
        case 0x204:
            return runtime_pair_dispatch_api.right_button_down();
        }
    }
    return 0;
}



uint32_t copy_runtime_input_session_record(RuntimeInputSessionRecord *record)
{
    std::memcpy(record, runtime_display_context.input_text, sizeof(*record));
    uint32_t status = runtime_display_context.input_cursor;
    runtime_display_context.input_cursor = 0;
    return status;
}

void initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, uint32_t character_width, void *session_value)
{
    if(runtime_display_context.input_scene_identifier != 0)
    {
        return;
    }

    runtime_input_session_api.reset_byte_queue();
    runtime_display_context.input_text[1] = '\0';
    runtime_display_context.input_cursor = 0;
    runtime_display_context.input_text_flags = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(session_value));
    runtime_display_context.input_text[0] = '-';
    runtime_display_context.input_end = character_width;
    if(runtime_display_context.input_end == 0)
    {
        runtime_display_context.input_end = 0x20;
    }
    runtime_display_context.input_caret_tick = runtime_input_session_api.get_time();

    RuntimeLockRecord *record = runtime_input_session_api.acquire_record(selector);
    void *font_identity = nullptr;
    if(record != nullptr)
    {
        font_identity = record->identity_context;
    }
    if(runtime_input_session_api.initialize_text(runtime_display_context.input_text, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(first)),
           static_cast<uint32_t>(reinterpret_cast<uintptr_t>(second)), font_identity, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fourth)),
           static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fifth)), &runtime_display_context.input_text_state)
        != 0)
    {
        runtime_display_context.input_scene_index = runtime_input_session_api.find_scene_index(0x80000);
        intptr_t scene_identifier;
        if((record->flags & 0x04000000) != 0)
        {
            scene_identifier = runtime_display_context.input_alternate_scene_identifier;
        }
        else
        {
            scene_identifier = record->scene_identifier;
        }
        DisplaySceneNode *locked_scene = runtime_input_session_api.lock_scene(scene_identifier);
        if(locked_scene != nullptr)
        {
            DisplaySceneDescriptor descriptor;
            runtime_display_context.input_scene_identifier = reinterpret_cast<intptr_t>(runtime_input_session_api.acquire_scene(runtime_display_context.input_scene_index,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(first)), static_cast<int32_t>(reinterpret_cast<uintptr_t>(second)), runtime_display_context.input_text_state.bounds[2],
                runtime_display_context.input_text_state.bounds[3], 0x20000, reinterpret_cast<intptr_t>(&runtime_display_context), &descriptor, &default_display_pixel_format));
            if(runtime_input_session_api.begin_update(runtime_display_context.input_scene_identifier) == 0)
            {
                runtime_input_session_api.draw_text(&runtime_display_context.input_text_state, &descriptor);
                const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
                runtime_input_session_api.end_update(runtime_display_context.input_scene_identifier, &transform, &runtime_display_context.input_text_state.bounds_rectangle);
            }
        }
        runtime_input_session_api.unlock_scene(scene_identifier);
    }
    if(record != nullptr)
    {
        runtime_input_session_api.release_record(record);
    }
    runtime_display_context.flags |= 0x100;
}

int run_runtime_command_loop(RuntimeCommandLoopState *state)
{
    uint32_t initial_flags = state->flags;
    if((initial_flags & 0x03000040) == 0)
    {
        return 0;
    }
    const uint32_t restore_flag = initial_flags & 0x100000;
    state->flags = (initial_flags & 0xffefffff) | 0x01000000;
    runtime_command_loop_api.begin_first();
    runtime_command_loop_api.begin_second();
    runtime_command_loop_api.begin_third(0);
    runtime_command_loop_api.post_message(state->window, 0x7ffd, 0x60000000, 0);
    while(true)
    {
        runtime_command_loop_api.process(state);
        if((state->flags & 0x02000000) != 0)
        {
            LPARAM script_state = runtime_command_loop_api.get_script_state();
            graphics_host_flags &= 0xfdffffff;
            runtime_command_loop_api.post_message(state->window, 0x7ffd, 0x70000000, script_state);
        }
        if((state->flags & 0x40) != 0)
        {
            runtime_command_loop_api.complete_first();
            runtime_command_loop_api.cancel_first();
            runtime_command_loop_api.cancel_second();
            state->flags &= 0xfeffffbf;
            runtime_command_loop_api.post_message(state->window, 0x7ffd, 0x90000000, 0);
            return 1;
        }
        runtime_command_loop_api.sleep(100);
        if((state->flags & 0x01000000) == 0)
        {
            state->flags |= restore_flag;
            runtime_command_loop_api.cancel_first();
            runtime_command_loop_api.cancel_second();
            runtime_command_loop_api.cancel_third();
            runtime_command_loop_api.post_message(state->window, 0x7ffd, 0x80000000, 0);
            return 1;
        }
    }
}

uint32_t run_pending_runtime_external_command()
{
    uint32_t result = 0;
    if((runtime_scene_control_flags & 0x200000) != 0)
    {
        runtime_display_context.external_command_pending = 1;
        if(runtime_external_command_api.send_message(runtime_display_context.window, 0x7ffd, 0x02000000, reinterpret_cast<LPARAM>(runtime_display_context.command_context)) != 0)
        {
            result = 0;
            while(runtime_display_context.external_command_pending != 0)
            {
                runtime_external_command_api.process_message(&runtime_display_context);
                result |= runtime_external_command_api.run_command_loop(&runtime_display_context);
                runtime_external_command_api.sleep(10);
            }
        }
        runtime_display_context.external_command_pending = 0;
        runtime_scene_control_flags &= 0xffdfffff;
    }
    return result;
}

RuntimeScriptOpcodeDisposition execute_simple_runtime_script_opcode(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, uint32_t opcode, int32_t random_value,
    uint32_t saved_cursor)
{
    ScriptParserState *parser = &link->parser;
    char first[0x20];
    char second[0x20];
    uint32_t typed_value[8];
    uint32_t value_type;

    switch(opcode)
    {
    case 0xb0000000:
    case 0x70000000:
    {
        uint32_t target_flags = 0;
        RuntimeTreeNode *target;
        if(parse_runtime_tree_command_target(parser, second, first, &target_flags) == 0)
        {
            target = parser->owner;
        }
        else
        {
            target = static_cast<RuntimeTreeNode *>(find_runtime_tree_identity_by_name_recursive(tree, first));
            if(target == nullptr)
            {
                target = static_cast<RuntimeTreeNode *>(find_runtime_tree_root_identity_by_name(first));
                if(target == nullptr)
                {
                    return RuntimeScriptOpcodeDisposition::complete;
                }
            }
        }
        if(target_flags == 0)
        {
            state->accumulated_tree_flags = target->flags & 0xff;
        }
        else
        {
            state->accumulated_tree_flags = target_flags | 0x10000000;
        }
        if(target->parent == nullptr)
        {
            if(state->current_runtime_resource != nullptr && activate_default_comment_scene("m_DEF_LOAD") > 0)
            {
                wait_for_display_scene_ready(500);
            }
            destroy_runtime_tree_resources(target);
            RuntimeTreeNode *previous = target->previous;
            if(previous == nullptr)
            {
                rebuild_runtime_tree_resources(target);
                refresh_runtime_pointer_region();
                return RuntimeScriptOpcodeDisposition::finish_link;
            }
            auto *deactivated = reinterpret_cast<RuntimeTreeNode *>(static_cast<uintptr_t>(deactivate_runtime_tree_and_visuals(target, previous)));
            if((previous->flags & 0x200) == 0 || deactivated == previous)
            {
                reset_runtime_tree_parser_contexts(deactivated);
            }
            else
            {
                opcode = 0x70000000;
            }
            if(opcode != 0xb0000000)
            {
                rebuild_runtime_tree_resources(deactivated);
                deactivated = static_cast<RuntimeTreeNode *>(state->runtime_tree_identity);
            }
            state->runtime_tree_identity = deactivated;
            refresh_runtime_pointer_region();
            // Replacing the root may release the current link and parser, so skip the common tail.
            return RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor;
        }

        destroy_runtime_tree_resources(target);
        const bool root_level = target->parent == reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        std::memcpy(first, target->name, sizeof(first));
        std::memcpy(typed_value, parser->owner->name, 0x20);
        if(root_level || (reset_runtime_tree_parser_contexts(tree), root_level) || find_runtime_tree_descendant_identity_by_name(tree, first) != nullptr)
        {
            deactivate_runtime_tree_and_visuals(target, nullptr);
        }
        void *continuation;
        if(root_level)
        {
            continuation = find_runtime_tree_root_identity_by_name(typed_value);
        }
        else
        {
            continuation = find_runtime_tree_descendant_identity_by_name(tree, typed_value);
        }
        if(continuation == nullptr)
        {
            state->active_script_link = nullptr;
        }
        rebuild_runtime_pointer_resources();
        refresh_runtime_pointer_region();
        // A missing continuation may deactivate and release the current link, so skip the common tail.
        return continuation == nullptr ? RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor : RuntimeScriptOpcodeDisposition::complete;
    }

    case 0xa0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary == nullptr)
            {
                if(find_runtime_tree_identity_by_name_recursive(nullptr, first) != nullptr)
                {
                    return RuntimeScriptOpcodeDisposition::pause;
                }
            }
            else
            {
                const uint32_t flags = query_runtime_resource_playback_flags(primary->resource_identity);
                if(flags != 0 && (flags & 0x80001000) == 0 && ((flags & 0x2000) == 0 || (flags & 1) == 0 || (flags & 0x20) != 0)
                    && ((flags & 0x400) == 0 || query_runtime_resource_frame_limit(primary->resource_identity) != 0xffffffff))
                {
                    return RuntimeScriptOpcodeDisposition::pause;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xf0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary == nullptr)
            {
                if(find_runtime_tree_identity_by_name_recursive(nullptr, first) == nullptr)
                {
                    return RuntimeScriptOpcodeDisposition::pause;
                }
            }
            else
            {
                const uint32_t flags = query_runtime_resource_playback_flags(primary->resource_identity);
                if(flags != 0 && (flags & 0x80001001) == 0 && (flags & 0x2000) != 0)
                {
                    return RuntimeScriptOpcodeDisposition::pause;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x90000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            uint32_t flags = 0;
            uint32_t parsed_flag;
            while((parsed_flag = parse_image_flag(parser)) != 0xffffffff)
            {
                flags |= parsed_flag;
            }
            if(primary != nullptr)
            {
                set_runtime_resource_state(primary->resource_identity, flags);
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x80000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
        {
            parse_script_typed_value(parser, typed_value, &value_type);
            if(value_type != 0x7fffffff)
            {
                resolve_state_field_reference(first, second, typed_value, static_cast<int>(value_type));
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xd0000000:
    case 0xc0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            if(opcode == 0xd0000000)
            {
                rotate_runtime_named_node_cursor_next(first, 1);
            }
            else
            {
                rotate_runtime_named_node_cursor_previous(first, 1);
            }
            rebuild_runtime_pointer_resources();
            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xe0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreeLink84 *zone = find_global_runtime_tree_link_0084_by_name(first);
            if(zone != nullptr)
            {
                const int32_t x = parse_script_integer_expression(parser);
                const int32_t y = parse_script_integer_expression(parser);
                if(x != 0x7fffffff && y != 0x7fffffff)
                {
                    update_runtime_tree_link_0084(tree, zone, x, y, zone->width - zone->x + x, zone->height - zone->y + y, 0, nullptr, nullptr, 0, 0, 0x7fffffff);
                    rebuild_runtime_pointer_resources();
                    update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                }
                else if(parse_script_value_token(parser, second, sizeof(second)) == 0xffffffff)
                {
                    zone->movement_flags |= 2;
                }
                else
                {
                    RuntimeTreeLink8C *path = find_global_runtime_tree_link_008c_by_name(second);
                    if(path != nullptr)
                    {
                        if((zone->movement_flags & 1) == 0)
                        {
                            zone->movement_deadline = state->script_clock;
                            zone->movement_flags = (zone->movement_flags & 0xfffffffd) | 1;
                        }
                        if(state->script_clock < zone->movement_deadline)
                        {
                            return RuntimeScriptOpcodeDisposition::pause;
                        }
                        if((zone->movement_flags & 2) == 0 && path->x <= zone->x && path->y <= zone->y && zone->width <= path->width && zone->height <= path->height)
                        {
                            if((path->flags & 1) != 0)
                            {
                                update_runtime_tree_link_0084(tree->identity, zone->identity, zone->x + path->line_first, zone->y + path->line_second, zone->width + path->line_first,
                                    zone->height + path->line_second, 0, nullptr, nullptr, 0, 0, 0x7fffffff);
                                zone->movement_deadline = state->script_clock + path->time;
                            }
                            rebuild_runtime_pointer_resources();
                            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                            return RuntimeScriptOpcodeDisposition::pause;
                        }
                    }
                    zone->movement_flags &= 0xfffffffc;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 1:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                primary->flags &= 0x7fffffff;
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x100:
        if((link->owner_flags & 0x40000000) == 0)
        {
            if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
            {
                uintptr_t selection = static_cast<uint32_t>(parse_script_integer_expression(parser));
                char selection_name[0x20];
                if(selection == 0x7fffffff)
                {
                    if(parse_script_value_token(parser, selection_name, sizeof(selection_name)) == 0xffffffff)
                    {
                        return RuntimeScriptOpcodeDisposition::complete;
                    }
                    selection = reinterpret_cast<uintptr_t>(selection_name);
                }
                else
                {
                    selection &= 0xffff;
                }
                RuntimeTreeSecondaryResourceLink *secondary = find_global_runtime_tree_secondary_resource_link_by_name(first);
                RuntimeFixedNameListNode *fixed = find_runtime_fixed_name_list_node(second);
                if(secondary != nullptr && fixed != nullptr)
                {
                    link->backend_child = attach_runtime_generic_backend_child(nullptr, fixed->resource_identity, secondary->resource_identity, selection, 0);
                    if(link->backend_child != nullptr)
                    {
                        link->secondary_resource_identity = secondary->resource_identity;
                        link->fixed_resource_identity = fixed->resource_identity;
                        link->owner_flags |= 0x40010000;
                        return RuntimeScriptOpcodeDisposition::pause;
                    }
                }
            }
        }
        else
        {
            const uint32_t flags = get_runtime_generic_backend_child_flags(link->backend_child);
            if(flags != 0x7fffffff && (flags & 0x200) != 0)
            {
                return RuntimeScriptOpcodeDisposition::pause;
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x200:
        if((state->flags & 0x100) != 0)
        {
            return RuntimeScriptOpcodeDisposition::pause;
        }
        if(state->input_cursor == 0)
        {
            void *first_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            void *second_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            parse_script_value_token(parser, first, sizeof(first));
            void *fourth_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            void *fifth_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            if(parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
            {
                uint32_t character_width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(character_width == 0x7fffffff)
                {
                    character_width = 0;
                }
                void *session_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_image_flag(parser)));
                RuntimeFixedNameListNode *fixed = find_runtime_fixed_name_list_node(first);
                initialize_runtime_input_session(first_value, second_value, fixed == nullptr ? nullptr : fixed->resource_identity, fourth_value, fifth_value, character_width, session_value);
                return RuntimeScriptOpcodeDisposition::pause;
            }
        }
        else
        {
            parse_script_integer_expression(parser);
            parse_script_integer_expression(parser);
            parse_script_value_token(parser, first, sizeof(first));
            parse_script_integer_expression(parser);
            parse_script_integer_expression(parser);
            if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
            {
                RuntimeInputSessionRecord input{};
                copy_runtime_input_session_record(&input);
                resolve_state_field_reference(first, second, &input, 4);
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x300:
    {
        state->external_command_pending = 1;
        uint32_t result = 0;
        const int32_t command = parse_script_integer_expression(parser);
        if(command != 0x7fffffff)
        {
            ScriptObjectFieldSnapshot snapshot;
            LPARAM parameter = 0;
            if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
            {
                if(parse_script_value_token(parser, second, sizeof(second)) == 0xffffffff || get_script_object_field_snapshot(first, second, &snapshot) == 0)
                {
                    state->external_command_pending = 0;
                    return RuntimeScriptOpcodeDisposition::complete;
                }
                parameter = reinterpret_cast<LPARAM>(&snapshot);
            }
            if(should_send_runtime_script_message(command))
            {
                result = static_cast<uint32_t>(SendMessageA(runtime_display_context.window, 0x7ffd, static_cast<WPARAM>(command), parameter));
            }
        }
        if(result != 0)
        {
            result = 0;
            while(state->external_command_pending != 0)
            {
                process_runtime_message(state);
                result |= static_cast<uint32_t>(run_runtime_command_loop(state));
                Sleep(10);
            }
        }
        state->external_command_pending = 0;
        // after this /MESSAGE rather than issue it again.
        return result == 0 ? RuntimeScriptOpcodeDisposition::complete : RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor;
    }

    case 0x400:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreeSceneLink *scene = find_global_runtime_tree_scene_link_by_name(first);
            intptr_t identifier;
            int32_t x;
            int32_t y;
            uint32_t width;
            uint32_t height;
            if(scene == nullptr)
            {
                if(!strings_equal(first, "BACKGND"))
                {
                    return RuntimeScriptOpcodeDisposition::complete;
                }
                identifier = state->input_alternate_scene_identifier;
                x = parse_script_integer_expression(parser);
                y = parse_script_integer_expression(parser);
                width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                height = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(x == 0x7fffffff)
                {
                    x = 0;
                }
                if(y == 0x7fffffff)
                {
                    y = 0;
                }
                if(width == 0x7fffffff)
                {
                    width = runtime_display_context.width;
                }
                if(height == 0x7fffffff)
                {
                    height = runtime_display_context.height;
                }
            }
            else
            {
                identifier = scene->scene_identifier;
                x = parse_script_integer_expression(parser);
                y = parse_script_integer_expression(parser);
                width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                height = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(x == 0x7fffffff)
                {
                    x = 0;
                }
                if(y == 0x7fffffff)
                {
                    y = 0;
                }
                if(width == 0x7fffffff)
                {
                    width = scene->width;
                }
                if(height == 0x7fffffff)
                {
                    height = scene->height;
                }
            }
            update_runtime_resource_scene_region(identifier, x, y, width, height);
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x500:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            clear_runtime_named_node_children(first);
            rebuild_runtime_pointer_resources();
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x600:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            const uint32_t fade_flag = parse_image_flag(parser);
            if(static_cast<int32_t>(fade_flag) > 0)
            {
                const int32_t duration = parse_script_integer_expression(parser);
                RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
                if(duration != 0x7fffffff && primary != nullptr)
                {
                    RuntimeLockRecord *record = acquire_runtime_lock_record(primary->resource_identity);
                    if(record != nullptr)
                    {
                        if((record->type_flags & 0x8000) != 0)
                        {
                            if(fade_flag == 0x03000000)
                            {
                                set_runtime_sound_volume(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), 0);
                                fade_out_runtime_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), duration, 1);
                            }
                            else if(fade_flag == 0x07000000)
                            {
                                fade_in_runtime_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), duration, 1);
                            }
                        }
                        release_runtime_lock_record(record);
                    }
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x1000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                primary->flags |= 0x80000000;
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x3000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
        {
            const int32_t delta = parse_script_integer_expression(parser);
            if(delta != 0x7fffffff)
            {
                add_script_object_integer(first, second, delta);
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x4000:
        if(parse_script_value_token(parser, first, sizeof(first)) == 0xffffffff || parse_script_value_token(parser, second, sizeof(second)) == 0xffffffff)
        {
            return RuntimeScriptOpcodeDisposition::complete;
        }
        [[fallthrough]];

    case 0x40000:
    case 0x50000:
        for(;;)
        {
            const uint32_t control_opcode = parse_script_opcode(parser);
            if(control_opcode == 0xffffffff)
            {
                return RuntimeScriptOpcodeDisposition::complete;
            }
            if(control_opcode <= 0x6000)
            {
                if(control_opcode == 0x6000)
                {
                    return RuntimeScriptOpcodeDisposition::complete;
                }
                if(control_opcode == 0x5000)
                {
                    parse_script_typed_value(parser, typed_value, &value_type);
                    if((value_type != 0x7fffffff && compare_script_object_field(first, second, typed_value, static_cast<int32_t>(value_type)))
                        || scan_runtime_tree_link_007c_control_boundary(link, 0x60000) == 0x6000)
                    {
                        return RuntimeScriptOpcodeDisposition::complete;
                    }
                }
                continue;
            }
            uint32_t boundary = 0xffffffff;
            if(control_opcode == 0x70000)
            {
                const int32_t minimum = parse_script_integer_expression(parser);
                const int32_t maximum = parse_script_integer_expression(parser);
                if(minimum != 0x7fffffff && maximum != 0x7fffffff && minimum <= random_value && random_value <= maximum)
                {
                    return RuntimeScriptOpcodeDisposition::complete;
                }
                boundary = scan_runtime_tree_link_007c_control_boundary(link, 0x60000);
            }
            else if(control_opcode == 0x80000)
            {
                if(parse_script_value_token(parser, reinterpret_cast<char *>(typed_value), 0x20) != 0xffffffff && script_object_container_state_matches_by_name(typed_value))
                {
                    return RuntimeScriptOpcodeDisposition::complete;
                }
                boundary = scan_runtime_tree_link_007c_control_boundary(link, 0x60000);
            }
            if(boundary == 0x6000)
            {
                return RuntimeScriptOpcodeDisposition::complete;
            }
        }

    case 0xa000:
        if((link->owner_flags & 0x20000000) == 0)
        {
            link->owner_flags |= 0x20000000;
            begin_display_scene_update(state->input_alternate_scene_identifier);
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xb000:
        if((link->owner_flags & 0x20000000) != 0)
        {
            link->owner_flags &= 0xdfffffff;
            end_display_scene_update(state->input_alternate_scene_identifier, nullptr, nullptr);
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xc000:
        if((state->flags & 0x10) != 0)
        {
            return RuntimeScriptOpcodeDisposition::pause;
        }
        if((state->flags & 0x20) == 0)
        {
            if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && load_and_initialize_runtime_game_dll(first))
            {
                return RuntimeScriptOpcodeDisposition::pause;
            }
        }
        else
        {
            state->flags &= 0xffffffdf;
            parse_script_value_token(parser, first, sizeof(first));
            if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
            {
                resolve_state_field_reference(first, second, state->game_result_data, static_cast<int>(state->game_result_type));
                std::memset(state->game_result_data, 0, sizeof(state->game_result_data));
                state->game_result_type = 0;
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xd000:
        stop_runtime_game_dll();
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xe000:
        if(state->nested_runtime_state_count == 0)
        {
            state->flags |= 0x4000;
            enter_runtime_state_1000();
        }
        ++state->nested_runtime_state_count;
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xf000:
        if(state->nested_runtime_state_count == 1)
        {
            state->flags &= 0xffffbfff;
            leave_runtime_state_1000();
        }
        if(state->nested_runtime_state_count != 0)
        {
            --state->nested_runtime_state_count;
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x7000:
    case 0x8000:
    case 0x60000000:
    {
        void *parent_identity;
        if(opcode == 0x7000)
        {
            parent_identity = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        }
        else if(opcode == 0x8000)
        {
            parent_identity = tree;
        }
        else
        {
            parent_identity = parser->owner;
        }
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            if(parse_script_value_token(parser, second, sizeof(second)) == 0xffffffff)
            {
                std::memcpy(second, first, sizeof(second));
                std::memcpy(first, parser->resource->name, sizeof(first));
            }
            RuntimeTreeNode *activated = activate_runtime_tree_with_notifications(first, second, parent_identity, nullptr);
            if(activated != nullptr)
            {
                rebuild_runtime_tree_resources(activated);
            }
            refresh_runtime_pointer_region();
        }
        return RuntimeScriptOpcodeDisposition::complete;
    }

    case 0x9000:
        if(parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
        {
            DisplaySceneNode *destination = nullptr;
            RuntimeTreePrimaryResourceLink *destination_primary = find_global_runtime_tree_primary_resource_link_by_name(second);
            if(destination_primary != nullptr)
            {
                RuntimeLockRecord *record = acquire_runtime_lock_record(destination_primary->resource_identity);
                if(record != nullptr)
                {
                    destination = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(record->scene_identifier));
                    release_runtime_lock_record(record);
                }
            }
            else
            {
                RuntimeTreeSceneLink *destination_scene = find_global_runtime_tree_scene_link_by_name(second);
                if(destination_scene != nullptr)
                {
                    destination = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(destination_scene->scene_identifier));
                }
                else if(strings_equal(second, "BACKGND"))
                {
                    destination = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(state->input_alternate_scene_identifier));
                }
            }
            if(destination != nullptr)
            {
                int32_t destination_x = parse_script_integer_expression(parser);
                int32_t destination_y = parse_script_integer_expression(parser);
                if(destination_x == 0x7fffffff)
                {
                    destination_x = 0;
                }
                if(destination_y == 0x7fffffff)
                {
                    destination_y = 0;
                }
                if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
                {
                    DisplaySceneNode *source = nullptr;
                    DisplayRectangle rectangle{};
                    RuntimeTreePrimaryResourceLink *source_primary = find_global_runtime_tree_primary_resource_link_by_name(first);
                    if(source_primary != nullptr)
                    {
                        RuntimeLockRecord *record = acquire_runtime_lock_record(source_primary->resource_identity);
                        if(record != nullptr)
                        {
                            auto *resource = reinterpret_cast<RuntimeResourceObject *>(record);
                            source = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(record->scene_identifier));
                            rectangle.left = parse_script_integer_expression(parser);
                            rectangle.top = parse_script_integer_expression(parser);
                            rectangle.right = parse_script_integer_expression(parser);
                            rectangle.bottom = parse_script_integer_expression(parser);
                            if(rectangle.left == 0x7fffffff)
                            {
                                rectangle.left = 0;
                            }
                            if(rectangle.top == 0x7fffffff)
                            {
                                rectangle.top = 0;
                            }
                            if(rectangle.right == 0x7fffffff)
                            {
                                rectangle.right = static_cast<int32_t>(resource->output_width);
                            }
                            if(rectangle.bottom == 0x7fffffff)
                            {
                                rectangle.bottom = static_cast<int32_t>(resource->output_height);
                            }
                            release_runtime_lock_record(record);
                        }
                    }
                    else
                    {
                        RuntimeTreeSceneLink *source_scene = find_global_runtime_tree_scene_link_by_name(second);
                        if(source_scene != nullptr && source_scene->scene_identifier != 0)
                        {
                            source = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(source_scene->scene_identifier));
                            rectangle.left = parse_script_integer_expression(parser);
                            rectangle.top = parse_script_integer_expression(parser);
                            rectangle.right = parse_script_integer_expression(parser);
                            rectangle.bottom = parse_script_integer_expression(parser);
                            if(rectangle.left == 0x7fffffff)
                            {
                                rectangle.left = 0;
                            }
                            if(rectangle.top == 0x7fffffff)
                            {
                                rectangle.top = 0;
                            }
                            if(rectangle.right == 0x7fffffff)
                            {
                                rectangle.right = static_cast<int32_t>(source_scene->width);
                            }
                            if(rectangle.bottom == 0x7fffffff)
                            {
                                rectangle.bottom = static_cast<int32_t>(source_scene->height);
                            }
                        }
                        else if(strings_equal(second, "BACKGND"))
                        {
                            source = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(state->input_alternate_scene_identifier));
                            rectangle.left = parse_script_integer_expression(parser);
                            rectangle.top = parse_script_integer_expression(parser);
                            rectangle.right = parse_script_integer_expression(parser);
                            rectangle.bottom = parse_script_integer_expression(parser);
                            if(rectangle.left == 0x7fffffff)
                            {
                                rectangle.left = 0;
                            }
                            if(rectangle.top == 0x7fffffff)
                            {
                                rectangle.top = 0;
                            }
                            if(rectangle.right == 0x7fffffff)
                            {
                                rectangle.right = state->width;
                            }
                            if(rectangle.bottom == 0x7fffffff)
                            {
                                rectangle.bottom = state->height;
                            }
                        }
                    }
                    if(source != nullptr)
                    {
                        rectangle.right += rectangle.left;
                        rectangle.bottom += rectangle.top;
                        blit_display_scene(destination, destination_x, destination_y, source, &rectangle, 0x06000000);
                    }
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x10000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                set_runtime_resource_loop_count(primary->resource_identity, 1);
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x20000:
    case 0x30000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
        {
            if(opcode == 0x20000)
            {
                add_script_object_to_runtime_named_node(first, second);
            }
            else
            {
                remove_script_object_from_runtime_named_node(first, second);
            }
            rebuild_runtime_pointer_resources();
            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xc0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                set_runtime_resource_state(primary->resource_identity, 1);
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xb0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            seek_runtime_tree_link_007c_label(link, first);
            return RuntimeScriptOpcodeDisposition::commit_cursor;
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xd0000:
        reset_runtime_session();
        SendMessageA(runtime_display_context.window, 0x7ffd, 0x10000000, 0);
        state->flags &= 0xffefffff;
        // A session reset invalidates the old parser cursor, so do not restore it.
        return RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor;

    case 0xe0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff && parse_script_value_token(parser, second, sizeof(second)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                update_runtime_tree_primary_resource_link(tree, primary, second, 0, 0, 0);
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x90000:
        if(parse_script_value_token(parser, first, sizeof(first)) != 0xffffffff)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                const int32_t x = parse_script_integer_expression(parser);
                const int32_t y = parse_script_integer_expression(parser);
                if(x != 0x7fffffff && y != 0x7fffffff)
                {
                    update_runtime_tree_primary_resource_link(tree, primary, nullptr, x - primary->x, y - primary->y, 0);
                    rebuild_runtime_pointer_resources();
                    update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                }
                else if(parse_script_value_token(parser, second, sizeof(second)) == 0xffffffff)
                {
                    primary->flags |= 2;
                }
                else
                {
                    RuntimeTreeLink8C *path = find_global_runtime_tree_link_008c_by_name(second);
                    if(path != nullptr)
                    {
                        if((primary->flags & 1) == 0)
                        {
                            primary->movement_deadline = state->script_clock;
                            primary->flags = (primary->flags & 0xfffffffd) | 1;
                        }
                        if(state->script_clock < primary->movement_deadline)
                        {
                            return RuntimeScriptOpcodeDisposition::pause;
                        }
                        if((primary->flags & 2) == 0 && path->x <= primary->x && path->y <= primary->y && primary->x <= static_cast<int32_t>(path->width)
                            && primary->y <= static_cast<int32_t>(path->height))
                        {
                            if((path->flags & 1) != 0)
                            {
                                update_runtime_tree_primary_resource_link(tree, primary, nullptr, path->line_first, path->line_second, 0);
                                primary->movement_deadline = state->script_clock + path->time;
                            }
                            rebuild_runtime_pointer_resources();
                            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                            return RuntimeScriptOpcodeDisposition::pause;
                        }
                    }
                    primary->flags &= 0xfffffffc;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0xf0000:
        if((link->owner_flags & 0x40000000) == 0)
        {
            const int32_t duration = parse_script_integer_expression(parser);
            if(duration != 0x7fffffff)
            {
                link->wait_deadline = state->script_clock + static_cast<uint32_t>(duration);
                link->owner_flags |= 0x40000000;
                return RuntimeScriptOpcodeDisposition::pause;
            }
        }
        else
        {
            if(state->script_clock < link->wait_deadline)
            {
                return RuntimeScriptOpcodeDisposition::pause;
            }
            link->owner_flags &= 0xbfffffff;
        }
        return RuntimeScriptOpcodeDisposition::complete;

    case 0x40000000:
    case 0x40000001:
    case 0x50000000:
    case 0x50000001:
    {
        uint32_t target_flags = 0;
        if(parse_runtime_tree_command_target(parser, first, second, &target_flags) == 0)
        {
            return RuntimeScriptOpcodeDisposition::complete;
        }
        state->accumulated_tree_flags = target_flags;
        if((opcode & 0xff) != 0)
        {
            state->accumulated_tree_flags = opcode & 0xff;
        }
        if(target_flags != 0)
        {
            state->accumulated_tree_flags |= 0x10000000;
        }
        if(state->current_runtime_resource != nullptr && activate_default_comment_scene("m_DEF_LOAD") > 0)
        {
            wait_for_display_scene_ready(500);
        }
        destroy_runtime_tree_resources(tree);
        RuntimeTreeNode *activated = activate_runtime_tree_with_notifications(first, second, nullptr, nullptr);
        void *published_identity = state->runtime_tree_identity;
        if(activated == nullptr)
        {
            RuntimeTreeNode *rebuild = nullptr;
            if((link->owner_flags & 0x10000000) != 0)
            {
                copy_runtime_tree_command_name(first, opcode);
                const uint32_t cursor = find_runtime_tree_link_007c_opcode_value(link, 0x700, first, 1);
                if(cursor != 0xffffffff)
                {
                    saved_cursor = cursor;
                }
                rebuild = find_and_create_runtime_tree_jump(parser, first, saved_cursor);
            }
            if(rebuild != nullptr || SendMessageA(state->window, 0x7ffd, 0x04000000, reinterpret_cast<LPARAM>(first)) == 0)
            {
                if(rebuild == nullptr)
                {
                    rebuild = tree;
                }
                rebuild_runtime_tree_resources(rebuild);
                published_identity = state->runtime_tree_identity;
            }
        }
        else
        {
            if(tree->identity == activated)
            {
                reset_runtime_tree_parser_contexts(activated);
                SendMessageA(state->window, 0x7ffd, 0x01000000, 0);
            }
            else if((opcode & 0xffffff00) == 0x50000000)
            {
                deactivate_runtime_tree_and_visuals(tree, activated);
            }
            rebuild_runtime_tree_resources(activated);
            activated->flags |= state->accumulated_tree_flags;
            published_identity = state->runtime_tree_identity;
        }
        state->runtime_tree_identity = published_identity;
        refresh_runtime_pointer_region();
        // The command invalidates the saved parser cursor, so bypass the common restore path.
        return RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor;
    }

    case 0x60000:
        scan_runtime_tree_link_007c_control_boundary(link, 0x6000);
        return RuntimeScriptOpcodeDisposition::complete;

    default:
        return RuntimeScriptOpcodeDisposition::unhandled;
    }
}

bool should_send_runtime_script_message(int32_t command)
{
    // The transition can release ReplyMessage's script caller before the UI thread finishes the callback. Suppress another synchronous 2010 send from that same physical press at
    // the sending boundary, where it cannot block waiting for the still-busy UI thread.
    if(command == 0x7da)
    {
        if(modern_windows_fullscreen_toggle_latched)
        {
            return false;
        }
        modern_windows_fullscreen_toggle_latched = true;
    }
    return true;
}

DWORD WINAPI execute_script_commands(LPVOID parameter)
{
    auto *state = static_cast<RuntimeCommandLoopState *>(parameter);
    uint32_t previous_tick = runtime_script_executor_api.get_tick_count();
    // Initialize before the first outer pass so SWRAND always has a defined prior value.
    int32_t random_value = 0;
    while(true)
    {
        if((state->flags & 1) != 0)
        {
            return 0;
        }
        runtime_script_executor_api.process_children(state->script_clock);
        runtime_script_executor_api.process_message(state);
        runtime_script_executor_api.process_text_input(state);
        runtime_script_executor_api.process_pair_message();
        runtime_script_executor_api.run_command_loop(state);
        if((state->flags & 0x100000) == 0)
        {
            previous_tick = runtime_script_executor_api.time_get_time();
            runtime_script_executor_api.sleep(10);
            continue;
        }

        RuntimeTreeNode *tree = runtime_script_executor_api.resolve_tree(state->runtime_tree_identity);
        bool restart_outer = false;
        if(tree != nullptr)
        {
            runtime_script_executor_api.synchronize_plan_mode();
            if(!runtime_script_executor_api.process_pending_tree_switch(tree))
            {
                runtime_script_executor_api.acknowledge_event();
                for(RuntimeTreeLink7C *link = script_runtime_root->global_link_007c_head; link != nullptr; link = link->next)
                {
                    runtime_script_executor_api.process_children(state->script_clock);
                    runtime_script_executor_api.process_message(state);
                    runtime_script_executor_api.process_text_input(state);
                    if(runtime_script_executor_api.run_external_command() != 0 || runtime_script_executor_api.process_pair_message() != 0 || runtime_script_executor_api.run_command_loop(state) != 0)
                    {
                        break;
                    }

                    ScriptParserState *parser = nullptr;
                    if(runtime_script_executor_api.activate_link(link) != 0)
                    {
                        state->active_script_link = link;
                        parser = &link->parser;
                        while(true)
                        {
                            uint32_t saved_cursor = parser->cursor;
                            const uint32_t opcode = runtime_script_executor_api.parse_opcode(parser);
                            if(opcode == 0xffffffff)
                            {
                                saved_cursor = parser->start_offset;
                                link->owner_flags &= 0x7fffffff;
                                parser->cursor = saved_cursor;
                                break;
                            }
                            const RuntimeScriptOpcodeDisposition disposition = runtime_script_executor_api.dispatch_opcode(state, tree, link, opcode, random_value, saved_cursor);
                            if(disposition == RuntimeScriptOpcodeDisposition::complete || disposition == RuntimeScriptOpcodeDisposition::unhandled)
                            {
                                continue;
                            }
                            if(disposition == RuntimeScriptOpcodeDisposition::finish_link)
                            {
                                saved_cursor = parser->start_offset;
                                link->owner_flags &= 0x7fffffff;
                            }
                            else if(disposition == RuntimeScriptOpcodeDisposition::commit_cursor)
                            {
                                saved_cursor = parser->cursor;
                            }
                            else if(disposition == RuntimeScriptOpcodeDisposition::restart_outer_commit_cursor)
                            {
                                // This disposition returns directly to the outer loop. The command may have destroyed the current tree or link, so neither parser read nor write is valid
                                // here.
                                restart_outer = true;
                                break;
                            }
                            parser->cursor = saved_cursor;
                            if(disposition == RuntimeScriptOpcodeDisposition::restart_outer)
                            {
                                restart_outer = true;
                            }
                            break;
                        }
                    }
                    state->active_script_link = nullptr;
                    if(restart_outer)
                    {
                        break;
                    }
                }
            }
        }

        state->active_script_link = nullptr;
        const uint32_t current_tick = runtime_script_executor_api.time_get_time();
        random_value = runtime_script_executor_api.select_random(-10000, 10000);
        state->script_clock = current_tick + (state->script_clock - previous_tick);
        previous_tick = current_tick;
        runtime_script_executor_api.sleep(10);
    }
}

void process_runtime_message(RuntimeCommandLoopState *state)
{
    uint32_t message = runtime_message_processor_api.dequeue_message();
    if(message == 0)
    {
        return;
    }
    bool handled = false;
    if(message == 0x30f)
    {
        state->flags &= 0xfffbffff;
        runtime_message_processor_api.handle_message_30f();
        handled = true;
    }
    else if(message == 0x311)
    {
        state->flags |= 0x40000;
        runtime_message_processor_api.handle_message_311();
        handled = true;
    }
    if(handled && runtime_message_processor_api.query_state(nullptr, nullptr, nullptr) == 0)
    {
        RuntimeCommandBounds bounds{ 0, 0, state->width, state->height };
        runtime_message_processor_api.update_target(nullptr, &bounds, 1);
        runtime_message_processor_api.present();
    }
}

bool update_runtime_target(void *, RuntimeCommandBounds *bounds, int mode)
{
    if(mode == 1)
    {
        runtime_target_update_api.draw_bounds(bounds, 1);
        return true;
    }
    if(mode == 0x10000)
    {
        if((runtime_target_flags & 0x100000) == 0 && bounds->first == 0)
        {
            return runtime_target_update_api.begin_target(bounds->height, bounds->second, bounds->width) == 0;
        }
        return true;
    }
    if(mode == 0x20000 && (runtime_target_flags & 0x100000) == 0 && bounds->first == 0)
    {
        return runtime_target_update_api.end_target() == 0;
    }
    return true;
}

void enqueue_runtime_message(uint32_t message)
{
    if((graphics_host_flags & 0x400) == 0)
    {
        return;
    }
    runtime_queue_api.enter_queue_lock();
    if(message == 0x30f)
    {
        runtime_display_context.message_write_index = 0;
        runtime_display_context.message_read_index = 0;
    }
    uint32_t previous_index = runtime_display_context.message_write_index == 0 ? 0x1f : runtime_display_context.message_write_index - 1;
    if(runtime_display_context.message_write_index == runtime_display_context.message_read_index || runtime_display_context.message_queue[previous_index] != message)
    {
        runtime_display_context.message_available = 1;
        runtime_display_context.message_queue[runtime_display_context.message_write_index] = message;
        ++runtime_display_context.message_write_index;
        if(runtime_display_context.message_write_index == 0x20)
        {
            runtime_display_context.message_write_index = 0;
        }
        if(runtime_display_context.message_write_index == runtime_display_context.message_read_index)
        {
            ++runtime_display_context.message_read_index;
            if(runtime_display_context.message_read_index == 0x20)
            {
                runtime_display_context.message_read_index = 0;
            }
        }
    }
    runtime_queue_api.leave_queue_lock();
}

uint32_t dequeue_runtime_message()
{
    if(runtime_display_context.message_available == 0)
    {
        return 0;
    }
    uint32_t message = 0;
    runtime_queue_api.enter_queue_lock();
    if(runtime_display_context.message_write_index != runtime_display_context.message_read_index)
    {
        message = runtime_display_context.message_queue[runtime_display_context.message_read_index];
        ++runtime_display_context.message_read_index;
        if(runtime_display_context.message_read_index == 0x20)
        {
            runtime_display_context.message_read_index = 0;
        }
        if(runtime_display_context.message_write_index == runtime_display_context.message_read_index)
        {
            runtime_display_context.message_available = 0;
        }
    }
    runtime_queue_api.leave_queue_lock();
    return message;
}


} // namespace gag
