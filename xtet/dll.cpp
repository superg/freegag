#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <mmsystem.h>
#include <mutex>
#include <stdexcept>
#include "action_definitions.h"
#include "api.h"
#include "asset_decoders.h"
#include "asset_manifest.h"
#include "audio_coordinator.h"
#include "figurine_geometry.h"
#include "game_runtime.h"
#include "game_timing.h"
#include "game_worker.h"
#include "indexed_renderer.h"
#include "resource_provider.h"
#include "rli_animation.h"
#include "scene_description.h"
#include "sfs_archive.h"

namespace
{

struct GameState
{
    HMODULE module{};
    HWND window{};
    xtet::GameHostContext *host_context{};
    std::array<void *, xtet::kCallbackCount> callbacks{};
    xtet::ResourceView sfs{};
    xtet::SfsArchive archive;
    std::vector<xtet::ActionDefinition> action_definitions;
    xtet::AssetManifest asset_manifest;
    std::map<std::string, xtet::IndexedBitmap> bitmaps;
    std::map<std::string, xtet::WavePcm> waves;
    xtet::SceneDescription scene;
    std::vector<xtet::RliAnimation> animations;
    xtet::AudioCoordinator audio;
    xtet::FigurineGeometryTables figurine_geometry;
    xtet::GameplayRuntime gameplay_runtime;
    xtet::GameWorker worker;
    std::uint32_t result{};
    std::uint32_t selected_level{ 1 };
    std::uint32_t level_effect_deadline{};
    bool level_effect_active{};
    bool audio_enabled{ true };
    std::atomic_bool initialized{};
};

// Declared before g_game so it remains alive while GameWorker is stopped by
// GameState destruction during DLL unload.
std::recursive_mutex g_mutex;
GameState g_game;

void present_dirty_region(const xtet::FigurineRenderRegion &region);
bool render_gameplay_frame(const xtet::FallingFigurine *excluded_first = nullptr, const xtet::FallingFigurine *excluded_second = nullptr);

void send_result()
{
    const xtet::GameResultDescriptor descriptor{ 2, 0, sizeof(g_game.result), &g_game.result };
    SendMessageA(g_game.window, xtet::kGameMessage, (WPARAM)&descriptor, 0x40);
}

void fail_initialization()
{
    g_game.worker.setEnabled(false);
    g_game.audio.destroy();
    g_game.initialized = false;
    send_result();
    PostMessageA(g_game.window, xtet::kGameMessage, 0, 1);
}

void drain_keyboard_messages()
{
    MSG message{};
    while(PeekMessageA(&message, g_game.window, 0x100, 0x108, PM_REMOVE | PM_NOYIELD))
    {
    }
}

void stop_gameplay()
{
    g_game.worker.setEnabled(false);
    if(g_game.audio.valid())
        g_game.audio.setLoopPlaying(false);
    g_game.gameplay_runtime.stop();
    g_game.initialized = false;
}

xtet::FigurineBoardChangeCallback make_presentation_callback(xtet::IndexedFramebuffer framebuffer);

void post_game_result(std::uint32_t score)
{
    g_game.result = score;
    send_result();
}

void post_game_termination()
{
    PostMessageA(g_game.window, xtet::kGameMessage, 0, 0);
}

bool get_framebuffer(xtet::IndexedFramebuffer &framebuffer)
{
    if(!g_game.host_context || !g_game.host_context->framebuffer || g_game.host_context->width == 0 || g_game.host_context->height == 0)
        return false;
    framebuffer = { (std::uint8_t *)g_game.host_context->framebuffer, g_game.host_context->width, g_game.host_context->height, g_game.host_context->width };
    return true;
}

void present_dirty_region(const xtet::FigurineRenderRegion &region)
{
    if(!g_game.callbacks[0] || region.width == 0 || region.height == 0)
        return;
    const xtet::DirtyRegionCallback dirty_region = (xtet::DirtyRegionCallback)g_game.callbacks[0];
    dirty_region(region.x, region.y, (std::int32_t)region.width, (std::int32_t)region.height);
}

bool present_result_screen(const char *path)
{
    xtet::IndexedFramebuffer framebuffer;
    const auto bitmap = g_game.bitmaps.find(path);
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(!get_framebuffer(framebuffer) || bitmap == g_game.bitmaps.end() || homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position
        || !xtet::blit_opaque(bitmap->second, framebuffer, homes[0]->children[1].position->x, homes[0]->children[1].position->y))
        return false;
    present_dirty_region({ homes[0]->children[1].position->x, homes[0]->children[1].position->y, bitmap->second.width, bitmap->second.height });
    return true;
}

bool present_level_face(std::uint32_t level, bool excited)
{
    if(level < 1 || level > 10)
        return false;
    char path[16];
    std::snprintf(path, sizeof(path), excited ? "f%02u!.bmp" : "f%02u.bmp", level);
    const auto bitmap = g_game.bitmaps.find(path);
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(bitmap == g_game.bitmaps.end() || homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position)
        return false;
    g_game.level_effect_active = excited;
    if(!render_gameplay_frame())
        return false;
    present_dirty_region({ homes[0]->children[1].position->x, homes[0]->children[1].position->y, bitmap->second.width, bitmap->second.height });
    return true;
}

bool render_gameplay_frame(const xtet::FallingFigurine *excluded_first, const xtet::FallingFigurine *excluded_second)
{
    xtet::IndexedFramebuffer framebuffer;
    if(!get_framebuffer(framebuffer) || !xtet::render_initial_scene(g_game.scene, g_game.bitmaps, framebuffer))
        return false;
    const xtet::GameProgress &progress = g_game.gameplay_runtime.progress();
    if(progress.level > 1)
    {
        char path[16];
        std::snprintf(path, sizeof(path), g_game.level_effect_active ? "f%02u!.bmp" : "f%02u.bmp", progress.level);
        const auto face = g_game.bitmaps.find(path);
        const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
        if(face == g_game.bitmaps.end() || homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position
            || !xtet::blit_opaque(face->second, framebuffer, homes[0]->children[1].position->x, homes[0]->children[1].position->y))
            return false;
    }
    const auto digits = g_game.bitmaps.find("digit.bmp");
    if(digits == g_game.bitmaps.end() || !xtet::render_score(progress.score, digits->second, framebuffer))
        return false;
    for(const xtet::FigurineBoardEntry &entry : g_game.gameplay_runtime.entries())
    {
        if(entry.figurine == nullptr || entry.figurine == excluded_first || entry.figurine == excluded_second)
            continue;
        xtet::FigurineSpriteSelection selection;
        xtet::FigurineRenderRegion region;
        if(!xtet::select_figurine_sprite(*entry.figurine, selection) || !xtet::render_figurine_sprite(selection, g_game.scene, g_game.bitmaps, framebuffer, region))
            return false;
    }
    return true;
}

xtet::FigurineBoardChangeCallback make_presentation_callback(xtet::IndexedFramebuffer framebuffer)
{
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position)
        return {};
    const std::int32_t origin_x = homes[0]->children[1].position->x;
    const std::int32_t origin_y = homes[0]->children[1].position->y;
    return [framebuffer, origin_x, origin_y, pending_regions = std::vector<xtet::FigurineRenderRegion>{}](const xtet::FallingFigurine &figurine, bool adding) mutable
    {
        xtet::FigurineSpriteSelection selection;
        xtet::FigurineRenderRegion sprite_region;
        if(!xtet::select_figurine_sprite(figurine, selection) || !xtet::render_figurine_sprite(selection, g_game.scene, g_game.bitmaps, framebuffer, sprite_region))
            return;
        std::vector<xtet::FigurineRenderRegion> regions;
        if(!xtet::collect_figurine_board_regions(figurine, framebuffer.width, framebuffer.height, regions, origin_x, origin_y))
            return;
        if(!adding)
        {
            pending_regions = std::move(regions);
            pending_regions.push_back(sprite_region);
            return;
        }
        if(!render_gameplay_frame())
            return;
        pending_regions.insert(pending_regions.end(), regions.begin(), regions.end());
        pending_regions.push_back(sprite_region);
        std::int64_t left = framebuffer.width;
        std::int64_t top = framebuffer.height;
        std::int64_t right = 0;
        std::int64_t bottom = 0;
        for(const xtet::FigurineRenderRegion &region : pending_regions)
        {
            left = (std::min)(left, (std::int64_t)region.x);
            top = (std::min)(top, (std::int64_t)region.y);
            right = (std::max)(right, (std::int64_t)region.x + region.width);
            bottom = (std::max)(bottom, (std::int64_t)region.y + region.height);
        }
        pending_regions.clear();
        if(right > left && bottom > top)
            present_dirty_region({ (std::int32_t)left, (std::int32_t)top, (std::uint32_t)(right - left), (std::uint32_t)(bottom - top) });
    };
}

