#include "script.h"
#include "runtime_internal.h"

namespace freegag
{
int32_t select_bounded_random_value(int32_t minimum, int32_t maximum)
{
    if(!script_random_seeded)
    {
        std::srand(runtime_milliseconds());
        script_random_seeded = true;
    }
    if(minimum < -10000)
        minimum = -10000;
    if(maximum > 10000)
        maximum = 10000;
    if(maximum <= minimum)
        return minimum;
    return minimum + std::rand() % (maximum - minimum);
}


int copy_string(char *destination, const char *source)
{
    int length = 0;
    while((*destination = *source) != '\0')
    {
        ++destination;
        ++source;
        ++length;
    }
    return length;
}

ScriptTextBuffer *create_script_text_buffer()
{
    constexpr uint32_t allocation_size = 64000;
    auto *buffer = reinterpret_cast<ScriptTextBuffer *>(new (std::nothrow) uint8_t[allocation_size]{});
    if(buffer == nullptr)
        return nullptr;
    buffer->length = 0;
    buffer->capacity = allocation_size - sizeof(*buffer);
    buffer->data = reinterpret_cast<char *>(buffer + 1);
    return buffer;
}

void clear_script_text_buffer(ScriptTextBuffer *buffer)
{
    if(buffer != nullptr)
        buffer->length = 0;
}

void begin_script_text_document(ScriptTextBuffer *buffer)
{
    if(buffer != nullptr)
    {
        buffer->length += copy_string(buffer->data + buffer->length, "[CFG]");
        buffer->data[buffer->length++] = '\r';
        buffer->data[buffer->length++] = '\n';
        buffer->data[buffer->length++] = '\r';
        buffer->data[buffer->length++] = '\n';
    }
}

void end_script_text_document(ScriptTextBuffer *buffer)
{
    if(buffer != nullptr)
    {
        buffer->data[buffer->length++] = '\r';
        buffer->data[buffer->length++] = '\n';
        buffer->length += copy_string(buffer->data + buffer->length, "[END]");
    }
}

void append_script_text_property(ScriptTextBuffer *buffer, uint32_t property, const char *value)
{
    if(buffer == nullptr)
        return;
    const char *name = nullptr;
    switch(property)
    {
    case 0x01:
        name = "object";
        break;
    case 0x03:
        name = "event";
        break;
    case 0x04:
        name = "mouse";
        break;
    case 0x07:
        name = "command";
        break;
    case 0x08:
        name = "list";
        break;
    case 0x0b:
        name = "fademask";
        break;
    case 0x0c:
        name = "load";
        break;
    case 0x0d:
        name = "source";
        break;
    case 0x0e:
        name = "section";
        break;
    case 0x0f:
        name = "time";
        break;
    case 0x10:
        name = "font";
        break;
    case 0x20:
        name = "language";
        break;
    case 0x40:
        name = "inventory";
        break;
    case 0x50:
        name = "flags";
        break;
    case 0x60:
        name = "try";
        break;
    case 0x70:
        name = "catch";
        break;
    case 0x80:
        name = "volume";
        break;
    case 0x90:
        name = "exception";
        break;
    case 0xa0:
        name = "drivespeed";
        break;
    case 0xb0:
        name = "exfiles";
        break;
    default:
        return;
    }
    buffer->length += copy_string(buffer->data + buffer->length, name);
    buffer->data[buffer->length++] = '=';
    if(value != nullptr)
    {
        buffer->length += copy_string(buffer->data + buffer->length, value);
        buffer->data[buffer->length++] = ' ';
    }
}

void end_script_text_statement(ScriptTextBuffer *buffer)
{
    if(buffer != nullptr)
    {
        if(buffer->data[buffer->length - 1] == ' ')
            --buffer->length;
        buffer->data[buffer->length++] = ';';
        buffer->data[buffer->length++] = '\r';
        buffer->data[buffer->length++] = '\n';
    }
}

// Shared helper for the slash-scope emitters.
static void append_script_text_scope_name(ScriptTextBuffer *buffer, const char *name)
{
    buffer->data[buffer->length++] = '/';
    buffer->length += copy_string(buffer->data + buffer->length, name);
    buffer->data[buffer->length++] = ':';
}

void append_script_text_scope(ScriptTextBuffer *buffer, uint32_t scope)
{
    if(buffer == nullptr)
        return;
    switch(scope)
    {
    case SCRIPT_SCOPE_FILE:
        append_script_text_scope_name(buffer, "FILE");
        return;
    case SCRIPT_SCOPE_ZONE:
        append_script_text_scope_name(buffer, "ZONE");
        return;
    case SCRIPT_SCOPE_POSITION:
        append_script_text_scope_name(buffer, "POS");
        return;
    case SCRIPT_SCOPE_COMMAND:
        append_script_text_scope_name(buffer, "COMM");
        return;
    case SCRIPT_SCOPE_MOUSE:
        append_script_text_scope_name(buffer, "MOUSE");
        return;
    case SCRIPT_SCOPE_ALTERNATE_MOUSE:
        append_script_text_scope_name(buffer, "AMOUSE");
        return;
    case SCRIPT_SCOPE_FLAGS:
        append_script_text_scope_name(buffer, "F");
        return;
    case SCRIPT_SCOPE_GLOBAL:
        append_script_text_scope_name(buffer, "GLOBAL");
        return;
    default:
        return;
    }
}

void append_script_text_preload_directive(ScriptTextBuffer *buffer, uint32_t scope)
{
    if(buffer != nullptr && scope == SCRIPT_SCOPE_PRELOAD)
        append_script_text_scope_name(buffer, "PRELOAD");
}

void append_script_text_scoped_tokens(ScriptTextBuffer *buffer, uint32_t scope, const char *text)
{
    if(text[0] == '\0')
        return;

    int offset = 0;
    do
    {
        int token_length = 0;
        char token[32];
        char character = text[offset];
        while(character != '\0' && text[offset] != ':' && text[offset] != ' ')
        {
            token[token_length++] = text[offset];
            ++offset;
            character = text[offset];
        }
        if(token_length != 0)
        {
            token[token_length] = '\0';
            append_script_text_scope(buffer, scope);
            if(text[offset] == ' ')
            {
                ++offset;
                append_script_text_delimiter(buffer, token, ' ');
            }
            else if(text[offset] == ':')
            {
                ++offset;
                append_script_text_delimiter(buffer, token, ':');
                token_length = 0;
                character = text[offset];
                while(character != '\0' && text[offset] > '/' && text[offset] < ':')
                {
                    token[token_length++] = text[offset];
                    ++offset;
                    character = text[offset];
                }
                if(token_length != 0)
                {
                    token[token_length] = '\0';
                    ++offset;
                    append_script_text_delimiter(buffer, token, ' ');
                }
            }
        }
    } while(text[offset] != '\0');
}

void append_script_text_delimiter(ScriptTextBuffer *buffer, const char *text, char delimiter)
{
    if(buffer != nullptr)
    {
        if(text != nullptr)
            buffer->length += copy_string(buffer->data + buffer->length, text);
        buffer->data[buffer->length++] = delimiter;
    }
}

void append_script_text_integer(ScriptTextBuffer *buffer, uint32_t value, char delimiter)
{
    if(buffer != nullptr)
    {
        if(static_cast<int32_t>(value) < 0)
        {
            value = 0 - value;
            buffer->data[buffer->length++] = '-';
        }
        uint32_t divisor = 1;
        uint32_t quotient = value / 10;
        while(quotient != 0)
        {
            quotient = value / (divisor * 100);
            divisor *= 10;
        }
        while(divisor != 0)
        {
            buffer->data[buffer->length++] = static_cast<char>(value / divisor) + '0';
            value %= divisor;
            divisor /= 10;
        }
        buffer->data[buffer->length++] = delimiter;
    }
}

int find_script_property_value(char *value, const char *property_name, const char *text, uint32_t text_length, uint32_t start_offset)
{
    if(value != nullptr)
        std::memset(value, 0, 32);

    uint32_t offset = start_offset;
    while(true)
    {
        if(offset < text_length)
        {
            while(text[offset] != '[' && text[offset] != '=')
            {
                ++offset;
                if(offset >= text_length)
                    return -1;
            }

            if(text[offset] != '[')
            {
                if(offset != 0)
                {
                    while(true)
                    {
                        const char preceding = text[offset - 1];
                        if(preceding == ']' || preceding == '\n' || preceding == '\r' || preceding == ' ' || preceding == ';')
                            break;
                        --offset;
                        if(offset == 0)
                            break;
                    }
                }

                char candidate[32];
                int candidate_length = 0;
                while(text[offset] != '=' && candidate_length < 31)
                    candidate[candidate_length++] = text[offset++];
                ++offset;
                candidate[candidate_length] = '\0';
                if(strings_equal(candidate, property_name))
                {
                    if(value != nullptr)
                    {
                        int value_length = 0;
                        while(offset < text_length)
                        {
                            const char character = text[offset];
                            if(character == '[' || character == ':' || character == ';' || character == ' ' || character == '\r')
                                break;
                            value[value_length++] = character;
                            ++offset;
                            if(value_length >= 31)
                                break;
                        }
                    }
                    return static_cast<int>(offset + 1);
                }
            }
        }

        if(offset >= text_length || text[offset] == '[')
            return -1;
    }
}

int find_script_section(const char *section_name, const char *text, int text_length)
{
    int remaining = text_length;
    const char *cursor = text;
    bool found_opening_bracket = false;
    while(true)
    {
        while(remaining != 0)
        {
            --remaining;
            found_opening_bracket = *cursor++ == '[';
            if(found_opening_bracket)
                break;
        }
        if(!found_opening_bracket)
            return -1;

        const char *candidate = section_name;
        while(true)
        {
            const char text_character = *cursor;
            const char name_character = *candidate;
            --remaining;
            if(remaining == 0)
                return -1;
            ++cursor;
            ++candidate;
            if(name_character == '\0' || text_character == ']')
            {
                found_opening_bracket = false;
                if(static_cast<char>(text_character + name_character) == ']')
                    return text_length - remaining;
                break;
            }
            found_opening_bracket = false;
            if(text_character != name_character)
                break;
        }
    }
}

int32_t parse_path_numeric_identifier(const char *path)
{
    uint32_t offset = 0;
    int32_t result = -1;
    while(path[offset] != '\0' && path[offset] != '\\')
        ++offset;
    if(path[offset] == '\0')
    {
        if(offset != 0)
            offset = 0;
        if(path[offset] == '\0')
            return result;
    }

    do
    {
        const char character = path[offset];
        if(character >= '0' && character <= '9')
        {
            if(result == -1)
                result = character - '0';
            else
                result = character - '0' + result * 10;
        }
        ++offset;
    } while(path[offset] != '\0');
    return result;
}

uint32_t extract_script_property_name(ScriptParserState *parser, char *name)
{
    name[0] = '\0';
    if(parser == nullptr)
        return SCRIPT_PARSE_END;

    const uint32_t text_length = parser->text_length;
    uint32_t token_start = parser->cursor;
    for(uint32_t offset = parser->cursor; offset < text_length; ++offset)
    {
        switch(parser->text[offset])
        {
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case '(':
        case ')':
        case ',':
        case '/':
        case ':':
        case ';':
            token_start = offset + 1;
            break;
        case '=':
        {
            std::memset(name, 0, 32);
            uint32_t name_length = 0;
            while(token_start < text_length && name_length < 31 && parser->text[token_start] != '=')
                name[name_length++] = parser->text[token_start++];
            name[name_length] = '\0';
            parser->cursor = token_start + 1;
            return name_length;
        }
        case '[':
            return SCRIPT_PARSE_END;
        default:
            break;
        }
    }
    return SCRIPT_PARSE_END;
}

uint32_t extract_script_scope_name(ScriptParserState *parser, char *name)
{
    name[0] = '\0';
    if(parser == nullptr)
        return SCRIPT_PARSE_END;

    uint32_t offset = parser->cursor;
    while(offset < parser->text_length)
    {
        const char character = parser->text[offset];
        if(character == '/')
            break;
        if(character == ';' || character == '[')
            return SCRIPT_PARSE_END;
        ++offset;
    }
    if(offset >= parser->text_length)
        return SCRIPT_PARSE_END;

    std::memset(name, 0, 32);
    uint32_t name_length = 0;
    while(++offset < parser->text_length && name_length < 31)
    {
        const char character = parser->text[offset];
        switch(character)
        {
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case '(':
        case ',':
        case '/':
        case ':':
        case ';':
        case '[':
            parser->cursor = offset;
            return name_length;
        default:
            name[name_length++] = character;
            break;
        }
    }
    name[name_length] = '\0';
    parser->cursor = offset;
    return name_length;
}

uint32_t extract_script_parenthesized_text(ScriptParserState *parser, char *text, uint32_t text_capacity)
{
    text[0] = '\0';
    if(parser == nullptr || parser->cursor >= parser->text_length || parser->text[parser->cursor] != '(')
        return SCRIPT_PARSE_END;

    std::memset(text, 0, text_capacity);
    uint32_t text_length = 0;
    uint32_t offset = parser->cursor;
    while(offset + 1 < parser->text_length && text_length < text_capacity - 1 && parser->text[offset + 1] != ')')
    {
        const char character = parser->text[offset + 1];
        if(character == '[')
            return SCRIPT_PARSE_END;
        text[text_length++] = character;
        ++offset;
    }
    text[text_length] = '\0';
    parser->cursor = offset + 2;
    return text_length;
}

int find_whitespace_token_index(const char *text, const char *token)
{
    int token_index = -1;
    int text_offset = 0;
    while(text[text_offset] != '\0')
    {
        while(text[text_offset] == '\t' || text[text_offset] == '\n' || text[text_offset] == '\r' || text[text_offset] == ' ')
            ++text_offset;
        if(text[text_offset] == '\0')
            return token[0] == '\0' ? token_index + 1 : -1;

        ++token_index;
        int candidate_offset = 0;
        int matches = 1;
        while(true)
        {
            const char character = text[text_offset++];
            if(character == '\0' || character == '\t' || character == '\n' || character == '\r' || character == ' ')
            {
                if(matches != 0 && token[candidate_offset] == '\0')
                    return token_index;
                if(character == '\0')
                    return -1;
                break;
            }
            if(matches != 0)
            {
                if(token[candidate_offset] == character)
                    ++candidate_offset;
                else
                    --matches;
            }
        }
    }
    return -1;
}

uint32_t extract_script_token(ScriptParserState *parser, char *token, uint32_t token_capacity)
{
    token[0] = '\0';
    if(parser == nullptr)
        return SCRIPT_PARSE_END;

    uint32_t offset = parser->cursor;
    while(true)
    {
        if(offset >= parser->text_length)
            return SCRIPT_PARSE_END;
        switch(parser->text[offset])
        {
        case '(':
        {
            if(parser->text_length - 1 <= offset)
                return SCRIPT_PARSE_END;
            char character;
            do
            {
                character = parser->text[offset + 1];
                ++offset;
                if(offset >= parser->text_length)
                    break;
                if(character == ')')
                    break;
            } while(character != '[');
            if(character != ')')
                return SCRIPT_PARSE_END;
        }
            [[fallthrough]];
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case ',':
        case ':':
            ++offset;
            break;
        case '/':
        case ';':
        case '[':
            return SCRIPT_PARSE_END;
        default:
        {
            std::memset(token, 0, token_capacity);
            uint32_t token_length = 0;
            while(offset < parser->text_length && token_length < token_capacity - 1)
            {
                const char character = parser->text[offset];
                switch(character)
                {
                case '\t':
                case '\n':
                case '\r':
                case ' ':
                case '(':
                case ',':
                case '/':
                case ':':
                case ';':
                case '[':
                    token[token_length] = '\0';
                    parser->cursor = offset;
                    return token_length;
                default:
                    token[token_length++] = character;
                    ++offset;
                    break;
                }
            }
            token[token_length] = '\0';
            parser->cursor = offset;
            return token_length;
        }
        }
    }
}

void parse_script_typed_value(ScriptParserState *parser, void *value, uint32_t *value_type)
{
    const uint32_t saved_cursor = parser->cursor;
    const int32_t integer_value = parse_script_integer_expression(parser);
    *static_cast<int32_t *>(value) = integer_value;
    if(integer_value != SCRIPT_INTEGER_INVALID)
    {
        *value_type = SCRIPT_VALUE_TYPE_INTEGER;
        return;
    }

    parser->cursor = saved_cursor;
    const uint32_t flag = parse_image_flag(parser);
    *static_cast<uint32_t *>(value) = flag;
    if(flag != 0)
    {
        *value_type = SCRIPT_VALUE_TYPE_BOOLEAN;
        return;
    }

    parser->cursor = saved_cursor;
    if(parse_script_value_token(parser, static_cast<char *>(value), 0x20) != SCRIPT_PARSE_END)
    {
        *value_type = SCRIPT_VALUE_TYPE_STRING;
        return;
    }
    *value_type = SCRIPT_VALUE_TYPE_INVALID;
}

void append_natural_mouse_image_flag(ScriptTextBuffer *buffer, uint32_t flags)
{
    if(buffer == nullptr)
        return;
    while(flags != 0)
    {
        uint32_t emitted = 0;
        if((flags & SCRIPT_IMAGE_NATURAL_MOUSE) != 0)
        {
            emitted = 1;
            flags &= ~SCRIPT_IMAGE_NATURAL_MOUSE;
            append_script_text_scope(buffer, 0x0a000000);
            buffer->length += copy_string(buffer->data + buffer->length, "NATURALMOUSE");
            buffer->data[buffer->length++] = ' ';
        }
        if(emitted == 0)
            break;
    }
}

void serialize_image_flag_overrides(ScriptTextBuffer *buffer, uint32_t flags)
{
    while(buffer != nullptr)
    {
        bool emitted = false;
        if((flags & SCRIPT_IMAGE_PRIMARY) != 0)
        {
            emitted = true;
            flags &= ~SCRIPT_IMAGE_PRIMARY;
            append_script_text_scope(buffer, 0x0a000000);
            buffer->length += copy_string(buffer->data + buffer->length, "PRIMARY");
            buffer->data[buffer->length++] = ' ';
        }
        if((flags & SCRIPT_IMAGE_NO_PALETTE) != 0)
        {
            emitted = true;
            flags &= ~SCRIPT_IMAGE_NO_PALETTE;
            if((script_runtime_root->flags & SCRIPT_RUNTIME_NO_PALETTE_ADJUSTMENT) == 0)
            {
                append_script_text_scope(buffer, 0x0a000000);
                buffer->length += copy_string(buffer->data + buffer->length, "NOPAL");
                buffer->data[buffer->length++] = ' ';
            }
        }
        else if((script_runtime_root->flags & SCRIPT_RUNTIME_NO_PALETTE_ADJUSTMENT) != 0)
        {
            buffer->data[buffer->length++] = '/';
            buffer->length += copy_string(buffer->data + buffer->length, "INVERT_NOPAL");
            buffer->data[buffer->length++] = ' ';
        }
        if(!emitted || flags == 0)
            return;
    }
}

uint32_t parse_script_parameter_token(const char *text, int32_t token_index, void *value, uint32_t *value_type)
{
    char token[0x104];
    token[0] = '\0';
    int32_t current_index = -1;
    uint32_t offset = 0;
    while(text[offset] != '\0')
    {
        while(text[offset] == '\t' || text[offset] == '\n' || text[offset] == '\r' || text[offset] == ' ')
            ++offset;
        if(text[offset] == '\0')
            break;
        ++current_index;
        if(current_index == token_index)
        {
            uint32_t length = 0;
            while(text[offset] != '\0' && text[offset] != '\t' && text[offset] != '\n' && text[offset] != '\r' && text[offset] != ' ')
                token[length++] = text[offset++];
            token[length] = '\0';
            break;
        }
        while(text[offset] != '\0' && text[offset] != '\t' && text[offset] != '\n' && text[offset] != '\r' && text[offset] != ' ')
            ++offset;
    }

    const uint32_t expected_type = *value_type;
    if(token[0] == '\0')
    {
        if(expected_type == SCRIPT_VALUE_TYPE_BOOLEAN)
            *static_cast<uint32_t *>(value) = SCRIPT_BOOLEAN_FALSE;
        else if(expected_type == SCRIPT_VALUE_TYPE_INTEGER || expected_type == SCRIPT_VALUE_TYPE_STRING)
            *static_cast<uint32_t *>(value) = 0;
        return 0;
    }

    ScriptParserState token_parser;
    token_parser.text = token;
    token_parser.text_length = static_cast<uint32_t>(std::strlen(token));
    token_parser.cursor = 0;
    parse_script_typed_value(&token_parser, value, value_type);
    return *value_type != SCRIPT_VALUE_TYPE_INVALID && (expected_type == SCRIPT_VALUE_TYPE_NONE || expected_type == *value_type) ? 1 : 0;
}

uint32_t evaluate_script_parameter(ScriptParserState *parser, const char *name, void *value, uint32_t *value_type)
{
    const int32_t token_index = find_whitespace_token_index(parser->scratch_text, name);
    return parse_script_parameter_token(parser->creation_text, token_index, value, value_type);
}

int32_t parse_script_integer_expression(ScriptParserState *parser)
{
    if(parser == nullptr)
        return SCRIPT_INTEGER_INVALID;

    const uint32_t saved_cursor = parser->cursor;
    int32_t result = parse_script_integer_literal(parser);
    if(result != SCRIPT_INTEGER_INVALID)
        return result;

    char token[0x20];
    if(extract_script_token(parser, token, sizeof(token)) == SCRIPT_PARSE_END)
    {
        parser->cursor = saved_cursor;
        return SCRIPT_INTEGER_INVALID;
    }
    if(fixed_dword_memory_equal(token, "PARAM", 4))
    {
        parse_script_value_token(parser, token, sizeof(token));
        // The evaluator materializes the parameter before checking its type, so reserve enough temporary storage for any typed value.
        uint32_t parameter_value[8];
        uint32_t value_type = SCRIPT_VALUE_TYPE_INTEGER;
        if(evaluate_script_parameter(parser, token, parameter_value, &value_type) == 0)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        return static_cast<int32_t>(parameter_value[0]);
    }
    if(fixed_dword_memory_equal(token, "RAND", 4))
    {
        int32_t minimum = parse_script_integer_expression(parser);
        if(minimum == SCRIPT_INTEGER_INVALID)
            minimum = -10000;
        int32_t maximum = parse_script_integer_expression(parser);
        if(maximum == SCRIPT_INTEGER_INVALID)
            maximum = 10000;
        return select_bounded_random_value(minimum, maximum);
    }
    if(fixed_dword_memory_equal(token, "RELZ", 4))
    {
        char name[0x20];
        if(parse_script_value_token(parser, name, sizeof(name)) == SCRIPT_PARSE_END)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        int32_t offset = parse_script_integer_expression(parser);
        if(offset == SCRIPT_INTEGER_INVALID)
            offset = 0;
        RuntimeTreeLink84 *link = find_global_runtime_tree_link_0084_by_name(name);
        if(link == nullptr)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        return offset + (token[4] == 'X' ? link->x : link->y);
    }
    if(fixed_dword_memory_equal(token, "RELI", 4))
    {
        char name[0x20];
        if(parse_script_value_token(parser, name, sizeof(name)) == SCRIPT_PARSE_END)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        int32_t offset = parse_script_integer_expression(parser);
        if(offset == SCRIPT_INTEGER_INVALID)
            offset = 0;
        RuntimeTreePrimaryResourceLink *link = find_global_runtime_tree_primary_resource_link_by_name(name);
        if(link == nullptr)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        return offset + (token[4] == 'X' ? link->x : link->y);
    }
    if(fixed_dword_memory_equal(token, "RELM", 4))
    {
        int32_t offset = parse_script_integer_expression(parser);
        if(offset == SCRIPT_INTEGER_INVALID)
            offset = 0;
        int32_t runtime_value;
        script_runtime_root->get_property(token[4] == 'X' ? ScriptRuntimeProperty::POINTER_X : ScriptRuntimeProperty::POINTER_Y, nullptr, &runtime_value);
        return runtime_value + offset;
    }
    if(fixed_dword_memory_equal(token, "VALUE", 4))
    {
        char object_name[0x20];
        char field_name[0x20];
        if(parse_script_value_token(parser, object_name, sizeof(object_name)) == SCRIPT_PARSE_END || parse_script_value_token(parser, field_name, sizeof(field_name)) == SCRIPT_PARSE_END)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        result = get_script_object_integer(object_name, field_name);
        if(result == SCRIPT_INTEGER_INVALID)
            parser->cursor = saved_cursor;
        return result;
    }
    if(fixed_dword_memory_equal(token, "PHASE", 4))
    {
        if(parse_script_value_token(parser, token, sizeof(token)) == SCRIPT_PARSE_END)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        RuntimeTreePrimaryResourceLink *link = find_global_runtime_tree_primary_resource_link_by_name(token);
        if(link == nullptr)
        {
            parser->cursor = saved_cursor;
            return SCRIPT_INTEGER_INVALID;
        }
        script_runtime_root->get_property(ScriptRuntimeProperty::RESOURCE_FRAME, &link->resource_identity, &result);
        return result;
    }

    parser->cursor = saved_cursor;
    return SCRIPT_INTEGER_INVALID;
}

uint32_t parse_script_value_token(ScriptParserState *parser, char *value, uint32_t value_capacity)
{
    uint32_t result = extract_script_token(parser, value, value_capacity);
    if(result == SCRIPT_PARSE_END)
        return result;

    if(fixed_dword_memory_equal(value, "PARAM", 4))
    {
        char parameter_name[0x20];
        parse_script_value_token(parser, parameter_name, sizeof(parameter_name));
        uint32_t value_type = SCRIPT_VALUE_TYPE_STRING;
        result = evaluate_script_parameter(parser, parameter_name, value, &value_type) == 0 ? SCRIPT_PARSE_END : 0x20;
    }
    if(fixed_dword_memory_equal(value, "SVALUE", 4))
    {
        char object_name[0x20];
        char field_name[0x20];
        parse_script_value_token(parser, object_name, sizeof(object_name));
        parse_script_value_token(parser, field_name, sizeof(field_name));
        result = get_script_object_string(object_name, field_name, value) == 0 ? SCRIPT_PARSE_END : 0x20;
    }
    return result;
}

uint32_t parse_image_flag(ScriptParserState *parser)
{
    if(parser == nullptr)
        return SCRIPT_PARSE_END;

    char token[0x20];
    if(extract_script_token(parser, token, sizeof(token)) == SCRIPT_PARSE_END)
        return SCRIPT_PARSE_END;
    if(fixed_dword_memory_equal(token, "BVALUE", 4))
    {
        char object_name[0x20];
        char field_name[0x20];
        parse_script_value_token(parser, object_name, sizeof(object_name));
        parse_script_value_token(parser, field_name, sizeof(field_name));
        uint32_t value = SCRIPT_BOOLEAN_TRUE;
        return query_or_create_script_object_field(object_name, field_name, &value, 1);
    }
    if(fixed_dword_memory_equal(token, "PARAM", 4))
    {
        char parameter_name[0x20];
        parse_script_value_token(parser, parameter_name, sizeof(parameter_name));
        // A mismatched parameter may be a string, and the evaluator writes it before reporting the mismatch.
        uint32_t parameter_value[8];
        uint32_t value_type = SCRIPT_VALUE_TYPE_BOOLEAN;
        if(evaluate_script_parameter(parser, parameter_name, parameter_value, &value_type) == 0)
            return SCRIPT_PARSE_END;
        return parameter_value[0];
    }

    struct ImageFlagMapping
    {
        const char *name;
        uint32_t value;
    };
    static constexpr ImageFlagMapping mappings[]{
        { "PRIMARY",        0x00000001           },
        { "CONTROL",        0x00000020           },
        { "PERMANENT",      0x00000400           },
        { "LOAD_ONLY",      0x00000200           },
        { "DOUBLE",         0x00200000           },
        { "SEPARATED",      0x00000040           },
        { "STOPPED",        0x00000010           },
        { "NOSKIP",         0x00100000           },
        { "NOCLS",          0x01000000           },
        { "NOPAL",          0x04000000           },
        { "ON",             SCRIPT_BOOLEAN_TRUE  },
        { "OFF",            SCRIPT_BOOLEAN_FALSE },
        { "NATURALMOUSE",   0x00010000           },
        { "DUAL",           0x00200000           },
        { "ONE_STEP",       0x00000200           },
        { "RESTART",        0x00020000           },
        { "NOFADE",         0x00000001           },
        { "PALFADE",        0x00000002           },
        { "FRAMEFADE",      0x00000004           },
        { "COLORED",        0x00000001           },
        { "LOWCASE",        0x00000010           },
        { "UPPCASE",        0x00000020           },
        { "NOINV",          0x00000010           },
        { "NOSAVE",         0x00000100           },
        { "RESIDENT",       0x00000400           },
        { "COMMENT",        0x00000800           },
        { "INVENTORY_PACK", 0x00001000           },
        { "NOMOUSE",        0x00002000           },
        { "NOCONTROL",      0x00004000           },
        { "NONTRANSP",      0x00000020           },
        { "STATIC",         0x00000002           },
        { "FIXSIZE",        0x02000000           },
        { "FIXPOS",         0x04000000           },
        { "NOCOMMENT",      0x00000001           },
        { "PAL_NOADJUST",   0x04000000           },
    };
    for(const ImageFlagMapping &mapping : mappings)
        if(strings_equal(token, mapping.name))
            return mapping.value;
    return 0;
}

uint32_t parse_runtime_tree_command_target(ScriptParserState *parser, char *resource_name, char *tree_name, uint32_t *flags)
{
    uint32_t saved_cursor = parser->cursor;
    *flags = parse_image_flag(parser);
    if(*flags != 0)
    {
        if(*flags == SCRIPT_PARSE_END)
            *flags = 0;
        return 0;
    }

    parser->cursor = saved_cursor;
    parse_script_value_token(parser, resource_name, 0x20);
    saved_cursor = parser->cursor;
    *flags = parse_image_flag(parser);
    if(*flags == SCRIPT_PARSE_END)
    {
        std::memcpy(tree_name, resource_name, 0x20);
        std::memcpy(resource_name, parser->resource->name, 0x20);
        *flags = 0;
        return 1;
    }
    if(*flags == 0)
    {
        parser->cursor = saved_cursor;
        parse_script_value_token(parser, tree_name, 0x20);
        const uint32_t trailing_flags = parse_image_flag(parser);
        if(static_cast<int32_t>(trailing_flags) > 0)
        {
            *flags = trailing_flags;
            return 1;
        }
    }
    else
    {
        std::memcpy(tree_name, resource_name, 0x20);
        std::memcpy(resource_name, parser->resource->name, 0x20);
    }
    return 1;
}


uint32_t apply_runtime_tree_image_flags(ScriptParserState *parser)
{
    auto *owner = static_cast<RuntimeTreeNode *>(parser->owner);
    while(true)
    {
        const uint32_t flag = parse_image_flag(parser);
        if(flag == SCRIPT_PARSE_END)
            break;
        if(flag == 1)
            script_runtime_root->flags |= flag;
        else if(flag == 0x04000000)
            script_runtime_root->palette_flags |= flag;
        else
            owner->flags |= flag;
    }
    return 1;
}

RuntimeTreeNode *update_conditional_runtime_tree(ScriptParserState *parser)
{
    RuntimeTreeNode *owner = parser->owner;
    char resource_name[0x20];
    char tree_name[0x20];
    if(parse_script_value_token(parser, resource_name, sizeof(resource_name)) == SCRIPT_PARSE_END)
        return nullptr;
    const uint32_t tree_name_result = parse_script_value_token(parser, tree_name, sizeof(tree_name));
    RuntimeGenericResourceNode *resource = nullptr;
    if(tree_name_result == SCRIPT_PARSE_END)
    {
        std::memcpy(tree_name, resource_name, sizeof(tree_name));
        resource = parser->resource;
    }

    RuntimeTreeNode *node = reinterpret_cast<RuntimeTreeNode *(*)(void *, const void *)>(find_runtime_tree_descendant_identity_by_name)(owner, tree_name);
    bool saw_condition = false;
    uint8_t conditions_match = 1;
    void *parent_selector = owner;
    while(true)
    {
        uint32_t scope = parse_script_scope_code(parser);
        if(scope == SCRIPT_SCOPE_VALUE)
        {
            saw_condition = true;
            char object_name[0x20];
            char field_name[0x20];
            uint8_t value[0x80];
            if(parse_script_value_token(parser, object_name, sizeof(object_name)) != SCRIPT_PARSE_END && parse_script_value_token(parser, field_name, sizeof(field_name)) != SCRIPT_PARSE_END)
            {
                parse_script_typed_value(parser, value, &scope);
                if(scope != SCRIPT_INTEGER_INVALID)
                    conditions_match = static_cast<uint8_t>(conditions_match != 0 && compare_script_object_field(object_name, field_name, value, static_cast<int32_t>(scope)));
            }
        }
        else if(scope == SCRIPT_SCOPE_GLOBAL)
        {
            if(owner->parent == reinterpret_cast<RuntimeTreeNode *>(static_cast<intptr_t>(-1)))
                return nullptr;
            parent_selector = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        }
        else if(scope == SCRIPT_SCOPE_CONTAINER_CONDITION)
        {
            saw_condition = true;
            char container_name[0x20];
            scope = parse_script_value_token(parser, container_name, sizeof(container_name));
            if(scope != SCRIPT_PARSE_END)
                conditions_match = static_cast<uint8_t>(conditions_match != 0 && script_object_container_state_matches_by_name(container_name));
        }

        if(scope == SCRIPT_PARSE_END)
        {
            if(node != nullptr && (node->flags & RUNTIME_TREE_COMMENT) != 0)
            {
                saw_condition = true;
                conditions_match = 0;
            }
            if(saw_condition)
            {
                if(node == nullptr)
                {
                    if(conditions_match != 0)
                    {
                        if(tree_name_result != SCRIPT_PARSE_END)
                            resource = find_or_load_runtime_generic_resource(resource_name);
                        node = create_runtime_tree_node(resource, parent_selector, tree_name, nullptr);
                    }
                }
                else if(conditions_match == 0)
                {
                    destroy_runtime_tree_node(node, nullptr);
                    node = nullptr;
                }
            }
            return node;
        }
    }
}

RuntimeTreeNode *create_conditional_runtime_tree(ScriptParserState *parser)
{
    RuntimeTreeNode *owner = parser->owner;
    char resource_name[0x20];
    char tree_name[0x20];
    if(parse_script_value_token(parser, resource_name, sizeof(resource_name)) == SCRIPT_PARSE_END)
        return nullptr;
    const uint32_t tree_name_result = parse_script_value_token(parser, tree_name, sizeof(tree_name));
    RuntimeGenericResourceNode *resource = nullptr;
    if(tree_name_result == SCRIPT_PARSE_END)
    {
        std::memcpy(tree_name, resource_name, sizeof(tree_name));
        resource = parser->resource;
    }

    void *parent_selector = owner;
    while(true)
    {
        uint32_t scope = parse_script_scope_code(parser);
        if(scope == SCRIPT_SCOPE_VALUE)
        {
            char object_name[0x20];
            char field_name[0x20];
            uint8_t value[0x80];
            if(parse_script_value_token(parser, object_name, sizeof(object_name)) != SCRIPT_PARSE_END && parse_script_value_token(parser, field_name, sizeof(field_name)) != SCRIPT_PARSE_END)
            {
                parse_script_typed_value(parser, value, &scope);
                if(scope != SCRIPT_INTEGER_INVALID && !compare_script_object_field(object_name, field_name, value, static_cast<int32_t>(scope)))
                    return nullptr;
            }
        }
        else if(scope == SCRIPT_SCOPE_GLOBAL)
        {
            if(owner->parent == reinterpret_cast<RuntimeTreeNode *>(static_cast<intptr_t>(-1)))
                return nullptr;
            parent_selector = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
        }
        else if(scope == SCRIPT_SCOPE_CONTAINER_CONDITION)
        {
            char container_name[0x20];
            scope = parse_script_value_token(parser, container_name, sizeof(container_name));
            if(scope != SCRIPT_PARSE_END && !script_object_container_state_matches_by_name(container_name))
                return nullptr;
        }

        if(scope == SCRIPT_PARSE_END)
        {
            if(tree_name_result != SCRIPT_PARSE_END)
                resource = find_or_load_runtime_generic_resource(resource_name);
            return create_runtime_tree_node(resource, parent_selector, tree_name, nullptr);
        }
    }
}

int32_t parse_script_integer_literal(ScriptParserState *parser)
{
    uint32_t offset = parser->cursor;
    bool negative = false;
    while(true)
    {
        if(offset >= parser->text_length)
            return SCRIPT_INTEGER_INVALID;
        switch(parser->text[offset])
        {
        case '(':
        {
            if(parser->text_length - 1 <= offset)
                return SCRIPT_INTEGER_INVALID;
            char character;
            do
            {
                character = parser->text[offset + 1];
                ++offset;
                if(offset >= parser->text_length)
                    break;
                if(character == ')')
                    break;
            } while(character != '[');
            if(character != ')')
                return SCRIPT_INTEGER_INVALID;
        }
            [[fallthrough]];
        case '\t':
        case '\n':
        case '\r':
        case ' ':
        case ',':
        case ':':
            ++offset;
            continue;
        case '-':
            negative = true;
            [[fallthrough]];
        case '+':
            ++offset;
            break;
        case '/':
        case ';':
        case '[':
            return SCRIPT_INTEGER_INVALID;
        default:
            break;
        }
        break;
    }

    int32_t value = 0;
    bool no_digits = true;
    while(offset < parser->text_length)
    {
        const char character = parser->text[offset];
        if(character < '0' || character > '9')
            break;
        value = character - '0' + value * 10;
        ++offset;
        no_digits = false;
    }
    if(no_digits)
        return SCRIPT_INTEGER_INVALID;
    parser->cursor = offset;
    return negative ? -value : value;
}

uint32_t parse_script_property_code(ScriptParserState *parser)
{
    if(parser == nullptr)
        return SCRIPT_PARSE_END;
    char name[32];
    if(extract_script_property_name(parser, name) == SCRIPT_PARSE_END)
        return SCRIPT_PARSE_END;

    struct Mapping
    {
        const char *name;
        uint32_t code;
    };
    static constexpr Mapping mappings[]{
        { "object",      0x01 },
        { "image",       0x05 },
        { "zone",        0x02 },
        { "mouse",       0x04 },
        { "event",       0x03 },
        { "local",       0x06 },
        { "command",     0x07 },
        { "list",        0x08 },
        { "path",        0x09 },
        { "load",        0x0c },
        { "sublocation", 0x0a },
        { "section",     0x0e },
        { "source",      0x0d },
        { "font",        0x10 },
        { "fademask",    0x0b },
        { "time",        0x0f },
        { "text",        0x30 },
        { "language",    0x20 },
        { "inventory",   0x40 },
        { "flags",       0x50 },
        { "try",         0x60 },
        { "catch",       0x70 },
        { "volume",      0x80 },
        { "exception",   0x90 },
        { "drivespeed",  0xa0 },
        { "exfiles",     0xb0 },
        { "class",       0xc0 },
        { "template",    0xd0 },
        { "params",      0xe0 },
        { "layer",       0xf0 },
    };
    for(const Mapping &mapping : mappings)
        if(strings_equal(name, mapping.name))
            return mapping.code;
    return 0;
}

uint32_t parse_script_scope_code(ScriptParserState *parser)
{
    if(parser == nullptr)
        return SCRIPT_PARSE_END;
    char name[32];
    if(extract_script_scope_name(parser, name) == SCRIPT_PARSE_END)
        return SCRIPT_PARSE_END;

    struct Mapping
    {
        const char *name;
        uint32_t code;
    };
    static constexpr Mapping mappings[]{
        { "FILE",         SCRIPT_SCOPE_FILE                },
        { "LIST",         SCRIPT_SCOPE_LIST                },
        { "GLOBAL",       SCRIPT_SCOPE_GLOBAL              },
        { "RECT",         SCRIPT_SCOPE_RECTANGLE           },
        { "F",            SCRIPT_SCOPE_FLAGS               },
        { "POS",          SCRIPT_SCOPE_POSITION            },
        { "MOUSE",        SCRIPT_SCOPE_MOUSE               },
        { "AMOUSE",       SCRIPT_SCOPE_ALTERNATE_MOUSE     },
        { "DEST",         SCRIPT_SCOPE_DESTINATION         },
        { "SOUR",         SCRIPT_SCOPE_SOURCE              },
        { "ZONE",         SCRIPT_SCOPE_ZONE                },
        { "COMM",         SCRIPT_SCOPE_COMMAND             },
        { "PCOMM",        SCRIPT_SCOPE_PARENT_COMMAND      },
        { "OWNER",        SCRIPT_SCOPE_OWNER               },
        { "C",            SCRIPT_SCOPE_CONTAINER_CONDITION },
        { "RATIO",        SCRIPT_SCOPE_RATIO               },
        { "RAD",          SCRIPT_SCOPE_RADIUS              },
        { "LINE",         SCRIPT_SCOPE_LINE                },
        { "TIME",         SCRIPT_SCOPE_TIME                },
        { "PATH",         SCRIPT_SCOPE_PATH                },
        { "IMAGE",        SCRIPT_SCOPE_IMAGE               },
        { "LOOP",         SCRIPT_SCOPE_LOOP                },
        { "R",            SCRIPT_SCOPE_RANDOM              },
        { "P",            SCRIPT_SCOPE_PRIORITY            },
        { "KEYUP",        SCRIPT_SCOPE_KEY_UP              },
        { "TRANSPARENT",  SCRIPT_SCOPE_TRANSPARENT         },
        { "TEXT",         SCRIPT_SCOPE_TEXT                },
        { "FONT",         SCRIPT_SCOPE_FONT                },
        { "V",            SCRIPT_SCOPE_VALUE               },
        { "UPDOWN",       SCRIPT_SCOPE_UP_DOWN             },
        { "NOMATCHES",    SCRIPT_SCOPE_NO_MATCHES          },
        { "Z",            SCRIPT_SCOPE_Z                   },
        { "LAYER",        SCRIPT_SCOPE_LAYER               },
        { "INVERT_NOPAL", SCRIPT_SCOPE_INVERT_NO_PALETTE   },
    };
    for(const Mapping &mapping : mappings)
        if(strings_equal(name, mapping.name))
            return mapping.code;
    return 0;
}

uint32_t parse_script_opcode(ScriptParserState *parser)
{
    if(parser == nullptr)
        return SCRIPT_PARSE_END;
    char name[32];
    if(extract_script_scope_name(parser, name) == SCRIPT_PARSE_END)
        return SCRIPT_PARSE_END;

    struct Mapping
    {
        const char *name;
        uint32_t code;
    };
    static constexpr Mapping mappings[]{
        { "PLOAD",           0x40000000 },
        { "PRELOAD",         0x50000000 },
        { "GPLOAD",          0x00007000 },
        { "CHLOAD",          0x00008000 },
        { "LOADNOFADE",      0x40000001 },
        { "RELOADNOFADE",    0x50000001 },
        { "PEXIT",           0x70000000 },
        { "CONTINUE",        0xb0000000 },
        { "SPLOAD",          0x60000000 },
        { "SET",             0x80000000 },
        { "PLAY",            0x90000000 },
        { "WAIT",            0xa0000000 },
        { "LSCROLL",         0xc0000000 },
        { "RSCROLL",         0xd0000000 },
        { "MOVZ",            0xe0000000 },
        { "WAITB",           0xf0000000 },
        { "LCYCLE",          0x00010000 },
        { "INCLUDE",         0x00020000 },
        { "EXCLUDE",         0x00030000 },
        { "SWRAND",          0x00040000 },
        { "SWLOCK",          0x00050000 },
        { "BREAK",           0x00060000 },
        { "RAND",            0x00070000 },
        { "COND",            0x00080000 },
        { "MOVI",            0x00090000 },
        { "LABEL",           0x000a0000 },
        { "GOTO",            0x000b0000 },
        { "STOP",            0x000c0000 },
        { "QUIT",            0x000d0000 },
        { "FCHANGE",         0x000e0000 },
        { "SLEEP",           0x000f0000 },
        { "DISABLE",         0x00001000 },
        { "ENABLE",          0x00000001 },
        { "ADD",             0x00003000 },
        { "SWVALUE",         0x00004000 },
        { "VALUE",           0x00005000 },
        { "CSEND",           0x00006000 },
        { "COPY",            0x00009000 },
        { "DRAW_BEGIN",      0x0000a000 },
        { "DRAW_END",        0x0000b000 },
        { "GAME",            0x0000c000 },
        { "GEXIT",           0x0000d000 },
        { "HIDE_MOUSE",      0x0000e000 },
        { "SHOW_MOUSE",      0x0000f000 },
        { "COMMENT",         0x00000100 },
        { "INPSTR",          0x00000200 },
        { "MESSAGE",         0x00000300 },
        { "CLS",             0x00000400 },
        { "EMPTY",           0x00000500 },
        { "FADE",            0x00000600 },
        { "CATCH",           0x00000700 },
        { "DISABLE_GLOBALS", 0x00000800 },
    };
    for(const Mapping &mapping : mappings)
        if(strings_equal(name, mapping.name))
            return mapping.code;
    return 0;
}

bool fixed_dword_memory_equal(const void *left, const void *right, uint32_t byte_count)
{
    const uint32_t *left_dwords = static_cast<const uint32_t *>(left);
    const uint32_t *right_dwords = static_cast<const uint32_t *>(right);
    uint32_t count = byte_count >> 2;
    bool equal = count == 0;
    while(count != 0)
    {
        --count;
        equal = true;
        if(*left_dwords != *right_dwords)
            return false;
        ++left_dwords;
        ++right_dwords;
    }
    return equal;
}

void copy_file_name_from_path(char *destination, const char *source)
{
    int index = 0;
    while(source[index] != '\0')
        ++index;
    while(index >= 0 && source[index] != '\\')
        --index;
    int destination_index = 0;
    do
    {
        ++index;
        destination[destination_index] = source[index];
        ++destination_index;
    } while(source[index] != '\0');
}

void copy_runtime_tree_command_name(char *destination, uint32_t command)
{
    const char *source = nullptr;
    if(command == 0x40000000)
        source = "PLOAD";
    else if(command == 0x40000001)
        source = "LOADNOFADE";
    else if(command == 0x50000000)
        source = "PRELOAD";
    else if(command == 0x50000001)
        source = "RELOADNOFADE";
    if(source == nullptr)
    {
        destination[0] = '\0';
        return;
    }
    copy_string(destination, source);
}

ScriptObjectState *create_script_object_state(const void *name)
{
    auto *object = static_cast<ScriptObjectState *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(ScriptObjectState)));
    if(object != nullptr)
    {
        std::memcpy(object->name, name, sizeof(object->name));
        object->identity = object;
    }
    return object;
}

