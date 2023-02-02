extends Panel

var main : Control

var break_icon_tex = preload("res://assets/debug_pause.png")
var cont_icon_tex = preload("res://assets/debug_pause_cont.png")

onready var code_panel = find_node("code_panel")
onready var stack_text_panel = find_node("stack_text_panel")
onready var reg_text_panel = find_node("reg_text_panel")
onready var flag_text_panel = find_node("flag_text_panel")
onready var hw_text_panel = find_node("hw_text_panel")
onready var breakpoints_list_panel = find_node("breakpoints_list_panel")
onready var watchpoints_panel = find_node("watchpoints_panel")
onready var watchpoints_list_panel = find_node("watchpoints_list_panel")
onready var watchpoint_popup = find_node("watchpoint_popup")
onready var wp_access_r = find_node("wp_access_r")
onready var wp_access_w = find_node("wp_access_w")
onready var wp_addr = find_node("wp_addr")
onready var wp_cond = find_node("wp_cond")
onready var wp_value = find_node("wp_value")
onready var wp_active = find_node("wp_active")
onready var wp_addr_space_cpu = find_node("wp_addr_space_cpu")
onready var wp_addr_space_stack = find_node("wp_addr_space_stack")
onready var wp_addr_space_global = find_node("wp_addr_space_global")
onready var wp_value_size = find_node("wp_value_size")

onready var trace_log_panel = find_node("trace_log_panel")
onready var trace_log_filter = find_node("trace_log_filter")
onready var trace_log = find_node("trace_log")

onready var break_cont = find_node("break_cont")
onready var step_over = find_node("step_over")
onready var step_into = find_node("step_into")
onready var step_out = find_node("step_out")

onready var main_bar = find_node("main_bar")
onready var search_panel = find_node("search_panel")

enum CODE_PANEL_MENU_ID {
	CURRENT_BREAK,
	REMOVE_ALL_BRKS,
	RUN_CURSOR,
	ADD_REMOVE_BRK,
	ADD_WP,
	REMOVE_ALL_WPS,
}

enum WATCHPOINTS_ACCESS {
	R = 0,
	W = 1,
	RW = 2
}
var WATCHPOINTS_ACCESS_S = [
	"R-",
	"-W",
	"RW"
]

enum WATCHPOINTS_COND {
	ANY = 0,
	EQU,
	LESS,
	GREATER,
	LESS_EQU,
	GREATER_EQU,
	NOT_EQU
}
var WATCHPOINTS_COND_S = [
	"ANY",
	"==",
	"<",
	">",
	"<=",
	">=",
	"!=",
]

enum BW_POINTS_ADDR_SPACE {
	CPU = 0,
	STACK,
	GLOBAL
}

var BW_POINTS_ADDR_SPACE_S = [
	"CPU",
	"STACK",
	"GLOBAL"
]

var TRACE_LOG_FILTER_LIST = [
	"call",
	"+ c*",
	"+ rst", 
	"+ pchl",
	"+ jmp",
	"+ j*",
	"+ ret, r*",
	"all"
]

onready var code_panel_menu = code_panel.get_menu()

const STACK_TEXT_LINES = 6
const CODE_PANEL_LINES_AHEAD = 6
const BREAKPOINT_AUTO_POSTFIX = ":A"

var code_panel_cursor_line_last = 0
var trace_log_cursor_line_last = -1

const OPCODE_CALL_LEN = 3
const OPCODE_CNZ	= 0xC4
const OPCODE_CZ		= 0xCC
const OPCODE_CALL	= 0xCD
const OPCODE_CNC	= 0xD4
const OPCODE_CC		= 0xDC
const OPCODE_CPO	= 0xE4
const OPCODE_CPE	= 0xEC
const OPCODE_CP		= 0xF4
const OPCODE_CM		= 0xFC
const step_over_cmds = [OPCODE_CNZ, OPCODE_CZ, OPCODE_CALL, OPCODE_CNC, OPCODE_CC, OPCODE_CPO, OPCODE_CPE, OPCODE_CP, OPCODE_CM]

