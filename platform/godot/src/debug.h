#pragma once
#include "..\..\..\src\board.h"
#include <memory>

class Debug
{
	static const constexpr size_t MEM_BANK_SIZE = 0xffff;
	static const constexpr size_t RAM_SIZE = MEM_BANK_SIZE;
	static const constexpr size_t RAM_DISK_SIZE = MEM_BANK_SIZE * 4;
	static const constexpr size_t MEMORY_SIZE = RAM_SIZE + RAM_DISK_SIZE;

public:
	Debug(Board* _boardP, Memory* _memoryP);
	void run(const size_t _addr);
	void read(const size_t _addr);
	void write(const size_t _addr);
	auto disasm(const size_t _addr, const size_t _lines, const size_t _before_addr_lines) const
	->std::string;

private:
    const std::string get_mnemonic(const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const;
	const std::string get_disasm_line(const size_t _addr, const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const;
	auto get_cmd_len(const uint8_t _addr) const -> const size_t;
	size_t get_addr(const size_t _end_addr, const size_t _before_addr_lines) const;
	uint64_t mem_runs[MEMORY_SIZE];
	uint64_t mem_reads[MEMORY_SIZE];
	uint64_t mem_writes[MEMORY_SIZE];
	Memory* memoryP;
};
