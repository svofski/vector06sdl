;Иван Городецкий, Уфа, 09-13.01.2016

bdos		.equ	5

		.org	100h

		di
		xra	a
		out	10h
		lxi	sp,100h
		mvi	a,0C3h
		sta	0
		sta	5
		lxi	h,Restart
		shld	1
		lxi	h,bdosDispatch
		shld	6

;Печатаем результат
		call	Cls
		mvi	a,0C9h
		sta	38h
		ei
		hlt
		lxi	h, colors+15
colorset:
		mvi	a, 88h
		out	0
		mvi	c, 15
colorset1:	mov	a, c
		out	2
		mov	a, m
		out	0Ch
		dcx	h
		out	0Ch
		out	0Ch
		dcr	c
		out	0Ch
		out	0Ch
		out	0Ch
		jp	colorset1
		mvi	a,255
		out	3

Restart:
		di
		call	Cls
		lxi	h,0E0FFh
		shld	cursor

		di
		lxi h,TestVI53otf
		shld TestVI53+1
		lxi d,OtFtxt
		mvi c,9
		call bdos
;BIN
		lxi d,BINtxt
		mvi c,9
		call bdos
		xra a
		sta BINBCD
		call TestBatch
;BCD
		lxi d,BCDtxt
		mvi c,9
		call bdos
		mvi a,1
		sta BINBCD
		call TestBatch

		lxi h,TestVI53L
		shld TestVI53+1
		lxi d,LAtxt
		mvi c,9
		call bdos

;BIN
		lxi d,BINtxt
		mvi c,9
		call bdos
		xra a
		sta BINBCD
		call TestBatch
;BCD
		lxi d,BCDtxt
		mvi c,9
		call bdos
		mvi a,1
		sta BINBCD
		call TestBatch		
stop:		jmp	stop

TestBatch:		
;mode 0
		lxi d,m0txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00110000b
		call TestVI53

;mode 1
		lxi d,m1txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00110010b
		call TestVI53
		
;mode 2
		lxi d,m2txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00110100b
		call TestVI53

;mode 3
		lxi d,m3txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00110110b
		call TestVI53

;mode 4
		lxi d,m4txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00111000b
		call TestVI53

;mode 5
		lxi d,m5txt
		mvi c,9
		call bdos
		lda BINBCD
		ori 00111010b
		call TestVI53		
		ret

TestVI53:
		jmp	TestVI53otf

TestVI53otf:
		push psw
		out	8
		xra	a
		out	0Bh
		out	0Bh
		in	0Bh
		call	phex2

		mvi e,' '
		mvi c,2
		call bdos
		pop psw
		push psw
		out	8
		xra	a
		out	0Bh
		out	0Bh
		in	0Bh
		in	0Bh
		call	phex2
		
		mvi e,' '
		mvi c,2
		call bdos
		pop psw
		push psw
		out	8
		xra	a
		out	0Bh
		out	0Bh
		in	0Bh
		in	0Bh
		in	0Bh
		call	phex2

		mvi e,' '
		mvi c,2
		call bdos
		pop psw
		out	8
		xra	a
		out	0Bh
		out	0Bh
		in	0Bh
		in	0Bh
		in	0Bh
		in	0Bh
		call	phex2
		ret

TestVI53L:
		push psw
		out	8
		xra	a
		out	0Bh
		out	0Bh
		pop psw
		push psw
		ani 11001111b
		out 8
		in	0Bh
		call	phex2
		mvi e,' '
		mvi c,2
		call bdos
		in	0Bh
		call	phex2
				
		mvi e,' '
		mvi c,2
		call bdos
		pop psw
		ani 11001111b
		out	8
		in	0Bh
		call	phex2

		mvi e,' '
		mvi c,2
		call bdos
		in	0Bh
		call	phex2
		ret

bdosDispatch:
		push	psw
		push	b
		push	d
		push	h
		mov	a,c
		cpi	2
		cz	PrintCharBDOS
		cpi	9
		cz	PrintText
		pop	h
		pop	d
		pop	b
		pop	psw
		ret

cursor		.dw	0E0FFh-7

PrintText:
		push	psw
		lhld	cursor
		xchg
PrintTextLoop:
		mov	a,m
		cpi	'$'
		jz	PrintTextExit
		cpi	10		;LF
		jnz	Chk13
		lxi	b,-8
		xchg\ dad b\ xchg
		inx	h
		jmp	PrintTextLoop
Chk13:
		cpi	13		;CR
		jnz	PrintC
		mvi	d,0E0h
		inx	h
		jmp	PrintTextLoop
PrintC:
		call	PrintChar
		inx	h
		inr	d
		jmp	PrintTextLoop
PrintTextExit:
		xchg
		shld	cursor
		pop	psw
		ret

PrintCharBDOS:
		push	psw
		push	h
		mov	a,e
		lhld	cursor
		xchg
		call	PrintChar
		inr	d
		xchg	
		shld	cursor
		pop	h
		pop	psw
		ret
PrintChar:
		push	psw
		push	b
		push	d
		push	h
		mov	l, a
		mvi	h, 0
		dad	h
		dad	h
		dad	h
		lxi	b, Font-256
		dad	b
		mvi	b, 7
loopchar:
		mov	a, m
		inx	h
		stax	d
		dcr	e
		dcr	b
		jnz	loopchar
		pop	h
		pop	d
		pop	b
		pop	psw
		ret

Cls:
		lxi	h,0E000h
		xra	a
ClrScr:
		mov	m,a
		inx	h
		cmp	h
		jnz	ClrScr
		ret

; display hex string (pointer in hl, byte count in b)
hexstr:	mov	a,m
	call	phex2
	inx	h
	dcr	b
	jnz	hexstr
	ret

; display hex
; display the big-endian 32-bit value pointed to by hl
phex8:	push	psw
	push	b
	push	h
	mvi	b,4
ph8lp:	mov	a,m
	call	phex2
	inx	h
	dcr	b
	jnz	ph8lp
	pop	h
	pop	b
	pop	psw
	ret

; display byte in a
phex2:	push	psw
	rrc
	rrc
	rrc
	rrc
	call	phex1
	pop	psw
; fall through	

; display low nibble in a
phex1:	push	psw
	push	b
	push	d
	push	h
	ani	0fh
	cpi	10
	jc	ph11
	adi	'A'-'9'-1
ph11:	adi	'0'
	mov	e,a
	mvi	c,2
	call	bdos
	pop	h
	pop	d
	pop	b
	pop	psw
	ret

#define col 173

colors:
		.db 0,col,0,col,0,col,0,col,0,col,0,col,0,col,0,col
BINBCD .db 0
		
OtFtxt: .db "ON THE FLY",13,10,'$'
LAtxt: .db 13,10,13,10,"LATCHED",13,10,'$'
BINtxt: .db "BIN$"
BCDtxt: .db 13,10,"BCD$"
m0txt: .db 13,10,"MODE 0: $"
m1txt: .db 13,10,"MODE 1: $"
m2txt: .db 13,10,"MODE 2: $"
m3txt: .db 13,10,"MODE 3: $"
m4txt: .db 13,10,"MODE 4: $"
m5txt: .db 13,10,"MODE 5: $"

Font:

		.end	

