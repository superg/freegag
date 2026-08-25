#include "runtime_model.h"
#include "host_events.h"
#include "runtime_internal.h"

namespace freegag
{
static_assert(offsetof(RuntimePointerRegion, left) == offsetof(RuntimeTreeLink84, x));
static_assert(offsetof(RuntimePointerRegion, top) == offsetof(RuntimeTreeLink84, y));
static_assert(offsetof(RuntimePointerRegion, right) == offsetof(RuntimeTreeLink84, width));
static_assert(offsetof(RuntimePointerRegion, bottom) == offsetof(RuntimeTreeLink84, height));
static_assert(offsetof(RuntimePointerRegion, scene_mask) == offsetof(RuntimeTreeLink84, command_mask));
static_assert(offsetof(RuntimePointerRegion, first_scene_bit) == offsetof(RuntimeTreeLink84, primary_command_bit));
static_assert(offsetof(RuntimePointerRegion, current_scene_bit) == offsetof(RuntimeTreeLink84, pointer_current_scene_bit));
static_assert(offsetof(RuntimePointerRegion, priority) == offsetof(RuntimeTreeLink84, parameter));
static_assert(offsetof(RuntimePointerRegion, visual_override) == offsetof(RuntimeTreeLink84, mouse_visual));
static_assert(offsetof(RuntimePointerRegion, owner_identity) == offsetof(RuntimeTreeLink84, owner_group_identity));
static_assert(offsetof(RuntimePointerRegion, state_object) == offsetof(RuntimeTreeLink84, owner_object));
static_assert(offsetof(RuntimePointerRegion, primary_resource) == offsetof(RuntimeTreeLink84, primary_resource));
static_assert(sizeof(RuntimePointerRegion) == sizeof(RuntimeTreeLink84));

void remove_runtime_generic_resource(void *identity)
{
    if(script_runtime_root == nullptr || script_runtime_root->generic_resources == nullptr)
        return;
    RuntimeGenericResourceNode *previous = nullptr;
    RuntimeGenericResourceNode *node = script_runtime_root->generic_resources;
    while(node->identity != identity)
    {
        previous = node;
        node = node->next;
        if(node == nullptr)
            return;
    }
    if(node->active_references == 0)
    {
        script_runtime_root->set_property(ScriptRuntimeProperty::RELEASE_RESOURCE, node);
        if(previous == nullptr)
            script_runtime_root->generic_resources = node->next;
        else
            previous->next = node->next;
        free_runtime_heap(script_runtime_root->heap, 0, node);
    }
}

RuntimeNamedNode *find_runtime_named_child(void *parent_identity, void *child_identity)
{
    for(RuntimeNamedNode *parent = script_runtime_root->runtime_nodes; parent != nullptr; parent = parent->next)
    {
        if(parent->identity == parent_identity)
        {
            for(RuntimeNamedNode *child = parent->children; child != nullptr; child = child->next)
            {
                if(child->identity == child_identity)
                    return child;
                if(parent->child_sentinel == child)
                    return nullptr;
            }
            return nullptr;
        }
    }
    return nullptr;
}

RuntimeResourceCacheEntry *find_runtime_resource_cache_entry(void *parent_identity, const char *name)
{
    for(RuntimeNamedNode *parent = script_runtime_root->runtime_nodes; parent != nullptr; parent = parent->next)
    {
        if(parent->identity == parent_identity)
        {
            auto *entry = parent->cache_entries;
            auto *sentinel = parent->cache_entry_sentinel;
            while(entry != nullptr)
            {
                if(strings_equal(entry->name, name))
                    return entry;
                if(entry == sentinel)
                    return nullptr;
                entry = entry->next;
            }
            return nullptr;
        }
    }
    return nullptr;
}

void append_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry)
{
    ++parent->status;
    auto *head = parent->cache_entries;
    auto *tail = parent->cache_entry_sentinel;
    if(head == nullptr)
    {
        parent->cache_entries = entry;
        parent->cache_entry_sentinel = entry;
        parent->cache_entry_cursor = entry;
        head = entry;
        tail = entry;
    }
    else
    {
        tail->next = entry;
        head->previous = entry;
    }
    entry->next = head;
    entry->previous = tail;
    parent->cache_entry_sentinel = entry;
}

void remove_runtime_named_child(RuntimeNamedNode *parent, RuntimeResourceCacheEntry *entry)
{
    if(parent->cache_entry_cursor == entry)
        parent->cache_entry_cursor = entry->next;
    if(parent->cache_entries == entry)
        parent->cache_entries = entry->next;
    if(parent->cache_entry_sentinel == entry)
        parent->cache_entry_sentinel = entry->previous;
    if(entry != entry->previous)
        entry->previous->next = entry->next;
    if(entry == entry->next)
    {
        parent->cache_entry_cursor = nullptr;
        parent->cache_entries = nullptr;
        parent->cache_entry_sentinel = nullptr;
    }
    else
    {
        entry->next->previous = entry->previous;
    }
    free_runtime_heap(script_runtime_root->heap, 0, entry);
    --parent->status;
}

uint32_t remove_runtime_named_child_by_identity(void *parent_identity, void *child_identity)
{
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && parent->identity != parent_identity)
        parent = parent->next;
    if(parent == nullptr)
        return 0;
    auto *entry = parent->cache_entries;
    const auto *sentinel = parent->cache_entry_sentinel;
    while(entry != nullptr)
    {
        if(entry->data == child_identity)
        {
            remove_runtime_named_child(parent, entry);
            return 1;
        }
        if(entry == sentinel)
            return 0;
        entry = entry->next;
    }
    return 0;
}

RuntimeResourceCacheEntry *get_or_create_runtime_resource_cache_entry(void *parent_identity, const char *name)
{
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && parent->identity != parent_identity)
        parent = parent->next;
    if(parent == nullptr)
        return nullptr;
    RuntimeResourceCacheEntry *entry = find_runtime_resource_cache_entry(parent_identity, name);
    if(entry != nullptr)
        return entry;
    entry = static_cast<RuntimeResourceCacheEntry *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeResourceCacheEntry)));
    std::memcpy(entry->name, name, sizeof(entry->name));
    append_runtime_named_child(parent, entry);
    return entry;
}

RuntimeResourceCacheEntry *get_or_create_runtime_child_by_data(void *parent_identity, void *data)
{
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && parent->identity != parent_identity)
        parent = parent->next;
    if(parent == nullptr)
        return nullptr;
    auto *entry = parent->cache_entries;
    const auto *sentinel = parent->cache_entry_sentinel;
    while(entry != nullptr)
    {
        if(entry->data == data)
            return entry;
        if(entry == sentinel)
            break;
        entry = entry->next;
    }
    entry = static_cast<RuntimeResourceCacheEntry *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeResourceCacheEntry)));
    entry->data = data;
    append_runtime_named_child(parent, entry);
    return entry;
}

void add_script_object_to_runtime_named_node(const void *node_name, const char *object_name)
{
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && !fixed_dword_memory_equal(node_name, parent->name, 0x20))
        parent = parent->next;
    if(parent == nullptr)
        return;

    ScriptObjectState *object = script_runtime_root->objects;
    while(object != nullptr && !strings_equal(object_name, object->name))
        object = object->next;
    if(object == nullptr)
        return;

    auto *entry = parent->cache_entries;
    const auto *sentinel = parent->cache_entry_sentinel;
    while(entry != nullptr)
    {
        if(entry->data == object->identity)
            return;
        if(entry == sentinel)
            break;
        entry = entry->next;
    }

    entry = static_cast<RuntimeResourceCacheEntry *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeResourceCacheEntry)));
    entry->data = object->identity;
    std::memcpy(entry->name, object->name, sizeof(entry->name));
    append_runtime_named_child(parent, entry);
}

