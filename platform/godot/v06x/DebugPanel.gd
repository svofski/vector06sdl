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

onready var callstack_panel = find_node("callstack_panel")
onready var callstack_list_panel = find_node("callstack_list_panel")

onready var break_cont = find_node("break_cont")
onready var step_over = find_node("step_over")
onready var step_into = find_node("step_into")
onready var step_out = find_node("step_out")

onready var main_bar = find_node("main_bar")
onready var search_panel = find_node("search_panel")

const CODE_PANEL_MENU_ID_CURRENT_BREAK = 10
const CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS = 20
const CODE_PANEL_MENU_ID_RUN_CURSOR = 30
onready var code_panel_menu = code_panel.get_menu()

const STACK_TEXT_LINES = 6
const CODE_PANEL_LINES_AHEAD = 6
const BREAKPOINT_AUTO_POSTFIX = ":A"

var code_panel_cursor_line_last = 0

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
const DEBUG_LABELS_FILE_NAME = "debug.txt"

# Called when the node enters the scene tree for the first time.
func _ready():
	code_panel_menu.add_item("show the current break", CODE_PANEL_MENU_ID_CURRENT_BREAK)
	code_panel_menu.add_item("run to the selected line", CODE_PANEL_MENU_ID_RUN_CURSOR)
	code_panel_menu.add_item("remove all breakpoints", CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS)
	code_panel_menu.connect("id_pressed", self, "code_panel_menu_id_pressed")
	set_ui_on_cont()
	
func debug_panel_size_update():
	if main.debug_break_enabled:
		var addrS = asm_line_to_addr(code_panel.get_line(0))
		codePanel_scroll_to_addr(addrS.hex_to_int(), 0)
	code_panel.margin_bottom = self.margin_bottom
	code_panel.cursor_set_line(code_panel_cursor_line_last)
	callstack_panel.margin_bottom = self.margin_bottom
	callstack_list_panel.margin_bottom = self.margin_bottom / callstack_list_panel.rect_scale.y - 10
	
func _on_debug_panel_item_rect_changed():
	debug_panel_size_update()
func _on_debug_panel_resized():
	debug_panel_size_update()
func _on_debug_panel_focus_exited():
	debug_panel_size_update()

func set_ui_on_break():
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_CURRENT_BREAK), false)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS), false)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_RUN_CURSOR), false)
		
		break_cont.icon = cont_icon_tex
		regTextPanel_update(true)
		flagTextPanel_update(true)
		stackTextPanel_update(true)
		codePanel_update(true)
		breakpointsListPanel_update(true)
		hw_text_panel_update(true)
		step_over.disabled = false;
		step_into.disabled = false;
		step_out.disabled = false;
		search_panel.editable = true;
		
func set_ui_on_cont():
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_CURRENT_BREAK), true)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS), true)
		code_panel_menu.set_item_disabled(code_panel_menu.get_item_index(CODE_PANEL_MENU_ID_RUN_CURSOR), true)
		break_cont.icon = break_icon_tex
		regTextPanel_update(false)
		flagTextPanel_update(false)
		stackTextPanel_update(false)
		codePanel_update(false)
		breakpointsListPanel_update(false)
		hw_text_panel_update(false)
		step_over.disabled = true;
		step_into.disabled = true;
		step_out.disabled = true;
		search_panel.editable = false;

func _on_break_cont_pressed():
	if main.is_debug_break():
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

func codePanel_update(enabled):
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
	
func get_reg_pc():
	return main.debug_read_registers()[5]

func regTextPanel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers()
		reg_text_panel.text = "AF %04X\nBC %04X\nDE %04X\nHL %04X\nSP %04X\nPC %04X" % [regs[0], regs[1], regs[2], regs[3],regs[4],regs[5]]
		reg_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		reg_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func flagTextPanel_update(enabled):
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

func stackTextPanel_update(enabled):
	if enabled:
		var stack = main.debug_read_stack(STACK_TEXT_LINES)
		stack_text_panel.text  = ""
		for stack_byte in stack:
			stack_text_panel.text += "%04X\n" % stack_byte
		stack_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		stack_text_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func breakpointsListPanel_update(enabled):
	breakpoints_list_panel.unselect_all()
	if enabled:
		breakpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		breakpoints_list_panel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

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
		hw_text_panel.text  = "cycles %d\nlast run %d\niff %d\ncrt (%d, %d)\nmode stack %d\npage stack %d\nmode ram %d\npage ram %d" % [total_v_cycles, last_run_v_cycles, iff, raster_pixel, raster_line, mode_stack, page_stack, mode_map, page_map]
		
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

func insert_breakpoint(addrS, auto_remove = false):
	var brk_exists = breakpoint_list_find(addrS) != -1
	if not brk_exists:
		var addr = addrS.hex_to_int()
		if auto_remove:
			addrS += BREAKPOINT_AUTO_POSTFIX
		breakpoints_list_panel.add_item(addrS)
		main.debug_insert_breakpoint(addr)
		
