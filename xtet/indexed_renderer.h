#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "asset_decoders.h"
#include "gameplay_state.h"
#include "match_animation.h"
#include "rli_animation.h"
#include "scene_description.h"

namespace xtet
{

struct IndexedFramebuffer
{
    std::uint8_t *pixels{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::size_t stride{};
};

struct FigurineRenderRegion
{
    std::int32_t x{};
    std::int32_t y{};
    std::uint32_t width{};
    std::uint32_t height{};
};

bool blit_opaque(const IndexedBitmap &source, IndexedFramebuffer destination, std::int32_t x, std::int32_t y);
bool blit_transparent(const IndexedBitmap &source, IndexedFramebuffer destination, std::int32_t x, std::int32_t y, bool flip_x = false, bool flip_y = false);
bool blit_rli_frame(const RliFrameRecord &frame, IndexedFramebuffer destination, std::int32_t x, std::int32_t y);
bool blit_rli_frame_canvas(const RliFrameRecord &frame, std::uint32_t canvas_width, std::uint32_t canvas_height, IndexedFramebuffer destination, std::int32_t x, std::int32_t y, bool flip_x = false,
    bool flip_y = false);
bool render_score(std::uint32_t score, const IndexedBitmap &digit_atlas, IndexedFramebuffer destination);
bool render_initial_scene(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer);
int hit_test_sprite_collection(const SceneNode &collection, const std::map<std::string, IndexedBitmap> &bitmaps, std::int32_t x, std::int32_t y);
bool render_figurine_sprite(const FigurineSpriteSelection &selection, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    FigurineRenderRegion &region);
bool collect_figurine_board_regions(const FallingFigurine &figurine, std::uint32_t framebuffer_width, std::uint32_t framebuffer_height, std::vector<FigurineRenderRegion> &regions,
    std::int32_t origin_x = 0, std::int32_t origin_y = 0);
bool fill_figurine_board_regions(const FallingFigurine &figurine, IndexedFramebuffer framebuffer, std::uint8_t fill_index, std::int32_t origin_x = 0, std::int32_t origin_y = 0);
FigurineBoardChangeCallback make_figurine_board_change_callback(std::uint32_t framebuffer_width, std::uint32_t framebuffer_height,
    const std::function<void(const FigurineRenderRegion &, bool)> &region_callback);
FigurineBoardChangeCallback make_figurine_framebuffer_change_callback(IndexedFramebuffer framebuffer, const std::function<void(const FigurineRenderRegion &, bool)> &region_callback);
bool render_figurine_board_change(const FallingFigurine &figurine, bool adding, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback);
FigurineBoardChangeCallback make_figurine_presentation_callback(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback);
bool render_match_animation_plan(const MatchAnimationPlan &plan, const std::vector<RliAnimation> &animations, IndexedFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &)> &dirty_callback, std::int32_t origin_x = 0, std::int32_t origin_y = 0);
bool render_match_blink_sequence(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, const std::vector<RliAnimation> &animations,
    IndexedFramebuffer framebuffer, const std::function<void(const FigurineRenderRegion &)> &dirty_callback, const std::function<void(std::uint32_t)> &delay_callback, std::int32_t origin_x = 0,
    std::int32_t origin_y = 0);

} // namespace xtet
