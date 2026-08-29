#include "runtime.h"
#include <chrono>
#include <mutex>
#include <thread>
#include "host_events.h"
#include "runtime_internal.h"

namespace freegag
{
void enable_runtime_subsystem()
{
    if((graphics_host_flags & RUNTIME_HOST_AUDIO_ENABLED) == 0)
    {
        graphics_host_flags |= RUNTIME_HOST_AUDIO_ENABLED;
        toggle_runtime_sound_state();
    }
}

void disable_runtime_subsystem()
{
    if((graphics_host_flags & RUNTIME_HOST_AUDIO_ENABLED) != 0)
    {
        graphics_host_flags &= ~RUNTIME_HOST_AUDIO_ENABLED;
        toggle_runtime_sound_state();
    }
}

void set_runtime_resource_variant(uint32_t value)
{
    if(script_runtime_root != nullptr)
        script_runtime_root->resource_variant = value;
}

void suspend_runtime_state()
{
    std::lock_guard lock(runtime_pointer_scene_mutex);
    if((graphics_host_flags & RUNTIME_HOST_SCENE_SWITCH_DEFERRED) == 0)
    {
        runtime_state_value = saved_runtime_state_value;
        switch_runtime_scene(nullptr);
        graphics_host_flags |= RUNTIME_HOST_SCENE_SWITCH_DEFERRED;
    }
}

void resume_runtime_state()
{
    std::lock_guard lock(runtime_pointer_scene_mutex);
    if((graphics_host_flags & RUNTIME_HOST_SCENE_TRANSITION_GUARDED) == 0 && (graphics_host_flags & RUNTIME_HOST_SCENE_SWITCH_DEFERRED) != 0)
    {
        graphics_host_flags &= ~RUNTIME_HOST_SCENE_SWITCH_DEFERRED;
        switch_runtime_scene(reinterpret_cast<void *>(runtime_state_value));
    }
}

void reset_runtime_input_queue()
{
    if((graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY)
    {
        enter_runtime_input_queue_lock();
        runtime_display_context.queued_input_write_index = 0;
        runtime_display_context.queued_input_read_index = 0;
        leave_runtime_input_queue_lock();
    }
}

void enqueue_runtime_byte(uint8_t value)
{
    if((graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY)
    {
        enter_runtime_byte_queue_lock();
        runtime_display_context.byte_available = 1;
        runtime_display_context.byte_queue[runtime_display_context.byte_write_index] = value;
        ++runtime_display_context.byte_write_index;
        if(runtime_display_context.byte_write_index == 0x20)
            runtime_display_context.byte_write_index = 0;
        if(runtime_display_context.byte_read_index == runtime_display_context.byte_write_index)
        {
            ++runtime_display_context.byte_read_index;
            if(runtime_display_context.byte_read_index == 0x20)
                runtime_display_context.byte_read_index = 0;
        }
        leave_runtime_byte_queue_lock();
    }
}

uint8_t dequeue_runtime_byte()
{
    if(runtime_display_context.byte_available == 0)
        return 0;
    uint8_t value = 0;
    if((graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY)
    {
        enter_runtime_byte_queue_lock();
        if(runtime_display_context.byte_read_index != runtime_display_context.byte_write_index)
        {
            value = runtime_display_context.byte_queue[runtime_display_context.byte_read_index];
            ++runtime_display_context.byte_read_index;
            if(runtime_display_context.byte_read_index == 0x20)
                runtime_display_context.byte_read_index = 0;
            if(runtime_display_context.byte_read_index == runtime_display_context.byte_write_index)
                runtime_display_context.byte_available = 0;
        }
        leave_runtime_byte_queue_lock();
    }
    return value;
}

void reset_runtime_byte_queue()
{
    if((graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY)
    {
        enter_runtime_byte_queue_lock();
        runtime_display_context.byte_write_index = 0;
        runtime_display_context.byte_read_index = 0;
        leave_runtime_byte_queue_lock();
    }
}

DisplaySceneNode *acquire_runtime_text_input_scene(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format)
{
    release_runtime_text_input_scene_guard();
    const intptr_t current_identifier = runtime_display_context.input_scene_identifier;
    if(current_identifier != 0 && begin_display_scene_update(current_identifier) == 0)
    {
        auto *current = reinterpret_cast<DisplaySceneNode *>(current_identifier);
        if(width <= static_cast<uint32_t>(current->width) && height <= static_cast<uint32_t>(current->height))
        {
            runtime_text_input_guarded_scene = current_identifier;
            flags |= DISPLAY_SCENE_PRESERVE_DIMENSIONS;
        }
        else
        {
            end_display_scene_update(current_identifier, nullptr, nullptr);
        }
    }

    DisplaySceneNode *scene = acquire_display_scene_node(index, x, y, width, height, flags, owner, descriptor, format);
    if(scene == nullptr)
        release_runtime_text_input_scene_guard();
    return scene;
}

uint32_t begin_runtime_text_input_scene_update(intptr_t identifier)
{
    const uint32_t result = begin_display_scene_update(identifier);
    if(result != 0)
        release_runtime_text_input_scene_guard();
    return result;
}

uint32_t end_runtime_text_input_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle)
{
    const uint32_t result = end_display_scene_update(identifier, transform, rectangle);
    release_runtime_text_input_scene_guard();
    return result;
}

void process_runtime_text_input(RuntimeCommandLoopState *state)
{
    if((state->flags & RUNTIME_COMMAND_TEXT_INPUT_ACTIVE) == 0)
        return;

    bool changed = false;
    uint8_t value = dequeue_runtime_byte();
    uint32_t cursor = state->input_cursor;
    if(state->input_end - cursor == 1 || value == 0x0d)
    {
        state->input_text[cursor] = '\0';
        ++state->input_cursor;
        state->flags &= ~RUNTIME_COMMAND_TEXT_INPUT_ACTIVE;
        if(state->input_scene_identifier != 0)
        {
            release_display_scene_node(state->input_scene_identifier, reinterpret_cast<intptr_t>(state));
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
                uint32_t letter_mode = state->input_text_flags & RUNTIME_TEXT_INPUT_LETTER_CASE_MASK;
                if(letter_mode == RUNTIME_TEXT_INPUT_LOWERCASE)
                    value |= 0x20;
                else if(letter_mode == RUNTIME_TEXT_INPUT_UPPERCASE)
                    value &= 0xdf;
            }
            state->input_text[cursor] = static_cast<char>(value);
            cursor = state->input_cursor++;
            state->input_text[cursor + 1] = '\0';
            changed = true;
        }
    }

    if(state->input_scene_identifier == 0)
        return;

    uint32_t current_tick = runtime_milliseconds();
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
        state->input_caret_tick = current_tick;

    RuntimeStandaloneTextState &text_state = runtime_display_context.input_text_state;
    if(changed
        && initialize_runtime_standalone_text(runtime_display_context.input_text, text_state.x, text_state.y, text_state.font_identity, text_state.low_color, text_state.high_color, &text_state) != 0)
    {
        DisplaySceneDescriptor descriptor;
        runtime_display_context.input_scene_identifier = reinterpret_cast<intptr_t>(acquire_runtime_text_input_scene(runtime_display_context.input_scene_index, 0, 0, text_state.bounds[2],
            text_state.bounds[3], 0x120000, reinterpret_cast<intptr_t>(&runtime_display_context), &descriptor, nullptr));
        if(begin_runtime_text_input_scene_update(runtime_display_context.input_scene_identifier) == 0)
        {
            draw_runtime_standalone_text(&text_state, &descriptor);
            const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
            end_runtime_text_input_scene_update(runtime_display_context.input_scene_identifier, &transform, &text_state.bounds_rectangle);
        }
    }
}

void enqueue_runtime_input(RuntimeQueuedInputType type, uint32_t packed_position)
{
    bool input_enabled = (graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY;
    // A borderless transition temporarily clears the queue-enable bit while the capture child remains interactive. Retain the physical release, but not resize-generated moves that would
    // change the active game region before that release is applied.
    if(desktop_presentation_state.fullscreen && (graphics_host_flags & RUNTIME_HOST_MESSAGE_QUEUE_ENABLED) != 0 && type == RuntimeQueuedInputType::LEFT_BUTTON_UP)
        input_enabled = true;
    if(input_enabled)
    {
        enter_runtime_input_queue_lock();
        runtime_display_context.queued_input_available = 1;
        runtime_display_context.queued_inputs[runtime_display_context.queued_input_write_index] = { type, packed_position };
        ++runtime_display_context.queued_input_write_index;
        if(runtime_display_context.queued_input_write_index == 0x20)
            runtime_display_context.queued_input_write_index = 0;
        if(runtime_display_context.queued_input_read_index == runtime_display_context.queued_input_write_index)
        {
            ++runtime_display_context.queued_input_read_index;
            if(runtime_display_context.queued_input_read_index == 0x20)
                runtime_display_context.queued_input_read_index = 0;
        }
        leave_runtime_input_queue_lock();
    }
}

bool dequeue_runtime_input(RuntimeQueuedInput *input)
{
    if(runtime_display_context.queued_input_available == 0)
        return false;
    bool dequeued = false;
    if((graphics_host_flags & RUNTIME_HOST_INPUT_READY) == RUNTIME_HOST_INPUT_READY)
    {
        enter_runtime_input_queue_lock();
        if(runtime_display_context.queued_input_read_index != runtime_display_context.queued_input_write_index)
        {
            *input = runtime_display_context.queued_inputs[runtime_display_context.queued_input_read_index];
            ++runtime_display_context.queued_input_read_index;
            if(runtime_display_context.queued_input_read_index == 0x20)
                runtime_display_context.queued_input_read_index = 0;
            if(runtime_display_context.queued_input_read_index == runtime_display_context.queued_input_write_index)
                runtime_display_context.queued_input_available = 0;
            dequeued = true;
        }
        leave_runtime_input_queue_lock();
    }
    return dequeued;
}

bool synchronize_runtime_plan_mode()
{
    bool changed = false;
    if((runtime_display_context.flags & RUNTIME_HOST_CREDITS_ACTIVE) == 0)
    {
        if((runtime_display_context.flags & RUNTIME_HOST_PLAN_MODE) != 0)
        {
            changed = clear_runtime_plans_inactive();
            if(changed)
                rebuild_runtime_pointer_resources();
            runtime_display_context.flags &= ~RUNTIME_HOST_PLAN_MODE;
        }
    }
    else if((runtime_display_context.flags & RUNTIME_HOST_PLAN_MODE) == 0)
    {
        changed = set_runtime_plans_inactive();
        if(changed)
            rebuild_runtime_pointer_resources();
        runtime_display_context.flags |= RUNTIME_HOST_PLAN_MODE;
    }
    return changed;
}

bool process_pending_runtime_tree_switch(RuntimeTreeNode *node)
{
    bool changed = false;
    if((runtime_display_context.flags & RUNTIME_HOST_TREE_SWITCH_PENDING) != 0)
    {
        runtime_display_context.accumulated_tree_flags = 0;
        destroy_runtime_tree_resources(node);
        RuntimeTreeNode *activated = activate_runtime_tree_with_notifications(runtime_display_context.first_runtime_path, runtime_display_context.second_runtime_path, nullptr, nullptr);
        if(activated == nullptr)
        {
            rebuild_runtime_tree_resources(node);
        }
        else
        {
            changed = node->identity != activated;
            if(!changed)
                reset_runtime_tree_parser_contexts(activated);
            rebuild_runtime_tree_resources(activated);
            activated->flags |= runtime_display_context.accumulated_tree_flags;
        }
        update_runtime_pointer_region(runtime_scene_x, runtime_scene_y);
        runtime_display_context.flags &= ~RUNTIME_HOST_TREE_SWITCH_PENDING;
    }
    return changed;
}



RuntimeTreeNode *activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *parent_selector, void *creation_context)
{
    RuntimeGenericResourceNode *resource = find_or_load_runtime_generic_resource(resource_name);
    RuntimeTreeNode *node = create_runtime_tree_node(resource, parent_selector, tree_name, creation_context);
    if(node != nullptr)
    {
        if((node->flags & RUNTIME_TREE_INVENTORY_PACK) != 0 || node->name[0] == 0)
        {
            set_script_runtime_flags(SCRIPT_RUNTIME_INVENTORY_OPEN, 0);
            set_script_runtime_flags(SCRIPT_RUNTIME_INVENTORY_CLOSE, 0);
        }
        if((node->flags & RUNTIME_TREE_COMMENT) != 0)
            activate_runtime_tree_node_comment(node);
    }
    return node;
}


uint32_t process_runtime_queued_input()
{
    RuntimeQueuedInput input;
    if(dequeue_runtime_input(&input) && (runtime_display_context.flags & RUNTIME_HOST_PROPERTY_STATE_ACTIVE) == 0)
    {
        switch(input.type)
        {
        case RuntimeQueuedInputType::POINTER_MOVE:
            return update_runtime_pointer_region(static_cast<int32_t>(input.packed_position & 0xffff), static_cast<int32_t>(input.packed_position >> 16));
        case RuntimeQueuedInputType::LEFT_BUTTON_DOWN:
            return handle_runtime_right_button_down();
        case RuntimeQueuedInputType::LEFT_BUTTON_UP:
            return handle_runtime_left_button_up();
        case RuntimeQueuedInputType::RIGHT_BUTTON_DOWN:
            return handle_runtime_left_button_down();
        }
    }
    return 0;
}



RuntimeInputText take_runtime_input_text()
{
    RuntimeInputText input;
    std::memcpy(input.data(), runtime_display_context.input_text, input.size());
    runtime_display_context.input_cursor = 0;
    return input;
}

void initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, uint32_t character_width, void *session_value)
{
    if(runtime_display_context.input_scene_identifier != 0)
        return;

    reset_runtime_byte_queue();
    runtime_display_context.input_text[1] = '\0';
    runtime_display_context.input_cursor = 0;
    runtime_display_context.input_text_flags = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(session_value));
    runtime_display_context.input_text[0] = '-';
    runtime_display_context.input_end = character_width;
    if(runtime_display_context.input_end == 0)
        runtime_display_context.input_end = 0x20;
    runtime_display_context.input_caret_tick = runtime_milliseconds();

    RuntimeLockRecord *record = acquire_runtime_lock_record(selector);
    void *font_identity = nullptr;
    if(record != nullptr)
        font_identity = record->identity_context;
    if(initialize_runtime_standalone_text(runtime_display_context.input_text, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(first)), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(second)),
           font_identity, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fourth)), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fifth)), &runtime_display_context.input_text_state)
        != 0)
    {
        runtime_display_context.input_scene_index = find_available_display_scene_index(0x80000);
        intptr_t scene_identifier;
        if((record->flags & SCRIPT_IMAGE_NO_PALETTE) != 0)
            scene_identifier = runtime_display_context.input_alternate_scene_identifier;
        else
            scene_identifier = record->scene_identifier;
        DisplaySceneNode *locked_scene = lock_display_scene_node(scene_identifier);
        if(locked_scene != nullptr)
        {
            DisplaySceneDescriptor descriptor;
            runtime_display_context.input_scene_identifier = reinterpret_cast<intptr_t>(acquire_display_scene_node(runtime_display_context.input_scene_index,
                static_cast<int32_t>(reinterpret_cast<uintptr_t>(first)), static_cast<int32_t>(reinterpret_cast<uintptr_t>(second)), runtime_display_context.input_text_state.bounds[2],
                runtime_display_context.input_text_state.bounds[3], 0x20000, reinterpret_cast<intptr_t>(&runtime_display_context), &descriptor, &default_display_pixel_format));
            if(begin_display_scene_update(runtime_display_context.input_scene_identifier) == 0)
            {
                draw_runtime_standalone_text(&runtime_display_context.input_text_state, &descriptor);
                const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
                end_display_scene_update(runtime_display_context.input_scene_identifier, &transform, &runtime_display_context.input_text_state.bounds_rectangle);
            }
        }
        unlock_display_scene_node(scene_identifier);
    }
    if(record != nullptr)
        release_runtime_lock_record(record);
    runtime_display_context.flags |= RUNTIME_HOST_TEXT_INPUT_COMMITTED;
}

