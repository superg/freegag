#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <stdint.h>
#include <vector>
#include "action_definitions.h"
#include "asset_decoders.h"
#include "asset_manifest.h"
#include "audio_coordinator.h"
#include "figurine_geometry.h"
#include "game_progress.h"
#include "game_runtime.h"
#include "game_timing.h"
#include "game_worker.h"
#include "gameplay_state.h"
#include "indexed_renderer.h"
#include "match_animation.h"
#include "rli_animation.h"
#include "runtime_tables.h"
#include "scene_description.h"
#include "sfs_archive.h"

namespace
{

void count_scene_nodes(const xtet::SceneNode &node, std::array<size_t, 5> &counts)
{
    ++counts[(size_t)node.type];
    for(const xtet::SceneNode &child : node.children)
        count_scene_nodes(child, counts);
}

} // namespace

int main(int argc, char **argv)
{
    if(argc != 2)
        return 2;
    std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
    if(!input)
        return 3;
    const std::streamoff size = input.tellg();
    if(size <= 0)
        return 4;
    std::vector<uint8_t> archive_bytes((size_t)size);
    input.seekg(0);
    if(!input.read((char *)archive_bytes.data(), size))
        return 5;

    std::vector<uint8_t> executable_archive_bytes;
    std::string executable_archive_error;
    if(!xtet::load_executable_sfs("XTETDLL.SFS", executable_archive_bytes, executable_archive_error) || executable_archive_bytes != archive_bytes || !executable_archive_error.empty())
        return 114;

    xtet::SfsArchive archive;
    if(!archive.mount({ archive_bytes.data(), archive_bytes.size() }))
        return 6;
    const xtet::SfsEntry *entry = archive.find("acts.txt");
    if(!entry || entry->hash_a != 0xfb32894c || entry->hash_b != 0xa8cf14d6 || entry->virtual_offset != 0x302678 || entry->size != 0x3171)
        return 7;
    std::vector<uint8_t> contents;
    if(!archive.read(*entry, contents) || contents.size() != entry->size)
        return 8;
    std::vector<xtet::ActionDefinition> definitions;
    if(!xtet::parse_action_definitions(contents, definitions) || definitions.size() != 548)
        return 9;
    if(archive.find("missing.file") || archive.find("acts.txt.extra"))
        return 10;
    xtet::AssetManifest manifest;
    if(!xtet::load_asset_manifest(archive, { "base_scr.txt", "man.txt", "woman.txt" }, manifest))
        return 12;
    if(manifest.script_paths.size() != 7 || manifest.bitmap_paths.size() != 63 || manifest.wave_paths.size() != 18)
        return 13;
    xtet::SceneDescription scene;
    if(!xtet::load_scene_description(archive, { "base_scr.txt", "man.txt", "woman.txt" }, scene))
        return 21;
    std::array<size_t, 5> scene_counts{};
    for(const xtet::SceneNode &root : scene.roots)
        count_scene_nodes(root, scene_counts);
    if(scene.roots.size() != 22 || scene_counts != std::array<size_t, 5>{ 7, 69, 71, 18, 104 })
        return 22;
    const xtet::SceneNode &home = scene.roots[0];
    const xtet::SceneNode &digit = scene.roots[1];
    const xtet::SceneNode &man = scene.roots[20];
    const xtet::SceneNode &woman = scene.roots[21];
    if(home.type != xtet::SceneNodeType::sprites || home.links != std::vector<std::string>{ "home_scr" } || !home.size || home.size->x != 640 || home.size->y != 480 || home.children.size() != 3
        || !home.children[1].position || home.children[1].position->x != 243 || home.children[1].position->y != 77 || home.children[1].children.size() != 117
        || home.children[1].children[111].children.size() != 1 || home.children[1].children[111].children[0].loaded_path != "over.bmp" || home.children[1].children[112].children.size() != 1
        || home.children[1].children[112].children[0].loaded_path != "wind.bmp" || home.children[2].children.size() != 8)
        return 23;
    if(digit.type != xtet::SceneNodeType::bitmap || digit.loaded_path != "digit.bmp" || digit.links != std::vector<std::string>{ "pal_none", "map_digit" })
        return 24;
    if(man.links != std::vector<std::string>{ "man" } || man.children.size() != 10 || woman.links != std::vector<std::string>{ "woman" } || woman.children.size() != 20)
        return 25;
    if(xtet::find_scene_links(scene, "home_scr").size() != 1 || xtet::find_scene_links(scene, "map_digit").size() != 1 || xtet::find_scene_links(scene, "loop").size() != 8
        || xtet::find_scene_links(scene, "act").size() != 6 || xtet::find_scene_links(scene, "pal_none").size() != 63 || !xtet::find_scene_links(scene, "missing_link").empty())
        return 26;
    size_t free_figurine_slot = 0;
    if(!xtet::find_free_figurine_scene_slot(6, {}, free_figurine_slot) || free_figurine_slot != 3)
        return 103;
    int used_slot_marker = 0;
    std::vector<xtet::FigurineBoardEntry> used_slot_entries{
        { &used_slot_marker, nullptr, 3 }
    };
    if(!xtet::find_free_figurine_scene_slot(6, used_slot_entries, free_figurine_slot) || free_figurine_slot != 4)
        return 104;
    size_t decoded_bitmaps = 0;
    size_t decoded_waves = 0;
    std::map<std::string, xtet::IndexedBitmap> decoded_bitmap_assets;
    std::map<std::string, xtet::WavePcm> decoded_wave_assets;
    for(const std::string &path : manifest.bitmap_paths)
    {
        std::vector<uint8_t> asset_bytes;
        xtet::IndexedBitmap bitmap;
        if(!archive.read(path, asset_bytes) || !xtet::decode_indexed_bitmap(asset_bytes, bitmap) || bitmap.pixels.size() != (size_t)bitmap.width * bitmap.height)
            return 14;
        ++decoded_bitmaps;
        decoded_bitmap_assets.emplace(path, bitmap);
        if(path == "xtet.bmp" && (bitmap.width != 640 || bitmap.height != 480))
            return 15;
    }
    const xtet::SceneNode &controls = home.children[2];
    if(xtet::map_scaled_cursor_coordinate(319, 640, 640) != 319 || xtet::map_scaled_cursor_coordinate(0, 1280, 640) != 0 || xtet::map_scaled_cursor_coordinate(1279, 1280, 640) != 639
        || xtet::map_scaled_cursor_coordinate(719, 1440, 640) != 319 || xtet::map_scaled_cursor_coordinate(1079, 1080, 480) != 479 || xtet::map_scaled_cursor_coordinate(-1, 1280, 640) != -1
        || xtet::map_scaled_cursor_coordinate(1280, 1280, 640) != 1280)
        return 138;
    for(size_t index = 0; index < controls.children.size(); ++index)
    {
        const xtet::SceneNode &control = controls.children[index];
        if(control.type != xtet::SceneNodeType::sprite_bitmap || !control.position || control.children.size() != 1 || control.children[0].type != xtet::SceneNodeType::bitmap)
            return 134;
        const auto bitmap = decoded_bitmap_assets.find(control.children[0].loaded_path);
        if(bitmap == decoded_bitmap_assets.end())
            return 135;
        const auto opaque_pixel = std::find_if(bitmap->second.pixels.begin(), bitmap->second.pixels.end(), [](uint8_t pixel) { return pixel != 0; });
        if(opaque_pixel == bitmap->second.pixels.end())
            return 136;
        const size_t pixel_offset = (size_t)(opaque_pixel - bitmap->second.pixels.begin());
        const int32_t hit_x = control.position->x + (int32_t)(pixel_offset % bitmap->second.width);
        const int32_t hit_y = control.position->y + (int32_t)(pixel_offset / bitmap->second.width);
        if(xtet::hit_test_sprite_collection(controls, decoded_bitmap_assets, hit_x, hit_y) != (int)index)
            return 137;
    }
    for(const std::string &path : manifest.wave_paths)
    {
        std::vector<uint8_t> asset_bytes;
        xtet::WavePcm wave;
        if(!archive.read(path, asset_bytes) || !xtet::decode_wave_pcm(asset_bytes, wave) || wave.samples.empty())
        {
            std::cout << "wave decode failed: " << path << " bytes=" << asset_bytes.size() << '\n';
            return 16;
        }
        ++decoded_waves;
        decoded_wave_assets.emplace(path, std::move(wave));
    }
    uint32_t next_sound_handle = 1;
    std::vector<uint32_t> destroyed_sound_handles;
    std::vector<bool> queued_replace_flags;
    size_t stop_count = 0;
    size_t start_count = 0;
    const xtet::AudioHostCallbacks audio_callbacks{ [&next_sound_handle](const xtet::PcmFormat *) { return next_sound_handle++; },
        [&destroyed_sound_handles](uint32_t handle) { destroyed_sound_handles.push_back(handle); },
        [&queued_replace_flags](uint32_t, const void *samples, uint32_t size, bool replace)
        {
            if(samples == nullptr || size == 0)
                return false;
            queued_replace_flags.push_back(replace);
            return true;
        },
        [&stop_count](uint32_t, bool)
        {
            ++stop_count;
            return true;
        },
        [&start_count](uint32_t, bool)
        {
            ++start_count;
            return true;
        } };
    xtet::AudioCoordinator audio;
    if(!audio.initialize(scene, decoded_wave_assets, audio_callbacks) || !audio.valid() || next_sound_handle != 7 || !audio.initializeLoopQueue() || queued_replace_flags.size() != 2402
        || !queued_replace_flags[0] || std::any_of(queued_replace_flags.begin() + 1, queued_replace_flags.end(), [](bool replace) { return replace; }) || stop_count != 1
        || !audio.queueRandom("act", 7) || queued_replace_flags.size() != 2403 || !queued_replace_flags.back() || !audio.queueFirst("win") || !audio.setLoopPlaying(true)
        || !audio.setLoopPlaying(false) || start_count != 1 || stop_count != 2)
        return 107;
    audio.destroy();
    if(destroyed_sound_handles.size() != 6 || audio.valid())
        return 108;
    std::vector<uint8_t> malformed_asset;
    xtet::IndexedBitmap malformed_bitmap;
    if(!archive.read("xtet.bmp", malformed_asset))
        return 17;
    malformed_asset[0] = 0;
    if(xtet::decode_indexed_bitmap(malformed_asset, malformed_bitmap))
        return 18;
    xtet::WavePcm malformed_wave;
    if(!archive.read("act1.wav", malformed_asset))
        return 19;
    malformed_asset[4] = 0xff;
    malformed_asset[5] = 0xff;
    malformed_asset[6] = 0xff;
    malformed_asset[7] = 0xff;
    if(xtet::decode_wave_pcm(malformed_asset, malformed_wave))
        return 20;
    std::vector<xtet::RliAnimation> animations;
    if(!xtet::load_rli_animations(archive, { "m.rli", "rm.rli", "w.rli", "rw.rli" }, animations) || animations.size() != 4)
        return 31;
    const std::array<size_t, 4> animation_sizes{ 40048, 38598, 57538, 56818 };
    const std::array<size_t, 4> animation_record_counts{ 40, 40, 51, 51 };
    for(size_t index = 0; index < animations.size(); ++index)
        if(animations[index].resident_bytes.size() != animation_sizes[index] || animations[index].frame_records.size() != animation_record_counts[index] || animations[index].width != 108
            || animations[index].height != 108 || animations[index].flags != 0x1000)
            return 32;
    const std::array<uint64_t, 4> animation_pixel_hashes{ 2783471652202405095ull, 2892418359157948333ull, 1871060767316094678ull, 6540132390339871046ull };
    for(size_t animation_index = 0; animation_index < animations.size(); ++animation_index)
    {
        const xtet::RliAnimation &animation = animations[animation_index];
        size_t raw_frames = 0;
        size_t rle_frames = 0;
        size_t palette_frames = 0;
        size_t pixel_frames = 0;
        uint64_t pixel_hash = 14695981039346656037ull;
        for(size_t index = 0; index < animation.frame_records.size(); ++index)
        {
            raw_frames += (animation.frame_records[index].flags & 1) != 0;
            rle_frames += (animation.frame_records[index].flags & 2) != 0;
            palette_frames += (animation.frame_records[index].flags & 4) != 0;
            pixel_frames += !animation.frame_records[index].pixels.empty();
            for(uint8_t pixel : animation.frame_records[index].pixels)
            {
                pixel_hash ^= pixel;
                pixel_hash *= 1099511628211ull;
            }
        }
        if(raw_frames != 0 || rle_frames != animation.frame_records.size() || palette_frames != 0 || pixel_frames != rle_frames || pixel_hash != animation_pixel_hashes[animation_index])
            return 35;
    }
    std::vector<uint8_t> malformed_rli = animations[0].resident_bytes;
    xtet::RliAnimation rejected_animation;
    malformed_rli[0] = 0;
    if(xtet::decode_rli_animation("m.rli", malformed_rli, rejected_animation))
        return 33;
    malformed_rli = animations[0].resident_bytes;
    malformed_rli[2] = 0xff;
    malformed_rli[3] = 0xff;
    malformed_rli[4] = 0xff;
    malformed_rli[5] = 0x7f;
    if(xtet::decode_rli_animation("m.rli", malformed_rli, rejected_animation))
        return 34;
    xtet::RuntimeTables runtime_tables;
    if(!runtime_tables.initialize(15) || runtime_tables.slotCount() != 15 || runtime_tables.tables().size() != 20)
        return 39;
    for(const std::vector<void *> &table : runtime_tables.tables())
        if(table.size() != 15 || std::any_of(table.begin(), table.end(), [](void *value) { return value != nullptr; }))
            return 40;
    runtime_tables.clear();
    if(runtime_tables.slotCount() != 0 || std::any_of(runtime_tables.tables().begin(), runtime_tables.tables().end(), [](const std::vector<void *> &table) { return !table.empty(); }))
        return 41;
    const xtet::FigurineGeometryTables figurine_geometry = xtet::build_figurine_geometry_tables();
    if(figurine_geometry.first_family.size() != 5 || figurine_geometry.second_family.size() != 10)
        return 42;
    const std::array<xtet::FigurineOffset, 8> zero_offsets{};
    const std::array<xtet::FigurineOffset, 8> one_offsets{
        xtet::FigurineOffset{ -1, 0  },
        { 0,  -1 },
        { 1,  0  },
        { 0,  1  },
        { 0,  1  },
        { -1, 0  },
        { 0,  -1 },
        { 1,  0  }
    };
    const std::array<xtet::FigurineOffset, 8> reverse_offsets{
        xtet::FigurineOffset{ 0,  1  },
        { -1, 0  },
        { 0,  -1 },
        { 1,  0  },
        { -1, 0  },
        { 0,  -1 },
        { 1,  0  },
        { 0,  1  }
    };
    const std::array<xtet::FigurineOffset, 8> two_offsets{
        xtet::FigurineOffset{ -2, 0  },
        { 0,  -2 },
        { 2,  0  },
        { 0,  2  },
        { 0,  2  },
        { -2, 0  },
        { 0,  -2 },
        { 2,  0  }
    };
    const std::array<std::array<xtet::FigurineOffset, 8>, 10> expected_geometry{ zero_offsets, one_offsets, reverse_offsets, one_offsets, two_offsets, two_offsets, one_offsets, two_offsets,
        one_offsets, one_offsets };
    if(figurine_geometry.second_family != expected_geometry || !std::equal(figurine_geometry.first_family.begin(), figurine_geometry.first_family.end(), expected_geometry.begin()))
        return 43;
    const std::array<uint32_t, 10> tick_intervals{ 300, 200, 150, 135, 125, 117, 110, 105, 100, 90 };
    for(uint32_t level = 1; level <= tick_intervals.size(); ++level)
        if(xtet::get_game_tick_interval(level) != tick_intervals[level - 1])
            return 46;
    if(xtet::get_game_tick_interval(0) != 500 || xtet::get_game_tick_interval(11) != 500)
        return 47;
    if(!runtime_tables.initialize(15))
        return 49;
    int32_t family_balance = 0;
    xtet::FallingFigurine falling_figurine = xtet::select_falling_figurine(0, 17, 1, family_balance, runtime_tables.slotCount());
    if(falling_figurine.first_family || falling_figurine.shape_index != 7 || falling_figurine.orientation != 1 || falling_figurine.column != 7 || falling_figurine.row != 2 || family_balance != -1
        || !xtet::can_place_figurine(falling_figurine, runtime_tables))
        return 50;
    family_balance = -3;
    const xtet::FallingFigurine forced_first_family = xtet::select_falling_figurine(0, 7, 0, family_balance, runtime_tables.slotCount());
    if(!forced_first_family.first_family || forced_first_family.shape_index != 2 || forced_first_family.orientation != -1 || family_balance != -2)
        return 51;
    family_balance = 3;
    const xtet::FallingFigurine forced_second_family = xtet::select_falling_figurine(1, 7, 0, family_balance, runtime_tables.slotCount());
    if(forced_second_family.first_family || forced_second_family.shape_index != 7 || family_balance != 2)
        return 52;
    xtet::RuntimeTables spawn_board;
    if(!spawn_board.initialize(15))
        return 110;
    int spawn_marker = 0;
    xtet::FallingFigurine spawned_figurine;
    std::vector<xtet::FigurineBoardEntry> spawn_entries;
    std::vector<bool> spawn_phases;
    family_balance = 0;
    if(!xtet::spawn_falling_figurine(1, 7, 0, family_balance, spawn_board, spawn_entries, spawned_figurine, &spawn_marker, 6,
           [&spawn_phases](const xtet::FallingFigurine &, bool adding) { spawn_phases.push_back(adding); })
        || spawn_entries.size() != 1 || spawn_entries[0].value != &spawn_marker || spawn_entries[0].figurine != &spawned_figurine || spawn_entries[0].scene_slot != 3 || spawned_figurine.row != 2
        || spawned_figurine.previous_orientation != spawned_figurine.orientation || spawned_figurine.previous_column != spawned_figurine.column || spawned_figurine.previous_row != spawned_figurine.row
        || spawn_phases != std::vector<bool>{ true })
        return 111;
    xtet::RuntimeTables blocked_spawn_board;
    if(!blocked_spawn_board.initialize(15))
        return 112;
    int blocked_marker = 0;
    for(size_t row = 0; row < blocked_spawn_board.tables().size(); ++row)
        for(size_t column = 0; column < blocked_spawn_board.slotCount(); ++column)
            if(!blocked_spawn_board.set(row, column, &blocked_marker))
                return 113;
    int failed_spawn_marker = 0;
    xtet::FallingFigurine failed_spawn_figurine;
    std::vector<xtet::FigurineBoardEntry> blocked_spawn_entries;
    family_balance = 0;
    if(xtet::spawn_falling_figurine(0, 17, 1, family_balance, blocked_spawn_board, blocked_spawn_entries, failed_spawn_figurine, &failed_spawn_marker, 6) || !blocked_spawn_entries.empty()
        || family_balance != -1)
        return 114;
    xtet::GameplayRuntime gameplay_runtime;
    xtet::GameProgress runtime_progress;
    runtime_progress.base_level = 1;
    runtime_progress.level = 1;
    runtime_progress.gameplay_state = 1;
    std::vector<bool> runtime_phases;
    xtet::GameTickResult runtime_tick_result;
    xtet::CascadeResult runtime_cascade_result;
    if(!gameplay_runtime.initialize(15, 6, runtime_progress)
        || !gameplay_runtime.updateTick(
            1, 7, 0, 100, figurine_geometry, definitions, [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return true; },
            [&runtime_phases](const xtet::FallingFigurine &, bool adding) { runtime_phases.push_back(adding); }, runtime_tick_result, runtime_cascade_result)
        || runtime_tick_result != xtet::GameTickResult::spawned || gameplay_runtime.activeValue() == nullptr || gameplay_runtime.entries().size() != 1 || runtime_phases != std::vector<bool>{ true })
        return 115;
    runtime_phases.clear();
    if(!gameplay_runtime.updateTick(
           0, 0, 0, 100, figurine_geometry, definitions, [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return true; },
           [&runtime_phases](const xtet::FallingFigurine &, bool adding) { runtime_phases.push_back(adding); }, runtime_tick_result, runtime_cascade_result)
        || runtime_tick_result != xtet::GameTickResult::moved || gameplay_runtime.activeValue() == nullptr || runtime_phases != std::vector<bool>{ false, true })
        return 116;
    runtime_phases.clear();
    uint32_t runtime_drain_count = 0;
    xtet::GameplayInputOutcome runtime_input_outcome;
    if(!gameplay_runtime.handleInput(
           xtet::GameplayInput::hard_drop, figurine_geometry, definitions, [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return true; },
           [&runtime_drain_count]() { ++runtime_drain_count; }, [&runtime_phases](const xtet::FallingFigurine &, bool adding) { runtime_phases.push_back(adding); }, runtime_input_outcome,
           runtime_cascade_result)
        || runtime_input_outcome.result != xtet::GameplayInputResult::rejected || runtime_input_outcome.moves == 0 || gameplay_runtime.activeValue() == nullptr || runtime_drain_count != 0
        || runtime_phases.size() != (size_t)runtime_input_outcome.moves * 2)
        return 117;
    runtime_phases.clear();
    if(!gameplay_runtime.updateTick(
           0, 0, 0, 100, figurine_geometry, definitions, [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return true; },
           [&runtime_phases](const xtet::FallingFigurine &, bool adding) { runtime_phases.push_back(adding); }, runtime_tick_result, runtime_cascade_result)
        || runtime_tick_result != xtet::GameTickResult::settled || gameplay_runtime.activeValue() != nullptr || !runtime_phases.empty() || gameplay_runtime.entries().size() != 1)
        return 118;
    uint32_t dispatched_destroy_count = 0;
    uint32_t dispatched_key = 0;
    std::vector<bool> dispatched_mouse_buttons;
    const xtet::GameWindowMessageCallbacks message_callbacks{ [&dispatched_destroy_count]() { ++dispatched_destroy_count; }, [&dispatched_key](uint32_t key) { dispatched_key = key; },
        [&dispatched_mouse_buttons](bool pressed) { dispatched_mouse_buttons.push_back(pressed); } };
    if(xtet::dispatch_game_window_message(0, 0x100, 0x25, message_callbacks) != 1 || xtet::dispatch_game_window_message(5, 0x100, 0x25, message_callbacks) != 1 || dispatched_key != 0
        || xtet::dispatch_game_window_message(1, 0x100, 0x27, message_callbacks) != 0 || dispatched_key != 0x27 || xtet::dispatch_game_window_message(2, 0x0002, 0, message_callbacks) != 0
        || dispatched_destroy_count != 1 || xtet::dispatch_game_window_message(3, 0x0201, 0, message_callbacks) != 0 || xtet::dispatch_game_window_message(4, 0x0202, 0, message_callbacks) != 0
        || dispatched_mouse_buttons != std::vector<bool>{ true, false } || xtet::dispatch_game_window_message(1, 0x0200, 0, message_callbacks) != 0)
        return 119;
    std::vector<std::string> key_events;
    const xtet::GameKeyDownCallbacks key_callbacks{ [&key_events]() { key_events.push_back("stop"); }, [&key_events](uint32_t score) { key_events.push_back("result:" + std::to_string(score)); },
        [&key_events]() { key_events.push_back("terminate"); }, [&key_events](uint32_t key) { key_events.push_back("key:" + std::to_string(key)); },
        [&key_events]() { key_events.push_back("drain"); } };
    uint32_t key_state = 1;
    xtet::handle_game_key_down(key_state, 0x25, 0, 0, 42, key_callbacks);
    if(key_state != 1 || key_events != std::vector<std::string>{ "key:37" })
        return 120;
    key_events.clear();
    xtet::handle_game_key_down(key_state, 0x1b, 0, 0, 42, key_callbacks);
    if(key_state != 0 || key_events != std::vector<std::string>{ "stop", "result:42", "terminate" })
        return 121;
    key_events.clear();
    key_state = 2;
    xtet::handle_game_key_down(key_state, 0x1b, 99, 100, 42, key_callbacks);
    if(key_state != 2 || key_events != std::vector<std::string>{ "drain" })
        return 122;
    key_events.clear();
    key_state = 3;
    xtet::handle_game_key_down(key_state, 0x20, 100, 100, 42, key_callbacks);
    if(key_state != 0 || key_events != std::vector<std::string>{ "stop", "result:42", "terminate" })
        return 123;
    key_events.clear();
    key_state = 4;
    xtet::handle_game_key_down(key_state, 0x20, 0, 0, 42, key_callbacks);
    if(key_state != 4 || !key_events.empty())
        return 124;
    std::vector<bool> loop_playing_events;
    const auto loop_playing_callback = [&loop_playing_events](bool playing)
    {
        loop_playing_events.push_back(playing);
        return true;
    };
    key_state = 1;
    if(!xtet::set_game_paused(key_state, true, loop_playing_callback) || key_state != 4 || loop_playing_events != std::vector<bool>{ false }
        || !xtet::set_game_paused(key_state, false, loop_playing_callback) || key_state != 1 || loop_playing_events != std::vector<bool>{ false, true }
        || !xtet::set_game_paused(key_state, false, loop_playing_callback) || loop_playing_events != std::vector<bool>{ false, true })
        return 129;
    key_state = 5;
    if(!xtet::set_game_paused(key_state, false, loop_playing_callback) || key_state != 1 || loop_playing_events != std::vector<bool>{ false, true, true })
        return 130;
    xtet::GameWorker game_worker;
    std::mutex worker_test_mutex;
    std::condition_variable worker_test_condition;
    uint32_t worker_tick_count = 0;
    if(!game_worker.start([]() { return 1; },
           [&]()
           {
               std::lock_guard<std::mutex> lock(worker_test_mutex);
               ++worker_tick_count;
               worker_test_condition.notify_all();
           })
        || !game_worker.running())
        return 131;
    game_worker.setEnabled(true);
    {
        std::unique_lock<std::mutex> lock(worker_test_mutex);
        if(!worker_test_condition.wait_for(lock, std::chrono::seconds(1), [&worker_tick_count]() { return worker_tick_count != 0; }))
            return 132;
    }
    game_worker.stop();
    if(game_worker.running())
        return 133;
    xtet::GameplayRuntime failed_runtime;
    if(!failed_runtime.initialize(15, 6, runtime_progress))
        return 125;
    int failed_runtime_blocker = 0;
    for(size_t row = 0; row < failed_runtime.board().tables().size(); ++row)
        for(size_t column = 0; column < failed_runtime.board().slotCount(); ++column)
            if(!failed_runtime.board().set(row, column, &failed_runtime_blocker))
                return 126;
    if(!failed_runtime.updateTick(
           0, 17, 1, 0xfffffff0, figurine_geometry, definitions, [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return true; }, {},
           runtime_tick_result, runtime_cascade_result)
        || runtime_tick_result != xtet::GameTickResult::spawn_failed || failed_runtime.progress().gameplay_state != 2
        || failed_runtime.resultInputDeadline() != xtet::calculate_result_input_deadline(0xfffffff0))
        return 127;
    failed_runtime.stop();
    if(failed_runtime.progress().gameplay_state != 0 || failed_runtime.activeValue() != nullptr || !failed_runtime.entries().empty() || failed_runtime.board().slotCount() != 0
        || !std::all_of(failed_runtime.board().tables().begin(), failed_runtime.board().tables().end(), [](const std::vector<void *> &table) { return table.empty(); }))
        return 128;
    xtet::FigurineTemplate falling_shape;
    if(!xtet::get_oriented_figurine_template(falling_figurine.first_family, falling_figurine.shape_index, falling_figurine.orientation, falling_shape))
        return 53;
    bool collision_installed = false;
    int collision_marker = 0;
    for(int shape_row = 0; shape_row < 5 && !collision_installed; ++shape_row)
        for(int shape_column = 0; shape_column < 5 && !collision_installed; ++shape_column)
            if(falling_shape[(size_t)shape_row * 5 + shape_column] != 0)
            {
                const size_t board_row = (size_t)(falling_figurine.row + shape_row - 2);
                const size_t board_column = (size_t)(falling_figurine.column + shape_column - 2);
                collision_installed = runtime_tables.set(board_row, board_column, &collision_marker);
            }
    if(!collision_installed || xtet::can_place_figurine(falling_figurine, runtime_tables) || !xtet::can_place_figurine(falling_figurine, runtime_tables, &collision_marker))
        return 54;
    falling_figurine.row = -2;
    if(!xtet::can_place_figurine(falling_figurine, runtime_tables, &collision_marker))
        return 55;
    falling_figurine.row = 2;
    xtet::FigurineSpriteSelection sprite_selection;
    if(!xtet::select_figurine_sprite(falling_figurine, sprite_selection) || sprite_selection.family != xtet::FigurineSpriteFamily::woman || sprite_selection.frame_index != 7
        || sprite_selection.mirror_horizontal || sprite_selection.mirror_vertical || sprite_selection.x != 74 || sprite_selection.y != -11)
        return 56;
    xtet::FallingFigurine man_figurine = forced_first_family;
    man_figurine.orientation = -2;
    if(!xtet::select_figurine_sprite(man_figurine, sprite_selection) || sprite_selection.family != xtet::FigurineSpriteFamily::man || sprite_selection.frame_index != 7
        || sprite_selection.mirror_horizontal || !sprite_selection.mirror_vertical)
        return 57;
    const std::array<std::array<bool, 2>, 8> expected_mirroring{
        std::array<bool, 2>{ true,  false },
         { false, true  },
         { false, true  },
         { true,  false },
         { false, false },
         { false, false },
         { true,  true  },
         { true,  true  }
    };
    const std::array<int8_t, 8> orientations{ -4, -3, -2, -1, 1, 2, 3, 4 };
    for(size_t index = 0; index < orientations.size(); ++index)
    {
        man_figurine.orientation = orientations[index];
        if(!xtet::select_figurine_sprite(man_figurine, sprite_selection) || sprite_selection.mirror_horizontal != expected_mirroring[index][0]
            || sprite_selection.mirror_vertical != expected_mirroring[index][1])
            return 58;
    }
    falling_figurine.orientation = -3;
    if(!xtet::select_figurine_sprite(falling_figurine, sprite_selection))
        return 59;
    std::vector<uint8_t> figurine_pixels(640 * 480, 0x55);
    std::vector<uint8_t> expected_figurine_pixels = figurine_pixels;
    xtet::FigurineRenderRegion figurine_region;
    const xtet::IndexedBitmap &woman_frame = decoded_bitmap_assets.at("w8_.bmp");
    if(!xtet::blit_transparent(woman_frame, { expected_figurine_pixels.data(), 640, 480, 640 }, 317, 66, false, true))
        return 60;
    for(size_t y = 0; y < 480; ++y)
        for(size_t x = 0; x < 640; ++x)
            if(x < 243 || x >= 498 || y < 77 || y >= 417)
                expected_figurine_pixels[y * 640 + x] = 0x55;
    if(!xtet::render_figurine_sprite(sprite_selection, scene, decoded_bitmap_assets, { figurine_pixels.data(), 640, 480, 640 }, figurine_region) || figurine_pixels != expected_figurine_pixels
        || figurine_region.x != 317 || figurine_region.y != 77 || figurine_region.width != woman_frame.width || figurine_region.height != woman_frame.height - 11)
        return 60;
    std::vector<xtet::FigurineRenderRegion> board_regions;
    if(!xtet::collect_figurine_board_regions(falling_figurine, 160, 120, board_regions) || board_regions.empty())
        return 99;
    for(size_t index = 0; index < board_regions.size(); ++index)
    {
        const xtet::FigurineRenderRegion &region = board_regions[index];
        if(region.width == 0 || region.height == 0 || region.width > 17 || region.height > 17 || region.x % 17 != 0 || region.y % 17 != 0
            || (index != 0 && (region.y < board_regions[index - 1].y || (region.y == board_regions[index - 1].y && region.x <= board_regions[index - 1].x))))
            return 100;
    }
    std::vector<bool> board_region_phases;
    const xtet::FigurineBoardChangeCallback board_region_callback =
        xtet::make_figurine_board_change_callback(160, 120, [&board_region_phases](const xtet::FigurineRenderRegion &, bool adding) { board_region_phases.push_back(adding); });
    if(!board_region_callback)
        return 101;
    board_region_callback(falling_figurine, false);
    board_region_callback(falling_figurine, true);
    if(board_region_phases.size() != board_regions.size() * 2 || !std::all_of(board_region_phases.begin(), board_region_phases.begin() + board_regions.size(), [](bool adding) { return !adding; })
        || !std::all_of(board_region_phases.begin() + board_regions.size(), board_region_phases.end(), [](bool adding) { return adding; }))
        return 102;
    const size_t board_region_area =
        std::accumulate(board_regions.begin(), board_regions.end(), (size_t)0, [](size_t area, const xtet::FigurineRenderRegion &region) { return area + (size_t)region.width * region.height; });
    std::vector<uint8_t> board_change_pixels(160 * 120, 0xaa);
    std::vector<bool> framebuffer_region_phases;
    const xtet::FigurineBoardChangeCallback framebuffer_change_callback = xtet::make_figurine_framebuffer_change_callback({ board_change_pixels.data(), 160, 120, 160 },
        [&framebuffer_region_phases](const xtet::FigurineRenderRegion &, bool adding) { framebuffer_region_phases.push_back(adding); });
    if(!framebuffer_change_callback)
        return 105;
    framebuffer_change_callback(falling_figurine, false);
    if((size_t)std::count(board_change_pixels.begin(), board_change_pixels.end(), (uint8_t)0x13) != board_region_area)
        return 106;
    framebuffer_change_callback(falling_figurine, true);
    if((size_t)std::count(board_change_pixels.begin(), board_change_pixels.end(), (uint8_t)0) != board_region_area || framebuffer_region_phases.size() != board_regions.size() * 2)
        return 107;
    std::vector<uint8_t> presentation_pixels(640 * 480, 0xaa);
    std::vector<uint8_t> expected_presentation_pixels = presentation_pixels;
    std::vector<xtet::FigurineRenderRegion> presentation_board_regions;
    if(!xtet::collect_figurine_board_regions(falling_figurine, 640, 480, presentation_board_regions, 243, 77))
        return 108;
    std::vector<bool> presented_board_phases;
    std::vector<xtet::FigurineRenderRegion> presented_sprite_regions;
    const auto presented_board_callback = [&presented_board_phases](const xtet::FigurineRenderRegion &, bool adding) { presented_board_phases.push_back(adding); };
    const auto presented_sprite_callback = [&presented_sprite_regions](const xtet::FigurineRenderRegion &region) { presented_sprite_regions.push_back(region); };
    if(!xtet::render_figurine_board_change(falling_figurine, false, scene, decoded_bitmap_assets, { presentation_pixels.data(), 640, 480, 640 }, presented_board_callback, presented_sprite_callback)
        || !xtet::render_figurine_board_change(falling_figurine, true, scene, decoded_bitmap_assets, { presentation_pixels.data(), 640, 480, 640 }, presented_board_callback,
            presented_sprite_callback))
        return 108;
    xtet::FigurineSpriteSelection expected_presented_selection;
    xtet::FigurineRenderRegion expected_presented_region;
    if(!xtet::fill_figurine_board_regions(falling_figurine, { expected_presentation_pixels.data(), 640, 480, 640 }, 0, 243, 77)
        || !xtet::select_figurine_sprite(falling_figurine, expected_presented_selection)
        || !xtet::render_figurine_sprite(expected_presented_selection, scene, decoded_bitmap_assets, { expected_presentation_pixels.data(), 640, 480, 640 }, expected_presented_region)
        || presentation_pixels != expected_presentation_pixels || presented_board_phases.size() != presentation_board_regions.size() * 2 || presented_sprite_regions.size() != 1
        || presented_sprite_regions[0].x != expected_presented_region.x || presented_sprite_regions[0].y != expected_presented_region.y
        || presented_sprite_regions[0].width != expected_presented_region.width || presented_sprite_regions[0].height != expected_presented_region.height)
        return 109;
    xtet::RuntimeTables movement_board;
    if(!movement_board.initialize(15))
        return 61;
    int figurine_marker = 0;
    xtet::FallingFigurine moving_figurine = forced_first_family;
    moving_figurine.previous_orientation = moving_figurine.orientation;
    moving_figurine.previous_column = moving_figurine.column;
    moving_figurine.previous_row = moving_figurine.row;
    if(!xtet::place_figurine_on_board(moving_figurine, movement_board, &figurine_marker))
        return 62;
    const auto count_figurine_slots = [&movement_board, &figurine_marker]()
    {
        size_t count = 0;
        for(const std::vector<void *> &table : movement_board.tables())
            count += (size_t)std::count(table.begin(), table.end(), &figurine_marker);
        return count;
    };
    const size_t occupied_slot_count = count_figurine_slots();
    std::vector<bool> board_change_additions;
    std::vector<int8_t> board_change_columns;
    if(occupied_slot_count == 0
        || !xtet::try_move_falling_figurine(moving_figurine, xtet::FigurineMove::right, figurine_geometry, movement_board, &figurine_marker, nullptr,
            [&board_change_additions, &board_change_columns](const xtet::FallingFigurine &figurine, bool adding)
            {
                board_change_additions.push_back(adding);
                board_change_columns.push_back(figurine.column);
            })
        || moving_figurine.column != forced_first_family.column + 1 || moving_figurine.previous_column != moving_figurine.column || count_figurine_slots() != occupied_slot_count
        || board_change_additions != std::vector<bool>{ false, true } || board_change_columns != std::vector<int8_t>{ forced_first_family.column, (int8_t)(forced_first_family.column + 1) })
        return 63;
    const xtet::FallingFigurine before_rotation = moving_figurine;
    const int8_t expected_orientation = before_rotation.orientation == 4  ? 1
                                      : before_rotation.orientation == -4 ? -1
                                      : before_rotation.orientation < 0   ? (int8_t)(before_rotation.orientation - 1)
                                                                          : (int8_t)(before_rotation.orientation + 1);
    const int rotation_offset_index = expected_orientation < 1 ? 3 - expected_orientation : expected_orientation - 1;
    const xtet::FigurineOffset rotation_offset = figurine_geometry.first_family[moving_figurine.shape_index][rotation_offset_index];
    if(!xtet::try_move_falling_figurine(moving_figurine, xtet::FigurineMove::rotate, figurine_geometry, movement_board, &figurine_marker) || moving_figurine.orientation != expected_orientation
        || moving_figurine.column != before_rotation.column + rotation_offset.x || moving_figurine.row != before_rotation.row + rotation_offset.y)
        return 64;
    xtet::RuntimeTables expected_movement_board;
    if(!expected_movement_board.initialize(15) || !xtet::place_figurine_on_board(moving_figurine, expected_movement_board, &figurine_marker)
        || movement_board.tables() != expected_movement_board.tables())
        return 66;
    while(xtet::try_move_falling_figurine(moving_figurine, xtet::FigurineMove::left, figurine_geometry, movement_board, &figurine_marker))
    {
    }
    const xtet::FallingFigurine rejected_position = moving_figurine;
    const size_t rejected_slot_count = count_figurine_slots();
    if(xtet::try_move_falling_figurine(moving_figurine, xtet::FigurineMove::left, figurine_geometry, movement_board, &figurine_marker) || moving_figurine.orientation != rejected_position.orientation
        || moving_figurine.column != rejected_position.column || moving_figurine.row != rejected_position.row || count_figurine_slots() != rejected_slot_count)
        return 65;
    std::vector<xtet::FigurineBoardEntry> movement_entries{
        { &figurine_marker, &moving_figurine }
    };
    xtet::FigurineMatch movement_match;
    if(xtet::process_falling_move(moving_figurine, xtet::FigurineMove::right, figurine_geometry, definitions, movement_board, movement_entries, &figurine_marker, movement_match)
            != xtet::GameplayMoveResult::moved
        || movement_match.candidate != nullptr)
        return 85;
    for(const xtet::ActionDefinition &definition : definitions)
    {
        xtet::FallingFigurine action_man;
        action_man.first_family = true;
        action_man.shape_index = (uint8_t)definition.values[0];
        action_man.orientation = 1;
        action_man.column = 7;
        action_man.row = 8;
        xtet::FallingFigurine action_woman;
        action_woman.shape_index = (uint8_t)definition.values[1];
        action_woman.orientation = definition.values[2];
        action_woman.column = (int8_t)(action_man.column + definition.values[3] - 2);
        action_woman.row = (int8_t)(action_man.row + definition.values[4] - 2);
        const xtet::ActionDefinition *matched_action = xtet::find_matching_action(action_man, action_woman, definitions);
        if(matched_action == nullptr || !std::equal(matched_action->values.begin(), matched_action->values.begin() + 5, definition.values.begin())
            || xtet::find_matching_action(action_man, action_man, definitions) != nullptr)
            return 67;
        xtet::MatchAnimationPlan animation_plan;
        if(!xtet::build_match_animation_plan(action_man, action_woman, definition, false, animation_plan))
            return 71;
        const auto animation_index = [](xtet::MatchAnimationResource resource) {
            return resource == xtet::MatchAnimationResource::man ? 0u : resource == xtet::MatchAnimationResource::rotated_man ? 1u : resource == xtet::MatchAnimationResource::woman ? 2u : 3u;
        };
        if((size_t)(animation_plan.man.first_frame + animation_plan.man.frame_count) > animations[animation_index(animation_plan.man.resource)].frame_records.size()
            || (size_t)(animation_plan.woman.first_frame + animation_plan.woman.frame_count) > animations[animation_index(animation_plan.woman.resource)].frame_records.size())
            return 72;
        if(animation_plan.man.x != action_man.column * 17 - 45 + definition.values[6] || animation_plan.man.y != action_man.row * 17 - 45 + definition.values[7]
            || animation_plan.woman.x != action_woman.column * 17 - 45 + definition.values[8] || animation_plan.woman.y != action_woman.row * 17 - 45 + definition.values[9])
            return 74;
        if(animation_plan.man.temporary_slot_count != 4 || animation_plan.woman.temporary_slot_count != 3)
            return 75;
        xtet::MatchAnimationPlan expanded_animation_plan;
        if(!xtet::build_match_animation_plan(action_man, action_woman, definition, true, expanded_animation_plan) || std::abs(expanded_animation_plan.man.x - animation_plan.man.x) != 1
            || std::abs(expanded_animation_plan.man.y - animation_plan.man.y) != 1 || std::abs(expanded_animation_plan.woman.x - animation_plan.woman.x) != 1
            || std::abs(expanded_animation_plan.woman.y - animation_plan.woman.y) != 1)
            return 73;
    }
    bool match_candidate_tested = false;
    for(const xtet::ActionDefinition &definition : definitions)
    {
        xtet::RuntimeTables match_board;
        if(!match_board.initialize(15))
            return 68;
        xtet::FallingFigurine match_man;
        match_man.first_family = true;
        match_man.shape_index = (uint8_t)definition.values[0];
        match_man.orientation = 1;
        match_man.column = 7;
        match_man.row = 8;
        const int8_t target_man_column = match_man.column;
        xtet::FallingFigurine match_woman;
        match_woman.shape_index = (uint8_t)definition.values[1];
        match_woman.orientation = definition.values[2];
        match_woman.column = (int8_t)(match_man.column + definition.values[3] - 2);
        match_woman.row = (int8_t)(match_man.row + definition.values[4] - 2);
        match_woman.previous_orientation = match_woman.orientation;
        match_woman.previous_column = match_woman.column;
        match_woman.previous_row = match_woman.row;
        match_man.column = (int8_t)(target_man_column - 3);
        match_man.previous_orientation = match_man.orientation;
        match_man.previous_column = match_man.column;
        match_man.previous_row = match_man.row;
        int match_man_marker = 0;
        int match_woman_marker = 0;
        if(!xtet::place_figurine_on_board(match_man, match_board, &match_man_marker) || !xtet::place_figurine_on_board(match_woman, match_board, &match_woman_marker))
            continue;
        std::vector<xtet::FigurineBoardEntry> match_entries{
            { &match_man_marker,   &match_man   },
            { &match_woman_marker, &match_woman }
        };
        const xtet::FigurineMatch match = xtet::find_match_candidate(match_man, xtet::FigurineMove::right, definitions, match_board, match_entries, &match_man_marker);
        if(match.candidate != &match_entries[1] || match.action == nullptr)
            return 69;
        xtet::GameProgress match_progress;
        match_progress.base_level = 1;
        match_progress.level = 1;
        match_progress.gameplay_state = 1;
        std::vector<uint32_t> presented_scores;
        const xtet::ProgressUpdateCallback progress_callback = [&presented_scores](const xtet::GameProgress &progress, const xtet::ProgressUpdate &update)
        {
            if(update.score_changed)
                presented_scores.push_back(progress.score);
        };
        if(!xtet::remove_matched_pair(&match_man_marker, match, match_board, match_entries, match_progress, progress_callback) || !match_entries.empty() || match_progress.score != 2
            || presented_scores != std::vector<uint32_t>{ 1, 2 }
            || std::any_of(match_board.tables().begin(), match_board.tables().end(),
                [](const std::vector<void *> &table) { return std::any_of(table.begin(), table.end(), [](void *value) { return value != nullptr; }); }))
            return 83;
        match_candidate_tested = true;
        break;
    }
    if(!match_candidate_tested)
        return 70;
    if(xtet::translate_gameplay_key(0x20) != xtet::GameplayInput::hard_drop || xtet::translate_gameplay_key(0x25) != xtet::GameplayInput::left
        || xtet::translate_gameplay_key(0x26) != xtet::GameplayInput::rotate || xtet::translate_gameplay_key(0x27) != xtet::GameplayInput::right
        || xtet::translate_gameplay_key(0x28) != xtet::GameplayInput::down || xtet::translate_gameplay_key(0x24) != xtet::GameplayInput::none)
        return 84;
    xtet::RuntimeTables cascade_board;
    if(!cascade_board.initialize(15))
        return 86;
    int cascade_marker = 0;
    xtet::FallingFigurine cascade_figurine = forced_first_family;
    cascade_figurine.previous_orientation = cascade_figurine.orientation;
    cascade_figurine.previous_column = cascade_figurine.column;
    cascade_figurine.previous_row = cascade_figurine.row;
    if(!xtet::place_figurine_on_board(cascade_figurine, cascade_board, &cascade_marker))
        return 87;
    std::vector<xtet::FigurineBoardEntry> cascade_entries{
        { &cascade_marker, &cascade_figurine }
    };
    xtet::GameProgress cascade_progress;
    cascade_progress.base_level = 1;
    cascade_progress.level = 1;
    cascade_progress.gameplay_state = 1;
    uint32_t cascade_callback_count = 0;
    xtet::CascadeResult cascade_result;
    if(!xtet::settle_board_after_match(
           figurine_geometry, definitions, cascade_board, cascade_entries, cascade_progress,
           [&cascade_callback_count](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &)
           {
               ++cascade_callback_count;
               return true;
           },
           cascade_result)
        || cascade_result.moves == 0 || cascade_result.matches != 0 || cascade_callback_count != 0 || cascade_entries.size() != 1 || cascade_progress.score != 0)
        return 88;
    xtet::FallingFigurine below_cascade = cascade_figurine;
    below_cascade.row = (int8_t)(below_cascade.row + 1);
    if(xtet::can_place_figurine(below_cascade, cascade_board, &cascade_marker))
        return 89;
    xtet::RuntimeTables tick_board;
    if(!tick_board.initialize(15))
        return 90;
    int tick_marker = 0;
    xtet::FallingFigurine tick_figurine = forced_first_family;
    tick_figurine.previous_orientation = tick_figurine.orientation;
    tick_figurine.previous_column = tick_figurine.column;
    tick_figurine.previous_row = tick_figurine.row;
    std::vector<xtet::FigurineBoardEntry> tick_entries;
    void *active_value = nullptr;
    xtet::GameProgress tick_progress;
    tick_progress.base_level = 1;
    tick_progress.level = 1;
    tick_progress.gameplay_state = 1;
    uint32_t spawn_count = 0;
    const auto spawn_tick_figurine = [&]() -> void *
    {
        ++spawn_count;
        if(!xtet::place_figurine_on_board(tick_figurine, tick_board, &tick_marker))
            return nullptr;
        tick_entries.push_back({ &tick_marker, &tick_figurine });
        return &tick_marker;
    };
    const auto reject_tick_match = [](const xtet::FallingFigurine &, const xtet::FallingFigurine &, const xtet::ActionDefinition &) { return false; };
    xtet::GameTickResult tick_result;
    xtet::CascadeResult tick_cascade_result;
    if(!xtet::update_game_tick(figurine_geometry, definitions, tick_board, tick_entries, active_value, tick_progress, spawn_tick_figurine, reject_tick_match, tick_result, tick_cascade_result)
        || tick_result != xtet::GameTickResult::spawned || active_value != &tick_marker || spawn_count != 1)
        return 91;
    if(!xtet::update_game_tick(figurine_geometry, definitions, tick_board, tick_entries, active_value, tick_progress, spawn_tick_figurine, reject_tick_match, tick_result, tick_cascade_result)
        || tick_result != xtet::GameTickResult::moved || active_value != &tick_marker || spawn_count != 1)
        return 92;
    uint32_t drained_keyboard_count = 0;
    xtet::GameplayInputOutcome input_outcome;
    std::vector<bool> input_board_changes;
    if(!xtet::handle_gameplay_input(
           xtet::GameplayInput::hard_drop, figurine_geometry, definitions, tick_board, tick_entries, active_value, tick_progress, reject_tick_match, [&drained_keyboard_count]()
           { ++drained_keyboard_count; }, input_outcome, tick_cascade_result, [&input_board_changes](const xtet::FallingFigurine &, bool adding) { input_board_changes.push_back(adding); })
        || input_outcome.result != xtet::GameplayInputResult::rejected || input_outcome.moves == 0 || active_value != &tick_marker || drained_keyboard_count != 0
        || input_board_changes.size() != (size_t)input_outcome.moves * 2)
        return 93;
    for(size_t index = 0; index < input_board_changes.size(); index += 2)
        if(input_board_changes[index] || !input_board_changes[index + 1])
            return 98;
    if(!xtet::update_game_tick(figurine_geometry, definitions, tick_board, tick_entries, active_value, tick_progress, spawn_tick_figurine, reject_tick_match, tick_result, tick_cascade_result))
        return 97;
    if(tick_result != xtet::GameTickResult::settled || tick_entries.size() != 1 || spawn_count != 1 || tick_progress.gameplay_state != 1)
        return 94;
    xtet::RuntimeTables failed_spawn_board;
    if(!failed_spawn_board.initialize(15))
        return 95;
    std::vector<xtet::FigurineBoardEntry> failed_spawn_entries;
    void *failed_active_value = nullptr;
    xtet::GameProgress failed_spawn_progress;
    failed_spawn_progress.gameplay_state = 1;
    if(!xtet::update_game_tick(
           figurine_geometry, definitions, failed_spawn_board, failed_spawn_entries, failed_active_value, failed_spawn_progress, []() -> void * { return nullptr; }, reject_tick_match, tick_result,
           tick_cascade_result)
        || tick_result != xtet::GameTickResult::spawn_failed || failed_spawn_progress.gameplay_state != 2 || failed_active_value != nullptr)
        return 96;
    const xtet::ActionDefinition &rendered_action = definitions.front();
    xtet::FallingFigurine rendered_man;
    rendered_man.first_family = true;
    rendered_man.shape_index = (uint8_t)rendered_action.values[0];
    rendered_man.orientation = 1;
    rendered_man.column = 7;
    rendered_man.row = 8;
    xtet::FallingFigurine rendered_woman;
    rendered_woman.shape_index = (uint8_t)rendered_action.values[1];
    rendered_woman.orientation = rendered_action.values[2];
    rendered_woman.column = (int8_t)(rendered_man.column + rendered_action.values[3] - 2);
    rendered_woman.row = (int8_t)(rendered_man.row + rendered_action.values[4] - 2);
    xtet::MatchAnimationPlan rendered_plan;
    if(!xtet::build_match_animation_plan(rendered_man, rendered_woman, rendered_action, false, rendered_plan))
        return 76;
    std::vector<uint8_t> match_pixels(320 * 240, 0x33);
    std::vector<xtet::FigurineRenderRegion> match_regions;
    if(!xtet::render_match_animation_plan(rendered_plan, animations, { match_pixels.data(), 320, 240, 320 },
           [&match_regions](const xtet::FigurineRenderRegion &region) { match_regions.push_back(region); })
        || match_regions.size() != 7)
        return 77;
    for(size_t index = 0; index < match_regions.size(); ++index)
    {
        const xtet::FigurineRenderRegion &region = match_regions[index];
        if(region.x != match_regions[0].x || region.y != match_regions[0].y || region.width != match_regions[0].width || region.height != match_regions[0].height || region.width == 0
            || region.height == 0)
            return 78;
    }
    if(std::all_of(match_pixels.begin(), match_pixels.end(), [](uint8_t pixel) { return pixel == 0x33; }))
        return 79;
    std::vector<uint8_t> blink_pixels(320 * 240, 0x44);
    std::vector<xtet::FigurineRenderRegion> blink_regions;
    std::vector<uint32_t> blink_delays;
    if(!xtet::render_match_blink_sequence(
           rendered_man, rendered_woman, rendered_action, animations, { blink_pixels.data(), 320, 240, 320 },
           [&blink_regions](const xtet::FigurineRenderRegion &region) { blink_regions.push_back(region); }, [&blink_delays](uint32_t delay) { blink_delays.push_back(delay); })
        || blink_regions.size() != 6 || blink_delays != std::vector<uint32_t>{ 400, 400, 400, 400 }
        || !std::all_of(blink_pixels.begin(), blink_pixels.end(), [](uint8_t pixel) { return pixel == 0x44; }))
        return 80;
    xtet::GameProgress progress;
    progress.score = 28;
    progress.base_level = 1;
    progress.level = 1;
    progress.gameplay_state = 1;
    const xtet::ProgressUpdate first_removal = xtet::update_progress_after_figurine_removal(progress);
    const xtet::ProgressUpdate second_removal = xtet::update_progress_after_figurine_removal(progress);
    if(!first_removal.score_changed || first_removal.level_changed || first_removal.game_over || !second_removal.score_changed || !second_removal.level_changed || second_removal.game_over
        || progress.score != 30 || progress.level != 2 || progress.gameplay_state != 1)
        return 81;
    progress.score = 299;
    progress.level = 10;
    const xtet::ProgressUpdate final_removal = xtet::update_progress_after_figurine_removal(progress);
    if(!final_removal.score_changed || final_removal.level_changed || !final_removal.game_over || progress.score != 300 || progress.level != 10 || progress.gameplay_state != 3
        || xtet::update_progress_after_figurine_removal(progress).score_changed || progress.score != 300)
        return 82;
    std::vector<uint8_t> rendered_pixels(640 * 480, 0xcc);
    const xtet::IndexedFramebuffer framebuffer{ rendered_pixels.data(), 640, 480, 640 };
    if(!xtet::render_initial_scene(scene, decoded_bitmap_assets, framebuffer))
        return 27;
    std::vector<uint8_t> expected_pixels = decoded_bitmap_assets.at("xtet.bmp").pixels;
    const xtet::IndexedBitmap &field = decoded_bitmap_assets.at("f01.bmp");
    if(!xtet::blit_opaque(field, { expected_pixels.data(), 640, 480, 640 }, 243, 77) || rendered_pixels != expected_pixels)
        return 28;
    const xtet::IndexedBitmap &digit_atlas = decoded_bitmap_assets.at("digit.bmp");
    if(!xtet::render_score(0, digit_atlas, framebuffer))
        return 44;
    const uint32_t glyph_width = digit_atlas.width / 4;
    const uint32_t glyph_height = digit_atlas.height / 10;
    for(uint32_t digit_index = 0; digit_index < 4; ++digit_index)
        for(uint32_t row = 0; row < glyph_height; ++row)
            for(uint32_t column = 0; column < glyph_width; ++column)
                expected_pixels[(438 + row) * 640 + 359 + digit_index * glyph_width + column] = digit_atlas.pixels[row * digit_atlas.width + column];
    if(rendered_pixels != expected_pixels)
        return 45;
    std::vector<uint8_t> capped_score_pixels(640 * 480, 0);
    std::vector<uint8_t> maximum_score_pixels(640 * 480, 0);
    if(!xtet::render_score(10000, digit_atlas, { capped_score_pixels.data(), 640, 480, 640 }) || !xtet::render_score(9999, digit_atlas, { maximum_score_pixels.data(), 640, 480, 640 })
        || capped_score_pixels != maximum_score_pixels || xtet::render_score(0, {}, { maximum_score_pixels.data(), 640, 480, 640 }))
        return 48;
    const xtet::IndexedBitmap clipping_source{
        3, 2, {},
          { 1, 2, 3, 4, 5, 6 }
    };
    std::vector<uint8_t> clipped_pixels(12, 0);
    if(!xtet::blit_opaque(clipping_source, { clipped_pixels.data(), 4, 3, 4 }, -1, 1) || clipped_pixels != std::vector<uint8_t>{ 0, 0, 0, 0, 2, 3, 0, 0, 5, 6, 0, 0 })
        return 29;
    if(xtet::blit_opaque(clipping_source, { clipped_pixels.data(), 4, 3, 3 }, 0, 0) || xtet::blit_opaque({}, { clipped_pixels.data(), 4, 3, 4 }, 0, 0))
        return 30;
    const xtet::IndexedBitmap transparent_source{
        3, 2, {},
          { 0, 2, 3, 4, 0, 6 }
    };
    std::vector<uint8_t> transparent_pixels(12, 9);
    if(!xtet::blit_transparent(transparent_source, { transparent_pixels.data(), 4, 3, 4 }, 1, 1) || transparent_pixels != std::vector<uint8_t>{ 9, 9, 9, 9, 9, 9, 2, 3, 9, 4, 9, 6 })
        return 36;
    std::fill(transparent_pixels.begin(), transparent_pixels.end(), (uint8_t)9);
    if(!xtet::blit_transparent(transparent_source, { transparent_pixels.data(), 4, 3, 4 }, -1, 0, true, true) || transparent_pixels != std::vector<uint8_t>{ 9, 4, 9, 9, 2, 9, 9, 9, 9, 9, 9, 9 })
        return 37;
    xtet::RliFrameRecord rli_frame;
    rli_frame.left = 1;
    rli_frame.top = 1;
    rli_frame.right = 2;
    rli_frame.bottom = 2;
    rli_frame.pixels = { 0, 2, 3, 4 };
    rli_frame.coverage = { 1, 0, 1, 1 };
    std::vector<uint8_t> rli_pixels(16, 9);
    if(!xtet::blit_rli_frame(rli_frame, { rli_pixels.data(), 4, 4, 4 }, 0, 0) || rli_pixels != std::vector<uint8_t>{ 9, 9, 9, 9, 9, 0, 9, 9, 9, 3, 4, 9, 9, 9, 9, 9 })
        return 38;
    xtet::RliFrameRecord transparent_rli_frame;
    transparent_rli_frame.right = 1;
    transparent_rli_frame.pixels = { 0, 5 };
    transparent_rli_frame.coverage = { 1, 1 };
    std::vector<uint8_t> transparent_rli_pixels{ 7, 7 };
    if(!xtet::blit_rli_frame_canvas(transparent_rli_frame, 2, 1, { transparent_rli_pixels.data(), 2, 1, 2 }, 0, 0) || transparent_rli_pixels != std::vector<uint8_t>{ 7, 5 })
        return 113;
    std::vector<uint8_t> corrupt_archive = archive_bytes;
    corrupt_archive[0x24] ^= 1;
    xtet::SfsArchive rejected_archive;
    if(rejected_archive.mount({ corrupt_archive.data(), corrupt_archive.size() }))
        return 11;
    std::cout << "acts.txt bytes=" << contents.size() << " definitions=" << definitions.size() << " scripts=" << manifest.script_paths.size() << " bitmaps=" << manifest.bitmap_paths.size()
              << " waves=" << manifest.wave_paths.size() << " decoded=" << decoded_bitmaps << "/" << decoded_waves
              << " scene_nodes=" << scene_counts[0] + scene_counts[1] + scene_counts[2] + scene_counts[3] + scene_counts[4] << '\n';
    return 0;
}
