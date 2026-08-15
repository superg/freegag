#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mmsystem.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "xtet/api.h"

namespace
{

constexpr std::size_t kWaveOutBufferCount = 2;
// XTET's artwork and playfield are authored for a 640x480 host surface.  Passing
// 320x240 does not make the DLL scale; it simply clips the artwork to its upper
// left quarter.
constexpr int kWidth = 640;
constexpr int kHeight = 480;

using xtet::GameExec;
using xtet::GameHostContext;
using xtet::GameInit;
using xtet::GameResultDescriptor;
using xtet::GameWndProc;
using xtet::kGameMessage;

struct App
{
    HMODULE dll{};
    GameInit init{};
    GameWndProc game_wnd_proc{};
    GameExec exec{};
    GameHostContext context{};
    std::array<void *, 35> callbacks{};
    HBITMAP bitmap{};
    HDC memory_dc{};
    void *pixels{};
    bool initialized{};
    bool audio_initializing{};
    bool shutting_down{};
    bool have_result{};
    std::uint32_t result{};
};

App g_app;

#pragma pack(push, 1)
struct PcmFormat16
{
    std::uint16_t format_tag;
    std::uint16_t channels;
    std::uint32_t samples_per_second;
    std::uint32_t average_bytes_per_second;
    std::uint16_t block_alignment;
    std::uint16_t bits_per_sample;
};
#pragma pack(pop)

static_assert(sizeof(PcmFormat16) == 0x10);

struct QueuedSoundData
{
    const char *bytes{};
    DWORD size{};
};

struct PreparedSoundData
{
    WAVEHDR header{};
};

struct SoundHandle
{
    std::uint32_t id{};
    HWAVEOUT output{};
    WAVEFORMATEX format{};
    std::vector<QueuedSoundData> queue;
    std::vector<std::unique_ptr<PreparedSoundData>> prepared;
    // waveOutReset posts WOM_DONE for returned headers. Keep reset headers alive
    // until the sound handle is destroyed so those delayed messages can never
    // alias a newly allocated header at the same address.
    std::vector<std::unique_ptr<PreparedSoundData>> retired;
    std::size_t submitted_count{};
    std::size_t completed_count{};
    bool playing{};
    bool paused{};
};

std::recursive_mutex g_sound_mutex;
std::unordered_map<std::uint32_t, std::unique_ptr<SoundHandle>> g_sounds;
std::uint32_t g_next_sound_handle = 1;

void audio_trace(const char *format, ...)
{
#ifdef _DEBUG
    char message[448]{};
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    char buffer[512]{};
    std::snprintf(buffer, sizeof(buffer), "[%10lu] %s", timeGetTime(), message);
    OutputDebugStringA(buffer);
#else
    (void)format;
#endif
}

void __fastcall invalidate_region(int x, int y, int width, int height)
{
    if(width <= 0 || height <= 0 || !g_app.context.window || !g_app.memory_dc)
        return;

    // XTET renders all frames of some effects before returning to the message
    // loop. GAG presents each dirty rectangle synchronously, so deferring this
    // through WM_PAINT would coalesce the animation into its final frame.
    HDC window_dc = GetDC(g_app.context.window);
    if(!window_dc)
        return;
    BitBlt(window_dc, x, y, width, height, g_app.memory_dc, x, y, SRCCOPY);
    ReleaseDC(g_app.context.window, window_dc);
}

void discard_prepared_audio(SoundHandle &sound)
{
    if(!sound.output)
        return;
    const MMRESULT reset_result = waveOutReset(sound.output);
    audio_trace("XTET audio h=%u reset result=%u inflight=%zu queued=%zu submitted=%zu\n", sound.id, reset_result, sound.prepared.size(), sound.queue.size(), sound.submitted_count);
    for(auto &data : sound.prepared)
    {
        const MMRESULT result = waveOutUnprepareHeader(sound.output, &data->header, sizeof(data->header));
        if(result != MMSYSERR_NOERROR)
        {
            audio_trace("XTET audio h=%u reset unprepare failed result=%u flags=%08lx\n", sound.id, result, data->header.dwFlags);
        }
        sound.retired.push_back(std::move(data));
    }
    sound.prepared.clear();
    sound.submitted_count = 0;
    sound.paused = false;
}

bool submit_audio(SoundHandle &sound, const QueuedSoundData &queued)
{
    auto prepared = std::make_unique<PreparedSoundData>();
    prepared->header.lpData = const_cast<LPSTR>(queued.bytes);
    prepared->header.dwBufferLength = queued.size;
    const MMRESULT prepare_result = waveOutPrepareHeader(sound.output, &prepared->header, sizeof(prepared->header));
    if(prepare_result != MMSYSERR_NOERROR)
    {
        audio_trace("XTET audio h=%u prepare failed result=%u bytes=%lu\n", sound.id, prepare_result, queued.size);
        return false;
    }
    const MMRESULT write_result = waveOutWrite(sound.output, &prepared->header, sizeof(prepared->header));
    if(write_result != MMSYSERR_NOERROR)
    {
        audio_trace("XTET audio h=%u write failed result=%u bytes=%lu\n", sound.id, write_result, queued.size);
        waveOutUnprepareHeader(sound.output, &prepared->header, sizeof(prepared->header));
        return false;
    }
    sound.prepared.push_back(std::move(prepared));
    const std::size_t ordinal = sound.submitted_count + 1;
    if(ordinal <= 8 || ordinal % 100 == 0)
    {
        audio_trace("XTET audio h=%u submit=%zu bytes=%lu inflight=%zu queued=%zu\n", sound.id, ordinal, queued.size, sound.prepared.size(), sound.queue.size());
    }
    return true;
}

bool submit_pending_audio(SoundHandle &sound)
{
    while(sound.prepared.size() < kWaveOutBufferCount && sound.submitted_count < sound.queue.size())
    {
        if(!submit_audio(sound, sound.queue[sound.submitted_count]))
            return false;
        ++sound.submitted_count;
    }
    return true;
}

std::uint32_t __fastcall sound_create(const PcmFormat16 *source_format)
{
    if(!source_format || source_format->format_tag != WAVE_FORMAT_PCM || (source_format->channels != 1 && source_format->channels != 2)
        || (source_format->bits_per_sample != 8 && source_format->bits_per_sample != 16) || source_format->samples_per_second == 0 || source_format->block_alignment == 0)
    {
        return 0;
    }

    auto sound = std::make_unique<SoundHandle>();
    sound->format.wFormatTag = source_format->format_tag;
    sound->format.nChannels = source_format->channels;
    sound->format.nSamplesPerSec = source_format->samples_per_second;
    sound->format.nAvgBytesPerSec = source_format->average_bytes_per_second;
    sound->format.nBlockAlign = source_format->block_alignment;
    sound->format.wBitsPerSample = source_format->bits_per_sample;
    sound->format.cbSize = 0;
    // GAG consumes a newly queued one-shot without a separate StartSound call.
    // XTET explicitly stops its loop handle before constructing that queue, so
    // default-active handles preserve both behaviors.
    sound->playing = true;
    const MMRESULT open_result = waveOutOpen(&sound->output, WAVE_MAPPER, &sound->format, reinterpret_cast<DWORD_PTR>(g_app.context.window), 0, CALLBACK_WINDOW);
    if(open_result != MMSYSERR_NOERROR)
    {
        audio_trace("XTET audio open failed result=%u format=%u/%lu/%u-bit\n", open_result, source_format->channels, source_format->samples_per_second, source_format->bits_per_sample);
        return 0;
    }

    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    std::uint32_t handle = g_next_sound_handle++;
    while(handle == 0 || g_sounds.find(handle) != g_sounds.end())
    {
        handle = g_next_sound_handle++;
    }
    sound->id = handle;
    audio_trace("XTET audio create h=%u format=%u/%lu/%u-bit align=%u avg=%lu\n", handle, sound->format.nChannels, sound->format.nSamplesPerSec, sound->format.wBitsPerSample,
        sound->format.nBlockAlign, sound->format.nAvgBytesPerSec);
    g_sounds.emplace(handle, std::move(sound));
    return handle;
}

void __fastcall sound_destroy(std::uint32_t handle)
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    const auto found = g_sounds.find(handle);
    if(found == g_sounds.end())
        return;
    found->second->playing = false;
    discard_prepared_audio(*found->second);
    waveOutClose(found->second->output);
    g_sounds.erase(found);
}

