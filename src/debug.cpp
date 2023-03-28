#include "debug.h"
#include "i8080.h"
#include "serialize.h"
#include "utils_string.h"

#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <vector>
#include <mutex>

Debug::Debug(Memory* _memoryP)
  : mem_runs()
  , mem_reads()
  , mem_writes()
  , trace_log()
  , memoryP(_memoryP)
  , wp_break(false)
{
    auto read_func = [this](const uint32_t _addr, const uint8_t _val,
                       const bool _is_opcode) {
        this->read(_addr, _val, _is_opcode);
    };

    auto write_func = [this](const uint32_t _addr, const uint8_t _val) {
        this->write(_addr, _val);
    };

    memoryP->debug_onread = read_func;
    memoryP->debug_onwrite = write_func;
}

void Debug::read(
  const size_t _global_addr, const uint8_t _val, const bool _is_opcode)
{
    if (_is_opcode) {
        mem_runs[_global_addr]++;
        trace_log_update(_global_addr, _val);
    } else {
        mem_reads[_global_addr]++;
        wp_break |= check_watchpoint(Watchpoint::Access::R, _global_addr, _val);
    }
}

void Debug::write(const size_t _global_addr, const uint8_t _val)
{
    mem_writes[_global_addr]++;
    wp_break |= check_watchpoint(Watchpoint::Access::W, _global_addr, _val);
}

static const char* mnemonics[0x100] = { "NOP", "LXI B,", "STAX B", "INX B",
    "INR B", "DCR B", "MVI B,", "RLC", "DB 08", "DAD B", "LDAX B", "DCX B",
    "INR C", "DCR C", "MVI C,", "RRC", "DB 10", "LXI D,", "STAX D", "INX D",
    "INR D", "DCR D", "MVI D,", "RAL", "DB 18", "DAD D", "LDAX D", "DCX D",
    "INR E", "DCR E", "MVI E,", "RAR", "DB 20", "LXI H,", "SHLD", "INX H",
    "INR H", "DCR H", "MVI H,", "DAA", "DB 28", "DAD H", "LHLD", "DCX H",
    "INR L", "DCR L", "MVI L,", "CMA", "DB 30", "LXI SP,", "STA", "INX SP",
    "INR M", "DCR M", "MVI M,", "STC", "DB 38", "DAD SP", "LDA", "DCX SP",
    "INR A", "DCR A", "MVI A,", "CMC",

    "MOV B, B", "MOV B, C", "MOV B, D", "MOV B, E", "MOV B, H", "MOV B, L",
    "MOV B, M", "MOV B, A", "MOV C, B", "MOV C, C", "MOV C, D", "MOV C, E",
    "MOV C, H", "MOV C, L", "MOV C, M", "MOV C, A", "MOV D, B", "MOV D, C",
    "MOV D, D", "MOV D, E", "MOV D, H", "MOV D, L", "MOV D, M", "MOV D, A",
    "MOV E, B", "MOV E, C", "MOV E, D", "MOV E, E", "MOV E, H", "MOV E, L",
    "MOV E, M", "MOV E, A", "MOV H, B", "MOV H, C", "MOV H, D", "MOV H, E",
    "MOV H, H", "MOV H, L", "MOV H, M", "MOV H, A", "MOV L, B", "MOV L, C",
    "MOV L, D", "MOV L, E", "MOV L, H", "MOV L, L", "MOV L, M", "MOV L, A",
    "MOV M, B", "MOV M, C", "MOV M, D", "MOV M, E", "MOV M, H", "MOV M, L",
    "HLT", "MOV M, A", "MOV A, B", "MOV A, C", "MOV A, D", "MOV A, E",
    "MOV A, H", "MOV A, L", "MOV A, M", "MOV A, A",

    "ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A",
    "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A",
    "SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A",
    "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A",
    "ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A",
    "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A",
    "ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A",
    "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A",

    "RNZ", "POP B", "JNZ", "JMP", "CNZ", "PUSH B", "ADI", "RST 0", "RZ", "RET",
    "JZ", "DB CB", "CZ", "CALL", "ACI", "RST 1", "RNC", "POP D", "JNC", "OUT",
    "CNC", "PUSH D", "SUI", "RST 2", "RC", "DB D9", "JC", "IN", "CC", "DB DD",
    "SBI", "RST 3", "RPO", "POP H", "JPO", "XTHL", "CPO", "PUSH H", "ANI",
    "RST 4", "RPE", "PCHL", "JPE", "XCHG", "CPE", "DB ED", "XRI", "RST 5", "RP",
    "POP PSW", "JP", "DI", "CP", "PUSH PSW", "ORI", "RST 6", "RM", "SPHL", "JM",
    "EI", "CM", "DB FD", "CPI", "RST 7" };

