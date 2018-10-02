#pragma once

#include <stdint.h>

struct Breakpoint {
    int addr;
    int kind;

    Breakpoint(int addr_, int kind_) : addr(addr_), kind(kind_) {}

    bool operator==(const Breakpoint & b) const {
        return this->addr == b.addr && this->kind == b.kind;
    }

};

struct Watchpoint {
    enum wptype {
        WRITE, READ, ACCESS
    };

    uint32_t addr;
    uint32_t length;
    int type;

    Watchpoint(wptype type_, int addr_, int length_) : addr(addr_), 
        length(length_), type(type_) {}

    bool operator==(const Watchpoint & b) const {
        return this->addr == b.addr && this->length == b.length &&
            this->type == b.type;
    }
};
