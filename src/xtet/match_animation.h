#pragma once

#include <array>
#include <stdint.h>
#include "action_definitions.h"
#include "gameplay_state.h"

namespace xtet
{

enum class MatchAnimationResource
{
    man,
    rotated_man,
    woman,
    rotated_woman
};

struct MatchAnimationParticipant
{
    MatchAnimationResource resource{};
    uint8_t first_frame{};
    uint8_t frame_count{};
    bool mirror_horizontal{};
    bool mirror_vertical{};
    int x{};
    int y{};
    std::array<uint8_t, 4> temporary_slots{};
    uint8_t temporary_slot_count{};
};

struct MatchAnimationPlan
{
    MatchAnimationParticipant man;
    MatchAnimationParticipant woman;
};

bool build_match_animation_plan(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, bool expanded, MatchAnimationPlan &plan);

} // namespace xtet
