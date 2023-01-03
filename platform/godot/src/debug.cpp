#include "debug.h"

#include <string>
#include <iomanip>

Debug::Debug(Board* _boardP, Memory* _memoryP)
: mem_runs(), mem_reads(), mem_writes(), memoryP(_memoryP)
{
	auto read_func =
		[this](const size_t _addr)
		{
			this->read(_addr);
		};

	auto write_func =
		[this](const size_t _addr)
		{
			this->write(_addr);
		};

	auto run_func =
		[this](const size_t _addr)
		{
			this->run(_addr);
		};

	_boardP->debug_on_single_step = run_func;
	_memoryP->debug_onread = read_func;
	_memoryP->debug_onwrite = write_func;
}

void Debug::run(const size_t _addr)
{
	mem_runs[_addr % MEMORY_SIZE]++;
}

void Debug::read(const size_t _addr)
{
	mem_reads[_addr % MEMORY_SIZE]++;
}

void Debug::write(const size_t _addr)
{
	mem_writes[_addr % MEMORY_SIZE]++;
}


 /*
 *  Based on original code by:
 *  Juergen Buchmueller (MESS project, BSD-3-clause licence)
 *  and
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017
 */
static const char* mnemonics[0x100] =
{
	"NOP",    "LXI B,",  "STAX B", "INX B",  "INR B", "DCR B", "MVI B,", "RLC", "DB 08H", "DAD B",  "LDAX B", "DCX B",  "INR C", "DCR C", "MVI C,", "RRC",
	"DB 10H", "LXI D,",  "STAX D", "INX D",  "INR D", "DCR D", "MVI D,", "RAL", "DB 18H", "DAD D",  "LDAX D", "DCX D",  "INR E", "DCR E", "MVI E,", "RAR",
	"DB 20H", "LXI H,",  "SHLD",   "INX H",  "INR H", "DCR H", "MVI H,", "DAA", "DB 28H", "DAD H",  "LHLD",   "DCX H",  "INR L", "DCR L", "MVI L,", "CMA",
	"DB 30H", "LXI SP,", "STA",    "INX SP", "INR M", "DCR M", "MVI M,", "STC", "DB 38H", "DAD SP", "LDA",    "DCX SP", "INR A", "DCR A", "MVI A,", "CMC",

	"MOV B, B", "MOV B, C", "MOV B, D", "MOV B, E", "MOV B, H", "MOV B, L", "MOV B, M", "MOV B, A", "MOV C, B", "MOV C, C", "MOV C, D", "MOV C, E", "MOV C, H", "MOV C, L", "MOV C, M", "MOV C, A",
	"MOV D, B", "MOV D, C", "MOV D, D", "MOV D, E", "MOV D, H", "MOV D, L", "MOV D, M", "MOV D, A", "MOV E, B", "MOV E, C", "MOV E, D", "MOV E, E", "MOV E, H", "MOV E, L", "MOV E, M", "MOV E, A",
	"MOV H, B", "MOV H, C", "MOV H, D", "MOV H, E", "MOV H, H", "MOV H, L", "MOV H, M", "MOV H, A", "MOV L, B", "MOV L, C", "MOV L, D", "MOV L, E", "MOV L, H", "MOV L, L", "MOV L, M", "MOV L, A",
	"MOV M, B", "MOV M, C", "MOV M, D", "MOV M, E", "MOV M, H", "MOV M, L", "HLT",      "MOV M, A", "MOV A, B", "MOV A, C", "MOV A, D", "MOV A, E", "MOV A, H", "MOV A, L", "MOV A, M", "MOV A, A",

	"ADD B", "ADD C", "ADD D", "ADD E", "ADD H", "ADD L", "ADD M", "ADD A", "ADC B", "ADC C", "ADC D", "ADC E", "ADC H", "ADC L", "ADC M", "ADC A",
	"SUB B", "SUB C", "SUB D", "SUB E", "SUB H", "SUB L", "SUB M", "SUB A", "SBB B", "SBB C", "SBB D", "SBB E", "SBB H", "SBB L", "SBB M", "SBB A",
	"ANA B", "ANA C", "ANA D", "ANA E", "ANA H", "ANA L", "ANA M", "ANA A", "XRA B", "XRA C", "XRA D", "XRA E", "XRA H", "XRA L", "XRA M", "XRA A",
	"ORA B", "ORA C", "ORA D", "ORA E", "ORA H", "ORA L", "ORA M", "ORA A", "CMP B", "CMP C", "CMP D", "CMP E", "CMP H", "CMP L", "CMP M", "CMP A",

	"RNZ", "POP B",   "JNZ", "JMP",  "CNZ", "PUSH B",   "ADI", "RST 0", "RZ",  "RET",     "JZ",  "DB CBH",  "CZ",  "CALL",   "ACI", "RST 1",
	"RNC", "POP D",   "JNC", "OUT",  "CNC", "PUSH D",   "SUI", "RST 2", "RC",  "DB D9H",  "JC",  "IN",      "CC",  "DB DDH", "SBI", "RST 3",
	"RPO", "POP H",   "JPO", "XTHL", "CPO", "PUSH H",   "ANI", "RST 4", "RPE", "PCHL",    "JPE", "XCHG",    "CPE", "DB EDH", "XRI", "RST 5",
	"RP",  "POP PSW", "JP",  "DI",   "CP",  "PUSH PSW", "ORI", "RST 6", "RM",  "SPHL",    "JM",  "EI",      "CM",  "DB FDH", "CPI", "RST 7"
};


