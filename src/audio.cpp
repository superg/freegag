#include "audio.h"
#include <SDL3/SDL.h>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <vector>
#include "runtime_clock.h"

namespace gag
{

namespace
{

std::mutex runtime_sound_mutex;
std::mutex runtime_sound_lifecycle_mutex;
std::condition_variable_any runtime_silent_transport_condition;
std::array<RuntimeSoundSlot, 1024> runtime_sound_slots{};
std::array<std::vector<uint8_t>, 2> runtime_sound_outputs;
RuntimePcmFormat runtime_sound_output_format{};
SDL_AudioStream *runtime_sound_stream;
std::jthread runtime_silent_transport_thread;
uint32_t runtime_sound_mixing_suppressed;
uint32_t runtime_sound_base_state;
std::atomic_uint32_t runtime_sound_output_initialized;
uint32_t runtime_sound_mixer_data_size = 0x600;
uint32_t runtime_sound_maximum_handle;
uint32_t runtime_sound_output_ready;
uint32_t runtime_sound_output_index;
std::atomic_int32_t runtime_sound_enabled;
bool runtime_sound_sdl_initialized;
void (*runtime_sound_mixer)(uint32_t marker);

void stop_runtime_sound_transport();

uint32_t mix_next_runtime_sound_block()
{
    std::lock_guard lock(runtime_sound_mutex);
    const uint32_t output_index = runtime_sound_output_index;
    runtime_sound_mixer(runtime_milliseconds());
    runtime_sound_output_index ^= 1;
    return output_index;
}

void SDLCALL provide_runtime_sound_data(void *, SDL_AudioStream *stream, int additional_amount, int)
{
    while(additional_amount > 0 && runtime_sound_output_initialized != 0)
    {
        std::vector<uint8_t> &output = runtime_sound_outputs[mix_next_runtime_sound_block()];
        if(!SDL_PutAudioStreamData(stream, output.data(), static_cast<int>(output.size())))
        {
            return;
        }
        additional_amount -= static_cast<int>(output.size());
    }
}

void run_silent_runtime_sound_transport(std::stop_token stop_token)
{
    const auto block_duration = std::chrono::duration<double>(static_cast<double>(runtime_sound_mixer_data_size) / runtime_sound_output_format.average_bytes_per_second);
    auto deadline = std::chrono::steady_clock::now();
    while(!stop_token.stop_requested())
    {
        mix_next_runtime_sound_block();
        deadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(block_duration);
        std::mutex wait_mutex;
        std::unique_lock wait_lock(wait_mutex);
        runtime_silent_transport_condition.wait_until(wait_lock, stop_token, deadline, [] { return false; });
    }
}

bool configure_runtime_sound_mixer(const RuntimePcmFormat *format)
{
    if(format == nullptr || format->format_tag != 1 || (format->channel_count != 1 && format->channel_count != 2) || (format->bits_per_sample != 8 && format->bits_per_sample != 16)
        || format->samples_per_second < 11000)
    {
        return false;
    }

    runtime_sound_output_format = *format;
    runtime_sound_output_format.block_alignment = static_cast<uint16_t>(format->channel_count * format->bits_per_sample / 8);
    runtime_sound_output_format.average_bytes_per_second = runtime_sound_output_format.block_alignment * format->samples_per_second;
    runtime_sound_mixer_data_size = runtime_sound_output_format.block_alignment * (format->samples_per_second / 11000) * 0x800;
    if(runtime_sound_mixer_data_size == 0)
    {
        return false;
    }
    for(std::vector<uint8_t> &output : runtime_sound_outputs)
    {
        output.assign(runtime_sound_mixer_data_size, format->bits_per_sample == 8 ? 0x80 : 0);
    }
    runtime_sound_output_index = 0;
    if(format->bits_per_sample == 8)
    {
        runtime_sound_mixer = format->channel_count == 1 ? mix_runtime_sound_8bit_mono : mix_runtime_sound_8bit_stereo;
    }
    else
    {
        runtime_sound_mixer = format->channel_count == 1 ? mix_runtime_sound_16bit_mono : mix_runtime_sound_16bit_stereo;
    }
    return true;
}

bool start_runtime_sound_transport(const RuntimePcmFormat *format)
{
    stop_runtime_sound_transport();
    {
        std::lock_guard mixer_lock(runtime_sound_mutex);
        if(!configure_runtime_sound_mixer(format))
        {
            return false;
        }
    }

    if(runtime_sound_sdl_initialized)
    {
        SDL_AudioSpec specification{};
        specification.format = format->bits_per_sample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16;
        specification.channels = static_cast<int>(format->channel_count);
        specification.freq = static_cast<int>(format->samples_per_second);
        runtime_sound_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification, provide_runtime_sound_data, nullptr);
        if(runtime_sound_stream != nullptr)
        {
            runtime_sound_output_initialized = 1;
            runtime_sound_output_ready = 1;
            if(SDL_ResumeAudioStreamDevice(runtime_sound_stream))
            {
                return true;
            }
            runtime_sound_output_initialized = 0;
            runtime_sound_output_ready = 0;
            SDL_DestroyAudioStream(runtime_sound_stream);
            runtime_sound_stream = nullptr;
        }
    }