var last_total_v_cycles = 0

var debug_labels = {}
const DEBUG_LABELS_FILE_NAME_RETROASSEMBLER = "debug.txt"
const DEBUG_LABELS_FILE_NAME_TASM_EXT = "sym"

var last_searchs = []
var last_search_idx = 0

const ADDR_SPACE_CPU = 0
const ADDR_SPACE_STACK = 1
const ADDR_SPACE_GLOBAL = 2

# Called when the node enters the scene tree for the first time.
func _ready():
	code_panel_menu.add_item("show the current break", CODE_PANEL_MENU_ID.CURRENT_BREAK)
	code_panel_menu.add_item("run to the selected line", CODE_PANEL_MENU_ID.RUN_CURSOR)
	code_panel_menu.add_item("add/remove breakpoint", CODE_PANEL_MENU_ID.ADD_REMOVE_BRK)
	code_panel_menu.add_item("remove all breakpoints", CODE_PANEL_MENU_ID.REMOVE_ALL_BRKS)
	code_panel_menu.add_item("add watchpoint", CODE_PANEL_MENU_ID.ADD_WP)
	code_panel_menu.add_item("remove all watchpoints", CODE_PANEL_MENU_ID.REMOVE_ALL_WPS)
	code_panel_menu.connect("id_pressed", self, "code_panel_menu_id_pressed")
	
	for item in TRACE_LOG_FILTER_LIST:
		trace_log_filter.add_item(item)

	set_ui_on_cont()
	
func debug_panel_size_update():
	if main.debug_is_ui_break():
		var addrS = asm_line_to_addr(code_panel.get_line(0))
		codePanel_scroll_to_addr(addrS.hex_to_int(), 0)
		trace_log_update(true)
	code_panel.margin_bottom = self.margin_bottom
	code_panel.cursor_set_line(code_panel_cursor_line_last)
	trace_log_panel.margin_right = self.margin_right
	trace_log.margin_bottom = self.margin_bottom	
	trace_log.margin_right = self.margin_right
	trace_log.cursor_set_line(trace_log_cursor_line_last)
	watchpoints_panel.margin_right = margin_right
	watchpoints_list_panel.rect_size.x = watchpoints_panel.rect_size.x * 2 - 8
	
func _on_debug_panel_item_rect_changed():
	debug_panel_size_update()
func _on_debug_panel_resized():
	debug_panel_size_update()
func _on_debug_panel_focus_exited():
	debug_panel_size_update()

func set_ui_on_break():
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.CURRENT_BREAK), false)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.REMOVE_ALL_BRKS), false)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.RUN_CURSOR), false)
		
		break_cont.icon = cont_icon_tex
		regTextPanel_update(true)
		flag_text_panel_update(true)
		stack_text_panel_update(true)
		code_panel_update(true)
		breakpoints_list_panel_update(true)
		watchpoints_list_panel_update(true)
		hw_text_panel_update(true)
		trace_log_update(true)
		trace_log_filter.disabled = false
		step_over.disabled = false;
		step_into.disabled = false;
		step_out.disabled = false;
		search_panel.editable = true;
		
func set_ui_on_cont():
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.CURRENT_BREAK), true)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.REMOVE_ALL_BRKS), true)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID.RUN_CURSOR), true)
		break_cont.icon = break_icon_tex
		regTextPanel_update(false)
		flag_text_panel_update(false)
		stack_text_panel_update(false)
		code_panel_update(false)
		breakpoints_list_panel_update(false)
		watchpoints_list_panel_update(false)
		hw_text_panel_update(false)
		trace_log_update(false)
		trace_log_filter.disabled = true
		step_over.disabled = true;
		step_into.disabled = true;
		step_out.disabled = true;
		search_panel.editable = false;

func _on_break_cont_pressed():
	if main.debug_is_ui_break():
		set_ui_on_cont()
		# to pass the breakpoint on the current addr
		main.debug_step_into()
		main.debug_continue()
	else:
		set_ui_on_break()
		main.debug_break()

func _on_restart_pressed():
	last_total_v_cycles = 0
	main.reload_file()

