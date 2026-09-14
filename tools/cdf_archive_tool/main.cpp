#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "cdf_format.h"
#include "portable_path.h"

namespace cdf_archive_tool
{

using freegag::CDF_BLOCK_SIZE;
using freegag::CDF_ENTRY_COMPRESSED;
using freegag::CdfEntry;

constexpr uint32_t kHeaderSize = 19;
constexpr uint32_t kMaximumEntryCount = 2000;
constexpr uint32_t kCompressedBufferSize = 0x10000;

struct ManifestEntry
{
    std::string name;
    std::string physical_name;
    uint8_t flags;
    std::array<char, sizeof(CdfEntry::name)> serialized_name{};
};

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
    if(name.empty() || name.size() >= sizeof(CdfEntry::name) || name == "." || name == ".." || name.back() == ' ' || name.back() == '.' || is_reserved_windows_name(name))
        throw std::runtime_error("unsafe resource filename: " + std::string(name));
    for(unsigned char character : name)
        if(character < 0x20 || character == 0x7f || std::string_view("<>:\"/\\|?*").find(static_cast<char>(character)) != std::string_view::npos)
            throw std::runtime_error("unsafe resource filename: " + std::string(name));
}

void validate_physical_name(std::string_view name)
{
    if(name.empty() || name.size() > 255 || name == "." || name == ".." || name.back() == ' ' || name.back() == '.' || is_reserved_windows_name(name))
        throw std::runtime_error("unsafe physical filename: " + std::string(name));
    for(unsigned char character : name)
        if(character < 0x20 || character == 0x7f || std::string_view("<>:\"/\\|?*").find(static_cast<char>(character)) != std::string_view::npos)
            throw std::runtime_error("unsafe physical filename: " + std::string(name));
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
            temporary_ = destination_.parent_path() / (".cdf_archive_tool." + std::to_string(index) + ".tmp");
            if(!exists_case_insensitive(temporary_))
                break;
            temporary_.clear();
        }
        if(temporary_.empty())
            throw std::runtime_error("could not allocate a temporary output filename");
        stream_.open(temporary_, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
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

    std::fstream &stream()
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
    std::fstream stream_;
    bool published_{};
};

uint32_t checked_position(std::fstream &stream)
{
    const std::streampos position = stream.tellp();
    if(position == std::streampos(-1) || position > static_cast<std::streamoff>(std::numeric_limits<uint32_t>::max()))
        throw std::runtime_error("CDF archive exceeds its 32-bit format limit");
    return static_cast<uint32_t>(position);
}

void write_bytes(std::fstream &stream, const void *data, size_t size)
{
    stream.write(static_cast<const char *>(data), static_cast<std::streamsize>(size));
    if(!stream)
        throw std::runtime_error("could not write output file");
}

void seek_output(std::fstream &stream, uint32_t position)
{
    stream.clear();
    stream.seekp(position, std::ios::beg);
    if(!stream)
        throw std::runtime_error("could not seek output file");
}

