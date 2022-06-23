extends Control

const ROM : int = 0
const COM : int = 1
const FDD : int = 2
const EDD : int = 3
const WAV : int = 4

const MIX_SAMPLERATE : int = 48000

const V06X = preload("res://bin/v06x.gdns")
onready var v06x : V06X = V06X.new()

var texture : ImageTexture
var textureImage : Image
var playback : AudioStreamPlayback

onready var gamepad_label = $HUD/PanelContainer/MarginContainer/HBoxContainer/GamepadLabel
onready var rus_lat = $HUD/PanelContainer/MarginContainer/HBoxContainer/RusLat
onready var shader_sel = $HUD/PanelContainer/MarginContainer/HBoxContainer/ShaderSelectButton
onready var volume_knob = $HUD/PanelContainer/MarginContainer/HBoxContainer/VolumeKnob
onready var panel2 = $HUD/Panel2
onready var sound_panel = $HUD/Panel2/SoundPanel
onready var shader_panel = $HUD/Panel2/ShaderPanel
onready var osk_panel = $HUD/Panel2/OnScreenKeyboard
var shader_index = 0
var shaders = []

onready var resume_timer: Timer = $ResumeTimer
onready var click_timer: Timer = $ClickTimer

func updateTexture(buttmap : PoolByteArray):
	if textureImage == null:
		textureImage = Image.new()
		textureImage.create_from_data(576,288,false,Image.FORMAT_RGBA8,buttmap)
		
	if texture == null:
		texture = ImageTexture.new()
		texture.create(576, 288, Image.FORMAT_RGBA8, Texture.FLAG_VIDEO_SURFACE) # no filtering
		$VectorScreen.texture = texture
		shader_panel.set_texture(texture)

	textureImage.data.data = buttmap
	texture.set_data(textureImage)

func _ready():
	Engine.target_fps = 50
	OS.set_use_vsync(false)
	set_process(true)
	var beef = v06x.Init()
	print("beef: %08x" % beef)
	$VectorScreen.texture = texture
	
	get_tree().get_root().connect("size_changed", self, "onSizeChanged")	

	playback = $AudioStreamPlayer.get_stream_playback()
	$AudioStreamPlayer.play()
		
	onSizeChanged()
	Input.connect("joy_connection_changed", self, "_on_joy_connection_changed")
	_on_joy_connection_changed(0, Input.get_joy_name(0) != "")
	_on_joy_connection_changed(1, Input.get_joy_name(1) != "")
	create_shader_list()
	shader_panel.set_shader_list(shaders)

	load_state()
	_on_SoundPanel_volumes_changed()

	# this timer is to resume audio after toggle fullscreen (avoid hiccups)
	resume_timer.one_shot = true
	resume_timer.connect("timeout", self, "_timer_resume")
	# this timer is to delay single clicks and get clean doubleclicks
	click_timer.wait_time = 0.25
	click_timer.one_shot = true
	click_timer.connect("timeout", self, "_on_click_timer")
	
	$HUD/Panel2/OnScreenKeyboard.connect("key_make", self, "_osk_make")
	$HUD/Panel2/OnScreenKeyboard.connect("key_break", self, "_osk_break")
	
	
func _notification(what):
	#print("notification: ", what)
	if what == MainLoop.NOTIFICATION_WM_QUIT_REQUEST:
		set_process(false)
		$AudioStreamPlayer.stop()
		save_state()
	if what == NOTIFICATION_WM_FOCUS_OUT:
		osk_panel.all_keys_up()

# Called whenever a joypad has been connected or disconnected.
func _on_joy_connection_changed(device_id, connected):
	if device_id == 0:
		gamepad_label.name1 = Input.get_joy_name(device_id)
		gamepad_label.enabled1 = connected 
	if device_id == 1:
		gamepad_label.name2 = Input.get_joy_name(device_id)
		gamepad_label.enabled2 = connected

func refresh_files(item):
	$FileDialog._update_file_list()

