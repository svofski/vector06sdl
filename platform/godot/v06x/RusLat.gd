extends Label

var stylebox: StyleBoxTexture

# Called when the node enters the scene tree for the first time.
func _ready():
	stylebox = self.get_stylebox("normal")
	set_lit(false)

func set_lit(lit: bool):
	if lit:
		stylebox.region_rect = Rect2(0, 64, 64, 64)
	else:
		stylebox.region_rect = Rect2(0, 0, 64, 64)
