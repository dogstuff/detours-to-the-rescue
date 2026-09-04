
/// @file dttr_pcdogs.h
/// PCDOGS symbols and typed wrappers. Define `DTTR_SDK_ENABLE_UNSTABLE`
/// before including this header to expose experimental symbols.
#ifndef ${header_guard}
#define ${header_guard}

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

#include <dttr_core.h>


<%def name="render_type_row(row)">
% if type_row_kind(row) == TYPE_ROW.TYPE_ALIAS:
${doxy_brief(row_doc(row) or "PCDOGS value alias.")}
typedef ${c_type(row.source_type)} ${alias_name(row.name)};
% elif type_row_kind(row) == TYPE_ROW.FUNCTION_TYPE_ALIAS:
${doxy_comment(row_doc(row) or "PCDOGS callback type.", params=param_doc_pairs(row.params))}
typedef ${c_type(row.ret)}(${row.calling}*${function_type_name(row.name)})${c_params(row.params) if row.params else "(void)"};
% elif type_row_kind(row) == TYPE_ROW.STRUCT:
% if row.incomplete:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
${doxy_brief(row_doc(row)) if row_doc(row) else ""}
% else:
${doxy_brief(row_doc(row)) if row_doc(row) else ""}
% endif
% if row.incomplete and row.size is not None:
// Size 0x${format(row.size, "X")}; layout reflects currently known fields.
% elif row.size is not None:
// Size 0x${format(row.size, "X")}
% endif
struct ${struct_name(row.name)} {
% for member in row.members:
	${c_type(member.type)} ${member.name};${" ///< " + member_doc(member) if member_doc(member) else ""}
% endfor
};
% if row.incomplete:
#endif
% endif
% elif type_row_kind(row) == TYPE_ROW.ENUM:
${doxy_brief(row_doc(row) or "PCDOGS enum.")}
typedef enum ${enum_name(row.name)} {
% for value in row.values:
	${value.name} = ${value.value},${" ///< " + doxy_inline(row_doc(value)) if row_doc(value) else ""}
% endfor
} ${enum_name(row.alias or row.name)};
	% endif
		</%def>
% for name in forward_names:
typedef struct ${struct_name(name)} ${struct_name(name)};
% endfor
	% for row in type_prefix_rows:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
	${render_type_row(row).strip()}
% if row.unstable:
#endif
% endif
	% endfor
% if packed_type_rows:
<% in_pack = False %>
% for row in packed_type_rows:
<% is_struct = type_row_kind(row) == TYPE_ROW.STRUCT %>
% if is_struct and not in_pack:
#pragma pack(push, 1)
<% in_pack = True %>
% elif in_pack and not is_struct:
#pragma pack(pop)
<% in_pack = False %>
% endif
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
${render_type_row(row).strip()}
% if row.unstable:
#endif
% endif
% endfor
% if in_pack:
#pragma pack(pop)
% endif
% endif

#ifndef DTTR_PCDOGS_API
#define DTTR_PCDOGS_API extern
#endif

/// Hook-site shape for a PCDOGS function.
///
/// `REL32` sites can use the `Hook()` helper and `PatchSpec()` helper because
/// the function entry has a full trampoline prologue window. `HOTPATCH` sites describe a
/// two-part hotpatch layout: a five-byte pre-entry jump slot plus the short entry
/// instruction window reported by `HookPrologueSize()`. The typed helper does
/// not install those hotpatch hooks yet; use the metadata for audits or explicit low-level
/// patching only.
typedef enum DTTR_PCDOGS_T_Hook_Kind {
	DTTR_PCDOGS_HOOK_UNSUPPORTED = 0, ///< No hook metadata is available.
	DTTR_PCDOGS_HOOK_REL32 = 1,       ///< Entry can be detoured with a trampoline `E9 <rel32>` hook.
	DTTR_PCDOGS_HOOK_HOTPATCH = 2,    ///< Hotpatch layout: pre-entry slot plus entry-window metadata.
} DTTR_PCDOGS_T_Hook_Kind;

typedef enum DTTR_PCDOGS_T_Data_Resolver {
	DTTR_PCDOGS_DATA_RESOLVE_XREF_U32 = 1,
} DTTR_PCDOGS_T_Data_Resolver;

/// Data-symbol write policy.
///
/// `Write` enforces this policy and only writes `RAW_MEMORY` symbols. `UnsafeWrite`
/// bypasses the policy check but still requires writable process memory. Reserve
/// `UnsafeWrite` for explicit patching, reverse-engineering work, or SDK internals.
/// `READ_ONLY` marks decoded dispatch/jump/lookup/opcode/index tables. `ENGINE_MANAGED`
/// marks live pointers or state that the game may replace or overwrite. `PATCH_ONLY`
/// is for symbols that should be changed through patch specs or patch groups.
typedef enum DTTR_PCDOGS_T_Write_Policy {
	DTTR_PCDOGS_WRITE_POLICY_UNKNOWN = 0,     ///< Untyped or insufficiently classified symbol.
	DTTR_PCDOGS_WRITE_POLICY_READ_ONLY = 1,   ///< Decoded table data for read-only inspection through `Read`.
	DTTR_PCDOGS_WRITE_POLICY_ENGINE_MANAGED = 2, ///< Live game-managed pointer/state; use higher-level helpers or patch flows.
	DTTR_PCDOGS_WRITE_POLICY_RAW_MEMORY = 3,  ///< Plain data slot writable through `Write`.
	DTTR_PCDOGS_WRITE_POLICY_PATCH_ONLY = 4,  ///< Change through patch specs or patch groups.
} DTTR_PCDOGS_T_Write_Policy;

/// Descriptor/accessor helpers return `DTTR_Result`.
///
/// PCDOGS-specific failures use `DTTR_ERR_*` values for missing symbols,
/// unresolved addresses, unsafe calls or writes, policy mismatches, unsupported
/// contracts, and ABI/layout drift.

typedef enum DTTR_PCDOGS_T_Calling_Convention {
	DTTR_PCDOGS_CC_CDECL = 0,
	DTTR_PCDOGS_CC_STDCALL = 1,
	DTTR_PCDOGS_CC_FASTCALL = 2,
} DTTR_PCDOGS_T_Calling_Convention;

typedef uint32_t DTTR_PCDOGS_T_Build_Mask;

typedef enum DTTR_PCDOGS_T_Build_Mask_Value {
	DTTR_PCDOGS_BUILD_MASK_NONE = 0,
	DTTR_PCDOGS_BUILD_MASK_EN = 1u << 0,
	DTTR_PCDOGS_BUILD_MASK_EU = 1u << 1,
	DTTR_PCDOGS_BUILD_MASK_SC = 1u << 2,
	DTTR_PCDOGS_BUILD_MASK_EU_SC = DTTR_PCDOGS_BUILD_MASK_EU | DTTR_PCDOGS_BUILD_MASK_SC,
	DTTR_PCDOGS_BUILD_MASK_EN_EU = DTTR_PCDOGS_BUILD_MASK_EN | DTTR_PCDOGS_BUILD_MASK_EU,
	DTTR_PCDOGS_BUILD_MASK_EN_SC = DTTR_PCDOGS_BUILD_MASK_EN | DTTR_PCDOGS_BUILD_MASK_SC,
	DTTR_PCDOGS_BUILD_MASK_ALL = DTTR_PCDOGS_BUILD_MASK_EN | DTTR_PCDOGS_BUILD_MASK_EU | DTTR_PCDOGS_BUILD_MASK_SC,
} DTTR_PCDOGS_T_Build_Mask_Value;

typedef struct DTTR_PCDOGS_T_Symbol_Function {
	DTTR_PCDOGS_T_Calling_Convention calling_convention;
	const char* sig;
	const char* mask;
	int32_t match_offset;
	DTTR_PCDOGS_T_Hook_Kind hook_kind;
	uint32_t patch_size;
	uint32_t entry_patch_size;
	bool callable;
	DTTR_PCDOGS_T_Build_Mask supported_builds;
	uintptr_t address;
	bool resolved;
	DTTR_Core_Hook* hook;
	DTTR_Core_Hook* entry_hook;
	uint8_t* trampoline;
} DTTR_PCDOGS_T_Symbol_Function;

