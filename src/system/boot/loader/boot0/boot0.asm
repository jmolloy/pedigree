;--Comment's and preprocessing stripped in preperation for port --
;boot0.asm
;MBR Boot Loader

ORG 0x0500

BITS 16
CPU 8086

ENTRY:		CLI
		XOR		AX, AX
		MOV		DS, AX
		MOV		ES, AX
		MOV		SS, AX
		MOV		SP, 0x7C00
		STI

		CLD
		MOV		DI, 0x0500
		MOV		CX, 0x0100
		REP MOVSW
		JMP		0x0000:START

START:		MOV		SI, 0x07BE
		MOV		CX, 0x0004
CheckEntry:	CMP		BYTE [SI], 0x80
		JE		FoundActive
		CMP		BYTE [SI], 0x00
		JNE		InvalidTable
		ADD		SI, 0x10

		LOOP		CheckEntry
		INT		0x18

FoundActive:	MOV		DH, BYTE [SI+1]
		MOV		CX, [SI+2]
		JMP		BootPartition

InvalidTable:	MOV		SI, InvalidTableErrorMsg
		JMP		Abort

BootPartition:	MOV		DI, 0x0005

.START		MOV		BX, 0x7C00
		MOV		AX, 0x0201
		INT		0x13
		JNC		CallBoot
		XOR		AX, AX
		INT		0x13
		DEC		DI
		JNZ		.START

		MOV		SI, LoadingErrorMsg
		JMP		Abort

CallBoot	MOV		DI, 0x7DFE
		CMP		WORD [DI], 0xAA55
		JNE		.ERROR
		JMP		0x0000:0x7C00

.ERROR		MOV		SI, CallBootErrorMsg

Abort:		CALL		PrintString
		MOV		AH, 0x00
		INT		0x16
		INT		0x19

PrintString:	LODSB
		OR		AL, AL
		JZ		.DONE
		MOV		AH, 0x0E
		MOV		BX, 0x0007
		INT		0x10
		JMP		PrintString

.DONE		RET

InvalidTableErrorMsg	DB	0x0D, 0x0A, "Invalid Partition Table", 0x00
LoadingErrorMsg:		DB	0x0D, 0x0A, "Error Loading Boot Sector", 0x00
CallBootErrorMsg:	DB	0x00, 0x0A, "Invalid Boot Sector", 0x00

		TIMES		440-($-$$) DB 0

DummySig:	DD		0x00000000
NullBytes:	DW		0x0000
DummyTable:	TIMES		510-($-$$) DB 0

		DW		0xAA55