#include "runtime_tree.h"
#include "runtime_internal.h"

namespace gag
{
void set_script_runtime_flags(uint32_t mask, int enabled)
{
    if(script_runtime_root != nullptr)
    {
        if(enabled != 0)
        {
            script_runtime_root->flags |= mask;
        }
        else
        {
            script_runtime_root->flags &= ~mask;
        }
    }
}

void reset_script_runtime_transient_indices()
{
    if(script_runtime_root != nullptr)
    {
        script_runtime_root->transient_index_1 = 0;
        script_runtime_root->transient_index_2 = 0;
    }
}

RuntimeGenericResourceNode *find_runtime_generic_resource(void *identity)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    for(RuntimeGenericResourceNode *node = script_runtime_root->generic_resources; node != nullptr; node = node->next)
    {
        if(node->identity == identity)
        {
            return node;
        }
    }
    return nullptr;
}

void remove_all_runtime_generic_resources()
{
    if(script_runtime_root == nullptr)
    {
        return;
    }
    RuntimeGenericResourceNode *node = script_runtime_root->generic_resources;
    while(node != nullptr)
    {
        RuntimeGenericResourceNode *next = node->next;
        remove_runtime_generic_resource(node->identity);
        node = next;
    }
}

uintptr_t dispatch_runtime_tree_section_command(ScriptParserState *parser)
{
    parser->owner->flags &= ~0x200u;
    char resource_name[0x20];
    char section_name[0x20];
    char creation_text[0x104];
    if(runtime_tree_basic_command_api.parse_value(parser, resource_name, sizeof(resource_name)) == 0xffffffff)
    {
        return 0;
    }
    runtime_tree_basic_command_api.extract_parenthesized(parser, creation_text, sizeof(creation_text));
    RuntimeGenericResourceNode *resource;
    if(runtime_tree_basic_command_api.parse_value(parser, section_name, sizeof(section_name)) == 0xffffffff)
    {
        resource = parser->resource;
        std::memcpy(section_name, resource_name, sizeof(section_name));
    }
    else
    {
        resource = runtime_tree_basic_command_api.load_resource(resource_name);
        if(resource == nullptr)
        {
            return 0;
        }
        runtime_tree_basic_command_api.extract_parenthesized(parser, creation_text, sizeof(creation_text));
    }
    RuntimeTreeNode *result = runtime_tree_basic_command_api.dispatch_section(resource, parser->owner, section_name, creation_text);
    return result == parser->owner ? 0 : reinterpret_cast<uintptr_t>(result);
}

bool parse_runtime_language(ScriptParserState *parser)
{
    return runtime_tree_basic_command_api.parse_value(parser, script_runtime_root->language, sizeof(script_runtime_root->language)) != 0xffffffff;
}

RuntimeTreeNode *create_runtime_tree_command(ScriptParserState *parser)
{
    RuntimeTreeNode *owner = parser->owner;
    char resource_name[0x20];
    char tree_name[0x20];
    if(runtime_tree_basic_command_api.parse_value(parser, resource_name, sizeof(resource_name)) == 0xffffffff)
    {
        return nullptr;
    }
    const uint32_t tree_name_result = runtime_tree_basic_command_api.parse_value(parser, tree_name, sizeof(tree_name));
    void *parent_selector = nullptr;
    while(true)
    {
        const uint32_t scope = runtime_tree_basic_command_api.parse_scope(parser);
        if(scope == 0x00200000)
        {
            if(owner->parent == reinterpret_cast<RuntimeTreeNode *>(static_cast<intptr_t>(-1)))
            {
                return nullptr;
            }
            parent_selector = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        }
        if(scope == 0xffffffff)
        {
            break;
        }
    }
    RuntimeGenericResourceNode *resource;
    if(tree_name_result == 0xffffffff)
    {
        std::memcpy(tree_name, resource_name, sizeof(tree_name));
        resource = parser->resource;
    }
    else
    {
        resource = runtime_tree_basic_command_api.load_resource(resource_name);
    }
    return runtime_tree_basic_command_api.create_node(resource, parent_selector, tree_name, nullptr);
}


void set_runtime_generic_resource_position(void *identity, uint32_t position)
{
    if(script_runtime_root == nullptr)
    {
        return;
    }
    for(RuntimeGenericResourceNode *node = script_runtime_root->generic_resources; node != nullptr; node = node->next)
    {
        if(node->identity == identity)
        {
            if(position < node->resource_metadata)
            {
                node->current_position = position;
            }
            return;
        }
    }
}

uint32_t read_runtime_generic_resource_token(void *identity, char *output, uint32_t capacity, uint8_t delimiter)
{
    RuntimeGenericResourceNode *resource = find_runtime_generic_resource(identity);
    if(resource == nullptr)
    {
        return 0xffffffff;
    }
    const auto *data = static_cast<const uint8_t *>(resource->resource_data);
    uint32_t length = resource->resource_metadata;
    uint32_t position = resource->current_position;
    uint32_t copied = 0;
    while(position < length)
    {
        uint8_t value = data[position];
        if(value == delimiter || value == ';' || (value != '\r' && value != '\n'))
        {
            break;
        }
        ++position;
    }
    std::memset(output, 0, capacity);
    if(data[position] == ';' || data[position] == delimiter)
    {
        return 0x7fffffff;
    }
    while(position < length && copied < capacity && data[position] != delimiter && data[position] != ';')
    {
        output[copied] = static_cast<char>(data[position]);
        ++position;
        ++copied;
    }
    output[copied] = 0;
    if(data[position] != ';')
    {
        ++position;
    }
    resource->current_position = position;
    return copied;
}

RuntimeGenericResourceNode *find_or_load_runtime_generic_resource(const char *resource_name)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    char name[0x100]{};
    copy_file_name_from_path(name, resource_name);
    RuntimeGenericResourceNode *last = nullptr;
    for(RuntimeGenericResourceNode *node = script_runtime_root->generic_resources; node != nullptr; node = node->next)
    {
        last = node;
        if(fixed_dword_memory_equal(name, node->name, 0x20))
        {
            return static_cast<RuntimeGenericResourceNode *>(node->identity);
        }
    }
    void *resource_data = name;
    void *resource_metadata;
    script_runtime_root->get_property(6, &resource_data, &resource_metadata);
    if(resource_data == nullptr)
    {
        return nullptr;
    }
    auto *node = static_cast<RuntimeGenericResourceNode *>(runtime_generic_resource_load_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeGenericResourceNode)));
    if(node == nullptr)
    {
        return nullptr;
    }
    std::memcpy(node->name, name, sizeof(node->name));
    node->identity = node;
    node->resource_data = resource_data;
    node->resource_metadata = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(resource_metadata));
    if(last != nullptr)
    {
        last->next = node;
    }
    else
    {
        script_runtime_root->generic_resources = node;
    }
    return node;
}


RuntimeTreeParserContext *find_or_create_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name, RuntimeGenericResourceNode *resource, uint32_t start_offset, const char *creation_text)
{
    RuntimeTreeParserContext *last = nullptr;
    for(RuntimeTreeParserContext *context = owner->parser_contexts; context != nullptr; context = context->next)
    {
        last = context;
        if(strings_equal(context->name, name))
        {
            return context;
        }
    }
    auto *context = static_cast<RuntimeTreeParserContext *>(runtime_tree_parser_context_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeParserContext)));
    if(context != nullptr)
    {
        copy_string(context->name, name);
        context->owner = owner;
        context->resource = resource;
        context->resource_data = resource->resource_data;
        context->resource_metadata = resource->resource_metadata;
        context->start_offset = start_offset;
        context->cursor = start_offset;
        resource->current_position = start_offset;
        ++resource->active_references;
        if(creation_text != nullptr)
        {
            copy_string(context->creation_text, creation_text);
        }
        context->creation_text_pointer = context->creation_text;
        context->scratch_text_pointer = context->scratch_text;
        context->name_pointer = context->name;
        if(last != nullptr)
        {
            last->next = context;
        }
        else
        {
            owner->parser_contexts = context;
        }
    }
    return context;
}


void release_runtime_tree_parser_contexts(RuntimeTreeNode *owner)
{
    RuntimeTreeParserContext *context = owner->parser_contexts;
    while(context != nullptr)
    {
        RuntimeTreeParserContext *next = context->next;
        if(context->resource->active_references != 0)
        {
            --context->resource->active_references;
        }
        if(context->resource->active_references == 0)
        {
            runtime_tree_parser_release_api.remove_resource(context->resource);
        }
        runtime_tree_parser_release_api.heap_free(script_runtime_root->heap, 0, context);
        context = next;
    }
}

RuntimeTreeParserContext *find_existing_runtime_tree_parser_context(RuntimeTreeNode *owner, const char *name)
{
    RuntimeTreeParserContext *context = owner->parser_contexts;
    while(context != nullptr && !strings_equal(context->name, name))
    {
        context = context->next;
    }
    return context;
}


RuntimeTreeNode *dispatch_runtime_tree_parser(RuntimeTreeParserContext *context)
{
    auto *parser = reinterpret_cast<ScriptParserState *>(context);
    RuntimeTreeNode *owner = context->owner;
    uint32_t property_cursor = context->cursor;
    const auto dispatch_root_operation = [](uint32_t operation, const void *value)
    { script_runtime_root->set_property(operation, 0, reinterpret_cast<RuntimeGenericResourceNode *>(const_cast<void *>(value))); };
    while(true)
    {
        const uint32_t property = runtime_tree_parser_direct_dispatch_api.parse_property(parser);
        if(property == 0xffffffff)
        {
            break;
        }
        switch(property)
        {
        case 1:
            runtime_tree_parser_direct_dispatch_api.parse_object(parser);
            break;
        case 2:
            runtime_tree_parser_direct_dispatch_api.parse_link_0084(parser);
            break;
        case 3:
            runtime_tree_parser_direct_dispatch_api.parse_link_007c(parser);
            break;
        case 4:
            runtime_tree_parser_direct_dispatch_api.parse_visual(parser);
            break;
        case 5:
            runtime_tree_parser_direct_dispatch_api.parse_primary(parser);
            break;
        case 6:
            runtime_tree_parser_direct_dispatch_api.parse_container(parser);
            break;
        case 7:
            runtime_tree_parser_direct_dispatch_api.parse_command(parser);
            break;
        case 8:
            runtime_tree_parser_direct_dispatch_api.parse_named(parser);
            break;
        case 9:
            runtime_tree_parser_direct_dispatch_api.parse_link_008c(parser);
            break;
        case 0x0a:
            runtime_tree_parser_direct_dispatch_api.create_conditional(parser);
            break;
        case 0x0b:
        {
            int32_t value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
            uint32_t operation = 4;
            while(true)
            {
                if(value != 0x7fffffff)
                {
                    dispatch_root_operation(operation, reinterpret_cast<void *>(static_cast<intptr_t>(value)));
                }
                uint32_t flag;
                do
                {
                    flag = runtime_tree_parser_special_dispatch_api.parse_image_flag(parser);
                    if(static_cast<int32_t>(flag) < 1)
                    {
                        break;
                    }
                    if(flag == 2)
                    {
                        value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
                        operation = 1;
                        goto dispatch_property_0b_value;
                    }
                } while(flag != 4);
                if(static_cast<int32_t>(flag) < 1)
                {
                    break;
                }
                value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
                operation = 2;
dispatch_property_0b_value:;
            }
            break;
        }
        case 0x0c:
            runtime_tree_parser_direct_dispatch_api.parse_auxiliary_names(parser);
            break;
        case 0x0d:
        {
            char source[0x104];
            runtime_tree_parser_direct_dispatch_api.set_resource_position(context->resource, context->cursor);
            const uint32_t result = runtime_tree_parser_direct_dispatch_api.read_resource_token(context->resource, source, sizeof(source), ';');
            if(result != 0xffffffff)
            {
                const bool had_source = source[0] != '\0';
                dispatch_root_operation(5, source);
                if(source[0] == '\0' && had_source)
                {
                    RuntimeTreeNode *jump = nullptr;
                    if((owner->flags & 0x200) != 0)
                    {
                        jump = runtime_tree_parser_special_dispatch_api.find_jump(parser, "source", property_cursor);
                    }
                    if(jump != nullptr)
                    {
                        return jump;
                    }
                    dispatch_root_operation(0x50, nullptr);
                    break;
                }
            }
            break;
        }
        case 0x0e:
        {
            if(runtime_tree_parser_special_dispatch_api.create_command(parser) != nullptr)
            {
                break;
            }
            RuntimeTreeNode *jump = nullptr;
            if((owner->flags & 0x200) != 0)
            {
                jump = runtime_tree_parser_special_dispatch_api.find_jump(parser, "section", property_cursor);
            }
            if(jump != nullptr)
            {
                return jump;
            }
            dispatch_root_operation(0x60, nullptr);
            break;
        }
        case 0x0f:
        {
            const int32_t value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
            if(value == 0x7fffffff)
            {
                break;
            }
            else
            {
                dispatch_root_operation(8, reinterpret_cast<void *>(static_cast<intptr_t>(value)));
            }
            break;
        }
        case 0x10:
            runtime_tree_parser_direct_dispatch_api.create_fixed_name(parser);
            break;
        case 0x20:
            runtime_tree_parser_direct_dispatch_api.parse_language(parser);
            break;
        case 0x30:
            runtime_tree_parser_direct_dispatch_api.parse_secondary(parser);
            break;
        case 0x40:
            runtime_tree_parser_direct_dispatch_api.parse_value(parser, script_runtime_root->parser_value_0848, 0x20);
            break;
        case 0x50:
            runtime_tree_parser_direct_dispatch_api.apply_image_flags(parser);
            break;
        case 0x60:
            owner->flags |= 0x200;
            break;
        case 0x70:
            owner->flags &= 0xfffffdff;
            break;
        case 0x80:
        {
            const int32_t value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
            if(value == 0x7fffffff)
            {
                break;
            }
            else
            {
                script_runtime_root->parser_integer_0820 = static_cast<uint32_t>(value);
            }
            break;
        }
        case 0x90:
            runtime_tree_parser_direct_dispatch_api.set_resource_position(context->resource, context->cursor);
            runtime_tree_parser_direct_dispatch_api.read_resource_token(context->resource, script_runtime_root->parser_text_0868, 0x80, ';');
            break;
        case 0xa0:
        {
            const int32_t value = runtime_tree_parser_special_dispatch_api.parse_integer(parser);
            if(value == 0x7fffffff)
            {
                break;
            }
            else
            {
                dispatch_root_operation(0x0c, reinterpret_cast<void *>(static_cast<intptr_t>(value)));
            }
            break;
        }
        case 0xb0:
            runtime_tree_parser_direct_dispatch_api.set_resource_position(context->resource, context->cursor);
            runtime_tree_parser_direct_dispatch_api.read_resource_token(context->resource, script_runtime_root->default_auxiliary_names, 0x104, ';');
            break;
        case 0xc0:
            if(runtime_tree_parser_special_dispatch_api.strings_equal(owner->name, context->name_pointer))
            {
                runtime_tree_parser_direct_dispatch_api.parse_value(parser, owner->class_name, 0x20);
                if(runtime_tree_parser_special_dispatch_api.strings_equal(owner->class_name, "INVENTORY_PACK"))
                {
                    owner->flags |= 0x1000;
                }
                if(runtime_tree_parser_special_dispatch_api.strings_equal(owner->class_name, "COMMENT"))
                {
                    owner->flags |= 0x800;
                    break;
                }
            }
            break;
        case 0xd0:
            runtime_tree_parser_direct_dispatch_api.dispatch_section(parser);
            break;
        case 0xe0:
            runtime_tree_parser_direct_dispatch_api.set_resource_position(context->resource, context->cursor);
            runtime_tree_parser_direct_dispatch_api.read_resource_token(context->resource, context->scratch_text_pointer, 0x104, ';');
            break;
        case 0xf0:
            runtime_tree_parser_direct_dispatch_api.parse_scene(parser);
            break;
        default:
            break;
        }
        property_cursor = context->cursor;
    }

    if((owner->flags & 0x2000) != 0)
    {
        dispatch_root_operation(0x0d, nullptr);
    }
    if((owner->flags & 0x4000) != 0)
    {
        dispatch_root_operation(0x10, nullptr);
    }
    if((owner->flags & 0x400) != 0)
    {
        runtime_tree_parser_direct_dispatch_api.add_auxiliary_name(owner, context->resource->name);
    }
    for(RuntimeVisualObject *visual = script_runtime_root->visual_objects; visual != nullptr; visual = visual->next)
    {
        if((visual->flags & 1) != 0)
        {
            owner->default_visual = static_cast<RuntimeVisualObject *>(visual->identity);
            break;
        }
    }
    runtime_tree_parser_direct_dispatch_api.publish_links(owner);
    return owner;
}



