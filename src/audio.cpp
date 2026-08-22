#include "audio.h"
#include "runtime_internal.h"

namespace gag
{

void destroy_runtime_sound_handle(uint32_t handle)
{
    if(runtime_sound_enabled == 0)
    {
        return;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            slot->active = 0;
            RuntimeSoundBufferNode *buffer = slot->buffers;
            while(buffer != nullptr)
            {
                RuntimeSoundBufferNode *next = buffer->next;
                runtime_sound_destroy_api.heap_free(runtime_sound_destroy_api.get_process_heap(), 0, buffer);
                buffer = next;
            }
            if(runtime_sound_maximum_handle <= handle)
            {
                runtime_sound_maximum_handle = handle;
                if(handle != 0)
                {
                    do
                    {
                        runtime_sound_maximum_handle = handle;
                        if(slot->active != 0)
                        {
                            break;
                        }
                        --slot;
                        --handle;
                        runtime_sound_maximum_handle = handle;
                    } while(runtime_sound_slots < slot);
                }
            }
            runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
            return;
        }
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
}

uint32_t create_runtime_sound_handle(WAVEFORMATEX *source_format)
{
    if(runtime_sound_enabled == 0 || runtime_sound_fault != 0)
    {
        return 0;
    }
    uint16_t conversion_flags = 0;
    if(runtime_sound_create_api.ensure_ready(source_format, 0x600) == 0)
    {
        return 0;
    }
    runtime_sound_create_api.wait_for_single_object(runtime_sound_lifecycle_mutex, INFINITE);
    runtime_sound_create_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(runtime_sound_fault != 0)
    {
        runtime_sound_create_api.release_mutex(runtime_sound_mutex);
        runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
        return 0;
    }
    if(runtime_sound_create_api.formats_equal(runtime_sound_output_format, source_format) == 0)
    {
        uint32_t candidate = 1;
        do
        {
            if(runtime_sound_slots[candidate].active != 0)
            {
                break;
            }
            ++candidate;
        } while(candidate < 0x400);
        if(candidate > 0x3ff)
        {
            candidate = 0x3ff;
        }
        if(runtime_sound_slots[candidate].active == 0)
        {
            runtime_sound_output_initialized = 0;
            if(runtime_sound_output_ready != 0)
            {
                runtime_sound_create_api.wave_out_reset(runtime_sound_wave_out);
                runtime_sound_create_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[0], sizeof(WAVEHDR));
                runtime_sound_create_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[1], sizeof(WAVEHDR));
                runtime_sound_create_api.wave_out_reset(runtime_sound_wave_out);
                runtime_sound_create_api.wave_out_close(runtime_sound_wave_out);
                runtime_sound_create_api.release_mutex(runtime_sound_mutex);
                if(runtime_sound_thread != nullptr)
                {
                    runtime_sound_create_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
                    runtime_sound_create_api.wait_for_single_object(runtime_sound_thread, INFINITE);
                    runtime_sound_create_api.close_handle(runtime_sound_thread);
                    runtime_sound_thread = nullptr;
                    runtime_sound_thread_id = 0;
                }
                runtime_sound_ready = 0;
                uint32_t handle = 1;
                if(runtime_sound_maximum_handle > 1)
                {
                    do
                    {
                        runtime_sound_create_api.destroy_sound(handle);
                        ++handle;
                    } while(handle < runtime_sound_maximum_handle);
                }
                runtime_sound_create_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
            }
            runtime_sound_create_api.cleanup_format_buffer();
            runtime_sound_maximum_handle = 0;
            runtime_sound_thread = runtime_sound_create_api.create_thread(nullptr, 0, run_runtime_sound_thread, nullptr, 0, &runtime_sound_thread_id);
            if(runtime_sound_thread == nullptr)
            {
                runtime_sound_create_api.release_mutex(runtime_sound_mutex);
                runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
                return 0;
            }
            if(runtime_sound_create_api.initialize_mixer(source_format, 0x600) == 0)
            {
                runtime_sound_create_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
                runtime_sound_create_api.wait_for_single_object(runtime_sound_thread, INFINITE);
                runtime_sound_create_api.close_handle(runtime_sound_thread);
                runtime_sound_thread = nullptr;
                runtime_sound_thread_id = 0;
                runtime_sound_create_api.release_mutex(runtime_sound_mutex);
                runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
                return 0;
            }
            runtime_sound_ready = 1;
        }
        else if(runtime_sound_create_api.calculate_conversion(runtime_sound_output_format, source_format, &conversion_flags) == 0)
        {
            runtime_sound_create_api.release_mutex(runtime_sound_mutex);
            runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
            return 0;
        }
    }
    uint32_t handle = 1;
    do
    {
        if(runtime_sound_slots[handle].active == 0)
        {
            break;
        }
        ++handle;
    } while(handle < 0x400);
    if(handle < 0x400)
    {
        RuntimeSoundSlot &slot = runtime_sound_slots[handle];
        slot.playing = 0;
        slot.buffers = nullptr;
        slot.playback_state = 0;
        slot.schedule_state = 0;
        slot.loop_value_1 = 0;
        slot.loop_value_2 = 0;
        slot.volume = 100;
        slot.conversion_flags = conversion_flags;
        slot.transition_flags = 0;
        slot.active = 1;
        if(runtime_sound_maximum_handle <= handle)
        {
            runtime_sound_maximum_handle = handle;
        }
        runtime_sound_create_api.release_mutex(runtime_sound_mutex);
        runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
        return handle;
    }
    runtime_sound_create_api.release_mutex(runtime_sound_mutex);
    runtime_sound_create_api.release_mutex(runtime_sound_lifecycle_mutex);
    return 0xffffffff;
}

