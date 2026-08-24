#pragma once

#include "audio_types.h"
#include "binary_stream.h"
#include "runtime_services.h"

namespace gag
{
struct RuntimeSceneSwitchApi
{
    RuntimeLockRecord *(*acquire)(void *identity);
    void (*release)(RuntimeLockRecord *record);
    uint32_t (*offset_scene)(intptr_t identifier, int32_t x_delta, int32_t y_delta);
};

struct RuntimeDisplayResetApi
{
    void (*switch_scene)(void *identity);
    void (*set_script_flags)(uint32_t mask, int enabled);
    void (*reset_transient_indices)();
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
};

struct RuntimeDisplayShutdownApi
{
    RuntimeNamedNode *(*get_named_node)(const char *name);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
    uint32_t (*shutdown_host)();
    void (*teardown_surface)();
};

struct RuntimeResourceWaitApi
{
    void (*sleep)(uint32_t milliseconds);
};

struct RuntimeResourceHostApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    uint32_t (*destroy_host)(AsyncFileHost *host);
    AsyncFileHost *(*create_host)(const char *root, uint32_t requested_bytes, int32_t mode);
    void (*set_host_mode)(AsyncFileHost *host, int32_t mode);
    uint32_t (*close_archive)(CdfArchive *archive);
};

struct RuntimeResourceTypeApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    void (*update_host)(const char *path, int32_t reset);
    uint8_t (*get_archive_flags)(CdfArchive *archive, const char *name);
};

struct RuntimeCdfStreamApi
{
    int (*compare_names)(const char *left, const char *right);
};

struct ArchiveCommentEnumerationApi
{
    CdfArchive *(*open_archive)(const char *path, intptr_t alternate_stream);
    uint32_t (*get_error)(CdfArchive *archive);
    uint32_t (*get_entry_size)(CdfArchive *archive, uint8_t selector, const char *name);
    int (*read_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    uint32_t (*close_archive)(CdfArchive *archive);
};



struct RuntimeResourceLoadApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    RuntimeResourceCacheEntry *(*find_cache_entry)(void *parent_identity, const char *name);
    AsyncFileRecord *(*open_async_record)(AsyncFileHost *host, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags);
    uint32_t (*get_async_size)(AsyncFileRecord *record);
    int32_t (*activate_loading_scene)(const char *name);
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
    uint32_t (*read_async_record)(AsyncFileRecord *record, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);
    void (*deactivate_loading_scene)(const char *name);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    RuntimeResourceCacheEntry *(*get_or_create_cache_entry)(void *parent_identity, const char *name);
    uint8_t (*get_archive_flags)(CdfArchive *archive, const char *name);
    uint32_t (*get_archive_size)(CdfArchive *archive, uint8_t selector, const char *name);
    void *(*open_archive_stream)(CdfArchive *archive, const char *name);
    int (*read_archive_entry)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void (*sleep)(uint32_t milliseconds);
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
    std::shared_ptr<SharedBinaryInputState> file;
    uint32_t file_size;
    uint32_t remaining_size;
    uint32_t start_offset;
    uint32_t end_offset;
    uint32_t current_offset;
    void *buffer_start_cursor;
    uint32_t buffered_bytes;
    uint32_t available_bytes;
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
    std::shared_ptr<SharedBinaryInputState> file;
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


struct AsyncFileLockApi
{
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    void (*sleep)(uint32_t milliseconds);
};

struct AsyncFileHostApi
{
    uint32_t (*time_get_time)();
};

struct RuntimeNamedLockApi
{
    RuntimeThreadId (*get_current_thread_id)();
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
    void (*sleep)(uint32_t milliseconds);
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
    void (*set_property)(uint32_t operation, int32_t argument, RuntimeGenericResourceNode *node);
    void (*get_property)(uint32_t operation, void **resource_data, void *result);
    RuntimeHeap *heap;
    uint32_t parser_integer_0820;
    uint32_t state_value_0824;
    char language[0x20];
    char parser_value_0848[0x20];
    char parser_text_0868[0x104];
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



struct RuntimeGenericResourceLoadApi
{
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
};



struct RuntimeTreeParserContext
{
    RuntimeTreeNode *owner;
    char *name_pointer;
    char *creation_text_pointer;
    char *scratch_text_pointer;
    uint8_t unknown_0010[4];
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


struct RuntimeTreeParserContextApi
{
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
};



struct RuntimeTreeParserReleaseApi
{
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
    void (*remove_resource)(void *identity);
};



struct RuntimeTreeCreationApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    RuntimeGenericResourceNode *(*find_resource)(void *identity);
    void *(*find_root_by_name)(const void *name);
    RuntimeTreeNode *(*find_ancestor_root)(void *identity);
    void *(*find_descendant_by_name)(void *root_identity, const void *name);
    int (*find_section)(const char *section_name, const char *text, int text_length);
    int (*find_property)(char *value, const char *property_name, const char *text, uint32_t text_length, uint32_t start_offset);
    RuntimeTreeNode *(*begin_enumeration)(void *identity);
    RuntimeTreeNode *(*next_enumeration)(RuntimeTreeNode *root);
    void *(*heap_alloc)(RuntimeHeap *heap, uint32_t flags, size_t bytes);
    bool (*heap_free)(RuntimeHeap *heap, uint32_t flags, void *memory);
    RuntimeTreeParserContext *(*create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);
    void (*remove_resource)(void *identity);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*activate_node)(RuntimeTreeNode *node);
};



struct RuntimeTreeJumpApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    uint32_t (*parse_name)(ScriptParserState *parser, char *name, uint32_t capacity);
    void (*synchronize_owner)(RuntimeTreeNode *owner);
    RuntimeGenericResourceNode *(*find_or_load_resource)(const char *name);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
};



