class_name bas2asc
extends SceneTree

enum State {
	DUMMY0 = 0,
	DUMMY1 = 1,
	LINENUM0 = 2,
	LINENUM1 = 3,
	TOKENS = 4,
	END = 5
}

enum Mode {
	INITIAL = 0x00,
	TOKENIZE = 0x01,
	QUOTE = 0x20,
	VERBATIM = 0x40,
}

class Tokens:
	var QUOTE='"'
	var Chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABC' + \
		'DEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ЮАБЦДЕФГ' + \
		'ХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧ' + char(127)
	var Words = ['CLS','FOR','NEXT','DATA','INPUT','DIM','READ','CUR','GOTO',
			'RUN','IF','RESTORE','GOSUB','RETURN','REM','STOP','OUT','ON',
			'PLOT','LINE','POKE','PRINT','DEF','CONT','LIST','CLEAR',
			'CLOAD','CSAVE','NEW','TAB(','TO','SPC(','FN','THEN','NOT',
			'STEP','+','-','*','/','^','AND','OR','>','=','<','SGN','INT',
			'ABS','USR','FRE','INP','POS','SQR','RND','LOG','EXP','COS',
			'SIN','TAN','ATN','PEEK','LEN','STR$','VAL','ASC','CHR$',
			'LEFT$','RIGHT$','MID$','POINT','INKEY$','AT','&','BEEP',
			'PAUSE','VERIFY','HOME','EDIT','DELETE','MERGE','AUTO','HIMEM',
			'@','ASN','ADDR','PI','RENUM','ACS','LG','LPRINT','LLIST',
			'SCREEN','COLOR','GET','PUT','BSAVE','BLOAD','PLAY','PAINT',
			'CIRCLE']

	#var by_initial = {key:[] for key in "ABCDEFGHIJKLMNOPQRSTUVWXYZ*/^>=<&@+-"}
	#for w in Words:
	#    by_initial[w[0]].append(w)
	var by_initial = {}
	
	func _init():
		for key in "ABCDEFGHIJKLMNOPQRSTUVWXYZ*/^>=<&@+-":
			by_initial[key] = []
		for w in Words:
			by_initial[w[0]].push_back(w)
			
	func gettext(c):
		if c < 0x20:
			return c
		elif c < 0x80:
			return ord(Chars[c - 0x20])
		else:
			return Words[c - 0x80]

	func chars(text):
		var result = []
		for c in text:
			var index = Chars.find(c)
			if index != -1:
				result.append(0x20 + index)
			else:
				result.append(ord(c))
		return result

var TOKENS = Tokens.new()

func format_token(t):
	if t is int:
		return char(t)
	if t is String:
		return t

func process_line(line):
	var formatted_tokens = []
	for tok in line.slice(1, line.size()):
		formatted_tokens.push_back(format_token(tok))
	var result = str(line[0]) + ' ' + ''.join(formatted_tokens)
	return result
	#return str(line[0]) + ' ' + ''.join([format_token(x) for x in line[1:]])

func readbas(path):
	var result = []
	#with open(path, 'rb') as fi: 
	var inputfile = File.new()
	var error = inputfile.open(path, File.READ)
	if error:
		printerr("Could not read " + path)
		return
		
	#mv = memoryview(fi.read())
	var mv = inputfile.get_buffer(inputfile.get_len())
	#print(path, ' ' , inputfile.get_len(), ' ' , len(mv))
	#print(mv)
	#return []

	var state = State.DUMMY0 
	var fin = 0
	var line = []
	var c
	for i in range(len(mv)):
		c = mv[i]
		#print(i, ': %02x' % c, ' ', ' state=', state)
		if state == State.DUMMY0:
			if c == 0: 
				fin = fin + 1
			state = State.DUMMY1
		elif state == State.DUMMY1:
			if c == 0:
				fin = fin + 1
			if fin == 3:
				state = State.END
			else:
				state = State.LINENUM0
			line = []
		elif state == State.LINENUM0:
			line.append(c)
			state = State.LINENUM1
		elif state == State.LINENUM1:
			line[0] = line[0] + c * 256
			state = State.TOKENS
		elif state == State.TOKENS:
			if c == 0:
				fin = 1
				state = State.DUMMY0
				result.append(process_line(line))
			elif c > 0 and c <= 31:
				line.append(c)
			elif c <= 228:
				line.append(TOKENS.gettext(c))
			else:
				line.append(c)
		elif state == State.END:
			break
	return result

