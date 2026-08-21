#include "game_runtime.h"
#include <algorithm>

namespace xtet
{

uint32_t dispatch_game_window_message(uint32_t gameplay_state, uint32_t message, uint32_t wparam, const GameWindowMessageCallbacks &callbacks)
{
    if(gameplay_state < 1 || gameplay_state > 4)
        return 1;
    switch(message)
    {
    case 0x0002:
        if(callbacks.destroy)
            callbacks.destroy();
        break;
    case 0x0100:
        if(callbacks.key_down)
            callbacks.key_down(wparam);
        break;
    case 0x0201:
        if(callbacks.mouse_button)
            callbacks.mouse_button(true);
        break;
    case 0x0202:
        if(callbacks.mouse_button)
            callbacks.mouse_button(false);
        break;
    }
    return 0;
}

uint32_t calculate_result_input_deadline(uint32_t current_time)
{
    return current_time + 2000;
}

void handle_game_key_down(uint32_t &gameplay_state, uint32_t key, uint32_t current_time, uint32_t result_input_deadline, uint32_t score, const GameKeyDownCallbacks &callbacks)
{
    if(gameplay_state == 2 || gameplay_state == 3)
    {
        if(current_time >= result_input_deadline)
        {
            gameplay_state = 0;
            if(callbacks.stop_gameplay)
                callbacks.stop_gameplay();
            if(callbacks.post_result)
                callbacks.post_result(score);
            if(callbacks.post_termination)
                callbacks.post_termination();
        }
        else if(callbacks.drain_keyboard)
            callbacks.drain_keyboard();
        return;
    }
    if(key == 0x1b)
    {
        gameplay_state = 0;
        if(callbacks.stop_gameplay)
            callbacks.stop_gameplay();
        if(callbacks.post_result)
            callbacks.post_result(score);
        if(callbacks.post_termination)
            callbacks.post_termination();
        return;
    }
    if(gameplay_state == 1 && callbacks.gameplay_key)
        callbacks.gameplay_key(key);
}

bool set_game_paused(uint32_t &gameplay_state, bool paused, const std::function<bool(bool)> &loop_playing_callback)
{
    if(gameplay_state == 1 && paused)
    {
        if(!loop_playing_callback || !loop_playing_callback(false))
            return false;
        gameplay_state = 4;
    }
    else if((gameplay_state == 4 || gameplay_state == 5) && !paused)
    {
        if(!loop_playing_callback || !loop_playing_callback(true))
            return false;
        gameplay_state = 1;
    }
    return true;
}

bool GameplayRuntime::initialize(size_t board_width, size_t last_scene_slot, const GameProgress &progress)
{
    RuntimeTables board;
    if(last_scene_slot < 3 || !board.initialize(board_width))
        return false;
    board_ = std::move(board);
    figurines_.clear();
    entries_.clear();
    active_value_ = nullptr;
    family_balance_ = 0;
    last_scene_slot_ = last_scene_slot;
    progress_ = progress;
    result_input_deadline_ = 0;
    return true;
}

bool GameplayRuntime::updateTick(uint32_t family_random, uint32_t shape_random, uint32_t orientation_random, uint32_t current_time, const FigurineGeometryTables &geometry,
    const std::vector<ActionDefinition> &definitions, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback,
    const FigurineBoardChangeCallback &board_change_callback, GameTickResult &tick_result, CascadeResult &cascade_result, const ProgressUpdateCallback &progress_callback)
{
    const uint32_t previous_state = progress_.gameplay_state;
    const auto spawn_callback = [&]() -> void *
    {
        std::unique_ptr<FallingFigurine> figurine = std::make_unique<FallingFigurine>();
        FallingFigurine *value = figurine.get();
        if(!spawn_falling_figurine(family_random, shape_random, orientation_random, family_balance_, board_, entries_, *figurine, value, last_scene_slot_, board_change_callback))
            return nullptr;
        figurines_.push_back(std::move(figurine));
        return value;
    };
    if(!update_game_tick(geometry, definitions, board_, entries_, active_value_, progress_, spawn_callback, match_callback, tick_result, cascade_result, board_change_callback, progress_callback))
        return false;
    if(previous_state == 1 && (progress_.gameplay_state == 2 || progress_.gameplay_state == 3))
        result_input_deadline_ = calculate_result_input_deadline(current_time);
    discardRemovedFigurines();
    return true;
}

bool GameplayRuntime::handleInput(GameplayInput input, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions,
    const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, const std::function<void()> &drain_keyboard_callback,
    const FigurineBoardChangeCallback &board_change_callback, GameplayInputOutcome &outcome, CascadeResult &cascade_result, const ProgressUpdateCallback &progress_callback)
{
    if(!handle_gameplay_input(input, geometry, definitions, board_, entries_, active_value_, progress_, match_callback, drain_keyboard_callback, outcome, cascade_result, board_change_callback,
           progress_callback))
        return false;
    discardRemovedFigurines();
    return true;
}

RuntimeTables &GameplayRuntime::board()
{
    return board_;
}

const RuntimeTables &GameplayRuntime::board() const
{
    return board_;
}

std::vector<FigurineBoardEntry> &GameplayRuntime::entries()
{
    return entries_;
}

const std::vector<FigurineBoardEntry> &GameplayRuntime::entries() const
{
    return entries_;
}

void *GameplayRuntime::activeValue() const
{
    return active_value_;
}

int32_t GameplayRuntime::familyBalance() const
{
    return family_balance_;
}

GameProgress &GameplayRuntime::progress()
{
    return progress_;
}

const GameProgress &GameplayRuntime::progress() const
{
    return progress_;
}

uint32_t GameplayRuntime::resultInputDeadline() const
{
    return result_input_deadline_;
}

void GameplayRuntime::stop()
{
    progress_.gameplay_state = 0;
    active_value_ = nullptr;
    board_.clear();
    entries_.clear();
    figurines_.clear();
}

void GameplayRuntime::discardRemovedFigurines()
{
    figurines_.erase(std::remove_if(figurines_.begin(), figurines_.end(), [&](const std::unique_ptr<FallingFigurine> &figurine)
                         { return std::none_of(entries_.begin(), entries_.end(), [&](const FigurineBoardEntry &entry) { return entry.figurine == figurine.get(); }); }),
        figurines_.end());
}

} // namespace xtet
