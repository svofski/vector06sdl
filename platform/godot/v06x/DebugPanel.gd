extends Panel

var debugCmdHandlerFuncName : String
var mainControl : Control

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


var stackText = """B021
				B16D
				B072
				C003
				0000
				0000
				0000 
				0000"""
				
var regText = """AF 1542
				BC 150B
				DE 020C
				HL A6D7
				SP 8000
				PC 0100"""
				
var flagText = """  C 0
			  Z 0
			  P 0
			  S 0
			 AC 0"""

var otherText = """CPU 1234560
			CRT l/p 260, 160
			RAM-DSK S 1
			RAM-DSK M 2-8ACE
			INT 0"""

# Called when the node enters the scene tree for the first time.
func _ready():
	#var v06x = find_node("main").v06x
	stackTextPanel.text = stackText
	regTextPanel.text = regText
	flagTextPanel.text = flagText
	otherTextPanel.text = otherText
			
	breakPointsListPanel.add_item("0000")
	breakPointsListPanel.add_item("FFFF")
	breakPointsListPanel.add_item("8000")

func SetEmuCmdHandler(_mainControl, _debugCmdHandlerFuncName):
	mainControl = _mainControl
	debugCmdHandlerFuncName = _debugCmdHandlerFuncName

func DebugPanelSizeUpdate():
	#var windowSize = get_tree().get_root().size
	#print("windowSize=", windowSize)
	#var scale = widthDefault / windowSize.x	

	stackPanel.rect_size.y = codePanel.rect_position.y - 1
	regPanel.rect_size.y = codePanel.rect_position.y - 1
	flagPanel.rect_size.y = codePanel.rect_position.y - 1
	otherPanel.rect_size.y = codePanel.rect_position.y - 1
	breakPointsPanel.rect_size.y = codePanel.rect_position.y - 1

	codePanel.rect_size.x = self.rect_size.x / codePanel.rect_scale.x
	codePanel.rect_size.y = (self.rect_size.y - codePanel.rect_position.y) / codePanel.rect_scale.y
	
func _on_DebugPanel_item_rect_changed():
	DebugPanelSizeUpdate()
func _on_DebugPanel_resized():
	DebugPanelSizeUpdate()
func _on_DebugPanel_focus_exited():
	DebugPanelSizeUpdate()
	
func _on_DebugPanel_gui_input(event):
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed:
			print("test")

func _on_Pause_pressed():
	mainControl.call(debugCmdHandlerFuncName, "$?#00")
	
	
