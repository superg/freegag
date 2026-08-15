#pragma once

#include <array>
#include <cstdint>
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
    std::uint8_t first_frame{};
    std::uint8_t frame_count{};
    bool mirror_horizontal{};
    bool mirror_vertical{};
    int x{};
    int y{};
    std::array<std::uint8_t, 4> temporary_slots{};
    std::uint8_t temporary_slot_count{};
};

struct MatchAnimationPlan
{
    MatchAnimationParticipant man;
    MatchAnimationParticipant woman;
};

bool build_match_animation_plan(const FallingFigurine &first, const FallingFigurine &second, const ActionDefinition &action, bool expanded, MatchAnimationPlan &plan);

} // namespace xtet