uint32_t queue_runtime_sound_data(uint32_t handle, void *data, uint32_t size, int32_t replace)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle <= runtime_sound_maximum_handle && handle != 0)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            auto *node = static_cast<RuntimeSoundBufferNode *>(runtime_sound_destroy_api.heap_alloc(runtime_sound_destroy_api.get_process_heap(), HEAP_ZERO_MEMORY, sizeof(RuntimeSoundBufferNode)));
            node->size = size;
            node->offset = 0;
            node->next = nullptr;
            node->data = data;
            node->schedule_offset = 0;
            slot->playback_state = 0;
            RuntimeSoundBufferNode *tail = slot->buffers;
            if(replace != 0)
            {
                while(tail != nullptr)
                {
                    RuntimeSoundBufferNode *next = tail->next;
                    runtime_sound_destroy_api.heap_free(runtime_sound_destroy_api.get_process_heap(), 0, tail);
                    tail = next;
                }
                slot->schedule_state = 0;
                tail = nullptr;
            }
            if(tail == nullptr)
            {
                slot->buffers = node;
            }
            else
            {
                while(tail->next != nullptr)
                {
                    tail = tail->next;
                }
                tail->next = node;
            }
            runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
            return 1;
        }
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    return 0;
}

uint32_t start_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(reset_timing != 0)
        {
            slot->schedule_state = 0;
        }
        slot->playing = 1;
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
        return 1;
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    return 0;
}

uint32_t stop_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->playing != 0)
        {
            slot->playing = 0;
            if(reset_timing != 0)
            {
                slot->playback_state = 0;
            }
        }
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
        return 1;
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    return 0;
}

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value)
{
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            slot->loop_value_1 = value;
            slot->loop_value_2 = value;
        }
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    }
}

RuntimeSoundSlot *get_runtime_sound_slot(uint32_t handle)
{
    if(handle != 0 && handle < 0x400 && runtime_sound_slots[handle].active != 0)
    {
        return &runtime_sound_slots[handle];
    }
    return nullptr;
}

uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->playing != 0)
        {
            if(reset_timing != 0)
            {
                slot->playback_state = 0;
            }
            const uint32_t old_flags = slot->transition_flags;
            slot->transition_flags = old_flags & ~2u;
            slot->transition_flags = (old_flags & ~2u) | 1;
            slot->fade_current = 0;
            const uint64_t blocks = (static_cast<uint64_t>(runtime_sound_output_format->nAvgBytesPerSec * duration_ms) / 1000) / runtime_sound_mixer_data_size;
            if(static_cast<int32_t>(blocks) == 0)
            {
                slot->fade_step = 100;
            }
            else
            {
                slot->fade_step = static_cast<uint8_t>(100 / blocks);
                slot->fade_block_index = 0;
            }
            if(slot->fade_step == 0)
            {
                const int32_t index = static_cast<int32_t>(blocks / 100) - 1;
                slot->fade_block_index = index;
                slot->fade_current = index;
            }
            slot->playing = 0;
        }
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
        return 1;
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    return 0;
}

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(reset_timing != 0)
        {
            slot->schedule_state = 0;
        }
        if(slot->playing == 0)
        {
            const uint32_t old_flags = slot->transition_flags;
            slot->transition_flags = old_flags & ~1u;
            slot->transition_flags = (old_flags & ~1u) | 2;
            slot->fade_current = 0;
            const uint64_t blocks = (static_cast<uint64_t>(runtime_sound_output_format->nAvgBytesPerSec * duration_ms) / 1000) / runtime_sound_mixer_data_size;
            if(static_cast<int32_t>(blocks) == 0)
            {
                slot->fade_step = 100;
            }
            else
            {
                slot->fade_step = static_cast<uint8_t>(100 / blocks);
                slot->fade_block_index = 0;
            }
            if(slot->fade_step == 0)
            {
                const int32_t index = static_cast<int32_t>(blocks / 100) - 1;
                slot->fade_block_index = index;
                slot->fade_current = index;
            }
            slot->playing = 1;
        }
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
        return 1;
    }
    runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
    return 0;
}

uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    if(volume > 100)
    {
        volume = 100;
    }
    runtime_sound_destroy_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    if(handle <= runtime_sound_maximum_handle && handle != 0)
    {
        runtime_sound_slots[handle].volume = volume;
        runtime_sound_destroy_api.release_mutex(runtime_sound_mutex);
        return 1;
    }
    return 0;
}

