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
#include "portable_string.h"
#include "resource.h"
#include "runtime.h"
#include "runtime_model.h"
#include "runtime_tree.h"
#include "script.h"
#include "text.h"

namespace gag
{
namespace
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
constexpr uint32_t save_name_text_color = 51;
constexpr uint32_t save_name_frame_color = 1;

constexpr char save_load_script[] = R"([CFG]
fademask=0;
flags=NOSAVE;
mouse=CM /FILE:K_Ukaz.bmp /F:NOPAL;
mouse=NM /FILE:K_None.bmp /F:NOPAL;
mouse=EXM /FILE:K_None.bmp /F:NOPAL;
command=Comment /MOUSE:CM;
command=Go;
object=MM EXIT::OFF SEL::0;
object=SL INIT::OFF EDITING::OFF EMPTY::OFF CLOSE::OFF;
object=INPUT NAME::X;

[LOAD]
flags=NOSAVE NOCOMMENT;
sublocation=COMMON;
zone=z_PREVIEW /POS::294,120,320,240 /COMM:Comment /MOUSE:CM /P:100;
sublocation=TAG_LOAD;
image=HW_1 /FILE::FGSL0000.bmp /F:PRIMARY;
image=i_dSAVE /FILE::Fscr0014.bmp /POS::18,146 /F:SEPARATED /F:NOPAL;
event=e_LOAD /ZONE::z_LOAD /COMM:Comment /MESSAGE::2102;
event=e_PREVIEW_CLEAR /ZONE::z_PREVIEW /TRANSPARENT /SWVALUE::MM::SEL /VALUE::0 /GOTO::e_PREVIEW_CLEAR /BREAK /CSEND /CLS::BACKGND::0,50,280,220 /CLS::BACKGND::211,410,429,70 /LABEL::e_PREVIEW_CLEAR /SET::MM::SEL::0;
event=e_PREVIEW /ZONE::z_PREVIEW /COMM:Comment /MESSAGE::2102;
event=e_EXIT_LOAD /ZONE::z_EXIT /COMM:Comment /PEXIT:NOFADE;

[SAVE]
flags=NOSAVE NOCOMMENT;
sublocation=COMMON;
sublocation=TAG_SAVE;
local=l_INIT SL::INIT::OFF;
local=l_SAVE_NOT_EDITING SL::EDITING::OFF;
image=HW_1 /FILE::FGSL0000.bmp /F:PRIMARY;
image=i_dLOAD /FILE::Fscr0015.bmp /POS::7,171 /F:SEPARATED /F:NOPAL;
zone=z_NAME /POS::302,366,304,28 /C:l_SAVE_NOT_EDITING /COMM:Comment /MOUSE:CM /P:100;
event=e_SAVE /ZONE::z_SAVE /COMM:Comment /MESSAGE::2102 /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;
event=e_NAME /ZONE::z_NAME /COMM:Comment /SET::SL:EDITING:ON /MESSAGE::2103 /INPSTR:302:366:SaveCaption:51:1:INPUT:NAME:16 /MESSAGE::2104 /SET::SL:EDITING:OFF /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;
event=e_INIT /C:l_INIT /SET::SL:INIT:ON /MESSAGE::2105 /SWVALUE::SL::EMPTY /VALUE::ON /GOTO::e_INIT_INPUT /BREAK /CSEND /GOTO::e_INIT_DONE /LABEL::e_INIT_INPUT /SET::SL:EDITING:ON /MESSAGE::2103 /INPSTR:302:366:SaveCaption:51:1:INPUT:NAME:16 /MESSAGE::2104 /SET::SL:EDITING:OFF /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND /LABEL::e_INIT_DONE;
event=e_EXIT_SAVE /ZONE::z_EXIT /COMM:Comment /MESSAGE::2106 /SWVALUE::SL::CLOSE /VALUE::ON /PEXIT:NOFADE /BREAK /CSEND;

[COMMON]
zone=z_MAIN /RECT::0,0,640,480 /MOUSE:NM /COMM:Go;
sublocation=TAG_NEXT;
sublocation=TAG_BACK;
sublocation=TAG_EXIT;
image=i_dNEW /FILE::Fscr0011.bmp /POS::98,41 /F:SEPARATED /F:NOPAL;
image=i_dEXIT /FILE::Fscr0012.bmp /POS::89,70 /F:SEPARATED /F:NOPAL;
image=i_dCONT /FILE::Fscr0013.bmp /POS::63,100 /F:SEPARATED /F:NOPAL;
image=i_dHELP /FILE::Fscr0016.bmp /POS::59,196 /F:SEPARATED /F:NOPAL;
image=i_dCRED /FILE::Fscr0017.bmp /POS::57,244 /F:SEPARATED /F:NOPAL;
font=SaveCaption /FILE:Font2.rus;
layer=SavePreview /POS:294,120,320,240 /Z:458752;
layer=SaveCaption /POS:302,366,304,28 /Z:458753;
event=e_BACK /ZONE::z_BACK /COMM:Comment /MESSAGE::2100;
event=e_NEXT /ZONE::z_NEXT /COMM:Comment /MESSAGE::2101;

[TAG_NEXT]
template=MENU_3S( z_NEXT 536 420 100 60 Comment CM i_sNEXT Helpnext.bmp 536 420 l_ISSet l_ISnSet i_dNEXT Helpnext.bmp 536 420 l_ISDis l_ISnDis MM EXIT EXIT SEL 100 e_SEL100 211 410 429 70 z_MAIN);

[TAG_LOAD]
template=MENU_3S( z_LOAD 7 171 266 25 Comment CM i_sLOAD Fscr0005.bmp 7 171 l_ISLoadSet l_ISnLoadSet i_dLOAD Fscr0005.bmp 7 171 l_ISLoadDis l_ISnLoadDis MM EXIT EXIT SEL 400 e_SEL400 0 50 280 220 z_MAIN);

[TAG_SAVE]
template=MENU_3S( z_SAVE 18 146 246 25 Comment CM i_sSAVE Fscr0004.bmp 18 146 l_ISSaveSet l_ISnSaveSet i_dSAVE Fscr0004.bmp 18 146 l_ISSaveDis l_ISnSaveDis MM EXIT EXIT SEL 500 e_SEL500 0 50 280 220 z_MAIN);

[TAG_BACK]
template=MENU_3S( z_BACK 297 414 100 60 Comment CM i_sBACK Helpback.bmp 297 414 l_ISBSet l_ISnBSet i_dBACK Helpback.bmp 297 414 l_ISBDis l_ISnBDis MM EXIT EXIT SEL 200 e_SEL200 211 410 429 70 z_MAIN);

[TAG_EXIT]
template=MENU_3S( z_EXIT 211 416 100 60 Comment CM i_sEXIT Helpexit.bmp 211 416 l_ISESet l_ISnESet i_dEXIT Helpexit.bmp 211 416 l_ISEDis l_ISnEDis MM EXIT EXIT SEL 300 e_SEL300 211 410 429 70);

[MENU_3S]
class=TEMPLATE;
params=p_Zn p_ZpX p_ZpY p_ZpW p_ZpH p_ZComm p_ZMouse  p_ISn p_ISFn p_ISIpX p_ISIpY p_LSssN p_LSdsN  p_IDn p_IDFn p_IDIpX p_IDIpY p_LDssN p_LDdsN  p_On p_OSel p_ODis p_OCurrIndx  p_nOurIndx  p_En  p_clsX p_clsY p_clsW p_clsH p_clsZ;
local=PARAM:p_LSssN  PARAM:p_On::PARAM:p_OSel::ON  PARAM:p_On::PARAM:p_ODis::OFF;
local=PARAM:p_LSdsN  PARAM:p_On::PARAM:p_OSel::OFF  PARAM:p_On::PARAM:p_ODis::OFF;
local=PARAM:p_LDssN  PARAM:p_On::PARAM:p_ODis::ON  PARAM:p_On::PARAM:p_OSel::OFF;
local=PARAM:p_LDdsN  PARAM:p_On::PARAM:p_ODis::OFF;
zone=PARAM:p_Zn  /C::PARAM:p_LDdsN /POS::PARAM:p_ZpX,PARAM:p_ZpY,PARAM:p_ZpW,PARAM:p_ZpH  /COMM::PARAM:p_ZComm  /MOUSE::PARAM:p_ZMouse /P:100;
zone=PARAM:p_Zn  /C::PARAM:p_LDssN /POS::PARAM:p_ZpX,PARAM:p_ZpY,PARAM:p_ZpW,PARAM:p_ZpH  /COMM::PARAM:p_ZComm  /MOUSE::PARAM:p_ZMouse;
image=PARAM:p_ISn  /C::PARAM:p_LSssN  /FILE::PARAM:p_ISFn  /POS::PARAM:p_ISIpX,PARAM:p_ISIpY;
image=PARAM:p_ISn  /C::PARAM:p_LSdsN  /FILE::PARAM:p_ISFn  /POS::PARAM:p_ISIpX,PARAM:p_ISIpY  /F:STOPPED;
image=PARAM:p_IDn  /C::PARAM:p_LDssN  /FILE::PARAM:p_IDFn  /POS::PARAM:p_IDIpX,PARAM:p_IDIpY  /F:SEPARATED /F:NOPAL;
event=PARAM:p_En /ZONE::PARAM:p_Zn /C::PARAM:p_LDdsN /TRANSPARENT /SWVALUE::PARAM:p_On::PARAM:p_OCurrIndx /VALUE::PARAM:p_nOurIndx /GOTO::PARAM:p_En /BREAK /CSEND /DRAW_BEGIN /CLS::BACKGND::0,50,280,220 /CLS::BACKGND::211,410,429,70 /PLAY::PARAM:p_ISn::RESTART /DRAW_END /LABEL::PARAM:p_En /SET::PARAM:p_On::PARAM:p_OCurrIndx::PARAM:p_nOurIndx;
event=PARAM:p_clsZ /ZONE::PARAM:p_clsZ /SWVALUE::PARAM:p_On::PARAM:p_OCurrIndx /VALUE::0 /GOTO::PARAM:p_clsZ /BREAK /CSEND /CLS::BACKGND::0,50,280,220 /CLS::BACKGND::211,410,429,70 /LABEL::PARAM:p_clsZ /SET::PARAM:p_On::PARAM:p_OCurrIndx::0;

[END]
)";

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

void *capture_save_state(void *game_context, uint32_t *size, int mode)
{
    return capture_save_game_bitmap(game_context, size, mode);
}

uintptr_t get_save_script_state()
{
    return reinterpret_cast<uintptr_t>(serialize_current_runtime_state());
}

bool write_save_state(char *path, char *name, void *bitmap, uintptr_t script_state)
{
    return write_synchronized_cdf_package(path, name, bitmap, reinterpret_cast<void *>(script_state));
}

bool save_file_exists(const char *path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error);
}

ScriptedSaveLoadPersistenceApi persistence_api{ capture_save_state, get_save_script_state, free_heap_memory, write_save_state, save_file_exists };
ScriptedSaveLoadController controller{};

bool archive_entry_less(const ArchiveCommentEntry &left, const ArchiveCommentEntry &right)
{
    return compare_ascii_case_insensitive(left.file_name, right.file_name) < 0;
}

RuntimeTreeSceneLink *find_tree_scene_link(RuntimeTreeNode *tree, const char *name)
{
    if(tree == nullptr)
    {
        return nullptr;
    }
    for(RuntimeTreeSceneLink *link = tree->scene_link_head; link != nullptr; link = link->next)
    {
        if(compare_ascii_case_insensitive(link->name, name) == 0)
        {
            return link;
        }
        if(link == tree->scene_link_tail)
        {
            break;
        }
    }
    for(RuntimeTreeNode *child = tree->child; child != nullptr; child = child->next)
    {
        RuntimeTreeSceneLink *link = find_tree_scene_link(child, name);
        if(link != nullptr)
        {
            return link;
        }
    }
    return nullptr;
}

