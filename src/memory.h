#pragma once

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <functional>

#define TOTAL_MEMORY (64 * 1024 + 256 * 1024)


class Memory {
public:
    typedef std::array<uint8_t, TOTAL_MEMORY> heatmap_t;

private:
    uint8_t bytes[TOTAL_MEMORY];
    bool mode_stack;
    uint8_t mode_map;
    uint32_t page_map;
    uint32_t page_stack;

    std::vector<uint8_t> bootbytes;

    heatmap_t heatmap;

    uint32_t tobank(uint32_t a) const;

public:
    uint32_t bigram_select(uint32_t addr, bool stackrq) const;
    uint8_t get_byte(uint32_t addr, bool stackrq) const;
    /* virtual addr, physical addr, stackrq, value */
    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onwrite;
    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onread;

    /* bigaddr */
    std::function<void(uint32_t)> debug_onwrite;
    std::function<void(uint32_t)> debug_onread;

public:
    Memory();
    void control_write(uint8_t w8);
    uint8_t read(uint32_t addr, bool stackrq) const;
    void write(uint32_t addr, uint8_t w8, bool stackrq);
    void init_from_vector(const std::vector<uint8_t> & from, uint32_t start_addr);
    void attach_boot(std::vector<uint8_t> boot);
    void detach_boot();
    uint8_t * buffer();
    size_t buffer_size() const { return sizeof(bytes); }
    heatmap_t& get_heatmap() { return heatmap; }
    void cool_off_heatmap();
    void export_bytes(uint8_t * dst, uint32_t addr, uint32_t size) const;

    void serialize(std::vector<uint8_t> & to);
    void deserialize(std::vector<uint8_t>::iterator from, uint32_t size);
    
    auto get_mode_stack() const -> const bool;
    auto get_mode_map() const -> const uint8_t;
    auto get_page_map() const -> const uint32_t;
    auto get_page_stack() const -> const uint32_t;
};
