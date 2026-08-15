#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include "inflate.h"
#include "typedefs.h"
#include "walkthrough.hh"



#define DEBUG
// #define EXTRACT_DUPLICATES
#define SIZEOF(arg__) (sizeof(arg__) / sizeof(arg__[0]))



using namespace std;
using namespace std::filesystem;



static const char *g_CDF_MAGIC = "CDF97a";

static const struct
{
    const char *extension;
    u32 type;
} g_FILE_TYPES[] = {
    { ".256", 1 },
    { ".BMP", 1 },
    { ".CFG", 4 },
    { ".CIN", 4 },
    { ".DAT", 0 },
    { ".FLC", 3 },
    { ".FLZ", 3 },
    { ".MOV", 3 },
    { ".MVZ", 3 },
    { ".RBM", 1 },
    { ".RMV", 3 },
    { ".RUS", 1 },
    { ".RWV", 2 },
    { ".TXT", 0 },
    { ".WAV", 2 }
};



#pragma pack(push, 1)
struct CdfHeader
{
    char magic[7];
    u32 index_size;
    u32 files_count;
    u32 size_total;
};


struct IndexEntry
{
    u8 type;
    char filename[35];
    u32 offset;
    u32 size_dec;
};
#pragma pack(pop)

static_assert(sizeof(CdfHeader) == 19, "CDF header layout changed");
static_assert(sizeof(IndexEntry) == 44, "CDF index-entry layout changed");



int cdf_extract(string a_out_path, string a_in_fn);
int cdf_pack(string a_out_fn, string a_in_path);
u8 *block_decode(u32 &, ifstream &, u32, u32);
void block_encode(ofstream &, u8 *, u32, bool);
void block_free(u8 *);
u32 decode(u8 *, u8 *, u32);
u8 *encode(u32 &, u8 *, u32, bool);
void file_list_cb(void *, path);
u32 file_get_type(string a_ext);
streampos file_get_size(ifstream &);



int main(int argc, char *argv[])
{
    bool show_usage = false;

    if(argc == 4)
    {
        string mode(argv[1]);
        if(mode == "x")
            cdf_extract(argv[3], argv[2]);
        else if(mode == "p")
            cdf_pack(argv[2], argv[3]);
        else
            show_usage = true;
    }
    else
        show_usage = true;

    if(show_usage)
    {
        cout << "usage: " << (argc ? argv[0] : "<program> ") << " <mode> <cdf-file> <directory>" << endl;
        cout << "\tmode: x - extract files, p - pack files" << endl;
        cout << "\tcdf-file: path to cdf-file" << endl;
        cout << "\tdirectory: archive files directory path" << endl;

        return 1;
    }

    return 0;
}


int cdf_extract(string a_out_path, string a_in_fn)
{
    ifstream iF(a_in_fn.c_str(), ios::binary);

    if(!iF.is_open())
    {
        cout << "error: unable to open file " << a_in_fn << endl;
        return 2;
    }

    path output_path(a_out_path);
    create_directories(output_path);

    u32 file_size = static_cast<u32>(file_get_size(iF));

    CdfHeader header;
    iF.read((char *)&header, sizeof(CdfHeader));

    if(strcmp(header.magic, g_CDF_MAGIC))
    {
        cout << "error: file " << a_in_fn << " isn't CDF file" << endl;
        return 3;
    }

    cout << "magic: " << header.magic << endl;
    cout << "index size: " << header.index_size << endl;
    cout << "files count: " << header.files_count << endl;
    cout << "total file size: " << header.size_total << endl;

    // index table is in the end of file, thus calculate offset from total file size
    u32 index_size;
    IndexEntry *index = (IndexEntry *)block_decode(index_size, iF, file_size - header.index_size, 0);

#ifdef DEBUG
    {
        ofstream oF("table.raw", ios::binary);
        if(oF.is_open())
            oF.write((char *)index, index_size);
    }
#endif

    // decode and store files
    cout << "extracting files:" << endl;
    u32 entries_count = index_size / sizeof(IndexEntry);
#ifdef EXTRACT_DUPLICATES
    list<string> saved_filenames;
#endif
    for(u32 i = 0; i < entries_count; ++i)
    {
        string filename(index[i].filename);

#ifdef EXTRACT_DUPLICATES
        // preserve files with duplicated filename too
        if(find(saved_filenames.begin(), saved_filenames.end(), filename) != saved_filenames.end())
            filename += ".DUP";
        saved_filenames.push_back(filename);
#endif

        cout << "extracting " << filename << "... ";

        // compressed file
        if(index[i].type & 0x10)
        {
            u32 data_size;
            u8 *data = block_decode(data_size, iF, index[i].offset, index[i].size_dec);
            if(data_size == index[i].size_dec)
            {
                ofstream oF(path(output_path / filename).string().c_str(), ios::binary);
                oF.write((char *)data, index[i].size_dec);
            }
            else
                cout << "error: specified size " << index[i].size_dec << " and decoded " << data_size << " are different" << endl;

            block_free(data);
        }
        // uncompressed file
        else
        {
            u8 *file_buffer = new u8[index[i].size_dec];
            iF.seekg(index[i].offset, ios::beg);
            iF.read((char *)file_buffer, index[i].size_dec);
            ofstream oF(path(output_path / filename).string().c_str(), ios::binary);
            oF.write((char *)file_buffer, index[i].size_dec);
            delete[] file_buffer;
        }

        cout << "done" << endl;
    }

    block_free((u8 *)index);

    return 0;
}


