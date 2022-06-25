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

onready var gamepad_label = find_node("GamepadLabel")
onready var rus_lat = find_node("RusLat")
onready var shader_sel = find_node("ShaderSelectButton")
onready var panel2 = find_node("Panel2")
onready var sound_panel = find_node("SoundPanel")
onready var osk_panel = find_node("OnScreenKeyboard")
var shader_index = 0
var shaders = []

onready var resume_timer: Timer = $ResumeTimer
onready var click_timer: Timer = $ClickTimer

# for saving / restoring config
var file_path: String = ""
var file_kind = [ROM, 256, false]

const DYNAMIC_SHADER_LIST = false

func updateTexture(buttmap : PoolByteArray):
	if textureImage == null:
		textureImage = Image.new()
		textureImage.create_from_data(576,288,false,Image.FORMAT_RGBA8,buttmap)
		
	if texture == null:
		texture = ImageTexture.new()
		texture.create(576, 288, Image.FORMAT_RGBA8, Texture.FLAG_VIDEO_SURFACE) # no filtering
		$VectorScreen.texture = texture
		panel2.set_texture(texture)

	textureImage.data.data = buttmap
	texture.set_data(textureImage)

func _ready():
	Engine.target_fps = 50
	OS.set_use_vsync(false)
	#set_process(true)
	set_physics_process(true)
	var beef = v06x.Init()
	print("beef: %08x" % beef)
	$VectorScreen.texture = texture
	
	get_tree().get_root().connect("size_changed", self, "_on_size_changed")	

	playback = $AudioStreamPlayer.get_stream_playback()
	$AudioStreamPlayer.play()
		
	_on_size_changed()
	Input.connect("joy_connection_changed", self, "_on_joy_connection_changed")
	_on_joy_connection_changed(0, Input.get_joy_name(0) != "")
	_on_joy_connection_changed(1, Input.get_joy_name(1) != "")
	create_shader_list()
	if not DYNAMIC_SHADER_LIST:
		panel2.set_shader_list(shaders)

	load_state()

	# this timer is to resume audio after toggle fullscreen (avoid hiccups)
	resume_timer.one_shot = true
	resume_timer.connect("timeout", self, "_timer_resume")
	# this timer is to delay single clicks and get clean doubleclicks
	click_timer.wait_time = 0.25
	click_timer.one_shot = true
	click_timer.connect("timeout", self, "_on_click_timer")
	
	osk_panel.connect("key_make", self, "_osk_make")
	osk_panel.connect("key_break", self, "_osk_break")

	# the absolutely bloody insane dance to make sure
	# that grid cell sizes are updated at the time 
	# we attach panel2 on top of the main hud	
	panel2.connect("visibility_changed", self, "shader_panel_visibility_changed")
	panel2.connect("resized", self, "place_shader_panel_please")

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
		
#func _process(delta):
func _physics_process(delta):
	v06x.SetJoysticks(poll_joy(0), poll_joy(1))
	
	var buttmap = v06x.ExecuteFrame()  # buttmap is 576x288, 32bpp
	updateTexture(buttmap)
	var sound = v06x.GetSound(MIX_SAMPLERATE / Engine.target_fps)  # playback.get_frames_available())
	update_playback(sound)
	rus_lat.set_lit(v06x.GetRusLat())

func _on_load_asset_pressed():
	$FileDialog.popup()
	
func _on_size_changed():
	var sz = get_viewport_rect().size
	
	$HUD.rect_size.x = sz.x
	$HUD.rect_position.x = 0
	$HUD.rect_position.y = sz.y - $HUD.rect_size.y

	if $HUD.visible:
		sz.y -= $HUD.rect_size.y
		
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

	if panel2.visible:
		shader_panel_visibility_changed()

func place_shader_panel_please():
	if $HUD.visible:
		panel2.rect_position.y = $HUD.rect_position.y - panel2.rect_size.y
	else:
		panel2.rect_position.y = get_viewport_rect().size.y - panel2.rect_size.y

func shader_panel_visibility_changed():
	if panel2.visible:
		panel2.rect_position = $VectorScreen.rect_position
		panel2.rect_size = $VectorScreen.rect_size
		panel2.rect_size.y = 0
	else:
		if DYNAMIC_SHADER_LIST:
			panel2.set_shader_list([])
		
