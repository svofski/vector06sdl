        ; 🐟  (try me)
        ; 8080 assembler code
        .project v06x.rom
        .tape v06c-rom


LOGOXY		equ $8454

ayctrl  	EQU     $15
aydata  	EQU     $14
        
        
NCHECKERS       EQU 8
        
		.org 100h
		di
		xra	a
		out	10h
		mvi	a,0C3h
		sta	0
		lxi	h,Restart
		shld	1
		mvi	a,0C9h
		sta	38h
Restart:
		lxi sp, $100
		call ay_off
		lxi h, $ffff
		shld rnd16+1

		ei
		hlt		
		lxi h, clut_zero+15
		call   load_clut
		
		xra a
		sta hscroll_x0
	        sta vscroll_y0
		
	
		mvi c, $ff
		call cls8000
		
		;mvi e, $80
		;mvi a, $ff
		;call hline

                mvi a, 8
                sta vcolumn_height
                mvi a, 255
                sta vcolumn_top
		

                ; draw vertical and horizontal lines
                mvi c, NCHECKERS / 2
                mvi e, 0
sqlo0:                
                mvi d, 256 / NCHECKERS
sqlo1:                
                push b
                push d
                mvi a, 0
                sta vcol_pixel_mask
		call vcolumn
		pop d
		push d
		mvi a, 255
		call hline
		pop d
		pop b
		inr e 
		dcr d
		jnz sqlo1
		mvi a, 256 / NCHECKERS
		add e
		mov e, a
		dcr c
		jnz sqlo0

		
		;; row height = 2 squares
                mvi a, 2
                sta vcolumn_height

                lxi h, hscroll_x0
                mvi m, 255

                ; fourth row
                mvi a, 15
                out 2
                mvi a, 255-64-64-64
                sta vcolumn_top
                call checkers_row
                mvi a, 9
                out 2


                ; big logo
                call blit                
                
		ei
		hlt
		lxi h, colors+15
		call load_clut

                mvi a, 8
                out $2


main_loop:      ei
                hlt
                mvi a, 8
                out $2
                
		lda nframe
		inr a
		sta nframe

                ; fourth row
                ;mvi a, 15
                ;out 2
                mvi a, 255-64-64-64
                sta vcolumn_top
                call checkers_row
                ;mvi a, 0
                ;out 2

                lxi h, hscroll_x0
                inr m

                ; vertical scroll
                mvi c, NCHECKERS
                mvi b, 0
                lxi h, vscroll_y0
                mov e, m
                inr m
vscroll_yloop:
                push b
                push d
                mov a, b
                call hline
                pop d
                pop b
                mov a, b
                cma
                mov b, a
                mvi a, 256 / NCHECKERS
                add e
                mov e, a
                dcr c
                jnz vscroll_yloop

                
                ;; beam yielding tormoz 1
                ;mvi a, 1
                ;out 2
                mvi a, 20
                xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl
                dcr a
                jnz .-13

                ;; top row
                ;mvi a, 15
                ;out 2
                mvi a, 255
                sta vcolumn_top
                call checkers_row

                ;; beam yielding tormoz 2
                ;mvi a, 12
                ;out 2
                mvi a, 15
                xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl
                dcr a
                jnz .-13

                ;; second row
                ;mvi a, 15
                ;out 2
                mvi a, 255-64
                sta vcolumn_top
                call checkers_row
                ;mvi a, 9
                ;out 2

                ;; beam yielding tormoz 3
                ;mvi a, 12
                ;out 2
                mvi a, 15
                xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl\ xthl
                dcr a
                jnz .-13
                ;; third row
                ;mvi a, 15
                ;out 2
                mvi a, 255-64-64
                sta vcolumn_top
                call checkers_row
                ;mvi a, 15
                ;out 2


end_scroll:                
		jmp main_loop
                
                ; horizontal line: y in e, a = bitmap
hline:          lxi h, $a000
                mvi d, 0
                dad d
                
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h
                mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h \ mov m, a \ inr h

                ret


                
checkers_row:                
                ; horizontal scroll		
                mvi c, NCHECKERS
                lxi h, hscroll_x0
                mov e, m
                ;inr m
hscroll_xloop:                
                push b
                push d
                call vcolumn
                pop d
                pop b
                mvi a, 256 / NCHECKERS
                add e
                mov e, a
                dcr c
                jnz hscroll_xloop
                ret                
                
vcol_pixel_mask db 0

                ; vertical xor line: x in e
                ; trick: only calculate the first byte, then copy it all the way up
