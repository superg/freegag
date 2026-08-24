#include "display_host.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
#include "host_events.h"
#include "runtime_internal.h"

namespace gag
{
namespace
{
constexpr size_t presentation_queue_capacity = 3;

struct PresenterState
{
    SDL_Window *window{};
    SDL_Renderer *renderer{};
    SDL_Texture *texture{};
    std::thread::id main_thread_id;
    int32_t width{};
    int32_t height{};
    std::vector<uint32_t> root_pixels;
    std::vector<uint32_t> front_pixels;
    std::array<std::vector<uint32_t>, presentation_queue_capacity> snapshot_buffers;
    std::deque<size_t> available_snapshots;
    std::deque<size_t> queued_snapshots;
    std::mutex mutex;
    std::condition_variable queue_changed;
    bool shutting_down{};
    bool runtime_failure_reported{};
    bool presentation_wake_pending{};
    bool presenter_service_pending{};
    bool repaint_pending{};
    bool texture_has_frame{};
    bool fullscreen_transition_pending{};
    bool pending_fullscreen{};
    bool pending_mouse_warp{};
    float pending_mouse_x{};
    float pending_mouse_y{};
};

PresenterState presenter;

DisplayRectangle clipped_rectangle(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    return { std::clamp(left, 0, presenter.width), std::clamp(top, 0, presenter.height), std::clamp(right, 0, presenter.width), std::clamp(bottom, 0, presenter.height) };
}

void update_front_buffer(const DisplayRectangle &requested, uint32_t mode)
{
    const DisplayRectangle rectangle = clipped_rectangle(requested.left, requested.top, requested.right, requested.bottom);
    if(rectangle.left >= rectangle.right || rectangle.top >= rectangle.bottom)
    {
        return;
    }

    for(int32_t y = rectangle.top; y < rectangle.bottom; ++y)
    {
        const size_t offset = static_cast<size_t>(y) * presenter.width + rectangle.left;
        const size_t count = static_cast<size_t>(rectangle.right - rectangle.left);
        if(mode == 1)
        {
            std::copy_n(presenter.root_pixels.data() + offset, count, presenter.front_pixels.data() + offset);
        }
        else if(mode == 2)
        {
            std::fill_n(presenter.front_pixels.data() + offset, count, uint32_t{});
        }
    }
}

bool create_presenter_texture()
{
    if(presenter.texture != nullptr)
    {
        SDL_DestroyTexture(presenter.texture);
        presenter.texture = nullptr;
    }
    presenter.texture = SDL_CreateTexture(presenter.renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, presenter.width, presenter.height);
    presenter.texture_has_frame = false;
    if(presenter.texture != nullptr)
    {
        SDL_SetTextureScaleMode(presenter.texture, SDL_SCALEMODE_NEAREST);
    }
    return presenter.texture != nullptr;
}

bool upload_and_present(const std::vector<uint32_t> &snapshot)
{
    const auto attempt = [&]()
    {
        void *texture_pixels = nullptr;
        int pitch = 0;
        if(!SDL_LockTexture(presenter.texture, nullptr, &texture_pixels, &pitch))
        {
            return false;
        }
        for(int32_t y = 0; y < presenter.height; ++y)
        {
            std::memcpy(static_cast<uint8_t *>(texture_pixels) + static_cast<size_t>(y) * pitch, snapshot.data() + static_cast<size_t>(y) * presenter.width,
                static_cast<size_t>(presenter.width) * sizeof(uint32_t));
        }
        SDL_UnlockTexture(presenter.texture);
        presenter.texture_has_frame = true;
        return SDL_SetRenderDrawColor(presenter.renderer, 0, 0, 0, 0xff) && SDL_RenderClear(presenter.renderer) && SDL_RenderTexture(presenter.renderer, presenter.texture, nullptr, nullptr)
            && SDL_RenderPresent(presenter.renderer);
    };

    if(attempt())
    {
        return true;
    }
    return create_presenter_texture() && attempt();
}

bool present_cached_texture()
{
    if(presenter.renderer == nullptr || presenter.texture == nullptr || !presenter.texture_has_frame)
    {
        return true;
    }
    return SDL_SetRenderDrawColor(presenter.renderer, 0, 0, 0, 0xff) && SDL_RenderClear(presenter.renderer) && SDL_RenderTexture(presenter.renderer, presenter.texture, nullptr, nullptr)
        && SDL_RenderPresent(presenter.renderer);
}

void fail_runtime_presentation()
{
    if(presenter.runtime_failure_reported)
    {
        return;
    }
    presenter.runtime_failure_reported = true;
    std::fprintf(stderr, "SDL presenter failed: %s\n", SDL_GetError());
    post_application_event(HostApplicationCommand::close_requested);
}

void present_snapshot(const std::vector<uint32_t> &snapshot)
{
    if(!snapshot.empty() && !upload_and_present(snapshot))
    {
        fail_runtime_presentation();
    }
}

bool present_next_queued_snapshot()
{
    size_t snapshot_index;
    {
        std::lock_guard lock(presenter.mutex);
        if(presenter.queued_snapshots.empty())
        {
            return false;
        }
        snapshot_index = presenter.queued_snapshots.front();
        presenter.queued_snapshots.pop_front();
    }
    present_snapshot(presenter.snapshot_buffers[snapshot_index]);
    {
        std::lock_guard lock(presenter.mutex);
        presenter.available_snapshots.push_back(snapshot_index);
    }
    presenter.queue_changed.notify_one();
    return true;
}

void queue_presentation(const DisplayRectangle &rectangle, uint32_t mode)
{
    if(presenter.main_thread_id == std::this_thread::get_id())
    {
        while(present_next_queued_snapshot())
        {
        }
        std::vector<uint32_t> snapshot;
        {
            std::lock_guard lock(presenter.mutex);
            if(presenter.shutting_down)
            {
                return;
            }
            update_front_buffer(rectangle, mode);
            snapshot = presenter.front_pixels;
        }
        present_snapshot(std::move(snapshot));
        return;
    }

    bool wake_presenter = false;
    std::unique_lock lock(presenter.mutex);
    presenter.queue_changed.wait(lock, [] { return presenter.shutting_down || !presenter.available_snapshots.empty(); });
    if(presenter.shutting_down)
    {
        return;
    }
    update_front_buffer(rectangle, mode);
    const size_t snapshot_index = presenter.available_snapshots.front();
    presenter.available_snapshots.pop_front();
    presenter.snapshot_buffers[snapshot_index] = presenter.front_pixels;
    presenter.queued_snapshots.push_back(snapshot_index);
    if(!presenter.presentation_wake_pending)
    {
        presenter.presentation_wake_pending = true;
        wake_presenter = true;
    }
    lock.unlock();
    if(wake_presenter)
    {
        post_host_event(HostPresentPendingFramesEvent{});
    }
}
}

GraphicsHostInitializationResult *initialize_runtime_graphics()
{
    if((runtime_scene_control_flags & 0x800) == 0)
    {
        return nullptr;
    }
    if((runtime_scene_control_flags & 0x400) != 0)
    {
        return &graphics_host_state;
    }

    void *surface = create_display_surface(runtime_game_host_context.width, runtime_game_host_context.height);
    if(surface == nullptr)
    {
        return nullptr;
    }

    runtime_game_host_context.display_surface = surface;
    runtime_game_host_context.palette_entries = reinterpret_cast<PaletteEntry *>(get_display_palette_entries());

    auto *descriptor = &runtime_display_context.display_pixel_format;
    *descriptor = { 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0, nullptr, nullptr };
    runtime_game_host_context.bits_per_pixel = 32;
    graphics_host_state.bits_per_pixel = 32;

    runtime_display_host = runtime_bootstrap_api.initialize_scene_host(reinterpret_cast<intptr_t>(surface), descriptor, runtime_game_host_context.width, runtime_game_host_context.height,
        reinterpret_cast<int (*)(void *, void *, uint32_t)>(&update_runtime_target), &runtime_game_host_context, 0x0f);
    if(runtime_display_host == nullptr)
    {
        return nullptr;
    }

    runtime_display_scene_identifier =
        reinterpret_cast<intptr_t>(runtime_bootstrap_api.acquire_scene_node(0, 0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 0x20022, 0, nullptr, nullptr));
    if(runtime_display_scene_identifier == 0)
    {
        return nullptr;
    }

    DisplaySceneNode *node = runtime_bootstrap_api.lock_scene_node(runtime_display_scene_identifier);
    if(node == nullptr)
    {
        return nullptr;
    }
    runtime_game_host_context.unknown_0028 = node->callback_first_position;
    runtime_game_host_context.framebuffer = reinterpret_cast<void *>(node->callback_first_position);
    runtime_game_host_context.unknown_0030 = node->callback_first_position;
    runtime_bootstrap_api.unlock_scene_node(runtime_display_scene_identifier);

    if(runtime_bootstrap_api.acquire_display_lock(nullptr, nullptr, nullptr) == 0)
    {
        runtime_bootstrap_api.set_clip_rectangle(nullptr);
        runtime_bootstrap_api.release_display_lock();
    }
    operate_display_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
    runtime_bootstrap_api.reset_display_state();
    runtime_display_thread = new (std::nothrow) std::jthread([] { execute_script_commands(&runtime_display_context); });
    if(runtime_display_thread == nullptr)
    {
        return nullptr;
    }
    runtime_scene_control_flags |= 0x600;
    return &graphics_host_state;
}

void invalidate_game_framebuffer_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    DisplayRectangle rectangle{ x, y, x + width, y + height };
    if(framebuffer_invalidate_api.acquire_lock(nullptr, nullptr, nullptr) == 0)
    {
        framebuffer_invalidate_api.dispatch_update(&rectangle, 0);
        framebuffer_invalidate_api.release_lock();
    }
}

