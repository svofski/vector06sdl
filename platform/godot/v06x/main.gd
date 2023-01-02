extends Control

const ROM : int = 0
const COM : int = 1
const FDD : int = 2
const EDD : int = 3
const WAV : int = 4
const DIR : int = 5

const MIX_SAMPLERATE : int = 48000

const V06X = preload("res://bin/v06x.gdns")
onready var v06x : V06X = V06X.new()

var texture : ImageTexture
var textureImage : Image
var playback : AudioStreamPlayback

var maintained_aspect = 5.0/4

onready var hud_panel = find_node("HUD")
onready var gamepad_label = find_node("GamepadLabel")
onready var rus_lat = find_node("RusLat")
onready var shader_sel = find_node("ShaderSelectButton")
onready var panel2 = find_node("Panel2")
onready var sound_panel = find_node("SoundPanel")
onready var osk_panel = find_node("OnScreenKeyboard")
onready var scope_panel = find_node("ScopePanel")
onready var mem_panel = find_node("MemoryView")
onready var nice_tooltip = find_node("NiceTooltip")
onready var loadass = [find_node("LoadAss2"), find_node("LoadAss3")]
onready var debugPanel = find_node("DebugPanel")

onready var tape_texture = loadass[0].texture
onready var floppy_texture = loadass[1].texture
onready var tape_size = loadass[0].rect_size
onready var floppy_size = loadass[1].rect_size

onready var default_scope_panel_pos = scope_panel.rect_position

var shader_index = 0
var shaders = []

onready var resume_timer: Timer = $ResumeTimer
onready var click_timer: Timer = $ClickTimer

# for saving / restoring config
var file_path = ["", ""]
var file_kind = [[ROM, 256, false], [ROM, 256, false]]
var loadedFilePath = ""

const DYNAMIC_SHADER_LIST = false

const SCOPE_SMALL: int = 0
const SCOPE_BIG: int = 1
var scope_state: int = SCOPE_SMALL

var dialog_device: int = 0 # A: or B: for file dialog
var debug_break_enabled = false

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
	debugPanel.main = self
	
	var cmdline_assets = ["", ""]
	var i: int = 0
	for arg in OS.get_cmdline_args():
		#print("arg: ", arg)
		if i < len(cmdline_assets) and not arg.begins_with("--"):
			cmdline_assets[i] = arg
			print("Will try to load asset: ", cmdline_assets[i])
			i = i + 1
	
	Engine.target_fps = 50
	Engine.iterations_per_second = 50
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

	get_tree().connect("files_dropped", self, "_on_files_dropped")

	#var should_reset: bool = false
	for k in range(len(cmdline_assets)):
		if cmdline_assets[k] == "":
			continue
		var rom_like = load_file(k, cmdline_assets[k])
		print("cmdline_assets[%d]=%s, rom_like=%s" % [k, cmdline_assets[k], str(rom_like)])
		#if k == 0 and rom_like: should_reset = true
	#if should_reset:
	#	call_deferred("_on_blksbr_pressed")
		
	update_load_asses()
		
	nice_tooltip.enabled = true

func _notification(what):
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
		
func _physics_process(delta):
	if debug_break_enabled:
		return
	
	v06x.SetJoysticks(poll_joy(0), poll_joy(1))
	
	var buttmap = v06x.ExecuteFrame()  # buttmap is 576x288, 32bpp
	updateTexture(buttmap)
	var sound = v06x.GetSound(MIX_SAMPLERATE / Engine.target_fps)  # playback.get_frames_available())
	update_playback(sound)
	rus_lat.set_lit(v06x.GetRusLat())

	if hud_panel == null:
		return

	if hud_panel.visible || scope_panel.get_parent() == self:
		scope_panel.update_texture(sound)

	if mem_panel.visible:
		mem_panel.ram = v06x.GetMem(0, 65536*5)
		mem_panel.heatmap = v06x.GetHeatmap(0, 65536*5)
		mem_panel.emit_signal("visibility_changed")

