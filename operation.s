section .text

global _operation_lock@4
global _operation_unlock@4

extern _Sleep@4
extern _GetTickCount@0
extern _GetCurrentThreadId@0

; operation_lock: this function locks a resource using its OPERATION_LOCK structure.
; in newserv, the OPERATION_LOCK comes first in any resource structure, so any
; resource can be locked by simply passing the resource to operation_lock.
_operation_lock@4:
    push ebx
    push edx
    push ecx
    push eax
    call _GetCurrentThreadId@0
    push eax
    mov edx, [esp+0x18]
    mov ecx, [edx+0x08]
    cmp eax, ecx
    jne _operation_lock_if1_false
    inc dword [edx+0x0C]
    jmp _operation_lock_end
  _operation_lock_if1_false:
    mov edx, [esp+0x18]
    cmp dword [edx+0x08], 0x00000000
    je _operation_lock_lock
    push 0x00000005
    call _Sleep@4
    jmp _operation_lock_if1_false
  _operation_lock_lock:
    mov edx, [esp+0x18]
    mov eax, [esp]
    mov [edx+0x08], eax
    mov ecx, [edx+0x08]
    cmp ecx, eax
    jne _operation_lock_if1_false
    mov ecx, [esp+0x14]
    mov [edx], ecx
    call _GetTickCount@0
    mov edx, [esp+0x18]
    mov [edx+0x04], eax
    mov dword [edx+0x0C], 0x00000001
  _operation_lock_end:
    add esp, 4
    pop eax
    pop ecx
    pop edx
    pop ebx
    ret 4

; operation_unlock: unlocks a locked resource. raises an exception (int 03) if
; the object is locked by another thread.
_operation_unlock@4:
    push edx
    push ecx
    push eax
    call _GetCurrentThreadId@0
    mov edx, [esp+0x10]
    cmp [edx+0x08], eax
    je _operation_unlock_threadIDOk
    int 03
  _operation_unlock_threadIDOk:
    cmp dword [edx+0x0C], 0x00000000
    jne _operation_unlock_lockCountOk
    int 03
  _operation_unlock_lockCountOk:
    dec dword [edx+0x0C]
    cmp dword [edx+0x0C], 0x00000000
    jne _operation_unlock_end
    mov dword [edx], 0x00000000
    mov dword [edx+0x04], 0x00000000
    mov dword [edx+0x08], 0x00000000
  _operation_unlock_end:
    pop eax
    pop ecx
    pop edx
    ret 4
