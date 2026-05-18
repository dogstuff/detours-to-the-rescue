% if header_guard.endswith("UNSTABLE_H"):

/// @file dttr_pcdogs_unstable.h
/// Experimental PCDOGS symbols. Expect churn.
/// @ingroup sdk_pcdogs_unstable
/// These declarations may change as symbols get renamed or pinned down.
% else:

/// @file dttr_pcdogs.h
/// Stable PCDOGS symbols and typed wrappers.
/// @ingroup sdk_pcdogs
% endif
#ifndef ${header_guard}
#define ${header_guard}

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <windows.h>

#include <dttr_core.h>
% if header_guard.endswith("UNSTABLE_H"):
// Public unstable users extend the stable surface. Include it first and keep
// duplicate symbol metadata out of this header. The private implementation
// stays standalone because the stable full header defines the same DTTR_* APIs.
#ifndef DTTR_PCDOGS_IMPLEMENTATION
#include <dttr_pcdogs.h>
#endif
% endif

% if header_guard.endswith("UNSTABLE_H"):

/// @addtogroup sdk_pcdogs_unstable
/// @{
% else:

/// @addtogroup sdk_pcdogs
/// @{
% endif

<%
typed_fields = [
    "name",
    "public",
    "cc",
    "ret",
    "return_kind",
    "params",
    "args",
    "try_params",
    "try_args",
    "signature",
    "delta",
    "hook_kind",
    "hook_prologue_size",
    "callable",
]
explicit_signature_names = {c_symbol(row["name"]) for row in signatures}
signature_entries = [
    (c_symbol(row["name"]), c_sig(row["pattern"]), c_mask(row["pattern"]), c_enum(row["required"]))
    for row in signatures
]
signature_entries.extend(
    (
        "FN_" + c_symbol(row["name"]),
        c_sig(row["pattern"]),
        c_mask(row["pattern"]),
        c_enum(row["required"]),
    )
    for row in functions
    if "FN_" + c_symbol(row["name"]) not in explicit_signature_names
)
typed_functions = [row for row in functions if row.get("typed")]

def data_write_policy(row):
    name = row["name"].lower()
    if name.endswith((
        "_dispatch_table",
        "_index_table",
        "_jump_table",
        "_lookup_table",
        "_opcode_table",
        "_op_table",
    )):
        return "DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY"
    if name in {
        "player_actor",
        "current_entity_camera",
        "render_list_state",
        "current_level_data",
        "d3d_device7",
        "ddraw_back_buffer",
        "ddraw_primary_surface",
        "ddraw_z_buffer",
    }:
        return "DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED"
    if row.get("typed"):
        return "DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY"
    return "DTTR_PCDOGS_DATA_WRITE_POLICY_UNKNOWN"

def typed_data_is_pointer(row):
    return str(row.get("typed", {}).get("type", "")).strip().endswith("*")

def data_patch_spec_kind(row):
    if typed_data_is_pointer(row):
        return "DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK"
    return "DTTR_PCDOGS_PATCH_UNSUPPORTED"

def typed_row(fn):
    typed = fn["typed"]
    ident = fn["name"].lower()
    public = c_pascal_token(fn["name"])
    display_name = fn["name"]
    typedef_name = f"DTTR_PCDOGS_F_{public}_proto"
    ret = c_type(typed["return"])
    signature = c_symbol(typed["signature"])
    if signature not in explicit_signature_names:
        signature = "FN_" + c_symbol(fn["name"])
    param_docs = param_doc_pairs(typed["params"], fallback=True)
    return {
        "name": ident,
        "public": public,
        "display_name": display_name,
        "typedef_name": typedef_name,
        "accessor_struct_name": f"dttr_pcdogs_function_accessor_{public}",
        "cc": CC_KEYWORD[str(typed["abi"])],
        "ret": ret,
        "return_kind": "DTTR_PCDOGS_RETURN_VOID" if ret == "void" else "DTTR_PCDOGS_RETURN_VALUE",
        "params": c_params(typed["params"]),
        "args": c_args(typed["args"]),
        "try_params": c_params(typed["try_params"]),
        "try_args": c_args(typed["try_args"]),
        "call_or_params": (
            c_params([("const DTTR_Core_Context*", "ctx"), *typed["params"], (ret, "fallback_ret")])
            if ret != "void"
            else ""
        ),
        "call_or_args": (
            "(" + ", ".join(["ctx", *typed["try_args"], "&ret_"]) + ")"
            if ret != "void"
            else ""
        ),
        "signature": signature,
        "delta": c_int(typed["delta"]),
        "hook_kind": HOOK_ENUM[str(typed["hook_kind"])],
        "hook_prologue_size": c_uint(typed["hook_prologue_size"]),
        "callable": c_bool(typed["callable"]),
        "function_id": f"DTTR_PCDOGS_FUNCTION_{fn['symbol_id']}",
        "symbol_id": f"DTTR_PCDOGS_SYMBOL_FUNCTION_ID_{fn['symbol_id']}",
        "symbol_name": fn.get("display", fn["name"]),
        "doc": symbol_doc("function", fn),
        "group_doc": f"Helpers for `{display_name}`.",
        "param_docs": param_docs,
        "try_param_docs": param_doc_pairs(typed["try_params"], fallback=True),
        "is_callable_param_docs": [("ctx", "Runtime context for this check.")],
        "hook_param_docs": [
            ("ctx", "Runtime context for hook install."),
            ("detour", f"Replacement function with the `{typedef_name}` signature."),
            ("out_original", "Receives the trampoline when requested."),
        ],
        "unhook_param_docs": [("ctx", "Runtime context for hook detach.")],
        "call_or_param_docs": [
            ("ctx", "Runtime context for symbol resolution."),
            *param_docs,
            ("fallback_ret", "Returned when no call is made."),
        ],
        "return_doc": "`true` when the call ran and wrote the return value." if typed["return"] != "void" else "`true` when the call ran.",
    }

typed_function_rows = [typed_row(fn) for fn in typed_functions]
%>
<%def name="render_type_row(row)">
% if row.__class__.__name__ == "TypeAlias":
${doxy_brief(row_doc(row) or "PCDOGS value alias.")}
typedef ${c_type(row.m_source_type)} ${alias_name(row.m_name)};
% elif row.__class__.__name__ == "FunctionTypeAlias":
${doxy_comment(row_doc(row) or "PCDOGS callback type.", params=param_doc_pairs(row.m_params))}
typedef ${c_type(row.m_ret)}(${row.m_calling}*${function_type_name(row.m_name)})${c_params(row.m_params)};
% elif row.__class__.__name__ == "Struct":
% if row.m_incomplete and not unstable:
#ifdef DTTR_PCDOGS_IMPLEMENTATION
${doxy_brief(row_doc(row) or "Private incomplete PCDOGS layout.")}
% else:
${doxy_brief(row_doc(row) or "PCDOGS struct layout.")}
% endif
% if row.m_incomplete and row.m_size is not None:
// Size 0x${format(row.m_size, "X")}; layout is generated from currently known fields.
% elif row.m_size is not None:
// Size 0x${format(row.m_size, "X")}
% endif
struct ${struct_name(row.m_name)} {
% for member in row.m_members:
	${c_type(member.m_type)} ${member.m_name};${" ///< " + member_doc(member) if member_doc(member) else ""}
% endfor
};
% if row.m_incomplete and not unstable:
#endif
% endif
% elif row.__class__.__name__ == "Enum":
${doxy_brief(row_doc(row) or "PCDOGS enum.")}
typedef enum ${enum_name(row.m_name)} {
% for value in row.m_values:
	${value.m_name} = ${value.m_value},${" ///< " + doxy_inline(row_doc(value)) if row_doc(value) else ""}
% endfor
} ${enum_name(row.m_alias or row.m_name)};
	% endif
		</%def>
		% if unstable and external_type_rows:
		% for name in external_forward_names:
		typedef struct ${struct_name(name)} ${struct_name(name)};
		% endfor
		<% in_external_pack = False %>
		% for row in external_type_rows:
		<% is_external_struct = row.__class__.__name__ == "Struct" %>
		% if is_external_struct and not in_external_pack:
		#pragma pack(push, 1)
		<% in_external_pack = True %>
		% elif in_external_pack and not is_external_struct:
		#pragma pack(pop)
		<% in_external_pack = False %>
		% endif
		% if is_external_struct:
		${render_type_row(row).strip()}
		% else:
		#ifdef DTTR_PCDOGS_IMPLEMENTATION
		${render_type_row(row).strip()}
		#endif
		% endif
		% endfor
		% if in_external_pack:
		#pragma pack(pop)
		% endif
		% endif
	% for row in type_prefix_rows:
	${render_type_row(row).strip()}
	% endfor