class CdfReader
{
public:
    explicit CdfReader(const std::filesystem::path &path)
        : stream_(path, std::ios::binary)
    {
        if(!stream_)
            throw std::runtime_error("could not open CDF archive: " + path.string());
        const uintmax_t wide_size = std::filesystem::file_size(path);
        if(wide_size > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("CDF archive exceeds its 32-bit format limit");
        file_size_ = static_cast<uint32_t>(wide_size);
        if(file_size_ < kHeaderSize)
            throw std::runtime_error("truncated CDF archive");

        std::array<char, 7> signature{};
        read_at(0, signature.data(), signature.size());
        if(signature != std::to_array<char>({ 'C', 'D', 'F', '9', '7', 'a', '\0' }))
            throw std::runtime_error("only CDF97a resource archives are supported");
        uint32_t index_size{};
        uint32_t entry_count{};
        read_at(7, &index_size, sizeof(index_size));
        read_at(11, &entry_count, sizeof(entry_count));
        read_at(15, &index_data_size_, sizeof(index_data_size_));
        if(entry_count == 0 || entry_count > kMaximumEntryCount || index_size > file_size_ - kHeaderSize)
            throw std::runtime_error("invalid CDF header");
        index_offset_ = file_size_ - index_size;
        if(index_offset_ < kHeaderSize)
            throw std::runtime_error("invalid CDF index position");

        const uint64_t decoded_index_size = static_cast<uint64_t>(entry_count) * sizeof(CdfEntry);
        if(decoded_index_size > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("CDF index is too large");
        const std::vector<uint8_t> index = decode_container(index_offset_, index_size, static_cast<uint32_t>(decoded_index_size), true);
        entries_.resize(entry_count);
        std::memcpy(entries_.data(), index.data(), index.size());

        uint64_t total_size = 0;
        for(const CdfEntry &entry : entries_)
        {
            const auto terminator = std::find(std::begin(entry.name), std::end(entry.name), '\0');
            if(terminator == std::end(entry.name))
                throw std::runtime_error("CDF index contains an unterminated filename");
            validate_resource_name(std::string_view(entry.name, terminator));
            if(entry.file_offset < kHeaderSize || entry.file_offset > index_offset_ || entry.uncompressed_size > index_data_size_ - std::min(index_data_size_, static_cast<uint32_t>(total_size)))
                throw std::runtime_error("CDF index contains invalid entry bounds");
            total_size += entry.uncompressed_size;
            if(total_size > std::numeric_limits<uint32_t>::max())
                throw std::runtime_error("CDF uncompressed size overflows");
        }
        if(total_size != index_data_size_)
            throw std::runtime_error("CDF uncompressed size does not match its header");
    }

    const std::vector<CdfEntry> &entries() const
    {
        return entries_;
    }

    std::vector<uint8_t> read_entry(const CdfEntry &entry)
    {
        if((entry.flags & CDF_ENTRY_COMPRESSED) == 0)
        {
            if(entry.uncompressed_size > index_offset_ - entry.file_offset)
                throw std::runtime_error("uncompressed resource extends into the CDF index");
            std::vector<uint8_t> result(entry.uncompressed_size);
            read_at(entry.file_offset, result.data(), result.size());
            return result;
        }
        return decode_container(entry.file_offset, index_offset_ - entry.file_offset, entry.uncompressed_size, false);
    }

private:
    void read_at(uint32_t position, void *destination, size_t size)
    {
        if(position > file_size_ || size > file_size_ - position)
            throw std::runtime_error("CDF read is outside the archive");
        if(size == 0)
            return;
        stream_.clear();
        stream_.seekg(position, std::ios::beg);
        stream_.read(static_cast<char *>(destination), static_cast<std::streamsize>(size));
        if(!stream_)
            throw std::runtime_error("could not read CDF archive");
    }

    std::vector<uint8_t> decode_container(uint32_t position, uint32_t available_size, uint32_t decoded_size, bool require_exact_size)
    {
        const uint32_t chunk_count = decoded_size / CDF_BLOCK_SIZE + (decoded_size % CDF_BLOCK_SIZE != 0 ? 1 : 0);
        const uint64_t table_size_wide = static_cast<uint64_t>(chunk_count + 1) * sizeof(uint32_t);
        if(table_size_wide > available_size)
            throw std::runtime_error("truncated CDF chunk table");
        const uint32_t table_size = static_cast<uint32_t>(table_size_wide);
        std::vector<uint32_t> offsets(chunk_count + 1);
        read_at(position, offsets.data(), table_size);
        if(offsets.front() != table_size || offsets.back() > available_size || (require_exact_size && offsets.back() != available_size) || !std::is_sorted(offsets.begin(), offsets.end()))
            throw std::runtime_error("invalid CDF chunk table");

        std::vector<uint8_t> result(decoded_size);
        std::vector<uint8_t> compressed(kCompressedBufferSize);
        uint32_t output_position = 0;
        for(uint32_t chunk = 0; chunk < chunk_count; ++chunk)
        {
            const uint32_t compressed_size = offsets[chunk + 1] - offsets[chunk];
            if(compressed_size < 2 || compressed_size > compressed.size())
                throw std::runtime_error("invalid CDF chunk size");
            read_at(position + offsets[chunk], compressed.data(), compressed_size);
            const uint32_t output_size = std::min(CDF_BLOCK_SIZE, decoded_size - output_position);
            const int actual_size = freegag::decode_cdf_block(compressed.data(), compressed_size, result.data() + output_position, output_size);
            if(actual_size != static_cast<int>(output_size))
                throw std::runtime_error("could not decode CDF chunk");
            output_position += output_size;
        }
        return result;
    }

    std::ifstream stream_;
    uint32_t file_size_{};
    uint32_t index_offset_{};
    uint32_t index_data_size_{};
    std::vector<CdfEntry> entries_;
};

std::string entry_name(const CdfEntry &entry)
{
    return std::string(entry.name, std::find(std::begin(entry.name), std::end(entry.name), '\0'));
}

std::string manifest_name(const CdfEntry &entry)
{
    static constexpr char digits[] = "0123456789abcdef";
    const auto terminator = std::find(std::begin(entry.name), std::end(entry.name), '\0');
    std::string result(entry.name, terminator);
    const auto last_nonzero = std::find_if(std::rbegin(entry.name), std::rend(entry.name), [](char value) { return value != '\0'; });
    if(terminator != std::end(entry.name) && last_nonzero.base() > terminator + 1)
    {
        for(auto position = terminator; position != last_nonzero.base(); ++position)
        {
            const auto value = static_cast<uint8_t>(*position);
            result += "\\x";
            result += digits[value >> 4];
            result += digits[value & 0x0f];
        }
    }
    return result;
}

int hex_digit(char character)
{
    if(character >= '0' && character <= '9')
        return character - '0';
    if(character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    if(character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    return -1;
}

ManifestEntry decode_manifest_name(std::string_view encoded)
{
    ManifestEntry result;
    const size_t escape = encoded.find("\\x");
    const std::string_view visible_name = encoded.substr(0, escape);
    validate_resource_name(visible_name);
    result.name = visible_name;
    result.physical_name = result.name;
    std::copy(visible_name.begin(), visible_name.end(), result.serialized_name.begin());
    if(escape == std::string_view::npos)
        return result;
    if((encoded.size() - escape) % 4 != 0)
        throw std::runtime_error("malformed escaped CDF filename field");

    size_t output_position = visible_name.size();
    for(size_t position = escape; position < encoded.size(); position += 4)
    {
        if(encoded[position] != '\\' || encoded[position + 1] != 'x')
            throw std::runtime_error("malformed escaped CDF filename field");
        const int high = hex_digit(encoded[position + 2]);
        const int low = hex_digit(encoded[position + 3]);
        if(high < 0 || low < 0 || output_position >= result.serialized_name.size())
            throw std::runtime_error("malformed escaped CDF filename field");
        result.serialized_name[output_position++] = static_cast<char>((high << 4) | low);
    }
    if(result.serialized_name[visible_name.size()] != '\0')
        throw std::runtime_error("escaped CDF filename tail must begin with \\x00");
    return result;
}

bool is_decimal(std::string_view text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char character) { return std::isdigit(character) != 0; });
}

uint8_t parse_manifest_flag(std::string_view text, uint32_t line_number)
{
    unsigned value = 0;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto conversion = std::from_chars(begin, end, value, 10);
    if(conversion.ec != std::errc{} || conversion.ptr != end || value > std::numeric_limits<uint8_t>::max())
        throw std::runtime_error("invalid flag on manifest line " + std::to_string(line_number));
    return static_cast<uint8_t>(value);
}

std::string decode_physical_name(std::string_view &line, uint32_t line_number)
{
    if(line.empty())
        throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
    if(line.back() != '"' && line.find('"') != std::string_view::npos)
        throw std::runtime_error("malformed physical filename on manifest line " + std::to_string(line_number));

    std::string result;
    if(line.back() == '"')
    {
        const size_t opening_quote = line.rfind('"', line.size() - 2);
        if(opening_quote == std::string_view::npos || opening_quote == 0 || line[opening_quote - 1] != ' ' || opening_quote + 1 == line.size() - 1)
            throw std::runtime_error("malformed physical filename on manifest line " + std::to_string(line_number));
        result = line.substr(opening_quote + 1, line.size() - opening_quote - 2);
        line.remove_suffix(line.size() - opening_quote + 1);
    }
    else
    {
        const size_t separator = line.rfind(' ');
        if(separator == std::string_view::npos || separator == 0 || separator + 1 == line.size())
            throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
        const std::string_view final_field = line.substr(separator + 1);
        if(is_decimal(final_field))
            return {};
        result = final_field;
        line.remove_suffix(line.size() - separator);
    }
    validate_physical_name(result);
    return result;
}

std::string encode_physical_name(std::string_view name)
{
    if(name.find(' ') == std::string_view::npos && !is_decimal(name))
        return std::string(name);
    return '"' + std::string(name) + '"';
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

std::string extract_resource(const std::filesystem::path &directory, std::string_view name, const std::vector<uint8_t> &content)
{
    const std::filesystem::path destination = directory / name;
    std::filesystem::path existing;
    if(!exists_case_insensitive(destination, &existing))
    {
        write_new_file(destination, content);
        return {};
    }
    if(std::filesystem::is_regular_file(existing) && file_matches(existing, content))
        return {};

    for(uint64_t suffix = 1;; ++suffix)
    {
        const std::string physical_name = std::string(name) + "." + std::to_string(suffix);
        validate_physical_name(physical_name);
        const std::filesystem::path candidate = directory / physical_name;
        if(exists_case_insensitive(candidate, &existing))
        {
            if(std::filesystem::is_regular_file(existing) && file_matches(existing, content))
                return existing.filename().string();
        }
        else
        {
            write_new_file(candidate, content);
            return physical_name;
        }
        if(suffix == std::numeric_limits<uint64_t>::max())
            throw std::runtime_error("could not allocate a physical filename for resource: " + std::string(name));
    }
}

std::vector<uint8_t> read_file(const std::filesystem::path &path)
{
    const uintmax_t wide_size = std::filesystem::file_size(path);
    if(wide_size > std::numeric_limits<uint32_t>::max())
        throw std::runtime_error("resource exceeds the CDF 32-bit size limit: " + path.string());
    std::vector<uint8_t> result(static_cast<size_t>(wide_size));
    std::ifstream stream(path, std::ios::binary);
    if(!stream)
        throw std::runtime_error("could not read resource: " + path.string());
    if(!result.empty())
        stream.read(reinterpret_cast<char *>(result.data()), static_cast<std::streamsize>(result.size()));
    if(!stream && !result.empty())
        throw std::runtime_error("could not read resource: " + path.string());
    return result;
}

void extract_archive(const std::filesystem::path &requested_archive)
{
    const std::filesystem::path archive_path = resolve_input_path(requested_archive, "CDF archive");
    const std::filesystem::path manifest_path = std::filesystem::current_path() / (archive_path.filename().string() + ".manifest");
    if(exists_case_insensitive(manifest_path))
        throw std::runtime_error("manifest destination already exists: " + manifest_path.string());

    CdfReader archive(archive_path);
    for(const CdfEntry &entry : archive.entries())
        if(names_equal(entry_name(entry), manifest_path.filename().string()))
            throw std::runtime_error("resource filename collides with the manifest: " + entry_name(entry));

    std::string manifest;
    for(const CdfEntry &entry : archive.entries())
    {
        const std::string name = entry_name(entry);
        const std::vector<uint8_t> content = archive.read_entry(entry);
        const std::string physical_name = extract_resource(std::filesystem::current_path(), name, content);
        manifest += manifest_name(entry) + " " + std::to_string(entry.flags);
        if(!physical_name.empty())
            manifest += " " + encode_physical_name(physical_name);
        manifest += '\n';
    }

    TemporaryOutput output(manifest_path);
    write_bytes(output.stream(), manifest.data(), manifest.size());
    output.publish();
}

std::vector<ManifestEntry> read_manifest(const std::filesystem::path &path)
{
    std::ifstream stream(path);
    if(!stream)
        throw std::runtime_error("could not open manifest: " + path.string());
    std::vector<ManifestEntry> entries;
    std::string line;
    uint32_t line_number = 0;
    while(std::getline(stream, line))
    {
        ++line_number;
        if(!line.empty() && line.back() == '\r')
            line.pop_back();
        std::string_view fields = line;
        const std::string physical_name = decode_physical_name(fields, line_number);
        const size_t separator = fields.rfind(' ');
        if(separator == std::string_view::npos || separator == 0 || separator + 1 == fields.size())
            throw std::runtime_error("malformed manifest line " + std::to_string(line_number));
        ManifestEntry entry = decode_manifest_name(fields.substr(0, separator));
        entry.flags = parse_manifest_flag(fields.substr(separator + 1), line_number);
        if(!physical_name.empty())
            entry.physical_name = physical_name;
        entries.push_back(std::move(entry));
        if(entries.size() > kMaximumEntryCount)
            throw std::runtime_error("manifest contains too many resources");
    }
    if(!stream.eof())
        throw std::runtime_error("could not read manifest");
    if(entries.empty())
        throw std::runtime_error("manifest contains no resources");
    return entries;
}

uint32_t write_compressed_container(std::fstream &stream, freegag::LegacyGzipCompressionSession &compression, const uint8_t *source, uint32_t source_size)
{
    const uint32_t chunk_count = source_size / CDF_BLOCK_SIZE + (source_size % CDF_BLOCK_SIZE != 0 ? 1 : 0);
    const uint32_t table_size = (chunk_count + 1) * sizeof(uint32_t);
    std::vector<uint32_t> offsets(chunk_count + 1);
    offsets[0] = table_size;
    const uint32_t table_position = checked_position(stream);
    write_bytes(stream, offsets.data(), table_size);

    std::vector<uint8_t> compressed(kCompressedBufferSize);
    uint32_t source_position = 0;
    for(uint32_t chunk = 0; chunk < chunk_count; ++chunk)
    {
        const uint32_t block_size = std::min(CDF_BLOCK_SIZE, source_size - source_position);
        const uint32_t compressed_size = freegag::encode_cdf_block(compression, source + source_position, block_size, compressed.data(), static_cast<uint32_t>(compressed.size()));
        if(compressed_size == 0)
            throw std::runtime_error("could not encode CDF chunk");
        write_bytes(stream, compressed.data(), compressed_size);
        offsets[chunk + 1] = offsets[chunk] + compressed_size;
        source_position += block_size;
    }

    const uint32_t end_position = checked_position(stream);
    seek_output(stream, table_position);
    write_bytes(stream, offsets.data(), table_size);
    seek_output(stream, end_position);
    return end_position - table_position;
}

void build_archive(const std::filesystem::path &requested_manifest)
{
    const std::filesystem::path manifest_path = resolve_input_path(requested_manifest, "manifest");
    const std::string manifest_name = manifest_path.filename().string();
    constexpr std::string_view suffix = ".manifest";
    if(!ends_with_ascii_case_insensitive(manifest_name, suffix) || manifest_name.size() == suffix.size())
        throw std::runtime_error("manifest filename must end in .manifest");
    const std::string archive_name = manifest_name.substr(0, manifest_name.size() - suffix.size());
    validate_resource_name(archive_name);

    const std::filesystem::path destination = std::filesystem::current_path() / archive_name;
    if(exists_case_insensitive(destination))
        throw std::runtime_error("archive destination already exists: " + destination.string());
    const std::vector<ManifestEntry> manifest = read_manifest(manifest_path);

    TemporaryOutput output(destination);
    std::fstream &stream = output.stream();
    uint32_t index_size = 0;
    const uint32_t entry_count = static_cast<uint32_t>(manifest.size());
    uint32_t total_size = 0;
    write_bytes(stream, freegag::CDF_97A_SIGNATURE, sizeof(freegag::CDF_97A_SIGNATURE));
    write_bytes(stream, &index_size, sizeof(index_size));
    write_bytes(stream, &entry_count, sizeof(entry_count));
    write_bytes(stream, &total_size, sizeof(total_size));

    freegag::LegacyGzipCompressionSession compression;
    std::vector<CdfEntry> index(entry_count);
    for(uint32_t position = 0; position < entry_count; ++position)
    {
        const ManifestEntry &manifest_entry = manifest[position];
        const std::filesystem::path requested_resource = manifest_path.parent_path() / manifest_entry.physical_name;
        const std::filesystem::path resource_path = resolve_input_path(requested_resource, "resource");
        const std::vector<uint8_t> content = read_file(resource_path);
        if(content.size() > std::numeric_limits<uint32_t>::max() - total_size)
            throw std::runtime_error("CDF total uncompressed size overflows");

        CdfEntry &entry = index[position];
        entry.flags = manifest_entry.flags;
        std::memcpy(entry.name, manifest_entry.serialized_name.data(), manifest_entry.serialized_name.size());
        entry.file_offset = checked_position(stream);
        entry.uncompressed_size = static_cast<uint32_t>(content.size());
        if((entry.flags & CDF_ENTRY_COMPRESSED) != 0)
            write_compressed_container(stream, compression, content.data(), entry.uncompressed_size);
        else
            write_bytes(stream, content.data(), content.size());
        total_size += entry.uncompressed_size;
    }

    index_size = write_compressed_container(stream, compression, reinterpret_cast<const uint8_t *>(index.data()), static_cast<uint32_t>(index.size() * sizeof(CdfEntry)));
    seek_output(stream, 7);
    write_bytes(stream, &index_size, sizeof(index_size));
    write_bytes(stream, &entry_count, sizeof(entry_count));
    write_bytes(stream, &total_size, sizeof(total_size));
    output.publish();
}

void print_usage()
{
    std::cerr << "Usage:\n"
              << "  cdf_archive_tool extract <path-to-archive.cdf>\n"
              << "  cdf_archive_tool build <path-to-archive.cdf.manifest>\n";
}

}

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        cdf_archive_tool::print_usage();
        return 1;
    }
    try
    {
        const std::string_view command = argv[1];
        if(command == "extract")
            cdf_archive_tool::extract_archive(argv[2]);
        else if(command == "build")
            cdf_archive_tool::build_archive(argv[2]);
        else
        {
            cdf_archive_tool::print_usage();
            return 1;
        }
    }
    catch(const std::exception &exception)
    {
        std::cerr << "cdf_archive_tool: " << exception.what() << '\n';
        return 1;
    }
    return 0;
}
