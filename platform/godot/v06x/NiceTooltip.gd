extends PanelContainer

export var text: String setget _set_text
export var enabled: bool setget _set_enabled

onready var label = find_node("Label")
onready var timer = find_node("Timer")

var alpha = 0

func _ready():
	timer.connect("timeout", self, "timeout")
	timer.stop()
	add_stylebox_override("panel", get_stylebox("nice_tooltip"))
	update_sizes()

func _set_text(t: String):
	text = t
	if label != null:
		label.text = text
	update_sizes()
	
func _set_enabled(ena: bool):
	enabled = ena
	if not enabled:
		hideTooltip()

func update_sizes():
	if label != null:
		rect_size = label.rect_size

func showTooltip(pos: Vector2, txt: String):
	_set_text(txt)
	pos.y -= label.rect_size.y * 1.25
	rect_position = pos
	self.modulate = Color(1, 1, 1, 1)
	alpha = 2

	if enabled:		
		timer.start(0.02)
		timer.start()
		visible = true
	
func timeout():
	alpha -= 0.04
	var m = self.modulate
	m.a = alpha
	self.modulate = m
	if alpha <= 0:
		hideTooltip()

func hideTooltip():
	visible = false
	timer.stop()
	
