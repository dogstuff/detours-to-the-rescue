#define DTTR_SDK_ENABLE_UNSTABLE

#include <dttr_util_unstable.h>

static DTTR_Util_PkgWalkResult make_result(DTTR_Util_PkgVisitStatus status) {
	DTTR_Util_PkgWalkResult result = {0};

	result.status = status;

	return result;
}

DTTR_Util_PkgWalkOptions DTTR_Util_PkgWalk_DefaultOptions() {
	DTTR_Util_PkgWalkOptions options = {0};

	options.struct_size = sizeof(options);
	options.domains = DTTR_UTIL_PKG_DOMAIN_ALL_KNOWN;
	options.max_depth = 64;
	options.load_entries = true;
	options.toc_count = DTTR_UTIL_PKG_DEFAULT_TOC_COUNT;

	return options;
}

const char *DTTR_Util_PkgVisitStatusName(DTTR_Util_PkgVisitStatus status) {
	switch (status) {
	case DTTR_UTIL_PKG_STATUS_OK:
		return "ok";
	case DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT:
		return "invalid argument";
	case DTTR_UTIL_PKG_STATUS_UNRESOLVED_SYMBOL:
		return "unresolved symbol";
	case DTTR_UTIL_PKG_STATUS_LOAD_FAILED:
		return "load failed";
	case DTTR_UTIL_PKG_STATUS_UNKNOWN_ENTRY_KIND:
		return "unknown entry kind";
	case DTTR_UTIL_PKG_STATUS_DECODE_UNSUPPORTED:
		return "decode unsupported";
	case DTTR_UTIL_PKG_STATUS_BOUNDS_INVALID:
		return "bounds invalid";
	default:
		return "unknown status";
	}
}

static bool default_load_entry(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TocEntry *entry,
	void *userdata,
	void **out_entry,
	size_t *out_size,
	DTTR_Util_PkgVisitStatus *out_status
) {
	if (out_entry) {
		*out_entry = NULL;
	}

	if (out_size) {
		*out_size = 0;
	}

	if (!entry || !out_entry || !out_size || toc_index < 0) {
		if (out_status) {
			*out_status = DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT;
		}

		return false;
	}

	void *loaded = NULL;
	if (!DTTR_PCDOGS_F_PKGLoadEntry->Try(ctx, toc_index, NULL, &loaded) || !loaded) {
		if (out_status) {
			*out_status = DTTR_UTIL_PKG_STATUS_LOAD_FAILED;
		}

		return false;
	}

	*out_entry = loaded;
	*out_size = entry->size;

	if (out_status) {
		*out_status = DTTR_UTIL_PKG_STATUS_OK;
	}

	return true;
}

static void default_free_entry(
	const DTTR_Core_Context *ctx,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TocEntry *entry,
	void *entry_data,
	void *userdata
) {
	if (!entry_data) {
		return;
	}

	BOOL ignored = FALSE;
	DTTR_PCDOGS_F_ResourceFreeData->Try(ctx, entry_data, &ignored);
}

static bool resolve_toc(
	const DTTR_Core_Context *ctx,
	const DTTR_Util_PkgWalkOptions *options,
	const DTTR_PCDOGS_T_Pkg_TocEntry **out_entries,
	uint32_t *out_count
) {
	if (!out_entries || !out_count) {
		return false;
	}

	*out_entries = NULL;
	*out_count = 0;

	uint32_t requested_count = options->toc_count ? options->toc_count
												  : DTTR_UTIL_PKG_DEFAULT_TOC_COUNT;

	if (options->toc_entries) {
		*out_entries = options->toc_entries;
		*out_count = requested_count;
		return true;
	}

	uintptr_t toc_addr = 0;
	DTTR_Core_Result resolved = DTTR_PCDOGS_DataResolve(
		ctx,
		DTTR_PCDOGS_DATA_PKG_TOC,
		&toc_addr
	);
	if (!DTTR_Core_ResultOk(resolved) || !toc_addr) {
		return false;
	}

	*out_entries = (const DTTR_PCDOGS_T_Pkg_TocEntry *)toc_addr;
	*out_count = requested_count;

	return true;
}