void present_score(const xtet::GameProgress &progress, const xtet::ProgressUpdate &update)
{
    if(update.level_changed)
    {
        present_level_face(progress.level, false);
        present_level_face(progress.level, true);
        g_game.level_effect_active = true;
        g_game.level_effect_deadline = timeGetTime() + 1000;
        if(g_game.audio_enabled)
            g_game.audio.queueRandom("level", (std::uint32_t)std::rand());
    }
    if(update.game_over)
    {
        present_result_screen("wind.bmp");
        g_game.audio.setLoopPlaying(false);
        if(g_game.audio_enabled)
            g_game.audio.queueRandom("win", (std::uint32_t)std::rand());
    }
    xtet::IndexedFramebuffer framebuffer;
    const auto digits = g_game.bitmaps.find("digit.bmp");
    if(!get_framebuffer(framebuffer) || digits == g_game.bitmaps.end() || !xtet::render_score(progress.score, digits->second, framebuffer))
        return;
    const std::uint32_t glyph_width = digits->second.width / 4;
    const std::uint32_t glyph_height = digits->second.height / 10;
    if(framebuffer.width <= 359 || framebuffer.height <= 438)
        return;
    const std::uint32_t width = (std::min)(glyph_width * 4, framebuffer.width - 359);
    const std::uint32_t height = (std::min)(glyph_height, framebuffer.height - 438);
    present_dirty_region({ 359, 438, width, height });
}