RuntimeTreeNode *create_runtime_tree_node(RuntimeGenericResourceNode *resource, void *parent_selector, const char *tree_name, void *creation_context)
{
    RuntimeTreeNode *current = runtime_tree_creation_api.find_node(parent_selector);
    RuntimeGenericResourceNode *resolved_resource = runtime_tree_creation_api.find_resource(resource);
    if(resolved_resource == nullptr)
    {
        return nullptr;
    }

    RuntimeTreeNode *root = nullptr;
    RuntimeTreeNode *last = nullptr;
    if(current == nullptr)
    {
        void *existing = runtime_tree_creation_api.find_root_by_name(tree_name);
        if(existing != nullptr)
        {
            return static_cast<RuntimeTreeNode *>(existing);
        }
        last = script_runtime_root->runtime_tree;
    }
    else
    {
        root = runtime_tree_creation_api.find_ancestor_root(current);
        void *existing = runtime_tree_creation_api.find_descendant_by_name(root, tree_name);
        if(existing != nullptr)
        {
            return static_cast<RuntimeTreeNode *>(existing);
        }
        last = current->child;
    }
    while(last != nullptr && last->next != nullptr)
    {
        last = last->next;
    }

    uint32_t text_length = resolved_resource->resource_metadata;
    uint32_t start_offset = static_cast<uint32_t>(runtime_tree_creation_api.find_section(tree_name, static_cast<const char *>(resolved_resource->resource_data), text_length));
    if(start_offset == 0xffffffff)
    {
        return nullptr;
    }
    char class_name[0x20];
    if(runtime_tree_creation_api.find_property(class_name, "class", static_cast<const char *>(resolved_resource->resource_data), text_length, start_offset) != -1)
    {
        if(strings_equal(class_name, "TEMPLATE"))
        {
            return nullptr;
        }
        if(current == nullptr)
        {
            for(RuntimeTreeNode *node = script_runtime_root->runtime_tree; node != nullptr; node = node->next)
            {
                if(strings_equal(node->class_name, class_name))
                {
                    return nullptr;
                }
            }
        }
        else
        {
            for(RuntimeTreeNode *node = runtime_tree_creation_api.begin_enumeration(root); node != nullptr; node = runtime_tree_creation_api.next_enumeration(root))
            {
                if(strings_equal(node->class_name, class_name))
                {
                    return nullptr;
                }
            }
        }
    }

    auto *node = static_cast<RuntimeTreeNode *>(runtime_tree_creation_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeNode)));
    if(node == nullptr)
    {
        return nullptr;
    }
    std::memcpy(node->name, tree_name, sizeof(node->name));
    node->identity = node;
    constexpr auto root_parent_selector = static_cast<intptr_t>(-1);
    if(parent_selector == reinterpret_cast<void *>(root_parent_selector))
    {
        node->parent = reinterpret_cast<RuntimeTreeNode *>(root_parent_selector);
    }
    else
    {
        node->parent = current;
    }
    RuntimeTreeParserContext *context = runtime_tree_creation_api.create_parser_context(node, tree_name, resolved_resource, start_offset, static_cast<const char *>(creation_context));
    if(context == nullptr)
    {
        runtime_tree_creation_api.heap_free(script_runtime_root->heap, 0, node);
        runtime_tree_creation_api.remove_resource(resource);
        return nullptr;
    }

    if(parent_selector == reinterpret_cast<void *>(root_parent_selector))
    {
        if(script_runtime_root->runtime_tree != nullptr)
        {
            script_runtime_root->runtime_tree->previous = node;
            node->next = script_runtime_root->runtime_tree;
        }
        script_runtime_root->runtime_tree = node;
    }
    else if(last != nullptr)
    {
        last->next = node;
        node->previous = last;
    }
    else if(current != nullptr)
    {
        current->child = node;
    }
    else
    {
        script_runtime_root->runtime_tree = node;
    }

    RuntimeTreeNode *result = runtime_tree_creation_api.dispatch_parser(context);
    if(result != nullptr && (result->flags & 0x8000) == 0)
    {
        runtime_tree_creation_api.activate_node(result);
    }
    return result;
}


RuntimeTreeNode *find_and_create_runtime_tree_jump(ScriptParserState *parser, const char *target, uint32_t success_cursor)
{
    RuntimeTreeNode *result = nullptr;
    uint32_t saved_cursor = parser->cursor;
    for(;;)
    {
        uint32_t property = runtime_tree_jump_api.parse_property(parser);
        if(property == 0x70)
        {
            char resource_name[0x20];
            runtime_tree_jump_api.parse_name(parser, resource_name, sizeof(resource_name));
            if(strings_equal(resource_name, target))
            {
                runtime_tree_jump_api.synchronize_owner(parser->owner);
                if(runtime_tree_jump_api.parse_name(parser, resource_name, sizeof(resource_name)) != -1)
                {
                    char tree_name[0x20];
                    RuntimeGenericResourceNode *resource;
                    if(runtime_tree_jump_api.parse_name(parser, tree_name, sizeof(tree_name)) == -1)
                    {
                        std::memcpy(tree_name, resource_name, sizeof(tree_name));
                        resource = parser->resource;
                    }
                    else
                    {
                        resource = runtime_tree_jump_api.find_or_load_resource(resource_name);
                    }
                    result = runtime_tree_jump_api.create_node(resource, nullptr, tree_name, nullptr);
                }
                break;
            }
        }
        if(property == 0xffffffff)
        {
            break;
        }
    }
    parser->cursor = result == nullptr ? saved_cursor : success_cursor;
    return result;
}



void reset_runtime_tree_parser_context_recursive(ScriptParserState *parser)
{
    parser->cursor = parser->start_offset;
    for(;;)
    {
        uint32_t property = runtime_tree_parser_reset_api.parse_property(parser);
        if(property == 0xffffffff)
        {
            return;
        }
        if(property == 10)
        {
            RuntimeTreeNode *included = runtime_tree_parser_reset_api.resolve_included_tree(parser);
            if(included != nullptr)
            {
                for(RuntimeTreeParserContext *context = included->parser_contexts; context != nullptr; context = context->next)
                {
                    reset_runtime_tree_parser_context_recursive(reinterpret_cast<ScriptParserState *>(context));
                }
            }
        }
    }
}

void reset_runtime_tree_parser_contexts(void *identity)
{
    RuntimeTreeNode *node = runtime_tree_parser_reset_api.find_node(identity);
    if(node != nullptr)
    {
        for(RuntimeTreeParserContext *context = node->parser_contexts; context != nullptr; context = context->next)
        {
            reset_runtime_tree_parser_context_recursive(reinterpret_cast<ScriptParserState *>(context));
        }
    }
}


RuntimeTreeNode *dispatch_runtime_tree_section(void *resource_identity, void *node_identity, const char *section_name, const char *creation_text)
{
    RuntimeTreeNode *node = runtime_tree_section_dispatch_api.find_node(node_identity);
    if(node == nullptr)
    {
        return nullptr;
    }
    RuntimeGenericResourceNode *resource = runtime_tree_section_dispatch_api.find_resource(resource_identity);
    if(resource == nullptr)
    {
        return nullptr;
    }
    int start_offset = runtime_tree_section_dispatch_api.find_section(section_name, static_cast<const char *>(resource->resource_data), static_cast<int>(resource->resource_metadata));
    if(start_offset == -1)
    {
        return nullptr;
    }
    RuntimeTreeParserContext *context = runtime_tree_section_dispatch_api.create_parser_context(node, section_name, resource, static_cast<uint32_t>(start_offset), creation_text);
    if(context == nullptr)
    {
        runtime_tree_section_dispatch_api.remove_resource(resource_identity);
        return nullptr;
    }
    return runtime_tree_section_dispatch_api.dispatch_parser(context);
}


void add_runtime_tree_auxiliary_name(RuntimeTreeNode *owner, const char *name)
{
    for(RuntimeTreeAuxiliaryNode *node = owner->auxiliary_head; node != nullptr; node = node->next)
    {
        if(strings_equal(node->name, name))
        {
            return;
        }
    }
    auto *node = static_cast<RuntimeTreeAuxiliaryNode *>(runtime_tree_auxiliary_create_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeAuxiliaryNode)));
    if(node == nullptr)
    {
        return;
    }
    node->identity = const_cast<char *>(name);
    void *metadata;
    runtime_tree_auxiliary_create_api.resolve(6, &node->identity, &metadata);
    if(node->identity == nullptr)
    {
        runtime_tree_auxiliary_create_api.heap_free(script_runtime_root->heap, 0, node);
        return;
    }
    copy_string(node->name, name);
    node->identity = node;
    node->next = owner->auxiliary_head;
    owner->auxiliary_head = node;
}

uint32_t parse_runtime_tree_auxiliary_names(ScriptParserState *parser)
{
    char name[0x20];
    while(parse_script_value_token(parser, name, sizeof(name)) != 0xffffffff)
    {
        add_runtime_tree_auxiliary_name(parser->owner, name);
    }
    return 0;
}

uint32_t add_default_runtime_tree_auxiliary_names(RuntimeTreeNode *owner)
{
    const char *text = script_runtime_root->default_auxiliary_names;
    if(text[0] == '\0')
    {
        return 0;
    }
    const int length = static_cast<int>(std::strlen(text));
    int offset = 0;
    uint32_t result = 0;
    while(offset < length)
    {
        while(offset < length && text[offset] == ' ')
        {
            ++offset;
        }
        char name[0x80];
        int name_length = 0;
        while(offset < length && text[offset] != ' ')
        {
            name[name_length++] = text[offset++];
        }
        name[name_length] = '\0';
        if(name[0] != '\0')
        {
            result = 1;
            add_runtime_tree_auxiliary_name(owner, name);
        }
    }
    return result;
}

void release_runtime_tree_auxiliary_nodes(RuntimeTreeNode *owner)
{
    while(owner->auxiliary_head != nullptr)
    {
        RuntimeTreeAuxiliaryNode *node = owner->auxiliary_head;
        owner->auxiliary_head = node->next;
        runtime_tree_auxiliary_release_api.notify(7, 0, node);
        runtime_tree_auxiliary_release_api.heap_free(script_runtime_root->heap, 0, node);
    }
}



// Frees the inclusive head-through-tail range.
template<typename Link>
void free_runtime_tree_link_range(Link *head, Link *tail)
{
    for(Link *link = head; link != nullptr;)
    {
        Link *next = link->next;
        runtime_tree_destruction_core_api.heap_free(script_runtime_root->heap, 0, link);
        if(link == tail)
        {
            break;
        }
        link = next;
    }
}

// Updates the tail or head link after removal.
template<typename Link>
void route_runtime_tree_global_link(Link *&head, Link *tail, Link *value)
{
    if(tail == nullptr)
    {
        head = value;
    }
    else
    {
        tail->next = value;
    }
}