uint32_t parse_script_object_state(ScriptParserState *parser)
{
    char name[0x80];
    if(parse_script_value_token(parser, name, 0x20) == SCRIPT_PARSE_END)
        return 0;
    ScriptObjectState *previous = nullptr;
    ScriptObjectState *object = script_runtime_root->objects;
    while(object != nullptr && !fixed_dword_memory_equal(name, object->name, 0x20))
    {
        previous = object;
        object = object->next;
    }
    if(object == nullptr)
    {
        object = create_script_object_state(name);
        if(object == nullptr)
            return 0;
        if(previous == nullptr)
            script_runtime_root->objects = object;
        else
            previous->next = object;
    }
    object->image_flags |= script_runtime_root->palette_flags;

    bool invert_no_palette = false;
    while(true)
    {
        uint32_t code = parse_script_value_token(parser, name, 0x20);
        if(code != SCRIPT_PARSE_END)
        {
            if(object->field_count < 0x20)
            {
                char string_value[0x80];
                uint32_t value = static_cast<uint32_t>(parse_script_integer_expression(parser));
                if(value == SCRIPT_INTEGER_INVALID)
                {
                    const uint32_t cursor = parser->cursor;
                    value = parse_image_flag(parser);
                    if(value == 0)
                    {
                        parser->cursor = cursor;
                        if(parse_script_value_token(parser, string_value, 0x20) == SCRIPT_PARSE_END)
                            continue;
                        value = SCRIPT_INTEGER_INVALID;
                    }
                }
                uint32_t index = 0;
                while(index < object->field_count && !fixed_dword_memory_equal(name, object->field_names[index], 0x20))
                    ++index;
                const uint32_t bit = 1u << (index & 0x1f);
                if(value == SCRIPT_BOOLEAN_TRUE)
                {
                    object->active_field_mask |= bit;
                }
                else if(value == SCRIPT_BOOLEAN_FALSE)
                {
                    object->active_field_mask &= ~bit;
                }
                else if(value == SCRIPT_INTEGER_INVALID)
                {
                    std::memcpy(object->string_values[index], string_value, 0x20);
                    object->active_field_mask |= bit;
                }
                else
                {
                    object->integer_values[index] = static_cast<int32_t>(value);
                    if(static_cast<int32_t>(value) < 1)
                        object->active_field_mask &= ~bit;
                    else
                        object->active_field_mask |= bit;
                }
                if(index == object->field_count)
                {
                    std::memcpy(object->field_names[index], name, 0x20);
                    ++object->field_count;
                }
            }
            continue;
        }

        code = parse_script_scope_code(parser);
        if(code == 0x00060000)
        {
            invert_no_palette = true;
        }
        else if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 0x10000)
                object->mouse_flags |= SCRIPT_IMAGE_NATURAL_MOUSE;
            else
                object->image_flags |= code;
        }
        else if(code == 0x0c000000)
        {
            if(parse_script_value_token(parser, name, 0x20) != SCRIPT_PARSE_END)
            {
                for(uint32_t index = 0; index < script_runtime_root->command_definition_count; ++index)
                {
                    if(fixed_dword_memory_equal(name, script_runtime_root->command_definitions[index].name, 0x20))
                    {
                        object->command_mask |= 1u << (index & 0x1f);
                        break;
                    }
                }
            }
        }
        else if(code == 0x0d000000)
        {
            parse_script_value_token(parser, object->mouse_visual_name, 0x20);
        }
        else if(code == 0x20000000)
        {
            parse_script_value_token(parser, object->alternate_mouse_visual_name, 0x20);
        }
        if(code == SCRIPT_PARSE_END)
        {
            if((object->mouse_flags & SCRIPT_IMAGE_NATURAL_MOUSE) == 0)
            {
                if(invert_no_palette)
                    object->image_flags ^= SCRIPT_IMAGE_NO_PALETTE;
            }
            else
            {
                object->visual_object = find_runtime_visual_object(object->mouse_visual_name);
                object->alternate_visual_object = find_runtime_visual_object(object->alternate_mouse_visual_name);
            }
            return object->field_count;
        }
    }
}



ScriptObjectState *find_script_object_by_identity(void *identity)
{
    for(ScriptObjectState *object = script_runtime_root->objects; object != nullptr; object = object->next)
        if(object->identity == identity)
            return object;
    for(ScriptObjectContainer *container = script_runtime_root->containers; container != nullptr; container = container->next)
    {
        for(uint32_t index = 0; index < container->slot_count; ++index)
        {
            ScriptObjectState *object = container->slots[index].object;
            if(object != nullptr && object->identity == identity)
                return object;
        }
    }
    return nullptr;
}

int32_t query_or_create_script_object_field(const char *object_name, const void *field_name, uint32_t *value, int32_t value_type)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object == nullptr)
    {
        *value = 0;
        return SCRIPT_INTEGER_INVALID;
    }
    for(uint32_t index = 0; index < object->field_count; ++index)
    {
        if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
        {
            uint32_t bit = 1u << index;
            *value = bit;
            return (object->active_field_mask & bit) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
        }
    }
    uint32_t index = object->field_count;
    if(index >= 32)
    {
        *value = 0;
        return SCRIPT_INTEGER_INVALID;
    }
    uint32_t bit = 1u << index;
    if(value_type == SCRIPT_VALUE_TYPE_BOOLEAN)
    {
        if(*value == SCRIPT_BOOLEAN_TRUE)
            object->active_field_mask |= bit;
        else if(*value == SCRIPT_BOOLEAN_FALSE)
            object->active_field_mask &= ~bit;
    }
    else if(value_type == SCRIPT_VALUE_TYPE_INTEGER)
    {
        object->integer_values[index] = static_cast<int32_t>(*value);
        if(static_cast<int32_t>(*value) < 1)
            object->active_field_mask &= ~bit;
        else
            object->active_field_mask |= bit;
    }
    else if(value_type == SCRIPT_VALUE_TYPE_STRING)
    {
        std::memcpy(object->string_values[index], value, 0x20);
        if(*reinterpret_cast<const char *>(value) == '\0')
            object->active_field_mask &= ~bit;
        else
            object->active_field_mask |= bit;
    }
    else
    {
        *value = 0;
        return SCRIPT_INTEGER_INVALID;
    }
    *value = bit;
    std::memcpy(object->field_names[index], field_name, 0x20);
    ++object->field_count;
    return (object->active_field_mask & bit) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
}

