#include "startup.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include "runtime_internal.h"

namespace gag
{

int run_startup(int argc, char *argv[])
{
    ApplicationState *state = initialize_gag_application(640, 480, GetModuleHandleA(nullptr), has_xtet_argument(argc, argv), SW_SHOWDEFAULT);
    if(state == nullptr)
    {
        return 1;
    }

    set_runtime_flag_40();

    MSG message{};
    message.message = WM_COMMAND;
    do
    {
        GetMessageA(&message, nullptr, 0, 0);
        TranslateMessage(&message);
        DispatchMessageA(&message);
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    } while(message.message != WM_QUIT);

    if((state->flags & 0x2000) != 0)
    {
        ShowCursor(TRUE);
        std::fprintf(stderr, "%s: %s\n", state->message_table, application_message(state, 16));
    }

    return static_cast<int>(message.wParam);
}


} // namespace gag
