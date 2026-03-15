        .z80                  ; <--- ADD THIS: Sets the assembler to Z80 mode
        .org 0x0000          ; Code starts at memory address 0x0000

        ; ---Z80 ASSEMBLY PROGRAM START---

        ; Set VRAM Address to 0x2000
        ld a, $00            ; Load low byte. Used $ instead of 0x for compatibility
        out ($99), a         ; Send to VDP Port 0x99 (Byte 1)
        ld a, $20            ; Load high byte (0x20 = 32)
        out ($99), a         ; Send to VDP Port 0x99 (Byte 2)

        ; Write 0xFF to VRAM
        ld a, $FF            ; Value to write
        out ($98), a         ; Send to VDP Port 0x98 (Write)

        ; Infinite Loop
loop:   jr loop             ; Jump to label 'loop' (Cleaner than '$')