func show_shader_panel():
	if DYNAMIC_SHADER_LIST:
		panel2.set_shader_list(shaders)
	panel2.visible = true

func hide_shader_panel():
	panel2.visible = false

# return true if should autostart
func load_file(path):
	var file = File.new()
	if file.open(path, File.READ) == OK:
		var content = file.get_buffer(min(file.get_len(), 1024*1024))
		var korg = getKind(path)
		file_kind = korg
		file_path = path
		v06x.LoadAsset(content, korg[0], korg[1])
		return korg[2]
	return false

func _on_FileDialog_file_selected(path):
	print("File selected: ", path)
	if load_file(path):
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
	#print("make: ", scancode)
	v06x.KeyDown(scancode)
	
func _osk_break(scancode):
	#print("break: ", scancode)
	v06x.KeyUp(scancode)

func _input(event: InputEvent):
	if $FileDialog.visible:
		return
	if event is InputEventKey:
		if event.pressed:
			if not event.echo:
				if event.scancode == KEY_ENTER and event.alt:
					toggle_fullscreen()
				else:
					osk_panel._on_key_make(event.scancode, true)
			get_tree().set_input_as_handled()
		elif not event.pressed:
			osk_panel._on_key_break(event.scancode)
			get_tree().set_input_as_handled()

func _on_click_timer():
	if $HUD.visible:
		panel2.visible = false
		$HUD.visible = false
		_on_size_changed()
	else:
		$HUD.rect_position.y = get_viewport_rect().size.y
		$HUD.visible = true
		var anim = $AnimationPlayer.get_animation("hud_slide_in")
		anim.track_set_key_value(0, 0, Vector2(0, $HUD.rect_position.y))
		anim.track_set_key_value(0, 1, Vector2(0, get_viewport_rect().size.y - $HUD.rect_size.y))
		$AnimationPlayer.play("hud_slide_in")

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
	cfg.set_value("asset0", "file", file_path)
	cfg.set_value("asset0", "kind", file_kind[0])
	cfg.set_value("asset0", "org", file_kind[1])
	cfg.set_value("asset0", "autostart", file_kind[2])
	cfg.set_value("sound", "volume_8253", sound_panel.get_volume(0))
	cfg.set_value("sound", "volume_beep", sound_panel.get_volume(1))
	cfg.set_value("sound", "volume_ay", sound_panel.get_volume(2))
	cfg.set_value("sound", "volume_covox", sound_panel.get_volume(3))
	cfg.set_value("sound", "volume_db", sound_panel.get_volume(4))
	
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
		
		sound_panel.set_volume(0, cfg.get_value("sound", "volume_8253", 10))
		sound_panel.set_volume(1, cfg.get_value("sound", "volume_beep", 10))
		sound_panel.set_volume(2, cfg.get_value("sound", "volume_ay", 10))
		sound_panel.set_volume(3, cfg.get_value("sound", "volume_covox", 10))
		sound_panel.set_volume(4, cfg.get_value("sound", "volume_db", 0))
		_on_SoundPanel_volumes_changed()

		file_path = cfg.get_value("asset0", "file", "")
		file_kind[0] = cfg.get_value("asset0", "kind", ROM)
		file_kind[1] = cfg.get_value("asset0", "org", 256)
		file_kind[2] = cfg.get_value("asset0", "autostart", false)
		
		if file_kind[0] in [FDD]:
			load_file(file_path) # load but no restart


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


func _on_SoundPanel_volumes_changed():
	v06x.SetVolumes(sound_panel.get_volume(0),
		sound_panel.get_volume(1),
		sound_panel.get_volume(2),
		sound_panel.get_volume(3),
		1.5)
	$AudioStreamPlayer.volume_db = sound_panel.get_volume(4)

func _on_shaderselect_pressed():
	if panel2.visible:
		panel2.visible = false
	else:
		show_shader_panel()

func _timer_resume():
	$AudioStreamPlayer.stream_paused = false


func toggle_fullscreen():
	$AudioStreamPlayer.stream_paused = true
	#set_process(false)
	OS.window_fullscreen = not OS.window_fullscreen
	resume_timer.wait_time = 0.1
	resume_timer.start()