typedef struct DTTR_PCDOGS_T_Symbol_Data {
	uintptr_t address;
	bool resolved;
	DTTR_PCDOGS_T_Write_Policy write_policy;
	DTTR_PCDOGS_T_Build_Mask supported_builds;
} DTTR_PCDOGS_T_Symbol_Data;

typedef struct DTTR_PCDOGS_T_Symbol_Function_XRef {
	int32_t function_index;
	int32_t ref_function_index;
	uint32_t instr_off;
	uint32_t addr_off;
	uint32_t indirections;
} DTTR_PCDOGS_T_Symbol_Function_XRef;

typedef struct DTTR_PCDOGS_T_Symbol_XRef {
	int32_t data_index;
	int32_t function_index;
	uint32_t instr_off;
	uint32_t addr_off;
} DTTR_PCDOGS_T_Symbol_XRef;

enum {
	DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE = ${len(functions)}
};

enum {
	DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE = ${len(globals)}
};

enum {
	DTTR_PCDOGS_FUNCTION_COUNT_VALUE = ${len(public_functions)}
};

enum {
	DTTR_PCDOGS_DATA_COUNT_VALUE = ${len(globals)}
};

enum {
	DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE = ${len(function_xrefs)}
};

enum {
	DTTR_PCDOGS_SYMBOL_XREF_COUNT_VALUE = ${len(xrefs)}
};

#define DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_DATA_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE)
#define DTTR_PCDOGS_FUNCTION_COUNT ((uint32_t)DTTR_PCDOGS_FUNCTION_COUNT_VALUE)
#define DTTR_PCDOGS_DATA_COUNT ((uint32_t)DTTR_PCDOGS_DATA_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_STORAGE_COUNT \
	(DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE ? DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE : 1u)
#define DTTR_PCDOGS_SYMBOL_XREF_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_XREF_COUNT_VALUE)

/// Function symbol identifiers.
typedef enum DTTR_PCDOGS_T_Symbol_Function_ID {
% for row in public_functions:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
	DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row.symbol_id} = ${row.index}, ///< ${doxy_inline(symbol_doc(DOC_KIND.FUNCTION, row))}
% if row.unstable:
#endif
% endif
% endfor
} DTTR_PCDOGS_T_Symbol_Function_ID;

/// Data symbol identifiers.
typedef enum DTTR_PCDOGS_T_Symbol_Data_ID {
% if not globals:
	DTTR_PCDOGS_SYMBOL_DATA_ID_NONE = -1,
% endif
% for row in globals:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
	DTTR_PCDOGS_SYMBOL_DATA_ID_${row.symbol_id}, ///< ${doxy_inline(symbol_doc(DOC_KIND.GLOBAL, row))}
% if row.unstable:
#endif
% endif
% endfor
} DTTR_PCDOGS_T_Symbol_Data_ID;

/// Function identifiers for the public SDK facade.
typedef enum DTTR_PCDOGS_T_Function_ID {
% for row in public_functions:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
	DTTR_PCDOGS_FUNCTION_${row.symbol_id} = ${row.public_index}, ///< ${doxy_inline(symbol_doc(DOC_KIND.FUNCTION, row))}
% if row.unstable:
#endif
% endif
% endfor
} DTTR_PCDOGS_T_Function_ID;

/// Stable global identifiers for the public SDK facade.
typedef enum DTTR_PCDOGS_T_Data_ID {
% if not globals:
	DTTR_PCDOGS_DATA_NONE = -1,
% endif
% for row in globals:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
	DTTR_PCDOGS_DATA_${row.symbol_id} = ${row.public_index}, ///< ${doxy_inline(symbol_doc(DOC_KIND.GLOBAL, row))}
% if row.unstable:
#endif
% endif
% endfor
} DTTR_PCDOGS_T_Data_ID;

typedef enum DTTR_PCDOGS_T_Patch_Kind {
	DTTR_PCDOGS_PATCH_UNSUPPORTED = 0,
	DTTR_PCDOGS_PATCH_FUNCTION_HOOK = 1,
	DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK = 2,
	DTTR_PCDOGS_PATCH_TARGET = 3,
	DTTR_PCDOGS_PATCH_ADDRESS_BYTES = 4,
	DTTR_PCDOGS_PATCH_AOB_BYTES = 5,
	DTTR_PCDOGS_PATCH_AOB_REL32_JMP = 6,
} DTTR_PCDOGS_T_Patch_Kind;

typedef struct DTTR_PCDOGS_T_Patch_Spec {
	DTTR_PCDOGS_T_Patch_Kind kind;
	bool required;
	DTTR_PCDOGS_T_Function_ID function;
	DTTR_PCDOGS_T_Data_ID global;
	DTTR_Core_TargetSpec target;
	uintptr_t address;
	const char* aob;
	intptr_t offset;
	const uint8_t* patch_bytes;
	size_t patch_size;
	void* detour;
	void* new_value;
	void** out_original;
} DTTR_PCDOGS_T_Patch_Spec;

typedef struct DTTR_PCDOGS_T_Patch_Report {
	size_t attempted;
	size_t installed;
	size_t skipped_optional;
	size_t failed_index;
	DTTR_Status status;
	const char* message;
} DTTR_PCDOGS_T_Patch_Report;

#define DTTR_PCDOGS_PATCH_SPEC_TARGET(required_, target_) \
	{                                                      \
		.kind = DTTR_PCDOGS_PATCH_TARGET,                \
		.required = (required_),                         \
		.target = (target_),                             \
	}

#define DTTR_PCDOGS_PATCH_SPEC_ADDRESS_BYTES(required_, address_, ...) \
	{                                                                  \
		.kind = DTTR_PCDOGS_PATCH_ADDRESS_BYTES,                     \
		.required = (required_),                                     \
		.address = (address_),                                       \
		.patch_bytes = (const uint8_t[]){__VA_ARGS__},               \
		.patch_size = sizeof((const uint8_t[]){__VA_ARGS__}),        \
	}

#define DTTR_PCDOGS_PATCH_SPEC_AOB_BYTES(required_, aob_, offset_, ...) \
	{                                                                   \
		.kind = DTTR_PCDOGS_PATCH_AOB_BYTES,                          \
		.required = (required_),                                      \
		.aob = (aob_),                                                \
		.offset = (offset_),                                          \
		.patch_bytes = (const uint8_t[]){__VA_ARGS__},                \
		.patch_size = sizeof((const uint8_t[]){__VA_ARGS__}),         \
	}

#define DTTR_PCDOGS_PATCH_SPEC_AOB_REL32_JMP(required_, aob_, offset_, detour_) \
	{                                                                           \
		.kind = DTTR_PCDOGS_PATCH_AOB_REL32_JMP,                              \
		.required = (required_),                                              \
		.aob = (aob_),                                                        \
		.offset = (offset_),                                                  \
		.detour = (detour_),                                                  \
	}

#define DTTR_PCDOGS_INSTALL_PATCHES(ctx_, specs_, out_group_, out_report_) \
	DTTR_PCDOGS_PatchGroup_Install(                                             \
		(ctx_),                                                        \
		(specs_),                                                      \
		DTTR_ARRAY_COUNT(specs_),                                      \
		(out_group_),                                                  \
		(out_report_)                                                  \
	)

#ifdef __cplusplus
extern "C" {
#endif

bool DTTR_PCDOGS_FunctionSymbolID(
	DTTR_PCDOGS_T_Function_ID id,
	DTTR_PCDOGS_T_Symbol_Function_ID* out_symbol_id
);
bool DTTR_PCDOGS_DataSymbolID(
	DTTR_PCDOGS_T_Data_ID id,
	DTTR_PCDOGS_T_Symbol_Data_ID* out_symbol_id
);
DTTR_Result DTTR_PCDOGS_FunctionResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Function_ID id,
	uintptr_t* out_addr
);
DTTR_Result DTTR_PCDOGS_SymbolFunctionResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Symbol_Function_ID id,
	uintptr_t* out_addr
);
DTTR_Result DTTR_PCDOGS_DataResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Data_ID id,
	uintptr_t* out_addr
);
DTTR_Result DTTR_PCDOGS_SymbolDataResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Symbol_Data_ID id,
	uintptr_t* out_addr
);
DTTR_Result DTTR_PCDOGS_PatchGroup_HookFunction(
	DTTR_Core_PatchGroup* group,
	DTTR_PCDOGS_T_Function_ID id,
	void* detour,
	void** out_original
);
DTTR_Result DTTR_PCDOGS_Hook_DataPointer(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Data_ID id,
	void* new_value,
	void** out_original,
	DTTR_Core_Hook** out_hook
);
DTTR_Result DTTR_PCDOGS_PatchGroup_HookDataPointer(
	DTTR_Core_PatchGroup* group,
	DTTR_PCDOGS_T_Data_ID id,
	void* new_value,
	void** out_original
);
DTTR_Result DTTR_PCDOGS_PatchGroup_Install(
	const DTTR_Core_Context* ctx,
	const DTTR_PCDOGS_T_Patch_Spec* specs,
	size_t spec_count,
	DTTR_Core_PatchGroup** out_group,
	DTTR_PCDOGS_T_Patch_Report* out_report
);

