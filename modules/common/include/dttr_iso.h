#ifndef DTTR_ISO_H
#define DTTR_ISO_H

#include <stdbool.h>
#include <stddef.h>

#define DTTR_ISO_MAX_PATH 260

typedef struct {
	bool open;
	char iso_path[DTTR_ISO_MAX_PATH];
} DTTR_IsoImage;

bool DTTR_ISO_CachePathForFile(
	const char *cache_root,
	const char *iso_relative_path,
	char *out_path,
	size_t out_path_size
);
bool DTTR_ISO_Open(DTTR_IsoImage *iso, const char *iso_path);
bool DTTR_ISO_ExtractFile(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root,
	char *out_path,
	size_t out_path_size
);
bool DTTR_ISO_ExtractTree(
	DTTR_IsoImage *iso,
	const char *iso_relative_path,
	const char *cache_root
);
const char *DTTR_ISO_LastError();
bool DTTR_ISO_LastErrorWasNotFound();
void DTTR_ISO_Close(DTTR_IsoImage *iso);

#endif // DTTR_ISO_H
