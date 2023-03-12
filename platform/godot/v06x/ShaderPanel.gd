extends PanelContainer

var shader_list = []
var tr_list = []
var texture: Texture = null
signal selected(num)
signal aspect_ratio_selected(index, aspect)
signal pixel_scale_selected(index, scale)

onready var container = find_node("Grid")
onready var aspect_selector = find_node("AspectRatio")
onready var pixelscale_selector = find_node("PixelScale")

const MARGIN: int = 20

func _ready():
	aspect_selector.add_item("1:1", 0)
	aspect_selector.add_item("2:1", 1)
	aspect_selector.add_item("5:4", 2)
	aspect_selector.add_item("4:3", 3)
	
	pixelscale_selector.add_item("1", 0)
	pixelscale_selector.add_item("1.5", 1)
	pixelscale_selector.add_item("2", 2)
	pixelscale_selector.add_item("2.5", 3)
	pixelscale_selector.add_item("3", 4)
	pixelscale_selector.add_item("4", 5)
	pixelscale_selector.add_item("5", 5)

func set_aspect_index(index: int) -> void:
	aspect_selector.selected = index
	
func set_pixelscale_index(index: int) -> void:
	pixelscale_selector.selected = index

func set_texture(tex):
	texture = tex
	for tr in tr_list:
		tr.texture = tex
	
func set_shader_list(shaders):
	shader_list = shaders
	update_controls()
	
func update_controls():
	for tr in tr_list:
		container.remove_child(tr)
		tr.set_owner(null)
	tr_list.clear()
	#for c in container.get_children():
	#	container.remove_child(c)
	
	for i in range(len(shader_list)):
		var shadr = shader_list[i]		
		var tr = preload("res://ShaderCell.tscn").instance()
		tr_list.append(tr)
		container.add_child(tr)
		tr.texture = texture
		tr.caption = shadr
		tr.shader = load("res://shaders/%s.shader" % shadr)
		tr.connect("pressed", self, "_on_cell_pressed", [i])

	aspect_selector.connect("item_selected", self, "_on_aspect_ratio_selected")
	pixelscale_selector.connect("item_selected", self, "_on_pixelscale_selected")
	
func _on_cell_pressed(num):
	emit_signal("selected", num)

func _on_aspect_ratio_selected(id):
	var text = aspect_selector.get_item_text(aspect_selector.selected)
	var intpart = float(text.substr(0, text.find(":")))
	var fracpart = float(text.substr(text.find(":") + 1, text.length()))
	#print("aspect ratio selected: ", text, "=", intpart/fracpart)	
	for shader_cell in tr_list:
		shader_cell.aspect = intpart/fracpart
		shader_cell.call_deferred("resized")
	emit_signal("aspect_ratio_selected", aspect_selector.selected, intpart/fracpart)

func _on_pixelscale_selected(id):
	var scale = float(pixelscale_selector.get_item_text(pixelscale_selector.selected))
	emit_signal("pixel_scale_selected", pixelscale_selector.selected, scale)
