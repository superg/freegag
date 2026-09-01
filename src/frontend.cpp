#include "frontend.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <system_error>
#include "application.h"
#include "application_paths.h"
#include "cdf_archive.h"
#include "display_host.h"
#include "extractor.h"
#include "game_host.h"
#include "host_events.h"
#include "platform_menu.h"
#include "portable_path.h"
#include "portable_string.h"
#include "runtime_internal.h"
#include "startup.h"
#include "xtet/api.h"
#include "xtet/resource_provider.h"
#include "xtet/sfs_archive.h"

namespace freegag
{

constexpr int frontend_width = 480;
constexpr int frontend_height = 180;
constexpr int settings_width = 660;
constexpr int settings_height = 620;

static ImVec4 ui_color(float red, float green, float blue, float alpha = 1.0f)
{
    return ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, alpha);
}

static std::string path_text(const std::filesystem::path &path)
{
    const std::u8string value = path.u8string();
    return { reinterpret_cast<const char *>(value.data()), value.size() };
}

static ImFont *add_system_ui_font(ImGuiIO &io, float size_pixels, float rasterizer_density = 1.0f)
{
    std::vector<std::filesystem::path> candidates;
#if defined(SDL_PLATFORM_MACOS)
    candidates = {
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
#elif defined(SDL_PLATFORM_WINDOWS)
    if(const char *windows_directory = std::getenv("WINDIR"))
    {
        candidates.emplace_back(std::filesystem::path(windows_directory) / "Fonts" / "segoeui.ttf");
        candidates.emplace_back(std::filesystem::path(windows_directory) / "Fonts" / "arial.ttf");
    }
#else
    candidates = {
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
#endif

    ImFontConfig font_config;
    font_config.RasterizerDensity = rasterizer_density;
    for(const std::filesystem::path &candidate : candidates)
    {
        std::error_code error;
        if(!std::filesystem::is_regular_file(candidate, error))
            continue;
        const std::string filename = path_text(candidate);
        if(ImFont *font = io.Fonts->AddFontFromFileTTF(filename.c_str(), size_pixels, &font_config))
            return font;
    }

    font_config.SizePixels = size_pixels;
    return io.Fonts->AddFontDefault(&font_config);
}

static std::string lowercase_ascii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

static BindingContext binding_context(BindingAction action)
{
    return action <= BindingAction::GAGBOY_PAUSE ? BindingContext::GAME : BindingContext::HOST_UI;
}

static const char *binding_action_name(BindingAction action)
{
    switch(action)
    {
    case BindingAction::POINTER_LEFT:
        return "Left";
    case BindingAction::POINTER_RIGHT:
        return "Right";
    case BindingAction::POINTER_UP:
        return "Up";
    case BindingAction::POINTER_DOWN:
        return "Down";
    case BindingAction::PRIMARY_CLICK:
        return "Primary Action";
    case BindingAction::SECONDARY_CLICK:
        return "Secondary Action";
    case BindingAction::GAGBOY_HARD_DROP:
        return "Hard Drop";
    case BindingAction::GAGBOY_PAUSE:
        return "Pause";
    case BindingAction::OPEN_SETTINGS:
        return "Open Settings";
    case BindingAction::RETURN_TO_LAUNCHER:
        return "Stop Game";
    }
    return "Unknown";
}

static const char *gamepad_button_name(SDL_GamepadButton button)
{
    switch(button)
    {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return "South / A";
    case SDL_GAMEPAD_BUTTON_EAST:
        return "East / B";
    case SDL_GAMEPAD_BUTTON_WEST:
        return "West / X";
    case SDL_GAMEPAD_BUTTON_NORTH:
        return "North / Y";
    case SDL_GAMEPAD_BUTTON_BACK:
        return "Back";
    case SDL_GAMEPAD_BUTTON_GUIDE:
        return "Guide / Home";
    case SDL_GAMEPAD_BUTTON_START:
        return "Start";
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return "L1";
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return "R1";
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return "D-pad Up";
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return "D-pad Down";
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return "D-pad Left";
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return "D-pad Right";
    default:
        return "Unassigned";
    }
}

static std::vector<InputBinding> default_bindings()
{
    return {
        { BindingAction::POINTER_LEFT,       SDL_SCANCODE_LEFT,  SDL_GAMEPAD_BUTTON_DPAD_LEFT  },
        { BindingAction::POINTER_RIGHT,      SDL_SCANCODE_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT },
        { BindingAction::POINTER_UP,         SDL_SCANCODE_UP,    SDL_GAMEPAD_BUTTON_DPAD_UP    },
        { BindingAction::POINTER_DOWN,       SDL_SCANCODE_DOWN,  SDL_GAMEPAD_BUTTON_DPAD_DOWN  },
        { BindingAction::PRIMARY_CLICK,      SDL_SCANCODE_Z,     SDL_GAMEPAD_BUTTON_SOUTH      },
        { BindingAction::SECONDARY_CLICK,    SDL_SCANCODE_X,     SDL_GAMEPAD_BUTTON_EAST       },
        { BindingAction::GAGBOY_HARD_DROP,   SDL_SCANCODE_SPACE, SDL_GAMEPAD_BUTTON_WEST       },
        { BindingAction::GAGBOY_PAUSE,       SDL_SCANCODE_P,     SDL_GAMEPAD_BUTTON_START      },
        { BindingAction::OPEN_SETTINGS,      SDL_SCANCODE_F10,   SDL_GAMEPAD_BUTTON_GUIDE      },
        { BindingAction::RETURN_TO_LAUNCHER, SDL_SCANCODE_F9,    SDL_GAMEPAD_BUTTON_BACK       },
    };
}

static InputBinding *find_binding(FrontendState *frontend, BindingAction action)
{
    const auto found = std::find_if(frontend->preferences.bindings.begin(), frontend->preferences.bindings.end(), [action](const InputBinding &binding) { return binding.action == action; });
    return found == frontend->preferences.bindings.end() ? nullptr : &*found;
}

static bool is_settings_key(FrontendState *frontend, const SDL_KeyboardEvent &event)
{
    const InputBinding *binding = find_binding(frontend, BindingAction::OPEN_SETTINGS);
    return (binding != nullptr && event.scancode == binding->key) || (event.scancode == SDL_SCANCODE_COMMA && (event.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0);
}

static bool is_settings_button(FrontendState *frontend, SDL_GamepadButton button, SDL_Gamepad *gamepad)
{
    const InputBinding *binding = find_binding(frontend, BindingAction::OPEN_SETTINGS);
    const bool chord = gamepad != nullptr && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) && SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    return (binding != nullptr && button == binding->button) || chord;
}

static bool validate_cdf(const std::filesystem::path &path)
{
    const std::string native = host_path_to_utf8(path);
    CdfArchive *archive = open_cdf_archive(native.c_str(), 0);
    if(archive == nullptr)
        return false;
    close_cdf_archive(archive);
    return true;
}

static bool validate_sfs(const std::filesystem::path &path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if(!stream)
        return false;
    const std::streamoff length = stream.tellg();
    if(length <= 0 || static_cast<uint64_t>(length) > static_cast<uint64_t>(SIZE_MAX))
        return false;
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    xtet::SfsArchive archive;
    return stream && archive.mount({ bytes.data(), bytes.size() }) && archive.find("acts.txt") != nullptr && archive.find("base_scr.txt") != nullptr;
}

static DataInstallation validate_installation(const std::filesystem::path &requested)
{
    DataInstallation installation;
    std::error_code error;
    installation.directory = std::filesystem::absolute(requested, error).lexically_normal();
    if(error)
    {
        error.clear();
        installation.directory = requested.lexically_normal();
    }
    installation.id = path_text(installation.directory);
    if(error || !std::filesystem::is_directory(installation.directory, error))
    {
        installation.validation_message = "Folder is unavailable.";
        return installation;
    }

    std::vector<std::filesystem::path> numbered_cdfs;
    std::filesystem::path gag01;
    std::filesystem::path gag02;
    std::filesystem::path gary;
    std::filesystem::path sfs;
    for(std::filesystem::directory_iterator iterator(installation.directory, error), end; !error && iterator != end; iterator.increment(error))
    {
        if(!iterator->is_regular_file(error))
            continue;
        const std::string name = lowercase_ascii(path_text(iterator->path().filename()));
        if(name == "gag01.cdf")
            gag01 = iterator->path();
        else if(name == "gag02.cdf")
            gag02 = iterator->path();
        else if(name == "gary.cdf")
            gary = iterator->path();
        else if(name == "xtetdll.sfs")
            sfs = iterator->path();
        const bool numbered_gag =
            name.size() > 7 && name.starts_with("gag") && name.ends_with(".cdf") && std::all_of(name.begin() + 3, name.end() - 4, [](unsigned char character) { return std::isdigit(character) != 0; });
        if(numbered_gag || name == "gary.cdf")
            numbered_cdfs.push_back(iterator->path());
    }
    if(!gag01.empty() && !gary.empty())
    {
        installation.validation_message = "This folder mixes GAG and GAG+ data. Put each game in a separate folder.";
        return installation;
    }
    const bool gary_installation = !gary.empty();
    if(!gary_installation && (gag01.empty() || gag02.empty()))
    {
        installation.validation_message = gary_installation ? "GARY.CDF is missing." : "GAG01.CDF and GAG02.CDF are required.";
        return installation;
    }
    if(sfs.empty())
    {
        installation.validation_message = "XTETDLL.SFS is missing.";
        return installation;
    }
    for(const std::filesystem::path &cdf : numbered_cdfs)
        if(!validate_cdf(cdf))
        {
            installation.validation_message = path_text(cdf.filename()) + " is not a valid CDF archive.";
            return installation;
        }
    if(!validate_sfs(sfs))
    {
        installation.validation_message = "XTETDLL.SFS is not a valid SFS archive.";
        return installation;
    }
    installation.valid = true;
    installation.validation_message = "Ready to play";
    return installation;
}

struct GameSettings
{
    bool fullscreen{};
    bool integer_scaling{};
    bool low_color{};
    bool sound{ true };
    bool subtitles{};
};

static bool parse_setting_value(const std::string &line, const char *key, bool default_value)
{
    const std::string prefix = std::string(key) + "=";
    if(!line.starts_with(prefix))
        return default_value;
    return lowercase_ascii(line.substr(prefix.size())) == "true";
}

static GameSettings load_game_settings(const std::filesystem::path &directory)
{
    GameSettings settings;
    std::ifstream stream(directory / "freegag.ini");
    std::string section;
    std::string line;
    while(std::getline(stream, line))
    {
        if(line == "[Game]")
        {
            section = "Game";
            continue;
        }
        if(!line.empty() && line.front() == '[')
        {
            section.clear();
            continue;
        }
        if(section != "Game")
            continue;
        if(line.starts_with("Fullscreen="))
            settings.fullscreen = parse_setting_value(line, "Fullscreen", settings.fullscreen);
        else if(line.starts_with("IntegerScaling="))
            settings.integer_scaling = parse_setting_value(line, "IntegerScaling", settings.integer_scaling);
        else if(line.starts_with("LowColorResources="))
            settings.low_color = parse_setting_value(line, "LowColorResources", settings.low_color);
        else if(line.starts_with("Sound="))
            settings.sound = parse_setting_value(line, "Sound", settings.sound);
        else if(line.starts_with("Subtitles="))
            settings.subtitles = parse_setting_value(line, "Subtitles", settings.subtitles);
    }
    return settings;
}

static void save_game_settings(const std::filesystem::path &directory, const GameSettings &settings)
{
    std::vector<std::string> window_lines;
    std::ifstream input(directory / "freegag.ini");
    std::string line;
    bool window_section = false;
    while(std::getline(input, line))
    {
        if(line == "[Window]")
            window_section = true;
        else if(!line.empty() && line.front() == '[')
            window_section = false;
        if(window_section)
            window_lines.push_back(line);
    }
    const std::filesystem::path target = directory / "freegag.ini";
    const std::filesystem::path temporary = directory / "freegag.ini.tmp";
    std::ofstream output(temporary, std::ios::trunc);
    output << "[Game]\n";
    output << "Fullscreen=" << (settings.fullscreen ? "true" : "false") << '\n';
    output << "IntegerScaling=" << (settings.integer_scaling ? "true" : "false") << '\n';
    output << "LowColorResources=" << (settings.low_color ? "true" : "false") << '\n';
    output << "Sound=" << (settings.sound ? "true" : "false") << '\n';
    output << "Subtitles=" << (settings.subtitles ? "true" : "false") << '\n';
    if(!window_lines.empty())
    {
        output << '\n';
        for(const std::string &window_line : window_lines)
            output << window_line << '\n';
    }
    output.close();
    std::error_code error;
    std::filesystem::rename(temporary, target, error);
    if(error)
    {
        std::filesystem::remove(target, error);
        error.clear();
        std::filesystem::rename(temporary, target, error);
    }
}

static nlohmann::json preferences_json(const FrontendPreferences &preferences)
{
    nlohmann::json result;
    result["version"] = 1;
    result["lastSelection"] = preferences.last_selection;
    result["pointerSpeed"] = preferences.pointer_speed;
    result["analogDeadZone"] = preferences.analog_dead_zone;
    result["installations"] = nlohmann::json::array();
    for(const DataInstallation &installation : preferences.installations)
        result["installations"].push_back({
            { "id",        installation.id                   },
            { "directory", path_text(installation.directory) }
        });
    result["bindings"] = nlohmann::json::array();
    for(const InputBinding &binding : preferences.bindings)
        result["bindings"].push_back({
            { "action", static_cast<int>(binding.action) },
            { "key",    static_cast<int>(binding.key)    },
            { "button", static_cast<int>(binding.button) }
        });
    return result;
}

static void save_preferences(FrontendState *frontend)
{
    std::error_code error;
    std::filesystem::create_directories(frontend->preferences_path.parent_path(), error);
    std::filesystem::path temporary = frontend->preferences_path;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::trunc);
        if(!stream)
        {
            frontend->warning = "Could not save frontend preferences.";
            return;
        }
        stream << std::setw(2) << preferences_json(frontend->preferences) << '\n';
        if(!stream)
        {
            std::filesystem::remove(temporary, error);
            frontend->warning = "Could not finish writing frontend preferences.";
            return;
        }
    }
    std::filesystem::rename(temporary, frontend->preferences_path, error);
    if(!error)
        return;
    std::filesystem::path backup = frontend->preferences_path;
    backup += ".bak";
    error.clear();
    std::filesystem::remove(backup, error);
    error.clear();
    if(std::filesystem::exists(frontend->preferences_path, error))
        std::filesystem::rename(frontend->preferences_path, backup, error);
    if(!error)
        std::filesystem::rename(temporary, frontend->preferences_path, error);
    if(!error)
        std::filesystem::remove(backup, error);
    else
    {
        std::error_code restore_error;
        if(!std::filesystem::exists(frontend->preferences_path, restore_error) && std::filesystem::exists(backup, restore_error))
            std::filesystem::rename(backup, frontend->preferences_path, restore_error);
        frontend->warning = "Could not atomically replace frontend preferences.";
    }
}

