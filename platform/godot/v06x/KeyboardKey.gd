tool
extends PanelContainer

signal key_make(scancode)
signal key_break(scancode)

export var top_text: String = "Й" setget _set_top_text
export var bottom_text: String = "J" setget _set_bottom_text
export var scancode: int = -1
export var key_style: StyleBox = null setget _set_key_style
export var text_color: Color = Color.black  setget _set_text_color

export var visual_pressed: bool  = false setget _set_visual_pressed

onready var label1 = $Label1
onready var label2 = $Label2
onready var overlay = $Overlay

var engaged = false
var hover = false
onready var stylebox_hover = self.get_stylebox("key_hover").duplicate()

func _ready():
	if key_style == null:
		key_style = get("custom_styles/panel")
	update_visuals()
	overlay.add_stylebox_override("panel", self.get_stylebox("key_normal"))

func _set_visual_pressed(v):
	visual_pressed = v
	update_style()
	
func update_style():
	if stylebox_hover == null:
		return
		
	if visual_pressed or hover:
		overlay.add_stylebox_override("panel", stylebox_hover)
		var left = self.get_stylebox("panel").content_margin_left
		var top = self.get_stylebox("panel").content_margin_top
		var right = self.get_stylebox("panel").content_margin_right
		var bottom = self.get_stylebox("panel").content_margin_bottom
		overlay.get_stylebox("panel").set_expand_margin_individual(left, top, right, bottom)
	else:
		overlay.add_stylebox_override("panel", self.get_stylebox("key_normal"))
	
func _set_top_text(t):
	top_text = t
	update_visuals()
	
func _set_bottom_text(t):
	bottom_text = t
	update_visuals()

func _set_text_color(c):
	text_color = c
	#text_color = Color(randi())
	update_visuals()
	
func _set_key_style(c):
	key_style = c
	update_visuals()
	#var custom_panel = get("custom_styles/panel").duplicate()
	#custom_panel.set("bg_color", c)
	#set("custom_styles/panel", custom_panel)

func update_visuals():
	if label1 == null || label2 == null:
		return

	label1.text = top_text
	label2.text = bottom_text
	if key_style != null:
		set("custom_styles/panel", key_style)
	label1.add_color_override("font_color", text_color)
	label2.add_color_override("font_color", text_color)
	if len(bottom_text) == 0:
		label1.size_flags_horizontal = SIZE_SHRINK_CENTER
		label1.size_flags_vertical = SIZE_SHRINK_CENTER
	rect_size = rect_min_size

func _on_mouse_entered():
	hover = true
	update_style()

func _on_mouse_exited():
	hover = false
	update_style()
	if engaged:
		engaged = false
		emit_signal("key_break", scancode)

func _gui_input(event):	
	if event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == BUTTON_LEFT:
				engaged = true
				update_style()
				emit_signal("key_make", scancode)
		else:
			if engaged and event.button_index == BUTTON_LEFT:
				engaged = false
				update_style()
				emit_signal("key_break", scancode)