void update_runtime_tree_global_links(RuntimeTreeNode *removed, RuntimeTreeNode *replacement)
{
    if(removed->parent != nullptr)
    {
        return;
    }
    route_runtime_tree_global_link(script_runtime_root->global_scene_links, script_runtime_root->global_scene_link_tail, replacement == nullptr ? nullptr : replacement->scene_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_secondary_resource_links, script_runtime_root->global_secondary_resource_link_tail,
        replacement == nullptr ? nullptr : replacement->secondary_resource_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_primary_resource_links, script_runtime_root->global_primary_resource_link_tail,
        replacement == nullptr ? nullptr : replacement->primary_resource_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_0084_head, script_runtime_root->global_link_0084_tail, replacement == nullptr ? nullptr : replacement->link_0084_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_008c_head, script_runtime_root->global_link_008c_tail, replacement == nullptr ? nullptr : replacement->link_008c_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_007c_head, script_runtime_root->global_link_007c_tail, replacement == nullptr ? nullptr : replacement->link_007c_head);
    route_runtime_tree_global_link(script_runtime_root->containers, script_runtime_root->container_tail, replacement == nullptr ? nullptr : replacement->container_head);
}

void publish_runtime_tree_global_links(RuntimeTreeNode *node)
{
    if(node->parent != nullptr)
    {
        if(node->parent == reinterpret_cast<RuntimeTreeNode *>(static_cast<intptr_t>(-1)))
        {
            if(node->scene_link_tail != nullptr)
            {
                script_runtime_root->global_scene_link_tail = node->scene_link_tail;
            }
            if(node->secondary_resource_link_tail != nullptr)
            {
                script_runtime_root->global_secondary_resource_link_tail = node->secondary_resource_link_tail;
            }
            if(node->primary_resource_link_tail != nullptr)
            {
                script_runtime_root->global_primary_resource_link_tail = node->primary_resource_link_tail;
            }
            if(node->link_0084_tail != nullptr)
            {
                script_runtime_root->global_link_0084_tail = node->link_0084_tail;
            }
            if(node->link_008c_tail != nullptr)
            {
                script_runtime_root->global_link_008c_tail = node->link_008c_tail;
            }
            if(node->link_007c_tail != nullptr)
            {
                script_runtime_root->global_link_007c_tail = node->link_007c_tail;
            }
            if(node->container_tail != nullptr)
            {
                script_runtime_root->container_tail = node->container_tail;
            }
        }
        return;
    }
    route_runtime_tree_global_link(script_runtime_root->global_scene_links, script_runtime_root->global_scene_link_tail, node->scene_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_secondary_resource_links, script_runtime_root->global_secondary_resource_link_tail, node->secondary_resource_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_primary_resource_links, script_runtime_root->global_primary_resource_link_tail, node->primary_resource_link_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_0084_head, script_runtime_root->global_link_0084_tail, node->link_0084_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_008c_head, script_runtime_root->global_link_008c_tail, node->link_008c_head);
    route_runtime_tree_global_link(script_runtime_root->global_link_007c_head, script_runtime_root->global_link_007c_tail, node->link_007c_head);
    route_runtime_tree_global_link(script_runtime_root->containers, script_runtime_root->container_tail, node->container_head);
}

void append_script_runtime_flags(ScriptTextBuffer *buffer, uint32_t flags)
{
    if(flags == 0)
    {
        return;
    }
    append_script_text_property(buffer, 0x50, nullptr);
    while((flags & 0x04000101) != 0)
    {
        if((flags & 0x04000000) != 0)
        {
            flags &= ~0x04000000u;
            append_script_text_delimiter(buffer, "PAL_NOADJUST", ' ');
        }
        if((flags & 1) != 0)
        {
            flags &= ~1u;
            append_script_text_delimiter(buffer, "NOCOMMENT", ' ');
        }
        if((flags & 0x100) != 0)
        {
            flags &= ~0x100u;
            append_script_text_delimiter(buffer, "NOSAVE", ' ');
        }
    }
    end_script_text_statement(buffer);
}

void serialize_runtime_tree_sections(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr || script_runtime_root->runtime_tree == nullptr)
    {
        return;
    }
    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    for(RuntimeTreeNode *node = script_runtime_root->runtime_tree; node != nullptr; node = node->next)
    {
        RuntimeTreeParserContext *context = find_existing_runtime_tree_parser_context(node, reinterpret_cast<const char *>(node));
        if(context != nullptr)
        {
            append_script_text_property(buffer, 0xe, nullptr);
            append_script_text_delimiter(buffer, context->resource->name, ':');
            append_script_text_delimiter(buffer, node->name, ' ');
            if(node->parent == reinterpret_cast<RuntimeTreeNode *>(static_cast<intptr_t>(-1)))
            {
                append_script_text_scope(buffer, 0x200000);
            }
            end_script_text_statement(buffer);
        }
    }
}

void serialize_runtime_language(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr)
    {
        return;
    }
    const char *language = script_runtime_root->language;
    if(*language != '\0')
    {
        append_script_text_property(buffer, 0x20, nullptr);
        append_script_text_delimiter(buffer, language, ' ');
        end_script_text_statement(buffer);
    }
}

void serialize_runtime_fixed_name_nodes(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr || script_runtime_root->fixed_name_nodes == nullptr)
    {
        return;
    }
    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    for(RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes; node != nullptr; node = node->next)
    {
        append_script_text_property(buffer, 0x10, node->name);
        append_script_text_scope(buffer, 0x1000000);
        append_script_text_delimiter(buffer, node->serialized_value, ' ');
        end_script_text_statement(buffer);
    }
}

ScriptTextBuffer *serialize_current_runtime_state()
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }

    if(script_runtime_root->serialized_script == nullptr)
    {
        script_runtime_root->serialized_script = create_script_text_buffer();
        if(script_runtime_root->serialized_script == nullptr)
        {
            return nullptr;
        }
    }
    else
    {
        clear_script_text_buffer(script_runtime_root->serialized_script);
    }

    ScriptTextBuffer *buffer = script_runtime_root->serialized_script;
    begin_script_text_document(buffer);

    struct RuntimePropertyQueryResult
    {
        uint32_t integer;
        char text[0x104];
    } operation_result{};
    script_runtime_root->get_property(8, nullptr, &operation_result.integer);
    append_script_text_property(buffer, 0x0f, nullptr);
    append_script_text_integer(buffer, operation_result.integer, ' ');
    end_script_text_statement(buffer);

    script_runtime_root->get_property(12, nullptr, &operation_result.integer);
    append_script_text_property(buffer, 0xa0, nullptr);
    append_script_text_integer(buffer, operation_result.integer, ' ');
    end_script_text_statement(buffer);

    script_runtime_root->get_property(4, nullptr, &operation_result.integer);
    append_script_text_property(buffer, 0x0b, nullptr);
    append_script_text_integer(buffer, operation_result.integer, ' ');

    script_runtime_root->get_property(1, nullptr, &operation_result.integer);
    append_script_text_delimiter(buffer, "PALFADE", ':');
    append_script_text_integer(buffer, operation_result.integer, ' ');

    script_runtime_root->get_property(2, nullptr, &operation_result.integer);
    append_script_text_delimiter(buffer, "FRAMEFADE", ':');
    append_script_text_integer(buffer, operation_result.integer, ' ');
    end_script_text_statement(buffer);

    serialize_runtime_language(buffer);
    append_script_runtime_flags(buffer, script_runtime_root->flags | 0x100);
    append_script_runtime_flags(buffer, script_runtime_root->palette_flags);

    if(script_runtime_root->parser_integer_0820 != 0)
    {
        append_script_text_property(buffer, 0x80, nullptr);
        append_script_text_integer(buffer, script_runtime_root->parser_integer_0820, ' ');
        end_script_text_statement(buffer);
    }
    if(script_runtime_root->default_auxiliary_names[0] != '\0')
    {
        append_script_text_property(buffer, 0xb0, script_runtime_root->default_auxiliary_names);
        end_script_text_statement(buffer);
    }
    if(script_runtime_root->parser_text_0868[0] != '\0')
    {
        append_script_text_property(buffer, 0x90, script_runtime_root->parser_text_0868);
        end_script_text_statement(buffer);
    }
    if(script_runtime_root->parser_value_0848[0] != '\0')
    {
        append_script_text_property(buffer, 0x40, script_runtime_root->parser_value_0848);
        end_script_text_statement(buffer);
    }

    script_runtime_root->get_property(5, nullptr, operation_result.text);
    append_script_text_property(buffer, 0x0d, operation_result.text);
    end_script_text_statement(buffer);

    serialize_runtime_visual_objects(buffer);
    serialize_runtime_fixed_name_nodes(buffer);
    serialize_runtime_command_definitions(buffer);
    serialize_script_object_states(buffer);
    serialize_runtime_named_nodes(buffer);

    if(script_runtime_root->parser_text_0868[0] != '\0')
    {
        append_script_text_delimiter(buffer, nullptr, '\r');
        append_script_text_delimiter(buffer, nullptr, '\n');
        append_script_text_property(buffer, 0x60, nullptr);
        append_script_text_delimiter(buffer, nullptr, ';');
    }
    serialize_runtime_tree_sections(buffer);
    if(script_runtime_root->parser_text_0868[0] != '\0')
    {
        append_script_text_property(buffer, 0x70, script_runtime_root->parser_text_0868);
        end_script_text_statement(buffer);
    }

    append_script_text_delimiter(buffer, nullptr, '\r');
    append_script_text_delimiter(buffer, nullptr, '\n');
    RuntimeTreeNode *owner = find_runtime_tree_tail();
    if(owner != nullptr)
    {
        RuntimeTreeParserContext *context = find_existing_runtime_tree_parser_context(owner, owner->name);
        if(context != nullptr)
        {
            append_script_text_property(buffer, 3, "e_START");
            append_script_text_preload_directive(buffer, 0x50000000);
            append_script_text_delimiter(buffer, context->resource->name, ':');
            append_script_text_delimiter(buffer, owner->name, ' ');
            end_script_text_statement(buffer);
        }
    }
    end_script_text_document(buffer);
    return buffer;
}

RuntimeTreeNode *destroy_runtime_tree_node(void *identity, void *replacement_identity)
{
    RuntimeTreeNode *node = runtime_tree_destruction_core_api.find_node(identity);
    RuntimeTreeNode *replacement = runtime_tree_destruction_core_api.find_node(replacement_identity);
    if(node == nullptr)
    {
        return replacement;
    }
    if((node->flags & 0x8000) != 0)
    {
        runtime_tree_destruction_core_api.notify(0x40, 0, node);
    }
    if(replacement != nullptr && replacement->parent == node)
    {
        // PRELOAD creates the replacement as a child, while teardown subsequently reuses it after recursively freeing the parent's children.
        // Detach and promote that explicitly selected replacement so modern heap reclamation cannot invalidate the new active tree.
        RuntimeTreeNode *replacement_previous = nullptr;
        RuntimeTreeNode *child = node->child;
        while(child != nullptr && child != replacement)
        {
            replacement_previous = child;
            child = child->next;
        }
        if(child == replacement)
        {
            if(replacement_previous == nullptr)
            {
                node->child = replacement->next;
            }
            else
            {
                replacement_previous->next = replacement->next;
            }
            if(replacement->next != nullptr)
            {
                replacement->next->previous = replacement_previous;
            }

            RuntimeTreeNode *next = node->next;
            replacement->parent = node->parent;
            replacement->previous = node->previous;
            replacement->next = next;
            node->next = replacement;

            // The replacement was parsed while the removed parent remained in the global lists. Rebind its zone-qualified events away from the matching parent
            // zones.
            for(RuntimeTreeLink7C *event_link = replacement->link_007c_head; event_link != nullptr; event_link = event_link->next)
            {
                RuntimeTreeLink84 *removed_zone = node->link_0084_head;
                while(removed_zone != nullptr && removed_zone != event_link->zone_link)
                {
                    if(removed_zone == node->link_0084_tail)
                    {
                        removed_zone = nullptr;
                        break;
                    }
                    removed_zone = removed_zone->next;
                }
                if(removed_zone != nullptr)
                {
                    for(RuntimeTreeLink84 *replacement_zone = replacement->link_0084_head; replacement_zone != nullptr; replacement_zone = replacement_zone->next)
                    {
                        if(strings_equal(replacement_zone->name, removed_zone->name))
                        {
                            event_link->zone_link = replacement_zone;
                            break;
                        }
                        if(replacement_zone == replacement->link_0084_tail)
                        {
                            break;
                        }
                    }
                }
                if(event_link == replacement->link_007c_tail)
                {
                    break;
                }
            }
        }
    }
    for(RuntimeTreeNode *child = node->child; child != nullptr;)
    {
        RuntimeTreeNode *next = child->next;
        destroy_runtime_tree_node(child->identity, nullptr);
        child = next;
    }
    RuntimeTreeNode *parent = runtime_tree_destruction_core_api.find_node(node->parent);
    if(node->scene_link_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_scene_links(node->parent, node);
        free_runtime_tree_link_range(node->scene_link_head, node->scene_link_tail);
    }
    if(node->secondary_resource_link_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_secondary_links(node->parent, node);
        free_runtime_tree_link_range(node->secondary_resource_link_head, node->secondary_resource_link_tail);
    }
    if(node->primary_resource_link_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_primary_links(node->parent, node);
        free_runtime_tree_link_range(node->primary_resource_link_head, node->primary_resource_link_tail);
    }
    if(node->link_007c_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_links_7c(node->parent, node);
        free_runtime_tree_link_range(node->link_007c_head, node->link_007c_tail);
    }
    if(node->link_0084_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_links_84(node->parent, node);
        free_runtime_tree_link_range(node->link_0084_head, node->link_0084_tail);
    }
    if(node->link_008c_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_links_8c(node->parent, node);
        free_runtime_tree_link_range(node->link_008c_head, node->link_008c_tail);
    }
    if(node->container_tail != nullptr)
    {
        runtime_tree_destruction_core_api.remove_containers(node->parent, node);
        for(ScriptObjectContainer *container = node->container_head; container != nullptr;)
        {
            ScriptObjectContainer *next = container->next;
            runtime_tree_destruction_core_api.destroy_container(container);
            if(container == node->container_tail)
            {
                break;
            }
            container = next;
        }
    }
    runtime_tree_destruction_core_api.release_auxiliary(node);
    runtime_tree_destruction_core_api.release_parsers(node);
    if((node->flags & 0x2000) != 0)
    {
        runtime_tree_destruction_core_api.notify(0xe, 0, nullptr);
    }
    if((node->flags & 0x4000) != 0)
    {
        runtime_tree_destruction_core_api.notify(0x20, 0, nullptr);
    }
    if(replacement != nullptr && (replacement->flags & 0x200) != 0)
    {
        RuntimeTreeParserContext *context = runtime_tree_destruction_core_api.find_parser(replacement, reinterpret_cast<const char *>(replacement));
        if(context != nullptr)
        {
            replacement = runtime_tree_destruction_core_api.dispatch_parser(context);
        }
    }
    runtime_tree_destruction_core_api.update_global_links(node, replacement);
    if(node->previous == nullptr)
    {
        if(parent == nullptr)
        {
            script_runtime_root->runtime_tree = node->next;
        }
        else
        {
            parent->child = node->next;
        }
    }
    else
    {
        node->previous->next = node->next;
    }
    if(node->next != nullptr)
    {
        node->next->previous = node->previous;
    }
    runtime_tree_destruction_core_api.heap_free(script_runtime_root->heap, 0, node);
    return replacement;
}


