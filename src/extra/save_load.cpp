#include "save_load.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <new>
#include <vector>
#include "application.h"
#include "cdf_archive.h"
#include "display_host.h"
#include "display_scene.h"
#include "media.h"
#include "portable_path.h"
#include "portable_string.h"
#include "resource.h"
#include "runtime.h"
#include "runtime_model.h"
#include "runtime_tree.h"
#include "script.h"
#include "text.h"

namespace freegag
{
constexpr uintptr_t previous_save_message = 2100;
constexpr uintptr_t next_save_message = 2101;
constexpr uintptr_t load_save_message = 2102;
constexpr uintptr_t begin_save_name_input_message = 2103;
constexpr uintptr_t finish_save_name_input_message = 2104;
constexpr uintptr_t initialize_save_screen_message = 2105;
constexpr uintptr_t exit_save_screen_message = 2106;
constexpr uint32_t preview_width = 320;
constexpr uint32_t preview_height = 240;
constexpr uint32_t caption_width = 304;
constexpr uint32_t caption_height = 28;
constexpr uint32_t max_save_name_characters = 15;
constexpr uint32_t save_name_text_color = 208;
constexpr uint32_t save_name_frame_color = 96;

struct ScriptedSaveLoadController
{
    enum class InputCompletion
    {
        submit,
        previous,
        next,
        exit
    };

    SaveLoadScreenMode mode;
    bool pending;
    RuntimeTreeNode *tree;
    RuntimeTreeSceneLink *preview_layer;
    RuntimeTreeSceneLink *caption_layer;
    ArchiveCommentCollection saves;
    uint32_t selection;
    char directory[0x104];
    char current_name[0x104];
    bool editing;
    bool completion_requested;
    InputCompletion input_completion;
    void *snapshot;
    uint32_t snapshot_size;
    bool owns_snapshot;
    uintptr_t script_state;
};

bool save_file_exists(const char *path)
{
    std::filesystem::path resolved_path;
    if(!resolve_existing_host_path_case_insensitive(path, &resolved_path))
        return false;
    std::error_code error;
    return std::filesystem::is_regular_file(resolved_path, error);
}

ScriptedSaveLoadController controller{};

RuntimeTreeSceneLink *find_tree_scene_link(RuntimeTreeNode *tree, const char *name)
{
    if(tree == nullptr)
        return nullptr;
    for(RuntimeTreeSceneLink *link = tree->scene_link_head; link != nullptr; link = link->next)
    {
        if(compare_ascii_case_insensitive(link->name, name) == 0)
            return link;
        if(link == tree->scene_link_tail)
            break;
    }
    for(RuntimeTreeNode *child = tree->child; child != nullptr; child = child->next)
    {
        RuntimeTreeSceneLink *link = find_tree_scene_link(child, name);
        if(link != nullptr)
            return link;
    }
    return nullptr;
}

void activate_scripted_layer(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
        return;
    activate_display_scene_node(link->scene_identifier);
}

void prepare_caption_layer(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
        return;
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(link->scene_identifier));
    const PaletteEntry *palette = get_display_palette_entries();
    if(palette != nullptr)
        configure_display_scene_palette(scene, reinterpret_cast<const uint32_t *>(palette), 256);
    activate_scripted_layer(link);
}

struct DecodedPreview
{
    const BitmapColor *palette;
    const uint8_t *pixels;
};