void CALLBACK runtime_wave_out_callback(HWAVEOUT wave_out, UINT message, DWORD_PTR, DWORD_PTR, DWORD_PTR)
{
    if(message == WOM_OPEN)
    {
        runtime_wave_out_callback_api.post_message(runtime_sound_window, WOM_OPEN, reinterpret_cast<WPARAM>(wave_out), 0);
    }
    else if(message == WOM_CLOSE)
    {
        runtime_wave_out_callback_api.post_message(runtime_sound_window, WOM_CLOSE, reinterpret_cast<WPARAM>(wave_out), 0);
        runtime_sound_output_ready = 0;
    }
    else if(message == WOM_DONE)
    {
        runtime_wave_out_callback_api.post_message(runtime_sound_window, WOM_DONE, reinterpret_cast<WPARAM>(wave_out), runtime_wave_out_callback_api.time_get_time());
    }
}

uint32_t shutdown_runtime_sound()
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_shutdown_api.wait_for_single_object(runtime_sound_lifecycle_mutex, INFINITE);
    runtime_sound_output_initialized = 0;
    if(runtime_sound_output_ready != 0)
    {
        runtime_sound_shutdown_api.wave_out_reset(runtime_sound_wave_out);
        runtime_sound_shutdown_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[0], sizeof(WAVEHDR));
        runtime_sound_shutdown_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[1], sizeof(WAVEHDR));
        runtime_sound_shutdown_api.wave_out_reset(runtime_sound_wave_out);
        runtime_sound_shutdown_api.wave_out_close(runtime_sound_wave_out);
        if(runtime_sound_thread != nullptr)
        {
            runtime_sound_shutdown_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
            runtime_sound_shutdown_api.wait_for_single_object(runtime_sound_thread, INFINITE);
            runtime_sound_shutdown_api.close_handle(runtime_sound_thread);
            runtime_sound_thread = nullptr;
            runtime_sound_thread_id = 0;
        }
        uint32_t handle = 1;
        if(runtime_sound_maximum_handle > 1)
        {
            do
            {
                runtime_sound_shutdown_api.destroy_sound(handle);
                ++handle;
            } while(handle < runtime_sound_maximum_handle);
        }
        runtime_sound_shutdown_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
    }
    runtime_sound_shutdown_api.cleanup_format_buffer();
    runtime_sound_enabled = 0;
    runtime_sound_shutdown_api.close_handle(runtime_sound_mutex);
    runtime_sound_shutdown_api.close_handle(runtime_sound_lifecycle_mutex);
    return 1;
}

void toggle_runtime_sound_state()
{
    runtime_sound_toggle_state = ~runtime_sound_toggle_state;
}

uint32_t pause_runtime_sound_output(int32_t close_output)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_pause_resume_api.wait_for_single_object(runtime_sound_lifecycle_mutex, INFINITE);
    runtime_sound_mixing_suppressed = 1;
    if(close_output != 0)
    {
        runtime_sound_output_initialized = 0;
        if(runtime_sound_output_ready != 0)
        {
            runtime_sound_pause_resume_api.wave_out_reset(runtime_sound_wave_out);
            runtime_sound_pause_resume_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[0], sizeof(WAVEHDR));
            runtime_sound_pause_resume_api.wave_out_unprepare_header(runtime_sound_wave_out, runtime_sound_headers[1], sizeof(WAVEHDR));
            runtime_sound_pause_resume_api.wave_out_reset(runtime_sound_wave_out);
            runtime_sound_pause_resume_api.wave_out_close(runtime_sound_wave_out);
            if(runtime_sound_thread != nullptr)
            {
                runtime_sound_pause_resume_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
                runtime_sound_pause_resume_api.wait_for_single_object(runtime_sound_thread, INFINITE);
                runtime_sound_pause_resume_api.close_handle(runtime_sound_thread);
                runtime_sound_thread = nullptr;
                runtime_sound_thread_id = 0;
            }
        }
    }
    runtime_sound_pause_resume_api.release_mutex(runtime_sound_lifecycle_mutex);
    return 1;
}