uint32_t parse_runtime_named_node(ScriptParserState *parser)
{
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == SCRIPT_PARSE_END)
        return 0;

    RuntimeNamedNode *previous = nullptr;
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && !fixed_dword_memory_equal(value, parent->name, 0x20))
    {
        previous = parent;
        parent = parent->next;
    }
    if(parent == nullptr)
    {
        parent = static_cast<RuntimeNamedNode *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeNamedNode)));
        if(parent == nullptr)
            return 0;
        std::memcpy(parent->name, value, sizeof(parent->name));
        parent->identity = parent;
        if(previous == nullptr)
            script_runtime_root->runtime_nodes = parent;
        else
            previous->next = parent;
    }

    for(;;)
    {
        uint32_t result = parse_script_value_token(parser, value, 0x20);
        if(result == SCRIPT_PARSE_END)
        {
            result = parse_script_scope_code(parser);
            if(result == SCRIPT_SCOPE_FLAGS)
            {
                result = parse_image_flag(parser);
                if(result == SCRIPT_PARSE_END)
                    return 0;
                parent->flags |= result;
            }
            else if(result == SCRIPT_SCOPE_ZONE)
            {
                int32_t integer = parse_script_integer_expression(parser);
                if(integer != SCRIPT_INTEGER_INVALID)
                    parent->zone_left = integer;
                integer = parse_script_integer_expression(parser);
                if(integer != SCRIPT_INTEGER_INVALID)
                    parent->zone_top = integer;
                integer = parse_script_integer_expression(parser);
                parent->visible_entry_count = static_cast<uint32_t>(integer);
                if(integer == SCRIPT_INTEGER_INVALID)
                    parent->visible_entry_count = 1;
                integer = parse_script_integer_expression(parser);
                if(integer != SCRIPT_INTEGER_INVALID)
                    parent->zone_right = integer;
                integer = parse_script_integer_expression(parser);
                result = static_cast<uint32_t>(integer);
                if(integer != SCRIPT_INTEGER_INVALID)
                    parent->zone_bottom = integer;
            }
        }
        else
        {
            ScriptObjectState *object = script_runtime_root->objects;
            while(object != nullptr && !fixed_dword_memory_equal(value, object->name, 0x20))
                object = object->next;
            if(object != nullptr)
            {
                auto *entry = parent->cache_entries;
                const auto *sentinel = parent->cache_entry_sentinel;
                while(entry != nullptr && entry->data != object->identity && entry != sentinel)
                    entry = entry->next;
                if(entry == nullptr || entry->data != object->identity)
                {
                    entry = static_cast<RuntimeResourceCacheEntry *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeResourceCacheEntry)));
                    entry->data = object->identity;
                    std::memcpy(entry->name, object->name, sizeof(entry->name));
                    append_runtime_named_child(parent, entry);
                }
            }
        }
        if(result == SCRIPT_PARSE_END)
            return 0;
    }
}

void remove_script_object_from_runtime_named_node(const void *node_name, const char *object_name)
{
    RuntimeNamedNode *parent = script_runtime_root->runtime_nodes;
    while(parent != nullptr && !fixed_dword_memory_equal(node_name, parent->name, 0x20))
        parent = parent->next;
    if(parent == nullptr)
        return;

    ScriptObjectState *object = script_runtime_root->objects;
    while(object != nullptr && !strings_equal(object_name, object->name))
        object = object->next;
    if(object == nullptr)
        return;

    auto *entry = parent->cache_entries;
    const auto *sentinel = parent->cache_entry_sentinel;
    while(entry != nullptr)
    {
        if(entry->data == object->identity)
        {
            remove_runtime_named_child(parent, entry);
            return;
        }
        if(entry == sentinel)
            return;
        entry = entry->next;
    }
}

uint32_t rotate_runtime_named_node_cursor_previous(const void *node_name, int32_t count)
{
    RuntimeNamedNode *node = script_runtime_root->runtime_nodes;
    while(node != nullptr && !fixed_dword_memory_equal(node_name, node->name, 0x20))
        node = node->next;
    if(node == nullptr)
        return 0;
    if(node->status <= node->visible_entry_count)
        return 1;
    while(count != 0)
    {
        node->cache_entry_cursor = node->cache_entry_cursor->previous;
        --count;
    }
    return 1;
}

uint32_t rotate_runtime_named_node_cursor_next(const void *node_name, int32_t count)
{
    RuntimeNamedNode *node = script_runtime_root->runtime_nodes;
    while(node != nullptr && !fixed_dword_memory_equal(node_name, node->name, 0x20))
        node = node->next;
    if(node == nullptr)
        return 0;
    if(node->status <= node->visible_entry_count)
        return 1;
    while(count != 0)
    {
        node->cache_entry_cursor = node->cache_entry_cursor->next;
        --count;
    }
    return 1;
}

uint32_t clear_runtime_named_node_children(const void *node_name)
{
    RuntimeNamedNode *node = script_runtime_root->runtime_nodes;
    while(node != nullptr && !fixed_dword_memory_equal(node_name, node->name, 0x20))
        node = node->next;
    if(node == nullptr)
        return 0;
    while(node->cache_entries != nullptr)
        remove_runtime_named_child(node, node->cache_entries);
    return 1;
}

void serialize_runtime_named_nodes(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr || script_runtime_root->runtime_nodes == nullptr)
        return;
    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    for(RuntimeNamedNode *node = script_runtime_root->runtime_nodes; node != nullptr; node = node->next)
    {
        append_script_text_property(buffer, 8, node->name);
        auto *entry = node->cache_entries;
        do
        {
            if(entry == nullptr)
                break;
            append_script_text_delimiter(buffer, entry->name, ' ');
            entry = entry->next;
        } while(entry != node->cache_entries);
        append_script_text_scope(buffer, 0x0f000000);
        append_script_text_integer(buffer, node->zone_left, ',');
        append_script_text_integer(buffer, node->zone_top, ',');
        append_script_text_integer(buffer, node->visible_entry_count, ',');
        append_script_text_integer(buffer, node->zone_right, ',');
        append_script_text_integer(buffer, node->zone_bottom, ' ');
        end_script_text_statement(buffer);
    }
}

void purge_disabled_runtime_named_nodes()
{
    if(script_runtime_root == nullptr)
        return;
    RuntimeNamedNode *retained_head = nullptr;
    RuntimeNamedNode *retained_tail = nullptr;
    RuntimeNamedNode *node = script_runtime_root->runtime_nodes;
    while(node != nullptr)
    {
        RuntimeNamedNode *next = node->next;
        if((node->flags & RUNTIME_NAMED_NODE_ENABLED) == 0)
        {
            auto *entry = node->cache_entries;
            for(uint32_t index = 0; index < node->status; ++index)
            {
                RuntimeResourceCacheEntry *next_entry = entry->next;
                free_runtime_heap(script_runtime_root->heap, 0, entry);
                entry = next_entry;
            }
            free_runtime_heap(script_runtime_root->heap, 0, node);
        }
        else
        {
            if(retained_head == nullptr)
                retained_head = node;
            if(retained_tail != nullptr)
                retained_tail->next = node;
            node->next = nullptr;
            retained_tail = node;
        }
        node = next;
    }
    script_runtime_root->runtime_nodes = retained_head;
}

RuntimeNamedNode *get_or_create_runtime_named_node(const char *name)
{
    RuntimeNamedNode *last = script_runtime_root->runtime_nodes;
    for(RuntimeNamedNode *node = last; node != nullptr; node = node->next)
    {
        last = node;
        if(strings_equal(name, node->name))
            return static_cast<RuntimeNamedNode *>(node->identity);
    }
    auto *node = static_cast<RuntimeNamedNode *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeNamedNode)));
    if(node == nullptr)
        return nullptr;
    int32_t index = 0;
    while(name[index] != '\0')
    {
        node->name[index] = name[index];
        ++index;
    }
    node->identity = node;
    if(last != nullptr)
        last->next = node;
    else
        script_runtime_root->runtime_nodes = node;
    return static_cast<RuntimeNamedNode *>(node->identity);
}

bool set_runtime_plans_inactive()
{
    bool changed = false;
    script_runtime_root->flags |= SCRIPT_RUNTIME_PLANS_INACTIVE;
    if(script_runtime_root->plan_terminal != nullptr)
    {
        for(RuntimePlanNode *node = script_runtime_root->plan_nodes; node != nullptr; node = node->next)
        {
            if((node->flags & RUNTIME_PLAN_INACTIVE) == 0)
            {
                node->flags |= RUNTIME_PLAN_INACTIVE;
                changed = true;
            }
            if(script_runtime_root->plan_terminal == node)
                return changed;
        }
    }
    return changed;
}