#define CMD_BYTES_MAX 3

static const uint8_t cmd_lens[0x100] =
{
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,3,3,3,1,2,1,1,1,3,3,3,3,2,1,
	1,1,3,2,3,1,2,1,1,1,3,2,3,3,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1,
	1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1
};

auto Debug::get_cmd_len(const uint8_t _addr) const -> const size_t
{
	return cmd_lens[_addr];
}

static size_t cmd[3];
#define CMD_OPCODE cmd[0]
#define CMD_OP_L cmd[1]
#define CMD_OP_H cmd[2]
#define MAX_DATA_CHR_LEN 11
#define MAX_CMD_LEN 13

auto Debug::get_mnemonic(const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const
->const std::string
{
	CMD_OPCODE = _opcode;
	CMD_OP_L = _data_l;
	CMD_OP_H = _data_h;

	std::stringstream out;
	out << mnemonics[_opcode];

	if (cmd_lens[_opcode] == 2)
	{
		out << " " << std::setw(sizeof(uint8_t)*2) << std::setfill('0');
		out << std::uppercase << std::hex << static_cast<int>(_data_l);
	}
	else if (cmd_lens[_opcode] == 3)
	{
		auto data_w = ((uint16_t)_data_h << 8) + _data_l;
		out << " " << std::setw(sizeof(uint16_t)*2) << std::setfill('0');
		out << std::uppercase << std::hex << static_cast<int>(data_w);
	}

	int i = out.str().size();
	for(; i < MAX_CMD_LEN; i++)
	{
			out << " ";
	}

	return out.str();
}

auto Debug::get_disasm_db_line(const size_t _addr, const uint8_t _data) const
->const std::string
{
	std::stringstream out;

	out << "0x";
	out << std::setw(sizeof(uint16_t)*2) << std::setfill('0');
	out << std::uppercase << std::hex << static_cast<int>(_addr) << ":";

	// print data in a format " %02X"
	out << " ";
	out << std::setw(sizeof(uint8_t)*2) << std::setfill('0');
	out << std::uppercase << std::hex << static_cast<int>(_data);

	// add whitespaces at the end of data
	for(int i = 3; i < MAX_DATA_CHR_LEN; i++)
	{
		out << " ";
	}

	out << "DB ";
	out << std::setw(sizeof(uint8_t)*2) << std::setfill('0');
	out << std::uppercase << std::hex << static_cast<int>(_data);

	return out.str();
}

auto Debug::get_disasm_line(const size_t _addr, const uint8_t _opcode, const uint8_t _data_l, const uint8_t _data_h) const
->const std::string
{
	CMD_OPCODE = _opcode;
	CMD_OP_L = _data_l;
	CMD_OP_H = _data_h;

	auto cmd_len = cmd_lens[_opcode];
	std::stringstream out;

	out << "0x";
	out << std::setw(sizeof(uint16_t)*2) << std::setfill('0');
	out << std::uppercase << std::hex << static_cast<int>(_addr) << ":";

	// print data in a format " %02X %02X %02X "
	int i = 0;
	for(; i < cmd_len; i++)
	{
		out << " ";
		out << std::setw(sizeof(uint8_t)*2) << std::setfill('0');
		out << std::uppercase << std::hex << static_cast<int>(cmd[i]);
	}
	// add whitespaces at the end of data
	i *= 3; // every byte takes three chars " %02X"
	for(; i < MAX_DATA_CHR_LEN; i++)
	{
		out << " ";
	}

	out << get_mnemonic(_opcode, _data_l, _data_h);

	return out.str();
}

size_t Debug::get_addr(const size_t _end_addr, const size_t _before_addr_lines) const
{
	if (_before_addr_lines == 0) return _end_addr;

	size_t start_addr = (_end_addr - _before_addr_lines * CMD_BYTES_MAX) & 0xffff;
	#define MAX_ATTEMPTS 41

	int lines = 0;
	int addr_diff_max = _before_addr_lines * CMD_BYTES_MAX + 1;
	size_t addr = start_addr & 0xffff;

	for(int attempt = MAX_ATTEMPTS; attempt > 0 && lines != _before_addr_lines; attempt--)
	{
		addr = start_addr & 0xffff;
		int addr_diff = addr_diff_max;
		lines = 0;

		while(addr_diff > 0 && addr != _end_addr)
		{
			auto opcode = memoryP->get_byte(addr, false);
			auto cmd_len = get_cmd_len(opcode);
			addr = (addr + cmd_len) & 0xffff;
			addr_diff -= cmd_len;
			lines++;
		}
		
		if (addr == _end_addr && lines == _before_addr_lines)
		{
			return start_addr;
		}

		start_addr++;
		addr_diff_max--;
	}
	return _end_addr;
}

auto Debug::disasm(const size_t _addr, const size_t _lines, const size_t _before_addr_lines) const
->std::string
{
	std::string out;
	if (_lines == 0) return out;

	
	size_t addr = _addr;
	auto pc = i8080cpu::i8080_pc();
	int lines = _lines;

	if (_before_addr_lines > 0)
	{
		addr = get_addr(_addr & 0xffff, _before_addr_lines) & 0xffff;

		if (addr == _addr)
		{
			addr = (addr - _before_addr_lines) & 0xffff;

			lines = _lines - _before_addr_lines;

			for (int i=0; i < _before_addr_lines; i++)
			{
				if (addr == pc) 
				{
					out += ">";
				}
				else
				{
					out += " ";
				}
				auto db = memoryP->get_byte(addr, false);
				out += get_disasm_db_line(addr, db);

				if (lines > 0 || i != _before_addr_lines-1)
				{
					out += "\n";
				}
				
				addr = (addr + 1) & 0xffff;
			}
		}
	}

	for (int i=0; i < lines; i++)
	{
		if (addr == pc) 
		{
			out += ">";
		}
		else
		{
			out += " ";
		}

		auto opcode = memoryP->get_byte(addr, false);
		auto data_l = memoryP->get_byte((addr+1) & 0xffff, false);
		auto data_h = memoryP->get_byte((addr+2) & 0xffff, false);

		uint32_t bigaddr = memoryP->bigram_select(addr, false);

		std::string runsS = std::to_string(mem_runs[bigaddr]);
		std::string readsS = std::to_string(mem_reads[bigaddr]);
		std::string writesS = std::to_string(mem_writes[bigaddr]);

		std::string runsS_readsS_writesS = "(" + runsS + "," + readsS + "," + writesS + ")";

		out += get_disasm_line(addr, opcode, data_l, data_h);
		out += runsS_readsS_writesS;
		if (i != lines-1)
		{
			out += "\n";
		}
		
		addr = (addr + get_cmd_len(opcode)) & 0xffff;
	}
	return out;
}

void Debug::reset()
{
    std::fill(mem_runs, mem_runs + MEMORY_SIZE, 0);
	std::fill(mem_reads, mem_reads + MEMORY_SIZE, 0);
	std::fill(mem_writes, mem_writes + MEMORY_SIZE, 0);
}