    runtime_sound_output_initialized = 1;
    runtime_sound_output_ready = 1;
    runtime_silent_transport_thread = std::jthread(run_silent_runtime_sound_transport);
    return true;
}

void stop_runtime_sound_transport()
{
    runtime_sound_output_initialized = 0;
    runtime_sound_output_ready = 0;
    if(runtime_sound_stream != nullptr)
    {
        SDL_DestroyAudioStream(runtime_sound_stream);
        runtime_sound_stream = nullptr;
    }
    if(runtime_silent_transport_thread.joinable())
    {
        runtime_silent_transport_thread.request_stop();
        runtime_silent_transport_condition.notify_all();
        runtime_silent_transport_thread.join();
    }
}

void release_runtime_sound_buffers(RuntimeSoundSlot &slot)
{
    RuntimeSoundBufferNode *buffer = slot.buffers;
    while(buffer != nullptr)
    {
        RuntimeSoundBufferNode *next = buffer->next;
        delete buffer;
        buffer = next;
    }
    slot.buffers = nullptr;
}

} // namespace

void destroy_runtime_sound_handle(uint32_t handle)
{
    if(runtime_sound_enabled == 0)
    {
        return;
    }
    std::lock_guard lock(runtime_sound_mutex);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            slot->active = 0;
            release_runtime_sound_buffers(*slot);
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
                    } while(runtime_sound_slots.data() < slot);
                }
            }
            return;
        }
    }
}

uint32_t create_runtime_sound_handle(const RuntimePcmFormat *source_format)
{
    if(runtime_sound_enabled == 0 || source_format == nullptr)
    {
        return 0;
    }
    uint16_t conversion_flags = 0;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    if(runtime_sound_output_ready == 0 && !start_runtime_sound_transport(source_format))
    {
        return 0;
    }
    std::unique_lock mixer_lock(runtime_sound_mutex);
    if(runtime_pcm_formats_equal(&runtime_sound_output_format, source_format) == 0)
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
            mixer_lock.unlock();
            if(!start_runtime_sound_transport(source_format))
            {
                return 0;
            }
            mixer_lock.lock();
            runtime_sound_maximum_handle = 0;
        }
        else if(calculate_runtime_pcm_conversion(&runtime_sound_output_format, source_format, &conversion_flags) == 0)
        {
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
        return handle;
    }
    return 0xffffffff;
}

uint32_t queue_runtime_sound_data(uint32_t handle, void *data, uint32_t size, int32_t replace)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lock(runtime_sound_mutex);
    if(handle <= runtime_sound_maximum_handle && handle != 0)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            auto *node = new (std::nothrow) RuntimeSoundBufferNode{};
            if(node == nullptr)
            {
                return 0;
            }
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
                    delete tail;
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
            return 1;
        }
    }
    return 0;
}

uint32_t start_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lock(runtime_sound_mutex);
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(reset_timing != 0)
        {
            slot->schedule_state = 0;
        }
        slot->playing = 1;
        return 1;
    }
    return 0;
}

