BITS 32

PEB_LDR_OFFSET                                  equ 0x0C
PEB_OFFSET                                      equ 0x30
LDR_INLOADORDERMODULELIST_OFFSET                equ 0x0C
LDR_DATA_TABLE_ENTRY_DLLBASE_OFFSET             equ 0x18
LDR_DATA_TABLE_ENTRY_BASEDLLNAME_BUFFER_OFFSET  equ 0x30
PE_DOS_E_LFANEW_OFFSET                          equ 0x3C
PE_EXPORT_DIRECTORY_RVA_OFFSET                  equ 0x78
PE_EXPORT_NUMBER_OF_NAMES_OFFSET                equ 0x18
PE_EXPORT_ADDRESS_OF_FUNCTIONS_OFFSET           equ 0x1C
PE_EXPORT_ADDRESS_OF_NAMES_OFFSET               equ 0x20
PE_EXPORT_ADDRESS_OF_NAME_ORDINALS_OFFSET       equ 0x24

LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE              equ 0
LOAD_WITH_ALTERED_SEARCH_PATH                   equ 8

WCHAR_SIZE                                      equ 2

DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_READY         equ 1
DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_READY      equ 2
DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_READY       equ 3
DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_READY     equ 4
DTTR_LOADER_SHELLCODE_STATUS_LOADED                 equ 5

DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_NOT_FOUND     equ 0x11
DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_NOT_FOUND  equ 0x12
DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_NOT_FOUND   equ 0x13
DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_NOT_FOUND equ 0x14
DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_FAILED     equ 0x80

struc DTTR_LoaderShellcodePayload
    .m_dll_path:            resb 260
    .m_status:              resd 1
    .m_module:              resd 1
    .m_last_error:          resd 1
    .m_original_entry:      resd 1
    .m_kernel32_name:       resw 13
    .m_loadlibraryex_name:  resb 15
    .m_exitthread_name:     resb 11
    .m_getlasterror_name:   resb 13
    .m_exitthread:          resd 1
endstruc

    jmp payload_entry_ref

loader_entry:
    ; Pop ESI = payload
    pop esi
    push esi
    pushfd
    pushad

    ; Resolve PEB->Ldr
    mov eax, [fs:PEB_OFFSET]
    mov eax, [eax + PEB_LDR_OFFSET]

    ; Get first entry of InLoadOrderModuleList
    lea edi, [eax + LDR_INLOADORDERMODULELIST_OFFSET]
    mov eax, [edi]

.scan_next_module:
    ; If the node is equal to the head, kernel32.dll wasn't found
    cmp eax, edi
    je .kernel32_not_found

    ; Load the module's full dll name
    mov edx, [eax + LDR_DATA_TABLE_ENTRY_BASEDLLNAME_BUFFER_OFFSET]

    ; Skip if null
    test edx, edx
    jz .advance_module

    ; Compare current name to kernel32.dll (case insensitive)
    push eax
    lea ecx, [esi + DTTR_LoaderShellcodePayload.m_kernel32_name]
    call strcmpi_wide_ascii
    pop ebx
    ; If the names match, we found kernel32.dll yay
    test eax, eax
    jnz .found_kernel32
    mov eax, ebx

.advance_module:
    ; Advance to the next module in InLoadOrderModuleList
    mov eax, [eax]
    jmp .scan_next_module

.found_kernel32:

    mov ebx, [ebx + LDR_DATA_TABLE_ENTRY_DLLBASE_OFFSET]  ; DllBase
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_READY

    ; Resolve the imports needed to load the sidecar and terminate the thread gracefully.
    ; Resolve LoadLibraryEx(...)
    lea edx, [esi + DTTR_LoaderShellcodePayload.m_loadlibraryex_name]
    call resolve_module_export
    test eax, eax
    jz .loadlibrary_not_found
    mov edi, eax
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_READY

    ; Resolve ExitThread(...)
    lea edx, [esi + DTTR_LoaderShellcodePayload.m_exitthread_name]
    call resolve_module_export
    test eax, eax
    jz .exitthread_not_found
    mov [esi + DTTR_LoaderShellcodePayload.m_exitthread], eax
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_READY

    ; Resolve GetLastError(...)
    lea edx, [esi + DTTR_LoaderShellcodePayload.m_getlasterror_name]
    call resolve_module_export
    test eax, eax
    jz .getlasterror_not_found
    mov ebp, eax
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_READY

    ; LoadLibraryExA(m_dll_path)
    push byte LOAD_WITH_ALTERED_SEARCH_PATH
    push byte LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE
    push esi
    call edi
    mov [esi + DTTR_LoaderShellcodePayload.m_module], eax

    ; Continue loading if result was valid
    test eax, eax
    jnz .loaded

    ; Otherwise GetLastError() (WinAPI)
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_FAILED
    call ebp
    mov [esi + DTTR_LoaderShellcodePayload.m_last_error], eax
    jmp .restore_and_exit

