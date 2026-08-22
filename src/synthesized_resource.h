#pragma once

#include <stdint.h>
#include <vector>

namespace gag
{

struct CdfArchive;

struct SynthesizedResourceSourceApi
{
    uint32_t (*get_size)(CdfArchive *archive, uint8_t selector, const char *name);
    int (*read)(CdfArchive *archive, uint8_t selector, const char *name, void *destination);
};

struct SynthesizedResource
{
    std::vector<uint8_t> data;
    uint32_t resource_type;
};

uint32_t get_synthesized_resource_type(const char *name);
bool synthesize_resource(CdfArchive *archive, const char *name, const SynthesizedResourceSourceApi &source_api, SynthesizedResource *resource);

} // namespace gag
