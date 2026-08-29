#include "text.h"
#include "runtime_internal.h"

namespace freegag
{

uint32_t runtime_generic_comments_suppressed_cache;

bool are_runtime_generic_comments_suppressed()
{
    return script_runtime_root != nullptr && (script_runtime_root->flags & SCRIPT_RUNTIME_COMMENTS_SUPPRESSED) != 0;
}

RuntimeGenericBackend *acquire_runtime_generic_backend(void *identity)
{
    for(;;)
    {
        RuntimeGenericBackend *result = nullptr;
        uint32_t contended = 0;
        lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
        for(RuntimeGenericBackend *backend = runtime_generic_backend_head; backend != nullptr; backend = backend->next)
        {
            if(backend->identity == identity)
            {
                contended = (backend->flags & RUNTIME_GENERIC_BACKEND_LOCKED) >> 16;
                if(contended == 0)
                {
                    backend->flags |= RUNTIME_GENERIC_BACKEND_LOCKED;
                    result = backend;
                }
                break;
            }
        }
        unlock_runtime_mutex(runtime_generic_backend_mutex);
        if(contended == 0)
            return result;
        runtime_sleep(0);
    }
}

RuntimeGenericBackend *create_runtime_generic_backend(uintptr_t text_address, uint32_t text_size)
{
    auto *backend = static_cast<RuntimeGenericBackend *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeGenericBackend)));
    RuntimeGenericBackend *result = backend;
    if(backend != nullptr)
    {
        backend->identity = backend;
        backend->text_size = text_size;
        backend->text = reinterpret_cast<const char *>(text_address);
        lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
        if(runtime_generic_backend_enabled == 0)
        {
            result = nullptr;
            free_runtime_heap(runtime_process_heap(), 0, backend);
        }
        else
        {
            backend->next = runtime_generic_backend_head;
            runtime_generic_backend_head = backend;
        }
        unlock_runtime_mutex(runtime_generic_backend_mutex);
    }
    return result;
}

void *find_available_runtime_generic_child(uint32_t maximum_end_position)
{
    void *result = nullptr;
    lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
    for(RuntimeGenericBackend *backend = runtime_generic_backend_head; backend != nullptr; backend = backend->next)
    {
        for(RuntimeGenericBackendChild *child = backend->children; child != nullptr; child = child->next)
        {
            if((child->flags & (RUNTIME_GENERIC_CHILD_SELECTION_OVERRIDE | RUNTIME_GENERIC_CHILD_MODE_200)) == 0 && child->state_end_position <= maximum_end_position)
            {
                result = child->identity;
                unlock_runtime_mutex(runtime_generic_backend_mutex);
                return result;
            }
        }
    }
    unlock_runtime_mutex(runtime_generic_backend_mutex);
    return result;
}

void *find_next_runtime_generic_backend_child(void *previous_identity)
{
    void *result = nullptr;
    bool return_next = previous_identity == nullptr;
    lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
    for(RuntimeGenericBackend *backend = runtime_generic_backend_head; backend != nullptr; backend = backend->next)
    {
        for(RuntimeGenericBackendChild *child = backend->children; child != nullptr; child = child->next)
        {
            if(return_next)
            {
                result = child->identity;
                unlock_runtime_mutex(runtime_generic_backend_mutex);
                return result;
            }
            if(child->identity == previous_identity)
                return_next = true;
        }
    }
    unlock_runtime_mutex(runtime_generic_backend_mutex);
    return result;
}

int32_t find_runtime_generic_text_entry(RuntimeGenericBackend *backend, int32_t category, const char *name)
{
    uint32_t position = 0;
    int32_t found_category;
    char token[32];
    do
    {
        do
        {
            found_category = 0;
            token[0] = '\0';
            while(position < backend->text_size && backend->text[position] != '=')
                ++position;
            if(backend->text[position] == '=')
            {
                while(backend->text[position] != '\t' && backend->text[position] != '\n' && backend->text[position] != '\r')
                {
                    if(backend->text[position] == ':')
                        break;
                    --position;
                    if(backend->text[position] == ' ')
                        break;
                }
                uint32_t input = position + 1;
                uint32_t length = 0;
                while(backend->text[input] != '=' && length < 30)
                    token[length++] = backend->text[input++];
                token[length] = '\0';
                if(std::strcmp(token, "section") == 0)
                    found_category = 0x10;
                if(std::strcmp(token, "entry") == 0)
                    found_category = 0x20;
                if(std::strcmp(token, "control") == 0)
                    found_category = 0x30;
                if(std::strcmp(token, "text") == 0)
                    found_category = 0x40;
                position = input + 1;
            }
            else
            {
                position = RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND;
            }
        } while(category != found_category && position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND);

        if(position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND)
        {
            uint32_t length = 0;
            while(backend->text[position] != ' ' && static_cast<uint8_t>(backend->text[position]) != 9 && static_cast<uint8_t>(backend->text[position]) != 0x3b && length < 30)
                token[length++] = backend->text[position++];
            token[length] = '\0';
        }
    } while(std::strcmp(token, name) != 0 && position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND);

    int32_t result = -1;
    if(position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND)
    {
        result = static_cast<int32_t>(position + 1);
        if(found_category != 0x20)
        {
            const uint32_t size = backend->text_size;
            while(true)
            {
                if(position < size)
                    while(position < size && backend->text[position] != '=' && backend->text[position] != '[')
                        ++position;
                if(backend->text[position] == '=')
                {
                    while(position < size && backend->text[position] != ';' && backend->text[position] != '[')
                        ++position;
                    if(backend->text[position] == ';')
                        result = static_cast<int32_t>(position + 1);
                }
                if(backend->text[position] == '[' || position == size)
                    break;
            }
        }
    }
    return result;
}

