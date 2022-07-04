extends Sprite


var image: Image

var size: float = 100
var alpha: float = 1.0

# Called when the node enters the scene tree for the first time.
func _ready():
	#image = Image.new()
	#image.create(256, 256, false, Image.FORMAT_RGBA8)	
	#var t: ImageTexture = ImageTexture.new()
	#t.create_from_image(image)
	#texture = t
	pass
	
func _draw():
	var c: Color = Color8(255, 255, 255, alpha * 255)
	draw_rect(Rect2(-size/2, -size/2, size, size), c, false)
	