bool validate_preview(const uint8_t *data, uint32_t size, DecodedPreview *preview)
{
    if(data == nullptr || preview == nullptr || size < sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader))
        return false;
    const BitmapFileHeader file_header = decode_bitmap_file_header(data);
    const BitmapInfoHeader info_header = decode_bitmap_info_header(data + sizeof(BitmapFileHeader));
    constexpr uint32_t palette_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    constexpr uint32_t palette_bytes = 256 * sizeof(BitmapColor);
    constexpr uint32_t pixel_bytes = preview_width * preview_height;
    if(file_header.bfType != 0x4d42 || info_header.biSize != sizeof(BitmapInfoHeader) || info_header.biWidth != static_cast<int32_t>(preview_width)
        || info_header.biHeight != static_cast<int32_t>(preview_height) || info_header.biPlanes != 1 || info_header.biBitCount != 8 || info_header.biCompression != 0
        || file_header.bfOffBits < palette_offset + palette_bytes || file_header.bfOffBits > size || pixel_bytes > size - file_header.bfOffBits
        || (file_header.bfSize != 0 && (file_header.bfSize > size || file_header.bfSize < file_header.bfOffBits + pixel_bytes)))
    {
        return false;
    }

    preview->palette = reinterpret_cast<const BitmapColor *>(data + palette_offset);
    preview->pixels = data + file_header.bfOffBits;
    return true;
}

void prepare_preview_palette(const DecodedPreview &preview, uint32_t *palette)
{
    for(uint32_t index = 0; index < 256; ++index)
    {
        palette[index] =
            static_cast<uint32_t>(preview.palette[index].rgbRed) | (static_cast<uint32_t>(preview.palette[index].rgbGreen) << 8) | (static_cast<uint32_t>(preview.palette[index].rgbBlue) << 16);
    }
}

bool copy_preview_pixels(const DecodedPreview &preview, DisplaySceneNode *scene)
{
    if(scene == nullptr || scene->width != static_cast<int32_t>(preview_width) || scene->height != static_cast<int32_t>(preview_height))
        return false;

    const uint32_t destination_bits = scene->rectangle_callback_format.bits_per_pixel;
    if(destination_bits == 8)
    {
        auto *destination = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position));
        for(uint32_t y = 0; y < preview_height; ++y)
        {
            const uint8_t *source = preview.pixels + (preview_height - y - 1) * preview_width;
            uint8_t *row = destination + y * scene->sync_secondary_position;
            std::memcpy(row, source, preview_width);
        }
        return true;
    }
    if(destination_bits == 32)
    {
        uint32_t mapping[256];
        for(uint32_t index = 0; index < 256; ++index)
            mapping[index] = 0xff000000u | static_cast<uint32_t>(preview.palette[index].rgbRed) << 16 | static_cast<uint32_t>(preview.palette[index].rgbGreen) << 8 | preview.palette[index].rgbBlue;
        auto *destination = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position));
        for(uint32_t y = 0; y < preview_height; ++y)
        {
            const uint8_t *source = preview.pixels + (preview_height - y - 1) * preview_width;
            auto *row = reinterpret_cast<uint32_t *>(destination + y * scene->sync_secondary_position);
            for(uint32_t x = 0; x < preview_width; ++x)
                row[x] = mapping[source[x]];
        }
        return true;
    }
    return false;
}

void clear_scene(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
        return;
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(link->scene_identifier));
    const uint32_t begin_result = begin_display_scene_update(link->scene_identifier);
    if(begin_result != 0)
        return;
    std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(scene->callback_first_position)), 0, static_cast<size_t>(scene->sync_secondary_position) * static_cast<size_t>(scene->height));
    DisplayRectangle rectangle{ 0, 0, scene->width, scene->height };
    DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(scene->width), static_cast<int16_t>(scene->height), scene->callback_first_position };
    const DisplayRectangleTransform transform{ descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
    end_display_scene_update(link->scene_identifier, &transform, &rectangle);
}