RuntimeGenericBackendChild *create_runtime_generic_backend_child(void *backend_identity, void *font_identity, const uintptr_t *context, uintptr_t selection, uint32_t flags)
{
    RuntimeGenericBackendChild *child = nullptr;
    RuntimeGenericBackend *backend = acquire_runtime_generic_backend(backend_identity);
    if(backend != nullptr)
    {
        uint32_t default_selection = RUNTIME_GENERIC_TEXT_INTEGER_INVALID;
        uint32_t parser_position;
        uint32_t text_search_position;
        uint32_t additional_flags;
        if((selection >> 16) == 0)
        {
            default_selection = selection & 0xffff;
            parser_position = 0;
            text_search_position = 0;
            additional_flags = RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER;
        }
        else
        {
            const char *name = reinterpret_cast<const char *>(selection);
            int32_t entry_position = find_runtime_generic_text_entry(backend, 0x20, name);
            if(entry_position != -1)
            {
                uint32_t position = static_cast<uint32_t>(entry_position);
                default_selection = parse_runtime_generic_integer(backend->text, &position, backend->text_size, 0);
            }
            parser_position = find_runtime_generic_text_entry(backend, 0x30, name);
            additional_flags = 0;
            text_search_position = find_runtime_generic_text_entry(backend, 0x40, name);
        }
        if(default_selection != RUNTIME_GENERIC_TEXT_INTEGER_INVALID && parser_position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND && text_search_position != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND)
        {
            child = static_cast<RuntimeGenericBackendChild *>(allocate_runtime_heap(runtime_process_heap(), runtime_heap_zero_memory, sizeof(RuntimeGenericBackendChild)));
            if(child != nullptr)
            {
                child->identity = child;
                child->parent = backend;
                child->font_identity = font_identity;
                child->default_selection = default_selection;
                child->parser_position = parser_position;
                child->text_search_position = text_search_position;
                child->flags |= additional_flags | flags;
                child->context[0] = context[0];
                child->context[1] = context[1];
                lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
                child->next = backend->children;
                backend->children = child;
                ++backend->child_count;
                unlock_runtime_mutex(runtime_generic_backend_mutex);
                build_runtime_generic_backend_child_state(child->identity, 0, nullptr, nullptr, nullptr);
            }
        }
        backend->flags &= ~RUNTIME_GENERIC_BACKEND_LOCKED;
    }
    return child;
}


void process_available_runtime_generic_children(uint32_t maximum_end_position)
{
    const bool comments_suppressed = are_runtime_generic_comments_suppressed();
    if(runtime_generic_comments_suppressed_cache != static_cast<uint32_t>(comments_suppressed))
    {
        for(void *identity = find_next_runtime_generic_backend_child(nullptr); identity != nullptr; identity = find_next_runtime_generic_backend_child(identity))
        {
            uintptr_t context[2];
            RuntimeGenericChildState state;
            if(query_runtime_generic_backend_child_state(identity, state.words, nullptr, context) != 0 && context[1] != 0)
            {
                const intptr_t scene_identifier = query_display_scene_by_index(static_cast<int32_t>(context[1]), nullptr, nullptr);
                if(scene_identifier != 0)
                    set_display_scene_node_position(scene_identifier, static_cast<int32_t>(state.words[5]), comments_suppressed ? 10000 : static_cast<int32_t>(state.words[6]));
            }
        }
        runtime_generic_comments_suppressed_cache = comments_suppressed;
    }

    for(void *identity = find_available_runtime_generic_child(maximum_end_position); identity != nullptr; identity = find_available_runtime_generic_child(maximum_end_position))
    {
        uintptr_t context[2];
        DisplaySceneDescriptor descriptor;
        RuntimeGenericChildState state;
        if(build_runtime_generic_backend_child_state(identity, 0, state.words, &descriptor, context) != 0)
        {
            DisplayRectangle *rectangle = &state.fields.rectangle;
            if(rectangle->right <= static_cast<uint16_t>(descriptor.width) && rectangle->bottom <= static_cast<uint16_t>(descriptor.height))
            {
                rectangle->right = static_cast<uint16_t>(descriptor.width);
                rectangle->bottom = static_cast<uint16_t>(descriptor.height);
            }
            if(context[1] == 0)
                context[1] = find_available_display_scene_index(0x80000);
            int32_t x = static_cast<int32_t>(state.words[5]);
            int32_t y = static_cast<int32_t>(state.words[6]);
            if(comments_suppressed)
                y = 10000;
            DisplaySceneNode *scene =
                acquire_display_scene_node(static_cast<uint32_t>(context[1]), x, y, rectangle->right, rectangle->bottom, 0x20000, static_cast<intptr_t>(context[0]), &descriptor, nullptr);
            const intptr_t scene_identifier = reinterpret_cast<intptr_t>(scene);
            if(begin_display_scene_update(scene_identifier) == 0)
            {
                publish_runtime_generic_backend_child_state(identity, state.words, &descriptor, static_cast<int32_t>(maximum_end_position));
                const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
                end_display_scene_update(scene_identifier, &transform, rectangle);
            }
            set_runtime_generic_backend_child_context(identity, context);
        }
        else if(get_runtime_generic_backend_child_context(identity, context))
        {
            if(context[0] == 0x0047EF60)
            {
                destroy_runtime_generic_backend_child(identity);
                const intptr_t scene_identifier = query_display_scene_by_index(static_cast<int32_t>(context[1]), nullptr, nullptr);
                if(scene_identifier != 0)
                    release_display_scene_node(scene_identifier, static_cast<int32_t>(context[0]));
            }
            else
            {
                enable_runtime_generic_backend_child_mode_200(identity);
            }
        }
    }
}



RuntimeGenericBackendChild *acquire_runtime_generic_backend_child(void *identity)
{
    for(;;)
    {
        RuntimeGenericBackendChild *result = nullptr;
        uint32_t contended = 0;
        lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
        bool found = false;
        for(RuntimeGenericBackend *backend = runtime_generic_backend_head; backend != nullptr && !found; backend = backend->next)
        {
            for(RuntimeGenericBackendChild *child = backend->children; child != nullptr; child = child->next)
            {
                if(child->identity == identity)
                {
                    contended = (child->flags & RUNTIME_GENERIC_CHILD_LOCKED) >> 16;
                    if(contended == 0)
                    {
                        child->flags |= RUNTIME_GENERIC_CHILD_LOCKED;
                        result = child;
                    }
                    found = true;
                    break;
                }
            }
        }
        unlock_runtime_mutex(runtime_generic_backend_mutex);
        if(contended == 0)
            return result;
        runtime_sleep(0);
    }
}

