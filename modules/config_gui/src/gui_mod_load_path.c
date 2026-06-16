#include "gui_mod_load_path.h"

#include <dttr_path.h>

#include <stdio.h>
#include <string.h>

// Resolves the "modules" dir.
bool config_ui_build_mod_dependency_dir(const char *mods_dir, char *out, size_t out_size) {
	if (!out || out_size == 0) {
		return false;
	}

	out[0] = '\0';

	if (!mods_dir || !mods_dir[0]) {
		return false;
	}

	size_t len = strlen(mods_dir);
	while (len > 0 && DTTR_Path_IsSeparator(mods_dir[len - 1])) {
		len--;
	}

	if (len == 0) {
		return false;
	}

	size_t parent_len = len;
	while (parent_len > 0 && !DTTR_Path_IsSeparator(mods_dir[parent_len - 1])) {
		parent_len--;
	}

	const int written = snprintf(out, out_size, "%.*smodules", (int)parent_len, mods_dir);
	if (written <= 0 || (size_t)written >= out_size) {
		out[0] = '\0';
		return false;
	}

	return true;
}
