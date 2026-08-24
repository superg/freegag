#pragma once

#include <memory>
#include "../runtime_input.h"
#include "gameplay_state.h"

namespace xtet
{

struct GameInputCallbacks
{
    std::function<void()> destroy;
    std::function<void(uint32_t)> key_down;
    std::function<void(bool)> mouse_button;
};

struct GameKeyDownCallbacks
{
    std::function<void()> stop_gameplay;
    std::function<void(uint32_t)> post_result;
    std::function<void()> post_termination;
    std::function<void(uint32_t)> gameplay_key;
    std::function<void()> drain_keyboard;
};

uint32_t dispatch_game_input(uint32_t gameplay_state, const gag::RuntimeInputEvent &event, const GameInputCallbacks &callbacks);
uint32_t calculate_result_input_deadline(uint32_t current_time);
void handle_game_key_down(uint32_t &gameplay_state, uint32_t key, uint32_t current_time, uint32_t result_input_deadline, uint32_t score, const GameKeyDownCallbacks &callbacks);
bool set_game_paused(uint32_t &gameplay_state, bool paused, const std::function<bool(bool)> &loop_playing_callback);

class GameplayRuntime
{
public:
    bool initialize(size_t board_width, size_t last_scene_slot, const GameProgress &progress);
    bool updateTick(uint32_t family_random, uint32_t shape_random, uint32_t orientation_random, uint32_t current_time, const FigurineGeometryTables &geometry,
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
    int32_t familyBalance() const;
    GameProgress &progress();
    const GameProgress &progress() const;
    uint32_t resultInputDeadline() const;
    void stop();

private:
    void discardRemovedFigurines();

    RuntimeTables board_;
    std::vector<std::unique_ptr<FallingFigurine>> figurines_;
    std::vector<FigurineBoardEntry> entries_;
    void *active_value_{};
    int32_t family_balance_{};
    size_t last_scene_slot_{};
    GameProgress progress_{};
    uint32_t result_input_deadline_{};
};

} // namespace xtet