int32_t parse_runtime_generic_integer(const char *text, uint32_t *position, uint32_t end, uint32_t flags)
{
    uint32_t cursor = *position;
    const char alternate_stop = (flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) == 0 ? ':' : '#';
    while(cursor < end && text[cursor] != ';' && text[cursor] != '@' && text[cursor] != alternate_stop && (text[cursor] < '0' || text[cursor] > '9'))
        ++cursor;
    if(text[cursor] < '0' || text[cursor] > '9')
        return RUNTIME_GENERIC_TEXT_INTEGER_INVALID;
    if(text[cursor] == '+')
        ++cursor;
    const bool negative = text[cursor] == '-';
    if(negative)
        ++cursor;
    int32_t value = 0;
    while(text[cursor] >= '0' && text[cursor] <= '9')
    {
        value = text[cursor] - '0' + value * 10;
        ++cursor;
    }
    if(negative)
        value = -value;
    *position = text[cursor] == ';' ? cursor : cursor + 1;
    return value;
}

int32_t skip_runtime_generic_statement(const char *text, uint32_t *position, uint32_t end, uint32_t flags)
{
    uint32_t cursor = *position;
    do
    {
        while(cursor < end && text[cursor] != ';' && text[cursor] != '@')
            ++cursor;
        if(text[cursor] == '@')
        {
            if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) == 0)
            {
                ++cursor;
                while(cursor < end && text[cursor] != '@')
                    ++cursor;
                if(cursor >= end)
                    break;
                ++cursor;
            }
            else if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) == RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER)
            {
                if(cursor >= end)
                    break;
                ++cursor;
            }
        }
        if(cursor >= end)
            return RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID;
    } while(text[cursor] != ';');
    if(cursor >= end)
        return RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID;
    *position = cursor + 1;
    return 0;
}

int32_t parse_runtime_generic_directive(const char *text, uint32_t *position, uint32_t end, uint32_t flags)
{
    if((flags & RUNTIME_MEDIA_CLOSE_REQUESTED) != 0)
        return RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT;
    uint32_t cursor = *position;
    char name[32]{};
    while(cursor < end && text[cursor] != '@' && text[cursor] != '[')
        ++cursor;
    if(text[cursor] != '@')
        return RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID;
    ++cursor;
    uint32_t length = 0;
    while(cursor < end && length < 30 && text[cursor] != '@' && text[cursor] != ':' && text[cursor] != '[')
        name[length++] = text[cursor++];
    name[length] = '\0';
    if(cursor == end || text[cursor] == '[')
        return RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID;
    int32_t result = RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID;
    if(length == 0 || std::strcmp(name, "THIS") == 0)
        result = RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT;
    if(std::strcmp(name, "TEXT") == 0)
        result = RUNTIME_GENERIC_TEXT_DIRECTIVE_REFERENCE;
    if(std::strcmp(name, "JMP") == 0)
        result = RUNTIME_GENERIC_TEXT_DIRECTIVE_JUMP;
    if(std::strcmp(name, "END") == 0)
        result = RUNTIME_GENERIC_TEXT_DIRECTIVE_END;
    *position = cursor + 1;
    return result;
}

uint32_t build_runtime_generic_backend_child_state(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context)
{
    uint32_t result = 0;
    if(state != nullptr)
        state[0] = RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND;
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child == nullptr)
        return result;

    uint32_t flags = child->flags;
    uint32_t effective_selection = selection;
    if((flags & RUNTIME_GENERIC_CHILD_USE_CURRENT_SELECTION) == 0)
    {
        if((flags & RUNTIME_GENERIC_CHILD_SELECTION_OVERRIDE) == 0)
            effective_selection = child->default_selection;
    }
    else if((flags & RUNTIME_GENERIC_CHILD_SELECTION_OVERRIDE) == 0)
    {
        effective_selection = child->state[10];
    }

    if((flags & RUNTIME_GENERIC_CHILD_STATE_VALID) == 0 || child->computed_state[9] != effective_selection)
    {
        RuntimeGenericBackend *parent = child->parent;
        uint32_t cursor = child->parser_position;
        const char *text = parent->text;
        flags = child->flags;
        uint32_t parsed_selection;
        do
        {
            parsed_selection = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
            if(parsed_selection == effective_selection && parsed_selection != RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
            {
                child->computed_state[0] = 0;
                child->flags |= RUNTIME_GENERIC_CHILD_STATE_VALID;
                child->computed_state[4] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(child->font_identity));
                child->computed_state[3] = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(text));

                uint32_t state_start;
                uint32_t following_selection;
                do
                {
                    state_start = cursor;
                    skip_runtime_generic_statement(text, &cursor, parent->text_size, flags);
                    following_selection = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                } while(following_selection == effective_selection);

                cursor = state_start;
                child->computed_state[5] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                child->computed_state[6] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                child->computed_state[7] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                child->computed_state[8] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                child->computed_state[1] = select_runtime_generic_text(&child->computed_state[11], text, &cursor, parent->text_size, child->text_search_position, child->font_identity, flags);
                child->computed_state[9] = parsed_selection;
                child->computed_state[2] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                child->computed_state[10] = static_cast<uint32_t>(parse_runtime_generic_integer(text, &cursor, parent->text_size, flags));
                if(child->computed_state[1] == RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND)
                    child->computed_state[0] = RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND;
                if(child->computed_state[5] == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                    child->computed_state[5] = child->state[5];
                if(child->computed_state[6] == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                    child->computed_state[6] = child->state[6];
                if(child->computed_state[2] == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                    child->computed_state[2] = 0;
                break;
            }
            skip_runtime_generic_statement(text, &cursor, parent->text_size, flags);
        } while(parsed_selection != RUNTIME_GENERIC_TEXT_INTEGER_INVALID);
    }

    if((child->flags & RUNTIME_GENERIC_CHILD_STATE_VALID) != 0 && child->computed_state[9] == effective_selection)
    {
        result = 1;
        if(state != nullptr)
            std::memcpy(state, child->computed_state, sizeof(child->computed_state));
    }
    if(descriptor != nullptr)
        *descriptor = child->descriptor;
    if(context != nullptr)
        std::memcpy(context, child->context, sizeof(child->context));
    release_runtime_generic_backend_child_lock(child);
    return result;
}

void publish_runtime_generic_backend_child_state(void *identity, const uint32_t *state, const DisplaySceneDescriptor *descriptor, int32_t end_offset)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child == nullptr)
        return;
    if(state[0] == 0)
    {
        child->flags |= 2;
        child->state_end_position = state[2] + end_offset;
        if(descriptor != nullptr)
            child->descriptor = *descriptor;
        if(child->descriptor.pixels != 0)
            draw_runtime_generic_text(child->parent->text, child->parent->text_size, state, child->font_identity, &child->descriptor, child->flags);
    }
    std::memcpy(child->state, state, sizeof(child->state));
    child->flags &= ~1u;
    release_runtime_generic_backend_child_lock(child);
}

