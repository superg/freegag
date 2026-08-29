#pragma once

#include "runtime_types.h"

namespace freegag
{

void enable_runtime_subsystem();

void disable_runtime_subsystem();

void set_runtime_resource_variant(uint32_t value);

void suspend_runtime_state();

void resume_runtime_state();

void set_runtime_pointer_window_active(bool active);

void reset_runtime_input_queue();

void enqueue_runtime_byte(uint8_t value);

uint8_t dequeue_runtime_byte();

void reset_runtime_byte_queue();

void enqueue_runtime_input(RuntimeQueuedInputType type, uint32_t packed_position);

bool dequeue_runtime_input(RuntimeQueuedInput *input);

bool synchronize_runtime_plan_mode();

bool process_pending_runtime_tree_switch(RuntimeTreeNode *node);

RuntimeTreeNode *activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *parent_selector, void *creation_context);

uint32_t process_runtime_queued_input();

RuntimeInputText take_runtime_input_text();

DisplaySceneNode *acquire_runtime_text_input_scene(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format);

uint32_t begin_runtime_text_input_scene_update(intptr_t identifier);

uint32_t end_runtime_text_input_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);

void initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, uint32_t character_width, void *session_value);

void process_runtime_text_input(RuntimeCommandLoopState *state);

bool update_runtime_target(void *unused, RuntimeCommandBounds *bounds, int mode);

int run_runtime_command_loop(RuntimeCommandLoopState *state);

uint32_t run_pending_runtime_external_command();

void execute_script_commands(void *parameter);

RuntimeScriptOpcodeDisposition execute_simple_runtime_script_opcode(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, uint32_t opcode, int32_t random_value = 0,
    uint32_t saved_cursor = SCRIPT_PARSE_END);

bool should_send_runtime_script_message(int32_t command);

} // namespace freegag