std::uint32_t __fastcall sound_queue(std::uint32_t handle, const void *bytes, std::uint32_t size, int replace)
{
    if(!bytes || size == 0)
        return 0;
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    const auto found = g_sounds.find(handle);
    if(found == g_sounds.end())
        return 0;
    SoundHandle &sound = *found->second;
    if(replace != 0)
    {
        discard_prepared_audio(sound);
        sound.queue.clear();
    }
    sound.queue.push_back({ static_cast<const char *>(bytes), size });
    const std::size_t queue_size = sound.queue.size();
    if(replace != 0 || queue_size <= 8 || queue_size % 100 == 0)
    {
        audio_trace("XTET audio h=%u queue=%zu bytes=%u replace=%d playing=%d inflight=%zu\n", handle, queue_size, size, replace, sound.playing, sound.prepared.size());
    }
    if(sound.playing && !g_app.audio_initializing && !submit_pending_audio(sound))
        return 0;
    return 1;
}

std::uint32_t __fastcall sound_start(std::uint32_t handle, int restart)
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    const auto found = g_sounds.find(handle);
    if(found == g_sounds.end())
        return 0;
    SoundHandle &sound = *found->second;
    audio_trace("XTET audio h=%u start timing_reset=%d paused=%d queued=%zu submitted=%zu\n", handle, restart, sound.paused, sound.queue.size(), sound.submitted_count);
    // GAG resets only the handle's scheduling timestamp when this flag is set;
    // it does not rewind queue nodes or their byte offsets.
    if(sound.paused)
    {
        if(waveOutRestart(sound.output) != MMSYSERR_NOERROR)
            return 0;
        sound.paused = false;
    }
    sound.playing = true;
    if(g_app.audio_initializing)
        return 1;
    return submit_pending_audio(sound) ? 1u : 0u;
}