uint32_t measure_runtime_font_glyph(uint8_t character, const RuntimeMediaBackend *backend)
{
    const uint16_t atlas_stride = backend->destination_stride;
    const uint32_t glyph_width = atlas_stride >> 4;
    uint32_t rows_remaining = backend->destination_reserved >> 4;
    const uint8_t *pixels = backend->destination_pixels + (character >> 4) * rows_remaining * atlas_stride + (character & 0x0f) * glyph_width;
    uint32_t maximum_width = 0;
    do
    {
        uint32_t transparent_run = 0;
        uint32_t row_width = 0;
        uint32_t columns_remaining = glyph_width;
        do
        {
            ++transparent_run;
            if(*pixels != 0)
            {
                row_width += transparent_run;
                transparent_run = 0;
            }
            ++pixels;
            --columns_remaining;
        } while(columns_remaining != 0);
        pixels += atlas_stride - glyph_width;
        if(static_cast<int32_t>(maximum_width) < static_cast<int32_t>(row_width))
            maximum_width = row_width;
        --rows_remaining;
    } while(rows_remaining != 0);
    if(maximum_width == 0)
        maximum_width = atlas_stride >> 5;
    return maximum_width;
}

uint32_t draw_runtime_font_glyph(DisplaySceneDescriptor *destination, uint8_t character, int32_t x, int32_t y, const RuntimeMediaBackend *font, uint32_t low_color, uint32_t high_color)
{
    uint8_t low_mask = 0xff;
    uint8_t high_mask = 0xff;
    uint8_t low_value = 0;
    uint8_t high_value = 0;
    if(high_color < 0x100)
    {
        high_value = static_cast<uint8_t>(high_color);
        high_mask = 0;
    }
    if(low_color < 0x100)
    {
        low_value = static_cast<uint8_t>(low_color);
        low_mask = 0;
    }

    const uint32_t destination_width = static_cast<uint16_t>(destination->width);
    const uint32_t destination_height = static_cast<uint16_t>(destination->height);
    const uint32_t atlas_stride = font->destination_stride;
    const uint32_t glyph_width = atlas_stride >> 4;
    uint32_t rows_remaining = font->destination_reserved >> 4;
    int32_t source_row_advance = static_cast<int32_t>(atlas_stride - glyph_width);
    int32_t destination_row_advance = static_cast<int32_t>(destination_width - glyph_width);
    uint32_t draw_width = glyph_width;

    const int32_t right_overflow = static_cast<int32_t>(glyph_width) + x - static_cast<int32_t>(destination_width);
    if(right_overflow > 0 && static_cast<int32_t>(destination_width) <= static_cast<int32_t>(glyph_width) + x)
    {
        draw_width -= static_cast<uint32_t>(right_overflow);
        if(draw_width == 0 || static_cast<int32_t>(glyph_width) < right_overflow)
            return glyph_width;
        source_row_advance += right_overflow;
        destination_row_advance += right_overflow;
    }
    const int32_t bottom_overflow = static_cast<int32_t>(rows_remaining) + y - static_cast<int32_t>(destination_height);
    if(bottom_overflow > 0 && static_cast<int32_t>(destination_height) <= static_cast<int32_t>(rows_remaining) + y)
    {
        const uint32_t clipped_width = draw_width - static_cast<uint32_t>(bottom_overflow);
        const bool underflow = static_cast<int32_t>(draw_width) < bottom_overflow;
        draw_width = clipped_width;
        if(draw_width == 0 || underflow)
            return glyph_width;
    }

    const uint8_t *source = font->destination_pixels + (character >> 4) * rows_remaining * atlas_stride + (character & 0x0f) * glyph_width;
    auto *destination_pixels = reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(destination->pixels));
    uint32_t *output = destination_pixels + destination_width * y + x;
    const auto palette_color = [](uint8_t index)
    {
        const PaletteEntry color = runtime_game_host_context.palette_entries[index];
        return (index == 0 ? 0u : 0xff000000u) | static_cast<uint32_t>(color.peRed) << 16 | static_cast<uint32_t>(color.peGreen) << 8 | color.peBlue;
    };
    uint32_t maximum_width = 0;
    do
    {
        uint32_t transparent_run = 0;
        uint32_t row_width = 0;
        uint32_t columns_remaining = draw_width;
        do
        {
            ++transparent_run;
            const uint8_t pixel = *source;
            if(pixel != 0)
            {
                const uint8_t output_index = pixel < 0x10 ? static_cast<uint8_t>((pixel & high_mask) | high_value) : static_cast<uint8_t>((pixel & low_mask) | low_value);
                *output = palette_color(output_index);
                row_width += transparent_run;
                transparent_run = 0;
            }
            ++source;
            ++output;
            --columns_remaining;
        } while(columns_remaining != 0);
        source += source_row_advance;
        output += destination_row_advance;
        if(static_cast<int32_t>(maximum_width) < static_cast<int32_t>(row_width))
            maximum_width = row_width;
        --rows_remaining;
    } while(rows_remaining != 0);
    if(maximum_width == 0)
        maximum_width = draw_width >> 1;
    return maximum_width;
}

uint32_t initialize_runtime_standalone_text(const char *text, uint32_t x, uint32_t y, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state)
{
    if(get_runtime_media_backend_type(font_identity) != 0xac)
        return 0;

    uint32_t text_length = 0;
    while(text[text_length] != '\0')
        ++text_length;
    const uint32_t end = text_length + 1;
    if(text_length == 0)
        return 0;

    std::memset(state, 0, sizeof(*state));
    uint32_t position = 0;
    measure_runtime_generic_text(state->bounds, text, &position, end, font_identity, 0);
    state->text = text;
    state->font_identity = font_identity;
    state->x = x;
    state->y = y;
    state->low_color = low_color;
    state->high_color = high_color;
    return 1;
}

void draw_runtime_standalone_text(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination)
{
    uint32_t text_length = 0;
    while(state->text[text_length] != '\0')
        ++text_length;
    uint32_t generic_state[9]{};
    generic_state[1] = state->layout_state[1];
    generic_state[7] = state->low_color;
    generic_state[8] = state->high_color;
    draw_runtime_generic_text(state->text, text_length + 1, generic_state, state->font_identity, destination, 0);
}