int run_runtime_command_loop(RuntimeCommandLoopState *state)
{
    uint32_t initial_flags = state->flags;
    if((initial_flags & (RUNTIME_COMMAND_LOOP_RUNNING | RUNTIME_COMMAND_RESUME_REQUESTED | RUNTIME_COMMAND_LOOP_STOP_REQUESTED)) == 0)
        return 0;
    const uint32_t restore_flag = initial_flags & RUNTIME_COMMAND_SCRIPT_EXECUTION_ENABLED;
    state->flags = (initial_flags & ~RUNTIME_COMMAND_SCRIPT_EXECUTION_ENABLED) | RUNTIME_COMMAND_LOOP_RUNNING;
    pause_runtime_game_dll();
    runtime_animation_control_flags |= RUNTIME_ANIMATION_PAUSED;
    pause_runtime_sound_output(0);
    post_application_event(HostApplicationCommand::COMMAND_COMPLETED);
    while(true)
    {
        drain_runtime_resource_destructions();
        if((state->flags & RUNTIME_COMMAND_RESUME_REQUESTED) != 0)
            graphics_host_flags &= ~RUNTIME_HOST_RESUME_PENDING;
        if((state->flags & RUNTIME_COMMAND_LOOP_STOP_REQUESTED) != 0)
        {
            reset_runtime_session();
            resume_runtime_sound_output();
            runtime_animation_control_flags &= ~RUNTIME_ANIMATION_PAUSED;
            state->flags &= ~(RUNTIME_COMMAND_LOOP_RUNNING | RUNTIME_COMMAND_LOOP_STOP_REQUESTED);
            post_application_event(HostApplicationCommand::RUNTIME_SHUTDOWN);
            return 1;
        }
        runtime_sleep(100);
        if((state->flags & RUNTIME_COMMAND_LOOP_RUNNING) == 0)
        {
            state->flags |= restore_flag;
            resume_runtime_sound_output();
            runtime_animation_control_flags &= ~RUNTIME_ANIMATION_PAUSED;
            resume_runtime_game_dll();
            return 1;
        }
    }
}