void render_preview_data(const uint8_t *data, uint32_t size)
{
    if(controller.preview_layer == nullptr || controller.preview_layer->scene_identifier == 0)
        return;
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(controller.preview_layer->scene_identifier));
    DecodedPreview preview{};
    const bool valid = validate_preview(data, size, &preview);
    if(!valid || scene->width != static_cast<int32_t>(preview_width) || scene->height != static_cast<int32_t>(preview_height))
        return;
    if(scene->rectangle_callback_format.bits_per_pixel == 8)
    {
        uint32_t palette[256];
        prepare_preview_palette(preview, palette);
        const bool palette_result = configure_display_scene_palette(scene, palette, 256);
        if(!palette_result)
            return;
    }
    const uint32_t begin_result = begin_display_scene_update(controller.preview_layer->scene_identifier);
    if(begin_result != 0)
        return;
    const bool copied = copy_preview_pixels(preview, scene);
    if(copied)
    {
        DisplayRectangle rectangle{ 0, 0, scene->width, scene->height };
        DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(scene->width), static_cast<int16_t>(scene->height), scene->callback_first_position };
        const DisplayRectangleTransform transform{ descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
        end_display_scene_update(controller.preview_layer->scene_identifier, &transform, &rectangle);
    }
    else
    {
        end_display_scene_update(controller.preview_layer->scene_identifier, nullptr, nullptr);
    }
}

void render_preview()
{
    clear_scene(controller.preview_layer);
    if(controller.preview_layer == nullptr || controller.preview_layer->scene_identifier == 0)
        return;
    if(controller.mode == SaveLoadScreenMode::SAVE)
    {
        uint32_t size = controller.snapshot_size;
        if(controller.snapshot != nullptr && size == 0)
        {
            BitmapFileHeader header;
            std::memcpy(&header, controller.snapshot, sizeof(header));
            size = header.bfSize;
        }
        render_preview_data(static_cast<const uint8_t *>(controller.snapshot), size);
        return;
    }
    if(controller.selection >= controller.saves.count)
        return;
    CdfArchive *archive = open_cdf_archive(controller.saves.entries[controller.selection].path, 0);
    if(archive == nullptr)
        return;
    const uint32_t size = get_cdf_entry_size(archive, 0, "COMMENT.BMP");
    constexpr uint32_t minimum_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + 256 * sizeof(BitmapColor) + preview_width * preview_height;
    if(size < minimum_size || size > 16 * 1024 * 1024)
    {
        close_cdf_archive(archive);
        return;
    }
    std::vector<uint8_t> data(size);
    const bool read = read_cdf_entry(archive, 0, "COMMENT.BMP", data.data()) != 0;
    close_cdf_archive(archive);
    if(read)
        render_preview_data(data.data(), size);
}

void prepare_display_name(const char *source, char *display)
{
    const size_t length = std::strlen(source);
    const size_t displayed_length = std::min(length, static_cast<size_t>(max_save_name_characters));
    std::memcpy(display, source, displayed_length);
    size_t display_length = displayed_length;
    if(length > max_save_name_characters)
    {
        std::memcpy(display + display_length, "...", 3);
        display_length += 3;
    }
    display[display_length] = '\0';
}

bool prepare_caption_text(const char *source, void *font_identity, char *caption, RuntimeStandaloneTextState *text_state)
{
    prepare_display_name(source, caption);
    return caption[0] != '\0' && initialize_runtime_standalone_text(caption, 0, 0, font_identity, save_name_text_color, save_name_frame_color, text_state) != 0;
}

void render_caption()
{
    clear_scene(controller.caption_layer);
    if(controller.caption_layer == nullptr || controller.caption_layer->scene_identifier == 0 || controller.editing)
        return;
    const char *source =
        controller.mode == SaveLoadScreenMode::SAVE ? controller.current_name : (controller.selection < controller.saves.count ? controller.saves.entries[controller.selection].comment : "");
    if(source[0] == '\0')
        return;
    char font_name[0x20]{};
    copy_string(font_name, "SaveCaption");
    RuntimeFixedNameListNode *font_node = find_runtime_fixed_name_list_node(font_name);
    RuntimeLockRecord *font_record = font_node == nullptr ? nullptr : acquire_runtime_lock_record(font_node->resource_identity);
    if(font_record == nullptr)
        return;
    void *font_identity = font_record->identity_context;
    char caption[0x104]{};
    RuntimeStandaloneTextState text_state{};
    if(!prepare_caption_text(source, font_identity, caption, &text_state))
    {
        release_runtime_lock_record(font_record);
        return;
    }
    if(begin_display_scene_update(controller.caption_layer->scene_identifier) == 0)
    {
        auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(controller.caption_layer->scene_identifier));
        DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(caption_width), static_cast<int16_t>(caption_height), scene->callback_first_position };
        draw_runtime_standalone_text(&text_state, &descriptor);
        DisplayRectangle rectangle{ 0, 0, scene->width, scene->height };
        const DisplayRectangleTransform transform{ descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
        end_display_scene_update(controller.caption_layer->scene_identifier, &transform, &rectangle);
    }
    release_runtime_lock_record(font_record);
}