RuntimeTreeNode *find_runtime_tree_node(RuntimeTreeNode *root, void *identity)
{
    while(root != nullptr)
    {
        if(root->identity == identity)
        {
            return root;
        }
        RuntimeTreeNode *result = nullptr;
        if(root->child != nullptr)
        {
            result = find_runtime_tree_node(root->child, identity);
        }
        if(result != nullptr)
        {
            return result;
        }
        root = root->next;
    }
    return nullptr;
}

RuntimeTreeNode *find_runtime_tree_node_by_identity(void *identity)
{
    return find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
}

void *find_last_runtime_tree_scene_link(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            void *result = find_last_runtime_tree_scene_link(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->scene_link_tail;
}

void *find_last_runtime_tree_secondary_resource_link(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            void *result = find_last_runtime_tree_secondary_resource_link(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->secondary_resource_link_tail;
}

void *find_last_runtime_tree_primary_resource_link(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            void *result = find_last_runtime_tree_primary_resource_link(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->primary_resource_link_tail;
}

void *find_last_runtime_scene_link_by_identity(void *identity)
{
    RuntimeTreeNode *root = find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
    return root == nullptr ? nullptr : find_last_runtime_tree_scene_link(root);
}

void *find_last_runtime_primary_resource_link_by_identity(void *identity)
{
    RuntimeTreeNode *root = find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
    return root == nullptr ? nullptr : find_last_runtime_tree_primary_resource_link(root);
}

void *find_last_runtime_secondary_resource_link_by_identity(void *identity)
{
    RuntimeTreeNode *root = find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
    return root == nullptr ? nullptr : find_last_runtime_tree_secondary_resource_link(root);
}

uint32_t parse_runtime_command_definition(ScriptParserState *parser)
{
    uint32_t &count = script_runtime_root->command_definition_count;
    RuntimeCommandDefinition *definitions = script_runtime_root->command_definitions;
    const uint32_t original_count = count;
    if(original_count > 0x1f)
    {
        return 0;
    }

    char name[0x20];
    if(parse_script_value_token(parser, name, sizeof(name)) == 0xffffffff)
    {
        return 0;
    }

    uint32_t index = 0;
    while(index < original_count && !fixed_dword_memory_equal(name, definitions[index].name, sizeof(name)))
    {
        ++index;
    }
    if(index == original_count)
    {
        std::memcpy(definitions[index].name, name, sizeof(name));
    }

    for(;;)
    {
        uint32_t code = parse_script_scope_code(parser);
        if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 0xffffffff)
            {
                break;
            }
            definitions[index].flags |= code;
        }
        else if(code == 0x0d000000)
        {
            code = parse_script_value_token(parser, name, sizeof(name));
            if(code == 0xffffffff)
            {
                break;
            }
            for(RuntimeVisualObject *object = script_runtime_root->visual_objects; object != nullptr; object = object->next)
            {
                if(fixed_dword_memory_equal(name, object->name, sizeof(name)))
                {
                    definitions[index].visual_object = object;
                    break;
                }
            }
        }
        if(code == 0xffffffff)
        {
            break;
        }
    }

    ++count;
    return count;
}

void append_dual_image_flag(ScriptTextBuffer *buffer, uint32_t flags)
{
    if(buffer != nullptr && flags != 0 && (flags & 0x00200000) != 0)
    {
        append_script_text_scope(buffer, 0x0a000000);
        buffer->length += copy_string(buffer->data + buffer->length, "DUAL");
        buffer->data[buffer->length++] = ' ';
    }
}

void serialize_runtime_command_definitions(ScriptTextBuffer *buffer)
{
    if(script_runtime_root == nullptr)
    {
        return;
    }
    const uint32_t count = script_runtime_root->command_definition_count;
    const RuntimeCommandDefinition *definitions = script_runtime_root->command_definitions;
    if(count != 0)
    {
        append_script_text_delimiter(buffer, nullptr, '\r');
        append_script_text_delimiter(buffer, nullptr, '\n');
    }
    for(uint32_t index = 0; index < count; ++index)
    {
        append_script_text_property(buffer, 7, definitions[index].name);
        if(definitions[index].visual_object != nullptr)
        {
            append_script_text_scope(buffer, 0x0d000000);
            append_script_text_delimiter(buffer, definitions[index].visual_object->name, ' ');
        }
        append_dual_image_flag(buffer, definitions[index].flags);
        end_script_text_statement(buffer);
    }
}

void clear_runtime_command_definitions()
{
    if(script_runtime_root != nullptr)
    {
        script_runtime_root->command_definition_count = 0;
        std::memset(script_runtime_root->command_definitions, 0, sizeof(script_runtime_root->command_definitions));
        if(runtime_scene_slots != reinterpret_cast<RuntimeSceneSlot *>(script_runtime_root->command_definitions))
        {
            std::memset(runtime_scene_slots, 0, sizeof(RuntimeSceneSlot) * 32);
        }
    }
}

uint32_t parse_runtime_tree_scene_link(ScriptParserState *parser)
{
    RuntimeTreeNode *node = parser->owner;
    char name[0x104];
    if(parse_script_value_token(parser, name, 0x20) == 0xffffffff)
    {
        return 0;
    }
    auto *link = static_cast<RuntimeTreeSceneLink *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeSceneLink)));
    if(link == nullptr)
    {
        return 0;
    }
    std::memcpy(link->name, name, sizeof(link->name));
    link->identity = link;
    for(;;)
    {
        uint32_t code = parse_script_scope_code(parser);
        if(code == 0x02000000)
        {
            const int32_t left = parse_script_integer_expression(parser);
            if(left != 0x7fffffff)
            {
                link->x = left;
            }
            const int32_t top = parse_script_integer_expression(parser);
            if(top != 0x7fffffff)
            {
                link->y = top;
            }
            const int32_t right = parse_script_integer_expression(parser);
            if(right != 0x7fffffff)
            {
                link->width = right - link->x + 1;
            }
            const int32_t bottom = parse_script_integer_expression(parser);
            if(bottom != 0x7fffffff)
            {
                link->height = bottom - link->y + 1;
            }
            code = static_cast<uint32_t>(bottom);
        }
        else if(code == 0x00040000)
        {
            code = static_cast<uint32_t>(parse_script_integer_expression(parser));
            if(code != 0x7fffffff)
            {
                link->z = code;
            }
        }
        else if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 2 || code == 0x20 || code == 0x02000000 || code == 0x04000000)
            {
                link->flags |= code;
            }
        }
        else if(code == 0x0b000000)
        {
            const int32_t x = parse_script_integer_expression(parser);
            if(x != 0x7fffffff)
            {
                link->x = x;
            }
            const int32_t y = parse_script_integer_expression(parser);
            if(y != 0x7fffffff)
            {
                link->y = y;
            }
            const int32_t width = parse_script_integer_expression(parser);
            if(width != 0x7fffffff)
            {
                link->width = width;
            }
            const int32_t height = parse_script_integer_expression(parser);
            if(height != 0x7fffffff)
            {
                link->height = height;
            }
            code = static_cast<uint32_t>(height);
        }
        if(code == 0xffffffff)
        {
            if(node->scene_link_tail == nullptr)
            {
                link->next = node->scene_link_head;
                node->scene_link_head = link;
                insert_runtime_tree_scene_link(node, link);
            }
            else
            {
                link->next = node->scene_link_tail->next;
                node->scene_link_tail->next = link;
            }
            node->scene_link_tail = link;
            return 0;
        }
    }
}

uint32_t parse_runtime_tree_secondary_resource_link(ScriptParserState *parser)
{
    RuntimeTreeNode *node = parser->owner;
    char name[0x80];
    if(parse_script_value_token(parser, name, 0x20) == 0xffffffff)
    {
        return 0;
    }
    auto *link =
        static_cast<RuntimeTreeSecondaryResourceLink *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeSecondaryResourceLink)));
    if(link == nullptr)
    {
        return 0;
    }
    std::memcpy(link->name, name, sizeof(link->name));
    link->identity = link;
    uint32_t code;
    do
    {
        code = parse_script_scope_code(parser);
        if(code == 0x01000000)
        {
            parse_script_file_value(parser, link->file_name, nullptr);
        }
    } while(code != 0xffffffff);
    if(node->secondary_resource_link_tail == nullptr)
    {
        link->next = node->secondary_resource_link_head;
        node->secondary_resource_link_head = link;
        insert_runtime_tree_secondary_resource_link(node, link);
    }
    else
    {
        link->next = node->secondary_resource_link_tail->next;
        node->secondary_resource_link_tail->next = link;
    }
    node->secondary_resource_link_tail = link;
    if((node->flags & 0x400) != 0)
    {
        add_runtime_tree_auxiliary_name(node, link->file_name);
    }
    return 0;
}

uint32_t parse_runtime_tree_primary_resource_link(ScriptParserState *parser)
{
    RuntimeTreeNode *node = parser->owner;
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == 0xffffffff)
    {
        return 0;
    }
    bool expand_list = false;
    bool invert_no_palette = false;
    char list_name[0x80];
    auto *link = static_cast<RuntimeTreePrimaryResourceLink *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreePrimaryResourceLink)));
    if(link == nullptr)
    {
        return 0;
    }
    std::memcpy(link->identifier, value, sizeof(link->identifier));
    link->identity = link;
    link->ratio_x = 1;
    link->ratio_y = 1;
    if((script_runtime_root->flags & 0x20) != 0 && node->parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        link->flags |= 0x80000000;
    }
    link->image_flags |= script_runtime_root->palette_flags;
    for(;;)
    {
        uint32_t code = parse_script_scope_code(parser);
        if(code == 0x00050000)
        {
            if(parse_script_value_token(parser, value, 0x20) != 0xffffffff)
            {
                RuntimeTreeSceneLink *scene = find_global_runtime_tree_scene_link_by_name(value);
                if(scene != nullptr)
                {
                    link->source_value = scene->z;
                }
            }
        }
        else if(code == 0x00060000)
        {
            invert_no_palette = true;
        }
        else if(code == 0x00100000)
        {
            expand_list = true;
            code = parse_script_value_token(parser, list_name, 0x20);
        }
        else if(code == 0x00300000)
        {
            const int32_t ratio_x = parse_script_integer_expression(parser);
            if(ratio_x != 0x7fffffff)
            {
                link->ratio_x = static_cast<uint32_t>(ratio_x);
            }
            code = static_cast<uint32_t>(parse_script_integer_expression(parser));
            if(code != 0x7fffffff)
            {
                link->ratio_y = code;
            }
        }
        else if(code == 0x00900000)
        {
            code = static_cast<uint32_t>(parse_script_integer_expression(parser));
            if(code != 0x7fffffff)
            {
                link->loop_count = code;
            }
        }
        else if(code == 0x00e00000)
        {
            parse_script_value_token(parser, value, 0x20);
            link->secondary_link = find_global_runtime_tree_secondary_resource_link_by_name(value);
        }
        else if(code == 0x00f00000)
        {
            parse_script_value_token(parser, value, 0x20);
            link->fixed_name_node = find_runtime_fixed_name_list_node(value);
        }
        else if(code == 0x01000000)
        {
            if(parse_script_file_value(parser, value, nullptr) != 0xffffffff && !fixed_dword_memory_equal(value, link->file_name, 0x20))
            {
                link->previous_resource_identity = link->resource_identity;
                link->resource_identity = nullptr;
                std::memcpy(link->file_name, value, sizeof(link->file_name));
            }
        }
        else if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 0x00200000)
            {
                link->ratio_x = 2;
                link->ratio_y = 2;
            }
            else if(code == 0x01000000)
            {
                link->flags |= 0x01000000;
            }
            else
            {
                link->image_flags |= code;
            }
        }
        else if(code == 0x0b000000)
        {
            const int32_t x = parse_script_integer_expression(parser);
            if(x != 0x7fffffff)
            {
                link->x = x;
            }
            const int32_t y = parse_script_integer_expression(parser);
            if(y != 0x7fffffff)
            {
                link->y = y;
            }
            const int32_t width = parse_script_integer_expression(parser);
            if(width != 0x7fffffff)
            {
                link->width = static_cast<uint32_t>(width);
            }
            code = static_cast<uint32_t>(parse_script_integer_expression(parser));
            if(code != 0x7fffffff)
            {
                link->height = code;
            }
        }
        else if(code == 0x0e000000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code == 0xffffffff)
            {
                break;
            }
            if(!script_object_container_state_matches_by_name(value))
            {
                runtime_named_node_memory_api.heap_free(script_runtime_root->heap, 0, link);
                return 0;
            }
        }
        if(code == 0xffffffff)
        {
            break;
        }
    }
    if(invert_no_palette)
    {
        link->image_flags ^= 0x04000000;
    }
    if(!expand_list)
    {
        if(node->primary_resource_link_tail == nullptr)
        {
            link->next = node->primary_resource_link_head;
            node->primary_resource_link_head = link;
            insert_runtime_tree_primary_resource_link(node, link);
        }
        else
        {
            link->next = node->primary_resource_link_tail->next;
            node->primary_resource_link_tail->next = link;
        }
        node->primary_resource_link_tail = link;
        if((node->flags & 0x400) != 0)
        {
            add_runtime_tree_auxiliary_name(node, link->file_name);
        }
        return 0;
    }

    RuntimeNamedNode *named = script_runtime_root->runtime_nodes;
    while(named != nullptr && !fixed_dword_memory_equal(list_name, named, 0x20))
    {
        named = named->next;
    }
    if(named != nullptr)
    {
        int32_t x = link->x;
        const int32_t y = link->y;
        auto *child = named->cache_entry_cursor;
        for(uint32_t index = 0; index < named->visible_entry_count; ++index)
        {
            char generated_name[0x80];
            append_three_digit_decimal_suffix(link->identifier, index, generated_name);
            void *resource_object = nullptr;
            void *primary_identity;
            uint32_t resource_value = 0;
            if(child == nullptr)
            {
                primary_identity = create_or_update_runtime_tree_primary_resource_link(node, generated_name, nullptr, static_cast<int32_t>(link->source_value), x, y, link->image_flags);
                if(primary_identity != nullptr)
                {
                    static_cast<RuntimeTreePrimaryResourceLink *>(primary_identity)->flags |= 0x80000000;
                }
            }
            else
            {
                resource_object = child->data;
                auto *expanded_resource = static_cast<RuntimeExpandedListResource *>(resource_object);
                resource_value = expanded_resource->link_value;
                primary_identity = create_or_update_runtime_tree_primary_resource_link(node, generated_name, expanded_resource->primary_resource_name, static_cast<int32_t>(link->source_value), x, y,
                    link->image_flags);
            }
            create_or_update_runtime_tree_link_0084(node, generated_name, x, y, x + named->zone_left, y + named->zone_top, 0, resource_object, primary_identity, reinterpret_cast<uintptr_t>(named),
                resource_value, named->zone_bottom);
            if(child != nullptr)
            {
                child = child->next;
                if(child == named->cache_entry_cursor)
                {
                    child = nullptr;
                }
            }
            x += named->zone_right + named->zone_left;
        }
    }
    runtime_named_node_memory_api.heap_free(script_runtime_root->heap, 0, link);
    return 0;
}