uint32_t initialize_sdl_presenter(int32_t width, int32_t height, uint32_t)
{
    presenter.main_thread_id = std::this_thread::get_id();
    presenter.shutting_down = false;
    presenter.runtime_failure_reported = false;
    presenter.presentation_wake_pending = false;
    presenter.presenter_service_pending = false;
    presenter.repaint_pending = false;
    presenter.texture_has_frame = false;
    display_palette_flags = 0x80000000;

    if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        std::fprintf(stderr, "Unable to initialize SDL video: %s\n", SDL_GetError());
        display_palette_flags = 0;
        return 1;
    }
    SDL_HideCursor();

    presenter.window = SDL_CreateWindow("GAG", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if(presenter.window != nullptr)
    {
        SDL_SetWindowMinimumSize(presenter.window, width, height);
        presenter.renderer = SDL_CreateRenderer(presenter.window, nullptr);
    }
    if(presenter.renderer == nullptr)
    {
        std::fprintf(stderr, "Unable to initialize SDL presenter: %s\n", SDL_GetError());
        shutdown_sdl_presenter();
        return 1;
    }
    SDL_StartTextInput(presenter.window);
    SDL_SetRenderVSync(presenter.renderer, SDL_RENDERER_VSYNC_DISABLED);
    SDL_SetRenderLogicalPresentation(presenter.renderer, width, height, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    return 0;
}

bool show_sdl_presenter()
{
    return presenter.window != nullptr && SDL_ShowWindow(presenter.window);
}

void shutdown_sdl_presenter()
{
    begin_sdl_presenter_shutdown();
    teardown_display_palette_surface();
    if(presenter.renderer != nullptr)
    {
        SDL_DestroyRenderer(presenter.renderer);
        presenter.renderer = nullptr;
    }
    if(presenter.window != nullptr)
    {
        SDL_StopTextInput(presenter.window);
        SDL_DestroyWindow(presenter.window);
        presenter.window = nullptr;
    }
    display_palette_flags = 0;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void begin_sdl_presenter_shutdown()
{
    {
        std::lock_guard lock(presenter.mutex);
        presenter.shutting_down = true;
        presenter.queued_snapshots.clear();
        presenter.presentation_wake_pending = false;
        presenter.presenter_service_pending = false;
        presenter.repaint_pending = false;
    }
    presenter.queue_changed.notify_all();
}

bool convert_sdl_presenter_event(SDL_Event *event)
{
    return presenter.renderer != nullptr && event != nullptr && SDL_ConvertEventToRenderCoordinates(presenter.renderer, event);
}

bool set_sdl_presenter_fullscreen(bool fullscreen)
{
    if(presenter.window == nullptr || presenter.renderer == nullptr)
    {
        return false;
    }
    if(presenter.fullscreen_transition_pending && presenter.pending_fullscreen == fullscreen)
    {
        return true;
    }
    if(!presenter.fullscreen_transition_pending && ((SDL_GetWindowFlags(presenter.window) & SDL_WINDOW_FULLSCREEN) != 0) == fullscreen)
    {
        return true;
    }

    float window_x;
    float window_y;
    SDL_GetMouseState(&window_x, &window_y);
    float logical_x;
    float logical_y;
    presenter.pending_mouse_warp = SDL_RenderCoordinatesFromWindow(presenter.renderer, window_x, window_y, &logical_x, &logical_y) && logical_x >= 0.0f && logical_y >= 0.0f
                                && logical_x < static_cast<float>(presenter.width) && logical_y < static_cast<float>(presenter.height);
    if(presenter.pending_mouse_warp)
    {
        presenter.pending_mouse_x = logical_x;
        presenter.pending_mouse_y = logical_y;
    }
    if(!SDL_SetWindowFullscreen(presenter.window, fullscreen))
    {
        presenter.pending_mouse_warp = false;
        return false;
    }
    presenter.fullscreen_transition_pending = true;
    presenter.pending_fullscreen = fullscreen;
    return true;
}

void complete_sdl_presenter_fullscreen_transition(bool fullscreen)
{
    if(!presenter.fullscreen_transition_pending || presenter.pending_fullscreen != fullscreen)
    {
        return;
    }
    presenter.fullscreen_transition_pending = false;
    if(!presenter.pending_mouse_warp || presenter.window == nullptr || presenter.renderer == nullptr)
    {
        presenter.pending_mouse_warp = false;
        return;
    }

    float window_x;
    float window_y;
    if(SDL_RenderCoordinatesToWindow(presenter.renderer, presenter.pending_mouse_x, presenter.pending_mouse_y, &window_x, &window_y))
    {
        SDL_WarpMouseInWindow(presenter.window, window_x, window_y);
    }
    presenter.pending_mouse_warp = false;
}

bool get_sdl_presenter_window_rectangle(DisplayRectangle *rectangle)
{
    if(presenter.window == nullptr || rectangle == nullptr)
    {
        return false;
    }
    int x;
    int y;
    int width;
    int height;
    if(!SDL_GetWindowPosition(presenter.window, &x, &y) || !SDL_GetWindowSize(presenter.window, &width, &height))
    {
        return false;
    }
    *rectangle = { x, y, x + width, y + height };
    return true;
}

bool set_sdl_presenter_window_rectangle(const DisplayRectangle &rectangle)
{
    if(presenter.window == nullptr || rectangle.right <= rectangle.left || rectangle.bottom <= rectangle.top)
    {
        return false;
    }
    return SDL_SetWindowPosition(presenter.window, rectangle.left, rectangle.top) && SDL_SetWindowSize(presenter.window, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top);
}

bool is_sdl_presenter_rectangle_visible(const DisplayRectangle &rectangle)
{
    if(rectangle.right <= rectangle.left || rectangle.bottom <= rectangle.top)
    {
        return false;
    }
    const SDL_Rect candidate{ rectangle.left, rectangle.top, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top };
    return SDL_GetDisplayForRect(&candidate) != 0;
}

bool get_sdl_presenter_mouse_position(int32_t *x, int32_t *y)
{
    if(presenter.renderer == nullptr || x == nullptr || y == nullptr)
    {
        return false;
    }
    float window_x;
    float window_y;
    SDL_GetMouseState(&window_x, &window_y);
    float logical_x;
    float logical_y;
    if(!SDL_RenderCoordinatesFromWindow(presenter.renderer, window_x, window_y, &logical_x, &logical_y))
    {
        return false;
    }
    *x = static_cast<int32_t>(logical_x);
    *y = static_cast<int32_t>(logical_y);
    return true;
}

void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode)
{
    queue_presentation({ x, y, x + width, y + height }, static_cast<uint32_t>(mode));
}

void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode)
{
    if(rectangle != nullptr)
    {
        queue_presentation(*rectangle, mode);
    }
}

uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch)
{
    uint32_t busy;
    do
    {
        busy = (display_palette_flags & 0x40000000) >> 30;
        if(busy != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        else
        {
            std::lock_guard lock(display_host_mutex);
            busy = (display_palette_flags & 0x40000000) >> 30;
            if(busy == 0)
            {
                display_palette_flags |= 0x40000000;
            }
        }
    } while(busy != 0);

    *pixels = presenter.root_pixels.data();
    *pitch = static_cast<uint32_t>(presenter.width * sizeof(uint32_t));
    *rectangle = { 0, 0, presenter.width, presenter.height };
    if(!presenter.root_pixels.empty())
    {
        return 0;
    }
    {
        std::lock_guard lock(display_host_mutex);
        display_palette_flags &= 0xbfffffff;
    }
    return 0x200000;
}

uint32_t end_display_target()
{
    if((display_palette_flags & 0x40000000) == 0)
    {
        return 0x200000;
    }
    {
        std::lock_guard lock(display_host_mutex);
        display_palette_flags &= 0xbfffffff;
    }
    return 0;
}

void *create_display_surface(int32_t width, int32_t height)
{
    teardown_display_palette_surface();
    presenter.width = width;
    presenter.height = height;
    presenter.root_pixels.assign(static_cast<size_t>(width) * height, 0);
    presenter.front_pixels.assign(static_cast<size_t>(width) * height, 0);
    presenter.available_snapshots.clear();
    presenter.queued_snapshots.clear();
    for(size_t index = 0; index < presenter.snapshot_buffers.size(); ++index)
    {
        presenter.snapshot_buffers[index].assign(static_cast<size_t>(width) * height, 0);
        presenter.available_snapshots.push_back(index);
    }
    display_palette_width = width;
    display_palette_height = height;
    display_palette_bits_per_pixel = 32;
    display_palette_pixels = presenter.root_pixels.data();
    if(!create_presenter_texture())
    {
        std::fprintf(stderr, "Unable to create SDL presenter texture: %s\n", SDL_GetError());
        teardown_display_palette_surface();
        return nullptr;
    }
    return display_palette_pixels;
}

void teardown_display_palette_surface()
{
    if(presenter.texture != nullptr)
    {
        SDL_DestroyTexture(presenter.texture);
        presenter.texture = nullptr;
    }
    {
        std::lock_guard lock(presenter.mutex);
        presenter.root_pixels.clear();
        presenter.front_pixels.clear();
        for(auto &snapshot : presenter.snapshot_buffers)
        {
            snapshot.clear();
        }
        presenter.available_snapshots.clear();
        presenter.queued_snapshots.clear();
        presenter.presentation_wake_pending = false;
        presenter.presenter_service_pending = false;
        presenter.repaint_pending = false;
        presenter.texture_has_frame = false;
    }
    display_palette_width = 0;
    display_palette_height = 0;
    display_palette_bits_per_pixel = 0;
    display_palette_pixels = nullptr;
}

PaletteEntry *get_display_palette_entries()
{
    return display_palette_entries;
}

uint32_t apply_display_palette(const PaletteEntry *palette_entries, uint32_t)
{
    std::lock_guard lock(display_host_mutex);
    if(palette_entries != nullptr)
    {
        std::memcpy(display_palette_entries, palette_entries, sizeof(display_palette_entries));
    }
    return palette_entries != nullptr ? 0xec : 0;
}

void enable_display_palette_mode() {}

void disable_display_palette_mode() {}

void drain_sdl_presenter_frames()
{
    std::lock_guard lock(presenter.mutex);
    presenter.presentation_wake_pending = false;
    presenter.presenter_service_pending = true;
}

void request_sdl_presenter_repaint()
{
    std::lock_guard lock(presenter.mutex);
    presenter.repaint_pending = true;
}

void service_sdl_presenter()
{
    bool service_pending;
    bool repaint_pending;
    {
        std::lock_guard lock(presenter.mutex);
        service_pending = presenter.presenter_service_pending;
        repaint_pending = presenter.repaint_pending;
        presenter.presenter_service_pending = false;
        presenter.repaint_pending = false;
    }

    bool presented_frame = false;
    if(service_pending)
    {
        while(present_next_queued_snapshot())
        {
            presented_frame = true;
        }
    }
    if(repaint_pending && !presented_frame && !present_cached_texture())
    {
        fail_runtime_presentation();
    }
}

} // namespace gag
