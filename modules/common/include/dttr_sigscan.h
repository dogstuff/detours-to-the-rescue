#ifndef DTTR_SIGSCAN_H
#define DTTR_SIGSCAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline bool DTTR_Sigscan_MatchesAt(
	const uint8_t *base,
	const uint8_t *sig,
	const char *mask,
	size_t mask_len
) {
	for (size_t i = 0; i < mask_len; i++) {
		if (mask[i] != 'x') {
			continue;
		}

		if (base[i] != sig[i]) {
			return false;
		}
	}

	return true;
}

static inline size_t DTTR_Sigscan_Anchor(const char *mask, size_t mask_len) {
	for (size_t i = 0; i < mask_len; i++) {
		if (mask[i] == 'x') {
			return i;
		}
	}

	return mask_len;
}

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
		if (DTTR_Sigscan_MatchesAt(base + i, sig_bytes, mask, mask_len)) {
			return base + i;
		}
	}

	return NULL;
}

// Scans bytes for every masked signature match. Returns the total match count and
// writes up to `out_cap` matching addresses when `out_addrs` is not null.
static inline size_t DTTR_Sigscan_BytesAll(
	const uint8_t *base,
	size_t size,
	const void *sig,
	const char *mask,
	uintptr_t *out_addrs,
	size_t out_cap
) {
	if (!base || !sig || !mask) {
		return 0;
	}

	if (!out_addrs) {
		out_cap = 0;
	}

	const uint8_t *sig_bytes = sig;
	const size_t mask_len = strlen(mask);
	if (mask_len == 0 || mask_len > size) {
		return 0;
	}

	const size_t anchor = DTTR_Sigscan_Anchor(mask, mask_len);
	if (anchor == mask_len) {
		const size_t total = size - mask_len + 1u;
		const size_t copy_count = total < out_cap ? total : out_cap;

		for (size_t i = 0; i < copy_count; i++) {
			out_addrs[i] = (uintptr_t)(base + i);
		}

		return total;
	}

	const uint8_t anchor_byte = sig_bytes[anchor];
	const uint8_t *cursor = base + anchor;
	const uint8_t *end = cursor + (size - mask_len + 1u);
	size_t count = 0;

	while (cursor < end) {
		const uint8_t *candidate = memchr(cursor, anchor_byte, (size_t)(end - cursor));
		if (!candidate) {
			break;
		}

		const size_t offset = (size_t)(candidate - base) - anchor;
		if (!DTTR_Sigscan_MatchesAt(base + offset, sig_bytes, mask, mask_len)) {
			cursor = candidate + 1;
			continue;
		}

		if (count < out_cap) {
			out_addrs[count] = (uintptr_t)(base + offset);
		}

		count++;
		cursor = candidate + 1;
	}

	return count;
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
