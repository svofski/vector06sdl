tool
extends Panel

signal key_make(scancode)
signal key_break(scancode)

const longKeys = [300, 311, 400, 401, 403, 404] # also brown
const greenishKeys = [402, 50,51,52, 252, 200]
const mustardKeys = [150,151,152, 250,251, 352]
const spaceKey = 402

const top_text = [
			"; 1 2 3 4 5 6 7 8 9 0 - /",
			"Й Ц У К Е Н Г Ш Щ З Х :",
			"УС Ф Ы В А П Р О Л Д Ж Э .",
			"СС Я Ч С М И Т Ь Б Ю , ВК",
			"РУС ТАБ ___ ПС ЗБ"]
const bottom_text = [
			"+ ! \" # ¤ % & ' ( ) ___ = ?",
			"J C U K E N G [ ] Z H *",
			"___ F Y W A P R O L D V \\ >",
			"___ Q ^ S M I T X B @ < ___",
			"LAT ___ ___ ___ _"]
const num_text = [
			"ВВОД БЛК СБР",
			"F1 F2 F3",
			"F4 F5 АР2",
			"↖ ↑ СТР",
			"← ↓ →"];

const BLK = -1000

const scancodes = [
		   [KEY_SEMICOLON, KEY_1, KEY_2,
			KEY_3, KEY_4, KEY_5, KEY_6,
			KEY_7, KEY_8, KEY_9, KEY_0,
			KEY_EQUAL, KEY_SLASH],
			
			[KEY_J, KEY_C, KEY_U, KEY_K,
			 KEY_E, KEY_N, KEY_G, KEY_BRACKETLEFT,
			 KEY_BRACKETRIGHT, KEY_Z, KEY_H,
			 KEY_APOSTROPHE],

			[KEY_CONTROL, KEY_F, KEY_Y, KEY_W,
			 KEY_A, KEY_P, KEY_R, KEY_O,
			 KEY_L, KEY_D, KEY_V, KEY_BACKSLASH, KEY_PERIOD],
			

			[KEY_SHIFT, KEY_Q, KEY_QUOTELEFT, KEY_S,
			 KEY_M, KEY_I, KEY_T, KEY_X,
			 KEY_B, KEY_MINUS, KEY_COMMA, KEY_ENTER],

			[KEY_F6, KEY_TAB, KEY_SPACE,
			 KEY_ALT, KEY_BACKSPACE]]

const scancodes_num = [
			[KEY_F11, BLK, KEY_F12],
			[KEY_F1, KEY_F2, KEY_F3],
			[KEY_F4, KEY_F5, KEY_ESCAPE],
			[KEY_HOME, KEY_UP, KEY_END],
			[KEY_LEFT, KEY_DOWN, KEY_RIGHT]]

const scancodes_blk = [KEY_F11, BLK, KEY_F12]

const HSEPARATION = 4
const VSEPARATION = 4

onready var vbox = $HBoxContainer/VBoxContainer1
onready var vboxnum = $HBoxContainer/VBoxContainer2

onready var style_normal = $Normal.key_style
onready var style_lightpoop = $LightGreenPoop.key_style
onready var style_darkpoop = $DarkGreenPoop.key_style
onready var style_brownpoop = $BrownPoop.key_style

var scancode2key = {}
var pressed_keys = {}

func _ready():
	#var vbox1 = $HBoxContainer/VBoxContainer1
	#for i in range(vbox1.get_child_count()-1, -1, -1):
	#	var child = vbox1.get_child(i)
	#	vbox1.remove_child(child)
	create_keys()
	
func _notification(what):
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED) or \
		(what == NOTIFICATION_THEME_CHANGED):
		
		$HBoxContainer.add_constant_override("separation", $Normal.rect_min_size.x + HSEPARATION)	
		rect_min_size = Vector2(($Normal.rect_min_size.x + HSEPARATION) * 17,
			($Normal.rect_min_size.y + VSEPARATION) * 5)
		#rect_size = rect_min_size

func _get_key(code: int):
	var row = floor(code/100)
	var col = code % 100
	var num = 0
	if col >= 50:
		col -= 50
		num = 1
	
	if num == 0: 
		var marginbox = vbox.get_child(row)
		var hbox = marginbox.get_child(0)
		return hbox.get_child(col)
	else:
		var hbox = vboxnum.get_child(row)
		return hbox.get_child(col)

func create_keys():
	var height = 0
	
	var key
	vbox.add_constant_override("separation", VSEPARATION)
	vboxnum.add_constant_override("separation", VSEPARATION)
	
	for row in range(len(top_text)):
		var txt_top  = top_text[row].split(" ")
		var txt_bot = bottom_text[row].split(" ")
		var txt_num = num_text[row].split(" ")
		
		var marginbox = MarginContainer.new()
		var hbox = HBoxContainer.new()
		hbox.add_constant_override("separation", HSEPARATION)
		marginbox.add_child(hbox)
		
		vbox.add_child(marginbox)
		for col in range(len(txt_top)):
			var s1 = txt_top[col]
			var s2 = txt_bot[col]
			if s1 == "___":
				s1 = ""
			if s2 == "___":
				s2 = ""
			
			key = makekey(s1, s2, scancodes[row][col])
			hbox.add_child(key)
			key.text_color = Color.black
		#print("key size=", key.rect_size)
		height = height + key.rect_min_size.y + 2*vbox.get_constant("separation")
		if row == 1:
			marginbox.add_constant_override("margin_left", key.rect_size.x / 2)

		# second vbox, numpad
		var hbox2 = HBoxContainer.new()
		hbox2.add_constant_override("separation", HSEPARATION)
		vboxnum.add_child(hbox2)
		for col in range(len(txt_num)):
			key = makekey(txt_num[col], "", scancodes_num[row][col])
			hbox2.add_child(key)
			key.text_color = Color.black
			#key.key_style = style_normal

	rect_min_size = Vector2(rect_min_size.x, height)
	
	for l in longKeys:
		var key15 = _get_key(l)
		key15.rect_min_size.x = key.rect_min_size.x * 1.5
		key15.key_style = style_brownpoop
		key15.text_color = Color.white
	
	for l in [spaceKey]:
		var key7 = _get_key(l)
		key7.rect_min_size.x = (key.rect_min_size.x + HSEPARATION) * 7

	for l in mustardKeys:
		var k = _get_key(l)
		k.key_style = style_lightpoop
		
	for l in greenishKeys:
		var k = _get_key(l)
		k.key_style = style_darkpoop
				
func makekey(txt_top, txt_bot, scancode):
	var butt = preload("res://KeyboardKey.tscn").instance()
	butt.top_text = txt_top
	butt.bottom_text = txt_bot
	butt.scancode = scancode
	butt.connect("key_make", self, "_on_key_make")
	butt.connect("key_break", self, "_on_key_break")
	scancode2key[scancode] = butt
	return butt

func _on_key_make(scancode):
	pressed_keys[scancode] = true
	show_key_down(scancode)
	emit_signal("key_make", scancode)

func _on_key_break(scancode):
	pressed_keys.erase(scancode)
	show_key_up(scancode)
	emit_signal("key_break", scancode)
	
func show_key_down(scancode):
	var key = scancode2key[scancode]
	if key != null:
		key.visual_pressed = true
		
func show_key_up(scancode):
	var key = scancode2key[scancode]
	if key != null:
		key.visual_pressed = false

func all_keys_up():
	for scancode in pressed_keys.keys():
		show_key_up(scancode)
		emit_signal("key_break", scancode)
	pressed_keys.clear()
