#pragma once

#include "runtime_types.h"

namespace freegag
{

void serialize_script_object_states(ScriptTextBuffer *buffer);

uint32_t parse_runtime_visual_object(ScriptParserState *parser);

void *create_or_update_runtime_visual_object(const void *name, const void *file_name, int32_t position_x, int32_t position_y, uint32_t flags, uint32_t palette_flags);

void serialize_runtime_visual_objects(ScriptTextBuffer *buffer);

void remove_runtime_generic_resource(void *identity);

RuntimeNamedNode *find_runtime_named_child(void *parent_identity, void *child_identity);

RuntimeResourceCacheEntry *find_runtime_resource_cache_entry(void *parent_identity, const char *name);

RuntimeResourceCacheEntry *get_or_create_runtime_resource_cache_entry(void *parent_identity, const char *name);

RuntimeResourceCacheEntry *get_or_create_runtime_child_by_data(void *parent_identity, void *data);

void add_script_object_to_runtime_named_node(const void *node_name, const char *object_name);

uint32_t parse_runtime_named_node(ScriptParserState *parser);

void remove_script_object_from_runtime_named_node(const void *node_name, const char *object_name);

uint32_t rotate_runtime_named_node_cursor_previous(const void *node_name, int32_t count);

uint32_t rotate_runtime_named_node_cursor_next(const void *node_name, int32_t count);

uint32_t clear_runtime_named_node_children(const void *node_name);

void remove_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

uint32_t remove_runtime_named_child_by_identity(void *parent_identity, void *child_identity);

void append_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry);

void serialize_runtime_named_nodes(ScriptTextBuffer *buffer);

void purge_disabled_runtime_named_nodes();

RuntimeNamedNode *get_or_create_runtime_named_node(const char *name);

bool set_runtime_plans_inactive();

bool clear_runtime_plans_inactive();

RuntimeVisualObject *find_runtime_visual_object(const char *name);

void enqueue_runtime_event_record(const uintptr_t *record);

void acknowledge_current_runtime_event_record();

uint32_t read_runtime_event_record(uintptr_t *record, int32_t advance);

int32_t select_pointer_region_scene(RuntimePointerRegion *region);

uint32_t synchronize_runtime_pointer_owner_slots(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);

RuntimeGenericBackendChild *attach_runtime_generic_backend_child(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, uintptr_t selection, uint32_t flags);

void rebuild_runtime_tree_resources(void *identity);

void rebuild_runtime_pointer_resources();

uint32_t handle_runtime_left_button_up();

uint32_t handle_runtime_left_button_down();

uint32_t handle_runtime_right_button_down();

uint32_t update_runtime_pointer_region(int32_t x, int32_t y);

uint32_t refresh_runtime_pointer_region();

int32_t activate_default_comment_scene(const char *name);

void activate_runtime_tree_node_comment(RuntimeTreeNode *node);

void deactivate_default_comment_scene(const char *name);

void deactivate_runtime_tree_node_comment(RuntimeTreeNode *node);

void set_runtime_tree_comment_mode(RuntimeTreeNode *root, int enabled);

RuntimeTreeNode *begin_runtime_tree_enumeration(void *identity);

RuntimeTreeNode *get_next_runtime_tree_node(RuntimeTreeNode *root);

int destroy_runtime_comment_trees();

intptr_t deactivate_runtime_tree_and_visuals(void *identity, void *second);

void *find_runtime_tree_identity_by_name_recursive(void *start_identity, const void *name);

void *find_runtime_tree_descendant_identity_by_name(void *root_identity, const void *name);

void *find_runtime_tree_root_identity_by_name(const void *name);

bool has_runtime_pointer_inventory_pack();

} // namespace freegag