void render_selection()
{
    render_preview();
    render_caption();
}

void set_script_boolean(const char *field, bool enabled)
{
    char object_name[0x20]{};
    char field_name[0x20]{};
    copy_string(object_name, "SL");
    copy_string(field_name, field);
    const uint32_t script_boolean_value = enabled ? SCRIPT_BOOLEAN_TRUE : SCRIPT_BOOLEAN_FALSE;
    resolve_state_field_reference(object_name, field_name, &script_boolean_value, SCRIPT_VALUE_TYPE_BOOLEAN);
}

void update_current_name_from_selection()
{
    controller.current_name[0] = '\0';
    if(controller.selection < controller.saves.count)
        copy_string(controller.current_name, controller.saves.entries[controller.selection].comment);
}

void reset_controller()
{
    destroy_archive_comment_collection(&controller.saves);
    if(controller.owns_snapshot && controller.snapshot != nullptr)
        free_heap_memory(controller.snapshot);
    controller = {};
}

bool change_selection(int32_t delta)
{
    if(controller.saves.count == 0)
        return false;
    const int32_t count = static_cast<int32_t>(controller.saves.count);
    controller.selection = static_cast<uint32_t>((static_cast<int32_t>(controller.selection) + delta + count) % count);
    update_current_name_from_selection();
    if(controller.mode == SaveLoadScreenMode::LOAD)
        render_selection();
    else
        render_caption();
    return true;
}