#define CMD_BYTES_MAX 3

static const uint8_t cmd_lens[0x100] = { 1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
    1, 2, 1, 1, 3, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 3, 3, 1, 1, 1,
    2, 1, 1, 1, 3, 1, 1, 1, 2, 1, 1, 3, 3, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1, 1, 1, 3, 2, 3,
    1, 2, 1, 1, 1, 3, 2, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1,
    2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1 };

auto Debug::get_cmd_len(const uint8_t _addr) const -> const size_t
{
    return cmd_lens[_addr];
}

static size_t cmd[3];
#define CMD_OPCODE cmd[0]
#define CMD_OP_L cmd[1]
#define CMD_OP_H cmd[2]
#define MAX_DATA_CHR_LEN 11

auto get_mnemonic(const uint8_t _opcode, const uint8_t _data_l,
  const uint8_t _data_h, const int _cmd_len_min) -> const std::string
{
    CMD_OPCODE = _opcode;
    CMD_OP_L = _data_l;
    CMD_OP_H = _data_h;

    std::stringstream out;
    out << mnemonics[_opcode];

    if (cmd_lens[_opcode] == 2) {
        out << " " << std::setw(sizeof(uint8_t) * 2) << std::setfill('0');
        out << std::uppercase << std::hex << static_cast<int>(_data_l);
    } else if (cmd_lens[_opcode] == 3) {
        auto data_w = ((uint16_t)_data_h << 8) + _data_l;
        out << " " << std::setw(sizeof(uint16_t) * 2) << std::setfill('0');
        out << std::uppercase << std::hex << static_cast<int>(data_w);
    }

    int i = out.str().size();
    for (; i < _cmd_len_min; i++) {
        out << " ";
    }

    return out.str();
}

// 0 - call
// 1 - c*
// 2 - rst
// 3 - pchl
// 4 - jmp,
// 5 - j*
// 6 - ret, r*
// 7 - other
static const uint8_t opcode_types[0x100] = {
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 7, 5, 4,
    1, 7, 7, 2, 6, 6, 5, 7, 1, 0, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1,
    7, 7, 2, 6, 3, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2, 6, 7, 5, 7, 1, 7, 7, 2};

#define OPCODE_TYPE_MAX 7

inline uint8_t get_opcode_type(const uint8_t _opcode)
{
    return opcode_types[_opcode];
}

#define OPCODE_PCHL 0xE9

auto Debug::get_disasm_db_line(const size_t _addr, const uint8_t _data) const
  -> const std::string
{
    std::stringstream out;

    out << "0x";
    out << std::setw(sizeof(uint16_t) * 2) << std::setfill('0');
    out << std::uppercase << std::hex << static_cast<int>(_addr) << ":";

    // print data in a format " %02X"
    out << " ";
    out << std::setw(sizeof(uint8_t) * 2) << std::setfill('0');
    out << std::uppercase << std::hex << static_cast<int>(_data);

    // add whitespaces at the end of data
    for (int i = 3; i < MAX_DATA_CHR_LEN; i++) {
        out << " ";
    }

    out << "DB ";
    out << std::setw(sizeof(uint8_t) * 2) << std::setfill('0');
    out << std::uppercase << std::hex << static_cast<int>(_data);

    return out.str();
}

