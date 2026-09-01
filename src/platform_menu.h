#pragma once

#include <SDL3/SDL.h>



namespace freegag
{

enum class PlatformMenuCommand
{
    SETTINGS,
    CONTROLS,
    RETURN_TO_LAUNCHER
};

bool initialize_platform_menu();

void shutdown_platform_menu();

bool extract_platform_menu_command(const SDL_Event &event, PlatformMenuCommand *command);

void post_platform_menu_command(PlatformMenuCommand command);

}