uint32_t resume_runtime_sound_output()
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_pause_resume_api.wait_for_single_object(runtime_sound_lifecycle_mutex, INFINITE);
    runtime_sound_mixing_suppressed = 0;
    if(runtime_sound_output_initialized == 0 && runtime_sound_output_ready == 0)
    {
        runtime_sound_thread = runtime_sound_pause_resume_api.create_thread(nullptr, 0, run_runtime_sound_thread, nullptr, 0, &runtime_sound_thread_id);
        if(runtime_sound_thread == nullptr)
        {
            runtime_sound_output_initialized = 0;
        }
        else
        {
            while(runtime_sound_window == nullptr)
            {
                if(runtime_sound_fault != 0)
                {
                    return 0;
                }
                runtime_sound_pause_resume_api.sleep(0);
            }
            const MMRESULT result =
                runtime_sound_pause_resume_api.wave_out_open(&runtime_sound_wave_out, WAVE_MAPPER, runtime_sound_output_format, reinterpret_cast<DWORD_PTR>(runtime_wave_out_callback), 0, 0x30000);
            if(result == MMSYSERR_NOERROR)
            {
                while(runtime_sound_output_ready == 0)
                {
                    runtime_sound_pause_resume_api.sleep(0);
                }
                runtime_sound_output_initialized = 1;
            }
            else
            {
                runtime_sound_pause_resume_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
                runtime_sound_pause_resume_api.wait_for_single_object(runtime_sound_thread, INFINITE);
                runtime_sound_pause_resume_api.close_handle(runtime_sound_thread);
                runtime_sound_thread = nullptr;
                runtime_sound_thread_id = 0;
                runtime_sound_output_initialized = 0;
            }
        }
    }
    runtime_sound_pause_resume_api.release_mutex(runtime_sound_lifecycle_mutex);
    return 1;
}

uint32_t ensure_runtime_sound_ready(WAVEFORMATEX *format, uint32_t mixer_argument)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    runtime_sound_readiness_api.wait_for_single_object(runtime_sound_lifecycle_mutex, INFINITE);
    if(runtime_sound_ready == 0)
    {
        runtime_sound_maximum_handle = 0;
        runtime_sound_thread = runtime_sound_readiness_api.create_thread(nullptr, 0, run_runtime_sound_thread, nullptr, 0, &runtime_sound_thread_id);
        if(runtime_sound_thread != nullptr)
        {
            runtime_sound_readiness_api.sleep(2);
            if(runtime_sound_readiness_api.initialize_mixer(format, mixer_argument) == 0)
            {
                runtime_sound_readiness_api.post_message(runtime_sound_window, WOM_CLOSE, 0, 0);
                runtime_sound_readiness_api.wait_for_single_object(runtime_sound_thread, INFINITE);
                runtime_sound_readiness_api.close_handle(runtime_sound_thread);
                runtime_sound_thread = nullptr;
                runtime_sound_thread_id = 0;
            }
            else
            {
                runtime_sound_ready = 1;
            }
        }
    }
    runtime_sound_readiness_api.release_mutex(runtime_sound_lifecycle_mutex);
    return 1;
}

DWORD WINAPI run_runtime_sound_thread(LPVOID)
{
    runtime_sound_window_creation_failed = 0;
    runtime_sound_window =
        runtime_sound_thread_api.create_window_ex(0, runtime_sound_window_class_name, nullptr, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, 0x244, 0x1e0, nullptr, nullptr, runtime_sound_instance, nullptr);
    if(runtime_sound_window == nullptr)
    {
        runtime_sound_window_creation_failed = 1;
        return 0;
    }
    runtime_sound_thread_api.show_window(runtime_sound_window, SW_HIDE);
    runtime_sound_thread_api.set_thread_priority(runtime_sound_thread_api.get_current_thread(), THREAD_PRIORITY_HIGHEST);
    MSG message{};
    while(runtime_sound_thread_api.get_message(&message, nullptr, 0, 0) != 0)
    {
        runtime_sound_thread_api.dispatch_message(&message);
    }
    runtime_sound_window = nullptr;
    runtime_sound_thread_api.set_thread_priority(runtime_sound_thread_api.get_current_thread(), THREAD_PRIORITY_NORMAL);
    runtime_sound_thread_api.exit_thread(1);
    return 1;
}

