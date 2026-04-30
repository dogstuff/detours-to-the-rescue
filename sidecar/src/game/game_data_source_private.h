#ifndef GAME_DATA_SOURCE_PRIVATE_H
#define GAME_DATA_SOURCE_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>

void dttr_game_data_source_init(void);
void dttr_game_data_source_cleanup(void);
bool dttr_game_data_source_resolve_existing_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);
bool dttr_game_data_source_resolve_read_path(
	const char *path,
	char *out_path,
	size_t out_path_size
);

#endif /* GAME_DATA_SOURCE_PRIVATE_H */
