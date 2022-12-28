tool
extends Control

signal mouse_addr_changed
var mouse_addr: int = 0

const RAMSIZE: int = 65536 * 5
const NLINES: int = RAMSIZE / 16

const dummy_string = "00000: 00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00   ................"

const koia = '................................'
const koib = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
const koic = '\u007f\u2500\u2502\u250c\u2510\u2514\u2518\u251c\u2524\u252c\u2534\u253c\u2580\u2584\u2588\u258c\u2590\u2591\u2592\u2593\u2320\u25a0\u2219\u221a\u2248\u2264\u2265\u00a0\u2321\u00b0\u00b2\u00b7\u00f7\u2550\u2551\u2552\u0451\u2553\u2554\u2555\u2556\u2557\u2558\u2559\u255a\u255b\u255c\u255d\u255e\u255f\u2560\u2561\u0401\u2562\u2563\u2564\u2565\u2566\u2567\u2568\u2569\u256a\u256b\u256c\u00a9\u044e\u0430\u0431\u0446\u0434\u0435\u0444\u0433\u0445\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f\u044f\u0440\u0441\u0442\u0443\u0436\u0432\u044c\u044b\u0437\u0448\u044d\u0449\u0447\u044a\u042e\u0410\u0411\u0426\u0414\u0415\u0424\u0413\u0425\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f\u042f\u0420\u0421\u0422\u0423\u0416\u0412\u042c\u042b\u0417\u0428\u042d\u0429\u0427\u042a'

const encoding = koia + koib + koic

#const  encodings = {
#	'koi8-r': '\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f\u2500\u2502\u250c\u2510\u2514\u2518\u251c\u2524\u252c\u2534\u253c\u2580\u2584\u2588\u258c\u2590\u2591\u2592\u2593\u2320\u25a0\u2219\u221a\u2248\u2264\u2265\xa0\u2321\xb0\xb2\xb7\xf7\u2550\u2551\u2552\u0451\u2553\u2554\u2555\u2556\u2557\u2558\u2559\u255a\u255b\u255c\u255d\u255e\u255f\u2560\u2561\u0401\u2562\u2563\u2564\u2565\u2566\u2567\u2568\u2569\u256a\u256b\u256c\xa9\u044e\u0430\u0431\u0446\u0434\u0435\u0444\u0433\u0445\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f\u044f\u0440\u0441\u0442\u0443\u0436\u0432\u044c\u044b\u0437\u0448\u044d\u0449\u0447\u044a\u042e\u0410\u0411\u0426\u0414\u0415\u0424\u0413\u0425\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f\u042f\u0420\u0421\u0422\u0423\u0416\u0412\u042c\u042b\u0417\u0428\u042d\u0429\u0427\u042a',
#	'koi8-u': '\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f\u2500\u2502\u250c\u2510\u2514\u2518\u251c\u2524\u252c\u2534\u253c\u2580\u2584\u2588\u258c\u2590\u2591\u2592\u2593\u2320\u25a0\u2219\u221a\u2248\u2264\u2265\xa0\u2321\xb0\xb2\xb7\xf7\u2550\u2551\u2552\u0451\u0454\u2554\u0456\u0457\u2557\u2558\u2559\u255a\u255b\u0491\u255d\u255e\u255f\u2560\u2561\u0401\u0404\u2563\u0406\u0407\u2566\u2567\u2568\u2569\u256a\u0490\u256c\xa9\u044e\u0430\u0431\u0446\u0434\u0435\u0444\u0433\u0445\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f\u044f\u0440\u0441\u0442\u0443\u0436\u0432\u044c\u044b\u0437\u0448\u044d\u0449\u0447\u044a\u042e\u0410\u0411\u0426\u0414\u0415\u0424\u0413\u0425\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f\u042f\u0420\u0421\u0422\u0423\u0416\u0412\u042c\u042b\u0417\u0428\u042d\u0429\u0427\u042a',
#	'cp866': '\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f\u0410\u0411\u0412\u0413\u0414\u0415\u0416\u0417\u0418\u0419\u041a\u041b\u041c\u041d\u041e\u041f\u0420\u0421\u0422\u0423\u0424\u0425\u0426\u0427\u0428\u0429\u042a\u042b\u042c\u042d\u042e\u042f\u0430\u0431\u0432\u0433\u0434\u0435\u0436\u0437\u0438\u0439\u043a\u043b\u043c\u043d\u043e\u043f\u2591\u2592\u2593\u2502\u2524\u2561\u2562\u2556\u2555\u2563\u2551\u2557\u255d\u255c\u255b\u2510\u2514\u2534\u252c\u251c\u2500\u253c\u255e\u255f\u255a\u2554\u2569\u2566\u2560\u2550\u256c\u2567\u2568\u2564\u2565\u2559\u2558\u2552\u2553\u256b\u256a\u2518\u250c\u2588\u2584\u258c\u2590\u2580\u0440\u0441\u0442\u0443\u0444\u0445\u0446\u0447\u0448\u0449\u044a\u044b\u044c\u044d\u044e\u044f\u0401\u0451\u0404\u0454\u0407\u0457\u040e\u045e\xb0\u2219\xb7\u221a\u2116\xa4\u25a0\xa0',
#	'latin1': '\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff'
#  };

var font: Font
var line_h: float
var char_w: float
var scroll_range: int

var model: Control = null


var mouse_line: int = 0

func _ready():
	font = DynamicFont.new()
	font.font_data = load("res://assets/3270Condensed-Regular.ttf")
	font.size = 10
	refresh_font_metrics()
	scroll_range = line_h * RAMSIZE / 16

	var vsb: VScrollBar = get_parent().get_v_scrollbar()
	vsb.connect("changed", self, "_on_fucked")