LRESULT CALLBACK runtime_sound_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if(message == WOM_OPEN)
    {
        for(uint32_t index = 0; index < 2; ++index)
        {
            RuntimeSoundOutputBlock &output = runtime_sound_outputs[index];
            output.format->wFormatTag = runtime_sound_output_format->wFormatTag;
            output.format->nChannels = runtime_sound_output_format->nChannels;
            output.format->nSamplesPerSec = runtime_sound_output_format->nSamplesPerSec;
            output.format->nAvgBytesPerSec = runtime_sound_output_format->nAvgBytesPerSec;
            output.format->nBlockAlign = runtime_sound_output_format->nBlockAlign;
            output.format->wBitsPerSample = runtime_sound_output_format->wBitsPerSample;
            output.header->dwLoops = 0;
            output.header->dwFlags = 0;
            output.header->lpData = reinterpret_cast<LPSTR>(output.data);
            output.header->dwBufferLength = runtime_sound_mixer_data_size;
            output.header->dwUser = 0;
            runtime_sound_window_api.wave_out_prepare_header(reinterpret_cast<HWAVEOUT>(wparam), output.header, sizeof(WAVEHDR));
            output.header->dwFlags |= WHDR_DONE;
        }
        runtime_sound_output_index = 0;
        runtime_sound_output_initialized = 1;
        runtime_sound_output_ready = 1;
    }
    if(message == WOM_OPEN || message == WOM_DONE)
    {
        if(runtime_sound_output_initialized != 0)
        {
            for(uint32_t count = 0; count < 2; ++count)
            {
                RuntimeSoundOutputBlock &output = runtime_sound_outputs[runtime_sound_output_index];
                if((output.header->dwFlags & WHDR_DONE) != 0)
                {
                    output.header->dwFlags &= ~WHDR_DONE;
                    runtime_sound_window_api.wait_for_single_object(runtime_sound_mutex, INFINITE);
                    // HWAVEOUT is an opaque pointer-sized handle and cannot serve as a millisecond marker.
                    // RuntimeWaveOutCallback already supplies timeGetTime() in lParam for WOM_DONE.
                    runtime_sound_mixer(message == WOM_DONE ? static_cast<uint32_t>(lparam) : static_cast<uint32_t>(wparam));
                    runtime_sound_window_api.release_mutex(runtime_sound_mutex);
                    runtime_sound_window_api.wave_out_write(reinterpret_cast<HWAVEOUT>(wparam), output.header, sizeof(WAVEHDR));
                    ++runtime_sound_output_index;
                    if(runtime_sound_output_index > 1)
                    {
                        runtime_sound_output_index = 0;
                    }
                }
            }
        }
        return 0;
    }
    if(message == WM_DESTROY)
    {
        runtime_sound_window_api.post_quit_message(0);
        return 0;
    }
    if(message == WOM_CLOSE)
    {
        runtime_sound_window_api.destroy_window(window);
        return 0;
    }
    return runtime_sound_window_api.def_window_proc(window, message, wparam, lparam);
}

// Saturating 8-bit addition used by the PCM mixers.
void mix_unsigned_8bit_sample(uint8_t *destination, uint8_t scaled_sample, uint8_t scaled_silence)
{
    const int mixed = static_cast<int>(*destination) + static_cast<int>(scaled_sample) - static_cast<int>(scaled_silence);
    if(mixed < 0)
    {
        *destination = 2;
    }
    else if(mixed > 255)
    {
        *destination = 0xfd;
    }
    else
    {
        *destination = static_cast<uint8_t>(mixed);
    }
}

// Signed 16-bit overflow clamping used by the PCM mixers.
void mix_signed_16bit_sample(uint16_t *destination, uint16_t scaled_sample)
{
    const uint16_t original = *destination;
    *destination = static_cast<uint16_t>(original + scaled_sample);
    if(original < 0x8000)
    {
        if(scaled_sample < 0x8000 && *destination > 0x7fff)
        {
            *destination = 0x7ffa;
        }
    }
    else if(scaled_sample > 0x7ffe && *destination < 0x8000)
    {
        *destination = 0x8005;
    }
}