bool clear_runtime_plans_inactive()
{
    bool changed = false;
    script_runtime_root->flags &= ~SCRIPT_RUNTIME_PLANS_INACTIVE;
    if(script_runtime_root->plan_terminal != nullptr)
    {
        for(RuntimePlanNode *node = script_runtime_root->plan_nodes; node != nullptr; node = node->next)
        {
            if((node->flags & RUNTIME_PLAN_INACTIVE) != 0)
            {
                node->flags &= ~RUNTIME_PLAN_INACTIVE;
                changed = true;
            }
            if(script_runtime_root->plan_terminal == node)
                return changed;
        }
    }
    return changed;
}

uint32_t parse_runtime_visual_object(ScriptParserState *parser)
{
    bool invert_no_palette = false;
    bool parsed_file = false;
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == SCRIPT_PARSE_END)
        return 0;

    RuntimeVisualObject *previous = nullptr;
    RuntimeVisualObject *object = script_runtime_root->visual_objects;
    while(object != nullptr && !fixed_dword_memory_equal(value, object->name, 0x20))
    {
        previous = object;
        object = object->next;
    }
    if(object == nullptr)
    {
        object = static_cast<RuntimeVisualObject *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeVisualObject)));
        if(object == nullptr)
            return 0;
        if(previous == nullptr)
            script_runtime_root->visual_objects = object;
        else
            previous->next = object;
        std::memcpy(object->name, value, sizeof(object->name));
        object->identity = object;
    }

    if(object->serialized_file[0] != '\0')
        std::memset(object->serialized_file, 0, sizeof(object->serialized_file));
    object->palette_flags = script_runtime_root->palette_flags;
    object->flags |= RUNTIME_VISUAL_RESOURCE_CHANGED;
    for(;;)
    {
        uint32_t code = parse_script_scope_code(parser);
        if(code == 0x01000000)
        {
            const uint32_t file_result = parse_script_file_value(parser, value, object->serialized_file);
            if(file_result != SCRIPT_PARSE_END)
            {
                parsed_file = true;
                if(!fixed_dword_memory_equal(value, object->file_name, 0x20))
                {
                    if(object->scene_identity != nullptr)
                        object->previous_scene_identity = object->scene_identity;
                    object->scene_identity = nullptr;
                    std::memcpy(object->file_name, value, sizeof(object->file_name));
                }
                else
                {
                    object->flags &= ~RUNTIME_VISUAL_RESOURCE_CHANGED;
                }
            }
        }
        else if(code == 0x00060000)
        {
            invert_no_palette = true;
        }
        else if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 1)
            {
                for(RuntimeVisualObject *current = script_runtime_root->visual_objects; current != nullptr; current = current->next)
                    current->flags &= ~RUNTIME_VISUAL_PRIMARY;
                object->flags |= RUNTIME_VISUAL_PRIMARY;
                parser->primary_visual = object;
            }
            else
            {
                object->palette_flags |= code;
            }
        }
        else if(code == 0x0b000000)
        {
            const int32_t x = parse_script_integer_expression(parser);
            if(x != SCRIPT_INTEGER_INVALID)
                object->position_x = x;
            const int32_t y = parse_script_integer_expression(parser);
            if(y != SCRIPT_INTEGER_INVALID)
                object->position_y = y;
            code = static_cast<uint32_t>(y);
        }
        if(code == SCRIPT_PARSE_END)
            break;
    }
    if(invert_no_palette)
        object->palette_flags ^= SCRIPT_IMAGE_NO_PALETTE;
    if(!parsed_file)
    {
        if(object->file_name[0] != '\0')
            std::memset(object->file_name, 0, sizeof(object->file_name));
        if(object->scene_identity != nullptr)
            object->previous_scene_identity = object->scene_identity;
        object->scene_identity = nullptr;
    }
    return 0;
}

void *create_or_update_runtime_visual_object(const void *name, const void *file_name, int32_t position_x, int32_t position_y, uint32_t flags, uint32_t palette_flags)
{
    RuntimeVisualObject *previous = nullptr;
    RuntimeVisualObject *object = script_runtime_root->visual_objects;
    while(object != nullptr && !fixed_dword_memory_equal(name, object->name, 0x20))
    {
        previous = object;
        object = object->next;
    }
    if(object != nullptr)
    {
        object->previous_scene_identity = object->scene_identity;
        object->scene_identity = nullptr;
    }
    else
    {
        object = static_cast<RuntimeVisualObject *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeVisualObject)));
        if(object == nullptr)
            return nullptr;
        if(previous == nullptr)
            script_runtime_root->visual_objects = object;
        else
            previous->next = object;
        std::memcpy(object->name, name, sizeof(object->name));
        object->identity = object;
    }

    object->flags |= RUNTIME_VISUAL_RESOURCE_CHANGED;
    if(!fixed_dword_memory_equal(file_name, object->file_name, 0x20))
    {
        std::memcpy(object->file_name, file_name, sizeof(object->file_name));
    }
    else
    {
        object->previous_scene_identity = nullptr;
        object->flags &= ~RUNTIME_VISUAL_RESOURCE_CHANGED;
    }
    object->position_x = position_x;
    object->position_y = position_y;
    if((flags & SCRIPT_IMAGE_PRIMARY) != 0)
    {
        for(RuntimeVisualObject *current = script_runtime_root->visual_objects; current != nullptr; current = current->next)
            current->flags &= ~RUNTIME_VISUAL_PRIMARY;
        object->flags |= RUNTIME_VISUAL_PRIMARY;
    }
    object->palette_flags |= palette_flags;
    return object->identity;
}

void serialize_runtime_visual_objects(ScriptTextBuffer *buffer)
{
    RuntimeVisualObject *object = script_runtime_root->visual_objects;
    if(object == nullptr)
        return;
    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    for(; object != nullptr; object = object->next)
    {
        append_script_text_property(buffer, 4, object->name);
        if(object->serialized_file[0] != '\0')
        {
            append_script_text_scoped_tokens(buffer, 0x01000000, object->serialized_file);
            if(object->position_x != 0 || object->position_y != 0)
            {
                append_script_text_scope(buffer, 0x0b000000);
                append_script_text_integer(buffer, object->position_x, ',');
                append_script_text_integer(buffer, object->position_y, ' ');
            }
        }
        serialize_image_flag_overrides(buffer, (object->flags & SCRIPT_IMAGE_PRIMARY) | object->palette_flags);
        end_script_text_statement(buffer);
    }
}

void serialize_script_object_states(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr || script_runtime_root->objects == nullptr)
        return;
    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    for(ScriptObjectState *object = script_runtime_root->objects; object != nullptr; object = object->next)
    {
        append_script_text_property(buffer, 1, object->name);
        uint32_t field_mask = 1;
        for(uint32_t index = 0; index < object->field_count; ++index)
        {
            if(object->integer_values[index] != 0)
            {
                append_script_text_delimiter(buffer, object->field_names[index], ':');
                append_script_text_integer(buffer, object->integer_values[index], ' ');
            }
            if(object->string_values[index][0] != '\0')
            {
                append_script_text_delimiter(buffer, object->field_names[index], ':');
                append_script_text_delimiter(buffer, object->string_values[index], ' ');
            }
            append_script_text_delimiter(buffer, object->field_names[index], ':');
            append_script_text_delimiter(buffer, (object->active_field_mask & field_mask) != 0 ? "ON" : "OFF", ' ');
            field_mask *= 2;
        }
        if(object->mouse_visual_name[0] != '\0')
        {
            append_script_text_scope(buffer, 0x0d000000);
            append_script_text_delimiter(buffer, object->mouse_visual_name, ' ');
        }
        if(object->alternate_mouse_visual_name[0] != '\0')
        {
            append_script_text_scope(buffer, 0x20000000);
            append_script_text_delimiter(buffer, object->alternate_mouse_visual_name, ' ');
        }
        serialize_image_flag_overrides(buffer, object->image_flags);
        append_natural_mouse_image_flag(buffer, object->mouse_flags);
        field_mask = 1;
        for(uint32_t index = 0; index < script_runtime_root->command_definition_count; ++index)
        {
            if((object->command_mask & field_mask) != 0)
            {
                append_script_text_scope(buffer, 0x0c000000);
                append_script_text_delimiter(buffer, script_runtime_root->command_definitions[index].name, ' ');
            }
            field_mask *= 2;
        }
        end_script_text_statement(buffer);
    }
}