struct RuntimeTreeConditionalCreateApi
{
    bool (*compare_field)(const char *object_name, const void *field_name, const void *value, int32_t value_type);
    bool (*container_matches)(const void *name);
    RuntimeTreeNode *(*find_descendant)(void *root_identity, const void *name);
    RuntimeGenericResourceNode *(*load_resource)(const char *name);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
    RuntimeTreeNode *(*destroy_node)(void *identity, void *replacement_identity);
};



struct RuntimeTreeParserResetApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    RuntimeTreeNode *(*resolve_included_tree)(ScriptParserState *parser);
    RuntimeTreeNode *(*find_node)(void *identity);
};

struct RuntimeTreeParserDirectDispatchApi
{
    uint32_t (*parse_property)(ScriptParserState *parser);
    uint32_t (*parse_object)(ScriptParserState *parser);
    uint32_t (*parse_link_0084)(ScriptParserState *parser);
    uint32_t (*parse_link_007c)(ScriptParserState *parser);
    uint32_t (*parse_visual)(ScriptParserState *parser);
    uint32_t (*parse_primary)(ScriptParserState *parser);
    uint32_t (*parse_container)(ScriptParserState *parser);
    uint32_t (*parse_command)(ScriptParserState *parser);
    uint32_t (*parse_named)(ScriptParserState *parser);
    uint32_t (*parse_link_008c)(ScriptParserState *parser);
    RuntimeTreeNode *(*create_conditional)(ScriptParserState *parser);
    uint32_t (*parse_auxiliary_names)(ScriptParserState *parser);
    uint32_t (*create_fixed_name)(ScriptParserState *parser);
    bool (*parse_language)(ScriptParserState *parser);
    uint32_t (*parse_secondary)(ScriptParserState *parser);
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*apply_image_flags)(ScriptParserState *parser);
    uintptr_t (*dispatch_section)(ScriptParserState *parser);
    void (*set_resource_position)(void *identity, uint32_t position);
    uint32_t (*read_resource_token)(void *identity, char *output, uint32_t capacity, uint8_t delimiter);
    uint32_t (*parse_scene)(ScriptParserState *parser);
    void (*add_auxiliary_name)(RuntimeTreeNode *owner, const char *name);
    void (*publish_links)(RuntimeTreeNode *owner);
};


struct RuntimeTreeParserSpecialDispatchApi
{
    int32_t (*parse_integer)(ScriptParserState *parser);
    uint32_t (*parse_image_flag)(ScriptParserState *parser);
    RuntimeTreeNode *(*create_command)(ScriptParserState *parser);
    RuntimeTreeNode *(*find_jump)(ScriptParserState *parser, const char *property_name, uint32_t cursor);
    bool (*strings_equal)(const char *left, const char *right);
};



struct RuntimeTreeSectionDispatchApi
{
    RuntimeTreeNode *(*find_node)(void *identity);
    RuntimeGenericResourceNode *(*find_resource)(void *identity);
    int (*find_section)(const char *section_name, const char *text, int text_length);
    RuntimeTreeParserContext *(*create_parser_context)(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text);
    RuntimeTreeNode *(*dispatch_parser)(RuntimeTreeParserContext *context);
    void (*remove_resource)(void *identity);
};



