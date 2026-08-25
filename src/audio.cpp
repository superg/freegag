#include "audio.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "runtime_clock.h"

namespace freegag
{


constexpr uint32_t runtime_sound_slot_count = 1024;
constexpr auto runtime_sound_control_interval = std::chrono::milliseconds(5);
constexpr uint64_t nanoseconds_per_second = 1'000'000'000;

struct RuntimeSoundSegment
{
    const uint8_t *data;
    uint32_t size;
    uint32_t offset;
    uint32_t schedule_offset;
};

struct RuntimeSoundSlot
{
    std::mutex mutex;
    RuntimePcmFormat format{};
    std::deque<RuntimeSoundSegment> segments;
    std::vector<uint8_t> transfer_buffer;
    std::vector<uint8_t> partial_frame;
    SDL_AudioStream *stream{};
    uint32_t control_state{};
    uint32_t playback_marker{};
    uint32_t schedule_marker{};
    uint32_t loop_value{};
    uint32_t loop_remaining{};
    uint32_t transition_flags{};
    float gain{ 1.0f };
    float fade_start_gain{ 1.0f };
    float fade_target_gain{ 1.0f };
    std::chrono::steady_clock::time_point fade_start_time{};
    std::chrono::steady_clock::time_point fade_end_time{};
    std::chrono::steady_clock::time_point silent_update_time{ std::chrono::steady_clock::now() };
    uint64_t silent_fraction{};
    uint32_t silent_unaligned_bytes{};
    bool active{ true };
    bool callback_enabled{ true };
    bool bound{};
    bool stream_failed{};
    bool fade_active{};
};

std::mutex runtime_sound_registry_mutex;
std::mutex runtime_sound_lifecycle_mutex;
std::mutex runtime_sound_worker_wait_mutex;
std::condition_variable_any runtime_sound_worker_condition;
std::array<std::shared_ptr<RuntimeSoundSlot>, runtime_sound_slot_count> runtime_sound_slots;
std::jthread runtime_sound_control_thread;
SDL_AudioDeviceID runtime_sound_device;
std::atomic_bool runtime_sound_enabled;
bool runtime_sound_sdl_initialized;
bool runtime_sound_muted;
bool runtime_sound_output_suppressed;
std::atomic_bool runtime_sound_output_closed;

std::shared_ptr<RuntimeSoundSlot> find_runtime_sound_slot(uint32_t handle)
{
    if(handle == 0 || handle >= runtime_sound_slot_count)
        return {};
    std::lock_guard lock(runtime_sound_registry_mutex);
    return runtime_sound_slots[handle];
}

std::vector<std::shared_ptr<RuntimeSoundSlot>> snapshot_runtime_sound_slots()
{
    std::vector<std::shared_ptr<RuntimeSoundSlot>> slots;
    std::lock_guard lock(runtime_sound_registry_mutex);
    for(uint32_t handle = 1; handle < runtime_sound_slot_count; ++handle)
        if(runtime_sound_slots[handle] != nullptr)
            slots.push_back(runtime_sound_slots[handle]);
    return slots;
}

bool validate_runtime_pcm_format(const RuntimePcmFormat *format)
{
    if(format == nullptr || format->format_tag != 1 || (format->channel_count != 1 && format->channel_count != 2) || (format->bits_per_sample != 8 && format->bits_per_sample != 16)
        || format->samples_per_second < 11000)
    {
        return false;
    }
    return true;
}

SDL_AudioSpec make_runtime_sound_input_spec(const RuntimePcmFormat &format)
{
    SDL_AudioSpec specification{};
    specification.format = format.bits_per_sample == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    specification.channels = format.channel_count;
    specification.freq = static_cast<int>(format.samples_per_second);
    return specification;
}

bool runtime_sound_slot_should_advance(const RuntimeSoundSlot &slot)
{
    return slot.control_state == 0 || ((slot.transition_flags & 2) != 0 && slot.gain > 0.0f);
}

uint32_t runtime_sound_schedule_block_size(const RuntimeSoundSlot &slot)
{
    return slot.format.block_alignment * (slot.format.samples_per_second / 11000) * 0x800;
}

void complete_runtime_sound_segment(RuntimeSoundSlot &slot, uint32_t marker)
{
    RuntimeSoundSegment &segment = slot.segments.front();
    if(slot.loop_value == 0)
    {
        slot.segments.pop_front();
        slot.playback_marker = marker;
        return;
    }
    segment.offset = 0;
    if(slot.loop_value == RUNTIME_SOUND_LOOP_INFINITE || --slot.loop_remaining != 0)
        return;
    slot.loop_remaining = slot.loop_value;
    slot.control_state = 1;
    slot.playback_marker = marker;
}

void consume_runtime_sound_bytes(RuntimeSoundSlot &slot, size_t requested_bytes, uint32_t marker)
{
    const size_t block_alignment = slot.format.block_alignment;
    slot.transfer_buffer.clear();
    if(!slot.partial_frame.empty())
    {
        slot.transfer_buffer.insert(slot.transfer_buffer.end(), slot.partial_frame.begin(), slot.partial_frame.end());
        slot.partial_frame.clear();
    }
    while(slot.transfer_buffer.size() < requested_bytes && !slot.segments.empty() && runtime_sound_slot_should_advance(slot))
    {
        RuntimeSoundSegment &segment = slot.segments.front();
        const uint32_t schedule_block_size = runtime_sound_schedule_block_size(slot);
        if(slot.schedule_marker == 0 && (segment.offset == schedule_block_size || segment.schedule_offset == schedule_block_size))
            slot.schedule_marker = marker;
        const size_t remaining = segment.size - segment.offset;
        const size_t wanted = requested_bytes - slot.transfer_buffer.size();
        size_t copied = (std::min)(remaining, wanted);
        if(slot.schedule_marker == 0 && segment.schedule_offset < schedule_block_size)
            copied = (std::min)(copied, static_cast<size_t>(schedule_block_size - segment.schedule_offset));
        slot.transfer_buffer.insert(slot.transfer_buffer.end(), segment.data + segment.offset, segment.data + segment.offset + copied);
        segment.offset += static_cast<uint32_t>(copied);
        if(segment.schedule_offset <= schedule_block_size)
            segment.schedule_offset += static_cast<uint32_t>(copied);
        if(segment.offset == segment.size)
            complete_runtime_sound_segment(slot, marker);
    }
    const size_t complete_bytes = slot.transfer_buffer.size() - slot.transfer_buffer.size() % block_alignment;
    if(complete_bytes != slot.transfer_buffer.size())
    {
        slot.partial_frame.assign(slot.transfer_buffer.begin() + complete_bytes, slot.transfer_buffer.end());
        slot.transfer_buffer.resize(complete_bytes);
    }
}

void SDLCALL provide_runtime_sound_data(void *userdata, SDL_AudioStream *stream, int additional_amount, int)
{
    auto *slot = static_cast<RuntimeSoundSlot *>(userdata);
    if(additional_amount <= 0)
        return;
    std::vector<uint8_t> data;
    {
        std::lock_guard lock(slot->mutex);
        if(!slot->active || !slot->callback_enabled || !runtime_sound_slot_should_advance(*slot))
            return;
        const size_t alignment = slot->format.block_alignment;
        const size_t requested = (static_cast<size_t>(additional_amount) + alignment - 1) / alignment * alignment;
        consume_runtime_sound_bytes(*slot, requested, runtime_milliseconds());
        if(slot->transfer_buffer.size() > static_cast<size_t>((std::numeric_limits<int>::max)()))
        {
            slot->stream_failed = true;
            return;
        }
        data = slot->transfer_buffer;
    }
    if(!data.empty() && !SDL_PutAudioStreamData(stream, data.data(), static_cast<int>(data.size())))
    {
        std::lock_guard lock(slot->mutex);
        slot->stream_failed = true;
    }
}

void apply_runtime_sound_device_gain_locked()
{
    if(runtime_sound_device != 0)
        SDL_SetAudioDeviceGain(runtime_sound_device, runtime_sound_muted || runtime_sound_output_suppressed ? 0.0f : 1.0f);
}

void synchronize_runtime_sound_slot_binding_locked(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    SDL_AudioStream *stream = nullptr;
    bool bound = false;
    bool should_bind = false;
    {
        std::lock_guard lock(slot->mutex);
        stream = slot->stream;
        bound = slot->bound;
        should_bind = slot->active && !slot->stream_failed && !runtime_sound_output_closed && runtime_sound_device != 0 && runtime_sound_slot_should_advance(*slot);
    }
    bool new_bound = bound;
    if(stream != nullptr && should_bind && !bound)
    {
        new_bound = SDL_BindAudioStream(runtime_sound_device, stream);
        if(!new_bound)
        {
            std::lock_guard lock(slot->mutex);
            slot->stream_failed = true;
        }
    }
    else if(stream != nullptr && !should_bind && bound)
    {
        SDL_UnbindAudioStream(stream);
        new_bound = false;
    }
    if(new_bound != bound)
    {
        std::lock_guard lock(slot->mutex);
        if(slot->stream == stream)
        {
            slot->bound = new_bound;
            slot->silent_update_time = std::chrono::steady_clock::now();
            slot->silent_fraction = 0;
            slot->silent_unaligned_bytes = 0;
        }
    }
}

SDL_AudioStream *begin_runtime_sound_slot_transition_locked(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    SDL_AudioStream *stream = nullptr;
    bool bound = false;
    {
        std::lock_guard lock(slot->mutex);
        slot->callback_enabled = false;
        stream = slot->stream;
        bound = slot->bound;
    }
    if(stream != nullptr && bound)
    {
        SDL_UnbindAudioStream(stream);
        std::lock_guard lock(slot->mutex);
        if(slot->stream == stream)
            slot->bound = false;
    }
    if(stream != nullptr)
    {
        SDL_LockAudioStream(stream);
        SDL_UnlockAudioStream(stream);
    }
    return stream;
}

void ensure_runtime_sound_slot_stream_locked(const std::shared_ptr<RuntimeSoundSlot> &slot);

void end_runtime_sound_slot_transition_locked(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    {
        std::lock_guard lock(slot->mutex);
        slot->callback_enabled = true;
        slot->silent_update_time = std::chrono::steady_clock::now();
        slot->silent_fraction = 0;
        slot->silent_unaligned_bytes = 0;
    }
    ensure_runtime_sound_slot_stream_locked(slot);
    synchronize_runtime_sound_slot_binding_locked(slot);
}

void ensure_runtime_sound_slot_stream_locked(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    RuntimePcmFormat format{};
    float gain = 1.0f;
    {
        std::lock_guard lock(slot->mutex);
        if(!slot->active || slot->stream != nullptr || slot->stream_failed || runtime_sound_device == 0)
            return;
        format = slot->format;
        gain = slot->gain;
    }
    const SDL_AudioSpec input_specification = make_runtime_sound_input_spec(format);
    SDL_AudioStream *stream = SDL_CreateAudioStream(&input_specification, nullptr);
    bool ready = stream != nullptr;
    if(ready)
        ready = SDL_SetAudioStreamGetCallback(stream, provide_runtime_sound_data, slot.get()) && SDL_SetAudioStreamGain(stream, gain);
    if(!ready)
    {
        if(stream != nullptr)
            SDL_DestroyAudioStream(stream);
        std::lock_guard lock(slot->mutex);
        slot->stream_failed = true;
        return;
    }
    bool active = false;
    {
        std::lock_guard lock(slot->mutex);
        active = slot->active;
        if(active)
            slot->stream = stream;
    }
    if(!active)
    {
        SDL_DestroyAudioStream(stream);
        return;
    }
    synchronize_runtime_sound_slot_binding_locked(slot);
}

bool open_runtime_sound_device_locked()
{
    if(runtime_sound_output_closed || !runtime_sound_sdl_initialized)
        return false;
    bool opened_device = false;
    if(runtime_sound_device == 0)
    {
        SDL_AudioSpec preferred_specification{};
        preferred_specification.format = SDL_AUDIO_F32;
        preferred_specification.channels = 2;
        preferred_specification.freq = 44100;
        runtime_sound_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &preferred_specification);
        if(runtime_sound_device == 0)
            return false;
        if(!SDL_ResumeAudioDevice(runtime_sound_device))
        {
            SDL_CloseAudioDevice(runtime_sound_device);
            runtime_sound_device = 0;
            return false;
        }
        opened_device = true;
        apply_runtime_sound_device_gain_locked();
    }
    for(const std::shared_ptr<RuntimeSoundSlot> &slot : snapshot_runtime_sound_slots())
    {
        if(opened_device)
        {
            std::lock_guard lock(slot->mutex);
            if(slot->stream == nullptr)
                slot->stream_failed = false;
        }
        ensure_runtime_sound_slot_stream_locked(slot);
        synchronize_runtime_sound_slot_binding_locked(slot);
    }
    return true;
}