RuntimeVisualObject *find_runtime_visual_object(const char *name)
{
    for(RuntimeVisualObject *object = script_runtime_root->visual_objects; object != nullptr; object = object->next)
        if(strings_equal(name, object->name))
            return object;
    return nullptr;
}

void enqueue_runtime_event_record(const uintptr_t *record)
{
    if(script_runtime_root != nullptr && record != nullptr)
    {
        std::memcpy(script_runtime_root->event_records[script_runtime_root->transient_index_2], record, sizeof(script_runtime_root->event_records[0]));
        ++script_runtime_root->transient_index_2;
        if(script_runtime_root->transient_index_2 > 0x1f)
            script_runtime_root->transient_index_2 = 0;
        if(script_runtime_root->transient_index_2 == script_runtime_root->transient_index_1)
        {
            ++script_runtime_root->transient_index_1;
            if(script_runtime_root->transient_index_1 > 0x1f)
                script_runtime_root->transient_index_1 = 0;
        }
    }
}

void acknowledge_current_runtime_event_record()
{
    if(script_runtime_root != nullptr && script_runtime_root->transient_index_2 != script_runtime_root->transient_index_1)
    {
        uintptr_t &flags = script_runtime_root->event_records[script_runtime_root->transient_index_1][14];
        if((flags & RUNTIME_EVENT_ACKNOWLEDGED) != 0)
            read_runtime_event_record(nullptr, 1);
        else
            flags |= RUNTIME_EVENT_ACKNOWLEDGED;
    }
}

uint32_t read_runtime_event_record(uintptr_t *record, int32_t advance)
{
    if(record != nullptr)
        record[14] = 0;
    if(script_runtime_root == nullptr || script_runtime_root->transient_index_2 == script_runtime_root->transient_index_1)
        return 0;
    if(record != nullptr)
        std::memcpy(record, script_runtime_root->event_records[script_runtime_root->transient_index_1], sizeof(script_runtime_root->event_records[0]));
    if(advance != 0)
    {
        ++script_runtime_root->transient_index_1;
        if(script_runtime_root->transient_index_1 > 0x1f)
            script_runtime_root->transient_index_1 = 0;
    }
    return 1;
}

int32_t select_pointer_region_scene(RuntimePointerRegion *region)
{
    region->current_scene_bit = 0;
    if(region->scene_mask == 0)
        return -1;
    region->current_scene_bit = region->first_scene_bit != 0 ? region->first_scene_bit : 1;
    uint32_t state_mask = region->state_object != nullptr ? region->state_object->command_mask : 0;
    for(int32_t attempts = 0; attempts < 32; ++attempts)
    {
        uint32_t bit = region->current_scene_bit;
        if((bit & region->scene_mask) != 0)
        {
            int32_t index = 0;
            for(uint32_t shifted = bit; shifted != 1; shifted >>= 1)
                ++index;
            if((runtime_scene_slots[index].flags & SCRIPT_IMAGE_DOUBLE_SIZE) == 0 || (bit & state_mask) != 0)
                return index;
        }
        region->current_scene_bit = bit == 0x80000000 ? 1 : bit * 2;
    }
    region->current_scene_bit = 0;
    return -1;
}