bool present_match_effect(const xtet::FallingFigurine &first, const xtet::FallingFigurine &second, const xtet::ActionDefinition &action)
{
    xtet::IndexedFramebuffer framebuffer;
    if(!get_framebuffer(framebuffer) || !g_game.callbacks[0] || (g_game.audio_enabled && !g_game.audio.queueRandom("act", (std::uint32_t)std::rand())))
        return false;
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    std::array<xtet::FigurineRenderRegion, 4> sprite_regions{};
    std::size_t sprite_index = 0;
    for(const xtet::FallingFigurine *figurine : { &first, &second })
    {
        xtet::FigurineSpriteSelection selection;
        if(!xtet::select_figurine_sprite(*figurine, selection) || !xtet::render_figurine_sprite(selection, g_game.scene, g_game.bitmaps, framebuffer, sprite_regions[sprite_index++]))
            return false;
        xtet::FallingFigurine committed = *figurine;
        committed.orientation = committed.previous_orientation;
        committed.column = committed.previous_column;
        committed.row = committed.previous_row;
        if(!xtet::select_figurine_sprite(committed, selection) || !xtet::render_figurine_sprite(selection, g_game.scene, g_game.bitmaps, framebuffer, sprite_regions[sprite_index++]))
            return false;
    }
    if(homes.size() != 1 || homes[0]->children.size() < 2 || !homes[0]->children[1].position || !render_gameplay_frame(&first, &second))
        return false;
    std::vector<xtet::FigurineRenderRegion> initial_regions;
    for(const xtet::FallingFigurine *figurine : { &first, &second })
    {
        std::vector<xtet::FigurineRenderRegion> regions;
        if(!xtet::collect_figurine_board_regions(*figurine, framebuffer.width, framebuffer.height, regions, homes[0]->children[1].position->x, homes[0]->children[1].position->y))
            return false;
        initial_regions.insert(initial_regions.end(), regions.begin(), regions.end());
    }
    initial_regions.insert(initial_regions.end(), sprite_regions.begin(), sprite_regions.end());
    const auto present_animation_region = [initial_regions = std::move(initial_regions), initial_update = true](const xtet::FigurineRenderRegion &animation_region) mutable
    {
        if(!initial_update)
        {
            present_dirty_region(animation_region);
            return;
        }
        initial_update = false;
        std::int64_t left = animation_region.x;
        std::int64_t top = animation_region.y;
        std::int64_t right = (std::int64_t)animation_region.x + animation_region.width;
        std::int64_t bottom = (std::int64_t)animation_region.y + animation_region.height;
        for(const xtet::FigurineRenderRegion &region : initial_regions)
        {
            left = (std::min)(left, (std::int64_t)region.x);
            top = (std::min)(top, (std::int64_t)region.y);
            right = (std::max)(right, (std::int64_t)region.x + region.width);
            bottom = (std::max)(bottom, (std::int64_t)region.y + region.height);
        }
        present_dirty_region({ (std::int32_t)left, (std::int32_t)top, (std::uint32_t)(right - left), (std::uint32_t)(bottom - top) });
    };
    return xtet::render_match_blink_sequence(
        first, second, action, g_game.animations, framebuffer, present_animation_region, [](std::uint32_t delay) { Sleep(delay); }, homes[0]->children[1].position->x,
        homes[0]->children[1].position->y);
}