.loaded:
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_LOADED
    jmp .restore_and_exit

.kernel32_not_found:
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_KERNEL32_NOT_FOUND
    jmp .restore_and_exit

.loadlibrary_not_found:
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_LOADLIBRARY_NOT_FOUND
    jmp .restore_and_exit

.exitthread_not_found:
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_EXITTHREAD_NOT_FOUND
    jmp .restore_and_exit

.getlasterror_not_found:
    mov byte [esi + DTTR_LoaderShellcodePayload.m_status], DTTR_LOADER_SHELLCODE_STATUS_GETLASTERROR_NOT_FOUND

.restore_and_exit:
    popad
    popfd
    pop esi

    ; Return to the game's entrypoint
    call dword [esi + DTTR_LoaderShellcodePayload.m_original_entry]

    ; ExitThread(status)
    push eax
    call dword [esi + DTTR_LoaderShellcodePayload.m_exitthread]

    int3

; Returns whether strings pointed to by EDX and ECX are equal (case-insensitve)
strcmpi_wide_ascii:

.next_char:
    mov ax, [edx]
    mov bx, [ecx]
    cmp ax, 'A'
    jb .fold_rhs
    cmp ax, 'Z'
    ja .fold_rhs
    add ax, 0x20

.fold_rhs:
    cmp bx, 'A'
    jb .compare
    cmp bx, 'Z'
    ja .compare
    add bx, 0x20

.compare:
    cmp ax, bx
    jne .no_match
    test ax, ax
    je .match
    add edx, byte 2
    add ecx, byte 2
    jmp .next_char

.no_match:
    xor eax, eax
    ret

.match:
    mov eax, 1
    ret

; Get the absolute address of the symbol at
resolve_module_export:

    push esi
    push edi
    push ebp

    ; Read RVAs and compute effective addresses
    ; EAX = PE header
    mov eax, [ebx + PE_DOS_E_LFANEW_OFFSET]
    add eax, ebx

    ; EAX = &IMAGE_EXPORT_DIRECTORY
    mov eax, [eax + PE_EXPORT_DIRECTORY_RVA_OFFSET]
    add eax, ebx

    ; ECX = number of named exports
    mov ecx, [eax + PE_EXPORT_NUMBER_OF_NAMES_OFFSET]

    ; EDI = export name RVA table
    mov edi, [eax + PE_EXPORT_ADDRESS_OF_NAMES_OFFSET]
    add edi, ebx

    ; ESI = name ordinals table
    mov esi, [eax + PE_EXPORT_ADDRESS_OF_NAME_ORDINALS_OFFSET]
    add esi, ebx

    ; EBP = export function RVA table
    mov ebp, [eax + PE_EXPORT_ADDRESS_OF_FUNCTIONS_OFFSET]
    add ebp, ebx

.export_name_loop:
    dec ecx
    js .not_found

    mov eax, [edi + ecx * 4]
    add eax, ebx

    push ecx
    push edx

.compare:
    mov cl, [eax]
    mov ch, [edx]
    cmp cl, ch
    jne .no_match
    test cl, cl
    je .match
    inc eax
    inc edx
    jmp .compare

.no_match:
    pop edx
    pop ecx
    jmp .export_name_loop

.match:
    pop edx
    pop ecx
    movzx eax, word [esi + ecx * 2]
    mov eax, [ebp + eax * 4]
    add eax, ebx
    jmp .done

.not_found:
    xor eax, eax

.done:
    pop ebp
    pop edi
    pop esi
    ret

payload_entry_ref:
    call loader_entry
