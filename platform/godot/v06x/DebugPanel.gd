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
onready var breakPointsTextPanel = find_node("BreakPointsTextPanel")
onready var breakPointsListPanel = find_node("BreakPointsListPanel")

onready var breakCont = find_node("BreakCont")
onready var stepOver = find_node("StepOver")
onready var stepInto = find_node("StepInto")
onready var stepOut = find_node("StepOut")

var debugging = false

var stackText = """B021
				B16D
				B072
				C003
				0000
				0000
				0000 
				0000"""

var otherText = """CPU 1234560
			CRT l/p 260, 160
			RAM-DSK S 1
			RAM-DSK M 2-8ACE
			INT 0"""

# Called when the node enters the scene tree for the first time.
func _ready():
	stackTextPanel.text = stackText
	regTextPanel_update(false)
	flagTextPanel_update(false)
	otherTextPanel.text = otherText
			
	breakPointsListPanel.add_item("0000")
	breakPointsListPanel.add_item("FFFF")
	breakPointsListPanel.add_item("8000")

func debug_panel_size_update():
	stackPanel.rect_size.y = codePanel.rect_position.y - 1
	regPanel.rect_size.y = codePanel.rect_position.y - 1
	flagPanel.rect_size.y = codePanel.rect_position.y - 1
	otherPanel.rect_size.y = codePanel.rect_position.y - 1
	breakPointsPanel.rect_size.y = codePanel.rect_position.y - 1

	codePanel.rect_size.x = self.rect_size.x / codePanel.rect_scale.x
	codePanel.rect_size.y = (self.rect_size.y - codePanel.rect_position.y) / codePanel.rect_scale.y
	
func _on_DebugPanel_item_rect_changed():
	debug_panel_size_update()
func _on_DebugPanel_resized():
	debug_panel_size_update()
func _on_DebugPanel_focus_exited():
	debug_panel_size_update()

func _on_Pause_pressed():
	if not debugging:
		debugging = true
		
	main.debug_break_cont()
		
	if main.is_debug_break():
		breakCont.icon = contIconTex
		regTextPanel_update(true)
		flagTextPanel_update(true)
		codePanel_update()
		stepOver.disabled = false;
		stepInto.disabled = false;
		stepOut.disabled = false;
	else:
		breakCont.icon = breakIconTex
		regTextPanel_update(false)
		flagTextPanel_update(false)
		stepOver.disabled = true;
		stepInto.disabled = true;
		stepOut.disabled = true;

func _on_Restart_pressed():
	main.ReloadFile()
	
var step_into_pressed = 0

func _on_StepInto_pressed():
	print("TEST PRINT: %s gd: stepInto processing started." % Time.get_time_string_from_system())
	main.debug_step_into()
	regTextPanel_update(true)
	flagTextPanel_update(true)
	codePanel_update()
	step_into_pressed=step_into_pressed+1
	print("TEST PRINT: %s gd: stepInto done. pressed=%d" % [Time.get_time_string_from_system(), step_into_pressed])

func codePanel_update():
	var regs = main.debug_read_registers();
	var pc = regs[5]
	codePanel.text = main.debug_disasm(pc, 40, 6)
	codePanel.cursor_set_line(6)
	
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
