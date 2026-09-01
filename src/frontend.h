#pragma once

#include <SDL3/SDL.h>
#include "frontend_types.h"



namespace freegag
{

FrontendState *initialize_frontend(int argc, char *argv[]);

SDL_AppResult iterate_frontend(FrontendState *frontend);

SDL_AppResult dispatch_frontend_event(FrontendState *frontend, SDL_Event *event);

void shutdown_frontend(FrontendState *frontend);

}