func remove_breakpoint(addrS):
	var idx = breakpoint_list_find(addrS)
	if idx != -1:
		breakpoints_list_panel.remove_item(idx)
		var addr = addrS.hex_to_int()
		main.debug_remove_breakpoint(addr)

func remove_all_breakpoints():
	code_panel.remove_breakpoints()
	for idx in range(breakpoints_list_panel.get_item_count()):
		var addrS = breakpoints_list_panel_get_addr(idx)
		var addr = addrS.hex_to_int()
		main.debug_remove_breakpoint(addr)
	breakpoints_list_panel.clear()
	codePanel_update(true)
	
func breakpoints_list_panel_get_addr(idx):
	return breakpoints_list_panel.get_item_text(idx).left(6)

func _on_code_panel_breakpoint_toggled(row):
	var addrS = asm_line_to_addr(code_panel.get_line(row))
	var bp = code_panel.is_line_set_as_breakpoint(row)
	if bp:
		insert_breakpoint(addrS)
	else:
		remove_breakpoint(addrS)

func codePanel_scroll_to_addr(addr, lines_before = CODE_PANEL_LINES_AHEAD):
	var lines = code_panel.get_visible_rows()
	code_panel.remove_breakpoints()	
	code_panel.text = main.debug_disasm(addr, lines, lines_before)
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
	
func code_panel_menu_id_pressed(id):
	match id:
		CODE_PANEL_MENU_ID_CURRENT_BREAK:
			var pc = get_reg_pc()
			codePanel_scroll_to_addr(pc)
			code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
		CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS:
			remove_all_breakpoints()
		CODE_PANEL_MENU_ID_RUN_CURSOR:
			var addrS = asm_line_to_addr(code_panel.get_line(code_panel.cursor_get_line()))
			insert_breakpoint(addrS, true)
			_on_break_cont_pressed()

func _physics_process(delta):
	if not main.debug_break_enabled and main.debug_is_break():
		main.debug_break_enabled = true
		var pc = get_reg_pc()
		var addrS = "0x%04X" % pc
		if is_breakpoint_auto(addrS):
			remove_breakpoint(addrS)
		set_ui_on_break()
		
func _on_search_panel_text_entered(new_text : String):
	# is it in the 0xNNNN format to show the code at that addr?
	if new_text.left(2) == "0x" or new_text.left(2) == "0X":
		var addr = new_text.hex_to_int()
		codePanel_scroll_to_addr(addr, 0)
	else:
		if debug_labels.has(new_text.to_lower()):
			var addrS = "0x" + debug_labels[new_text.to_lower()]
			var addr = addrS.hex_to_int()
			codePanel_scroll_to_addr(addr, 0)

func _on_code_panel_gui_input(event):
	var cursor_line = code_panel.cursor_get_line()
	if event is InputEventKey:
		if event.pressed:
			if event.scancode == KEY_UP and cursor_line == 0 and code_panel_cursor_line_last == 0:
				var addrS = asm_line_to_addr(code_panel.get_line(0))
				codePanel_scroll_to_addr(addrS.hex_to_int(), 1)
			if event.scancode == KEY_DOWN and cursor_line == code_panel.get_line_count()-1 and code_panel_cursor_line_last == code_panel.get_line_count()-1:
				var addrS = asm_line_to_addr(code_panel.get_line(1))
				codePanel_scroll_to_addr(addrS.hex_to_int(), 0)
				code_panel.cursor_set_line(cursor_line)
	code_panel_cursor_line_last = cursor_line

func _on_step_over_pressed():
	var pc = get_reg_pc()
	var opcode = main.debug_read_executed_memory(pc, 1)[0]
	if step_over_cmds.find(opcode) != -1:
		var new_pc = (pc + OPCODE_CALL_LEN) % 0xffff
		var addrS = "0x%04X" % new_pc
		insert_breakpoint(addrS, true)
		_on_break_cont_pressed()
	else:
		_on_step_into_pressed()
		
func debug_load_labels(rom_path : String):
	var dir_end = rom_path.find_last("/")
	var labels_path = rom_path.left(dir_end + 1) + DEBUG_LABELS_FILE_NAME
	var file = File.new()

	if file.open(labels_path, File.READ) == OK:
		var labels = file.get_as_text().split("\n")
		file.close()
		debug_labels.clear()
		for line in labels:
			var separator_id = line.find(" ")
			var key = line.left(separator_id)
			var val = line.right(separator_id+2)
			debug_labels[key] = val
	
func save_debug():
	var cfg = ConfigFile.new()
	cfg.load("user://v06x.debug")
	for key in debug_labels:
		cfg.set_value("debug_labels", key, debug_labels[key])
	cfg.save("user://v06x.debug")
	
func load_debug():
	var cfg = ConfigFile.new()
	var err = cfg.load("user://v06x.debug")
	if err == OK:
		debug_labels.clear()
		for key in cfg.get_section_keys("debug_labels"):
			debug_labels[key] = cfg.get_value("debug_labels",key)


