static void load_preferences(FrontendState *frontend)
{
    frontend->preferences.bindings = default_bindings();
    std::ifstream stream(frontend->preferences_path);
    if(!stream)
        return;
    try
    {
        const nlohmann::json source = nlohmann::json::parse(stream);
        if(source.value("version", 0) != 1)
            throw std::runtime_error("unsupported schema version");
        frontend->preferences.last_selection = source.value("lastSelection", "");
        frontend->preferences.pointer_speed = std::clamp(source.value("pointerSpeed", 350.0f), 80.0f, 1200.0f);
        frontend->preferences.analog_dead_zone = std::clamp(source.value("analogDeadZone", 0.22f), 0.05f, 0.9f);
        for(const nlohmann::json &item : source.value("installations", nlohmann::json::array()))
        {
            const std::string directory = item.value("directory", "");
            if(!directory.empty())
                frontend->preferences.installations.push_back(validate_installation(host_path_from_utf8(directory.c_str())));
        }
        if(!frontend->preferences.installations.empty())
        {
            const auto selected = std::find_if(frontend->preferences.installations.begin(), frontend->preferences.installations.end(),
                [frontend](const DataInstallation &installation) { return installation.id == frontend->preferences.last_selection; });
            DataInstallation installation = selected == frontend->preferences.installations.end() ? std::move(frontend->preferences.installations.front()) : std::move(*selected);
            frontend->preferences.installations.clear();
            frontend->preferences.installations.push_back(std::move(installation));
            frontend->preferences.last_selection = frontend->preferences.installations.front().id;
        }
        for(const nlohmann::json &item : source.value("bindings", nlohmann::json::array()))
        {
            const int action_value = item.value("action", -1);
            if(action_value < 0 || action_value > static_cast<int>(BindingAction::RETURN_TO_LAUNCHER))
                continue;
            InputBinding *binding = find_binding(frontend, static_cast<BindingAction>(action_value));
            if(binding != nullptr)
            {
                binding->key = static_cast<SDL_Scancode>(item.value("key", static_cast<int>(binding->key)));
                binding->button = static_cast<SDL_GamepadButton>(item.value("button", static_cast<int>(binding->button)));
            }
        }
    }
    catch(const std::exception &exception)
    {
        frontend->preferences = {};
        frontend->preferences.bindings = default_bindings();
        frontend->warning = std::string("frontend.json is malformed; defaults are in use. ") + exception.what();
    }
}

static void add_installation(FrontendState *frontend, const std::filesystem::path &directory)
{
    DataInstallation candidate = validate_installation(directory);
    const std::string candidate_id = candidate.id;
    frontend->preferences.installations.clear();
    frontend->preferences.installations.push_back(std::move(candidate));
    frontend->selected_installation = candidate_id;
    frontend->preferences.last_selection = frontend->selected_installation;
    save_preferences(frontend);
}

static DataInstallation *selected_installation(FrontendState *frontend)
{
    const auto found = std::find_if(frontend->preferences.installations.begin(), frontend->preferences.installations.end(),
        [frontend](const DataInstallation &entry) { return entry.id == frontend->selected_installation; });
    return found == frontend->preferences.installations.end() ? nullptr : &*found;
}

