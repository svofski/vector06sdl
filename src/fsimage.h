#pragma once

#include <vector>
#include <functional>
#include <tuple>
#include <string>
#include "util.h"

struct MDHeaderData
{
    uint8_t User;
    char Name[8];
    char Ext[3];
    uint8_t Extent;
    uint8_t Unknown1;
    uint8_t Unknown2;
    uint8_t Records;
    uint16_t FAT[8];
} __attribute__((packed));

struct MDHeader
{
    typedef MDHeaderData datatype;

    MDHeaderData * fields;
    uint8_t * bytes;

    static constexpr size_t SIZE = sizeof(MDHeaderData);

    uint8_t user() const { return fields->User; }

    std::string name() const { 
        return util::rtrim_copy(std::string(fields->Name, 8));
    }

    std::string ext() const {
        return util::rtrim_copy(std::string(fields->Ext, 3));
    }

    MDHeader(uint8_t * bytes)
        : fields(reinterpret_cast<MDHeaderData *>(bytes)), bytes(bytes) 
    {}

    void init_with_filename(std::string filename);
    void overwrite(const MDHeader & other);
    bool operator==(const MDHeader & rhs) const;
    bool operator!=(const MDHeader & rhs) const;
};

struct Dirent
{
    MDHeader header;
    std::vector<uint16_t> chain;
    int size;

    Dirent() :header(nullptr), size(0) {}

    Dirent(MDHeader header) : header(header), size(0) { }

    Dirent(const Dirent& rhs)
        : header(rhs.header), chain(rhs.chain), size(rhs.size) {
    }

    std::string name() const { return header.name(); }

    std::string ext() const { return header.ext(); }

    uint8_t user() const { return header.user(); }
};

class FilesystemImage {
    typedef std::vector<uint8_t> bytes_t;
    bytes_t bytes;

public:
    static constexpr int SECTOR_SIZE = 1024;
    static constexpr int SECTORS_CYL = 10;
    static constexpr int MAX_FS_BYTES = 860160;
    static constexpr int CLUSTER_BYTES = 2048;
    static constexpr int SYSTEM_TRACKS = 6;

    struct dir_iterator {
        FilesystemImage & fs;
        int position;

        dir_iterator(FilesystemImage & fs, int position) 
            : fs(fs), position(position) {
        }

        MDHeader operator*() const {
            return MDHeader(&fs.bytes[position]);
        }

        const dir_iterator& operator++() { // prefix
            position += 32;
            return *this;
        }

        bool operator==(const dir_iterator& rhs) {
            return position == rhs.position;
        }

        bool operator!=(const dir_iterator& rhs) {
            return position != rhs.position;
        }
    };

    FilesystemImage() : FilesystemImage(0) {}

    FilesystemImage(int size) : bytes(size, 0xe5) {}

    FilesystemImage(const bytes_t & data) : bytes(data) 
    {
    }

    bool mount_local_dir(std::string path);

    void set_data(const bytes_t & data);
    bytes_t& data() { return bytes; }
    bytes_t::iterator map_sector(int track, int head, int sector);
    dir_iterator begin();
    dir_iterator end();
    Dirent find_file(const std::string & filename);
    void listdir(std::function<void(const Dirent &)> cb);

    // load full file information and sector map
    Dirent load_dirent(MDHeader header);

    std::tuple<int,int,int> cluster_to_ths(int cluster);
    bytes_t read_bytes(const Dirent & de);
    bytes_t read_file(const std::string & filename);
    std::vector<uint16_t> build_available_chain(int length);

    // return ok, internal file name
    std::tuple<bool,std::string> 
        save_file(const std::string & filename, bytes_t content);
};

