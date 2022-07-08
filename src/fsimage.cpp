#include "fsimage.h"

#include <cstdint>
#include <vector>
#include <assert.h>
#include <iostream>
#include "util.h"
#include "tinydir.h"

void MDHeader::init_with_filename(std::string stem, std::string ext) 
{
    std::fill(bytes, bytes + 32, 0);
    std::fill(std::begin(fields->Name), std::end(fields->Name), ' ');
    std::fill(std::begin(fields->Ext), std::end(fields->Ext), ' ');

    for (int i = 0, end = (int)std::min(sizeof(datatype::Name), stem.size());
            i < end; ++i) {
        fields->Name[i] = std::toupper(stem[i]);
    }

    for (int i = 0, end = (int)std::min(sizeof(datatype::Ext), ext.size());
            i < end; ++i) {
        fields->Ext[i] = std::toupper(ext[i]);
    }
}

void MDHeader::overwrite(const MDHeader & other)
{
    std::copy(other.bytes, other.bytes + 32, bytes);
}

bool MDHeader::operator==(const MDHeader & rhs) const {
    return fields == rhs.fields ||
        (fields != nullptr && rhs.fields != nullptr &&
        memcmp(fields, rhs.fields, sizeof(datatype)) == 0);
}

bool MDHeader::operator!=(const MDHeader & rhs) const {
    return !(*this == rhs);
}


bool FilesystemImage::mount_local_dir(std::string path)
{
    tinydir_dir dir;
    if (tinydir_open(&dir, path.c_str()) != 0) {
        return false;
    }

    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);

        std::string fullpath = path + '/' + file.name;
        size_t length = util::filesize(fullpath);

        //printf("%s, %lx\n", file.name, length);

        if (!file.is_dir && length <= MAX_FS_BYTES) {
            auto content = util::load_binfile(fullpath);
            if (content.size() > 0) {
                /* auto [ok, filename] =*/ save_file(file.name, content);
            }
        }

        tinydir_next(&dir);
    }

    tinydir_close(&dir);
    return true;
}

void FilesystemImage::set_data(const bytes_t & data)
{
    if (bytes.size() < data.size()) {
        bytes.resize(data.size());
    }
    std::copy(data.begin(), data.end(), bytes.begin());
}

FilesystemImage::bytes_t::iterator
    FilesystemImage::map_sector(int track, int head, int sector)
{
    unsigned offset = track * SECTOR_SIZE * SECTORS_CYL + 
        head * SECTOR_SIZE * (SECTORS_CYL/2) +
        (sector - 1) * SECTOR_SIZE;

    if (offset + SECTOR_SIZE >= bytes.size()) {
        return bytes.end();
    }
    return bytes.begin() + offset;
}

FilesystemImage::dir_iterator FilesystemImage::begin() {
    return dir_iterator(*this, 0xa000);
}

FilesystemImage::dir_iterator FilesystemImage::end() {
    return dir_iterator(*this, 0xb000);
}

Dirent FilesystemImage::find_file(const std::string & filename)
{
    std::vector<uint8_t> protobuf(MDHeader::SIZE);
    MDHeader proto(&protobuf[0]);
    auto [path, stem, ext] = util::split_path(filename);
    if (ext.size() > 0) {
        ext = ext.substr(1);
    }
    proto.init_with_filename(stem, ext);
    for (auto dit = begin(); dit != end(); ++dit) {
        if ((*dit).user() == proto.user() &&
            (*dit).name() == proto.name() &&
            (*dit).ext() == proto.ext()) {

            return load_dirent(*dit);
        }
    }
    return Dirent();
}

void FilesystemImage::listdir(std::function<void(const Dirent &)> cb)
{
    for (auto it = begin(); it != end(); ++it) {
        if ((*it).user() < 0x10 && (*it).fields->Extent == 0) {
            cb(load_dirent(*it));
        }
    }
}

// load full file information and sector map
Dirent FilesystemImage::load_dirent(MDHeader header)
{
    Dirent dirent(header);

    for (auto dit = begin(); dit != end(); ++dit) {
        MDHeader h = *dit;
        if (h.user() < 0x10 && h.name() == header.name() &&
                h.ext() == header.ext()) {
            for (int i = 0; i < 8; ++i) {
                dirent.chain.push_back(h.fields->FAT[i]);
            }
            if (h.fields->Records != 0x80) {
                dirent.size = h.fields->Extent * 2048 * 8 + 
                    h.fields->Records * 128;
                break;
            }
        }
    }

    return dirent;
}

std::tuple<int,int,int> FilesystemImage::cluster_to_ths(int cluster) const
{
    int track = 8 + cluster / 5;
    int head = track % 2;
    track >>= 1;
    int sector = 1 + (cluster % 5);
    return {track, head, sector};
}

