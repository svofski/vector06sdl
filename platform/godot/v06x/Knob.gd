extends Control

export var min_value: float = 0 setget _set_min_value
export var max_value: float = 100 setget _set_max_value
export var value: float = 0 setget _set_value
export var speed: float = 1
export var rounded: bool = false

signal value_changed(value)

# Declare member variables here. Examples:
# var a = 2
# var b = "text"

var start_pos = Vector2(0,0)
var start_val: float
var engaged: bool = false
var float_value: float = 0

# Called when the node enters the scene tree for the first time.
func _ready():
	var w = $Sprite.texture.get_size()[0] / $Sprite.hframes
	var h = $Sprite.texture.get_size()[1] / $Sprite.vframes
	
	print("sprite tex size=", w, " ", h, " rect_size=", rect_size)
	
	$Sprite.scale = Vector2(self.rect_size[0] / w, self.rect_size[1] / h)
	update_visuals()

func _set_min_value(v):
	min_value = v

func _set_max_value(v):
	max_value = v
	
func _set_value(v):
	value = clamp(v, min_value, max_value)
	float_value = value
	if rounded:
		value = round(value)
	update_visuals()
	emit_signal("value_changed", value)

func update_visuals():
	var value_range = max_value - min_value
	var offset_value = value - min_value
	var scaled = offset_value / value_range
	$Sprite.frame = round(100 * scaled)
	#$Text.text = "%+3.1fdB" % value
	
func _notification(what):
	if what == NOTIFICATION_MOUSE_EXIT:
		engaged = false
		
func _gui_input(event):
	if event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == BUTTON_LEFT:
				#print("Click at ", event.position)
				start_pos = event.position
				start_val = value
				engaged = true
			elif event.button_index == BUTTON_WHEEL_UP:
				_set_value(float_value - speed * 10)
			elif event.button_index == BUTTON_WHEEL_DOWN:
				_set_value(float_value + speed * 10) 
		else:
			engaged = false
	if engaged and event is InputEventMouseMotion:
		#print("Motion to ", event.position)
		var diff = (event.position.y - start_pos.y) * speed
		_set_value(start_val - diff)
