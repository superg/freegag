#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace xtet
{

constexpr std::size_t kRuntimeTableCount = 20;

class RuntimeTables
{
public:
    bool initialize(std::size_t slot_count);
    void clear();
    std::size_t slotCount() const;
    const std::array<std::vector<void *>, kRuntimeTableCount> &tables() const;
    bool set(std::size_t table_index, std::size_t slot_index, void *value);
    void clearValue(const void *value);

private:
    std::array<std::vector<void *>, kRuntimeTableCount> tables_;
    std::size_t slot_count_{};
};

} // namespace xtet
