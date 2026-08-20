#pragma once

#include <array>
#include <stddef.h>
#include <vector>

namespace xtet
{

constexpr size_t kRuntimeTableCount = 20;

class RuntimeTables
{
public:
    bool initialize(size_t slot_count);
    void clear();
    size_t slotCount() const;
    const std::array<std::vector<void *>, kRuntimeTableCount> &tables() const;
    bool set(size_t table_index, size_t slot_index, void *value);
    void clearValue(const void *value);

private:
    std::array<std::vector<void *>, kRuntimeTableCount> tables_;
    size_t slot_count_{};
};

} // namespace xtet