// Shared implementation for the PCM mixer entry points.
void mix_runtime_sound_pcm(uint32_t marker, uint32_t mode)
{
    const bool sixteen_bit = mode >= 2;
    uint8_t *output = runtime_sound_outputs[runtime_sound_output_index].data;
    if(output == nullptr)
    {
        return;
    }
    uint32_t clear_bytes = runtime_sound_mixer_data_size;
    if(mode == 1 || mode == 2)
    {
        clear_bytes &= ~1U;
    }
    else if(mode == 3)
    {
        clear_bytes &= ~3U;
    }
    std::memset(output, sixteen_bit ? 0 : 0x80, clear_bytes);
    if(runtime_sound_base_state != 0)
    {
        return;
    }
    uint32_t handle = 1;
    do
    {
        RuntimeSoundSlot &slot = runtime_sound_slots[handle];
        if(slot.active != 0)
        {
            if(slot.playing != 0)
            {
                if((slot.transition_flags & 2) == 0 || slot.volume == 0)
                {
                    slot.transition_flags &= ~2U;
                    if(slot.playback_state == 0)
                    {
                        slot.playback_state = marker;
                    }
                    if(slot.buffers != nullptr)
                    {
                        slot.buffers->schedule_offset = 0;
                    }
                    ++handle;
                    continue;
                }
                if(slot.fade_step == 0)
                {
                    if(slot.fade_current == 0)
                    {
                        slot.fade_current = slot.fade_block_index;
                        --slot.volume;
                    }
                    else
                    {
                        --slot.fade_current;
                    }
                }
                else if(slot.volume < slot.fade_step)
                {
                    slot.volume = 0;
                }
                else
                {
                    slot.volume = static_cast<uint8_t>(slot.volume - slot.fade_step);
                }
            }
            if((slot.transition_flags & 1) == 0 || slot.volume > 99)
            {
                slot.transition_flags &= ~1U;
            }
            else if(slot.fade_step == 0)
            {
                if(slot.fade_current == 0)
                {
                    slot.fade_current = slot.fade_block_index;
                    ++slot.volume;
                }
                else
                {
                    --slot.fade_current;
                }
            }
            else
            {
                const uint8_t volume = static_cast<uint8_t>(slot.volume + slot.fade_step);
                slot.volume = volume < 101 ? volume : 100;
            }
            uint32_t output_offset = 0;
            for(;;)
            {
                RuntimeSoundBufferNode *node = slot.buffers;
                if(node == nullptr || output_offset >= runtime_sound_mixer_data_size)
                {
                    break;
                }
                if(node->data != nullptr)
                {
                    uint32_t output_multiplier = 1;
                    uint32_t input_stride = 1;
                    const uint16_t conversion = slot.conversion_flags;
                    if(sixteen_bit)
                    {
                        if((conversion & 0x1000) != 0)
                        {
                            output_multiplier = 2;
                        }
                    }
                    else if((conversion & 0x2000) != 0)
                    {
                        input_stride = 2;
                    }
                    if((conversion & 0x00f0) != 0)
                    {
                        input_stride = sixteen_bit ? 0x10 / ((conversion & 0x00f0) >> 4) : (input_stride << 4) / ((conversion & 0x00f0) >> 4);
                    }
                    if((conversion & 0x000f) != 0)
                    {
                        output_multiplier = (conversion & 0x000f) * output_multiplier * 2;
                    }
                    if((node->offset * output_multiplier) / input_stride == runtime_sound_mixer_data_size
                        || (node->schedule_offset * output_multiplier) / input_stride == runtime_sound_mixer_data_size)
                    {
                        slot.schedule_state = marker;
                    }
                    const uint32_t available = ((node->size - node->offset) * output_multiplier) / input_stride;
                    uint32_t output_bytes = runtime_sound_mixer_data_size - output_offset;
                    if(available <= output_bytes)
                    {
                        output_bytes = available;
                    }
                    if((sixteen_bit ? output_bytes >> 1 : output_bytes) != 0)
                    {
                        uint32_t source_count = (sixteen_bit ? output_bytes >> 1 : output_bytes) / output_multiplier;
                        uint8_t *destination = output + output_offset;
                        const uint8_t *source = static_cast<const uint8_t *>(node->data) + node->offset;
                        const uint32_t volume = slot.volume;
                        const uint8_t scaled_silence = static_cast<uint8_t>((volume * 0x80U) / 100U);
                        if(runtime_sound_mixing_suppressed == 0)
                        {
                            if(sixteen_bit)
                            {
                                uint16_t *destination_16 = reinterpret_cast<uint16_t *>(destination);
                                if(conversion == 0)
                                {
                                    do
                                    {
                                        const int16_t sample = *reinterpret_cast<const int16_t *>(source);
                                        source += 2;
                                        mix_signed_16bit_sample(destination_16++, static_cast<uint16_t>((static_cast<int64_t>(sample) * volume) / 100));
                                    } while(--source_count != 0);
                                }
                                else if((conversion & 0xf000) == 0)
                                {
                                    do
                                    {
                                        const int16_t sample = *reinterpret_cast<const int16_t *>(source);
                                        source += input_stride * 2;
                                        const uint16_t scaled = static_cast<uint16_t>((static_cast<int64_t>(sample) * volume) / 100);
                                        uint32_t repeat = output_multiplier;
                                        do
                                        {
                                            mix_signed_16bit_sample(destination_16++, scaled);
                                        } while(--repeat != 0);
                                    } while(--source_count != 0);
                                }
                                else
                                {
                                    uint32_t byte_count = source_count << 1;
                                    do
                                    {
                                        const uint8_t scaled_byte = static_cast<uint8_t>((*source * volume) / 100U - 0x80U);
                                        source += input_stride;
                                        const uint16_t scaled = static_cast<uint16_t>(scaled_byte << 8);
                                        uint32_t repeat = output_multiplier >> 1;
                                        do
                                        {
                                            mix_signed_16bit_sample(destination_16++, scaled);
                                        } while(--repeat != 0);
                                    } while(--byte_count != 0);
                                }
                            }
                            else if(conversion == 0)
                            {
                                do
                                {
                                    mix_unsigned_8bit_sample(destination++, static_cast<uint8_t>((*source++ * volume) / 100U), scaled_silence);
                                } while(--source_count != 0);
                            }
                            else if((conversion & 0xf000) == 0)
                            {
                                do
                                {
                                    uint8_t scaled = static_cast<uint8_t>((*source * volume) / 100U);
                                    source += input_stride;
                                    uint32_t repeat = output_multiplier;
                                    do
                                    {
                                        mix_unsigned_8bit_sample(destination++, scaled, scaled_silence);
                                    } while(--repeat != 0);
                                } while(--source_count != 0);
                            }
                            else
                            {
                                do
                                {
                                    const uint8_t centered = static_cast<uint8_t>(source[1] - 0x80);
                                    uint8_t scaled = static_cast<uint8_t>((centered * volume) / 100U);
                                    source += input_stride;
                                    uint32_t repeat = output_multiplier;
                                    do
                                    {
                                        mix_unsigned_8bit_sample(destination++, scaled, scaled_silence);
                                    } while(--repeat != 0);
                                } while(--source_count != 0);
                            }
                        }
                    }
                    output_offset += output_bytes;
                    const uint32_t input_bytes = (output_bytes * input_stride) / output_multiplier;
                    node->offset += input_bytes;
                    if(node->schedule_offset <= runtime_sound_mixer_data_size)
                    {
                        node->schedule_offset += input_bytes;
                    }
                    if(node->offset < node->size)
                    {
                        break;
                    }
                }
                if(slot.loop_value_1 == 0)
                {
                    slot.buffers = node->next;
                    runtime_sound_destroy_api.heap_free(runtime_sound_destroy_api.get_process_heap(), 0, node);
                    slot.playback_state = marker;
                    continue;
                }
                node->offset = 0;
                if(slot.loop_value_1 == 0xffffffff || --slot.loop_value_2 != 0)
                {
                    continue;
                }
                slot.loop_value_2 = slot.loop_value_1;
                slot.playing = 1;
                slot.playback_state = marker;
                break;
            }
        }
        ++handle;
    } while(handle <= static_cast<uint8_t>(runtime_sound_maximum_handle));
}

