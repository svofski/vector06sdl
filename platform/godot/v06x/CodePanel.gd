extends TextEdit

func _ready():
	add_color_region(" 0x", ":", Color(0.437789, 0.714844, 0.435608))
	add_color_region(">", "", Color(1, 0.968627, 0.129412))
	var key_words = [
		'NOP', 
		"XTHL", "PCHL", "XCHG", "SPHL",
		"LXI",
		"LHLD", "SHLD",
		"LDA", "STA", 
		"LDAX", "STAX",
		"INR", "INX", "DCR", "DCX",
		"MVI", "MOV",
		"RLC", "RRC", "RAL", "RAR",
		"DB", 
		"DAD", "DAA", "CMA", "STC", "CMC", "HLT", "ADD", "ADC", "SUB", "SBB", "ANA", "XRA", "ORA", "CMP", "ADI", "ACI", "SUI", "SBI", "ANI", "XRI", "ORI", "CPI", 
		"RNZ", 
		"POP", "PUSH",
		"JNZ", "JMP", "JZ", "JNC", "JC", "JPO", "JPE", "JP", "JM",
		"CNZ", "CZ",  "CALL", "CNC", "CC", "CPO", "CPE", "CP", "CM",
		"RZ", "RET", "RNC", "RC", "RPO", "RPE", "RP", "RM",
		"RST", "OUT", "IN", "DI", "EI"]
	for k_word in key_words:
		add_keyword_color(k_word, Color(0.539063, 0.722082, 1))