uint32_t synchronize_runtime_pointer_owner_slots(void *owner_identity, void *tree_identity, RuntimePointerRegion *region)
{
    RuntimeNamedNode *node = script_runtime_root->runtime_nodes;
    while(node != nullptr && node->identity != owner_identity)
        node = node->next;
    if(node == nullptr)
        return 0;

    auto *child = node->cache_entry_cursor;
    RuntimeTreeLink84 *link = find_global_runtime_tree_link_0084_by_identity(region);
    for(uint32_t index = 0; index < node->visible_entry_count; ++index)
    {
        if(child == nullptr)
        {
            if(link != nullptr && link->owner_group_identity == reinterpret_cast<uintptr_t>(owner_identity))
            {
                link->command_mask = 0;
                if(link->primary_resource_identity != nullptr)
                    static_cast<RuntimeTreePrimaryResourceLink *>(link->primary_resource_identity)->flags |= RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
            }
        }
        else if(link != nullptr && link->owner_group_identity == reinterpret_cast<uintptr_t>(owner_identity))
        {
            auto *object = static_cast<ScriptObjectState *>(child->data);
            update_runtime_tree_link_0084(tree_identity, link->identity, 0, 0, 0, 0, 0, object, nullptr, 0, object->command_mask, SCRIPT_INTEGER_INVALID);
            if(link->primary_resource_identity != nullptr)
            {
                const void *file_name = nullptr;
                if((object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0)
                    file_name = object->mouse_visual_name;
                else if(object->visual_object != nullptr)
                    file_name = object->visual_object->file_name;
                update_runtime_tree_primary_resource_link(tree_identity, link->primary_resource_identity, file_name, 0, 0, 0);
                static_cast<RuntimeTreePrimaryResourceLink *>(link->primary_resource_identity)->flags &= ~RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
            }
        }
        if(link != nullptr)
            link = link->next;
        if(child != nullptr)
        {
            child = child->next;
            if(child == node->cache_entry_cursor)
                child = nullptr;
        }
    }
    return 1;
}

RuntimeGenericBackendChild *attach_runtime_generic_backend_child(void *resource_identity, void *fixed_resource_identity, void *secondary_resource_identity, uintptr_t selection, uint32_t flags)
{
    RuntimeGenericBackendChild *child = nullptr;
    RuntimeLockRecord *resource_record = acquire_runtime_lock_record(resource_identity);
    void *owner_identity = resource_identity;
    if(resource_record == nullptr)
    {
        owner_identity = secondary_resource_identity;
    }
    else
    {
        auto *resource = reinterpret_cast<RuntimeResourceObject *>(resource_record);
        if(secondary_resource_identity == nullptr)
            secondary_resource_identity = resource->secondary_resource_identity;
        if(fixed_resource_identity == nullptr)
            fixed_resource_identity = resource->fixed_resource_identity;
    }

    RuntimeLockRecord *secondary_record = acquire_runtime_lock_record(secondary_resource_identity);
    RuntimeLockRecord *fixed_record = acquire_runtime_lock_record(fixed_resource_identity);
    if(secondary_record != nullptr && fixed_record != nullptr)
    {
        const uintptr_t context[2]{ reinterpret_cast<uintptr_t>(owner_identity), find_available_display_scene_index(0x80000) };
        auto *secondary_resource = reinterpret_cast<RuntimeResourceObject *>(secondary_record);
        auto *fixed_resource = reinterpret_cast<RuntimeResourceObject *>(fixed_record);
        child = create_runtime_generic_backend_child(secondary_resource->backend, fixed_resource->backend, context, selection, flags);
        if(child != nullptr)
        {
            const intptr_t identifier = (fixed_resource->backend_flags & SCRIPT_IMAGE_NO_PALETTE) != 0 ? runtime_display_scene_identifier : fixed_resource->scene_identifier;
            DisplaySceneNode *locked_scene = lock_display_scene_node(identifier);
            if(locked_scene == nullptr)
            {
                destroy_runtime_generic_backend_child(child);
            }
            else
            {
                DisplaySceneDescriptor descriptor;
                DisplaySceneNode *scene =
                    acquire_display_scene_node(static_cast<uint32_t>(context[1]), 10000, 10000, 0x10, 0x10, 0, reinterpret_cast<intptr_t>(owner_identity), &descriptor, &default_display_pixel_format);
                if(scene == nullptr)
                {
                    destroy_runtime_generic_backend_child(child);
                }
                else if(resource_record != nullptr)
                {
                    auto *resource = reinterpret_cast<RuntimeResourceObject *>(resource_record);
                    resource->secondary_resource_identity = secondary_resource_identity;
                    resource->fixed_resource_identity = fixed_resource_identity;
                    resource->type_flags |= RUNTIME_RESOURCE_GENERIC_CHILD_ATTACHED;
                    resource->generic_backend_child = child;
                }
                unlock_display_scene_node(identifier);
            }
        }
    }
    if(secondary_record != nullptr)
        release_runtime_lock_record(secondary_record);
    if(fixed_record != nullptr)
        release_runtime_lock_record(fixed_record);
    if(resource_record != nullptr)
        release_runtime_lock_record(resource_record);
    return child;
}



void rebuild_runtime_tree_resources(void *identity)
{
    RuntimeTreeNode *root = find_runtime_tree_node_by_identity(identity);
    if(root == nullptr)
        return;
    if(root->parent == nullptr)
        runtime_pointer_root_identity = identity;
    for(RuntimeTreeSceneLink *link = root->scene_link_head; link != nullptr; link = link->next)
    {
        if(link->scene_identifier == 0)
        {
            link->scene_identifier =
                reinterpret_cast<intptr_t>(acquire_display_scene_node(link->z, link->x, link->y, link->width, link->height, link->flags | DISPLAY_SCENE_PRIMARY, 0, nullptr, nullptr));
        }
    }
    for(RuntimeTreeSecondaryResourceLink *link = root->secondary_resource_link_head; link != nullptr; link = link->next)
        if(link->resource_identity == nullptr)
            link->resource_identity = construct_runtime_resource(link->file_name, 0, 0, 0, 0, 0, 0, RUNTIME_RESOURCE_NATURAL_MOUSE | RUNTIME_RESOURCE_ONE_STEP);

    void *previous_owner = nullptr;
    for(RuntimePointerRegion *region = runtime_pointer_regions; region != nullptr; region = region->next)
    {
        if(region->owner_identity != nullptr && region->owner_identity != previous_owner)
        {
            previous_owner = region->owner_identity;
            synchronize_runtime_pointer_owner_slots(region->owner_identity, runtime_pointer_root_identity, region);
        }
    }

    uint32_t count = runtime_resource_count;
    auto *primary_head = script_runtime_root->global_primary_resource_links;
    RuntimeTreePrimaryResourceLink *first_primary = primary_head;
    while(first_primary != nullptr && (first_primary->image_flags & SCRIPT_IMAGE_PRIMARY) == 0)
        first_primary = first_primary->next;
    if(first_primary != nullptr && first_primary->resource_identity == nullptr)
    {
        first_primary->resource_identity = construct_runtime_resource(first_primary->file_name, first_primary->source_value, first_primary->x, first_primary->y, first_primary->ratio_x,
            first_primary->ratio_y, first_primary->loop_count, first_primary->image_flags);
        const uint32_t flags = query_runtime_scene_flags(first_primary->resource_identity);
        if(flags == 0)
            first_primary->flags |= RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
        else if((flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) != 0)
            ++count;
    }

    for(RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes; node != nullptr; node = node->next)
    {
        if(node->previous_resource_identity != nullptr)
        {
            request_runtime_resource_destruction(node->previous_resource_identity);
            node->previous_resource_identity = nullptr;
        }
        if(node->resource_identity == nullptr)
            node->resource_identity = construct_runtime_resource(node->serialized_value, 0, 0, 0, 0, 0, 0, node->resource_flags | RUNTIME_RESOURCE_HIDDEN);
    }

    for(RuntimeVisualObject *visual = script_runtime_root->visual_objects; visual != nullptr; visual = visual->next)
    {
        if(visual->previous_scene_identity != nullptr)
        {
            request_runtime_resource_destruction(visual->previous_scene_identity);
            visual->previous_scene_identity = nullptr;
        }
        if((visual->flags & RUNTIME_VISUAL_RESOURCE_CHANGED) != 0)
        {
            visual->scene_identity = construct_runtime_resource(visual->file_name, 0, 0, 0, static_cast<uint32_t>(visual->position_x), static_cast<uint32_t>(visual->position_y), 0,
                visual->palette_flags | RUNTIME_RESOURCE_HALF_SIZE);
            visual->flags &= ~RUNTIME_VISUAL_RESOURCE_CHANGED;
        }
    }

    for(RuntimeTreePrimaryResourceLink *link = primary_head; link != nullptr; link = link->next)
    {
        if((link->flags & RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING) != 0)
            continue;
        bool created = false;
        if(link->resource_identity == nullptr)
        {
            created = true;
            link->resource_identity = construct_runtime_resource(link->file_name, link->source_value, link->x, link->y, link->ratio_x, link->ratio_y, link->loop_count, link->image_flags);
        }
        const uint32_t flags = query_runtime_scene_flags(link->resource_identity);
        if(flags == 0)
        {
            link->flags |= RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
            continue;
        }
        if(created && (flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) != 0)
            ++count;
        if((script_runtime_root->flags & SCRIPT_RUNTIME_COMMENTS_SUPPRESSED) == 0 && (flags & RUNTIME_RESOURCE_GENERIC_CHILD_ATTACHED) == 0 && link->fixed_name_node != nullptr
            && link->secondary_link != nullptr)
        {
            uint32_t configuration_flags = 0;
            if((link->image_flags & SCRIPT_IMAGE_STOPPED) != 0 || ((flags & RUNTIME_RESOURCE_TYPE_SOUND) != 0 && (link->image_flags & SCRIPT_IMAGE_LOAD_ONLY) != 0))
                configuration_flags = RUNTIME_GENERIC_CHILD_MODE_200;
            if((flags & RUNTIME_RESOURCE_TYPE_ANIMATION) != 0)
                configuration_flags |= RUNTIME_GENERIC_CHILD_SELECTION_OVERRIDE;
            attach_runtime_generic_backend_child(link->resource_identity, link->fixed_name_node->resource_identity, link->secondary_link->resource_identity, 0, configuration_flags);
        }
    }

    set_runtime_tree_comment_mode(root, 1);
    wait_for_runtime_resource_count(count);
    if(root->parent == nullptr)
    {
        reset_runtime_byte_queue();
        reset_runtime_input_queue();
        reset_script_runtime_transient_indices();
        set_runtime_resource_state(current_runtime_resource, 0);
        runtime_display_context.flags |= RUNTIME_HOST_SCRIPT_TREE_ACTIVE;
    }
    send_application_event(HostApplicationCommand::STATE_ACTIVATED, root);
    on_scripted_save_load_tree_rebuilt(root);
}


void rebuild_runtime_pointer_resources()
{
    RuntimeTreeNode *root = find_runtime_tree_node_by_identity(runtime_pointer_root_identity);
    uint32_t count = runtime_resource_count;
    auto *primary = script_runtime_root->global_primary_resource_links;
    if(root == nullptr)
        return;

    void *previous_owner = nullptr;
    for(RuntimePointerRegion *region = runtime_pointer_regions; region != nullptr; region = region->next)
    {
        if(region->owner_identity != nullptr && previous_owner != region->owner_identity)
        {
            previous_owner = region->owner_identity;
            synchronize_runtime_pointer_owner_slots(region->owner_identity, runtime_pointer_root_identity, region);
        }
        region->previous_owner_identity = nullptr;
        count = runtime_resource_count;
        primary = script_runtime_root->global_primary_resource_links;
    }

    if(primary != nullptr)
    {
        for(RuntimeTreePrimaryResourceLink *link = primary; link != nullptr; link = link->next)
        {
            if(link->previous_resource_identity != nullptr)
            {
                const uint32_t scene_flags = query_runtime_scene_flags(link->previous_resource_identity);
                if(scene_flags != 0)
                {
                    if((scene_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) != 0)
                    {
                        --count;
                        if((link->flags & RUNTIME_RESOURCE_NO_CLOSE) == 0)
                        {
                            finalize_runtime_resource_destruction(link->previous_resource_identity);
                            link->previous_resource_identity = nullptr;
                            continue;
                        }
                    }
                    request_runtime_resource_destruction(link->previous_resource_identity);
                }
                link->previous_resource_identity = nullptr;
            }
        }

        for(RuntimeTreePrimaryResourceLink *link = script_runtime_root->global_primary_resource_links; link != nullptr; link = link->next)
        {
            if((link->flags & RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING) == 0)
            {
                if(link->resource_identity == nullptr)
                {
                    link->resource_identity = construct_runtime_resource(link->file_name, link->source_value, link->x, link->y, link->ratio_x, link->ratio_y, link->loop_count, link->image_flags);
                    const uint32_t scene_flags = query_runtime_scene_flags(link->resource_identity);
                    if(scene_flags == 0)
                        link->flags |= RUNTIME_PRIMARY_RESOURCE_REFRESH_PENDING;
                    else if((scene_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) != 0)
                        ++count;
                }
                if(link->previous_x != 0 || link->previous_y != 0)
                {
                    link->previous_x = 0;
                    link->previous_y = 0;
                    update_runtime_scene_position(link->resource_identity, link->x, link->y);
                }
            }
            else
            {
                const uint32_t scene_flags = query_runtime_scene_flags(link->resource_identity);
                if(scene_flags != 0)
                {
                    if((scene_flags & RUNTIME_RESOURCE_VISUAL_TYPE_MASK) == 0)
                    {
                        request_runtime_resource_destruction(link->resource_identity);
                    }
                    else
                    {
                        --count;
                        if((link->flags & RUNTIME_RESOURCE_NO_CLOSE) == 0)
                            finalize_runtime_resource_destruction(link->resource_identity);
                        else
                            request_runtime_resource_destruction(link->resource_identity);
                    }
                    link->resource_identity = nullptr;
                }
            }
        }
    }
    set_runtime_tree_comment_mode(root, 1);
    wait_for_runtime_resource_count(count);
}


uint32_t handle_runtime_left_button_up()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_SCRIPT_TREE_ACTIVE) == 0 || (runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED) != 0)
        return 0;
    runtime_pointer_event_record[11] = 0x10000000;
    if((runtime_scene_control_flags & RUNTIME_HOST_POINTER_MODE_MASK) == RUNTIME_HOST_POINTER_MODE_PRIMARY)
    {
        RuntimePointerRegion *region = reinterpret_cast<RuntimePointerRegion *>(find_global_runtime_tree_link_0084_by_identity(active_runtime_pointer_region));
        if(region != nullptr && region->scene_mask != 0)
        {
            int32_t scene_index;
            if(region->current_scene_bit == 0)
            {
                scene_index = select_pointer_region_scene(region);
            }
            else
            {
                scene_index = 0;
                for(uint32_t bit = region->current_scene_bit; bit != 1; bit >>= 1)
                    ++scene_index;
            }
            if(scene_index != -1 && (runtime_scene_slots[scene_index].flags & SCRIPT_IMAGE_DOUBLE_SIZE) == 0)
            {
                runtime_pointer_state_mask = region->current_scene_bit;
                runtime_pointer_event_record[11] |= 1;
                if(region->state_object != nullptr)
                {
                    runtime_pointer_event_state_object = region->state_object;
                    runtime_pointer_event_record[11] = (runtime_pointer_event_record[11] & ~1u) | 5;
                }
                runtime_pointer_event_record[11] |= 8;
                runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(region);
            }
        }
    }
    enqueue_runtime_pointer_event();
    return 0;
}