#var step_into_pressed = 0
func _on_step_into_pressed():
	#print("TEST PRINT: %s gd: step_into processing started." % Time.get_time_string_from_system())
	main.debug_step_into()
	set_ui_on_break()
	#step_into_pressed=step_into_pressed+1
	#print("TEST PRINT: %s gd: step_into done. pressed=%d" % [Time.get_time_string_from_system(), step_into_pressed])

func code_panel_update(enabled):
	if enabled:
		code_panel.syntax_highlighting = true
		code_panel.add_color_override("current_line_color", Color(0.248718, 0.390625, 0.257319))
		var pc = get_reg_pc()
		codePanel_scroll_to_addr(pc)
		code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	else:
		code_panel.syntax_highlighting = false
		code_panel.add_color_override("current_line_color", Color(0.351563, 0.351563, 0.351563))
		code_panel.remove_breakpoints()

func trace_log_update(enabled):
	if enabled:
		trace_log.syntax_highlighting = true
		trace_log.add_color_override("current_line_color", Color(0.248718, 0.390625, 0.257319))
		trace_log_scroll(0)
		if trace_log_cursor_line_last == -1:
			trace_log.cursor_set_line(trace_log.get_line_count()-1)
			trace_log_cursor_line_last = trace_log.cursor_get_line()
		else:
			trace_log.cursor_set_line(trace_log_cursor_line_last)
	else:
		trace_log.syntax_highlighting = false
		trace_log.add_color_override("current_line_color", Color(0.351563, 0.351563, 0.351563))
	
func get_reg_pc():
	return main.debug_read_registers()[5]

func regTextPanel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers()
		reg_text_panel.text = "AF %04X\nBC %04X\nDE %04X\nHL %04X\nSP %04X\nPC %04X" % [regs[0], regs[1], regs[2], regs[3],regs[4],regs[5]]
		reg_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		reg_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func flag_text_panel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers()
		var flags = regs[0] & 0xff
		var c = 1 if flags & 0x01 else 0
		var z = 1 if flags & 0x40 else 0
		var p = 1 if flags & 0x04 else 0
		var s = 1 if flags & 0x80 else 0
		var ac = 1 if flags & 0x10 else 0
		flag_text_panel.text = " C %01d\n Z %01d\n P %01d\n S %01d\nAC %01d" % [c, z, p, s, ac]
		flag_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		flag_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func stack_text_panel_update(enabled):
	if enabled:
		var stack = main.debug_read_stack(STACK_TEXT_LINES)
		stack_text_panel.text  = ""
		for stack_byte in stack:
			stack_text_panel.text += "%04X\n" % stack_byte
		stack_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		stack_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func breakpoints_list_panel_update(enabled):
	breakpoints_list_panel.unselect_all()
	if enabled:
		breakpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		breakpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func watchpoints_list_panel_update(enabled):
	watchpoints_list_panel.unselect_all()
	if enabled:
		watchpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		watchpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func hw_text_panel_update(enabled):
	if enabled:
		var hw_info = main.debug_read_hw_info()
		var total_v_cycles = hw_info[0]
		var last_run_v_cycles = total_v_cycles - last_total_v_cycles
		last_total_v_cycles = total_v_cycles
		var iff = hw_info[1]
		var raster_pixel = hw_info[2]
		var raster_line = hw_info[3]
		var mode_stack = hw_info[4]
		var page_stack = hw_info[5] / 0xffff
		var mode_map = hw_info[6]
		var page_map = hw_info[7] / 0xffff
		var ram_e = "E" if mode_map & 0x80 else "_"
		var ram_8 = "8" if mode_map & 0x40 else "_"
		var ram_ac = "AC" if mode_map & 0x20 else "__"
		var ram_mode = ram_8 + ram_ac + ram_e
		
		hw_text_panel.text  = "cycles %d\nlast run %d\niff %d\ncrt (%d, %d)\nmode stack %d\nmode ram %s\npage stack %d\npage ram %d" % [total_v_cycles, last_run_v_cycles, iff, raster_pixel, raster_line, mode_stack, ram_mode, page_stack, page_map]
		
		hw_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		hw_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

	
