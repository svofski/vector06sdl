tool
extends PanelContainer

export var texture: Texture setget _set_texture
export var shader: Shader setget _set_shader
export var caption: String setget _set_caption

signal pressed

var tr: TextureRect
var label: Label
var engaged = false
var hover = false

func _ready():
	tr = $TextureRect
	label = $Label
	self.add_stylebox_override("panel", self.get_stylebox("cell"))
	_set_texture(texture)
	_set_shader(shader)
	_set_caption(caption)

func _set_texture(t):
	texture = t
	if tr != null:
		tr.texture = t

func _set_shader(s):
	shader = s
	if tr != null:
		var mat:ShaderMaterial = tr.material
		if mat == null:
			mat = ShaderMaterial.new()
			tr.material = mat
		mat.shader = s
	
func _set_caption(c):
	caption = c
	if label != null:
		label.text = c

func _on_mouse_entered():
	self.add_stylebox_override("panel", self.get_stylebox("hover"))

func _on_mouse_exited():
	self.add_stylebox_override("panel", self.get_stylebox("cell"))

func _gui_input(event):
	if engaged and event is InputEventMouseMotion:
		var r = Rect2(0, 0, rect_size.x, rect_size.y)
		if hover and not r.has_point(event.position):
			hover = false
			_on_mouse_exited()
		if not hover and r.has_point(event.position):
			hover = true
			_on_mouse_entered()
	
	if event is InputEventMouseButton:
		if event.pressed:
			if event.button_index == BUTTON_LEFT:
				engaged = true
				hover = true
		else:
			engaged = false
			if hover and event.button_index == BUTTON_LEFT:
				emit_signal("pressed")	


# maintain aspect
func _on_resized():
	rect_min_size.y = rect_size.x * 4 / 5
