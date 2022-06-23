extends Panel

#export var grid_size: Vector2 = Vector2(0,0)

var shader_list = []
var tr_list = []
var texture: Texture = null
signal selected(num)

onready var container = $Grid

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
	
	for i in range(len(shader_list)):
		var shadr = shader_list[i]		
		var tr = preload("res://ShaderCell.tscn").instance()
		tr_list.append(tr)
		container.add_child(tr)
		tr.texture = texture
		tr.caption = shadr
		tr.shader = load("res://shaders/%s.shader" % shadr)
		tr.connect("pressed", self, "_on_cell_pressed", [i])
	
	update_sizes()
		
func _on_cell_pressed(num):
	emit_signal("selected", num)
		
func update_sizes():
	var cell_size = Vector2((rect_size.x-MARGIN)/3, (rect_size.x-MARGIN*2)/4)
	for tr in tr_list:
		tr.rect_min_size = cell_size
	if container != null:
		#var grid_size = container.rect_size
		
		var hsep = container.get_constant("hseparation")
		var vsep = container.get_constant("vseparation")
		#print("container.child count=", container.get_child_count())
		var grid_size = Vector2((cell_size.x + hsep) * container.columns,
			(cell_size.y + vsep) * container.get_child_count() / container.columns)
		
		#print("shader_panel update_sizes() rect=", rect_size, " grid=", grid_size)
		# do our own "layout"
		container.rect_position = Vector2(MARGIN/2, 0) #Vector2(MARGIN/2,MARGIN/2)	
		rect_min_size = grid_size
		

func _notification(what):
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED) or \
		(what == NOTIFICATION_THEME_CHANGED):
		update_sizes()
