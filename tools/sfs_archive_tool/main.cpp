#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "portable_path.h"
#include "sfs_archive.h"

namespace sfs_archive_tool
{

bool ends_with_ascii_case_insensitive(std::string_view text, std::string_view suffix)
{
    if(text.size() < suffix.size())
        return false;
    text.remove_prefix(text.size() - suffix.size());
    for(size_t index = 0; index < suffix.size(); ++index)
    {
        const auto left = static_cast<unsigned char>(text[index]);
        const auto right = static_cast<unsigned char>(suffix[index]);
        if(std::tolower(left) != std::tolower(right))
            return false;
    }
    return true;
}

bool names_equal(std::string_view left, std::string_view right)
{
    if(left.size() != right.size())
        return false;
    return std::equal(left.begin(), left.end(), right.begin(),
        [](char left_character, char right_character) { return std::tolower(static_cast<unsigned char>(left_character)) == std::tolower(static_cast<unsigned char>(right_character)); });
}

bool is_reserved_windows_name(std::string_view name)
{
    const size_t dot = name.find('.');
    const std::string_view stem = name.substr(0, dot);
    const auto equals = [stem](std::string_view candidate)
    {
        return stem.size() == candidate.size()
            && std::equal(stem.begin(), stem.end(), candidate.begin(), candidate.end(),
                [](char left, char right) { return std::toupper(static_cast<unsigned char>(left)) == std::toupper(static_cast<unsigned char>(right)); });
    };
    if(equals("CON") || equals("PRN") || equals("AUX") || equals("NUL"))
        return true;
    if(stem.size() == 4 && stem[3] >= '1' && stem[3] <= '9')
    {
        const std::string prefix{ static_cast<char>(std::toupper(static_cast<unsigned char>(stem[0]))), static_cast<char>(std::toupper(static_cast<unsigned char>(stem[1]))),
            static_cast<char>(std::toupper(static_cast<unsigned char>(stem[2]))) };
        return prefix == "COM" || prefix == "LPT";
    }
    return false;
}

void validate_resource_name(std::string_view name)
{
    if(name.empty() || name.size() > 0xe3 || name == "." || name == ".." || name == "..." || name.back() == ' ' || name.back() == '.' || is_reserved_windows_name(name))
        throw std::runtime_error("unsafe resource filename: " + std::string(name));
    for(unsigned char character : name)
        if(character < 0x20 || character == 0x7f || std::string_view("<>:\"/\\|?*").find(static_cast<char>(character)) != std::string_view::npos)
            throw std::runtime_error("unsafe resource filename: " + std::string(name));
}

void validate_output_name(std::string_view name)
{
    if(name.empty() || name.size() > 255 || name == "." || name == ".." || name.back() == ' ' || name.back() == '.' || is_reserved_windows_name(name))
        throw std::runtime_error("unsafe output filename: " + std::string(name));
    for(unsigned char character : name)
        if(character < 0x20 || character == 0x7f || std::string_view("<>:\"/\\|?*").find(static_cast<char>(character)) != std::string_view::npos)
            throw std::runtime_error("unsafe output filename: " + std::string(name));
}

std::filesystem::path resolve_input_path(const std::filesystem::path &requested, std::string_view description)
{
    std::filesystem::path resolved;
    if(!freegag::resolve_existing_host_path_case_insensitive(requested, &resolved) || !std::filesystem::is_regular_file(resolved))
        throw std::runtime_error("could not open " + std::string(description) + ": " + requested.string());
    return resolved;
}

bool exists_case_insensitive(const std::filesystem::path &path, std::filesystem::path *resolved = nullptr)
{
    std::filesystem::path match;
    if(!freegag::resolve_existing_host_path_case_insensitive(path, &match))
        return false;
    if(resolved != nullptr)
        *resolved = std::move(match);
    return true;
}

class TemporaryOutput
{
public:
    explicit TemporaryOutput(std::filesystem::path destination)
        : destination_(std::move(destination))
    {
        for(uint32_t index = 0; index != 10000; ++index)
        {
            temporary_ = destination_.parent_path() / (".sfs_archive_tool." + std::to_string(index) + ".tmp");
            if(!exists_case_insensitive(temporary_))
                break;
            temporary_.clear();
        }
        if(temporary_.empty())
            throw std::runtime_error("could not allocate a temporary output filename");
        stream_.open(temporary_, std::ios::binary | std::ios::trunc);
        if(!stream_)
            throw std::runtime_error("could not create temporary file: " + temporary_.string());
    }

