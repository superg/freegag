#pragma once

#include <memory>
#include "gameplay_state.h"

namespace xtet
{

struct GameWindowMessageCallbacks
{
    std::function<void()> destroy;
    std::function<void(std::uint32_t)> key_down;
    std::function<void(bool)> mouse_button;
};

struct GameKeyDownCallbacks
{
    std::function<void()> stop_gameplay;
    std::function<void(std::uint32_t)> post_result;
    std::function<void()> post_termination;
    std::function<void(std::uint32_t)> gameplay_key;
    std::function<void()> drain_keyboard;
};

std::uint32_t dispatch_game_window_message(std::uint32_t gameplay_state, std::uint32_t message, std::uint32_t wparam, const GameWindowMessageCallbacks &callbacks);
std::uint32_t calculate_result_input_deadline(std::uint32_t current_time);
void handle_game_key_down(std::uint32_t &gameplay_state, std::uint32_t key, std::uint32_t current_time, std::uint32_t result_input_deadline, std::uint32_t score,
    const GameKeyDownCallbacks &callbacks);
bool set_game_paused(std::uint32_t &gameplay_state, bool paused, const std::function<bool(bool)> &loop_playing_callback);

class GameplayRuntime
{
public:
    bool initialize(std::size_t board_width, std::size_t last_scene_slot, const GameProgress &progress);
    bool updateTick(std::uint32_t family_random, std::uint32_t shape_random, std::uint32_t orientation_random, std::uint32_t current_time, const FigurineGeometryTables &geometry,
        const std::vector<ActionDefinition> &definitions, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback,
        const FigurineBoardChangeCallback &board_change_callback, GameTickResult &tick_result, CascadeResult &cascade_result, const ProgressUpdateCallback &progress_callback = {});
    bool handleInput(GameplayInput input, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions,
        const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, const std::function<void()> &drain_keyboard_callback,
        const FigurineBoardChangeCallback &board_change_callback, GameplayInputOutcome &outcome, CascadeResult &cascade_result, const ProgressUpdateCallback &progress_callback = {});

    RuntimeTables &board();
    const RuntimeTables &board() const;
    std::vector<FigurineBoardEntry> &entries();
    const std::vector<FigurineBoardEntry> &entries() const;
    void *activeValue() const;
    std::int32_t familyBalance() const;
    GameProgress &progress();
    const GameProgress &progress() const;
    std::uint32_t resultInputDeadline() const;
    void stop();

private:
    void discardRemovedFigurines();

    RuntimeTables board_;
    std::vector<std::unique_ptr<FallingFigurine>> figurines_;
    std::vector<FigurineBoardEntry> entries_;
    void *active_value_{};
    std::int32_t family_balance_{};
    std::size_t last_scene_slot_{};
    GameProgress progress_{};
    std::uint32_t result_input_deadline_{};
};

} // namespace xtet