std::uint32_t __fastcall sound_stop(std::uint32_t handle, int reset)
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    const auto found = g_sounds.find(handle);
    if(found == g_sounds.end())
        return 0;
    SoundHandle &sound = *found->second;
    audio_trace("XTET audio h=%u stop timing_reset=%d paused=%d queued=%zu submitted=%zu\n", handle, reset, sound.paused, sound.queue.size(), sound.submitted_count);
    sound.playing = false;
    // GAG leaves descriptor offsets intact for either flag value. Pausing this
    // handle's independent waveOut stream is the closest equivalent to removing
    // it from GAG's shared software mix.
    if(!sound.paused)
    {
        if(waveOutPause(sound.output) != MMSYSERR_NOERROR)
            return 0;
        sound.paused = true;
    }
    return 1;
}

void audio_buffer_done(HWAVEOUT output, WAVEHDR *completed)
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    for(auto &entry : g_sounds)
    {
        SoundHandle &sound = *entry.second;
        if(sound.output != output)
            continue;
        const auto found = std::find_if(sound.prepared.begin(), sound.prepared.end(), [completed](const std::unique_ptr<PreparedSoundData> &data) { return &data->header == completed; });
        if(found == sound.prepared.end())
            return;
        waveOutUnprepareHeader(sound.output, &(*found)->header, sizeof((*found)->header));
        sound.prepared.erase(found);
        ++sound.completed_count;
        if(sound.completed_count <= 8 || sound.completed_count % 100 == 0)
        {
            audio_trace("XTET audio h=%u done=%zu inflight=%zu submitted=%zu queued=%zu playing=%d\n", sound.id, sound.completed_count, sound.prepared.size(), sound.submitted_count,
                sound.queue.size(), sound.playing);
        }
        if(sound.playing)
            submit_pending_audio(sound);
        return;
    }
}

void shutdown_audio()
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    for(auto &entry : g_sounds)
    {
        entry.second->playing = false;
        discard_prepared_audio(*entry.second);
        waveOutClose(entry.second->output);
    }
    g_sounds.clear();
}

void finish_audio_initialization()
{
    std::lock_guard<std::recursive_mutex> lock(g_sound_mutex);
    g_app.audio_initializing = false;
    for(auto &entry : g_sounds)
    {
        SoundHandle &sound = *entry.second;
        if(sound.playing)
            submit_pending_audio(sound);
    }
}

template<typename T>
T ordinal(HMODULE module, WORD value)
{
    return reinterpret_cast<T>(GetProcAddress(module, MAKEINTRESOURCEA(value)));
}

