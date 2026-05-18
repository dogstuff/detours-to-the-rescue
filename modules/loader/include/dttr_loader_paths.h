#ifndef DTTR_LOADER_PATHS_H
#define DTTR_LOADER_PATHS_H

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

bool DTTR_LoaderPath_IsISOW(const wchar_t *path);
size_t DTTR_Loader_GameSubpathCount();
const wchar_t *DTTR_Loader_GameSubpathAt(size_t index);
const char *DTTR_LoaderISO_GameRoot();
const char *DTTR_LoaderISO_GameEXEPath();
const char *DTTR_LoaderISO_GamePkgPath();
const char *DTTR_LoaderISO_GameDataPath();
bool DTTR_LoaderISO_CacheRootForPath(
	const char *cache_base_dir,
	const char *iso_path,
	char *out_path,
	size_t out_path_size
);

#endif // DTTR_LOADER_PATHS_H
