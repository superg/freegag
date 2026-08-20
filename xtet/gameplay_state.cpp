#include "gameplay_state.h"
#include <algorithm>
#include "figurine_geometry.h"

namespace xtet
{
namespace
{

std::int8_t rotate_orientation(std::int8_t orientation, int quarter_turns)
{
    while(quarter_turns-- > 0)
    {
        if(orientation > 0)
            orientation = orientation == 4 ? 1 : (std::int8_t)(orientation + 1);
        else
            orientation = orientation == -4 ? -1 : (std::int8_t)(orientation - 1);
    }
    return orientation;
}

std::int8_t mirror_orientation(std::int8_t orientation)
{
    switch(orientation)
    {
    case 2:
        return -4;
    case 4:
        return -2;
    case -4:
        return 2;
    case -2:
        return 4;
    default:
        return (std::int8_t)-orientation;
    }
}

FigurineBoardEntry *find_board_entry(std::vector<FigurineBoardEntry> &entries, const void *value)
{
    for(FigurineBoardEntry &entry : entries)
        if(entry.value == value)
            return &entry;
    return nullptr;
}

bool path_crosses_matching_cells(const FallingFigurine &figurine, const RuntimeTables &board, const std::vector<FigurineBoardEntry> &entries, const void *value)
{
    int column_step = 0;
    int row_step = 0;
    if(figurine.column != figurine.previous_column)
        column_step = figurine.column < figurine.previous_column ? -1 : 1;
    else if(figurine.row != figurine.previous_row)
        row_step = figurine.row < figurine.previous_row ? -1 : 1;
    else
        return false;

    FigurineTemplate shape;
    if(!get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, shape))
        return true;
    int center_column = figurine.previous_column;
    int center_row = figurine.previous_row;
    while(center_column != figurine.column || center_row != figurine.row)
    {
        for(int shape_row = 0; shape_row < 5; ++shape_row)
            for(int shape_column = 0; shape_column < 5; ++shape_column)
            {
                if(shape[(std::size_t)shape_row * 5 + shape_column] != 2)
                    continue;
                const int board_column = center_column + shape_column - 2;
                const int board_row = center_row + shape_row - 2;
                if(board_row < 0 || board_row >= (int)kRuntimeTableCount || board_column < 0 || board_column >= (int)board.slotCount())
                    continue;
                const void *occupant = board.tables()[(std::size_t)board_row][(std::size_t)board_column];
                if(occupant == nullptr || occupant == value)
                    continue;
                const FigurineBoardEntry *entry = nullptr;
                for(const FigurineBoardEntry &candidate : entries)
                    if(candidate.value == occupant)
                    {
                        entry = &candidate;
                        break;
                    }
                FigurineTemplate occupant_shape;
                if(entry && entry->figurine && get_oriented_figurine_template(entry->figurine->first_family, entry->figurine->shape_index, entry->figurine->orientation, occupant_shape))
                {
                    const int relative_column = board_column - entry->figurine->column + 2;
                    const int relative_row = board_row - entry->figurine->row + 2;
                    if(relative_column >= 0 && relative_column < 5 && relative_row >= 0 && relative_row < 5 && occupant_shape[(std::size_t)relative_row * 5 + relative_column] == 2)
                        return true;
                }
            }
        center_column += column_step;
        center_row += row_step;
    }
    return false;
}

} // namespace

FallingFigurine select_falling_figurine(std::uint32_t family_random, std::uint32_t shape_random, std::uint32_t orientation_random, std::int32_t &family_balance, std::size_t board_width)
{
    FallingFigurine result;
    result.first_family = (family_random & 1) != 0;
    if(result.first_family && family_balance >= 3)
        result.first_family = false;
    else if(!result.first_family && family_balance <= -3)
        result.first_family = true;
    family_balance += result.first_family ? 1 : -1;
    result.shape_index = (std::uint8_t)(shape_random % (result.first_family ? 5 : 10));
    result.orientation = (orientation_random & 1) != 0 ? 1 : -1;
    result.column = board_width == 0 ? 0 : (std::int8_t)((board_width - 1) / 2);
    result.row = 2;
    return result;
}