bool parse_automatic_save_number(const char *name, uint64_t *number)
{
    if(name == nullptr || number == nullptr || compare_ascii_case_insensitive(name, "save", 4) != 0 || name[4] == '\0')
        return false;
    uint64_t value = 0;
    for(size_t index = 4; name[index] != '\0'; ++index)
    {
        if(name[index] < '0' || name[index] > '9')
            return false;
        const uint32_t digit = static_cast<uint32_t>(name[index] - '0');
        if(value > (std::numeric_limits<uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    *number = value;
    return true;
}

bool prepare_automatic_save_name(char *name)
{
    uint64_t maximum = 0;
    for(uint32_t index = 0; index < controller.saves.count; ++index)
    {
        uint64_t value;
        if(parse_automatic_save_number(controller.saves.entries[index].comment, &value))
            maximum = std::max(maximum, value);
    }
    if(maximum == std::numeric_limits<uint64_t>::max())
        return false;
    const int length = std::snprintf(name, 0x104, "save%llu", static_cast<unsigned long long>(maximum + 1));
    return length > 0 && length < 0x104;
}

const ArchiveCommentEntry *find_save_by_name(const char *name)
{
    for(uint32_t index = controller.saves.count; index != 0; --index)
        if(compare_ascii_case_insensitive(controller.saves.entries[index - 1].comment, name) == 0)
            return &controller.saves.entries[index - 1];
    return nullptr;
}

bool prepare_new_save_path(char *path)
{
    uint32_t identifier = controller.saves.next_identifier;
    for(;;)
    {
        char file_name[0x20];
        const int file_length = std::snprintf(file_name, sizeof(file_name), "GAG%03u.GSF", identifier);
        const int path_length = std::snprintf(path, 0x104, "%s%s", controller.directory, file_name);
        if(file_length <= 0 || file_length >= static_cast<int>(sizeof(file_name)) || path_length <= 0 || path_length >= 0x104)
            return false;
        if(!save_file_exists(path))
            return true;
        if(identifier == runtime_infinite_wait)
            return false;
        ++identifier;
    }
}

bool save_current_state(ApplicationState *state, const char *requested_name)
{
    if(state == nullptr || requested_name == nullptr)
        return false;
    char name[0x104]{};
    copy_string(name, requested_name);
    if(name[0] == '\0' && !prepare_automatic_save_name(name))
        return false;

    char path[0x104]{};
    const ArchiveCommentEntry *matching = find_save_by_name(name);
    if(matching != nullptr)
        copy_string(path, matching->path);
    else if(!prepare_new_save_path(path))
        return false;

    copy_string(controller.current_name, name);
    if(!write_synchronized_cdf_package(path, controller.current_name, controller.snapshot, reinterpret_cast<void *>(controller.script_state)))
    {
        render_caption();
        return false;
    }
    state->flags = (state->flags & ~APPLICATION_LOAD_DISABLED) | APPLICATION_PREFERENCES_CHANGED;
    set_script_boolean("CLOSE", true);
    return true;
}

void request_input_completion(ScriptedSaveLoadController::InputCompletion completion)
{
    if(controller.completion_requested)
        return;
    controller.completion_requested = true;
    controller.input_completion = completion;
    enqueue_runtime_byte('\r');
}

void finish_save_name_input(const char *input, ApplicationState *state)
{
    controller.editing = false;
    controller.completion_requested = false;
    switch(controller.input_completion)
    {
    case ScriptedSaveLoadController::InputCompletion::previous:
        change_selection(-1);
        break;
    case ScriptedSaveLoadController::InputCompletion::next:
        change_selection(1);
        break;
    case ScriptedSaveLoadController::InputCompletion::exit:
        set_script_boolean("CLOSE", true);
        break;
    case ScriptedSaveLoadController::InputCompletion::submit:
        copy_string(controller.current_name, input == nullptr ? "" : input);
        save_current_state(state, controller.current_name);
        break;
    }
    controller.input_completion = ScriptedSaveLoadController::InputCompletion::submit;
}

uint32_t enumerate_archive_comment_entries(const char *directory, const char *extension, ArchiveCommentCollection *collection)
{
    if(directory == nullptr || extension == nullptr || collection == nullptr)
        return ARCHIVE_COMMENT_ENUMERATION_FAILED;
    *collection = {};
    std::error_code error;
    std::vector<std::filesystem::path> files;
    const std::filesystem::path requested_directory = *directory == '\0' ? std::filesystem::path(".") : std::filesystem::path(directory);
    std::filesystem::path enumeration_directory;
    if(!resolve_existing_host_path_case_insensitive(requested_directory, &enumeration_directory))
        return ARCHIVE_COMMENT_ENUMERATION_FAILED;
    for(std::filesystem::directory_iterator entry(enumeration_directory, error), end; !error && entry != end; entry.increment(error))
    {
        if(!entry->is_regular_file(error))
        {
            error.clear();
            continue;
        }
        const std::string entry_extension = entry->path().extension().string();
        if(compare_ascii_case_insensitive(entry_extension.c_str(), extension) == 0)
            files.push_back(entry->path());
    }
    if(files.empty())
        return ARCHIVE_COMMENT_ENUMERATION_EMPTY;
    std::sort(files.begin(), files.end(),
        [](const std::filesystem::path &left, const std::filesystem::path &right)
        {
            const std::string left_name = left.filename().string();
            const std::string right_name = right.filename().string();
            return compare_ascii_case_insensitive(left_name.c_str(), right_name.c_str()) < 0;
        });

    collection->entries = new (std::nothrow) ArchiveCommentEntry[10];
    if(collection->entries == nullptr)
        return ARCHIVE_COMMENT_ENUMERATION_FAILED;
    collection->capacity = 10;

    for(const std::filesystem::path &file : files)
    {
        const std::string file_name = file.filename().string();
        if(compare_ascii_case_insensitive(file_name.c_str(), "AutoSave.cdf") == 0)
            continue;
        const std::string path = file.string();
        CdfArchive *archive = open_cdf_archive(path.c_str(), 0);
        if(archive == nullptr)
        {
            if(get_cdf_error(nullptr) == CDF_ERROR_STORAGE_FAILURE)
            {
                destroy_archive_comment_collection(collection);
                std::filesystem::remove(file, error);
                return ARCHIVE_COMMENT_ENUMERATION_FAILED;
            }
            continue;
        }

        char comment[0x104]{};
        const uint32_t comment_size = get_cdf_entry_size(archive, 0, "COMMENT.TXT");
        if(comment_size < sizeof(comment) && read_cdf_entry(archive, 0, "COMMENT.TXT", comment) != 0)
        {
            comment[comment_size] = '\0';
            if(collection->count == collection->capacity)
            {
                auto *grown = new (std::nothrow) ArchiveCommentEntry[collection->capacity + 10];
                if(grown == nullptr)
                {
                    close_cdf_archive(archive);
                    break;
                }
                std::copy_n(collection->entries, collection->count, grown);
                delete[] collection->entries;
                collection->entries = grown;
                collection->capacity += 10;
            }
            ArchiveCommentEntry &entry = collection->entries[collection->count++];
            std::memset(&entry, 0, sizeof(entry));
            copy_string(entry.path, path.c_str());
            copy_string(entry.comment, comment);
            copy_string(entry.file_name, file_name.c_str());
            entry.numeric_identifier = parse_path_numeric_identifier(file_name.c_str());
            const uint32_t identifier_limit = static_cast<uint32_t>(entry.numeric_identifier + 1);
            if(static_cast<int32_t>(collection->next_identifier) < static_cast<int32_t>(identifier_limit))
                collection->next_identifier = identifier_limit;
            else if(collection->count > collection->next_identifier)
                collection->next_identifier = collection->count;
        }
        close_cdf_archive(archive);
    }

    if(collection->count == 0)
    {
        destroy_archive_comment_collection(collection);
        return ARCHIVE_COMMENT_ENUMERATION_EMPTY;
    }
    std::sort(collection->entries, collection->entries + collection->count,
        [](const ArchiveCommentEntry &left, const ArchiveCommentEntry &right) { return compare_ascii_case_insensitive(left.file_name, right.file_name) < 0; });
    return ARCHIVE_COMMENT_ENUMERATION_SUCCESS;
}

void destroy_archive_comment_collection(ArchiveCommentCollection *collection)
{
    if(collection == nullptr)
        return;
    if(collection->entries != nullptr)
        delete[] collection->entries;
    *collection = {};
}

const char *save_load_screen_section(SaveLoadScreenMode mode)
{
    return mode == SaveLoadScreenMode::SAVE ? "SAVE" : "LOAD";
}

bool request_scripted_save_load_screen(SaveLoadScreenMode mode, ApplicationState *state)
{
    reset_controller();
    controller.mode = mode;
    controller.pending = true;
    controller.input_completion = ScriptedSaveLoadController::InputCompletion::submit;
    if(state != nullptr)
        copy_string(controller.directory, state->installation_path);
    const uint32_t enumeration_result = enumerate_archive_comment_entries(controller.directory, ".GSF", &controller.saves);
    if(mode == SaveLoadScreenMode::LOAD && enumeration_result != ARCHIVE_COMMENT_ENUMERATION_SUCCESS)
    {
        controller.pending = false;
        return false;
    }
    if(mode == SaveLoadScreenMode::SAVE && enumeration_result != ARCHIVE_COMMENT_ENUMERATION_SUCCESS && enumeration_result != ARCHIVE_COMMENT_ENUMERATION_EMPTY)
    {
        controller.pending = false;
        return false;
    }
    if(controller.saves.count != 0)
    {
        controller.selection = controller.saves.count - 1;
        update_current_name_from_selection();
    }
    if(mode == SaveLoadScreenMode::SAVE)
    {
        if(state == nullptr)
        {
            reset_controller();
            return false;
        }
        if((state->flags & APPLICATION_SNAPSHOT_ACTIVE) != 0)
        {
            controller.script_state = state->script_state;
            if(state->saved_memory != nullptr)
            {
                controller.snapshot = state->saved_memory;
            }
            else
            {
                controller.snapshot = capture_save_game_bitmap(&controller.snapshot_size, 1);
                controller.owns_snapshot = controller.snapshot != nullptr;
            }
        }
        else
        {
            controller.snapshot = capture_save_game_bitmap(&controller.snapshot_size, 1);
            controller.owns_snapshot = controller.snapshot != nullptr;
            controller.script_state = reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
        }
    }
    set_runtime_paths_once("SAVELOAD.CFG", save_load_screen_section(mode));
    return true;
}

bool handle_scripted_save_load_message(uintptr_t message, ApplicationState *state)
{
    if(message == previous_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::SAVE && controller.editing)
        {
            if(controller.saves.count > 1)
                request_input_completion(ScriptedSaveLoadController::InputCompletion::previous);
        }
        else
        {
            change_selection(-1);
        }
        return true;
    }
    if(message == next_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::SAVE && controller.editing)
        {
            if(controller.saves.count > 1)
                request_input_completion(ScriptedSaveLoadController::InputCompletion::next);
        }
        else
        {
            change_selection(1);
        }
        return true;
    }
    if(message == load_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::LOAD)
        {
            if(state != nullptr && controller.selection < controller.saves.count)
                finish_application_state_load(state, controller.saves.entries[controller.selection].path);
        }
        else if(controller.editing)
        {
            request_input_completion(ScriptedSaveLoadController::InputCompletion::submit);
        }
        else
        {
            save_current_state(state, controller.current_name);
        }
        return true;
    }
    if(message == begin_save_name_input_message)
    {
        set_script_boolean("CLOSE", false);
        controller.editing = true;
        controller.completion_requested = false;
        controller.input_completion = ScriptedSaveLoadController::InputCompletion::submit;
        clear_scene(controller.caption_layer);
        return true;
    }
    if(message == finish_save_name_input_message)
    {
        RuntimeInputText input = take_runtime_input_text();
        input.back() = '\0';
        finish_save_name_input(input.data(), state);
        return true;
    }
    if(message == initialize_save_screen_message)
    {
        set_script_boolean("CLOSE", false);
        set_script_boolean("EMPTY", controller.saves.count == 0);
        render_preview();
        return true;
    }
    if(message == exit_save_screen_message)
    {
        if(controller.editing)
            request_input_completion(ScriptedSaveLoadController::InputCompletion::exit);
        else if(controller.mode == SaveLoadScreenMode::SAVE)
            set_script_boolean("CLOSE", true);
        return true;
    }
    return false;
}

void on_scripted_save_load_tree_rebuilt(RuntimeTreeNode *tree)
{
    if(!controller.pending)
        return;
    if(tree == nullptr || compare_ascii_case_insensitive(tree->name, save_load_screen_section(controller.mode)) != 0)
        return;
    controller.pending = false;
    controller.tree = tree;
    controller.preview_layer = find_tree_scene_link(tree, "SavePreview");
    controller.caption_layer = find_tree_scene_link(tree, "SaveCaption");
    activate_scripted_layer(controller.preview_layer);
    prepare_caption_layer(controller.caption_layer);
    render_selection();
}

void on_scripted_save_load_tree_resources_destroyed(RuntimeTreeNode *tree)
{
    if(tree != nullptr && tree == controller.tree)
        reset_controller();
}

} // namespace freegag
