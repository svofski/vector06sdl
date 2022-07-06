extends PanelContainer

signal volumes_changed

const TOOLTIP_NODE_PATH = "/root/main/NiceTooltip"

onready var knobs = [$VBoxContainer/Knob8253, $VBoxContainer/KnobBeep, $VBoxContainer/KnobAY, $VBoxContainer/KnobCovox, $VBoxContainer/KnobMaster]

func nice_tooltip():
	var tt = get_tree().get_root().get_node(TOOLTIP_NODE_PATH)
	return tt

func _on_knob_changed(value, index):
	update_hint(index, value)
	emit_signal("volumes_changed")

func _on_knob_pressed(tag, index):
	if tag < 0:
		return
	knobs[tag].center_led = !knobs[tag].center_led
	emit_signal("volumes_changed")

func _on_chan_pressed(chan, knob):
	if knob != 0 and knob != 2:
		return
	knobs[knob].chan_led_state[chan] = 1 - knobs[knob].chan_led_state[chan]
	emit_signal("volumes_changed")

func update_hint(index, value):
	var text = ""
	match index:
		0: text = "ВИ53\n%3.1f" % value
		1: text = "Бипер\n%3.1f" % value
		2: text = "AY-3-8910\n%3.1f" % value
		3: text = "Covox\n%3.1f" % value
		4: text = "Усиление\n%3.1fdB" % value
	if text != "":
		var pos = knobs[index].get_global_rect().position
		nice_tooltip().showTooltip(pos, text)
	#knobs[0].hint_tooltip = "ВИ53: %3.1f" % knobs[0].value

# Called when the node enters the scene tree for the first time.
func _ready():
	for i in range(len(knobs)):
		var k = knobs[i]
		k.tag = i
		k.connect("value_changed", self, "_on_knob_changed", [i])
		k.connect("pressed", self, "_on_knob_pressed", [i])


	knobs[0].chan_led_visible = [true, true, true]
	knobs[0].chan_led_state = [true, true, true]
	knobs[0].connect("chan_pressed", self, "_on_chan_pressed", [0])
	
	knobs[2].chan_led_state = [true, true, true]
	knobs[2].connect("chan_pressed", self, "_on_chan_pressed", [2])
	
func get_volume(which: int):
	var mul = 0.01 * int(knobs[which].center_led)
	if which == 4: # master volume 
		if not knobs[which].center_led:
			return -INF
		else:
			return knobs[which].value
	return knobs[which].value * mul
	
func get_timer_chan_enable(chan: int) -> bool:
	return bool(knobs[0].chan_led_state[chan])

func get_ay_chan_enable(chan: int) -> bool:
	return bool(knobs[2].chan_led_state[chan])

func set_volume(which: int, value: float):
	var mul = 100
	if which == 4: # master, dB
		mul = 1
	knobs[which].value = value * mul
	
func update_sizes():
	if knobs == null:
		return
	var scale = rect_size.x / 120.0
	var font_size = 10 * scale
	for k in knobs:
		k.rect_min_size = Vector2(32 * scale, 32 * scale)
		k.find_node("Label").get("custom_fonts/font").size = font_size
	#for k in knobs:
	#	k.update_sizes()
	
func _notification(what):
	if what == NOTIFICATION_RESIZED:
		update_sizes()
