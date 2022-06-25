extends PanelContainer

#export var grid_size: Vector2 = Vector2(0,0)

var shader_list = []
var tr_list = []
var texture: Texture = null
signal selected(num)

onready var container = find_node("Grid")

const MARGIN: int = 20

func _ready():
	pass

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

func _on_cell_pressed(num):
	emit_signal("selected", num)
		
