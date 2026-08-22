#include "display_host.h"
#include "runtime_internal.h"

namespace gag
{
GraphicsHostInitializationResult *initialize_runtime_graphics(const LegacyDisplayPixelFormat *requested_format)
{
    if((runtime_scene_control_flags & 0x800) == 0)
    {
        return nullptr;
    }
    if((runtime_scene_control_flags & 0x400) != 0)
    {
        return &graphics_host_state;
    }

    LegacyDisplayPixelFormat mode_format{};
    const LegacyDisplayPixelFormat *format = requested_format;
    if(format == nullptr)
    {
        DisplayMode *mode = runtime_bootstrap_api.find_current_mode();
        if(mode == nullptr)
        {
            return nullptr;
        }
        mode_format = { mode->pixel_format_flags, mode->pixel_format_reserved, static_cast<uint32_t>(mode->bits_per_pixel), mode->red_mask, mode->green_mask, mode->blue_mask };
        format = &mode_format;
    }

    void *surface = runtime_bootstrap_api.create_surface(runtime_game_host_context.width, runtime_game_host_context.height, format, 0);
    if(surface == nullptr)
    {
        return nullptr;
    }

    runtime_game_host_context.display_surface = surface;
    runtime_game_host_context.palette_dc = runtime_bootstrap_api.get_palette_dc();
    runtime_game_host_context.palette_dib_dc = runtime_bootstrap_api.get_palette_dib_dc();
    runtime_game_host_context.palette = runtime_bootstrap_api.get_palette_handle();
    HBITMAP bitmap = runtime_bootstrap_api.get_palette_bitmap();
    runtime_game_host_context.bitmap = bitmap;
    runtime_game_host_context.selected_bitmap = runtime_bootstrap_api.get_palette_bitmap();
    runtime_game_host_context.palette_entries = runtime_bootstrap_api.get_palette_entries();
    runtime_game_host_context.window = graphics_host_state.capture_window;

    auto *descriptor = &runtime_display_context.display_pixel_format;
    descriptor->bits_per_pixel = format->bits_per_pixel;
    descriptor->red_mask = format->red_mask;
    descriptor->green_mask = format->green_mask;
    descriptor->blue_mask = format->blue_mask;
    if(format->bits_per_pixel == 8)
    {
        descriptor->palette_count = 0x100;
    }
    else if(format->bits_per_pixel == 16)
    {
        descriptor->palette_count = format->green_mask == 0x7e0 ? 0x10000 : 0x8000;
    }
    else if(format->bits_per_pixel == 24)
    {
        descriptor->palette_count = 0x1000000;
    }
    descriptor->palette_source = nullptr;
    descriptor->palette_entries = nullptr;
    runtime_game_host_context.bits_per_pixel = format->bits_per_pixel;
    graphics_host_state.bits_per_pixel = format->bits_per_pixel;

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
    runtime_bootstrap_api.operate_surface(0, 0, runtime_game_host_context.width, runtime_game_host_context.height, 2);
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

uint32_t initialize_direct_draw_runtime()
{
    OSVERSIONINFOA version;
    version.dwOSVersionInfoSize = sizeof(version);
    uint32_t result = 0x10000;
    if(display_bootstrap_api.get_version(&version) != FALSE)
    {
        result = 1;
        display_platform_id = version.dwPlatformId;
        if(display_platform_id >= 1 && display_platform_id <= 2)
        {
            direct_draw_module = display_bootstrap_api.load_library("DDRAW.DLL");
            if(direct_draw_module != nullptr)
            {
                result = 0x100000;
                FARPROC procedure = display_bootstrap_api.get_proc_address(direct_draw_module, "DirectDrawCreate");
                if(procedure != nullptr)
                {
                    result = 0x200000;
                    void *display = nullptr;
                    if(reinterpret_cast<DirectDrawCreateProcedure>(procedure)(nullptr, &display, nullptr) == 0)
                    {
                        result = 0;
                        display_direct_draw = display;
                    }
                }
            }
        }
    }
    display_bootstrap_error = result;
    return result;
}

HRESULT WINAPI collect_direct_draw_display_mode(LegacyDirectDrawSurfaceDescriptor *descriptor, void *)
{
    if((descriptor->flags & 0x1006) != 0x1006)
    {
        return 1;
    }
    auto *mode = static_cast<DisplayMode *>(display_bootstrap_api.heap_alloc(display_bootstrap_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(DisplayMode)));
    if(mode == nullptr)
    {
        return 0;
    }
    mode->surface_caps = descriptor->caps;
    mode->width = descriptor->width;
    mode->height = descriptor->height;
    mode->pixel_format_flags = descriptor->pixel_format.flags;
    mode->bits_per_pixel = descriptor->pixel_format.bits_per_pixel;
    if((descriptor->pixel_format.flags & 0x40) != 0)
    {
        mode->red_mask = descriptor->pixel_format.red_mask;
        mode->green_mask = descriptor->pixel_format.green_mask;
        mode->blue_mask = descriptor->pixel_format.blue_mask;
    }

    for(DisplayMode *existing = display_mode_head; existing != nullptr; existing = existing->next)
    {
        if(existing->width == mode->width && existing->height == mode->height && existing->bits_per_pixel == mode->bits_per_pixel && existing->green_mask == mode->green_mask)
        {
            existing->pixel_format_flags = descriptor->pixel_format.flags;
            existing->flags |= 0x20000;
            display_bootstrap_api.heap_free(display_bootstrap_api.get_process_heap(), 0, mode);
            return 1;
        }
    }

    bool unsupported = false;
    if(mode->bits_per_pixel == 8)
    {
        mode->pixel_value_count = 0x100;
    }
    else if(mode->bits_per_pixel == 16)
    {
        if(mode->green_mask == 0x3e0)
        {
            mode->pixel_value_count = 0x8000;
        }
        else if(mode->green_mask == 0x7e0)
        {
            mode->pixel_value_count = 0x10000;
        }
        else
        {
            unsupported = true;
        }
    }
    else if(mode->bits_per_pixel == 24)
    {
        mode->pixel_value_count = 0x1000000;
    }
    if((descriptor->pixel_format.flags & 1) != 0)
    {
        mode->alpha_mask = descriptor->pixel_format.alpha_mask;
    }
    if(unsupported)
    {
        display_bootstrap_api.heap_free(display_bootstrap_api.get_process_heap(), 0, mode);
        return 1;
    }

    mode->flags |= 0x20000;
    ++display_mode_count;
    if(display_mode_tail == nullptr)
    {
        display_mode_head = mode;
        display_mode_tail = mode;
    }
    else
    {
        display_mode_tail->next = mode;
        display_mode_tail = mode;
    }
    return 1;
}

uint32_t enumerate_direct_draw_display_modes()
{
    uint32_t result = 0x200000;
    if((display_palette_flags & 1) != 0)
    {
        display_bootstrap_api.set_cooperative_mode(0x1000);
        if(display_bootstrap_api.enumerate_modes(display_direct_draw, collect_direct_draw_display_mode) == 0)
        {
            result = 0;
        }
        display_bootstrap_api.set_cooperative_mode(0);
    }
    display_bootstrap_error = result;
    return result;
}



uint32_t enumerate_windows_display_modes()
{
    uint32_t result = 0x200000;
    if((display_palette_flags & 2) != 0)
    {
        HDC dc = windows_display_enumeration_api.get_dc(nullptr);
        DWORD mode_index = 0;
        DEVMODEA settings;
        BOOL has_mode;
        do
        {
            has_mode = windows_display_enumeration_api.enum_display_settings(nullptr, mode_index, &settings);
            ++mode_index;
            if(windows_display_enumeration_api.change_display_settings(&settings, 2) == DISP_CHANGE_SUCCESSFUL)
            {
                bool rejected = false;
                auto *mode = static_cast<DisplayMode *>(windows_display_enumeration_api.heap_alloc(windows_display_enumeration_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(DisplayMode)));
                if(mode != nullptr)
                {
                    mode->device_mode_fields = settings.dmFields;
                    mode->surface_caps = settings.dmDisplayFlags;
                    mode->width = static_cast<int32_t>(settings.dmPelsWidth);
                    mode->height = static_cast<int32_t>(settings.dmPelsHeight);
                    mode->refresh_rate = settings.dmDisplayFrequency;
                    mode->bits_per_pixel = static_cast<int32_t>(settings.dmBitsPerPel);

                    if((mode->device_mode_fields & 0x400000) != 0)
                    {
                        for(DisplayMode *existing = display_mode_head; existing != nullptr; existing = existing->next)
                        {
                            if(existing->width == mode->width && existing->height == mode->height && existing->bits_per_pixel == mode->bits_per_pixel && existing->green_mask == mode->green_mask)
                            {
                                if(existing->refresh_rate < mode->refresh_rate)
                                {
                                    std::memcpy(&existing->unknown_0004, &mode->unknown_0004, 7 * sizeof(uint32_t));
                                }
                                rejected = true;
                                break;
                            }
                        }
                    }

                    if(!rejected)
                    {
                        if(mode->bits_per_pixel == 8)
                        {
                            mode->pixel_value_count = 0x100;
                        }
                        else if(mode->bits_per_pixel == 16)
                        {
                            auto *bitmap_info = static_cast<BITMAPINFO *>(windows_display_enumeration_api.heap_alloc(windows_display_enumeration_api.get_process_heap(), 0, 0x428));
                            if(bitmap_info != nullptr)
                            {
                                mode->red_mask = 0xf800;
                                mode->green_mask = 0x7e0;
                                mode->blue_mask = 0x1f;
                                HBITMAP bitmap;
                                do
                                {
                                    std::memset(bitmap_info, 0, 0x428);
                                    bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                                    bitmap_info->bmiHeader.biSizeImage = 200;
                                    bitmap_info->bmiHeader.biWidth = 10;
                                    bitmap_info->bmiHeader.biHeight = -10;
                                    bitmap_info->bmiHeader.biPlanes = 1;
                                    bitmap_info->bmiHeader.biBitCount = 16;
                                    bitmap_info->bmiHeader.biClrUsed = mode->green_mask == 0x7e0 ? 0x10000 : 0x8000;
                                    bitmap_info->bmiHeader.biCompression = mode->green_mask == 0x7e0 ? 3 : 0;
                                    auto *masks = reinterpret_cast<uint32_t *>(bitmap_info->bmiColors);
                                    masks[0] = mode->red_mask;
                                    masks[1] = mode->green_mask;
                                    masks[2] = mode->blue_mask;
                                    void *bits;
                                    bitmap = windows_display_enumeration_api.create_dib_section(dc, bitmap_info, 0, &bits, nullptr, 0);
                                    if(bitmap != nullptr || mode->green_mask != 0x7e0)
                                    {
                                        break;
                                    }
                                    mode->green_mask = 0x3e0;
                                    mode->red_mask = 0x7c00;
                                } while(true);

                                if(bitmap == nullptr)
                                {
                                    rejected = true;
                                    windows_display_enumeration_api.heap_free(windows_display_enumeration_api.get_process_heap(), 0, mode);
                                }
                                windows_display_enumeration_api.heap_free(windows_display_enumeration_api.get_process_heap(), 0, bitmap_info);
                                if(bitmap != nullptr)
                                {
                                    windows_display_enumeration_api.delete_object(bitmap);
                                    mode->pixel_value_count = mode->green_mask == 0x7e0 ? 0x10000 : 0x8000;
                                }
                            }
                        }
                        else if(mode->bits_per_pixel == 24)
                        {
                            mode->pixel_value_count = 0x1000000;
                        }
                    }

                    if(rejected)
                    {
                        windows_display_enumeration_api.heap_free(windows_display_enumeration_api.get_process_heap(), 0, mode);
                    }
                    else
                    {
                        mode->flags |= 0x10000;
                        ++display_mode_count;
                        if(display_mode_tail == nullptr)
                        {
                            display_mode_head = mode;
                            display_mode_tail = mode;
                        }
                        else
                        {
                            display_mode_tail->next = mode;
                            display_mode_tail = mode;
                        }
                    }
                    result = 0;
                }
            }
        } while(has_mode != FALSE);
        windows_display_enumeration_api.release_dc(nullptr, dc);
    }
    display_bootstrap_error = result;
    return result;
}


uint32_t initialize_display_mode_host(HWND window, uint32_t options)
{
    uint32_t result = 0;
    if((display_palette_flags & 0x10) == 0)
    {
        display_palette_flags = (display_palette_flags & 0x80000003) | options;
        display_palette_window = window;
    }
    if((display_palette_flags & 0x80000000) == 0)
    {
        display_host_initialization_api.initialize_critical_section(&display_host_critical_section);
        display_palette_flags |= 2;
        result = display_host_initialization_api.enumerate_windows_modes();
        display_palette_flags &= 0xfffffffd;
        if(result == 0)
        {
            result = 1;
            if((options & 0x300000) != 0x300000)
            {
                result = display_host_initialization_api.initialize_direct_draw();
            }
            if(result == 0)
            {
                display_palette_flags |= 1;
                result = display_host_initialization_api.enumerate_direct_draw_modes();
            }
            else if(result == 1 || result == 0x100000)
            {
                result = 0;
                display_palette_flags |= 2;
            }
            if(result != 0)
            {
                display_host_initialization_api.delete_critical_section(&display_host_critical_section);
                return result;
            }
            display_host_initialization_api.find_current_mode();
            display_palette_flags |= 0x80000000;
            return 0;
        }
        display_host_initialization_api.delete_critical_section(&display_host_critical_section);
    }
    return result;
}



DisplayMode *begin_display_mode_enumeration(uint32_t mask)
{
    display_mode_iterator = display_mode_head;
    while(display_mode_iterator != nullptr && (display_mode_iterator->flags & mask & 0x30000) == 0)
    {
        display_mode_iterator = display_mode_iterator->next;
    }
    return display_mode_iterator;
}

DisplayMode *get_next_display_mode(uint32_t mask)
{
    display_mode_iterator = display_mode_iterator->next;
    while(display_mode_iterator != nullptr && (display_mode_iterator->flags & mask & 0x30000) == 0)
    {
        display_mode_iterator = display_mode_iterator->next;
    }
    return display_mode_iterator;
}

DisplayMode *find_current_display_mode()
{
    DisplayMode *result = nullptr;
    DisplayMode *mode = display_mode_head;
    if(mode != nullptr)
    {
        HDC display = CreateICA("DISPLAY", nullptr, nullptr, nullptr);
        int bits_per_pixel = GetDeviceCaps(display, BITSPIXEL);
        int width = GetDeviceCaps(display, HORZRES);
        int height = GetDeviceCaps(display, VERTRES);
        DeleteDC(display);

        // Render through an 8-bit or 16-bit DIB without changing the true-color desktop mode.
        if(bits_per_pixel > 16)
        {
            build_modern_windows_virtual_display_mode(&modern_windows_virtual_display_mode, width, height, modern_windows_color_mode);
            current_display_mode = &modern_windows_virtual_display_mode;
            return current_display_mode;
        }

        while(mode != nullptr)
        {
            if(mode->width == width && mode->height == height && mode->bits_per_pixel == bits_per_pixel)
            {
                result = mode;
                current_display_mode = mode;
                break;
            }
            mode = mode->next;
        }
    }
    return result;
}

DisplayMode *get_current_display_mode()
{
    if((graphics_host_flags & 0x800) != 0)
    {
        return find_current_display_mode();
    }
    return nullptr;
}

DisplayMode *begin_available_display_modes(uint32_t mask)
{
    if((graphics_host_flags & 0x800) != 0)
    {
        return begin_display_mode_enumeration(mask);
    }
    return nullptr;
}

DisplayMode *get_next_available_display_mode(uint32_t mask)
{
    if((graphics_host_flags & 0x800) != 0)
    {
        return get_next_display_mode(mask);
    }
    return nullptr;
}

HWND find_top_level_display_window(HWND window)
{
    while((static_cast<uint32_t>(display_cooperative_level_api.get_window_long(window, GWL_STYLE)) & WS_CHILD) != 0)
    {
        window = display_cooperative_level_api.get_parent(window);
    }
    return window;
}

void shutdown_display_mode_host()
{
    if((display_palette_flags & 0x80000000) == 0)
    {
        return;
    }
    while(true)
    {
        if((display_palette_flags & 0x40000000) == 0)
        {
            display_mode_host_shutdown_api.enter_critical_section(&display_host_critical_section);
            if((display_palette_flags & 0x40000000) == 0)
            {
                break;
            }
            display_mode_host_shutdown_api.leave_critical_section(&display_host_critical_section);
        }
        else
        {
            display_mode_host_shutdown_api.sleep(5);
        }
    }
    display_mode_host_shutdown_api.teardown_palette_surface();
    DisplayMode *mode = display_mode_head;
    while(mode != nullptr)
    {
        DisplayMode *next = mode->next;
        display_mode_host_shutdown_api.heap_free(display_mode_host_shutdown_api.get_process_heap(), 0, mode);
        mode = next;
    }
    display_palette_flags &= 0x7fffffff;
    display_mode_host_shutdown_api.delete_critical_section(&display_host_critical_section);

    std::memset(&display_mode_host_state, 0, sizeof(display_mode_host_state));
}


// Shared busy-wait and critical-section acquisition sequence.
uint32_t set_display_cooperative_mode(uint32_t mode)
{
    if(display_palette_window == nullptr)
    {
        return 0x200000;
    }
    display_palette_api.enter_lock();
    if((mode & 0x1000) == 0)
    {
        if((display_palette_flags & 0x1000) != 0)
        {
            HWND window = find_top_level_display_window(display_palette_window);
            if(display_cooperative_level_api.set_cooperative_level(display_direct_draw, window, 8) == 0)
            {
                display_palette_flags &= ~0x1000U;
            }
        }
    }
    else if((display_palette_flags & 0x1000) == 0)
    {
        HWND window = find_top_level_display_window(display_palette_window);
        if(display_cooperative_level_api.set_cooperative_level(display_direct_draw, window, 0x15) == 0)
        {
            display_palette_flags |= 0x1000;
        }
    }
    display_palette_api.leave_lock();
    return 0;
}

void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode)
{
    while(true)
    {
        if((display_palette_flags & 0x40000000) == 0)
        {
            display_palette_api.enter_lock();
            if((display_palette_flags & 0x40000000) == 0)
            {
                break;
            }
            display_palette_api.leave_lock();
        }
        else
        {
            display_surface_operation_api.sleep(5);
        }
    }
    if((display_palette_flags & 0x100002) == 0)
    {
        RECT rectangle{ x, y, x + width - 1, y + height - 1 };
        if(mode == 1)
        {
            display_surface_operation_api.blt_fast(display_direct_draw_primary_surface, x, y, display_direct_draw_secondary_surface, &rectangle, 0x10);
        }
        else if(mode == 2)
        {
            uint32_t effects[25]{};
            effects[0] = 100;
            display_surface_operation_api.blt(display_direct_draw_primary_surface, &rectangle, display_direct_draw_secondary_surface, &rectangle, 0x400, effects);
        }
    }
    else if(mode == 1)
    {
        if(modern_windows_presentation_is_scaled())
        {
            display_surface_operation_api.stretch_blt(display_palette_dc, 0, 0, modern_windows_presentation_state.viewport_width, modern_windows_presentation_state.viewport_height,
                display_palette_dib_dc, 0, 0, display_palette_width, display_palette_height, SRCCOPY);
        }
        else
        {
            display_surface_operation_api.bit_blt(display_palette_dc, x, y, width, height, display_palette_dib_dc, x, y, SRCCOPY);
        }
    }
    else if(mode == 2)
    {
        if(modern_windows_presentation_is_scaled())
        {
            const RECT rectangle = map_modern_windows_presentation_rectangle(x, y, x + width, y + height, display_palette_width, display_palette_height);
            display_surface_operation_api.pat_blt(display_palette_dc, rectangle.left, rectangle.top, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, BLACKNESS);
        }
        else
        {
            display_surface_operation_api.pat_blt(display_palette_dc, x, y, width, height, BLACKNESS);
        }
    }
    display_palette_api.leave_lock();
}

void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode)
{
    while(true)
    {
        if((display_palette_flags & 0x40000000) == 0)
        {
            display_region_synchronization_api.enter_critical_section(&display_host_critical_section);
            if((display_palette_flags & 0x40000000) == 0)
            {
                break;
            }
            display_region_synchronization_api.leave_critical_section(&display_host_critical_section);
        }
        else
        {
            display_region_synchronization_api.sleep(5);
        }
    }
    if((display_palette_flags & 0x100002) == 0)
    {
        auto *native_rectangle = reinterpret_cast<RECT *>(rectangle);
        if(mode == 1)
        {
            display_region_synchronization_api.blt_fast(display_direct_draw_primary_surface, rectangle->left, rectangle->top, display_direct_draw_secondary_surface, native_rectangle, 0x10);
        }
        else if(mode == 2)
        {
            uint32_t effects[25]{};
            effects[0] = sizeof(effects);
            display_region_synchronization_api.blt(display_direct_draw_primary_surface, native_rectangle, display_direct_draw_secondary_surface, native_rectangle, 0x400, effects);
        }
    }
    else if(mode == 1)
    {
        BOOL result;
        if(modern_windows_presentation_is_scaled())
        {
            result = display_region_synchronization_api.stretch_blt(display_palette_dc, 0, 0, modern_windows_presentation_state.viewport_width, modern_windows_presentation_state.viewport_height,
                display_palette_dib_dc, 0, 0, display_palette_width, display_palette_height, SRCCOPY);
        }
        else
        {
            result = display_region_synchronization_api.bit_blt(display_palette_dc, rectangle->left, rectangle->top, rectangle->right - rectangle->left, rectangle->bottom - rectangle->top,
                display_palette_dib_dc, rectangle->left, rectangle->top, SRCCOPY);
        }
    }
    else if(mode == 2)
    {
        if(modern_windows_presentation_is_scaled())
        {
            const RECT destination = map_modern_windows_presentation_rectangle(rectangle->left, rectangle->top, rectangle->right, rectangle->bottom, display_palette_width, display_palette_height);
            display_region_synchronization_api.pat_blt(display_palette_dc, destination.left, destination.top, destination.right - destination.left, destination.bottom - destination.top, BLACKNESS);
        }
        else
        {
            display_region_synchronization_api.pat_blt(display_palette_dc, rectangle->left, rectangle->top, rectangle->right - rectangle->left, rectangle->bottom - rectangle->top, BLACKNESS);
        }
    }
    display_region_synchronization_api.leave_critical_section(&display_host_critical_section);
}

uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch)
{
    const bool direct_draw = (display_palette_flags & 0x100002) == 0;
    uint32_t busy;
    do
    {
        busy = (display_palette_flags & 0x40000000) >> 30;
        if(busy != 0)
        {
            display_target_begin_api.sleep(5);
        }
        else
        {
            display_target_begin_api.enter_critical_section(&display_host_critical_section);
            busy = (display_palette_flags & 0x40000000) >> 30;
            if(busy == 0)
            {
                display_palette_flags |= 0x40000000;
            }
            display_target_begin_api.leave_critical_section(&display_host_critical_section);
        }
    } while(busy != 0);

    if(direct_draw)
    {
        constexpr HRESULT surface_lost = static_cast<HRESULT>(0x887601c2);
        if(display_target_begin_api.is_surface_lost(display_direct_draw_secondary_surface) == surface_lost && display_target_begin_api.restore_surface(display_direct_draw_secondary_surface) != 0)
        {
            return 0x200000;
        }
        LegacyDirectDrawSurfaceDescriptor descriptor{};
        descriptor.size = sizeof(descriptor);
        if(display_target_begin_api.lock_surface(display_direct_draw_secondary_surface, nullptr, &descriptor, 1, nullptr) == 0)
        {
            display_palette_pixels = descriptor.surface;
            *pixels = descriptor.surface;
            *pitch = descriptor.pitch;
            rectangle->left = 0;
            rectangle->top = 0;
            rectangle->right = static_cast<int32_t>(descriptor.width);
            rectangle->bottom = static_cast<int32_t>(descriptor.height);
            return 0;
        }
    }
    else
    {
        *pixels = display_palette_pixels;
        *pitch = static_cast<uint32_t>((current_display_mode->bits_per_pixel >> 3) * display_palette_width);
        rectangle->left = 0;
        rectangle->top = 0;
        rectangle->right = display_palette_width;
        rectangle->bottom = display_palette_height;
        if(display_palette_pixels != nullptr)
        {
            return 0;
        }
    }
    display_palette_flags &= 0xbfffffff;
    return 0x200000;
}