RuntimeTreeNode *get_runtime_tree_root()
{
    return script_runtime_root != nullptr ? script_runtime_root->runtime_tree : nullptr;
}

RuntimeTreeNode *find_runtime_tree_tail()
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *node = script_runtime_root->runtime_tree;
    if(node == nullptr)
    {
        return nullptr;
    }
    while(node->next != nullptr)
    {
        node = node->next;
    }
    return node;
}

RuntimeTreeNode *find_runtime_tree_ancestor_root(void *identity)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *node = find_runtime_tree_node(script_runtime_root->runtime_tree, identity);
    if(node == nullptr)
    {
        return nullptr;
    }
    while(node->parent != nullptr && node->parent != reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        node = node->parent;
    }
    return node;
}

RuntimeTreeSceneLink *find_global_runtime_tree_scene_link_by_name(const void *name)
{
    for(RuntimeTreeSceneLink *link = script_runtime_root->global_scene_links; link != nullptr; link = link->next)
    {
        if(fixed_dword_memory_equal(name, link, 0x20))
        {
            return link;
        }
    }
    return nullptr;
}

RuntimeTreeSceneLink *find_runtime_tree_scene_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreeSceneLink *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        RuntimeTreeSceneLink *link = static_cast<RuntimeTreeSceneLink *>(find_last_runtime_tree_scene_link(previous));
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->scene_link_tail;
}

void insert_runtime_tree_scene_link(RuntimeTreeNode *node, RuntimeTreeSceneLink *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->global_scene_link_tail != nullptr)
            {
                script_runtime_root->global_scene_link_tail->next = link;
            }
            else
            {
                script_runtime_root->global_scene_links = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->global_scene_link_tail != nullptr)
            {
                link->next = script_runtime_root->global_scene_link_tail->next;
                script_runtime_root->global_scene_link_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_scene_links;
                script_runtime_root->global_scene_links = link;
            }
            return;
        }
        RuntimeTreeSceneLink *previous = find_runtime_tree_scene_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->scene_link_head;
        parent->scene_link_head = link;
        node = parent;
    }
}

void remove_runtime_tree_scene_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->scene_link_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreeSceneLink *successor = node->scene_link_tail->next;
            if(script_runtime_root->global_scene_link_tail != nullptr)
            {
                script_runtime_root->global_scene_link_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_scene_links = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            RuntimeTreeSceneLink *previous = script_runtime_root->global_scene_links;
            if(previous == node->scene_link_head)
            {
                if(script_runtime_root->global_scene_link_tail == node->scene_link_tail)
                {
                    script_runtime_root->global_scene_link_tail = nullptr;
                }
                script_runtime_root->global_scene_links = node->scene_link_tail->next;
                return;
            }
            while(previous->next != node->scene_link_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_scene_link_tail == node->scene_link_tail)
            {
                script_runtime_root->global_scene_link_tail = previous;
            }
            previous->next = node->scene_link_tail->next;
            return;
        }
        if(parent->scene_link_head != node->scene_link_head)
        {
            RuntimeTreeSceneLink *previous = parent->scene_link_head;
            while(previous->next != node->scene_link_head)
            {
                previous = previous->next;
            }
            previous->next = node->scene_link_tail->next;
            return;
        }
        if(find_last_runtime_tree_scene_link(parent) == node->scene_link_tail)
        {
            parent->scene_link_head = nullptr;
        }
        else
        {
            parent->scene_link_head = node->scene_link_tail->next;
        }
        parent = parent->parent;
        if(node->scene_link_tail == nullptr)
        {
            return;
        }
    }
}

RuntimeTreeSecondaryResourceLink *find_global_runtime_tree_secondary_resource_link_by_name(const void *name)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    for(RuntimeTreeSecondaryResourceLink *link = script_runtime_root->global_secondary_resource_links; link != nullptr; link = link->next)
    {
        if(fixed_dword_memory_equal(name, link, 0x20))
        {
            return link;
        }
    }
    return nullptr;
}

RuntimeTreeSecondaryResourceLink *find_runtime_tree_secondary_resource_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreeSecondaryResourceLink *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        RuntimeTreeSecondaryResourceLink *link = static_cast<RuntimeTreeSecondaryResourceLink *>(find_last_runtime_tree_secondary_resource_link(previous));
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->secondary_resource_link_tail;
}

void insert_runtime_tree_secondary_resource_link(RuntimeTreeNode *node, RuntimeTreeSecondaryResourceLink *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->global_secondary_resource_link_tail != nullptr)
            {
                script_runtime_root->global_secondary_resource_link_tail->next = link;
            }
            else
            {
                script_runtime_root->global_secondary_resource_links = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->global_secondary_resource_link_tail != nullptr)
            {
                link->next = script_runtime_root->global_secondary_resource_link_tail->next;
                script_runtime_root->global_secondary_resource_link_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_secondary_resource_links;
                script_runtime_root->global_secondary_resource_links = link;
            }
            return;
        }
        RuntimeTreeSecondaryResourceLink *previous = find_runtime_tree_secondary_resource_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->secondary_resource_link_head;
        parent->secondary_resource_link_head = link;
        node = parent;
    }
}

void remove_runtime_tree_secondary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->secondary_resource_link_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreeSecondaryResourceLink *successor = node->secondary_resource_link_tail->next;
            if(script_runtime_root->global_secondary_resource_link_tail != nullptr)
            {
                script_runtime_root->global_secondary_resource_link_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_secondary_resource_links = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            RuntimeTreeSecondaryResourceLink *previous = script_runtime_root->global_secondary_resource_links;
            if(previous == node->secondary_resource_link_head)
            {
                if(script_runtime_root->global_secondary_resource_link_tail == node->secondary_resource_link_tail)
                {
                    script_runtime_root->global_secondary_resource_link_tail = nullptr;
                }
                script_runtime_root->global_secondary_resource_links = node->secondary_resource_link_tail->next;
                return;
            }
            while(previous->next != node->secondary_resource_link_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_secondary_resource_link_tail == node->secondary_resource_link_tail)
            {
                script_runtime_root->global_secondary_resource_link_tail = previous;
            }
            previous->next = node->secondary_resource_link_tail->next;
            return;
        }
        if(parent->secondary_resource_link_head != node->secondary_resource_link_head)
        {
            RuntimeTreeSecondaryResourceLink *previous = parent->secondary_resource_link_head;
            while(previous->next != node->secondary_resource_link_head)
            {
                previous = previous->next;
            }
            previous->next = node->secondary_resource_link_tail->next;
            return;
        }
        if(find_last_runtime_tree_secondary_resource_link(parent) == node->secondary_resource_link_tail)
        {
            parent->secondary_resource_link_head = nullptr;
        }
        else
        {
            parent->secondary_resource_link_head = node->secondary_resource_link_tail->next;
        }
        parent = parent->parent;
        if(node->secondary_resource_link_tail == nullptr)
        {
            return;
        }
    }
}

RuntimeTreePrimaryResourceLink *find_runtime_tree_primary_resource_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreePrimaryResourceLink *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        auto *link = static_cast<RuntimeTreePrimaryResourceLink *>(find_last_runtime_tree_primary_resource_link(previous));
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->primary_resource_link_tail;
}

void insert_runtime_tree_primary_resource_link(RuntimeTreeNode *node, RuntimeTreePrimaryResourceLink *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->plan_terminal != nullptr)
            {
                script_runtime_root->global_primary_resource_link_tail->next = link;
            }
            else
            {
                script_runtime_root->global_primary_resource_links = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->plan_terminal != nullptr)
            {
                link->next = script_runtime_root->global_primary_resource_link_tail->next;
                script_runtime_root->global_primary_resource_link_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_primary_resource_links;
                script_runtime_root->global_primary_resource_links = link;
            }
            return;
        }
        RuntimeTreePrimaryResourceLink *previous = find_runtime_tree_primary_resource_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->primary_resource_link_head;
        parent->primary_resource_link_head = link;
        node = parent;
    }
}

void remove_runtime_tree_primary_resource_link_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->primary_resource_link_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreePrimaryResourceLink *successor = node->primary_resource_link_tail->next;
            if(script_runtime_root->plan_terminal != nullptr)
            {
                script_runtime_root->global_primary_resource_link_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_primary_resource_links = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            auto *previous = script_runtime_root->global_primary_resource_links;
            if(previous == node->primary_resource_link_head)
            {
                if(script_runtime_root->global_primary_resource_link_tail == node->primary_resource_link_tail)
                {
                    script_runtime_root->plan_terminal = nullptr;
                }
                script_runtime_root->global_primary_resource_links = node->primary_resource_link_tail->next;
                return;
            }
            while(previous->next != node->primary_resource_link_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_primary_resource_link_tail == node->primary_resource_link_tail)
            {
                script_runtime_root->global_primary_resource_link_tail = previous;
            }
            previous->next = node->primary_resource_link_tail->next;
            return;
        }
        if(parent->primary_resource_link_head != node->primary_resource_link_head)
        {
            RuntimeTreePrimaryResourceLink *previous = parent->primary_resource_link_head;
            while(previous->next != node->primary_resource_link_head)
            {
                previous = previous->next;
            }
            previous->next = node->primary_resource_link_tail->next;
            return;
        }
        if(find_last_runtime_tree_primary_resource_link(parent) == node->primary_resource_link_tail)
        {
            parent->primary_resource_link_head = nullptr;
        }
        else
        {
            parent->primary_resource_link_head = node->primary_resource_link_tail->next;
        }
        parent = parent->parent;
        if(node->primary_resource_link_tail == nullptr)
        {
            return;
        }
    }
}

void update_runtime_tree_primary_resource_link(void *tree_identity, void *link_identity, const void *name, int32_t x_delta, int32_t y_delta, uint32_t image_flags)
{
    RuntimeTreeNode *node = find_runtime_tree_node(script_runtime_root->runtime_tree, tree_identity);
    if(node == nullptr)
    {
        return;
    }
    RuntimeTreePrimaryResourceLink *link = node->primary_resource_link_head;
    while(link != nullptr && link->identity != link_identity)
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        return;
    }
    if(name != nullptr && !fixed_dword_memory_equal(link->file_name, name, 0x20))
    {
        std::memcpy(link->file_name, name, 0x20);
        link->previous_resource_identity = link->resource_identity;
        link->resource_identity = nullptr;
    }
    if(x_delta != 0)
    {
        link->previous_x = link->x;
        link->x += x_delta;
    }
    if(y_delta != 0)
    {
        link->previous_y = link->y;
        link->y += y_delta;
    }
    if(image_flags != 0)
    {
        link->image_flags = image_flags;
    }
}

void append_three_digit_decimal_suffix(const char *prefix, uint32_t value, char *output)
{
    std::memset(output, 0, 0x20);
    size_t index = 0;
    while(prefix[index] != '\0')
    {
        output[index] = prefix[index];
        ++index;
    }
    for(uint32_t divisor = 100; divisor != 0; divisor /= 10)
    {
        uint32_t digit = value / divisor;
        output[index++] = static_cast<char>(digit + '0');
        value -= divisor * digit;
    }
    output[index] = '\0';
}