% for name in forward_names:
typedef struct ${struct_name(name)} ${struct_name(name)};
% endfor
% if packed_type_rows:
<% in_pack = False %>
% for row in packed_type_rows:
<% is_struct = row.__class__.__name__ == "Struct" %>
% if is_struct and not in_pack:
#pragma pack(push, 1)
<% in_pack = True %>
% elif in_pack and not is_struct:
#pragma pack(pop)
<% in_pack = False %>
% endif
${render_type_row(row).strip()}
% endfor
% if in_pack:
#pragma pack(pop)
% endif
% endif
% if unstable:

/// Cast a stable opaque Level_Data pointer to the unstable audited runtime layout.
/// The stable API keeps Level_Data opaque; this helper makes the intentional
/// bridge explicit for callers that opt into unstable layouts. Packaged SDK users
/// should define DTTR_SDK_ENABLE_UNSTABLE before including dttr_sdk.h; source-tree
/// users may include dttr_pcdogs_unstable.h directly.
/// @param ptr_ Stable Level_Data pointer.
/// @return Mutable unstable runtime data pointer.
static inline DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeDataMutable(void* ptr_) {
	return (DTTR_PCDOGS_T_Level_RuntimeData*)ptr_;
}

/// Const-preserving variant of DTTR_PCDOGS_LevelData_AsRuntimeData.
/// @param ptr_ Stable Level_Data pointer.
/// @return Const unstable runtime data pointer.
static inline const DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeDataConst(const void* ptr_) {
	return (const DTTR_PCDOGS_T_Level_RuntimeData*)ptr_;
}

#ifdef __cplusplus
static inline DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeData(DTTR_PCDOGS_T_Level_Data* ptr_) {
	return DTTR_PCDOGS_LevelData_AsRuntimeDataMutable(ptr_);
}

static inline const DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeData(const DTTR_PCDOGS_T_Level_Data* ptr_) {
	return DTTR_PCDOGS_LevelData_AsRuntimeDataConst(ptr_);
}

static inline DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeData(void* ptr_) {
	return DTTR_PCDOGS_LevelData_AsRuntimeDataMutable(ptr_);
}

static inline const DTTR_PCDOGS_T_Level_RuntimeData*
DTTR_PCDOGS_LevelData_AsRuntimeData(const void* ptr_) {
	return DTTR_PCDOGS_LevelData_AsRuntimeDataConst(ptr_);
}
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define DTTR_PCDOGS_LevelData_AsRuntimeData(ptr_)                                      \
	_Generic(                                                                          \
		(ptr_),                                                                        \
		const DTTR_PCDOGS_T_Level_Data*: DTTR_PCDOGS_LevelData_AsRuntimeDataConst,     \
		const DTTR_PCDOGS_T_Level_RuntimeData*: DTTR_PCDOGS_LevelData_AsRuntimeDataConst, \
		const void*: DTTR_PCDOGS_LevelData_AsRuntimeDataConst,                         \
		default: DTTR_PCDOGS_LevelData_AsRuntimeDataMutable                            \
	)(ptr_)
#else
#define DTTR_PCDOGS_LevelData_AsRuntimeData(ptr_)                                      \
	DTTR_PCDOGS_LevelData_AsRuntimeDataMutable((void*)(ptr_))
#endif
% endif

#ifndef DTTR_PCDOGS_API
#define DTTR_PCDOGS_API extern
#endif

% if unstable:
#ifndef DTTR_PCDOGS_H
% endif
/// Generated hook-site shape for a PCDOGS function.
///
/// `REL32` sites can use the generated `Hook()` helper and `PatchSpec()` helper because
/// the function entry has a full trampoline prologue window. `HOTPATCH` sites describe a
/// two-part hotpatch layout: a five-byte pre-entry jump slot plus the short entry
/// instruction window reported by `HookPrologueSize()`. The generated typed helper does
/// not install those hotpatch hooks yet; use the metadata for audits or explicit low-level
/// patching only.
typedef enum DTTR_PCDOGS_T_Hook_Kind {
	DTTR_PCDOGS_HOOK_UNSUPPORTED = 0, ///< No generated hook metadata is available.
	DTTR_PCDOGS_HOOK_REL32 = 1,       ///< Entry can be detoured with a trampoline `E9 <rel32>` hook.
	DTTR_PCDOGS_HOOK_HOTPATCH = 2,    ///< Hotpatch layout: pre-entry slot plus entry-window metadata.
} DTTR_PCDOGS_T_Hook_Kind;

typedef enum DTTR_PCDOGS_T_Data_Resolver {
	DTTR_PCDOGS_DATA_RESOLVE_XREF_U32 = 1,
} DTTR_PCDOGS_T_Data_Resolver;

/// Generated data-symbol write policy.
///
/// `Write` enforces this policy and only writes `RAW_MEMORY` symbols. `UnsafeWrite`
/// bypasses the policy gate but still requires writable process memory. Reserve
/// `UnsafeWrite` for explicit patching, reverse-engineering work, or SDK internals.
/// `READ_ONLY` marks decoded dispatch/jump/lookup/opcode/index tables. `ENGINE_OWNED`
/// marks live pointers or state that the game may replace or overwrite. `PATCH_ONLY`
/// is for symbols that should be changed through generated patch specs or patch groups.
typedef enum DTTR_PCDOGS_T_Data_Write_Policy {
	DTTR_PCDOGS_DATA_WRITE_POLICY_UNKNOWN = 0,     ///< Untyped or insufficiently classified symbol.
	DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY = 1,   ///< Decoded table data; read or inspect, do not write through `Write`.
	DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED = 2, ///< Live game-owned pointer/state; use higher-level helpers or patch flows.
	DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY = 3,  ///< Plain generated data slot writable through `Write`.
	DTTR_PCDOGS_DATA_WRITE_POLICY_PATCH_ONLY = 4,  ///< Change through generated patch specs or patch groups.
} DTTR_PCDOGS_T_Data_Write_Policy;

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
	DTTR_PCDOGS_T_Data_Write_Policy write_policy;
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
	DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE = ${len(function_xrefs)}
};

enum {
	DTTR_PCDOGS_SYMBOL_XREF_COUNT_VALUE = ${len(xrefs)}
};

#define DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_FUNCTION_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_DATA_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE)
#define DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_STORAGE_COUNT \
	(DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE ? DTTR_PCDOGS_SYMBOL_FUNCTION_XREF_COUNT_VALUE : 1u)
#define DTTR_PCDOGS_SYMBOL_XREF_COUNT ((uint32_t)DTTR_PCDOGS_SYMBOL_XREF_COUNT_VALUE)

/// Function symbol identifiers.
typedef enum DTTR_PCDOGS_T_Symbol_Function_Id {
% for row in functions:
	DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row['symbol_id']}, ///< ${doxy_inline(symbol_doc('function', row))}
