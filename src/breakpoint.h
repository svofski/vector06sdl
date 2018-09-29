#pragma once

struct Breakpoint {
    int addr;
    int kind;

    Breakpoint(int addr_, int kind_) : addr(addr_), kind(kind_) {}

    bool operator==(const Breakpoint & b) const {
        return this->addr == b.addr && this->kind == b.kind;
    }
};