uint32_t parse_runtime_tree_link_0084(ScriptParserState *parser)
{
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == 0xffffffff)
    {
        return 0;
    }
    auto *link = static_cast<RuntimeTreeLink84 *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeLink84)));
    if(link == nullptr)
    {
        return 0;
    }
    RuntimeTreeNode *node = parser->owner;
    std::memcpy(link->name, value, sizeof(link->name));
    link->identity = link;
    uint32_t code;
    do
    {
        code = parse_script_scope_code(parser);
        if(code == 0x00b00000)
        {
            const int32_t parameter = parse_script_integer_expression(parser);
            if(parameter != 0x7fffffff)
            {
                link->parameter = parameter;
            }
            code = static_cast<uint32_t>(parameter);
        }
        else if(code == 0x00800000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                link->primary_resource = find_global_runtime_tree_primary_resource_link_by_name(value);
            }
        }
        else if(code == 0x0b000000 || code == 0x02000000)
        {
            const bool position = code == 0x0b000000;
            const int32_t x = parse_script_integer_expression(parser);
            if(x != 0x7fffffff)
            {
                link->x = x;
            }
            const int32_t y = parse_script_integer_expression(parser);
            if(y != 0x7fffffff)
            {
                link->y = y;
            }
            const int32_t width = parse_script_integer_expression(parser);
            if(width != 0x7fffffff)
            {
                link->width = position ? width + link->x : width;
            }
            const int32_t height = parse_script_integer_expression(parser);
            if(height != 0x7fffffff)
            {
                link->height = position ? height + link->y : height;
            }
            code = static_cast<uint32_t>(height);
        }
        else if(code == 0x0d000000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                for(RuntimeVisualObject *visual = script_runtime_root->visual_objects; visual != nullptr; visual = visual->next)
                {
                    if(fixed_dword_memory_equal(value, visual, 0x20))
                    {
                        link->mouse_visual = static_cast<RuntimeVisualObject *>(visual->identity);
                        break;
                    }
                }
            }
        }
        else if(code == 0x0c000000 || code == 0x10000000)
        {
            const uint32_t command_code = code;
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                for(uint32_t index = 0; index < script_runtime_root->command_definition_count; ++index)
                {
                    if(fixed_dword_memory_equal(value, script_runtime_root->command_definitions[index].name, 0x20))
                    {
                        const uint32_t bit = 1u << (index & 31);
                        link->command_mask |= bit;
                        if(command_code == 0x10000000)
                        {
                            link->primary_command_bit = bit;
                        }
                        break;
                    }
                }
            }
        }
        else if(code == 0x0e000000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff && !script_object_container_state_matches_by_name(value))
            {
                runtime_named_node_memory_api.heap_free(script_runtime_root->heap, 0, link);
                return 0;
            }
        }
        else if(code == 0x30000000)
        {
            parse_script_value_token(parser, value, 0x20);
            link->owner_object = find_script_object_by_name(value);
        }
    } while(code != 0xffffffff);

    if(node->link_0084_tail == nullptr)
    {
        link->next = node->link_0084_head;
        node->link_0084_head = link;
        insert_runtime_tree_link_0084(node, link);
    }
    else
    {
        link->next = node->link_0084_tail->next;
        node->link_0084_tail->next = link;
    }
    node->link_0084_tail = link;
    return 0;
}

void *create_or_update_runtime_tree_primary_resource_link(void *tree_identity, const void *identifier, const void *file_name, int32_t source_value, int32_t x_delta, int32_t y_delta,
    uint32_t image_flags)
{
    RuntimeTreeNode *node = find_runtime_tree_node(script_runtime_root->runtime_tree, tree_identity);
    if(node == nullptr)
    {
        return nullptr;
    }
    RuntimeTreePrimaryResourceLink *link = node->primary_resource_link_head;
    while(link != nullptr && !fixed_dword_memory_equal(identifier, link, 0x20))
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        link = static_cast<RuntimeTreePrimaryResourceLink *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreePrimaryResourceLink)));
        if(link == nullptr)
        {
            return nullptr;
        }
        std::memcpy(link->identifier, identifier, sizeof(link->identifier));
        link->identity = link;
        if(node->primary_resource_link_tail == nullptr)
        {
            link->next = node->primary_resource_link_head;
            node->primary_resource_link_head = link;
            insert_runtime_tree_primary_resource_link(node, link);
        }
        else
        {
            link->next = node->primary_resource_link_tail->next;
            node->primary_resource_link_tail->next = link;
        }
        node->primary_resource_link_tail = link;
    }
    if(file_name != nullptr && !fixed_dword_memory_equal(link->file_name, file_name, 0x20))
    {
        std::memcpy(link->file_name, file_name, sizeof(link->file_name));
        link->previous_resource_identity = link->resource_identity;
        link->resource_identity = nullptr;
    }
    if(x_delta != 0x7fffffff)
    {
        link->previous_x = link->x;
        link->x += x_delta;
    }
    if(y_delta != 0x7fffffff)
    {
        link->previous_y = link->y;
        link->y += y_delta;
    }
    if(source_value != 0x7fffffff)
    {
        link->source_value = static_cast<uint32_t>(source_value);
    }
    if(image_flags != 0)
    {
        link->image_flags = image_flags;
    }
    return link->identity;
}

void *create_or_update_runtime_tree_link_0084(void *tree_identity, const void *name, int32_t x, int32_t y, uint32_t width, uint32_t height, uintptr_t mouse_visual_value, void *owner_identity,
    void *primary_resource_identity, uintptr_t owner_group_identity, uint32_t command_mask, uint32_t parameter)
{
    RuntimeTreeNode *node = find_runtime_tree_node(script_runtime_root->runtime_tree, tree_identity);
    if(node == nullptr)
    {
        return nullptr;
    }
    bool existing = false;
    RuntimeTreeLink84 *link = node->link_0084_head;
    while(link != nullptr && !fixed_dword_memory_equal(name, link, 0x20))
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        link = static_cast<RuntimeTreeLink84 *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeLink84)));
        if(link == nullptr)
        {
            return nullptr;
        }
        std::memcpy(link->name, name, sizeof(link->name));
        link->owner_group_identity = owner_group_identity;
        link->identity = link;
        if(node->link_0084_tail == nullptr)
        {
            link->next = node->link_0084_head;
            node->link_0084_head = link;
            insert_runtime_tree_link_0084(node, link);
        }
        else
        {
            link->next = node->link_0084_tail->next;
            node->link_0084_tail->next = link;
        }
        node->link_0084_tail = link;
    }
    else
    {
        existing = true;
    }
    link->movement_deadline = 0xffffffff;
    if(parameter != 0x7fffffff)
    {
        link->parameter = parameter;
    }
    if(command_mask != 0)
    {
        link->command_mask = command_mask;
    }
    if(mouse_visual_value != 0)
    {
        link->mouse_visual_value = mouse_visual_value;
    }
    if(owner_identity != nullptr && owner_identity != link->owner_identity)
    {
        link->previous_owner_identity = link->owner_identity;
        link->owner_identity = owner_identity;
    }
    if(primary_resource_identity != nullptr && primary_resource_identity != link->primary_resource_identity)
    {
        link->previous_primary_resource_identity = link->primary_resource_identity;
        link->primary_resource_identity = primary_resource_identity;
    }
    if(x != 0 || y != 0 || width != 0 || height != 0)
    {
        if(existing && link->primary_resource_identity != nullptr)
        {
            update_runtime_tree_primary_resource_link(tree_identity, link->primary_resource_identity, nullptr, x - link->x, y - link->y, 0);
        }
        link->x = x;
        link->y = y;
        link->width = width;
        link->height = height;
    }
    return link->identity;
}

RuntimeTreePrimaryResourceLink *find_global_runtime_tree_primary_resource_link_by_name(const void *name)
{
    auto *link = script_runtime_root->global_primary_resource_links;
    while(link != nullptr)
    {
        if(fixed_dword_memory_equal(name, link, 0x20))
        {
            return link;
        }
        link = link->next;
    }
    return nullptr;
}

RuntimeTreeLink84 *find_last_runtime_tree_link_0084(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            RuntimeTreeLink84 *result = find_last_runtime_tree_link_0084(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->link_0084_tail;
}

RuntimeTreeLink84 *find_runtime_tree_link_0084_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreeLink84 *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        RuntimeTreeLink84 *link = find_last_runtime_tree_link_0084(previous);
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->link_0084_tail;
}

void insert_runtime_tree_link_0084(RuntimeTreeNode *node, RuntimeTreeLink84 *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->global_link_0084_tail != nullptr)
            {
                script_runtime_root->global_link_0084_tail->next = link;
            }
            else
            {
                script_runtime_root->global_link_0084_head = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->global_link_0084_tail != nullptr)
            {
                link->next = script_runtime_root->global_link_0084_tail->next;
                script_runtime_root->global_link_0084_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_link_0084_head;
                script_runtime_root->global_link_0084_head = link;
            }
            return;
        }
        RuntimeTreeLink84 *previous = find_runtime_tree_link_0084_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->link_0084_head;
        parent->link_0084_head = link;
        node = parent;
    }
}

void remove_runtime_tree_link_0084_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->link_0084_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreeLink84 *successor = node->link_0084_tail->next;
            if(script_runtime_root->global_link_0084_tail != nullptr)
            {
                script_runtime_root->global_link_0084_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_link_0084_head = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            RuntimeTreeLink84 *previous = script_runtime_root->global_link_0084_head;
            if(previous == node->link_0084_head)
            {
                if(script_runtime_root->global_link_0084_tail == node->link_0084_tail)
                {
                    script_runtime_root->global_link_0084_tail = nullptr;
                }
                script_runtime_root->global_link_0084_head = node->link_0084_tail->next;
                return;
            }
            while(previous->next != node->link_0084_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_link_0084_tail == node->link_0084_tail)
            {
                script_runtime_root->global_link_0084_tail = previous;
            }
            previous->next = node->link_0084_tail->next;
            return;
        }
        if(parent->link_0084_head != node->link_0084_head)
        {
            RuntimeTreeLink84 *previous = parent->link_0084_head;
            while(previous->next != node->link_0084_head)
            {
                previous = previous->next;
            }
            previous->next = node->link_0084_tail->next;
            return;
        }
        if(find_last_runtime_tree_link_0084(parent) == node->link_0084_tail)
        {
            parent->link_0084_head = nullptr;
        }
        else
        {
            parent->link_0084_head = node->link_0084_tail->next;
        }
        parent = parent->parent;
        if(node->link_0084_tail == nullptr)
        {
            return;
        }
    }
}

void update_runtime_tree_link_0084(void *tree_identity, void *link_identity, int32_t x, int32_t y, uint32_t width, uint32_t height, uintptr_t mouse_visual_value, void *owner_identity,
    void *primary_resource_identity, uintptr_t owner_group_identity, uint32_t command_mask, uint32_t parameter)
{
    RuntimeTreeNode *node = find_runtime_tree_node(script_runtime_root->runtime_tree, tree_identity);
    if(node == nullptr)
    {
        return;
    }
    RuntimeTreeLink84 *link = node->link_0084_head;
    while(link != nullptr && link->identity != link_identity)
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        return;
    }
    if(parameter != 0x7fffffff)
    {
        link->parameter = parameter;
    }
    if(command_mask != 0)
    {
        link->command_mask = command_mask;
    }
    if(mouse_visual_value != 0)
    {
        link->mouse_visual_value = mouse_visual_value;
    }
    if(owner_identity != nullptr && owner_identity != link->owner_identity)
    {
        link->previous_owner_identity = link->owner_identity;
        link->owner_identity = owner_identity;
    }
    if(primary_resource_identity != nullptr && primary_resource_identity != link->primary_resource_identity)
    {
        link->previous_primary_resource_identity = link->primary_resource_identity;
        link->primary_resource_identity = primary_resource_identity;
    }
    if(owner_group_identity != 0)
    {
        link->owner_group_identity = owner_group_identity;
    }
    if(x != 0 || y != 0 || width != 0 || height != 0)
    {
        if(link->primary_resource_identity != nullptr)
        {
            update_runtime_tree_primary_resource_link(tree_identity, link->primary_resource_identity, nullptr, x - link->x, y - link->y, 0);
        }
        link->x = x;
        link->y = y;
        link->width = width;
        link->height = height;
    }
}

RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_name(const void *name)
{
    for(RuntimeTreeLink84 *link = script_runtime_root->global_link_0084_head; link != nullptr; link = link->next)
    {
        if(fixed_dword_memory_equal(name, link, 0x20))
        {
            return link;
        }
    }
    return nullptr;
}

RuntimeTreeLink84 *find_global_runtime_tree_link_0084_by_identity(void *identity)
{
    for(RuntimeTreeLink84 *link = script_runtime_root->global_link_0084_head; link != nullptr; link = link->next)
    {
        if(link->identity == identity)
        {
            return link;
        }
    }
    return nullptr;
}

uint32_t parse_runtime_tree_link_008c(ScriptParserState *parser)
{
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == 0xffffffff)
    {
        return 0;
    }
    auto *link = static_cast<RuntimeTreeLink8C *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeLink8C)));
    if(link == nullptr)
    {
        return 0;
    }
    RuntimeTreeNode *node = parser->owner;
    std::memcpy(link->name, value, sizeof(link->name));
    link->identity = link;
    uint32_t code;
    do
    {
        code = parse_script_scope_code(parser);
        if(code == 0x00500000)
        {
            const int32_t first = parse_script_integer_expression(parser);
            if(first != 0x7fffffff)
            {
                link->line_first = first;
            }
            const int32_t second = parse_script_integer_expression(parser);
            if(second != 0x7fffffff)
            {
                link->line_second = second;
            }
            link->flags |= 1;
            code = static_cast<uint32_t>(second);
        }
        else if(code == 0x00400000)
        {
            link->flags |= 2;
        }
        else if(code == 0x00600000)
        {
            const int32_t time = parse_script_integer_expression(parser);
            if(time != 0x7fffffff)
            {
                link->time = time;
            }
            code = static_cast<uint32_t>(time);
        }
        else if(code == 0x02000000)
        {
            const int32_t x = parse_script_integer_expression(parser);
            if(x != 0x7fffffff)
            {
                link->x = x;
            }
            const int32_t y = parse_script_integer_expression(parser);
            if(y != 0x7fffffff)
            {
                link->y = y;
            }
            const int32_t width = parse_script_integer_expression(parser);
            if(width != 0x7fffffff)
            {
                link->width = width;
            }
            const int32_t height = parse_script_integer_expression(parser);
            if(height != 0x7fffffff)
            {
                link->height = height;
            }
            code = static_cast<uint32_t>(height);
        }
    } while(code != 0xffffffff);

    if(node->link_008c_tail == nullptr)
    {
        link->next = node->link_008c_head;
        node->link_008c_head = link;
        insert_runtime_tree_link_008c(node, link);
    }
    else
    {
        link->next = node->link_008c_tail->next;
        node->link_008c_tail->next = link;
    }
    node->link_008c_tail = link;
    return 0;
}