bool find_free_figurine_scene_slot(std::size_t last_slot, const std::vector<FigurineBoardEntry> &entries, std::size_t &slot)
{
    constexpr std::size_t first_slot = 3;
    if(last_slot < first_slot)
        return false;
    for(std::size_t candidate = first_slot; candidate <= last_slot; ++candidate)
    {
        const bool used = std::any_of(entries.begin(), entries.end(), [candidate](const FigurineBoardEntry &entry) { return entry.scene_slot == candidate; });
        if(!used)
        {
            slot = candidate;
            return true;
        }
    }
    return false;
}

bool spawn_falling_figurine(std::uint32_t family_random, std::uint32_t shape_random, std::uint32_t orientation_random, std::int32_t &family_balance, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, FallingFigurine &figurine, void *value, std::size_t last_scene_slot, const FigurineBoardChangeCallback &board_change_callback)
{
    if(value == nullptr || find_board_entry(entries, value) != nullptr)
        return false;
    FallingFigurine spawned = select_falling_figurine(family_random, shape_random, orientation_random, family_balance, board.slotCount());
    while(!can_place_figurine(spawned, board))
        spawned.row = (std::int8_t)(spawned.row - 1);
    if(spawned.row < 1)
        return false;
    if(!place_figurine_on_board(spawned, board, value))
        return false;
    std::size_t scene_slot = kInvalidFigurineSceneSlot;
    find_free_figurine_scene_slot(last_scene_slot, entries, scene_slot);
    figurine = spawned;
    entries.push_back({ value, &figurine, scene_slot });
    if(board_change_callback)
        board_change_callback(figurine, true);
    figurine.previous_orientation = figurine.orientation;
    figurine.previous_column = figurine.column;
    figurine.previous_row = figurine.row;
    return true;
}

bool can_place_figurine(const FallingFigurine &figurine, const RuntimeTables &board, const void *self, const void *paired_object)
{
    FigurineTemplate shape;
    if(!get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, shape))
        return false;
    for(int shape_row = 0; shape_row < 5; ++shape_row)
        for(int shape_column = 0; shape_column < 5; ++shape_column)
        {
            if(shape[(std::size_t)shape_row * 5 + shape_column] == 0)
                continue;
            const int board_row = figurine.row + shape_row - 2;
            if(board_row < 0)
                continue;
            const int board_column = figurine.column + shape_column - 2;
            if(board_row >= (int)kRuntimeTableCount || board_column < 0 || board_column >= (int)board.slotCount())
                return false;
            const void *occupant = board.tables()[(std::size_t)board_row][(std::size_t)board_column];
            if(occupant != nullptr && occupant != self && occupant != paired_object)
                return false;
        }
    return true;
}

bool select_figurine_sprite(const FallingFigurine &figurine, FigurineSpriteSelection &selection)
{
    FigurineTemplate shape;
    if(!get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, shape))
        return false;

    FigurineSpriteSelection result;
    result.family = figurine.first_family ? FigurineSpriteFamily::man : FigurineSpriteFamily::woman;
    result.frame_index = figurine.shape_index;
    if(figurine.orientation % 2 == 0)
        result.frame_index += figurine.first_family ? 5 : 10;

    switch(figurine.orientation)
    {
    case 1:
    case 2:
        break;
    case 3:
    case 4:
        result.mirror_horizontal = true;
        result.mirror_vertical = true;
        break;
    case -4:
        result.mirror_horizontal = true;
        break;
    case -3:
    case -2:
        result.mirror_vertical = true;
        break;
    case -1:
        result.mirror_horizontal = true;
        break;
    default:
        return false;
    }

    result.x = figurine.column * 17 - 45;
    result.y = figurine.row * 17 - 45;
    selection = result;
    return true;
}

