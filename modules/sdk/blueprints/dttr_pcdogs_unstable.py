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
    member("int16_t", "yaw", 0x0),
    member("int16_t", "pitch", 0x2),
    member("int16_t", "roll", 0x4),
    member("int16_t", "look_at_pitch", 0x6),
    member("int16_t", "orbit_yaw", 0x8),
    member("int16_t", "fov", 0xA),
    member("int32_t", "focal_distance", 0xC),
    member("Math_Vec3i32XZY", "eye_pos", 0x10),
    member("Math_Vec3i32XZY", "target_pos", 0x1C),
    member("Entity_State*", "active_entity_slot_ptr", 0x28),
    member("Math_Sizei16", "screen_half", 0x2C),
    member("Math_Matrix3x3i16", "view_matrix", 0x30),
    member("int16_t", "view_matrix_padding", 0x42),
    member(
        "uint8_t",
        "frustum_setup_prefix_44[20]",
        0x44,
        doc="Camera/frustum setup prefix before the five validated 12-byte clip-plane records.",
    ),
    member(
        "Render_FrustumClipPlane",
        "frustum_planes[5]",
        0x58,
        doc="Five plane records written by Scene_RenderFrame and read by Render_CheckActorVisibilityAndFrustum.",
    ),
    member("Math_Matrix3x3i16", "node_view_matrix", 0x94),
    member("int16_t", "node_view_matrix_padding", 0xA6),
    member("Math_Vec3i32", "node_view_translation", 0xA8),
    member("int32_t", "projection_near_fp", 0xB4),
    member("int32_t", "dynamic_level_scale", 0xB8),
    member("Actor_State*", "render_actor_ptr", 0xBC),
    member("uint32_t", "render_pass_flags", 0xC0),
    member("void*", "post_sorted_callback", 0xC4),
    member("void*", "pre_shadow_callback", 0xC8),
    member("void*", "sorted_list_head", 0xCC),
    member("void*", "sorted_list_buckets[16384]", 0xD0),
    member(
        "uint32_t",
        "sorted_bucket_tail",
        0x100D0,
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
    member("void*", "collision_data", 0x0),
    member("Actor_State*", "actor_list", 0x4),
    member("void*", "trigger_list", 0x8),
    member("Pkg_CameraDef*", "camera_data", 0xC),
    member("Material_Entry*", "material_table", 0x10),
    member("uint32_t", "reserved_01", 0x14),
    member("uint32_t", "flags", 0x18),
    member("uint16_t", "actor_count", 0x1C),
    member("uint16_t", "material_count", 0x1E),
    member("char*", "string_table", 0x20),
    size=36,
)

unstable.struct(
    "Level_RuntimeData",
    member("Camera_Runtime*", "cam_default", 0x0),
    member("Camera_Runtime*", "cam_current", 0x4),
    member("int16_t", "current_entity_index", 0x8),
    member(
        "int16_t",
        "entity_count",
        0xA,
        doc=(
            "Number of valid level-local runtime entity slots in entity_array for the "
            "currently loaded level."
        ),
    ),
    member(
        "Entity_State*",
        "entity_array",
        0xC,
        doc=(
            "Array of level-local runtime entity slots. Pointers into this array "
            "identify entity spawn slots within the currently loaded level."
        ),
    ),
    member(
        "Audio_SoundDefinition*",
        "sound_definition_list",
        0x10,
        doc="Current-level sound-definition array, indexed by level-local sound operands.",
    ),
    member("int16_t", "sound_definition_count", 0x14),
    member("int16_t", "var_count", 0x16),
    member("int32_t*", "var_list", 0x18),
    member("int16_t", "powerup_count", 0x1C),
    member("int16_t", "powerup_type_count", 0x1E),
    member(
        "Powerup_Entry*",
        "powerup_list",
        0x20,
        doc="0x1c-stride current-level powerup spawn-record list keyed by powerup_count.",
    ),
    member(
        "Pkg_ActorTemplate*",
        "powerup_actor_slots[16]",
        0x24,
        doc=(
            "Fixed 16-slot powerup actor-template/clone-source table. "
            "Resource_FixUpLevelPointers fixes each non-null slot with Resource_FixUpActorPointers; "
            "Powerup_CloneActor reads these Pkg_ActorTemplate* sources when creating spawned powerup actors."
        ),
    ),
    member("char*", "themes[5]", 0x64),
    member("int32_t", "theme_count", 0x78),
    member("Trail_Entry*", "trail_list", 0x7C),
    member("Pkg_SpriteEntry*", "sprite_list", 0x80),
    member("Nav_Network*", "nav_net", 0x84),
    member("Material_Entry*", "usable_materials", 0x88),
    size=140,
    doc="Concrete runtime level-data block carried by Level_Data* APIs.",
)

unstable.struct(
    "Powerup_Entry",
    member("Pkg_ActorRecord*", "template_record", 0x0),
    member("int16_t", "spawn_params_a", 0x4),
    member("int16_t", "spawn_params_b", 0x6),
    member(
        "int32_t",
        "runtime_value_08",
        0x8,
        doc="Powerup-list dword whose low 16 bits are used by attached/local-position paths.",
    ),
    member("uint8_t", "powerup_type", 0xC),
    member("uint8_t", "flags", 0xD),
    member("int16_t", "max_spawn_count", 0xE),
    member("Math_Vec3i32", "pos", 0x10),
    size=28,
    doc="0x1c-stride Level_RuntimeData.powerup_list entry walked by Powerup_UpdateSpawnLogic with template_record, flags, max_spawn_count, pos vector field.",
)

unstable.struct(
    "Material_EntryFull",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "reserved_01", 0x1),
    member("uint16_t", "reserved_02", 0x2),
    member("DDraw_IDirectDrawSurface7*", "texture_data_ptr", 0x4),
    member("uint8_t*", "palette_data_ptr", 0x8),
    member("Math_Sizeu16", "dimensions", 0xC),
    member("uint16_t", "format", 0x10),
    member("uint16_t", "reserved_03", 0x12),
    member("DDraw_IDirectDrawSurface7*", "d3d_texture", 0x14),
    member("uint32_t", "texture_handle", 0x18),
    member("uint32_t", "ref_count", 0x1C),
    member("uint32_t", "reserved_04", 0x20),
    size=36,
    doc="Expanded/runtime material-entry form with the 20-byte descriptor fields plus runtime DirectDraw/D3D texture handles.",
)

unstable.struct(
    "Material_SetRuntime",
    member("Material_Entry*", "entries", 0x0),
    member("uint16_t", "count", 0x4),
    member("uint16_t", "reserved_06", 0x6),
    member("uint32_t", "flags", 0x8),
    member("Material_FrameData*", "material_data_array", 0xC),
    member("uint32_t", "reserved_10", 0x10),
    size=20,
)

unstable.struct(
    "Mesh_TransformEntry",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("char", "bone_index", 0x2),
    member(
        "uint8_t",
        "signal_id_hi",
        0x3,
        doc="High byte of the generic mesh-command signal_id word, with no isolated stable semantic for transform-specific consumers.",
    ),
    member("uint32_t", "resource_ptr", 0x4),
    member("int16_t", "poly_start_index", 0x8),
    member("int16_t", "poly_count", 0xA),
    member(
        "int16_t",
        "payload_word_0_c",
        0xC,
        doc=(
            "Variant mesh-command payload start; type 0 passes cmd+0x0C (PC EN) to "
            "Animation_ProcessController, while other command types reinterpret the payload."
        ),
    ),
    member("int16_t", "effect_count", 0xE),
    member("Math_Vec2i16", "scale", 0x10),
    size=20,
)

unstable.struct(
    "Movie_PlaybackBuffer",
    member("uint8_t", "decoder_state[396]", 0x0),
    member("char", "movie_alias[64]", 0x18C),
    member("uint8_t", "decode_scratch[64]", 0x1CC),
    member("uint8_t", "frame_pixel_data[572]", 0x20C),
    member("int32_t*", "callback_context", 0x448),
    size=1100,
)

unstable.struct(
    "Pkg_ActorRecord_UnstableLayout",
    member("uint32_t", "flags", 0x0),
    member("Math_Vec3i32", "camera_pos", 0x4),
    member("int32_t", "collision_radius_sq", 0x10),
    member("int32_t", "collision_height_sq", 0x14),
    member("uint8_t", "active_flag", 0x18),
    member("uint8_t", "respawn_mode", 0x19),
    member("uint16_t", "padding_1a", 0x1A),
    member("Level_RuntimeData*", "level_data", 0x1C),
    member("int16_t", "self_index", 0x20),
    member("int16_t", "default_anim_state", 0x22),
    member("int32_t", "link_targets[9]", 0x24),
    member("Pkg_ScriptHeader*", "script_data_ptr", 0x48),
    member("uint32_t", "actor_template_ptr", 0x4C),
    member("uint32_t", "component_node_ptrs[2]", 0x50),
    member("Pkg_ActorRecord_UnstableLayout*", "spawn_state_ptr", 0x58),
    member("Math_Vec3i32", "default_ref_pos", 0x5C),
    member("Math_Vec2i32", "default_home", 0x68),
    member("int32_t", "default_direction", 0x70),
    member("int32_t", "default_direction_2", 0x74),
    member("int32_t", "default_movement_params[4]", 0x78),
    member("int32_t", "default_facing_angle", 0x88),
    member("int32_t", "default_rotation", 0x8C),
    member("int32_t", "default_anim_param_0", 0x90),
    member("int32_t", "default_anim_param_1", 0x94),
    member("int16_t", "default_anim_param_2", 0x98),
    member("uint8_t", "default_direction_mode", 0x9A),
    member("uint8_t", "default_anim_byte_3", 0x9B),
    member("int32_t", "default_speed", 0x9C),
    member(
        "int32_t",
        "default_scale",
        0xA0,
        doc=(
            "Authored/default visual scale slot used by record-style actor initialization paths."
        ),
    ),
    member("int32_t", "default_prop_4", 0xA4),
    member("int32_t", "default_prop_5", 0xA8),
    member(
        "int32_t",
        "default_size",
        0xAC,
        doc=(
            "Authored/default size slot used by record-style actor initialization paths."
        ),
    ),
    member("int32_t", "default_prop_7", 0xB0),
    member("int32_t", "default_extra[3]", 0xB4),
    member("Camera_EntityView*", "entity_ref_pos_ptr", 0xC0),
    member("Math_Vec3i32", "live_ref_pos", 0xC4),
    member("Math_Vec2i32", "home_pos", 0xD0),
    member("int32_t", "live_direction", 0xD8),
    member("int32_t", "live_direction_2", 0xDC),
    member("int32_t", "live_movement_params[4]", 0xE0),
    member(
        "int32_t",
        "live_facing_angle",
        0xF0,
        doc="Live mirror of default_facing_angle in the runtime actor-state block.",
    ),
    member("int32_t", "live_rotation", 0xF4),
    member("int32_t", "live_anim_param_0", 0xF8),
    member("int32_t", "live_anim_param_1", 0xFC),
    member("int16_t", "live_anim_param_2", 0x100),
    member("uint8_t", "live_direction_mode", 0x102),
    member("uint8_t", "live_anim_byte_3", 0x103),
    member("int32_t", "live_speed", 0x104),
    member(
        "int32_t",
        "live_scale",
        0x108,
        doc=("Live visual scale slot used by runtime actor-state paths."),
    ),
    member("int32_t", "live_prop_4", 0x10C),
    member("int32_t", "live_prop_5", 0x110),
    member(
        "int32_t",
        "live_size",
        0x114,
        doc=("Live size slot used by runtime actor-state paths."),
    ),
    member("int32_t", "live_prop_7", 0x118),
    member("int32_t", "live_extra[3]", 0x11C),
    member("Actor_State*", "active_actor", 0x128),
    member("uint8_t", "team_bitmask[16]", 0x12C),
    member("uint32_t", "spawn_timestamp", 0x13C),
    member("uint8_t", "script_entity_index", 0x140),
    member(
        "uint8_t",
        "script_entity_stack[3]",
        0x141,
        doc=("Three contiguous script-entity stack bytes."),
    ),
    member("int32_t", "path_best_distance", 0x144),
    member("Math_Vec3i32", "path_target", 0x148),
    member("int32_t", "path_result_x", 0x154),
    member("int32_t", "camera_sin_factor", 0x158),
    member("int32_t", "path_result_z", 0x15C),
    member("int32_t", "path_waypoint_x", 0x160),
    member("int32_t", "path_waypoint_z", 0x164),
    member("int32_t", "path_waypoint_y_2", 0x168),
    member("int32_t", "path_facing", 0x16C),
    member("int32_t", "live_velocity", 0x170),
    member("int32_t", "default_coll_rad", 0x174),
    member("int32_t", "default_coll_ht", 0x178),
    member("uint32_t", "default_flags", 0x17C),
    member("int32_t", "runtime_state[4]", 0x180),
    member("int16_t", "runtime_jump_state", 0x190),
    member("int16_t", "runtime_state_4_hi", 0x192),
    member("int16_t", "runtime_counter", 0x194),
    member("int16_t", "runtime_state_5_hi", 0x196),
    member("int32_t", "runtime_state_6", 0x198),
    member("int32_t", "runtime_state_7", 0x19C),
    member("int32_t", "runtime_state_8", 0x1A0),
    member("int32_t", "ai_scratch_padding[8]", 0x1A4),
    size=452,
    doc=(
        "Runtime actor overlay containing provisional player-specific offsets like +0x74 "
        "(PC EN) and +0x172 (PC EN). Field semantics may be unstable."
    ),
)

unstable.struct(
    "Pkg_MeshNodeHeader",
    member("uint32_t", "node_type", 0x0),
    member("uint32_t", "parent_index", 0x4),
    member("uint32_t", "node_data_offset", 0x8),
    member("uint32_t", "link_data", 0xC),
    member("uint32_t", "bone_transforms[12]", 0x10),
    member("Math_Recti16", "bounds", 0x40),
    member("uint32_t", "mesh_flags", 0x48),
    member("uint32_t", "mesh_config[3]", 0x4C),
    member("uint32_t", "visibility_mask", 0x58),
    member(
        "uint16_t",
        "padding_5c",
        0x5C,
        doc="Pad/opaque mesh-node header word, not referenced by Resource_FixUpMeshNode.",
    ),
    member(
        "uint16_t",
        "padding_5e",
        0x5E,
        doc="Pad/opaque mesh-node header word, not referenced by Resource_FixUpMeshNode.",
    ),
    member("uint32_t", "vertex_format", 0x60),
    member("uint8_t", "subtype_id", 0x64),
    member("uint8_t", "subtype_flags", 0x65),
    member("uint16_t", "polygon_count", 0x66),
    member("uint16_t", "vertex_count", 0x68),
    member("uint16_t", "material_ref_count", 0x6A),
    member("uint32_t", "polygon_offset", 0x6C),
    member("uint32_t", "vertex_offset", 0x70),
    member("uint32_t", "normal_offset", 0x74),
    member("uint32_t", "resource_manager_ptr", 0x78),
    member("uint32_t", "material_indices_offset", 0x7C),
    member("uint32_t", "secondary_vertex_ptr", 0x80),
    member("uint32_t", "vertex_color_ptr", 0x84),
    member("uint32_t", "node_runtime_flags", 0x88),
    member("uint32_t", "anim_state_index", 0x8C),
    member("uint32_t", "uv_data_ptr", 0x90),
    member("uint32_t", "aux_entry_array_ptr", 0x94),
    member("Math_Vec3u", "cached_world_pos", 0x98),
    member("Math_BoundingSphereu16", "bounding_sphere", 0xA4),
    member("uint32_t", "bone_ref_array_ptr", 0xAC),
    member("uint32_t", "morph_target_list_ptr", 0xB0),
    member("uint32_t", "render_batch_array_ptr", 0xB4),
    member("uint16_t", "draw_order_flags", 0xB8),
    member(
        "uint8_t",
        "render_node_entry_count",
        0xBA,
        doc="0x20-stride render-node entry count at +0xBC (PC EN), passed to render-entry fixup.",
    ),
    member(
        "uint8_t",
        "lod_count",
        0xBB,
        doc="0x28-stride LOD entry count at lod_array_ptr, used while rebasing LOD entries.",
    ),
    member(
        "Mesh_RenderNodeEntry*",
        "render_node_entry_table_ptr",
        0xBC,
        doc="0x20-stride mesh render-node entry table used by mesh-node fixup and render paths.",
    ),
    member("uint32_t", "lod_array_ptr", 0xC0),
    member("uint32_t", "default_vertex_color", 0xC4),
    member("uint32_t", "bone_data_ptr", 0xC8),
    member("uint32_t", "material_batch_base", 0xCC),
    member("uint32_t", "component_list_ptr", 0xD0),
    member("uint32_t", "init_world_pos_z", 0xD4),
    member("uint32_t", "bounding_radius", 0xD8),
    member("uint32_t", "runtime_anim_timer", 0xDC),
    member("uint32_t", "runtime_transform[4]", 0xE0),
    member("uint16_t", "strip_vertex_count", 0xF0),
    member("int16_t", "aux_entry_count", 0xF2),
    member("uint32_t", "special_node_data_ptr", 0xF4),
    member("uint32_t", "data_material_ref_ptr", 0xF8),
    member(
        "uint32_t",
        "padding_fc",
        0xFC,
        doc="Pad/opaque mesh-node header dword; no stable semantics have been isolated.",
    ),
    member(
        "uint32_t",
        "relative_offset_list_ptr",
        0x100,
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
    member("int16_t", "lod_level", 0x0),
    member("int16_t", "sprite_layer_count", 0x2),
    member("void*", "render_data_ptr", 0x4),
    member("int16_t", "rot_angle_x", 0x8),
    member("int16_t", "face_count", 0xA),
    member(
        "int16_t",
        "lod_reserved_0_c",
        0xC,
        doc="Opaque LOD descriptor word used by render selection with lod_level, sprite_layer_count, render_data_ptr, face_count/start, and threshold.",
    ),
    member("uint16_t", "lod_distance_threshold", 0xE),
    member("int16_t", "face_start_index", 0x10),
    member(
        "int16_t",
        "lod_reserved_12",
        0x12,
        doc="Opaque LOD descriptor word; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_14",
        0x14,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_18",
        0x18,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_reserved_1_c",
        0x1C,
        doc="Opaque LOD descriptor dword; no stable direct semantics have been isolated.",
    ),
    member(
        "int16_t",
        "lod_reserved_20",
        0x20,
        doc="Opaque LOD descriptor word; no stable direct semantics have been isolated.",
    ),
    member(
        "int16_t",
        "lod_padding_22",
        0x22,
        doc="Alignment/pad word before the relocated +0x24 (PC EN) slot; no stable independent semantics have been isolated.",
    ),
    member(
        "int32_t",
        "lod_relocated_ptr_24",
        0x24,
        doc="Relocated pointer/offset slot in LOD data rebased by Resource_FixUpMeshNode and Actor_CloneTemplateWithTemplateRelativeFixups at stride 0x28.",
    ),
    size=40,
)

unstable.struct(
    "Pkg_MeshOffsetTable",
    member("uint32_t", "mesh_offsets[16]", 0x0),
    member("uint8_t", "offset_padding[64]", 0x40),
    size=128,
)

unstable.struct(
    "Pkg_SpriteMaterialLayer",
    member("Material_Entry*", "texture_db", 0x0),
    member("Material_Entry*", "material", 0x4),
    member("Animation_FrameData*", "anim_frames", 0x8),
    size=12,
)

unstable.struct(
    "Pkg_SpriteEntry_AltLayout",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "layer_index", 0x1),
    member("uint16_t", "control_flags", 0x2),
    member("Pkg_SpriteMaterialLayer", "material_layers[2]", 0x4),
    member("Scene_Node*", "scene_node_ref", 0x1C),
    member("Math_Vec2i16", "base", 0x20),
    member("Math_Vec2i16", "offset", 0x24),
    member("int32_t", "move_start_time", 0x28),
    member("int32_t", "move_duration", 0x2C),
    member("int32_t", "move_ease_a", 0x30),
    member("int32_t", "move_ease_b", 0x34),
    member("int32_t", "pos_base_val", 0x38),
    member("int32_t", "pos_delta_val", 0x3C),
    member("Math_Vec2i32", "src_scale", 0x40),
    member("int32_t", "scale_start_time", 0x48),
    member("int32_t", "scale_duration", 0x4C),
    member("int32_t", "scale_ease_a", 0x50),
    member("int32_t", "scale_ease_b", 0x54),
    member(
        "uint8_t",
        "src_color_r",
        0x58,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "src_color_g",
        0x59,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "src_color_b",
        0x5A,
        doc="Source RGB byte for color tween, read through the low 24-bit color path.",
    ),
    member(
        "uint8_t",
        "padding_5b",
        0x5B,
        doc="Unvalidated high byte beside source RGB, with no alpha use isolated.",
    ),
    member(
        "int32_t",
        "target_color_word",
        0x5C,
        doc="Target/current seed color word, with low 24-bit RGB validated.",
    ),
    member("int32_t", "color_start_time", 0x60),
    member("int32_t", "color_duration", 0x64),
    member("int32_t", "color_ease_a", 0x68),
    member("int32_t", "color_ease_b", 0x6C),
    member("int32_t", "render_order", 0x70),
    member("Math_Recti16", "clip", 0x74),
    member("uint8_t", "link_index", 0x7C),
    member(
        "uint8_t",
        "anchor_code",
        0x7D,
        doc="Anchor dispatch selector 1..8 read by UI_UpdateAndRenderSprites.",
    ),
    member("uint8_t", "state_flags", 0x7E),
    member("uint8_t", "padding_7f", 0x7F),
    member(
        "uint8_t",
        "cur_color_r",
        0x80,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_g",
        0x81,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_b",
        0x82,
        doc="Current/fallback render RGB byte, rebuilt into the active color word.",
    ),
    member(
        "uint8_t",
        "cur_color_reserved_high",
        0x83,
        doc="Copied high byte of the current color word; no direct alpha behavior has been validated.",
    ),
    member("int32_t", "order_field_84", 0x84),
    member("int32_t", "dst_render_order", 0x88),
    member("int32_t", "order_start_time", 0x8C),
    member("int32_t", "order_duration", 0x90),
    member("int32_t", "order_ease_a", 0x94),
    member("int32_t", "order_ease_b", 0x98),
    member("Math_Vec2i32", "cur_scale", 0x9C),
    member("Math_Vec2i16", "screen", 0xA4),
    member("int16_t", "frame_counter", 0xA8),
    member("int16_t", "padding_aa", 0xAA),
    size=172,
    doc="Alternate recovered layout for the 0xac-stride sprite/UI entry; renamed from misleading Pkg_PowerupEntry because Level_RuntimeData.powerup_list uses separate 0x1c-stride Powerup_Entry records.",
)

unstable.struct(
    "Pkg_SpriteLayerBinding",
    member("void*", "scene_node_ptr", 0x0),
    member("Render_SpriteContext*", "sprite_context_ptr", 0x4),
    member("void*", "descriptor_aux_ptr", 0x8),
    size=12,
)

unstable.struct(
    "Pkg_SpriteEntry",
    member("uint32_t", "flags_and_layer_count", 0x0),
    member(
        "Pkg_SpriteLayerBinding",
        "layers[2]",
        0x4,
        doc="Two 0x0C sprite layer bindings: scene node pointer, Render_SpriteContext pointer, and descriptor aux pointer.",
    ),
    member("uint32_t", "sprite_resource_index", 0x1C),
    member(
        "Math_Vec2i16",
        "movement_source",
        0x20,
        doc="Unpacked signed source X/Y words for sprite movement interpolation.",
    ),
    member("void*", "layer_0_texture_ptr", 0x24),
    member("uint32_t", "layer_0_transform[5]", 0x28),
    member("uint32_t", "layer_0_anim_state", 0x3C),
    member("Math_Vec2i32", "layer_0_scale", 0x40),
    member(
        "uint32_t",
        "scale_tween_start_frame",
        0x48,
        doc="Layer0 scale interpolation start frame, paired with scale_tween_end_frame.",
    ),
    member(
        "uint32_t",
        "scale_tween_end_frame",
        0x4C,
        doc="Layer0 scale interpolation end frame, cleared after the target is reached.",
    ),
    member(
        "uint32_t",
        "scale_tween_ease_in_fp12",
        0x50,
        doc="Layer0 scale interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "scale_tween_ease_out_fp12",
        0x54,
        doc="Layer0 scale interpolation ease-out percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "color_tween_source_rgb_reserved_high",
        0x58,
        doc="Source RGB snapshot for layer0 color interpolation, with low 24 bits validated.",
    ),
    member(
        "uint32_t",
        "color_tween_target_color_word",
        0x5C,
        doc="Layer0 interpolation target/current color word, with low 24-bit RGB validated.",
    ),
    member(
        "uint32_t",
        "color_tween_start_frame",
        0x60,
        doc="Layer0 color interpolation start frame, paired with color_tween_end_frame.",
    ),
    member(
        "uint32_t",
        "color_tween_end_frame",
        0x64,
        doc="Layer0 color interpolation end frame, cleared after the target is reached.",
    ),
    member(
        "uint32_t",
        "color_tween_ease_in_fp12",
        0x68,
        doc="Layer0 color interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "color_tween_ease_out_fp12",
        0x6C,
        doc="Layer0 color interpolation ease-out percentage, stored in fp12 units.",
    ),
    member("int32_t", "rotation_angle_fp12", 0x70),
    member("void*", "layer_1_texture_ptr", 0x74),
    member("uint32_t", "layer_1_render_flags", 0x78),
    member("uint32_t", "layer_1_control_flags", 0x7C),
    member(
        "uint32_t",
        "layer_1_current_color_word",
        0x80,
        doc="Current/fallback render color word, with low 24-bit RGB validated.",
    ),
    member(
        "uint32_t",
        "rotation_anim_start_angle",
        0x84,
        doc="Current/start rotation angle, eased toward rotation_anim_target_angle.",
    ),
    member(
        "uint32_t",
        "rotation_anim_target_angle",
        0x88,
        doc="Target rotation angle, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "rotation_anim_start_frame",
        0x8C,
        doc="Rotation interpolation start frame, paired with rotation_anim_end_frame.",
    ),
    member(
        "uint32_t",
        "rotation_anim_end_frame",
        0x90,
        doc="Rotation interpolation end frame, and nonzero keeps the script command waiting.",
    ),
    member(
        "uint32_t",
        "rotation_anim_ease_in",
        0x94,
        doc="Rotation interpolation ease-in percentage, stored in fp12 units.",
    ),
    member(
        "uint32_t",
        "rotation_anim_ease_out",
        0x98,
        doc="Rotation interpolation ease-out percentage, stored in fp12 units.",
    ),
    member("Math_Vec2i32", "layer_1_scale", 0x9C),
    member("void*", "layer_1_texture_ptr_2", 0xA4),
    member(
        "uint32_t",
        "sprite_sort_key",
        0xA8,
        doc="Low-word sprite depth/sort key, compared by UI_CompareSpriteDepth.",
    ),
    size=172,
)

unstable.struct(
    "Pkg_TrailListEntry",
    member("uint16_t", "count", 0x0),
    member("uint16_t", "reserved", 0x2),
    member("Component_TrailObject*", "ptr", 0x4),
    size=8,
)

unstable.struct(
    "Pkg_UILayoutResource",
    member("uint32_t", "checksum", 0x0),
    member("uint32_t", "entry_count", 0x4),
    member("uint8_t", "layout_padding[12]", 0x8),
    size=20,
)

unstable.struct(
    "Pkg_Header",
    member("Pkg_TOCEntry", "entries[138]", 0x0),
    member(
        "uint8_t",
        "header_reserved[944]",
        0x450,
        doc="Unparsed 0x3B0-byte package-header tail, left after PKG_OpenAndReadTOC copies only the first 0x450 bytes of the 0x800-byte header.",
    ),
    size=2048,
)

unstable.struct(
    "Scene_NodePayload",
    member("Scene_Node*", "parent_node_ptr", 0x0),
    member("Scene_Node*", "child_list_head", 0x4),
    member("Scene_Node*", "sibling_link", 0x8),
    member("Scene_LocalTransform", "transform", 0xC),
    member("uint8_t", "node_type", 0x1A),
    member("uint8_t", "padding_1b[1]", 0x1B),
    member("uint16_t", "padding", 0x1C),
    member("uint8_t", "padding_1e[2]", 0x1E),
    size=32,
    doc="Compact scene-node payload/resource-record prefix shared by older recovered Group/Model/Object shapes; no direct loader/parser owner has been validated yet.",
)

unstable.struct(
    "Scene_SubNodePayload",
    member("Scene_Node*", "parent_node_ptr", 0x0),
    member("Scene_Node*", "child_list_head", 0x4),
    member("Scene_Node*", "sibling_link", 0x8),
    member("Scene_LocalTransform", "transform", 0xC),
    member("uint8_t", "node_type", 0x1A),
    member("uint8_t", "padding_1b", 0x1B),
    member("uint16_t", "padding_1c", 0x1C),
    member("uint8_t", "padding_1e[2]", 0x1E),
    member("int32_t", "extra_data", 0x20),
    size=36,
    doc="Compact scene sub-node payload/resource-record variant; no direct loader/parser owner has been validated yet.",
)
BLUEPRINT = unstable
