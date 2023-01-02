extends Panel

var main : Control

var breakIconTex = preload("res://assets/debug_pause.png")
var contIconTex = preload("res://assets/debug_pause_cont.png")

onready var codePanel = find_node("CodePanel")

onready var stackPanel = find_node("StackPanel")
onready var stackLabel = find_node("StackLabel")
onready var stackTextPanel = find_node("StackTextPanel")

onready var regPanel = find_node("RegPanel")
onready var regLabel = find_node("RegLabel")
onready var regTextPanel = find_node("RegTextPanel")

onready var flagPanel = find_node("FlagPanel")
onready var flagLabel = find_node("FlagLabel")
onready var flagTextPanel = find_node("FlagTextPanel")

onready var otherPanel = find_node("OtherPanel")
onready var otherLabel = find_node("OtherLabel")
onready var otherTextPanel = find_node("OtherTextPanel")

onready var breakPointsPanel = find_node("BreakPointsPanel")
onready var breakPointsLabel = find_node("BreakPointsLabel")
onready var breakPointsListPanel = find_node("BreakPointsListPanel")

onready var callstackPanel = find_node("CallStackPanel")
onready var callstackListPanel = find_node("CallStackListPanel")

onready var breakCont = find_node("BreakCont")
onready var stepOver = find_node("StepOver")
onready var stepInto = find_node("StepInto")
onready var stepOut = find_node("StepOut")

const MENU_ID_CURRENT_BREAK = 10
const MENU_ID_REMOVE_ALL_BRKS = 20
onready var codePanel_menu = codePanel.get_menu()

var debugging = false
				
var STACK_TEXT_LINES = 6
var breakpoints = {}

var otherText = """CPU 1234560
			CRT l/p 260, 160
			RAM-DSK S 1
			RAM-DSK M 2-8ACE
			INT 0"""

# Called when the node enters the scene tree for the first time.
func _ready():
	codePanel_menu.add_item("current break", MENU_ID_CURRENT_BREAK)
	codePanel_menu.add_item("remove all breakpoints", MENU_ID_REMOVE_ALL_BRKS)
	codePanel_menu.connect("id_pressed", self, "codePanel_menu_id_pressed")
	set_ui_on_cont()
	
func debug_panel_size_update():
	stackPanel.rect_size.y = codePanel.rect_position.y - 1
	regPanel.rect_size.y = codePanel.rect_position.y - 1
	flagPanel.rect_size.y = codePanel.rect_position.y - 1
	otherPanel.rect_size.y = codePanel.rect_position.y - 1
	breakPointsPanel.rect_size.y = codePanel.rect_position.y - 1
	breakPointsListPanel.rect_size.y = 1.8*breakPointsPanel.rect_size.y
	callstackPanel.margin_bottom = codePanel.margin_bottom
	callstackListPanel.margin_bottom = (1.0/callstackListPanel.rect_scale.y) * callstackPanel.margin_bottom - 10

	#codePanel.rect #= self.rect_size.x / codePanel.rect_scale.x
	codePanel.rect_size.y = (self.rect_size.y - codePanel.rect_position.y) / codePanel.rect_scale.y
	
func _on_DebugPanel_item_rect_changed():
	debug_panel_size_update()
func _on_DebugPanel_resized():
	debug_panel_size_update()
func _on_DebugPanel_focus_exited():
	debug_panel_size_update()

func set_ui_on_break():
		codePanel_menu.set_item_disabled(codePanel_menu.get_item_index(MENU_ID_CURRENT_BREAK), false)
		codePanel_menu.set_item_disabled(codePanel_menu.get_item_index(MENU_ID_REMOVE_ALL_BRKS), false)
		breakCont.icon = contIconTex
		regTextPanel_update(true)
		flagTextPanel_update(true)
		stackTextPanel_update(true)
		codePanel_update(true)
		breakpointsListPanel_update(true)
		stepOver.disabled = false;
		stepInto.disabled = false;
		stepOut.disabled = false;	
		
func set_ui_on_cont():
		codePanel_menu.set_item_disabled(codePanel_menu.get_item_index(MENU_ID_CURRENT_BREAK), true)
		codePanel_menu.set_item_disabled(codePanel_menu.get_item_index(MENU_ID_REMOVE_ALL_BRKS), true)
		breakCont.icon = breakIconTex
		regTextPanel_update(false)
		flagTextPanel_update(false)
		stackTextPanel_update(false)
		codePanel_update(false)
		breakpointsListPanel_update(false)
		stepOver.disabled = true;
		stepInto.disabled = true;
		stepOut.disabled = true;

func _on_Pause_pressed():
	if not debugging:
		debugging = true
		
	debug_break_cont()
		
	if main.is_debug_break():
		set_ui_on_break()
	else:
		set_ui_on_cont()

func _on_Restart_pressed():
	main.ReloadFile()

