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

export var chan_led_visible: PoolByteArray = [false, false, false] setget _set_chan_led_visible
export var chan_led_state: PoolByteArray = [false, false, false] setget _set_chan_led_state

signal value_changed(value)
signal pressed(tag)
signal chan_pressed(n)

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


const CLICK_DISTANCE_THRESHOLD : float = 10.0

onready var label = find_node("Label")
onready var top = find_node("Sprite")
onready var bevel = find_node("Bevel")
onready var led = find_node("Led")
onready var pointer = find_node("Pointer")

var chan_led = []

# Called when the node enters the scene tree for the first time.
func _ready():
	chan_led = [find_node("ChanLed1"), find_node("ChanLed2"), find_node("ChanLed3")]
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
	update()
	pointer.update()
	
	for i in range(len(chan_led)):
		chan_led[i].visible = chan_led_visible[i]
		chan_led[i].frame = 1 - int(chan_led_state[i])

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
	
	var w = top.texture.get_size()[0] / top.hframes
	var h = top.texture.get_size()[1] / top.vframes

	base_scale = Vector2(sz / w, sz / h)
	var chan_margin_x = 40 * base_scale.x
	top.scale = base_scale
	bevel.scale = base_scale
	led.scale = base_scale
	pointer.scale = base_scale
	if label.visible:
		match caption_side:
			0: # buttom
				label.rect_position.x = chan_margin_x + rect_size.x / 2 - label.rect_size.x / 2
				label.rect_position.y = sz
				led.position = Vector2(chan_margin_x + rect_size.x / 2, sz / 2)
			2: # top
				label.rect_position.x = chan_margin_x+ rect_size.x / 2 - label.rect_size.x / 2
				label.rect_position.y = 0
				led.position = Vector2(chan_margin_x + rect_size.x / 2, rect_size.y - sz / 2)
			1: # right
				led.position = Vector2(chan_margin_x + sz / 2, rect_size.y / 2)
				label.rect_position.x = chan_margin_x + sz
				label.rect_position.y = led.position.y - label.rect_size.y / 2
			3: # left
				led.position = Vector2(chan_margin_x + label.rect_size.x + sz / 2, rect_size.y / 2)
				label.rect_position.x = chan_margin_x
				label.rect_position.y = rect_size.y / 2 - label.rect_size.y / 2
		#print(caption, " label@=", label.rect_position)
	top.position = led.position
	bevel.position = led.position
	pointer.position = led.position

	for k in len(chan_led):
		chan_led[k].position.x = 16 * base_scale.x
	chan_led[1].position.y = led.position.y
	chan_led[0].position.y = led.position.y - 32 * base_scale.y
	chan_led[2].position.y = led.position.y + 32 * base_scale.y
	
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
				start_pos = event.position
				start_val = value
				total_distance = 0
				engaged = true
			elif event.button_index == BUTTON_WHEEL_UP:
				move_knob(-speed * 2)
			elif event.button_index == BUTTON_WHEEL_DOWN:
				move_knob(speed * 2)
		else:
			engaged = false
			if event.button_index == BUTTON_LEFT and total_distance < CLICK_DISTANCE_THRESHOLD:
				var done = false
				for k in range(len(chan_led)):
					var r: Rect2 = chan_led[k].get_rect()
					r.grow(-5)
					var loc: Vector2 = chan_led[k].to_local(event.global_position)
					#print("kth: ", k, " loc=", loc, "  has=", r.has_point(loc))
					if r.has_point(loc):
						emit_signal("chan_pressed", k)
						done = true
						break
				if not done:
					emit_signal("pressed", tag)
	if engaged and event is InputEventMouseMotion:
		move_knob(-event.relative.y * speed)
		total_distance += event.relative.length()

var depth: float = 0

func move_knob(diff):
	if depth > 0:
		depth = clamp(depth - abs(diff), 0, 10)
		if depth > 0:
			return
		depth = -1
		
	var next: float = value + diff
	if depth > -1:
		for s in stops:
			if (value <= s and next >= s) or (value >= s and next <= s):
				next = s
				depth = 5
				break
	if depth == -1:
		depth = 0

	_set_value(next)

func _set_chan_led_visible(v):
	for l in range(min(len(chan_led_visible), len(v))):
		chan_led_visible[l] = int(v[l])
	update_visuals()
	
func _set_chan_led_state(v):
	for l in range(min(len(chan_led), len(v))):
		chan_led_state[l] = int(v[l])
	update_visuals()
		