void draw_runtime_generic_text(const char *text, uint32_t end, const uint32_t *state, void *font_identity, DisplaySceneDescriptor *destination, uint32_t flags)
{
    uint32_t cursor = state[1];
    RuntimeMediaBackend *font = acquire_runtime_media_backend(font_identity);
    if(font == nullptr)
        return;
    const auto *format = static_cast<const RuntimeFontFormat *>(font->format_data);
    const uint32_t cell_width = static_cast<uint32_t>(format->fixed_cell_width >> 4);
    const int32_t signed_cell_height = format->fixed_cell_height;
    const uint32_t cell_height = signed_cell_height < 1 ? static_cast<uint32_t>(-signed_cell_height) >> 4 : static_cast<uint32_t>(signed_cell_height >> 4);
    int32_t x = static_cast<uint16_t>(destination->x);
    int32_t y = static_cast<uint16_t>(destination->y);

    while(cursor < end)
    {
        switch(text[cursor])
        {
        case '\n':
        case '\r':
            ++cursor;
            break;
        case ' ':
            x += static_cast<int32_t>(cell_width >> 1);
            break;
        case '#':
        case ';':
            if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) != 0)
                goto release;
            x += static_cast<int32_t>(draw_runtime_font_glyph(destination, static_cast<uint8_t>(text[cursor]), x, y, font, state[7], state[8]));
            break;
        case '@':
            if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) != 0)
            {
                x += static_cast<int32_t>(draw_runtime_font_glyph(destination, static_cast<uint8_t>(text[cursor]), x, y, font, state[7], state[8]));
                break;
            }
            if(end - 10 <= cursor)
                goto release;
            {
                const int32_t directive = parse_runtime_generic_directive(text, &cursor, end, flags);
                if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
                    break;
                if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT || directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_END)
                    goto release;
                if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_JUMP)
                {
                    const int32_t target = parse_runtime_generic_integer(text, &cursor, end, flags);
                    if(target == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                        goto release;
                    for(;;)
                    {
                        int32_t candidate_directive;
                        do
                        {
                            candidate_directive = parse_runtime_generic_directive(text, &cursor, end, flags);
                            if(candidate_directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
                                goto release;
                        } while(candidate_directive != RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT);
                        const int32_t candidate = parse_runtime_generic_integer(text, &cursor, end, flags);
                        if(candidate == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                            goto release;
                        if(candidate == target)
                            break;
                    }
                }
            }
            break;
        case '[':
            goto release;
        case '\\':
            if(end - 1 <= cursor)
                goto release;
            ++cursor;
            if(text[cursor] == 'n')
            {
                x = static_cast<uint16_t>(destination->x);
                y += static_cast<int32_t>(cell_height);
            }
            else if(text[cursor] == 't')
            {
                if(text[cursor + 1] < '0' || text[cursor + 1] > '9')
                {
                    x += static_cast<int32_t>(cell_width * 2);
                }
                else
                {
                    int32_t tab_width = 0;
                    ++cursor;
                    while(text[cursor] >= '0' && text[cursor] <= '9')
                    {
                        tab_width = text[cursor] - '0' + tab_width * 10;
                        ++cursor;
                    }
                    x += tab_width;
                }
            }
            break;
        default:
            x += static_cast<int32_t>(draw_runtime_font_glyph(destination, static_cast<uint8_t>(text[cursor]), x, y, font, state[7], state[8]));
            break;
        }
        ++cursor;
    }

release:
    release_runtime_media_backend_lock(static_cast<RuntimeMediaBackend *>(font_identity));
}

void measure_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, void *font_identity, uint32_t flags)
{
    uint32_t cursor = *position;
    std::memset(bounds, 0, 4 * sizeof(*bounds));
    RuntimeMediaBackend *font = acquire_runtime_media_backend(font_identity);
    if(font == nullptr)
        return;

    const auto *format = static_cast<const RuntimeFontFormat *>(font->format_data);
    const uint32_t cell_width = static_cast<uint32_t>(format->fixed_cell_width >> 4);
    const int32_t signed_cell_height = format->fixed_cell_height;
    const uint32_t cell_height = signed_cell_height < 1 ? static_cast<uint32_t>(-signed_cell_height) >> 4 : static_cast<uint32_t>(signed_cell_height >> 4);
    uint32_t current_width = 0;
    uint32_t maximum_width = 0;
    uint32_t height = cell_height;
    uint32_t output_height = 0;
    bool publish = false;

    do
    {
        uint32_t next_cursor = cursor;
        switch(text[cursor])
        {
        case '\n':
        case '\r':
            next_cursor = cursor + 1;
            break;
        case ' ':
            current_width += cell_width >> 1;
            break;
        case '#':
        case ';':
            if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) != 0)
            {
                publish = true;
                goto complete;
            }
            current_width += measure_runtime_font_glyph(static_cast<uint8_t>(text[cursor]), font);
            break;
        case '@':
            if((flags & RUNTIME_GENERIC_CHILD_ALTERNATE_DELIMITER) != 0)
            {
                current_width += measure_runtime_font_glyph(static_cast<uint8_t>(text[cursor]), font);
                break;
            }
            if(end - 10 <= cursor)
                goto release;
            {
                const int32_t directive = parse_runtime_generic_directive(text, &cursor, end, flags);
                next_cursor = cursor;
                if(directive != RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
                {
                    if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT || directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_END)
                    {
                        publish = true;
                        goto complete;
                    }
                    if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_JUMP)
                    {
                        const int32_t target = parse_runtime_generic_integer(text, &cursor, end, flags);
                        if(target == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                            goto release;
                        for(;;)
                        {
                            int32_t candidate_directive;
                            do
                            {
                                candidate_directive = parse_runtime_generic_directive(text, &cursor, end, flags);
                                if(candidate_directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
                                    goto release;
                            } while(candidate_directive != RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT);
                            const int32_t candidate = parse_runtime_generic_integer(text, &cursor, end, flags);
                            if(candidate == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                                goto release;
                            next_cursor = cursor;
                            if(candidate == target)
                                break;
                        }
                    }
                }
            }
            break;
        case '[':
            goto release;
        case '\\':
            if(end - 10 <= cursor)
                goto release;
            next_cursor = cursor + 1;
            if(text[next_cursor] == 'n')
            {
                if(maximum_width < current_width)
                    maximum_width = current_width;
                current_width = 0;
                height += cell_height;
            }
            else if(text[next_cursor] == 't')
            {
                int32_t tab_width;
                if(text[next_cursor + 1] < '0' || text[next_cursor + 1] > '9')
                {
                    tab_width = static_cast<int32_t>(cell_width * 2);
                    cursor = next_cursor;
                }
                else
                {
                    tab_width = 0;
                    cursor += 2;
                    while(text[cursor] >= '0')
                    {
                        if(text[cursor] > '9')
                            break;
                        tab_width = text[cursor] - '0' + tab_width * 10;
                        ++cursor;
                    }
                }
                current_width += static_cast<uint32_t>(tab_width);
                next_cursor = cursor;
            }
            break;
        default:
            current_width += measure_runtime_font_glyph(static_cast<uint8_t>(text[cursor]), font);
            break;
        }
        cursor = next_cursor + 1;
    } while(cursor < end);
    publish = true;

complete:
    if(publish)
    {
        if(maximum_width < current_width)
            maximum_width = current_width;
        if(height != 0)
            output_height = height;
        bounds[0] = 0;
        bounds[1] = 0;
        bounds[2] = maximum_width;
        bounds[3] = output_height;
        *position = cursor;
    }
release:
    release_runtime_media_backend_lock(static_cast<RuntimeMediaBackend *>(font_identity));
}