uint32_t stop_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lock(runtime_sound_mutex);
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
        return 1;
    }
    return 0;
}

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value)
{
    if(handle != 0 && handle <= runtime_sound_maximum_handle)
    {
        std::lock_guard lock(runtime_sound_mutex);
        RuntimeSoundSlot *slot = &runtime_sound_slots[handle];
        if(slot->active != 0)
        {
            slot->loop_value_1 = value;
            slot->loop_value_2 = value;
        }
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
    std::lock_guard lock(runtime_sound_mutex);
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
            const uint64_t blocks = (static_cast<uint64_t>(runtime_sound_output_format.average_bytes_per_second * duration_ms) / 1000) / runtime_sound_mixer_data_size;
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
        return 1;
    }
    return 0;
}

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lock(runtime_sound_mutex);
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
            const uint64_t blocks = (static_cast<uint64_t>(runtime_sound_output_format.average_bytes_per_second * duration_ms) / 1000) / runtime_sound_mixer_data_size;
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
        return 1;
    }
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
    std::lock_guard lock(runtime_sound_mutex);
    if(handle <= runtime_sound_maximum_handle && handle != 0)
    {
        runtime_sound_slots[handle].volume = volume;
        return 1;
    }
    return 0;
}

uint32_t shutdown_runtime_sound()
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    stop_runtime_sound_transport();
    {
        std::lock_guard mixer_lock(runtime_sound_mutex);
        for(uint32_t handle = 1; handle <= runtime_sound_maximum_handle; ++handle)
        {
            RuntimeSoundSlot &slot = runtime_sound_slots[handle];
            if(slot.active != 0)
            {
                release_runtime_sound_buffers(slot);
                slot = {};
            }
        }
        runtime_sound_maximum_handle = 0;
        runtime_sound_base_state = 0;
        runtime_sound_mixing_suppressed = 0;
    }
    runtime_sound_enabled = 0;
    if(runtime_sound_sdl_initialized)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        runtime_sound_sdl_initialized = false;
    }
    return 1;
}

void toggle_runtime_sound_state()
{
    std::lock_guard lock(runtime_sound_mutex);
    runtime_sound_base_state = ~runtime_sound_base_state;
}

uint32_t pause_runtime_sound_output(int32_t close_output)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    {
        std::lock_guard mixer_lock(runtime_sound_mutex);
        runtime_sound_mixing_suppressed = 1;
    }
    if(close_output != 0)
    {
        stop_runtime_sound_transport();
    }
    return 1;
}

uint32_t resume_runtime_sound_output()
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    {
        std::lock_guard mixer_lock(runtime_sound_mutex);
        runtime_sound_mixing_suppressed = 0;
    }
    if(runtime_sound_output_initialized == 0 && runtime_sound_output_ready == 0)
    {
        return start_runtime_sound_transport(&runtime_sound_output_format) ? 1 : 0;
    }
    return 1;
}

uint32_t ensure_runtime_sound_ready(const RuntimePcmFormat *format, uint32_t)
{
    if(runtime_sound_enabled == 0)
    {
        return 0;
    }
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    if(runtime_sound_output_ready == 0)
    {
        runtime_sound_maximum_handle = 0;
        return start_runtime_sound_transport(format) ? 1 : 0;
    }
    return 1;
}

void initialize_runtime_sound()
{
    if(runtime_sound_enabled != 0)
    {
        return;
    }
    runtime_sound_sdl_initialized = SDL_InitSubSystem(SDL_INIT_AUDIO);
    runtime_sound_enabled = 1;
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
    uint8_t *output = runtime_sound_outputs[runtime_sound_output_index].data();
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
                    delete node;
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

uint32_t runtime_pcm_formats_equal(const RuntimePcmFormat *left, const RuntimePcmFormat *right)
{
    if(left == nullptr || right == nullptr)
    {
        return 0;
    }
    return left->bits_per_sample == right->bits_per_sample && left->format_tag == right->format_tag && left->channel_count == right->channel_count
        && left->samples_per_second == right->samples_per_second;
}

uint32_t calculate_runtime_pcm_conversion(const RuntimePcmFormat *source, const RuntimePcmFormat *destination, uint16_t *conversion_flags)
{
    uint16_t flags = 0;
    if(source == nullptr || destination == nullptr)
    {
        return 0;
    }
    if(source->bits_per_sample > 15 && destination->bits_per_sample < 9)
    {
        flags = 0x1000;
    }
    if(source->bits_per_sample < 9 && destination->bits_per_sample > 15)
    {
        flags |= 0x2000;
    }
    if(source->channel_count > 1)
    {
        if(destination->channel_count < 2)
        {
            flags = static_cast<uint16_t>((flags & 0xff00) | 1);
        }
    }
    else if(destination->channel_count > 1)
    {
        flags |= 0x80;
    }
    const uint32_t source_rate = source->samples_per_second;
    const uint32_t destination_rate = destination->samples_per_second;
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

} // namespace gag