bool place_figurine_on_board(const FallingFigurine &figurine, RuntimeTables &board, void *value)
{
    if(value == nullptr || !can_place_figurine(figurine, board, value))
        return false;
    FigurineTemplate shape;
    if(!get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, shape))
        return false;
    for(int shape_row = 0; shape_row < 5; ++shape_row)
        for(int shape_column = 0; shape_column < 5; ++shape_column)
            if(shape[(std::size_t)shape_row * 5 + shape_column] != 0)
            {
                const int board_row = figurine.row + shape_row - 2;
                if(board_row >= 0)
                    board.set((std::size_t)board_row, (std::size_t)(figurine.column + shape_column - 2), value);
            }
    return true;
}

bool try_move_falling_figurine(FallingFigurine &figurine, FigurineMove move, const FigurineGeometryTables &geometry, RuntimeTables &board, void *value, const void *paired_object,
    const FigurineBoardChangeCallback &board_change_callback)
{
    FigurineTemplate validated_shape;
    if(value == nullptr || !get_oriented_figurine_template(figurine.first_family, figurine.shape_index, figurine.orientation, validated_shape))
        return false;
    const FallingFigurine original = figurine;
    switch(move)
    {
    case FigurineMove::rotate:
    {
        if(figurine.orientation == 4)
            figurine.orientation = 1;
        else if(figurine.orientation == -4)
            figurine.orientation = -1;
        else if(figurine.orientation < 0)
            figurine.orientation = (std::int8_t)(figurine.orientation - 1);
        else
            figurine.orientation = (std::int8_t)(figurine.orientation + 1);
        const int offset_index = figurine.orientation < 1 ? 3 - figurine.orientation : figurine.orientation - 1;
        const FigurineOffset offset = figurine.first_family ? geometry.first_family[figurine.shape_index][offset_index] : geometry.second_family[figurine.shape_index][offset_index];
        figurine.column = (std::int8_t)(figurine.column + offset.x);
        figurine.row = (std::int8_t)(figurine.row + offset.y);
        break;
    }
    case FigurineMove::up:
        figurine.row = (std::int8_t)(figurine.row - 1);
        break;
    case FigurineMove::right:
        figurine.column = (std::int8_t)(figurine.column + 1);
        break;
    case FigurineMove::down:
        figurine.row = (std::int8_t)(figurine.row + 1);
        break;
    case FigurineMove::left:
        figurine.column = (std::int8_t)(figurine.column - 1);
        break;
    }

    if(!can_place_figurine(figurine, board, value, paired_object))
    {
        figurine.orientation = original.previous_orientation;
        figurine.column = original.previous_column;
        figurine.row = original.previous_row;
        return false;
    }
    if(board_change_callback)
        board_change_callback(original, false);
    board.clearValue(value);
    if(!place_figurine_on_board(figurine, board, value))
    {
        figurine = original;
        place_figurine_on_board(figurine, board, value);
        if(board_change_callback)
            board_change_callback(figurine, true);
        return false;
    }
    if(board_change_callback)
        board_change_callback(figurine, true);
    figurine.previous_orientation = figurine.orientation;
    figurine.previous_column = figurine.column;
    figurine.previous_row = figurine.row;
    return true;
}

