tool
extends PanelContainer

export var texture: Texture setget _set_texture
signal pressed

onready var desaturate_mat: ShaderMaterial = find_node("TextureButton").material
onready var texture_button: TextureButton = find_node("TextureButton")

func _ready():
	self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_normal"))	
	$TextureButton.connect("pressed", self, "_on_tb_pressed")
	
func _notification(what):
	if what == NOTIFICATION_MOUSE_ENTER:
		texture_button.material = null
		self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_hover"))	
	if what == NOTIFICATION_MOUSE_EXIT:
		self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_normal"))
		texture_button.material = desaturate_mat
		
func _on_tb_pressed():
	emit_signal("pressed")

func _set_texture(tex):
	texture = tex
	$TextureButton.texture_normal = tex