uint32_t handle_runtime_left_button_down()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_SCRIPT_TREE_ACTIVE) == 0)
        return 0;
    if(destroy_runtime_comment_trees() != 0)
        return 1;
    if((runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED) != 0)
        return 0;
    bool set_flag_2 = false;
    bool set_flag_4 = false;
    const uint32_t mode = runtime_scene_control_flags & RUNTIME_HOST_POINTER_MODE_MASK;
    if(mode != RUNTIME_HOST_POINTER_MODE_MASK)
    {
        const char empty_name[0x20]{};
        if(has_runtime_pointer_inventory_pack() == 0 && find_runtime_tree_descendant_identity_by_name(runtime_pointer_root_identity, empty_name) == nullptr)
        {
            if((runtime_scene_control_flags & RUNTIME_HOST_SCENE_TRANSITION_GUARDED) == 0)
                set_flag_2 = true;
        }
        else
        {
            set_flag_4 = true;
        }
    }
    if(mode == RUNTIME_HOST_POINTER_MODE_PRIMARY)
    {
        RuntimePointerRegion *region = reinterpret_cast<RuntimePointerRegion *>(find_global_runtime_tree_link_0084_by_identity(active_runtime_pointer_region));
        if(region != nullptr)
        {
            const uint32_t state_mask = region->state_object != nullptr ? region->state_object->command_mask : 0;
            const uint32_t original_bit = region->current_scene_bit;
            const uint32_t eligible_mask = region->scene_mask | state_mask;
            if(eligible_mask != 0 && original_bit != 0)
            {
                uint32_t attempts = 0;
                int32_t scene_index = 0;
                do
                {
                    do
                    {
                        region->current_scene_bit = region->current_scene_bit == 0x80000000 ? 1 : region->current_scene_bit * 2;
                        ++attempts;
                    } while(attempts < 32 && (eligible_mask & region->current_scene_bit) == 0);
                    scene_index = 0;
                    for(uint32_t bit = region->current_scene_bit; bit != 1; bit >>= 1)
                        ++scene_index;
                } while(attempts < 32 && (region->current_scene_bit & state_mask) == 0 && (runtime_scene_slots[scene_index].flags & SCRIPT_IMAGE_DOUBLE_SIZE) != 0);
                if(attempts < 33)
                {
                    const bool is_view = strings_equal(runtime_scene_slots[scene_index].name, "IView");
                    if(is_view)
                    {
                        set_flag_2 = false;
                        set_flag_4 = false;
                    }
                    const bool is_hide = strings_equal(runtime_scene_slots[scene_index].name, "Hide");
                    if(is_hide)
                    {
                        set_flag_2 = false;
                        set_flag_4 = true;
                    }
                    if(is_view || is_hide)
                    {
                        runtime_pointer_state_mask = region->current_scene_bit;
                        runtime_pointer_event_record[11] = region->state_object == nullptr ? 1 : 5;
                        if(region->state_object != nullptr)
                            runtime_pointer_event_state_object = region->state_object;
                        runtime_pointer_event_record[11] |= 8;
                        runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(region);
                        enqueue_runtime_pointer_event();
                    }
                    else
                    {
                        if(region->current_scene_bit != original_bit)
                        {
                            set_flag_2 = false;
                            set_flag_4 = false;
                        }
                        RuntimeVisualObject *visual = runtime_scene_slots[scene_index].visual_object;
                        if(visual != nullptr && current_runtime_scene_identity != visual->scene_identity)
                            switch_runtime_scene(visual->scene_identity);
                    }
                }
            }
        }
    }
    else if(mode == RUNTIME_HOST_POINTER_MODE_MASK)
    {
        set_flag_2 = false;
        set_flag_4 = false;
        active_runtime_pointer_region = nullptr;
        runtime_scene_control_flags &= ~(RUNTIME_HOST_PALETTE_STATE | RUNTIME_HOST_POINTER_MODE_MASK);
        update_runtime_pointer_region(runtime_scene_x, runtime_scene_y);
        ScriptObjectState *object = find_script_object_by_identity(runtime_pointer_state_owner);
        if(object != nullptr && (object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0 && object->visual_object != nullptr)
        {
            request_runtime_resource_destruction(object->visual_object->scene_identity);
            remove_runtime_visual_object(object->visual_object);
            object->visual_object = nullptr;
        }
    }
    if(set_flag_4)
        set_script_runtime_flags(4, 1);
    if(set_flag_2)
        set_script_runtime_flags(2, 1);
    return 0;
}

