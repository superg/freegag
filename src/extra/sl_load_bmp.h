#pragma once

#include <cstdint>
#include <utility>



namespace freegag
{

class RuntimeHeap;

std::pair<void *, uint32_t> synthesize_sl_load_bmp(RuntimeHeap *heap);

}
