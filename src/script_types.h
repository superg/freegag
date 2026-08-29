#pragma once

#include "application_types.h"
#include "runtime_services.h"

namespace freegag
{
inline constexpr uint32_t SCRIPT_PARSE_END = 0xffffffff;

inline constexpr int32_t SCRIPT_INTEGER_INVALID = 0x7fffffff;

enum ScriptValueType : uint32_t
{
    SCRIPT_VALUE_TYPE_NONE = 0,
    SCRIPT_VALUE_TYPE_BOOLEAN = 1,
    SCRIPT_VALUE_TYPE_INTEGER = 2,
    SCRIPT_VALUE_TYPE_STRING = 4,
    SCRIPT_VALUE_TYPE_INVALID = 0x7fffffff
};

enum ScriptBooleanValue : uint32_t
{
    SCRIPT_BOOLEAN_TRUE = 0x03000000,
    SCRIPT_BOOLEAN_FALSE = 0x07000000
};

enum class RuntimeResourceSceneRole : uint32_t
{
    INDEXED_SOURCE,
    XRGB_COMPOSITION,
};

enum ScriptScopeCode : uint32_t
{
    SCRIPT_SCOPE_VALUE = 0x00010000,
    SCRIPT_SCOPE_UP_DOWN = 0x00020000,
    SCRIPT_SCOPE_NO_MATCHES = 0x00030000,
    SCRIPT_SCOPE_Z = 0x00040000,
    SCRIPT_SCOPE_LAYER = 0x00050000,
    SCRIPT_SCOPE_INVERT_NO_PALETTE = 0x00060000,
    SCRIPT_SCOPE_LIST = 0x00100000,
    SCRIPT_SCOPE_GLOBAL = 0x00200000,
    SCRIPT_SCOPE_RATIO = 0x00300000,
    SCRIPT_SCOPE_RADIUS = 0x00400000,
    SCRIPT_SCOPE_LINE = 0x00500000,
    SCRIPT_SCOPE_TIME = 0x00600000,
    SCRIPT_SCOPE_PATH = 0x00700000,
    SCRIPT_SCOPE_IMAGE = 0x00800000,
    SCRIPT_SCOPE_LOOP = 0x00900000,
    SCRIPT_SCOPE_RANDOM = 0x00a00000,
    SCRIPT_SCOPE_PRIORITY = 0x00b00000,
    SCRIPT_SCOPE_KEY_UP = 0x00c00000,
    SCRIPT_SCOPE_TRANSPARENT = 0x00d00000,
    SCRIPT_SCOPE_TEXT = 0x00e00000,
    SCRIPT_SCOPE_FONT = 0x00f00000,
    SCRIPT_SCOPE_FILE = 0x01000000,
    SCRIPT_SCOPE_RECTANGLE = 0x02000000,
    SCRIPT_SCOPE_SOURCE = 0x05000000,
    SCRIPT_SCOPE_DESTINATION = 0x06000000,
    SCRIPT_SCOPE_FLAGS = 0x0a000000,
    SCRIPT_SCOPE_POSITION = 0x0b000000,
    SCRIPT_SCOPE_COMMAND = 0x0c000000,
    SCRIPT_SCOPE_MOUSE = 0x0d000000,
    SCRIPT_SCOPE_CONTAINER_CONDITION = 0x0e000000,
    SCRIPT_SCOPE_ZONE = 0x0f000000,
    SCRIPT_SCOPE_PARENT_COMMAND = 0x10000000,
    SCRIPT_SCOPE_ALTERNATE_MOUSE = 0x20000000,
    SCRIPT_SCOPE_OWNER = 0x30000000,
    SCRIPT_SCOPE_PRELOAD = 0x50000000
};

enum ScriptImageFlag : uint32_t
{
    SCRIPT_IMAGE_PRIMARY = 0x00000001,
    SCRIPT_IMAGE_NATURAL_MOUSE = 0x00010000,
    SCRIPT_IMAGE_STOPPED = 0x00000010,
    SCRIPT_IMAGE_LOAD_ONLY = 0x00000200,
    SCRIPT_IMAGE_DOUBLE_SIZE = 0x00200000,
    SCRIPT_IMAGE_NO_PALETTE = 0x04000000
};

inline constexpr uint32_t RUNTIME_NAMED_NODE_ENABLED = 0x00000001;

inline constexpr uint32_t RUNTIME_GENERIC_BACKEND_LOCKED = 0x00010000;

enum RuntimeGenericChildFlag : uint32_t
{
    RUNTIME_GENERIC_CHILD_STATE_VALID = 0x00000001,
    RUNTIME_GENERIC_CHILD_USE_CURRENT_SELECTION = 0x00000002,
    RUNTIME_GENERIC_CHILD_SELECTION_OVERRIDE = 0x00000100,
    RUNTIME_GENERIC_CHILD_MODE_200 = 0x00000200,
    RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER = 0x00001000,
    RUNTIME_GENERIC_CHILD_LOCKED = 0x00010000
};

inline constexpr uint32_t RUNTIME_GENERIC_CHILD_FLAGS_UNAVAILABLE = 0x7fffffff;

enum ScriptRuntimeFlag : uint32_t
{
    SCRIPT_RUNTIME_COMMENTS_SUPPRESSED = 0x00000001,
    SCRIPT_RUNTIME_INVENTORY_OPEN = 0x00000002,
    SCRIPT_RUNTIME_INVENTORY_CLOSE = 0x00000004,
    SCRIPT_RUNTIME_NO_SAVE = 0x00000100,
    SCRIPT_RUNTIME_STREAMING_ACTIVE = 0x00000010,
    SCRIPT_RUNTIME_PLANS_INACTIVE = 0x00000020,
    SCRIPT_RUNTIME_NO_PALETTE_ADJUSTMENT = 0x04000000
};

struct ScriptTextBuffer
{
    uint32_t length;
    uint32_t capacity;
    char *data;
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
    RuntimeGenericResourceNode *resource;
    const char *text;
    uint32_t text_length;
    uint32_t start_offset;
    uint32_t cursor;
    RuntimeVisualObject *primary_visual;
};



struct ScriptObjectState
{
    char name[0x20];
    void *identity;
    ScriptObjectState *next;
    char field_names[32][0x20];
    uint32_t field_count;
    uint32_t mouse_flags;
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
    uint32_t link_movement_flags;
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
    uint32_t link_movement_deadline;
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
    uint8_t reserved_0000[0x430];
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
    RuntimeGenericResourceNode *next;
};



struct RuntimePlanNode
{
    uint8_t reserved_0000[0x24];
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
    uint32_t flags;
    char class_name[0x20];
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
    RuntimeThreadId owner_thread;
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
    RuntimeThreadId owner_thread;
    uint32_t recursion_count;
    intptr_t scene_identifier;
    DisplaySceneDescriptor scene_descriptor;
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
    RuntimeResourceSceneRole scene_role;
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    RuntimeGenericBackendChild *generic_backend_child;
    intptr_t callback_position;
};

} // namespace freegag