#ifdef __cplusplus
}
#endif

#ifdef DTTR_PCDOGS_IMPLEMENTATION

% if hidden_functions:
// Internal implementation-only function identifiers.
enum {
% for row in hidden_functions:
	DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row.symbol_id} = ${row.index},
% endfor
};

% endif
// PCDOGS signature rows.
#ifndef DTTR_PCDOGS_SIGNATURE_ROWS_DEF
#define DTTR_PCDOGS_SIGNATURE_ROWS_DEF

% if signature_entries:
#define DTTR_PCDOGS_SIGNATURE_ROWS(SIGNATURE) \
% for row in signature_entries:
	SIGNATURE(${row.name}, ${row.sig}, ${row.mask}, ${row.required})${' \\' if loop.index != len(signature_entries) - 1 else ''}
% endfor
% else:
#define DTTR_PCDOGS_SIGNATURE_ROWS(SIGNATURE)
% endif

#endif  // DTTR_PCDOGS_SIGNATURE_ROWS_DEF

// DTTR_PCDOGS_TYPED_FUNCTIONS:START
#ifndef DTTR_PCDOGS_TYPED_FUNCTION_ROWS_DEF
#define DTTR_PCDOGS_TYPED_FUNCTION_ROWS_DEF

#define DTTR_PCDOGS_TYPED_FUNCTION_ROWS(FN) \
% for row_i, row in enumerate(typed_function_rows):
	FN( \
% for value_i, value in enumerate(row.macro_values()):
		${value}${',' if value_i != len(row.macro_values()) - 1 else ''} \
% endfor
	)${' \\' if row_i != len(typed_function_rows) - 1 else ''}
% endfor

#endif  // DTTR_PCDOGS_TYPED_FUNCTION_ROWS_DEF
// DTTR_PCDOGS_TYPED_FUNCTIONS:END

// PCDOGS function symbols.

#ifndef DTTR_PCDOGS_FUNCTION_ROWS_DEF
#define DTTR_PCDOGS_FUNCTION_ROWS_DEF

% if functions:
#define DTTR_PCDOGS_SYMBOL_FUNCTION_ROWS(FN) \
% for row in functions:
	FN(${row.symbol_id}, ${CC_ENUM[str(row.calling_convention)]}, ${HOOK_ENUM[str(row.hook.kind)]}, ${c_bool(row.callable)}, ${c_build_mask(row.supported_builds)}, ${c_sig(row.pattern)}, ${c_mask(row.pattern)}, ${c_int(row.match_offset)}, ${c_uint(row.hook.patch_size)}, ${c_uint(row.hook.entry_patch_size)})${' \\' if loop.index != len(functions) - 1 else ''}
% endfor
% else:
#define DTTR_PCDOGS_SYMBOL_FUNCTION_ROWS(FN)
% endif

#endif  // DTTR_PCDOGS_FUNCTION_ROWS_DEF

// PCDOGS global symbols.

#ifndef DTTR_PCDOGS_DATA_ROWS_DEF
#define DTTR_PCDOGS_DATA_ROWS_DEF

% if globals:
#define DTTR_PCDOGS_DATA_ROWS(TYPED_DATA, UNTYPED_DATA, CTX) \
% for row in globals:
% if row.typed:
		TYPED_DATA(CTX, ${row.symbol_id.lower()}, ${c_public_token(row.name)}, ${c_type(row.typed.type)}, DTTR_PCDOGS_DATA_${row.symbol_id}, ${DATA_RESOLVER[str(row.typed.resolver)]}, ${row.typed.ref_function.lower()}, ${c_uint(row.typed.instr_off)}, ${c_uint(row.typed.addr_off)}, ${c_uint(row.typed.indirections)})${' \\' if loop.index != len(globals) - 1 else ''}
% else:
	UNTYPED_DATA(CTX, ${row.symbol_id.lower()})${' \\' if loop.index != len(globals) - 1 else ''}
% endif
% endfor
% else:
#define DTTR_PCDOGS_DATA_ROWS(TYPED_DATA, UNTYPED_DATA, CTX)
% endif

% if globals:
#define DTTR_PCDOGS_SYMBOL_DATA_ROWS(DATA) \
% for row in globals:
	DATA(${row.symbol_id})${' \\' if loop.index != len(globals) - 1 else ''}
% endfor
% else:
#define DTTR_PCDOGS_SYMBOL_DATA_ROWS(DATA)
% endif

#define DTTR_PCDOGS_TYPED_DATA_AS_TYPED(DATA, name, public, type, data_id, resolver, ref_fn, instr_off, addr_off, indirections) \
	DATA(name, public, type, data_id, resolver, ref_fn, instr_off, addr_off, indirections)
#define DTTR_PCDOGS_SKIP_UNTYPED_DATA(DATA, name)
#define DTTR_PCDOGS_TYPED_DATA_ROWS(DATA) \
	DTTR_PCDOGS_DATA_ROWS(DTTR_PCDOGS_TYPED_DATA_AS_TYPED, DTTR_PCDOGS_SKIP_UNTYPED_DATA, DATA)

#endif  // DTTR_PCDOGS_DATA_ROWS_DEF

// PCDOGS function XRefs.

#ifndef DTTR_PCDOGS_FUNCTION_XREF_ROWS_DEF
#define DTTR_PCDOGS_FUNCTION_XREF_ROWS_DEF

% if function_xrefs:
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROWS(FN_XREF) \
% for row in function_xrefs:
	FN_XREF(${loop.index}, ${row.function_symbol}, ${row.ref_function_symbol}, ${c_uint(row.instr_off)}, ${c_uint(row.addr_off)}, ${c_uint(row.indirections)})${' \\' if loop.index != len(function_xrefs) - 1 else ''}
% endfor
% else:
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROWS(FN_XREF)
% endif

#endif  // DTTR_PCDOGS_FUNCTION_XREF_ROWS_DEF

// PCDOGS symbol XRefs.

#ifndef DTTR_PCDOGS_XREF_ROWS_DEF
#define DTTR_PCDOGS_XREF_ROWS_DEF

% if xrefs:
#define DTTR_PCDOGS_SYMBOL_XREF_ROWS(XREF) \
% for row in xrefs:
	XREF(${loop.index}, ${row.global_symbol}, ${row.function_symbol}, ${c_uint(row.instr_off)}, ${c_uint(row.addr_off)})${' \\' if loop.index != len(xrefs) - 1 else ''}
% endfor
% else:
#define DTTR_PCDOGS_SYMBOL_XREF_ROWS(XREF)
% endif

#endif  // DTTR_PCDOGS_XREF_ROWS_DEF

#define DTTR_PCDOGS_SYMBOL_FUNCTION(                                                     \
	cc,                                                                                  \
	sig,                                                                                 \
	mask,                                                                                \
	match_offset,                                                                        \
	hook_kind,                                                                           \
	patch_size,                                                                          \
	entry_patch_size,                                                                    \
	callable,                                                                            \
	supported_builds                                                                     \
)                                                                                        \
	{cc, sig, mask, match_offset, hook_kind, patch_size, entry_patch_size, callable,       \
	 supported_builds, 0, false, NULL, NULL, NULL}

#define DTTR_PCDOGS_SYMBOL_DATA(policy, supported_builds) \
	{0, false, policy, supported_builds}

