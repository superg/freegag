#include "indexed_renderer.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include "figurine_geometry.h"

namespace xtet
{
namespace
{

bool framebuffer_valid(IndexedFramebuffer framebuffer)
{
    return framebuffer.pixels && framebuffer.width != 0 && framebuffer.height != 0 && framebuffer.stride >= framebuffer.width;
}

bool find_gameplay_origin(const SceneDescription &scene, int32_t &x, int32_t &y)
{
    const std::vector<const SceneNode *> homes = find_scene_links(scene, "home_scr");
    if(homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position)
        return false;
    x = homes[0]->children[1].position->x;
    y = homes[0]->children[1].position->y;
    return true;
}

bool find_gameplay_viewport(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, FigurineRenderRegion &viewport)
{
    int32_t x = 0;
    int32_t y = 0;
    const auto face = bitmaps.find("f01.bmp");
    if(!find_gameplay_origin(scene, x, y) || face == bitmaps.end())
        return false;
    viewport = { x, y, face->second.width, face->second.height };
    return viewport.width != 0 && viewport.height != 0;
}

bool blit_transparent_clipped(const IndexedBitmap &source, IndexedFramebuffer destination, int32_t x, int32_t y, bool flip_x, bool flip_y, const FigurineRenderRegion &clip)
{
    if(!framebuffer_valid(destination) || source.width == 0 || source.height == 0 || source.pixels.size() != (size_t)source.width * source.height)
        return false;
    const int64_t clipped_left = std::max<int64_t>({ 0, (int64_t)x, (int64_t)clip.x });
    const int64_t clipped_top = std::max<int64_t>({ 0, (int64_t)y, (int64_t)clip.y });
    const int64_t clipped_right = std::min<int64_t>({ (int64_t)destination.width, (int64_t)x + source.width, (int64_t)clip.x + clip.width });
    const int64_t clipped_bottom = std::min<int64_t>({ (int64_t)destination.height, (int64_t)y + source.height, (int64_t)clip.y + clip.height });
    if(clipped_left >= clipped_right || clipped_top >= clipped_bottom)
        return true;
    for(int64_t destination_y = clipped_top; destination_y < clipped_bottom; ++destination_y)
    {
        size_t source_y = (size_t)(destination_y - y);
        if(flip_y)
            source_y = source.height - source_y - 1;
        for(int64_t destination_x = clipped_left; destination_x < clipped_right; ++destination_x)
        {
            size_t source_x = (size_t)(destination_x - x);
            if(flip_x)
                source_x = source.width - source_x - 1;
            const uint8_t pixel = source.pixels[source_y * source.width + source_x];
            if(pixel != 0)
                destination.pixels[(size_t)destination_y * destination.stride + (size_t)destination_x] = pixel;
        }
    }
    return true;
}

bool render_node(const SceneNode &node, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer, int32_t parent_x, int32_t parent_y)
{
    if(node.type == SceneNodeType::empty || node.type == SceneNodeType::bitmap || node.type == SceneNodeType::wave)
        return true;
    if(!node.shown.value_or(false))
        return true;
    const int64_t x_64 = (int64_t)parent_x + (node.position ? node.position->x : 0);
    const int64_t y_64 = (int64_t)parent_y + (node.position ? node.position->y : 0);
    if(x_64 < (std::numeric_limits<int32_t>::min)() || x_64 > (std::numeric_limits<int32_t>::max)() || y_64 < (std::numeric_limits<int32_t>::min)() || y_64 > (std::numeric_limits<int32_t>::max)())
        return false;
    const int32_t x = (int32_t)x_64;
    const int32_t y = (int32_t)y_64;

    if(node.type == SceneNodeType::sprite_bitmap)
    {
        if(node.children.size() != 1 || node.children[0].type != SceneNodeType::bitmap || node.children[0].loaded_path.empty())
            return false;
        const auto bitmap = bitmaps.find(node.children[0].loaded_path);
        if(bitmap == bitmaps.end())
            return false;
        return node.transparent.value_or(false) ? blit_transparent(bitmap->second, framebuffer, x, y) : blit_opaque(bitmap->second, framebuffer, x, y);
    }
    for(const SceneNode &child : node.children)
    {
        if(!render_node(child, bitmaps, framebuffer, x, y))
            return false;
    }
    return true;
}

} // namespace

bool blit_opaque(const IndexedBitmap &source, IndexedFramebuffer destination, int32_t x, int32_t y)
{
    if(!framebuffer_valid(destination) || source.width == 0 || source.height == 0 || source.pixels.size() != (size_t)source.width * source.height)
        return false;
    const int64_t right = (int64_t)x + source.width;
    const int64_t bottom = (int64_t)y + source.height;
    const int64_t clipped_left = std::max<int64_t>(0, x);
    const int64_t clipped_top = std::max<int64_t>(0, y);
    const int64_t clipped_right = std::min<int64_t>(destination.width, right);
    const int64_t clipped_bottom = std::min<int64_t>(destination.height, bottom);
    if(clipped_left >= clipped_right || clipped_top >= clipped_bottom)
        return true;
    const size_t copy_width = (size_t)(clipped_right - clipped_left);
    const size_t source_x = (size_t)(clipped_left - x);
    for(int64_t destination_y = clipped_top; destination_y < clipped_bottom; ++destination_y)
    {
        const size_t source_y = (size_t)(destination_y - y);
        const uint8_t *source_row = source.pixels.data() + source_y * source.width + source_x;
        uint8_t *destination_row = destination.pixels + (size_t)destination_y * destination.stride + (size_t)clipped_left;
        std::copy_n(source_row, copy_width, destination_row);
    }
    return true;
}

bool blit_transparent(const IndexedBitmap &source, IndexedFramebuffer destination, int32_t x, int32_t y, bool flip_x, bool flip_y)
{
    if(!framebuffer_valid(destination) || source.width == 0 || source.height == 0 || source.pixels.size() != (size_t)source.width * source.height)
        return false;
    const int64_t right = (int64_t)x + source.width;
    const int64_t bottom = (int64_t)y + source.height;
    const int64_t clipped_left = std::max<int64_t>(0, x);
    const int64_t clipped_top = std::max<int64_t>(0, y);
    const int64_t clipped_right = std::min<int64_t>(destination.width, right);
    const int64_t clipped_bottom = std::min<int64_t>(destination.height, bottom);
    if(clipped_left >= clipped_right || clipped_top >= clipped_bottom)
        return true;
    for(int64_t destination_y = clipped_top; destination_y < clipped_bottom; ++destination_y)
    {
        size_t source_y = (size_t)(destination_y - y);
        if(flip_y)
            source_y = source.height - source_y - 1;
        for(int64_t destination_x = clipped_left; destination_x < clipped_right; ++destination_x)
        {
            size_t source_x = (size_t)(destination_x - x);
            if(flip_x)
                source_x = source.width - source_x - 1;
            const uint8_t pixel = source.pixels[source_y * source.width + source_x];
            if(pixel != 0)
                destination.pixels[(size_t)destination_y * destination.stride + (size_t)destination_x] = pixel;
        }
    }
    return true;
}

bool blit_rli_frame(const RliFrameRecord &frame, IndexedFramebuffer destination, int32_t x, int32_t y)
{
    if(!framebuffer_valid(destination) || frame.right < frame.left || frame.bottom < frame.top)
        return false;
    const uint32_t width = (uint32_t)(frame.right - frame.left + 1);
    const uint32_t height = (uint32_t)(frame.bottom - frame.top + 1);
    if(frame.pixels.size() != (size_t)width * height || frame.coverage.size() != frame.pixels.size())
        return false;
    const int64_t origin_x = (int64_t)x + frame.left;
    const int64_t origin_y = (int64_t)y + frame.top;
    for(uint32_t source_y = 0; source_y < height; ++source_y)
    {
        const int64_t destination_y = origin_y + source_y;
        if(destination_y < 0 || destination_y >= destination.height)
            continue;
        for(uint32_t source_x = 0; source_x < width; ++source_x)
        {
            const int64_t destination_x = origin_x + source_x;
            const size_t source_index = (size_t)source_y * width + source_x;
            if(destination_x >= 0 && destination_x < destination.width && frame.coverage[source_index] != 0)
                destination.pixels[(size_t)destination_y * destination.stride + (size_t)destination_x] = frame.pixels[source_index];
        }
    }
    return true;
}

bool blit_rli_frame_canvas(const RliFrameRecord &frame, uint32_t canvas_width, uint32_t canvas_height, IndexedFramebuffer destination, int32_t x, int32_t y, bool flip_x, bool flip_y)
{
    const int32_t frame_width = frame.right - frame.left + 1;
    const int32_t frame_height = frame.bottom - frame.top + 1;
    if(!framebuffer_valid(destination) || canvas_width == 0 || canvas_height == 0 || frame.left < 0 || frame.top < 0 || frame.right >= (int32_t)canvas_width || frame.bottom >= (int32_t)canvas_height
        || frame_width <= 0 || frame_height <= 0 || frame.pixels.size() != (size_t)frame_width * frame_height || frame.coverage.size() != frame.pixels.size())
        return false;
    for(int32_t frame_y = 0; frame_y < frame_height; ++frame_y)
        for(int32_t frame_x = 0; frame_x < frame_width; ++frame_x)
        {
            const size_t source_index = (size_t)frame_y * frame_width + frame_x;
            if(frame.coverage[source_index] == 0 || frame.pixels[source_index] == 0)
                continue;
            int32_t canvas_x = frame.left + frame_x;
            int32_t canvas_y = frame.top + frame_y;
            if(flip_x)
                canvas_x = (int32_t)canvas_width - 1 - canvas_x;
            if(flip_y)
                canvas_y = (int32_t)canvas_height - 1 - canvas_y;
            const int32_t destination_x = x + canvas_x;
            const int32_t destination_y = y + canvas_y;
            if(destination_x >= 0 && destination_y >= 0 && destination_x < (int32_t)destination.width && destination_y < (int32_t)destination.height)
                destination.pixels[(size_t)destination_y * destination.stride + destination_x] = frame.pixels[source_index];
        }
    return true;
}

bool render_score(uint32_t score, const IndexedBitmap &digit_atlas, IndexedFramebuffer destination)
{
    if(!framebuffer_valid(destination) || digit_atlas.width < 4 || digit_atlas.height < 10 || digit_atlas.pixels.size() != (size_t)digit_atlas.width * digit_atlas.height)
        return false;
    const uint32_t glyph_width = digit_atlas.width / 4;
    const uint32_t glyph_height = digit_atlas.height / 10;
    if(glyph_width == 0 || glyph_height == 0)
        return false;
    score = (std::min)(score, (uint32_t)9999);
    uint32_t divisor = 1000;
    for(uint32_t index = 0; index < 4; ++index)
    {
        const uint32_t digit = (score / divisor) % 10;
        const int32_t destination_x = 359 + (int32_t)(index * glyph_width);
        const int32_t destination_y = 438;
        for(uint32_t row = 0; row < glyph_height; ++row)
        {
            const int64_t y = (int64_t)destination_y + row;
            if(y < 0 || y >= destination.height)
                continue;
            for(uint32_t column = 0; column < glyph_width; ++column)
            {
                const int64_t x = (int64_t)destination_x + column;
                if(x >= 0 && x < destination.width)
                    destination.pixels[(size_t)y * destination.stride + (size_t)x] = digit_atlas.pixels[(size_t)(digit * glyph_height + row) * digit_atlas.width + column];
            }
        }
        divisor /= 10;
    }
    return true;
}

bool render_initial_scene(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer)
{
    if(!framebuffer_valid(framebuffer))
        return false;
    const std::vector<const SceneNode *> homes = find_scene_links(scene, "home_scr");
    return homes.size() == 1 && render_node(*homes[0], bitmaps, framebuffer, 0, 0);
}

int32_t map_scaled_cursor_coordinate(int32_t value, int32_t client_extent, int32_t framebuffer_extent)
{
    if(value < 0 || value >= client_extent || client_extent <= 0 || framebuffer_extent <= 0 || client_extent == framebuffer_extent)
        return value;
    return (int32_t)((int64_t)value * framebuffer_extent / client_extent);
}

int hit_test_sprite_collection(const SceneNode &collection, const std::map<std::string, IndexedBitmap> &bitmaps, int32_t x, int32_t y)
{
    for(size_t reverse_index = collection.children.size(); reverse_index != 0; --reverse_index)
    {
        const size_t index = reverse_index - 1;
        const SceneNode &sprite = collection.children[index];
        if(sprite.type != SceneNodeType::sprite_bitmap || !sprite.position || sprite.children.size() != 1 || sprite.children[0].type != SceneNodeType::bitmap || sprite.children[0].loaded_path.empty())
            continue;
        const auto bitmap = bitmaps.find(sprite.children[0].loaded_path);
        if(bitmap == bitmaps.end())
            continue;
        const int64_t local_x = (int64_t)x - sprite.position->x;
        const int64_t local_y = (int64_t)y - sprite.position->y;
        if(local_x < 0 || local_y < 0 || local_x >= bitmap->second.width || local_y >= bitmap->second.height)
            continue;
        if(sprite.transparent.value_or(false) && bitmap->second.pixels[(size_t)local_y * bitmap->second.width + (size_t)local_x] == 0)
            continue;
        return (int)index;
    }
    return -1;
}

bool render_figurine_sprite(const FigurineSpriteSelection &selection, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    FigurineRenderRegion &region)
{
    if(!framebuffer_valid(framebuffer))
        return false;
    const char *link = selection.family == FigurineSpriteFamily::man ? "man" : "woman";
    const std::vector<const SceneNode *> collections = find_scene_links(scene, link);
    if(collections.size() != 1 || selection.frame_index >= collections[0]->children.size())
        return false;
    const SceneNode &sprite = collections[0]->children[selection.frame_index];
    if(sprite.type != SceneNodeType::sprite_bitmap || sprite.transparent != true || sprite.children.size() != 1 || sprite.children[0].type != SceneNodeType::bitmap
        || sprite.children[0].loaded_path.empty())
        return false;
    const auto bitmap = bitmaps.find(sprite.children[0].loaded_path);
    int32_t origin_x = 0;
    int32_t origin_y = 0;
    FigurineRenderRegion viewport;
    if(bitmap == bitmaps.end() || !find_gameplay_origin(scene, origin_x, origin_y) || !find_gameplay_viewport(scene, bitmaps, viewport)
        || !blit_transparent_clipped(bitmap->second, framebuffer, selection.x + origin_x, selection.y + origin_y, selection.mirror_horizontal, selection.mirror_vertical, viewport))
        return false;

    const int64_t destination_x = (int64_t)selection.x + origin_x;
    const int64_t destination_y = (int64_t)selection.y + origin_y;
    const int64_t left = std::max<int64_t>({ 0, destination_x, (int64_t)viewport.x });
    const int64_t top = std::max<int64_t>({ 0, destination_y, (int64_t)viewport.y });
    const int64_t right = std::min<int64_t>({ (int64_t)framebuffer.width, destination_x + bitmap->second.width, (int64_t)viewport.x + viewport.width });
    const int64_t bottom = std::min<int64_t>({ (int64_t)framebuffer.height, destination_y + bitmap->second.height, (int64_t)viewport.y + viewport.height });
    region.x = (int32_t)left;
    region.y = (int32_t)top;
    region.width = right > left ? (uint32_t)(right - left) : 0;
    region.height = bottom > top ? (uint32_t)(bottom - top) : 0;
    return true;
}

bool collect_figurine_board_regions(const FallingFigurine &figurine, uint32_t framebuffer_width, uint32_t framebuffer_height, std::vector<FigurineRenderRegion> &regions, int32_t origin_x,
    int32_t origin_y)
{
    regions.clear();
    FigurineTemplate shape;
    if(framebuffer_width == 0 || framebuffer_height == 0 || !get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, shape))
        return false;
    for(int shape_row = 0; shape_row < 5; ++shape_row)
        for(int shape_column = 0; shape_column < 5; ++shape_column)
        {
            if(shape[(size_t)shape_row * 5 + shape_column] == 0)
                continue;
            const int board_row = figurine.row + shape_row - 2;
            const int board_column = figurine.column + shape_column - 2;
            if(board_row < 0 || board_column < 0)
                continue;
            const int64_t cell_left = (int64_t)origin_x + (int64_t)board_column * 17;
            const int64_t cell_top = (int64_t)origin_y + (int64_t)board_row * 17;
            const int64_t left = std::max<int64_t>(0, cell_left);
            const int64_t top = std::max<int64_t>(0, cell_top);
            const int64_t right = std::min<int64_t>(framebuffer_width, cell_left + 17);
            const int64_t bottom = std::min<int64_t>(framebuffer_height, cell_top + 17);
            if(right > left && bottom > top)
                regions.push_back({ (int32_t)left, (int32_t)top, (uint32_t)(right - left), (uint32_t)(bottom - top) });
        }
    return true;
}