func debug_break_cont():
	# to pass the breakpoint without removing it
	if main.is_debug_break():
		main.debug_step_into()
	main.debug_break_cont()

var step_into_pressed = 0

func _on_StepInto_pressed():
	print("TEST PRINT: %s gd: stepInto processing started." % Time.get_time_string_from_system())
	main.debug_step_into()
	set_ui_on_break()
	step_into_pressed=step_into_pressed+1
	print("TEST PRINT: %s gd: stepInto done. pressed=%d" % [Time.get_time_string_from_system(), step_into_pressed])

func codePanel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers();
		var pc = regs[5]
		codePanel_scroll_to_addr(pc)
	else:
		codePanel.syntax_highlighting = false
		codePanel.add_color_override("current_linde_color", Color(0.351563, 0.351563, 0.351563))
		codePanel.remove_breakpoints()
	
func regTextPanel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers()
		regTextPanel.text = "AF %04X\nBC %04X\nDE %04X\nHL %04X\nSP %04X\nPC %04X" % [regs[0], regs[1], regs[2], regs[3],regs[4],regs[5]]
		regTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		regTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func flagTextPanel_update(enabled):
	if enabled:
		var regs = main.debug_read_registers()
		var flags = regs[0] & 0xff
		var c = 1 if flags & 0x01 else 0
		var z = 1 if flags & 0x40 else 0
		var p = 1 if flags & 0x04 else 0
		var s = 1 if flags & 0x80 else 0
		var ac = 1 if flags & 0x10 else 0
		flagTextPanel.text = " C %01d\n Z %01d\n P %01d\n S %01d\nAC %01d" % [c, z, p, s, ac]
		flagTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		flagTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func stackTextPanel_update(enabled):
	if enabled:
		var stack = main.debug_read_stack(STACK_TEXT_LINES)
		stackTextPanel.text  = ""
		for stack_byte in stack:
			stackTextPanel.text += "%02X\n" % stack_byte
		stackTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		stackTextPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func breakpointsListPanel_update(enabled):
	breakPointsListPanel.unselect_all()
	if enabled:
		breakPointsListPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 1.0))
	else:
		breakPointsListPanel.add_color_override("font_color", Color(1.0, 1.0, 1.0, 0.5))

func itemlist_find(itemlist : ItemList, txt):
	for idx in range(itemlist.get_item_count()):
		if itemlist.get_item_text(idx) == txt:
			return idx
	return -1
	
func asm_get_addr(line):
	return line.left(7).right(3)
	
func _on_CodePanel_breakpoint_toggled(row):
	var addrS = asm_get_addr(codePanel.get_line(row))
	var idx = itemlist_find(breakPointsListPanel, addrS)
	var bp = codePanel.is_line_set_as_breakpoint(row)
	if bp:
		if idx == -1:
			breakPointsListPanel.add_item(addrS)
			var addr = ("0x" + addrS).hex_to_int()
			main.debug_insert_breakpoint(addr)
	else:
		if idx > -1:
			breakPointsListPanel.remove_item(idx)
			var addr = ("0x" + addrS).hex_to_int()
			main.debug_remove_breakpoint(addr)

func codePanel_scroll_to_addr(addr):
		codePanel.text = main.debug_disasm(addr, 40, 6)
		codePanel.cursor_set_line(6)
		codePanel.syntax_highlighting = true
		codePanel.add_color_override("current_line_color", Color(0.248718, 0.390625, 0.257319))
		codePanel.remove_breakpoints()
		for brk_idx in range(breakPointsListPanel.get_item_count()):
			var brkS = breakPointsListPanel.get_item_text(brk_idx)
			for line_idx in range(codePanel.get_line_count()):
				if brkS == asm_get_addr(codePanel.get_line(line_idx)):
					codePanel.set_line_as_breakpoint(line_idx, true)	

func remove_all_breakpoints():
	codePanel.remove_breakpoints()
	for idx in range(breakPointsListPanel.get_item_count()):
		var addrS = breakPointsListPanel.get_item_text(idx)
		var addr = ("0x" + addrS).hex_to_int()
		main.debug_remove_breakpoint(addr)
	breakPointsListPanel.clear()

func _on_BreakPointsListPanel_item_selected(index):
	var addr = ("0x" + breakPointsListPanel.get_item_text(index)).hex_to_int()
	codePanel_scroll_to_addr(addr)
	
func codePanel_menu_id_pressed(id):
	if id == MENU_ID_CURRENT_BREAK:
		var regs = main.debug_read_registers();
		var pc = regs[5]
		codePanel_scroll_to_addr(pc)
	if id == MENU_ID_REMOVE_ALL_BRKS:
		remove_all_breakpoints()

func _physics_process(delta):
	if not main.debug_break_enabled and main.debug_is_break():
		main.debug_break_enabled = true
		set_ui_on_break()
		
		































