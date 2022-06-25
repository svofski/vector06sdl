tool
extends Control

export var min_value: float = 0 setget _set_min_value
export var max_value: float = 100 setget _set_max_value
export var value: float = 0 setget _set_value
export var speed: float = 1
export var rounded: bool = false
export var round_precision: int = 1
export var caption: String setget _set_caption
export var caption_side: int = 0 setget _set_caption_side
export var center_led: bool = false setget _set_center_led
export var tag: int = 0
export var number_leds: int = 16
export var stops = [0]
export var color_ledhole: Color = Color.black.lightened(0.1)
export var color_ledlight: Color = Color.orangered.lightened(0.4)


signal value_changed(value)
signal pressed(tag)

# Declare member variables here. Examples:
# var a = 2
# var b = "text"

var start_pos = Vector2(0,0)
var start_val: float
var engaged: bool = false
var float_value: float = 0
var total_distance: float = 0
var base_scale = Vector2(1, 1)
var pointer_angle = 230
var hover: bool = false

var pit: float = 0
var grace: float = 0

const CLICK_DISTANCE_THRESHOLD : float = 10.0

onready var label = $Label

onready var top = $Sprite
onready var bevel = $Bevel
onready var led = $Led
onready var pointer = $Pointer

# Called when the node enters the scene tree for the first time.
func _ready():
	label.text = caption
		
	update_sizes()	
	update_visuals()

	pointer.connect("draw", self, "_sprite_draw")

func _sprite_draw():	
	if pointer == null:
		return
	var x = 32 * cos(deg2rad(pointer_angle))
	var y = 32 * sin(deg2rad(pointer_angle))
	#var x = 48 * cos(deg2rad(pointer_angle))
	#var y = 48 * sin(deg2rad(pointer_angle))
	pointer.draw_circle(Vector2(x, y), 2 + int(hover), Color.white)

func _draw():
	if led == null:
		return
	for p in range(number_leds):
		var angle = -230 + 280 * p / (number_leds-1)
		var x = 62 * cos(deg2rad(angle))
		var y = 62 * sin(deg2rad(angle))
		draw_circle(led.position + base_scale * Vector2(x, y), base_scale.x * 2, color_ledhole)

		if angle <= pointer_angle:
			draw_circle(led.position + base_scale * Vector2(x, y), base_scale.x * 1.8, color_ledlight)

func _set_center_led(v):
	center_led = v
	update_visuals()

func _set_caption(v):
	caption = v
	if label != null:
		label.text = v	
		update_sizes()

func _set_caption_side(v):
	caption_side = v
	update_sizes()

func _set_min_value(v):
	min_value = v

func _set_max_value(v):
	max_value = v
	
func _set_value(v):
	value = clamp(v, min_value, max_value)
	float_value = value
	if rounded:
		value = round(value * round_precision) / round_precision
	update_visuals()
	emit_signal("value_changed", value)

func update_visuals():
	var value_range = max_value - min_value
	var offset_value = value - min_value
	var scaled = offset_value / value_range
	
	if top == null:
		return
	
	var frame = round(100 * scaled)	
	bevel.frame = frame
	led.frame = 1 - int(center_led)
	pointer_angle = -230 + 280 * scaled
	#print(pointer_angle)
	update()
	pointer.update()
	
	#$Text.text = "%+3.1fdB" % value

func update_sizes():
	if label == null:
		return
	if top == null:
		return		
		
	$bg.rect_size = rect_size
	label.visible = len(label.text) > 0
	var label_height = 0
	var label_width = 0
	if label.visible:
		label_height = label.rect_size.y
		label_width = label.rect_size.x

	var sz = min(rect_size.x, rect_size.y)
	if caption_side in [0, 2]:
		sz = min(rect_size.x, rect_size.y - label_height)
	else:
		sz = min(rect_size.x - label_width, rect_size.y)
	
	var w = $Sprite.texture.get_size()[0] / $Sprite.hframes
	var h = $Sprite.texture.get_size()[1] / $Sprite.vframes
	#print("sprite tex size=", w, " ", h, " rect_size=", rect_size, " scale=", sz/w)


	base_scale = Vector2(sz / w, sz / h)
	top.scale = base_scale
	bevel.scale = base_scale
	led.scale = base_scale
	pointer.scale = base_scale
	if label.visible:
		match caption_side:
			0: # buttom
				label.rect_position.x = rect_size.x / 2 - label.rect_size.x / 2
				label.rect_position.y = sz
				led.position = Vector2(rect_size.x / 2, sz / 2)
			2: # top
				label.rect_position.x = rect_size.x / 2 - label.rect_size.x / 2
				label.rect_position.y = 0
				led.position = Vector2(rect_size.x / 2, rect_size.y - sz / 2)
			1: # right
				led.position = Vector2(sz / 2, rect_size.y / 2)
				label.rect_position.x = sz
				label.rect_position.y = led.position.y - label.rect_size.y / 2
			3: # left
				led.position = Vector2(label.rect_size.x + sz / 2, rect_size.y / 2)
				label.rect_position.x = 0
				label.rect_position.y = rect_size.y / 2 - label.rect_size.y / 2
		#print(caption, " label@=", label.rect_position)
	top.position = led.position
	bevel.position = led.position
	pointer.position = led.position
	
	update() # redraw background
	
	
func _notification(what):
	if what == NOTIFICATION_MOUSE_ENTER:
		hover = true
		top.scale = base_scale * 0.95
		bevel.scale = base_scale * 1.02
		pointer.update()
	if what == NOTIFICATION_MOUSE_EXIT:
		hover = false
		top.scale = base_scale
		bevel.scale = base_scale
		engaged = false
		pointer.update()
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED) or \
		(what == NOTIFICATION_THEME_CHANGED):
		update_sizes()
	
func _gui_input(event):
	if event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == BUTTON_LEFT:
				#print("Click at ", event.position)
				start_pos = event.position
				start_val = value
				total_distance = 0
				engaged = true
			elif event.button_index == BUTTON_WHEEL_UP:
				#_set_value(float_value - speed * 2)
				move_knob(-speed * 2)
			elif event.button_index == BUTTON_WHEEL_DOWN:
				#_set_value(float_value + speed * 2) 
				move_knob(speed * 2)
		else:
			engaged = false
			if event.button_index == BUTTON_LEFT and total_distance < CLICK_DISTANCE_THRESHOLD:
				emit_signal("pressed", tag)
	if engaged and event is InputEventMouseMotion:
		#print("Motion to ", event.position)
		#var diff = (event.position.y - start_pos.y) * speed
		move_knob(-event.relative.y * speed)
		total_distance += event.relative.length()
		#_set_value(start_val - diff)

func move_knob(diff):
	if rounded:
		var rval = round(value * round_precision) / round_precision
		var falling = false
		for s in stops:
			if (float_value <= s && float_value + diff >= s) ||\
				(float_value >=s && float_value + diff <= s):
				falling = true
				float_value = s
				break
				
		if pit == 0 and grace <= 0 and falling:
			pit = 5
		
		if pit > 0:
			pit = pit - abs(diff)
			diff = 0
			if pit <= 0:
				pit = 0
				grace = 2
		elif grace > 0:
			grace = grace - abs(diff)
			if grace < 0:
				grace = 0
		
	_set_value(value + diff)
			