bool fill_figurine_board_regions(const FallingFigurine &figurine, IndexedFramebuffer framebuffer, uint8_t fill_index, int32_t origin_x, int32_t origin_y)
{
    if(!framebuffer_valid(framebuffer))
        return false;
    std::vector<FigurineRenderRegion> regions;
    if(!collect_figurine_board_regions(figurine, framebuffer.width, framebuffer.height, regions, origin_x, origin_y))
        return false;
    for(const FigurineRenderRegion &region : regions)
        for(uint32_t row = 0; row < region.height; ++row)
            std::fill_n(framebuffer.pixels + (size_t)(region.y + row) * framebuffer.stride + region.x, region.width, fill_index);
    return true;
}

FigurineBoardChangeCallback make_figurine_board_change_callback(uint32_t framebuffer_width, uint32_t framebuffer_height, const std::function<void(const FigurineRenderRegion &, bool)> &region_callback)
{
    if(!region_callback || framebuffer_width == 0 || framebuffer_height == 0)
        return {};
    return [framebuffer_width, framebuffer_height, region_callback](const FallingFigurine &figurine, bool adding)
    {
        std::vector<FigurineRenderRegion> regions;
        if(!collect_figurine_board_regions(figurine, framebuffer_width, framebuffer_height, regions))
            return;
        for(const FigurineRenderRegion &region : regions)
            region_callback(region, adding);
    };
}

