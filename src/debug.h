#pragma once
#include "memory.h"
#include <memory>
#include <mutex>
#include <map>
#include <vector>

class Debug
{
  public:
    class Breakpoint
    {
      public:
        Breakpoint(const size_t _global_addr, const bool _active = true)
          : global_addr(_global_addr)
          , active(_active)
        {}
        auto check() const -> const bool;
        auto is_active() const -> const bool;
        void print() const;

      private:
        size_t global_addr;
        bool active;
    };

    class Watchpoint
    {
      public:
        enum class Access : size_t
        {
            R = 0,
            W,
            RW,
            COUNT
        };
        static constexpr const char* access_s[] = { "R-", "-W", "RW" };
        static constexpr size_t VAL_BYTE_SIZE = sizeof(uint8_t);
        static constexpr size_t VAL_WORD_SIZE = sizeof(uint16_t);
        static constexpr size_t VAL_MAX_SIZE = VAL_WORD_SIZE;

        enum class Condition : size_t
        {
            ANY = 0,
            EQU,
            LESS,
            GREATER,
            LESS_EQU,
            GREATER_EQU,
            NOT_EQU,
            COUNT
        };
        static constexpr const char* conditions_s[] = { "ANY", "==", "<", ">",
            "<=", ">=", "!=" };

        Watchpoint(const Access _access, const size_t _global_addr,
          const Condition _cond, const uint16_t _value,
          const size_t _value_size = VAL_BYTE_SIZE, const bool _active = true)
          : access(static_cast<Debug::Watchpoint::Access>(
              (size_t)_access % (size_t)Access::COUNT))
          , global_addr(_global_addr)
          , cond(static_cast<Debug::Watchpoint::Condition>(
              (size_t)_cond & (size_t)Condition::COUNT))
          , value(_value & 0xffff)
          , value_size(_value_size)
          , active(_active)
          , break_l(false)
          , break_h(false)
        {}
        auto check(const Watchpoint::Access _access, const size_t _global_addr,
          const uint8_t _value) -> const bool;
        auto is_active() const -> const bool;
        auto get_global_addr() const -> const size_t;
        auto check_addr(const size_t _global_addr) const -> const bool;
        void reset();
        void print() const;

      private:
        Access access;
        size_t global_addr;
        Condition cond;
        uint16_t value;
        size_t value_size;
        bool active;
        bool break_l;
        bool break_h;
    };
    using Watchpoints = std::vector<Debug::Watchpoint>;

    static const constexpr size_t MEM_BANK_SIZE = 0x10000;
    static const constexpr size_t RAM_SIZE = MEM_BANK_SIZE;
    static const constexpr size_t RAM_DISK_SIZE = MEM_BANK_SIZE * 4;
    static const constexpr size_t GLOBAL_MEM_SIZE = RAM_SIZE + RAM_DISK_SIZE;
    static const constexpr size_t TRACE_LOG_SIZE = 100000;

    enum AddrSpace : size_t
    {
        CPU = 0, // cpu adressed space. range: [0x0000, 0xffff]
        STACK, // cpu adressed space via stack commands: xthl, push, pop. range:
               // 0x0000 - 0xFFFF accessed
        GLOBAL // flat virtual addr space (ram + ram-disk). range: [0x0000,
               // 0x4ffff]
    };

    Debug(Memory* _memory);
    void read(
      const size_t _global_addr, const uint8_t _val, const bool _is_opcode);
    void write(const size_t _global_addr, const uint8_t _val);
    auto get_disasm(const size_t _addr, const size_t _lines,
      const size_t _before_addr_lines) const -> std::string;
    void reset();
    void serialize(std::vector<uint8_t>& to);
    void deserialize(std::vector<uint8_t>::iterator it, size_t size);

    void add_breakpoint(const size_t _addr, const bool _active = true,
      const AddrSpace _addr_space = AddrSpace::CPU);
    void del_breakpoint(
      const size_t _addr, const AddrSpace _addr_space = AddrSpace::CPU);
    void add_watchpoint(const Watchpoint::Access _access, const size_t _addr,
      const Watchpoint::Condition _cond, const uint16_t _value,
      const size_t _value_size = 1, const bool _active = true,
      const AddrSpace _addr_space = AddrSpace::CPU);
    void del_watchpoint(
      const size_t _addr, const AddrSpace _addr_space = AddrSpace::CPU);
    void reset_watchpoints();
    void print_breakpoints();
    void print_watchpoints();
    auto get_global_addr(size_t _addr, const AddrSpace _addr_space) const
      -> const size_t;
    bool check_breakpoints(const size_t _global_addr);
    bool check_watchpoint(const Watchpoint::Access _access,
      const size_t _global_addr, const uint8_t _value);
    bool check_break();
    auto get_trace_log(const int _offset, const size_t _lines,
      const size_t _filter) -> std::string;
    void set_labels(const char* _labels_c);

  private:
    auto get_disasm_line(const size_t _addr, const uint8_t _opcode,
      const uint8_t _data_l, const uint8_t _data_h) const -> const std::string;
    auto get_disasm_db_line(const size_t _addr, const uint8_t _data) const
      -> const std::string;
    auto get_cmd_len(const uint8_t _addr) const -> const size_t;
    auto get_addr(const size_t _end_addr, const size_t _before_addr_lines) const
      -> size_t;
    auto watchpoints_find(const size_t global_addr) -> Watchpoints::iterator;
    void watchpoints_erase(const size_t global_addr);

    void trace_log_update(const size_t _global_addr, const uint8_t _val);
    auto trace_log_next_line(const int _idx_offset, const bool _reverse,
      const size_t _filter) const -> int;
    auto trace_log_nearest_forward_line(
      const size_t _idx_offset, const size_t _filter) const -> int;

    uint64_t mem_runs[GLOBAL_MEM_SIZE];
    uint64_t mem_reads[GLOBAL_MEM_SIZE];
    uint64_t mem_writes[GLOBAL_MEM_SIZE];

    struct TraceLog
    {
        int64_t global_addr;
        uint8_t opcode;
        uint8_t data_l;
        uint8_t data_h;

        auto to_str() const -> std::string;
        void clear();
    };
    TraceLog trace_log[TRACE_LOG_SIZE];
    size_t trace_log_idx = 0;
    int trace_log_idx_view_offset = 0;

    std::map<size_t, std::string> labels;

    Memory* memoryP;

    std::mutex breakpoints_mutex;
    std::mutex watchpoints_mutex;
    std::map<size_t, Debug::Breakpoint> breakpoints;
    Watchpoints watchpoints;
    bool wp_break;
};