func isnum(c):
	return c >= '0' and c <= '9'

# For given initial letter in position i, return list of keywords 
# that start with this letter.
# Each entry is ["keyword", count=0, position=i]
# If the mode is INITIAL, clear list of words and complete words
# If the mode is VERBATIM or QUOTE, return nothing
func pick_keywords(initial, i, mode, words, complete):
	#try:
	if (mode & (Mode.VERBATIM|Mode.QUOTE)) != 0:
		return

	#kws = [[x,0,i] for x in Tokens.by_initial[initial][:]]
	var kws = []
	if TOKENS.by_initial.has(initial):
		for x in TOKENS.by_initial[initial]:
			kws.append([x, 0, i])
		for w in kws:
			if len(w[0]) == 1:
				complete.append(w)
			else:
				words.append(w)
	else:
		if mode == Mode.INITIAL:
			words.clear()
			complete.clear()
	#except:
	#    if mode == Mode.INITIAL:
	#        words.clear()
	#        complete.clear()

# Parse number at the beginning of line and skip initial spaces
func get_linenumber(s):
	var linenum = 0
	#for text,c in enumerate(s):
	var text = 0
	for i in range(len(s)):
		text = i
		var c = s[i]
		if isnum(c):
			linenum = linenum * 10 + int(c)
		else:
			if c == ' ':
				pass
			else:
				break
	return [linenum,text]

# Update word match count, move complete words to complete, delete mismatches
# See pick_keywords
func trackwords(c, words, complete):
	var todelete = []
	#for j,wc in enumerate(words):   # track keywords
	for j in len(words):
		var wc = words[j]
		var k = wc[1] + 1
		if wc[0][k] == c: 
			wc[1] = k
			if len(wc[0])-1 == k:
				complete.append(wc)
				todelete.append(j) 
		else:
			todelete.append(j)  # char mismatch, this word is out

	todelete.invert()
	for j in todelete:
		words.remove(j)		# purge untracked words

# input: sorted by start position [[tok, x, pos]]
# output: only one token per pos, the longest
func suppress_nonmax(complete):
	var pos = -1
	var result = []
	for t in complete:
		if t[2] == pos:
			if len(t[0]) > len(result[-1][0]):
				result[-1] = t
		else:
			result.append(t)
			pos = t[2]
	return result

# for overlapping tokens, pick the longest even if it starts later
# IFK=ATHEN3 
#     AT
#      THEN <-- winrar
func suppress_overlaps(complete):
	var current_end=-1
	var current_len=-1
	var result=[]
	for t in complete:
		if t[2] < current_end:
			if len(t[0]) > current_len:
				result[-1] = t
				current_len = len(t[0])
				current_end = t[2]+current_len
		else:
			result.append(t)
			current_len = len(t[0])
			current_end = t[2]+current_len
	return result

class MyCustomSorter:
	static func sort_ascending(a, b):
		if a[2] < b[2]:
			return true
		return false


