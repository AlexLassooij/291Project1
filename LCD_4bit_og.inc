cseg

; When using a 24.5MHz crystal in fast mode
; one cycle takes 1.0/24.5MHz = 40.81633 ns

;---------------------------------;
; Wait 40 microseconds            ;
;---------------------------------;
Wait40uSec:
	push AR0
	mov R0, #196
L0: 
	nop
	nop
	djnz R0, L0 ; 1+1+3 cycles->5*40.81633ns*196=40us
	pop AR0
    ret

;---------------------------------;
; Wait 'R2' milliseconds          ;
;---------------------------------;
Wait_Milli_Seconds mac
	push AR2
	mov R2, %0
	lcall ?Wait_Milli_Seconds
	pop AR2
endmac

?Wait_Milli_Seconds:
	push AR0
	push AR1
L3: mov R1, #240
L2: mov R0, #180
L1: djnz R0, L1 ; 3 cycles->3*40.81633ns*180=22.04082us
    djnz R1, L2 ; 22.04082us*46=1.013ms
    djnz R2, L3 ; number of millisecons to wait passed in R2
    pop AR1
    pop AR0
    ret
	
;---------------------------------;
; Toggles the 'E' pin in the LCD  ;
;---------------------------------;
LCD_pulse:
	setb LCD_E
	lcall Wait40uSec
	lcall Wait40uSec
	lcall Wait40uSec
	lcall Wait40uSec
	clr LCD_E
    ret

;---------------------------------;
; Writes acc to LCD in 4-bit mode ;
;---------------------------------;
LCD_byte:
	; Write high 4 bits first
	mov c, ACC.7
	mov LCD_D7, c
	mov c, ACC.6
	mov LCD_D6, c
	mov c, ACC.5
	mov LCD_D5, c
	mov c, ACC.4
	mov LCD_D4, c
    lcall LCD_pulse
    lcall Wait40uSec
	lcall Wait40uSec
	lcall Wait40uSec
	lcall Wait40uSec
	; Write low 4 bits next
	mov c, ACC.3
	mov LCD_D7, c
	mov c, ACC.2
	mov LCD_D6, c
	mov c, ACC.1
	mov LCD_D5, c
	mov c, ACC.0
	mov LCD_D4, c
    lcall LCD_pulse
	ret

;---------------------------------;
; Write data to LCD               ;
;---------------------------------;
WriteData mac
	mov a, %0
	lcall ?WriteData
endmac
	
?WriteData:
	setb LCD_RS
	ljmp LCD_byte

;---------------------------------;
; Write command to LCD            ;
;---------------------------------;
WriteCommand mac
	mov a, %0
	lcall ?WriteCommand
endmac

?WriteCommand:
	clr LCD_RS
	ljmp LCD_byte

