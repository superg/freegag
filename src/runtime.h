#pragma once

#include "runtime_types.h"

namespace gag
{

void enable_runtime_subsystem();

void disable_runtime_subsystem();

void set_active_object_field_0824(uint32_t value);

void set_runtime_flag_01000000();

void clear_runtime_flag_01000000();

void clear_runtime_command_state();

void set_credits_runtime_flag();

void enter_runtime_state_1000();

void leave_runtime_state_1000();

void reset_runtime_pair_queue();

void enqueue_runtime_byte(uint8_t value);

uint8_t dequeue_runtime_byte();

void reset_runtime_byte_queue();

void enqueue_runtime_pair(uint32_t first, uint32_t second);

int dequeue_runtime_pair(RuntimeMessagePair *pair);

bool synchronize_runtime_plan_mode();

bool process_pending_runtime_tree_switch(RuntimeTreeNode *node);

RuntimeTreeNode *activate_runtime_tree_with_notifications(const char *resource_name, const char *tree_name, void *parent_selector, void *creation_context);

uint32_t process_runtime_pair_message();

uint32_t copy_runtime_input_session_record(RuntimeInputSessionRecord *record);

DisplaySceneNode *acquire_runtime_text_input_scene(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format);

uint32_t begin_runtime_text_input_scene_update(intptr_t identifier);

uint32_t end_runtime_text_input_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);

void initialize_runtime_input_session(void *first, void *second, void *selector, void *fourth, void *fifth, uint32_t character_width, void *session_value);

void enqueue_runtime_message(uint32_t message);

uint32_t dequeue_runtime_message();

void process_runtime_text_input(RuntimeCommandLoopState *state);

void process_runtime_message(RuntimeCommandLoopState *state);

bool update_runtime_target(void *unused, RuntimeCommandBounds *bounds, int mode);

int run_runtime_command_loop(RuntimeCommandLoopState *state);

uint32_t run_pending_runtime_external_command();

DWORD WINAPI execute_script_commands(LPVOID parameter);

RuntimeScriptOpcodeDisposition execute_simple_runtime_script_opcode(RuntimeCommandLoopState *state, RuntimeTreeNode *tree, RuntimeTreeLink7C *link, uint32_t opcode, int32_t random_value = 0,
    uint32_t saved_cursor = 0xffffffff);

bool should_send_runtime_script_message(int32_t command);

} // namespace gag