int32_t get_script_object_integer(const char *object_name, const void *field_name)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object != nullptr)
    {
        for(uint32_t index = 0; index < object->field_count; ++index)
            if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
                return object->integer_values[index];
    }
    return SCRIPT_INTEGER_INVALID;
}

uint32_t get_script_object_string(const char *object_name, const void *field_name, void *destination)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object != nullptr)
    {
        for(uint32_t index = 0; index < object->field_count; ++index)
        {
            if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
            {
                std::memcpy(destination, object->string_values[index], 0x20);
                return 1;
            }
        }
    }
    return 0;
}

int32_t add_script_object_integer(const char *object_name, const void *field_name, int32_t delta)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object != nullptr)
    {
        for(uint32_t index = 0; index < object->field_count; ++index)
        {
            if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
            {
                int32_t value = object->integer_values[index] + delta;
                uint32_t bit = 1u << index;
                if(value < 1)
                    object->active_field_mask &= ~bit;
                else
                    object->active_field_mask |= bit;
                object->integer_values[index] = value;
                return value;
            }
        }
    }
    return SCRIPT_INTEGER_INVALID;
}

bool compare_script_object_field(const char *object_name, const void *field_name, const void *value, int32_t value_type)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object != nullptr)
    {
        for(uint32_t index = 0; index < object->field_count; ++index)
        {
            if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
            {
                if(value_type == SCRIPT_VALUE_TYPE_BOOLEAN)
                {
                    uint32_t field_value = (object->active_field_mask & (1u << index)) != 0 ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
                    return *static_cast<const uint32_t *>(value) == field_value;
                }
                if(value_type == SCRIPT_VALUE_TYPE_INTEGER)
                    return object->integer_values[index] == *static_cast<const int32_t *>(value);
                if(value_type == SCRIPT_VALUE_TYPE_STRING)
                    return strings_equal(static_cast<const char *>(value), object->string_values[index]);
            }
        }
    }
    return true;
}

