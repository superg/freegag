#pragma once

#include "runtime_types.h"

namespace gag
{

void release_runtime_tree_auxiliary_nodes(RuntimeTreeNode *owner);

void add_runtime_tree_auxiliary_name(RuntimeTreeNode *owner, const char *name);

uint32_t parse_runtime_tree_auxiliary_names(ScriptParserState *parser);

uint32_t add_default_runtime_tree_auxiliary_names(RuntimeTreeNode *owner);

RuntimeTreeNode *destroy_runtime_tree_node(void *identity, void *replacement_identity);

void update_runtime_tree_global_links(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);

void publish_runtime_tree_global_links(RuntimeTreeNode *node);

void append_script_runtime_flags(ScriptTextBuffer *buffer, uint32_t flags);

void serialize_runtime_tree_sections(ScriptTextBuffer *buffer);

void serialize_runtime_language(ScriptTextBuffer *buffer);

void serialize_runtime_fixed_name_nodes(ScriptTextBuffer *buffer);

ScriptTextBuffer *serialize_current_runtime_state();

RuntimeTreeNode *get_runtime_tree_root();

RuntimeTreeNode *find_runtime_tree_tail();

RuntimeTreeNode *find_runtime_tree_ancestor_root(void *identity);

RuntimeGenericResourceNode *find_runtime_generic_resource(void *identity);

void remove_all_runtime_generic_resources();

void set_runtime_generic_resource_position(void *identity, uint32_t position);

uint32_t read_runtime_generic_resource_token(void *identity, char *output, uint32_t capacity, uint8_t delimiter);

RuntimeGenericResourceNode *find_or_load_runtime_generic_resource(const char *resource_name);

RuntimeTreeParserContext *find_or_create_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);

void release_runtime_tree_parser_contexts(RuntimeTreeNode *owner);

RuntimeTreeParserContext *find_existing_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name);

RuntimeTreeNode *dispatch_runtime_tree_parser(RuntimeTreeParserContext *context);

RuntimeTreeNode *create_runtime_tree_node(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);

RuntimeTreeNode *find_and_create_runtime_tree_jump(ScriptParserState *parser, const char *target, uint32_t success_cursor);

void reset_runtime_tree_parser_context_recursive(ScriptParserState *parser);

void reset_runtime_tree_parser_contexts(void *identity);

RuntimeTreeNode *dispatch_runtime_tree_section(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);

uintptr_t dispatch_runtime_tree_section_command(ScriptParserState *parser);

bool parse_runtime_language(ScriptParserState *parser);

RuntimeTreeNode *create_runtime_tree_command(ScriptParserState *parser);

void set_script_runtime_flags(uint32_t mask, int enabled);

void reset_script_runtime_transient_indices();

uint32_t parse_runtime_command_definition(ScriptParserState *parser);

void append_dual_image_flag(ScriptTextBuffer *buffer, uint32_t flags);

void serialize_runtime_command_definitions(ScriptTextBuffer *buffer);

void clear_runtime_command_definitions();

RuntimeTreeNode *find_runtime_tree_node(RuntimeTreeNode *root, void *identity);

RuntimeTreeNode *find_runtime_tree_node_by_identity(void *identity);

void *find_last_runtime_tree_scene_link(RuntimeTreeNode *root);

void *find_last_runtime_tree_secondary_resource_link(RuntimeTreeNode *root);

void *find_last_runtime_tree_primary_resource_link(RuntimeTreeNode *root);

void *find_last_runtime_scene_link_by_identity(void *identity);

void *find_last_runtime_primary_resource_link_by_identity(void *identity);

void *find_last_runtime_secondary_resource_link_by_identity(void *identity);

uint32_t parse_runtime_tree_scene_link(ScriptParserState *parser);

uint32_t parse_runtime_tree_secondary_resource_link(ScriptParserState *parser);

RuntimeTreeSceneLink *find_global_runtime_tree_scene_link_by_name(const void *name);

RuntimeTreeSceneLink *find_runtime_tree_scene_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_scene_link(RuntimeTreeNode *node, RuntimeTreeSceneLink *link);

void remove_runtime_tree_scene_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

RuntimeTreeSecondaryResourceLink *find_global_runtime_tree_secondary_resource_link_by_name(const void *name);

