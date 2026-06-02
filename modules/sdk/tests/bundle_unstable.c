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
	DTTR_PCDOGS_T_Graphics_PolygonRenderRef *polygon_refs,
	int32_t count
) {
	return count;
}

// Compile stable generated hook helpers and typed call helpers.
static void compile_hook_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_Video_PlayMovieFile_proto original = 0;
	DTTR_PCDOGS_T_Function_Id function_id = DTTR_PCDOGS_F_Video_PlayMovieFile->FunctionId;
	DTTR_PCDOGS_T_Symbol_Function_Id symbol_id = DTTR_PCDOGS_F_Video_PlayMovieFile
													 ->SymbolId;
	if (function_id != DTTR_PCDOGS_FUNCTION_VIDEO_PLAY_MOVIE_FILE
		|| symbol_id != DTTR_PCDOGS_SYMBOL_FUNCTION_ID_VIDEO_PLAY_MOVIE_FILE) {
		return;
	}

	const DTTR_PCDOGS_T_Symbol_Function *function_meta = DTTR_PCDOGS_SymbolFunctionAt(
		(uint32_t)function_id
	);
	if (!function_meta
		|| !(function_meta->supported_builds & DTTR_PCDOGS_BUILD_MASK_ALL)) {
		return;
	}

	DTTR_PCDOGS_F_Video_PlayMovieFile->Hook(ctx, compile_movie_playfile_detour, &original);
	DTTR_PCDOGS_F_Video_PlayMovieFile->Unhook(ctx);
	BOOL played = 0;
	DTTR_PCDOGS_F_Video_PlayMovieFile->Call(ctx, "intro.avi", 0, &played);
	DTTR_PCDOGS_F_Video_PlayMovieFile->Status(ctx);

	DTTR_PCDOGS_F_Powerup_UpdateSpawnLogic_proto powerup_original = 0;
	DTTR_PCDOGS_F_Powerup_UpdateSpawnLogic
		->Hook(ctx, compile_powerup_update_spawn_logic_detour, &powerup_original);
	DTTR_PCDOGS_F_Powerup_UpdateSpawnLogic->Unhook(ctx);

	DTTR_PCDOGS_F_Graphics_RenderPolygonBatch_proto render_batch_original = 0;
	DTTR_PCDOGS_F_Graphics_RenderPolygonBatch
		->Hook(ctx, compile_render_polygon_batch_detour, &render_batch_original);
	DTTR_PCDOGS_F_Graphics_RenderPolygonBatch->Unhook(ctx);
}

// Compile typed patch specs and fixed-array patch-group install helpers.
static void compile_patch_spec_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_Video_PlayMovieFile_proto original = 0;
	DTTR_Core_PatchGroup *group = 0;
	DTTR_PCDOGS_T_Patch_Report report = {0};
	const DTTR_PCDOGS_T_Patch_Spec specs[] = {
		DTTR_PCDOGS_F_Video_PlayMovieFile
			->PatchSpec(true, compile_movie_playfile_detour, &original),
	};

	DTTR_PCDOGS_INSTALL_PATCHES(ctx, specs, &group, &report);
	DTTR_Core_PatchGroupRelease(&group);
}

// Provide a detour with the unstable generated hook signature for compile checks.
static void __cdecl compile_unstable_detour() {}

// Compile unstable generated hook helpers without executing them.
static void compile_unstable_hook_helpers(const DTTR_Core_Context *ctx) {
	DTTR_PCDOGS_F_Debug_RunNoOpStub_proto original = 0;
	DTTR_PCDOGS_F_Debug_RunNoOpStub->Hook(ctx, compile_unstable_detour, &original);
	DTTR_PCDOGS_F_Debug_RunNoOpStub->Unhook(ctx);
}

// Compile generated global read, write, and policy helpers.
static void compile_global_helpers() {
	char value[DTTR_PCDOGS_D_AUDIO_OPEN_STREAM_PKG_BASE_PATH_COUNT] = {0};
	DTTR_PCDOGS_T_Data_Id data_id = DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->DataId;
	DTTR_PCDOGS_T_Symbol_Data_Id symbol_id = DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath
												 ->SymbolId;
	if (data_id != DTTR_PCDOGS_DATA_AUDIO_OPEN_STREAM_PKG_BASE_PATH
		|| symbol_id != DTTR_PCDOGS_SYMBOL_DATA_ID_AUDIO_OPEN_STREAM_PKG_BASE_PATH) {
		return;
	}

	const DTTR_PCDOGS_T_Symbol_Data *data_meta = DTTR_PCDOGS_SymbolDataAt(
		(uint32_t)symbol_id
	);
	if (!data_meta || !(data_meta->supported_builds & DTTR_PCDOGS_BUILD_MASK_ALL)) {
		return;
	}

	DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Ptr();
	DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Read(&value);
	DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Write(&value);
	DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->UnsafeWrite(&value);
	DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Status();
	if (DTTR_PCDOGS_D_Audio_OpenStream_PKGBasePath->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_RAW_MEMORY) {
		return;
	}

	if (DTTR_PCDOGS_D_Camera_UpdateFollow_DynamicLevelScale->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_RAW_MEMORY) {
		return;
	}

	if (DTTR_PCDOGS_D_Audio_TriggerMusicTransition_PKGResourceCurrentLevelData->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_ENGINE_MANAGED) {
		return;
	}

	if (DTTR_PCDOGS_D_Graphics_AdjustLevelScale_ListState->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_ENGINE_MANAGED) {
		return;
	}

	DTTR_PCDOGS_T_Patch_Spec current_level_data_patch
		= DTTR_PCDOGS_D_Audio_TriggerMusicTransition_PKGResourceCurrentLevelData
			  ->PatchSpec(true, 0, 0);
	if (current_level_data_patch.kind != DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK
		|| current_level_data_patch.global
			   != DTTR_PCDOGS_DATA_AUDIO_TRIGGER_MUSIC_TRANSITION_PKG_RESOURCE_CURRENT_LEVEL_DATA) {
		return;
	}

	if (DTTR_PCDOGS_D_Window_ProcessGameProc_LowMessageDispatchTable->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_READ_ONLY) {
		return;
	}

	if (DTTR_PCDOGS_D_Script_OpSetVariable_OpJumpTable->Policy()
		!= DTTR_PCDOGS_WRITE_POLICY_READ_ONLY) {
		return;
	}

	int32_t player_current_level_id = -1;
	int16_t menu_level_index = -1;
	DTTR_PCDOGS_D_Player_ProcessMovement_CurrentLevelID->Read(&player_current_level_id);
	DTTR_PCDOGS_D_Menu_ProcessMenuTransition_LevelIndex->Read(&menu_level_index);
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
	return DTTR_PCDOGS_F_Video_PlayMovieFile->IsCallable(&ctx->runtime);
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
												 DTTR_PCDOGS_T_PKG_ActorTemplate *
											 ),
};