bool initialize_audio()
{
    if(!g_game.callbacks[1] || !g_game.callbacks[2] || !g_game.callbacks[3] || !g_game.callbacks[4] || !g_game.callbacks[5])
        return false;
    const xtet::SoundCreateCallback create_sound = (xtet::SoundCreateCallback)g_game.callbacks[1];
    const xtet::SoundDestroyCallback destroy_sound = (xtet::SoundDestroyCallback)g_game.callbacks[2];
    const xtet::SoundQueueCallback queue_sound = (xtet::SoundQueueCallback)g_game.callbacks[3];
    const xtet::SoundControlCallback stop_sound = (xtet::SoundControlCallback)g_game.callbacks[4];
    const xtet::SoundControlCallback start_sound = (xtet::SoundControlCallback)g_game.callbacks[5];
    const xtet::AudioHostCallbacks callbacks{ [create_sound](const xtet::PcmFormat *format) { return create_sound(format); }, [destroy_sound](xtet::SoundHandle handle) { destroy_sound(handle); },
        [queue_sound](xtet::SoundHandle handle, const void *samples, std::uint32_t size, bool replace) { return queue_sound(handle, samples, size, replace ? 1 : 0) != 0; },
        [stop_sound](xtet::SoundHandle handle, bool reset) { return stop_sound(handle, reset ? 1 : 0) != 0; },
        [start_sound](xtet::SoundHandle handle, bool restart) { return start_sound(handle, restart ? 1 : 0) != 0; } };
    if(!g_game.audio.initialize(g_game.scene, g_game.waves, callbacks) || !g_game.audio.initializeLoopQueue() || !g_game.audio.setLoopPlaying(true))
    {
        g_game.audio.destroy();
        return false;
    }
    return true;
}

void handle_gameplay_key(std::uint32_t key)
{
    xtet::IndexedFramebuffer framebuffer;
    if(!get_framebuffer(framebuffer))
        return;
    const xtet::FigurineBoardChangeCallback presentation_callback = make_presentation_callback(framebuffer);
    xtet::GameplayInputOutcome outcome;
    xtet::CascadeResult cascade_result;
    g_game.gameplay_runtime.handleInput(xtet::translate_gameplay_key(key), g_game.figurine_geometry, g_game.action_definitions, present_match_effect, drain_keyboard_messages, presentation_callback,
        outcome, cascade_result, present_score);
}

void run_game_tick()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if(!g_game.initialized)
        return;
    xtet::IndexedFramebuffer framebuffer;
    if(!get_framebuffer(framebuffer))
        throw std::runtime_error("XTET framebuffer unavailable");
    const std::uint32_t current_time = timeGetTime();
    if(g_game.level_effect_active && g_game.level_effect_deadline < current_time)
    {
        present_level_face(g_game.gameplay_runtime.progress().level, false);
        g_game.level_effect_active = false;
    }
    std::rand();
    std::uint32_t family_random = 0;
    std::uint32_t shape_random = 0;
    std::uint32_t orientation_random = 0;
    if(g_game.gameplay_runtime.activeValue() == nullptr)
    {
        family_random = (std::uint32_t)std::rand();
        shape_random = (std::uint32_t)std::rand();
        orientation_random = (std::uint32_t)std::rand();
    }
    xtet::GameTickResult tick_result;
    xtet::CascadeResult cascade_result;
    if(!g_game.gameplay_runtime.updateTick(family_random, shape_random, orientation_random, current_time, g_game.figurine_geometry, g_game.action_definitions, present_match_effect,
           make_presentation_callback(framebuffer), tick_result, cascade_result, present_score))
        throw std::runtime_error("XTET gameplay tick failed");
    if(tick_result == xtet::GameTickResult::settled && g_game.audio_enabled)
        g_game.audio.queueRandom("stop", (std::uint32_t)std::rand());
    else if(tick_result == xtet::GameTickResult::spawn_failed)
    {
        present_result_screen("over.bmp");
        g_game.audio.setLoopPlaying(false);
        if(g_game.audio_enabled)
            g_game.audio.queueRandom("over", (std::uint32_t)std::rand());
    }
}