auto Debug::get_disasm_line(const size_t _addr, const uint8_t _opcode,
  const uint8_t _data_l, const uint8_t _data_h) const -> const std::string
{
    CMD_OPCODE = _opcode;
    CMD_OP_L = _data_l;
    CMD_OP_H = _data_h;

    auto cmd_len = cmd_lens[_opcode];
    std::stringstream out;

    out << "0x";
    out << std::setw(sizeof(uint16_t) * 2) << std::setfill('0');
    out << std::uppercase << std::hex << static_cast<int>(_addr) << ":";

    // print data in a format " %02X %02X %02X "
    int i = 0;
    for (; i < cmd_len; i++) {
        out << " ";
        out << std::setw(sizeof(uint8_t) * 2) << std::setfill('0');
        out << std::uppercase << std::hex << static_cast<int>(cmd[i]);
    }
    // add whitespaces at the end of data
    i *= 3; // every byte takes three chars " %02X"
    for (; i < MAX_DATA_CHR_LEN; i++) {
        out << " ";
    }

    out << get_mnemonic(_opcode, _data_l, _data_h, 13);

    return out.str();
}

size_t Debug::get_addr(
  const size_t _end_addr, const size_t _before_addr_lines) const
{
    if (_before_addr_lines == 0)
        return _end_addr;

    size_t start_addr =
      (_end_addr - _before_addr_lines * CMD_BYTES_MAX) & 0xffff;
#define MAX_ATTEMPTS 41

    size_t lines = 0;
    int addr_diff_max = _before_addr_lines * CMD_BYTES_MAX + 1;
    size_t addr = start_addr & 0xffff;

    for (int attempt = MAX_ATTEMPTS; attempt > 0 && lines != _before_addr_lines;
         attempt--) {
        addr = start_addr & 0xffff;
        int addr_diff = addr_diff_max;
        lines = 0;

        while (addr_diff > 0 && addr != _end_addr) {
            auto opcode = memoryP->get_byte(addr, false);
            auto cmd_len = get_cmd_len(opcode);
            addr = (addr + cmd_len) & 0xffff;
            addr_diff -= cmd_len;
            lines++;
        }

        if (addr == _end_addr && lines == _before_addr_lines) {
            return start_addr;
        }

        start_addr++;
        addr_diff_max--;
    }
    return _end_addr;
}

auto Debug::get_disasm(const size_t _addr, const size_t _lines,
  const size_t _before_addr_lines) const -> std::string
{
    std::string out;
    if (_lines == 0)
        return out;

    size_t addr = _addr;
    auto pc = static_cast<size_t>(i8080cpu::i8080_pc());
    int lines = _lines;

    if (_before_addr_lines > 0) {
        // find new addr that's _before_addr_lines commands ahead of _addr
        addr = get_addr(_addr & 0xffff, _before_addr_lines) & 0xffff;

        // if it fails to find an addr, we assume a data is ahead
        if (addr == _addr) {
            addr = (addr - _before_addr_lines) & 0xffff;

            lines = _lines - _before_addr_lines;

            for (size_t i = 0; i < _before_addr_lines; i++) {
                if (addr == pc) {
                    out += ">";
                } else {
                    out += " ";
                }
                auto db = memoryP->get_byte(addr, false);
                out += get_disasm_db_line(addr, db);

                size_t bigaddr = memoryP->bigram_select(addr, false);
                std::string runsS = std::to_string(mem_runs[bigaddr]);
                std::string readsS = std::to_string(mem_reads[bigaddr]);
                std::string writesS = std::to_string(mem_writes[bigaddr]);
                std::string runsS_readsS_writesS =
                  " (" + runsS + "," + readsS + "," + writesS + ")";
                out += runsS_readsS_writesS;

                if (labels.find(addr & 0xffff) != labels.end()) {
                    out += " " + labels.at(addr & 0xffff);
                }

                if (lines > 0 || i != _before_addr_lines - 1) {
                    out += "\n";
                }

                addr = (addr + 1) & 0xffff;
            }
        }
    }

    for (int i = 0; i < lines; i++) {
        if (addr == pc) {
            out += ">";
        } else {
            out += " ";
        }

        auto opcode = memoryP->get_byte(addr, false);
        auto data_l = memoryP->get_byte((addr + 1) & 0xffff, false);
        auto data_h = memoryP->get_byte((addr + 2) & 0xffff, false);
        out += get_disasm_line(addr, opcode, data_l, data_h);

        size_t bigaddr = memoryP->bigram_select(addr, false);
        std::string runsS = std::to_string(mem_runs[bigaddr]);
        std::string readsS = std::to_string(mem_reads[bigaddr]);
        std::string writesS = std::to_string(mem_writes[bigaddr]);
        std::string runsS_readsS_writesS =
          "(" + runsS + "," + readsS + "," + writesS + ")";
        out += runsS_readsS_writesS;

        if (labels.find(addr & 0xffff) != labels.end()) {
            out += " " + labels.at(addr & 0xffff);
        }

        if (get_cmd_len(opcode) == 3 || opcode == OPCODE_PCHL) {
            int label_addr = 0;
            if (opcode == OPCODE_PCHL) {
                label_addr = i8080cpu::i8080_regs_hl() & 0xffff;
            } else {
                label_addr = (data_h << 8 | data_l) % 0xffff;
            }

            if (labels.find(label_addr) != labels.end()) {
                out += " (" + labels.at(label_addr) + ")";
            }
        }

        if (i != lines - 1) {
            out += "\n";
        }

        addr = (addr + get_cmd_len(opcode)) & 0xffff;
    }
    return out;
}