uint32_t run_pending_runtime_external_command()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_EXTERNAL_COMMAND_PENDING) != 0)
    {
        runtime_display_context.external_command_pending = 0;
        runtime_scene_control_flags &= ~RUNTIME_HOST_EXTERNAL_COMMAND_PENDING;
    }
    return 0;
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
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
            }
        }
        if(target_flags == 0)
            state->accumulated_tree_flags = target->flags & RUNTIME_TREE_LOW_FLAG_MASK;
        else
            state->accumulated_tree_flags = target_flags | RUNTIME_TREE_EXPLICIT_FLAGS;
        if(target->parent == nullptr)
        {
            if(state->current_runtime_resource != nullptr && activate_default_comment_scene("m_DEF_LOAD") > 0)
                wait_for_display_scene_ready(500);
            destroy_runtime_tree_resources(target);
            RuntimeTreeNode *previous = target->previous;
            if(previous == nullptr)
            {
                rebuild_runtime_tree_resources(target);
                refresh_runtime_pointer_region();
                return RuntimeScriptOpcodeDisposition::FINISH_LINK;
            }
            auto *deactivated = reinterpret_cast<RuntimeTreeNode *>(static_cast<uintptr_t>(deactivate_runtime_tree_and_visuals(target, previous)));
            if((previous->flags & RUNTIME_TREE_SECTION_FALLBACK_ENABLED) == 0 || deactivated == previous)
                reset_runtime_tree_parser_contexts(deactivated);
            else
                opcode = 0x70000000;
            if(opcode != 0xb0000000)
            {
                rebuild_runtime_tree_resources(deactivated);
                deactivated = static_cast<RuntimeTreeNode *>(state->runtime_tree_identity);
            }
            state->runtime_tree_identity = deactivated;
            refresh_runtime_pointer_region();
            // Replacing the root may release the current link and parser, so skip the common tail.
            return RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR;
        }

        destroy_runtime_tree_resources(target);
        const bool root_level = target->parent == reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        std::memcpy(first, target->name, sizeof(first));
        std::memcpy(typed_value, parser->owner->name, 0x20);
        if(root_level || (reset_runtime_tree_parser_contexts(tree), root_level) || find_runtime_tree_descendant_identity_by_name(tree, first) != nullptr)
            deactivate_runtime_tree_and_visuals(target, nullptr);
        void *continuation;
        if(root_level)
            continuation = find_runtime_tree_root_identity_by_name(typed_value);
        else
            continuation = find_runtime_tree_descendant_identity_by_name(tree, typed_value);
        if(continuation == nullptr)
            state->active_script_link = nullptr;
        rebuild_runtime_pointer_resources();
        refresh_runtime_pointer_region();
        // A missing continuation may deactivate and release the current link, so skip the common tail.
        return continuation == nullptr ? RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR : RuntimeScriptOpcodeDisposition::COMPLETE;
    }

    case 0xa0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary == nullptr)
            {
                if(find_runtime_tree_identity_by_name_recursive(nullptr, first) != nullptr)
                    return RuntimeScriptOpcodeDisposition::PAUSE;
            }
            else
            {
                const uint32_t flags = query_runtime_resource_playback_flags(primary->resource_identity);
                if(flags != 0 && (flags & (RUNTIME_MEDIA_INITIALIZING | RUNTIME_RESOURCE_TYPE_BITMAP)) == 0
                    && ((flags & RUNTIME_RESOURCE_TYPE_ANIMATION) == 0 || (flags & RUNTIME_RESOURCE_PRIMARY) == 0 || (flags & RUNTIME_RESOURCE_INDEPENDENT_SCENE) != 0)
                    && ((flags & RUNTIME_RESOURCE_LOOP) == 0 || query_runtime_resource_frame_limit(primary->resource_identity) != RUNTIME_RESOURCE_FRAME_LIMIT_UNBOUNDED))
                {
                    return RuntimeScriptOpcodeDisposition::PAUSE;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xf0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary == nullptr)
            {
                if(find_runtime_tree_identity_by_name_recursive(nullptr, first) == nullptr)
                    return RuntimeScriptOpcodeDisposition::PAUSE;
            }
            else
            {
                const uint32_t flags = query_runtime_resource_playback_flags(primary->resource_identity);
                if(flags != 0 && (flags & (RUNTIME_MEDIA_INITIALIZING | RUNTIME_RESOURCE_TYPE_BITMAP | RUNTIME_RESOURCE_PRIMARY)) == 0 && (flags & RUNTIME_RESOURCE_TYPE_ANIMATION) != 0)
                    return RuntimeScriptOpcodeDisposition::PAUSE;
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x90000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            uint32_t flags = 0;
            uint32_t parsed_flag;
            while((parsed_flag = parse_image_flag(parser)) != SCRIPT_PARSE_END)
                flags |= parsed_flag;
            if(primary != nullptr)
                set_runtime_resource_state(primary->resource_identity, flags);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x80000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
        {
            parse_script_typed_value(parser, typed_value, &value_type);
            if(value_type != SCRIPT_VALUE_TYPE_INVALID)
                resolve_state_field_reference(first, second, typed_value, static_cast<ScriptValueType>(value_type));
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xd0000000:
    case 0xc0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            if(opcode == 0xd0000000)
                rotate_runtime_named_node_cursor_next(first, 1);
            else
                rotate_runtime_named_node_cursor_previous(first, 1);
            rebuild_runtime_pointer_resources();
            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xe0000000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreeLink84 *zone = find_global_runtime_tree_link_0084_by_name(first);
            if(zone != nullptr)
            {
                const int32_t x = parse_script_integer_expression(parser);
                const int32_t y = parse_script_integer_expression(parser);
                if(x != SCRIPT_INTEGER_INVALID && y != SCRIPT_INTEGER_INVALID)
                {
                    update_runtime_tree_link_0084(tree, zone, x, y, zone->width - zone->x + x, zone->height - zone->y + y, 0, nullptr, nullptr, 0, 0, SCRIPT_INTEGER_INVALID);
                    rebuild_runtime_pointer_resources();
                    update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                }
                else if(parse_script_value_token(parser, second, sizeof(second)) == SCRIPT_PARSE_END)
                {
                    zone->movement_flags |= 2;
                }
                else
                {
                    RuntimeTreeLink8C *path = find_global_runtime_tree_link_008c_by_name(second);
                    if(path != nullptr)
                    {
                        if((zone->movement_flags & RUNTIME_MOVEMENT_ACTIVE) == 0)
                        {
                            zone->movement_deadline = state->script_clock;
                            zone->movement_flags = (zone->movement_flags & ~RUNTIME_MOVEMENT_STOPPED) | RUNTIME_MOVEMENT_ACTIVE;
                        }
                        if(state->script_clock < zone->movement_deadline)
                            return RuntimeScriptOpcodeDisposition::PAUSE;
                        if((zone->movement_flags & RUNTIME_MOVEMENT_STOPPED) == 0 && path->x <= zone->x && path->y <= zone->y && zone->width <= path->width && zone->height <= path->height)
                        {
                            if((path->flags & RUNTIME_PATH_HAS_LINE_DELTA) != 0)
                            {
                                update_runtime_tree_link_0084(tree->identity, zone->identity, zone->x + path->line_first, zone->y + path->line_second, zone->width + path->line_first,
                                    zone->height + path->line_second, 0, nullptr, nullptr, 0, 0, SCRIPT_INTEGER_INVALID);
                                zone->movement_deadline = state->script_clock + path->time;
                            }
                            rebuild_runtime_pointer_resources();
                            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                            return RuntimeScriptOpcodeDisposition::PAUSE;
                        }
                    }
                    zone->movement_flags &= ~RUNTIME_MOVEMENT_MASK;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 1:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                primary->flags &= ~RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x100:
        if((link->owner_flags & RUNTIME_SCRIPT_LINK_WAIT_PENDING) == 0)
        {
            if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
            {
                uintptr_t selection = static_cast<uint32_t>(parse_script_integer_expression(parser));
                char selection_name[0x20];
                if(selection == SCRIPT_INTEGER_INVALID)
                {
                    if(parse_script_value_token(parser, selection_name, sizeof(selection_name)) == SCRIPT_PARSE_END)
                        return RuntimeScriptOpcodeDisposition::COMPLETE;
                    selection = reinterpret_cast<uintptr_t>(selection_name);
                }
                else
                {
                    selection &= 0xffff;
                }
                RuntimeTreeSecondaryResourceLink *secondary = find_global_runtime_tree_secondary_resource_link_by_name(first);
                RuntimeFixedNameListNode *fixed = find_runtime_fixed_name_list_node(second);
                if((script_runtime_root->flags & SCRIPT_RUNTIME_COMMENTS_SUPPRESSED) == 0 && secondary != nullptr && fixed != nullptr)
                {
                    link->backend_child = attach_runtime_generic_backend_child(nullptr, fixed->resource_identity, secondary->resource_identity, selection, 0);
                    if(link->backend_child != nullptr)
                    {
                        link->secondary_resource_identity = secondary->resource_identity;
                        link->fixed_resource_identity = fixed->resource_identity;
                        link->owner_flags |= RUNTIME_SCRIPT_LINK_WAIT_PENDING | RUNTIME_SCRIPT_LINK_BACKEND_CHILD_ACTIVE;
                        return RuntimeScriptOpcodeDisposition::PAUSE;
                    }
                }
            }
        }
        else
        {
            const uint32_t flags = get_runtime_generic_backend_child_flags(link->backend_child);
            if(flags != RUNTIME_GENERIC_CHILD_FLAGS_UNAVAILABLE && (flags & RUNTIME_GENERIC_CHILD_MODE_200) != 0)
                return RuntimeScriptOpcodeDisposition::PAUSE;
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x200:
        if((state->flags & RUNTIME_COMMAND_TEXT_INPUT_ACTIVE) != 0)
            return RuntimeScriptOpcodeDisposition::PAUSE;
        if(state->input_cursor == 0)
        {
            void *first_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            void *second_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            parse_script_value_token(parser, first, sizeof(first));
            void *fourth_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            void *fifth_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_script_integer_expression(parser)));
            if(parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
            {
                uint32_t character_width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(character_width == SCRIPT_INTEGER_INVALID)
                    character_width = 0;
                void *session_value = reinterpret_cast<void *>(static_cast<uintptr_t>(parse_image_flag(parser)));
                RuntimeFixedNameListNode *fixed = find_runtime_fixed_name_list_node(first);
                initialize_runtime_input_session(first_value, second_value, fixed == nullptr ? nullptr : fixed->resource_identity, fourth_value, fifth_value, character_width, session_value);
                return RuntimeScriptOpcodeDisposition::PAUSE;
            }
        }
        else
        {
            parse_script_integer_expression(parser);
            parse_script_integer_expression(parser);
            parse_script_value_token(parser, first, sizeof(first));
            parse_script_integer_expression(parser);
            parse_script_integer_expression(parser);
            if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
            {
                RuntimeInputText input = take_runtime_input_text();
                resolve_state_field_reference(first, second, input.data(), SCRIPT_VALUE_TYPE_STRING);
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x300:
    {
        state->external_command_pending = 1;
        uint32_t result = 0;
        const int32_t command = parse_script_integer_expression(parser);
        if(command != SCRIPT_INTEGER_INVALID)
        {
            HostApplicationPayload payload;
            if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
            {
                if(parse_script_value_token(parser, second, sizeof(second)) == SCRIPT_PARSE_END || !has_script_object_field(first, second))
                {
                    state->external_command_pending = 0;
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
                }
                payload = HostStateFieldQuery{ first, second };
            }
            if(should_send_runtime_script_message(command))
            {
                HostEventResult event_result = send_application_event(static_cast<uint32_t>(command), std::move(payload));
                if(const auto *value = std::get_if<uint32_t>(&event_result))
                    result = *value;
            }
        }
        if(result != 0)
        {
            result = 0;
            while(state->external_command_pending != 0)
            {
                result |= static_cast<uint32_t>(run_runtime_command_loop(state));
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        state->external_command_pending = 0;
        // after this /MESSAGE rather than issue it again.
        return result == 0 ? RuntimeScriptOpcodeDisposition::COMPLETE : RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR;
    }

    case 0x400:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
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
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
                identifier = state->input_alternate_scene_identifier;
                x = parse_script_integer_expression(parser);
                y = parse_script_integer_expression(parser);
                width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                height = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(x == SCRIPT_INTEGER_INVALID)
                    x = 0;
                if(y == SCRIPT_INTEGER_INVALID)
                    y = 0;
                if(width == SCRIPT_INTEGER_INVALID)
                    width = runtime_display_context.width;
                if(height == SCRIPT_INTEGER_INVALID)
                    height = runtime_display_context.height;
            }
            else
            {
                identifier = scene->scene_identifier;
                x = parse_script_integer_expression(parser);
                y = parse_script_integer_expression(parser);
                width = static_cast<uint32_t>(parse_script_integer_expression(parser));
                height = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(x == SCRIPT_INTEGER_INVALID)
                    x = 0;
                if(y == SCRIPT_INTEGER_INVALID)
                    y = 0;
                if(width == SCRIPT_INTEGER_INVALID)
                    width = scene->width;
                if(height == SCRIPT_INTEGER_INVALID)
                    height = scene->height;
            }
            update_runtime_resource_scene_region(identifier, x, y, width, height);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x500:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            clear_runtime_named_node_children(first);
            rebuild_runtime_pointer_resources();
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x600:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            const uint32_t fade_flag = parse_image_flag(parser);
            if(static_cast<int32_t>(fade_flag) > 0)
            {
                const int32_t duration = parse_script_integer_expression(parser);
                RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
                if(duration != SCRIPT_INTEGER_INVALID && primary != nullptr)
                {
                    RuntimeLockRecord *record = acquire_runtime_lock_record(primary->resource_identity);
                    if(record != nullptr)
                    {
                        if((record->type_flags & RUNTIME_RESOURCE_TYPE_SOUND) != 0)
                        {
                            if(fade_flag == SCRIPT_BOOLEAN_TRUE)
                            {
                                set_runtime_sound_volume(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), 0);
                                fade_out_runtime_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), duration, 1);
                            }
                            else if(fade_flag == SCRIPT_BOOLEAN_FALSE)
                            {
                                fade_in_runtime_sound(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(record->identity_context)), duration, 1);
                            }
                        }
                        release_runtime_lock_record(record);
                    }
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x900:
    case 0xa00:
    case 0xb00:
        // The later executables recognize these opcodes but contain no execution branches for them.
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xc00:
        if(!evaluate_runtime_condition_by_identity(link->identity))
            scan_runtime_tree_link_007c_control_boundary(link, 0x6000);
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x1000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                primary->flags |= RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x3000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
        {
            const int32_t delta = parse_script_integer_expression(parser);
            if(delta != SCRIPT_INTEGER_INVALID)
                add_script_object_integer(first, second, delta);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x4000:
        if(parse_script_value_token(parser, first, sizeof(first)) == SCRIPT_PARSE_END || parse_script_value_token(parser, second, sizeof(second)) == SCRIPT_PARSE_END)
            return RuntimeScriptOpcodeDisposition::COMPLETE;
        [[fallthrough]];

    case 0x40000:
    case 0x50000:
        for(;;)
        {
            const uint32_t control_opcode = parse_script_opcode(parser);
            if(control_opcode == SCRIPT_PARSE_END)
                return RuntimeScriptOpcodeDisposition::COMPLETE;
            if(control_opcode <= 0x6000)
            {
                if(control_opcode == 0x6000)
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
                if(control_opcode == 0x5000)
                {
                    parse_script_typed_value(parser, typed_value, &value_type);
                    if((value_type != SCRIPT_VALUE_TYPE_INVALID && compare_script_object_field(first, second, typed_value, static_cast<int32_t>(value_type)))
                        || scan_runtime_tree_link_007c_control_boundary(link, 0x60000) == 0x6000)
                    {
                        return RuntimeScriptOpcodeDisposition::COMPLETE;
                    }
                }
                continue;
            }
            uint32_t boundary = SCRIPT_PARSE_END;
            if(control_opcode == 0x70000)
            {
                const int32_t minimum = parse_script_integer_expression(parser);
                const int32_t maximum = parse_script_integer_expression(parser);
                if(minimum != SCRIPT_INTEGER_INVALID && maximum != SCRIPT_INTEGER_INVALID && minimum <= random_value && random_value <= maximum)
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
                boundary = scan_runtime_tree_link_007c_control_boundary(link, 0x60000);
            }
            else if(control_opcode == 0x80000)
            {
                if(parse_script_value_token(parser, reinterpret_cast<char *>(typed_value), 0x20) != SCRIPT_PARSE_END && script_object_container_state_matches_by_name(typed_value))
                    return RuntimeScriptOpcodeDisposition::COMPLETE;
                boundary = scan_runtime_tree_link_007c_control_boundary(link, 0x60000);
            }
            if(boundary == 0x6000)
                return RuntimeScriptOpcodeDisposition::COMPLETE;
        }

    case 0xa000:
        if((link->owner_flags & RUNTIME_SCRIPT_LINK_SCENE_UPDATE_OPEN) == 0)
        {
            link->owner_flags |= RUNTIME_SCRIPT_LINK_SCENE_UPDATE_OPEN;
            begin_display_scene_update(state->input_alternate_scene_identifier);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xb000:
        if((link->owner_flags & RUNTIME_SCRIPT_LINK_SCENE_UPDATE_OPEN) != 0)
        {
            link->owner_flags &= ~RUNTIME_SCRIPT_LINK_SCENE_UPDATE_OPEN;
            end_display_scene_update(state->input_alternate_scene_identifier, nullptr, nullptr);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xc000:
        if((state->flags & RUNTIME_COMMAND_GAME_BUSY) != 0)
            return RuntimeScriptOpcodeDisposition::PAUSE;
        if((state->flags & RUNTIME_COMMAND_GAME_RESULT_READY) == 0)
        {
            if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && load_and_initialize_runtime_game_dll(first))
                return RuntimeScriptOpcodeDisposition::PAUSE;
        }
        else
        {
            state->flags &= ~RUNTIME_COMMAND_GAME_RESULT_READY;
            parse_script_value_token(parser, first, sizeof(first));
            if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
            {
                resolve_state_field_reference(first, second, state->game_result_data, static_cast<ScriptValueType>(state->game_result_type));
                std::memset(state->game_result_data, 0, sizeof(state->game_result_data));
                state->game_result_type = 0;
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xd000:
        stop_runtime_game_dll();
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xe000:
        if(state->nested_runtime_state_count == 0)
        {
            state->flags |= RUNTIME_COMMAND_NESTED_STATE_ACTIVE;
            suspend_runtime_state();
        }
        ++state->nested_runtime_state_count;
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xf000:
        if(state->nested_runtime_state_count == 1)
        {
            state->flags &= ~RUNTIME_COMMAND_NESTED_STATE_ACTIVE;
            resume_runtime_state();
        }
        if(state->nested_runtime_state_count != 0)
            --state->nested_runtime_state_count;
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x7000:
    case 0x8000:
    case 0x60000000:
    {
        void *parent_identity;
        if(opcode == 0x7000)
            parent_identity = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        else if(opcode == 0x8000)
            parent_identity = tree;
        else
            parent_identity = parser->owner;
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            if(parse_script_value_token(parser, second, sizeof(second)) == SCRIPT_PARSE_END)
            {
                std::memcpy(second, first, sizeof(second));
                std::memcpy(first, parser->resource->name, sizeof(first));
            }
            RuntimeTreeNode *activated = activate_runtime_tree_with_notifications(first, second, parent_identity, nullptr);
            if(activated != nullptr)
                rebuild_runtime_tree_resources(activated);
            refresh_runtime_pointer_region();
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;
    }

    case 0x9000:
        if(parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
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
                    destination = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(destination_scene->scene_identifier));
                else if(strings_equal(second, "BACKGND"))
                    destination = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(state->input_alternate_scene_identifier));
            }
            if(destination != nullptr)
            {
                int32_t destination_x = parse_script_integer_expression(parser);
                int32_t destination_y = parse_script_integer_expression(parser);
                if(destination_x == SCRIPT_INTEGER_INVALID)
                    destination_x = 0;
                if(destination_y == SCRIPT_INTEGER_INVALID)
                    destination_y = 0;
                if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
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
                            if(rectangle.left == SCRIPT_INTEGER_INVALID)
                                rectangle.left = 0;
                            if(rectangle.top == SCRIPT_INTEGER_INVALID)
                                rectangle.top = 0;
                            if(rectangle.right == SCRIPT_INTEGER_INVALID)
                                rectangle.right = static_cast<int32_t>(resource->output_width);
                            if(rectangle.bottom == SCRIPT_INTEGER_INVALID)
                                rectangle.bottom = static_cast<int32_t>(resource->output_height);
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
                            if(rectangle.left == SCRIPT_INTEGER_INVALID)
                                rectangle.left = 0;
                            if(rectangle.top == SCRIPT_INTEGER_INVALID)
                                rectangle.top = 0;
                            if(rectangle.right == SCRIPT_INTEGER_INVALID)
                                rectangle.right = static_cast<int32_t>(source_scene->width);
                            if(rectangle.bottom == SCRIPT_INTEGER_INVALID)
                                rectangle.bottom = static_cast<int32_t>(source_scene->height);
                        }
                        else if(strings_equal(second, "BACKGND"))
                        {
                            source = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(state->input_alternate_scene_identifier));
                            rectangle.left = parse_script_integer_expression(parser);
                            rectangle.top = parse_script_integer_expression(parser);
                            rectangle.right = parse_script_integer_expression(parser);
                            rectangle.bottom = parse_script_integer_expression(parser);
                            if(rectangle.left == SCRIPT_INTEGER_INVALID)
                                rectangle.left = 0;
                            if(rectangle.top == SCRIPT_INTEGER_INVALID)
                                rectangle.top = 0;
                            if(rectangle.right == SCRIPT_INTEGER_INVALID)
                                rectangle.right = state->width;
                            if(rectangle.bottom == SCRIPT_INTEGER_INVALID)
                                rectangle.bottom = state->height;
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
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x10000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
                set_runtime_resource_loop_count(primary->resource_identity, 1);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x20000:
    case 0x30000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
        {
            if(opcode == 0x20000)
                add_script_object_to_runtime_named_node(first, second);
            else
                remove_script_object_from_runtime_named_node(first, second);
            rebuild_runtime_pointer_resources();
            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xc0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
                set_runtime_resource_state(primary->resource_identity, 1);
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xb0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            seek_runtime_tree_link_007c_label(link, first);
            return RuntimeScriptOpcodeDisposition::COMMIT_CURSOR;
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xd0000:
        reset_runtime_session();
        send_application_event(HostApplicationCommand::CLOSE_REQUESTED);
        state->flags &= ~RUNTIME_COMMAND_SCRIPT_EXECUTION_ENABLED;
        // A session reset invalidates the old parser cursor, so do not restore it.
        return RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR;

    case 0xe0000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END && parse_script_value_token(parser, second, sizeof(second)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                update_runtime_tree_primary_resource_link(tree, primary, second, 0, 0, 0);
                rebuild_runtime_pointer_resources();
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x90000:
        if(parse_script_value_token(parser, first, sizeof(first)) != SCRIPT_PARSE_END)
        {
            RuntimeTreePrimaryResourceLink *primary = find_global_runtime_tree_primary_resource_link_by_name(first);
            if(primary != nullptr)
            {
                const int32_t x = parse_script_integer_expression(parser);
                const int32_t y = parse_script_integer_expression(parser);
                if(x != SCRIPT_INTEGER_INVALID && y != SCRIPT_INTEGER_INVALID)
                {
                    update_runtime_tree_primary_resource_link(tree, primary, nullptr, x - primary->x, y - primary->y, 0);
                    rebuild_runtime_pointer_resources();
                    update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                }
                else if(parse_script_value_token(parser, second, sizeof(second)) == SCRIPT_PARSE_END)
                {
                    primary->flags |= RUNTIME_MOVEMENT_STOPPED;
                }
                else
                {
                    RuntimeTreeLink8C *path = find_global_runtime_tree_link_008c_by_name(second);
                    if(path != nullptr)
                    {
                        if((primary->flags & RUNTIME_MOVEMENT_ACTIVE) == 0)
                        {
                            primary->movement_deadline = state->script_clock;
                            primary->flags = (primary->flags & ~RUNTIME_MOVEMENT_STOPPED) | RUNTIME_MOVEMENT_ACTIVE;
                        }
                        if(state->script_clock < primary->movement_deadline)
                            return RuntimeScriptOpcodeDisposition::PAUSE;
                        if((primary->flags & RUNTIME_MOVEMENT_STOPPED) == 0 && path->x <= primary->x && path->y <= primary->y && primary->x <= static_cast<int32_t>(path->width)
                            && primary->y <= static_cast<int32_t>(path->height))
                        {
                            if((path->flags & RUNTIME_PATH_HAS_LINE_DELTA) != 0)
                            {
                                update_runtime_tree_primary_resource_link(tree, primary, nullptr, path->line_first, path->line_second, 0);
                                primary->movement_deadline = state->script_clock + path->time;
                            }
                            rebuild_runtime_pointer_resources();
                            update_runtime_pointer_region(runtime_pointer_x, runtime_pointer_y);
                            return RuntimeScriptOpcodeDisposition::PAUSE;
                        }
                    }
                    primary->flags &= ~RUNTIME_MOVEMENT_MASK;
                }
            }
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0xf0000:
        if((link->owner_flags & RUNTIME_SCRIPT_LINK_WAIT_PENDING) == 0)
        {
            const int32_t duration = parse_script_integer_expression(parser);
            if(duration != SCRIPT_INTEGER_INVALID)
            {
                link->wait_deadline = state->script_clock + static_cast<uint32_t>(duration);
                link->owner_flags |= RUNTIME_SCRIPT_LINK_WAIT_PENDING;
                return RuntimeScriptOpcodeDisposition::PAUSE;
            }
        }
        else
        {
            if(state->script_clock < link->wait_deadline)
                return RuntimeScriptOpcodeDisposition::PAUSE;
            link->owner_flags &= ~RUNTIME_SCRIPT_LINK_WAIT_PENDING;
        }
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    case 0x40000000:
    case 0x40000001:
    case 0x50000000:
    case 0x50000001:
    {
        uint32_t target_flags = 0;
        if(parse_runtime_tree_command_target(parser, first, second, &target_flags) == 0)
            return RuntimeScriptOpcodeDisposition::COMPLETE;
        state->accumulated_tree_flags = target_flags;
        if((opcode & 0xff) != 0)
            state->accumulated_tree_flags = opcode & RUNTIME_TREE_LOW_FLAG_MASK;
        if(target_flags != 0)
            state->accumulated_tree_flags |= RUNTIME_TREE_EXPLICIT_FLAGS;
        if(state->current_runtime_resource != nullptr && activate_default_comment_scene("m_DEF_LOAD") > 0)
            wait_for_display_scene_ready(500);
        destroy_runtime_tree_resources(tree);
        RuntimeTreeNode *activated = activate_runtime_tree_with_notifications(first, second, nullptr, nullptr);
        void *published_identity = state->runtime_tree_identity;
        if(activated == nullptr)
        {
            RuntimeTreeNode *rebuild = nullptr;
            if((link->owner_flags & RUNTIME_SCRIPT_LINK_TREE_FALLBACK) != 0)
            {
                copy_runtime_tree_command_name(first, opcode);
                const uint32_t cursor = find_runtime_tree_link_007c_opcode_value(link, 0x700, first, 1);
                if(cursor != SCRIPT_PARSE_END)
                    saved_cursor = cursor;
                rebuild = find_and_create_runtime_tree_jump(parser, first, saved_cursor);
            }
            if(rebuild == nullptr)
            {
                send_application_event(HostApplicationCommand::RUNTIME_FAILURE);
                rebuild = tree;
            }
            rebuild_runtime_tree_resources(rebuild);
            published_identity = state->runtime_tree_identity;
        }
        else
        {
            if(tree->identity == activated)
                reset_runtime_tree_parser_contexts(activated);
            else if((opcode & 0xffffff00) == 0x50000000)
                deactivate_runtime_tree_and_visuals(tree, activated);
            rebuild_runtime_tree_resources(activated);
            activated->flags |= state->accumulated_tree_flags;
            published_identity = state->runtime_tree_identity;
        }
        state->runtime_tree_identity = published_identity;
        refresh_runtime_pointer_region();
        // The command invalidates the saved parser cursor, so bypass the common restore path.
        return RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR;
    }

    case 0x60000:
        scan_runtime_tree_link_007c_control_boundary(link, 0x6000);
        return RuntimeScriptOpcodeDisposition::COMPLETE;

    default:
        return RuntimeScriptOpcodeDisposition::UNHANDLED;
    }
}

bool should_send_runtime_script_message(int32_t command)
{
    // The transition can acknowledge its host-event caller before the UI thread finishes the callback. Suppress another synchronous 2010 send from that same physical press at the sending
    // boundary, where it cannot block waiting for the still-busy UI thread.
    if(command == static_cast<int32_t>(HostApplicationCommand::TOGGLE_FULLSCREEN))
    {
        if(desktop_fullscreen_toggle_latched)
            return false;
        desktop_fullscreen_toggle_latched = true;
    }
    return true;
}

void execute_script_commands(void *parameter)
{
    auto *state = static_cast<RuntimeCommandLoopState *>(parameter);
    uint32_t previous_tick = runtime_milliseconds();
    // Initialize before the first outer pass so SWRAND always has a defined prior value.
    int32_t random_value = 0;
    while(true)
    {
        drain_runtime_resource_destructions();
        if((state->flags & RUNTIME_COMMAND_SHUTDOWN_REQUESTED) != 0)
            return;
        process_available_runtime_generic_children(state->script_clock);
        process_runtime_text_input(state);
        process_runtime_queued_input();
        run_runtime_command_loop(state);
        if((state->flags & RUNTIME_COMMAND_SCRIPT_EXECUTION_ENABLED) == 0)
        {
            previous_tick = runtime_milliseconds();
            runtime_sleep(10);
            continue;
        }

        RuntimeTreeNode *tree = find_runtime_tree_node_by_identity(state->runtime_tree_identity);
        bool restart_outer = false;
        if(tree != nullptr)
        {
            synchronize_runtime_plan_mode();
            if(!process_pending_runtime_tree_switch(tree))
            {
                acknowledge_current_runtime_event_record();
                for(RuntimeTreeLink7C *link = script_runtime_root->global_link_007c_head; link != nullptr; link = link->next)
                {
                    process_available_runtime_generic_children(state->script_clock);
                    process_runtime_text_input(state);
                    if(run_pending_runtime_external_command() != 0 || process_runtime_queued_input() != 0 || run_runtime_command_loop(state) != 0)
                        break;

                    ScriptParserState *parser = nullptr;
                    if(activate_runtime_tree_link_007c(link) != 0)
                    {
                        state->active_script_link = link;
                        parser = &link->parser;
                        while(true)
                        {
                            uint32_t saved_cursor = parser->cursor;
                            const uint32_t opcode = parse_script_opcode(parser);
                            if(opcode == SCRIPT_PARSE_END)
                            {
                                saved_cursor = parser->start_offset;
                                link->owner_flags &= ~RUNTIME_SCRIPT_LINK_ACTIVE;
                                parser->cursor = saved_cursor;
                                break;
                            }
                            const RuntimeScriptOpcodeDisposition disposition = execute_simple_runtime_script_opcode(state, tree, link, opcode, random_value, saved_cursor);
                            if(disposition == RuntimeScriptOpcodeDisposition::COMPLETE || disposition == RuntimeScriptOpcodeDisposition::UNHANDLED)
                                continue;
                            if(disposition == RuntimeScriptOpcodeDisposition::FINISH_LINK)
                            {
                                saved_cursor = parser->start_offset;
                                link->owner_flags &= ~RUNTIME_SCRIPT_LINK_ACTIVE;
                            }
                            else if(disposition == RuntimeScriptOpcodeDisposition::COMMIT_CURSOR)
                            {
                                saved_cursor = parser->cursor;
                            }
                            else if(disposition == RuntimeScriptOpcodeDisposition::RESTART_OUTER_COMMIT_CURSOR)
                            {
                                // This disposition returns directly to the outer loop. The command may have destroyed the current tree or link, so neither parser read nor write is valid
                                // here.
                                restart_outer = true;
                                break;
                            }
                            parser->cursor = saved_cursor;
                            if(disposition == RuntimeScriptOpcodeDisposition::RESTART_OUTER)
                                restart_outer = true;
                            break;
                        }
                    }
                    state->active_script_link = nullptr;
                    if(restart_outer)
                        break;
                }
            }
        }

        state->active_script_link = nullptr;
        const uint32_t current_tick = runtime_milliseconds();
        random_value = select_bounded_random_value(-10000, 10000);
        state->script_clock = current_tick + (state->script_clock - previous_tick);
        previous_tick = current_tick;
        runtime_sleep(10);
    }
}

bool update_runtime_target(void *, RuntimeCommandBounds *bounds, int mode)
{
    if(mode == 1)
    {
        synchronize_display_region(reinterpret_cast<DisplayRectangle *>(bounds), 1);
        return true;
    }
    if(mode == 0x10000)
    {
        if((runtime_target_flags & RUNTIME_TARGET_ACTIVE) == 0 && bounds->first == 0)
            return begin_runtime_target_from_bounds_fields(bounds->height, bounds->second, bounds->width) == 0;
        return true;
    }
    if(mode == 0x20000 && (runtime_target_flags & RUNTIME_TARGET_ACTIVE) == 0 && bounds->first == 0)
        return end_display_target() == 0;
    return true;
}

} // namespace freegag