void close_runtime_sound_device_locked()
{
    if(runtime_sound_device == 0)
        return;
    for(const std::shared_ptr<RuntimeSoundSlot> &slot : snapshot_runtime_sound_slots())
    {
        SDL_AudioStream *stream = nullptr;
        bool bound = false;
        {
            std::lock_guard lock(slot->mutex);
            stream = slot->stream;
            bound = slot->bound;
        }
        if(stream != nullptr && bound)
        {
            SDL_UnbindAudioStream(stream);
            std::lock_guard lock(slot->mutex);
            if(slot->stream == stream)
                slot->bound = false;
        }
    }
    SDL_CloseAudioDevice(runtime_sound_device);
    runtime_sound_device = 0;
}

void update_runtime_sound_fade(RuntimeSoundSlot &slot, std::chrono::steady_clock::time_point current_time, bool &gain_changed, bool &binding_changed)
{
    if(!slot.fade_active)
        return;
    if(current_time >= slot.fade_end_time)
    {
        slot.gain = slot.fade_target_gain;
        slot.fade_active = false;
        if(slot.fade_target_gain == 0.0f)
        {
            slot.transition_flags &= ~2u;
            if(slot.playback_marker == 0)
                slot.playback_marker = runtime_milliseconds();
        }
        else
        {
            slot.transition_flags &= ~1u;
        }
        gain_changed = true;
        binding_changed = true;
        return;
    }
    const auto elapsed = current_time - slot.fade_start_time;
    const auto duration = slot.fade_end_time - slot.fade_start_time;
    const float progress = static_cast<float>(std::chrono::duration<double>(elapsed).count() / std::chrono::duration<double>(duration).count());
    slot.gain = slot.fade_start_gain + (slot.fade_target_gain - slot.fade_start_gain) * progress;
    gain_changed = true;
}

