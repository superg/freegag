#pragma once

#include <functional>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include "action_definitions.h"
#include "game_progress.h"
#include "runtime_tables.h"

namespace xtet
{

inline constexpr size_t kInvalidFigurineSceneSlot = (size_t)-1;

struct FigurineGeometryTables;

struct FallingFigurine
{
    bool first_family{};
    uint8_t shape_index{};
    int8_t orientation{};
    int8_t column{};
    int8_t row{};
    int8_t previous_orientation{};
    int8_t previous_column{};
    int8_t previous_row{};
};

enum class FigurineSpriteFamily
{
    MAN,
    WOMAN
};

struct FigurineSpriteSelection
{
    FigurineSpriteFamily family{};
    uint8_t frame_index{};
    bool mirror_horizontal{};
    bool mirror_vertical{};
    int x{};
    int y{};
};

enum class FigurineMove
{
    ROTATE,
    UP,
    RIGHT,
    DOWN,
    LEFT
};

struct FigurineBoardEntry
{
    void *value{};
    FallingFigurine *figurine{};
    size_t scene_slot{ kInvalidFigurineSceneSlot };
};

struct FigurineMatch
{
    FigurineBoardEntry *candidate{};
    const ActionDefinition *action{};
};

enum class GameplayMoveResult
{
    REJECTED,
    MOVED,
    MATCHED
};

enum class GameplayInput
{
    NONE,
    ROTATE,
    LEFT,
    RIGHT,
    DOWN,
    HARD_DROP
};

struct CascadeResult
{
    uint32_t moves{};
    uint32_t matches{};
};

enum class GameTickResult
{
    INACTIVE,
    SPAWNED,
    SPAWN_FAILED,
    MOVED,
    SETTLED,
    MATCHED
};

enum class GameplayInputResult
{
    IGNORED,
    REJECTED,
    MOVED,
    MATCHED
};

struct GameplayInputOutcome
{
    GameplayInputResult result{ GameplayInputResult::IGNORED };
    uint32_t moves{};
};

using FigurineBoardChangeCallback = std::function<void(const FallingFigurine &, bool)>;

FallingFigurine select_falling_figurine(uint32_t family_random, uint32_t shape_random, uint32_t orientation_random, int32_t &family_balance, size_t board_width);
bool find_free_figurine_scene_slot(size_t last_slot, const std::vector<FigurineBoardEntry> &entries, size_t &slot);
bool spawn_falling_figurine(uint32_t family_random, uint32_t shape_random, uint32_t orientation_random, int32_t &family_balance, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries,
    FallingFigurine &figurine, void *value, size_t last_scene_slot, const FigurineBoardChangeCallback &board_change_callback = {});
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
GameplayInput translate_gameplay_key(uint32_t virtual_key);
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
