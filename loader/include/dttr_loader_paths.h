#ifndef DTTR_LOADER_PATHS_H
#define DTTR_LOADER_PATHS_H

#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

bool dttr_loader_path_is_iso_w(const wchar_t *path);
size_t dttr_loader_game_subpath_count(void);
const wchar_t *dttr_loader_game_subpath_at(size_t index);
const char *dttr_loader_iso_game_root(void);
const char *dttr_loader_iso_game_exe_path(void);
const char *dttr_loader_iso_game_pkg_path(void);
const char *dttr_loader_iso_game_data_path(void);
bool dttr_loader_iso_cache_root_for_path(
	const char *cache_base_dir,
	const char *iso_path,
	char *out_path,
	size_t out_path_size
);

#endif /* DTTR_LOADER_PATHS_H */