const ActionDefinition *find_matching_action(const FallingFigurine &first, const FallingFigurine &second, const std::vector<ActionDefinition> &definitions)
{
    if(first.first_family == second.first_family)
        return nullptr;
    const FallingFigurine &man = first.first_family ? first : second;
    const FallingFigurine &woman = first.first_family ? second : first;
    if(man.shape_index >= 5 || woman.shape_index >= 10 || man.orientation == 0 || woman.orientation == 0)
        return nullptr;

    int relative_x = woman.column - man.column;
    int relative_y = woman.row - man.row;
    std::int8_t relative_orientation = woman.orientation;
    const int quarter_turns = man.orientation < 0 ? -man.orientation : man.orientation;
    if(quarter_turns == 4)
    {
        const int old_x = relative_x;
        relative_x = -relative_y;
        relative_y = old_x;
        relative_orientation = rotate_orientation(relative_orientation, 1);
    }
    else if(quarter_turns == 3)
    {
        relative_x = -relative_x;
        relative_y = -relative_y;
        relative_orientation = rotate_orientation(relative_orientation, 2);
    }
    else if(quarter_turns == 2)
    {
        const int old_x = relative_x;
        relative_x = relative_y;
        relative_y = -old_x;
        relative_orientation = rotate_orientation(relative_orientation, 3);
    }
    if(man.orientation < 0)
    {
        relative_x = -relative_x;
        relative_orientation = mirror_orientation(relative_orientation);
    }

    for(const ActionDefinition &definition : definitions)
        if(definition.values[0] == (std::int8_t)man.shape_index && definition.values[1] == (std::int8_t)woman.shape_index && definition.values[2] == relative_orientation
            && definition.values[3] == relative_x + 2 && definition.values[4] == relative_y + 2)
            return &definition;
    return nullptr;
}

FigurineMatch find_match_candidate(FallingFigurine &figurine, FigurineMove move, const std::vector<ActionDefinition> &definitions, const RuntimeTables &board, std::vector<FigurineBoardEntry> &entries,
    const void *value)
{
    if(move == FigurineMove::rotate || value == nullptr)
        return {};
    int first_column = figurine.previous_column;
    int last_column = figurine.previous_column;
    int first_row = figurine.previous_row;
    int last_row = figurine.previous_row;
    switch(move)
    {
    case FigurineMove::up:
        first_row -= 3;
        break;
    case FigurineMove::right:
        last_column += 3;
        break;
    case FigurineMove::down:
        last_row += 3;
        break;
    case FigurineMove::left:
        first_column -= 3;
        break;
    default:
        return {};
    }

    for(int center_row = first_row; center_row <= last_row; ++center_row)
        for(int center_column = first_column; center_column <= last_column; ++center_column)
        {
            figurine.column = (std::int8_t)center_column;
            figurine.row = (std::int8_t)center_row;
            for(int board_row = center_row - 2; board_row <= center_row + 2; ++board_row)
                for(int board_column = center_column - 2; board_column <= center_column + 2; ++board_column)
                {
                    if(board_row < 0 || board_row >= (int)kRuntimeTableCount || board_column < 0 || board_column >= (int)board.slotCount())
                        continue;
                    void *occupant = board.tables()[(std::size_t)board_row][(std::size_t)board_column];
                    if(occupant == nullptr || occupant == value)
                        continue;
                    FigurineBoardEntry *candidate = find_board_entry(entries, occupant);
                    if(candidate == nullptr || candidate->figurine == nullptr || candidate->figurine->first_family == figurine.first_family)
                        continue;
                    const ActionDefinition *action = find_matching_action(figurine, *candidate->figurine, definitions);
                    if(action != nullptr && can_place_figurine(figurine, board, value, occupant) && !path_crosses_matching_cells(figurine, board, entries, value))
                        return { candidate, action };
                }
        }
    figurine.column = figurine.previous_column;
    figurine.row = figurine.previous_row;
    return {};
}

GameplayMoveResult process_falling_move(FallingFigurine &figurine, FigurineMove move, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, void *value, FigurineMatch &match, const FigurineBoardChangeCallback &board_change_callback)
{
    match = {};
    if(try_move_falling_figurine(figurine, move, geometry, board, value, nullptr, board_change_callback))
        return GameplayMoveResult::moved;
    if(move != FigurineMove::rotate)
    {
        match = find_match_candidate(figurine, move, definitions, board, entries, value);
        if(match.candidate != nullptr)
            return GameplayMoveResult::matched;
    }
    return GameplayMoveResult::rejected;
}

