extends Control

onready var texture_rect = find_node("TextureRect")
onready var reticle: Sprite = find_node("Reticle")
onready var model: Control = null

var mouse_addr: int = 0

signal mouse_addr_changed

var mem_image: Image
var mem_texture: ImageTexture

var heatmap_image: Image
var heatmap_texture: ImageTexture

# 32*256 = 8192 = 1 screen page 256x256
# / 4 because rgba packs 4 bytes
# 2 * because 2 screens in one line
# 32 because 32 columns of 8 pixels
const RAM_PAGES = 5
const mem_tex_size = Vector2(2 * 32 / 1, 256 * 4 * RAM_PAGES)
const pixel_size = Vector2(2, 4 * RAM_PAGES) * Vector2(256, 256)


# Called when the node enters the scene tree for the first time.
func _ready():
	var mat: ShaderMaterial = ShaderMaterial.new()
	mat.shader = load("res://bowser/memorytex.shader")
	texture_rect.material = mat
	update_sizes()

func set_model(m):
	model = m
	
# 64kb as 8 pages of 256x256 pixels: 2x4, 5 times
func _get_minimum_size():
	var parent = get_parent() as ScrollContainer
	var vsb_width: int = 0
	if parent != null:
		vsb_width = parent.get_v_scrollbar().rect_size.x

	return pixel_size + Vector2(vsb_width, 0)
	
func update_sizes():
	if texture_rect == null: return
	
	var trimg = Image.new()
	trimg.create(pixel_size.x, pixel_size.y, false, Image.FORMAT_RGBA8 )
	var trtex = ImageTexture.new()
	trtex.create_from_image(trimg)
	texture_rect.texture = trtex

func update_texture():
	if mem_image == null:
		mem_image = Image.new()
		mem_image.create(mem_tex_size.x, mem_tex_size.y, false, Image.FORMAT_R8)
	if mem_texture == null:
		mem_texture = ImageTexture.new()
		mem_texture.create(mem_tex_size.x, mem_tex_size.y, Image.FORMAT_R8, 
			Texture.FLAG_VIDEO_SURFACE)
		

	if heatmap_image == null:
		heatmap_image = Image.new()
		heatmap_image.create(mem_tex_size.x, mem_tex_size.y, false, Image.FORMAT_R8)
	if heatmap_texture == null:
		heatmap_texture = ImageTexture.new()
		heatmap_texture.create(mem_tex_size.x, mem_tex_size.y, Image.FORMAT_R8, Texture.FLAG_VIDEO_SURFACE)
	
	#print(len(mem_image.data.data))
	#print("mem_tex_size in bytes should be=", mem_tex_size.x * mem_tex_size.y, " m=",
	#	mem_tex_size)
	mem_image.data.data = model.ram
	mem_texture.set_data(mem_image)
	
	heatmap_image.data.data = model.heatmap
	heatmap_texture.set_data(heatmap_image)
	
	texture_rect.material.set_shader_param("mem", mem_texture)
	texture_rect.material.set_shader_param("heatmap", heatmap_texture)
	texture_rect.material.set_shader_param("mem_sz", mem_tex_size)
	texture_rect.material.set_shader_param("gobshite", texture_rect.rect_size)
	
	if reticle.visible:
		#reticle.scale = reticle.scale * Vector2(0.9, 0.9)
		reticle.size = reticle.size * 0.9
		reticle.alpha += 0.05
		if reticle.size <= 0.1:
			reticle.visible = false
		reticle.update()
	

func _notification(what):
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED):
		update_sizes()


func _on_TextureRect_gui_input(event):
	if event is InputEventMouseMotion:
		var page: int = floor(event.position.y / 256) # 2 screens, 16k
		var base: int = page * 16 * 1024
		var page_y: int = int(floor(event.position.y)) % 256
		var addr: int = base + int(floor(event.position.x)) / 8 * 256 + (255 - page_y)
		if mouse_addr != addr:
			mouse_addr = addr
			emit_signal("mouse_addr_changed")
		
		#print("position=" + str(event.position),  " base addr: %05x page_y=%d addr=%04x" % [base, page_y, addr])

func scroll_to_addr(addr: int):
	var base: int = addr / (16 * 1024) # number of 16k page
	var ofs: int = addr % (16 * 1024)  # offset withing 16k page
	var x: int = (ofs / 256) * 8	   # x position
	var y: int = 255 - ofs % 256	   # y position rel to page start
	y += base * 256
	#print("texture view scrolling to ", Vector2(x, y))

	var sc: ScrollContainer = get_parent() as ScrollContainer	
	if sc != null:
		sc.scroll_vertical = y - sc.get_v_scrollbar().page / 2
	
	if reticle != null:
		reticle.position = Vector2(x + 4, y)
		reticle.alpha = 0
		reticle.size = 120
		reticle.visible = true
