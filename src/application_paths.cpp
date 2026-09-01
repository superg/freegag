#include "application_paths.h"
#include <system_error>
#include "portable_path.h"


namespace freegag
{

static bool find_application_archive(const std::filesystem::path &directory, const char *name, std::filesystem::path *archive)
{
    std::filesystem::path resolved;
    std::error_code error;
    if(directory.empty() || !resolve_existing_host_path_case_insensitive(directory / name, &resolved) || !std::filesystem::is_regular_file(resolved, error))
        return false;
    *archive = std::move(resolved);
    return true;
}

bool select_application_file_root(const std::filesystem::path &directory, ApplicationFileRootSelection *selection)
{
    std::filesystem::path archive;
    bool gary = find_application_archive(directory, "GARY.CDF", &archive);
    if(!gary && !find_application_archive(directory, "Gag01.cdf", &archive))
        return false;

    std::error_code error;
    std::filesystem::path absolute_directory = std::filesystem::absolute(directory, error);
    if(error)
        return false;
    selection->directory = absolute_directory.lexically_normal();
    selection->archive = std::filesystem::absolute(archive, error).lexically_normal();
    if(error)
        return false;
    selection->gary = gary;
    return true;
}

bool select_application_file_root(const std::filesystem::path &executable_directory, const std::filesystem::path &working_directory, ApplicationFileRootSelection *selection)
{
    if(selection == nullptr)
        return false;
    *selection = {};
    return select_application_file_root(executable_directory, selection) || select_application_file_root(working_directory, selection);
}

}
