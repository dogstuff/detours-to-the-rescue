#ifndef DTTR_PATH_H
#define DTTR_PATH_H

#include <stdbool.h>
#include <stddef.h>

#include <sds.h>

#define DTTR_PATH_NATIVE_SEPARATOR '\\'

char DTTR_Path_AsciiLower(char ch);
bool DTTR_Path_AsciiIeqN(const char *lhs, const char *rhs, size_t n);

bool DTTR_Path_CopyString(char *out, size_t out_size, const char *value);
bool DTTR_Path_CopySds(char *out, size_t out_size, sds value);

bool DTTR_Path_IsSeparator(char ch);
const char *DTTR_Path_SkipSeparators(const char *path);
size_t DTTR_Path_SegmentLen(const char *path);
bool DTTR_Path_IsRelativeSegment(const char *segment, size_t segment_len);
bool DTTR_Path_IsSafeRelative(const char *path);
bool DTTR_Path_MatchesNormalized(const char *lhs, const char *rhs);

bool DTTR_Path_IsWindowsAbsolute(const char *path);
bool DTTR_Path_IsAnyAbsolute(const char *path);
bool DTTR_Path_ExactExists(const char *path);

sds DTTR_Path_CurrentDir();
sds DTTR_Path_ModuleDir(void *module);
sds DTTR_Path_ModuleSibling(void *module, const char *relative_path);
sds DTTR_Path_ResolveRelativeTo(const char *base_dir, const char *path);
sds DTTR_Path_NativeRoot(const char *path, const char **rest);

bool DTTR_Path_AppendChar(sds *path, char ch);
bool DTTR_Path_AppendSeparator(sds *path, char separator);
bool DTTR_Path_AppendSegment(sds *path, const char *segment, char separator);

#endif // DTTR_PATH_H
