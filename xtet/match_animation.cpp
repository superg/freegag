#include "match_animation.h"

namespace xtet
{
namespace
{

void transform_vector(std::int8_t orientation, int source_x, int source_y, int &x, int &y)
{
    switch(orientation)
    {
    case 1:
        x = source_x;
        y = source_y;
        break;
    case 2:
        x = -source_y;
        y = source_x;
        break;
    case 3:
        x = -source_x;
        y = -source_y;
        break;
    case 4:
        x = source_y;
        y = -source_x;
        break;
    case -4:
        x = source_y;
        y = source_x;
        break;
    case -3:
        x = source_x;
        y = -source_y;
        break;
    case -2:
        x = -source_y;
        y = -source_x;
        break;
    case -1:
        x = -source_x;
        y = source_y;
        break;
    default:
        x = 0;
        y = 0;
        break;
    }
}

void expand_component(int &value)
{
    value += value < 0 ? -1 : 1;
}

void select_resource(std::int8_t orientation, bool man, MatchAnimationParticipant &participant)
{
    participant.resource = man ? ((orientation % 2) == 0 ? MatchAnimationResource::rotated_man : MatchAnimationResource::man)
                               : ((orientation % 2) == 0 ? MatchAnimationResource::rotated_woman : MatchAnimationResource::woman);
    participant.mirror_horizontal = orientation == 3 || orientation == 4 || orientation == -4 || orientation == -1;
    participant.mirror_vertical = orientation == 3 || orientation == 4 || orientation == -3 || orientation == -2;
}

} // namespace

bool build_match_animation_plan(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, bool expanded, MatchAnimationPlan &plan)
{
    const FallingFigurine &man = first.first_family ? first : second;
    const FallingFigurine &woman = first.first_family ? second : first;
    if(!man.first_family || woman.first_family || man.shape_index >= 5 || woman.shape_index >= 10 || man.orientation == 0 || woman.orientation == 0)
        return false;

    constexpr std::array<std::uint8_t, 5> man_frames{ 0, 8, 16, 24, 32 };
    constexpr std::array<std::uint8_t, 5> alternate_man_frames{ 4, 12, 20, 28, 36 };
    constexpr std::array<std::uint8_t, 10> woman_frames{ 0, 6, 12, 18, 24, 30, 36, 39, 42, 48 };
    constexpr std::array<std::uint8_t, 10> alternate_woman_frames{ 3, 9, 15, 21, 27, 33, 36, 39, 45, 48 };
    constexpr std::array<std::uint8_t, 4> first_man_slots{ 1, 2, 5, 3 };
    constexpr std::array<std::uint8_t, 4> second_man_slots{ 1, 2, 5, 4 };
    constexpr std::array<std::uint8_t, 3> first_woman_slots{ 0, 4, 6 };
    constexpr std::array<std::uint8_t, 3> second_woman_slots{ 0, 3, 6 };

    MatchAnimationPlan result;
    const std::uint8_t flags = (std::uint8_t)action.values[5];
    result.man.first_frame = (flags & 1) != 0 ? alternate_man_frames[man.shape_index] : man_frames[man.shape_index];
    result.man.frame_count = 4;
    result.woman.first_frame = (flags & 2) != 0 ? alternate_woman_frames[woman.shape_index] : woman_frames[woman.shape_index];
    result.woman.frame_count = 3;
    result.man.temporary_slots = (flags & 4) != 0 ? second_man_slots : first_man_slots;
    result.man.temporary_slot_count = 4;
    const std::array<std::uint8_t, 3> &woman_slots = (flags & 4) != 0 ? second_woman_slots : first_woman_slots;
    for(std::size_t index = 0; index < woman_slots.size(); ++index)
        result.woman.temporary_slots[index] = woman_slots[index];
    result.woman.temporary_slot_count = 3;

    int man_offset_x = 0;
    int man_offset_y = 0;
    int woman_offset_x = 0;
    int woman_offset_y = 0;
    transform_vector(man.orientation, action.values[6], action.values[7], man_offset_x, man_offset_y);
    transform_vector(man.orientation, action.values[8], action.values[9], woman_offset_x, woman_offset_y);
    if(expanded)
    {
        expand_component(man_offset_x);
        expand_component(man_offset_y);
        expand_component(woman_offset_x);
        expand_component(woman_offset_y);
    }
    result.man.x = man.column * 17 - 45 + man_offset_x;
    result.man.y = man.row * 17 - 45 + man_offset_y;
    result.woman.x = woman.column * 17 - 45 + woman_offset_x;
    result.woman.y = woman.row * 17 - 45 + woman_offset_y;
    select_resource(man.orientation, true, result.man);
    select_resource(woman.orientation, false, result.woman);
    plan = result;
    return true;
}

} // namespace xtet
