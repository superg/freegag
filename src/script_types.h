#pragma once

#include "application_types.h"

namespace gag
{
struct ScriptTextBuffer
{
    uint32_t length;
    uint32_t capacity;
    char *data;
};

struct ScriptUtilityApi
{
    LPVOID(WINAPI *virtual_alloc)(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protection);
    uint32_t (*get_tick_count)();
    void (*seed_random)(unsigned int seed);
    int (*random)();
};



struct RuntimeTreeNode;
struct RuntimeGenericResourceNode;
struct RuntimeTreeLink84;
struct RuntimeTreePrimaryResourceLink;
struct RuntimeVisualObject;

struct ScriptParserState
{
    RuntimeTreeNode *owner;
    char *name;
    char *creation_text;
    char *scratch_text;
    uint8_t unknown_0010[4];
    RuntimeGenericResourceNode *resource;
    const char *text;
    uint32_t text_length;
    uint32_t start_offset;
    uint32_t cursor;
    RuntimeVisualObject *primary_visual;
};



struct ScriptValueParseApi
{
    uint32_t (*evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);
};

struct ScriptTypedValueApi
{
    int32_t (*parse_integer_expression)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    uint32_t (*parse_value_token)(ScriptParserState *parser, char *value, uint32_t capacity);
};

struct RuntimeTreeCommandTargetApi
{
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    uint32_t (*parse_value_token)(ScriptParserState *parser, char *value, uint32_t value_capacity);
};

struct ScriptIntegerExpressionApi
{
    uint32_t (*evaluate_parameter)(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type);
    int32_t (*select_random)(int32_t minimum, int32_t maximum);
    RuntimeTreeLink84 *(*find_link_0084)(const void *name);
    RuntimeTreePrimaryResourceLink *(*find_primary_link)(const void *name);
    int32_t (*get_object_integer)(const char *object_name, const void *field_name);
    void (*query_runtime)(uint32_t operation, const void *source, int32_t *value);
};



struct ScriptObjectState
{
    char name[0x20];
    void *identity;
    ScriptObjectState *next;
    char field_names[32][0x20];
    uint32_t field_count;
    uint32_t flags_042c;
    char mouse_visual_name[0x20];
    char alternate_mouse_visual_name[0x20];
    RuntimeVisualObject *visual_object;
    RuntimeVisualObject *alternate_visual_object;
    uint32_t image_flags;
    uint32_t command_mask;
    uint32_t active_field_mask;
    int32_t integer_values[32];
    char string_values[32][0x20];
};


struct ScriptObjectMemoryApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
};

struct ScriptObjectReleaseApi
{
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct ScriptObjectFieldSnapshot
{
    char object_name[0x20];
    char field_name[0x20];
    uint32_t active;
    int32_t integer_value;
    char string_value[0x20];
};



struct ScriptObjectParseApi
{
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*parse_scope)(ScriptParserState *parser);
    int32_t (*parse_integer)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    bool (*fixed_equal)(const void *left, const void *right, uint32_t bytes);
    RuntimeVisualObject *(*find_visual)(const char *name);
    ScriptObjectState *(*create_object)(const void *name);
};



struct ScriptObjectSlot
{
    ScriptObjectState *object;
    uint32_t *active_field_mask;
    uint32_t field_mask;
};

struct ScriptObjectContainer
{
    char name[0x20];
    void *identity;
    ScriptObjectContainer *next;
    uint32_t current_mask;
    uint32_t required_mask;
    uint32_t slot_count;
    ScriptObjectSlot slots[32];
};

struct RuntimeVisualObject
{
    char name[0x20];
    void *identity;
    RuntimeVisualObject *next;
    char file_name[0x20];
    char serialized_file[0x104];
    int32_t position_x;
    int32_t position_y;
    void *previous_scene_identity;
    void *scene_identity;
    uint32_t flags;
    uint32_t palette_flags;
};



struct RuntimePointerRegion
{
    char name[0x20];
    void *identity;
    RuntimePointerRegion *next;
    uint8_t unknown_0028[4];
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    uint8_t unknown_003c[4];
    uint32_t scene_mask;
    uint32_t first_scene_bit;
    uint32_t current_scene_bit;
    uint32_t priority;
    RuntimeVisualObject *visual_override;
    void *owner_identity;
    ScriptObjectState *state_object;
    RuntimeTreePrimaryResourceLink *primary_resource;
    void *previous_owner_identity;
    void *previous_primary_resource_identity;
};


struct RuntimeSceneSlot
{
    char name[0x20];
    RuntimeVisualObject *visual_object;
    uint32_t flags;
};

struct RuntimeResourceCacheEntry;

struct RuntimeNamedNode
{
    char name[0x20];
    void *identity;
    uint32_t flags;
    uint32_t visible_entry_count;
    RuntimeNamedNode *next;
    int32_t zone_left;
    int32_t zone_top;
    int32_t zone_right;
    int32_t zone_bottom;
    uint32_t status;
    union
    {
        RuntimeNamedNode *children;
        RuntimeResourceCacheEntry *cache_entries;
    };
    union
    {
        RuntimeNamedNode *child_sentinel;
        RuntimeResourceCacheEntry *cache_entry_sentinel;
    };
    union
    {
        RuntimeNamedNode *child_cursor;
        RuntimeResourceCacheEntry *cache_entry_cursor;
    };
};


struct RuntimeResourceCacheEntry
{
    char name[0x20];
    void *data;
    uint32_t size;
    uint32_t flags_and_references;
    RuntimeResourceCacheEntry *next;
    RuntimeResourceCacheEntry *previous;
};

