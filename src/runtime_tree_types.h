#pragma once

#include <atomic>
#include "audio_types.h"
#include "runtime_services.h"
#include "shared_binary_file.h"

namespace freegag
{
inline constexpr uint32_t RUNTIME_MOVEMENT_DEADLINE_INACTIVE = 0xffffffff;

enum RuntimeResourceTokenResult : uint32_t
{
    RUNTIME_RESOURCE_TOKEN_EMPTY = 0x7fffffff,
    RUNTIME_RESOURCE_TOKEN_UNAVAILABLE = 0xffffffff
};

enum class ScriptRuntimeProperty : uint32_t
{
    PALETTE_TRANSITION_STEP = 1,
    RECTANGLE_TRANSITION_STEP_SIZE = 2,
    AVAILABLE_SCENE_TRANSITIONS = 4,
    RESOURCE_PATH = 5,
    RESOURCE_DATA = 6,
    RELEASE_RESOURCE = 7,
    SHARED_VALUE = 8,
    POINTER_X = 9,
    POINTER_Y = 10,
    RESOURCE_FRAME = 11,
    RESOURCE_STREAM_RATE_BYTES_PER_MILLISECOND = 12,
    BEGIN_SUSPENDED_TRANSITION = 13,
    END_SUSPENDED_TRANSITION = 14,
    BEGIN_PROPERTY_STATE = 16,
    END_PROPERTY_STATE = 32,
    DESTROY_TREE = 64,
    MISSING_SOURCE = 80,
    MISSING_SECTION = 96,
    BEGIN_NO_INVENTORY = 112,
    END_NO_INVENTORY = 128
};

enum RuntimeTreeNodeFlag : uint32_t
{
    RUNTIME_TREE_NO_SAVE = 0x00000100,
    RUNTIME_TREE_SECTION_FALLBACK_ENABLED = 0x00000200,
    RUNTIME_TREE_RESIDENT = 0x00000400,
    RUNTIME_TREE_COMMENT = 0x00000800,
    RUNTIME_TREE_INVENTORY_PACK = 0x00001000,
    RUNTIME_TREE_SOURCE_DEFINED = 0x00002000,
    RUNTIME_TREE_NO_CONTROL = 0x00004000,
    RUNTIME_TREE_ACTIVE = 0x00008000,
    RUNTIME_TREE_AUTO_CONTROL = 0x00010000,
    RUNTIME_TREE_NO_INVENTORY = 0x00020000
};

inline constexpr uint32_t RUNTIME_PLAN_INACTIVE = 0x80000000;

enum RuntimeVisualFlag : uint32_t
{
    RUNTIME_VISUAL_PRIMARY = 0x00000001,
    RUNTIME_VISUAL_RESOURCE_CHANGED = 0x00100000
};

enum RuntimeMovementFlag : uint32_t
{
    RUNTIME_MOVEMENT_ACTIVE = 0x00000001,
    RUNTIME_MOVEMENT_STOPPED = 0x00000002,
    RUNTIME_MOVEMENT_MASK = RUNTIME_MOVEMENT_ACTIVE | RUNTIME_MOVEMENT_STOPPED
};

enum RuntimePathFlag : uint32_t
{
    RUNTIME_PATH_HAS_LINE_DELTA = 0x00000001,
    RUNTIME_PATH_HAS_RADIUS = 0x00000002
};

inline constexpr uint32_t RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING = 0x80000000;

enum RuntimeScriptLinkOwnerFlag : uint32_t
{
    RUNTIME_SCRIPT_LINK_BACKEND_CHILD_ACTIVE = 0x00010000,
    RUNTIME_SCRIPT_LINK_TREE_FALLBACK = 0x10000000,
    RUNTIME_SCRIPT_LINK_SCENE_UPDATE_OPEN = 0x20000000,
    RUNTIME_SCRIPT_LINK_WAIT_PENDING = 0x40000000,
    RUNTIME_SCRIPT_LINK_ACTIVE = 0x80000000
};

enum RuntimeTreeActivationFlag : uint32_t
{
    RUNTIME_TREE_LOW_FLAG_MASK = 0x000000ff,
    RUNTIME_TREE_EXPLICIT_FLAGS = 0x10000000
};

inline constexpr uintptr_t RUNTIME_EVENT_ACKNOWLEDGED = 0x00020000;

enum RuntimeInteractionFlag : uint32_t
{
    RUNTIME_INTERACTION_COMMAND = 0x00000001,
    RUNTIME_INTERACTION_SOURCE = 0x00000002,
    RUNTIME_INTERACTION_DESTINATION = 0x00000004,
    RUNTIME_INTERACTION_ZONE = 0x00000008,
    RUNTIME_INTERACTION_RECTANGLE = 0x00000020,
    RUNTIME_INTERACTION_PRIMARY_RESOURCE = 0x00000040,
    RUNTIME_INTERACTION_RANDOM_RANGE = 0x00000080,
    RUNTIME_INTERACTION_CONDITION = 0x00000100,
    RUNTIME_INTERACTION_TRANSPARENT = 0x00000200,
    RUNTIME_INTERACTION_UP_DOWN = 0x00100000,
    RUNTIME_INTERACTION_NO_MATCHES = 0x00200000,
    RUNTIME_INTERACTION_PARENT_COMMAND = 0x10000000,
    RUNTIME_INTERACTION_VALUE_MASK = 0x0000000f,
    RUNTIME_INTERACTION_HIGH_STATE_MASK = 0xf0000000,
    RUNTIME_INTERACTION_STATE_MASK = 0xf000000f,
    RUNTIME_INTERACTION_CANDIDATE_MASK = 0xf0000fff,
    RUNTIME_INTERACTION_PRESERVED_STATE_MASK = 0x0ffffff0
};

struct AsyncFileRecord;

struct AsyncFileHost
{
    AsyncFileHost *self;
    uint32_t flags;
    AsyncFileHost *next;
    int32_t mode;
    RuntimeMutex *primary_lock;
    RuntimeMutex *secondary_lock;
    std::jthread *thread;
    uint32_t bytes_per_sector;
    uint32_t file_offset;
    std::shared_ptr<SharedBinaryFile> file;
    uint32_t file_size;
    uint32_t remaining_size;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t current_offset;
    void *buffer_start_cursor;
    uint32_t buffered_bytes;
    std::atomic<uint32_t> available_bytes;
    uint32_t buffer_size;
    void *read_cursor;
    void *write_cursor;
    void *secondary_cursor;
    void *buffer;
    AsyncFileRecord *active_file;
    AsyncFileRecord *files;
};

struct AsyncFileRecord
{
    AsyncFileRecord *self;
    uint32_t flags;
    AsyncFileRecord *next;
    std::shared_ptr<SharedBinaryFile> file;
    uint32_t file_size;
    uint32_t remaining_size;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t current_offset;
    uint32_t timestamp;
    void *buffer;
    void *buffer_cursor;
    uint32_t buffered_bytes;
    uint32_t previous_offset;
    uint32_t next_offset;
    AsyncFileHost *host;
};


using RuntimeCommandDefinition = RuntimeSceneSlot;


struct ScriptRuntimeRoot
{
    ScriptRuntimeRoot *self;
    uint32_t flags;
    uint32_t palette_flags;
    uintptr_t event_records[32][16];
    uint32_t transient_index_1;
    uint32_t transient_index_2;
    void (*set_property)(ScriptRuntimeProperty property, RuntimeGenericResourceNode *value);
    void (*get_property)(ScriptRuntimeProperty property, void **resource_data, void *result);
    RuntimeHeap *heap;
    uint32_t volume;
    uint32_t resource_variant;
    char language[0x20];
    char inventory_name[0x20];
    char exception_text[0x104];
    char default_auxiliary_names[0x104];
    uint32_t command_definition_count;
    RuntimeCommandDefinition command_definitions[32];
    RuntimeGenericResourceNode *generic_resources;
    RuntimeTreeNode *runtime_tree;
    ScriptObjectState *objects;
    RuntimeVisualObject *visual_objects;
    RuntimeNamedNode *runtime_nodes;
    RuntimeFixedNameListNode *fixed_name_nodes;
    union
    {
        RuntimePlanNode *plan_nodes;
        RuntimeTreePrimaryResourceLink *global_primary_resource_links;
    };
    RuntimeTreeLink7C *global_link_007c_head;
    union
    {
        RuntimeTreeLink84 *global_link_0084_head;
        RuntimePointerRegion *global_pointer_regions;
    };
    ScriptObjectContainer *containers;
    RuntimeTreeLink8C *global_link_008c_head;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_links;
    RuntimeTreeSceneLink *global_scene_links;
    union
    {
        RuntimePlanNode *plan_terminal;
        RuntimeTreePrimaryResourceLink *global_primary_resource_link_tail;
    };
    RuntimeTreeLink7C *global_link_007c_tail;
    union
    {
        RuntimeTreeLink84 *global_link_0084_tail;
        RuntimePointerRegion *global_pointer_region_tail;
    };
    ScriptObjectContainer *container_tail;
    RuntimeTreeLink8C *global_link_008c_tail;
    RuntimeTreeSecondaryResourceLink *global_secondary_resource_link_tail;
    RuntimeTreeSceneLink *global_scene_link_tail;
    ScriptTextBuffer *serialized_script;
};



struct RuntimeTreeParserContext
{
    RuntimeTreeNode *owner;
    char *name_pointer;
    char *creation_text_pointer;
    char *scratch_text_pointer;
    RuntimeGenericResourceNode *resource;
    void *resource_data;
    uint32_t resource_metadata;
    uint32_t start_offset;
    uint32_t cursor;
    RuntimeVisualObject *primary_visual;
    char creation_text[0x104];
    char scratch_text[0x104];
    char name[0x20];
    RuntimeTreeParserContext *next;
};


struct RuntimeTreeSceneLink
{
    char name[0x20];
    void *identity;
    uint32_t z;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t flags;
    intptr_t scene_identifier;
    RuntimeTreeSceneLink *next;
};


struct RuntimeTreePrimaryResourceLink
{
    char identifier[0x20];
    void *identity;
    RuntimeTreePrimaryResourceLink *next;
    uint32_t flags;
    char file_name[0x20];
    void *resource_identity;
    uint32_t movement_deadline;
    uint32_t image_flags;
    uint32_t loop_count;
    int32_t x;
    int32_t y;
    uint32_t source_value;
    uint32_t width;
    uint32_t height;
    uint32_t ratio_x;
    uint32_t ratio_y;
    RuntimeTreeSecondaryResourceLink *secondary_link;
    RuntimeFixedNameListNode *fixed_name_node;
    void *previous_resource_identity;
    int32_t previous_x;
    int32_t previous_y;
};

struct RuntimeTreeSecondaryResourceLink
{
    char name[0x20];
    void *identity;
    char file_name[0x20];
    void *resource_identity;
    RuntimeTreeSecondaryResourceLink *next;
};



struct RuntimeTreeLink84
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink84 *next;
    uint32_t movement_flags;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t movement_deadline;
    uint32_t command_mask;
    uint32_t primary_command_bit;
    uint32_t pointer_current_scene_bit;
    uint32_t parameter;
    union
    {
        RuntimeVisualObject *mouse_visual;
        uintptr_t mouse_visual_value;
    };
    uintptr_t owner_group_identity;
    union
    {
        ScriptObjectState *owner_object;
        void *owner_identity;
    };
    union
    {
        RuntimeTreePrimaryResourceLink *primary_resource;
        void *primary_resource_identity;
    };
    void *previous_owner_identity;
    void *previous_primary_resource_identity;
};

struct RuntimeTreeLink7C
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink7C *next;
    ScriptParserState parser;
    RuntimeGenericBackendChild *backend_child;
    void *fixed_resource_identity;
    void *secondary_resource_identity;
    uint32_t wait_deadline;
    uint32_t owner_flags;
    uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    uint32_t reserved_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t reserved_00a8;
    uint32_t flags;
    uint32_t reserved_00b0;
};

struct RuntimeTreeLink8C
{
    char name[0x20];
    void *identity;
    RuntimeTreeLink8C *next;
    uint32_t time;
    uint32_t flags;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    int32_t line_first;
    int32_t line_second;
};



struct RuntimeTreeInteractionCriteria
{
    uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    uint32_t reserved_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t reserved_00a8;
    uint32_t flags;
    uint32_t reserved_00b0;
    const RuntimeTreeLink7C *source_link;
};



} // namespace freegag