func poll_joy(cur_joy):
	return ~(int(Input.is_joy_button_pressed(cur_joy, JOY_DPAD_RIGHT)) \
		| (int(Input.is_joy_button_pressed(cur_joy, JOY_DPAD_LEFT)) << 1) \
		| (int(Input.is_joy_button_pressed(cur_joy, JOY_DPAD_UP)) << 2) \
		| (int(Input.is_joy_button_pressed(cur_joy, JOY_DPAD_DOWN)) << 3) \
		| (int(Input.is_joy_button_pressed(cur_joy, JOY_XBOX_A)) << 6) \
		| (int(Input.is_joy_button_pressed(cur_joy, JOY_XBOX_B)) << 7))
		
func _process(delta):
	v06x.SetJoysticks(poll_joy(0), poll_joy(1))
	
	var buttmap = v06x.ExecuteFrame()  # buttmap is 576x288, 32bpp
	updateTexture(buttmap)
	var sound = v06x.GetSound(MIX_SAMPLERATE / Engine.target_fps)  # playback.get_frames_available())
	update_playback(sound)
	rus_lat.set_lit(v06x.GetRusLat())

func _on_Button2_pressed():
	$FileDialog.popup()
	
func onSizeChanged():
	var sz = get_viewport_rect().size
	var maintained_aspect = 5.0/4
	var aspect = sz[0]/sz[1]
	var rs = self.rect_size
	if aspect > maintained_aspect:
		# wider than needed
		rs = Vector2(sz[1] * maintained_aspect, sz[1])
	else:
		# taller than needed
		rs = Vector2(sz[0], sz[0] * 1/maintained_aspect)
	self.rect_size = sz
	$VectorScreen.rect_size = rs
	$VectorScreen.rect_position = Vector2(sz[0]/2 - rs[0]/2, 0)
		#sz[1]/2 - rs[1]/2)
	$HUD.rect_size.x = rs.x
	$HUD.rect_size.y = rect_size.y
	$HUD.rect_position.x = $VectorScreen.rect_position.x
	$HUD.rect_position.y = 0 #rect_size.y - $HUD.rect_size.y

func _on_FileDialog_file_selected(path):
	print("File selected: ", path)
	var file = File.new()
	file.open(path, File.READ)	
	var content = file.get_buffer(min(file.get_len(), 1024*1024))
	var korg = getKind(path)
	v06x.LoadAsset(content, korg[0], korg[1])
	if korg[2]:
		v06x.Reset(false)

func getKind(path : String):
	var ret = [ROM, 256, true]
	var ext = path.to_lower().get_extension()
	if ext == "rom" || ext == "com":
		ret = [ROM, 256, true]
	if ext == "fdd":
		ret = [FDD, 0, false]
	if ext == "edd":
		ret = [EDD, 0, true]
	if ext == "wav":
		ret = [WAV, 0, false]
	return ret

func _on_blkvvod_pressed():
	v06x.Reset(true)

func _on_blksbr_pressed():
	v06x.Reset(false)

func _osk_make(scancode):
	print("make: ", scancode)
	v06x.KeyDown(scancode)
	
func _osk_break(scancode):
	print("break: ", scancode)
	v06x.KeyUp(scancode)

func _input(event: InputEvent):
	if $FileDialog.visible:
		return
	if event is InputEventKey:
		if event.pressed:
			if not event.echo:
				print("KeyDown: %08d" % event.scancode)
				if event.scancode == KEY_ENTER and event.alt:
					toggle_fullscreen()
				else:
					osk_panel._on_key_make(event.scancode)
					#v06x.KeyDown(event.scancode)
					#osk_panel.show_key_down(event.scancode)
			get_tree().set_input_as_handled()
		elif not event.pressed:
			#v06x.KeyUp(event.scancode)
			osk_panel._on_key_break(event.scancode)
			#osk_panel.show_key_up(event.scancode)
			get_tree().set_input_as_handled()

func _on_click_timer():
	$HUD.visible = not $HUD.visible
	
func _on_main_gui_input(event):
	if event is InputEventMouseButton and event.button_index == BUTTON_LEFT:
		if event.pressed:
			if event.doubleclick:
				click_timer.stop()
				toggle_fullscreen()
			else:
				click_timer.start()

func update_playback(buffer : PoolVector2Array):
	playback.push_buffer(buffer)

