#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "asset_decoders.h"
#include "scene_description.h"

namespace xtet
{

using SoundHandle = std::uint32_t;

struct AudioHostCallbacks
{
    std::function<SoundHandle(const PcmFormat *)> create;
    std::function<void(SoundHandle)> destroy;
    std::function<bool(SoundHandle, const void *, std::uint32_t, bool)> queue;
    std::function<bool(SoundHandle, bool)> stop;
    std::function<bool(SoundHandle, bool)> start;
};

class AudioCoordinator
{
public:
    bool initialize(const SceneDescription &scene, const std::map<std::string, WavePcm> &waves, const AudioHostCallbacks &callbacks);
    bool initializeLoopQueue();
    bool queueRandom(const std::string &link, std::uint32_t random_value);
    bool queueFirst(const std::string &link);
    bool setLoopPlaying(bool playing);
    void destroy();
    bool valid() const;

private:
    struct SoundGroup
    {
        SoundHandle handle{};
        std::vector<const WavePcm *> waves;
    };

    std::map<std::string, SoundGroup> groups_;
    AudioHostCallbacks callbacks_;
};

} // namespace xtet
