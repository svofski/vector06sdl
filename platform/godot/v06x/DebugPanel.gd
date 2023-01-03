extends Panel

var main : Control

var break_icon_tex = preload("res://assets/debug_pause.png")
var cont_icon_tex = preload("res://assets/debug_pause_cont.png")

onready var code_panel = find_node("code_panel")
onready var stack_text_panel = find_node("stack_text_panel")
onready var reg_text_panel = find_node("reg_text_panel")
onready var flag_text_panel = find_node("flag_text_panel")
onready var hardware_text_panel = find_node("hardware_text_panel")
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

var code_panel_cursor_line_last = 0

var hardware_text = """CPU 1234560
			CRT l/p 260, 160
			RAM-DSK S 1
			RAM-DSK M 2-8ACE
			IFF 0"""

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
		var regs = main.debug_read_registers();
		var pc = regs[5]
		codePanel_scroll_to_addr(pc)
		code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	else:
		code_panel.syntax_highlighting = false
		code_panel.add_color_override("current_line_color", Color(0.351563, 0.351563, 0.351563))
		code_panel.remove_breakpoints()
	
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

func itemlist_find(itemlist : ItemList, txt):
	for idx in range(itemlist.get_item_count()):
		if itemlist.get_item_text(idx) == txt:
			return idx
	return -1
	
func asm_line_to_addr(line):
	return line.left(7).right(1)

func insert_breakpoint(addrS):
	var brk_exists = itemlist_find(breakpoints_list_panel, addrS) != -1
	if not brk_exists:
		breakpoints_list_panel.add_item(addrS)
		var addr = addrS.hex_to_int()
		main.debug_insert_breakpoint(addr)
		
func remove_breakpoint(addrS):
	var idx = itemlist_find(breakpoints_list_panel, addrS)
	if idx != -1:
		breakpoints_list_panel.remove_item(idx)
		var addr = addrS.hex_to_int()
		main.debug_remove_breakpoint(addr)

func remove_all_breakpoints():
	code_panel.remove_breakpoints()
	for idx in range(breakpoints_list_panel.get_item_count()):
		var addrS = breakpoints_list_panel.get_item_text(idx)
		var addr = addrS.hex_to_int()
		main.debug_remove_breakpoint(addr)
	breakpoints_list_panel.clear()
	codePanel_update(true)

func _on_code_panel_breakpoint_toggled(row):
	var addrS = asm_line_to_addr(code_panel.get_line(row))
	var bp = code_panel.is_line_set_as_breakpoint(row)
	if bp:
		insert_breakpoint(addrS)
	else:
		remove_breakpoint(addrS)

func codePanel_scroll_to_addr(addr, lines_before = CODE_PANEL_LINES_AHEAD):
	var lines = code_panel.get_visible_rows()
	code_panel.text = main.debug_disasm(addr, lines, lines_before)
	code_panel.remove_breakpoints()
	# restore breakpoint markers in the code panel
	for brk_idx in range(breakpoints_list_panel.get_item_count()):
		var addrS = breakpoints_list_panel.get_item_text(brk_idx)
		for line_idx in range(code_panel.get_line_count()):
			if addrS == asm_line_to_addr(code_panel.get_line(line_idx)):
				code_panel.set_line_as_breakpoint(line_idx, true)	

func _on_breakpoints_list_panel_item_selected(index):
	var addr = breakpoints_list_panel.get_item_text(index).hex_to_int()
	codePanel_scroll_to_addr(addr)
	code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
	
func code_panel_menu_id_pressed(id):
	match id:
		CODE_PANEL_MENU_ID_CURRENT_BREAK:
			var regs = main.debug_read_registers();
			var pc = regs[5]
			codePanel_scroll_to_addr(pc)
			code_panel.cursor_set_line(CODE_PANEL_LINES_AHEAD)
		CODE_PANEL_MENU_ID_REMOVE_ALL_BRKS:
			remove_all_breakpoints()
		CODE_PANEL_MENU_ID_RUN_CURSOR:
			var addrS = asm_line_to_addr(code_panel.get_line(code_panel.cursor_get_line()))
			insert_breakpoint(addrS)
			_on_break_cont_pressed()

func _physics_process(delta):
	if not main.debug_break_enabled and main.debug_is_break():
		main.debug_break_enabled = true
		set_ui_on_break()
		
func _on_search_panel_text_entered(new_text : String):
	# is it in the 0xNNNN format to show the code at that addr?
	if new_text.left(2) == "0x" and new_text.length() < 7:
		var addr = new_text.hex_to_int()
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

































