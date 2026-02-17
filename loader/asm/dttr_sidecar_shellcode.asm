BITS 32

    jmp strict short call_trick

load_library:
    pop edx                         ; Data pointer (dir_path) pushed by call_trick

    pushfd                          ; Preserve thread entry register/flags state
    pushad
    push edx                        ; Save dir_path pointer (edx is volatile across calls)
    push edx
    mov eax, strict dword 0x0       ; SetDllDirectoryA address (patched at offset 0x08)
    call eax
    pop edx                         ; Restore dir_path pointer
    add edx, strict dword 0x0       ; dir_path_len + 1 (patched at offset 0x11)
    push edx
    mov eax, strict dword 0x0       ; LoadLibraryA address (patched at offset 0x17)
    call eax
    popad
    popfd

    mov eax, strict dword 0x0       ; Original entry address (patched at offset 0x20)
    call eax

    push eax
    mov eax, strict dword 0x0       ; ExitThread address (patched at offset 0x28)
    call eax

    int3

call_trick:

    call load_library ; Push data address as return address
    ; Embed dir_path (null-terminated) then dll_path (null-terminated) here