void *create_display_surface(int32_t width, int32_t height, const LegacyDisplayPixelFormat *format, uint32_t options)
{
    while(true)
    {
        if((display_palette_flags & 0x40000000) == 0)
        {
            display_palette_api.enter_lock();
            if((display_palette_flags & 0x40000000) == 0)
            {
                break;
            }
            display_palette_api.leave_lock();
        }
        else
        {
            display_surface_creation_api.sleep(5);
        }
    }

    void *result = nullptr;
    display_surface_creation_api.teardown();
    uint32_t mode_flags = (options & 0x100) | 0x10;
    if((display_palette_flags & 0x100002) == 0)
    {
        while(true)
        {
            uint32_t previous_flags = display_palette_flags;
            LegacyDirectDrawSurfaceDescriptor descriptor{};
            descriptor.size = sizeof(descriptor);
            if((mode_flags & 0x100) == 0)
            {
                descriptor.flags = 0;
                descriptor.caps = 0x200;
                if(display_surface_creation_api.create_direct_draw_surface(display_direct_draw, &descriptor, &display_direct_draw_primary_surface, nullptr) == 0)
                {
                    descriptor = {};
                    descriptor.size = sizeof(descriptor);
                    descriptor.flags = 0x1006;
                    descriptor.height = static_cast<uint32_t>(height);
                    descriptor.width = static_cast<uint32_t>(width);
                    descriptor.pixel_format.size = sizeof(descriptor.pixel_format);
                    descriptor.pixel_format.flags = format->flags;
                    descriptor.pixel_format.bits_per_pixel = format->bits_per_pixel;
                    descriptor.pixel_format.red_mask = format->red_mask;
                    descriptor.pixel_format.green_mask = format->green_mask;
                    descriptor.pixel_format.blue_mask = format->blue_mask;
                    descriptor.caps = 0x840;
                    if(display_surface_creation_api.create_direct_draw_surface(display_direct_draw, &descriptor, &display_direct_draw_secondary_surface, nullptr) == 0)
                    {
                        result = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
                        display_palette_flags |= mode_flags;
                    }
                    else
                    {
                        display_surface_creation_api.release_surface(display_direct_draw_primary_surface);
                        display_direct_draw_primary_surface = nullptr;
                    }
                }
                break;
            }

            if(display_surface_creation_api.set_cooperative_mode(0x1000) == 0)
            {
                descriptor.flags = 0x20;
                descriptor.back_buffer_count = 1;
                descriptor.caps = 0x218;
                if(display_surface_creation_api.create_direct_draw_surface(display_direct_draw, &descriptor, &display_direct_draw_primary_surface, nullptr) == 0)
                {
                    uint32_t caps = 4;
                    if(display_surface_creation_api.get_attached_surface(display_direct_draw_primary_surface, &caps, &display_direct_draw_secondary_surface) == 0)
                    {
                        result = reinterpret_cast<void *>(static_cast<intptr_t>(-1));
                        display_palette_flags |= mode_flags;
                    }
                }
            }
            if(display_direct_draw_secondary_surface != nullptr)
            {
                break;
            }
            if((previous_flags & 0x1000) == 0)
            {
                display_surface_creation_api.set_cooperative_mode(0);
            }
            if(display_direct_draw_primary_surface != nullptr)
            {
                display_surface_creation_api.release_surface(display_direct_draw_primary_surface);
                display_direct_draw_primary_surface = nullptr;
            }
            mode_flags &= ~0x100U;
        }
    }
    else
    {
        display_palette_dc = display_surface_creation_api.get_dc(display_palette_window);
        display_surface_creation_api.set_stretch_blt_mode(display_palette_dc, COLORONCOLOR);
        display_palette_dib_dc = display_surface_creation_api.create_compatible_dc(display_palette_dc);
        if(display_palette_dib_dc == nullptr)
        {
            display_surface_creation_api.release_dc(display_palette_window, display_palette_dc);
            display_palette_dc = nullptr;
        }
        else
        {
            HANDLE heap = display_surface_creation_api.get_process_heap();
            auto *bitmap_info = static_cast<BITMAPINFO *>(display_surface_creation_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x428));
            if(bitmap_info == nullptr)
            {
                display_surface_creation_api.delete_dc(display_palette_dib_dc);
                display_palette_dib_dc = nullptr;
            }
            else
            {
                DWORD compression;
                DWORD color_count;
                if(format->bits_per_pixel == 8)
                {
                    std::memset(display_palette_entries, 0, sizeof(display_palette_entries));
                    display_mode_host_state.palette_version = 0x300;
                    display_mode_host_state.palette_count = 0x100;
                    display_palette = display_surface_creation_api.create_palette(reinterpret_cast<const LOGPALETTE *>(&display_mode_host_state.palette_version));
                    if(display_palette != nullptr)
                    {
                        display_palette_previous_palette = display_surface_creation_api.select_palette(display_palette_dc, display_palette, FALSE);
                    }
                    compression = BI_RGB;
                    color_count = 0x100;
                }
                else if(format->bits_per_pixel == 0x10)
                {
                    compression = format->green_mask == 0x7e0 ? BI_BITFIELDS : BI_RGB;
                    color_count = format->green_mask == 0x7e0 ? 0x10000 : 0x8000;
                }
                else
                {
                    compression = 0;
                    color_count = 0;
                    if(format->bits_per_pixel == 0x20)
                    {
                        compression = BI_BITFIELDS;
                        color_count = 0x1000000;
                    }
                }

                if(display_palette == nullptr && format->bits_per_pixel == 8)
                {
                    display_surface_creation_api.delete_dc(display_palette_dib_dc);
                    display_surface_creation_api.release_dc(display_palette_window, display_palette_dc);
                    display_palette_dib_dc = nullptr;
                    display_palette_dc = nullptr;
                }
                else
                {
                    bitmap_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bitmap_info->bmiHeader.biWidth = width;
                    bitmap_info->bmiHeader.biHeight = -height;
                    bitmap_info->bmiHeader.biPlanes = 1;
                    bitmap_info->bmiHeader.biBitCount = static_cast<WORD>(format->bits_per_pixel);
                    bitmap_info->bmiHeader.biCompression = compression;
                    bitmap_info->bmiHeader.biSizeImage = (format->bits_per_pixel >> 3) * height * width;
                    bitmap_info->bmiHeader.biClrUsed = color_count;
                    reinterpret_cast<uint32_t *>(bitmap_info->bmiColors)[0] = format->red_mask;
                    reinterpret_cast<uint32_t *>(bitmap_info->bmiColors)[1] = format->green_mask;
                    reinterpret_cast<uint32_t *>(bitmap_info->bmiColors)[2] = format->blue_mask;
                    display_palette_bitmap = display_surface_creation_api.create_dib_section(display_palette_dc, bitmap_info, DIB_RGB_COLORS, &result, nullptr, 0);
                    if(display_palette_bitmap == nullptr)
                    {
                        if(format->bits_per_pixel == 8)
                        {
                            display_surface_creation_api.select_palette(display_palette_dc, display_palette_previous_palette, FALSE);
                            display_surface_creation_api.delete_object(display_palette);
                            display_palette = nullptr;
                            display_palette_previous_palette = nullptr;
                        }
                        display_surface_creation_api.delete_dc(display_palette_dib_dc);
                        display_surface_creation_api.release_dc(display_palette_window, display_palette_dc);
                        display_palette_dib_dc = nullptr;
                        display_palette_dc = nullptr;
                    }
                    else
                    {
                        display_palette_width = width;
                        display_palette_height = height;
                        display_palette_bits_per_pixel = static_cast<int32_t>(format->bits_per_pixel);
                        display_palette_pixels = result;
                        display_palette_previous_bitmap = static_cast<HBITMAP>(display_surface_creation_api.select_object(display_palette_dib_dc, display_palette_bitmap));
                        if(format->bits_per_pixel == 8)
                        {
                            display_surface_creation_api.set_palette_entries(display_palette, 0, 0xec, display_palette_entries);
                            display_surface_creation_api.realize_palette(display_palette_dc);
                            RGBQUAD colors[0x100];
                            for(uint32_t index = 0; index < 0x100; ++index)
                            {
                                colors[index].rgbRed = display_palette_entries[index].peRed;
                                colors[index].rgbGreen = display_palette_entries[index].peGreen;
                                colors[index].rgbBlue = display_palette_entries[index].peBlue;
                                colors[index].rgbReserved = 0;
                            }
                            display_surface_creation_api.set_dib_color_table(display_palette_dib_dc, 0, 0x100, colors);
                        }
                        display_palette_flags |= mode_flags;
                    }
                }
                display_surface_creation_api.heap_free(heap, 0, bitmap_info);
            }
        }
    }
    display_palette_api.leave_lock();
    return result;
}

