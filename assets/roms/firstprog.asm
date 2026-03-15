.z80
.org 0x0000          ; Code runs at memory address 0x0000 (MSX Start)

; ---Z80 ASSEMBLY PROGRAM START---

; Set VRAM Address to 0x2000 (Arbitrary location for fun)
ld a, 0x00           ; Load low byte of address into A
out (0x99), a        ; Send to VDP Port 0x99 (First byte of command)
ld a, 0x20           ; Load high byte of address (0x20 = 32)
out (0x99), a        ; Send to VDP Port 0x99 (Second byte)

; Write the value 0xFF to that address
ld a, 0xFF           ; Value to Write
out (0x98), a        ; Send to VDP Port 0x98 (Write to VRAM)

; Infinite Loop (Halt the system)
jr $                 ; jump to self (relatively nearest location)
