#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include "application_types.h"



struct ImGuiContext;

namespace freegag
{

enum class FrontendMode
{
    LAUNCHER,
    GAME,
    SETTINGS
};

enum class BindingContext
{
    GAME,
    HOST_UI
};

enum class SettingsTab
{
    SETTINGS,
    CONTROLS
};

enum class BindingAction
{
    POINTER_LEFT,
    POINTER_RIGHT,
    POINTER_UP,
    POINTER_DOWN,
    PRIMARY_CLICK,
    SECONDARY_CLICK,
    GAGBOY_HARD_DROP = 10,
    GAGBOY_PAUSE = 11,
    OPEN_SETTINGS = 13,
    RETURN_TO_LAUNCHER = 14
};

struct InputBinding
{
    BindingAction action{};
    SDL_Scancode key{ SDL_SCANCODE_UNKNOWN };
    SDL_GamepadButton button{ SDL_GAMEPAD_BUTTON_INVALID };
};

struct DataInstallation
{
    std::string id;
    std::string title;
    std::filesystem::path directory;
    std::string validation_message;
    bool valid{};
    bool gary{};
};

struct FrontendPreferences
{
    int version{ 1 };
    std::vector<DataInstallation> installations;
    std::string last_selection;
    std::vector<InputBinding> bindings;
    float pointer_speed{ 350.0f };
    float analog_dead_zone{ 0.22f };
    bool first_run_notice_seen{};
};

enum class FrontendDialog
{
    NONE,
    ADD_FOLDER,
    CHANGE_FOLDER,
    IMPORT_DESTINATION,
    IMPORT_CDFS,
    IMPORT_DLL
};

struct ImportState
{
    int step{};
    std::filesystem::path destination;
    std::vector<std::filesystem::path> cdf_sources;
    std::filesystem::path dll_source;
    std::atomic_bool running{};
    std::atomic_bool cancel_requested{};
    std::atomic<uint64_t> completed_bytes{};
    std::atomic<uint64_t> total_bytes{};
    std::jthread worker;
    std::mutex mutex;
    std::string result_message;
    bool succeeded{};
};

struct FrontendState
{
    FrontendMode mode{ FrontendMode::LAUNCHER };
    ApplicationState *game{};
    FrontendPreferences preferences;
    std::filesystem::path preferences_path;
    std::string warning;
    std::string selected_installation;
    std::optional<BindingAction> capture_action;
    std::optional<BindingAction> conflict_action;
    SDL_Scancode pending_capture_key{ SDL_SCANCODE_UNKNOWN };
    SDL_GamepadButton pending_capture_button{ SDL_GAMEPAD_BUTTON_INVALID };
    bool capture_gamepad{};
    bool settings_open{};
    SettingsTab settings_tab{ SettingsTab::SETTINGS };
    bool settings_tab_change_requested{ true };
    bool quit_requested{};
    bool gagboy_requested{};
    bool low_color_restart_required{};
    bool restart_requested{};
    bool restart_gagboy{};
    FrontendDialog active_dialog{ FrontendDialog::NONE };
    std::mutex dialog_mutex;
    bool dialog_completed{};
    std::vector<std::filesystem::path> dialog_paths;
    ImportState import;
    std::vector<SDL_Gamepad *> gamepads;
    std::array<bool, SDL_SCANCODE_COUNT> held_keys{};
    std::array<bool, SDL_GAMEPAD_BUTTON_COUNT> held_gamepad_buttons{};
    bool gagboy_axis_left{};
    bool gagboy_axis_right{};
    bool gagboy_axis_down{};
    bool gagboy_axis_up{};
    float pointer_x{ 320.0f };
    float pointer_y{ 240.0f };
    uint64_t last_frame_ticks{};
    int launcher_window_width{ 960 };
    int launcher_window_height{ 600 };
    bool launcher_window_maximized{};
    SDL_Window *settings_window{};
    SDL_Renderer *settings_renderer{};
    ImGuiContext *main_imgui_context{};
    ImGuiContext *settings_imgui_context{};
    SDL_Scancode suppressed_host_key{ SDL_SCANCODE_UNKNOWN };
    bool settings_chord_latched{};
};

}