FigurineBoardChangeCallback make_figurine_framebuffer_change_callback(IndexedFramebuffer framebuffer, const std::function<void(const FigurineRenderRegion &, bool)> &region_callback)
{
    if(!framebuffer_valid(framebuffer) || !region_callback)
        return {};
    return [framebuffer, region_callback](const FallingFigurine &figurine, bool adding)
    {
        std::vector<FigurineRenderRegion> regions;
        if(!fill_figurine_board_regions(figurine, framebuffer, adding ? 0 : 0x13) || !collect_figurine_board_regions(figurine, framebuffer.width, framebuffer.height, regions))
            return;
        for(const FigurineRenderRegion &region : regions)
            region_callback(region, adding);
    };
}

bool render_figurine_board_change(const FallingFigurine &figurine, bool adding, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback)
{
    int32_t origin_x = 0;
    int32_t origin_y = 0;
    if(!framebuffer_valid(framebuffer) || !board_region_callback || !sprite_region_callback || !find_gameplay_origin(scene, origin_x, origin_y)
        || !fill_figurine_board_regions(figurine, framebuffer, adding ? 0 : 0x13, origin_x, origin_y))
        return false;
    std::vector<FigurineRenderRegion> regions;
    if(!collect_figurine_board_regions(figurine, framebuffer.width, framebuffer.height, regions, origin_x, origin_y))
        return false;
    for(const FigurineRenderRegion &region : regions)
        board_region_callback(region, adding);
    if(!adding)
        return true;
    FigurineSpriteSelection selection;
    FigurineRenderRegion sprite_region;
    if(!select_figurine_sprite(figurine, selection) || !render_figurine_sprite(selection, scene, bitmaps, framebuffer, sprite_region))
        return false;
    sprite_region_callback(sprite_region);
    return true;
}

FigurineBoardChangeCallback make_figurine_presentation_callback(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback)
{
    if(!framebuffer_valid(framebuffer) || !board_region_callback || !sprite_region_callback)
        return {};
    return [&scene, &bitmaps, framebuffer, board_region_callback, sprite_region_callback](const FallingFigurine &figurine, bool adding)
    { render_figurine_board_change(figurine, adding, scene, bitmaps, framebuffer, board_region_callback, sprite_region_callback); };
}

bool render_match_animation_plan(const MatchAnimationPlan &plan, const std::vector<RliAnimation> &animations, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &)> &dirty_callback, int32_t origin_x, int32_t origin_y)
{
    if(!framebuffer_valid(framebuffer) || framebuffer.stride < framebuffer.width || !dirty_callback)
        return false;
    struct Layer
    {
        uint8_t slot{};
        const RliAnimation *animation{};
        const RliFrameRecord *frame{};
        const MatchAnimationParticipant *participant{};
    };
    const auto find_animation = [&animations](MatchAnimationResource resource) -> const RliAnimation *
    {
        const char *path = resource == MatchAnimationResource::man         ? "m.rli"
                         : resource == MatchAnimationResource::rotated_man ? "rm.rli"
                         : resource == MatchAnimationResource::woman       ? "w.rli"
                                                                           : "rw.rli";
        for(const RliAnimation &animation : animations)
            if(animation.path == path)
                return &animation;
        return nullptr;
    };
    std::vector<uint8_t> background((size_t)framebuffer.width * framebuffer.height);
    for(uint32_t row = 0; row < framebuffer.height; ++row)
        std::memcpy(background.data() + (size_t)row * framebuffer.width, framebuffer.pixels + (size_t)row * framebuffer.stride, framebuffer.width);
    std::vector<Layer> layers;
    const int64_t plan_left = std::max<int64_t>(0, std::min<int64_t>((int64_t)plan.man.x + origin_x, (int64_t)plan.woman.x + origin_x));
    const int64_t plan_top = std::max<int64_t>(0, std::min<int64_t>((int64_t)plan.man.y + origin_y, (int64_t)plan.woman.y + origin_y));
    const int64_t plan_right = std::min<int64_t>(framebuffer.width, std::max<int64_t>((int64_t)plan.man.x + origin_x + 108, (int64_t)plan.woman.x + origin_x + 108));
    const int64_t plan_bottom = std::min<int64_t>(framebuffer.height, std::max<int64_t>((int64_t)plan.man.y + origin_y + 108, (int64_t)plan.woman.y + origin_y + 108));
    const auto reveal_participant = [&](const MatchAnimationParticipant &participant)
    {
        const RliAnimation *animation = find_animation(participant.resource);
        if(animation == nullptr || participant.temporary_slot_count != participant.frame_count || (size_t)participant.first_frame + participant.frame_count > animation->frame_records.size())
            return false;
        for(uint8_t index = 0; index < participant.frame_count; ++index)
        {
            layers.push_back({ participant.temporary_slots[index], animation, &animation->frame_records[participant.first_frame + index], &participant });
            std::sort(layers.begin(), layers.end(), [](const Layer &first, const Layer &second) { return first.slot < second.slot; });
            for(uint32_t row = 0; row < framebuffer.height; ++row)
                std::memcpy(framebuffer.pixels + (size_t)row * framebuffer.stride, background.data() + (size_t)row * framebuffer.width, framebuffer.width);
            for(const Layer &layer : layers)
                if(!blit_rli_frame_canvas(*layer.frame, (uint32_t)layer.animation->width, (uint32_t)layer.animation->height, framebuffer, layer.participant->x + origin_x,
                       layer.participant->y + origin_y, layer.participant->mirror_horizontal, layer.participant->mirror_vertical))
                    return false;
            dirty_callback({ (int32_t)plan_left, (int32_t)plan_top, plan_right > plan_left ? (uint32_t)(plan_right - plan_left) : 0, plan_bottom > plan_top ? (uint32_t)(plan_bottom - plan_top) : 0 });
        }
        return true;
    };
    return reveal_participant(plan.man) && reveal_participant(plan.woman);
}

