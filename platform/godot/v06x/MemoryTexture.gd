extends Control

onready var texture_rect = find_node("TextureRect")
onready var model = get_parent().get_parent()

var mem_image: Image
var mem_texture: ImageTexture

var heatmap_image: Image
var heatmap_texture: ImageTexture

# 32*256 = 8192 = 1 screen page 256x256
# / 4 because rgba packs 4 bytes
# 2 * because 2 screens in one line
# 32 because 32 columns of 8 pixels
const mem_tex_size = Vector2(2 * 32 / 4, 256 * 4 * (1 + 4))
const pixel_size = Vector2(2, 4 * 5) * Vector2(256, 256)


# Called when the node enters the scene tree for the first time.
func _ready():
	var mat: ShaderMaterial = ShaderMaterial.new()
	mat.shader = load("res://bowser/memorytex.shader")
	texture_rect.material = mat
	model = get_parent().get_parent()
	update_sizes()
		
# 64kb as 8 pages of 256x256 pixels: 2x4, 5 times
func _get_minimum_size():
	var parent = get_parent() as ScrollContainer
	var vsb_width: int = 0
	if parent != null:
		vsb_width = parent.get_v_scrollbar().rect_size.x

	return pixel_size + Vector2(vsb_width, 0)
	
func update_sizes():
	if texture_rect == null: return
	
	print("update_sizes: gobshite=", texture_rect.rect_size, 
		" pixelsize=", pixel_size, " mem_sz=", mem_tex_size)
	
	var trimg = Image.new()
	trimg.create(pixel_size.x, pixel_size.y, false, Image.FORMAT_RGB8)
	var trtex = ImageTexture.new()
	trtex.create_from_image(trimg)
	texture_rect.texture = trtex

func update_texture():
	if mem_image == null:
		mem_image = Image.new()
		mem_image.create(mem_tex_size.x, mem_tex_size.y, false, Image.FORMAT_RGBA8)
	if mem_texture == null:
		mem_texture = ImageTexture.new()
		mem_texture.create(mem_tex_size.x, mem_tex_size.y, Image.FORMAT_RGBA8, Texture.FLAG_VIDEO_SURFACE)

	if heatmap_image == null:
		heatmap_image = Image.new()
		heatmap_image.create(mem_tex_size.x, mem_tex_size.y, false, Image.FORMAT_RGBA8)
	if heatmap_texture == null:
		heatmap_texture = ImageTexture.new()
		heatmap_texture.create(mem_tex_size.x, mem_tex_size.y, Image.FORMAT_RGBA8, Texture.FLAG_VIDEO_SURFACE)
	
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

func _notification(what):
	if (what == NOTIFICATION_RESIZED) or \
		(what == NOTIFICATION_VISIBILITY_CHANGED):
		update_sizes()