void mix_runtime_sound_8bit_mono(uint32_t marker)
{
    mix_runtime_sound_pcm(marker, 0);
}

void mix_runtime_sound_8bit_stereo(uint32_t marker)
{
    mix_runtime_sound_pcm(marker, 1);
}

void mix_runtime_sound_16bit_mono(uint32_t marker)
{
    mix_runtime_sound_pcm(marker, 2);
}

void mix_runtime_sound_16bit_stereo(uint32_t marker)
{
    mix_runtime_sound_pcm(marker, 3);
}

uint32_t initialize_runtime_wave_out_mixer(WAVEFORMATEX *format, uint32_t)
{
    if(runtime_sound_enabled == 0 || runtime_sound_fault != 0)
    {
        return 0;
    }
    runtime_sound_output_ready = 0;
    const int32_t sample_width = static_cast<int32_t>(format->nChannels) * static_cast<int32_t>(format->wBitsPerSample);
    runtime_sound_mixer_data_size = static_cast<uint32_t>((sample_width + (sample_width < 0 ? 7 : 0)) >> 3) * (format->nSamplesPerSec / 11000) * 0x800;
    constexpr size_t wave_header_offset = (sizeof(WAVEFORMATEX) + alignof(WAVEHDR) - 1) & ~(alignof(WAVEHDR) - 1);
    constexpr size_t wave_data_offset = wave_header_offset + sizeof(WAVEHDR);
    const size_t block_stride = runtime_sound_mixer_data_size + wave_data_offset;
    if(runtime_sound_output_initialized != 0)
    {
        return 1;
    }
    runtime_sound_format_buffer = runtime_wave_mixer_initialize_api.heap_alloc(runtime_wave_mixer_initialize_api.get_process_heap(), 0, block_stride * 2);
    if(runtime_sound_format_buffer == nullptr)
    {
        return 0;
    }
    std::memset(runtime_sound_format_buffer, 0, block_stride * 2);
    for(uint32_t index = 0; index < 2; ++index)
    {
        uint8_t *block = static_cast<uint8_t *>(runtime_sound_format_buffer) + block_stride * index;
        runtime_sound_outputs[index].format = reinterpret_cast<WAVEFORMATEX *>(block);
        runtime_sound_outputs[index].header = reinterpret_cast<WAVEHDR *>(block + wave_header_offset);
        runtime_sound_outputs[index].data = block + wave_data_offset;
        runtime_sound_headers[index] = runtime_sound_outputs[index].header;
    }
    runtime_sound_output_format = runtime_sound_outputs[0].format;
    if(format == nullptr)
    {
        runtime_wave_mixer_initialize_api.cleanup_format_buffer();
        return 0;
    }
    runtime_sound_output_format->wBitsPerSample = format->wBitsPerSample;
    runtime_sound_output_format->wFormatTag = format->wFormatTag;
    runtime_sound_output_format->nChannels = format->nChannels;
    runtime_sound_output_format->nSamplesPerSec = format->nSamplesPerSec;
    runtime_sound_output_format->nAvgBytesPerSec = runtime_sound_output_format->nChannels * runtime_sound_output_format->wBitsPerSample * runtime_sound_output_format->nSamplesPerSec >> 3;
    const int32_t block_width = static_cast<int32_t>(runtime_sound_output_format->nChannels) * static_cast<int32_t>(runtime_sound_output_format->wBitsPerSample);
    runtime_sound_output_format->nBlockAlign = static_cast<WORD>((block_width + (block_width < 0 ? 7 : 0)) >> 3);
    if(runtime_sound_output_format->wBitsPerSample == 8)
    {
        if(runtime_sound_output_format->nChannels == 1)
        {
            runtime_sound_mixer = mix_runtime_sound_8bit_mono;
        }
        else if(runtime_sound_output_format->nChannels == 2)
        {
            runtime_sound_mixer = mix_runtime_sound_8bit_stereo;
        }
    }
    else if(runtime_sound_output_format->wBitsPerSample == 16)
    {
        if(runtime_sound_output_format->nChannels == 1)
        {
            runtime_sound_mixer = mix_runtime_sound_16bit_mono;
        }
        else if(runtime_sound_output_format->nChannels == 2)
        {
            runtime_sound_mixer = mix_runtime_sound_16bit_stereo;
        }
    }
    while(runtime_sound_window == nullptr)
    {
        if(runtime_sound_window_creation_failed != 0)
        {
            runtime_wave_mixer_initialize_api.cleanup_format_buffer();
            return 0;
        }
        runtime_wave_mixer_initialize_api.sleep(0);
    }
    if(runtime_wave_mixer_initialize_api.wave_out_open(&runtime_sound_wave_out, WAVE_MAPPER, runtime_sound_output_format, reinterpret_cast<DWORD_PTR>(runtime_wave_out_callback), 0, 0x30000)
        != MMSYSERR_NOERROR)
    {
        runtime_sound_fault = 1;
        runtime_wave_mixer_initialize_api.cleanup_format_buffer();
        return 0;
    }
    runtime_sound_base_state = 0;
    while(runtime_sound_output_ready == 0)
    {
        runtime_wave_mixer_initialize_api.sleep(0);
    }
    return 1;
}