func tokenize2(s: String, addr: int):
	var tokens=[]
	var words=[]
	var complete=[]
	#[linenum, i] = get_linenumber(s)
	var linenum_i = get_linenumber(s)
	var linenum = linenum_i[0]
	var i = linenum_i[1]
	var seq_start = i
	pick_keywords(s[i], i, Mode.INITIAL, words, complete)
	var mode = Mode.TOKENIZE
	var breakchar
	while i < len(s):
		trackwords(s[i], words, complete)
		breakchar = s[i]
		if breakchar == TOKENS.QUOTE:
			mode ^= Mode.QUOTE

		# add keywords that start at the current position to tracking
		pick_keywords(s[i], i, mode, words, complete)

		# all tracked words ended, or end of line
		if len(words) == 0 or i + 1 == len(s):
			words = []
			if len(complete) > 0:
				# make sure that the tokens are in order of occurrence
				#complete.sort(key=lambda x: x[2], reverse=False)
				complete.sort_custom(MyCustomSorter, "sort_ascending")
				complete = suppress_nonmax(complete)   # INPUT vs INP..
				complete = suppress_overlaps(complete) # THEN over AT in ATHEN 
				# flush dangling character tokens 
				#tokens = tokens + TOKENS.chars(s[seq_start:complete[0][2]])
				tokens = tokens + TOKENS.chars(s.substr(seq_start, complete[0][2] - seq_start))
				#for j,b in enumerate(complete):
				for j in range(len(complete)):
					var b = complete[j]
					if j > 0 and b[2] != i: # tokens overlap, only keep 1st
						break
					tokens.append(TOKENS.Words.find(b[0]) + 0x80)
					i = b[2] + len(b[0]) 
					seq_start = i
					if b[0] in ['DATA','REM']:
						mode = Mode.VERBATIM
						break # mode switch, cancel following tokens
				if breakchar != TOKENS.QUOTE:
					i = i - 1
				complete = []
			else: 
				pass

		i = i + 1         

	#tokens = tokens + Tokens.chars(s[seq_start:]) + [0]
	tokens = tokens + TOKENS.chars(s.substr(seq_start, len(s))) + [0]
	var recsize = len(tokens) + 4
	addr += recsize
	tokens = [addr&255,addr>>8] + [linenum & 255, (linenum >> 8) & 255] + tokens
	return [tokens,addr]


func tost():
	print(TOKENS.QUOTE)
	print(get_linenumber("1234 PRINTA"))
	print(suppress_overlaps([["AT",0,3],["TO",0,4]]))
	print(suppress_overlaps([["AT",0,3],["THEN",0,4]]))
	print(suppress_nonmax([["INP",0,3],["INPUT",0,3],["PUT",0,2]]))

	print("t2=", tokenize2("1 CLS",0x4301))
	print("t2=", tokenize2("10 RESTORE6",0x4300))
	print("t2=", tokenize2("10 RESTOR6",0x4300))
	print("t2=", tokenize2("10 QTOA",0x4300))
	print("t2=", tokenize2("10 FORI=ATOB",0x4300))
	print("t2=", tokenize2("10 FORJ=0TO5",0x4300))

func enbas(path):
	var result = []
	var size = 0
	var addr = 0x4301
	var fi = File.new()
	if fi.open(path, File.READ) == OK:
		var text = fi.get_as_text()
		if fi.get_error() != OK || text.length() == 0:
			return null
		for line in text.split("\n", false):
			#print("Input=["+line+"]")
			var ta = tokenize2(line, addr)
			var tokens = ta[0]
			addr = ta[1]
			result = result + tokens

	result.append(0)
	result.append(0)

	# add padding for perfect file match
	#var padding = [0]*(256-(len(result) % 256))
	#return bytearray(result+padding)
	return PoolByteArray(result)

func asc2bas(ascfile, basfile: String) -> int:
	var tokenised = enbas(ascfile)
	if tokenised == null:
		return ERR_CANT_OPEN
	var fo = File.new()
	fo.open(basfile, File.WRITE)
	fo.store_buffer(tokenised)
	fo.close()
	return OK

func _init() -> void:
	#var arguments = OS.get_cmdline_args()
	#var tokenised = enbas(arguments[2])
	#var fo = File.new()
	#fo.open(arguments[3], File.WRITE)
	#fo.store_buffer(tokenised)
	#fo.close()
	#quit()
	pass