void report_worker_failure()
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if(!g_game.initialized)
        return;
    g_game.initialized = false;
    g_game.audio.setLoopPlaying(false);
    send_result();
    PostMessageA(g_game.window, xtet::kGameMessage, 0, 1);
}

bool initialize_worker()
{
    return g_game.worker.start(
        []()
        {
            std::lock_guard<std::recursive_mutex> lock(g_mutex);
            return xtet::get_game_tick_interval(g_game.gameplay_runtime.progress().level);
        },
        []() { run_game_tick(); }, []() { report_worker_failure(); });
}

bool load_declared_assets()
{
    std::vector<std::uint8_t> bytes;
    for(const std::string &path : g_game.asset_manifest.bitmap_paths)
    {
        if(g_game.bitmaps.find(path) != g_game.bitmaps.end())
            continue;
        xtet::IndexedBitmap bitmap;
        if(!g_game.archive.read(path, bytes) || !xtet::decode_indexed_bitmap(bytes, bitmap))
            return false;
        g_game.bitmaps.emplace(path, std::move(bitmap));
    }
    for(const std::string &path : g_game.asset_manifest.wave_paths)
    {
        if(g_game.waves.find(path) != g_game.waves.end())
            continue;
        xtet::WavePcm wave;
        if(!g_game.archive.read(path, bytes) || !xtet::decode_wave_pcm(bytes, wave))
            return false;
        g_game.waves.emplace(path, std::move(wave));
    }
    return true;
}

bool render_initial_frame()
{
    xtet::IndexedFramebuffer framebuffer;
    if(!get_framebuffer(framebuffer) || !g_game.callbacks[0])
        return false;
    if(!xtet::render_initial_scene(g_game.scene, g_game.bitmaps, framebuffer))
        return false;
    const auto digits = g_game.bitmaps.find("digit.bmp");
    if(digits == g_game.bitmaps.end() || !xtet::render_score(0, digits->second, framebuffer))
        return false;
    const xtet::DirtyRegionCallback dirty_region = (xtet::DirtyRegionCallback)g_game.callbacks[0];
    dirty_region(0, 0, g_game.host_context->width, g_game.host_context->height);
    return true;
}

void dispatch_key_down(std::uint32_t key)
{
    xtet::GameProgress &progress = g_game.gameplay_runtime.progress();
    const xtet::GameKeyDownCallbacks key_callbacks{ []() { stop_gameplay(); }, [](std::uint32_t score) { post_game_result(score); }, []() { post_game_termination(); },
        [](std::uint32_t gameplay_key) { handle_gameplay_key(gameplay_key); }, []() { drain_keyboard_messages(); } };
    xtet::handle_game_key_down(progress.gameplay_state, key, timeGetTime(), g_game.gameplay_runtime.resultInputDeadline(), progress.score, key_callbacks);
}

int find_pressed_button()
{
    POINT point;
    RECT client;
    if(!GetClientRect(g_game.window, &client) || !GetCursorPos(&point) || !ScreenToClient(g_game.window, &point))
        return -1;
    point.x = xtet::map_scaled_cursor_coordinate(point.x - client.left, client.right - client.left, g_game.host_context ? g_game.host_context->width : 0);
    point.y = xtet::map_scaled_cursor_coordinate(point.y - client.top, client.bottom - client.top, g_game.host_context ? g_game.host_context->height : 0);
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(homes.size() != 1 || homes[0]->children.size() != 3)
        return -1;
    return xtet::hit_test_sprite_collection(homes[0]->children[2], g_game.bitmaps, point.x, point.y);
}

