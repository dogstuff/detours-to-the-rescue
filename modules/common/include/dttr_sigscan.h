#ifndef DTTR_SIGSCAN_H
#define DTTR_SIGSCAN_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Scans bytes with 'x' mask entries as exact matches and all other entries as wildcards.
static inline const uint8_t *DTTR_Sigscan_Bytes(
	const uint8_t *base,
	size_t size,
	const void *sig,
	const char *mask
) {
	if (!base || !sig || !mask) {
		return NULL;
	}

	const uint8_t *sig_bytes = sig;
	const size_t mask_len = strlen(mask);
	if (mask_len == 0 || mask_len > size) {
		return NULL;
	}

	for (size_t i = 0; i <= size - mask_len; i++) {
		size_t j = 0;
		for (; j < mask_len; j++) {
			if (mask[j] != 'x') {
				continue;
			}
			if (base[i + j] != sig_bytes[j]) {
				break;
			}
		}

		if (j == mask_len) {
			return base + i;
		}
	}

	return NULL;
}

#endif // DTTR_SIGSCAN_H