bool has_script_object_field(const char *object_name, const void *field_name)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object != nullptr)
    {
        for(uint32_t index = 0; index < object->field_count; ++index)
            if(fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
                return true;
    }
    return false;
}



RuntimeFixedNameListNode *find_runtime_fixed_name_list_node(const void *name)
{
    if(script_runtime_root == nullptr)
        return nullptr;
    for(RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes; node != nullptr; node = node->next)
        if(fixed_dword_memory_equal(name, node->name, 0x20))
            return node;
    return nullptr;
}

uint32_t parse_script_file_value(ScriptParserState *parser, char *value, char *serialized_value)
{
    uint32_t result = parse_script_value_token(parser, value, 0x20);
    if(result == SCRIPT_PARSE_END)
        return result;

    const char *language_suffix = script_runtime_root->language;
    if(*language_suffix != '\0')
    {
        bool has_extension = false;
        for(uint32_t index = 0; value[index] != '\0'; ++index)
        {
            if(value[index] == '.')
            {
                has_extension = true;
                break;
            }
        }
        if(!has_extension)
            append_string(value, language_suffix);
    }

    if(serialized_value != nullptr)
    {
        append_string(serialized_value, value);
        const uint32_t saved_cursor = parser->cursor;
        char next_value[0x20];
        if(parse_script_value_token(parser, next_value, sizeof(next_value)) != SCRIPT_PARSE_END)
        {
            append_string(serialized_value, ":");
            append_string(serialized_value, next_value);
        }
        parser->cursor = saved_cursor;
        append_string(serialized_value, " ");
    }

    result = static_cast<uint32_t>(parse_script_integer_expression(parser));
    const uint32_t required_value = script_runtime_root->resource_variant;
    if(result != SCRIPT_INTEGER_INVALID && result != required_value)
    {
        value[0] = '\0';
        return SCRIPT_PARSE_END;
    }
    return result;
}