void advance_silent_runtime_sound_slot(RuntimeSoundSlot &slot, std::chrono::steady_clock::time_point current_time)
{
    const uint64_t elapsed_nanoseconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - slot.silent_update_time).count());
    slot.silent_update_time = current_time;
    const uint64_t byte_numerator = slot.silent_fraction + elapsed_nanoseconds * slot.format.average_bytes_per_second;
    const uint64_t elapsed_bytes = byte_numerator / nanoseconds_per_second;
    slot.silent_fraction = byte_numerator % nanoseconds_per_second;
    const uint64_t available_bytes = elapsed_bytes + slot.silent_unaligned_bytes;
    const uint64_t aligned_bytes = available_bytes - available_bytes % slot.format.block_alignment;
    slot.silent_unaligned_bytes = static_cast<uint32_t>(available_bytes - aligned_bytes);
    if(aligned_bytes != 0)
        consume_runtime_sound_bytes(slot, static_cast<size_t>(aligned_bytes), runtime_milliseconds());
}

void remove_failed_runtime_sound_stream(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    SDL_AudioStream *stream = nullptr;
    {
        std::lock_guard lock(slot->mutex);
        if(!slot->stream_failed || slot->stream == nullptr)
            return;
        slot->callback_enabled = false;
        stream = slot->stream;
    }
    SDL_DestroyAudioStream(stream);
    {
        std::lock_guard lock(slot->mutex);
        if(slot->stream == stream)
        {
            slot->stream = nullptr;
            slot->bound = false;
            slot->callback_enabled = true;
            slot->silent_update_time = std::chrono::steady_clock::now();
        }
    }
}