;---------------------------------;
; Configure LCD in 4-bit mode     ;
;---------------------------------;
LCD_4BIT:
	clr LCD_E   ; Resting state of LCD's enable pin is zero
	clr LCD_RW  ; We are only writing to the LCD in this program
	
	; After power on, let the LCD start up before initializing
	Wait_Milli_Seconds(#120)
	
	; First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(#0x33)
	WriteCommand(#0x33)
	WriteCommand(#0x32) ; change to 4-bit mode

	; Configure the LCD
	WriteCommand(#0x28)
	WriteCommand(#0x0c)
	WriteCommand(#0x01) ;  Clear screen command (takes some time)

    ;Wait for the clear screen command to finish.
	Wait_Milli_Seconds(#20)
    ret

;--------------------------------------;
; Send a constant string to LCD Line 1 ;
;--------------------------------------;
Send_Constant_String_L1 mac
	; data pointer high and low
	push dph
	push dpl
	push acc
	mov dptr, %0
	clr a
	lcall ?Set_Cursor_1
	lcall ?Send_Constant_String
	pop acc
	pop dpl
	pop dph
endmac

;--------------------------------------;
; Send a constant string to LCD Line 2 ;
;--------------------------------------;
Send_Constant_String_L2 mac
	; data pointer high and low
	push dph
	push dpl
	push acc
	mov dptr, %0
	clr a
	lcall ?Set_Cursor_2
	lcall ?Send_Constant_String
	pop acc
	pop dpl
	pop dph
endmac

?Send_Constant_String:
    clr a
    movc a, @a+dptr
    jz ?Send_Constant_String_Done
    lcall ?WriteData
    inc dptr
    sjmp ?Send_Constant_String
?Send_Constant_String_Done:
    ret	

;---------------------------------;
; Set LCD cursor at row, column   ;
;---------------------------------;
Set_Cursor mac
	push acc
	mov a, #%1
	dec a
	lcall ?Set_Cursor_%0 ; Select column and row
	pop acc
endmac

?Set_Cursor_2:
	orl a, #01000000B
?Set_Cursor_1:
	orl a, #10000000B
	ljmp ?WriteCommand ; Select column and row

;---------------------------------;
; Display a BCD number in the LCD ;
;---------------------------------;
Display_BCD mac
	push ar0
	mov r0, %0
	lcall ?Display_BCD
	pop ar0
endmac

?Display_BCD:
	push acc
	; Write most significant digit
	mov a, r0
	swap a
	anl a, #0fh
	orl a, #30h
	lcall ?WriteData
	; write least significant digit
	mov a, r0
	anl a, #0fh
	orl a, #30h
	lcall ?WriteData
	pop acc
	ret

Left_blank mac
	mov a, %0
	anl a, #0xf0
	swap a
	jz Left_blank_%M_a
	ljmp %1
Left_blank_%M_a:
	Display_char(#' ')
	mov a, %0
	anl a, #0x0f
	jz Left_blank_%M_b
	ljmp %1
Left_blank_%M_b:
	Display_char(#' ')
endmac

;------------------------------------;
; Display a char in the LCD          ;
;------------------------------------;
Display_char mac
	push acc
	mov a, %0
	lcall ?WriteData
	pop acc
endmac

Display_formated_BCD_2:
	Set_Cursor(2, 1)
	Display_BCD(bcd+4)
	Display_BCD(bcd+3)
	Display_BCD(bcd+2)
	Set_Cursor(2,1)
	Left_blank(bcd+4, skip_blank_format)
	Left_blank(bcd+3, skip_blank_format)
	Left_blank(bcd+2, skip_blank_format)
skip_blank_format:
	Set_Cursor(2,7)
	Display_char(#'.')
	Display_BCD(bcd+1)
	Display_BCD(bcd+0)
	ret

Display_unformated_BCD:
	Set_Cursor(2, 1)
	Display_BCD(bcd+4)
	Display_BCD(bcd+3)
	Display_BCD(bcd+2)
	Display_BCD(bcd+1)
	Display_BCD(bcd+0)
	ret

;Converts the hex number in TH0-TL0 to packed BCD in R2-R1-R0
hex2bcd_5:
	clr a
    mov R0, #0  ; Set packed BCD result to 00000 
    mov R1, #0
    mov R2, #0
    mov R3, #16 ; Loop counter.
    
hex2bcd_D0:
    mov a, TL0 ; Shift TH0-TL0 left through carry
    rlc a
    mov TL0, a
    
    mov a, TH0
    rlc a
    mov TH0, a
    
	; Perform bcd + bcd + carry
	; using BCD numbers
	mov a, R0
	addc a, R0
	da a
	mov R0, a
	
	mov a, R1
	addc a, R1
	da a
	mov R1, a
	
	mov a, R2
	addc a, R2
	da a
	mov R2, a
	
	djnz R3, hex2bcd_D0
	ret

; Dumps the 5-digit packed BCD number in R2-R1-R0 into the LCD
DisplayBCD_5:
	; 5th digit:
    mov a, R2
    anl a, #0FH
    orl a, #'0' ; convert to ASCII
	lcall ?WriteData
	; 4th digit:
    mov a, R1
    swap a
    anl a, #0FH
    orl a, #'0' ; convert to ASCII
	lcall ?WriteData
	; 3rd digit:
    mov a, R1
    anl a, #0FH
    orl a, #'0' ; convert to ASCII
	lcall ?WriteData
	; 2nd digit:
    mov a, R0
    swap a
    anl a, #0FH
    orl a, #'0' ; convert to ASCII
	lcall ?WriteData
	; 1st digit:
    mov a, R0
    anl a, #0FH
    orl a, #'0' ; convert to ASCII
	lcall ?WriteData
    
    ret

	WaitSec mac
    push AR2
    push AR3
    mov R3, %0
L4%M:	mov R2, #200
	lcall ?Wait_Milli_Seconds
	mov R2, #200
	lcall ?Wait_Milli_Seconds
	mov R2, #200
	lcall ?Wait_Milli_Seconds
	mov R2, #200
	lcall ?Wait_Milli_Seconds
	mov R2, #200
	lcall ?Wait_Milli_Seconds
	djnz R3, L4%M
    pop AR2
    pop AR3
endmac

Wait_one_second:	
    ;For a 24.5MHz clock one machine cycle takes 1/24.5MHz=40.81633ns
	push AR3
	push AR2
	push AR1
	push AR0
	mov R3, #2
X4: mov R2, #198 ; Calibrate using this number to account for overhead delays
X3: mov R1, #245
X2: mov R0, #167
X1: djnz R0, X1 ; 3 machine cycles -> 3*40.81633ns*167=20.44898us (see table 10.2 in reference manual)
    djnz R1, X2 ; 20.44898us*245=5.01ms
    djnz R2, X3 ; 5.01ms*198=0.991s + overhead
	djnz R3, X4
	pop AR3
	pop AR2
	pop AR1
	pop AR0
    ret

wait_for_response mac
response_loop_%M:
	jb ONREQ, frequency_response_%M ; loop while the button is not pressed
	jnb ONREQ, $ ; loop while the button is pressed
    clr mode
    ljmp %0

frequency_response_%M:
    jb CAL_freq, nF_response_%M 
	jnb CAL_freq, $ 
    ljmp calc_quad

nF_response_%M:
    jb CAL_nF, uF_response_%M 
	jnb CAL_nF, $ 
    ljmp calc_percent

uF_response_%M:
    jb CAL_uF, auto_recal_%M 
	jnb CAL_uF, $ 
    ljmp cap_pf

auto_recal_%M:
    jb AUTO, eval_en_%M 
	jnb AUTO, $ 
    setb mode
    ljmp %0

eval_en_%M:
    jnb mode, response_loop_%M
	;clr recalc_en
    WaitSec(#1)
    ljmp %0

endmac




	