void Debug::reset()
{
    std::fill(mem_runs, mem_runs + GLOBAL_MEM_SIZE, 0);
    std::fill(mem_reads, mem_reads + GLOBAL_MEM_SIZE, 0);
    std::fill(mem_writes, mem_writes + GLOBAL_MEM_SIZE, 0);

    for (size_t i = 0; i < TRACE_LOG_SIZE; i++) {
        trace_log[i].clear();
    }
    trace_log_idx = 0;
    trace_log_idx_view_offset = 0;
}

void Debug::serialize(std::vector<uint8_t>& to)
{
    auto mem_runs_p = reinterpret_cast<uint8_t*>(mem_runs);
    auto mem_reads_p = reinterpret_cast<uint8_t*>(mem_reads);
    auto mem_writes_p = reinterpret_cast<uint8_t*>(mem_writes);

    std::vector<uint8_t> tmp;
    tmp.insert(std::end(tmp), mem_runs_p,
      mem_runs_p + GLOBAL_MEM_SIZE * sizeof(uint64_t));
    tmp.insert(std::end(tmp), mem_reads_p,
      mem_reads_p + GLOBAL_MEM_SIZE * sizeof(uint64_t));
    tmp.insert(std::end(tmp), mem_writes_p,
      mem_writes_p + GLOBAL_MEM_SIZE * sizeof(uint64_t));

    SerializeChunk::insert_chunk(to, SerializeChunk::DEBUG, tmp);
}

void Debug::deserialize(std::vector<uint8_t>::iterator it, size_t size)
{
    auto mem_runs_p = reinterpret_cast<uint8_t*>(mem_runs);
    auto mem_reads_p = reinterpret_cast<uint8_t*>(mem_reads);
    auto mem_writes_p = reinterpret_cast<uint8_t*>(mem_writes);

    size_t mem_runs_size_in_bytes = GLOBAL_MEM_SIZE * sizeof(uint64_t);
    std::copy(it, it + mem_runs_size_in_bytes, mem_runs_p);
    it += mem_runs_size_in_bytes;

    size_t mem_reads_size_in_bytes = GLOBAL_MEM_SIZE * sizeof(uint64_t);
    std::copy(it, it + mem_reads_size_in_bytes, mem_reads_p);
    it += mem_reads_size_in_bytes;

    size_t mem_writes_size_in_bytes = GLOBAL_MEM_SIZE * sizeof(uint64_t);
    std::copy(it, it + mem_writes_size_in_bytes, mem_writes_p);
    it += mem_writes_size_in_bytes;
}

void Debug::add_breakpoint(
  const size_t _addr, const bool _active, const AddrSpace _addr_space)
{
    auto global_addr = get_global_addr(_addr, _addr_space);

    std::lock_guard<std::mutex> mlock(breakpoints_mutex);
    auto bp = breakpoints.find(global_addr);
    if (bp != breakpoints.end()) {
        breakpoints.erase(bp);
    }

    breakpoints.emplace(
      global_addr, std::move(Breakpoint(global_addr, _active)));
}

