#include "startup.h"
#include <SDL3/SDL.h>
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <cstdio>
#include "game_host.h"
#include "host_events.h"
#include "runtime_internal.h"

namespace gag
{
namespace
{
uint32_t runtime_key(SDL_Keycode key)
{
    if(key >= SDLK_A && key <= SDLK_Z)
    {
        return static_cast<uint32_t>('A' + key - SDLK_A);
    }
    if(key >= SDLK_0 && key <= SDLK_9)
    {
        return static_cast<uint32_t>('0' + key - SDLK_0);
    }
    switch(key)
    {
    case SDLK_BACKSPACE:
        return 0x08;
    case SDLK_TAB:
        return 0x09;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return 0x0d;
    case SDLK_ESCAPE:
        return 0x1b;
    case SDLK_SPACE:
        return 0x20;
    case SDLK_PAGEUP:
        return 0x21;
    case SDLK_PAGEDOWN:
        return 0x22;
    case SDLK_END:
        return 0x23;
    case SDLK_HOME:
        return 0x24;
    case SDLK_LEFT:
        return 0x25;
    case SDLK_UP:
        return 0x26;
    case SDLK_RIGHT:
        return 0x27;
    case SDLK_DOWN:
        return 0x28;
    case SDLK_INSERT:
        return 0x2d;
    case SDLK_DELETE:
        return 0x2e;
    default:
        return static_cast<uint32_t>(key);
    }
}

RuntimeMouseButton runtime_mouse_button(uint8_t button)
{
    if(button == SDL_BUTTON_LEFT)
    {
        return RuntimeMouseButton::left;
    }
    if(button == SDL_BUTTON_MIDDLE)
    {
        return RuntimeMouseButton::middle;
    }
    if(button == SDL_BUTTON_RIGHT)
    {
        return RuntimeMouseButton::right;
    }
    return RuntimeMouseButton::none;
}

bool prepare_current_runtime_pointer_input(ApplicationState *state, RuntimeInputEvent *input)
{
    if(input == nullptr || !get_sdl_presenter_mouse_position(&input->x, &input->y))
    {
        return false;
    }
    const bool inside = input->x >= 0 && input->y >= 0 && input->x < runtime_game_host_context.width && input->y < runtime_game_host_context.height;
    input->type = inside ? RuntimeInputType::pointer_move : RuntimeInputType::pointer_leave;
    set_game_cursor_active(state, inside ? 0 : 1);
    return true;
}

int dispatch_sdl_runtime_input(ApplicationState *state, const SDL_Event &source_event)
{
    SDL_Event event = source_event;
    convert_sdl_presenter_event(&event);
    RuntimeInputEvent input;
    if(event.type == SDL_EVENT_MOUSE_MOTION)
    {
        input.x = static_cast<int32_t>(event.motion.x);
        input.y = static_cast<int32_t>(event.motion.y);
        const bool inside = input.x >= 0 && input.y >= 0 && input.x < runtime_game_host_context.width && input.y < runtime_game_host_context.height;
        input.type = inside ? RuntimeInputType::pointer_move : RuntimeInputType::pointer_leave;
        set_game_cursor_active(state, inside ? 0 : 1);
    }
    else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        input.type = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? RuntimeInputType::button_down : RuntimeInputType::button_up;
        input.button = runtime_mouse_button(event.button.button);
        input.x = static_cast<int32_t>(event.button.x);
        input.y = static_cast<int32_t>(event.button.y);
        if(input.x < 0 || input.y < 0 || input.x >= runtime_game_host_context.width || input.y >= runtime_game_host_context.height)
        {
            return 1;
        }
        set_game_cursor_active(state, 0);
        if(input.button == RuntimeMouseButton::left && input.type == RuntimeInputType::button_down)
        {
            desktop_fullscreen_toggle_latched = false;
        }
    }
    else if(event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
    {
        input.type = RuntimeInputType::pointer_leave;
        set_game_cursor_active(state, 1);
    }
    else if(event.type == SDL_EVENT_WINDOW_MOUSE_ENTER)
    {
        if(!prepare_current_runtime_pointer_input(state, &input))
        {
            return 1;
        }
    }
    else if(event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
    {
        input.type = event.type == SDL_EVENT_KEY_DOWN ? RuntimeInputType::key_down : RuntimeInputType::key_up;
        input.key = runtime_key(event.key.key);
        input.repeat = event.key.repeat;
    }
    else if(event.type == SDL_EVENT_TEXT_INPUT)
    {
        input.type = RuntimeInputType::text;
        input.text = event.text.text == nullptr ? "" : event.text.text;
    }
    else
    {
        return 0;
    }
    return handle_runtime_input_event(input) ? 1 : -1;
}

SDL_AppResult startup_result(ApplicationState *state)
{
    if(state == nullptr)
    {
        return SDL_APP_FAILURE;
    }
    return state->shutdown_complete ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL initialize_startup_callbacks(void **appstate, int argc, char *argv[])
{
    ApplicationState *state = initialize_gag_application(640, 480, has_xtet_argument(argc, argv));
    if(state == nullptr)
    {
        return SDL_APP_FAILURE;
    }

    set_runtime_flag_40();
    use_portable_runtime_input(true);
    *appstate = state;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL iterate_startup_callbacks(void *appstate)
{
    auto *state = static_cast<ApplicationState *>(appstate);
    service_sdl_presenter();
    return startup_result(state);
}

SDL_AppResult SDLCALL dispatch_startup_event(void *appstate, SDL_Event *event)
{
    auto *state = static_cast<ApplicationState *>(appstate);
    if(state == nullptr || event == nullptr)
    {
        return SDL_APP_FAILURE;
    }

    if(event->type == application_host_event_type())
    {
        drain_host_events();
    }
    else if(event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        dispatch_application_action(state, ApplicationAction::exit);
    }
    else if(event->type == SDL_EVENT_WINDOW_MINIMIZED)
    {
        set_runtime_flag_01000000();
    }
    else if(event->type == SDL_EVENT_WINDOW_RESTORED)
    {
        clear_runtime_flag_01000000();
    }
    else if(event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN || event->type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN)
    {
        complete_sdl_presenter_fullscreen_transition(event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN);
    }
    else if(event->type == SDL_EVENT_WINDOW_RESIZED || event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_EXPOSED)
    {
        request_sdl_presenter_repaint();
    }
    else if(dispatch_sdl_runtime_input(state, *event) < 0)
    {
        SDL_PushEvent(event);
    }
    return startup_result(state);
}

void SDLCALL shutdown_startup_callbacks(void *appstate, SDL_AppResult)
{
    auto *state = static_cast<ApplicationState *>(appstate);
    if(state == nullptr)
    {
        return;
    }
    use_portable_runtime_input(false);
    if((state->flags & 0x2000) != 0)
    {
        SDL_ShowCursor();
        std::fprintf(stderr, "%s: %s\n", state->message_table, application_message(state, 16));
    }
    delete state;
}
} // namespace

int run_startup(int argc, char *argv[])
{
    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");
    return SDL_EnterAppMainCallbacks(argc, argv, initialize_startup_callbacks, iterate_startup_callbacks, dispatch_startup_event, shutdown_startup_callbacks);
}


} // namespace gag