bool present_control_overlay(std::size_t index, bool shown)
{
    xtet::IndexedFramebuffer framebuffer;
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(!get_framebuffer(framebuffer) || homes.size() != 1 || homes[0]->children.size() != 3)
        return false;
    const xtet::SceneNode &controls = homes[0]->children[2];
    if(index >= controls.children.size())
        return false;
    const xtet::SceneNode &control = controls.children[index];
    if(control.type != xtet::SceneNodeType::sprite_bitmap || !control.position || control.children.size() != 1 || control.children[0].type != xtet::SceneNodeType::bitmap
        || control.children[0].loaded_path.empty())
        return false;
    const auto bitmap = g_game.bitmaps.find(control.children[0].loaded_path);
    if(bitmap == g_game.bitmaps.end())
        return false;
    if(shown)
    {
        if(control.transparent.value_or(false))
        {
            if(!xtet::blit_transparent(bitmap->second, framebuffer, control.position->x, control.position->y))
                return false;
        }
        else if(!xtet::blit_opaque(bitmap->second, framebuffer, control.position->x, control.position->y))
            return false;
    }
    else if(!render_gameplay_frame())
        return false;
    present_dirty_region({ control.position->x, control.position->y, bitmap->second.width, bitmap->second.height });
    return true;
}

bool restart_game()
{
    g_game.worker.setEnabled(false);
    g_game.level_effect_active = false;
    if(!g_game.gameplay_runtime.initialize(15, 103, { 0, g_game.selected_level, g_game.selected_level, 1 }) || !present_level_face(g_game.selected_level, false))
        return false;
    present_score(g_game.gameplay_runtime.progress(), {});
    if(!g_game.audio.setLoopPlaying(true))
        return false;
    g_game.worker.setEnabled(true);
    return true;
}

void handle_mouse_button(bool pressed)
{
    xtet::GameProgress &progress = g_game.gameplay_runtime.progress();
    if(progress.gameplay_state == 2 || progress.gameplay_state == 3)
    {
        if(pressed)
            dispatch_key_down(0);
        return;
    }
    if(!pressed)
        return;
    const int button = find_pressed_button();
    if(progress.gameplay_state == 4)
    {
        if(button == 3 && xtet::set_game_paused(progress.gameplay_state, false, [](bool playing) { return g_game.audio.setLoopPlaying(playing); }))
        {
            if(!present_control_overlay(3, false))
                throw std::runtime_error("XTET pause button release failed");
            g_game.worker.setEnabled(true);
            PostMessageA(g_game.window, xtet::kGameMessage, 0, 0x20);
        }
        else if(button == 7)
            dispatch_key_down(0x1b);
        return;
    }
    if(progress.gameplay_state != 1)
        return;
    switch(button)
    {
    case 0:
        if(!restart_game())
            throw std::runtime_error("XTET restart failed");
        break;
    case 1:
        handle_gameplay_key(0x26);
        break;
    case 2:
        handle_gameplay_key(0x25);
        break;
    case 3:
        if(xtet::set_game_paused(progress.gameplay_state, true, [](bool playing) { return g_game.audio.setLoopPlaying(playing); }))
        {
            g_game.worker.setEnabled(false);
            if(!present_control_overlay(3, true))
                throw std::runtime_error("XTET pause button press failed");
            PostMessageA(g_game.window, xtet::kGameMessage, 0, 0x10);
        }
        break;
    case 4:
        if(g_game.selected_level < 10)
        {
            ++g_game.selected_level;
            progress.score = 0;
            progress.base_level = g_game.selected_level;
            progress.level = g_game.selected_level;
            present_score(progress, { true, true, false });
        }
        break;
    case 5:
        handle_gameplay_key(0x28);
        break;
    case 6:
        handle_gameplay_key(0x27);
        break;
    case 7:
        dispatch_key_down(0x1b);
        break;
    default:
        break;
    }
}

} // namespace