struct RuntimeExpandedListResource
{
    uint8_t unknown_0000[0x430];
    char primary_resource_name[0x4c];
    uint32_t link_value;
};

struct RuntimeGenericResourceNode
{
    char name[0x20];
    void *identity;
    void *resource_data;
    uint32_t current_position;
    uint32_t resource_metadata;
    uint32_t active_references;
    uint8_t unknown_0034[4];
    RuntimeGenericResourceNode *next;
};



struct RuntimeNamedNodeMemoryApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct RuntimeResourceReleaseApi
{
    void(WINAPI *enter_critical_section)(LPCRITICAL_SECTION section);
    void(WINAPI *leave_critical_section)(LPCRITICAL_SECTION section);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    RuntimeNamedNode *(*find_child)(void *parent_identity, void *child_identity);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    uint32_t (*remove_cache_entry)(void *parent_identity, void *child_identity);
    uint32_t (*close_async_record)(AsyncFileRecord *record);
    void (*set_script_flags)(uint32_t flags, int enabled);
};

struct RuntimePlanNode
{
    uint8_t unknown_0000[0x24];
    RuntimePlanNode *next;
    uint32_t flags;
};

struct RuntimeTreeSceneLink;
struct RuntimeTreeParserContext;
struct RuntimeTreePrimaryResourceLink;
struct RuntimeTreeSecondaryResourceLink;
struct RuntimeTreeLink7C;
struct RuntimeTreeLink84;
struct RuntimeTreeLink8C;
struct RuntimeTreeAuxiliaryNode;

struct RuntimeTreeNode
{
    char name[0x20];
    void *identity;
    RuntimeTreeNode *parent;
    uint8_t unknown_0028[4];
    uint32_t flags;
    uint8_t unknown_0030[0x10];
    char class_name[0x20];
    uint8_t unknown_0060[4];
    RuntimeTreeNode *iterator_current;
    uint32_t iterator_ascending;
    RuntimeTreeParserContext *parser_contexts;
    RuntimeVisualObject *default_visual;
    RuntimeTreeSceneLink *scene_link_head;
    RuntimeTreeSceneLink *scene_link_tail;
    RuntimeTreeLink7C *link_007c_head;
    RuntimeTreeLink7C *link_007c_tail;
    RuntimeTreeLink84 *link_0084_head;
    RuntimeTreeLink84 *link_0084_tail;
    RuntimeTreeLink8C *link_008c_head;
    RuntimeTreeLink8C *link_008c_tail;
    ScriptObjectContainer *container_head;
    ScriptObjectContainer *container_tail;
    RuntimeTreePrimaryResourceLink *primary_resource_link_head;
    RuntimeTreePrimaryResourceLink *primary_resource_link_tail;
    RuntimeTreeSecondaryResourceLink *secondary_resource_link_head;
    RuntimeTreeSecondaryResourceLink *secondary_resource_link_tail;
    RuntimeTreeNode *child;
    RuntimeTreeAuxiliaryNode *auxiliary_head;
    RuntimeTreeNode *next;
    RuntimeTreeNode *previous;
};



struct RuntimeTreeAuxiliaryNode
{
    char name[0x20];
    void *identity;
    RuntimeTreeAuxiliaryNode *next;
};


struct RuntimeTreeAuxiliaryReleaseApi
{
    void (*notify)(uint32_t operation, uint32_t unused, RuntimeTreeAuxiliaryNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};



struct RuntimeTreeAuxiliaryCreateApi
{
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    void (*resolve)(uint32_t operation, void **identity, void **metadata);
};



struct RuntimeTreeDestructionCoreApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    void (*notify)(uint32_t operation, uint32_t unused, void *value);
    void (*remove_scene_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_secondary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_primary_links)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_7c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_84)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_links_8c)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    void (*remove_containers)(RuntimeTreeNode *parent, RuntimeTreeNode *node);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL (*destroy_container)(ScriptObjectContainer *container);
    void (*release_auxiliary)(RuntimeTreeNode *owner);
    void (*release_parsers)(RuntimeTreeNode *owner);
    RuntimeTreeParserContext *(*find_parser)(RuntimeTreeNode *owner, const char *name);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*update_global_links)(RuntimeTreeNode *removed, RuntimeTreeNode *replacement);
};



struct RuntimeFixedNameListNode
{
    char name[0x20];
    void *identity;
    uint32_t flags;
    char serialized_value[0x20];
    uint32_t resource_flags;
    void *resource_identity;
    void *previous_resource_identity;
    RuntimeFixedNameListNode *next;
};



struct RuntimeLockRecord
{
    uint32_t state_flags;
    void *identity_context;
    uint32_t type_flags;
    uint32_t flags;
    void *data;
    DWORD owner_thread;
    uint32_t recursion_count;
    intptr_t scene_identifier;
};


struct RuntimeResourceObject
{
    uint32_t state_flags;
    void *backend;
    uint32_t type_flags;
    uint32_t backend_flags;
    void *data;
    DWORD owner_thread;
    uint32_t recursion_count;
    intptr_t scene_identifier;
    DisplaySceneDescriptor scene_descriptor;
    uint32_t presentation_owner;
    uint32_t field_0034;
    int32_t x;
    int32_t y;
    uint32_t output_width;
    uint32_t output_height;
    uint32_t requested_width;
    uint32_t requested_height;
    uint32_t frame_limit;
    uint32_t frames_remaining;
    int32_t previous_x;
    int32_t previous_y;
    uint8_t unknown_0060[4];
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    uint8_t unknown_006c[8];
    RuntimeGenericBackendChild *generic_backend_child;
    uint8_t unknown_0078[0x11c];
    intptr_t callback_position;
};

} // namespace gag