void teardown_display_palette_surface()
{
    while(true)
    {
        if((display_palette_flags & 0x40000000) == 0)
        {
            display_palette_api.enter_lock();
            if((display_palette_flags & 0x40000000) == 0)
            {
                break;
            }
            display_palette_api.leave_lock();
        }
        else
        {
            display_palette_teardown_api.sleep(5);
        }
    }
    if((display_palette_flags & 0x10) != 0)
    {
        if((display_palette_flags & 0x100002) != 0)
        {
            if(display_palette != nullptr)
            {
                display_palette_teardown_api.select_palette(display_palette_dc, display_palette_previous_palette, FALSE);
                display_palette_teardown_api.delete_object(display_palette);
                display_palette = nullptr;
                display_palette_previous_palette = nullptr;
            }
            display_palette_teardown_api.select_object(display_palette_dib_dc, display_palette_previous_bitmap);
            display_palette_teardown_api.delete_object(display_palette_bitmap);
            display_palette_bitmap = nullptr;
            display_palette_previous_bitmap = nullptr;
            display_palette_teardown_api.delete_dc(display_palette_dib_dc);
            display_palette_teardown_api.release_dc(display_palette_window, display_palette_dc);
            display_palette_dib_dc = nullptr;
            display_palette_dc = nullptr;
            display_palette_width = 0;
            display_palette_height = 0;
            display_palette_bits_per_pixel = 0;
            display_palette_pixels = nullptr;
        }
        display_palette_flags &= ~0x10U;
    }
    display_palette_api.leave_lock();
}