func is_breakpoint_auto(addrS):
	var idx = breakpoint_list_find(addrS)
	var postfix_idx = breakpoints_list_panel.get_item_text(idx).find(BREAKPOINT_AUTO_POSTFIX)
	return postfix_idx != -1

func breakpoint_list_find(addrS):
	for idx in range(breakpoints_list_panel.get_item_count()):
		if breakpoints_list_panel_get_addr(idx) == addrS:
			return idx
	return -1
	
func asm_line_to_addr(line):
	return line.left(7).right(1)

func add_breakpoint(addrS, auto_remove = false):
	var brk_exists = breakpoint_list_find(addrS) != -1
	if not brk_exists:
		var addr = addrS.hex_to_int()
		if auto_remove:
			addrS += BREAKPOINT_AUTO_POSTFIX
		breakpoints_list_panel.add_item(addrS)
		main.debug_add_breakpoint(addr, true, 0)
		
func del_breakpoint(addrS):
	var idx = breakpoint_list_find(addrS)
	if idx != -1:
		breakpoints_list_panel.remove_item(idx)
		var addr = addrS.hex_to_int()
		main.debug_del_breakpoint(addr, 0)

func del_all_breakpoints():
	code_panel.remove_breakpoints()
	for idx in range(breakpoints_list_panel.get_item_count()):
		var addrS = breakpoints_list_panel_get_addr(idx)
		var addr = addrS.hex_to_int()
		main.debug_del_breakpoint(addr, 0)
	breakpoints_list_panel.clear()
	code_panel_update(true)
	
func del_all_watchpoints():
	for idx in range(watchpoints_list_panel.get_item_count()):
		var global_addr = watchpoints_list_panel_get_global_addr(idx)
		main.debug_del_watchpoint(global_addr, ADDR_SPACE_GLOBAL)
	watchpoints_list_panel.clear()
	code_panel_update(true)
	
func breakpoints_list_panel_get_addr(idx):
	return breakpoints_list_panel.get_item_text(idx).left(6)
	
func watchpoints_list_panel_get_global_addr(idx):
	var txt = watchpoints_list_panel.get_item_text(idx)
	var addr_end_idx = txt.find(":")
	var addr_space = 0
	var global_addr = main.debug_get_global_addr(txt.left(addr_end_idx).hex_to_int(), addr_space)
	return global_addr

func _on_code_panel_breakpoint_toggled(row):
	var addrS = asm_line_to_addr(code_panel.get_line(row))
	var bp = code_panel.is_line_set_as_breakpoint(row)
	if bp:
		add_breakpoint(addrS)
	else:
		del_breakpoint(addrS)
		
func trace_log_scroll(offset):
	var lines = trace_log.get_visible_rows()
	var filter = trace_log_filter.selected
	trace_log.text = main.debug_get_trace_log(offset, lines, filter)

func codePanel_scroll_to_addr(addr, lines_before = CODE_PANEL_LINES_AHEAD):
	var lines = code_panel.get_visible_rows()
	code_panel.remove_breakpoints()	
	code_panel.text = main.debug_get_disasm(addr, lines, lines_before)
	# restore breakpoint markers in the code panel
	for brk_idx in range(breakpoints_list_panel.get_item_count()):
		var addrS = breakpoints_list_panel_get_addr(brk_idx)
		for line_idx in range(code_panel.get_line_count()):
			if addrS == asm_line_to_addr(code_panel.get_line(line_idx)):
				code_panel.set_line_as_breakpoint(line_idx, true)	

func _on_breakpoints_list_panel_item_selected(index):
	var addr = breakpoints_list_panel_get_addr(index).hex_to_int()
	codePanel_scroll_to_addr(addr)
	code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	
func _on_watchpoints_list_panel_item_selected(index):
	var global_addr = watchpoints_list_panel_get_global_addr(index)
	codePanel_scroll_to_addr(global_addr % 0xffff)
	code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	
