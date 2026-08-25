#pragma once

#include <stdint.h>
#include <vector>

namespace freegag
{

struct CdfArchive;

struct SynthesizedResource
{
    std::vector<uint8_t> data;
    uint32_t resource_type;
};

uint32_t get_synthesized_resource_type(const char *name);
bool synthesize_resource(CdfArchive *archive, const char *name, SynthesizedResource *resource);

} // namespace freegag