% endfor
} DTTR_PCDOGS_T_Symbol_Function_Id;

/// Data symbol identifiers.
typedef enum DTTR_PCDOGS_T_Symbol_Data_Id {
% if not globals:
	DTTR_PCDOGS_SYMBOL_DATA_ID_NONE = -1,
% endif
% for row in globals:
	DTTR_PCDOGS_SYMBOL_DATA_ID_${row['symbol_id']}, ///< ${doxy_inline(symbol_doc('global', row))}
% endfor
} DTTR_PCDOGS_T_Symbol_Data_Id;

% if not unstable:

/// Stable function identifiers for the public SDK facade.
typedef enum DTTR_PCDOGS_T_Function_Id {
% for row in functions:
	DTTR_PCDOGS_FUNCTION_${row['symbol_id']} = DTTR_PCDOGS_SYMBOL_FUNCTION_ID_${row['symbol_id']}, ///< ${doxy_inline(symbol_doc('function', row))}
% endfor
} DTTR_PCDOGS_T_Function_Id;

/// Stable global identifiers for the public SDK facade.
typedef enum DTTR_PCDOGS_T_Data_Id {
% if not globals:
	DTTR_PCDOGS_DATA_NONE = -1,
% endif
% for row in globals:
	DTTR_PCDOGS_DATA_${row['symbol_id']} = DTTR_PCDOGS_SYMBOL_DATA_ID_${row['symbol_id']}, ///< ${doxy_inline(symbol_doc('global', row))}
% endfor
} DTTR_PCDOGS_T_Data_Id;


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
	DTTR_PCDOGS_T_Function_Id function;
	DTTR_PCDOGS_T_Data_Id global;
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
	DTTR_Core_Status status;
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

DTTR_Core_Result DTTR_PCDOGS_FunctionResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Function_Id id,
	uintptr_t* out_addr
);
DTTR_Core_Result DTTR_PCDOGS_DataResolve(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Data_Id id,
	uintptr_t* out_addr
);
DTTR_Core_Result DTTR_PCDOGS_PatchGroup_HookSymbolFunction(
	DTTR_Core_PatchGroup* group,
	DTTR_PCDOGS_T_Function_Id id,
	void* detour,
	void** out_original
);
DTTR_Core_Result DTTR_PCDOGS_Hook_DataPointer(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Data_Id id,
	void* new_value,
	void** out_original,
	DTTR_Core_Hook** out_hook
);
DTTR_Core_Result DTTR_PCDOGS_PatchGroup_HookDataPointer(
	DTTR_Core_PatchGroup* group,
	DTTR_PCDOGS_T_Data_Id id,
	void* new_value,
	void** out_original
);
DTTR_Core_Result DTTR_PCDOGS_PatchGroup_Install(
	const DTTR_Core_Context* ctx,
	const DTTR_PCDOGS_T_Patch_Spec* specs,
	size_t spec_count,
	DTTR_Core_PatchGroup** out_group,
	DTTR_PCDOGS_T_Patch_Report* out_report
);

#ifdef __cplusplus
}
#endif

% endif
#ifdef DTTR_PCDOGS_IMPLEMENTATION

// PCDOGS signature rows.
#ifndef DTTR_PCDOGS_SIGNATURE_ROWS_DEF
#define DTTR_PCDOGS_SIGNATURE_ROWS_DEF

% if signature_entries:
#define DTTR_PCDOGS_SIGNATURE_ROWS(SIGNATURE) \
% for name, sig, mask, required in signature_entries:
	SIGNATURE(${name}, ${sig}, ${mask}, ${required})${' \\' if loop.index != len(signature_entries) - 1 else ''}
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
% for value_i, field in enumerate(typed_fields):
		${row[field]}${',' if value_i != len(typed_fields) - 1 else ''} \
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
	FN(${row['symbol_id']}, ${CC_ENUM[str(row['calling_convention'])]}, ${HOOK_ENUM[str(row['hook']['kind'])]}, ${c_bool(row['callable'])}, ${c_build_mask(row['supported_builds'])}, ${c_sig(row['pattern'])}, ${c_mask(row['pattern'])}, ${c_int(row['match_offset'])}, ${c_uint(row['hook']['patch_size'])}, ${c_uint(row['hook']['entry_patch_size'])})${' \\' if loop.index != len(functions) - 1 else ''}
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
% if row.get("typed"):
	TYPED_DATA(CTX, ${row['name']}, ${c_pascal_token(row['name'])}, ${c_type(row['typed']['type'])}, DTTR_PCDOGS_DATA_${row['symbol_id']}, ${DATA_RESOLVER[str(row['typed']['resolver'])]}, ${row['typed']['ref_function'].lower()}, ${c_uint(row['typed']['instr_off'])}, ${c_uint(row['typed']['addr_off'])}, ${c_uint(row['typed']['indirections'])})${' \\' if loop.index != len(globals) - 1 else ''}
% else:
	UNTYPED_DATA(CTX, ${row['name']})${' \\' if loop.index != len(globals) - 1 else ''}
% endif
% endfor
% else:
#define DTTR_PCDOGS_DATA_ROWS(TYPED_DATA, UNTYPED_DATA, CTX)
% endif

% if globals:
#define DTTR_PCDOGS_SYMBOL_DATA_ROWS(DATA) \
% for row in globals:
	DATA(${row['symbol_id']})${' \\' if loop.index != len(globals) - 1 else ''}
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
	FN_XREF(${loop.index}, ${row['function_symbol']}, ${row['ref_function_symbol']}, ${c_uint(row['instr_off'])}, ${c_uint(row['addr_off'])}, ${c_uint(row.get('indirections', 0))})${' \\' if loop.index != len(function_xrefs) - 1 else ''}
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
	XREF(${loop.index}, ${row['global_symbol']}, ${row['function_symbol']}, ${c_uint(row['instr_off'])}, ${c_uint(row['addr_off'])})${' \\' if loop.index != len(xrefs) - 1 else ''}
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

