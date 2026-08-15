#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>
#include "action_definitions.h"
#include "game_progress.h"
#include "runtime_tables.h"

namespace xtet
{

inline constexpr std::size_t kInvalidFigurineSceneSlot = (std::size_t)-1;

struct FigurineGeometryTables;

struct FallingFigurine
{
    bool first_family{};
    std::uint8_t shape_index{};
    std::int8_t orientation{};
    std::int8_t column{};
    std::int8_t row{};
    std::int8_t previous_orientation{};
    std::int8_t previous_column{};
    std::int8_t previous_row{};
};

enum class FigurineSpriteFamily
{
    man,
    woman
};

struct FigurineSpriteSelection
{
    FigurineSpriteFamily family{};
    std::uint8_t frame_index{};
    bool mirror_horizontal{};
    bool mirror_vertical{};
    int x{};
    int y{};
};

enum class FigurineMove
{
    rotate,
    up,
    right,
    down,
    left
};

struct FigurineBoardEntry
{
    void *value{};
    FallingFigurine *figurine{};
    std::size_t scene_slot{ kInvalidFigurineSceneSlot };
};

struct FigurineMatch
{
    FigurineBoardEntry *candidate{};
    const ActionDefinition *action{};
};

enum class GameplayMoveResult
{
    rejected,
    moved,
    matched
};

enum class GameplayInput
{
    none,
    rotate,
    left,
    right,
    down,
    hard_drop
};

struct CascadeResult
{
    std::uint32_t moves{};
    std::uint32_t matches{};
};

enum class GameTickResult
{
    inactive,
    spawned,
    spawn_failed,
    moved,
    settled,
    matched
};

enum class GameplayInputResult
{
    ignored,
    rejected,
    moved,
    matched
};

struct GameplayInputOutcome
{
    GameplayInputResult result{};
    std::uint32_t moves{};
};

using FigurineBoardChangeCallback = std::function<void(const FallingFigurine &, bool)>;

FallingFigurine select_falling_figurine(std::uint32_t family_random, std::uint32_t shape_random, std::uint32_t orientation_random, std::int32_t &family_balance, std::size_t board_width);
bool find_free_figurine_scene_slot(std::size_t last_slot, const std::vector<FigurineBoardEntry> &entries, std::size_t &slot);
bool spawn_falling_figurine(std::uint32_t family_random, std::uint32_t shape_random, std::uint32_t orientation_random, std::int32_t &family_balance, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, FallingFigurine &figurine, void *value, std::size_t last_scene_slot, const FigurineBoardChangeCallback &board_change_callback = {});
bool can_place_figurine(const FallingFigurine &figurine, const RuntimeTables &board, const void *self = nullptr, const void *paired_object = nullptr);
bool select_figurine_sprite(const FallingFigurine &figurine, FigurineSpriteSelection &selection);
bool place_figurine_on_board(const FallingFigurine &figurine, RuntimeTables &board, void *value);
bool try_move_falling_figurine(FallingFigurine &figurine, FigurineMove move, const FigurineGeometryTables &geometry, RuntimeTables &board, void *value, const void *paired_object = nullptr,
    const FigurineBoardChangeCallback &board_change_callback = {});
const ActionDefinition *find_matching_action(const FallingFigurine &first, const FallingFigurine &second, const std::vector<ActionDefinition> &definitions);
FigurineMatch find_match_candidate(FallingFigurine &figurine, FigurineMove move, const std::vector<ActionDefinition> &definitions, const RuntimeTables &board, std::vector<FigurineBoardEntry> &entries,
    const void *value);
GameplayMoveResult process_falling_move(FallingFigurine &figurine, FigurineMove move, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, void *value, FigurineMatch &match, const FigurineBoardChangeCallback &board_change_callback = {});
GameplayInput translate_gameplay_key(std::uint32_t virtual_key);
bool remove_matched_pair(void *source_value, const FigurineMatch &match, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries, GameProgress &progress,
    const ProgressUpdateCallback &progress_callback = {});
bool settle_board_after_match(const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries,
    GameProgress &progress, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, CascadeResult &result,
    const FigurineBoardChangeCallback &board_change_callback = {}, const ProgressUpdateCallback &progress_callback = {});
bool update_game_tick(const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries, void *&active_value,
    GameProgress &progress, const std::function<void *()> &spawn_callback, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback,
    GameTickResult &tick_result, CascadeResult &cascade_result, const FigurineBoardChangeCallback &board_change_callback = {}, const ProgressUpdateCallback &progress_callback = {});
bool handle_gameplay_input(GameplayInput input, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, void *&active_value, GameProgress &progress,
    const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, const std::function<void()> &drain_keyboard_callback,
    GameplayInputOutcome &outcome, CascadeResult &cascade_result, const FigurineBoardChangeCallback &board_change_callback = {}, const ProgressUpdateCallback &progress_callback = {});

} // namespace xtet