uint32_t select_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, uint32_t search_position, void *font_identity, uint32_t flags)
{
    uint32_t cursor = *position;
    uint32_t result = RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND;
    const int32_t directive = parse_runtime_generic_directive(text, &cursor, end, flags);
    if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
    {
        std::memset(bounds, 0, 4 * sizeof(*bounds));
    }
    else if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT)
    {
        result = cursor;
        measure_runtime_generic_text(bounds, text, &cursor, end, font_identity, flags);
    }
    else if(directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_REFERENCE)
    {
        const int32_t requested = parse_runtime_generic_integer(text, &cursor, end, flags);
        if(requested != RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
        {
            uint32_t candidate_cursor = search_position;
            for(;;)
            {
                int32_t candidate_directive;
                do
                {
                    candidate_directive = parse_runtime_generic_directive(text, &candidate_cursor, end, flags);
                    if(candidate_directive == RUNTIME_GENERIC_TEXT_DIRECTIVE_INVALID)
                        goto finish;
                } while(candidate_directive != RUNTIME_GENERIC_TEXT_DIRECTIVE_CURRENT);
                const int32_t candidate = parse_runtime_generic_integer(text, &candidate_cursor, end, flags);
                if(candidate == RUNTIME_GENERIC_TEXT_INTEGER_INVALID)
                    goto finish;
                if(candidate == requested)
                    break;
            }
            result = candidate_cursor;
            measure_runtime_generic_text(bounds, text, &candidate_cursor, end, font_identity, flags);
        }
    }

finish:
    if(result != RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND)
    {
        if(text[cursor] != ';')
            ++cursor;
        *position = cursor;
    }
    return result;
}

void release_runtime_generic_backend_child_lock(RuntimeGenericBackendChild *child)
{
    child->flags &= ~RUNTIME_GENERIC_CHILD_LOCKED;
}

void release_runtime_media_backend_lock(RuntimeMediaBackend *backend)
{
    if(backend != nullptr && backend->recursion_count != 0)
        --backend->recursion_count;
}

void render_runtime_generic_backend_child(RuntimeMediaBackend *backend)
{
    auto *resource = static_cast<RuntimeResourceObject *>(backend->extension_data);
    if(resource->generic_backend_child != nullptr && (resource->type_flags & RUNTIME_RESOURCE_PRIMARY) != 0 && (resource->type_flags & RUNTIME_RESOURCE_INTERNAL_PRIMARY) == 0)
    {
        uintptr_t context[2];
        RuntimeGenericChildState state;
        if(query_runtime_generic_backend_child_state(resource->generic_backend_child, state.words, nullptr, context))
        {
            intptr_t source_identifier = 0;
            if(context[1] != 0)
                source_identifier = query_display_scene_by_index(static_cast<int32_t>(context[1]), nullptr, nullptr);
            blit_display_scene(reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(resource->scene_identifier)), static_cast<int32_t>(state.words[6]) - resource->x,
                static_cast<int32_t>(state.words[7]) - resource->y, reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(source_identifier)), &state.fields.rectangle, 0);
        }
    }
}

