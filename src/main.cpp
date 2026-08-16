#include "startup.h"

// GAG.EXE: 0x0041CAE0
int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR command_line, int show_command)
{
    const gag::StartupApi api = gag::make_win32_startup_api();
    return gag::run_startup(instance, command_line, show_command, api);
}