uint32_t create_or_update_runtime_fixed_name_node(ScriptParserState *parser)
{
    char name[0x20];
    if(parse_script_value_token(parser, name, sizeof(name)) == 0)
        return 0;

    RuntimeFixedNameListNode *previous = nullptr;
    RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes;
    while(node != nullptr && !fixed_dword_memory_equal(name, node->name, sizeof(name)))
    {
        previous = node;
        node = node->next;
    }

    const bool created = node == nullptr;
    if(created)
    {
        node = static_cast<RuntimeFixedNameListNode *>(allocate_runtime_heap(script_runtime_root->heap, runtime_heap_zero_memory, sizeof(RuntimeFixedNameListNode)));
        if(node == nullptr)
            return 0;
        node->identity = node;
        node->resource_flags = SCRIPT_IMAGE_NO_PALETTE;
        std::memcpy(node->name, name, sizeof(name));
    }

    for(;;)
    {
        uint32_t code = parse_script_scope_code(parser);
        if(code == 0x01000000)
        {
            node->previous_resource_identity = node->resource_identity;
            node->resource_identity = nullptr;
            parse_script_file_value(parser, node->serialized_value, nullptr);
        }
        else if(code == 0x0a000000)
        {
            code = parse_image_flag(parser);
            if(code == 1)
            {
                node->resource_flags &= ~SCRIPT_IMAGE_NO_PALETTE;
            }
            else if(code == 0x04000000)
            {
                node->resource_flags |= SCRIPT_IMAGE_NO_PALETTE;
                continue;
            }
            node->flags |= code;
        }

        if(code == SCRIPT_PARSE_END)
        {
            if(created)
            {
                if(previous == nullptr)
                    script_runtime_root->fixed_name_nodes = node;
                else
                    previous->next = node;
            }
            return 1;
        }
    }
}