bool create_framebuffer(HWND window)
{
    struct BitmapInfo256
    {
        BITMAPINFOHEADER header;
        RGBQUAD colors[256];
    } info{};
    info.header.biSize = sizeof(BITMAPINFOHEADER);
    info.header.biWidth = kWidth;
    info.header.biHeight = -kHeight;
    info.header.biPlanes = 1;
    info.header.biBitCount = 8;
    info.header.biCompression = BI_RGB;
    info.header.biClrUsed = 256;
    info.header.biClrImportant = 256;
    HDC window_dc = GetDC(window);
    g_app.memory_dc = CreateCompatibleDC(window_dc);
    g_app.bitmap = CreateDIBSection(window_dc, reinterpret_cast<BITMAPINFO *>(&info), DIB_RGB_COLORS, &g_app.pixels, nullptr, 0);
    ReleaseDC(window, window_dc);
    if(!g_app.memory_dc || !g_app.bitmap || !g_app.pixels)
        return false;
    SelectObject(g_app.memory_dc, g_app.bitmap);
    std::memset(g_app.pixels, 0, kWidth * kHeight);
    return true;
}

bool install_game_palette(std::wstring &error)
{
    std::array<wchar_t, 32768> module_path_buffer{};
    const DWORD module_path_length = GetModuleFileNameW(g_app.dll, module_path_buffer.data(), static_cast<DWORD>(module_path_buffer.size()));
    if(module_path_length == 0 || module_path_length == module_path_buffer.size())
    {
        error = L"Could not determine the path of XTETDLL.DLL.";
        return false;
    }

    const std::filesystem::path dll_path(std::wstring_view(module_path_buffer.data(), module_path_length));
    const std::filesystem::path palette_path = dll_path.parent_path() / L"VE-GBNEW.BMP";
    if(!std::filesystem::is_regular_file(palette_path))
    {
        error = L"Required palette file is missing:\n" + palette_path.wstring() + L"\n\nVE-GBNEW.BMP must be in the same directory as XTETDLL.DLL.";
        return false;
    }

    std::ifstream input(palette_path, std::ios::binary);
    BITMAPFILEHEADER file_header{};
    BITMAPINFOHEADER info_header{};
    if(!input.read(reinterpret_cast<char *>(&file_header), sizeof(file_header)) || !input.read(reinterpret_cast<char *>(&info_header), sizeof(info_header)))
    {
        error = L"Could not read the BMP headers from:\n" + palette_path.wstring();
        return false;
    }

    constexpr std::uint16_t kBitmapSignature = 0x4d42;
    if(file_header.bfType != kBitmapSignature || info_header.biSize < sizeof(BITMAPINFOHEADER) || info_header.biWidth != kWidth || (info_header.biHeight != kHeight && info_header.biHeight != -kHeight)
        || info_header.biPlanes != 1 || info_header.biBitCount != 8 || info_header.biCompression != BI_RGB || (info_header.biClrUsed != 0 && info_header.biClrUsed != 256))
    {
        error = L"VE-GBNEW.BMP is not the expected 640x480, 256-color BMP:\n" + palette_path.wstring();
        return false;
    }

    const std::uint64_t palette_offset = sizeof(BITMAPFILEHEADER) + info_header.biSize;
    constexpr std::uint64_t kPaletteBytes = 256 * sizeof(RGBQUAD);
    if(palette_offset + kPaletteBytes > file_header.bfOffBits)
    {
        error = L"VE-GBNEW.BMP does not contain a complete 256-color palette:\n" + palette_path.wstring();
        return false;
    }

    std::array<RGBQUAD, 256> colors{};
    input.seekg(static_cast<std::streamoff>(palette_offset), std::ios::beg);
    if(!input.read(reinterpret_cast<char *>(colors.data()), sizeof(colors)))
    {
        error = L"Could not read the palette from:\n" + palette_path.wstring();
        return false;
    }

    const UINT applied = SetDIBColorTable(g_app.memory_dc, 0, 256, colors.data());
    if(applied != 256)
    {
        error = L"Could not install the VE-GBNEW.BMP palette into the framebuffer.";
        return false;
    }

    const std::wstring message = L"XTET: installed palette from " + palette_path.wstring() + L"\n";
    OutputDebugStringW(message.c_str());
    InvalidateRect(g_app.context.window, nullptr, FALSE);
    return true;
}

