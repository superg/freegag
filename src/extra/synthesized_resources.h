#pragma once

#include <cstdint>
#include <utility>



namespace freegag
{

class RuntimeHeap;

uint32_t get_synthesized_resource_type(const char *name);
std::pair<void *, uint32_t> synthesize_resource(RuntimeHeap *heap, const char *name);

}