    ~TemporaryOutput()
    {
        stream_.close();
        if(!published_)
        {
            std::error_code error;
            std::filesystem::remove(temporary_, error);
        }
    }

    std::ofstream &stream()
    {
        return stream_;
    }

    void publish()
    {
        stream_.flush();
        if(!stream_)
            throw std::runtime_error("could not finish temporary file: " + temporary_.string());
        stream_.close();
        if(exists_case_insensitive(destination_))
            throw std::runtime_error("destination already exists: " + destination_.string());
        std::error_code error;
        std::filesystem::rename(temporary_, destination_, error);
        if(error)
            throw std::runtime_error("could not publish destination: " + destination_.string());
        published_ = true;
    }

private:
    std::filesystem::path destination_;
    std::filesystem::path temporary_;
    std::ofstream stream_;
    bool published_{};
};

void write_bytes(std::ofstream &stream, const void *data, size_t size)
{
    stream.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
    if(!stream)
        throw std::runtime_error("could not write output file");
}

std::vector<uint8_t> read_file(const std::filesystem::path &path, std::string_view description)
{
    const uintmax_t wide_size = std::filesystem::file_size(path);
    if(wide_size > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error(std::string(description) + " exceeds the SFS 32-bit size limit: " + path.string());
    std::vector<uint8_t> result(static_cast<size_t>(wide_size));
    std::ifstream stream(path, std::ios::binary);
    if(!stream)
        throw std::runtime_error("could not read " + std::string(description) + ": " + path.string());
    if(!result.empty())
        stream.read(reinterpret_cast<char *>(result.data()), static_cast<std::streamsize>(result.size()));
    if(!stream && !result.empty())
        throw std::runtime_error("could not read " + std::string(description) + ": " + path.string());
    return result;
}

bool file_matches(const std::filesystem::path &path, const std::vector<uint8_t> &expected)
{
    std::error_code error;
    if(std::filesystem::file_size(path, error) != expected.size() || error)
        return false;
    std::ifstream stream(path, std::ios::binary);
    if(!stream)
        return false;
    std::array<uint8_t, 0x10000> buffer{};
    size_t position = 0;
    while(position < expected.size())
    {
        const size_t count = std::min(buffer.size(), expected.size() - position);
        stream.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(count));
        if(!stream || !std::equal(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(count), expected.begin() + static_cast<std::ptrdiff_t>(position)))
            return false;
        position += count;
    }
    return true;
}

void write_new_file(const std::filesystem::path &destination, const std::vector<uint8_t> &data)
{
    TemporaryOutput output(destination);
    write_bytes(output.stream(), data.data(), data.size());
    output.publish();
}

std::string encode_name(std::string_view name)
{
    if(name.find(' ') == std::string_view::npos)
        return std::string(name);
    return '"' + std::string(name) + '"';
}

std::string encode_filetime(uint64_t value)
{
    static constexpr char digits[] = "0123456789abcdef";
    std::string result(16, '0');
    for(size_t index = 0; index < result.size(); ++index)
    {
        result[result.size() - index - 1] = digits[value & 0x0f];
        value >>= 4;
    }
    return result;
}

uint64_t parse_filetime(std::string_view value, uint32_t line_number)
{
    uint64_t result = 0;
    if(value.size() != 16)
        throw std::runtime_error("invalid FILETIME on manifest line " + std::to_string(line_number));
    const auto conversion = std::from_chars(value.data(), value.data() + value.size(), result, 16);
    if(conversion.ec != std::errc{} || conversion.ptr != value.data() + value.size())
        throw std::runtime_error("invalid FILETIME on manifest line " + std::to_string(line_number));
    return result;
}

std::string parse_header_comment(std::string_view line, uint32_t line_number)
{
    constexpr std::string_view prefix = "@header-comment ";
    if(!line.starts_with(prefix))
        throw std::runtime_error("unsupported manifest directive on line " + std::to_string(line_number));
    line.remove_prefix(prefix.size());
    if(line.size() < 2 || line.front() != '"' || line.back() != '"' || line.substr(1, line.size() - 2).find('"') != std::string_view::npos)
        throw std::runtime_error("malformed header comment on manifest line " + std::to_string(line_number));
    std::string result(line.substr(1, line.size() - 2));
    if(result.empty() || result.size() >= 0xde || std::any_of(result.begin(), result.end(), [](unsigned char character) { return character < 0x20 || character > 0x7e; }))
        throw std::runtime_error("invalid header comment on manifest line " + std::to_string(line_number));
    return result;
}

std::string_view take_field(std::string_view &fields, uint32_t line_number)
{
    if(fields.empty())
        throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
    const size_t separator = fields.find(' ');
    const std::string_view result = fields.substr(0, separator);
    if(separator == std::string_view::npos)
        fields = {};
    else
        fields.remove_prefix(separator + 1);
    if(result.empty() || (!fields.empty() && fields.front() == ' '))
        throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
    return result;
}

std::string take_name(std::string_view &fields, uint32_t line_number)
{
    if(fields.empty())
        throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
    if(fields.front() != '"')
        return std::string(take_field(fields, line_number));
    const size_t closing_quote = fields.find('"', 1);
    if(closing_quote == std::string_view::npos || closing_quote == 1 || closing_quote + 1 >= fields.size() || fields[closing_quote + 1] != ' ')
        throw std::runtime_error("malformed filename on manifest line " + std::to_string(line_number));
    std::string result(fields.substr(1, closing_quote - 1));
    fields.remove_prefix(closing_quote + 2);
    if(fields.empty() || fields.front() == ' ' || fields.find('"') != std::string_view::npos)
        throw std::runtime_error("malformed filename on manifest line " + std::to_string(line_number));
    return result;
}

struct ArchiveManifest
{
    xtet::SfsArchiveMetadata metadata;
    std::vector<xtet::SfsCatalogEntry> catalog;
};

ArchiveManifest read_manifest(const std::filesystem::path &path)
{
    std::ifstream stream(path);
    if(!stream)
        throw std::runtime_error("could not open manifest: " + path.string());
    ArchiveManifest manifest;
    std::string line;
    uint32_t line_number = 0;
    while(std::getline(stream, line))
    {
        ++line_number;
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        if(!line.empty() && line.front() == '@')
        {
            if(line_number != 1 || !manifest.metadata.header_comment.empty())
                throw std::runtime_error("misplaced manifest directive on line " + std::to_string(line_number));
            manifest.metadata.header_comment = parse_header_comment(line, line_number);
            continue;
        }
        std::string_view fields = line;
        xtet::SfsCatalogEntry entry;
        entry.name = take_name(fields, line_number);
        entry.creation_time = parse_filetime(take_field(fields, line_number), line_number);
        entry.access_time = parse_filetime(take_field(fields, line_number), line_number);
        entry.write_time = parse_filetime(take_field(fields, line_number), line_number);
        if(!fields.empty())
            throw std::runtime_error("extra fields on manifest line " + std::to_string(line_number));
        if(entry.name != "." && entry.name != "...")
        {
            validate_resource_name(entry.name);
            const std::filesystem::path resource_path = resolve_input_path(path.parent_path() / entry.name, "resource");
            entry.content = read_file(resource_path, "resource");
        }
        manifest.catalog.push_back(std::move(entry));
    }
    if(!stream.eof())
        throw std::runtime_error("could not read manifest");
    if(manifest.catalog.empty())
        throw std::runtime_error("manifest contains no entries");
    return manifest;
}

std::string create_manifest(const std::vector<xtet::SfsCatalogEntry> &catalog, const xtet::SfsArchiveMetadata &metadata)
{
    std::string manifest;
    if(!metadata.header_comment.empty())
    {
        manifest += "@header-comment \"";
        manifest += metadata.header_comment;
        manifest += "\"\n";
    }
    for(const xtet::SfsCatalogEntry &entry : catalog)
    {
        manifest += encode_name(entry.name);
        manifest += ' ';
        manifest += encode_filetime(entry.creation_time);
        manifest += ' ';
        manifest += encode_filetime(entry.access_time);
        manifest += ' ';
        manifest += encode_filetime(entry.write_time);
        manifest += '\n';
    }
    return manifest;
}

void extract_archive(const std::filesystem::path &requested_archive)
{
    const std::filesystem::path archive_path = resolve_input_path(requested_archive, "SFS archive");
    const std::filesystem::path manifest_path = std::filesystem::current_path() / (archive_path.filename().string() + ".manifest");
    if(exists_case_insensitive(manifest_path))
        throw std::runtime_error("manifest destination already exists: " + manifest_path.string());

    const std::vector<uint8_t> archive_bytes = read_file(archive_path, "SFS archive");
    xtet::SfsArchive archive;
    if(!archive.mount({ archive_bytes.data(), archive_bytes.size() }))
        throw std::runtime_error("only valid SFS200 archives are supported");
    std::vector<xtet::SfsCatalogEntry> catalog;
    xtet::SfsArchiveMetadata metadata;
    if(!archive.read_catalog(catalog, &metadata))
        throw std::runtime_error("SFS archive does not use the supported XTET layout");
    if(metadata.header_comment.find('"') != std::string::npos
        || std::any_of(metadata.header_comment.begin(), metadata.header_comment.end(), [](unsigned char character) { return character < 0x20 || character > 0x7e; }))
        throw std::runtime_error("SFS archive header comment cannot be represented in the manifest");
    std::vector<uint8_t> canonical_bytes;
    if(!xtet::build_sfs_archive(catalog, metadata, canonical_bytes) || canonical_bytes != archive_bytes)
        throw std::runtime_error("SFS archive does not use canonical XTET compression");

    std::vector<bool> existing_files(catalog.size());
    for(size_t index = 0; index < catalog.size(); ++index)
    {
        const xtet::SfsCatalogEntry &entry = catalog[index];
        if(entry.name == "." || entry.name == "...")
            continue;
        validate_resource_name(entry.name);
        if(names_equal(entry.name, manifest_path.filename().string()))
            throw std::runtime_error("resource filename collides with the manifest: " + entry.name);
        const std::filesystem::path destination = std::filesystem::current_path() / entry.name;
        std::filesystem::path existing;
        if(exists_case_insensitive(destination, &existing))
        {
            if(!std::filesystem::is_regular_file(existing) || !file_matches(existing, entry.content))
                throw std::runtime_error("resource destination contains different content: " + existing.string());
            existing_files[index] = true;
        }
    }

    for(size_t index = 0; index < catalog.size(); ++index)
        if(catalog[index].name != "." && catalog[index].name != "..." && !existing_files[index])
            write_new_file(std::filesystem::current_path() / catalog[index].name, catalog[index].content);

    const std::string manifest = create_manifest(catalog, metadata);
    TemporaryOutput output(manifest_path);
    write_bytes(output.stream(), manifest.data(), manifest.size());
    output.publish();
}

void build_archive(const std::filesystem::path &requested_manifest)
{
    const std::filesystem::path manifest_path = resolve_input_path(requested_manifest, "manifest");
    const std::string manifest_name = manifest_path.filename().string();
    constexpr std::string_view suffix = ".manifest";
    if(!ends_with_ascii_case_insensitive(manifest_name, suffix) || manifest_name.size() == suffix.size())
        throw std::runtime_error("manifest filename must end in .manifest");
    const std::string archive_name = manifest_name.substr(0, manifest_name.size() - suffix.size());
    validate_output_name(archive_name);

    const std::filesystem::path destination = std::filesystem::current_path() / archive_name;
    if(exists_case_insensitive(destination))
        throw std::runtime_error("archive destination already exists: " + destination.string());
    const ArchiveManifest manifest = read_manifest(manifest_path);
    std::vector<uint8_t> archive_bytes;
    if(!xtet::build_sfs_archive(manifest.catalog, manifest.metadata, archive_bytes))
        throw std::runtime_error("manifest does not describe a supported XTET SFS archive");

    TemporaryOutput output(destination);
    write_bytes(output.stream(), archive_bytes.data(), archive_bytes.size());
    output.publish();
}

void print_usage()
{
    std::cerr << "Usage:\n"
              << "  sfs_archive_tool extract <path-to-archive.sfs>\n"
              << "  sfs_archive_tool build <path-to-archive.sfs.manifest>\n";
}

} // namespace sfs_archive_tool

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        sfs_archive_tool::print_usage();
        return 1;
    }
    try
    {
        const std::string_view command = argv[1];
        if(command == "extract")
            sfs_archive_tool::extract_archive(argv[2]);
        else if(command == "build")
            sfs_archive_tool::build_archive(argv[2]);
        else
        {
            sfs_archive_tool::print_usage();
            return 1;
        }
    }
    catch(const std::exception &exception)
    {
        std::cerr << "sfs_archive_tool: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
