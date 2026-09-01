#pragma once

#include <SDL3/SDL.h>
#include "application_types.h"

namespace freegag
{

int run_startup(int argc, char *argv[]);

void dispatch_sdl_runtime_input(ApplicationState *state, const SDL_Event &source_event);

} // namespace freegag