static DTTR_PCDOGS_T_Symbol_Function dttr_pcdogs_symbol_functions[DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE] = {
#define DTTR_PCDOGS_SYMBOL_ARRAY_FN(id, cc, hook_kind, callable, supported_builds, sig, mask, match_offset, patch_size, entry_patch_size) \
	[DTTR_PCDOGS_SYMBOL_FUNCTION_ID_##id] =                                                \
		DTTR_PCDOGS_SYMBOL_FUNCTION(                                                        \
			cc, sig, mask, match_offset, hook_kind, patch_size, entry_patch_size, callable, supported_builds \
		),
	DTTR_PCDOGS_SYMBOL_FUNCTION_ROWS(DTTR_PCDOGS_SYMBOL_ARRAY_FN)
#undef DTTR_PCDOGS_SYMBOL_ARRAY_FN
};

static const char* const dttr_pcdogs_symbol_function_names[DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE] = {
% for row in functions:
	[DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row.symbol_id}] = "${c_public_token(row.name)}",
% endfor
};

static DTTR_PCDOGS_T_Symbol_Data dttr_pcdogs_symbol_globals[DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE] = {
% for row in globals:
	[DTTR_PCDOGS_SYMBOL_DATA_ID_${row.symbol_id}] = DTTR_PCDOGS_SYMBOL_DATA(${data_write_policy(row)}, ${c_build_mask(row.supported_builds)}),
% endfor
};

static const DTTR_PCDOGS_T_Symbol_Function_ID dttr_pcdogs_public_function_symbol_ids[DTTR_PCDOGS_FUNCTION_COUNT_VALUE] = {
% for row in public_functions:
	[DTTR_PCDOGS_FUNCTION_${row.symbol_id}] = DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row.symbol_id},
% endfor
};

static const DTTR_PCDOGS_T_Symbol_Data_ID dttr_pcdogs_public_data_symbol_ids[DTTR_PCDOGS_DATA_COUNT_VALUE] = {
% for row in globals:
	[DTTR_PCDOGS_DATA_${row.symbol_id}] = DTTR_PCDOGS_SYMBOL_DATA_ID_${row.symbol_id},
% endfor
};

static const DTTR_PCDOGS_T_Symbol_Function_XRef dttr_pcdogs_symbol_function_xrefs
	[DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_STORAGE_COUNT] = {
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROW(                                           \
	index, function, ref_function, instr_off, addr_off, indirections                    \
)                                                                                        \
	[index] = {DTTR_PCDOGS_SYMBOL_FUNCTION_ID_##function,                                \
		DTTR_PCDOGS_SYMBOL_FUNCTION_ID_##ref_function, instr_off, addr_off, indirections},
	DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROWS(DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROW)
#undef DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_ROW
};

static const DTTR_PCDOGS_T_Symbol_XRef dttr_pcdogs_symbol_xrefs[DTTR_PCDOGS_SYMBOL_XREF_COUNT_VALUE] = {
#define DTTR_PCDOGS_SYMBOL_XREF_ROW(index, global, function, instr_off, addr_off) \
	[index] = {DTTR_PCDOGS_SYMBOL_DATA_ID_##global, DTTR_PCDOGS_SYMBOL_FUNCTION_ID_##function, instr_off, addr_off},
	DTTR_PCDOGS_SYMBOL_XREF_ROWS(DTTR_PCDOGS_SYMBOL_XREF_ROW)
#undef DTTR_PCDOGS_SYMBOL_XREF_ROW
};

#endif  // DTTR_PCDOGS_IMPLEMENTATION

#ifndef DTTR_PCDOGS_CORE_HOOK_HELPERS_DEFINED
#define DTTR_PCDOGS_CORE_HOOK_HELPERS_DEFINED
static bool dttr_pcdogs_core_api_field_available(
	const DTTR_Core_API* api,
	size_t field_offset,
	size_t field_size
) {
	return api && api->struct_size >= field_offset + field_size;
}

static bool dttr_pcdogs_hook_is_active(
	const DTTR_Core_Context* ctx,
	DTTR_Core_Hook* hook
) {
	if (!hook) {
		return false;
	}
	if (ctx && dttr_pcdogs_core_api_field_available(
				   ctx->api,
				   offsetof(DTTR_Core_API, hook_is_active),
				   sizeof(ctx->api->hook_is_active)
			   )
		&& ctx->api->hook_is_active) {
		return ctx->api->hook_is_active(hook);
	}
	return DTTR_Core_HookIsActive(hook);
}

static bool dttr_pcdogs_unhook_checked(
	const DTTR_Core_Context* ctx,
	DTTR_Core_Hook* hook
) {
	if (!ctx || !ctx->api || !hook) {
		return true;
	}
	if (dttr_pcdogs_core_api_field_available(
			ctx->api,
			offsetof(DTTR_Core_API, unhook_checked),
			sizeof(ctx->api->unhook_checked)
		)
		&& ctx->api->unhook_checked) {
		return ctx->api->unhook_checked(hook);
	}
	if (ctx->api->unhook) {
		ctx->api->unhook(hook);
	}
	return !dttr_pcdogs_hook_is_active(ctx, hook);
}
#endif  // DTTR_PCDOGS_CORE_HOOK_HELPERS_DEFINED

% for row in typed_function_rows:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif

${doxy_comment(row.doc, params=row.param_docs)}
typedef ${row.ret}(${row.cc}*${row.typedef_name}) ${row.params};

/// Accessor table for the `${row.display_name}` symbol object.
struct ${row.accessor_struct_name} {
	/// Stable SDK symbol ID.
	DTTR_PCDOGS_T_Symbol_Function_ID SymbolID;
	/// Public function ID.
	DTTR_PCDOGS_T_Function_ID FunctionID;
	/// Returns true after this symbol has resolved in the active process.
	bool (*IsResolved)();
	/// Returns this function descriptor's status in the active process.
	DTTR_Result (*Status)(const DTTR_Core_Context* ctx);
	/// Returns true when this symbol is resolved and safe to call.
	bool (*IsCallable)(const DTTR_Core_Context* ctx);
	/// Returns the resolved function address, or zero when unresolved.
	uintptr_t (*Address)();
	/// Returns the hook-site shape.
	DTTR_PCDOGS_T_Hook_Kind (*HookKind)();
	/// Returns the REL32 trampoline size or HOTPATCH entry-window size.
	uint32_t (*HookPrologueSize)();
	/// Calls the resolved function; value-returning wrappers write through `out_ret`.
	DTTR_Result (*Call) ${row.try_params};
	/// Installs REL32 hooks; HOTPATCH metadata returns false.
	bool (*Hook)(
		const DTTR_Core_Context* ctx,
		${row.typedef_name} detour,
		${row.typedef_name}* out_original
	);
	/// Builds a REL32 function-hook spec; HOTPATCH metadata returns an unsupported spec.
	DTTR_PCDOGS_T_Patch_Spec (*PatchSpec)(
		bool required,
		${row.typedef_name} detour,
		${row.typedef_name}* out_original
	);
	/// Detaches the hook installed through this accessor, if any.
	void (*Unhook)(const DTTR_Core_Context* ctx);
};

/// Accessor object for `${row.display_name}`.
DTTR_PCDOGS_API const struct ${row.accessor_struct_name}* const DTTR_PCDOGS_F_${row.public};
% if row.unstable:
#endif
% endif

% endfor
% for row in globals:
% if row.typed:
% if row.unstable:
#if defined(DTTR_SDK_ENABLE_UNSTABLE) || defined(DTTR_PCDOGS_IMPLEMENTATION)
% endif
% if is_c_array_type(row.typed.type):
#define DTTR_PCDOGS_D_${row.symbol_id}_COUNT ${c_array_type_count(row.typed.type)}

% endif
${doxy_comment(symbol_doc(DOC_KIND.GLOBAL, row), returns="Typed data symbol object.")}
struct DTTR_PCDOGS_D_${c_public_token(row.name)}_type {
	DTTR_PCDOGS_T_Symbol_Data_ID SymbolID;
	DTTR_PCDOGS_T_Data_ID DataID;
	DTTR_PCDOGS_T_Write_Policy (*Policy)();
	DTTR_Result (*Status)();
	bool (*IsResolved)();
	uintptr_t (*Address)();
	${c_data_ptr_decl(row.typed.type, '(*Ptr)()')};
	DTTR_Result (*Read)(${c_data_read_param(row.typed.type, 'out_value')});
	/// Writer that enforces the symbol write policy. Returns `DTTR_ERR_POLICY_MISMATCH` unless Policy() is RAW_MEMORY.
	DTTR_Result (*Write)(${c_data_write_param(row.typed.type, 'value')});
	/// Bypasses Policy(); still requires resolved, writable memory.
	DTTR_Result (*UnsafeWrite)(${c_data_write_param(row.typed.type, 'value')});
	// Builds a pointer-hook spec for pointer data; scalar data returns unsupported.
	DTTR_PCDOGS_T_Patch_Spec (*PatchSpec)(
		bool required,
		void* new_value,
		void** out_original
	);
};

DTTR_PCDOGS_API const struct DTTR_PCDOGS_D_${c_public_token(row.name)}_type* const DTTR_PCDOGS_D_${c_public_token(row.name)};
% if row.unstable:
#endif
% endif

% endif
% endfor

DTTR_PCDOGS_API uint32_t DTTR_PCDOGS_FunctionCount();
DTTR_PCDOGS_API uint32_t DTTR_PCDOGS_DataCount();
DTTR_PCDOGS_API bool DTTR_PCDOGS_ResolveAll(const DTTR_Core_Context* ctx);
DTTR_PCDOGS_API void DTTR_PCDOGS_Unhook_All(const DTTR_Core_Context* ctx);
DTTR_PCDOGS_API void DTTR_PCDOGS_Reset();

DTTR_PCDOGS_API uint32_t DTTR_PCDOGS_SymbolFunctionCount();
DTTR_PCDOGS_API uint32_t DTTR_PCDOGS_SymbolDataCount();
DTTR_PCDOGS_API const DTTR_PCDOGS_T_Symbol_Function* DTTR_PCDOGS_SymbolFunctionAt(
	uint32_t index
);
DTTR_PCDOGS_API const char* DTTR_PCDOGS_SymbolFunctionNameAt(uint32_t index);
DTTR_PCDOGS_API const DTTR_PCDOGS_T_Symbol_Data* DTTR_PCDOGS_SymbolDataAt(
	uint32_t index
);
DTTR_PCDOGS_API bool DTTR_PCDOGS_SymbolsResolveAll(const DTTR_Core_Context* ctx);
DTTR_PCDOGS_API bool DTTR_PCDOGS_SymbolFunctionIsCallable(
	const DTTR_Core_Context* ctx,
	const DTTR_PCDOGS_T_Symbol_Function* fn
);
#ifdef DTTR_PCDOGS_IMPLEMENTATION

#define DTTR_PCDOGS_COUNT_ONE(...) +1u

#define DTTR_PCDOGS_SIGNATURE(NAME, SIG, MASK, REQUIRED)                                 \
	static const char dttr_pcdogs_sig_##NAME[] = SIG;                                  \
	static const char dttr_pcdogs_mask_##NAME[] = MASK;

DTTR_PCDOGS_SIGNATURE_ROWS(DTTR_PCDOGS_SIGNATURE)

#define DTTR_PCDOGS_SIG(NAME) dttr_pcdogs_sig_##NAME
#define DTTR_PCDOGS_MASK(NAME) dttr_pcdogs_mask_##NAME

#define DTTR_PCDOGS_DEFINE_FUNCTION_STORAGE(                                             \
	name,                                                                                \
	public,                                                                              \
	cc,                                                                                  \
	ret,                                                                                 \
	return_kind,                                                                         \
	params,                                                                              \
	args,                                                                                \
	try_params,                                                                          \
	try_args,                                                                            \
	signature,                                                                           \
	delta,                                                                               \
	hook_kind,                                                                           \
	hook_prologue_size,                                                                  \
	callable                                                                             \
)                                                                                        \
	static uintptr_t dttr_pcdogs_##name##_addr;                                        \
	static DTTR_Core_Hook* dttr_pcdogs_##name##_hook;

#define DTTR_PCDOGS_DEFINE_DATA_STORAGE(                                               \
	name,                                                                                \
	public,                                                                              \
	type,                                                                                \
	data_id,                                                                             \
	resolver,                                                                            \
	ref_fn,                                                                              \
	instr_off,                                                                           \
	addr_off,                                                                            \
	indirections                                                                         \
)                                                                                        \
	static uintptr_t dttr_pcdogs_##name##_addr;