void destroy_runtime_fixed_name_list_nodes()
{
    if(script_runtime_root != nullptr)
    {
        RuntimeFixedNameListNode *node = script_runtime_root->fixed_name_nodes;
        while(node != nullptr)
        {
            RuntimeFixedNameListNode *next = node->next;
            free_runtime_heap(script_runtime_root->heap, 0, node);
            node = next;
        }
        script_runtime_root->fixed_name_nodes = nullptr;
    }
}

void destroy_script_object_states()
{
    if(script_runtime_root != nullptr)
    {
        ScriptObjectState *object = script_runtime_root->objects;
        while(object != nullptr)
        {
            ScriptObjectState *next = object->next;
            free_runtime_heap(script_runtime_root->heap, 0, object);
            object = next;
        }
        script_runtime_root->objects = nullptr;
    }
}

bool remove_runtime_visual_object(void *identity)
{
    RuntimeVisualObject *previous = nullptr;
    RuntimeVisualObject *object = script_runtime_root->visual_objects;
    while(object != nullptr && object->identity != identity)
    {
        previous = object;
        object = object->next;
    }
    if(object == nullptr)
        return false;
    if(previous == nullptr)
        script_runtime_root->visual_objects = object->next;
    else
        previous->next = object->next;
    return free_runtime_heap(script_runtime_root->heap, 0, object);
}