# it's impossible to tell where exactly the drop happens,
# mouse coordinates are all over the place
func _on_files_dropped(files: PoolStringArray, screen: int):
	if files.size() > 0:
		dialog_device = 0
		_on_FileDialog_file_selected(files[0])

func _on_load_asset_pressed(which: int):
	dialog_device = which
	$FileDialog.filters = ["*.rom,*.r0m,*.vec,*.bin,*.fdd,*.wav"]
	if which > 0:
		$FileDialog.filters = ["*.fdd"]
	$FileDialog.popup()

func update_debugger_size():
	var vert_available_size = hud_panel.rect_position.y
	if not hud_panel.visible:
		vert_available_size = get_viewport_rect().size.y
	if mem_panel.visible:
		mem_panel.rect_position = Vector2(0, 0)
		mem_panel.rect_size.y = vert_available_size
		debugPanel.debug_panel_size_update()
		#mem_panel.rect_size.x = 1000
		#print("update_debugger_size: mem_panel.rect_size is set to ", mem_panel.rect_size)

func update_crt_size(fit_to: Rect2):
	var sz = Vector2(fit_to.size.x, fit_to.size.x / maintained_aspect)
	if sz.y > fit_to.size.y:
		sz = Vector2(fit_to.size.y * maintained_aspect, fit_to.size.y)
	var centre = Vector2((fit_to.size.x - sz.x) / 2, (fit_to.size.y - sz.y) / 2)

	$VectorScreen.rect_size = sz
	$VectorScreen.rect_position = fit_to.position + centre

func update_scope_size(pos: Vector2, sz: Vector2):
	# if bigg
	#print("update_scope_size: rect_size=", sz)
	if scope_panel.get_parent() == self:
		scope_panel.rect_size = Vector2(sz.x, sz.x * 0.1)
		scope_panel.rect_position.y = sz.y - scope_panel.rect_size.y
		scope_panel.rect_position.x = pos.x

func _on_size_changed():
	var sz = get_viewport_rect().size # viewport maintains project size
	
	hud_panel.rect_position.y = sz.y - hud_panel.rect_size.y
	hud_panel.rect_position.x = (sz.x - hud_panel.rect_size.x) / 2
	if hud_panel.visible:
		sz.y -= hud_panel.rect_size.y

	update_debugger_size()

	var crt_rect = Rect2(Vector2(0, 0), sz)
	if mem_panel.visible:
		crt_rect = crt_rect.grow_individual(-mem_panel.rect_size.x, 0, 0, 0) 
	update_crt_size(crt_rect)

	if panel2.visible:
		shader_panel_visibility_changed()
		
	sz = get_viewport_rect().size
	if mem_panel.visible:
		sz.x = sz.x - mem_panel.rect_size.x
	if hud_panel.visible:
		sz.y = hud_panel.rect_position.y
	if scope_panel.visible:
		var pos = Vector2(0, 0)
		if mem_panel.visible:
			pos.x = mem_panel.rect_size.x
		update_scope_size(pos, sz)

	if mem_panel.visible:
		$VectorScreen.rect_position.x = mem_panel.rect_size.x

func place_shader_panel_please():
	if hud_panel.visible:
		panel2.rect_position.y = hud_panel.rect_position.y - panel2.rect_size.y
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

func make_load_hint(dev: int) -> String:
	match dev:
		0:  return "WAV/ROM/A: (%s)" % file_path[dev].get_file()
		1:  return "B: (%s)" % file_path[dev].get_file()
	return ""

func update_load_asses() -> void:
	var middle_x = rus_lat.rect_position.x + rus_lat.rect_size.x / 2
	loadass[0].hint_tooltip = make_load_hint(0)
	if file_kind[0][0] in [DIR, FDD]:
		loadass[0].texture = floppy_texture
		loadass[0].rect_size = floppy_size
		loadass[0].rect_position.x = middle_x - floppy_size.x / 2
	else:
		loadass[0].texture = tape_texture
		loadass[0].rect_size = tape_size
		loadass[0].rect_position.x = middle_x - tape_size.x / 2
	loadass[1].hint_tooltip = make_load_hint(1)