func code_panel_menu_id_pressed(id):
	match id:
		CODE_PANEL_MENU_ID.CURRENT_BREAK:
			var pc = get_reg_pc()
			codePanel_scroll_to_addr(pc)
			code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
		CODE_PANEL_MENU_ID.REMOVE_ALL_BRKS:
			del_all_breakpoints()
		CODE_PANEL_MENU_ID.RUN_CURSOR:
			var addrS = asm_line_to_addr(code_panel.get_line(code_panel.cursor_get_line()))
			add_breakpoint(addrS, true)
			_on_break_cont_pressed()
		CODE_PANEL_MENU_ID.ADD_REMOVE_BRK:
			var cursor_line = code_panel.cursor_get_line()
			var bp = code_panel.is_line_set_as_breakpoint(cursor_line)
			code_panel.set_line_as_breakpoint(cursor_line, not bp)
			_on_code_panel_breakpoint_toggled(cursor_line)
		CODE_PANEL_MENU_ID.ADD_WP:
			_on_watchpoints_list_panel_nothing_selected()
			
		CODE_PANEL_MENU_ID.REMOVE_ALL_WPS:
			del_all_watchpoints()
			
func _physics_process(delta):
	if not main.debug_is_ui_break() and main.debug_is_break():
		main.debug_set_ui_break(true)
		var pc = get_reg_pc()
		var addrS = "0x%04X" % pc
		if is_breakpoint_auto(addrS):
			del_breakpoint(addrS)
		set_ui_on_break()

func _on_search_panel_gui_input(event):
	if last_searchs.size() == 0:
		return
		
	if event is InputEventKey and event.pressed and not event.echo:
		if event.scancode == KEY_UP:
			last_search_idx = last_search_idx + 1
			if search_panel.text == last_searchs[clamp(last_search_idx, 0, last_searchs.size()-1)]:
				last_search_idx = last_search_idx + 1
			search_panel.text = last_searchs[clamp(last_search_idx, 0, last_searchs.size()-1)]
		elif event.scancode == KEY_DOWN:
			last_search_idx = last_search_idx - 1
			search_panel.text = last_searchs[clamp(last_search_idx, 0, last_searchs.size()-1)]
		else:
			last_search_idx = -1
		
		last_search_idx = clamp(last_search_idx, -1, last_searchs.size()-1)

func _on_search_panel_text_entered(new_text : String):
	last_searchs.erase(new_text)
	last_searchs.push_front(new_text)
	last_search_idx = -1
	# is it in the 0xNNNN format to show the code at that addr?
	if new_text.left(2) == "0x" or new_text.left(2) == "0X":
		var addr = new_text.hex_to_int()
		codePanel_scroll_to_addr(addr)
		code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	else:
		if debug_labels.has(new_text.to_lower()):
			var addrS = "0x" + debug_labels[new_text.to_lower()]
			var addr = addrS.hex_to_int()
			codePanel_scroll_to_addr(addr)
			code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)

func _on_code_panel_gui_input(event):
	if not main.debug_is_ui_break():
		return
	
	var cursor_line = code_panel.cursor_get_line()
	if event is InputEventKey:
		if event.pressed:
			if event.scancode == KEY_UP and cursor_line == 0 and code_panel_cursor_line_last == 0:
				var addrS = asm_line_to_addr(code_panel.get_line(0))
				codePanel_scroll_to_addr(addrS.hex_to_int(), 1)
			elif event.scancode == KEY_DOWN and cursor_line == code_panel.get_line_count()-1 and code_panel_cursor_line_last == code_panel.get_line_count()-1:
				var addrS = asm_line_to_addr(code_panel.get_line(1))
				codePanel_scroll_to_addr(addrS.hex_to_int(), 0)
				code_panel.cursor_set_line(cursor_line)
	if event is InputEventMouseButton:
		if event.pressed:
			match event.button_index:
				BUTTON_WHEEL_UP:
					code_panel.cursor_set_line(cursor_line - 1)
					if cursor_line == 0 and code_panel_cursor_line_last == 0:
						var addrS = asm_line_to_addr(code_panel.get_line(0))
						codePanel_scroll_to_addr(addrS.hex_to_int(), 1)
				BUTTON_WHEEL_DOWN:
					code_panel.cursor_set_line(cursor_line + 1)
					if cursor_line == code_panel.get_line_count()-1 and code_panel_cursor_line_last == code_panel.get_line_count()-1:
						var addrS = asm_line_to_addr(code_panel.get_line(1))
						codePanel_scroll_to_addr(addrS.hex_to_int(), 0)
						code_panel.cursor_set_line(cursor_line)
	code_panel_cursor_line_last = cursor_line