void Debug::del_breakpoint(const size_t _addr, const AddrSpace _addr_space)
{
    auto global_addr = get_global_addr(_addr, _addr_space);

    std::lock_guard<std::mutex> mlock(breakpoints_mutex);
    auto bp = breakpoints.find(global_addr);
    if (bp != breakpoints.end()) {
        breakpoints.erase(bp);
    }
}

auto Debug::watchpoints_find(const size_t global_addr) -> Watchpoints::iterator
{
    return std::find_if(watchpoints.begin(), watchpoints.end(),
      [global_addr](Watchpoint& w) { return w.check_addr(global_addr); });
}

void Debug::watchpoints_erase(const size_t global_addr)
{
    watchpoints.erase(std::remove_if(watchpoints.begin(), watchpoints.end(),
                        [global_addr](Watchpoint const& _w) {
                            return _w.get_global_addr() == global_addr;
                        }),
      watchpoints.end());
}

void Debug::add_watchpoint(const Watchpoint::Access _access, const size_t _addr,
  const Watchpoint::Condition _cond, const uint16_t _value,
  const size_t _value_size, const bool _active, const AddrSpace _addr_space)
{
    auto global_addr = get_global_addr(_addr, _addr_space);

    std::lock_guard<std::mutex> mlock(watchpoints_mutex);
    watchpoints.emplace_back(std::move(
      Watchpoint(_access, global_addr, _cond, _value, _value_size, _active)));
}

void Debug::del_watchpoint(const size_t _addr, const AddrSpace _addr_space)
{
    auto global_addr = get_global_addr(_addr, _addr_space);

    std::lock_guard<std::mutex> mlock(watchpoints_mutex);
    watchpoints_erase(global_addr);
}

bool Debug::check_breakpoints(const size_t _global_addr)
{
    std::lock_guard<std::mutex> mlock(breakpoints_mutex);
    auto bp = breakpoints.find(_global_addr);
    if (bp == breakpoints.end())
        return false;
    return bp->second.check();
}

bool Debug::check_watchpoint(const Watchpoint::Access _access,
  const size_t _global_addr, const uint8_t _value)
{
    std::lock_guard<std::mutex> mlock(watchpoints_mutex);
    auto wp = watchpoints_find(_global_addr);
    if (wp == watchpoints.end())
        return false;

    auto out = wp->check(_access, _global_addr, _value);

    if (out) {
        auto byte_l = memoryP->get_byte(_global_addr & 0xffff, false);
        auto byte_h = memoryP->get_byte((_global_addr + 1) & 0xffff, false);
        std::printf("wp break = true. addr: 0x%05lx, word: 0x%02x \n",
          _global_addr, byte_l | byte_h << 8);
    }
    return out;
}

void Debug::reset_watchpoints()
{
    std::lock_guard<std::mutex> mlock(watchpoints_mutex);
    for (auto& watchpoint : watchpoints) {
        watchpoint.reset();
    }
}

bool Debug::check_break()
{
    if (wp_break) {
        wp_break = false;
        print_watchpoints();
        reset_watchpoints();
        return true;
    }

    auto pc = i8080cpu::i8080_pc();
    auto global_addr = get_global_addr(pc, AddrSpace::CPU);

    auto break_ = check_breakpoints(global_addr);

    if (break_) {
        print_watchpoints();
    }

    return break_;
}

void Debug::trace_log_update(const size_t _global_addr, const uint8_t _val)
{
    //auto last_global_addr = trace_log[trace_log_idx].global_addr;
    //auto last_opcode = trace_log[trace_log_idx].opcode;

    trace_log_idx = (trace_log_idx - 1) % TRACE_LOG_SIZE;
    trace_log[trace_log_idx].global_addr = _global_addr;
    trace_log[trace_log_idx].opcode = _val;
    trace_log[trace_log_idx].data_l =
      memoryP->get_byte((_global_addr + 1) & 0xffff, false);
    trace_log[trace_log_idx].data_h =
      memoryP->get_byte((_global_addr + 2) & 0xffff, false);

    if (_val == OPCODE_PCHL) {
        uint16_t pc = i8080cpu::i8080_regs_hl();
        trace_log[trace_log_idx].data_l = pc & 0xff;
        trace_log[trace_log_idx].data_h = pc >> 8 & 0xff;
    } else {
        trace_log[trace_log_idx].data_l =
          memoryP->get_byte((_global_addr + 1) & 0xffff, false);
        trace_log[trace_log_idx].data_h =
          memoryP->get_byte((_global_addr + 2) & 0xffff, false);
    }
}