uint32_t parse_runtime_tree_link_007c(ScriptParserState *parser)
{
    char value[0x80];
    if(parse_script_value_token(parser, value, 0x20) == 0xffffffff)
    {
        return 0;
    }
    auto *link = static_cast<RuntimeTreeLink7C *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeTreeLink7C)));
    if(link == nullptr)
    {
        return 0;
    }
    RuntimeTreeNode *node = parser->owner;
    std::memcpy(link->name, value, sizeof(link->name));
    link->identity = link;
    std::memcpy(&link->parser, parser, sizeof(link->parser));
    link->parser.start_offset = parser->cursor;
    link->parser.cursor = parser->cursor;
    if((node->flags & 0x200) != 0)
    {
        link->owner_flags |= 0x10000000;
    }

    uint32_t code;
    do
    {
        code = parse_script_scope_code(parser);
        if(code == 0x00020000)
        {
            link->flags |= 0x00100000;
        }
        else if(code == 0x00030000)
        {
            link->flags |= 0x00200000;
        }
        else if(code == 0x00a00000)
        {
            const int32_t minimum = parse_script_integer_expression(parser);
            if(minimum != 0x7fffffff)
            {
                link->random_minimum = minimum;
            }
            const int32_t maximum = parse_script_integer_expression(parser);
            if(maximum != 0x7fffffff)
            {
                link->random_maximum = maximum;
            }
            link->flags |= 0x80;
            code = static_cast<uint32_t>(maximum);
        }
        else if(code == 0x00800000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                link->primary_resource = find_global_runtime_tree_primary_resource_link_by_name(value);
                if(link->primary_resource != nullptr)
                {
                    link->flags |= 0x40;
                }
            }
        }
        else if(code == 0x00d00000)
        {
            link->flags |= 0x200;
        }
        else if(code == 0x00c00000)
        {
            link->flags |= 0x10000000;
        }
        else if(code == 0x05000000 || code == 0x06000000)
        {
            const bool source = code == 0x05000000;
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                for(ScriptObjectState *object = script_runtime_root->objects; object != nullptr; object = object->next)
                {
                    if(fixed_dword_memory_equal(value, object, 0x20))
                    {
                        if(source)
                        {
                            link->source_object = object;
                            link->flags |= 2;
                        }
                        else
                        {
                            link->destination_object = object;
                            link->flags |= 4;
                        }
                        break;
                    }
                }
            }
        }
        else if(code == 0x02000000)
        {
            const int32_t x = parse_script_integer_expression(parser);
            if(x != 0x7fffffff)
            {
                link->x = x;
            }
            const int32_t y = parse_script_integer_expression(parser);
            if(y != 0x7fffffff)
            {
                link->y = y;
            }
            const int32_t width = parse_script_integer_expression(parser);
            if(width != 0x7fffffff)
            {
                link->width = width;
            }
            const int32_t height = parse_script_integer_expression(parser);
            if(height != 0x7fffffff)
            {
                link->height = height;
            }
            link->flags |= 0x20;
            code = static_cast<uint32_t>(height);
        }
        else if(code == 0x0c000000)
        {
            code = parse_script_value_token(parser, value, 0x20);
            if(code != 0xffffffff)
            {
                for(uint32_t index = 0; index < script_runtime_root->command_definition_count; ++index)
                {
                    if(fixed_dword_memory_equal(value, script_runtime_root->command_definitions[index].name, 0x20))
                    {
                        link->command_bit = 1u << (index & 31);
                        link->flags |= 1;
                        break;
                    }
                }
            }
        }
        else if(code == 0x0e000000)
        {
            const uint32_t result = parse_script_value_token(parser, value, 0x20);
            if(result != 0xffffffff)
            {
                link->condition = find_script_condition_container_by_name(value);
                if(link->condition != nullptr)
                {
                    link->flags |= 0x100;
                }
            }
        }
        else if(code == 0x0f000000)
        {
            const uint32_t result = parse_script_value_token(parser, value, 0x20);
            if(result != 0xffffffff)
            {
                link->zone_link = find_global_runtime_tree_link_0084_by_name(value);
                if(link->zone_link != nullptr)
                {
                    link->flags |= 8;
                }
            }
        }
    } while(code != 0xffffffff);

    if(node->link_007c_tail == nullptr)
    {
        link->next = node->link_007c_head;
        node->link_007c_head = link;
        insert_runtime_tree_link_007c(node, link);
    }
    else
    {
        link->next = node->link_007c_tail->next;
        node->link_007c_tail->next = link;
    }
    node->link_007c_tail = link;
    return 0;
}

RuntimeTreeInteractionCriteria make_runtime_tree_interaction_criteria(const RuntimeTreeLink7C *link)
{
    return { link->command_bit, link->source_object, link->destination_object, link->zone_link, link->unknown_0084, link->x, link->y, link->width, link->height, link->primary_resource,
        link->condition, link->random_minimum, link->random_maximum, link->unknown_00a8, link->flags, link->unknown_00b0, link };
}

uint32_t match_runtime_tree_link_007c_interaction_internal(uintptr_t *state, const RuntimeTreeInteractionCriteria *criteria, const RuntimeTreeLink7C *criteria_link)
{
    uint32_t criteria_flags = criteria->flags;
    if(criteria_flags == 0)
    {
        return 1;
    }
    const uint32_t state_flags = static_cast<uint32_t>(state[14]);
    if((criteria_flags & 0x00100000) != 0)
    {
        criteria_flags |= state_flags & 0x10000000;
    }
    if((criteria_flags & 0x0f) != 0)
    {
        if((criteria_flags & state_flags) != (criteria_flags & 0xf000000f))
        {
            return 0;
        }
        if(((criteria_flags ^ state_flags) & 0xf0000000) != 0)
        {
            return 0;
        }
    }
    for(uint32_t index = 0; index < 5; ++index)
    {
        const uintptr_t values[]{ criteria->command_bit, reinterpret_cast<uintptr_t>(criteria->source_object), reinterpret_cast<uintptr_t>(criteria->destination_object),
            reinterpret_cast<uintptr_t>(criteria->zone_link), criteria->unknown_0084 };
        if((criteria_flags & (1u << index)) != 0 && values[index] != state[index])
        {
            return 0;
        }
    }
    if((criteria_flags & 0x80) != 0)
    {
        const int32_t value = select_bounded_random_value(-10000, 10000);
        if(value < criteria->random_minimum || value > criteria->random_maximum)
        {
            return 0;
        }
    }
    if((criteria_flags & 0x100) != 0 && !script_object_container_state_matches_by_identity(criteria->condition))
    {
        return 0;
    }
    if((criteria_flags & 0x40) != 0)
    {
        RuntimeTreePrimaryResourceLink *primary = criteria->primary_resource;
        if(primary == nullptr)
        {
            return 0;
        }
        if((criteria_flags & 0x20) != 0)
        {
            if(static_cast<int32_t>(criteria->width) < primary->x || static_cast<int32_t>(criteria->height) < primary->y || primary->x + static_cast<int32_t>(primary->width) < criteria->x
                || primary->y + static_cast<int32_t>(primary->height) < criteria->y)
            {
                return 0;
            }
        }
    }
    if((criteria_flags & 0x00200000) != 0)
    {
        for(RuntimeTreeLink7C *candidate = script_runtime_root->global_link_007c_head; candidate != nullptr; candidate = candidate->next)
        {
            if(candidate != criteria_link && ((candidate->flags ^ state_flags) & 0xf0000fff) == 0)
            {
                const RuntimeTreeInteractionCriteria candidate_criteria = make_runtime_tree_interaction_criteria(candidate);
                uintptr_t state_copy[16];
                std::memcpy(state_copy, state, sizeof(state_copy));
                if(match_runtime_tree_link_007c_interaction_internal(state_copy, &candidate_criteria, candidate) != 0)
                {
                    return 0;
                }
            }
        }
        state[14] = criteria_flags;
    }
    if((criteria_flags & 0x200) == 0)
    {
        state[14] &= ~criteria_flags | 0x0ffffff0;
    }
    return 1;
}

uint32_t match_runtime_tree_link_007c_interaction(uintptr_t *state, const RuntimeTreeInteractionCriteria *criteria)
{
    return match_runtime_tree_link_007c_interaction_internal(state, criteria, criteria->source_link);
}

void seek_runtime_tree_link_007c_label(void *identity, const char *label)
{
    RuntimeTreeLink7C *link = script_runtime_root->global_link_007c_head;
    while(link != nullptr && link->identity != identity)
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        return;
    }
    const uint32_t saved_cursor = link->parser.cursor;
    link->parser.cursor = link->parser.start_offset;
    uint32_t opcode;
    do
    {
        opcode = parse_script_opcode(&link->parser);
        if(opcode == 0x000a0000)
        {
            char value[0x20];
            opcode = parse_script_value_token(&link->parser, value, sizeof(value));
            if(opcode == 0xffffffff)
            {
                break;
            }
            if(strings_equal(value, label))
            {
                return;
            }
        }
    } while(opcode != 0xffffffff);
    link->parser.cursor = saved_cursor;
}

uint32_t find_runtime_tree_link_007c_opcode_value(void *identity, uint32_t opcode, const char *value, int restore_cursor)
{
    RuntimeTreeLink7C *link = script_runtime_root->global_link_007c_head;
    while(link != nullptr && link->identity != identity)
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        return 0xffffffff;
    }
    const uint32_t saved_cursor = link->parser.cursor;
    link->parser.cursor = link->parser.start_offset;
    uint32_t result;
    do
    {
        result = parse_script_opcode(&link->parser);
        if(result == opcode)
        {
            char parsed_value[0x20];
            result = parse_script_value_token(&link->parser, parsed_value, sizeof(parsed_value));
            if(result == 0xffffffff)
            {
                break;
            }
            if(strings_equal(parsed_value, value))
            {
                result = link->parser.cursor;
                if(restore_cursor == 0)
                {
                    return result;
                }
                break;
            }
        }
    } while(result != 0xffffffff);
    link->parser.cursor = saved_cursor;
    return result;
}

uint32_t scan_runtime_tree_link_007c_control_boundary(void *identity, uint32_t requested_boundary)
{
    RuntimeTreeLink7C *link = script_runtime_root->global_link_007c_head;
    while(link != nullptr && link->identity != identity)
    {
        link = link->next;
    }
    if(link == nullptr)
    {
        return 0;
    }
    int32_t nesting = 0;
    for(;;)
    {
        const uint32_t opcode = parse_script_opcode(&link->parser);
        if(opcode == 0x00006000)
        {
            if(nesting == 0)
            {
                return opcode;
            }
            --nesting;
        }
        else if(opcode == 0x00004000 || opcode == 0x00040000 || opcode == 0x00050000)
        {
            ++nesting;
        }
        else if(opcode == 0x00060000 && requested_boundary == 0x00060000 && nesting == 0)
        {
            return opcode;
        }
        if(opcode == 0xffffffff)
        {
            return opcode;
        }
    }
}

uint32_t activate_runtime_tree_link_007c(RuntimeTreeLink7C *link)
{
    ScriptRuntimeRoot *root = script_runtime_root;
    if(root == nullptr || link == nullptr || (link->owner_flags & 0x80000000) != 0)
    {
        return root != nullptr && link != nullptr ? 1u : 0u;
    }
    const uint32_t index = root->transient_index_1;
    const bool empty = root->transient_index_2 == index;
    if(empty)
    {
        root->event_records[index][14] = 0;
    }
    const RuntimeTreeInteractionCriteria criteria = make_runtime_tree_interaction_criteria(link);
    const uint32_t matched = match_runtime_tree_link_007c_interaction_internal(root->event_records[index], &criteria, link);
    if(matched != 0)
    {
        link->owner_flags |= 0x80000000;
        if(!empty && (link->flags & 0xf000000f) != 0 && (root->event_records[index][14] & 0xf000000f) == 0)
        {
            ++root->transient_index_1;
            if(root->transient_index_1 == 0x20)
            {
                root->transient_index_1 = 0;
            }
        }
        return 1;
    }
    return 0;
}

RuntimeTreeLink7C *find_last_runtime_tree_link_007c(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            RuntimeTreeLink7C *result = find_last_runtime_tree_link_007c(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->link_007c_tail;
}

RuntimeTreeLink7C *find_runtime_tree_link_007c_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreeLink7C *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        RuntimeTreeLink7C *link = find_last_runtime_tree_link_007c(previous);
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->link_007c_tail;
}

void insert_runtime_tree_link_007c(RuntimeTreeNode *node, RuntimeTreeLink7C *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->global_link_007c_tail != nullptr)
            {
                script_runtime_root->global_link_007c_tail->next = link;
            }
            else
            {
                script_runtime_root->global_link_007c_head = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->global_link_007c_tail != nullptr)
            {
                link->next = script_runtime_root->global_link_007c_tail->next;
                script_runtime_root->global_link_007c_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_link_007c_head;
                script_runtime_root->global_link_007c_head = link;
            }
            return;
        }
        RuntimeTreeLink7C *previous = find_runtime_tree_link_007c_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->link_007c_head;
        parent->link_007c_head = link;
        node = parent;
    }
}