void run_runtime_sound_control(std::stop_token stop_token)
{
    while(!stop_token.stop_requested())
    {
        const auto current_time = std::chrono::steady_clock::now();
        for(const std::shared_ptr<RuntimeSoundSlot> &slot : snapshot_runtime_sound_slots())
        {
            bool gain_changed = false;
            bool binding_changed = false;
            bool stream_failed = false;
            SDL_AudioStream *stream = nullptr;
            float gain = 1.0f;
            {
                std::lock_guard lock(slot->mutex);
                if(!slot->active)
                    continue;
                update_runtime_sound_fade(*slot, current_time, gain_changed, binding_changed);
                stream = slot->stream;
                gain = slot->gain;
                stream_failed = slot->stream_failed;
                if(!runtime_sound_output_closed && runtime_sound_slot_should_advance(*slot) && !slot->bound)
                {
                    advance_silent_runtime_sound_slot(*slot, current_time);
                }
                else
                {
                    slot->silent_update_time = current_time;
                    slot->silent_fraction = 0;
                    slot->silent_unaligned_bytes = 0;
                }
            }
            if(stream_failed)
            {
                remove_failed_runtime_sound_stream(slot);
                continue;
            }
            if(gain_changed && stream != nullptr)
            {
                std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
                SDL_AudioStream *current_stream = nullptr;
                {
                    std::lock_guard lock(slot->mutex);
                    current_stream = slot->stream;
                }
                if(current_stream == stream)
                    SDL_SetAudioStreamGain(stream, gain);
            }
            if(binding_changed)
            {
                std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
                synchronize_runtime_sound_slot_binding_locked(slot);
            }
        }
        std::unique_lock wait_lock(runtime_sound_worker_wait_mutex);
        runtime_sound_worker_condition.wait_for(wait_lock, stop_token, runtime_sound_control_interval, [] { return false; });
    }
}