func _on_trace_log_gui_input(event):
	if not main.debug_is_ui_break():
		return
	
	var cursor_line = trace_log.cursor_get_line()
	if event is InputEventKey:
		if event.pressed:
			if event.scancode == KEY_UP and cursor_line == 0 and trace_log_cursor_line_last == 0:
				trace_log_scroll(1)
			elif event.scancode == KEY_DOWN and cursor_line == trace_log.get_line_count()-1 and trace_log_cursor_line_last == trace_log.get_line_count()-1:
				trace_log_scroll(-1)
				trace_log.cursor_set_line(cursor_line)				
	if event is InputEventMouseButton:
		if event.pressed:
			match event.button_index:
				BUTTON_WHEEL_UP:
					trace_log.cursor_set_line(cursor_line - 1)
					if cursor_line == 0 and trace_log_cursor_line_last == 0:
						trace_log_scroll(1)
				BUTTON_WHEEL_DOWN:
					trace_log.cursor_set_line(cursor_line + 1)
					if cursor_line == trace_log.get_line_count()-1 and trace_log_cursor_line_last == trace_log.get_line_count()-1:
						trace_log_scroll(-1)
						trace_log.cursor_set_line(cursor_line)
	trace_log_cursor_line_last = cursor_line

func _on_step_over_pressed():
	var pc = get_reg_pc()
	var opcode = main.debug_read_executed_memory(pc, 1)[0]
	if step_over_cmds.find(opcode) != -1:
		var new_pc = (pc + OPCODE_CALL_LEN) % 0xffff
		var addrS = "0x%04X" % new_pc
		add_breakpoint(addrS, true)
		_on_break_cont_pressed()
	else:
		_on_step_into_pressed()
		
func debug_load_labels(rom_path : String):
	var dir_end = rom_path.find_last("/")
	var rom_name_end = rom_path.find_last(".")
	
	var labels_path_retroassembler = rom_path.left(dir_end + 1) + DEBUG_LABELS_FILE_NAME_RETROASSEMBLER
	var labels_path_tasm = rom_path.left(rom_name_end + 1) + DEBUG_LABELS_FILE_NAME_TASM_EXT
	var file = File.new()

	if file.open(labels_path_retroassembler, File.READ) == OK or file.open(labels_path_tasm, File.READ) == OK:
		var labels_str = file.get_as_text()
		main.debug_set_labels(labels_str)
		var labels = labels_str.split("\n")
		file.close()
		debug_labels.clear()
		for line in labels:
			var separator_id_first = line.find(" ")
			var separator_id_last1 = line.find_last(" ")
			var separator_id_last2 = line.find_last("$")
			var separator_id_last = max(separator_id_last1, separator_id_last2)
			
			var key = line.left(separator_id_first).to_lower()
			var val = line.right(separator_id_last + 1)
			debug_labels[key] = val
	
func save_debug():
	var cfg = ConfigFile.new()
	cfg.load("user://v06x.debug")
	for key in debug_labels:
		cfg.set_value("debug_labels", key, debug_labels[key])
		
	cfg.set_value("debug_hw_info", "last_total_v_cycles", last_total_v_cycles)
	cfg.save("user://v06x.debug")
	
func load_debug():
	var labels_str = ""
	var cfg = ConfigFile.new()
	var err = cfg.load("user://v06x.debug")
	if err == OK:
		debug_labels.clear()
		for key in cfg.get_section_keys("debug_labels"):
			var val = cfg.get_value("debug_labels", key)
			debug_labels[key] = val
			labels_str += key + " " + val + "\n"
		main.debug_set_labels(labels_str)

		last_total_v_cycles = cfg.get_value("debug_hw_info", "last_total_v_cycles", 0)