void activate_scripted_layer(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
    {
        return;
    }
    activate_display_scene_node(link->scene_identifier);
}

void prepare_caption_layer(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
    {
        return;
    }
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(link->scene_identifier));
    const PaletteEntry *palette = get_display_palette_entries();
    if(palette != nullptr)
    {
        configure_display_scene_palette(scene, reinterpret_cast<const uint32_t *>(palette), 256);
    }
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
    {
        return false;
    }
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
    {
        return false;
    }

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
        {
            mapping[index] = 0xff000000u | static_cast<uint32_t>(preview.palette[index].rgbRed) << 16 | static_cast<uint32_t>(preview.palette[index].rgbGreen) << 8 | preview.palette[index].rgbBlue;
        }
        auto *destination = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(scene->callback_first_position));
        for(uint32_t y = 0; y < preview_height; ++y)
        {
            const uint8_t *source = preview.pixels + (preview_height - y - 1) * preview_width;
            auto *row = reinterpret_cast<uint32_t *>(destination + y * scene->sync_secondary_position);
            for(uint32_t x = 0; x < preview_width; ++x)
            {
                row[x] = mapping[source[x]];
            }
        }
        return true;
    }
    return false;
}

bool decode_preview(const uint8_t *data, uint32_t size, DisplaySceneNode *scene)
{
    DecodedPreview preview{};
    return validate_preview(data, size, &preview) && copy_preview_pixels(preview, scene);
}