int cdf_pack(string a_out_fn, string a_in_path)
{
    path in_path(a_in_path);

    if(!is_directory(in_path) || !exists(in_path))
    {
        cout << "error: unable to open input directory " << in_path << endl;
        return 1;
    }

    ofstream oF(a_out_fn.c_str(), ios::binary);
    if(!oF.is_open())
    {
        cout << "error: unable to create output file " << a_out_fn << endl;
        return 2;
    }

    // get file list
    list<path> file_list;
    walkthrough(in_path, file_list_cb, &file_list);
    file_list.sort();

    // skip header
    oF.seekp(sizeof(CdfHeader), ios::cur);

    // write file data
    list<IndexEntry> index_entries;
    u32 bytes_processed = 0;
    for(list<path>::iterator it = file_list.begin(); it != file_list.end(); ++it)
    {
        ifstream iF(it->string().c_str(), ios::binary);
        if(!iF.is_open())
        {
            cout << "error: unable to open file " << *it << endl;
            continue;
        }

        cout << "adding " << *it << "... ";

        u32 file_size = static_cast<u32>(file_get_size(iF));
        u8 *file_data = new u8[file_size];
        iF.read((char *)file_data, file_size);
        iF.close();

        bytes_processed += file_size;

        bool do_deflate = false;

        IndexEntry entry{};
        const string filename = it->filename().string();
        memcpy(entry.filename, filename.c_str(), min(filename.size(), sizeof(entry.filename) - 1));
        entry.offset = static_cast<u32>(static_cast<streamoff>(oF.tellp()));
        entry.size_dec = file_size;
        // 0x10 - flag that file contents are deflated
        entry.type = static_cast<u8>((do_deflate ? 0x10 : 0) | file_get_type(it->extension().string()));
        index_entries.push_back(entry);

        if(do_deflate)
            // FIXME: as full deflate is currently unsupported, pass false as
            //        4nd parameter, and encapsulate uncompressed blocks
            //        inside deflate blocks container, it has support for this
            //        when implemented, uncomment 2nd line and remove first
            block_encode(oF, file_data, file_size, false);
        //			block_encode(oF, file_data, file_size, do_deflate);
        else
            oF.write((char *)file_data, file_size);

        delete[] file_data;

        cout << "done" << endl;
    }

    // write files index
    u32 index_start = static_cast<u32>(static_cast<streamoff>(oF.tellp()));
    IndexEntry *files_index = new IndexEntry[index_entries.size()];
    u32 i = 0;
    for(list<IndexEntry>::iterator it = index_entries.begin(); it != index_entries.end(); ++it)
        files_index[i++] = *it;
    block_encode(oF, (u8 *)files_index, index_entries.size() * sizeof(IndexEntry), false);
    delete[] files_index;

    // write header
    CdfHeader header{};
    memcpy(header.magic, g_CDF_MAGIC, strlen(g_CDF_MAGIC) + 1);
    header.index_size = static_cast<u32>(oF.tellp()) - index_start;
    header.files_count = static_cast<u32>(index_entries.size());
    header.size_total = bytes_processed;

    oF.seekp(0, ios::beg);
    oF.write((char *)&header, sizeof(header));

    return 0;
}