FilesystemImage::bytes_t FilesystemImage::read_bytes(const Dirent & de)
{
    int resultptr = 0;
    bytes_t result(de.size);

    int remainder = de.size;

    for (uint16_t c : de.chain) {
        int clust = c * 2;
        if (clust < 2) {
            break;
        }

        for (int i = 0; i < 2; ++i) {
            auto [track, head, sector] = cluster_to_ths(clust + i);
            auto data = map_sector(track, head, sector);

            if (data == bytes.end()) {
                fprintf(stderr, "E: reading outside of disk boundary. "
                        "T:%d H:%d S:%d\n", track, head, sector);
                break;
            }

            int tocopy = std::min(SECTOR_SIZE, remainder);

            std::copy(data, data + tocopy, &result[resultptr]);
            resultptr += tocopy;
            remainder -= tocopy;
        }
    }

    return result;
}

FilesystemImage::bytes_t FilesystemImage::read_file(const std::string & filename)
{
    Dirent de = find_file(filename);
    if (de.size) {
        return read_bytes(de);
    }
    return {};
}

FilesystemImage::chain_t FilesystemImage::build_available_chain(int length)
{
    int maxclust = 
        (bytes.size() - SECTORS_CYL*SECTOR_SIZE*SYSTEM_TRACKS) / CLUSTER_BYTES;
    if (maxclust == 0) return {};

    bytes_t used(maxclust, 0);
    unsigned usedcount = 0;

    for (auto h = begin(); h != end(); ++h) {
        if ((*h).user() < 0x10) {
            for (int i = 0; i < 8; ++i) {
                used[(*h).fields->FAT[i]] = 1;
                ++usedcount;
            }
        }
    }

    if (used.size() - usedcount < (unsigned)length) {
        return {};
    }

    std::vector<uint16_t> chain;
    for (unsigned i = 2; i < used.size(); ++i) {
        if (!used[i]) {
            chain.push_back(i);
            if (chain.size() == (unsigned)length) {
                break;
            }
        }
    }

    return chain;
}

std::string FilesystemImage::allocate_file(const std::string & filename,
        const bytes_t & content, const chain_t & chain)
{
    auto [stem, ext] = unique_cpm_filename(filename);
    std::string internal_filename = stem + "." + ext;
    taken_names.insert(internal_filename);

    bytes_t protobuf(MDHeader::SIZE);
    MDHeader proto(&protobuf[0]);
    proto.init_with_filename(stem, ext);

    int remaining = content.size();
    int chain_index = 0;
    int extent = 0;
    for (auto h = begin(); h != end() && remaining > 0; ++h) {
        if ((*h).user() >= 0x10) {
            proto.fields->Records = std::min(0x80, (remaining + 127) / 128);
            proto.fields->Extent = extent++;        

            for (int i = 0; i < 8; ++i) {
                proto.fields->FAT[i] = remaining > 0 ? chain[chain_index] : 0;
                if (remaining > 0) {
                    remaining -= 2048;
                    ++chain_index;
                }
            }

            // update directory entry
            (*h).overwrite(proto); 
        }
    }

    return internal_filename;
}

// return (ok, internal file name)
std::tuple<bool,std::string> 
    FilesystemImage::save_file(const std::string & filename, bytes_t content)
{
    int size_in_clusters = (content.size() + 2047) / 2048;
    auto chain = build_available_chain(size_in_clusters);
    if (chain.size() == 0) return {false, ""};

    auto internal_filename = allocate_file(filename, content, chain);

    // write data
    int remaining = content.size();
    auto srcptr = content.begin();
    for (auto c : chain) {
        int clust = c * 2;
        for (int i = 0; i < 2; ++i) {
            auto [t,h,s] = cluster_to_ths(clust + i);
            auto sector_data = map_sector(t, h, s);
            if (sector_data == bytes.end()) {
                fprintf(stderr, "E: writing outside of disk boundary "
                        "T:%d H:%d S:%d\n", t, h, s);
                break;
            }
            int tocopy = std::min(SECTOR_SIZE, remaining);
            std::copy(srcptr, srcptr + tocopy, sector_data);
            std::fill(sector_data + tocopy, sector_data + SECTOR_SIZE, 0x1a);
            srcptr += tocopy;
            remaining -= tocopy;
        }
        if (remaining == 0) {
            break;
        }
    }

    return {true, internal_filename};
}

std::tuple<std::string, std::string>
    FilesystemImage::unique_cpm_filename(const std::string & filename)
{
    auto [path, stem, ext] = util::split_path(filename);
    constexpr size_t name_sz = sizeof(MDHeader::datatype::Name);
    constexpr size_t ext_sz = sizeof(MDHeader::datatype::Ext);
    util::str_toupper(stem);
    util::str_toupper(ext);
    ext = ext.substr(1, ext_sz);
    stem = stem.substr(0, name_sz);

    auto funnychars = [](std::string s) {
        for (auto & c : s) {
            if (c == '.') c = '_';
        }
    };
    funnychars(stem);
    funnychars(ext);

    bool name_taken = taken_names.count(stem + "." + ext);
    int n = 1;
    while (name_taken) {
        int nc = std::to_string(n).size() + 1;
        stem = stem.substr(0, name_sz - nc);
        stem += "@" + std::to_string(n);
        ++n;
        name_taken = taken_names.count(stem + "." + ext);
    }

    return {stem, ext};
}
