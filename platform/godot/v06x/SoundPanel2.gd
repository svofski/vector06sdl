extends PanelContainer

signal volumes_changed

# Declare member variables here. Examples:
# var a = 2
# var b = "text"

onready var knobs = [$VBoxContainer/Knob8253, $VBoxContainer/KnobBeep, $VBoxContainer/KnobAY, $VBoxContainer/KnobCovox, $VBoxContainer/KnobMaster]

func _on_knob_changed(value):
	emit_signal("volumes_changed")

func _on_knob_pressed(tag):
	if tag < 0:
		return
	knobs[tag].center_led = !knobs[tag].center_led
	emit_signal("volumes_changed")

# Called when the node enters the scene tree for the first time.
func _ready():
	for i in range(len(knobs)):
		var k = knobs[i]
		k.tag = i
		k.connect("value_changed", self, "_on_knob_changed")
		k.connect("pressed", self, "_on_knob_pressed")
	pass # Replace with function body.

func get_volume(which: int):
	var mul = 0.01 * int(knobs[which].center_led)
	if which == 4: # master volume 
		if not knobs[which].center_led:
			return -INF
		else:
			return knobs[which].value
	return knobs[which].value * mul

func set_volume(which: int, value: float):
	var mul = 100
	if which == 4: # master, dB
		mul = 1
	knobs[which].value = value * mul
