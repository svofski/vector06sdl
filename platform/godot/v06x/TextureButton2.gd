tool
extends PanelContainer

export var texture: Texture setget _set_texture
signal pressed

func _ready():
	self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_normal"))	
	$TextureButton.connect("pressed", self, "_on_tb_pressed")
	
func _notification(what):
	if what == NOTIFICATION_MOUSE_ENTER:
		self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_hover"))	
	if what == NOTIFICATION_MOUSE_EXIT:
		self.add_stylebox_override("panel", self.get_stylebox("texturebutton2_normal"))
	
func _on_tb_pressed():
	emit_signal("pressed")

func _set_texture(tex):
	texture = tex
	$TextureButton.texture_normal = tex
