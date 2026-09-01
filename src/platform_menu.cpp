#include "platform_menu.h"
#include <cstdint>



namespace freegag
{

uint32_t platform_menu_event_type{ UINT32_MAX };

#if defined(SDL_PLATFORM_MACOS)
bool install_native_platform_menu();
void uninstall_native_platform_menu();
#endif

bool initialize_platform_menu()
{
    platform_menu_event_type = SDL_RegisterEvents(1);
    if(platform_menu_event_type == UINT32_MAX)
        return false;
#if defined(SDL_PLATFORM_MACOS)
    return install_native_platform_menu();
#else
    return true;
#endif
}

void shutdown_platform_menu()
{
#if defined(SDL_PLATFORM_MACOS)
    uninstall_native_platform_menu();
#endif
    platform_menu_event_type = UINT32_MAX;
}

bool extract_platform_menu_command(const SDL_Event &event, PlatformMenuCommand *command)
{
    if(command == nullptr || platform_menu_event_type == UINT32_MAX || event.type != platform_menu_event_type)
        return false;
    *command = static_cast<PlatformMenuCommand>(event.user.code);
    return true;
}

void post_platform_menu_command(PlatformMenuCommand command)
{
    if(platform_menu_event_type == UINT32_MAX)
        return;
    SDL_Event event{};
    event.type = platform_menu_event_type;
    event.user.code = static_cast<int32_t>(command);
    SDL_PushEvent(&event);
}

}
