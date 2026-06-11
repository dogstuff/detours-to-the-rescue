#ifndef DTTR_SIDECAR_GAME_DATA_PRIVATE_H
#define DTTR_SIDECAR_GAME_DATA_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>

#include <sds.h>

void dttr_game_data_init();
void dttr_game_data_cleanup();

bool dttr_game_data_resolve_existing_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);
bool dttr_game_data_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);
const char *dttr_game_data_find_data_segment(const char *path);
sds dttr_game_data_resolve_media_path(const char *relative);

#endif // DTTR_SIDECAR_GAME_DATA_PRIVATE_H
