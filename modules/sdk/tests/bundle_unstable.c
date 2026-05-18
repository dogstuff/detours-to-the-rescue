#define DTTR_SDK_ENABLE_UNSTABLE
#include <dttr_sdk.h>

#include <stddef.h>

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
	char value = 0;
	DTTR_PCDOGS_T_Data_Write_Policy raw_policy = DTTR_PCDOGS_DATA_WRITE_POLICY_RAW_MEMORY;
	DTTR_PCDOGS_T_Data_Write_Policy read_only_policy = DTTR_PCDOGS_DATA_WRITE_POLICY_READ_ONLY;
	DTTR_PCDOGS_T_Data_Write_Policy engine_owned_policy
		= DTTR_PCDOGS_DATA_WRITE_POLICY_ENGINE_OWNED;
	DTTR_PCDOGS_T_Data_Write_Policy patch_only_policy
		= DTTR_PCDOGS_DATA_WRITE_POLICY_PATCH_ONLY;
	DTTR_PCDOGS_T_Data_Id data_id = DTTR_PCDOGS_D_Directory->DataId;
	DTTR_PCDOGS_T_Symbol_Data_Id symbol_id = DTTR_PCDOGS_D_Directory->SymbolId;
	if (data_id != DTTR_PCDOGS_DATA_DIRECTORY
		|| symbol_id != DTTR_PCDOGS_SYMBOL_DATA_ID_DIRECTORY) {
		return;
	}
	const DTTR_PCDOGS_T_Symbol_Data *data_meta = DTTR_PCDOGS_SymbolDataAt(
		(uint32_t)data_id
	);
	if (!data_meta || !(data_meta->supported_builds & DTTR_PCDOGS_BUILD_MASK_ALL)) {
		return;
	}
	DTTR_PCDOGS_D_Directory->Ptr();
	DTTR_PCDOGS_D_Directory->Read(&value);
	DTTR_PCDOGS_D_Directory->Write(value);
	DTTR_PCDOGS_D_Directory->UnsafeWrite(value);
	if (DTTR_PCDOGS_D_Directory->WritePolicy != raw_policy) {
		return;
	}

	if (DTTR_PCDOGS_D_LevelData->WritePolicy != raw_policy) {
		return;
	}

	if (DTTR_PCDOGS_D_CurrentLevelData->WritePolicy != engine_owned_policy) {
		return;
	}

	if (DTTR_PCDOGS_D_RenderListState->WritePolicy != engine_owned_policy) {
		return;
	}

	DTTR_PCDOGS_T_Patch_Spec current_level_data_patch = DTTR_PCDOGS_D_CurrentLevelData
															->PatchSpec(true, 0, 0);
	if (current_level_data_patch.kind != DTTR_PCDOGS_PATCH_DATA_POINTER_HOOK
		|| current_level_data_patch.global != DTTR_PCDOGS_DATA_CURRENT_LEVEL_DATA
		|| patch_only_policy == DTTR_PCDOGS_DATA_WRITE_POLICY_UNKNOWN) {
		return;
	}

	DTTR_PCDOGS_T_Patch_Spec scalar_patch = DTTR_PCDOGS_D_SpecialButton
												->PatchSpec(true, 0, 0);
	if (scalar_patch.kind != DTTR_PCDOGS_PATCH_UNSUPPORTED
		|| scalar_patch.global != DTTR_PCDOGS_DATA_SPECIAL_BUTTON) {
		return;
	}

	if (DTTR_PCDOGS_D_WindowLowMessageDispatchTable->WritePolicy != read_only_policy) {
		return;
	}

	if (DTTR_PCDOGS_D_ScriptSetVariableOpJumpTable->WritePolicy != read_only_policy) {
		return;
	}
}

