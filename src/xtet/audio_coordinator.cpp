#include "audio_coordinator.h"
#include <array>
#include <limits>

namespace xtet
{

namespace
{

constexpr std::array<const char *, 6> kSoundLinks{ "loop", "act", "stop", "level", "over", "win" };

bool formats_equal(const PcmFormat &first, const PcmFormat &second)
{
    return first.format_tag == second.format_tag && first.channel_count == second.channel_count && first.samples_per_second == second.samples_per_second
        && first.average_bytes_per_second == second.average_bytes_per_second && first.block_alignment == second.block_alignment && first.bits_per_sample == second.bits_per_sample;
}

} // namespace

bool AudioCoordinator::initialize(const SceneDescription &scene, const std::map<std::string, WavePcm> &waves, const AudioHostCallbacks &callbacks)
{
    destroy();
    if(!callbacks.create || !callbacks.destroy || !callbacks.queue || !callbacks.stop || !callbacks.start)
        return false;
    callbacks_ = callbacks;
    for(const char *link : kSoundLinks)
    {
        const std::vector<const SceneNode *> nodes = find_scene_links(scene, link);
        if(nodes.empty())
        {
            destroy();
            return false;
        }
        SoundGroup group;
        for(const SceneNode *node : nodes)
        {
            if(node == nullptr || node->type != SceneNodeType::wave)
            {
                destroy();
                return false;
            }
            const auto wave = waves.find(node->loaded_path);
            if(wave == waves.end() || wave->second.samples.empty() || wave->second.samples.size() > (std::numeric_limits<uint32_t>::max)()
                || (!group.waves.empty() && !formats_equal(group.waves[0]->format, wave->second.format)))
            {
                destroy();
                return false;
            }
            group.waves.push_back(&wave->second);
        }
        group.handle = callbacks_.create(&group.waves[0]->format);
        if(group.handle == 0)
        {
            destroy();
            return false;
        }
        groups_.emplace(link, std::move(group));
    }
    return true;
}

bool AudioCoordinator::initializeLoopQueue()
{
    const auto found = groups_.find("loop");
    if(found == groups_.end() || found->second.waves.size() != 8 || !callbacks_.stop(found->second.handle, false))
        return false;
    const SoundGroup &group = found->second;
    const auto queue_wave = [&](const WavePcm &wave, bool replace) { return callbacks_.queue(group.handle, wave.samples.data(), (uint32_t)wave.samples.size(), replace); };
    if(!queue_wave(*group.waves[0], true) || !queue_wave(*group.waves[0], false))
        return false;
    for(uint32_t pass = 0; pass < 300; ++pass)
        for(const WavePcm *wave : group.waves)
            if(!queue_wave(*wave, false))
                return false;
    return true;
}

bool AudioCoordinator::queueRandom(const std::string &link, uint32_t random_value)
{
    const auto found = groups_.find(link);
    if(found == groups_.end() || found->second.waves.empty())
        return false;
    const WavePcm &wave = *found->second.waves[random_value % found->second.waves.size()];
    return callbacks_.queue(found->second.handle, wave.samples.data(), (uint32_t)wave.samples.size(), true);
}

bool AudioCoordinator::queueFirst(const std::string &link)
{
    return queueRandom(link, 0);
}

bool AudioCoordinator::setLoopPlaying(bool playing)
{
    const auto found = groups_.find("loop");
    if(found == groups_.end())
        return false;
    return playing ? callbacks_.start(found->second.handle, false) : callbacks_.stop(found->second.handle, false);
}

void AudioCoordinator::destroy()
{
    if(callbacks_.destroy)
        for(const auto &entry : groups_)
            if(entry.second.handle != 0)
                callbacks_.destroy(entry.second.handle);
    groups_.clear();
    callbacks_ = {};
}

bool AudioCoordinator::valid() const
{
    return groups_.size() == kSoundLinks.size();
}

} // namespace xtet