u8 *block_decode(u32 &r_size, ifstream &a_if, u32 a_offset, u32 a_size)
{
    a_if.seekg(a_offset, ios::beg);

    // in the beginning, there are N block offsets, we are reading first
    // offset, calculate from it next offsets count and read others
    u32 offset0;
    a_if.read((char *)&offset0, sizeof(offset0));
    u32 *block_offsets = new u32[offset0 / sizeof(u32)];
    block_offsets[0] = offset0;
    u32 blocks_count = offset0 / sizeof(u32) - 1;
    a_if.read((char *)&block_offsets[1], blocks_count * sizeof(u32));

    // for file index we specify a_size = 0, else final uncompressed size
    u8 *data = new u8[a_size ? a_size : 0x8000 * blocks_count];
    r_size = 0;
    for(u32 i = 0; i < blocks_count; ++i)
    {
        a_if.seekg(a_offset + block_offsets[i], ios::beg);
        u32 block_size = block_offsets[i + 1] - block_offsets[i];
        u8 *buffer = new u8[block_size];
        a_if.read((char *)buffer, block_size);

        u32 out_size = decode(&data[r_size], buffer, block_size);
        r_size += out_size;

        if(!out_size)
        {
            cout << "error: unable to decode index " << i << endl;
        }

        delete[] buffer;
    }

    delete[] block_offsets;

    return data;
}


void block_encode(ofstream &a_of, u8 *a_in_data, u32 a_in_size, bool a_deflate)
{
    u32 block_size = 0x8000;

    u32 blocks_count = a_in_size / block_size + (a_in_size % block_size ? 1 : 0);
    u32 *block_offsets = new u32[blocks_count + 1];
    block_offsets[0] = (blocks_count + 1) * sizeof(u32);

    u32 start = static_cast<u32>(a_of.tellp());
    a_of.seekp(block_offsets[0], ios::cur);
    for(u32 i = 0; i < blocks_count; ++i)
    {
        u32 encode_size;
        u8 *encode_data = encode(encode_size, &a_in_data[i * block_size], min(block_size, a_in_size - i * block_size), a_deflate);
        a_of.write((char *)encode_data, encode_size);
        block_offsets[i + 1] = (u32)a_of.tellp() - start;
        delete[] encode_data;
    }

    // write blocks index
    a_of.seekp(start, ios::beg);
    a_of.write((char *)block_offsets, (blocks_count + 1) * sizeof(u32));

    // seek to written data end
    a_of.seekp(start + block_offsets[blocks_count], ios::beg);

    delete[] block_offsets;
}


void block_free(u8 *a_data)
{
    delete[] a_data;
}


u32 decode(u8 *r_out_data, u8 *a_in_data, u32 a_in_size)
{
    u32 out_size = 0;

    // 0x0040f8d0 of exe
    u16 data_type = a_in_data[0] | (a_in_data[1] << 8);

    u8 *in_data = a_in_data + sizeof(u16);
    u32 in_size = a_in_size - sizeof(u16);

    // data uncompressed, just copy contents
    if(data_type == 0)
    {
        memcpy(r_out_data, in_data, in_size);
        out_size = in_size;
    }
    else if(data_type == 8)
    {
        out_size = inflate_data(r_out_data, in_data, in_size);
    }

    return out_size;
}


u8 *encode(u32 &r_out_size, u8 *a_in_data, u32 a_in_size, bool a_deflate)
{
    // FIXME
    if(a_deflate)
        cout << "error: packer deflate support not implemented" << endl;

    // FIXME: add deflate support
    u16 data_type = a_deflate ? 8 : 0;

    r_out_size = a_in_size + sizeof(data_type);
    u8 *out_data = new u8[r_out_size];
    for(u32 i = 0; i < sizeof(data_type); ++i)
        out_data[i] = (u8)(data_type >> i * 8 & 0xff);
    memcpy(out_data + sizeof(data_type), a_in_data, a_in_size);

    return out_data;
}


void file_list_cb(void *a_data, path a_file)
{
    list<path> &file_list = *(list<path> *)a_data;
    file_list.push_back(a_file);
}


u32 file_get_type(string a_ext)
{
    for(u32 i = 0; i < SIZEOF(g_FILE_TYPES); ++i)
        if(string(g_FILE_TYPES[i].extension) == a_ext)
            return g_FILE_TYPES[i].type;

    return 0;
}


streampos file_get_size(ifstream &a_if)
{
    streampos last_pos = a_if.tellg();
    a_if.seekg(0, ios::end);
    streampos result = a_if.tellg();
    a_if.seekg(last_pos);

    return result;
}
