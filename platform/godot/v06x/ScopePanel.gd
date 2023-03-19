extends PanelContainer

signal long_hover
signal pressed

export var frame_length: int = 960

var sound_image: Image	# several frames of sound input
var sound_texture: ImageTexture	# texture for shader input
var frame_image: Image # current sound frame (1 row)
var sp: StreamPeerBuffer = StreamPeerBuffer.new()

const NFRAMES = 4
var frame_index: int = 0

var paused = false
var hover = false
var engaged = false
var long_hover_fired = false

onready var timer = find_node("Timer")
onready var texture_rect = find_node("TextureRect")

var frame_count: int = 0
var scale: float = 1.0
var midpoint: float = 0.0
var decimated = []

func _ready():
	update_sizes()
	
	var mat: ShaderMaterial = ShaderMaterial.new()
	mat.shader = load("res://scope/scope.shader")
	texture_rect.material = mat
	
	timer.one_shot = true
	timer.connect("timeout", self, "_on_timeout")
	
func update_sizes():
	if texture_rect == null: return
	
	var trimg = Image.new()
	trimg.create(rect_size.x, rect_size.y, false, Image.FORMAT_RGB8)
	var trtex = ImageTexture.new()
	trtex.create_from_image(trimg)
	texture_rect.texture = trtex

var decimated_pos = 0

func update_texture(frame_data: PoolVector2Array):
	if paused:
		return

	var i : int = 0
	var end: int = len(frame_data)
	if len(decimated) != frame_length:
		decimated.resize(frame_length)

	while (i < end) && (decimated_pos < frame_length):
		decimated[decimated_pos] = frame_data[i].x
		i += 4
		decimated_pos += 1
	
	if decimated_pos < frame_length:
		return
	decimated_pos -= frame_length
	
	frame_count += 1
	var arr_min = decimated.min()
	var arr_max = decimated.max()
	midpoint = 0.5 * (midpoint + 0.5 * (arr_min + arr_max))
	var new_scale = 0.75 / (arr_max - arr_min) if (arr_max - arr_min > 1e-6) else 1.0
	if new_scale < scale:
		scale = new_scale
	else:
		#scale = 0.5 * (scale + new_scale)
		scale = scale * 1.01

	i  = 0
	#while (i < 960/4) and abs(decimated[i] - midpoint) > scale * 0.01:
	#	i += 4
		
	while (i < 960/4) and decimated[i] > midpoint:
		i += 4
	while (i < 960/4) and decimated[i] < midpoint:
		i += 4
		
	var trigger: int = i
	#var trigger: int = 0
	#print("trigger=", trigger)

	var pa = PoolRealArray(decimated)
	sp.put_var(pa)
	# the raw data as follows:
	# 4 bytes: number of bytes in raw chunk
	# 4 bytes: type of chunk (24 = PoolVector2Array)
	# 4 bytes: count of elements in PoolVector2Array (960)
	# PoolVector2Array raw data
	var sound_as_bytes = sp.data_array.subarray(12, -1) # promised to be a slice	

	
	#print("Frame data=", len(frame_data))
	if sound_image == null:
		sound_image = Image.new()
		sound_image.create(frame_length, NFRAMES, false, Image.FORMAT_RF) # 8: GL_R32F
	if sound_texture == null:
		sound_texture = ImageTexture.new()
		# filtering ruins the sample history unfortunately
		# would be nice if we could only filter along x but not y
		sound_texture.create(frame_length, NFRAMES, Image.FORMAT_RF, Texture.FLAG_VIDEO_SURFACE)
	if frame_image == null:
		frame_image = Image.new()
		frame_image.create(frame_length, 1, false, Image.FORMAT_RF)

	frame_image.data.data = sound_as_bytes
	frame_index = (frame_index + 1) % NFRAMES

	sound_image.blit_rect(frame_image, Rect2(trigger, 0, frame_image.get_width(), 1), Vector2(0, frame_index))
	sp.clear()

	sound_texture.set_data(sound_image)
	texture_rect.material.set_shader_param("wav", sound_texture)
	texture_rect.material.set_shader_param("frame_index", frame_index)
	texture_rect.material.set_shader_param("midpoint", midpoint)
	texture_rect.material.set_shader_param("scale", scale)
	texture_rect.material.set_shader_param("gobshite", texture_rect.rect_size)
	
	#print(material.get_property_list())
	#print(material.get_shader_param("SCREEN_PIXEL_SIZE"))


func _notification(what):
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED):
		update_sizes()
	if what == NOTIFICATION_MOUSE_ENTER:
		hover = true
		long_hover_fired = false
		timer.wait_time = 1
		timer.start()
	elif what == NOTIFICATION_MOUSE_EXIT:
		timer.stop()
		hover = false

func _on_gui_input(event):
	if event is InputEventMouseButton:
		if event.button_index == BUTTON_RIGHT:
			paused = event.pressed
		if event.button_index == BUTTON_LEFT:
			if event.pressed:
				engaged = true
			else:
				if engaged:
					emit_signal("pressed")
				engaged = false
				
	if event is InputEventMouseMotion:
		if hover and not long_hover_fired:
			timer.stop()
			timer.start()

func _on_timeout():
	long_hover_fired = true
	emit_signal("long_hover")

func _on_ScopePanel_resized():
	#print("Scope size: ", rect_size)
	update_sizes()

func prepare_to_resize():
	texture_rect.texture = null
