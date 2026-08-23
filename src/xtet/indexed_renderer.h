#pragma once

#include <functional>
#include <map>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "asset_decoders.h"
#include "gameplay_state.h"
#include "match_animation.h"
#include "rli_animation.h"
#include "scene_description.h"

namespace xtet
{

struct XrgbFramebuffer
{
    uint32_t *pixels{};
    uint32_t width{};
    uint32_t height{};
    size_t stride{};
    const std::array<PaletteColor, 256> *palette{};
};

struct FigurineRenderRegion
{
    int32_t x{};
    int32_t y{};
    uint32_t width{};
    uint32_t height{};
};

bool blit_opaque(const IndexedBitmap &source, XrgbFramebuffer destination, int32_t x, int32_t y);
bool blit_transparent(const IndexedBitmap &source, XrgbFramebuffer destination, int32_t x, int32_t y, bool flip_x = false, bool flip_y = false);
bool blit_rli_frame(const RliFrameRecord &frame, XrgbFramebuffer destination, int32_t x, int32_t y);
bool blit_rli_frame_canvas(const RliFrameRecord &frame, uint32_t canvas_width, uint32_t canvas_height, XrgbFramebuffer destination, int32_t x, int32_t y, bool flip_x = false, bool flip_y = false);
bool render_score(uint32_t score, const IndexedBitmap &digit_atlas, XrgbFramebuffer destination);
bool render_initial_scene(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, XrgbFramebuffer framebuffer);
int32_t map_scaled_cursor_coordinate(int32_t value, int32_t client_extent, int32_t framebuffer_extent);
int hit_test_sprite_collection(const SceneNode &collection, const std::map<std::string, IndexedBitmap> &bitmaps, int32_t x, int32_t y);
bool render_figurine_sprite(const FigurineSpriteSelection &selection, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, XrgbFramebuffer framebuffer,
    FigurineRenderRegion &region);
bool collect_figurine_board_regions(const FallingFigurine &figurine, uint32_t framebuffer_width, uint32_t framebuffer_height, std::vector<FigurineRenderRegion> &regions, int32_t origin_x = 0,
    int32_t origin_y = 0);
bool fill_figurine_board_regions(const FallingFigurine &figurine, XrgbFramebuffer framebuffer, uint8_t fill_index, int32_t origin_x = 0, int32_t origin_y = 0);
FigurineBoardChangeCallback make_figurine_board_change_callback(uint32_t framebuffer_width, uint32_t framebuffer_height,
    const std::function<void(const FigurineRenderRegion &, bool)> &region_callback);
FigurineBoardChangeCallback make_figurine_framebuffer_change_callback(XrgbFramebuffer framebuffer, const std::function<void(const FigurineRenderRegion &, bool)> &region_callback);
bool render_figurine_board_change(const FallingFigurine &figurine, bool adding, const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, XrgbFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback);
FigurineBoardChangeCallback make_figurine_presentation_callback(const SceneDescription &scene, const std::map<std::string, IndexedBitmap> &bitmaps, XrgbFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &, bool)> &board_region_callback, const std::function<void(const FigurineRenderRegion &)> &sprite_region_callback);
bool render_match_animation_plan(const MatchAnimationPlan &plan, const std::vector<RliAnimation> &animations, XrgbFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &)> &dirty_callback, int32_t origin_x = 0, int32_t origin_y = 0);
bool render_match_blink_sequence(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, const std::vector<RliAnimation> &animations, XrgbFramebuffer framebuffer,
    const std::function<void(const FigurineRenderRegion &)> &dirty_callback, const std::function<void(uint32_t)> &delay_callback, int32_t origin_x = 0, int32_t origin_y = 0);

} // namespace xtet