static DTTR_Util_PkgVisitAction emit_visit(
	DTTR_Util_PkgWalkResult *result,
	DTTR_Util_PkgVisitor visitor,
	void *visitor_userdata,
	const DTTR_Util_PkgVisit *visit
) {
	if (result) {
		result->visited_count++;
		result->failed_count += visit && visit->status != DTTR_UTIL_PKG_STATUS_OK;
	}

	return visitor ? visitor(visit, visitor_userdata) : DTTR_UTIL_PKG_VISIT_CONTINUE;
}

static DTTR_Util_PkgVisit make_visit(
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitKind kind,
	DTTR_Util_PkgVisitStatus status,
	uint32_t depth,
	int32_t toc_index,
	const DTTR_PCDOGS_T_Pkg_TocEntry *toc_entry,
	const void *ptr,
	const void *loaded_entry_base,
	size_t loaded_entry_size,
	const DTTR_Util_PkgVisit *parent
) {
	DTTR_Util_PkgVisit visit = {0};

	visit.struct_size = sizeof(visit);
	visit.kind = kind;
	visit.status = status;
	visit.depth = depth;
	visit.toc_index = toc_index;

	if (toc_entry) {
		visit.pkg_offset = toc_entry->offset;
		visit.pkg_size = toc_entry->size;
	}

	visit.ptr = ptr;
	visit.parent = parent;
	visit.toc_entry = toc_entry;
	visit.loaded_entry_base = loaded_entry_base;
	visit.loaded_entry_size = loaded_entry_size;
	visit.userdata = options ? options->io_userdata : NULL;

	return visit;
}

static bool should_recurse(DTTR_Util_PkgVisitAction action) {
	return action == DTTR_UTIL_PKG_VISIT_RECURSE
		   || action == DTTR_UTIL_PKG_VISIT_LOAD_AND_RECURSE;
}

static bool should_load(
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitAction action
) {
	if (!(options->domains & DTTR_UTIL_PKG_DOMAIN_ENTRY)) {
		return false;
	}

	if (action == DTTR_UTIL_PKG_VISIT_LOAD_AND_RECURSE) {
		return true;
	}

	return options->load_entries && action == DTTR_UTIL_PKG_VISIT_RECURSE;
}

static bool emit_decode_boundary(
	DTTR_Util_PkgWalkResult *result,
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitor visitor,
	void *visitor_userdata,
	const DTTR_Util_PkgVisit *loaded_visit
) {
	if (!(options->domains & DTTR_UTIL_PKG_DOMAIN_KNOWN_CHILDREN)
		|| loaded_visit->depth + 1 > options->max_depth) {
		return false;
	}

	DTTR_Util_PkgVisit unsupported_visit = make_visit(
		options,
		DTTR_UTIL_PKG_VISIT_UNSUPPORTED,
		DTTR_UTIL_PKG_STATUS_OK,
		loaded_visit->depth + 1,
		loaded_visit->toc_index,
		loaded_visit->toc_entry,
		loaded_visit->ptr,
		loaded_visit->loaded_entry_base,
		loaded_visit->loaded_entry_size,
		loaded_visit
	);
	DTTR_Util_PkgVisitAction action = emit_visit(
		result,
		visitor,
		visitor_userdata,
		&unsupported_visit
	);
	if (action == DTTR_UTIL_PKG_VISIT_STOP) {
		result->stopped = true;
		return true;
	}

	return false;
}