auto Debug::TraceLog::to_str() const -> std::string
{
    std::stringstream out;

    out << "0x";
    out << std::setw(5) << std::setfill('0');
    out << std::uppercase << std::hex << static_cast<int>(global_addr) << ": ";

    out << get_mnemonic(opcode, data_l, data_h, 13);

    return out.str();
}

void Debug::TraceLog::clear()
{
    global_addr = -1;
    opcode = 0;
    data_l = 0;
    data_h = 0;
}

auto Debug::trace_log_next_line(
  const int _idx_offset, const bool _reverse, const size_t _filter) const -> int
{
    size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;

    size_t idx = trace_log_idx + _idx_offset;
    size_t idx_last = trace_log_idx + TRACE_LOG_SIZE - 1;

    int dir = _reverse ? -1 : 1;
    // for a forward scrolling we need to go to the second line if we were not
    // exactly at the filtered line
    bool forward_second_search = false;
    size_t first_line_idx = idx;

    for (; idx >= trace_log_idx && idx <= idx_last; idx += dir) {
        if (get_opcode_type(trace_log[idx % TRACE_LOG_SIZE].opcode) <= filter) {
            if ((!_reverse && !forward_second_search) ||
                (_reverse && idx == first_line_idx && !forward_second_search)) {
                forward_second_search = true;
                continue;
            } else {
                return idx - trace_log_idx;
            }
        }
    }

    return _idx_offset; // fails to reach the next line
}

auto Debug::trace_log_nearest_forward_line(
  const size_t _idx, const size_t _filter) const -> int
{
    size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;

    size_t idx = _idx;
    size_t idx_last = trace_log_idx + TRACE_LOG_SIZE - 1;

    for (; idx >= trace_log_idx && idx <= idx_last; idx++) {
        if (get_opcode_type(trace_log[idx % TRACE_LOG_SIZE].opcode) <= filter) {
            return idx;
        }
    }

    return _idx; // fails to reach the nearest line
}

auto Debug::get_trace_log(
  const int _offset, const size_t _lines, const size_t _filter) -> std::string
{
    size_t filter = _filter > OPCODE_TYPE_MAX ? OPCODE_TYPE_MAX : _filter;
    size_t offset = _offset < 0 ? -_offset : _offset;

    std::string out;

    for (size_t i = 0; i < offset; i++) {
        trace_log_idx_view_offset =
          trace_log_next_line(trace_log_idx_view_offset, _offset < 0, filter);
    }

    size_t idx = trace_log_idx + trace_log_idx_view_offset;
    size_t idx_last = trace_log_idx + TRACE_LOG_SIZE - 1;
    size_t line = 0;

    size_t first_line_idx =
      trace_log_nearest_forward_line(trace_log_idx, filter);

    for (; idx <= idx_last && line < _lines; idx++) {
        if (trace_log[idx % TRACE_LOG_SIZE].global_addr < 0)
            break;

        if (get_opcode_type(trace_log[idx % TRACE_LOG_SIZE].opcode) <= filter) {
            std::string line_s;

            if (idx == first_line_idx) {
                line_s += ">";
            } else {
                line_s += " ";
            }
            line_s += trace_log[idx % TRACE_LOG_SIZE].to_str();

            const size_t operand_addr = trace_log[idx % TRACE_LOG_SIZE].data_h
                                          << 8 |
                                        trace_log[idx % TRACE_LOG_SIZE].data_l;

            if (labels.find(operand_addr) != labels.end()) {
                line_s += "(" + labels.at(operand_addr) + ")";
            }
            if (line != 0) {
                line_s += "\n";
            }

            out = line_s + out;

            line++;
        }
    }

    return out;
}

