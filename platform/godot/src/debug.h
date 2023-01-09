#pragma once
#include "..\..\..\src\memory.h"
#include <memory>
#include <mutex>
#include <map>

class Debug
{
public:	
	class Breakpoint
	{
	public:

		Breakpoint(const size_t _global_addr, const bool _active = true)
		: global_addr(_global_addr), active(_active)
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
		enum class Access : size_t {R = 0, W, RW, COUNT};
		static constexpr const char* access_s[] = {"R-", "-W", "RW"};

		enum class Condition : size_t {ANY = 0, EQU, LESS, GREATER, LESS_EQU, GREATER_EQU, NOT_EQU, COUNT};
		static constexpr const char* conditions_s[] = {"ANY", "==", "<", ">", "<=", ">=", "!="};

		Watchpoint(const Access _access, const size_t _global_addr, const Condition _cond, const uint8_t _value, const bool _active = true)
		: access(static_cast<Debug::Watchpoint::Access>((size_t)_access % (size_t)Access::COUNT)), global_addr(_global_addr), cond(static_cast<Debug::Watchpoint::Condition>((size_t)_cond & (size_t)Condition::COUNT)), value(_value & 0xff), active(_active)
		{}
		auto check(const Watchpoint::Access _access, const uint8_t _value) const -> const bool;
		auto is_active() const -> const bool;
		void print() const;

	private:
		Access access;
		size_t global_addr;
		Condition cond;
		uint8_t value;
		bool active;
	};

	static const constexpr size_t MEM_BANK_SIZE	= 0x10000;
	static const constexpr size_t RAM_SIZE		= MEM_BANK_SIZE;
	static const constexpr size_t RAM_DISK_SIZE	= MEM_BANK_SIZE * 4;
	static const constexpr size_t GLOBAL_MEM_SIZE	= RAM_SIZE + RAM_DISK_SIZE;


	enum AddrSpace : size_t
	{
		CPU = 0, // range: [0x0000, 0xffff]
		STACK, // range: 0x0000 - 0xFFFF accessed via stack commands: xthl, push, pop
		GLOBAL // range: [0x0000, 0xffff] * 5 (ram + ram-disk)
	};

	Debug(Memory* _memory);
	void read(const size_t _global_addr, const uint8_t _val, const bool _run);
	void write(const size_t _global_addr, const uint8_t _val);
	auto disasm(const size_t _addr, const size_t _lines, const size_t _before_addr_lines) const ->std::string;
	void reset();
	void serialize(std::vector<uint8_t> &to);
	void deserialize(std::vector<uint8_t>::iterator it, size_t size);

	void add_breakpoint(const size_t _addr, const bool _active = true, const AddrSpace _addr_space = AddrSpace::CPU);
	void del_breakpoint(const size_t _addr, const AddrSpace _addr_space = AddrSpace::CPU);
	void add_watchpoint(const Watchpoint::Access _access, const size_t _addr, const Watchpoint::Condition _cond, const uint8_t _value, const bool _active = true, const AddrSpace _addr_space = AddrSpace::CPU);
	void del_watchpoint(const size_t _addr, const AddrSpace _addr_space = AddrSpace::CPU);
	void print_breakpoints();
	void print_watchpoints();

	bool check_breakpoints(const size_t _global_addr);
	bool check_watchpoint(const Watchpoint::Access _access, const size_t _global_addr, const uint8_t _value);
	bool check_break();

private:
	auto get_mnemonic(const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const -> const std::string;
	auto get_disasm_line(const size_t _addr, const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const ->const std::string ;
	auto get_disasm_db_line(const size_t _addr, const uint8_t _data) const ->const std::string;
	auto get_cmd_len(const uint8_t _addr) const -> const size_t;
	auto get_addr(const size_t _end_addr, const size_t _before_addr_lines) const -> size_t;
	auto get_breakpoint_global_addr(size_t _addr, const AddrSpace _addr_space) const -> const size_t;
	auto get_watchpoint_global_addr(size_t _addr, const AddrSpace _addr_space) const -> const size_t;

private:
	//uint8_t  mem[GLOBAL_MEM_SIZE];
	uint64_t mem_runs[GLOBAL_MEM_SIZE];
	uint64_t mem_reads[GLOBAL_MEM_SIZE];
	uint64_t mem_writes[GLOBAL_MEM_SIZE];

	Memory* memoryP;

	std::mutex breakpoints_mutex;
	std::mutex watchpoints_mutex;
	std::map<size_t, Debug::Breakpoint> breakpoints;
	std::map<size_t, Debug::Watchpoint> watchpoints;
	bool wp_break;
};
