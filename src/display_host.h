#pragma once

#include <SDL3/SDL.h>
#include "runtime_types.h"

namespace freegag
{

bool initialize_runtime_graphics();

uint32_t initialize_sdl_presenter(int32_t width, int32_t height, const char *window_title);

void set_sdl_presenter_integer_scaling(bool enabled);

bool show_sdl_presenter();

void shutdown_sdl_presenter();

void begin_sdl_presenter_shutdown();

void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode);

void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);

uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch);

uint32_t end_display_target();

void *create_display_surface(int32_t width, int32_t height);

void teardown_display_palette_surface();

uint32_t apply_display_palette(const PaletteEntry *palette, uint32_t update_flags);

PaletteEntry *get_display_palette_entries();

void invalidate_game_framebuffer_rect(int32_t x, int32_t y, int32_t width, int32_t height);

void drain_sdl_presenter_frames();

void request_sdl_presenter_repaint();

void service_sdl_presenter();

bool convert_sdl_presenter_event(SDL_Event *event);

bool set_sdl_presenter_fullscreen(bool fullscreen);

void complete_sdl_presenter_fullscreen_transition(bool fullscreen);

bool get_sdl_presenter_window_rectangle(DisplayRectangle *rectangle);

bool set_sdl_presenter_window_rectangle(const DisplayRectangle &rectangle);

bool center_sdl_presenter_window();

bool is_sdl_presenter_rectangle_visible(const DisplayRectangle &rectangle);

bool get_sdl_presenter_mouse_position(int32_t *x, int32_t *y);

} // namespace freegag