void clear_scene(RuntimeTreeSceneLink *link)
{
    if(link == nullptr || link->scene_identifier == 0)
    {
        return;
    }
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(link->scene_identifier));
    const uint32_t begin_result = begin_display_scene_update(link->scene_identifier);
    if(begin_result != 0)
    {
        return;
    }
    std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(scene->callback_first_position)), 0, static_cast<size_t>(scene->sync_secondary_position) * static_cast<size_t>(scene->height));
    DisplayRectangle rectangle{ 0, 0, scene->width, scene->height };
    DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(scene->width), static_cast<int16_t>(scene->height), 1, 0, scene->callback_first_position };
    const DisplayRectangleTransform transform{ descriptor.x, descriptor.y, static_cast<uint16_t>(descriptor.width), static_cast<uint16_t>(descriptor.height) };
    end_display_scene_update(link->scene_identifier, &transform, &rectangle);
}

void render_preview_data(const uint8_t *data, uint32_t size)
{
    if(controller.preview_layer == nullptr || controller.preview_layer->scene_identifier == 0)
    {
        return;
    }
    auto *scene = reinterpret_cast<DisplaySceneNode *>(static_cast<uintptr_t>(controller.preview_layer->scene_identifier));
    DecodedPreview preview{};
    const bool valid = validate_preview(data, size, &preview);
    if(!valid || scene->width != static_cast<int32_t>(preview_width) || scene->height != static_cast<int32_t>(preview_height))
    {
        return;
    }
    if(scene->rectangle_callback_format.bits_per_pixel == 8)
    {
        uint32_t palette[256];
        prepare_preview_palette(preview, palette);
        const bool palette_result = configure_display_scene_palette(scene, palette, 256);
        if(!palette_result)
        {
            return;
        }
    }
    const uint32_t begin_result = begin_display_scene_update(controller.preview_layer->scene_identifier);
    if(begin_result != 0)
    {
        return;
    }
    const bool copied = copy_preview_pixels(preview, scene);
    if(copied)
    {
        DisplayRectangle rectangle{ 0, 0, scene->width, scene->height };
        DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(scene->width), static_cast<int16_t>(scene->height), 1, 0, scene->callback_first_position };
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
    {
        return;
    }
    if(controller.mode == SaveLoadScreenMode::save)
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
    {
        return;
    }
    const ArchiveCommentEnumerationApi &api = get_archive_comment_enumeration_api();
    CdfArchive *archive = api.open_archive(controller.saves.entries[controller.selection].path, 0);
    if(archive == nullptr)
    {
        return;
    }
    const uint32_t size = api.get_entry_size(archive, 0, "COMMENT.BMP");
    constexpr uint32_t minimum_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + 256 * sizeof(BitmapColor) + preview_width * preview_height;
    if(size < minimum_size || size > 16 * 1024 * 1024)
    {
        api.close_archive(archive);
        return;
    }
    std::vector<uint8_t> data(size);
    const bool read = api.read_entry(archive, 0, "COMMENT.BMP", data.data()) != 0;
    api.close_archive(archive);
    if(read)
    {
        render_preview_data(data.data(), size);
    }
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
    {
        return;
    }
    const char *source =
        controller.mode == SaveLoadScreenMode::save ? controller.current_name : (controller.selection < controller.saves.count ? controller.saves.entries[controller.selection].comment : "");
    if(source[0] == '\0')
    {
        return;
    }
    char font_name[0x20]{};
    copy_string(font_name, "SaveCaption");
    RuntimeFixedNameListNode *font_node = find_runtime_fixed_name_list_node(font_name);
    RuntimeLockRecord *font_record = font_node == nullptr ? nullptr : acquire_runtime_lock_record(font_node->resource_identity);
    if(font_record == nullptr)
    {
        return;
    }
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
        DisplaySceneDescriptor descriptor{ 0, 0, static_cast<int16_t>(caption_width), static_cast<int16_t>(caption_height), 1, 0, scene->callback_first_position };
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
    const uint32_t value = enabled ? 0x03000000 : 0x07000000;
    resolve_state_field_reference(object_name, field_name, &value, 1);
}