DTTR_PCDOGS_TYPED_FUNCTION_ROWS(DTTR_PCDOGS_DEFINE_FUNCTION_STORAGE)
DTTR_PCDOGS_TYPED_DATA_ROWS(DTTR_PCDOGS_DEFINE_DATA_STORAGE)

static bool dttr_pcdogs_page_allows_read(DWORD protect) {
	switch (protect & 0xFFu) {
	case PAGE_READONLY:
	case PAGE_READWRITE:
	case PAGE_WRITECOPY:
	case PAGE_EXECUTE:
	case PAGE_EXECUTE_READ:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
		return true;
	default:
		return false;
	}
}

static bool dttr_pcdogs_page_allows_write(DWORD protect) {
	switch (protect & 0xFFu) {
	case PAGE_READWRITE:
	case PAGE_WRITECOPY:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
		return true;
	default:
		return false;
	}
}

static bool dttr_pcdogs_page_allows_execute(DWORD protect) {
	switch (protect & 0xFFu) {
	case PAGE_EXECUTE:
	case PAGE_EXECUTE_READ:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
		return true;
	default:
		return false;
	}
}

static bool dttr_pcdogs_region_has(uintptr_t addr, size_t size, bool write, bool exec) {
	if (!addr || !size) {
		return false;
	}

	size_t checked = 0;
	while (checked < size) {
		const uintptr_t cur = addr + checked;
		MEMORY_BASIC_INFORMATION mbi;
		if (!VirtualQuery((const void* )cur, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT
			|| (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
			return false;
		}

		if (exec && !dttr_pcdogs_page_allows_execute(mbi.Protect)) {
			return false;
		}
		if (!exec && write && !dttr_pcdogs_page_allows_write(mbi.Protect)) {
			return false;
		}
		if (!exec && !write && !dttr_pcdogs_page_allows_read(mbi.Protect)) {
			return false;
		}

		const uintptr_t end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
		if (end <= cur) {
			return false;
		}
		size_t chunk = (size_t)(end - cur);
		if (chunk > size - checked) {
			chunk = size - checked;
		}
		checked += chunk;
	}
	return true;
}

static bool dttr_pcdogs_module_region_has(
	const DTTR_Core_Context* ctx,
	uintptr_t addr,
	size_t size,
	bool write,
	bool exec
) {
	if (!ctx || !ctx->game_module
		|| !dttr_pcdogs_region_has(addr, size, write, exec)) {
		return false;
	}

	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery((const void*)addr, &mbi, sizeof(mbi))) {
		return false;
	}

	return mbi.AllocationBase == (void*)ctx->game_module;
}

static bool dttr_pcdogs_context_valid(const DTTR_Core_Context* ctx) {
	return ctx && ctx->game_module && ctx->api && ctx->api->sigscan;
}

static bool dttr_pcdogs_function_address_valid(
	const DTTR_Core_Context* ctx,
	uintptr_t addr,
	const char* sig,
	const char* mask,
	int32_t match_to_entry_delta
) {
	if (!dttr_pcdogs_context_valid(ctx) || !addr || !sig || !mask || !mask[0]) {
		return false;
	}
	const uintptr_t match = ctx->api->sigscan(ctx->game_module, sig, mask);
	if (!match || (uintptr_t)((intptr_t)match + match_to_entry_delta) != addr) {
		return false;
	}
	return dttr_pcdogs_region_has(addr, 1u, false, true);
}

static uintptr_t dttr_pcdogs_resolve_function(
	const DTTR_Core_Context* ctx,
	const char* sig,
	const char* mask,
	int32_t match_to_entry_delta
) {
	if (!dttr_pcdogs_context_valid(ctx) || !sig || !mask || !mask[0]) {
		return 0;
	}
	const uintptr_t match = ctx->api->sigscan(ctx->game_module, sig, mask);
	if (!match) {
		return 0;
	}
	const uintptr_t addr = (uintptr_t)((intptr_t)match + match_to_entry_delta);
	return dttr_pcdogs_region_has(addr, 1u, false, true) ? addr : 0;
}

static uintptr_t dttr_pcdogs_resolve_xref_u32(
	uintptr_t function_addr,
	uint32_t instr_off,
	uint32_t addr_off,
	uint32_t indirections
) {
	uintptr_t source = function_addr + (uintptr_t)instr_off + (uintptr_t)addr_off;
	uint32_t value32 = 0;
	if (!dttr_pcdogs_region_has(source, sizeof(value32), false, false)) {
		return 0;
	}
	memcpy(&value32, (const void* )source, sizeof(value32));
	uintptr_t value = (uintptr_t)value32;
	while (value && indirections) {
		uint32_t next32 = 0;
		if (!dttr_pcdogs_region_has(value, sizeof(next32), false, false)) {
			return 0;
		}
		memcpy(&next32, (const void* )value, sizeof(next32));
		value = (uintptr_t)next32;
		indirections--;
	}
	return value;
}

static uintptr_t dttr_pcdogs_resolve_global_address(
	DTTR_PCDOGS_T_Data_Resolver resolver,
	uintptr_t function_addr,
	uint32_t instr_off,
	uint32_t addr_off,
	uint32_t indirections
) {
	switch (resolver) {
	case DTTR_PCDOGS_DATA_RESOLVE_XREF_U32:
		return dttr_pcdogs_resolve_xref_u32(
			function_addr,
			instr_off,
			addr_off,
			indirections
		);
	default:
		return 0;
	}
}

bool DTTR_PCDOGS_FunctionSymbolID(
	DTTR_PCDOGS_T_Function_ID id,
	DTTR_PCDOGS_T_Symbol_Function_ID* out_symbol_id
) {
	if (!out_symbol_id || (int)id < 0 || (uint32_t)id >= DTTR_PCDOGS_FUNCTION_COUNT) {
		return false;
	}
	*out_symbol_id = dttr_pcdogs_public_function_symbol_ids[(uint32_t)id];
	return true;
}

bool DTTR_PCDOGS_DataSymbolID(
	DTTR_PCDOGS_T_Data_ID id,
	DTTR_PCDOGS_T_Symbol_Data_ID* out_symbol_id
) {
	if (!out_symbol_id || (int)id < 0 || (uint32_t)id >= DTTR_PCDOGS_DATA_COUNT) {
		return false;
	}
	*out_symbol_id = dttr_pcdogs_public_data_symbol_ids[(uint32_t)id];
	return true;
}
uint32_t DTTR_PCDOGS_SymbolFunctionCount() {
	return DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT;
}

uint32_t DTTR_PCDOGS_SymbolDataCount() { return DTTR_PCDOGS_SYMBOL_DATA_COUNT; }

const DTTR_PCDOGS_T_Symbol_Function* DTTR_PCDOGS_SymbolFunctionAt(uint32_t index) {
	if (index >= DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT) {
		return NULL;
	}
	return &dttr_pcdogs_symbol_functions[index];
}

const char* DTTR_PCDOGS_SymbolFunctionNameAt(uint32_t index) {
	if (index >= DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT) {
		return NULL;
	}
	return dttr_pcdogs_symbol_function_names[index];
}

const DTTR_PCDOGS_T_Symbol_Data* DTTR_PCDOGS_SymbolDataAt(uint32_t index) {
	if (index >= DTTR_PCDOGS_SYMBOL_DATA_COUNT) {
		return NULL;
	}
	return &dttr_pcdogs_symbol_globals[index];
}

bool DTTR_PCDOGS_SymbolFunctionIsCallable(
	const DTTR_Core_Context* ctx,
	const DTTR_PCDOGS_T_Symbol_Function* fn
) {
	return fn && fn->callable
		   && dttr_pcdogs_function_address_valid(
			   ctx,
			   fn->address,
			   fn->sig,
			   fn->mask,
			   fn->match_offset
		   );
}

bool DTTR_PCDOGS_SymbolsResolveAll(const DTTR_Core_Context* ctx) {
	if (!dttr_pcdogs_context_valid(ctx)) {
		return false;
	}

	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT; i++) {
		DTTR_PCDOGS_T_Symbol_Function* fn = &dttr_pcdogs_symbol_functions[i];
		fn->address = dttr_pcdogs_resolve_function(
			ctx,
			fn->sig,
			fn->mask,
			fn->match_offset
		);
		fn->resolved = fn->address != 0;
	}

% if function_xrefs:
	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT; i++) {
		const DTTR_PCDOGS_T_Symbol_Function_XRef* xref = &dttr_pcdogs_symbol_function_xrefs[i];
		if (xref->function_index < 0 || xref->ref_function_index < 0
			|| (uint32_t)xref->function_index >= DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT
			|| (uint32_t)xref->ref_function_index >= DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT) {
			continue;
		}
		DTTR_PCDOGS_T_Symbol_Function* fn =
			&dttr_pcdogs_symbol_functions[xref->function_index];
		const DTTR_PCDOGS_T_Symbol_Function* ref_fn =
			&dttr_pcdogs_symbol_functions[xref->ref_function_index];
		if (fn->resolved || !ref_fn->resolved) {
			continue;
		}
		uintptr_t value = dttr_pcdogs_resolve_xref_u32(
			ref_fn->address,
			xref->instr_off,
			xref->addr_off,
			xref->indirections
		);
		if (value && dttr_pcdogs_region_has(value, 1u, false, true)) {
			fn->address = value;
			fn->resolved = true;
		}
	}