func _on_watchpoints_list_panel_nothing_selected(): 
	if main.debug_is_ui_break():
		watchpoint_popup.set_position(get_local_mouse_position())
		watchpoint_popup.visible = true

func watchpoints_list_panel_add(access, addr, cond, val, size, active, addr_space):
	var accessS = WATCHPOINTS_ACCESS_S[access]
	var condS = WATCHPOINTS_COND_S[cond]
	var valS = "" if cond == WATCHPOINTS_COND.ANY else "0x%02X" % val
	var val_sizeS = "b" if size == 1 else "w"
	var activeS = "+" if active else "-"
	var addr_spaceS = BW_POINTS_ADDR_SPACE_S[addr_space]
	var wp_text = "0x%05X: %s %s%s %s %s %s" % [addr, accessS, condS, valS, val_sizeS, addr_spaceS, activeS]
	watchpoints_list_panel.add_item(wp_text)
	main.debug_add_watchpoint(access, addr, cond, val, size, active, addr_space)

func _on_watchpoint_popup_confirmed():
	var access = WATCHPOINTS_ACCESS.RW
	if wp_access_r.pressed and not wp_access_w.pressed:
		access = WATCHPOINTS_ACCESS.R
	if not wp_access_r.pressed and wp_access_w.pressed:
		access = WATCHPOINTS_ACCESS.W
	
	var addr = wp_addr.text.hex_to_int()
		
	var cond = WATCHPOINTS_COND.ANY
	if wp_cond.text == "==" or wp_cond.text == "=":
		cond = WATCHPOINTS_COND.EQU
	if wp_cond.text == "<":
		cond = WATCHPOINTS_COND.LESS		
	elif wp_cond.text == ">":
		cond = WATCHPOINTS_COND.GREATER
	elif wp_cond.text == "<=":
		cond = WATCHPOINTS_COND.LESS_EQU
	elif wp_cond.text == ">=":
		cond = WATCHPOINTS_COND.GREATER_EQU
	elif wp_cond.text == "!=":
		cond = WATCHPOINTS_COND.NOT_EQU
	
	var val = wp_value.text.hex_to_int()
	var active = wp_active.pressed
	
	var sizeS = wp_value_size.text
	var size = 1 if sizeS == "byte" else 2
	
	var addr_space = 0
	if wp_addr_space_cpu.pressed:
		addr_space = BW_POINTS_ADDR_SPACE.CPU
	elif wp_addr_space_stack.pressed:
		addr_space = BW_POINTS_ADDR_SPACE.STACK
	else: 
		addr_space = BW_POINTS_ADDR_SPACE.GLOBAL
	
	watchpoints_list_panel_add(access, addr, cond, val, size, active, addr_space)

func _on_wp_addr_space_cpu_pressed():
	if not wp_addr_space_cpu.pressed:
		wp_addr_space_cpu.pressed = true
	wp_addr_space_stack.pressed = false
	wp_addr_space_global.pressed = false

func _on_wp_addr_space_stack_pressed():
	wp_addr_space_cpu.pressed = false
	if not wp_addr_space_stack.pressed:
		wp_addr_space_stack.pressed = true
	wp_addr_space_global.pressed = false

func _on_wp_addr_space_global_pressed():
	wp_addr_space_cpu.pressed = false
	wp_addr_space_stack.pressed = false
	if not wp_addr_space_global.pressed:
		wp_addr_space_global.pressed = true

func _on_wp_access_nor_r_not_w_pressed():
	if not wp_access_r.pressed and not wp_access_w.pressed:
		wp_access_r.pressed = true
		wp_access_w.pressed = true

func _on_wp_value_size_pressed():
	if wp_value_size.text == "byte":
		wp_value_size.text = "word"
	else:
		wp_value_size.text = "byte"

func _on_trace_log_filter_item_selected(index):
	trace_log_update(true)