void remove_runtime_tree_link_007c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->link_007c_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreeLink7C *successor = node->link_007c_tail->next;
            if(script_runtime_root->global_link_007c_tail != nullptr)
            {
                script_runtime_root->global_link_007c_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_link_007c_head = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            RuntimeTreeLink7C *previous = script_runtime_root->global_link_007c_head;
            if(previous == node->link_007c_head)
            {
                if(script_runtime_root->global_link_007c_tail == node->link_007c_tail)
                {
                    script_runtime_root->global_link_007c_tail = nullptr;
                }
                script_runtime_root->global_link_007c_head = node->link_007c_tail->next;
                return;
            }
            while(previous->next != node->link_007c_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_link_007c_tail == node->link_007c_tail)
            {
                script_runtime_root->global_link_007c_tail = previous;
            }
            previous->next = node->link_007c_tail->next;
            return;
        }
        if(parent->link_007c_head != node->link_007c_head)
        {
            RuntimeTreeLink7C *previous = parent->link_007c_head;
            while(previous->next != node->link_007c_head)
            {
                previous = previous->next;
            }
            previous->next = node->link_007c_tail->next;
            return;
        }
        if(find_last_runtime_tree_link_007c(parent) == node->link_007c_tail)
        {
            parent->link_007c_head = nullptr;
        }
        else
        {
            parent->link_007c_head = node->link_007c_tail->next;
        }
        parent = parent->parent;
        if(node->link_007c_tail == nullptr)
        {
            return;
        }
    }
}

ScriptObjectContainer *find_last_script_object_container(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            ScriptObjectContainer *result = find_last_script_object_container(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->container_tail;
}

ScriptObjectContainer *find_script_object_container_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<ScriptObjectContainer *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        ScriptObjectContainer *container = find_last_script_object_container(previous);
        if(container != nullptr)
        {
            return container;
        }
    }
    return parent->container_tail;
}

uint32_t parse_script_object_container(ScriptParserState *parser)
{
    char object_name[0x80];
    char field_name[0x80];
    uint32_t value[32];
    uint32_t value_type;
    if(parse_script_value_token(parser, object_name, 0x20) == 0xffffffff)
    {
        return 0;
    }

    auto *container = static_cast<ScriptObjectContainer *>(runtime_named_node_memory_api.heap_alloc(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(ScriptObjectContainer)));
    if(container == nullptr)
    {
        return 0;
    }

    RuntimeTreeNode *node = parser->owner;
    std::memcpy(container->name, object_name, sizeof(container->name));
    container->identity = container;
    if(node->container_tail == nullptr)
    {
        container->next = node->container_head;
        node->container_head = container;
        insert_script_object_container(node, container);
    }
    else
    {
        container->next = node->container_tail->next;
        node->container_tail->next = container;
    }
    node->container_tail = container;

    uint32_t truth_value = 0;
    while(true)
    {
        if(parse_script_value_token(parser, object_name, 0x20) == 0xffffffff)
        {
            return 0;
        }
        if(parse_script_value_token(parser, field_name, 0x20) == 0xffffffff)
        {
            return 0;
        }
        parse_script_typed_value(parser, value, &value_type);

        const uint32_t parsed_value = value[0];
        uint32_t next_truth_value = truth_value;
        if(!strings_equal(object_name, "GLOBAL_SYSTEM_STATE"))
        {
            if(value_type != 0)
            {
                next_truth_value = value[0];
                if(value_type > 2)
                {
                    next_truth_value = truth_value;
                    if(value_type == 4)
                    {
                        next_truth_value = value[0] & 0xff;
                    }
                }
            }

            ScriptObjectState *object = find_script_object_by_name(object_name);
            if(object == nullptr)
            {
                object = create_script_object_state(object_name);
                container->slots[container->slot_count].object = object;
                if(object == nullptr)
                {
                    truth_value = next_truth_value;
                    continue;
                }
            }
            ++container->slot_count;
            query_or_create_script_object_field(object_name, field_name, value, static_cast<int32_t>(value_type));
            if(value[0] == 0)
            {
                --container->slot_count;
            }
            else
            {
                if(next_truth_value != 0x07000000 && static_cast<int32_t>(next_truth_value) > 0)
                {
                    container->required_mask |= 1u << ((container->slot_count - 1) & 31);
                }
                ScriptObjectSlot &slot = container->slots[container->slot_count - 1];
                slot.active_field_mask = &object->active_field_mask;
                slot.field_mask = value[0];
            }
        }
        else if(value_type == 1)
        {
            const auto append_system_slot = [&](uint32_t field_mask)
            {
                ++container->slot_count;
                if(parsed_value != 0x07000000)
                {
                    container->required_mask |= 1u << ((container->slot_count - 1) & 31);
                }
                ScriptObjectSlot &slot = container->slots[container->slot_count - 1];
                slot.active_field_mask = &script_runtime_root->flags;
                slot.field_mask = field_mask;
            };
            if(strings_equal(field_name, "DRIVE_BUSY"))
            {
                append_system_slot(0x10);
            }
            if(strings_equal(field_name, "NOCOMMENT"))
            {
                append_system_slot(1);
            }
            if(strings_equal(field_name, "INVENTORY_OPEN"))
            {
                append_system_slot(2);
            }
            if(strings_equal(field_name, "INVENTORY_CLOSE"))
            {
                append_system_slot(4);
            }
            next_truth_value = parsed_value;
        }
        truth_value = next_truth_value;
    }
}

void insert_script_object_container(RuntimeTreeNode *node, ScriptObjectContainer *container)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->container_tail != nullptr)
            {
                script_runtime_root->container_tail->next = container;
            }
            else
            {
                script_runtime_root->containers = container;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->container_tail != nullptr)
            {
                container->next = script_runtime_root->container_tail->next;
                script_runtime_root->container_tail->next = container;
            }
            else
            {
                container->next = script_runtime_root->containers;
                script_runtime_root->containers = container;
            }
            return;
        }
        ScriptObjectContainer *previous = find_script_object_container_insertion_predecessor(node);
        if(previous != nullptr)
        {
            container->next = previous->next;
            previous->next = container;
            return;
        }
        container->next = parent->container_head;
        parent->container_head = container;
        node = parent;
    }
}

void remove_script_object_container_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->container_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            ScriptObjectContainer *successor = node->container_tail->next;
            if(script_runtime_root->container_tail != nullptr)
            {
                script_runtime_root->container_tail->next = successor;
            }
            else
            {
                script_runtime_root->containers = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            ScriptObjectContainer *previous = script_runtime_root->containers;
            if(previous == node->container_head)
            {
                if(script_runtime_root->container_tail == node->container_tail)
                {
                    script_runtime_root->container_tail = nullptr;
                }
                script_runtime_root->containers = node->container_tail->next;
                return;
            }
            while(previous->next != node->container_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->container_tail == node->container_tail)
            {
                script_runtime_root->container_tail = previous;
            }
            previous->next = node->container_tail->next;
            return;
        }
        if(parent->container_head != node->container_head)
        {
            ScriptObjectContainer *previous = parent->container_head;
            while(previous->next != node->container_head)
            {
                previous = previous->next;
            }
            previous->next = node->container_tail->next;
            return;
        }
        if(find_last_script_object_container(parent) == node->container_tail)
        {
            parent->container_head = nullptr;
        }
        else
        {
            parent->container_head = node->container_tail->next;
        }
        parent = parent->parent;
        if(node->container_tail == nullptr)
        {
            return;
        }
    }
}

bool destroy_script_object_container(ScriptObjectContainer *container)
{
    bool result = true;
    for(uint32_t index = 0; index < container->slot_count; ++index)
    {
        if(container->slots[index].object != nullptr)
        {
            result &= script_object_release_api.heap_free(script_runtime_root->heap, 0, container->slots[index].object);
        }
    }
    return result & script_object_release_api.heap_free(script_runtime_root->heap, 0, container);
}

// Shared helper for the container-state queries.
static bool script_object_container_state_matches(ScriptObjectContainer *container)
{
    container->current_mask = 0;
    for(uint32_t index = 0; index < container->slot_count; ++index)
    {
        if((*container->slots[index].active_field_mask & container->slots[index].field_mask) != 0)
        {
            container->current_mask |= 1u << (index & 0x1f);
        }
    }
    return container->required_mask == container->current_mask;
}

bool script_object_container_state_matches_by_identity(void *identity)
{
    if(script_runtime_root == nullptr)
    {
        return false;
    }
    for(ScriptObjectContainer *container = script_runtime_root->containers; container != nullptr; container = container->next)
    {
        if(container->identity == identity)
        {
            return script_object_container_state_matches(container);
        }
    }
    return true;
}

bool script_object_container_state_matches_by_name(const void *name)
{
    if(script_runtime_root == nullptr)
    {
        return false;
    }
    for(ScriptObjectContainer *container = script_runtime_root->containers; container != nullptr; container = container->next)
    {
        if(fixed_dword_memory_equal(name, container->name, 0x20))
        {
            return script_object_container_state_matches(container);
        }
    }
    return true;
}

ScriptObjectContainer *find_script_condition_container_by_name(const void *name)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    for(ScriptObjectContainer *container = script_runtime_root->containers; container != nullptr; container = container->next)
    {
        if(fixed_dword_memory_equal(name, container->name, 0x20))
        {
            return container;
        }
    }
    return nullptr;
}

RuntimeTreeLink8C *find_last_runtime_tree_link_008c(RuntimeTreeNode *root)
{
    if(root == nullptr)
    {
        return nullptr;
    }
    RuntimeTreeNode *child = root->child;
    if(child != nullptr)
    {
        while(child->next != nullptr)
        {
            child = child->next;
        }
        while(child != nullptr)
        {
            RuntimeTreeLink8C *result = find_last_runtime_tree_link_008c(child);
            if(result != nullptr)
            {
                return result;
            }
            child = child->previous;
        }
    }
    return root->link_008c_tail;
}

RuntimeTreeLink8C *find_runtime_tree_link_008c_insertion_predecessor(RuntimeTreeNode *node)
{
    RuntimeTreeNode *parent = node->parent;
    if(parent == nullptr || parent == reinterpret_cast<RuntimeTreeNode *>(-1))
    {
        return reinterpret_cast<RuntimeTreeLink8C *>(-1);
    }
    for(RuntimeTreeNode *previous = node->previous; previous != nullptr; previous = previous->previous)
    {
        RuntimeTreeLink8C *link = find_last_runtime_tree_link_008c(previous);
        if(link != nullptr)
        {
            return link;
        }
    }
    return parent->link_008c_tail;
}

void insert_runtime_tree_link_008c(RuntimeTreeNode *node, RuntimeTreeLink8C *link)
{
    while(true)
    {
        RuntimeTreeNode *parent = node->parent;
        if(parent == nullptr)
        {
            if(script_runtime_root->global_link_008c_tail != nullptr)
            {
                script_runtime_root->global_link_008c_tail->next = link;
            }
            else
            {
                script_runtime_root->global_link_008c_head = link;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            if(script_runtime_root->global_link_008c_tail != nullptr)
            {
                link->next = script_runtime_root->global_link_008c_tail->next;
                script_runtime_root->global_link_008c_tail->next = link;
            }
            else
            {
                link->next = script_runtime_root->global_link_008c_head;
                script_runtime_root->global_link_008c_head = link;
            }
            return;
        }
        RuntimeTreeLink8C *previous = find_runtime_tree_link_008c_insertion_predecessor(node);
        if(previous != nullptr)
        {
            link->next = previous->next;
            previous->next = link;
            return;
        }
        link->next = parent->link_008c_head;
        parent->link_008c_head = link;
        node = parent;
    }
}

void remove_runtime_tree_link_008c_range(RuntimeTreeNode *parent, RuntimeTreeNode *node)
{
    if(node->link_008c_tail == nullptr)
    {
        return;
    }
    while(true)
    {
        if(parent == nullptr)
        {
            RuntimeTreeLink8C *successor = node->link_008c_tail->next;
            if(script_runtime_root->global_link_008c_tail != nullptr)
            {
                script_runtime_root->global_link_008c_tail->next = successor;
            }
            else
            {
                script_runtime_root->global_link_008c_head = successor;
            }
            return;
        }
        if(parent == reinterpret_cast<RuntimeTreeNode *>(-1))
        {
            RuntimeTreeLink8C *previous = script_runtime_root->global_link_008c_head;
            if(previous == node->link_008c_head)
            {
                if(script_runtime_root->global_link_008c_tail == node->link_008c_tail)
                {
                    script_runtime_root->global_link_008c_tail = nullptr;
                }
                script_runtime_root->global_link_008c_head = node->link_008c_tail->next;
                return;
            }
            while(previous->next != node->link_008c_head)
            {
                previous = previous->next;
            }
            if(script_runtime_root->global_link_008c_tail == node->link_008c_tail)
            {
                script_runtime_root->global_link_008c_tail = previous;
            }
            previous->next = node->link_008c_tail->next;
            return;
        }
        if(parent->link_008c_head != node->link_008c_head)
        {
            RuntimeTreeLink8C *previous = parent->link_008c_head;
            while(previous->next != node->link_008c_head)
            {
                previous = previous->next;
            }
            previous->next = node->link_008c_tail->next;
            return;
        }
        if(find_last_runtime_tree_link_008c(parent) == node->link_008c_tail)
        {
            parent->link_008c_head = nullptr;
        }
        else
        {
            parent->link_008c_head = node->link_008c_tail->next;
        }
        parent = parent->parent;
        if(node->link_008c_tail == nullptr)
        {
            return;
        }
    }
}

RuntimeTreeLink8C *find_global_runtime_tree_link_008c_by_name(const void *name)
{
    if(script_runtime_root == nullptr)
    {
        return nullptr;
    }
    for(RuntimeTreeLink8C *link = script_runtime_root->global_link_008c_head; link != nullptr; link = link->next)
    {
        if(fixed_dword_memory_equal(name, link, 0x20))
        {
            return link;
        }
    }
    return nullptr;
}


} // namespace gag