static bool begin_game(FrontendState *frontend, DataInstallation &installation, bool gagboy)
{
    installation = validate_installation(installation.directory);
    if(!installation.valid)
    {
        frontend->warning = installation.validation_message;
        return false;
    }
    set_sdl_presenter_integer_scaling(false);
    configure_sdl_presenter_logical_size(640, 480);
    frontend->game = initialize_gag_application(640, 480, path_text(installation.directory).c_str());
    if(frontend->game == nullptr)
    {
        configure_sdl_presenter_logical_size(frontend_width, frontend_height);
        frontend->warning = "The game engine could not start with this data folder.";
        return false;
    }
    if((frontend->game->flags & APPLICATION_WINDOWED) != 0)
    {
        SDL_RestoreWindow(get_sdl_presenter_window());
        SDL_SetWindowMinimumSize(get_sdl_presenter_window(), frontend->game->width, frontend->game->height);
        SDL_SetWindowSize(get_sdl_presenter_window(), frontend->game->width, frontend->game->height);
    }
    if(gagboy)
        copy_string(frontend->game->startup_config, "GAGBOY.CFG");
    graphics_host_flags |= RUNTIME_HOST_COMMAND_STOP_REQUESTED;
    frontend->selected_installation = installation.id;
    frontend->preferences.last_selection = installation.id;
    frontend->mode = FrontendMode::GAME;
    frontend->last_frame_ticks = SDL_GetTicks();
    SDL_HideCursor();
    save_preferences(frontend);
    return true;
}

static void close_frontend_settings(FrontendState *frontend)
{
    if(!frontend->settings_open)
        return;
    frontend->settings_open = false;
    frontend->capture_action.reset();
    frontend->conflict_action.reset();
    if(frontend->settings_window != nullptr)
        SDL_HideWindow(frontend->settings_window);
    if(frontend->game != nullptr && frontend->mode == FrontendMode::SETTINGS)
    {
        dispatch_application_action(frontend->game, ApplicationAction::RESUME);
        frontend->mode = FrontendMode::GAME;
        SDL_HideCursor();
    }
}

static void request_game_shutdown(FrontendState *frontend, bool quit)
{
    frontend->quit_requested = frontend->quit_requested || quit;
    close_frontend_settings(frontend);
    if(frontend->game != nullptr && (frontend->game->flags & APPLICATION_EXIT_REQUESTED) == 0)
        dispatch_application_action(frontend->game, ApplicationAction::EXIT);
}

static void show_frontend_settings(FrontendState *frontend, SettingsTab tab = SettingsTab::SETTINGS)
{
    if(frontend->settings_window == nullptr)
    {
        frontend->warning = "The settings window could not be initialized.";
        return;
    }
    frontend->settings_tab = tab;
    frontend->settings_tab_change_requested = true;
    frontend->settings_open = true;
    if(frontend->game != nullptr && frontend->mode == FrontendMode::GAME)
    {
        frontend->mode = FrontendMode::SETTINGS;
        dispatch_application_action(frontend->game, ApplicationAction::PAUSE);
    }
    SDL_ShowCursor();
    SDL_ShowWindow(frontend->settings_window);
    SDL_RaiseWindow(frontend->settings_window);
}

static void finish_game_shutdown(FrontendState *frontend)
{
    if(frontend->game == nullptr || !frontend->game->shutdown_complete)
        return;
    const bool fatal_error = (frontend->game->flags & APPLICATION_FATAL_ERROR) != 0;
    delete frontend->game;
    frontend->game = nullptr;
    frontend->mode = FrontendMode::LAUNCHER;
    frontend->held_keys.fill(false);
    frontend->held_gamepad_buttons.fill(false);
    frontend->gagboy_axis_left = false;
    frontend->gagboy_axis_right = false;
    frontend->gagboy_axis_down = false;
    frontend->gagboy_axis_up = false;
    frontend->suppressed_host_key = SDL_SCANCODE_UNKNOWN;
    frontend->settings_chord_latched = false;
    if(!frontend->restart_requested)
        set_sdl_presenter_fullscreen(false);
    configure_sdl_presenter_logical_size(frontend_width, frontend_height);
    SDL_RestoreWindow(get_sdl_presenter_window());
    SDL_SetWindowMinimumSize(get_sdl_presenter_window(), 420, 160);
    SDL_SetWindowSize(get_sdl_presenter_window(), frontend_width, frontend_height);
    SDL_SetWindowTitle(get_sdl_presenter_window(), "FreeGAG");
    SDL_ShowCursor();
    if(fatal_error)
        frontend->warning = "The game stopped because of an internal or data error. Revalidate the installation and review the terminal log for details.";
    if(frontend->restart_requested)
    {
        frontend->restart_requested = false;
        if(DataInstallation *installation = selected_installation(frontend))
            begin_game(frontend, *installation, frontend->restart_gagboy);
    }
}

static void SDLCALL dialog_callback(void *userdata, const char *const *filelist, int)
{
    auto *frontend = static_cast<FrontendState *>(userdata);
    std::lock_guard lock(frontend->dialog_mutex);
    frontend->dialog_paths.clear();
    if(filelist != nullptr)
        for(size_t index = 0; filelist[index] != nullptr; ++index)
            frontend->dialog_paths.emplace_back(host_path_from_utf8(filelist[index]));
    frontend->dialog_completed = true;
}

static void show_dialog(FrontendState *frontend, FrontendDialog dialog)
{
    if(frontend->active_dialog != FrontendDialog::NONE)
        return;
    frontend->active_dialog = dialog;
    frontend->dialog_completed = false;
    SDL_Window *parent = frontend->settings_open && frontend->settings_window != nullptr ? frontend->settings_window : get_sdl_presenter_window();
    if(dialog == FrontendDialog::CHANGE_FOLDER || dialog == FrontendDialog::IMPORT_DESTINATION)
    {
        SDL_ShowOpenFolderDialog(dialog_callback, frontend, parent, nullptr, false);
        return;
    }
    const SDL_DialogFileFilter cdf_filter[] = {
        { "CDF archives", "cdf;CDF" }
    };
    const SDL_DialogFileFilter dll_filter[] = {
        { "XTETDLL.DLL", "dll;DLL" }
    };
    SDL_ShowOpenFileDialog(dialog_callback, frontend, parent, dialog == FrontendDialog::IMPORT_CDFS ? cdf_filter : dll_filter, 1, nullptr, dialog == FrontendDialog::IMPORT_CDFS);
}

static void consume_dialog(FrontendState *frontend)
{
    FrontendDialog completed_dialog;
    std::vector<std::filesystem::path> paths;
    {
        std::lock_guard lock(frontend->dialog_mutex);
        if(!frontend->dialog_completed)
            return;
        completed_dialog = frontend->active_dialog;
        paths = std::move(frontend->dialog_paths);
        frontend->dialog_completed = false;
        frontend->active_dialog = FrontendDialog::NONE;
    }
    if(paths.empty())
        return;
    if(completed_dialog == FrontendDialog::CHANGE_FOLDER)
    {
        add_installation(frontend, paths.front());
        DataInstallation *installation = selected_installation(frontend);
        if(frontend->game != nullptr && installation != nullptr && installation->valid)
        {
            frontend->restart_requested = true;
            frontend->restart_gagboy = xtet::game_active();
            request_game_shutdown(frontend, false);
        }
        else if(installation != nullptr && installation->valid)
            frontend->launch_configured_game_requested = true;
    }
    else if(completed_dialog == FrontendDialog::IMPORT_DESTINATION)
    {
        frontend->import.destination = paths.front();
        frontend->import.step = 2;
    }
    else if(completed_dialog == FrontendDialog::IMPORT_CDFS)
    {
        frontend->import.cdf_sources = std::move(paths);
        frontend->import.step = 3;
    }
    else if(completed_dialog == FrontendDialog::IMPORT_DLL)
    {
        frontend->import.dll_source = paths.front();
        frontend->import.step = 4;
    }
}

static bool files_identical(const std::filesystem::path &left, const std::filesystem::path &right)
{
    std::error_code error;
    if(std::filesystem::file_size(left, error) != std::filesystem::file_size(right, error) || error)
        return false;
    std::ifstream first(left, std::ios::binary);
    std::ifstream second(right, std::ios::binary);
    std::array<char, 64 * 1024> first_buffer{};
    std::array<char, 64 * 1024> second_buffer{};
    while(first && second)
    {
        first.read(first_buffer.data(), first_buffer.size());
        second.read(second_buffer.data(), second_buffer.size());
        if(first.gcount() != second.gcount() || !std::equal(first_buffer.begin(), first_buffer.begin() + first.gcount(), second_buffer.begin()))
            return false;
    }
    return first.eof() && second.eof();
}

static bool copy_with_progress(ImportState *import, const std::filesystem::path &source, const std::filesystem::path &destination)
{
    std::ifstream input(source, std::ios::binary);
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    std::array<char, 1024 * 1024> buffer{};
    while(input && output && !import->cancel_requested)
    {
        input.read(buffer.data(), buffer.size());
        const std::streamsize count = input.gcount();
        if(count > 0)
        {
            output.write(buffer.data(), count);
            import->completed_bytes += static_cast<uint64_t>(count);
        }
    }
    return input.eof() && output && !import->cancel_requested;
}

