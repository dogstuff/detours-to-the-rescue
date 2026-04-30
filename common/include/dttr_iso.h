#ifndef DTTR_ISO_H
#define DTTR_ISO_H

#include <stdbool.h>
#include <stddef.h>

#define DTTR_ISO_MAX_PATH 260

typedef struct {
	bool m_open;
	char m_iso_path[DTTR_ISO_MAX_PATH];
} DTTR_IsoImage;

bool dttr_iso_cache_path_for_file(
	const char *cache_root,
	const char *iso_relative_path,
	char *out_path,
	size_t out_path_size
);
bool dttr_iso_open(DTTR_IsoImage *iso, const char *iso_path);
bool dttr_iso_extract_file(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root,
	char *out_path,
	size_t out_path_size
);
const char *dttr_iso_last_error(void);
void dttr_iso_close(DTTR_IsoImage *iso);

#endif /* DTTR_ISO_H */