HDC get_display_palette_dc()
{
    return display_palette_dc;
}

HDC get_display_palette_dib_dc()
{
    return display_palette_dib_dc;
}

HBITMAP get_display_palette_bitmap()
{
    return display_palette_bitmap;
}

HPALETTE get_display_palette_handle()
{
    return display_palette;
}

PALETTEENTRY *get_display_palette_entries()
{
    return display_palette_entries;
}

UINT apply_display_palette(const PALETTEENTRY *palette_entries, uint32_t update_flags)
{
    bool palette_unchanged = true;
    display_palette_api.enter_lock();
    bool present_after_update = false;
    if(palette_entries != nullptr)
    {
        std::memcpy(display_palette_entries, palette_entries, sizeof(display_palette_entries));
    }
    UINT result = 0;
    if((display_palette_flags & 0x100002) != 0 && display_palette_bits_per_pixel == 8)
    {
        if(current_display_mode->bits_per_pixel == 8)
        {
            if((display_palette_flags & 0x10000) == 0)
            {
                HWND host_window = display_palette_api.get_host_window();
                HWND foreground_window = display_palette_api.get_foreground_window();
                if(foreground_window != host_window)
                {
                    DWORD foreground_process;
                    DWORD host_process;
                    display_palette_api.get_window_thread_process_id(foreground_window, &foreground_process);
                    display_palette_api.get_window_thread_process_id(host_window, &host_process);
                    if(foreground_process != host_process)
                    {
                        display_palette_flags |= 0x10000;
                    }
                }
                if((display_palette_flags & 0x10000) == 0 && (display_palette_flags & 0x20000) != 0)
                {
                    palette_unchanged = false;
                    display_palette_api.unrealize_object(display_palette);
                    for(uint32_t index = 0; index < 0xec; ++index)
                    {
                        display_palette_entries[index].peFlags = 1;
                    }
                    update_flags |= 0x10000;
                    display_palette_api.select_palette(display_palette_dc, display_palette, FALSE);
                    display_palette_flags &= 0xfffdffff;
                }
            }
            if((display_palette_flags & 0x10000) != 0)
            {
                palette_unchanged = false;
                display_palette_api.unrealize_object(display_palette);
                for(uint32_t index = 0; index < 0xec; ++index)
                {
                    display_palette_entries[index].peFlags = 0;
                }
                update_flags |= 0x10000;
                display_palette_api.select_palette(display_palette_dc, display_palette, TRUE);
                display_palette_flags |= 0x20000;
            }
            if((update_flags & 0x10000) == 0)
            {
                if(display_palette_api.animate_palette(display_palette, 0, 0xec, display_palette_entries) != FALSE)
                {
                    result = 0xec;
                }
            }
            else
            {
                if(palette_unchanged)
                {
                    display_palette_api.unrealize_object(display_palette);
                }
                result = display_palette_api.set_palette_entries(display_palette, 0, 0xec, display_palette_entries);
                if(display_palette_api.realize_palette(display_palette_dc) == 0xffffffff)
                {
                    result = 0;
                }
                update_flags |= 0x20000;
            }
        }
        else
        {
            present_after_update = true;
            update_flags |= 0x20000;
        }
        if((update_flags & 0x20000) != 0)
        {
            RGBQUAD colors[0x100];
            for(uint32_t index = 0; index < 0x100; ++index)
            {
                colors[index].rgbBlue = display_palette_entries[index].peBlue;
                colors[index].rgbGreen = display_palette_entries[index].peGreen;
                colors[index].rgbRed = display_palette_entries[index].peRed;
                colors[index].rgbReserved = 0;
            }
            display_palette_api.set_dib_color_table(display_palette_dib_dc, 0, 0x100, colors);
        }
    }
    display_palette_api.leave_lock();
    if(present_after_update)
    {
        display_palette_api.present(display_palette_width, display_palette_height, 1);
    }
    return result;
}

void enable_display_palette_mode()
{
    display_palette_api.enter_lock();
    if((display_palette_flags & 0x100002) != 0)
    {
        display_palette_flags |= 0x10000;
        apply_display_palette(nullptr, 0);
    }
    display_palette_api.leave_lock();
}

void disable_display_palette_mode()
{
    display_palette_api.enter_lock();
    if((display_palette_flags & 0x100002) != 0)
    {
        display_palette_flags &= 0xfffeffff;
        apply_display_palette(nullptr, 0);
    }
    display_palette_api.leave_lock();
}


} // namespace gag