func save_state():
	var state = v06x.ExportState()
	#print("save_state: ", state)
	var file = File.new()
	file.open("user://v06x.savestate", File.WRITE)
	file.store_buffer(state)
	file.close()

	save_config()	

func load_state():
	var file = File.new()
	if not file.file_exists("user://v06x.savestate"):
		return false
	
	file.open("user://v06x.savestate", File.READ)
	var buffer = file.get_buffer(file.get_len())
	var res = v06x.RestoreState(buffer)

	load_config()

func save_config():
	var cfg = ConfigFile.new()
	cfg.load("user://v06x.settings")
	cfg.set_value("FileDialog", "current_dir", $FileDialog.current_dir)
	cfg.set_value("FileDialog", "current_path", $FileDialog.current_path)
	cfg.set_value("sound", "volume_db", $AudioStreamPlayer.volume_db)
	cfg.set_value("shader", "index", shader_index)
	cfg.save("user://v06x.settings")

func load_config():
	var cfg = ConfigFile.new()
	var err = cfg.load("user://v06x.settings")
	if err == OK:
		var dir = cfg.get_value("FileDialog", "current_dir")
		if dir != null:
			$FileDialog.current_dir = dir
		var path = cfg.get_value("FileDialog", "current_path")
		if path != null:
			$FileDialog.current_path = path
		shader_index = cfg.get_value("shader", "index", 0)
		_on_shader_selected(shader_index)
		
		volume_knob.value = cfg.get_value("sound", "volume_db", 0)
		_on_Volume_value_changed(volume_knob.value)

func create_shader_list():
	var dir = Directory.new()
	if dir.open("res://shaders/") == OK:
		dir.list_dir_begin()
		var file_name : String = dir.get_next()
		while file_name != "":
			if file_name.ends_with(".shader"):
				var shadr = file_name.get_file().trim_suffix(".shader")
				shaders.push_back(shadr)
			file_name = dir.get_next()
		dir.list_dir_end()
		

func _on_shader_selected(num):
	shader_index = num
	var shader_name = shaders[num]
	var mat:ShaderMaterial = $VectorScreen.material
	mat.shader = load("res://shaders/%s.shader" % shader_name)
	panel2.visible = false
	
# Sound stuffs: Volume etc

func _on_Volume_value_changed(value):
	$AudioStreamPlayer.volume_db = value
	volume_knob.hint_tooltip = "Громкость: %+3.0fdB" % value

func _on_SoundPanel_volumes_changed():
	v06x.SetVolumes(sound_panel.get_volume(0),
		sound_panel.get_volume(1),
		sound_panel.get_volume(2),
		sound_panel.get_volume(3),
		sound_panel.get_volume(4))

func _on_oskeyboard_pressed():
	if panel2.visible and osk_panel.visible:
		panel2.visible = false
	elif not panel2.visible:
		panel2.visible = true
	if panel2.visible:
		sound_panel.visible = false
		shader_panel.visible = false
		osk_panel.visible = true

func _on_shaderselect_pressed():
	if panel2.visible and shader_panel.visible:
		panel2.visible = false	
	elif not panel2.visible:
		panel2.visible = true
	if panel2.visible:
		osk_panel.visible = false
		sound_panel.visible = false
		shader_panel.visible = true
		print("shader min_size=", shader_panel.rect_min_size)
		#shader_panel.rect_min_size = shader_panel.grid_size

func _on_VolumeKnob_pressed(tag):
	if panel2.visible and sound_panel.visible:
		panel2.visible = false	
	elif not panel2.visible:
		panel2.visible = true
	if panel2.visible:
		osk_panel.visible = false
		shader_panel.visible = false
		sound_panel.visible = true
		#panel2.rect_min_size.y = sound_panel.rect_min_size.y
		
func _timer_resume():
	$AudioStreamPlayer.stream_paused = false
	#set_process(true)		
		
func toggle_fullscreen():
	$AudioStreamPlayer.stream_paused = true
	#set_process(false)
	OS.window_fullscreen = not OS.window_fullscreen
	resume_timer.wait_time = 0.1
	resume_timer.start()