% endif

	bool all_ok = true;
	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT; i++) {
		if (!dttr_pcdogs_symbol_functions[i].resolved) {
			all_ok = false;
		}
	}

	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_DATA_COUNT; i++) {
		dttr_pcdogs_symbol_globals[i].address = 0;
		dttr_pcdogs_symbol_globals[i].resolved = false;
	}

	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_XREF_COUNT; i++) {
		const DTTR_PCDOGS_T_Symbol_XRef* xref = &dttr_pcdogs_symbol_xrefs[i];
		if (xref->data_index < 0 || xref->function_index < 0
			|| (uint32_t)xref->data_index >= DTTR_PCDOGS_SYMBOL_DATA_COUNT
			|| (uint32_t)xref->function_index >= DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT) {
			continue;
		}
		const DTTR_PCDOGS_T_Symbol_Function* fn = &dttr_pcdogs_symbol_functions
												  [xref->function_index];
		if (!fn->resolved) {
			continue;
		}
		uintptr_t value = dttr_pcdogs_resolve_xref_u32(
			fn->address,
			xref->instr_off,
			xref->addr_off,
			0u
		);
		if (value && dttr_pcdogs_module_region_has(ctx, value, 1u, false, false)) {
			DTTR_PCDOGS_T_Symbol_Data* global = &dttr_pcdogs_symbol_globals
												  [xref->data_index];
			global->address = value;
			global->resolved = true;
		}
	}

	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_DATA_COUNT; i++) {
		if (!dttr_pcdogs_symbol_globals[i].resolved) {
			all_ok = false;
		}
	}
	return all_ok;
}

% for row in typed_function_rows:
static bool dttr_pcdogs_${row.name}_IsResolved() {
	return dttr_pcdogs_${row.name}_addr != 0;
}

static bool dttr_pcdogs_${row.name}_IsCallable(const DTTR_Core_Context* ctx) {
	return ${row.callable}
		   && dttr_pcdogs_function_address_valid(
			   ctx,
			   dttr_pcdogs_${row.name}_addr,
			   DTTR_PCDOGS_SIG(${row.signature}),
			   DTTR_PCDOGS_MASK(${row.signature}),
			   ${row.delta}
		   );
}

static DTTR_Result dttr_pcdogs_${row.name}_Status(
	const DTTR_Core_Context* ctx
) {
	if (!dttr_pcdogs_context_valid(ctx)) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, NULL};
	}
	if (!${row.callable}) {
		return (DTTR_Result){DTTR_ERR_NOT_CALLABLE, NULL};
	}
	if (!dttr_pcdogs_${row.name}_addr) {
		return (DTTR_Result){DTTR_ERR_UNRESOLVED, NULL};
	}
	if (!dttr_pcdogs_function_address_valid(
			ctx,
			dttr_pcdogs_${row.name}_addr,
			DTTR_PCDOGS_SIG(${row.signature}),
			DTTR_PCDOGS_MASK(${row.signature}),
			${row.delta}
		)) {
		return (DTTR_Result){DTTR_ERR_ABI_MISMATCH, NULL};
	}
	return (DTTR_Result){DTTR_OK, NULL};
}

static uintptr_t dttr_pcdogs_${row.name}_Address() {
	return dttr_pcdogs_${row.name}_addr;
}