static void run_import(FrontendState *frontend)
{
    ImportState *import = &frontend->import;
    import->running = true;
    import->cancel_requested = false;
    import->completed_bytes = 0;
    import->total_bytes = 0;
    import->succeeded = false;
    {
        std::lock_guard lock(import->mutex);
        import->result_message.clear();
    }
    std::error_code error;
    for(const std::filesystem::path &source : import->cdf_sources)
        import->total_bytes += std::filesystem::file_size(source, error);
    import->total_bytes += std::filesystem::file_size(import->dll_source, error);
    const std::filesystem::path staging = import->destination / (".freegag-import-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(staging, error);
    std::string message;
    std::vector<std::filesystem::path> committed_files;
    try
    {
        if(error)
            throw std::runtime_error("Could not create the temporary import folder.");
        std::filesystem::create_directories(import->destination, error);
        for(const std::filesystem::path &source : import->cdf_sources)
        {
            const std::filesystem::path staged = staging / source.filename();
            if(std::filesystem::exists(staged, error))
            {
                if(files_identical(source, staged))
                {
                    import->completed_bytes += std::filesystem::file_size(source, error);
                    continue;
                }
                throw std::runtime_error("Two selected CDF files have the same name but different contents.");
            }
            if(!copy_with_progress(import, source, staged))
                throw std::runtime_error(import->cancel_requested ? "Import canceled." : "A CDF file could not be copied.");
        }
        const std::filesystem::path staged_sfs = staging / "XTETDLL.SFS";
        extract_xtet_resource(staged_sfs, import->dll_source);
        import->completed_bytes = import->total_bytes.load();
        if(import->cancel_requested)
            throw std::runtime_error("Import canceled.");
        const DataInstallation validation = validate_installation(staging);
        if(!validation.valid)
            throw std::runtime_error("Imported data failed validation: " + validation.validation_message);
        std::vector<std::filesystem::path> staged_files;
        for(std::filesystem::directory_iterator iterator(staging), end; iterator != end; ++iterator)
            staged_files.push_back(iterator->path());
        for(const std::filesystem::path &source : staged_files)
        {
            const std::filesystem::path destination = import->destination / source.filename();
            if(std::filesystem::exists(destination, error) && !files_identical(source, destination))
                throw std::runtime_error(path_text(destination.filename()) + " already exists with different contents.");
        }
        for(const std::filesystem::path &source : staged_files)
        {
            const std::filesystem::path destination = import->destination / source.filename();
            if(std::filesystem::exists(destination, error))
                continue;
            std::filesystem::rename(source, destination, error);
            if(error)
                throw std::runtime_error("Could not commit " + path_text(destination.filename()) + ".");
            committed_files.push_back(destination);
        }
        import->succeeded = true;
        message = "Import completed and the game assets are configured.";
    }
    catch(const std::exception &exception)
    {
        message = exception.what();
        for(const std::filesystem::path &committed : committed_files)
        {
            std::error_code cleanup_error;
            std::filesystem::remove(committed, cleanup_error);
        }
    }
    std::filesystem::remove_all(staging, error);
    {
        std::lock_guard lock(import->mutex);
        import->result_message = std::move(message);
    }
    import->running = false;
}

static void start_import(FrontendState *frontend)
{
    if(frontend->import.running)
        return;
    frontend->import.worker = std::jthread([frontend] { run_import(frontend); });
    frontend->import.step = 5;
}

static void refresh_finished_import(FrontendState *frontend)
{
    if(frontend->import.step != 5 || frontend->import.running || !frontend->import.succeeded)
        return;
    add_installation(frontend, frontend->import.destination);
    if(DataInstallation *installation = selected_installation(frontend); installation != nullptr && installation->valid && frontend->game == nullptr)
        frontend->launch_configured_game_requested = true;
    frontend->import.step = 6;
}

static std::string detect_import_game(const std::vector<std::filesystem::path> &sources, bool *ready)
{
    bool gag01 = false;
    bool gag02 = false;
    bool gary = false;
    for(const std::filesystem::path &source : sources)
    {
        const std::string name = lowercase_ascii(path_text(source.filename()));
        gag01 = gag01 || name == "gag01.cdf";
        gag02 = gag02 || name == "gag02.cdf";
        gary = gary || name == "gary.cdf";
    }
    if(gary && (gag01 || gag02))
    {
        *ready = false;
        return "Ambiguous mixed GAG/GAG+ selection";
    }
    if(gary)
    {
        *ready = true;
        return "GAG+: Harry on Vacation";
    }
    *ready = gag01 && gag02;
    return *ready ? "GAG: The Impotent Mystery" : "Incomplete GAG selection (GAG01.CDF and GAG02.CDF are required)";
}

static void render_import(FrontendState *frontend)
{
    if(!ImGui::BeginPopupModal("Disc import", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    ImportState &import = frontend->import;
    ImGui::TextUnformatted("Import game data from original discs");
    ImGui::Separator();
    if(import.step <= 1)
    {
        ImGui::TextWrapped("1. Choose an empty destination folder. Source files are copied, never moved.");
        if(ImGui::Button("Choose Destination"))
            show_dialog(frontend, FrontendDialog::IMPORT_DESTINATION);
    }
    if(import.step >= 2)
    {
        ImGui::Text("Destination: %s", path_text(import.destination).c_str());
        if(import.step == 2 && ImGui::Button("Select all CDF files"))
            show_dialog(frontend, FrontendDialog::IMPORT_CDFS);
    }
    if(import.step >= 3)
    {
        ImGui::Text("CDF archives selected: %zu", import.cdf_sources.size());
        if(import.step == 3 && ImGui::Button("Select XTETDLL.DLL"))
            show_dialog(frontend, FrontendDialog::IMPORT_DLL);
    }
    if(import.step == 4)
    {
        ImGui::Text("DLL: %s", path_text(import.dll_source).c_str());
        bool import_ready = false;
        const std::string detected_game = detect_import_game(import.cdf_sources, &import_ready);
        ImGui::Text("Detected: %s", detected_game.c_str());
        ImGui::TextWrapped("Review the paths above. Existing different files will not be overwritten.");
        ImGui::BeginDisabled(!import_ready);
        if(ImGui::Button("Import and validate"))
            start_import(frontend);
        ImGui::EndDisabled();
    }
    if(import.step == 5)
    {
        const uint64_t total = import.total_bytes.load();
        const float progress = total == 0 ? 0.0f : static_cast<float>(import.completed_bytes.load()) / static_cast<float>(total);
        ImGui::ProgressBar(std::clamp(progress, 0.0f, 1.0f), ImVec2(420, 0));
        if(import.running)
        {
            if(ImGui::Button("Cancel import"))
                import.cancel_requested = true;
        }
        else
            import.step = 6;
    }
    if(import.step == 6)
    {
        std::string message;
        {
            std::lock_guard lock(import.mutex);
            message = import.result_message;
        }
        ImGui::TextWrapped("%s", message.c_str());
        if(ImGui::Button("Done"))
        {
            import.step = 0;
            import.destination.clear();
            import.cdf_sources.clear();
            import.dll_source.clear();
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if(!import.running && ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

static void reset_binding_group(FrontendState *frontend, BindingContext context)
{
    const std::vector<InputBinding> defaults = default_bindings();
    for(InputBinding &binding : frontend->preferences.bindings)
        if(binding_context(binding.action) == context)
        {
            const auto found = std::find_if(defaults.begin(), defaults.end(), [&binding](const InputBinding &candidate) { return candidate.action == binding.action; });
            if(found != defaults.end())
                binding = *found;
        }
    save_preferences(frontend);
}

static void resolve_binding_conflict(FrontendState *frontend, bool replace);

static void render_settings_heading(const char *label)
{
    ImGui::TextUnformatted(label);
    ImGui::Dummy(ImVec2(0, 3.0f));
}

static bool begin_settings_card(const char *id)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 7.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ui_color(37, 37, 39));
    return ImGui::BeginChild(id, ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding);
}

static void end_settings_card()
{
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

static void render_binding_group(FrontendState *frontend, BindingContext context, const char *label)
{
    ImGui::PushID(label);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    const float reset_width = ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - reset_width);
    if(ImGui::Button("Reset"))
        reset_binding_group(frontend, context);
    ImGui::Dummy(ImVec2(0, 2.0f));
    if(begin_settings_card("bindings-card"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(7.0f, 7.0f));
        if(ImGui::BeginTable("bindings", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Keyboard", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableNextRow(0, 28.0f);
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Action");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextDisabled("Keyboard");
            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("Controller");
            for(InputBinding &binding : frontend->preferences.bindings)
            {
                if(binding_context(binding.action) != context)
                    continue;
                ImGui::PushID(static_cast<int>(binding.action));
                ImGui::TableNextRow(0, 40.0f);
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(binding_action_name(binding.action));
                ImGui::TableSetColumnIndex(1);
                const char *key_name = binding.key == SDL_SCANCODE_UNKNOWN ? "Unassigned" : SDL_GetScancodeName(binding.key);
                if(ImGui::Button(key_name, ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    frontend->capture_action = binding.action;
                    frontend->capture_gamepad = false;
                }
                ImGui::TableSetColumnIndex(2);
                if(ImGui::Button(gamepad_button_name(binding.button), ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                {
                    frontend->capture_action = binding.action;
                    frontend->capture_gamepad = true;
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }
    end_settings_card();
    ImGui::PopID();
}

static void render_input_settings(FrontendState *frontend)
{
    ImGui::TextDisabled("Connected controllers: %zu", frontend->gamepads.size());
    ImGui::Dummy(ImVec2(0, 8.0f));
    render_settings_heading("Pointer");
    bool save_requested = false;
    if(begin_settings_card("pointer-card"))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(7.0f, 8.0f));
        if(ImGui::BeginTable("pointer-options", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableSetupColumn("Option", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow(0, 42.0f);
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Pointer speed");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SliderFloat("##pointer-speed", &frontend->preferences.pointer_speed, 80.0f, 1200.0f, "%.0f px/s");
            save_requested = ImGui::IsItemDeactivatedAfterEdit();
            ImGui::TableNextRow(0, 42.0f);
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Analog dead zone");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::SliderFloat("##analog-dead-zone", &frontend->preferences.analog_dead_zone, 0.05f, 0.9f, "%.2f");
            save_requested = ImGui::IsItemDeactivatedAfterEdit() || save_requested;
            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }
    end_settings_card();
    if(save_requested)
        save_preferences(frontend);
    ImGui::Dummy(ImVec2(0, 14.0f));
    render_binding_group(frontend, BindingContext::GAME, "Game Controls");
    ImGui::Dummy(ImVec2(0, 16.0f));
    render_binding_group(frontend, BindingContext::HOST_UI, "Application Shortcuts");
    if(frontend->conflict_action.has_value())
    {
        ImGui::OpenPopup("Replace binding?");
        if(ImGui::BeginPopupModal("Replace binding?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("This input is already assigned to %s in the same context. Replace that assignment?", binding_action_name(*frontend->conflict_action));
            if(ImGui::Button("Replace", ImVec2(120, 0)))
            {
                resolve_binding_conflict(frontend, true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                resolve_binding_conflict(frontend, false);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    else if(frontend->capture_action.has_value())
    {
        ImGui::OpenPopup("Capture binding");
        if(ImGui::BeginPopupModal("Capture binding", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Press a %s for %s.", frontend->capture_gamepad ? "controller button" : "key", binding_action_name(*frontend->capture_action));
            if(ImGui::Button("Cancel"))
            {
                frontend->capture_action.reset();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

static void render_game_data_settings(FrontendState *frontend)
{
    render_settings_heading("Game assets");
    DataInstallation *installation = selected_installation(frontend);
    if(installation != nullptr)
    {
        ImGui::TextDisabled("%s", path_text(installation->directory).c_str());
        ImGui::Text("Status: %s", installation->validation_message.c_str());
        if(ImGui::Button("Change…"))
            show_dialog(frontend, FrontendDialog::CHANGE_FOLDER);
        ImGui::SameLine();
        if(ImGui::Button("Revalidate"))
        {
            *installation = validate_installation(installation->directory);
            if(frontend->game == nullptr && installation->valid)
                frontend->launch_configured_game_requested = true;
        }
    }
    else
    {
        ImGui::TextDisabled("No game assets configured.");
        if(ImGui::Button("Choose…"))
            show_dialog(frontend, FrontendDialog::CHANGE_FOLDER);
    }
    ImGui::SameLine();
    if(ImGui::Button("Import from discs…"))
        ImGui::OpenPopup("Disc import");
}

static void render_display_settings(FrontendState *frontend)
{
    render_settings_heading("Display");
    if(frontend->game != nullptr)
    {
        bool fullscreen = (frontend->game->flags & APPLICATION_WINDOWED) == 0;
        if(ImGui::Checkbox("Fullscreen", &fullscreen))
        {
            set_sdl_presenter_fullscreen(fullscreen);
            if(fullscreen)
                frontend->game->flags = (frontend->game->flags & ~APPLICATION_WINDOWED) | APPLICATION_FULLSCREEN_PREFERENCE;
            else
                frontend->game->flags = (frontend->game->flags | APPLICATION_WINDOWED) & ~APPLICATION_FULLSCREEN_PREFERENCE;
            frontend->game->flags |= APPLICATION_PREFERENCES_CHANGED;
        }
        bool integer_scaling = get_sdl_presenter_integer_scaling();
        if(ImGui::Checkbox("Integer scaling", &integer_scaling))
        {
            set_sdl_presenter_integer_scaling(integer_scaling);
            frontend->game->flags |= APPLICATION_PREFERENCES_CHANGED;
            if(DataInstallation *installation = selected_installation(frontend))
            {
                GameSettings settings = load_game_settings(installation->directory);
                settings.integer_scaling = integer_scaling;
                save_game_settings(installation->directory, settings);
            }
        }
        bool low_color = frontend->game->low_color_resources;
        if(ImGui::Checkbox("Low-color resource variants", &low_color))
        {
            frontend->game->low_color_resources = low_color;
            frontend->game->flags |= APPLICATION_PREFERENCES_CHANGED;
            frontend->low_color_restart_required = true;
        }
        if(frontend->low_color_restart_required)
        {
            ImGui::TextColored(ui_color(184, 126, 70), "Low-color changes require a game restart.");
            if(ImGui::Button("Restart Game"))
            {
                frontend->low_color_restart_required = false;
                frontend->restart_requested = true;
                frontend->restart_gagboy = xtet::game_active();
                request_game_shutdown(frontend, false);
            }
        }
    }
    else if(DataInstallation *installation = selected_installation(frontend))
    {
        GameSettings settings = load_game_settings(installation->directory);
        bool changed = ImGui::Checkbox("Fullscreen", &settings.fullscreen);
        changed = ImGui::Checkbox("Integer scaling", &settings.integer_scaling) || changed;
        changed = ImGui::Checkbox("Low-color resource variants", &settings.low_color) || changed;
        if(changed)
            save_game_settings(installation->directory, settings);
        ImGui::TextDisabled("These values apply the next time this installation starts.");
    }
    else
        ImGui::TextWrapped("Choose a game asset folder to configure display settings.");
}

static void render_audio_text_settings(FrontendState *frontend)
{
    render_settings_heading("Audio & Text");
    if(frontend->game != nullptr)
    {
        bool sound = (frontend->game->flags & APPLICATION_SOUND_MUTED) == 0;
        if(ImGui::Checkbox("Sound", &sound))
            dispatch_application_action(frontend->game, ApplicationAction::TOGGLE_MUTE);
        bool subtitles = (frontend->game->flags & APPLICATION_SUBTITLES_ENABLED) != 0;
        if(ImGui::Checkbox("Subtitles", &subtitles))
            dispatch_application_action(frontend->game, ApplicationAction::TOGGLE_COMMENTS);
    }
    else if(DataInstallation *installation = selected_installation(frontend))
    {
        GameSettings settings = load_game_settings(installation->directory);
        bool changed = ImGui::Checkbox("Sound", &settings.sound);
        changed = ImGui::Checkbox("Subtitles", &settings.subtitles) || changed;
        if(changed)
            save_game_settings(installation->directory, settings);
    }
    else
        ImGui::TextWrapped("Choose a game asset folder to configure sound and subtitle settings.");
}

static void render_combined_settings(FrontendState *frontend)
{
    render_game_data_settings(frontend);
    ImGui::Dummy(ImVec2(0, 10.0f));
    render_display_settings(frontend);
    ImGui::Dummy(ImVec2(0, 10.0f));
    render_audio_text_settings(frontend);
}

static void render_settings(FrontendState *frontend)
{
    const ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(display_size);
    if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        const SettingsTab requested_tab = frontend->settings_tab;
        const bool tab_change_requested = frontend->settings_tab_change_requested;
        if(ImGui::BeginTabBar("settings-pages"))
        {
            const ImGuiTabItemFlags settings_flags = tab_change_requested && requested_tab == SettingsTab::SETTINGS ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if(ImGui::BeginTabItem("Settings", nullptr, settings_flags))
            {
                frontend->settings_tab = SettingsTab::SETTINGS;
                ImGui::EndTabItem();
            }
            const ImGuiTabItemFlags controls_flags = tab_change_requested && requested_tab == SettingsTab::CONTROLS ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if(ImGui::BeginTabItem("Controls", nullptr, controls_flags))
            {
                frontend->settings_tab = SettingsTab::CONTROLS;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        frontend->settings_tab_change_requested = false;
        ImGui::Spacing();
        if(!frontend->warning.empty())
        {
            ImGui::TextColored(ui_color(224, 154, 94), "%s", frontend->warning.c_str());
            ImGui::SameLine();
            if(ImGui::SmallButton("Dismiss"))
                frontend->warning.clear();
            ImGui::Spacing();
        }
        const char *content_id = frontend->settings_tab == SettingsTab::SETTINGS ? "settings-content" : "controls-content";
        if(ImGui::BeginChild(content_id, ImVec2(0, 0), ImGuiChildFlags_None))
        {
            if(frontend->settings_tab == SettingsTab::SETTINGS)
            {
                render_combined_settings(frontend);
                render_import(frontend);
            }
            else
                render_input_settings(frontend);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

static void render_settings_window(FrontendState *frontend)
{
    if(!frontend->settings_open || frontend->settings_imgui_context == nullptr || frontend->settings_renderer == nullptr)
        return;
    ImGui::SetCurrentContext(frontend->settings_imgui_context);
    const float display_scale = SDL_GetWindowDisplayScale(frontend->settings_window);
    SDL_SetRenderScale(frontend->settings_renderer, display_scale, display_scale);
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();
    render_settings(frontend);
    ImGui::Render();
    SDL_SetRenderDrawColor(frontend->settings_renderer, 30, 30, 32, 0xff);
    SDL_RenderClear(frontend->settings_renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), frontend->settings_renderer);
    SDL_RenderPresent(frontend->settings_renderer);
    ImGui::SetCurrentContext(frontend->main_imgui_context);
}

static void render_frontend(FrontendState *frontend)
{
    ImGui::SetCurrentContext(frontend->main_imgui_context);
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    // SDL_Renderer already projects the logical presentation into the high-DPI framebuffer. The platform backend reports the physical display scale, which would otherwise apply that scale a second
    // time to ImGui vertices and clip rectangles.
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::NewFrame();
    if(frontend->mode == FrontendMode::LAUNCHER)
    {
        const ImGuiIO &io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("FreeGAG", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        constexpr const char *message = "Open Settings for configuration.";
        const float message_width = ImGui::CalcTextSize(message).x;
        const float button_width = ImGui::CalcTextSize("Open Settings").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float content_width = std::max(message_width, button_width);
        const float content_height = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeight();
        ImGui::SetCursorPos(ImVec2(std::max(12.0f, (io.DisplaySize.x - content_width) * 0.5f), std::max(12.0f, (io.DisplaySize.y - content_height) * 0.5f)));
        ImGui::TextUnformatted(message);
        ImGui::SetCursorPosX((io.DisplaySize.x - button_width) * 0.5f);
        if(ImGui::Button("Open Settings"))
            show_frontend_settings(frontend);
        ImGui::End();
    }
    ImGui::Render();
    begin_sdl_presenter_frame();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), get_sdl_presenter_renderer());
    present_sdl_presenter_frame();
    render_settings_window(frontend);
}

static void set_capture_binding(FrontendState *frontend, SDL_Scancode key, SDL_GamepadButton button)
{
    if(!frontend->capture_action.has_value())
        return;
    InputBinding *target = find_binding(frontend, *frontend->capture_action);
    if(target == nullptr)
        return;
    for(InputBinding &binding : frontend->preferences.bindings)
    {
        if(binding.action == target->action || binding_context(binding.action) != binding_context(target->action))
            continue;
        if((key != SDL_SCANCODE_UNKNOWN && binding.key == key) || (button != SDL_GAMEPAD_BUTTON_INVALID && binding.button == button))
        {
            frontend->conflict_action = binding.action;
            frontend->pending_capture_key = key;
            frontend->pending_capture_button = button;
            return;
        }
    }
    if(key != SDL_SCANCODE_UNKNOWN)
        target->key = key;
    else
        target->button = button;
    frontend->capture_action.reset();
    save_preferences(frontend);
}

static void resolve_binding_conflict(FrontendState *frontend, bool replace)
{
    if(!frontend->capture_action.has_value() || !frontend->conflict_action.has_value())
        return;
    if(replace)
    {
        InputBinding *conflict = find_binding(frontend, *frontend->conflict_action);
        InputBinding *target = find_binding(frontend, *frontend->capture_action);
        if(conflict != nullptr && target != nullptr)
        {
            if(frontend->pending_capture_key != SDL_SCANCODE_UNKNOWN)
            {
                conflict->key = SDL_SCANCODE_UNKNOWN;
                target->key = frontend->pending_capture_key;
            }
            else
            {
                conflict->button = SDL_GAMEPAD_BUTTON_INVALID;
                target->button = frontend->pending_capture_button;
            }
            frontend->warning = std::string("Replaced the ") + binding_action_name(conflict->action) + " binding in this context.";
            save_preferences(frontend);
        }
    }
    frontend->capture_action.reset();
    frontend->conflict_action.reset();
    frontend->pending_capture_key = SDL_SCANCODE_UNKNOWN;
    frontend->pending_capture_button = SDL_GAMEPAD_BUTTON_INVALID;
}

static uint32_t gagboy_key(BindingAction action)
{
    switch(action)
    {
    case BindingAction::POINTER_LEFT:
        return 0x25;
    case BindingAction::POINTER_RIGHT:
        return 0x27;
    case BindingAction::POINTER_DOWN:
        return 0x28;
    case BindingAction::POINTER_UP:
    case BindingAction::PRIMARY_CLICK:
        return 0x26;
    case BindingAction::SECONDARY_CLICK:
        return 0x1b;
    case BindingAction::GAGBOY_HARD_DROP:
        return 0x20;
    case BindingAction::GAGBOY_PAUSE:
        return 0;
    default:
        return 0;
    }
}

static void send_runtime_key(uint32_t key, bool pressed)
{
    RuntimeInputEvent input;
    input.type = pressed ? RuntimeInputType::KEY_DOWN : RuntimeInputType::KEY_UP;
    input.key = key;
    handle_runtime_input_event(input);
}

static void send_runtime_button(FrontendState *frontend, RuntimeMouseButton button, bool pressed)
{
    RuntimeInputEvent input;
    input.type = pressed ? RuntimeInputType::BUTTON_DOWN : RuntimeInputType::BUTTON_UP;
    input.button = button;
    input.x = static_cast<int32_t>(frontend->pointer_x);
    input.y = static_cast<int32_t>(frontend->pointer_y);
    handle_runtime_input_event(input);
}

static bool dispatch_mapped_button(FrontendState *frontend, SDL_GamepadButton button, bool pressed)
{
    const InputBinding *return_binding = find_binding(frontend, BindingAction::RETURN_TO_LAUNCHER);
    if(pressed && return_binding != nullptr && return_binding->button == button)
    {
        request_game_shutdown(frontend, false);
        return true;
    }
    const bool gagboy = xtet::game_active();
    for(const InputBinding &binding : frontend->preferences.bindings)
    {
        if(binding.button != button)
            continue;
        if(gagboy && binding_context(binding.action) == BindingContext::GAME)
        {
            if(binding.action == BindingAction::GAGBOY_PAUSE)
            {
                if(pressed)
                    xtet::execute_game_command(xtet::game_paused() ? 4 : 2);
            }
            else
                send_runtime_key(gagboy_key(binding.action), pressed);
            return true;
        }
        if(!gagboy && binding.action == BindingAction::PRIMARY_CLICK)
        {
            send_runtime_button(frontend, RuntimeMouseButton::LEFT, pressed);
            return true;
        }
        if(!gagboy && binding.action == BindingAction::SECONDARY_CLICK)
        {
            send_runtime_button(frontend, RuntimeMouseButton::RIGHT, pressed);
            return true;
        }
    }
    return false;
}

static void update_controller_pointer(FrontendState *frontend)
{
    if(frontend->game == nullptr || frontend->mode != FrontendMode::GAME)
        return;
    const uint64_t now = SDL_GetTicks();
    const float elapsed = std::min(0.05f, static_cast<float>(now - frontend->last_frame_ticks) / 1000.0f);
    frontend->last_frame_ticks = now;
    float x = 0.0f;
    float y = 0.0f;
    float axis_x = 0.0f;
    float axis_y = 0.0f;
    if(!frontend->gamepads.empty())
    {
        SDL_Gamepad *gamepad = frontend->gamepads.front();
        axis_x = static_cast<float>(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX)) / 32767.0f;
        axis_y = static_cast<float>(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY)) / 32767.0f;
    }
    if(xtet::game_active())
    {
        const float threshold = frontend->preferences.analog_dead_zone;
        const auto update_axis = [](bool active, bool *latched, uint32_t key)
        {
            if(active != *latched)
            {
                *latched = active;
                send_runtime_key(key, active);
            }
        };
        update_axis(axis_x < -threshold, &frontend->gagboy_axis_left, 0x25);
        update_axis(axis_x > threshold, &frontend->gagboy_axis_right, 0x27);
        update_axis(axis_y > threshold, &frontend->gagboy_axis_down, 0x28);
        update_axis(axis_y < -threshold, &frontend->gagboy_axis_up, 0x26);
        return;
    }
    if(runtime_display_context.input_scene_identifier != 0)
        return;
    const auto held = [frontend](BindingAction action)
    {
        const InputBinding *binding = find_binding(frontend, action);
        return binding != nullptr && binding->key != SDL_SCANCODE_UNKNOWN && frontend->held_keys[binding->key];
    };
    x += held(BindingAction::POINTER_RIGHT) ? 1.0f : 0.0f;
    x -= held(BindingAction::POINTER_LEFT) ? 1.0f : 0.0f;
    y += held(BindingAction::POINTER_DOWN) ? 1.0f : 0.0f;
    y -= held(BindingAction::POINTER_UP) ? 1.0f : 0.0f;
    const auto held_button = [frontend](BindingAction action)
    {
        const InputBinding *binding = find_binding(frontend, action);
        return binding != nullptr && binding->button >= 0 && binding->button < SDL_GAMEPAD_BUTTON_COUNT && frontend->held_gamepad_buttons[binding->button];
    };
    x += held_button(BindingAction::POINTER_RIGHT) ? 1.0f : 0.0f;
    x -= held_button(BindingAction::POINTER_LEFT) ? 1.0f : 0.0f;
    y += held_button(BindingAction::POINTER_DOWN) ? 1.0f : 0.0f;
    y -= held_button(BindingAction::POINTER_UP) ? 1.0f : 0.0f;
    if(std::abs(axis_x) >= frontend->preferences.analog_dead_zone)
        x += axis_x;
    if(std::abs(axis_y) >= frontend->preferences.analog_dead_zone)
        y += axis_y;
    if(x == 0.0f && y == 0.0f)
        return;
    const float length = std::max(1.0f, std::sqrt(x * x + y * y));
    frontend->pointer_x = std::clamp(frontend->pointer_x + x / length * frontend->preferences.pointer_speed * elapsed, 0.0f, 639.0f);
    frontend->pointer_y = std::clamp(frontend->pointer_y + y / length * frontend->preferences.pointer_speed * elapsed, 0.0f, 479.0f);
    RuntimeInputEvent input;
    input.type = RuntimeInputType::POINTER_MOVE;
    input.x = static_cast<int32_t>(frontend->pointer_x);
    input.y = static_cast<int32_t>(frontend->pointer_y);
    handle_runtime_input_event(input);
}

static bool initialize_settings_window(FrontendState *frontend)
{
    frontend->settings_window = SDL_CreateWindow("FreeGAG Settings", settings_width, settings_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if(frontend->settings_window == nullptr)
        return false;
    SDL_SetWindowMinimumSize(frontend->settings_window, 560, 400);
    frontend->settings_renderer = SDL_CreateRenderer(frontend->settings_window, nullptr);
    if(frontend->settings_renderer == nullptr)
    {
        SDL_DestroyWindow(frontend->settings_window);
        frontend->settings_window = nullptr;
        return false;
    }
    frontend->settings_imgui_context = ImGui::CreateContext();
    if(frontend->settings_imgui_context == nullptr)
    {
        SDL_DestroyRenderer(frontend->settings_renderer);
        SDL_DestroyWindow(frontend->settings_window);
        frontend->settings_renderer = nullptr;
        frontend->settings_window = nullptr;
        return false;
    }
    ImGui::SetCurrentContext(frontend->settings_imgui_context);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;
    add_system_ui_font(io, 16.0f, std::max(1.0f, SDL_GetWindowDisplayScale(frontend->settings_window)));
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(12.0f, 10.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.Colors[ImGuiCol_Text] = ui_color(235, 235, 237);
    style.Colors[ImGuiCol_TextDisabled] = ui_color(152, 152, 157);
    style.Colors[ImGuiCol_WindowBg] = ui_color(30, 30, 32);
    style.Colors[ImGuiCol_ChildBg] = ui_color(30, 30, 32);
    style.Colors[ImGuiCol_PopupBg] = ui_color(38, 38, 40);
    style.Colors[ImGuiCol_Border] = ui_color(72, 72, 74);
    style.Colors[ImGuiCol_TableBorderLight] = ui_color(55, 55, 58);
    style.Colors[ImGuiCol_TableBorderStrong] = ui_color(64, 64, 67);
    style.Colors[ImGuiCol_FrameBg] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_FrameBgHovered] = ui_color(58, 58, 60);
    style.Colors[ImGuiCol_FrameBgActive] = ui_color(68, 68, 70);
    style.Colors[ImGuiCol_Button] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_ButtonHovered] = ui_color(58, 58, 60);
    style.Colors[ImGuiCol_ButtonActive] = ui_color(68, 68, 70);
    style.Colors[ImGuiCol_Header] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_HeaderHovered] = ui_color(58, 58, 60);
    style.Colors[ImGuiCol_HeaderActive] = ui_color(68, 68, 70);
    style.Colors[ImGuiCol_Tab] = ui_color(30, 30, 32);
    style.Colors[ImGuiCol_TabHovered] = ui_color(50, 50, 52);
    style.Colors[ImGuiCol_TabSelected] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_CheckMark] = ui_color(90, 156, 255);
    style.Colors[ImGuiCol_SliderGrab] = ui_color(90, 156, 255);
    style.Colors[ImGuiCol_SliderGrabActive] = ui_color(122, 178, 255);
    if(!ImGui_ImplSDL3_InitForSDLRenderer(frontend->settings_window, frontend->settings_renderer) || !ImGui_ImplSDLRenderer3_Init(frontend->settings_renderer))
    {
        if(ImGui::GetIO().BackendRendererUserData != nullptr)
            ImGui_ImplSDLRenderer3_Shutdown();
        if(ImGui::GetIO().BackendPlatformUserData != nullptr)
            ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(frontend->settings_imgui_context);
        SDL_DestroyRenderer(frontend->settings_renderer);
        SDL_DestroyWindow(frontend->settings_window);
        frontend->settings_imgui_context = nullptr;
        frontend->settings_renderer = nullptr;
        frontend->settings_window = nullptr;
        ImGui::SetCurrentContext(frontend->main_imgui_context);
        return false;
    }
    ImGui::SetCurrentContext(frontend->main_imgui_context);
    return true;
}

FrontendState *initialize_frontend(int argc, char *argv[])
{
    if(initialize_sdl_shell(frontend_width, frontend_height, "FreeGAG") != 0)
        return nullptr;
    auto *frontend = new (std::nothrow) FrontendState{};
    if(frontend == nullptr)
    {
        shutdown_sdl_shell();
        return nullptr;
    }
    const char *preference_root = SDL_GetPrefPath("superg", "FreeGAG");
    if(preference_root != nullptr)
        frontend->preferences_path = host_path_from_utf8(preference_root) / "frontend.json";
    else
        frontend->preferences_path = "frontend.json";
    load_preferences(frontend);
    frontend->selected_installation = frontend->preferences.last_selection;

    IMGUI_CHECKVERSION();
    frontend->main_imgui_context = ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;
    add_system_ui_font(io, 18.0f);
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.ChildRounding = 7.0f;
    style.FrameRounding = 5.0f;
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.Colors[ImGuiCol_WindowBg] = ui_color(30, 30, 32);
    style.Colors[ImGuiCol_Header] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_Button] = ui_color(46, 46, 48);
    style.Colors[ImGuiCol_ButtonHovered] = ui_color(58, 58, 60);
    style.Colors[ImGuiCol_ButtonActive] = ui_color(68, 68, 70);
    ImGui_ImplSDL3_InitForSDLRenderer(get_sdl_presenter_window(), get_sdl_presenter_renderer());
    ImGui_ImplSDLRenderer3_Init(get_sdl_presenter_renderer());
    if(!initialize_settings_window(frontend))
        frontend->warning = "The settings window could not be initialized.";
    if(!initialize_platform_menu())
        frontend->warning = "The native application menu could not be initialized.";
    SDL_SetWindowMinimumSize(get_sdl_presenter_window(), 420, 160);
    show_sdl_presenter();
    use_portable_runtime_input(true);

    int gamepad_count = 0;
    SDL_JoystickID *gamepad_ids = SDL_GetGamepads(&gamepad_count);
    for(int index = 0; index < gamepad_count; ++index)
        if(SDL_Gamepad *gamepad = SDL_OpenGamepad(gamepad_ids[index]))
            frontend->gamepads.push_back(gamepad);
    SDL_free(gamepad_ids);

    std::filesystem::path data_directory;
    for(int index = 1; index < argc; ++index)
        if(std::strcmp(argv[index], "--gagboy") == 0)
            frontend->gagboy_requested = true;
        else if(std::strcmp(argv[index], "--data-dir") == 0 && index + 1 < argc)
            data_directory = host_path_from_utf8(argv[++index]);
    if(!data_directory.empty())
    {
        add_installation(frontend, data_directory);
        if(DataInstallation *installation = selected_installation(frontend))
            begin_game(frontend, *installation, frontend->gagboy_requested);
    }
    else if(frontend->gagboy_requested)
    {
        ApplicationFileRootSelection selection;
        const char *base_path = SDL_GetBasePath();
        std::error_code error;
        if(select_application_file_root(base_path == nullptr ? std::filesystem::path{} : base_path, std::filesystem::current_path(error), &selection))
        {
            add_installation(frontend, selection.directory);
            if(DataInstallation *installation = selected_installation(frontend))
                begin_game(frontend, *installation, true);
        }
        else
            frontend->warning = "--gagboy needs a valid --data-dir or game data beside the executable/current directory.";
    }
    else if(DataInstallation *installation = selected_installation(frontend); installation != nullptr && installation->valid)
        begin_game(frontend, *installation, false);
    return frontend;
}

SDL_AppResult iterate_frontend(FrontendState *frontend)
{
    if(frontend == nullptr)
        return SDL_APP_FAILURE;
    service_sdl_presenter();
    finish_runtime_keyboard_input_drain();
    consume_dialog(frontend);
    refresh_finished_import(frontend);
    finish_game_shutdown(frontend);
    if(frontend->launch_configured_game_requested && frontend->game == nullptr)
    {
        frontend->launch_configured_game_requested = false;
        if(DataInstallation *installation = selected_installation(frontend); installation != nullptr && installation->valid)
        {
            close_frontend_settings(frontend);
            begin_game(frontend, *installation, false);
        }
    }
    if(frontend->quit_requested && frontend->game == nullptr)
        return SDL_APP_SUCCESS;
    update_controller_pointer(frontend);
    render_frontend(frontend);
    return SDL_APP_CONTINUE;
}

SDL_AppResult dispatch_frontend_event(FrontendState *frontend, SDL_Event *event)
{
    if(frontend == nullptr || event == nullptr)
        return SDL_APP_FAILURE;
    if(frontend->settings_open && frontend->settings_imgui_context != nullptr)
    {
        ImGui::SetCurrentContext(frontend->settings_imgui_context);
        ImGui_ImplSDL3_ProcessEvent(event);
    }
    else
    {
        ImGui::SetCurrentContext(frontend->main_imgui_context);
        ImGui_ImplSDL3_ProcessEvent(event);
    }
    ImGui::SetCurrentContext(frontend->main_imgui_context);
    const bool window_event = event->type >= SDL_EVENT_WINDOW_FIRST && event->type <= SDL_EVENT_WINDOW_LAST;
    const SDL_WindowID event_window = window_event ? event->window.windowID : 0;
    const SDL_WindowID main_window = SDL_GetWindowID(get_sdl_presenter_window());
    const SDL_WindowID settings_window = frontend->settings_window == nullptr ? 0 : SDL_GetWindowID(frontend->settings_window);
    PlatformMenuCommand menu_command;
    if(extract_platform_menu_command(*event, &menu_command))
    {
        if(menu_command == PlatformMenuCommand::SETTINGS)
            show_frontend_settings(frontend, SettingsTab::SETTINGS);
        else if(menu_command == PlatformMenuCommand::CONTROLS)
            show_frontend_settings(frontend, SettingsTab::CONTROLS);
        else if(menu_command == PlatformMenuCommand::RETURN_TO_LAUNCHER && frontend->game != nullptr)
            request_game_shutdown(frontend, false);
    }
    else if(event->type == application_host_event_type())
        drain_host_events();
    else if(event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event_window == settings_window)
        close_frontend_settings(frontend);
    else if(event->type == SDL_EVENT_QUIT || (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event_window == main_window))
    {
        if(frontend->game != nullptr)
            request_game_shutdown(frontend, true);
        else
            frontend->quit_requested = true;
    }
    else if(event->type == SDL_EVENT_GAMEPAD_ADDED)
    {
        if(SDL_Gamepad *gamepad = SDL_OpenGamepad(event->gdevice.which))
            frontend->gamepads.push_back(gamepad);
    }
    else if(event->type == SDL_EVENT_GAMEPAD_REMOVED)
    {
        const auto found = std::find_if(frontend->gamepads.begin(), frontend->gamepads.end(), [event](SDL_Gamepad *gamepad) { return SDL_GetGamepadID(gamepad) == event->gdevice.which; });
        if(found != frontend->gamepads.end())
        {
            SDL_CloseGamepad(*found);
            frontend->gamepads.erase(found);
        }
    }
    else if(frontend->mode == FrontendMode::SETTINGS && frontend->settings_chord_latched && event->type == SDL_EVENT_GAMEPAD_BUTTON_UP)
    {
        const SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event->gbutton.button);
        if(button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
            frontend->held_gamepad_buttons[button] = false;
        SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gbutton.which);
        if(gamepad == nullptr
            || (!SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) && !SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                && !SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE)))
            frontend->settings_chord_latched = false;
    }
    else if(frontend->capture_action.has_value() && event->type == SDL_EVENT_KEY_DOWN && !frontend->capture_gamepad)
        set_capture_binding(frontend, event->key.scancode, SDL_GAMEPAD_BUTTON_INVALID);
    else if(frontend->capture_action.has_value() && event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && frontend->capture_gamepad)
        set_capture_binding(frontend, SDL_SCANCODE_UNKNOWN, static_cast<SDL_GamepadButton>(event->gbutton.button));
    else if(event->type == SDL_EVENT_KEY_UP && frontend->suppressed_host_key == event->key.scancode)
        frontend->suppressed_host_key = SDL_SCANCODE_UNKNOWN;
    else if(frontend->mode == FrontendMode::SETTINGS && event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        const SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event->gbutton.button);
        SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gbutton.which);
        if(is_settings_button(frontend, button, gamepad))
        {
            frontend->settings_chord_latched = true;
            close_frontend_settings(frontend);
        }
        else
        {
            const InputBinding *return_binding = find_binding(frontend, BindingAction::RETURN_TO_LAUNCHER);
            if(return_binding != nullptr && return_binding->button == button)
                request_game_shutdown(frontend, false);
        }
    }
    else if(frontend->mode == FrontendMode::GAME && event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && is_settings_key(frontend, event->key))
    {
        frontend->suppressed_host_key = event->key.scancode;
        show_frontend_settings(frontend);
    }
    else if(frontend->mode == FrontendMode::SETTINGS && event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && is_settings_key(frontend, event->key))
    {
        frontend->suppressed_host_key = event->key.scancode;
        close_frontend_settings(frontend);
    }
    else if(frontend->mode == FrontendMode::LAUNCHER && event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && is_settings_key(frontend, event->key))
    {
        frontend->suppressed_host_key = event->key.scancode;
        show_frontend_settings(frontend);
    }
    else if(frontend->mode == FrontendMode::LAUNCHER && event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        const SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event->gbutton.button);
        SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gbutton.which);
        if(is_settings_button(frontend, button, gamepad))
            show_frontend_settings(frontend);
    }
    else if(frontend->mode == FrontendMode::SETTINGS && event->type == SDL_EVENT_KEY_DOWN)
    {
        const InputBinding *return_binding = find_binding(frontend, BindingAction::RETURN_TO_LAUNCHER);
        if(return_binding != nullptr && return_binding->key == event->key.scancode)
            request_game_shutdown(frontend, false);
    }
    else if(frontend->mode == FrontendMode::GAME && event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        const SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event->gbutton.button);
        if(button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
            frontend->held_gamepad_buttons[button] = true;
        SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gbutton.which);
        if(is_settings_button(frontend, button, gamepad))
        {
            frontend->settings_chord_latched = true;
            show_frontend_settings(frontend);
        }
        else if(!frontend->settings_chord_latched)
            dispatch_mapped_button(frontend, button, true);
    }
    else if(frontend->mode == FrontendMode::GAME && event->type == SDL_EVENT_GAMEPAD_BUTTON_UP)
    {
        const SDL_GamepadButton button = static_cast<SDL_GamepadButton>(event->gbutton.button);
        if(button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT)
            frontend->held_gamepad_buttons[button] = false;
        if(frontend->settings_chord_latched)
        {
            SDL_Gamepad *gamepad = SDL_GetGamepadFromID(event->gbutton.which);
            if(gamepad == nullptr
                || (!SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER) && !SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                    && !SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_GUIDE)))
                frontend->settings_chord_latched = false;
        }
        else
            dispatch_mapped_button(frontend, button, false);
    }
    else if(frontend->mode == FrontendMode::GAME && (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP))
    {
        frontend->held_keys[event->key.scancode] = event->type == SDL_EVENT_KEY_DOWN;
        const bool text_entry = runtime_display_context.input_scene_identifier != 0;
        bool mapped = false;
        const InputBinding *return_binding = find_binding(frontend, BindingAction::RETURN_TO_LAUNCHER);
        if(event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat && return_binding != nullptr && return_binding->key == event->key.scancode)
        {
            frontend->suppressed_host_key = event->key.scancode;
            request_game_shutdown(frontend, false);
            mapped = true;
        }
        for(const InputBinding &binding : frontend->preferences.bindings)
        {
            if(mapped)
                break;
            if(binding.key != event->key.scancode)
                continue;
            if(xtet::game_active() && binding_context(binding.action) == BindingContext::GAME)
            {
                if(binding.action == BindingAction::GAGBOY_PAUSE)
                {
                    if(event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat)
                        xtet::execute_game_command(xtet::game_paused() ? 4 : 2);
                }
                else
                    send_runtime_key(gagboy_key(binding.action), event->type == SDL_EVENT_KEY_DOWN);
                mapped = true;
                break;
            }
            if(!xtet::game_active() && !text_entry && binding.action == BindingAction::PRIMARY_CLICK)
            {
                send_runtime_button(frontend, RuntimeMouseButton::LEFT, event->type == SDL_EVENT_KEY_DOWN);
                mapped = true;
                break;
            }
            if(!xtet::game_active() && !text_entry && binding.action == BindingAction::SECONDARY_CLICK)
            {
                send_runtime_button(frontend, RuntimeMouseButton::RIGHT, event->type == SDL_EVENT_KEY_DOWN);
                mapped = true;
                break;
            }
            if(!xtet::game_active() && !text_entry && binding.action >= BindingAction::POINTER_LEFT && binding.action <= BindingAction::POINTER_DOWN)
            {
                mapped = true;
                break;
            }
        }
        if(!mapped)
            dispatch_sdl_runtime_input(frontend->game, *event);
    }
    else if(frontend->mode == FrontendMode::GAME)
        dispatch_sdl_runtime_input(frontend->game, *event);
    if(event->type == SDL_EVENT_WINDOW_MINIMIZED && event_window == main_window && frontend->game != nullptr)
        dispatch_application_action(frontend->game, ApplicationAction::PAUSE);
    else if(event->type == SDL_EVENT_WINDOW_RESTORED && event_window == main_window && frontend->game != nullptr && frontend->mode == FrontendMode::GAME)
        dispatch_application_action(frontend->game, ApplicationAction::RESUME);
    if(event_window == main_window && (event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN || event->type == SDL_EVENT_WINDOW_LEAVE_FULLSCREEN))
        complete_sdl_presenter_fullscreen_transition(event->type == SDL_EVENT_WINDOW_ENTER_FULLSCREEN);
    else if(event_window == main_window && (event->type == SDL_EVENT_WINDOW_RESIZED || event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_EXPOSED))
        request_sdl_presenter_repaint();
    return frontend->quit_requested && frontend->game == nullptr ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

void shutdown_frontend(FrontendState *frontend)
{
    if(frontend == nullptr)
        return;
    frontend->import.cancel_requested = true;
    if(frontend->import.worker.joinable())
        frontend->import.worker.join();
    if(frontend->game != nullptr)
    {
        request_game_shutdown(frontend, true);
        delete frontend->game;
    }
    for(SDL_Gamepad *gamepad : frontend->gamepads)
        SDL_CloseGamepad(gamepad);
    save_preferences(frontend);
    use_portable_runtime_input(false);
    shutdown_platform_menu();
    if(frontend->settings_imgui_context != nullptr)
    {
        ImGui::SetCurrentContext(frontend->settings_imgui_context);
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(frontend->settings_imgui_context);
    }
    if(frontend->settings_renderer != nullptr)
        SDL_DestroyRenderer(frontend->settings_renderer);
    if(frontend->settings_window != nullptr)
        SDL_DestroyWindow(frontend->settings_window);
    ImGui::SetCurrentContext(frontend->main_imgui_context);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(frontend->main_imgui_context);
    shutdown_sdl_shell();
    delete frontend;
}

}
