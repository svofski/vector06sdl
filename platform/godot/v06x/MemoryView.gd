extends PanelContainer

onready var dump = find_node("Dump")
onready var bitmaps = find_node("Bitmaps")

onready var hex_container = find_node("ScrollContainer")
onready var tex_container = find_node("ScrollContainer2") 

var ram: PoolByteArray
var heatmap: PoolByteArray

const MODE_HEX = 0
const MODE_TEX = 1

var mode = MODE_HEX

# Called when the node enters the scene tree for the first time.
func _ready():
	make_dummy_data()
	update_visuals()
	
func make_dummy_data():
	ram.resize(65536 * 5)
	heatmap.resize(65536 * 5)
	for i in range(len(ram)):
		ram[i] = 128 >> (i % 8)# 0xAA # i & 255 # randi() & 255
		heatmap[i] = i >> 8


func _on_PanelContainer_visibility_changed():
	dump.emit_signal("visibility_changed")
	bitmaps.update_texture()


func toggle_mode():
	mode = MODE_HEX if mode == MODE_TEX else MODE_TEX
	update_visuals()
	
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
