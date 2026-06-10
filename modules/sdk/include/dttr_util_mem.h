/// Helpers for checking whether live game memory can be read safely enough for
/// pointer walks.
///
/// This header is exposed through dttr_sdk.h only when DTTR_SDK_ENABLE_UNSTABLE
/// is set. The unstable surface is still being mapped, so source and ABI
/// details may change without notice.

#ifndef DTTR_UTIL_MEM_H
#define DTTR_UTIL_MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifndef DTTR_SDK_ENABLE_UNSTABLE
#error "Define DTTR_SDK_ENABLE_UNSTABLE before including dttr_util_mem.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Report whether the byte range `[ptr, ptr + size)` is committed and readable.
///
/// The range must fit inside one VirtualQuery region. A readable span that
/// crosses a region boundary still reports `false`.
static inline bool DTTR_Util_MemReadable(const void *ptr, size_t size) {
	if (!ptr || size == 0) {
		return false;
	}

#ifdef _WIN32
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
		return false;
	}

	const uintptr_t start = (uintptr_t)ptr;
	const uintptr_t end = start + size;
	const uintptr_t region_end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
	const DWORD bad = PAGE_NOACCESS | PAGE_GUARD;
	return end >= start && end <= region_end && mbi.State == MEM_COMMIT
		   && (mbi.Protect & bad) == 0;
#else
	// On non-Windows builds this stub treats every range as readable.
	return true;
#endif
}

#ifdef __cplusplus
}
#endif

#endif // DTTR_UTIL_MEM_H