void initialize_runtime_sound_class(HINSTANCE instance)
{
    if(runtime_sound_enabled != 0)
    {
        return;
    }
    WNDCLASSA window_class{};
    window_class.lpfnWndProc = runtime_sound_window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = runtime_sound_window_class_name;
    runtime_sound_instance = instance;
    if(runtime_sound_class_api.register_class(&window_class) != 0)
    {
        runtime_sound_lifecycle_mutex = runtime_sound_class_api.create_mutex(nullptr, FALSE, nullptr);
        runtime_sound_mutex = runtime_sound_class_api.create_mutex(nullptr, FALSE, nullptr);
        runtime_sound_enabled = 1;
    }
}

uint32_t runtime_wave_formats_equal(const WAVEFORMATEX *left, const WAVEFORMATEX *right)
{
    if(left == nullptr || right == nullptr)
    {
        return 0;
    }
    return left->wBitsPerSample == right->wBitsPerSample && left->wFormatTag == right->wFormatTag && left->nChannels == right->nChannels && left->nSamplesPerSec == right->nSamplesPerSec;
}

uint32_t calculate_runtime_wave_conversion(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, uint16_t *conversion_flags)
{
    uint16_t flags = 0;
    if(source == nullptr || destination == nullptr)
    {
        return 0;
    }
    if(source->wBitsPerSample > 15 && destination->wBitsPerSample < 9)
    {
        flags = 0x1000;
    }
    if(source->wBitsPerSample < 9 && destination->wBitsPerSample > 15)
    {
        flags |= 0x2000;
    }
    if(source->nChannels > 1)
    {
        if(destination->nChannels < 2)
        {
            flags = static_cast<uint16_t>((flags & 0xff00) | 1);
        }
    }
    else if(destination->nChannels > 1)
    {
        flags |= 0x80;
    }
    const uint32_t source_rate = source->nSamplesPerSec;
    const uint32_t destination_rate = destination->nSamplesPerSec;
    uint8_t low_flags = static_cast<uint8_t>(flags);
    if(source_rate <= destination_rate)
    {
        if(destination_rate > source_rate)
        {
            const uint32_t ratio = destination_rate / source_rate;
            if(ratio * source_rate != destination_rate || (ratio != 2 && ratio != 4))
            {
                return 0;
            }
            if(low_flags == 0)
            {
                low_flags = ratio == 2 ? 0x80 : 0x40;
            }
            else if(ratio == 2)
            {
                low_flags >>= 1;
            }
            else
            {
                low_flags = static_cast<uint8_t>((low_flags >> 2) | (low_flags << 7));
            }
            flags = static_cast<uint16_t>((flags & 0xff00) | low_flags);
        }
        *conversion_flags = flags;
        return 1;
    }
    const uint32_t ratio = source_rate / destination_rate;
    if(ratio * destination_rate != source_rate || (ratio != 2 && ratio != 4))
    {
        return 0;
    }
    if(low_flags == 0)
    {
        low_flags = ratio == 2 ? 1 : 2;
    }
    else if(ratio == 2)
    {
        low_flags = static_cast<uint8_t>(low_flags << 1);
    }
    else
    {
        low_flags = static_cast<uint8_t>((low_flags << 2) | (low_flags >> 7));
    }
    flags = static_cast<uint16_t>((flags & 0xff00) | low_flags);
    *conversion_flags = flags;
    return 1;
}

void cleanup_runtime_sound_format_buffer()
{
    if(runtime_sound_format_buffer != nullptr)
    {
        runtime_sound_base_state = 0;
        runtime_sound_format_cleanup_api.heap_free(runtime_sound_format_cleanup_api.get_process_heap(), 0, runtime_sound_format_buffer);
        runtime_sound_format_buffer = nullptr;
    }
}


} // namespace gag
