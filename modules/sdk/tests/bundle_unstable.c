#define DTTR_SDK_ENABLE_UNSTABLE
#include <dttr_sdk.h>

#include <stddef.h>

#include <pcdogs_layout_asserts.h>

#ifndef DTTR_PCDOGS_UNSTABLE_H
#error "unstable declarations missing"
#endif

#ifndef DTTR_UTIL_UNSTABLE_H
#error "unstable utility declarations missing"
#endif

DTTR_MODS_INFO("compile-check", "0.0.0", "sdk-tests")

// Provide a detour with the stable movie hook signature for compile checks.
static BOOL __cdecl compile_movie_playfile_detour(
	char const *moviePath,
	char useAltVideoRect
) {
	return 0;
}

static int32_t __cdecl compile_powerup_update_spawn_logic_detour() { return 0; }

static int32_t __cdecl compile_render_polygon_batch_detour(
	DTTR_PCDOGS_T_Scene_Node *node,
	DTTR_PCDOGS_T_Render_PolygonRenderRef *polygon_refs,
	int32_t count
) {
	return count;
}

// Compile stable generated hook helpers and typed call helpers.
static void compile_hook_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_MoviePlayFile_proto original = 0;
	DTTR_PCDOGS_T_Function_Id function_id = DTTR_PCDOGS_F_MoviePlayFile->FunctionId;
	DTTR_PCDOGS_T_Symbol_Function_Id symbol_id = DTTR_PCDOGS_F_MoviePlayFile->SymbolId;
	if (function_id != DTTR_PCDOGS_FUNCTION_MOVIE_PLAY_FILE
		|| symbol_id != DTTR_PCDOGS_SYMBOL_FUNCTION_ID_MOVIE_PLAY_FILE) {
		return;
	}

	const DTTR_PCDOGS_T_Symbol_Function *function_meta = DTTR_PCDOGS_SymbolFunctionAt(
		(uint32_t)function_id
	);
	if (!function_meta
		|| !(function_meta->supported_builds & DTTR_PCDOGS_BUILD_MASK_ALL)) {
		return;
	}

	DTTR_PCDOGS_F_MoviePlayFile->Hook(ctx, compile_movie_playfile_detour, &original);
	DTTR_PCDOGS_F_MoviePlayFile->Unhook(ctx);

	DTTR_PCDOGS_F_PowerupUpdateSpawnLogic_proto powerup_original = 0;
	DTTR_PCDOGS_F_PowerupUpdateSpawnLogic
		->Hook(ctx, compile_powerup_update_spawn_logic_detour, &powerup_original);
	DTTR_PCDOGS_F_PowerupUpdateSpawnLogic->Unhook(ctx);

	DTTR_PCDOGS_F_RenderPolygonBatch_proto render_batch_original = 0;
	DTTR_PCDOGS_F_RenderPolygonBatch
		->Hook(ctx, compile_render_polygon_batch_detour, &render_batch_original);
	DTTR_PCDOGS_F_RenderPolygonBatch->Unhook(ctx);
}

// Compile typed patch specs and fixed-array patch-group install helpers.
static void compile_patch_spec_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_MoviePlayFile_proto original = 0;
	DTTR_Core_PatchGroup *group = 0;
	DTTR_PCDOGS_T_Patch_Report report = {0};
	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_F_MoviePlayFile
			->PatchSpec(true, compile_movie_playfile_detour, &original),
	};

	DTTR_PCDOGS_INSTALL_PATCHES(ctx, specs, &group, &report);
	DTTR_Core_PatchGroupRelease(&group);
}

// Provide a detour with the unstable generated hook signature for compile checks.
static void __cdecl compile_unstable_detour() {}

// Compile unstable generated hook helpers without executing them.
static void compile_unstable_hook_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_StubNoOp_proto original = 0;
	DTTR_PCDOGS_F_StubNoOp->Hook(ctx, compile_unstable_detour, &original);
	DTTR_PCDOGS_F_StubNoOp->Unhook(ctx);
}

// Compile generated global read, write, and policy helpers.
static void compile_global_helpers() {
	char value[DTTR_PCDOGS_D_PKG_BASE_PATH_COUNT] = {0};
	DTTR_PCDOGS_T_Data_Id data_id = DTTR_PCDOGS_D_PkgBasePath->DataId;
	DTTR_PCDOGS_T_Symbol_Data_Id symbol_id = DTTR_PCDOGS_D_PkgBasePath->SymbolId;
	if (data_id != DTTR_PCDOGS_DATA_PKG_BASE_PATH
		|| symbol_id != DTTR_PCDOGS_SYMBOL_DATA_ID_PKG_BASE_PATH) {
		return;
	}

	const DTTR_PCDOGS_T_Symbol_Data *data_meta = DTTR_PCDOGS_SymbolDataAt(
		(uint32_t)symbol_id
	);
	if (!data_meta || !(data_meta->supported_builds & DTTR_PCDOGS_BUILD_MASK_ALL)) {
		return;
	}

	DTTR_PCDOGS_D_PkgBasePath->Ptr();
	DTTR_PCDOGS_D_PkgBasePath->Read(&value);
	DTTR_PCDOGS_D_PkgBasePath->Write(&value);
	DTTR_PCDOGS_D_PkgBasePath->UnsafeWrite(&value);
	if (DTTR_PCDOGS_D_PkgBasePath->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY) {
		return;
	}

	if (DTTR_PCDOGS_D_DynamicLevelScale->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY) {
		return;
	}

	if (DTTR_PCDOGS_D_CurrentLevelData->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED) {
		return;
	}

	if (DTTR_PCDOGS_D_RenderListState->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED) {
		return;
	}

	DTTR_PCDOGS_T_Patch_Spec current_level_data_patch = DTTR_PCDOGS_D_CurrentLevelData
															->PatchSpec(true, 0, 0);
	if (current_level_data_patch.kind != DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK
		|| current_level_data_patch.global != DTTR_PCDOGS_DATA_CURRENT_LEVEL_DATA) {
		return;
	}

	if (DTTR_PCDOGS_D_WindowLowMessageDispatchTable->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY) {
		return;
	}

	if (DTTR_PCDOGS_D_ScriptSetVariableOpJumpTable->WritePolicy
		!= DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY) {
		return;
	}
}

// Compile stateless active-actor utilities.
static void compile_actor_helpers(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Actor_State *actor
) {
	DTTR_PCDOGS_T_Actor_State *active_actor = DTTR_Util_GetActiveActor(ctx);
	DTTR_Util_SameActor(actor, active_actor);
}

// Compile mod-context examples against the runtime context nested in DTTR_Mods_Context.
static bool compile_mod_context_runtime_helpers(const DTTR_Mods_Context *ctx) {
	return DTTR_PCDOGS_F_MoviePlayFile->IsCallable(&ctx->runtime);
}

static void unstable_pkg_walk_compile_check(const DTTR_Core_Context *ctx) {
	DTTR_Util_PkgWalkOptions options = DTTR_Util_PkgWalk_DefaultOptions();
	DTTR_Util_PkgWalk(ctx, &options, 0, 0);
}

static void unstable_struct_layout_compile_check() {
	DTTR_PCDOGS_T_Actor_State actor = {0};
	actor.visual_scale.x = 0;
	actor.animation_component_state.animation_step = 0;
	actor.scale_factor = 0;
}

enum {
	unstable_actor_state_size_check = 1 / (sizeof(DTTR_PCDOGS_T_Actor_State) >= 0x1C4),
	unstable_level_data_const_dispatch_type_check
	= 1
	  / __builtin_types_compatible_p(
		  __typeof__(DTTR_Util_LevelDataAsRuntimeDataConst(
			  (const DTTR_PCDOGS_T_Level_Data *)0
		  )),
		  const DTTR_PCDOGS_T_Level_RuntimeData *
	  ),
	unstable_level_runtime_data_const_dispatch_type_check
	= 1
	  / __builtin_types_compatible_p(
		  __typeof__(DTTR_Util_LevelDataAsRuntimeDataConst(
			  (const DTTR_PCDOGS_T_Level_RuntimeData *)0
		  )),
		  const DTTR_PCDOGS_T_Level_RuntimeData *
	  ),
	unstable_powerup_actor_slot_type_check = 1
											 / __builtin_types_compatible_p(
												 __typeof__(((DTTR_PCDOGS_T_Level_RuntimeData
																  *)0)
																->powerup_actor_slots[0]),
												 DTTR_PCDOGS_T_Pkg_ActorTemplate *
											 ),
};