struct RuntimeTreeBasicCommandApi
{
    uint32_t (*parse_value)(ScriptParserState *parser, char *value, uint32_t capacity);
    uint32_t (*extract_parenthesized)(ScriptParserState *parser, char *text, uint32_t capacity);
    uint32_t (*parse_scope)(ScriptParserState *parser);
    RuntimeGenericResourceNode *(*load_resource)(const char *name);
    RuntimeTreeNode *(*dispatch_section)(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text);
    RuntimeTreeNode *(*create_node)(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context);
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
    uint8_t unknown_0048[4];
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
    uint8_t unknown_0028[0x10];
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
    uint32_t unknown_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t unknown_00a8;
    uint32_t flags;
    uint32_t unknown_00b0;
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
    uint8_t unknown_0040[0x0c];
    int32_t line_first;
    int32_t line_second;
};



struct RuntimeTreeInteractionCriteria
{
    uint32_t command_bit;
    ScriptObjectState *source_object;
    ScriptObjectState *destination_object;
    RuntimeTreeLink84 *zone_link;
    uint32_t unknown_0084;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    RuntimeTreePrimaryResourceLink *primary_resource;
    ScriptObjectContainer *condition;
    int32_t random_minimum;
    int32_t random_maximum;
    uint32_t unknown_00a8;
    uint32_t flags;
    uint32_t unknown_00b0;
    const RuntimeTreeLink7C *source_link;
};



struct RuntimeTreeDestructionApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    void (*set_resource_state)(void *identity, uint32_t state);
    uint32_t (*stop_game_dll)();
    void (*reset_display_state)();
    void *(*find_primary_tail)(void *identity);
    void *(*find_secondary_tail)(void *identity);
    void *(*find_scene_tail)(void *identity);
    uint32_t (*query_scene_flags)(void *identity);
    void (*destroy_resource_and_scene)(void *identity);
    void (*request_resource_destruction)(void *identity);
    uint32_t (*release_scene)(intptr_t identifier, intptr_t owner);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void (*wait_for_resource_count)(uint32_t count);
};



struct RuntimeResourceSceneDestructionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*destroy_resource)(void *identity);
    void (*release_record)(RuntimeLockRecord *record);
    void (*sleep)(uint32_t milliseconds);
    void (*update_scene_region)(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height);
};

struct RuntimeResourceSceneRegionApi
{
    DisplaySceneNode *(*lock_scene)(intptr_t identifier);
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*begin_scene_update)(intptr_t identifier);
    uint32_t (*render_backend_region)(void *backend_identity, DisplayRectangle *rectangle);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    uint32_t (*update_root_scene_region)(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value);
    void (*release_record)(RuntimeLockRecord *record);
    void (*unlock_scene)(intptr_t identifier);
};



struct RuntimeBitmapRegionRenderApi
{
    void (*wait_for_single_object)(RuntimeMutex *mutex, uint32_t milliseconds);
    void (*release_mutex)(RuntimeMutex *mutex);
    void (*copy_bitmap_region)(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);
};



struct RuntimeSceneTransitionSelectionApi
{
    int (*random)();
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    void (*apply_palette)(uint32_t value, uint32_t flags);
    void (*apply_rectangle)(uint8_t value, uint32_t flags);
};



struct RuntimeResourceStateApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*begin_scene_update)(intptr_t identifier);
    void (*finalize_backend)(void *identity);
    void (*configure_palette)(RuntimeResourceObject *resource);
    uint32_t (*end_scene_update)(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle);
    void (*clear_child_ready)(void *identity);
    void (*enable_child_mode)(void *identity);
    void (*disable_child_mode)(void *identity);
    void (*select_transition)(uint32_t flags);
    uint32_t (*restart_sound_data)(uint32_t handle);
    uint32_t (*start_sound)(uint32_t handle, int32_t reset_timing);
    uint32_t (*stop_sound)(uint32_t handle, int32_t reset_timing);
    void (*release_record)(RuntimeLockRecord *record);
};



struct RuntimeImmediateSceneTransitionApi
{
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*release_display_lock)();
    RuntimeLockRecord *(*acquire_record)(void *identity);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    void (*sleep)(uint32_t milliseconds);
    void (*synchronize_region)(DisplayRectangle *rectangle, uint32_t mode);
    uint32_t (*apply_palette)(const PaletteEntry *entries, uint32_t flags);
    void (*release_record)(RuntimeLockRecord *record);
};



struct RuntimePaletteSceneTransitionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    uint32_t (*apply_palette)(const PaletteEntry *entries, uint32_t flags);
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    uint32_t (*release_display_lock)();
    void (*release_record)(RuntimeLockRecord *record);
    uint32_t (*time_get_time)();
    void (*sleep)(uint32_t milliseconds);
    void (*invalidate_framebuffer)(int32_t x, int32_t y, int32_t width, int32_t height);
};



struct RuntimeRectangleSceneTransitionApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*apply_immediate)(uint32_t unused, uint32_t flags);
    uint32_t (*acquire_display_lock)(DisplayRectangle *primary, DisplayRectangle *secondary, uint32_t *flags);
    uint32_t (*set_clip_rectangle)(DisplayRectangle *rectangle);
    uint32_t (*release_display_lock)();
    void (*operate_surface)(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);
    void (*synchronize_region)(DisplayRectangle *rectangle, uint32_t mode);
    uint32_t (*apply_palette)(const PaletteEntry *entries, uint32_t flags);
    uint32_t (*dispatch_scene_update)(void *rectangle, uint32_t flags);
    uint32_t (*time_get_time)();
    uint32_t (*get_tick_count)();
    void (*sleep)(uint32_t milliseconds);
    void (*release_record)(RuntimeLockRecord *record);
};



struct RuntimePointerResourceRebuildApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    uint32_t (*synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    uint32_t (*query_scene_flags)(void *identity);
    void (*finalize_destruction)(void *identity);
    void (*request_destruction)(void *identity);
    RuntimeResourceConstructor construct_resource;
    void (*update_position)(void *identity, int32_t x, int32_t y);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void (*wait_for_count)(uint32_t count);
};

struct RuntimeTreeResourceRebuildApi
{
    RuntimeTreeNode *(*resolve_tree)(void *identity);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    RuntimeResourceConstructor construct_resource;
    uint32_t (*synchronize_owner)(void *owner_identity, void *tree_identity, RuntimePointerRegion *region);
    uint32_t (*query_scene_flags)(void *identity);
    void (*request_destruction)(void *identity);
    RuntimeGenericBackendChild *(*configure_resource)(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, uintptr_t value, uint32_t flags);
    void (*set_comment_mode)(RuntimeTreeNode *root, int enabled);
    void (*wait_for_count)(uint32_t count);
    void (*reset_byte_queue)();
    void (*reset_pair_queue)();
    void (*reset_transient_indices)();
    void (*set_resource_state)(void *identity, uint32_t state);
};

struct RuntimeGenericChildAttachmentApi
{
    RuntimeLockRecord *(*acquire_resource)(void *identity);
    void (*release_resource)(RuntimeLockRecord *record);
    uint32_t (*find_scene_index)(uint32_t candidate);
    RuntimeGenericBackendChild *(*create_child)(void *backend_identity, void *font_identity, const uintptr_t *context, uintptr_t selection, uint32_t flags);
    DisplaySceneNode *(*lock_scene)(intptr_t identifier);
    void (*unlock_scene)(intptr_t identifier);
    DisplaySceneNode *(*acquire_scene)(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
        const DisplayPixelFormatDescriptor *format);
    void *(*destroy_child)(void *identity);
};



struct RuntimePointerRefreshApi
{
    uint32_t (*update_region)(int32_t x, int32_t y);
};



struct RuntimeCommentTreeCleanupApi
{
    RuntimeTreeNode *(*begin_enumeration)(void *identity);
    RuntimeTreeNode *(*next_node)(RuntimeTreeNode *root);
    void (*destroy_resources)(void *identity);
    intptr_t (*deactivate_node)(void *identity, void *second);
    void (*finalize_destroyed_nodes)(void *identity);
    void (*rebuild_runtime_plans)();
};


struct RuntimeTreeDeactivateApi
{
    RuntimeTreeNode *(*resolve_identity)(void *identity);
    void (*request_resource_destruction)(void *identity);
    bool (*remove_visual_object)(void *identity);
    void (*set_script_flags)(uint32_t flags, int enabled);
    void (*deactivate_comment)(RuntimeTreeNode *node);
    intptr_t (*destroy_tree)(void *first, void *second);
};



struct RuntimeResourceLoopApi
{
    RuntimeLockRecord *(*acquire_record)(void *identity);
    void (*set_sound_loop)(uint32_t handle, uint32_t value);
    void (*release_record)(RuntimeLockRecord *record);
};



struct AsyncFileShutdownApi
{
    uint32_t (*destroy_host)(AsyncFileHost *identity);
    void (*enter_critical_section)(RuntimeMutex *mutex);
    void (*leave_critical_section)(RuntimeMutex *mutex);
};



struct RuntimeMediaBackendShutdownApi
{
    RuntimeMediaBackend *(*acquire_first_backend)();
    void (*release_backend_lock)(RuntimeMediaBackend *backend);
    bool (*heap_destroy)(RuntimeHeap *heap);
    uint32_t (*shutdown_sound)();
};



} // namespace gag