void synchronize_runtime_sound_slot_transport(const std::shared_ptr<RuntimeSoundSlot> &slot)
{
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    ensure_runtime_sound_slot_stream_locked(slot);
    synchronize_runtime_sound_slot_binding_locked(slot);
}

void replace_runtime_sound_segments(const std::shared_ptr<RuntimeSoundSlot> &slot, const uint8_t *data, uint32_t size)
{
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    SDL_AudioStream *stream = begin_runtime_sound_slot_transition_locked(slot);
    {
        std::lock_guard lock(slot->mutex);
        slot->segments.clear();
        slot->partial_frame.clear();
        slot->segments.push_back({ data, size, 0, 0 });
        slot->playback_marker = 0;
        slot->schedule_marker = 0;
    }
    if(stream != nullptr)
        SDL_ClearAudioStream(stream);
    end_runtime_sound_slot_transition_locked(slot);
}

void schedule_runtime_sound_fade(RuntimeSoundSlot &slot, float target_gain, int32_t duration_ms)
{
    const auto current_time = std::chrono::steady_clock::now();
    slot.fade_start_gain = slot.gain;
    slot.fade_target_gain = target_gain;
    slot.fade_start_time = current_time;
    slot.fade_end_time = current_time + std::chrono::milliseconds((std::max)(duration_ms, 0));
    slot.fade_active = duration_ms > 0 && slot.gain != target_gain;
    if(!slot.fade_active)
        slot.gain = target_gain;
}


void destroy_runtime_sound_handle(uint32_t handle)
{
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return;
    {
        std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
        SDL_AudioStream *stream = begin_runtime_sound_slot_transition_locked(slot);
        {
            std::lock_guard lock(slot->mutex);
            slot->active = false;
        }
        if(stream != nullptr)
            SDL_DestroyAudioStream(stream);
        {
            std::lock_guard lock(slot->mutex);
            slot->stream = nullptr;
            slot->bound = false;
            slot->segments.clear();
            slot->partial_frame.clear();
        }
    }
    std::lock_guard registry_lock(runtime_sound_registry_mutex);
    if(runtime_sound_slots[handle] == slot)
        runtime_sound_slots[handle].reset();
}

uint32_t create_runtime_sound_handle(const RuntimePcmFormat *source_format)
{
    if(!runtime_sound_enabled || !validate_runtime_pcm_format(source_format))
        return 0;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    uint32_t handle = 1;
    const std::shared_ptr<RuntimeSoundSlot> slot = std::make_shared<RuntimeSoundSlot>();
    slot->format = *source_format;
    slot->format.block_alignment = static_cast<uint16_t>(source_format->channel_count * source_format->bits_per_sample / 8);
    slot->format.average_bytes_per_second = slot->format.block_alignment * source_format->samples_per_second;
    {
        std::lock_guard registry_lock(runtime_sound_registry_mutex);
        while(handle < runtime_sound_slot_count && runtime_sound_slots[handle] != nullptr)
            ++handle;
        if(handle == runtime_sound_slot_count)
            return 0;
        runtime_sound_slots[handle] = slot;
    }
    ensure_runtime_sound_slot_stream_locked(slot);
    synchronize_runtime_sound_slot_binding_locked(slot);
    runtime_sound_worker_condition.notify_all();
    return handle;
}