static DTTR_PCDOGS_T_Hook_Kind dttr_pcdogs_${row.name}_HookKind() {
	return ${row.hook_kind};
}

static uint32_t dttr_pcdogs_${row.name}_HookPrologueSize() {
	return ${row.hook_prologue_size};
}

static DTTR_Result dttr_pcdogs_${row.name}_Call ${row.try_params} {
% if row.ret != "void":
	if (!out_ret) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, NULL};
	}
% endif
	DTTR_Result call_result = dttr_pcdogs_${row.name}_Status(ctx);
	if (!DTTR_ResultOK(call_result)) {
		return call_result;
	}
% if row.ret == "void":
	((${row.typedef_name})dttr_pcdogs_${row.name}_addr)${row.args};
% else:
	*out_ret = ((${row.typedef_name})dttr_pcdogs_${row.name}_addr)${row.args};
% endif
	return (DTTR_Result){DTTR_OK, NULL};
}


static bool dttr_pcdogs_${row.name}_Hook(
	const DTTR_Core_Context* ctx,
	${row.typedef_name} detour,
	${row.typedef_name}* out_original
) {
	if (dttr_pcdogs_${row.name}_hook
		&& !dttr_pcdogs_hook_is_active(ctx, dttr_pcdogs_${row.name}_hook)) {
		dttr_pcdogs_${row.name}_hook = 0;
	}

	if (!dttr_pcdogs_${row.name}_IsCallable(ctx) || !detour
		|| dttr_pcdogs_${row.name}_hook || !ctx->api->hook_function
		|| ${row.hook_kind} != DTTR_PCDOGS_HOOK_REL32) {
		return false;
	}
	void* original = 0;
	dttr_pcdogs_${row.name}_hook = ctx->api->hook_function(
		dttr_pcdogs_${row.name}_addr,
		(int)${row.hook_prologue_size},
		detour,
		&original
	);
	if (!dttr_pcdogs_${row.name}_hook) {
		return false;
	}
	if (out_original) {
		*out_original = (${row.typedef_name})original;
	}
	return true;
}

static DTTR_PCDOGS_T_Patch_Spec dttr_pcdogs_${row.name}_PatchSpec(
	bool required,
	${row.typedef_name} detour,
	${row.typedef_name}* out_original
) {
	DTTR_PCDOGS_T_Patch_Spec spec_;
	memset(&spec_, 0, sizeof(spec_));
	if (!${row.callable} || ${row.hook_kind} != DTTR_PCDOGS_HOOK_REL32) {
		spec_.required = required;
		return spec_;
	}
	spec_.kind = DTTR_PCDOGS_PATCH_FUNCTION_HOOK;
	spec_.required = required;
	spec_.function = ${row.function_id};
	spec_.detour = (void*)detour;
	spec_.out_original = (void**)out_original;
	return spec_;
}

static void dttr_pcdogs_${row.name}_Unhook(const DTTR_Core_Context* ctx) {
	if (!ctx || !ctx->api || !dttr_pcdogs_${row.name}_hook) {
		return;
	}

	if (dttr_pcdogs_unhook_checked(ctx, dttr_pcdogs_${row.name}_hook)) {
		dttr_pcdogs_${row.name}_hook = 0;
	}
}

static const struct ${row.accessor_struct_name} dttr_pcdogs_${row.name}_symbol = {
	.SymbolID = ${row.symbol_id},
	.FunctionID = ${row.function_id},
	.IsResolved = dttr_pcdogs_${row.name}_IsResolved,
	.Status = dttr_pcdogs_${row.name}_Status,
	.IsCallable = dttr_pcdogs_${row.name}_IsCallable,
	.Address = dttr_pcdogs_${row.name}_Address,
	.HookKind = dttr_pcdogs_${row.name}_HookKind,
	.HookPrologueSize = dttr_pcdogs_${row.name}_HookPrologueSize,
	.Call = dttr_pcdogs_${row.name}_Call,
	.Hook = dttr_pcdogs_${row.name}_Hook,
	.PatchSpec = dttr_pcdogs_${row.name}_PatchSpec,
	.Unhook = dttr_pcdogs_${row.name}_Unhook,
};

const struct ${row.accessor_struct_name}* const DTTR_PCDOGS_F_${row.public} =
	&dttr_pcdogs_${row.name}_symbol;

% endfor
% for row in globals:
% if row.typed:
static bool dttr_pcdogs_${row.symbol_id.lower()}_IsResolved() {
	return dttr_pcdogs_${row.symbol_id.lower()}_addr != 0;
}

static DTTR_PCDOGS_T_Write_Policy dttr_pcdogs_${row.symbol_id.lower()}_Policy() {
	return ${data_write_policy(row)};
}

static DTTR_Result dttr_pcdogs_${row.symbol_id.lower()}_Status() {
	return dttr_pcdogs_${row.symbol_id.lower()}_addr
		   ? (DTTR_Result){DTTR_OK, NULL}
		   : (DTTR_Result){DTTR_ERR_UNRESOLVED, NULL};
}

static uintptr_t dttr_pcdogs_${row.symbol_id.lower()}_Address() {
	return dttr_pcdogs_${row.symbol_id.lower()}_addr;
}

static ${c_data_ptr_decl(row.typed.type, 'dttr_pcdogs_' + row.symbol_id.lower() + '_Ptr()')} {
	return ${c_data_ptr_cast(row.typed.type)}dttr_pcdogs_${row.symbol_id.lower()}_addr;
}

static DTTR_Result dttr_pcdogs_${row.symbol_id.lower()}_Read(${c_data_read_param(row.typed.type, 'out_value')}) {
	if (!out_value) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, NULL};
	}
	DTTR_Result access_result = dttr_pcdogs_${row.symbol_id.lower()}_Status();
	if (!DTTR_ResultOK(access_result)) {
		return access_result;
	}
	if (!dttr_pcdogs_region_has(
			dttr_pcdogs_${row.symbol_id.lower()}_addr,
			sizeof(${c_type(row.typed.type)}),
			false,
			false
		)) {
		return (DTTR_Result){DTTR_ERR_READ_FAILED, NULL};
	}
	memcpy(out_value, (const void*)dttr_pcdogs_${row.symbol_id.lower()}_addr, sizeof(${c_type(row.typed.type)}));
	return (DTTR_Result){DTTR_OK, NULL};
}


static DTTR_Result dttr_pcdogs_${row.symbol_id.lower()}_UnsafeWrite(${c_data_write_param(row.typed.type, 'value')}) {
	DTTR_Result access_result = dttr_pcdogs_${row.symbol_id.lower()}_Status();
	if (!DTTR_ResultOK(access_result)) {
		return access_result;
	}
% if is_c_array_type(row.typed.type):
	if (!value) {
		return (DTTR_Result){DTTR_ERR_INVALID_ARGUMENT, NULL};
	}
% endif
	if (!dttr_pcdogs_region_has(
			dttr_pcdogs_${row.symbol_id.lower()}_addr,
			sizeof(${c_type(row.typed.type)}),
			true,
			false
		)) {
		return (DTTR_Result){DTTR_ERR_WRITE_FAILED, NULL};
	}
	memcpy((void*)dttr_pcdogs_${row.symbol_id.lower()}_addr, ${c_data_write_source(row.typed.type, 'value')}, sizeof(${c_type(row.typed.type)}));
	return (DTTR_Result){DTTR_OK, NULL};
}


static DTTR_Result dttr_pcdogs_${row.symbol_id.lower()}_Write(${c_data_write_param(row.typed.type, 'value')}) {
	if (dttr_pcdogs_${row.symbol_id.lower()}_Policy() != DTTR_PCDOGS_WRITE_POLICY_RAW_MEMORY) {
		return (DTTR_Result){DTTR_ERR_POLICY_MISMATCH, NULL};
	}
	return dttr_pcdogs_${row.symbol_id.lower()}_UnsafeWrite(value);
}


static DTTR_PCDOGS_T_Patch_Spec dttr_pcdogs_${row.symbol_id.lower()}_PatchSpec(
	bool required,
	void* new_value,
	void** out_original
) {
	DTTR_PCDOGS_T_Patch_Spec spec_;
	memset(&spec_, 0, sizeof(spec_));
	spec_.kind = ${data_patch_spec_kind(row)};
	spec_.required = required;
	spec_.global = DTTR_PCDOGS_DATA_${row.symbol_id};
	spec_.new_value = new_value;
	spec_.out_original = out_original;
	return spec_;
}