RuntimeTreeSecondaryResourceLink *find_runtime_tree_secondary_resource_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_secondary_resource_link(RuntimeTreeNode *node, RuntimeTreeSecondaryResourceLink *link);

void remove_runtime_tree_secondary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

RuntimeTreePrimaryResourceLink *find_runtime_tree_primary_resource_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_primary_resource_link(RuntimeTreeNode *node, RuntimeTreePrimaryResourceLink *link);

void remove_runtime_tree_primary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

void update_runtime_tree_primary_resource_link(void *tree_identity, void *link_identity, const void *name, int32_t x_delta, int32_t y_delta, uint32_t image_flags);

void append_three_digit_decimal_suffix(const char *prefix, uint32_t value, char *output);

void *create_or_update_runtime_tree_primary_resource_link(void *tree_identity, const void *identifier, const void *file_name, int32_t source_value, int32_t x_delta, int32_t y_delta,
    uint32_t image_flags);

void *create_or_update_runtime_tree_link_0084(void *tree_identity, const void *name, int32_t x, int32_t y, uint32_t width, uint32_t height, uintptr_t mouse_visual_value, void *owner_identity,
    void *primary_resource_identity, uintptr_t owner_group_identity, uint32_t command_mask, uint32_t parameter);

uint32_t parse_runtime_tree_primary_resource_link(ScriptParserState *parser);

RuntimeTreePrimaryResourceLink *find_global_runtime_tree_primary_resource_link_by_name(const void *name);

uint32_t parse_runtime_tree_link_0084(ScriptParserState *parser);

RuntimeTreeLink84 *find_last_runtime_tree_link_0084(RuntimeTreeNode *root);

RuntimeTreeLink84 *find_runtime_tree_link_0084_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_link_0084(RuntimeTreeNode *node, RuntimeTreeLink84 *link);

void remove_runtime_tree_link_0084_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

void update_runtime_tree_link_0084(void *tree_identity, void *link_identity, int32_t x, int32_t y, uint32_t width, uint32_t height, uintptr_t mouse_visual_value, void *owner_identity,
    void *primary_resource_identity, uintptr_t owner_group_identity, uint32_t command_mask, uint32_t parameter);

RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_name(const void *name);

RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_identity(void *identity);

uint32_t parse_runtime_tree_link_008c(ScriptParserState *parser);

uint32_t parse_runtime_tree_link_007c(ScriptParserState *parser);

void seek_runtime_tree_link_007c_label(void *identity, const char *label);

uint32_t find_runtime_tree_link_007c_opcode_value(void *identity, uint32_t opcode, const char *value, int restore_cursor);

uint32_t scan_runtime_tree_link_007c_control_boundary(void *identity, uint32_t requested_boundary);

uint32_t match_runtime_tree_link_007c_interaction(uintptr_t *state, const RuntimeTreeInteractionCriteria *criteria);

uint32_t activate_runtime_tree_link_007c(RuntimeTreeLink7C *link);

uint32_t parse_script_object_container(ScriptParserState *parser);

RuntimeTreeLink7C *find_last_runtime_tree_link_007c(RuntimeTreeNode *root);

RuntimeTreeLink7C *find_runtime_tree_link_007c_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_link_007c(RuntimeTreeNode *node, RuntimeTreeLink7C *link);

void remove_runtime_tree_link_007c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

ScriptObjectContainer *find_last_script_object_container(RuntimeTreeNode *root);

ScriptObjectContainer *find_script_object_container_insertion_predecessor(RuntimeTreeNode *node);

void insert_script_object_container(RuntimeTreeNode *node, ScriptObjectContainer *container);

void remove_script_object_container_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

bool destroy_script_object_container(ScriptObjectContainer *container);

bool script_object_container_state_matches_by_identity(void *identity);

bool script_object_container_state_matches_by_name(const void *name);

ScriptObjectContainer *find_script_condition_container_by_name(const void *name);

RuntimeTreeLink8C *find_last_runtime_tree_link_008c(RuntimeTreeNode *root);

RuntimeTreeLink8C *find_runtime_tree_link_008c_insertion_predecessor(RuntimeTreeNode *node);

void insert_runtime_tree_link_008c(RuntimeTreeNode *node, RuntimeTreeLink8C *link);

void remove_runtime_tree_link_008c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node);

RuntimeTreeLink8C *find_global_runtime_tree_link_008c_by_name(const void *name);

} // namespace gag