extern "C" void XTET_ABI GAME_DLL_INIT(xtet::GameHostContext *host_context, void **callback_table)
{
    g_game.worker.stop();
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    g_game.host_context = host_context;
    g_game.window = host_context ? host_context->window : nullptr;
    g_game.callbacks.fill(nullptr);
    if(callback_table)
    {
        for(std::size_t index = 0; index < g_game.callbacks.size(); ++index)
            g_game.callbacks[index] = callback_table[index];
    }
    g_game.sfs = xtet::load_embedded_sfs(g_game.module);
    g_game.action_definitions.clear();
    g_game.asset_manifest = {};
    g_game.bitmaps.clear();
    g_game.waves.clear();
    g_game.scene = {};
    g_game.animations.clear();
    g_game.level_effect_active = false;
    g_game.audio_enabled = true;
    std::vector<std::uint8_t> action_bytes;
    if(!g_game.archive.mount(g_game.sfs) || !g_game.archive.read("acts.txt", action_bytes) || !xtet::parse_action_definitions(action_bytes, g_game.action_definitions)
        || !xtet::load_asset_manifest(g_game.archive, { "base_scr.txt", "man.txt", "woman.txt" }, g_game.asset_manifest) || !load_declared_assets()
        || !xtet::load_scene_description(g_game.archive, { "base_scr.txt", "man.txt", "woman.txt" }, g_game.scene)
        || !xtet::load_rli_animations(g_game.archive, { "m.rli", "rm.rli", "w.rli", "rw.rli" }, g_game.animations) || !initialize_audio() || !render_initial_frame())
    {
        fail_initialization();
        return;
    }

    g_game.figurine_geometry = xtet::build_figurine_geometry_tables();
    const std::vector<const xtet::SceneNode *> homes = xtet::find_scene_links(g_game.scene, "home_scr");
    if(homes.size() != 1 || homes[0]->children.size() != 3 || homes[0]->children[1].children.size() != 117
        || !g_game.gameplay_runtime.initialize(15, homes[0]->children[1].children.size() - 14, { 0, 1, 1, 1 }) || !initialize_worker())
    {
        fail_initialization();
        return;
    }
    std::srand((unsigned int)std::time(nullptr));
    g_game.initialized = true;
    g_game.worker.setEnabled(true);
}

extern "C" std::uint32_t XTET_ABI GAME_DLL_WND_PROC(HWND, UINT message, WPARAM wparam, LPARAM)
{
    // The original ordinal checks inactive gameplay state before entering its
    // recursive Win32 critical section. This also lets synchronous result or
    // failure reporting re-enter through the host without blocking on state
    // synchronization owned by the reporting thread.
    if(!g_game.initialized.load())
        return 1;
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if(!g_game.initialized.load())
        return 1;
    xtet::GameWindowMessageCallbacks callbacks;
    callbacks.destroy = []() { stop_gameplay(); };
    callbacks.key_down = [](std::uint32_t key) { dispatch_key_down(key); };
    callbacks.mouse_button = [](bool pressed) { handle_mouse_button(pressed); };
    return xtet::dispatch_game_window_message(g_game.gameplay_runtime.progress().gameplay_state, message, (std::uint32_t)wparam, callbacks);
}

extern "C" void XTET_ABI GAME_DLL_EXEC(std::uint32_t command)
{
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    switch(command)
    {
    case 1:
        if(!g_game.window)
            return;
        stop_gameplay();
        post_game_result(g_game.gameplay_runtime.progress().score);
        post_game_termination();
        break;
    case 2:
        if(xtet::set_game_paused(g_game.gameplay_runtime.progress().gameplay_state, true, [](bool playing) { return g_game.audio.setLoopPlaying(playing); }))
        {
            g_game.worker.setEnabled(false);
            present_control_overlay(3, true);
        }
        break;
    case 4:
        if(xtet::set_game_paused(g_game.gameplay_runtime.progress().gameplay_state, false, [](bool playing) { return g_game.audio.setLoopPlaying(playing); }))
        {
            present_control_overlay(3, false);
            g_game.worker.setEnabled(true);
        }
        break;
    case 0x10:
        g_game.audio_enabled = false;
        break;
    case 0x20:
        g_game.audio_enabled = true;
        break;
    case 0x40:
        g_game.audio.setLoopPlaying(false);
        break;
    case 0x80:
        g_game.audio.setLoopPlaying(true);
        break;
    default:
        break;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if(reason == DLL_PROCESS_ATTACH)
    {
        g_game.module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