GameplayInput translate_gameplay_key(std::uint32_t virtual_key)
{
    switch(virtual_key)
    {
    case 0x20:
        return GameplayInput::hard_drop;
    case 0x25:
        return GameplayInput::left;
    case 0x26:
        return GameplayInput::rotate;
    case 0x27:
        return GameplayInput::right;
    case 0x28:
        return GameplayInput::down;
    default:
        return GameplayInput::none;
    }
}

bool remove_matched_pair(void *source_value, const FigurineMatch &match, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries, GameProgress &progress,
    const ProgressUpdateCallback &progress_callback)
{
    if(source_value == nullptr || match.candidate == nullptr || match.candidate->value == nullptr || match.candidate->value == source_value)
        return false;
    void *candidate_value = match.candidate->value;
    if(find_board_entry(entries, source_value) == nullptr || find_board_entry(entries, candidate_value) == nullptr)
        return false;
    board.clearValue(source_value);
    board.clearValue(candidate_value);
    entries.erase(
        std::remove_if(entries.begin(), entries.end(), [source_value, candidate_value](const FigurineBoardEntry &entry) { return entry.value == source_value || entry.value == candidate_value; }),
        entries.end());
    const ProgressUpdate first_update = update_progress_after_figurine_removal(progress);
    if(progress_callback && first_update.score_changed)
        progress_callback(progress, first_update);
    const ProgressUpdate second_update = update_progress_after_figurine_removal(progress);
    if(progress_callback && second_update.score_changed)
        progress_callback(progress, second_update);
    return true;
}

bool settle_board_after_match(const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries,
    GameProgress &progress, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, CascadeResult &result,
    const FigurineBoardChangeCallback &board_change_callback, const ProgressUpdateCallback &progress_callback)
{
    result = {};
    if(!match_callback)
        return false;
    bool restart = true;
    while(restart)
    {
        restart = false;
        for(int row = (int)kRuntimeTableCount - 1; row >= 0 && !restart; --row)
            for(std::size_t column = 0; column < board.slotCount() && !restart; ++column)
            {
                void *value = board.tables()[(std::size_t)row][column];
                if(value == nullptr)
                    continue;
                FigurineBoardEntry *entry = find_board_entry(entries, value);
                if(entry == nullptr || entry->figurine == nullptr)
                    return false;
                while(true)
                {
                    FigurineMatch match;
                    const GameplayMoveResult move_result = process_falling_move(*entry->figurine, FigurineMove::down, geometry, definitions, board, entries, value, match, board_change_callback);
                    if(move_result == GameplayMoveResult::moved)
                    {
                        ++result.moves;
                        continue;
                    }
                    if(move_result == GameplayMoveResult::matched)
                    {
                        if(match.candidate == nullptr || match.candidate->figurine == nullptr || match.action == nullptr || !match_callback(*entry->figurine, *match.candidate->figurine, *match.action)
                            || !remove_matched_pair(value, match, board, entries, progress, progress_callback))
                            return false;
                        ++result.matches;
                        restart = true;
                    }
                    break;
                }
            }
    }
    return true;
}