vcolumn:        
                lxi h, 0
                dad sp
                shld vcolumn_sp
                di
                
                ;lda vcol_pixel_mask
                ;ora a
                ;mov c, a
                ;jnz vcolumn_mask_ok
                
                ; pixel mask
                mvi h, PixelMask>>8
                mov a, e
                ani 7
                mov l, a
                mov c, m        ; c = pixel bit
                mov a, c
                ;sta vcol_pixel_mask

vcolumn_mask_ok:        
                mov a, e \ rrc \ rrc \ rrc \ ani $1f
                
vcolumn_bplane  equ .+1
                adi $c0 ; $c000
                mov h, a
vcolumn_top     equ .+1                
                mvi l, 31
                mov a, m \ xra c
                inr l ;\ inr l
                mov d, a
                mov e, a

                mov a, l
                ora a
                
                jnz .+4
                inr h
                sphl
                
vcolumn_height  .equ .+1                
                mvi b, 1
vcolumn_l1:     
                push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d\ push d
                dcr b
                jnz vcolumn_l1
vcolumn_sp      .equ .+1
                lxi sp, 0
                ei
                ret
                

                
cls8000
		lxi h, $8000
		mvi a, $a0
		sta cls_cond+1
cls80_1
		mov m, c \ inr l \ mov m, c \ inr l \ mov m, c \ inr l \ mov m, c \ inr l
		mov m, c \ inr l \ mov m, c \ inr l \ mov m, c \ inr l \ mov m, c \ inr l
		jnz cls80_1
		inr h
		mov a, h
cls_cond:
		cpi $a0
		jnz cls80_1
		ret
                
                

outer   	mov a, d
                out ayctrl
                mov a, e
                out aydata
                ret
ay_off		lxi d, $0800
		jmp outer
        

		; загрузить палитру
		; hl указывает на адрес палитры + 15
load_clut:
		mvi	a, 88h
		out	0
		mvi	c, 15
lclut1:		mov	a, c
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
		jp	lclut1
		mvi	a,255
		out	3
		ret

; выход:
; HL - число от 1 до 65535
rnd16:
		lxi h,65535
		dad h
		shld rnd16+1
		rnc
		mvi a,00000001b ;перевернул 80h - 10000000b
		xra l
		mov l,a
		mvi a,01101000b	;перевернул 16h - 00010110b
		xra h
		mov h,a
		shld rnd16+1
		ret

		.org . + 256 & 0xff00
PixelMask:
		.db 10000000b
		.db 01000000b
		.db 00100000b
		.db 00010000b
		.db 00001000b
		.db 00000100b
		.db 00000010b
		.db 00000001b

		; начальная палитра
clut_zero:	db 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

colors:		; рабочая палитра
		; плоскость $8000 = 1, все черное
		db 227q,227q,227q,227q
		db 227q,227q,227q,227q
		; эта часть постоянно перемешивается
		db 000q,122q,244q,244q
		db 122q,122q,377q,377q
		db 377q ; +1 to make shuffling simpler

scroll:		db 0
nframe:		db 0
hscroll_x0      db 0
vscroll_y0      db 0


		;
		; Вывести транспарант посреди экрана
		; Мы должны немного обгонять луч, чтобы
		; к началу сканирования первой строки
		; верх картинки уже был готов
blit:
		lxi h, 0
		dad sp
		shld blit_sp+1
		lxi sp, hello_jpg
		lxi h, LOGOXY + 80
		lda scroll
		add l
		mov l, a
		
                mov b, h        ; сохраним в b первый столб
		
                mvi a, 80       ; 40 строк (через одну, всего 80)
blit_line:
                mvi c, 192/8/8  ; размер по горизонтали в байтах/8
blit_line1:
                ; {1}
                pop d           ; берем два столбца строки в de
                mov m, e        ; записываем в экран первый столб
                inr h           ; столб += 1
                mov m, d        ; записываем второй столб
                inr h           ; столб += 1
                ; {2,3,4}
                db $d1,$73,$24,$72,$24
                db $d1,$73,$24,$72,$24
                db $d1,$73,$24,$72,$24
                dcr c
                jnz blit_line1

                dcr a           ; уменьшаем счетчик пар строк
                jz blit_sp      ; изя всё
                dcr l           ; следующая строка (через одну)
                ;dcr l
                mov h, b        ; снова первый столбец
                jmp blit_line
                
blit_sp:	lxi sp, 0
		ret


