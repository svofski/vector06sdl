extends Control

const ROM : int = 0
const COM : int = 1
const FDD : int = 2
const EDD : int = 3
const WAV : int = 4

const V06X = preload("res://bin/v06x.gdns")
onready var v06x = V06X.new()

var bytes

var texture
var textureImage
var playback

func updateTexture(buttmap : PoolByteArray):
	if textureImage == null:
		textureImage = Image.new()
		textureImage.create_from_data(576,288,false,Image.FORMAT_RGBA8,buttmap)
	
	if texture == null:
		texture = ImageTexture.new()
		texture.create(576, 288, Image.FORMAT_RGBA8, 7|Texture.FLAG_VIDEO_SURFACE)
		$VectorScreen.texture = texture

	textureImage.data.data = buttmap
	texture.set_data(textureImage)

func _ready():
	set_process(true)
	var beef = v06x.Init()
	print("beef: %08x" % beef)
	$VectorScreen.texture = texture
	
	get_tree().get_root().connect("size_changed", self, "onSizeChanged")	

	playback = $AudioStreamPlayer.get_stream_playback()
	$AudioStreamPlayer.play()
	onSizeChanged()
	
	load_state()

func _notification(what):
	if what == MainLoop.NOTIFICATION_WM_QUIT_REQUEST:
		save_state()


func refresh_files(item):
	$FileDialog._update_file_list()

func _process(delta):
	var buttmap = v06x.ExecuteFrame()  # buttmap is 576x288, 32bpp
	updateTexture(buttmap)
	#print("delta=", delta)
	#print(playback.get_frames_available())
	var sound = v06x.GetSound(960)  # playback.get_frames_available())
	update_playback(sound)
	$HUD/PanelContainer/MarginContainer/HBoxContainer/RusLat.set_lit(v06x.GetRusLat())

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
	$VectorScreen.rect_position = Vector2(sz[0]/2 - rs[0]/2,
		sz[1]/2 - rs[1]/2)
	$HUD.rect_size = sz

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

func _input(event: InputEvent):
	if $FileDialog.visible:
		return
	if event is InputEventKey:
		if event.pressed:
			if not event.echo: 
				print("KeyDown: %08x" % event.scancode)
				v06x.KeyDown(event.scancode)
			get_tree().set_input_as_handled()
		elif not event.pressed:
			v06x.KeyUp(event.scancode)
			get_tree().set_input_as_handled()
			
func _on_main_gui_input(event):
	if event is InputEventMouseButton:
		if event.pressed:
			$HUD.visible = not $HUD.visible

func update_playback(buffer : PoolVector2Array):
	playback.push_buffer(buffer)

func _on_Volume_value_changed(value):
	$AudioStreamPlayer.volume_db = value

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