void update_runtime_generic_backend_child(RuntimeMediaBackend *backend)
{
    auto *resource = static_cast<RuntimeResourceObject *>(backend->extension_data);
    void *identity = resource->generic_backend_child;
    if(identity == nullptr)
        return;

    uintptr_t context[2];
    DisplaySceneDescriptor descriptor;
    DisplayRectangle destination_rectangle;
    RuntimeGenericChildState state;
    if((resource->type_flags & RUNTIME_RESOURCE_PRIMARY) == 0 || (resource->type_flags & RUNTIME_RESOURCE_INTERNAL_PRIMARY) != 0)
    {
        if(build_runtime_generic_backend_child_state(identity, backend->frame_number, state.words, &descriptor, context) != 0)
        {
            DisplayRectangle *state_rectangle = &state.fields.rectangle;
            if(state_rectangle->right <= static_cast<uint16_t>(descriptor.width) && state_rectangle->bottom <= static_cast<uint16_t>(descriptor.height))
            {
                state_rectangle->right = static_cast<uint16_t>(descriptor.width);
                state_rectangle->bottom = static_cast<uint16_t>(descriptor.height);
            }
            if(context[1] == 0)
                context[1] = find_available_display_scene_index(0x80000);
            if(are_runtime_generic_comments_suppressed())
                state.words[6] = 10000;
            DisplaySceneNode *scene = acquire_display_scene_node(static_cast<uint32_t>(context[1]), static_cast<int32_t>(state.words[5]), static_cast<int32_t>(state.words[6]), state_rectangle->right,
                state_rectangle->bottom, 0x20000, static_cast<intptr_t>(context[0]), &descriptor, nullptr);
            if(begin_display_scene_update(reinterpret_cast<intptr_t>(scene)) == 0)
            {
                publish_runtime_generic_backend_child_state(identity, state.words, &descriptor, 0);
                const DisplayRectangleTransform transform = display_rectangle_transform(descriptor);
                end_display_scene_update(reinterpret_cast<intptr_t>(scene), &transform, state_rectangle);
            }
            set_runtime_generic_backend_child_context(identity, context);
        }
        return;
    }

    if(build_runtime_generic_backend_child_state(identity, backend->frame_number, state.words, nullptr, context) != 0)
    {
        if(context[1] == 0)
            std::memset(&descriptor, 0, sizeof(descriptor));
        else
            query_display_scene_by_index(static_cast<int32_t>(context[1]), &descriptor, nullptr);
        DisplayRectangle *state_rectangle = &state.fields.rectangle;
        if(state_rectangle->right <= static_cast<uint16_t>(descriptor.width) && state_rectangle->bottom <= static_cast<uint16_t>(descriptor.height))
        {
            state_rectangle->right = static_cast<uint16_t>(descriptor.width);
            state_rectangle->bottom = static_cast<uint16_t>(descriptor.height);
        }
        if(context[1] == 0)
            context[1] = find_available_display_scene_index(0x80000);
        DisplaySceneNode *scene =
            acquire_display_scene_node(static_cast<uint32_t>(context[1]), 10000, 10000, state_rectangle->right, state_rectangle->bottom, 0, static_cast<intptr_t>(context[0]), &descriptor, nullptr);
        if(begin_display_scene_update(resource->scene_identifier) == 0)
        {
            destination_rectangle.left = static_cast<int32_t>(state.words[5]) - resource->x;
            destination_rectangle.top = static_cast<int32_t>(state.words[6]) - resource->y;
            destination_rectangle.right = destination_rectangle.left + state_rectangle->right;
            destination_rectangle.bottom = destination_rectangle.top + state_rectangle->bottom;
            blit_display_scene(scene, 0, 0, reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(resource->scene_identifier)), &destination_rectangle, 0);
            descriptor.width = resource->scene_descriptor.width;
            descriptor.height = resource->scene_descriptor.height;
            descriptor.pixels = resource->scene_descriptor.pixels;
            descriptor.y = static_cast<int16_t>(state.words[6]) - static_cast<int16_t>(resource->y);
            descriptor.x = static_cast<int16_t>(state.words[5]) - static_cast<int16_t>(resource->x);
            if(!are_runtime_generic_comments_suppressed())
                publish_runtime_generic_backend_child_state(identity, state.words, &descriptor, 0);
            const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
            end_display_scene_update(resource->scene_identifier, &transform, &destination_rectangle);
        }
        set_runtime_generic_backend_child_context(identity, context);
        return;
    }

    if(query_runtime_generic_backend_child_state(identity, state.words, nullptr, context) != 0)
    {
        DisplaySceneNode *scene = nullptr;
        if(context[1] != 0)
            scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(query_display_scene_by_index(static_cast<int32_t>(context[1]), &descriptor, nullptr)));
        if(begin_display_scene_update(resource->scene_identifier) == 0)
        {
            DisplayRectangle *state_rectangle = &state.fields.rectangle;
            destination_rectangle.left = static_cast<int32_t>(state.words[5]) - resource->x;
            destination_rectangle.top = static_cast<int32_t>(state.words[6]) - resource->y;
            destination_rectangle.right = destination_rectangle.left + state_rectangle->right;
            destination_rectangle.bottom = destination_rectangle.top + state_rectangle->bottom;
            blit_display_scene(scene, 0, 0, reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(resource->scene_identifier)), &destination_rectangle, 0);
            descriptor.width = resource->scene_descriptor.width;
            descriptor.height = resource->scene_descriptor.height;
            descriptor.pixels = resource->scene_descriptor.pixels;
            descriptor.y = static_cast<int16_t>(state.words[6]) - static_cast<int16_t>(resource->y);
            descriptor.x = static_cast<int16_t>(state.words[5]) - static_cast<int16_t>(resource->x);
            if(!are_runtime_generic_comments_suppressed())
                publish_runtime_generic_backend_child_state(identity, state.words, &descriptor, 0);
            const DisplayRectangleTransform transform = display_rectangle_transform(resource->scene_descriptor);
            end_display_scene_update(resource->scene_identifier, &transform, &destination_rectangle);
        }
    }
}

