#pragma once

#include <filesystem>



namespace freegag
{

struct ApplicationFileRootSelection
{
    std::filesystem::path directory;
    std::filesystem::path archive;
    bool gary;
};

bool select_application_file_root(const std::filesystem::path &executable_directory, const std::filesystem::path &working_directory, ApplicationFileRootSelection *selection);

bool select_application_file_root(const std::filesystem::path &directory, ApplicationFileRootSelection *selection);

}
