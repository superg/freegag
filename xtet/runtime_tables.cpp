#include "runtime_tables.h"
#include <new>
#include <utility>

namespace xtet
{

bool RuntimeTables::initialize(std::size_t slot_count)
{
    clear();
    try
    {
        std::array<std::vector<void *>, kRuntimeTableCount> new_tables;
        for(std::vector<void *> &table : new_tables)
            table.assign(slot_count, nullptr);
        tables_ = std::move(new_tables);
        slot_count_ = slot_count;
        return true;
    }
    catch(const std::bad_alloc &)
    {
        clear();
        return false;
    }
}

void RuntimeTables::clear()
{
    for(std::vector<void *> &table : tables_)
        table.clear();
    slot_count_ = 0;
}

std::size_t RuntimeTables::slotCount() const
{
    return slot_count_;
}

const std::array<std::vector<void *>, kRuntimeTableCount> &RuntimeTables::tables() const
{
    return tables_;
}

bool RuntimeTables::set(std::size_t table_index, std::size_t slot_index, void *value)
{
    if(table_index >= tables_.size() || slot_index >= slot_count_)
        return false;
    tables_[table_index][slot_index] = value;
    return true;
}

void RuntimeTables::clearValue(const void *value)
{
    for(std::vector<void *> &table : tables_)
        for(void *&slot : table)
            if(slot == value)
                slot = nullptr;
}

} // namespace xtet
