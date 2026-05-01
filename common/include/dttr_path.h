#ifndef DTTR_PATH_H
#define DTTR_PATH_H

#include <stdbool.h>
#include <stddef.h>

#include <sds.h>

#define DTTR_PATH_NATIVE_SEPARATOR '\\'

char dttr_path_ascii_lower(char ch);
bool dttr_path_ascii_ieq_n(const char *lhs, const char *rhs, size_t n);

bool dttr_path_copy_string(char *out, size_t out_size, const char *value);
bool dttr_path_copy_sds(char *out, size_t out_size, sds value);

bool dttr_path_is_separator(char ch);
const char *dttr_path_skip_separators(const char *path);
size_t dttr_path_segment_len(const char *path);
bool dttr_path_is_relative_segment(const char *segment, size_t segment_len);
bool dttr_path_is_safe_relative(const char *path);

bool dttr_path_is_windows_absolute(const char *path);
bool dttr_path_is_any_absolute(const char *path);
bool dttr_path_exact_exists(const char *path);

sds dttr_path_current_dir(void);
sds dttr_path_module_dir(void *module);
sds dttr_path_module_sibling(void *module, const char *relative_path);
sds dttr_path_native_root(const char *path, const char **rest);
bool dttr_path_append_char(sds *path, char ch);
bool dttr_path_append_separator(sds *path, char separator);
bool dttr_path_append_segment(sds *path, const char *segment, char separator);

#endif /* DTTR_PATH_H */
