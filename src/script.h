#pragma once

#include "runtime_types.h"

namespace gag
{

int copy_string(char *destination, const char *source);

int32_t select_bounded_random_value(int32_t minimum, int32_t maximum);

ScriptTextBuffer *create_script_text_buffer();

void clear_script_text_buffer(ScriptTextBuffer *buffer);

void begin_script_text_document(ScriptTextBuffer *buffer);

void end_script_text_document(ScriptTextBuffer *buffer);

void append_script_text_property(ScriptTextBuffer *buffer, uint32_t property, const char *value);

void end_script_text_statement(ScriptTextBuffer *buffer);

void append_script_text_scope(ScriptTextBuffer *buffer, uint32_t scope);

void append_script_text_preload_directive(ScriptTextBuffer *buffer, uint32_t scope);

void append_script_text_scoped_tokens(ScriptTextBuffer *buffer, uint32_t scope, const char *text);

void append_script_text_delimiter(ScriptTextBuffer *buffer, const char *text, char delimiter);

void append_script_text_integer(ScriptTextBuffer *buffer, uint32_t value, char delimiter);

int find_script_property_value(char *value, const char *property_name, const char *text, uint32_t text_length, uint32_t start_offset);

int find_script_section(const char *section_name, const char *text, int text_length);

int32_t parse_path_numeric_identifier(const char *path);

uint32_t parse_script_property_code(ScriptParserState *parser);

uint32_t parse_script_scope_code(ScriptParserState *parser);

uint32_t parse_script_opcode(ScriptParserState *parser);

uint32_t extract_script_property_name(ScriptParserState *parser, char *name);

uint32_t extract_script_scope_name(ScriptParserState *parser, char *name);

uint32_t extract_script_parenthesized_text(ScriptParserState *parser, char *text, uint32_t text_capacity);

int find_whitespace_token_index(const char *text, const char *token);

uint32_t extract_script_token(ScriptParserState *parser, char *token, uint32_t token_capacity);

void parse_script_typed_value(ScriptParserState *parser, void *value, uint32_t *value_type);

void append_natural_mouse_image_flag(ScriptTextBuffer *buffer, uint32_t flags);

void serialize_image_flag_overrides(ScriptTextBuffer *buffer, uint32_t flags);

uint32_t parse_script_parameter_token(const char *text, int32_t token_index, void *value, uint32_t *value_type);

uint32_t evaluate_script_parameter(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);

int32_t parse_script_integer_expression(ScriptParserState *parser);

uint32_t parse_script_value_token(ScriptParserState *parser, char *value, uint32_t value_capacity);

uint32_t parse_image_flag(ScriptParserState *parser);

uint32_t parse_runtime_tree_command_target(ScriptParserState *parser, char *resource_name, char *tree_name, uint32_t *flags);

int32_t parse_script_integer_literal(ScriptParserState *parser);

bool fixed_dword_memory_equal(const void *left, const void *right, uint32_t byte_count);

void copy_runtime_tree_command_name(char *destination, uint32_t command);

ScriptObjectState *create_script_object_state(const void *name);

uint32_t parse_script_object_state(ScriptParserState *parser);

ScriptObjectState *find_script_object_by_identity(void *identity);

int32_t query_or_create_script_object_field(const char *object_name, const void *field_name, uint32_t *value, int32_t value_type);

int32_t get_script_object_integer(const char *object_name, const void *field_name);

uint32_t get_script_object_string(const char *object_name, const void *field_name, void *destination);

int32_t add_script_object_integer(const char *object_name, const void *field_name, int32_t delta);

bool compare_script_object_field(const char *object_name, const void *field_name, const void *value, int32_t value_type);

uint32_t get_script_object_field_snapshot(const char *object_name, const void *field_name, ScriptObjectFieldSnapshot *snapshot);

void destroy_script_object_states();

bool remove_runtime_visual_object(void *identity);

void destroy_runtime_visual_objects();

uint32_t apply_runtime_tree_image_flags(ScriptParserState *parser);

uint32_t parse_script_file_value(ScriptParserState *parser, char *value, char *serialized_value);

uint32_t create_or_update_runtime_fixed_name_node(ScriptParserState *parser);

RuntimeFixedNameListNode *find_runtime_fixed_name_list_node(const void *name);

void destroy_runtime_fixed_name_list_nodes();

const ArchiveCommentEnumerationApi &get_archive_comment_enumeration_api();

RuntimeTreeNode *update_conditional_runtime_tree(ScriptParserState *parser);

RuntimeTreeNode *create_conditional_runtime_tree(ScriptParserState *parser);

ScriptObjectState *find_script_object_by_name(const char *name);

ScriptObjectState *resolve_state_field_reference(const char *object_name, const char *field_name, const void *value, int value_type);

void copy_file_name_from_path(char *destination, const char *source);

} // namespace gag
