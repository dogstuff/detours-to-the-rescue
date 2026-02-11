BITS 32

    jmp strict short call_trick

load_library:
    pop edx                         ; DLL path pushed by call_trick

    pushfd                          ; Preserve thread entry register/flags state
    pushad
    push edx
    mov eax, strict dword 0x0       ; LoadLibraryA address (patched at offset 0x07)
    call eax
    popad
    popfd

    mov eax, strict dword 0x0       ; Original entry address (patched at offset 0x10)
    call eax

    push eax
    mov eax, strict dword 0x0       ; ExitThread address (patched at offset 0x18)
    call eax

    int3

call_trick:

    call load_library ; Push string as return address
    ; Embed string here