// Compile opaque-actor scale helpers and stateless active-actor utilities.
static void compile_actor_helpers(
	const DTTR_Core_Context *ctx,
	DTTR_PCDOGS_T_Actor_State *actor
) {
	DTTR_PCDOGS_T_Actor_Scale scale = {0};
	DTTR_PCDOGS_Actor_Scale_Read(actor, &scale);
	scale = DTTR_PCDOGS_Actor_Scale_MultiplyClamped(scale, 2);
	DTTR_PCDOGS_Actor_Scale_Write(actor, scale);
	DTTR_PCDOGS_Actor_Scale_PushMultiplied(actor, 2, &scale);
	DTTR_PCDOGS_Actor_Scale_Restore(actor, &scale);

	DTTR_PCDOGS_T_Vec3i32 vec = {0};
	DTTR_PCDOGS_Actor_ReadPosition(actor, &vec);
	DTTR_PCDOGS_Actor_WritePosition(actor, vec);
	DTTR_PCDOGS_Actor_WritePositionAndRenderMirror(actor, vec);
	DTTR_PCDOGS_Actor_ReadVelocity(actor, &vec);
	DTTR_PCDOGS_Actor_WriteVelocity(actor, vec);

	int32_t velocity[3] = {0};
	int16_t normal[3] = {0};
	int16_t contact[3] = {0};
	int32_t result = 0;
	DTTR_PCDOGS_T_Collision3D_Payload collision_payload = {
		.struct_size = sizeof(collision_payload),
	};
	DTTR_PCDOGS_Collision3D_ReadPayload(
		actor,
		velocity,
		normal,
		contact,
		&result,
		&collision_payload
	);

	DTTR_PCDOGS_T_PlayerActor_Query query = {
		.struct_size = sizeof(query),
	};
	DTTR_PCDOGS_PlayerActor_QueryForHook(
		ctx,
		DTTR_PCDOGS_FUNCTION_PLAYER_PROCESS_MOVEMENT,
		actor,
		&query
	);

	DTTR_PCDOGS_T_Actor_State *active_actor = DTTR_Util_GetActiveActor(ctx);
	DTTR_Util_SameActor(actor, active_actor);
}

// Compile camera helpers against opaque stable camera pointers.
static void compile_camera_helpers(DTTR_PCDOGS_T_Camera_Runtime *camera) {
	int16_t angle = 0;
	DTTR_PCDOGS_Camera_ReadMovementYaw(camera, &angle);
	DTTR_PCDOGS_Camera_ReadLookAtPitch(camera, &angle);
	DTTR_PCDOGS_Camera_ReadOrbitYaw(camera, &angle);
	DTTR_PCDOGS_Camera_ReadFov(camera, &angle);
	DTTR_PCDOGS_T_Camera_RuntimePose pose = {
		.struct_size = sizeof(pose),
	};
	DTTR_PCDOGS_Camera_ReadPose(camera, &pose);
	DTTR_PCDOGS_Camera_WritePose(camera, &pose);
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
	actor.scale_xy = 0;
	actor.scale_factor = 0;
}

enum {
	unstable_actor_state_size_check = 1 / (sizeof(DTTR_PCDOGS_T_Actor_State) >= 0x1C4),
	unstable_actor_state_scale_xy_offset_check = 1
												 / (offsetof(
														DTTR_PCDOGS_T_Actor_State,
														scale_xy
													)
													== 0x68),
	unstable_actor_state_scale_factor_offset_check = 1
													 / (offsetof(
															DTTR_PCDOGS_T_Actor_State,
															scale_factor
														)
														== 0xCE),
	unstable_level_data_const_dispatch_type_check
	= 1
	  / __builtin_types_compatible_p(
		  __typeof__(DTTR_PCDOGS_LevelData_AsRuntimeData(
			  (const DTTR_PCDOGS_T_Level_Data *)0
		  )),
		  const DTTR_PCDOGS_T_Level_RuntimeData *
	  ),
	unstable_level_runtime_data_const_dispatch_type_check
	= 1
	  / __builtin_types_compatible_p(
		  __typeof__(DTTR_PCDOGS_LevelData_AsRuntimeData(
			  (const DTTR_PCDOGS_T_Level_RuntimeData *)0
		  )),
		  const DTTR_PCDOGS_T_Level_RuntimeData *
	  ),
	unstable_powerup_actor_slot_type_check = 1
											 / __builtin_types_compatible_p(
												 __typeof__(((DTTR_PCDOGS_T_Level_RuntimeData
																  *)0)
																->powerup_actor_slot_0),
												 DTTR_PCDOGS_T_Actor_State *
											 ),
	unstable_mesh_relative_offset_list_offset_check
	= 1 / (offsetof(DTTR_PCDOGS_T_Pkg_MeshNodeHeader, relative_offset_list_ptr) == 0x100),
};
