extends Label

export var enabled1 := false setget _set_enabled1
export var enabled2 := false setget _set_enabled2
export var name1 := "" setget _set_name1
export var name2 := "" setget _set_name2

var stylebox: StyleBoxTexture

# Declare member variables here. Examples:
# var a = 2
# var b = "text"


# Called when the node enters the scene tree for the first time.
func _ready():
	stylebox = self.get_stylebox("normal")
	$Second.add_stylebox_override("normal", stylebox)
	_set_enabled1(enabled1)
	_set_enabled2(enabled2)
	pass # Replace with function body.


func _set_enabled1(v):
	#stylebox.modulate_color
	#print("Gamepad._set_enabled v=", v)
	enabled1 = v
	self.modulate.a = 1 if (enabled1 or enabled2) else 0.25
	$Second.visible = enabled1 and enabled2
	pass

func _set_enabled2(v):
	#stylebox.modulate_color
	#print("Gamepad._set_enabled v=", v)
	enabled2 = v
	self.modulate.a = 1 if (enabled1 or enabled2) else 0.25
	$Second.visible = enabled1 and enabled2
	pass

func update_hint():
	var msg : String
	if name1 != "":
		msg = "1: " + name1
	if name2 != "":
		if len(msg) > 0:
			msg += "\n"
		msg += "2: " + name2
	if len(msg) == 0:
		msg = "Геймпады не подключены"
	self.hint_tooltip = msg

func _set_name1(n):
	name1 = n
	update_hint()
	
func _set_name2(n):
	name2 = n
	update_hint()

# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass
