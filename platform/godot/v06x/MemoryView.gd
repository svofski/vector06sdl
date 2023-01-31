extends PanelContainer

onready var dump = find_node("Dump")
onready var bitmaps = find_node("Bitmaps")
onready var hint_label = find_node("HintLabel")

onready var hex_container = find_node("ScrollContainer")
onready var tex_container = find_node("ScrollContainer2") 

var ram: PoolByteArray
var heatmap: PoolByteArray

const MODE_HEX = 0
const MODE_TEX = 1

var mode = MODE_HEX

# Called when the node enters the scene tree for the first time.
func _ready():
	dump.set_model(self)
	bitmaps.set_model(self)
	
	bitmaps.connect("mouse_addr_changed", self, "_on_mouse_addr_changed", [bitmaps])
	dump.connect("mouse_addr_changed", self, "_on_mouse_addr_changed", [dump])
	
	make_dummy_data()
	update_visuals()
	
func make_dummy_data():
	ram.resize(65536 * 5)
	heatmap.resize(65536 * 5)
	for i in range(len(ram)):
		ram[i] = 128 >> (i % 8)# 0xAA # i & 255 # randi() & 255
		heatmap[i] = i >> 8

func _on_mouse_addr_changed(from):
	var a = from.mouse_addr
	var extra_hint: String = "RAM"
	if a >= 0x8000 and a < 0x10000:
		extra_hint = "VRAM Page %d" % ((a - 32768) / 8192)
	elif a >= 0x10000:
		extra_hint = "KVAZ Page %d" % ((a - 65536) / 65536)
	hint_label.text = "%04x %s" % [a & 65535, extra_hint]

func _on_PanelContainer_visibility_changed():
	dump.emit_signal("visibility_changed")
	bitmaps.update_texture()

func toggle_mode():
	mode = MODE_HEX if mode == MODE_TEX else MODE_TEX
	update_visuals()
	if mode == MODE_HEX:
		dump.scroll_to_addr(bitmaps.mouse_addr)
	else:
		bitmaps.scroll_to_addr(dump.mouse_addr)

func update_visuals():
	match mode:
		MODE_HEX: 
			tex_container.visible = false
			hex_container.visible = true
		MODE_TEX:
			hex_container.visible = false
			tex_container.visible = true
	get_tree().get_root().emit_signal("size_changed")

func _on_gui_input(event):
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_LEFT and not event.pressed:
			toggle_mode()			