void destroy_runtime_visual_objects()
{
    RuntimeVisualObject *object = script_runtime_root->visual_objects;
    while(object != nullptr)
    {
        RuntimeVisualObject *next = object->next;
        free_runtime_heap(script_runtime_root->heap, 0, object);
        object = next;
    }
    script_runtime_root->visual_objects = nullptr;
}

ScriptObjectState *find_script_object_by_name(const char *name)
{
    for(ScriptObjectState *object = script_runtime_root->objects; object != nullptr; object = object->next)
        if(fixed_dword_memory_equal(name, object->name, 0x20))
            return object;
    for(ScriptObjectContainer *container = script_runtime_root->containers; container != nullptr; container = container->next)
    {
        for(uint32_t index = 0; index < container->slot_count; ++index)
        {
            ScriptObjectState *object = container->slots[index].object;
            if(object != nullptr && fixed_dword_memory_equal(name, object->name, 0x20))
                return object;
        }
    }
    return nullptr;
}

ScriptObjectState *resolve_state_field_reference(const char *object_name, const char *field_name, const void *value, ScriptValueType value_type)
{
    ScriptObjectState *object = find_script_object_by_name(object_name);
    if(object == nullptr)
        return nullptr;
    uint32_t index = 0;
    while(index < object->field_count && !fixed_dword_memory_equal(field_name, object->field_names[index], 0x20))
        ++index;
    if(index >= 32)
        return object;
    uint32_t bit = 1u << index;
    if(value_type == SCRIPT_VALUE_TYPE_BOOLEAN)
    {
        uint32_t boolean_value = *static_cast<const uint32_t *>(value);
        if(boolean_value == SCRIPT_BOOLEAN_TRUE)
            object->active_field_mask |= bit;
        else if(boolean_value == SCRIPT_BOOLEAN_FALSE)
            object->active_field_mask &= ~bit;
    }
    else if(value_type == SCRIPT_VALUE_TYPE_INTEGER)
    {
        object->integer_values[index] = *static_cast<const int32_t *>(value);
        if(object->integer_values[index] < 1)
            object->active_field_mask &= ~bit;
        else
            object->active_field_mask |= bit;
    }
    else if(value_type == SCRIPT_VALUE_TYPE_STRING)
    {
        std::memcpy(object->string_values[index], value, 0x20);
        if(*static_cast<const char *>(value) == 0)
            object->active_field_mask &= ~bit;
        else
            object->active_field_mask |= bit;
    }
    if(object->field_count == index)
    {
        std::memcpy(object->field_names[index], field_name, 0x20);
        ++object->field_count;
    }
    return object;
}



} // namespace freegag