DTTR_Util_PkgWalkResult DTTR_Util_PkgWalk(
	const DTTR_Core_Context *ctx,
	const DTTR_Util_PkgWalkOptions *options,
	DTTR_Util_PkgVisitor visitor,
	void *userdata
) {
	DTTR_Util_PkgWalkOptions defaults = DTTR_Util_PkgWalk_DefaultOptions();
	if (!options) {
		options = &defaults;
	}

	if (!visitor) {
		return make_result(DTTR_UTIL_PKG_STATUS_INVALID_ARGUMENT);
	}

	DTTR_Util_PkgWalkResult result = make_result(DTTR_UTIL_PKG_STATUS_OK);
	if (!(options->domains & DTTR_UTIL_PKG_DOMAIN_TOC)) {
		return result;
	}

	const DTTR_PCDOGS_T_Pkg_TocEntry *toc_entries = NULL;
	uint32_t toc_count = 0;
	if (!resolve_toc(ctx, options, &toc_entries, &toc_count)) {
		return make_result(DTTR_UTIL_PKG_STATUS_UNRESOLVED_SYMBOL);
	}

	DTTR_Util_PkgLoadEntryFn load_entry = options->load_entry ? options->load_entry
															  : default_load_entry;
	DTTR_Util_PkgFreeEntryFn free_entry = options->free_entry ? options->free_entry
															  : default_free_entry;
	uint32_t max_depth = options->max_depth;

	for (uint32_t i = 0; i < toc_count; ++i) {
		const DTTR_PCDOGS_T_Pkg_TocEntry *toc_entry = &toc_entries[i];

		DTTR_Util_PkgVisit toc_visit = make_visit(
			options,
			DTTR_UTIL_PKG_VISIT_TOC_ENTRY,
			DTTR_UTIL_PKG_STATUS_OK,
			0,
			(int32_t)i,
			toc_entry,
			toc_entry,
			NULL,
			0,
			NULL
		);

		DTTR_Util_PkgVisitAction toc_action = emit_visit(
			&result,
			visitor,
			userdata,
			&toc_visit
		);

		if (toc_action == DTTR_UTIL_PKG_VISIT_STOP) {
			result.stopped = true;
			break;
		}

		if (toc_action == DTTR_UTIL_PKG_VISIT_SKIP_SUBTREE || max_depth < 1
			|| !should_load(options, toc_action)) {
			continue;
		}

		void *loaded = NULL;
		size_t loaded_size = 0;
		DTTR_Util_PkgVisitStatus load_status = DTTR_UTIL_PKG_STATUS_OK;

		if (!load_entry(
				ctx,
				(int32_t)i,
				toc_entry,
				options->io_userdata,
				&loaded,
				&loaded_size,
				&load_status
			)
			|| !loaded) {

			if (load_status == DTTR_UTIL_PKG_STATUS_OK) {
				load_status = DTTR_UTIL_PKG_STATUS_LOAD_FAILED;
			}

			DTTR_Util_PkgVisit failed_visit = make_visit(
				options,
				DTTR_UTIL_PKG_VISIT_UNSUPPORTED,
				load_status,
				1,
				(int32_t)i,
				toc_entry,
				NULL,
				NULL,
				0,
				&toc_visit
			);

			DTTR_Util_PkgVisitAction failed_action = emit_visit(
				&result,
				visitor,
				userdata,
				&failed_visit
			);

			if (failed_action == DTTR_UTIL_PKG_VISIT_STOP) {
				result.stopped = true;
				break;
			}

			continue;
		}

		result.loaded_count++;

		DTTR_Util_PkgVisit loaded_visit = make_visit(
			options,
			DTTR_UTIL_PKG_VISIT_LOADED_ENTRY,
			DTTR_UTIL_PKG_STATUS_OK,
			1,
			(int32_t)i,
			toc_entry,
			loaded,
			loaded,
			loaded_size,
			&toc_visit
		);

		DTTR_Util_PkgVisitAction loaded_action = emit_visit(
			&result,
			visitor,
			userdata,
			&loaded_visit
		);

		if (loaded_action == DTTR_UTIL_PKG_VISIT_STOP) {
			result.stopped = true;
			free_entry(ctx, (int32_t)i, toc_entry, loaded, options->io_userdata);
			break;
		}

		if (should_recurse(loaded_action)
			&& emit_decode_boundary(&result, options, visitor, userdata, &loaded_visit)) {
			free_entry(ctx, (int32_t)i, toc_entry, loaded, options->io_userdata);
			break;
		}

		free_entry(ctx, (int32_t)i, toc_entry, loaded, options->io_userdata);
	}

	return result;
}