func refresh_font_metrics():
	var fsz = font.get_string_size("0") 
	line_h = fsz.y
	char_w = fsz.x	
	scroll_range = line_h * RAMSIZE / 16

func set_model(m):
	model = m

func scroll_to_addr(addr: int):
	var sc: ScrollContainer = get_parent() as ScrollContainer	
	if sc != null:
		sc.scroll_vertical = addr / 16 * line_h - sc.get_v_scrollbar().page / 2

# Godot's ScrollContainer will try to fuck us in every way imaginable
# use this to restore proper scroll range
func _on_fucked():
	var vsb: VScrollBar = get_parent().get_v_scrollbar()
	#print("Fuck attempt: max_value=", vsb.max_value)
	# try to fuck off
	if vsb.max_value > scroll_range:
		vsb.set_deferred("max_value", scroll_range)

func _safechar(c: int) -> String:
	return encoding[c]  #char(c) if c >= 32 && c < 128 else '.'
	#return char(c) if c >= 32 && c < 128 else '.'

func _get_dump_str(addr: int):
	if addr > RAMSIZE - 16:
		return ""
		
	var s = "%05x: " % addr
	var t = "  "
	for x in range(16):
		s = s + "%02x " % model.ram[addr + x]
		t = t + _safechar(model.ram[addr + x])
	s += t
	return s
	
func _draw_line(num):
	if font == null: return
	
	var line_y = (num + 1) * line_h

	var x0: float = char_w * 7 - char_w / 2
	var xc0: float = char_w * (7 + 16*3) + char_w * 2
	var ry0: float = line_y - line_h * 0.8
	# avoid excessive drawing of parts where heat is 0	
	if num * 16 + 15 < RAMSIZE:
		var heatline: PoolByteArray = model.heatmap.subarray(num * 16, num * 16 + 15)
		if heatline.count(0) < heatline.size():
			var x: float = x0
			var xstep: float = char_w * 3
			var r = Rect2(x, ry0, xstep, line_h)
			for i in range(heatline.size()):
				var val = clamp(heatline[i], 0, 255)
				var col = Color8(255, 16, 0, val )
				draw_rect(r, col, true)
				r.position.x += xstep
	
	var c: Color = Color.lightgray
	var mouse_rect = Rect2(0, 0, 0, 0)
	if num == mouse_line:
		c = Color.white
		var mousecol = Color8(25, 100, 155, 150)
		mouse_rect.position = Vector2(x0 + (mouse_addr % 16) * char_w * 3, ry0)
		mouse_rect.size = Vector2(char_w * 3, line_h)
		draw_rect(mouse_rect, mousecol, true)

		mouse_rect.position = Vector2(xc0 + (mouse_addr % 16) * char_w, ry0)
		mouse_rect.size = Vector2(char_w, line_h)
		draw_rect(mouse_rect, mousecol, true)

	draw_string(font, Vector2(0, line_y), _get_dump_str(num * 16), c)
	
func draw_lines(from, to):
	for l in range(from, to):
		_draw_line(l)

func _draw():
	#print("margin_top=", margin_top)
	#print("margin_bottom=", margin_bottom)
	#print("size=", rect_size)
	#print("visibob=", margin_bottom + margin_top)
	#print("scrollbob max=", get_parent().get_v_scrollbar().max_value, 
	#	"current=", get_parent().get_v_scrollbar().value)
	#print("scrollbob page=", get_parent().get_v_scrollbar().page)

	refresh_font_metrics()

	var page = get_parent().get_v_scrollbar().page
	var page_lines = ceil(page / line_h) + 1
	var line1: int = floor(-margin_top / line_h)

	draw_lines(line1, line1+page_lines)

func _get_minimum_size():
	var parent = get_parent() as ScrollContainer
	var vsb_width: int = 0
	if parent != null:
		vsb_width = parent.get_v_scrollbar().rect_size.x
	if font != null:
		#return font.get_string_size(dummy_string) * Vector2(1, NLINES) + Vector2(vsb_width, 0)
		return font.get_string_size(dummy_string) * Vector2(0, NLINES)
	#return Vector2(10 * len(dummy_string), 10 * NLINES)
	return Vector2(0, 10 * NLINES)

func _notification(what):
	if what == NOTIFICATION_RESIZED:
		#rect_size = _get_minimum_size()
		var scrl: ScrollContainer = get_parent()
		scrl.get_v_scrollbar().max_value = NLINES * 10

func _on_gui_input(event):
	if event is InputEventMouseMotion:
		var y: int = int(floor(event.position.y))
		var line1: int = floor(-margin_top / line_h)
		mouse_line = floor(y / line_h)
		var addr: int = mouse_line * 16
		
		# find offset 0..15
		var ofs: int = -1
		var x: int = int(floor(event.position.x))
		var x0: float = floor(char_w * 7 - char_w / 2)
		var xc0: float = floor(char_w * (7 + 16*3) + char_w * 2)
		var xstep: float = char_w * 3
		if x < xc0:
			x = clamp(x, x0, x0 + xstep * 16)
			ofs = int(floor((x - x0) / xstep))
		else:
			x = clamp(x, xc0, xc0 + char_w * 16)
			ofs = int(floor((x - xc0) / char_w))

		if ofs == -1:
			return
			
		addr += ofs
		
		if addr != mouse_addr:
			mouse_addr = addr
			emit_signal("mouse_addr_changed")
