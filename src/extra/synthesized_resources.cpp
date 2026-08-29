#include "synthesized_resources.h"
#include "gagboy_cfg.h"
#include "media_types.h"
#include "portable_string.h"
#include "saveload_cfg.h"
#include "sl_left_bmp.h"
#include "sl_load_bmp.h"
#include "sl_right_bmp.h"
#include "sl_save_bmp.h"



namespace freegag
{

using SynthesizedResourceBuilder = std::pair<void *, uint32_t> (*)(RuntimeHeap *heap);

struct SynthesizedResourceDefinition
{
    const char *name;
    uint32_t resource_type;
    SynthesizedResourceBuilder builder;
};

const char *file_name_from_path(const char *path)
{
    const char *name = path;
    for(const char *cursor = path; *cursor != '\0'; ++cursor)
        if(*cursor == '\\' || *cursor == '/')
            name = cursor + 1;
    return name;
}

constexpr SynthesizedResourceDefinition synthesized_resources[]{
    { "GAGBOY.CFG",   RUNTIME_MEDIA_DATA_CONFIGURATION, synthesize_gagboy_cfg   },
    { "SAVELOAD.CFG", RUNTIME_MEDIA_DATA_CONFIGURATION, synthesize_saveload_cfg },
    { "SL-LEFT.BMP",  RUNTIME_MEDIA_DATA_BITMAP,        synthesize_sl_left_bmp  },
    { "SL-LOAD.BMP",  RUNTIME_MEDIA_DATA_BITMAP,        synthesize_sl_load_bmp  },
    { "SL-RIGHT.BMP", RUNTIME_MEDIA_DATA_BITMAP,        synthesize_sl_right_bmp },
    { "SL-SAVE.BMP",  RUNTIME_MEDIA_DATA_BITMAP,        synthesize_sl_save_bmp  }
};

const SynthesizedResourceDefinition *find_definition(const char *path)
{
    if(path == nullptr)
        return nullptr;
    const char *name = file_name_from_path(path);
    for(const SynthesizedResourceDefinition &definition : synthesized_resources)
        if(compare_ascii_case_insensitive(name, definition.name) == 0)
            return &definition;
    return nullptr;
}

uint32_t get_synthesized_resource_type(const char *name)
{
    const SynthesizedResourceDefinition *definition = find_definition(name);
    return definition == nullptr ? RUNTIME_MEDIA_DATA_UNKNOWN : definition->resource_type;
}

std::pair<void *, uint32_t> synthesize_resource(RuntimeHeap *heap, const char *name)
{
    const SynthesizedResourceDefinition *definition = find_definition(name);
    if(heap == nullptr || definition == nullptr)
        return {};
    return definition->builder(heap);
}

}
