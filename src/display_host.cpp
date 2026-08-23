#include "display_host.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <vector>
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
    HWND native_window{};
    DWORD main_thread_id{};
    int32_t width{};
    int32_t height{};
    std::vector<uint16_t> root_pixels;
    std::vector<uint16_t> front_pixels;
    std::array<std::vector<uint16_t>, presentation_queue_capacity> snapshot_buffers;
    std::deque<size_t> available_snapshots;
    std::deque<size_t> queued_snapshots;
    std::mutex mutex;
    std::condition_variable queue_changed;
    bool shutting_down{};
    bool runtime_failure_reported{};
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
            std::fill_n(presenter.front_pixels.data() + offset, count, uint16_t{});
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
    if(presenter.texture != nullptr)
    {
        SDL_SetTextureScaleMode(presenter.texture, SDL_SCALEMODE_NEAREST);
    }
    return presenter.texture != nullptr;
}

bool upload_and_present(const std::vector<uint16_t> &snapshot)
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
            auto *destination = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(texture_pixels) + static_cast<size_t>(y) * pitch);
            const uint16_t *source = snapshot.data() + static_cast<size_t>(y) * presenter.width;
            for(int32_t x = 0; x < presenter.width; ++x)
            {
                const uint16_t pixel = source[x];
                const uint32_t red = (pixel >> 11) & 0x1f;
                const uint32_t green = (pixel >> 5) & 0x3f;
                const uint32_t blue = pixel & 0x1f;
                destination[x] = 0xff000000u | ((red << 3 | red >> 2) << 16) | ((green << 2 | green >> 4) << 8) | (blue << 3 | blue >> 2);
            }
        }
        SDL_UnlockTexture(presenter.texture);
        return SDL_SetRenderDrawColor(presenter.renderer, 0, 0, 0, 0xff) && SDL_RenderClear(presenter.renderer) && SDL_RenderTexture(presenter.renderer, presenter.texture, nullptr, nullptr)
            && SDL_RenderPresent(presenter.renderer);
    };

    if(attempt())
    {
        return true;
    }
    return create_presenter_texture() && attempt();
}

void fail_runtime_presentation()
{
    if(presenter.runtime_failure_reported)
    {
        return;
    }
    presenter.runtime_failure_reported = true;
    std::fprintf(stderr, "SDL presenter failed: %s\n", SDL_GetError());
    HWND root = GetAncestor(presenter.native_window, GA_ROOT);
    PostMessageA(root != nullptr ? root : presenter.native_window, WM_CLOSE, 0, 0);
}

void present_snapshot(const std::vector<uint16_t> &snapshot)
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
    if(presenter.main_thread_id == GetCurrentThreadId())
    {
        while(present_next_queued_snapshot())
        {
        }
        std::vector<uint16_t> snapshot;
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
    lock.unlock();
    PostMessageA(presenter.native_window, sdl_presenter_message, 0, 0);
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
    runtime_game_host_context.palette_entries = get_display_palette_entries();
    runtime_game_host_context.window = graphics_host_state.capture_window;

    auto *descriptor = &runtime_display_context.display_pixel_format;
    *descriptor = { 0, 16, 0xf800, 0x07e0, 0x001f, 0x10000, nullptr, nullptr };
    runtime_game_host_context.bits_per_pixel = 16;
    graphics_host_state.bits_per_pixel = 16;

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
    DWORD thread_id;
    runtime_display_thread = runtime_bootstrap_api.create_thread(nullptr, 0, runtime_bootstrap_api.script_thread_entry, &runtime_display_context, 0, &thread_id);
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

uint32_t initialize_sdl_presenter(HWND window, uint32_t)
{
    presenter.native_window = window;
    presenter.main_thread_id = GetCurrentThreadId();
    presenter.shutting_down = false;
    presenter.runtime_failure_reported = false;
    InitializeCriticalSection(&display_host_critical_section);
    display_palette_flags = 0x80000000;

    if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        std::fprintf(stderr, "Unable to initialize SDL video: %s\n", SDL_GetError());
        DeleteCriticalSection(&display_host_critical_section);
        display_palette_flags = 0;
        return 1;
    }

    SDL_PropertiesID properties = SDL_CreateProperties();
    if(properties != 0)
    {
        SDL_SetPointerProperty(properties, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, window);
        presenter.window = SDL_CreateWindowWithProperties(properties);
        SDL_DestroyProperties(properties);
    }
    if(presenter.window != nullptr)
    {
        presenter.renderer = SDL_CreateRenderer(presenter.window, nullptr);
    }
    if(presenter.renderer == nullptr)
    {
        std::fprintf(stderr, "Unable to initialize SDL presenter: %s\n", SDL_GetError());
        shutdown_sdl_presenter();
        return 1;
    }
    SDL_SetRenderVSync(presenter.renderer, SDL_RENDERER_VSYNC_DISABLED);
    return 0;
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
        SDL_DestroyWindow(presenter.window);
        presenter.window = nullptr;
    }
    if((display_palette_flags & 0x80000000) != 0)
    {
        DeleteCriticalSection(&display_host_critical_section);
    }
    display_palette_flags = 0;
    presenter.native_window = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void begin_sdl_presenter_shutdown()
{
    {
        std::lock_guard lock(presenter.mutex);
        presenter.shutting_down = true;
        presenter.queued_snapshots.clear();
    }
    presenter.queue_changed.notify_all();
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
            Sleep(5);
        }
        else
        {
            EnterCriticalSection(&display_host_critical_section);
            busy = (display_palette_flags & 0x40000000) >> 30;
            if(busy == 0)
            {
                display_palette_flags |= 0x40000000;
            }
            LeaveCriticalSection(&display_host_critical_section);
        }
    } while(busy != 0);

    *pixels = presenter.root_pixels.data();
    *pitch = static_cast<uint32_t>(presenter.width * sizeof(uint16_t));
    *rectangle = { 0, 0, presenter.width, presenter.height };
    if(!presenter.root_pixels.empty())
    {
        return 0;
    }
    display_palette_flags &= 0xbfffffff;
    return 0x200000;
}

uint32_t end_display_target()
{
    if((display_palette_flags & 0x40000000) == 0)
    {
        return 0x200000;
    }
    display_palette_flags &= 0xbfffffff;
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
    display_palette_bits_per_pixel = 16;
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
    }
    display_palette_width = 0;
    display_palette_height = 0;
    display_palette_bits_per_pixel = 0;
    display_palette_pixels = nullptr;
}

PALETTEENTRY *get_display_palette_entries()
{
    return display_palette_entries;
}

UINT apply_display_palette(const PALETTEENTRY *palette_entries, uint32_t)
{
    EnterCriticalSection(&display_host_critical_section);
    if(palette_entries != nullptr)
    {
        std::memcpy(display_palette_entries, palette_entries, sizeof(display_palette_entries));
    }
    LeaveCriticalSection(&display_host_critical_section);
    return palette_entries != nullptr ? 0xec : 0;
}

void enable_display_palette_mode() {}

void disable_display_palette_mode() {}

void handle_sdl_presenter_message()
{
    present_next_queued_snapshot();
}

void repaint_sdl_presenter()
{
    while(present_next_queued_snapshot())
    {
    }
    std::vector<uint16_t> snapshot;
    {
        std::lock_guard lock(presenter.mutex);
        snapshot = presenter.front_pixels;
    }
    present_snapshot(std::move(snapshot));
}

} // namespace gag