void copy_game_result(WPARAM wparam)
{
    const auto *descriptor = reinterpret_cast<const GameResultDescriptor *>(wparam);
    if(!descriptor || descriptor->type != 2 || descriptor->size != sizeof(std::uint32_t) || !descriptor->data)
        return;
    std::memcpy(&g_app.result, descriptor->data, sizeof(g_app.result));
    g_app.have_result = true;
    char text[128];
    std::snprintf(text, sizeof(text), "XTET result payload: %u\n", g_app.result);
    OutputDebugStringA(text);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if(message == MM_WOM_DONE)
    {
        audio_buffer_done(reinterpret_cast<HWAVEOUT>(wparam), reinterpret_cast<WAVEHDR *>(lparam));
        return 0;
    }
    if(message == MM_WOM_OPEN || message == MM_WOM_CLOSE)
        return 0;

    if(message == kGameMessage)
    {
        if(lparam == 0x40)
        {
            copy_game_result(wparam);
            return 0;
        }
        if(lparam == 0 || lparam == 1)
        {
            g_app.initialized = false;
            DestroyWindow(window);
            return 0;
        }
    }

    if(g_app.initialized && g_app.game_wnd_proc)
    {
        g_app.game_wnd_proc(window, message, wparam, lparam);
    }

    switch(message)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        BitBlt(dc, 0, 0, kWidth, kHeight, g_app.memory_dc, 0, 0, SRCCOPY);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_KEYDOWN:
        if(wparam == VK_ESCAPE && g_app.initialized && g_app.exec)
        {
            g_app.shutting_down = true;
            g_app.exec(1);
        }
        return 0;
    case WM_CLOSE:
        if(g_app.initialized && g_app.exec && !g_app.shutting_down)
        {
            g_app.shutting_down = true;
            g_app.exec(1);
            return 0;
        }
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, message, wparam, lparam);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show)
{
    WNDCLASSA wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "XTET loader window";
    if(!RegisterClassA(&wc))
        return 1;

    constexpr DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT bounds{ 0, 0, kWidth, kHeight };
    AdjustWindowRect(&bounds, window_style, FALSE);
    HWND window = CreateWindowA(wc.lpszClassName, "XTET minigame loader", window_style, CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top, nullptr, nullptr,
        instance, nullptr);
    if(!window || !create_framebuffer(window))
        return 2;

    g_app.context.window = window;
    g_app.context.framebuffer = g_app.pixels;
    g_app.callbacks[0] = reinterpret_cast<void *>(&invalidate_region);
    g_app.callbacks[1] = reinterpret_cast<void *>(&sound_create);
    g_app.callbacks[2] = reinterpret_cast<void *>(&sound_destroy);
    g_app.callbacks[3] = reinterpret_cast<void *>(&sound_queue);
    // XTET's call sequence establishes its expected ordering: slot 4 stops the
    // loop before queue construction and on pause entry; slot 5 starts it after
    // initialization and on unpause. This ordering is also runtime-verified
    // against the original game.
    g_app.callbacks[4] = reinterpret_cast<void *>(&sound_stop);
    g_app.callbacks[5] = reinterpret_cast<void *>(&sound_start);

    g_app.dll = LoadLibraryA("XTETDLL.DLL");
    if(!g_app.dll)
    {
        MessageBoxA(window, "Could not load XTETDLL.DLL", "XTET loader", MB_ICONERROR);
        return 3;
    }
    g_app.init = ordinal<GameInit>(g_app.dll, 1);
    g_app.game_wnd_proc = ordinal<GameWndProc>(g_app.dll, 2);
    g_app.exec = ordinal<GameExec>(g_app.dll, 3);
    if(!g_app.init || !g_app.game_wnd_proc || !g_app.exec)
    {
        MessageBoxA(window, "XTETDLL.DLL does not expose ordinals 1, 2, and 3", "XTET loader", MB_ICONERROR);
        return 4;
    }

    std::wstring palette_error;
    if(!install_game_palette(palette_error))
    {
        MessageBoxW(window, palette_error.c_str(), L"XTET loader", MB_ICONERROR);
        return 5;
    }

    ShowWindow(window, show);
    UpdateWindow(window);
    g_app.initialized = true;
    g_app.audio_initializing = true;
    g_app.init(&g_app.context, g_app.callbacks.data());
    finish_audio_initialization();

    MSG message{};
    while(GetMessageA(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    if(g_app.have_result)
    {
        char text[128];
        std::snprintf(text, sizeof(text), "Score: %u", g_app.result);
        MessageBoxA(nullptr, text, "XTET loader", MB_OK);
    }
    if(g_app.bitmap)
        DeleteObject(g_app.bitmap);
    if(g_app.memory_dc)
        DeleteDC(g_app.memory_dc);
    shutdown_audio();
    if(g_app.dll)
        FreeLibrary(g_app.dll);
    return 0;
}