void update_current_name_from_selection()
{
    controller.current_name[0] = '\0';
    if(controller.selection < controller.saves.count)
    {
        copy_string(controller.current_name, controller.saves.entries[controller.selection].comment);
    }
}

void reset_controller()
{
    destroy_archive_comment_collection(&controller.saves);
    if(controller.owns_snapshot && controller.snapshot != nullptr)
    {
        persistence_api.free_memory(controller.snapshot);
    }
    controller = {};
}

bool change_selection(int32_t delta)
{
    if(controller.saves.count == 0)
    {
        return false;
    }
    const int32_t count = static_cast<int32_t>(controller.saves.count);
    controller.selection = static_cast<uint32_t>((static_cast<int32_t>(controller.selection) + delta + count) % count);
    update_current_name_from_selection();
    if(controller.mode == SaveLoadScreenMode::load)
    {
        render_selection();
    }
    else
    {
        render_caption();
    }
    return true;
}

bool parse_automatic_save_number(const char *name, uint64_t *number)
{
    if(name == nullptr || number == nullptr || compare_ascii_case_insensitive(name, "save", 4) != 0 || name[4] == '\0')
    {
        return false;
    }
    uint64_t value = 0;
    for(size_t index = 4; name[index] != '\0'; ++index)
    {
        if(name[index] < '0' || name[index] > '9')
        {
            return false;
        }
        const uint32_t digit = static_cast<uint32_t>(name[index] - '0');
        if(value > (std::numeric_limits<uint64_t>::max() - digit) / 10)
        {
            return false;
        }
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
        {
            maximum = std::max(maximum, value);
        }
    }
    if(maximum == std::numeric_limits<uint64_t>::max())
    {
        return false;
    }
    const int length = std::snprintf(name, 0x104, "save%llu", static_cast<unsigned long long>(maximum + 1));
    return length > 0 && length < 0x104;
}

const ArchiveCommentEntry *find_save_by_name(const char *name)
{
    for(uint32_t index = controller.saves.count; index != 0; --index)
    {
        if(compare_ascii_case_insensitive(controller.saves.entries[index - 1].comment, name) == 0)
        {
            return &controller.saves.entries[index - 1];
        }
    }
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
        {
            return false;
        }
        if(!persistence_api.file_exists(path))
        {
            return true;
        }
        if(identifier == runtime_infinite_wait)
        {
            return false;
        }
        ++identifier;
    }
}