uint32_t handle_runtime_right_button_down()
{
    if((runtime_scene_control_flags & RUNTIME_HOST_SCRIPT_TREE_ACTIVE) == 0)
        return 0;
    if(destroy_runtime_comment_trees() != 0)
        return 1;
    if((runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED) != 0)
        return 0;
    runtime_pointer_event_record[11] &= 0xefffffff;
    const uint32_t mode = runtime_scene_control_flags & RUNTIME_HOST_POINTER_MODE_MASK;
    if(mode == RUNTIME_HOST_POINTER_MODE_PRIMARY)
    {
        RuntimePointerRegion *region = reinterpret_cast<RuntimePointerRegion *>(find_global_runtime_tree_link_0084_by_identity(active_runtime_pointer_region));
        if(region == nullptr || region->scene_mask == 0)
            return 0;
        int32_t scene_index;
        if(region->current_scene_bit == 0)
        {
            scene_index = select_pointer_region_scene(region);
        }
        else
        {
            scene_index = 0;
            for(uint32_t bit = region->current_scene_bit; bit != 1; bit >>= 1)
                ++scene_index;
        }
        if(scene_index == -1)
            return 0;
        const uint32_t slot_flags = runtime_scene_slots[scene_index].flags;
        if((slot_flags & SCRIPT_IMAGE_DOUBLE_SIZE) == 0)
        {
            runtime_pointer_state_mask = region->current_scene_bit;
            runtime_pointer_event_record[11] = 1;
            if(region->state_object != nullptr)
            {
                runtime_pointer_event_record[11] = 5;
                runtime_pointer_event_state_object = region->state_object;
            }
            runtime_pointer_event_record[11] |= 8;
            runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(region);
            enqueue_runtime_pointer_event();
            return 0;
        }
        if((slot_flags & SCRIPT_IMAGE_DOUBLE_SIZE) != SCRIPT_IMAGE_DOUBLE_SIZE)
            return 0;
        ScriptObjectState *object = region->state_object;
        if(object != nullptr && (region->current_scene_bit & object->command_mask) != 0)
        {
            runtime_scene_control_flags |= (RUNTIME_HOST_POINTER_MODE_SECONDARY | RUNTIME_HOST_PALETTE_STATE);
            runtime_pointer_state_mask = region->current_scene_bit;
            RuntimeVisualObject *visual = object->visual_object;
            runtime_pointer_state_owner = object;
            if(visual == nullptr && (object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0)
            {
                visual = static_cast<RuntimeVisualObject *>(create_or_update_runtime_visual_object(object, object->mouse_visual_name, 0, 0, 0, object->image_flags));
                visual->flags &= ~RUNTIME_VISUAL_RESOURCE_CHANGED;
                visual->scene_identity = construct_runtime_resource(visual->file_name, 0, 0, 0, 0, 0, 0, visual->palette_flags | 2);
                object->visual_object = visual;
            }
            if(current_runtime_scene_identity != visual->scene_identity)
                switch_runtime_scene(visual->scene_identity);
            void *descendant = script_runtime_root == nullptr ? nullptr : find_runtime_tree_descendant_identity_by_name(runtime_pointer_root_identity, script_runtime_root->inventory_name);
            if(descendant != nullptr)
            {
                destroy_runtime_tree_resources(descendant);
                deactivate_runtime_tree_and_visuals(descendant, nullptr);
                return 1;
            }
        }
    }
    else if(mode == RUNTIME_HOST_POINTER_MODE_MASK)
    {
        runtime_pointer_event_record[11] = 3;
        RuntimePointerRegion *region = reinterpret_cast<RuntimePointerRegion *>(find_global_runtime_tree_link_0084_by_identity(active_runtime_pointer_region));
        if(region != nullptr)
        {
            runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(region);
            runtime_pointer_event_record[11] |= 8;
            if(region->state_object != nullptr && runtime_pointer_state_owner != region->state_object)
            {
                runtime_pointer_event_state_object = region->state_object;
                runtime_pointer_event_record[11] |= 4;
            }
        }
        enqueue_runtime_pointer_event();
        active_runtime_pointer_region = nullptr;
        runtime_scene_control_flags &= ~(RUNTIME_HOST_PALETTE_STATE | RUNTIME_HOST_POINTER_MODE_MASK);
        update_runtime_pointer_region(runtime_scene_x, runtime_scene_y);
        ScriptObjectState *object = find_script_object_by_identity(runtime_pointer_state_owner);
        if(object != nullptr && (object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0 && object->visual_object != nullptr)
        {
            request_runtime_resource_destruction(object->visual_object->scene_identity);
            remove_runtime_visual_object(object->visual_object);
            object->visual_object = nullptr;
        }
    }
    return 0;
}

uint32_t update_runtime_pointer_region(int32_t x, int32_t y)
{
    if((runtime_scene_control_flags & RUNTIME_HOST_SCRIPT_TREE_ACTIVE) == 0 || (runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED) != 0)
        return 0;
    RuntimeTreeNode *root = find_runtime_tree_node_by_identity(runtime_pointer_root_identity);
    if(root == nullptr)
        return 0;
    RuntimePointerRegion *selected = nullptr;
    for(RuntimePointerRegion *region = runtime_pointer_regions; region != nullptr; region = region->next)
        if(region->left <= x && x <= region->right && region->top <= y && y <= region->bottom && (selected == nullptr || selected->priority < region->priority))
            selected = region;
    if(selected != nullptr)
    {
        if(active_runtime_pointer_region == selected)
            return 0;
        runtime_pointer_event_record[11] = 0;
        uint32_t mode = runtime_scene_control_flags & RUNTIME_HOST_POINTER_MODE_MASK;
        if(mode == 0 || mode == RUNTIME_HOST_POINTER_MODE_PRIMARY)
        {
            runtime_pointer_event_record[11] = 8;
            runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(selected);
            enqueue_runtime_pointer_event();
            runtime_scene_control_flags = runtime_scene_control_flags & ~RUNTIME_HOST_PALETTE_STATE | RUNTIME_HOST_POINTER_MODE_PRIMARY;
            int32_t scene_index = select_pointer_region_scene(selected);
            if(scene_index != -1)
            {
                RuntimeVisualObject *visual = runtime_scene_slots[scene_index].visual_object;
                if(visual == nullptr)
                    scene_index = -1;
                else if(current_runtime_scene_identity != visual->scene_identity)
                    switch_runtime_scene(visual->scene_identity);
                if(scene_index != -1)
                {
                    active_runtime_pointer_region = selected;
                    return 0;
                }
            }
            RuntimeVisualObject *visual = selected->visual_override != nullptr ? selected->visual_override : root->default_visual;
            if(visual == nullptr)
            {
                switch_runtime_scene(nullptr);
                active_runtime_pointer_region = selected;
                return 0;
            }
            if(current_runtime_scene_identity != visual->scene_identity)
                switch_runtime_scene(visual->scene_identity);
        }
        else if(mode != RUNTIME_HOST_POINTER_MODE_MASK)
        {
            active_runtime_pointer_region = selected;
            return 0;
        }
        else
        {
            uint32_t mask = selected->scene_mask;
            if(selected->state_object != nullptr)
                mask |= selected->state_object->command_mask;
            runtime_pointer_event_record[11] = 8;
            runtime_pointer_event_record[0] = reinterpret_cast<uintptr_t>(selected);
            enqueue_runtime_pointer_event();
            if(selected->state_object != runtime_pointer_state_owner)
            {
                if((runtime_pointer_state_mask & mask) != 0)
                {
                    runtime_scene_control_flags &= ~RUNTIME_HOST_PALETTE_STATE;
                    active_runtime_pointer_region = selected;
                    return 0;
                }
                runtime_scene_control_flags |= RUNTIME_HOST_PALETTE_STATE;
                active_runtime_pointer_region = selected;
                return 0;
            }
            runtime_scene_control_flags |= RUNTIME_HOST_PALETTE_STATE;
        }
        active_runtime_pointer_region = selected;
        return 0;
    }
    active_runtime_pointer_region = nullptr;
    if((runtime_scene_control_flags & RUNTIME_HOST_POINTER_MODE_SECONDARY) == 0)
    {
        runtime_pointer_event_record[11] = 0;
        runtime_scene_control_flags &= ~(RUNTIME_HOST_PALETTE_STATE | RUNTIME_HOST_POINTER_MODE_MASK);
        if(root->default_visual == nullptr)
        {
            switch_runtime_scene(nullptr);
            return 0;
        }
        if(current_runtime_scene_identity != root->default_visual->scene_identity)
            switch_runtime_scene(root->default_visual->scene_identity);
    }
    else
    {
        runtime_scene_control_flags |= RUNTIME_HOST_PALETTE_STATE;
    }
    return 0;
}

uint32_t refresh_runtime_pointer_region()
{
    active_runtime_pointer_region = nullptr;
    return update_runtime_pointer_region(runtime_scene_x, runtime_scene_y);
}

bool has_runtime_pointer_inventory_pack()
{
    RuntimeTreeNode *node = begin_runtime_tree_enumeration(runtime_pointer_root_identity);
    while(node != nullptr)
    {
        if((node->flags & RUNTIME_TREE_INVENTORY_PACK) != 0)
            return true;
        node = get_next_runtime_tree_node(reinterpret_cast<RuntimeTreeNode *>(runtime_pointer_root_identity));
    }
    return false;
}


int32_t activate_default_comment_scene(const char *name)
{
    if((runtime_scene_control_flags & (RUNTIME_HOST_POINTER_MODE_SECONDARY | RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE)) != 0 || (runtime_scene_control_flags & RUNTIME_HOST_SCRIPT_TREE_ACTIVE) == 0
        || current_runtime_scene_identity == nullptr)
        return 0;
    RuntimeVisualObject *object = find_runtime_visual_object(name);
    if(object == nullptr)
        return -1;
    if(saved_default_comment_scene_identity != object->scene_identity || (runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED) == 0)
    {
        saved_default_comment_scene_identity = object->scene_identity;
        switch_runtime_scene(object->scene_identity);
    }
    runtime_scene_control_flags |= RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED;
    return 1;
}

void activate_runtime_tree_node_comment(RuntimeTreeNode *node)
{
    if(node->parent != nullptr && (runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE) == 0 && activate_default_comment_scene("m_DEF_COMMENT") > 0)
        runtime_scene_control_flags |= RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE;
}

void deactivate_default_comment_scene(const char *name)
{
    if((runtime_scene_control_flags & (RUNTIME_HOST_SCRIPT_TREE_ACTIVE | RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED)) == (RUNTIME_HOST_SCRIPT_TREE_ACTIVE | RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED))
    {
        RuntimeVisualObject *object = find_runtime_visual_object(name);
        if(object != nullptr && object->scene_identity == saved_default_comment_scene_identity)
        {
            runtime_scene_control_flags &= ~RUNTIME_HOST_DEFAULT_COMMENT_SCENE_LOCKED;
            saved_default_comment_scene_identity = nullptr;
            active_runtime_pointer_region = nullptr;
            update_runtime_pointer_region(runtime_scene_x, runtime_scene_y);
        }
    }
}

void deactivate_runtime_tree_node_comment(RuntimeTreeNode *node)
{
    if(node->parent != nullptr && (runtime_scene_control_flags & RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE) != 0)
    {
        runtime_scene_control_flags &= ~RUNTIME_HOST_DEFAULT_COMMENT_SCENE_ACTIVE;
        deactivate_default_comment_scene("m_DEF_COMMENT");
    }
}

void set_runtime_tree_comment_mode(RuntimeTreeNode *root, int enabled)
{
    RuntimeTreeNode *node = root;
    while(node != nullptr)
    {
        uint32_t old_flags = node->flags;
        if(enabled == 0)
        {
            node->flags = old_flags & ~RUNTIME_TREE_ACTIVE;
            if((old_flags & RUNTIME_TREE_COMMENT) != 0)
                deactivate_runtime_tree_node_comment(node);
        }
        else
        {
            node->flags = old_flags | RUNTIME_TREE_ACTIVE;
            if((old_flags & RUNTIME_TREE_COMMENT) != 0)
                activate_runtime_tree_node_comment(node);
        }
        node = node == root ? begin_runtime_tree_enumeration(root) : get_next_runtime_tree_node(root);
    }
}

RuntimeTreeNode *begin_runtime_tree_enumeration(void *identity)
{
    if(script_runtime_root == nullptr)
        return nullptr;
    RuntimeTreeNode *root = identity == nullptr ? script_runtime_root->runtime_tree : find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
    if(root == nullptr)
        return nullptr;
    root->iterator_current = nullptr;
    root->iterator_ascending = 0;
    RuntimeTreeNode *child = root->child;
    if(child == nullptr)
        return nullptr;
    if(child->child != nullptr)
    {
        root->iterator_current = child->child;
        return get_next_runtime_tree_node(static_cast<RuntimeTreeNode *>(identity));
    }
    root->iterator_current = child->next;
    return child;
}

RuntimeTreeNode *get_next_runtime_tree_node(RuntimeTreeNode *root)
{
    while(true)
    {
        if(script_runtime_root == nullptr || root == nullptr)
            return nullptr;
        RuntimeTreeNode *current = find_runtime_tree_node(root, root->iterator_current);
        if(current == nullptr)
            return nullptr;
        if(current->child == nullptr || root->iterator_ascending != 0)
        {
            root->iterator_ascending = 0;
            root->iterator_current = current->next;
            if(current->next != nullptr)
                return current;
            RuntimeTreeNode *parent = current->parent;
            if(parent == root || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
                return current;
            root->iterator_current = parent;
            root->iterator_ascending = 1;
            return current;
        }
        root->iterator_current = current->child;
    }
}

int destroy_runtime_comment_trees()
{
    int destroyed = 0;
    RuntimeTreeNode *node = begin_runtime_tree_enumeration(runtime_pointer_root_identity);
    if(node != nullptr)
    {
        do
        {
            if((node->flags & RUNTIME_TREE_COMMENT) != 0)
            {
                destroyed = 1;
                destroy_runtime_tree_resources(node);
                deactivate_runtime_tree_and_visuals(node, nullptr);
            }
            node = get_next_runtime_tree_node(static_cast<RuntimeTreeNode *>(runtime_pointer_root_identity));
        } while(node != nullptr);
        if(destroyed != 0)
        {
            reset_runtime_tree_parser_contexts(runtime_pointer_root_identity);
            rebuild_runtime_pointer_resources();
        }
    }
    return destroyed;
}

intptr_t deactivate_runtime_tree_and_visuals(void *identity, void *second)
{
    intptr_t result = 0;
    RuntimeTreeNode *node = find_runtime_tree_node_by_identity(identity);
    if(node != nullptr)
    {
        send_application_event(HostApplicationCommand::CREDITS_FINISHED, node);
        ScriptObjectState *object = script_runtime_root->objects;
        if(node->parent == nullptr)
        {
            while(object != nullptr)
            {
                if((object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0 && object->visual_object != nullptr)
                {
                    request_runtime_resource_destruction(object->visual_object->scene_identity);
                    remove_runtime_visual_object(object->visual_object);
                    object->visual_object = nullptr;
                }
                object = object->next;
            }
        }
        if((node->flags & RUNTIME_TREE_INVENTORY_PACK) != 0 || node->name[0] == 0)
        {
            set_script_runtime_flags(2, 0);
            set_script_runtime_flags(4, 0);
        }
        if((node->flags & RUNTIME_TREE_COMMENT) != 0)
            deactivate_runtime_tree_node_comment(node);
        result = reinterpret_cast<intptr_t>(destroy_runtime_tree_node(identity, second));
    }
    return result;
}



void *find_runtime_tree_identity_by_name_recursive(void *start_identity, const void *name)
{
    if(script_runtime_root == nullptr)
        return nullptr;
    RuntimeTreeNode *node = start_identity == nullptr ? script_runtime_root->runtime_tree : find_runtime_tree_node(script_runtime_root->runtime_tree, start_identity);
    while(node != nullptr)
    {
        if(node->child != nullptr)
        {
            void *identity = find_runtime_tree_identity_by_name_recursive(node->child, name);
            if(identity != nullptr)
                return identity;
        }
        if(fixed_dword_memory_equal(node, name, 0x20))
            return node->identity;
        node = node->next;
    }
    return nullptr;
}

void *find_runtime_tree_descendant_identity_by_name(void *root_identity, const void *name)
{
    if(script_runtime_root == nullptr)
        return nullptr;
    RuntimeTreeNode *root = root_identity == nullptr ? script_runtime_root->runtime_tree : find_runtime_tree_node(script_runtime_root->runtime_tree, root_identity);
    if(root != nullptr)
    {
        for(RuntimeTreeNode *child = root->child; child != nullptr; child = child->next)
        {
            void *identity = find_runtime_tree_identity_by_name_recursive(child, name);
            if(identity != nullptr)
                return identity;
        }
    }
    return nullptr;
}

void *find_runtime_tree_root_identity_by_name(const void *name)
{
    if(script_runtime_root == nullptr)
        return nullptr;
    for(RuntimeTreeNode *node = script_runtime_root->runtime_tree; node != nullptr; node = node->next)
        if(fixed_dword_memory_equal(node, name, 0x20))
            return node->identity;
    return nullptr;
}


} // namespace freegag