# return true if should autostart
func load_file(dev: int, path: String) -> bool:
	var file = File.new()
	if file.open(path, File.READ) == OK:
		var content = file.get_buffer(file.get_len())
		var korg = getKind(path)
		file_kind[dev] = korg
		file_path[dev] = path
		
		if file_kind[0][0] == FDD:
			v06x.Mount(dev, path)
		else:
			v06x.LoadAsset(content, korg[0], korg[1])
		update_load_asses()
		return korg[2]
	else:
		var dir = Directory.new()
		if dir.dir_exists(path):
			file_path[dev] = path
			file_kind[dev] = [DIR, 0, false]
			v06x.Mount(dev, path)	
			update_load_asses()
	return false

func _on_FileDialog_file_selected(path: String):
	print("File selected: ", path)
	loadedFilePath = path
	if load_file(dialog_device, path):
		v06x.Reset(false)
	
	nice_tooltip.showTooltip(loadass[dialog_device].rect_global_position, 
		path.get_file())

func ReloadFile():
	load_file(dialog_device, loadedFilePath)
	
func _on_FileDialog_dir_selected(dir):
	print("Directory selected: ", dir)
	loadedFilePath = dir
	load_file(dialog_device, dir)
	nice_tooltip.showTooltip(loadass[dialog_device].rect_global_position,
		dir.get_file())

func getKind(path : String):
	var ret = [ROM, 256, true]
	var ext = path.to_lower().get_extension()
	if ext == "rom" || ext == "com":
		ret = [ROM, 256, true]
	elif ext == "r0m":
		ret = [ROM, 0, true]
	elif ext == "fdd":
		ret = [FDD, 0, false]
	elif ext == "edd":
		ret = [EDD, 0, true]
	elif ext == "wav":
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
	if hud_panel.visible:
		panel2.visible = false
		hud_panel.visible = false
		hud_panel.rect_position.y = get_viewport_rect().size.y
		call_deferred("_on_size_changed")
	else:
		hud_panel.visible = true
		call_deferred("_on_size_changed")

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
	cfg.set_value("asset0", "file", file_path[0])
	cfg.set_value("asset0", "kind", file_kind[0][0])
	cfg.set_value("asset0", "org", file_kind[0][1])
	cfg.set_value("asset0", "autostart", file_kind[0][2])

	cfg.set_value("asset1", "file", file_path[1])
	cfg.set_value("asset1", "kind", file_kind[1][0])
	cfg.set_value("asset1", "org", file_kind[1][1])
	cfg.set_value("asset1", "autostart", file_kind[1][2])

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
			loadedFilePath = dir
		var path = cfg.get_value("FileDialog", "current_path")
		if path != null:
			$FileDialog.current_path = path
			loadedFilePath = path
		shader_index = cfg.get_value("shader", "index", 0)
		_on_shader_selected(shader_index)
		
		sound_panel.set_volume(0, cfg.get_value("sound", "volume_8253", 10))
		sound_panel.set_volume(1, cfg.get_value("sound", "volume_beep", 10))
		sound_panel.set_volume(2, cfg.get_value("sound", "volume_ay", 10))
		sound_panel.set_volume(3, cfg.get_value("sound", "volume_covox", 10))
		sound_panel.set_volume(4, cfg.get_value("sound", "volume_db", 0))
		_on_SoundPanel_volumes_changed()

		file_path[0] = cfg.get_value("asset0", "file", "")
		file_kind[0][0] = cfg.get_value("asset0", "kind", ROM)
		file_kind[0][1] = cfg.get_value("asset0", "org", 256)
		file_kind[0][2] = cfg.get_value("asset0", "autostart", false)
		
		if file_kind[0][0] in [FDD, DIR]:
			load_file(0, file_path[0]) # load but no restart

		file_path[1] = cfg.get_value("asset1", "file", "")
		file_kind[1][0] = cfg.get_value("asset1", "kind", FDD)
		file_kind[1][1] = cfg.get_value("asset1", "org", 256)
		file_kind[1][2] = cfg.get_value("asset1", "autostart", false)
		if file_kind[1][0] in [FDD, DIR]:
			load_file(1, file_path[1])

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
		1.5,
		sound_panel.get_timer_chan_enable(0),
		sound_panel.get_timer_chan_enable(1),
		sound_panel.get_timer_chan_enable(2),
		sound_panel.get_ay_chan_enable(0),
		sound_panel.get_ay_chan_enable(1),
		sound_panel.get_ay_chan_enable(2)
		)
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
	OS.window_fullscreen = not OS.window_fullscreen
	resume_timer.wait_time = 0.1
	resume_timer.start()