bool render_match_blink_sequence(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, const std::vector<RliAnimation> &animations,
    IndexedFramebuffer framebuffer, const std::function<void(const FigurineRenderRegion &)> &dirty_callback, const std::function<void(uint32_t)> &delay_callback, int32_t origin_x, int32_t origin_y)
{
    if(!framebuffer_valid(framebuffer) || !dirty_callback || !delay_callback)
        return false;
    MatchAnimationPlan normal_plan;
    MatchAnimationPlan expanded_plan;
    if(!build_match_animation_plan(first, second, action, false, normal_plan) || !build_match_animation_plan(first, second, action, true, expanded_plan))
        return false;
    std::vector<uint8_t> background((size_t)framebuffer.width * framebuffer.height);
    for(uint32_t row = 0; row < framebuffer.height; ++row)
        std::memcpy(background.data() + (size_t)row * framebuffer.width, framebuffer.pixels + (size_t)row * framebuffer.stride, framebuffer.width);
    const auto restore_background = [&]()
    {
        for(uint32_t row = 0; row < framebuffer.height; ++row)
            std::memcpy(framebuffer.pixels + (size_t)row * framebuffer.stride, background.data() + (size_t)row * framebuffer.width, framebuffer.width);
    };
    const auto render_plan = [&](const MatchAnimationPlan &plan)
    {
        restore_background();
        int64_t left = framebuffer.width;
        int64_t top = framebuffer.height;
        int64_t right = 0;
        int64_t bottom = 0;
        const auto accumulate_dirty = [&](const FigurineRenderRegion &region)
        {
            left = (std::min)(left, (int64_t)region.x);
            top = (std::min)(top, (int64_t)region.y);
            right = (std::max)(right, (int64_t)region.x + region.width);
            bottom = (std::max)(bottom, (int64_t)region.y + region.height);
        };
        if(!render_match_animation_plan(plan, animations, framebuffer, accumulate_dirty, origin_x, origin_y))
            return false;
        if(right > left && bottom > top)
            dirty_callback({ (int32_t)left, (int32_t)top, (uint32_t)(right - left), (uint32_t)(bottom - top) });
        return true;
    };
    if(!render_plan(normal_plan))
        return false;
    for(int cycle = 0; cycle < 2; ++cycle)
    {
        delay_callback(400);
        if(!render_plan(expanded_plan))
            return false;
        delay_callback(400);
        if(!render_plan(normal_plan))
            return false;
    }
    restore_background();
    const std::array<int64_t, 4> horizontal_bounds{ (int64_t)normal_plan.man.x + origin_x, (int64_t)normal_plan.woman.x + origin_x, (int64_t)expanded_plan.man.x + origin_x,
        (int64_t)expanded_plan.woman.x + origin_x };
    const std::array<int64_t, 4> vertical_bounds{ (int64_t)normal_plan.man.y + origin_y, (int64_t)normal_plan.woman.y + origin_y, (int64_t)expanded_plan.man.y + origin_y,
        (int64_t)expanded_plan.woman.y + origin_y };
    const int64_t left = (std::max)((int64_t)0, *std::min_element(horizontal_bounds.begin(), horizontal_bounds.end()));
    const int64_t top = (std::max)((int64_t)0, *std::min_element(vertical_bounds.begin(), vertical_bounds.end()));
    const int64_t right = (std::min)((int64_t)framebuffer.width, *std::max_element(horizontal_bounds.begin(), horizontal_bounds.end()) + 108);
    const int64_t bottom = (std::min)((int64_t)framebuffer.height, *std::max_element(vertical_bounds.begin(), vertical_bounds.end()) + 108);
    dirty_callback({ (int32_t)left, (int32_t)top, right > left ? (uint32_t)(right - left) : 0, bottom > top ? (uint32_t)(bottom - top) : 0 });
    return true;
}

} // namespace xtet
