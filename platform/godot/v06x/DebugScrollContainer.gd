extends ScrollContainer

onready var codePanel = find_node("CodePanel")

onready var stackPanel = find_node("StackPanel")
onready var stackLabel = find_node("StackLabel")
onready var stackTextPanel = find_node("StackTextPanel")

onready var regPanel = find_node("RegPanel")
onready var regLabel = find_node("RegLabel")
onready var regTextPanel = find_node("RegTextPanel")


var stackText = """B021
				B16D
				B072
				C003
				0000
				0000
				0000 
				0000"""
				
var RegText = """AF 1542
				BC 150B
				DE 020C
				HL A6D7
				SP 8000
				PC 0100
				RD_S 1
				RD_M 2-8ACE"""

# Called when the node enters the scene tree for the first time.
func _ready():
	#var v06x = find_node("main").v06x
	pass


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass

func DebugSizeInit():
	var width = self.rect_size.x
	
	stackLabel.rect_scale.x = .4
	stackLabel.rect_scale.y = .4	
	stackTextPanel.rect_scale.x = .5
	stackTextPanel.rect_scale.y = .5
	stackTextPanel.text = stackText
	stackPanel.rect_size.y = codePanel.rect_position.y - 1
	
	regLabel.rect_scale.x = .4
	regLabel.rect_scale.y = .4
	regTextPanel.rect_scale.x = .5
	regTextPanel.rect_scale.y = .5
	regTextPanel.text = RegText
	regPanel.rect_size.y = codePanel.rect_position.y - 1	
		
	codePanel.rect_size.x = width
	codePanel.rect_size.y = self.rect_size.y - codePanel.rect_position.y
	

	
func _on_DebugScrollContainer_resized():
	DebugSizeInit()
func _on_VSplitContainer_resized():
	DebugSizeInit()
func _on_PanelContainer_item_rect_changed():
	DebugSizeInit()

func _on_StackPanel_gui_input(event):
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and event.pressed:
			
			print("test")