bool save_current_state(ApplicationState *state, const char *requested_name)
{
    if(state == nullptr || requested_name == nullptr)
    {
        return false;
    }
    char name[0x104]{};
    copy_string(name, requested_name);
    if(name[0] == '\0' && !prepare_automatic_save_name(name))
    {
        return false;
    }

    char path[0x104]{};
    const ArchiveCommentEntry *matching = find_save_by_name(name);
    if(matching != nullptr)
    {
        copy_string(path, matching->path);
    }
    else if(!prepare_new_save_path(path))
    {
        return false;
    }

    copy_string(controller.current_name, name);
    if(!persistence_api.write_state(path, controller.current_name, controller.snapshot, controller.script_state))
    {
        render_caption();
        return false;
    }
    state->flags = (state->flags & 0xffefffff) | 0x40000;
    set_script_boolean("CLOSE", true);
    return true;
}

void request_input_completion(ScriptedSaveLoadController::InputCompletion completion)
{
    if(controller.completion_requested)
    {
        return;
    }
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
}

uint32_t enumerate_archive_comment_entries(const char *directory, const char *extension, ArchiveCommentCollection *collection)
{
    if(directory == nullptr || extension == nullptr || collection == nullptr)
    {
        return 0x10000;
    }
    *collection = {};
    const ArchiveCommentEnumerationApi &api = get_archive_comment_enumeration_api();

    std::error_code error;
    std::vector<std::filesystem::path> files;
    const std::filesystem::path enumeration_directory = *directory == '\0' ? std::filesystem::path(".") : std::filesystem::path(directory);
    for(std::filesystem::directory_iterator entry(enumeration_directory, error), end; !error && entry != end; entry.increment(error))
    {
        if(!entry->is_regular_file(error))
        {
            error.clear();
            continue;
        }
        const std::string entry_extension = entry->path().extension().string();
        if(compare_ascii_case_insensitive(entry_extension.c_str(), extension) == 0)
        {
            files.push_back(entry->path());
        }
    }
    if(files.empty())
    {
        return 2;
    }
    std::sort(files.begin(), files.end(),
        [](const std::filesystem::path &left, const std::filesystem::path &right)
        {
            const std::string left_name = left.filename().string();
            const std::string right_name = right.filename().string();
            return compare_ascii_case_insensitive(left_name.c_str(), right_name.c_str()) < 0;
        });

    collection->entries = new (std::nothrow) ArchiveCommentEntry[10];
    if(collection->entries == nullptr)
    {
        return 0x10000;
    }
    collection->capacity = 10;

    for(const std::filesystem::path &file : files)
    {
        const std::string file_name = file.filename().string();
        if(compare_ascii_case_insensitive(file_name.c_str(), "AutoSave.cdf") == 0)
        {
            continue;
        }
        const std::string path = file.string();
        CdfArchive *archive = api.open_archive(path.c_str(), 0);
        if(archive == nullptr)
        {
            if(api.get_error(nullptr) == 0x10000)
            {
                destroy_archive_comment_collection(collection);
                std::filesystem::remove(file, error);
                return 0x10000;
            }
            continue;
        }

        char comment[0x104]{};
        const uint32_t comment_size = api.get_entry_size(archive, 0, "COMMENT.TXT");
        if(comment_size < sizeof(comment) && api.read_entry(archive, 0, "COMMENT.TXT", comment) != 0)
        {
            comment[comment_size] = '\0';
            if(collection->count == collection->capacity)
            {
                auto *grown = new (std::nothrow) ArchiveCommentEntry[collection->capacity + 10];
                if(grown == nullptr)
                {
                    api.close_archive(archive);
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
            {
                collection->next_identifier = identifier_limit;
            }
            else if(collection->count > collection->next_identifier)
            {
                collection->next_identifier = collection->count;
            }
        }
        api.close_archive(archive);
    }

    if(collection->count == 0)
    {
        destroy_archive_comment_collection(collection);
        return 2;
    }
    std::sort(collection->entries, collection->entries + collection->count, archive_entry_less);
    return 0;
}

void destroy_archive_comment_collection(ArchiveCommentCollection *collection)
{
    if(collection == nullptr)
    {
        return;
    }
    if(collection->entries != nullptr)
    {
        delete[] collection->entries;
    }
    *collection = {};
}

bool find_save_load_virtual_script(const char *name, VirtualScriptResource *resource)
{
    if(name != nullptr && resource != nullptr && compare_ascii_case_insensitive(name, "SAVELOAD.CFG") == 0)
    {
        resource->data = save_load_script;
        resource->size = static_cast<uint32_t>(sizeof(save_load_script) - 1);
        resource->resource_type = 4;
        return true;
    }
    return false;
}

const char *save_load_screen_section(SaveLoadScreenMode mode)
{
    return mode == SaveLoadScreenMode::save ? "SAVE" : "LOAD";
}

bool request_scripted_save_load_screen(SaveLoadScreenMode mode, ApplicationState *state)
{
    reset_controller();
    controller.mode = mode;
    controller.pending = true;
    controller.input_completion = ScriptedSaveLoadController::InputCompletion::submit;
    if(state != nullptr)
    {
        copy_string(controller.directory, state->installation_path);
    }
    const uint32_t enumeration_result = enumerate_archive_comment_entries(controller.directory, ".GSF", &controller.saves);
    if(mode == SaveLoadScreenMode::load && enumeration_result != 0)
    {
        controller.pending = false;
        return false;
    }
    if(mode == SaveLoadScreenMode::save && enumeration_result != 0 && enumeration_result != 2)
    {
        controller.pending = false;
        return false;
    }
    if(controller.saves.count != 0)
    {
        controller.selection = controller.saves.count - 1;
        update_current_name_from_selection();
    }
    if(mode == SaveLoadScreenMode::save)
    {
        if(state == nullptr)
        {
            reset_controller();
            return false;
        }
        if((state->flags & 0x80000) != 0)
        {
            controller.script_state = state->script_state;
            if(state->saved_memory != nullptr)
            {
                controller.snapshot = state->saved_memory;
            }
            else
            {
                controller.snapshot = persistence_api.capture_state(state->game_context, &controller.snapshot_size, 1);
                controller.owns_snapshot = controller.snapshot != nullptr;
            }
        }
        else
        {
            controller.snapshot = persistence_api.capture_state(state->game_context, &controller.snapshot_size, 1);
            controller.owns_snapshot = controller.snapshot != nullptr;
            controller.script_state = persistence_api.get_script_state();
        }
    }
    set_runtime_paths_once("SAVELOAD.CFG", save_load_screen_section(mode));
    return true;
}

bool handle_scripted_save_load_message(uintptr_t message, ApplicationState *state)
{
    if(message == previous_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::save && controller.editing)
        {
            if(controller.saves.count > 1)
            {
                request_input_completion(ScriptedSaveLoadController::InputCompletion::previous);
            }
        }
        else
        {
            change_selection(-1);
        }
        return true;
    }
    if(message == next_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::save && controller.editing)
        {
            if(controller.saves.count > 1)
            {
                request_input_completion(ScriptedSaveLoadController::InputCompletion::next);
            }
        }
        else
        {
            change_selection(1);
        }
        return true;
    }
    if(message == load_save_message)
    {
        if(controller.mode == SaveLoadScreenMode::load)
        {
            if(state != nullptr && controller.selection < controller.saves.count)
            {
                finish_application_state_load(state, controller.saves.entries[controller.selection].path);
            }
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
        RuntimeInputSessionRecord completed_input{};
        copy_runtime_input_session_record(&completed_input);
        char input[0x20]{};
        std::memcpy(input, &completed_input, sizeof(input));
        input[sizeof(input) - 1] = '\0';
        finish_save_name_input(input, state);
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
        {
            request_input_completion(ScriptedSaveLoadController::InputCompletion::exit);
        }
        else if(controller.mode == SaveLoadScreenMode::save)
        {
            set_script_boolean("CLOSE", true);
        }
        return true;
    }
    return false;
}

void on_scripted_save_load_tree_rebuilt(RuntimeTreeNode *tree)
{
    if(!controller.pending)
    {
        return;
    }
    if(tree == nullptr || compare_ascii_case_insensitive(tree->name, save_load_screen_section(controller.mode)) != 0)
    {
        return;
    }
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
    {
        reset_controller();
    }
}

} // namespace gag