static const struct DTTR_PCDOGS_D_${c_public_token(row.name)}_type dttr_pcdogs_${row.symbol_id.lower()}_symbol = {
	.SymbolID = DTTR_PCDOGS_SYMBOL_DATA_ID_${row.symbol_id},
	.DataID = DTTR_PCDOGS_DATA_${row.symbol_id},
	.Policy = dttr_pcdogs_${row.symbol_id.lower()}_Policy,
	.Status = dttr_pcdogs_${row.symbol_id.lower()}_Status,
	.IsResolved = dttr_pcdogs_${row.symbol_id.lower()}_IsResolved,
	.Address = dttr_pcdogs_${row.symbol_id.lower()}_Address,
	.Ptr = dttr_pcdogs_${row.symbol_id.lower()}_Ptr,
	.Read = dttr_pcdogs_${row.symbol_id.lower()}_Read,
	.Write = dttr_pcdogs_${row.symbol_id.lower()}_Write,
	.UnsafeWrite = dttr_pcdogs_${row.symbol_id.lower()}_UnsafeWrite,
	.PatchSpec = dttr_pcdogs_${row.symbol_id.lower()}_PatchSpec,
};

const struct DTTR_PCDOGS_D_${c_public_token(row.name)}_type* const DTTR_PCDOGS_D_${c_public_token(row.name)} =
	&dttr_pcdogs_${row.symbol_id.lower()}_symbol;

% endif
% endfor

uint32_t DTTR_PCDOGS_FunctionCount() {
	return DTTR_PCDOGS_FUNCTION_COUNT;
}
uint32_t DTTR_PCDOGS_DataCount() {
	return DTTR_PCDOGS_DATA_COUNT;
}

bool DTTR_PCDOGS_ResolveAll(const DTTR_Core_Context* ctx) {
	bool all_ok = true;
#define DTTR_PCDOGS_RESOLVE_ONE(                                                         \
	name,                                                                                \
	public,                                                                              \
	cc,                                                                                  \
	ret,                                                                                 \
	return_kind,                                                                         \
	params,                                                                              \
	args,                                                                                \
	try_params,                                                                          \
	try_args,                                                                            \
	signature,                                                                           \
	delta,                                                                               \
	hook_kind,                                                                           \
	hook_prologue_size,                                                                  \
	callable                                                                             \
)                                                                                        \
	dttr_pcdogs_##name##_addr = dttr_pcdogs_resolve_function(                          \
		ctx,                                                                             \
		DTTR_PCDOGS_SIG(signature),                                                      \
		DTTR_PCDOGS_MASK(signature),                                                     \
		delta                                                                            \
	);                                                                                   \
	if (!dttr_pcdogs_##name##_addr) {                                                  \
		all_ok = false;                                                                  \
	}
DTTR_PCDOGS_TYPED_FUNCTION_ROWS(DTTR_PCDOGS_RESOLVE_ONE)
#undef DTTR_PCDOGS_RESOLVE_ONE

	DTTR_PCDOGS_SymbolsResolveAll(ctx);

#define DTTR_PCDOGS_CLEAR_DATA(                                                        \
	name,                                                                                \
	public,                                                                              \
	type,                                                                                \
	data_id,                                                                             \
	resolver,                                                                            \
	ref_fn,                                                                              \
	instr_off,                                                                           \
	addr_off,                                                                            \
	indirections                                                                         \
)                                                                                        \
	dttr_pcdogs_##name##_addr = 0;
DTTR_PCDOGS_TYPED_DATA_ROWS(DTTR_PCDOGS_CLEAR_DATA)
#undef DTTR_PCDOGS_CLEAR_DATA

#define DTTR_PCDOGS_RESOLVE_DATA(                                                      \
	name,                                                                                \
	public,                                                                              \
	type,                                                                                \
	data_id,                                                                             \
	resolver,                                                                            \
	ref_fn,                                                                              \
	instr_off,                                                                           \
	addr_off,                                                                            \
	indirections                                                                         \
)                                                                                        \
	do {                                                                                 \
		DTTR_PCDOGS_T_Symbol_Data_ID symbol_id_;                                       \
		const DTTR_PCDOGS_T_Symbol_Data* symbol_data_ =                                  \
			DTTR_PCDOGS_DataSymbolID((DTTR_PCDOGS_T_Data_ID)data_id, &symbol_id_)         \
				? DTTR_PCDOGS_SymbolDataAt((uint32_t)symbol_id_)                           \
				: NULL;                                                                     \
		dttr_pcdogs_##name##_addr = symbol_data_ ? symbol_data_->address : 0;            \
		if (!dttr_pcdogs_##name##_addr) {                                                \
			all_ok = false;                                                              \
		}                                                                                \
	} while (0);
DTTR_PCDOGS_TYPED_DATA_ROWS(DTTR_PCDOGS_RESOLVE_DATA)
#undef DTTR_PCDOGS_RESOLVE_DATA
	return all_ok;
}

void DTTR_PCDOGS_Unhook_All(const DTTR_Core_Context* ctx) {
#define DTTR_PCDOGS_UNHOOK_ONE(                                                          \
	name,                                                                                \
	public,                                                                              \
	cc,                                                                                  \
	ret,                                                                                 \
	return_kind,                                                                         \
	params,                                                                              \
	args,                                                                                \
	try_params,                                                                          \
	try_args,                                                                            \
	signature,                                                                           \
	delta,                                                                               \
	hook_kind,                                                                           \
	hook_prologue_size,                                                                  \
	callable                                                                             \
)                                                                                        \
	dttr_pcdogs_##name##_Unhook(ctx);
DTTR_PCDOGS_TYPED_FUNCTION_ROWS(DTTR_PCDOGS_UNHOOK_ONE)
#undef DTTR_PCDOGS_UNHOOK_ONE
}

void DTTR_PCDOGS_Reset() {
#define DTTR_PCDOGS_RESET_FUNCTION(                                                      \
	name,                                                                                \
	public,                                                                              \
	cc,                                                                                  \
	ret,                                                                                 \
	return_kind,                                                                         \
	params,                                                                              \
	args,                                                                                \
	try_params,                                                                          \
	try_args,                                                                            \
	signature,                                                                           \
	delta,                                                                               \
	hook_kind,                                                                           \
	hook_prologue_size,                                                                  \
	callable                                                                             \
)                                                                                        \
	dttr_pcdogs_##name##_addr = 0;
DTTR_PCDOGS_TYPED_FUNCTION_ROWS(DTTR_PCDOGS_RESET_FUNCTION)
#undef DTTR_PCDOGS_RESET_FUNCTION
#define DTTR_PCDOGS_RESET_DATA(                                                        \
	name,                                                                                \
	public,                                                                              \
	type,                                                                                \
	data_id,                                                                             \
	resolver,                                                                            \
	ref_fn,                                                                              \
	instr_off,                                                                           \
	addr_off,                                                                            \
	indirections                                                                         \
)                                                                                        \
	dttr_pcdogs_##name##_addr = 0;
DTTR_PCDOGS_TYPED_DATA_ROWS(DTTR_PCDOGS_RESET_DATA)
#undef DTTR_PCDOGS_RESET_DATA
	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT; i++) {
		dttr_pcdogs_symbol_functions[i].address = 0;
		dttr_pcdogs_symbol_functions[i].resolved = false;
	}
	for (uint32_t i = 0; i < DTTR_PCDOGS_SYMBOL_DATA_COUNT; i++) {
		dttr_pcdogs_symbol_globals[i].address = 0;
		dttr_pcdogs_symbol_globals[i].resolved = false;
	}
}

#undef DTTR_PCDOGS_DEFINE_DATA
#undef DTTR_PCDOGS_DEFINE_FUNCTION
#undef DTTR_PCDOGS_DEFINE_DATA_STORAGE
#undef DTTR_PCDOGS_DEFINE_FUNCTION_STORAGE
#undef DTTR_PCDOGS_RETURN_VALUE
#undef DTTR_PCDOGS_RETURN_VOID
#undef DTTR_PCDOGS_MASK
#undef DTTR_PCDOGS_SIG
#undef DTTR_PCDOGS_COUNT_ONE

#endif // DTTR_PCDOGS_IMPLEMENTATION

#endif // ${header_guard}
