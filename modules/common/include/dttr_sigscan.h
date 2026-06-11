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

// Returns the value of a single hex digit, or -1 when the character is not hex.
static inline int DTTR_Sigscan_HexValue(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	return -1;
}

// Converts a space-separated AOB string ("83 F8 ?? 7C") into the signature byte buffer
// and 'x'/'?' mask consumed by DTTR_Sigscan_Bytes. Writes at most `cap` tokens plus a NUL
// into each output buffer and returns the token count, or 0 when the pattern is empty,
// malformed, or longer than `cap`.
static inline size_t DTTR_Sigscan_ParseAob(
	const char *aob,
	char *sig_out,
	char *mask_out,
	size_t cap
) {
	if (!aob || !sig_out || !mask_out || cap == 0) {
		return 0;
	}

	size_t count = 0;
	const char *p = aob;

	while (*p) {
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
			p++;
		}

		if (!*p) {
			break;
		}

		if (count + 1 >= cap) {
			return 0;
		}

		if (*p == '?') {
			p++;
			if (*p == '?') {
				p++;
			}
			sig_out[count] = '?';
			mask_out[count] = '?';
			count++;
			continue;
		}

		const int hi = DTTR_Sigscan_HexValue(p[0]);
		const int lo = p[1] ? DTTR_Sigscan_HexValue(p[1]) : -1;
		if (hi < 0 || lo < 0) {
			return 0;
		}

		sig_out[count] = (char)((hi << 4) | lo);
		mask_out[count] = 'x';
		count++;
		p += 2;

		if (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
			return 0;
		}
	}

	if (count == 0) {
		return 0;
	}

	sig_out[count] = '\0';
	mask_out[count] = '\0';
	return count;
}

#endif // DTTR_SIGSCAN_H
