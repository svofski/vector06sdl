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
	void execute(const size_t _addr);
	void read(const size_t _addr);
	void write(const size_t _addr);

private:
	uint8_t mem_runs[MEMORY_SIZE];
	uint8_t mem_reads[MEMORY_SIZE];
	uint8_t mem_writes[MEMORY_SIZE];
};
