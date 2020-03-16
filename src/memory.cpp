#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <vector>

#include "memory.h"

using namespace std;

Memory::Memory() : mode_stack(false), mode_map(false), page_map(0), page_stack(0)
{
    memset(bytes, 0, sizeof(bytes));
}

void Memory::control_write(uint8_t w8)
{
    this->mode_stack = (w8 & 0x10) != 0;
    this->mode_map = (w8 & 0x20) != 0;

    this->page_map = ((w8 & 3) + 1) << 16;
    this->page_stack = (((w8 & 0xc) >> 2) + 1) << 16;

    //printf("memory: mode_stack=%x mode_map=%x page_map=%x page_stack=%x\n",
    //        this->mode_stack, this->mode_map, this->page_map, this->page_stack);
}

uint32_t Memory::bigram_select(uint32_t addr, bool stackrq) 
{
    if (!(this->mode_map || this->mode_stack)) {
        return addr;
    } else if (this->mode_stack && stackrq) {
        return addr + this->page_stack;
    } else if (this->mode_map && (addr >= 0xa000) && (addr < 0xe000)) {
        return addr + this->page_map;
    }
    return addr;
}

uint32_t Memory::tobank(uint32_t a) 
{
    return (a & 0x78000) | ((a<<2)&0x7ffc) | ((a>>13)&3);
}

uint8_t Memory::read(uint32_t addr, bool stackrq)
{
    uint8_t value;
    uint32_t phys = addr;

    uint32_t bigaddr = this->bigram_select(addr & 0xffff, stackrq);
    if (this->bootbytes.size() && bigaddr < this->bootbytes.size()) {
        value = this->bootbytes[bigaddr];
    } 
    else {
        phys = this->tobank(bigaddr);
        value = this->bytes[phys];
    }

    if (this->onread) this->onread(addr, phys, stackrq, value);

    return value;
}

void Memory::write(uint32_t addr, uint8_t w8, bool stackrq)
{
    uint32_t phys = this->tobank(this->bigram_select(addr & 0xffff, stackrq));
    if (this->onwrite) {
        this->onwrite(addr, phys, stackrq, w8);
    }
    this->bytes[phys] = w8;
}

void Memory::init_from_vector(const vector<uint8_t> & from, uint32_t start_addr)
{
    // clear the main ram because otherwise switching roms is a pain
    // but leave the kvaz alone
    if (start_addr < 65536) {
        memset(this->bytes, 0, 65536);
    }
    else {
        memset(this->bytes + start_addr, 0, sizeof(bytes) - start_addr);
    }
    for (unsigned i = 0; i < from.size(); ++i) {
        int addr = start_addr + i;
        //this->write(addr, from[i], false);
        uint32_t phys = this->tobank(addr);
        if (phys < sizeof(this->bytes)) {
            this->bytes[phys] = from[i];
        }
    }
}

void Memory::attach_boot(vector<uint8_t> boot)
{
    this->bootbytes = boot;
}

void Memory::detach_boot()
{
    this->bootbytes.clear();
}

uint8_t * Memory::buffer() 
{
    return bytes;
}

#include "serialize.h"

void Memory::serialize(std::vector<uint8_t> &to) {
    std::vector<uint8_t> tmp;
    tmp.push_back((uint8_t)mode_stack);
    tmp.push_back((uint8_t)mode_map);
    tmp.push_back((uint8_t)(page_map>>16));
    tmp.push_back((uint8_t)(page_stack>>16));
    tmp.push_back(sizeof(this->bytes)/65536); // normally 1+4, but we could get many ramdisks later
    tmp.insert(std::end(tmp), this->bytes, this->bytes + sizeof(this->bytes));
    tmp.insert(std::end(tmp), std::begin(this->bootbytes), std::end(this->bootbytes));

    SerializeChunk::insert_chunk(to, SerializeChunk::MEMORY, tmp);
}

void Memory::deserialize(std::vector<uint8_t>::iterator it, uint32_t size)
{
    auto begin = it;
    this->mode_stack = (bool) *it++;
    this->mode_map = (bool) *it++;
    this->page_map = ((uint32_t) *it++) << 16;
    this->page_stack = ((uint32_t) *it++) << 16;
    uint32_t stored_ramsize = 65536 * *it++;
    size_t nbytes = std::min(stored_ramsize, (uint32_t)sizeof(this->bytes));
    std::copy(it, it + nbytes, this->bytes);
    it += stored_ramsize;

    this->bootbytes.clear();
    this->bootbytes.assign(it, begin + size);
}