hello_jpg:
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$f0,$07,$ff,$ff,$ff,$fe,$07,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$00,$ff,$ff,$ff,$f0,$01
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$00,$00,$7f,$ff,$ff,$80,$03,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fe,$00,$00,$3f,$ff,$ff,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $fc,$07,$c0,$1f,$ff,$fc,$03,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f8,$0f,$e0,$0f,$ff,$f8,$0f,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f0,$1f,$f0,$07,$ff,$f0,$1f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f0,$1f,$f8,$07,$ff,$e0,$3f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $e0,$3f,$f8,$07,$ff,$c0,$7f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$e0,$3f,$f8,$03,$ff,$80,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$e0,$3f,$fc,$03,$ff,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$7f,$fc,$03,$fe,$01,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $c0,$7f,$fc,$03,$fe,$01,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$7f,$fc,$03,$fc,$03,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$7f,$fc,$01,$fc,$03,$ff,$ff,$ff,$e3,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$c0,$00,$1f,$80,$03,$c0,$7f,$fc,$01,$f8,$03,$ff,$ff,$fe,$01,$ff,$f8,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$00,$1f,$80,$03
        .db  $80,$7f,$fe,$01,$f8,$07,$ff,$ff,$f0,$00,$ff,$f0,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$00,$1f,$80,$03,$80,$7f,$fe,$01,$f0,$07,$ff,$ff
        .db  $f0,$00,$ff,$f0,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$f8,$01,$ff,$f8,$1f,$80,$7f,$fe,$01,$f0,$07,$e0,$3f,$f0,$00,$7f,$e0,$7f,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$fc,$01,$ff,$f8,$3f,$80,$7f,$fe,$01,$f0,$07,$80,$0f,$ff,$00,$3f,$c0,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fc,$01,$ff,$f8,$3f
        .db  $80,$7f,$fe,$01,$f0,$06,$00,$07,$ff,$80,$1f,$c1,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fe,$01,$ff,$f8,$7f,$80,$7f,$fe,$01,$e0,$0c,$00,$03
        .db  $ff,$c0,$1f,$83,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fe,$00,$ff,$f0,$7f,$80,$7f,$fe,$01,$e0,$08,$00,$01,$ff,$c0,$0f,$07,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$00,$ff,$f0,$ff,$80,$7f,$fe,$01,$e0,$00,$00,$00,$ff,$e0,$06,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$00,$ff,$f0,$ff
        .db  $80,$7f,$fe,$01,$e0,$01,$f0,$00,$ff,$f0,$04,$1f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$00,$7f,$e0,$ff,$80,$7f,$fe,$01,$e0,$03,$fc,$00
        .db  $ff,$f0,$00,$1f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$80,$7f,$e1,$ff,$80,$7f,$fe,$01,$e0,$07,$fe,$00,$7f,$f8,$00,$3f,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$80,$3f,$c1,$ff,$80,$7f,$fe,$01,$e0,$07,$ff,$00,$7f,$fc,$00,$7f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$3f,$c3,$ff
        .db  $80,$7f,$fe,$01,$e0,$0f,$ff,$00,$7f,$fc,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$3f,$c3,$ff,$80,$7f,$fe,$01,$e0,$0f,$ff,$00
        .db  $7f,$fe,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c0,$1f,$87,$ff,$80,$7f,$fe,$03,$e0,$0f,$ff,$80,$7f,$ff,$00,$7f,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$e0,$1f,$87,$ff,$80,$3f,$fe,$03,$e0,$0f,$ff,$80,$7f,$ff,$00,$3f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$e0,$0f,$07,$ff
        .db  $80,$3f,$fe,$03,$e0,$0f,$ff,$80,$7f,$ff,$00,$3f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f0,$0f,$0f,$ff,$c0,$3f,$fe,$03,$e0,$0f,$ff,$80
        .db  $7f,$fe,$00,$1f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f0,$0f,$0f,$ff,$c0,$3f,$fe,$03,$f0,$0f,$ff,$80,$7f,$fc,$00,$0f,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$f0,$06,$1f,$ff,$c0,$3f,$fc,$03,$f0,$0f,$ff,$80,$ff,$f8,$20,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f8,$06,$1f,$ff
        .db  $c0,$3f,$fc,$07,$f0,$0f,$ff,$80,$ff,$f0,$60,$07,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f8,$00,$1f,$ff,$c0,$1f,$fc,$07,$f0,$07,$ff,$80
        .db  $ff,$f0,$70,$07,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fc,$00,$3f,$ff,$e0,$1f,$fc,$07,$f8,$07,$ff,$01,$ff,$e0,$f8,$03,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$fc,$00,$3f,$ff,$e0,$1f,$f8,$0f,$f8,$03,$ff,$01,$ff,$c1,$f8,$01,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fc,$00,$7f,$ff
        .db  $f0,$0f,$f8,$1f,$fc,$03,$ff,$03,$ff,$83,$fc,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fe,$00,$7f,$ff,$f0,$07,$f0,$1f,$fe,$01,$fe,$03
        .db  $ff,$07,$fe,$00,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$fe,$00,$7f,$ff,$f8,$03,$e0,$3f,$fe,$00,$f8,$07,$fe,$07,$fe,$00,$7f,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$00,$ff,$ff,$fc,$00,$00,$7f,$ff,$00,$00,$0f,$f0,$0f,$ff,$00,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$00,$ff,$ff
        .db  $fe,$00,$00,$ff,$ff,$80,$00,$1f,$f0,$1f,$ff,$00,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$01,$ff,$ff,$ff,$80,$03,$ff,$ff,$e0,$00,$7f
        .db  $f0,$1f,$ff,$80,$0f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$81,$ff,$ff,$ff,$e0,$0f,$ff,$ff,$fc,$03,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$03,$ff,$ff,$ff,$ff,$ff,$fc
        .db  $3f,$ff,$ff,$ff,$fc,$ff,$ff,$ff,$f3,$ff,$ff,$ff,$ff,$8f,$cf,$9f,$ff,$39,$ff,$ff,$ff,$ff,$ff,$f8,$ff,$ff,$ff,$ff,$fc,$ff,$3f,$ff
        .db  $f3,$ff,$ff,$ff,$ff,$3f,$cf,$9f,$ff,$39,$ff,$ff,$ff,$ff,$ff,$f9,$ff,$ff,$ff,$ff,$fc,$ff,$3f,$ff,$f3,$ff,$ff,$ff,$ff,$3f,$cf,$ff
        .db  $ff,$39,$86,$60,$10,$d1,$f8,$30,$66,$7c,$30,$46,$64,$86,$0c,$30,$f0,$64,$f8,$4c,$c2,$08,$4c,$9f,$ff,$03,$32,$4e,$66,$4c,$f3,$93
        .db  $26,$79,$93,$32,$64,$f3,$39,$93,$f3,$24,$f3,$cc,$99,$33,$c9,$9f,$ff,$39,$02,$1e,$66,$4c,$92,$93,$26,$78,$13,$32,$64,$c3,$39,$93
        .db  $f3,$24,$f1,$cc,$99,$31,$c3,$9f,$ff,$39,$3e,$1e,$66,$4c,$f2,$93,$26,$79,$f3,$32,$64,$93,$39,$93,$f3,$24,$fc,$6d,$99,$3c,$43,$9f
        .db  $ff,$39,$3e,$4e,$66,$4c,$f3,$93,$26,$79,$f3,$32,$64,$93,$39,$93,$f3,$30,$fe,$61,$99,$3e,$49,$9f,$ff,$03,$82,$66,$70,$c1,$f8,$38
        .db  $60,$3c,$13,$33,$0c,$cb,$8c,$33,$f0,$7c,$f0,$f3,$c3,$30,$cc,$9f,$ff,$ff,$ff,$ff,$ff,$cf,$ff,$ff,$ff,$3f,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$fc,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$cf,$ff,$ff,$ff,$3f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f1,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$cf,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f9,$fe,$7f,$ff,$ff,$ff,$ff,$cf,$ff,$ff,$ff,$ff,$f2,$7f,$9f,$ff,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$f9,$fe,$7f,$ff,$ff,$ff,$ff,$cf,$ff,$ff,$ff,$ff,$f2,$7f,$9f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f9,$fe,$7f,$ff
        .db  $ff,$ff,$ff,$cf,$ff,$ff,$ff,$ff,$f2,$7f,$9f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c1,$86,$0c,$cc,$18,$30,$c3,$c1,$93,$e8,$c3,$48
        .db  $72,$61,$94,$70,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$99,$32,$64,$c9,$93,$26,$4f,$cc,$93,$e6,$79,$3f,$32,$4c,$91,$26,$7f,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$99,$02,$64,$c9,$93,$20,$4f,$cc,$93,$e6,$61,$3c,$32,$40,$93,$26,$7f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$99,$3e,$64,$c9
        .db  $93,$27,$cf,$cc,$93,$e6,$49,$39,$32,$4f,$93,$26,$7f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$99,$3e,$64,$c9,$93,$27,$cf,$cc,$c3,$e6,$49,$39
        .db  $32,$4f,$93,$26,$7f,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$c1,$82,$8e,$1c,$18,$30,$4f,$c1,$f3,$e0,$e5,$3c,$b2,$60,$93,$30,$ff,$ff,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$9f,$3f,$ff,$ff,$f3,$e7,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
        .db  $1e,$3f,$ff,$ff,$c7,$e7,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$f8,$70,$ff,$ff,$ff,$ff,$e7,$ff,$ff
        .db  $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff