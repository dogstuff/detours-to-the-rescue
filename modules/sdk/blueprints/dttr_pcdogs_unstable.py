#!/usr/bin/env python3
from __future__ import annotations

from blueprint import (
    UNKNOWN_PARAMS,
    Blueprint,
    CallingConvention,
    HookKind,
    Required,
    hook,
    member,
    param,
    xref,
)

unstable = Blueprint("unstable", unstable=True)

unstable.fn(
    "Component_UpdateProjectileLogic",
    "10 85 C0 74 ?? 57 E8 ??",
    match=-52,
    hook=6,
    ret="Component_SpawnParams*",
    params=[param("Component_Instance*", "comp")],
    doc=(
        "Projectile component update logic over Component_Instance projectile_state, "
        "projectile_timer, homing velocity fields, owner actor references, and spawn context."
    ),
)

unstable.fn(
    "Collision_ProcessPowerupCollisions",
    "83 EC 1C A1 ?? ??",
    hook=8,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Scans the powerup actor list against actor and dispatches powerup_collision_handler as "
        "(powerup_actor, actor, 0, -2). Unconsumed pairs may fall through to swept/sphere distance "
        "tests and Collision_ResolveActorToActorCollision(actor, powerup_actor, -1, 0)."
    ),
)

unstable.fn(
    "Actor_HandleCollisionResponse",
    "00 00 83 FF 06 0F 87 ??",
    match=-23,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_slot"),
    ],
    doc=(
        "Subtype collision-response helper called by Actor_ProcessCollisionResponse; it updates "
        "response vectors, normal fields, and selected actor record slots from the caller's "
        "Collision_Polygon pointer, selected collision slot, and actor record state."
    ),
)

unstable.fn(
    "Powerup_UpdateSpawnLogic",
    "83 EC 0C A1 ?? ??",
    hook=8,
    ret="int32_t",
    params=[],
    doc=(
        "Walks current Level_RuntimeData.powerup_list definitions and updates the fixed powerup "
        "actor-template/clone-source slots; recovered code shows 0x1C-stride Powerup_Entry records "
        "with 0x10 spawn-pending, 0x20 slot-15 selection, and 0x40/0x42 spawn-blocking flags."
    ),
)


# Internal resolver rows required by unstable typed data that is resolved through
# stable functions. These stay out of the unstable public accessor surface.
unstable.fn(
    "Movie_PlayIntro",
    "56 8B 74 24 08 C7 05 ??",
    public=False,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "movie_index",
            doc="Index into the four-entry startup movie filename/type tables.",
        )
    ],
    doc="Internal resolver row for movie_file_names; stable keeps the public callable wrapper.",
)

unstable.fn(
    "Render_AdjustLevelScale",
    "A1 ?? ?? ?? ?? 85 C0 7C",
    public=False,
    ret="int32_t",
    params=[
        param(
            "float",
            "measured_fps",
            doc="Averaged frame rate measured by Render_Frame before adjusting the level/render scale.",
        )
    ],
    doc="Internal resolver row for render_list_state; stable keeps the public callable wrapper.",
)

unstable.data(
    "movie_file_names",
    xref("Movie_PlayIntro", 15, 3),
    type="char*",
    doc="First entry/base of the four-entry movie filename pointer table used by intro and movie playback routines.",
)

unstable.data(
    "render_list_state",
    xref("Render_AdjustLevelScale", 101, 2),
    type="Render_ListState*",
    doc="Data pointer to active Render_ListState; Render_AdjustLevelScale writes dynamic level scale at +0xB8 (PC EN).",
)

unstable.struct(
    "Render_ListState",
    member("int16_t", "yaw", 0),
    member("int16_t", "pitch", 2),
    member("int16_t", "roll", 4),
    member("int16_t", "look_at_pitch", 6),
    member("int16_t", "orbit_yaw", 8),
    member("int16_t", "fov", 10),
    member("int32_t", "focal_distance", 12),
    member("int32_t", "eye_pos_x", 16),
    member("int32_t", "eye_pos_z", 20),
    member("int32_t", "eye_pos_y", 24),
    member("int32_t", "target_x", 28),
    member("int32_t", "target_z", 32),
    member("int32_t", "target_y", 36),
    member("Entity_State*", "active_entity_slot_ptr", 40),
    member("int16_t", "screen_half_w", 44),
    member("int16_t", "screen_half_h", 46),
    member("Math_Matrix3x3i16", "view_matrix", 48),
    member("int16_t", "view_matrix_padding", 66),
    member(
        "uint8_t",
        "frustum_setup_prefix_44[20]",
        68,
        doc="Camera/frustum setup prefix before the five validated 12-byte clip-plane records.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_plane_0",
        88,
        doc="Plane record written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_plane_1",
        100,
        doc="Plane record written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_plane_2",
        112,
        doc="Plane record written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_plane_3",
        124,
        doc="Plane record written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_plane_4",
        136,
        doc="Plane record written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member("Math_Matrix3x3i16", "node_view_matrix", 148),
    member("int16_t", "node_view_matrix_padding", 166),
    member("int32_t", "node_view_translation_x", 168),
    member("int32_t", "node_view_translation_y", 172),
    member("int32_t", "node_view_translation_z", 176),
    member("int32_t", "projection_near_fp", 180),
    member("int32_t", "dynamic_level_scale", 184),
    member("Actor_State*", "render_actor_ptr", 188),
    member("uint32_t", "render_pass_flags", 192),
    member("void*", "post_sorted_callback", 196),
    member("void*", "pre_shadow_callback", 200),
    member("void*", "sorted_list_head", 204),
    member("void*", "sorted_list_buckets[16384]", 208),
    member(
        "uint32_t",
        "sorted_bucket_tail",
        65744,
        doc="End/tail dword after the 16384 sorted bucket pointers.",
    ),
    size=65748,
    doc="Validated camera/render-list runtime state with five 12-byte frustum planes.",
)

unstable.fn(
    "CRT_CodecvtAlwaysNoConversion",
    "B0 01 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 81 EC 10 01 00 00 57 68 ?? ?? ?? ?? 68 04 01 00 00 FF 15 ?? ?? ?? ??",
    required=Required.EN,
    hook=hook(0, kind=HookKind.UNSUPPORTED),
    callable=False,
    ret="uint8_t",
    params=[],
    doc="Unsupported C++ runtime std::codecvt_base::do_always_noconv helper returning the native true result.",
)

unstable.fn(
    "Stub_NoOp",
    "C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 44 24 04 A3 ?? ?? ?? ??",
    hook=hook(0, kind=HookKind.UNSUPPORTED),
    callable=False,
    ret="void",
    params=[],
    doc="Unsupported nullsub/no-op target: single RET, no meaningful return value.",
)

unstable.fn(
    "CRT_ProbeRead4",
    "25 ?? ?? ?? ?? 83 EC 10",
    match=-24,
    cc=CallingConvention.STDCALL,
    hook=hook(0, kind=HookKind.UNSUPPORTED),
    callable=False,
    ret="int32_t",
    params=[param("void const*", "address")],
    doc="MSVC __rt_probe_read4 helper reached from _longjmp and used for guarded 4-byte reads.",
)

# Lower-confidence structured types: resolved, with layout confidence below the
# stable modder-facing surface threshold.
unstable.struct(
    "Level_State",
    member("void*", "collision_data", 0),
    member("Actor_State*", "actor_list", 4),
    member("void*", "trigger_list", 8),
    member("Pkg_CameraDef*", "camera_data", 12),
    member("Material_Entry*", "material_table", 16),
    member("uint32_t", "reserved_01", 20),
    member("uint32_t", "flags", 24),
    member("uint16_t", "actor_count", 28),
    member("uint16_t", "material_count", 30),
    member("char*", "string_table", 32),
    size=36,
)

unstable.struct(
    "Level_RuntimeData",
    member("Camera_Runtime*", "cam_default", 0),
    member("Camera_Runtime*", "cam_current", 4),
    member("int16_t", "current_entity_index", 8),
    member(
        "int16_t",
        "entity_count",
        10,
        doc=(
            "Number of valid level-local runtime entity slots in entity_array for the "
            "currently loaded level."
        ),
    ),
    member(
        "Entity_State*",
        "entity_array",
        12,
        doc=(
            "Array of level-local runtime entity slots. Pointers into this array "
            "identify entity spawn slots within the currently loaded level."
        ),
    ),
    member(
        "Audio_SoundDefinition*",
        "sound_definition_list",
        16,
        doc="Current-level sound-definition array, indexed by level-local sound operands.",
    ),
    member("int16_t", "sound_definition_count", 20),
    member("int16_t", "var_count", 22),
    member("int32_t*", "var_list", 24),
    member("int16_t", "powerup_count", 28),
    member("int16_t", "powerup_type_count", 30),
    member(
        "Powerup_Entry*",
        "powerup_list",
        32,
        doc="0x1c-stride current-level powerup spawn-record list keyed by powerup_count.",
    ),
    member(
        "Pkg_ActorTemplate*",
        "powerup_actor_slots[16]",
        36,
        doc=(
            "Fixed 16-slot powerup actor-template/clone-source table. "
            "Resource_FixUpLevelPointers fixes each non-null slot with Resource_FixUpActorPointers; "
            "Powerup_CloneActor reads these Pkg_ActorTemplate* sources when creating spawned powerup actors."
        ),
    ),
    member("char*", "theme_0", 100),
    member("char*", "theme_1", 104),
    member("char*", "theme_2", 108),
    member("char*", "theme_3", 112),
    member("char*", "theme_4", 116),
    member("int32_t", "theme_count", 120),
    member("Trail_Entry*", "trail_list", 124),
    member("Pkg_SpriteEntry*", "sprite_list", 128),
    member("Nav_Network*", "nav_net", 132),
    member("Material_Entry*", "usable_materials", 136),
    size=140,
    doc="Concrete runtime level-data block carried by Level_Data* APIs.",
)

unstable.struct(
    "Powerup_Entry",
    member("Pkg_ActorRecord*", "template_record", 0),
    member("int16_t", "spawn_params_a", 4),
    member("int16_t", "spawn_params_b", 6),
    member(
        "int32_t",
        "runtime_value_08",
        8,
        doc="Powerup-list dword whose low 16 bits are used by attached/local-position paths.",
    ),
    member("uint8_t", "powerup_type", 12),
    member("uint8_t", "flags", 13),
    member("int16_t", "max_spawn_count", 14),
    member("int32_t", "pos_x", 16),
    member("int32_t", "pos_y", 20),
    member("int32_t", "pos_z", 24),
    size=28,
    doc="0x1c-stride Level_RuntimeData.powerup_list entry walked by Powerup_UpdateSpawnLogic with template_record, flags, max_spawn_count, pos_x, pos_y, and pos_z fields.",
)

unstable.struct(
    "Material_EntryFull",
    member("uint8_t", "flags", 0),
    member("uint8_t", "reserved_01", 1),
    member("uint16_t", "reserved_02", 2),
    member("DDraw_IDirectDrawSurface7*", "texture_data_ptr", 4),
    member("uint8_t*", "palette_data_ptr", 8),
    member("uint16_t", "width", 12),
    member("uint16_t", "height", 14),
    member("uint16_t", "format", 16),
    member("uint16_t", "reserved_03", 18),
    member("DDraw_IDirectDrawSurface7*", "d3d_texture", 20),
    member("uint32_t", "texture_handle", 24),
    member("uint32_t", "ref_count", 28),
    member("uint32_t", "reserved_04", 32),
    size=36,
    doc="Expanded/runtime material-entry form with the 20-byte descriptor fields plus runtime DirectDraw/D3D texture handles.",
)

unstable.struct(
    "Material_SetRuntime",
    member("Material_Entry*", "entries", 0),
    member("uint16_t", "count", 4),
    member("uint16_t", "reserved_06", 6),
    member("uint32_t", "flags", 8),
    member("Material_FrameData*", "material_data_array", 12),
    member("uint32_t", "reserved_10", 16),
    size=20,
)

unstable.struct(
    "Mesh_TransformEntry",
    member("uint8_t", "type", 0),
    member("uint8_t", "flags", 1),
    member("char", "bone_index", 2),
    member(
        "uint8_t",
        "signal_id_hi",
        3,
        doc="High byte of the generic mesh-command signal_id word, with no isolated stable semantic for transform-specific consumers.",
    ),
    member("uint32_t", "resource_ptr", 4),
    member("int16_t", "poly_start_index", 8),
    member("int16_t", "poly_count", 10),
    member(
        "int16_t",
        "payload_word_0_c",
        12,
        doc=(
            "Variant mesh-command payload start; type 0 passes cmd+0x0C (PC EN) to "
            "Animation_ProcessController, while other command types reinterpret the payload."
        ),
    ),
    member("int16_t", "effect_count", 14),
    member("int16_t", "scale_x", 16),
    member("int16_t", "scale_y", 18),
    size=20,
)

unstable.struct(
    "Movie_PlaybackBuffer",
    member("uint8_t", "decoder_state[396]", 0),
    member("char", "movie_alias[64]", 396),
    member("uint8_t", "decode_scratch[64]", 460),
    member("uint8_t", "frame_pixel_data[572]", 524),
    member("int32_t*", "callback_context", 1096),
    size=1100,
)

unstable.struct(
    "Pkg_ActorRecord_UnstableLayout",
    member("uint32_t", "flags", 0),
    member("int32_t", "camera_pos_x", 4),
    member("int32_t", "camera_pos_y", 8),
    member("int32_t", "camera_pos_z", 12),
    member("int32_t", "collision_radius_sq", 16),
    member("int32_t", "collision_height_sq", 20),
    member("uint8_t", "active_flag", 24),
    member("uint8_t", "respawn_mode", 25),
    member("uint16_t", "padding_1a", 26),
    member("Level_RuntimeData*", "level_data", 28),
    member("int16_t", "self_index", 32),
    member("int16_t", "default_anim_state", 34),
    member("int32_t", "link_targets[9]", 36),
    member("Pkg_ScriptHeader*", "script_data_ptr", 72),
    member("uint32_t", "actor_template_ptr", 76),
    member("uint32_t", "component_node_ptr_0", 80),
    member("uint32_t", "component_node_ptr_1", 84),
    member("Pkg_ActorRecord_UnstableLayout*", "spawn_state_ptr", 88),
    member("int32_t", "default_ref_pos_x", 92),
    member("int32_t", "default_ref_pos_y", 96),
    member("int32_t", "default_ref_pos_z", 100),
    member("int32_t", "default_home_x", 104),
    member("int32_t", "default_home_y", 108),
    member("int32_t", "default_direction", 112),
    member("int32_t", "default_direction_2", 116),
    member("int32_t", "default_movement_param_0", 120),
    member("int32_t", "default_movement_param_1", 124),
    member("int32_t", "default_movement_param_2", 128),
    member("int32_t", "default_movement_param_3", 132),
    member("int32_t", "default_facing_angle", 136),
    member("int32_t", "default_rotation", 140),
    member("int32_t", "default_anim_param_0", 144),
    member("int32_t", "default_anim_param_1", 148),
    member("int16_t", "default_anim_param_2", 152),
    member("uint8_t", "default_direction_mode", 154),
    member("uint8_t", "default_anim_byte_3", 155),
    member("int32_t", "default_speed", 156),
    member(
        "int32_t",
        "default_scale",
        160,
        doc=(
            "Authored/default visual scale slot used by record-style actor initialization paths."
        ),
    ),
    member("int32_t", "default_prop_4", 164),
    member("int32_t", "default_prop_5", 168),
    member(
        "int32_t",
        "default_size",
        172,
        doc=(
            "Authored/default size slot used by record-style actor initialization paths."
        ),
    ),
    member("int32_t", "default_prop_7", 176),
    member("int32_t", "default_extra_0", 180),
    member("int32_t", "default_extra_1", 184),
    member("int32_t", "default_extra_2", 188),
    member("Camera_EntityView*", "entity_ref_pos_ptr", 192),
    member("int32_t", "live_ref_pos_x", 196),
    member("int32_t", "live_ref_pos_y", 200),
    member("int32_t", "live_ref_pos_z", 204),
    member("int32_t", "home_pos_x", 208),
    member("int32_t", "home_pos_y", 212),
    member("int32_t", "live_direction", 216),
    member("int32_t", "live_direction_2", 220),
    member("int32_t", "live_movement_param_0", 224),
    member("int32_t", "live_movement_param_1", 228),
    member("int32_t", "live_movement_param_2", 232),
    member("int32_t", "live_movement_param_3", 236),
    member(
        "int32_t",
        "live_facing_angle",
        240,
        doc="Live mirror of default_facing_angle in the runtime actor-state block.",
    ),
    member("int32_t", "live_rotation", 244),
    member("int32_t", "live_anim_param_0", 248),
    member("int32_t", "live_anim_param_1", 252),
    member("int16_t", "live_anim_param_2", 256),
    member("uint8_t", "live_direction_mode", 258),
    member("uint8_t", "live_anim_byte_3", 259),
    member("int32_t", "live_speed", 260),
    member(
        "int32_t",
        "live_scale",
        264,
        doc=("Live visual scale slot used by runtime actor-state paths."),
    ),
    member("int32_t", "live_prop_4", 268),
    member("int32_t", "live_prop_5", 272),
    member(
        "int32_t",
        "live_size",
        276,
        doc=("Live size slot used by runtime actor-state paths."),
    ),
    member("int32_t", "live_prop_7", 280),
    member("int32_t", "live_extra_0", 284),
    member("int32_t", "live_extra_1", 288),
    member("int32_t", "live_extra_2", 292),
    member("Actor_State*", "active_actor", 296),
    member("uint8_t", "team_bitmask[16]", 300),
    member("uint32_t", "spawn_timestamp", 316),
    member("uint8_t", "script_entity_index", 320),
    member(
        "uint8_t",
        "script_entity_stack[3]",
        321,
        doc=("Three contiguous script-entity stack bytes."),
    ),
    member("int32_t", "path_best_distance", 324),
    member("int32_t", "path_target_x", 328),
    member("int32_t", "path_target_y", 332),
    member("int32_t", "path_target_z", 336),
    member("int32_t", "path_result_x", 340),
    member("int32_t", "camera_sin_factor", 344),
    member("int32_t", "path_result_z", 348),
    member("int32_t", "path_waypoint_x", 352),
    member("int32_t", "path_waypoint_z", 356),
    member("int32_t", "path_waypoint_y_2", 360),
    member("int32_t", "path_facing", 364),
    member("int32_t", "live_velocity", 368),
    member("int32_t", "default_coll_rad", 372),
    member("int32_t", "default_coll_ht", 376),
    member("uint32_t", "default_flags", 380),
    member("int32_t", "runtime_state_0", 384),
    member("int32_t", "runtime_state_1", 388),
    member("int32_t", "runtime_state_2", 392),
    member("int32_t", "runtime_state_3", 396),
    member("int16_t", "runtime_jump_state", 400),
    member("int16_t", "runtime_state_4_hi", 402),
    member("int16_t", "runtime_counter", 404),
    member("int16_t", "runtime_state_5_hi", 406),
    member("int32_t", "runtime_state_6", 408),
    member("int32_t", "runtime_state_7", 412),
    member("int32_t", "runtime_state_8", 416),
    member("int32_t", "ai_scratch_padding[8]", 420),
    size=452,
    doc=(
        "Runtime actor overlay containing provisional player-specific offsets such as +0x74 "
        "(PC EN) and +0x172 (PC EN). Field semantics may be unstable."
    ),
)

unstable.struct(
    "Pkg_MeshNodeHeader",
    member("uint32_t", "node_type", 0),
    member("uint32_t", "parent_index", 4),
    member("uint32_t", "node_data_offset", 8),
    member("uint32_t", "link_data", 12),
    member("uint32_t", "bone_transforms[12]", 16),
    member("int16_t", "bounds_min_x", 64),
    member("int16_t", "bounds_min_y", 66),
    member("int16_t", "bounds_max_x", 68),
    member("int16_t", "bounds_max_y", 70),
    member("uint32_t", "mesh_flags", 72),
    member("uint32_t", "mesh_config[3]", 76),
    member("uint32_t", "visibility_mask", 88),
    member(
        "uint16_t",
        "padding_5c",
        92,
        doc="Pad/opaque mesh-node header word, not referenced by Resource_FixUpMeshNode.",
    ),
    member(
        "uint16_t",
        "padding_5e",
        94,
        doc="Pad/opaque mesh-node header word, not referenced by Resource_FixUpMeshNode.",
    ),
    member("uint32_t", "vertex_format", 96),
    member("uint8_t", "subtype_id", 100),
    member("uint8_t", "subtype_flags", 101),
    member("uint16_t", "polygon_count", 102),
    member("uint16_t", "vertex_count", 104),
    member("uint16_t", "material_ref_count", 106),
    member("uint32_t", "polygon_offset", 108),
    member("uint32_t", "vertex_offset", 112),
    member("uint32_t", "normal_offset", 116),
    member("uint32_t", "resource_manager_ptr", 120),
    member("uint32_t", "material_indices_offset", 124),
    member("uint32_t", "secondary_vertex_ptr", 128),
    member("uint32_t", "vertex_color_ptr", 132),
    member("uint32_t", "node_runtime_flags", 136),
    member("uint32_t", "anim_state_index", 140),
    member("uint32_t", "uv_data_ptr", 144),
    member("uint32_t", "aux_entry_array_ptr", 148),
    member("uint32_t", "cached_world_pos_x", 152),
    member("uint32_t", "cached_world_pos_y", 156),
    member("uint32_t", "cached_world_pos_z", 160),
    member("uint32_t", "bsphere_packed_xy", 164),
    member("uint32_t", "bsphere_packed_zr", 168),
    member("uint32_t", "bone_ref_array_ptr", 172),
    member("uint32_t", "morph_target_list_ptr", 176),
    member("uint32_t", "render_batch_array_ptr", 180),
    member("uint16_t", "draw_order_flags", 184),
    member(
        "uint8_t",
        "render_node_entry_count",
        186,
        doc="0x20-stride render-node entry count at +0xBC (PC EN), passed to render-entry fixup.",
    ),
    member(
        "uint8_t",
        "lod_count",
        187,
        doc="0x28-stride LOD entry count at lod_array_ptr, used while rebasing LOD entries.",
    ),
    member(
        "Mesh_RenderNodeEntry*",
        "render_node_entry_table_ptr",
        188,
        doc="0x20-stride mesh render-node entry table used by mesh-node fixup and render paths.",
    ),
    member("uint32_t", "lod_array_ptr", 192),
    member("uint32_t", "default_vertex_color", 196),
    member("uint32_t", "bone_data_ptr", 200),
    member("uint32_t", "material_batch_base", 204),
    member("uint32_t", "component_list_ptr", 208),
    member("uint32_t", "init_world_pos_z", 212),
    member("uint32_t", "bounding_radius", 216),
    member("uint32_t", "runtime_anim_timer", 220),
    member("uint32_t", "runtime_transform_x", 224),
    member("uint32_t", "runtime_transform_y", 228),
    member("uint32_t", "runtime_transform_z", 232),
    member("uint32_t", "runtime_transform_w", 236),
    member("uint16_t", "strip_vertex_count", 240),
    member("int16_t", "aux_entry_count", 242),
    member("uint32_t", "special_node_data_ptr", 244),
    member("uint32_t", "data_material_ref_ptr", 248),
    member(
        "uint32_t",
        "padding_fc",
        252,
        doc="Pad/opaque mesh-node header dword; no stable semantics have been isolated.",
    ),
    member(
        "uint32_t",
        "relative_offset_list_ptr",
        256,
        doc=(
            "Relative-offset list rebased in place by Resource_FixUpMeshNode; the function walks "
            "a dword list at +0x100 (PC EN) until a zero terminator and adds the rebased "
            "+0x100 (PC EN) base to each nonzero entry."
        ),
    ),
    size=260,
)

unstable.struct(
    "Pkg_LODEntry",
    member("int16_t", "lod_level", 0),
    member("int16_t", "sprite_layer_count", 2),
    member("void*", "render_data_ptr", 4),
    member("int16_t", "rot_angle_x", 8),
    member("int16_t", "face_count", 10),
    member(
        "int16_t",
        "lod_reserved_0_c",
        12,
        doc="Opaque LOD descriptor word used by render selection with lod_level, sprite_layer_count, render_data_ptr, face_count/start, and threshold.",
    ),
    member("uint16_t", "lod_distance_threshold", 14),
    member("int16_t", "face_start_index", 16),
    member(
        "int16_t",
        "lod_reserved_12",
        18,
        doc="Opaque LOD descriptor word; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_14",
        20,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_18",
        24,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_1_c",
        28,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int16_t",
        "lod_reserved_20",
        32,
        doc="Opaque LOD descriptor word; no stable direct semantics have been isolated.",
    ),
    member(
        "int16_t",
        "lod_padding_22",
        34,
        doc="Alignment/pad word before the relocated +0x24 (PC EN) slot; no stable independent semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_relocated_ptr_24",
        36,
        doc="Relocated pointer/offset slot in LOD data rebased by Resource_FixUpMeshNode and Actor_CloneTemplateWithTemplateRelativeFixups at stride 0x28.",
    ),
    size=40,
)

unstable.struct(
    "Pkg_MeshOffsetTable",
    member("uint32_t", "mesh_offsets[16]", 0),
    member("uint8_t", "offset_padding[64]", 64),
    size=128,
)

unstable.struct(
    "Pkg_SpriteEntry_AltLayout",
    member("uint8_t", "type", 0),
    member("uint8_t", "layer_index", 1),
    member("uint16_t", "control_flags", 2),
    member("Material_Entry*", "texture_db1", 4),
    member("Material_Entry*", "material_1", 8),
    member("Animation_FrameData*", "anim_frames_1", 12),
    member("Material_Entry*", "texture_db2", 16),
    member("Material_Entry*", "material_2", 20),
    member("Animation_FrameData*", "anim_frames_2", 24),
    member("Scene_Node*", "scene_node_ref", 28),
    member("int16_t", "base_x", 32),
    member("int16_t", "base_y", 34),
    member("int16_t", "offset_x", 36),
    member("int16_t", "offset_y", 38),
    member("int32_t", "move_start_time", 40),
    member("int32_t", "move_duration", 44),
    member("int32_t", "move_ease_a", 48),
    member("int32_t", "move_ease_b", 52),
    member("int32_t", "pos_base_val", 56),
    member("int32_t", "pos_delta_val", 60),
    member("int32_t", "src_scale_x", 64),
    member("int32_t", "src_scale_y", 68),
    member("int32_t", "scale_start_time", 72),
    member("int32_t", "scale_duration", 76),
    member("int32_t", "scale_ease_a", 80),
    member("int32_t", "scale_ease_b", 84),
    member(
        "uint8_t",
        "src_color_r",
        88,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "src_color_g",
        89,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "src_color_b",
        90,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "padding_5b",
        91,
        doc="Unvalidated high byte beside source RGB, with no alpha use isolated.",
    ),
    member(
        "int32_t",
        "target_color_word",
        92,
        doc="Target/current seed color word, with low 24-bit RGB validated.",
    ),
    member("int32_t", "color_start_time", 96),
    member("int32_t", "color_duration", 100),
    member("int32_t", "color_ease_a", 104),
    member("int32_t", "color_ease_b", 108),
    member("int32_t", "render_order", 112),
    member("int16_t", "clip_left", 116),
    member("int16_t", "clip_top", 118),
    member("int16_t", "clip_right", 120),
    member("int16_t", "clip_bottom", 122),
    member("uint8_t", "link_index", 124),
    member(
        "uint8_t",
        "anchor_code",
        125,
        doc="Anchor dispatch selector 1..8 read by UI_UpdateAndRenderSprites.",
    ),
    member("uint8_t", "state_flags", 126),
    member("uint8_t", "padding_7f", 127),
    member(
        "uint8_t",
        "cur_color_r",
        128,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_g",
        129,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_b",
        130,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_reserved_high",
        131,
        doc="Copied high byte of the current color word; no direct alpha behavior has been validated.",
    ),
    member("int32_t", "order_field_84", 132),
    member("int32_t", "dst_render_order", 136),
    member("int32_t", "order_start_time", 140),
    member("int32_t", "order_duration", 144),
    member("int32_t", "order_ease_a", 148),
    member("int32_t", "order_ease_b", 152),
    member("int32_t", "cur_scale_x", 156),
    member("int32_t", "cur_scale_y", 160),
    member("int16_t", "screen_x", 164),
    member("int16_t", "screen_y", 166),
    member("int16_t", "frame_counter", 168),
    member("int16_t", "padding_aa", 170),
    size=172,
    doc="Alternate recovered layout for the 0xac-stride sprite/UI entry; renamed from misleading Pkg_PowerupEntry because Level_RuntimeData.powerup_list uses separate 0x1c-stride Powerup_Entry records.",
)

unstable.struct(
    "Pkg_SpriteEntry",
    member("uint32_t", "flags_and_layer_count", 0),
    member("void*", "scene_node_ptr_0", 4),
    member(
        "Render_SpriteContext*",
        "layer_0_sprite_context_ptr",
        8,
        doc="Layer0 Render_SpriteContext/material runtime descriptor pointer, passed toward sprite rendering.",
    ),
    member(
        "void*",
        "layer_0_descriptor_aux_ptr",
        12,
        doc="Layer0 auxiliary command/material descriptor pointer, whose 0x0C descriptor payload is rebased by Resource_FixUpSpriteEntry.",
    ),
    member("void*", "scene_node_ptr_1", 16),
    member(
        "Render_SpriteContext*",
        "layer_1_sprite_context_ptr",
        20,
        doc="Layer1 Render_SpriteContext/material runtime descriptor pointer, passed toward sprite rendering.",
    ),
    member(
        "void*",
        "layer_1_descriptor_aux_ptr",
        24,
        doc="Layer1 auxiliary command/material descriptor pointer, with the same 0x0C descriptor role as layer0_descriptor_aux_ptr.",
    ),
    member("uint32_t", "sprite_resource_index", 28),
    member(
        "uint32_t",
        "movement_source_xy",
        32,
        doc="Packed signed source X/Y words for sprite movement interpolation.",
    ),
    member("void*", "layer_0_texture_ptr", 36),
    member("uint32_t", "layer_0_transform_0", 40),
    member("uint32_t", "layer_0_transform_1", 44),
    member("uint32_t", "layer_0_transform_2", 48),
    member("uint32_t", "layer_0_transform_3", 52),
    member("uint32_t", "layer_0_transform_4", 56),
    member("uint32_t", "layer_0_anim_state", 60),
    member("int32_t", "layer_0_scale_x", 64),
    member("int32_t", "layer_0_scale_y", 68),
    member(
        "uint32_t",
        "scale_tween_start_frame",
        72,
        doc="Layer0 scale interpolation start frame, paired with scale_tween_end_frame.",
    ),
    member(
        "uint32_t",
        "scale_tween_end_frame",
        76,
        doc="Layer0 scale interpolation end frame, cleared after the target is reached.",
    ),
    member(
        "uint32_t",
        "scale_tween_ease_in_fp12",
        80,
        doc="Layer0 scale interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "scale_tween_ease_out_fp12",
        84,
        doc="Layer0 scale interpolation ease-out percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "color_tween_source_rgb_reserved_high",
        88,
        doc="Source RGB snapshot for layer0 color interpolation, with low 24 bits validated.",
    ),
    member(
        "uint32_t",
        "color_tween_target_color_word",
        92,
        doc="Layer0 interpolation target/current color word, with low 24-bit RGB validated.",
    ),
    member(
        "uint32_t",
        "color_tween_start_frame",
        96,
        doc="Layer0 color interpolation start frame, paired with color_tween_end_frame.",
    ),
    member(
        "uint32_t",
        "color_tween_end_frame",
        100,
        doc="Layer0 color interpolation end frame, cleared after the target is reached.",
    ),
    member(
        "uint32_t",
        "color_tween_ease_in_fp12",
        104,
        doc="Layer0 color interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "color_tween_ease_out_fp12",
        108,
        doc="Layer0 color interpolation ease-out percentage, stored in fp12 units.",
    ),
    member("int32_t", "rotation_angle_fp12", 112),
    member("void*", "layer_1_texture_ptr", 116),
    member("uint32_t", "layer_1_render_flags", 120),
    member("uint32_t", "layer_1_control_flags", 124),
    member(
        "uint32_t",
        "layer_1_current_color_word",
        128,
        doc="Current/fallback render color word, with low 24-bit RGB validated.",
    ),
    member(
        "uint32_t",
        "rotation_anim_start_angle",
        132,
        doc="Current/start rotation angle, eased toward rotation_anim_target_angle.",
    ),
    member(
        "uint32_t",
        "rotation_anim_target_angle",
        136,
        doc="Target rotation angle, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "rotation_anim_start_frame",
        140,
        doc="Rotation interpolation start frame, paired with rotation_anim_end_frame.",
    ),
    member(
        "uint32_t",
        "rotation_anim_end_frame",
        144,
        doc="Rotation interpolation end frame, and nonzero keeps the script command waiting.",
    ),
    member(
        "uint32_t",
        "rotation_anim_ease_in",
        148,
        doc="Rotation interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "rotation_anim_ease_out",
        152,
        doc="Rotation interpolation ease-out percentage, stored in fp12 units.",
    ),
    member("int32_t", "layer_1_scale_x", 156),
    member("int32_t", "layer_1_scale_y", 160),
    member("void*", "layer_1_texture_ptr_2", 164),
    member(
        "uint32_t",
        "sprite_sort_key",
        168,
        doc="Low-word sprite depth/sort key, compared by UI_CompareSpriteDepth.",
    ),
    size=172,
)

unstable.struct(
    "Pkg_TrailListEntry",
    member("uint16_t", "count", 0),
    member("uint16_t", "reserved", 2),
    member("Component_TrailObject*", "ptr", 4),
    size=8,
)

unstable.struct(
    "Pkg_UiLayoutResource",
    member("uint32_t", "checksum", 0),
    member("uint32_t", "entry_count", 4),
    member("uint8_t", "layout_padding[12]", 8),
    size=20,
)

unstable.struct(
    "Pkg_Header",
    member("Pkg_TocEntry", "entries[138]", 0),
    member(
        "uint8_t",
        "header_reserved[944]",
        1104,
        doc="Unparsed 0x3B0-byte package-header tail, left after PKG_OpenAndReadTOC copies only the first 0x450 bytes of the 0x800-byte header.",
    ),
    size=2048,
)

unstable.struct(
    "Scene_NodePayload",
    member("Scene_Node*", "parent_node_ptr", 0),
    member("Scene_Node*", "child_list_head", 4),
    member("Scene_Node*", "sibling_link", 8),
    member("Scene_LocalTransform", "transform", 12),
    member("uint8_t", "node_type", 26),
    member("uint8_t", "padding_1b[1]", 27),
    member("uint16_t", "padding", 28),
    member("uint8_t", "padding_1e[2]", 30),
    size=32,
    doc="Compact scene-node payload/resource-record prefix shared by older recovered Group/Model/Object shapes; no direct loader/parser owner has been validated yet.",
)

unstable.struct(
    "Scene_SubNodePayload",
    member("Scene_Node*", "parent_node_ptr", 0),
    member("Scene_Node*", "child_list_head", 4),
    member("Scene_Node*", "sibling_link", 8),
    member("Scene_LocalTransform", "transform", 12),
    member("uint8_t", "node_type", 26),
    member("uint8_t", "padding_1b", 27),
    member("uint16_t", "padding_1c", 28),
    member("uint8_t", "padding_1e[2]", 30),
    member("int32_t", "extra_data", 32),
    size=36,
    doc="Compact scene sub-node payload/resource-record variant; no direct loader/parser owner has been validated yet.",
)
BLUEPRINT = unstable