uint32_t queue_runtime_sound_data(uint32_t handle, const void *data, uint32_t size, int32_t replace)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    if(replace != 0)
    {
        replace_runtime_sound_segments(slot, static_cast<const uint8_t *>(data), size);
    }
    else
    {
        std::lock_guard lock(slot->mutex);
        if(!slot->active)
            return 0;
        slot->segments.push_back({ static_cast<const uint8_t *>(data), size, 0, 0 });
        slot->playback_marker = 0;
    }
    runtime_sound_worker_condition.notify_all();
    return 1;
}

uint32_t pause_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    {
        std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
        begin_runtime_sound_slot_transition_locked(slot);
        {
            std::lock_guard lock(slot->mutex);
            if(reset_timing != 0)
                slot->schedule_marker = 0;
            slot->control_state = 1;
            if(!slot->segments.empty())
                slot->segments.front().schedule_offset = 0;
            if((slot->transition_flags & 2) == 0 && slot->playback_marker == 0)
                slot->playback_marker = runtime_milliseconds();
        }
        end_runtime_sound_slot_transition_locked(slot);
    }
    return 1;
}

uint32_t resume_runtime_sound(uint32_t handle, int32_t reset_timing)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    {
        std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
        begin_runtime_sound_slot_transition_locked(slot);
        {
            std::lock_guard lock(slot->mutex);
            if(slot->control_state != 0)
            {
                slot->control_state = 0;
                if(reset_timing != 0)
                    slot->playback_marker = 0;
            }
        }
        end_runtime_sound_slot_transition_locked(slot);
    }
    runtime_sound_worker_condition.notify_all();
    return 1;
}

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value)
{
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot != nullptr)
    {
        std::lock_guard lock(slot->mutex);
        slot->loop_value = value;
        slot->loop_remaining = value;
    }
}

uint32_t query_runtime_sound_status(uint32_t handle, RuntimeSoundStatus *status)
{
    if(status == nullptr)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
    {
        *status = {};
        return 0;
    }
    std::lock_guard lock(slot->mutex);
    status->control_state = slot->control_state;
    status->playback_marker = slot->playback_marker;
    status->schedule_marker = slot->schedule_marker;
    status->infinite_loop = slot->loop_value == RUNTIME_SOUND_LOOP_INFINITE;
    return 1;
}

uint32_t set_runtime_sound_playback_marker(uint32_t handle, uint32_t marker)
{
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    std::lock_guard lock(slot->mutex);
    slot->playback_marker = marker;
    return 1;
}

uint32_t set_runtime_sound_schedule_marker(uint32_t handle, uint32_t marker)
{
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    std::lock_guard lock(slot->mutex);
    slot->schedule_marker = marker;
    return 1;
}

uint32_t restart_runtime_sound_data(uint32_t handle)
{
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    {
        std::lock_guard lock(slot->mutex);
        if(slot->segments.empty())
            return 0;
    }
    SDL_AudioStream *stream = begin_runtime_sound_slot_transition_locked(slot);
    if(stream != nullptr)
        SDL_ClearAudioStream(stream);
    {
        std::lock_guard lock(slot->mutex);
        const RuntimeSoundSegment segment{ slot->segments.front().data, slot->segments.front().size, 0, 0 };
        slot->segments.clear();
        slot->segments.push_back(segment);
        slot->partial_frame.clear();
        slot->playback_marker = 0;
        slot->schedule_marker = 0;
        slot->control_state = 0;
    }
    end_runtime_sound_slot_transition_locked(slot);
    runtime_sound_worker_condition.notify_all();
    return 1;
}

uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    {
        std::lock_guard lock(slot->mutex);
        if(slot->control_state != 0)
        {
            if(reset_timing != 0)
                slot->playback_marker = 0;
            slot->transition_flags = (slot->transition_flags & ~2u) | 1;
            slot->control_state = 0;
            schedule_runtime_sound_fade(*slot, 1.0f, duration_ms);
            if(!slot->fade_active)
                slot->transition_flags &= ~1u;
        }
    }
    synchronize_runtime_sound_slot_transport(slot);
    runtime_sound_worker_condition.notify_all();
    return 1;
}

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    {
        std::lock_guard lock(slot->mutex);
        if(reset_timing != 0)
            slot->schedule_marker = 0;
        if(slot->control_state == 0)
        {
            slot->transition_flags = (slot->transition_flags & ~1u) | 2;
            slot->control_state = 1;
            schedule_runtime_sound_fade(*slot, 0.0f, duration_ms);
            if(!slot->fade_active)
                slot->transition_flags &= ~2u;
        }
    }
    synchronize_runtime_sound_slot_transport(slot);
    runtime_sound_worker_condition.notify_all();
    return 1;
}

uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume)
{
    if(!runtime_sound_enabled)
        return 0;
    const std::shared_ptr<RuntimeSoundSlot> slot = find_runtime_sound_slot(handle);
    if(slot == nullptr)
        return 0;
    const float gain = static_cast<float>((std::min)(volume, static_cast<uint8_t>(100))) / 100.0f;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    SDL_AudioStream *stream = nullptr;
    {
        std::lock_guard lock(slot->mutex);
        slot->gain = gain;
        if(slot->fade_active)
        {
            slot->fade_start_gain = gain;
            slot->fade_start_time = std::chrono::steady_clock::now();
        }
        stream = slot->stream;
    }
    if(stream != nullptr)
        SDL_SetAudioStreamGain(stream, gain);
    return 1;
}

uint32_t shutdown_runtime_sound()
{
    if(!runtime_sound_enabled.exchange(false))
        return 0;
    if(runtime_sound_control_thread.joinable())
    {
        runtime_sound_control_thread.request_stop();
        runtime_sound_worker_condition.notify_all();
        runtime_sound_control_thread.join();
    }
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    for(const std::shared_ptr<RuntimeSoundSlot> &slot : snapshot_runtime_sound_slots())
    {
        SDL_AudioStream *stream = nullptr;
        {
            std::lock_guard lock(slot->mutex);
            slot->active = false;
            slot->callback_enabled = false;
            stream = slot->stream;
        }
        if(stream != nullptr)
            SDL_DestroyAudioStream(stream);
    }
    {
        std::lock_guard registry_lock(runtime_sound_registry_mutex);
        runtime_sound_slots = {};
    }
    close_runtime_sound_device_locked();
    if(runtime_sound_sdl_initialized)
    {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        runtime_sound_sdl_initialized = false;
    }
    return 1;
}

void toggle_runtime_sound_state()
{
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    runtime_sound_muted = !runtime_sound_muted;
    apply_runtime_sound_device_gain_locked();
}

uint32_t pause_runtime_sound_output(int32_t close_output)
{
    if(!runtime_sound_enabled)
        return 0;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    runtime_sound_output_suppressed = true;
    if(close_output != 0)
    {
        runtime_sound_output_closed = true;
        close_runtime_sound_device_locked();
    }
    else
    {
        apply_runtime_sound_device_gain_locked();
    }
    return 1;
}

uint32_t resume_runtime_sound_output()
{
    if(!runtime_sound_enabled)
        return 0;
    std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
    runtime_sound_output_suppressed = false;
    runtime_sound_output_closed = false;
    open_runtime_sound_device_locked();
    apply_runtime_sound_device_gain_locked();
    runtime_sound_worker_condition.notify_all();
    return 1;
}

void initialize_runtime_sound()
{
    bool expected = false;
    if(!runtime_sound_enabled.compare_exchange_strong(expected, true))
        return;
    runtime_sound_muted = false;
    runtime_sound_output_suppressed = false;
    runtime_sound_output_closed = false;
    runtime_sound_sdl_initialized = SDL_InitSubSystem(SDL_INIT_AUDIO);
    {
        std::lock_guard lifecycle_lock(runtime_sound_lifecycle_mutex);
        open_runtime_sound_device_locked();
    }
    runtime_sound_control_thread = std::jthread(run_runtime_sound_control);
}

} // namespace freegag