int32_t update_runtime_resource_animation_backend(RuntimeMediaBackend *backend)
{
    auto *resource = static_cast<RuntimeResourceObject *>(backend->extension_data);
    uint32_t flags = backend->media_flags;
    if((flags & RUNTIME_MEDIA_CLOSE_REQUESTED) != 0)
    {
        const uint32_t resource_flags = resource->type_flags;
        queue_runtime_resource_destruction(resource, (resource_flags & 2) == 0);
        return 1;
    }
    if((flags & RUNTIME_MEDIA_INITIALIZING) != 0)
        return 0;
    if((flags & RUNTIME_MEDIA_DECODE_STARTED) != 0)
    {
        if(current_runtime_resource == resource && (flags & RUNTIME_MEDIA_SYNCHRONIZED_TIMING) == 0 && static_cast<int32_t>(backend->frame_duration * 4) < backend->timing_correction)
        {
            resource->state_flags |= 1;
        }
        else
        {
            begin_display_scene_update(resource->scene_identifier);
            resource->state_flags &= ~1u;
        }
        render_runtime_generic_backend_child(backend);
        backend->media_flags &= ~RUNTIME_MEDIA_DECODE_STARTED;
        return 1;
    }
    if((flags & RUNTIME_MEDIA_PAUSE_NOTIFIED) != 0)
    {
        // Win32 normally let a non-deferred secondary animation consume its initial pending state before the runtime-wide pause could notify it. Preserve that ordering invariant when standard-thread
        // scheduling delivers the pause notification first. Serialize with finalization and change only the pending bit so a stale callback snapshot cannot restore the animation's initial paused
        // state.
        const bool registration_allowed = (resource->type_flags & (RUNTIME_RESOURCE_DEFERRED_LOAD | RUNTIME_RESOURCE_PRIMARY)) != RUNTIME_RESOURCE_PRIMARY;
        if(registration_allowed && (flags & RUNTIME_MEDIA_RESOURCE_PENDING) != 0)
        {
            bool registered = false;
            lock_runtime_mutex_forever(runtime_media_backend_mutex, runtime_infinite_wait);
            if((backend->media_flags & RUNTIME_MEDIA_RESOURCE_PENDING) != 0)
            {
                backend->media_flags &= ~RUNTIME_MEDIA_RESOURCE_PENDING;
                registered = true;
            }
            unlock_runtime_mutex(runtime_media_backend_mutex);
            if(registered && (resource->type_flags & RUNTIME_RESOURCE_HALF_SIZE) == 0)
                ++runtime_resource_count;
        }
        return 1;
    }
    if((flags & RUNTIME_MEDIA_LOOP_BOUNDARY) != 0)
    {
        if(resource->frame_limit != RUNTIME_RESOURCE_FRAME_LIMIT_UNBOUNDED)
        {
            if(resource->frames_remaining == 0)
            {
                backend->media_flags = flags | RUNTIME_MEDIA_PAUSED;
                resource->frames_remaining = resource->frame_limit;
            }
            else
            {
                --resource->frames_remaining;
            }
        }
        backend->media_flags &= ~RUNTIME_MEDIA_LOOP_BOUNDARY;
        return 1;
    }
    if((flags & RUNTIME_MEDIA_PALETTE_CHANGED) != 0)
    {
        configure_runtime_resource_palette(resource);
        if((resource->type_flags & RUNTIME_RESOURCE_PRIMARY) != 0)
        {
            if((backend->media_flags & RUNTIME_MEDIA_RESOURCE_PENDING) == 0)
                apply_display_palette(backend->palette_entries, 0x20000);
            backend->dirty_left = 0;
            backend->dirty_top = 0;
            const auto *format = static_cast<const uint16_t *>(backend->format_data);
            backend->dirty_right = static_cast<uint32_t>(format[4]) * resource->requested_width;
            backend->dirty_bottom = static_cast<uint32_t>(format[5]) * resource->requested_height;
        }
    }
    if((backend->media_flags & RUNTIME_MEDIA_RESOURCE_PENDING) != 0 && (resource->type_flags & RUNTIME_RESOURCE_HALF_SIZE) == 0)
        ++runtime_resource_count;
    if((backend->media_flags & RUNTIME_MEDIA_FRAME_DECODED) != 0)
    {
        update_runtime_generic_backend_child(backend);
        if((resource->state_flags & RUNTIME_RESOURCE_PRESENTATION_DEFERRED) == 0)
        {
            const DisplayRectangleTransform transform{ static_cast<int16_t>(backend->destination_x), static_cast<int16_t>(backend->destination_y), backend->destination_stride,
                backend->destination_reserved };
            const DisplayRectangle dirty_rectangle{ backend->dirty_left, backend->dirty_top, backend->dirty_right, backend->dirty_bottom };
            end_display_scene_update(resource->scene_identifier, &transform, &dirty_rectangle);
        }
        backend->media_flags &= ~RUNTIME_MEDIA_FRAME_DECODED;
    }
    backend->media_flags &= ~(RUNTIME_MEDIA_RESOURCE_PENDING | RUNTIME_MEDIA_PALETTE_CHANGED | RUNTIME_MEDIA_PIXELS_CHANGED | RUNTIME_MEDIA_LOOP_BOUNDARY);
    return 1;
}

uint32_t get_runtime_generic_backend_child_flags(void *identity)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child == nullptr)
        return RUNTIME_GENERIC_CHILD_FLAGS_UNAVAILABLE;
    const uint32_t flags = child->flags;
    release_runtime_generic_backend_child_lock(child);
    return flags;
}

void clear_runtime_generic_backend_child_ready(void *identity)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        child->flags &= ~2u;
        release_runtime_generic_backend_child_lock(child);
    }
}

void enable_runtime_generic_backend_child_mode_200(void *identity)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        child->flags |= RUNTIME_GENERIC_CHILD_MODE_200;
        release_runtime_generic_backend_child_lock(child);
    }
}

void disable_runtime_generic_backend_child_mode_200(void *identity)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        child->flags &= ~RUNTIME_GENERIC_CHILD_MODE_200;
        release_runtime_generic_backend_child_lock(child);
    }
}

bool get_runtime_generic_backend_child_context(void *identity, uintptr_t *context)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        context[0] = child->context[0];
        context[1] = child->context[1];
        release_runtime_generic_backend_child_lock(child);
    }
    return child != nullptr;
}

bool set_runtime_generic_backend_child_context(void *identity, const uintptr_t *context)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        child->context[0] = context[0];
        child->context[1] = context[1];
        release_runtime_generic_backend_child_lock(child);
    }
    return child != nullptr;
}

uint32_t query_runtime_generic_backend_child_state(void *identity, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context)
{
    uint32_t result = 0;
    if(state != nullptr)
        state[0] = RUNTIME_GENERIC_TEXT_POSITION_NOT_FOUND;
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child != nullptr)
    {
        if((child->flags & 2) != 0)
        {
            if(descriptor != nullptr)
                *descriptor = child->descriptor;
            if(context != nullptr)
                std::memcpy(context, child->context, sizeof(child->context));
            if(state != nullptr)
                std::memcpy(state, child->state, sizeof(child->state));
            result = 1;
        }
        release_runtime_generic_backend_child_lock(child);
    }
    return result;
}

void *destroy_runtime_generic_backend_child(void *identity)
{
    RuntimeGenericBackendChild *child = acquire_runtime_generic_backend_child(identity);
    if(child == nullptr)
        return nullptr;
    RuntimeGenericBackend *parent = child->parent;
    lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
    RuntimeGenericBackendChild *previous = nullptr;
    RuntimeGenericBackendChild *current = parent->children;
    while(current != nullptr && current != child)
    {
        previous = current;
        current = current->next;
    }
    if(previous == nullptr)
        parent->children = child->next;
    else
        previous->next = child->next;
    --parent->child_count;
    unlock_runtime_mutex(runtime_generic_backend_mutex);
    free_runtime_heap(runtime_process_heap(), 0, child);
    return reinterpret_cast<void *>(1);
}

uint32_t destroy_runtime_generic_backend(void *identity)
{
    RuntimeGenericBackend *backend = acquire_runtime_generic_backend(identity);
    if(backend == nullptr)
        return 0;
    while(backend->children != nullptr)
        destroy_runtime_generic_backend_child(backend->children->identity);
    lock_runtime_mutex_forever(runtime_generic_backend_mutex, runtime_infinite_wait);
    RuntimeGenericBackend *previous = nullptr;
    RuntimeGenericBackend *current = runtime_generic_backend_head;
    while(current != nullptr && current != backend)
    {
        previous = current;
        current = current->next;
    }
    RuntimeGenericBackend *next = backend->next;
    if(previous != nullptr)
    {
        previous->next = backend->next;
        next = runtime_generic_backend_head;
    }
    runtime_generic_backend_head = next;
    unlock_runtime_mutex(runtime_generic_backend_mutex);
    free_runtime_heap(runtime_process_heap(), 0, backend);
    return 1;
}


} // namespace freegag
