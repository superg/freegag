#include "game_progress.h"

namespace xtet
{

ProgressUpdate update_progress_after_figurine_removal(GameProgress &progress)
{
    ProgressUpdate result;
    if(progress.gameplay_state != 1)
        return result;
    ++progress.score;
    result.score_changed = true;
    const uint32_t next_level = progress.base_level + progress.score / 30;
    if(next_level < 11)
    {
        result.level_changed = next_level != progress.level;
        progress.level = next_level;
    }
    else
    {
        progress.gameplay_state = 3;
        result.game_over = true;
    }
    return result;
}

} // namespace xtet
