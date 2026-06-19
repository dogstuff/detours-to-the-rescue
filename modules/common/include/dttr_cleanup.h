#ifndef DTTR_CLEANUP_H
#define DTTR_CLEANUP_H

#include <sds.h>
#include <windows.h>

static inline void DTTR_Cleanup_Sds(sds *value) {
	if (!value || !*value) {
		return;
	}

	sdsfree(*value);
	*value = NULL;
}

static inline void DTTR_Cleanup_Handle(HANDLE *handle) {
	if (!handle || !*handle || *handle == INVALID_HANDLE_VALUE) {
		return;
	}

	CloseHandle(*handle);
	*handle = NULL;
}

static inline void DTTR_Cleanup_VirtualAlloc(void **memory) {
	if (!memory || !*memory) {
		return;
	}

	VirtualFree(*memory, 0, MEM_RELEASE);
	*memory = NULL;
}

#endif // DTTR_CLEANUP_H