static DTTR_PCDOGS_T_Symbol_Data dttr_pcdogs_symbol_globals[DTTR_PCDOGS_SYMBOL_DATA_COUNT_VALUE] = {
% for row in globals:
	[DTTR_PCDOGS_SYMBOL_DATA_ID_${row['symbol_id']}] = DTTR_PCDOGS_SYMBOL_DATA(${data_write_policy(row)}, ${c_build_mask(row['supported_builds'])}),
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

% if unstable:
#endif  // !DTTR_PCDOGS_H
% endif

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

/// @name ${row["display_name"]}
/// Typed symbol object for `${row["display_name"]}`.
/// @{
${doxy_comment(row["doc"], params=row["param_docs"])}
typedef ${row["ret"]}(${row["cc"]}*${row["typedef_name"]}) ${row["params"]};

struct ${row["accessor_struct_name"]} {
	DTTR_PCDOGS_T_Symbol_Function_Id SymbolId;
% if not unstable:
	DTTR_PCDOGS_T_Function_Id FunctionId;
% endif
	bool (*IsResolved)();
	bool (*IsCallable)(const DTTR_Core_Context* ctx);
	uintptr_t (*Address)();
	DTTR_PCDOGS_T_Hook_Kind (*HookKind)(); ///< Returns the generated hook-site shape.
	uint32_t (*HookPrologueSize)();        ///< REL32 trampoline size, or HOTPATCH entry-window size.
	bool (*Try) ${row["try_params"]};
		// Installs only REL32 generated hooks; returns false for HOTPATCH metadata.
	bool (*Hook)(
		const DTTR_Core_Context* ctx,
		${row["typedef_name"]} detour,
		${row["typedef_name"]}* out_original
	);
% if not unstable:
		// Builds only REL32 function-hook specs; HOTPATCH metadata yields an unsupported spec.
	DTTR_PCDOGS_T_Patch_Spec (*PatchSpec)(
		bool required,
		${row["typedef_name"]} detour,
		${row["typedef_name"]}* out_original
	);
% endif
	void (*Unhook)(const DTTR_Core_Context* ctx);
% if row["ret"] != "void":
	${row["ret"]} (*Call) ${row["call_or_params"]};
% endif
};

DTTR_PCDOGS_API const struct ${row["accessor_struct_name"]}* const DTTR_PCDOGS_F_${row["public"]};

/// @}

% endfor
% for row in globals:
% if row.get("typed"):
${doxy_comment(symbol_doc("global", row), returns="Typed data symbol object.")}
struct DTTR_PCDOGS_D_${c_pascal_token(row['name'])}_type {
	DTTR_PCDOGS_T_Symbol_Data_Id SymbolId;
% if not unstable:
	DTTR_PCDOGS_T_Data_Id DataId;
% endif
	DTTR_PCDOGS_T_Data_Write_Policy WritePolicy;
	bool (*IsResolved)();
	uintptr_t (*Address)();
	${c_type(row['typed']['type'])}* (*Ptr)();
	bool (*Read)(${c_type(row['typed']['type'])}* out_value);
		/// Policy-gated writer. Returns false unless WritePolicy is RAW_MEMORY.
	bool (*Write)(${c_type(row['typed']['type'])} value);
		/// Bypasses WritePolicy; still requires resolved writable memory.
	bool (*UnsafeWrite)(${c_type(row['typed']['type'])} value);
% if not unstable:
		// Builds a pointer-hook spec for pointer data; scalar data returns unsupported.
	DTTR_PCDOGS_T_Patch_Spec (*PatchSpec)(
		bool required,
		void* new_value,
		void** out_original
	);
% endif
};

DTTR_PCDOGS_API const struct DTTR_PCDOGS_D_${c_pascal_token(row['name'])}_type* const DTTR_PCDOGS_D_${c_pascal_token(row['name'])};

% endif
% endfor
% if not unstable:

/// @name Actor_Position
/// Actor position wrapper helpers.
/// @{
typedef struct DTTR_PCDOGS_T_Vec3i32 {
	int32_t x;
	int32_t y;
	int32_t z;
} DTTR_PCDOGS_T_Vec3i32;

${doxy_comment("Read logical actor position fields in game fixed-point units.", params=[("actor", "Actor pointer from a hook or global."), ("out_position", "Receives position X/Y/Z.")], returns="`true` when all position fields were readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_ReadPosition(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32* out_position
);
${doxy_comment("Write logical actor position fields only. This does not update render mirrors or physics state.", params=[("actor", "Actor pointer from a hook or global."), ("position", "Position X/Y/Z in game fixed-point units.")], returns="`true` when all logical position fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_WritePosition(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 position
);
${doxy_comment("Write logical actor position and the render-position mirror fields together. This does not update velocity, collision, scripts, or camera-specific state.", params=[("actor", "Actor pointer from a hook or global."), ("position", "Position X/Y/Z in game fixed-point units.")], returns="`true` when all logical and render-mirror fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_WritePositionAndRenderMirror(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 position
);
/// @}

/// @name Actor_Velocity
/// Actor velocity wrapper helpers.
/// @{
${doxy_comment("Read actor velocity fields in game fixed-point units.", params=[("actor", "Actor pointer from a hook or global."), ("out_velocity", "Receives velocity X/Y/Z.")], returns="`true` when all velocity fields were readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_ReadVelocity(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32* out_velocity
);
${doxy_comment("Write actor velocity fields only.", params=[("actor", "Actor pointer from a hook or global."), ("velocity", "Velocity X/Y/Z in game fixed-point units.")], returns="`true` when all velocity fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_WriteVelocity(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 velocity
);
/// @}

/// @name Actor_Scale
/// Actor scale wrapper helpers.
/// @{
typedef struct DTTR_PCDOGS_T_Actor_Scale {
	int32_t scale_xy;
	int16_t scale_factor;
} DTTR_PCDOGS_T_Actor_Scale;

${doxy_comment("Read actor scale fields from the generated `Actor_State` layout.", params=[("actor", "Actor pointer from a hook or global."), ("out_scale", "Receives current scale.")], returns="`true` when both scale fields were readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_Scale_Read(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Actor_Scale* out_scale
);
${doxy_comment("Write actor scale fields through the generated `Actor_State` layout.", params=[("actor", "Actor pointer from a hook or global."), ("scale", "Scale values to write.")], returns="`true` when both scale fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_Scale_Write(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Actor_Scale scale
);
${doxy_comment("Return actor scale values multiplied with integer clamping.", params=[("scale", "Scale values to multiply."), ("multiplier", "Integer multiplier to apply to both scale fields.")], returns="Clamped multiplied scale values.")}
DTTR_PCDOGS_API DTTR_PCDOGS_T_Actor_Scale DTTR_PCDOGS_Actor_Scale_MultiplyClamped(
	DTTR_PCDOGS_T_Actor_Scale scale,
	int32_t multiplier
);
${doxy_comment("Read, save, multiply, and write an actor scale in one checked helper.", params=[("actor", "Actor pointer from a hook or global."), ("multiplier", "Integer multiplier to apply."), ("out_original", "Receives the original scale values when requested.")], returns="`true` when the original scale was read and the multiplied scale was written.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_Scale_PushMultiplied(
	DTTR_PCDOGS_T_Actor_State* actor,
	int32_t multiplier,
	DTTR_PCDOGS_T_Actor_Scale* out_original
);
${doxy_comment("Restore scale values previously returned by `DTTR_PCDOGS_Actor_Scale_Read` or `DTTR_PCDOGS_Actor_Scale_PushMultiplied`.", params=[("actor", "Actor pointer from a hook or global."), ("scale", "Scale values to restore.")], returns="`true` when both scale fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Actor_Scale_Restore(
	DTTR_PCDOGS_T_Actor_State* actor,
	const DTTR_PCDOGS_T_Actor_Scale* scale
);
/// @}

/// @name Collision3D_ReadPayload
/// Collision payload wrapper helpers.
/// @{
typedef struct DTTR_PCDOGS_T_Vec3i16 {
	int16_t x;
	int16_t y;
	int16_t z;
} DTTR_PCDOGS_T_Vec3i16;

typedef struct DTTR_PCDOGS_T_Collision3D_Payload {
	uint32_t struct_size;
	DTTR_PCDOGS_T_Actor_State* actor;
	int32_t* velocity_ptr;
	int16_t* surface_normal_ptr;
	int16_t* contact_point_ptr;
	int32_t* result_ptr;
	DTTR_PCDOGS_T_Vec3i32 velocity;
	DTTR_PCDOGS_T_Vec3i16 surface_normal;
	DTTR_PCDOGS_T_Vec3i16 contact_point;
	int32_t result_value;
	bool velocity_readable;
	bool velocity_writable;
	bool surface_normal_readable;
	bool contact_point_readable;
	bool result_readable;
	bool result_writable;
	void* reserved[4];
} DTTR_PCDOGS_T_Collision3D_Payload;

${doxy_comment("Copy and classify raw `Collision_DetectAndResolve3DCollision` payload pointers without taking ownership.", params=[("actor", "Actor/context pointer supplied to the collision hook."), ("velocity", "Three-int velocity payload pointer, or NULL."), ("surface_normal", "Three-int16 surface-normal payload pointer, or NULL."), ("contact_point", "Three-int16 contact-point payload pointer, or NULL."), ("result", "Result/contact payload pointer, or NULL."), ("out_payload", "Receives copied values plus readability/writability flags. Set struct_size before calling for forward-compatible partial fills.")], returns="`true` when at least one payload or actor pointer was classified.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Collision3D_ReadPayload(
	DTTR_PCDOGS_T_Actor_State* actor,
	int32_t* velocity,
	int16_t* surface_normal,
	int16_t* contact_point,
	int32_t* result,
	DTTR_PCDOGS_T_Collision3D_Payload* out_payload
);
/// @}

/// @name Camera_Runtime
/// Camera runtime wrapper helpers.
/// @{
typedef struct DTTR_PCDOGS_T_Camera_RuntimePose {
	uint32_t struct_size;
	int16_t yaw;
	int16_t pitch;
	int16_t roll;
	int16_t look_at_pitch;
	int16_t orbit_yaw;
	int16_t fov;
	int32_t focal_distance;
	DTTR_PCDOGS_T_Vec3i32 eye;
	DTTR_PCDOGS_T_Vec3i32 target;
	uint32_t flags;
	void* reserved[4];
} DTTR_PCDOGS_T_Camera_RuntimePose;

${doxy_comment("Read movement yaw from a Camera_Runtime record.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("out_yaw", "Receives the movement yaw angle word.")], returns="`true` when the yaw field was readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_ReadMovementYaw(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_yaw
);
${doxy_comment("Read camera look-at pitch from a Camera_Runtime record.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("out_pitch", "Receives the look-at pitch angle word.")], returns="`true` when the look-at pitch field was readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_ReadLookAtPitch(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_pitch
);
${doxy_comment("Read camera orbit yaw from a Camera_Runtime record.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("out_yaw", "Receives the orbit-yaw angle word.")], returns="`true` when the orbit-yaw field was readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_ReadOrbitYaw(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_yaw
);
${doxy_comment("Read camera FOV from a Camera_Runtime record without touching packed yaw globals.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("out_fov", "Receives the FOV angle word.")], returns="`true` when the FOV field was readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_ReadFov(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_fov
);
${doxy_comment("Read typed camera pose fields. The SDK presents eye/target as X/Y/Z even though the runtime layout stores X/Z/Y.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("out_pose", "Receives a versioned camera pose. Set struct_size before calling for forward-compatible partial fills.")], returns="`true` when all current pose fields were readable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_ReadPose(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	DTTR_PCDOGS_T_Camera_RuntimePose* out_pose
);
${doxy_comment("Write typed camera pose fields. The SDK maps X/Y/Z into the runtime's X/Z/Y storage order.", params=[("camera", "Camera runtime pointer from a documented camera hook."), ("pose", "Pose to write. `struct_size` may be zero or at least the current struct size.")], returns="`true` when all current pose fields were writable.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_Camera_WritePose(
	DTTR_PCDOGS_T_Camera_Runtime* camera,
	const DTTR_PCDOGS_T_Camera_RuntimePose* pose
);
/// @}

/// @name PlayerActor_QueryForHook
/// Player-actor classification wrapper helpers.
/// @{
typedef enum DTTR_PCDOGS_T_PlayerActor_Source {
	DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_NONE = 0,
	DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_CANDIDATE = 1,
	DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_RENDER_GLOBAL = 2,
	DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_ACTIVE_ACTOR = 3,
} DTTR_PCDOGS_T_PlayerActor_Source;

typedef enum DTTR_PCDOGS_T_PlayerActor_Confidence {
	DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_NONE = 0,
	DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_LOW = 1,
	DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_MEDIUM = 2,
	DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_HIGH = 3,
} DTTR_PCDOGS_T_PlayerActor_Confidence;

typedef struct DTTR_PCDOGS_T_PlayerActor_Query {
	uint32_t struct_size;
	uint32_t api_version;
	uint32_t flags;
	DTTR_PCDOGS_T_Actor_State* actor;
	DTTR_PCDOGS_T_PlayerActor_Source source;
	DTTR_PCDOGS_T_PlayerActor_Confidence confidence;
	uint64_t frame_index;
	bool mutable_now;
	bool retainable_after_callback;
	void* reserved[4];
} DTTR_PCDOGS_T_PlayerActor_Query;

${doxy_comment("Classify a player-actor candidate for the current hook timing. This is a thin timing/lifetime contract, not gameplay identity inference.", params=[("ctx", "Runtime context for optional active-actor checks."), ("call_site_function_id", "Generated function id for the hook or callback being handled."), ("candidate", "Actor-like pointer supplied by the hook, or NULL."), ("out_query", "Receives source, confidence, mutability, and retainability metadata. Set struct_size before calling for forward-compatible partial fills.")], returns="`true` when an actor-like pointer could be classified.")}
DTTR_PCDOGS_API bool DTTR_PCDOGS_PlayerActor_QueryForHook(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Function_Id call_site_function_id,
	DTTR_PCDOGS_T_Actor_State* candidate,
	DTTR_PCDOGS_T_PlayerActor_Query* out_query
);
/// @}

% endif
% if unstable:
#ifndef DTTR_PCDOGS_H
% endif
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
DTTR_PCDOGS_API const DTTR_PCDOGS_T_Symbol_Data* DTTR_PCDOGS_SymbolDataAt(
	uint32_t index
);
DTTR_PCDOGS_API bool DTTR_PCDOGS_SymbolsResolveAll(const DTTR_Core_Context* ctx);
DTTR_PCDOGS_API bool DTTR_PCDOGS_SymbolFunctionIsCallable(
	const DTTR_Core_Context* ctx,
	const DTTR_PCDOGS_T_Symbol_Function* fn
);
% if unstable:
#endif  // !DTTR_PCDOGS_H
% endif

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

% if not unstable:
static int32_t dttr_pcdogs_i32_from_i64_clamped(int64_t value) {
	if (value > INT32_MAX) {
		return INT32_MAX;
	}
	if (value < INT32_MIN) {
		return INT32_MIN;
	}
	return (int32_t)value;
}

static int16_t dttr_pcdogs_i16_from_i64_clamped(int64_t value) {
	if (value > INT16_MAX) {
		return INT16_MAX;
	}
	if (value < INT16_MIN) {
		return INT16_MIN;
	}
	return (int16_t)value;
}

static bool dttr_pcdogs_actor_position_region_has(
	const DTTR_PCDOGS_T_Actor_State* actor,
	bool write
) {
	return actor
		   && dttr_pcdogs_region_has((uintptr_t)&actor->position_x, sizeof(actor->position_x), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->position_y, sizeof(actor->position_y), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->position_z, sizeof(actor->position_z), write, false);
}

static bool dttr_pcdogs_actor_render_position_region_has(
	const DTTR_PCDOGS_T_Actor_State* actor,
	bool write
) {
	return actor
		   && dttr_pcdogs_region_has((uintptr_t)&actor->world_render_pos_x, sizeof(actor->world_render_pos_x), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->world_render_pos_y, sizeof(actor->world_render_pos_y), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->world_render_pos_z, sizeof(actor->world_render_pos_z), write, false);
}

static bool dttr_pcdogs_actor_velocity_region_has(
	const DTTR_PCDOGS_T_Actor_State* actor,
	bool write
) {
	return actor
		   && dttr_pcdogs_region_has((uintptr_t)&actor->velocity_x, sizeof(actor->velocity_x), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->velocity_y, sizeof(actor->velocity_y), write, false)
		   && dttr_pcdogs_region_has((uintptr_t)&actor->velocity_z, sizeof(actor->velocity_z), write, false);
}

bool DTTR_PCDOGS_Actor_ReadPosition(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32* out_position
) {
	if (!out_position || !dttr_pcdogs_actor_position_region_has(actor, false)) {
		return false;
	}
	out_position->x = actor->position_x;
	out_position->y = actor->position_y;
	out_position->z = actor->position_z;
	return true;
}

bool DTTR_PCDOGS_Actor_WritePosition(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 position
) {
	if (!dttr_pcdogs_actor_position_region_has(actor, true)) {
		return false;
	}
	actor->position_x = position.x;
	actor->position_y = position.y;
	actor->position_z = position.z;
	return true;
}

bool DTTR_PCDOGS_Actor_WritePositionAndRenderMirror(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 position
) {
	if (!dttr_pcdogs_actor_position_region_has(actor, true)
		|| !dttr_pcdogs_actor_render_position_region_has(actor, true)) {
		return false;
	}
	actor->position_x = position.x;
	actor->position_y = position.y;
	actor->position_z = position.z;
	actor->world_render_pos_x = position.x;
	actor->world_render_pos_y = position.y;
	actor->world_render_pos_z = position.z;
	return true;
}

bool DTTR_PCDOGS_Actor_ReadVelocity(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32* out_velocity
) {
	if (!out_velocity || !dttr_pcdogs_actor_velocity_region_has(actor, false)) {
		return false;
	}
	out_velocity->x = actor->velocity_x;
	out_velocity->y = actor->velocity_y;
	out_velocity->z = actor->velocity_z;
	return true;
}

bool DTTR_PCDOGS_Actor_WriteVelocity(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Vec3i32 velocity
) {
	if (!dttr_pcdogs_actor_velocity_region_has(actor, true)) {
		return false;
	}
	actor->velocity_x = velocity.x;
	actor->velocity_y = velocity.y;
	actor->velocity_z = velocity.z;
	return true;
}

bool DTTR_PCDOGS_Collision3D_ReadPayload(
	DTTR_PCDOGS_T_Actor_State* actor,
	int32_t* velocity,
	int16_t* surface_normal,
	int16_t* contact_point,
	int32_t* result,
	DTTR_PCDOGS_T_Collision3D_Payload* out_payload
) {
	if (!out_payload
		|| (out_payload->struct_size && out_payload->struct_size < sizeof(DTTR_PCDOGS_T_Collision3D_Payload))) {
		return false;
	}
	DTTR_PCDOGS_T_Collision3D_Payload payload;
	memset(&payload, 0, sizeof(payload));
	payload.struct_size = sizeof(payload);
	payload.actor = actor;
	payload.velocity_ptr = velocity;
	payload.surface_normal_ptr = surface_normal;
	payload.contact_point_ptr = contact_point;
	payload.result_ptr = result;
	payload.velocity_readable = dttr_pcdogs_region_has((uintptr_t)velocity, sizeof(int32_t) * 3u, false, false);
	payload.velocity_writable = dttr_pcdogs_region_has((uintptr_t)velocity, sizeof(int32_t) * 3u, true, false);
	if (payload.velocity_readable) {
		payload.velocity.x = velocity[0];
		payload.velocity.y = velocity[1];
		payload.velocity.z = velocity[2];
	}
	payload.surface_normal_readable = dttr_pcdogs_region_has((uintptr_t)surface_normal, sizeof(int16_t) * 3u, false, false);
	if (payload.surface_normal_readable) {
		payload.surface_normal.x = surface_normal[0];
		payload.surface_normal.y = surface_normal[1];
		payload.surface_normal.z = surface_normal[2];
	}
	payload.contact_point_readable = dttr_pcdogs_region_has((uintptr_t)contact_point, sizeof(int16_t) * 3u, false, false);
	if (payload.contact_point_readable) {
		payload.contact_point.x = contact_point[0];
		payload.contact_point.y = contact_point[1];
		payload.contact_point.z = contact_point[2];
	}
	payload.result_readable = dttr_pcdogs_region_has((uintptr_t)result, sizeof(int32_t), false, false);
	payload.result_writable = dttr_pcdogs_region_has((uintptr_t)result, sizeof(int32_t), true, false);
	if (payload.result_readable) {
		payload.result_value = *result;
	}
	*out_payload = payload;
	return actor || velocity || surface_normal || contact_point || result;
}

static bool dttr_pcdogs_camera_region_has(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	size_t offset,
	size_t size,
	bool write
) {
	return camera && dttr_pcdogs_region_has((uintptr_t)((const uint8_t*)camera + offset), size, write, false);
}

static bool dttr_pcdogs_camera_read_i16(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	const int16_t* field,
	int16_t* out_value
) {
	if (!out_value || !camera || !dttr_pcdogs_region_has((uintptr_t)field, sizeof(*field), false, false)) {
		return false;
	}
	*out_value = *field;
	return true;
}

bool DTTR_PCDOGS_Camera_ReadMovementYaw(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_yaw
) {
	return dttr_pcdogs_camera_read_i16(camera, camera ? &camera->yaw : NULL, out_yaw);
}

bool DTTR_PCDOGS_Camera_ReadLookAtPitch(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_pitch
) {
	return dttr_pcdogs_camera_read_i16(camera, camera ? &camera->look_at_pitch : NULL, out_pitch);
}

bool DTTR_PCDOGS_Camera_ReadOrbitYaw(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_yaw
) {
	return dttr_pcdogs_camera_read_i16(camera, camera ? &camera->orbit_yaw : NULL, out_yaw);
}

bool DTTR_PCDOGS_Camera_ReadFov(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	int16_t* out_fov
) {
	return dttr_pcdogs_camera_read_i16(camera, camera ? &camera->fov : NULL, out_fov);
}

bool DTTR_PCDOGS_Camera_ReadPose(
	const DTTR_PCDOGS_T_Camera_Runtime* camera,
	DTTR_PCDOGS_T_Camera_RuntimePose* out_pose
) {
	if (!out_pose
		|| (out_pose->struct_size && out_pose->struct_size < sizeof(DTTR_PCDOGS_T_Camera_RuntimePose))
		|| !dttr_pcdogs_camera_region_has(camera, 0, sizeof(*camera), false)) {
		return false;
	}
	DTTR_PCDOGS_T_Camera_RuntimePose pose;
	memset(&pose, 0, sizeof(pose));
	pose.struct_size = sizeof(pose);
	pose.yaw = camera->yaw;
	pose.pitch = camera->pitch;
	pose.roll = camera->roll;
	pose.look_at_pitch = camera->look_at_pitch;
	pose.orbit_yaw = camera->orbit_yaw;
	pose.fov = camera->fov;
	pose.focal_distance = camera->focal_distance;
	pose.eye.x = camera->eye_pos_x;
	pose.eye.y = camera->eye_pos_y;
	pose.eye.z = camera->eye_pos_z;
	pose.target.x = camera->target_x;
	pose.target.y = camera->target_y;
	pose.target.z = camera->target_z;
	*out_pose = pose;
	return true;
}

bool DTTR_PCDOGS_Camera_WritePose(
	DTTR_PCDOGS_T_Camera_Runtime* camera,
	const DTTR_PCDOGS_T_Camera_RuntimePose* pose
) {
	if (!pose
		|| (pose->struct_size && pose->struct_size < sizeof(DTTR_PCDOGS_T_Camera_RuntimePose))
		|| !dttr_pcdogs_camera_region_has(camera, 0, sizeof(*camera), true)) {
		return false;
	}
	camera->yaw = pose->yaw;
	camera->pitch = pose->pitch;
	camera->roll = pose->roll;
	camera->look_at_pitch = pose->look_at_pitch;
	camera->orbit_yaw = pose->orbit_yaw;
	camera->fov = pose->fov;
	camera->focal_distance = pose->focal_distance;
	camera->eye_pos_x = pose->eye.x;
	camera->eye_pos_y = pose->eye.y;
	camera->eye_pos_z = pose->eye.z;
	camera->target_x = pose->target.x;
	camera->target_y = pose->target.y;
	camera->target_z = pose->target.z;
	return true;
}

bool DTTR_PCDOGS_PlayerActor_QueryForHook(
	const DTTR_Core_Context* ctx,
	DTTR_PCDOGS_T_Function_Id call_site_function_id,
	DTTR_PCDOGS_T_Actor_State* candidate,
	DTTR_PCDOGS_T_PlayerActor_Query* out_query
) {
	if (!out_query
		|| (out_query->struct_size && out_query->struct_size < sizeof(DTTR_PCDOGS_T_PlayerActor_Query))) {
		return false;
	}
	DTTR_PCDOGS_T_PlayerActor_Query query;
	memset(&query, 0, sizeof(query));
	query.struct_size = sizeof(query);
	query.api_version = 1;
	query.actor = candidate;
	query.source = candidate ? DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_CANDIDATE
						 : DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_NONE;
	query.confidence = candidate ? DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_LOW
							  : DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_NONE;
	query.mutable_now = candidate != NULL;
	query.retainable_after_callback = false;

	DTTR_PCDOGS_T_Actor_State* render_actor = NULL;
	if (DTTR_PCDOGS_D_PlayerActor->Read(&render_actor) && render_actor) {
		if (!query.actor) {
			query.actor = render_actor;
			query.source = DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_RENDER_GLOBAL;
			query.confidence = DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_LOW;
		} else if (query.actor == render_actor) {
			query.source = DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_RENDER_GLOBAL;
			query.confidence = DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_MEDIUM;
		}
	}

	DTTR_PCDOGS_T_Actor_State* active_actor = NULL;
	if (DTTR_PCDOGS_F_EntityGetActiveActor->Try(ctx, &active_actor) && active_actor) {
		if (!query.actor) {
			query.actor = active_actor;
			query.source = DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_ACTIVE_ACTOR;
			query.confidence = DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_MEDIUM;
		} else if (query.actor == active_actor) {
			query.source = DTTR_PCDOGS_PLAYER_ACTOR_SOURCE_ACTIVE_ACTOR;
			query.confidence = DTTR_PCDOGS_PLAYER_ACTOR_CONFIDENCE_HIGH;
		}
	}

	switch (call_site_function_id) {
	case DTTR_PCDOGS_FUNCTION_PLAYER_PROCESS_MOVEMENT:
	case DTTR_PCDOGS_FUNCTION_ACTOR_PROCESS_PLAYER_BEHAVIOR:
		query.mutable_now = query.actor != NULL;
		break;
	case DTTR_PCDOGS_FUNCTION_ACTOR_PROCESS_RENDERING:
		query.mutable_now = false;
		break;
	default:
		break;
	}

	*out_query = query;
	return query.actor != NULL;
}

static bool dttr_pcdogs_actor_scale_region_has(
	const DTTR_PCDOGS_T_Actor_State* actor,
	bool write
) {
	return actor
		   && dttr_pcdogs_region_has(
			   (uintptr_t)&actor->scale_xy,
			   sizeof(actor->scale_xy),
			   write,
			   false
		   )
		   && dttr_pcdogs_region_has(
			   (uintptr_t)&actor->scale_factor,
			   sizeof(actor->scale_factor),
			   write,
			   false
		   );
}

bool DTTR_PCDOGS_Actor_Scale_Read(
	const DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Actor_Scale* out_scale
) {
	if (!out_scale || !dttr_pcdogs_actor_scale_region_has(actor, false)) {
		return false;
	}
	out_scale->scale_xy = actor->scale_xy;
	out_scale->scale_factor = actor->scale_factor;
	return true;
}

bool DTTR_PCDOGS_Actor_Scale_Write(
	DTTR_PCDOGS_T_Actor_State* actor,
	DTTR_PCDOGS_T_Actor_Scale scale
) {
	if (!dttr_pcdogs_actor_scale_region_has(actor, true)) {
		return false;
	}
	actor->scale_xy = scale.scale_xy;
	actor->scale_factor = scale.scale_factor;
	return true;
}

DTTR_PCDOGS_T_Actor_Scale DTTR_PCDOGS_Actor_Scale_MultiplyClamped(
	DTTR_PCDOGS_T_Actor_Scale scale,
	int32_t multiplier
) {
	DTTR_PCDOGS_T_Actor_Scale scaled = {
		dttr_pcdogs_i32_from_i64_clamped((int64_t)scale.scale_xy * multiplier),
		dttr_pcdogs_i16_from_i64_clamped((int64_t)scale.scale_factor * multiplier),
	};
	return scaled;
}

bool DTTR_PCDOGS_Actor_Scale_PushMultiplied(
	DTTR_PCDOGS_T_Actor_State* actor,
	int32_t multiplier,
	DTTR_PCDOGS_T_Actor_Scale* out_original
) {
	DTTR_PCDOGS_T_Actor_Scale original;
	if (!dttr_pcdogs_actor_scale_region_has(actor, true)
		|| !DTTR_PCDOGS_Actor_Scale_Read(actor, &original)) {
		return false;
	}
	if (out_original) {
		*out_original = original;
	}
	return DTTR_PCDOGS_Actor_Scale_Write(
		actor,
		DTTR_PCDOGS_Actor_Scale_MultiplyClamped(original, multiplier)
	);
}

bool DTTR_PCDOGS_Actor_Scale_Restore(
	DTTR_PCDOGS_T_Actor_State* actor,
	const DTTR_PCDOGS_T_Actor_Scale* scale
) {
	return scale && DTTR_PCDOGS_Actor_Scale_Write(actor, *scale);
}

% endif
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
		if (value) {
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
static bool dttr_pcdogs_${row["name"]}_IsResolved() {
	return dttr_pcdogs_${row["name"]}_addr != 0;
}

static bool dttr_pcdogs_${row["name"]}_IsCallable(const DTTR_Core_Context* ctx) {
	return ${row["callable"]}
		   && dttr_pcdogs_function_address_valid(
			   ctx,
			   dttr_pcdogs_${row["name"]}_addr,
			   DTTR_PCDOGS_SIG(${row["signature"]}),
			   DTTR_PCDOGS_MASK(${row["signature"]}),
			   ${row["delta"]}
		   );
}

static uintptr_t dttr_pcdogs_${row["name"]}_Address() {
	return dttr_pcdogs_${row["name"]}_addr;
}

static DTTR_PCDOGS_T_Hook_Kind dttr_pcdogs_${row["name"]}_HookKind() {
	return ${row["hook_kind"]};
}

static uint32_t dttr_pcdogs_${row["name"]}_HookPrologueSize() {
	return ${row["hook_prologue_size"]};
}

static bool dttr_pcdogs_${row["name"]}_Try ${row["try_params"]} {
	if (!dttr_pcdogs_${row["name"]}_IsCallable(ctx)) {
		return false;
	}
% if row["ret"] == "void":
	((${row["typedef_name"]})dttr_pcdogs_${row["name"]}_addr)${row["args"]};
% else:
	if (!out_ret) {
		return false;
	}
	*out_ret = ((${row["typedef_name"]})dttr_pcdogs_${row["name"]}_addr)${row["args"]};
% endif
	return true;
}

static bool dttr_pcdogs_${row["name"]}_Hook(
	const DTTR_Core_Context* ctx,
	${row["typedef_name"]} detour,
	${row["typedef_name"]}* out_original
) {
	if (dttr_pcdogs_${row["name"]}_hook
		&& !dttr_pcdogs_hook_is_active(ctx, dttr_pcdogs_${row["name"]}_hook)) {
		dttr_pcdogs_${row["name"]}_hook = 0;
	}

	if (!dttr_pcdogs_${row["name"]}_IsCallable(ctx) || !detour
		|| dttr_pcdogs_${row["name"]}_hook || !ctx->api->hook_function
		|| ${row["hook_kind"]} != DTTR_PCDOGS_HOOK_REL32) {
		return false;
	}
	void* original = 0;
	dttr_pcdogs_${row["name"]}_hook = ctx->api->hook_function(
		dttr_pcdogs_${row["name"]}_addr,
		(int)${row["hook_prologue_size"]},
		detour,
		&original
	);
	if (!dttr_pcdogs_${row["name"]}_hook) {
		return false;
	}
	if (out_original) {
		*out_original = (${row["typedef_name"]})original;
	}
	return true;
}

% if not unstable:
static DTTR_PCDOGS_T_Patch_Spec dttr_pcdogs_${row["name"]}_PatchSpec(
	bool required,
	${row["typedef_name"]} detour,
	${row["typedef_name"]}* out_original
) {
	DTTR_PCDOGS_T_Patch_Spec spec_;
	memset(&spec_, 0, sizeof(spec_));
	if (${row["hook_kind"]} != DTTR_PCDOGS_HOOK_REL32) {
		spec_.required = required;
		return spec_;
	}
	spec_.kind = DTTR_PCDOGS_PATCH_FUNCTION_HOOK;
	spec_.required = required;
	spec_.function = ${row["function_id"]};
	spec_.detour = (void*)detour;
	spec_.out_original = (void**)out_original;
	return spec_;
}

% endif
static void dttr_pcdogs_${row["name"]}_Unhook(const DTTR_Core_Context* ctx) {
	if (!ctx || !ctx->api || !dttr_pcdogs_${row["name"]}_hook) {
		return;
	}

	if (dttr_pcdogs_unhook_checked(ctx, dttr_pcdogs_${row["name"]}_hook)) {
		dttr_pcdogs_${row["name"]}_hook = 0;
	}
}

% if row["ret"] != "void":
static ${row["ret"]} dttr_pcdogs_${row["name"]}_Call${row["call_or_params"]} {
	${row["ret"]} ret_ = fallback_ret;
	dttr_pcdogs_${row["name"]}_Try${row["call_or_args"]};
	return ret_;
}

% endif
static const struct ${row["accessor_struct_name"]} dttr_pcdogs_${row["name"]}_symbol = {
	.SymbolId = ${row["symbol_id"]},
% if not unstable:
	.FunctionId = ${row["function_id"]},
% endif
	.IsResolved = dttr_pcdogs_${row["name"]}_IsResolved,
	.IsCallable = dttr_pcdogs_${row["name"]}_IsCallable,
	.Address = dttr_pcdogs_${row["name"]}_Address,
	.HookKind = dttr_pcdogs_${row["name"]}_HookKind,
	.HookPrologueSize = dttr_pcdogs_${row["name"]}_HookPrologueSize,
	.Try = dttr_pcdogs_${row["name"]}_Try,
	.Hook = dttr_pcdogs_${row["name"]}_Hook,
% if not unstable:
	.PatchSpec = dttr_pcdogs_${row["name"]}_PatchSpec,
% endif
	.Unhook = dttr_pcdogs_${row["name"]}_Unhook,
% if row["ret"] != "void":
	.Call = dttr_pcdogs_${row["name"]}_Call,
% endif
};

const struct ${row["accessor_struct_name"]}* const DTTR_PCDOGS_F_${row["public"]} =
	&dttr_pcdogs_${row["name"]}_symbol;

% endfor
% for row in globals:
% if row.get("typed"):
static bool dttr_pcdogs_${row['name'].lower()}_IsResolved() {
	return dttr_pcdogs_${row['name'].lower()}_addr != 0;
}

static uintptr_t dttr_pcdogs_${row['name'].lower()}_Address() {
	return dttr_pcdogs_${row['name'].lower()}_addr;
}

static ${c_type(row['typed']['type'])}* dttr_pcdogs_${row['name'].lower()}_Ptr() {
	return (${c_type(row['typed']['type'])}*)dttr_pcdogs_${row['name'].lower()}_addr;
}

static bool dttr_pcdogs_${row['name'].lower()}_Read(${c_type(row['typed']['type'])}* out_value) {
	if (!out_value
		|| !dttr_pcdogs_region_has(
			dttr_pcdogs_${row['name'].lower()}_addr,
			sizeof(${c_type(row['typed']['type'])}),
			false,
			false
		)) {
		return false;
	}
	memcpy(out_value, (const void*)dttr_pcdogs_${row['name'].lower()}_addr, sizeof(${c_type(row['typed']['type'])}));
	return true;
}

static bool dttr_pcdogs_${row['name'].lower()}_UnsafeWrite(${c_type(row['typed']['type'])} value) {
	if (!dttr_pcdogs_region_has(
			dttr_pcdogs_${row['name'].lower()}_addr,
			sizeof(${c_type(row['typed']['type'])}),
			true,
			false
		)) {
		return false;
	}
	memcpy((void*)dttr_pcdogs_${row['name'].lower()}_addr, &value, sizeof(${c_type(row['typed']['type'])}));
	return true;
}

static bool dttr_pcdogs_${row['name'].lower()}_Write(${c_type(row['typed']['type'])} value) {
	if (${data_write_policy(row)} != DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY) {
		return false;
	}
	return dttr_pcdogs_${row['name'].lower()}_UnsafeWrite(value);
}

% if not unstable:
static DTTR_PCDOGS_T_Patch_Spec dttr_pcdogs_${row['name'].lower()}_PatchSpec(
	bool required,
	void* new_value,
	void** out_original
) {
	DTTR_PCDOGS_T_Patch_Spec spec_;
	memset(&spec_, 0, sizeof(spec_));
	spec_.kind = ${data_patch_spec_kind(row)};
	spec_.required = required;
	spec_.global = DTTR_PCDOGS_DATA_${row['symbol_id']};
	spec_.new_value = new_value;
	spec_.out_original = out_original;
	return spec_;
}

% endif
static const struct DTTR_PCDOGS_D_${c_pascal_token(row['name'])}_type dttr_pcdogs_${row['name'].lower()}_symbol = {
	.SymbolId = DTTR_PCDOGS_SYMBOL_DATA_ID_${row['symbol_id']},
% if not unstable:
	.DataId = DTTR_PCDOGS_DATA_${row['symbol_id']},
% endif
	.WritePolicy = ${data_write_policy(row)},
	.IsResolved = dttr_pcdogs_${row['name'].lower()}_IsResolved,
	.Address = dttr_pcdogs_${row['name'].lower()}_Address,
	.Ptr = dttr_pcdogs_${row['name'].lower()}_Ptr,
	.Read = dttr_pcdogs_${row['name'].lower()}_Read,
	.Write = dttr_pcdogs_${row['name'].lower()}_Write,
	.UnsafeWrite = dttr_pcdogs_${row['name'].lower()}_UnsafeWrite,
% if not unstable:
	.PatchSpec = dttr_pcdogs_${row['name'].lower()}_PatchSpec,
% endif
};

const struct DTTR_PCDOGS_D_${c_pascal_token(row['name'])}_type* const DTTR_PCDOGS_D_${c_pascal_token(row['name'])} =
	&dttr_pcdogs_${row['name'].lower()}_symbol;

% endif
% endfor


uint32_t DTTR_PCDOGS_FunctionCount() {
	return 0u
DTTR_PCDOGS_TYPED_FUNCTION_ROWS(DTTR_PCDOGS_COUNT_ONE)
	;
}
uint32_t DTTR_PCDOGS_DataCount() {
	return 0u
DTTR_PCDOGS_TYPED_DATA_ROWS(DTTR_PCDOGS_COUNT_ONE)
	;
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
	if (dttr_pcdogs_##ref_fn##_addr) {                                                 \
		dttr_pcdogs_##name##_addr = dttr_pcdogs_resolve_global_address(                \
			resolver,                                                                    \
			dttr_pcdogs_##ref_fn##_addr,                                               \
			instr_off,                                                                   \
			addr_off,                                                                    \
			indirections                                                                 \
		);                                                                               \
	}                                                                                    \
	if (!dttr_pcdogs_##name##_addr) {                                                  \
		all_ok = false;                                                                  \
	}
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


/// @}

#endif // ${header_guard}
