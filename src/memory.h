#pragma once

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <functional>

#define TOTAL_MEMORY (64 * 1024 + 256 * 1024)


class Memory {
private:
    uint8_t bytes[TOTAL_MEMORY];
    bool mode_stack;
    bool mode_map;
    uint8_t page_map;
    uint8_t page_stack;

    std::vector<uint8_t> bootbytes;

public:
    /* virtual addr, physical addr, stackrq, value */
    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onwrite;
    std::function<void(uint32_t,uint32_t,bool,uint8_t)> onread;

public:
    Memory();
    void control_write(uint8_t w8);
    uint32_t bigram_select(uint32_t addr, bool stackrq);
    uint32_t tobank(uint32_t a);
    uint8_t read(uint32_t addr, bool stackrq);
    void write(uint32_t addr, uint8_t w8, bool stackrq);
    void init_from_vector(std::vector<uint8_t> & from, uint32_t start_addr);
    void attach_boot(std::vector<uint8_t> boot);
    void detach_boot();
    uint8_t * buffer();
};