auto Debug::get_global_addr(size_t _addr, const AddrSpace _addr_space) const
  -> const size_t
{
    if (_addr_space != AddrSpace::GLOBAL) {
        bool stack_space = _addr_space == AddrSpace::STACK ? true : false;
        _addr = memoryP->bigram_select(_addr & 0xffff, stack_space);
    } else {
        _addr = _addr % GLOBAL_MEM_SIZE;
    }
    return _addr;
}

void Debug::set_labels(const char* _labels_c)
{
    labels.clear();
    char* labels_c = strdup(_labels_c);

    char* label_c = strtok(labels_c, " $\n");
    char* addr_c = strtok(nullptr, " $\n");

    int addr = 0;

    while (label_c != nullptr) {
        if (label_c == nullptr || addr_c == nullptr)
            break;
        if (utils::str2int(&addr, addr_c, 16) == utils::STR2INT_SUCCESS) {
            auto it = labels.find(addr);
            if (it == labels.end()) {
                labels[addr] = label_c;
            } else {
                labels[addr] += ", " + std::string(label_c);
            }
        }
        label_c = strtok(nullptr, " $\n");
        addr_c = strtok(nullptr, " $\n");
    }

    free(labels_c);
}

void Debug::print_breakpoints()
{
    std::printf("breakpoints:\n");
    std::lock_guard<std::mutex> mlock(breakpoints_mutex);
    if (breakpoints.size()) {
        for (const auto& [addr, bp] : breakpoints) {
            bp.print();
        }
    }
}

void Debug::print_watchpoints()
{
    std::lock_guard<std::mutex> mlock(watchpoints_mutex);
    if (watchpoints.size()) {
        std::printf("watchpoints:\n");
        for (const auto& wp : watchpoints) {
            wp.print();
        }
    }
}

auto Debug::Breakpoint::is_active() const -> const bool
{
    return active;
}
auto Debug::Breakpoint::check() const -> const bool
{
    return active;
}

void Debug::Breakpoint::print() const
{
    std::printf("0x%06lx, active: %d \n", global_addr, active);
}

auto Debug::Watchpoint::is_active() const -> const bool
{
    return active;
}

auto Debug::Watchpoint::check(const Watchpoint::Access _access,
  const size_t _global_addr, const uint8_t _value) -> const bool
{
    if (!active)
        return false;
    if (access != Access::RW && access != _access)
        return false;

    if (break_l && (value_size == VAL_BYTE_SIZE || break_h))
        return true;

    bool* break_p;
    uint8_t value_byte;

    if (_global_addr == global_addr) {
        break_p = &break_l;
        value_byte = value & 0xff;
    } else {
        break_p = &break_h;
        value_byte = value >> 8 & 0xff;
    }

    switch (cond) {
        case Condition::ANY:
            *break_p = true;
            break;
        case Condition::EQU:
            *break_p = _value == value_byte;
            break;
        case Condition::LESS:
            *break_p = _value < value_byte;
            break;
        case Condition::GREATER:
            *break_p = _value > value_byte;
            break;
        case Condition::LESS_EQU:
            *break_p = _value <= value_byte;
            break;
        case Condition::GREATER_EQU:
            *break_p = _value >= value_byte;
            break;
        case Condition::NOT_EQU:
            *break_p = _value != value_byte;
            break;
        default:
            return false;
    };

    return break_l && (value_size == VAL_BYTE_SIZE || break_h);
}

auto Debug::Watchpoint::get_global_addr() const -> const size_t
{
    return global_addr;
}

auto Debug::Watchpoint::check_addr(const size_t _global_addr) const -> const
  bool
{
    return _global_addr == global_addr ||
           (_global_addr == global_addr + 1 && value_size == VAL_WORD_SIZE);
}

void Debug::Watchpoint::reset()
{
    break_l = false;
    break_h = false;
}

void Debug::Watchpoint::print() const
{
    std::printf("0x%05lx, access: %s, cond: %s, value: 0x%04x, value_size: %ld, "
                "active: %d \n",
      global_addr, access_s[static_cast<size_t>(access)],
      conditions_s[static_cast<size_t>(cond)], value, value_size, active);
}