bool update_game_tick(const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board, std::vector<FigurineBoardEntry> &entries, void *&active_value,
    GameProgress &progress, const std::function<void *()> &spawn_callback, const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback,
    GameTickResult &tick_result, CascadeResult &cascade_result, const FigurineBoardChangeCallback &board_change_callback, const ProgressUpdateCallback &progress_callback)
{
    tick_result = GameTickResult::inactive;
    cascade_result = {};
    if(progress.gameplay_state != 1)
        return true;
    if(active_value == nullptr)
    {
        if(!spawn_callback)
            return false;
        active_value = spawn_callback();
        if(active_value == nullptr)
        {
            progress.gameplay_state = 2;
            tick_result = GameTickResult::spawn_failed;
            return true;
        }
        FigurineBoardEntry *spawned_entry = find_board_entry(entries, active_value);
        if(spawned_entry == nullptr || spawned_entry->figurine == nullptr)
        {
            active_value = nullptr;
            return false;
        }
        tick_result = GameTickResult::spawned;
        return true;
    }

    FigurineBoardEntry *active_entry = find_board_entry(entries, active_value);
    if(active_entry == nullptr || active_entry->figurine == nullptr)
        return false;
    FigurineMatch match;
    const GameplayMoveResult move_result = process_falling_move(*active_entry->figurine, FigurineMove::down, geometry, definitions, board, entries, active_value, match, board_change_callback);
    if(move_result == GameplayMoveResult::moved)
    {
        tick_result = GameTickResult::moved;
        return true;
    }
    if(move_result == GameplayMoveResult::rejected)
    {
        active_value = nullptr;
        tick_result = GameTickResult::settled;
        return true;
    }
    if(match.candidate == nullptr || match.candidate->figurine == nullptr || match.action == nullptr || !match_callback)
        return false;
    void *matched_value = active_value;
    if(!match_callback(*active_entry->figurine, *match.candidate->figurine, *match.action) || !remove_matched_pair(matched_value, match, board, entries, progress, progress_callback))
        return false;
    active_value = nullptr;
    if(!settle_board_after_match(geometry, definitions, board, entries, progress, match_callback, cascade_result, board_change_callback, progress_callback))
        return false;
    tick_result = GameTickResult::matched;
    return true;
}

bool handle_gameplay_input(GameplayInput input, const FigurineGeometryTables &geometry, const std::vector<ActionDefinition> &definitions, RuntimeTables &board,
    std::vector<FigurineBoardEntry> &entries, void *&active_value, GameProgress &progress,
    const std::function<bool(const FallingFigurine &, const FallingFigurine &, const ActionDefinition &)> &match_callback, const std::function<void()> &drain_keyboard_callback,
    GameplayInputOutcome &outcome, CascadeResult &cascade_result, const FigurineBoardChangeCallback &board_change_callback, const ProgressUpdateCallback &progress_callback)
{
    outcome = {};
    cascade_result = {};
    if(input == GameplayInput::none || active_value == nullptr)
        return true;
    FigurineBoardEntry *active_entry = find_board_entry(entries, active_value);
    if(active_entry == nullptr || active_entry->figurine == nullptr)
        return false;

    FigurineMove move;
    switch(input)
    {
    case GameplayInput::rotate:
        move = FigurineMove::rotate;
        break;
    case GameplayInput::left:
        move = FigurineMove::left;
        break;
    case GameplayInput::right:
        move = FigurineMove::right;
        break;
    case GameplayInput::down:
    case GameplayInput::hard_drop:
        move = FigurineMove::down;
        break;
    default:
        return true;
    }

    FigurineMatch match;
    GameplayMoveResult move_result;
    do
    {
        move_result = process_falling_move(*active_entry->figurine, move, geometry, definitions, board, entries, active_value, match, board_change_callback);
        if(move_result == GameplayMoveResult::moved)
            ++outcome.moves;
    } while(input == GameplayInput::hard_drop && move_result == GameplayMoveResult::moved);

    if(move_result == GameplayMoveResult::moved)
    {
        outcome.result = GameplayInputResult::moved;
        return true;
    }
    if(move_result == GameplayMoveResult::rejected)
    {
        outcome.result = GameplayInputResult::rejected;
        return true;
    }
    if(match.candidate == nullptr || match.candidate->figurine == nullptr || match.action == nullptr || !match_callback || !drain_keyboard_callback)
        return false;
    void *matched_value = active_value;
    if(!match_callback(*active_entry->figurine, *match.candidate->figurine, *match.action) || !remove_matched_pair(matched_value, match, board, entries, progress, progress_callback))
        return false;
    active_value = nullptr;
    if(!settle_board_after_match(geometry, definitions, board, entries, progress, match_callback, cascade_result, board_change_callback, progress_callback))
        return false;
    drain_keyboard_callback();
    outcome.result = GameplayInputResult::matched;
    return true;
}

} // namespace xtet
