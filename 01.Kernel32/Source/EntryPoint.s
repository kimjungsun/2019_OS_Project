[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1020
	mov ds, ax
	mov es, ax

	cli
	lgdt [ GDTR ]

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;	Protection mode
	;	Disable Paging, Disable Cache,
	;	Internal FPU, Disable Align Check
	;	Enable ProtectedMode
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, 0x4000003B	; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1
	mov cr0, eax		; Set Flags saved above in CR0 Control Register to enter protected mode
	jmp dword 0x08: ( PROTECTEDMODE - $$ + 0x10200)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	Enter protected mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
[BITS 32]
PROTECTEDMODE:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push ( SWITCHSUCCESSMESSAGE - $$ + 0x10200 )
	push 4
	push 0
	call PRINTMESSAGE
	add esp, 12

	jmp dword 0x08:0x10400	; C Language Kernel Execution 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	Function Code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
PRINTMESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx

	mov eax, dword [ ebp + 12 ]
	mov esi, 160
	mul esi
	mov edi, eax

	mov eax, dword [ ebp + 8 ]
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, dword [ ebp + 16 ]

.MESSAGELOOP:
	mov cl, byte [ esi ]
	cmp cl, 0
	je .MESSAGEEND

	mov byte [ edi + 0xB8000 ], cl
	add esi, 1
	add edi, 2
	jmp .MESSAGELOOP

.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	DATA
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
align 8, db 0	; Add the following data to align it with 8bytes

dw 0x0000
GDTR:
	dw GDTEND - GDT - 1
	dd ( GDT - $$ + 0x10200)

GDT:
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
		
	CODEDESCRIPTOR:
		dw 0xFFFF	; Limit [15:0]
		dw 0x0000	; Base [15:0]
		db 0x00		; Base [23:16]
		db 0x9A		; P=1, DPL=0, Code Segment, Execute/Read
		db 0xCF		; G=1, D=1, L=0, Limit[19:16]
		db 0x00		; Base [31:24]

	DATADESCRIPTOR:
		dw 0xFFFF	; Limit [15:0]
		dw 0x0000	; Base [15:0]
		db 0x00		; Base [23:16]
		db 0x92		; P=1, DPL=0, Data Segment, Read/Write
		db 0xCF		; G=1, D=1, L=0, Limit[19:16]
		db 0x00		; Base [31:24]

GDTEND:

SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode Success~!!', 0

times 512 - ( $ - $$ ) db 0x00
