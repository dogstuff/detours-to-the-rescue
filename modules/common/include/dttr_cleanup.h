#ifndef DTTR_CLEANUP_H
#define DTTR_CLEANUP_H

#include <windows.h>

static inline void DTTR_Cleanup_VirtualAlloc(void **memory) {
	if (!memory || !*memory) {
		return;
	}

	VirtualFree(*memory, 0, MEM_RELEASE);
	*memory = NULL;
}

#endif // DTTR_CLEANUP_H