func make_scope_big():
	default_scope_panel_pos = scope_panel.rect_position
	scope_panel.get_parent().remove_child(scope_panel)
	add_child(scope_panel)
	call_deferred("_on_size_changed")
	scope_state = SCOPE_BIG

func make_scope_small():
	#scope_panel.prepare_to_resize()
	scope_panel.prepare_to_resize()
	scope_panel.rect_size = scope_panel.rect_min_size
	scope_panel.emit_signal("item_rect_changed")
	scope_panel.get_parent().remove_child(scope_panel)
	hud_panel.add_child(scope_panel)
	scope_panel.rect_position = default_scope_panel_pos
	scope_state = SCOPE_SMALL
	scope_panel.call_deferred("update_sizes")

func _on_ScopePanel_long_hover():
	pass

func _on_ScopePanel_pressed():
	if scope_state == SCOPE_SMALL:
		make_scope_big()
	else:
		make_scope_small()

func _on_BowserButton_pressed():
	mem_panel.visible = not mem_panel.visible
	call_deferred("_on_size_changed")

onready var rage_timer: Timer = find_node("RageTimer")
onready var bowser_button: Control = find_node("BowserButton")
onready var bowser_centre: Vector2 = bowser_button.rect_position + bowser_button.rect_size * 0.5

func _on_BowserButton_mouse_entered(enter0exit1: int):
	match enter0exit1:
		0: 	
			rage_timer.connect("timeout", self, "_bowser_rage")
			rage_timer.start(0.02)
		1:
			rage_timer.stop()
			rage_timer.disconnect("timeout", self, "_bowser_rage")
			_bowser_calm()
			
func _bowser_rage():
	bowser_button.rect_scale = Vector2(0.9 + randf() * 0.2, 0.9 + randf() * 0.2)
	bowser_button.rect_position = bowser_centre - bowser_button.rect_size * bowser_button.rect_scale / 2

func _bowser_calm():
	bowser_button.rect_scale = Vector2(1, 1)
	bowser_button.rect_position = bowser_centre - bowser_button.rect_size * bowser_button.rect_scale / 2

func debug_break_cont():
	debug_break_enabled = not debug_break_enabled
	if debug_break_enabled:
		debug_break()
	else:
		debug_continue()

func is_debug_break():
	return debug_break_enabled

func debug_break():
	return v06x.debug_break()
	
func debug_continue():
	return v06x.debug_continue()
	
func debug_step_into():
	return v06x.debug_step_into()
	
func debug_read_registers():
	return v06x.debug_read_registers()

func debug_disasm(addr, lines, lines_before_addr):
	return v06x.debug_disasm(addr, lines, lines_before_addr)

func debug_read_stack(lenght):
	return v06x.debug_read_stack(lenght)
	
func debug_insert_breakpoint(addr):
	return v06x.debug_insert_breakpoint(0, addr, 1)
	
func debug_remove_breakpoint(addr):
	return v06x.debug_remove_breakpoint(0, addr, 1)

func debug_is_break():
	return v06x.debug_is_break()










