#!/usr/bin/env python3
from __future__ import annotations

from blueprint import (
    UNKNOWN_PARAMS,
    Blueprint,
    CallingConvention,
    HookKind,
    Required,
    enum_value,
    hook,
    member,
    param,
    xref,
)

stable = Blueprint("stable")

stable.type_alias("Audio_AIL_HSample", "void*")

stable.type_alias("Audio_AIL_HStream", "void*")

stable.type_alias("Audio_AIL_HDigitalDriver", "void*")

stable.type_alias("DInput_JoystickState", "struct DIJOYSTATE")

stable.type_alias("DInput_DataFormat", "void")

stable.type_alias("D3D_DeviceDesc7", "void")

stable.type_alias("DDraw_SurfaceDesc2", "void")

stable.type_alias("DDraw_PixelFormat", "void")

stable.type_alias("DInput_DeviceInstanceA", "void")

stable.struct(
    "DInput_DeviceEnumContext",
    member("uint32_t", "count", 0x0),
    member("Win32_GUID*", "guid_list", 0x4),
    size=8,
)


stable.struct(
    "Animation_ChainEntry",
    member("Animation_DataBlock*", "animation_data_ptr", 0x0),
    member("void*", "bone_array_ptr", 0x4),
    member("uint8_t", "chain_type", 0x8),
    member("uint8_t", "state_flags[2]", 0x9),
    member("uint8_t", "blend_mode", 0xB),
    member("Animation_FrameHeader*", "frame_data_ptr", 0xC),
    member("int32_t", "blend_weight", 0x10),
    member("int32_t", "playback_time", 0x14),
    member("int16_t", "chain_index", 0x18),
    member("int16_t", "frame_counter", 0x1A),
    member("int32_t", "transition_time", 0x1C),
    size=32,
)

stable.struct(
    "Animation_ChannelHeader",
    member("uint8_t", "channel_params[7]", 0x0),
    member("uint8_t", "loop_flags", 0x7),
    member("int16_t", "start_keyframe", 0x8),
    member("int16_t", "end_keyframe", 0xA),
    size=12,
)

stable.struct(
    "Animation_ColorKeyframe",
    member("uint16_t", "frame_number", 0x0),
    member("uint16_t", "interpolation_flag", 0x2),
    member("uint32_t", "color", 0x4),
    member("uint32_t", "interpolation_mode", 0x8),
    size=12,
)

stable.struct(
    "Animation_VertexColorTarget",
    member("int16_t", "vertex_index", 0x0),
    member("int16_t", "frame_offset", 0x2),
    member("uint8_t", "base_r", 0x4),
    member("uint8_t", "base_g", 0x5),
    member("uint8_t", "base_b", 0x6),
    member(
        "uint8_t",
        "reserved_07",
        0x7,
        doc="Unused/pad byte after base RGB; Animation_ProcessVertexColor target-byte reads cover +0..+6.",
    ),
    size=8,
)

stable.struct(
    "Animation_VertexColorKeyframe",
    member("int16_t", "delta_r", 0x0),
    member("int16_t", "delta_g", 0x2),
    member("int16_t", "delta_b", 0x4),
    member("int16_t", "frame_number", 0x6),
    member("int32_t", "inv_frame_delta_q12", 0x8),
    size=12,
)

stable.struct(
    "Animation_VertexColorController",
    member(
        "uint8_t",
        "controller_type",
        0x0,
        doc="Observed value 1 for vertex-color controllers.",
    ),
    member(
        "uint8_t",
        "flags",
        0x1,
        doc="Bit 0x80 marks dirty/pending dispatch; low 3 bits select timing mode.",
    ),
    member(
        "uint16_t",
        "signal_id",
        0x2,
        doc="Signal id polled by Render_UpdateMeshCommandFlags, then used for controller updates.",
    ),
    member(
        "int32_t",
        "sample_time_q12",
        0x4,
        doc="Current Q12 playback sample time, advanced by mesh-command updates.",
    ),
    member(
        "uint32_t",
        "playback_limit_q12",
        0x8,
        doc="Q12 playback bound for sample_time_q12, used for clamping or wrapping.",
    ),
    member("uint8_t", "keyframe_count", 0xC),
    member(
        "uint8_t",
        "reserved_0d",
        0xD,
        doc="Alignment byte between keyframe_count and vertex_count; no validated semantic use in vertex-color paths.",
    ),
    member("int16_t", "vertex_count", 0xE),
    member(
        "Animation_VertexColorTarget*",
        "target_array",
        0x10,
        doc="Vertex-color target array with vertex index, frame offset, and base RGB bytes.",
    ),
    member(
        "Animation_VertexColorKeyframe*",
        "keyframe_array",
        0x14,
        doc="Signed RGB delta keyframe array, sampled by Animation_ProcessVertexColor.",
    ),
    size=24,
)

stable.struct(
    "Animation_ControllerEntry",
    member("uint8_t", "controller_type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("int16_t", "event_queue_id", 0x2),
    member("int32_t", "playback_speed", 0x4),
    member("int32_t", "current_time", 0x8),
    member("Animation_DataBlock*", "animation_data_ptr", 0xC),
    member("Animation_SplineChannel*", "param_data_ptr", 0x10),
    member("Animation_FrameHeader*", "frame_sequence_ptr", 0x14),
    member("int32_t", "blend_weight", 0x18),
    member("int32_t", "target_data", 0x1C),
    size=32,
)

stable.struct(
    "Animation_ControllerGroup",
    member("uint8_t", "group_flags", 0x0),
    member(
        "uint8_t",
        "active_controller_count",
        0x1,
        doc="Temporary controller_count override used for one Render_ProcessMeshCommands call; the original 16-bit count is restored afterward.",
    ),
    member("int16_t", "controller_count", 0x2),
    member("Animation_ControllerSlot**", "controller_slot_array", 0x4),
    size=8,
)

stable.struct(
    "Animation_ControllerSlot",
    member("Animation_DataBlock*", "anim_data_ptr", 0x0),
    member("uint8_t", "controller_type", 0x4),
    member("uint8_t", "status_flags", 0x5),
    member(
        "uint16_t",
        "reserved",
        0x6,
        doc=(
            "Unused slot stride/alignment word; Animation_ProcessController reads cover +0x0/+0x4/+0x5/+0x8/+0xC (PC EN)."
        ),
    ),
    member(
        "void*",
        "controller_payload_ptr",
        0x8,
        doc="Variant controller payload pointer containing type-specific byte/dword data interpreted by Animation_ProcessController.",
    ),
    member("Animation_FrameHeader*", "frame_sequence_ptr", 0xC),
    size=16,
)

stable.struct(
    "Animation_Data",
    member("uint8_t", "frame_count", 0x0),
    member("uint8_t", "bone_count", 0x1),
    member("uint8_t", "flags", 0x2),
    member("uint8_t", "reserved", 0x3),
    member("uint32_t", "keyframe_data[6]", 0x4),
    size=28,
)

stable.struct(
    "Animation_DataBlock",
    member("uint8_t", "num_position_channels", 0x0),
    member("uint8_t", "num_rotation_channels", 0x1),
    member("uint8_t", "num_visibility_channels", 0x2),
    member("uint8_t", "num_scale_channels", 0x3),
    member("uint8_t", "num_morph_channels", 0x4),
    member("uint8_t", "num_scalar_channels", 0x5),
    member(
        "uint8_t",
        "reserved_06",
        0x6,
        doc=(
            "Unreferenced byte before fixup_flags; Resource_FixUpAnimationData reads +0x7 (PC EN) and "
            "channel pointers, not +0x6 (PC EN)."
        ),
    ),
    member(
        "uint8_t",
        "fixup_flags",
        0x7,
        doc="Animation block fixup flags; bit 0x02 marks rebased channel pointer tables.",
    ),
    member("int16_t", "start_keyframe", 0x8),
    member("int16_t", "end_keyframe", 0xA),
    member("Animation_SplineChannel*", "position_channels", 0xC),
    member("Animation_SplineChannel*", "rotation_channels", 0x10),
    member("Animation_SplineChannel*", "visibility_channels", 0x14),
    member("Animation_SplineChannel*", "scale_channels", 0x18),
    member("Animation_SplineChannel*", "morph_channels", 0x1C),
    member("Animation_SplineChannel*", "scalar_channels", 0x20),
    size=36,
)


stable.struct(
    "Animation_FrameData",
    member("uint8_t", "frame_type", 0x0),
    member(
        "uint8_t",
        "reserved_01",
        0x1,
        doc="Opaque/alignment byte; no stable meaning is known.",
    ),
    member("int16_t", "normal_count", 0x2),
    member("int16_t", "vertex_count", 0x4),
    member(
        "uint8_t",
        "reserved_06[2]",
        0x6,
        doc="Opaque/alignment bytes before normal_data; no stable semantics have been isolated.",
    ),
    member("Mesh_VertexNormal*", "normal_data", 0x8),
    member("uint8_t", "frame_extra_data[10]", 0xC),
    member(
        "uint8_t",
        "reserved_16[2]",
        0x16,
        doc="Opaque/alignment tail bytes; no stable semantics have been isolated.",
    ),
    size=24,
)

stable.struct(
    "Animation_FrameHeader",
    member("uint8_t", "fixup_flags", 0x0),
    member("uint8_t", "padding_01", 0x1),
    member("int16_t", "frame_count", 0x2),
    member("Animation_FrameData**", "frame_ptrs", 0x4),
    size=8,
)

stable.struct(
    "Animation_FrameVertex",
    member("Math_Vec3i16", "pos", 0x0),
    member("int16_t", "padding", 0x6),
    size=8,
)

stable.struct(
    "Animation_MorphKeyframe",
    member("uint32_t", "timing_flags", 0x0),
    member("int16_t", "morph_target_index", 0x4),
    member("int16_t", "blend_weight", 0x6),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_id", 0x9),
    member("int16_t", "flags", 0xA),
    size=12,
)

stable.struct(
    "Animation_MorphTargetVertex",
    member("Math_Vec3i16", "delta", 0x0),
    member("int16_t", "flags", 0x6),
    size=8,
)

stable.struct(
    "Animation_MorphVertex",
    member("Math_Vec3i16", "delta", 0x0),
    member("int16_t", "flags", 0x6),
    member("Math_Vec3i16", "normal", 0x8),
    member("int16_t", "vertex_index", 0xE),
    size=16,
)

stable.struct(
    "Animation_PositionKeyframe",
    member("uint32_t", "timing_flags", 0x0),
    member("Math_Vec3i32", "pos", 0x4),
    member("Math_Vec3i32", "tangent_out", 0x10),
    member("Math_Vec3i32", "tangent_in", 0x1C),
    size=40,
)

stable.struct(
    "Animation_RotationKeyframe",
    member("uint32_t", "timing_flags", 0x0),
    member("Math_Quaternioni16", "quat", 0x4),
    member("uint32_t", "segment_flags", 0xC),
    member("Math_Quaternioni16", "tangent_out", 0x10),
    member("Math_Quaternioni16", "tangent_in", 0x18),
    size=32,
)

stable.struct(
    "Animation_ScalarKeyframe",
    member("uint32_t", "timing_flags", 0x0),
    member("int32_t", "value", 0x4),
    member("int32_t", "tangent_out", 0x8),
    member("int32_t", "tangent_in", 0xC),
    size=16,
)

stable.struct(
    "Animation_SplineChannel",
    member(
        "void*",
        "keyframe_table_or_minus_one_sentinel",
        0x0,
        doc=(
            "Sampled spline keyframe table pointer, or exactly -1 for the constant/sentinel "
            "case. Sampled channels require a nonzero table pointer."
        ),
    ),
    member("int32_t", "packed_first_key", 0x4),
    member(
        "uint8_t",
        "keyframe_count_must_be_nonzero_when_sampled",
        0x8,
        doc=(
            "Anim_CheckKeyframeActive indexes count - 1 on the sampled-channel path; "
            "a zero count is invalid for sampled channels."
        ),
    ),
    member("uint8_t", "target_index", 0x9),
    member("int16_t", "channel_flags", 0xA),
    size=12,
)

stable.struct(
    "Animation_StateTable",
    member("Animation_DataBlock*", "anim_ptrs[43]", 0x0),
    size=172,
)


stable.struct(
    "Animation_VisibilityKeyframe",
    member("uint32_t", "timing_and_visibility", 0x0),
    size=4,
)

stable.struct(
    "Audio_SampleHandle",
    member("int32_t", "sound_id", 0x0),
    member("int32_t", "volume", 0x4),
    member("int32_t", "frequency", 0x8),
    member("int32_t*", "sample_handle", 0xC),
    member("int32_t", "status_flags", 0x10),
    size=20,
)

stable.struct(
    "Audio_SampleHandleEntry",
    member("Audio_SampleHandleEntry*", "prev_ptr", 0x0),
    member("Audio_SampleHandleEntry*", "next_ptr", 0x4),
    member("int32_t*", "ail_handle", 0x8),
    member("Audio_SoundEntry*", "sound_entry", 0xC),
    member("uint32_t", "additional_data", 0x10),
    size=20,
)


stable.struct(
    "Audio_SoundDescriptor",
    member("int32_t", "sound_id", 0x0),
    member("int32_t", "flags", 0x4),
    size=8,
)

stable.struct(
    "Audio_SoundEntry",
    member("Audio_SoundEntry*", "prev", 0x0),
    member("Audio_SoundEntry*", "next", 0x4),
    member("Audio_SoundDefinition*", "sound_def_ptr", 0x8),
    member("Math_Vec3i32*", "position_ptr", 0xC),
    member("Math_Vec3i32", "listener_pos", 0x10),
    member("Math_Vec3i32", "sound_pos", 0x1C),
    member("int16_t", "volume", 0x28),
    member("int16_t", "pitch", 0x2A),
    member("int16_t", "pan", 0x2C),
    member("uint8_t", "flags", 0x2E),
    member("uint8_t", "padding_2f", 0x2F),
    size=48,
)

stable.struct(
    "Audio_SoundDefinition",
    member("void**", "sample_table_ptr", 0x0),
    member(
        "uint8_t",
        "flags",
        0x4,
        doc=(
            "Audio definition flags. ScriptOp_PlaySoundBlockOrWait mutates bit 0x40 as "
            "a script_playback_active_or_wait_latch around direct playback."
        ),
    ),
    member(
        "uint8_t",
        "reserved_05",
        0x5,
        doc=(
            "Reserved audio-definition byte; allocation/playback paths only consume flags at +0x4 "
            "(PC EN) and replacement_priority at +0x6 (PC EN)."
        ),
    ),
    member(
        "uint8_t",
        "replacement_priority",
        0x6,
        doc="Priority byte used by Audio_AllocateSoundSlot when selecting a non-protected active sound to evict; a requested sound can replace an active sound when requested priority is less than or equal to the active definition priority.",
    ),
    member(
        "uint8_t",
        "reserved_07",
        0x7,
        doc=(
            "Reserved audio-definition byte; allocation/playback paths only consume "
            "replacement_priority at +0x6 (PC EN) before volume/pitch."
        ),
    ),
    member("int32_t", "volume_fp12", 0x8),
    member("int32_t", "pitch_fp12", 0xC),
    member("void*", "spatial_data_ptr", 0x10),
    size=20,
    doc="20-byte sound definition for Audio_PlaySoundDefinition3D and Audio_AllocateSoundSlot, with sample table, flags, replacement priority, fixed-point volume/pitch, optional spatial data, and script playback latch at flags bit 0x40.",
)

stable.struct(
    "Audio_SoundSlot",
    member("int32_t", "cached_volume", 0x0),
    member("int32_t", "cached_pan", 0x4),
    member("int32_t", "cached_playback_rate", 0x8),
    member("int32_t", "base_playback_rate", 0xC),
    member("Audio_AIL_HSample", "sample_handle", 0x10),
    size=20,
)

stable.struct(
    "Audio_WaveFormat",
    member("int16_t", "format_tag", 0x0),
    member("int16_t", "channels", 0x2),
    member("int16_t", "bits_per_sample", 0x4),
    member("int16_t", "block_align", 0x6),
    size=8,
)

stable.struct(
    "Camera_Frustum",
    member("Camera_FrustumClipPlane", "clip_planes[5]", 0x0),
    member("int32_t", "state_flags[2]", 0x50),
    member("D3D_IDirect3DDevice7*", "d3d_device", 0x58),
    member("Math_Vec2i32", "viewport_pos", 0x5C),
    member("int32_t", "viewport_w", 0x64),
    size=104,
)

stable.struct(
    "Camera_FrustumClipPlane",
    member("Math_Vec3i32", "normal", 0x0),
    member("int32_t", "distance", 0xC),
    size=16,
)

stable.struct(
    "Camera_FrustumPlane",
    member("Math_Vec3i32", "normal", 0x0),
    size=12,
)

stable.struct(
    "Camera_RenderData",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "mode", 0x1),
    member("int16_t", "state", 0x2),
    member("int16_t", "pitch", 0x4),
    member("int16_t", "yaw", 0x6),
    member("int16_t", "roll", 0x8),
    member("int16_t", "fov_angle", 0xA),
    member("int32_t", "clip_distance", 0xC),
    member("Math_Vec2i32", "position_xy", 0x10),
    member("int32_t", "target_x", 0x18),
    member("int32_t", "target_z", 0x1C),
    member("Math_Vec2i16", "viewport_pos", 0x20),
    member("int16_t", "view_matrix[20]", 0x24),
    member("uint8_t", "combined_view_matrix[48]", 0x4C),
    member("uint8_t", "viewport_clip_data[12]", 0x7C),
    member(
        "int16_t",
        "world_rot_reserved",
        0x88,
        doc="Opaque/pad word after camera view matrix data; no stable meaning is known.",
    ),
    member(
        "uint8_t",
        "reserved_8a[2]",
        0x8A,
        doc="Opaque/pad bytes before view translation fields; no stable semantics have been isolated.",
    ),
    member("Math_Vec2i32", "view_translation_xy", 0x8C),
    member("int32_t", "position_z", 0x94),
    member("int32_t", "near_clip_distance", 0x98),
    member("int32_t", "max_render_distance", 0x9C),
    member("uint8_t", "projection_extra[8]", 0xA0),
    member("void*", "scene_setup_callback", 0xA8),
    member(
        "uint8_t",
        "reserved_ac[24]",
        0xAC,
        doc="Opaque trailing camera-render scratch/reserved block after scene_setup_callback; no stable semantic split has been isolated.",
    ),
    size=196,
)

stable.struct(
    "Camera_EntityView",
    member("uint32_t", "flags", 0x0),
    member("Math_Vec3i32", "pos", 0x4),
    member("uint32_t", "activation_radius", 0x10),
    member("uint32_t", "actor_activation_radius", 0x14),
    member("int16_t", "camera_def_index", 0x18),
    member("int16_t", "entity_index", 0x1A),
    member("uint8_t", "neighbor_entity_links[8]", 0x1C),
    member("uint8_t", "zone_boundaries[32]", 0x24),
    member("Pkg_ActorTemplate*", "actor_template", 0x44),
    member("uint32_t", "action_data[2]", 0x48),
    member("int32_t*", "camera_path_data", 0x50),
    member("uint8_t", "path_parameters[16]", 0x54),
    member("uint8_t", "camera_collision_volume[84]", 0x64),
    member("int32_t*", "transform_pointer", 0xB8),
    member("uint8_t", "view_transform_matrix[44]", 0xBC),
    member("int32_t", "rotation_calc", 0xE8),
    member("uint8_t", "camera_anim_state[52]", 0xEC),
    member("Actor_State*", "spawned_actor", 0x120),
    member("uint8_t", "linked_actor_data[16]", 0x124),
    member("uint32_t", "timestamp", 0x134),
    member("uint8_t", "transition_timers[20]", 0x138),
    member("Math_Vec3i32", "target_pos", 0x14C),
    member("uint8_t", "look_at_target_data[32]", 0x158),
    size=376,
)

stable.struct(
    "Camera_State",
    member("int32_t", "mode", 0x0),
    member("Math_Vec3i32", "cam_offset", 0x4),
    member("Math_Vec3i32", "target_offset", 0x10),
    member("int16_t", "fov_distance", 0x1C),
    member("int16_t", "transition_speed", 0x1E),
    member("int32_t", "flags", 0x20),
    size=36,
)

stable.struct(
    "Camera_Pose",
    member(
        "int16_t",
        "angle_vertical",
        0x0,
        doc="First wrapped 12-bit camera angle used by Camera_CalculatePosition.",
    ),
    member(
        "int16_t",
        "angle_horizontal",
        0x2,
        doc="Second wrapped 12-bit camera angle used by Camera_CalculatePosition.",
    ),
    member(
        "int16_t",
        "orbit_yaw",
        0x4,
        doc="Wrapped 12-bit orbit yaw interpolated by Camera_InterpolateTransition.",
    ),
    member(
        "int16_t",
        "fov",
        0x6,
        doc="Field-of-view value interpolated during camera transitions.",
    ),
    member(
        "int32_t",
        "distance_or_clip",
        0x8,
        doc="Copied pose scalar at +0x08 (PC EN) used by camera pose interpolation paths.",
    ),
    member(
        "Math_Vec3i32",
        "eye_pos",
        0xC,
        doc="Eye/source position interpolated directly or recomputed from target distance.",
    ),
    member(
        "Math_Vec3i32",
        "target_pos",
        0x18,
        doc="Look-at/target position interpolated during camera transitions.",
    ),
    size=36,
    doc=(
        "Scratch camera pose for transition interpolation, stored at live camera state +0x04 (PC EN) "
        "and copied to a stack target before easing back."
    ),
)

stable.struct(
    "Camera_TransitionState",
    member(
        "uint32_t",
        "flags_and_state",
        0x0,
        doc="Low byte has transition bit 0 cleared by Camera_InterpolateTransition.",
    ),
    member(
        "Camera_Pose",
        "pose",
        0x4,
        doc="Embedded live camera pose interpolated toward a target pose.",
    ),
    size=40,
    doc="Live camera-state prefix read by Camera_InterpolateTransition.",
)

stable.struct(
    "Checkers_Board",
    member("uint8_t", "cells[8][4]", 0x0),
    size=32,
)

stable.struct(
    "Collision_BoundingSphere",
    member("Math_Vec3i32", "center", 0x0),
    member("int32_t", "radius", 0xC),
    size=16,
)

stable.struct(
    "Collision_HitEvent",
    member("Actor_State*", "actor", 0x0),
    member("uint32_t", "start_frame", 0x4),
    member("uint32_t", "expire_frame", 0x8),
    size=12,
)

stable.struct(
    "Collision_Plane",
    member("Math_Vec3i32", "normal", 0x0),
    member("int32_t", "distance", 0xC),
    member("int32_t", "edge_index", 0x10),
    member("int32_t", "polygon_index", 0x14),
    member("int32_t", "surface_type", 0x18),
    size=28,
)

stable.struct(
    "Collision_Polygon",
    member("Collision_Plane*", "plane_data", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member(
        "void*",
        "adj_face_ptr",
        0xC,
        doc=(
            "Face-plane/adjacency data owned by the Collision_Node query, matching "
            "Pkg_CollisionFacePlane normal fields and packed adj_edge words."
        ),
    ),
    member("uint16_t", "flags", 0x10),
    member("int16_t", "material_index", 0x12),
    member("int16_t", "adj_edge_0", 0x14),
    member("int16_t", "padding_16", 0x16),
    size=24,
    doc=(
        "24-byte collision polygon array element owned by Collision_Node.polygons. Vertex indices "
        "reference the owning node's vertex array."
    ),
)

stable.struct(
    "Collision_Response",
    member("Math_Vec3i16", "surface_normal", 0x0),
    member("int16_t", "padding", 0x6),
    member("Math_Vec3i32", "response_vel", 0x8),
    member("int16_t", "penetration_depth", 0x14),
    member("uint16_t", "landing_state", 0x16),
    size=24,
)

stable.struct(
    "Collision_Slot",
    member("Math_Vec3i32", "normal", 0x0),
    member("uint8_t", "contact_position[4]", 0xC),
    member("uint8_t", "response_data[20]", 0x10),
    size=36,
)


stable.struct(
    "Component_CollisionBox",
    member("Math_Vec3i16", "half_width", 0x0),
    member("uint8_t", "is_box", 0x6),
    member("uint8_t", "padding_07[1]", 0x7),
    size=8,
)

stable.struct(
    "Component_Definition",
    member("uint32_t", "flags", 0x0),
    member("int32_t", "initial_vel_x", 0x4),
    member("int32_t", "initial_vel_z", 0x8),
    member("int32_t", "gravity", 0xC),
    member("int32_t", "homing_strength", 0x10),
    member("int32_t", "lifetime_range", 0x14),
    member("int16_t", "speed_min", 0x18),
    member("int16_t", "speed_variance", 0x1A),
    member("int32_t", "speed_max", 0x1C),
    member("int32_t", "collision_damage_type", 0x20),
    member("int32_t", "scatter_angle_x", 0x24),
    member("int32_t", "scatter_angle_z", 0x28),
    member("int32_t", "bounce_factor", 0x2C),
    member("int32_t", "trail_effect_id", 0x30),
    member("int32_t", "return_to_owner", 0x34),
    member("int32_t", "collision_radius", 0x38),
    member("int32_t", "collision_height", 0x3C),
    member("int32_t", "minigame_params", 0x40),
    member("int32_t", "sound_effect_id", 0x44),
    member("uint8_t", "team_id", 0x48),
    member("uint8_t", "spawn_limit_counter", 0x49),
    member("uint8_t", "collision_layer", 0x4A),
    member("uint8_t", "render_priority", 0x4B),
    member("int32_t", "spawn_offset_x", 0x4C),
    member("int16_t", "damage_amount", 0x50),
    member("int16_t", "damage_cooldown", 0x52),
    member("int32_t", "particle_count", 0x54),
    member("int32_t", "particle_spread", 0x58),
    member("int32_t", "particle_lifetime", 0x5C),
    member("int32_t", "particle_color", 0x60),
    member(
        "int32_t",
        "movement_mode",
        0x64,
        doc="Actor/component movement behavior selector consumed by movement/update paths.",
    ),
    member("int32_t", "visual_scale", 0x68),
    member("int32_t", "attachment_bone_id", 0x6C),
    size=112,
)


stable.struct(
    "Component_MeshHeader",
    member("uint32_t", "mesh_flags", 0x0),
    member("Mesh_Node*", "mesh_instance_ptr", 0x4),
    member("Animation_DataBlock*", "animation_data_ptr", 0x8),
    member("uint16_t", "lod_flags", 0xC),
    member("uint16_t", "lod_distance_threshold", 0xE),
    member("uint8_t", "mesh_block_data[108]", 0x10),
    member("Animation_ControllerSlot*", "animation_controller_ptr", 0x7C),
    size=128,
)

stable.struct(
    "Component_SpawnParams",
    member("Actor_State*", "owner", 0x0),
    member("Component_Definition*", "definition", 0x4),
    member("Math_Vec3i32", "initial_pos", 0x8),
    member("Actor_State*", "target_actor", 0x14),
    member("int32_t", "spawn_flags", 0x18),
    size=28,
)

stable.struct(
    "Component_TrailObject",
    member("Math_Vec3i16", "bone_offset", 0x0),
    member("int16_t", "bone_index", 0x6),
    member("uint8_t", "max_segments", 0x8),
    member("uint8_t", "processed_flag", 0x9),
    member("uint8_t", "active_count", 0xA),
    member("uint8_t", "flags", 0xB),
    member("uint8_t", "color_data", 0xC),
    member("uint8_t", "color_g", 0xD),
    member("uint8_t", "color_b", 0xE),
    member("uint8_t", "color_a", 0xF),
    member("int16_t", "width_start", 0x10),
    member("int16_t", "width_end", 0x12),
    member("int16_t", "fade_rate", 0x14),
    member("int16_t", "lifetime", 0x16),
    member("int16_t", "head_index", 0x18),
    member("int16_t", "segment_index", 0x1A),
    member("Trail_Segment*", "segment_array", 0x1C),
    size=32,
)

stable.struct(
    "D3D_DriverInfo",
    member("uint8_t", "driver_guid[40]", 0x0),
    member("uint8_t*", "driver_guid_ptr", 0x28),
    member("uint8_t", "device_capabilities[236]", 0x2C),
    member("uint32_t", "has_hardware_accel", 0x118),
    member("uint8_t", "display_mode_list[888]", 0x11C),
    member("uint32_t", "next_driver_offset", 0x494),
    member("uint8_t", "driver_flags[4]", 0x498),
    member("char*", "driver_description", 0x49C),
    member("uint8_t", "driver_extra_data[12]", 0x4A0),
    member("uint32_t", "colorkey_capability", 0x4AC),
    member("uint32_t", "capability_flags[3]", 0x4B0),
    member(
        "uint8_t",
        "display_mode_workspace[9936]",
        0x4BC,
        doc="DDraw_SurfaceDesc2 display-mode workspace and tail counters populated by DirectDraw enumeration callbacks; not reserved.",
    ),
    size=11148,
)


stable.struct(
    "File_Handle",
    member("char*", "ptr", 0x0),
    member("int32_t", "cnt", 0x4),
    member("char*", "base", 0x8),
    member("int32_t", "flag", 0xC),
    member("int32_t", "file", 0x10),
    member("int32_t", "charbuf", 0x14),
    member("int32_t", "bufsiz", 0x18),
    member("char*", "tmpfname", 0x1C),
    size=32,
)

stable.struct(
    "File_OpenMode",
    member("int32_t", "access", 0x0),
    member("int32_t", "share", 0x4),
    size=8,
)

stable.struct(
    "Input_Event",
    member(
        "int32_t",
        "type",
        0x0,
        doc="Input event type discriminator used by keyboard, gamepad, and menu/event handling.",
    ),
    member(
        "int32_t",
        "value",
        0x4,
        doc="Input event payload, usually a button/key value associated with the event type.",
    ),
    size=8,
    doc="Compact input event record, passed through game input processing.",
)

stable.struct(
    "Input_JoystickState",
    member(
        "Math_Vec3i32",
        "pos",
        0x0,
        doc="DInput_JoystickState lX/lY/lZ axes; input paths threshold X/Y for gamepad controls.",
    ),
    member(
        "Math_Vec3i32",
        "rot",
        0xC,
        doc="DInput_JoystickState lRx/lRy/lRz axes; Rz is thresholded for gamepad controls.",
    ),
    member("int32_t", "sliders[2]", 0x18),
    member("uint32_t", "pov_hat[4]", 0x20),
    member(
        "uint8_t",
        "rgb_buttons[32]",
        0x30,
        doc="DInput_JoystickState.rgbButtons prefix. JoyState_GetButtonByte reads byte offset 0x30 (PC EN) + button_index.",
    ),
    size=80,
    doc=(
        "DirectInput joystick snapshot read by Input_ReadGamepad and JoyState_GetAxis* "
        "helpers. Values are frame-local input samples."
    ),
)

stable.struct(
    "Input_State",
    member(
        "uint32_t", "button_bits", 0x0, doc="Current sampled button/control bitfield."
    ),
    member(
        "int16_t",
        "axis_x",
        0x4,
        doc="Primary horizontal analog axis in signed Q12-style units.",
    ),
    member(
        "int16_t",
        "axis_y",
        0x6,
        doc="Primary vertical analog axis in signed Q12-style units.",
    ),
    member(
        "int16_t",
        "axis_aux_0",
        0x8,
        doc="Secondary/alternate analog axis slot read by extended control codes.",
    ),
    member(
        "int16_t",
        "axis_aux_1",
        0xA,
        doc="Secondary/alternate analog axis slot read by extended control codes.",
    ),
    size=12,
    doc="Runtime input snapshot, shared by keyboard, joystick, gamepad, and script input checks.",
)


stable.struct(
    "Level_BlobHeader",
    member("uint32_t", "material_section_offset", 0x0),
    member("uint32_t", "scene_graph_offset", 0x4),
    member("uint32_t", "mesh_collision_offset", 0x8),
    size=12,
    doc="Package level-blob header whose relative offsets are rebased from the loaded blob base. Only these offsets are modeled; the payload fields are not.",
)


stable.struct(
    "Level_DataHeader",
    member("int32_t", "resource_manager_offset", 0x0),
    member("int32_t", "object_tree_offset", 0x4),
    member("int32_t", "level_data_offset", 0x8),
    size=12,
    doc="Level data header containing relative offsets fixed up against the loaded level-data relocation base. Only these offsets are modeled.",
)

stable.struct(
    "Level_Header",
    member("uint32_t", "magic", 0x0),
    member("uint32_t", "version", 0x4),
    member("uint32_t", "material_table_off", 0x8),
    member("uint32_t", "object_tree_off", 0xC),
    member("uint32_t", "collision_off", 0x10),
    member("uint32_t", "data_size", 0x14),
    size=24,
    doc="Level-file header for the material, object-tree, and collision relative offsets. The payload after the header is still undocumented.",
)


stable.struct(
    "Material_BlendTextureSet",
    member("DDraw_IDirectDrawSurface7*", "quadrants[4]", 0x0),
    size=16,
)

stable.struct(
    "Material_Descriptor",
    member("uint16_t", "flags", 0x0),
    member("Math_Sizeu8", "dimensions_minus_1", 0x2),
    member("uint32_t", "pixel_data_ofs", 0x4),
    member("uint32_t", "palette_ofs", 0x8),
    size=12,
)

stable.struct(
    "Material_RuntimeDescriptor",
    member("uint16_t", "flags", 0x0),
    member("Math_Sizeu8", "dimensions_minus_1", 0x2),
    member("uint8_t*", "pixel_data", 0x4),
    member("uint16_t*", "palette", 0x8),
    member("DDraw_IDirectDrawSurface7*", "texture_handles[25]", 0xC),
    member(
        "uint32_t",
        "average_transparent_color",
        0x70,
        doc=(
            "Packed average RGB fill color for transparent/black pixels. Material_LoadTexture resets it "
            "to 0xffffffff, may store the material-table average color at +0x70 (PC EN), and "
            "Material_CopyPixelDataToTexture substitutes the computed average RGB when alpha processing "
            "sees fully black pixels."
        ),
    ),
    size=116,
)

stable.struct(
    "Render_SpriteContext",
    member("uint32_t", "flags", 0x0),
    member("Material_RuntimeDescriptor*", "texture_descriptor", 0x4),
    member(
        "uint8_t",
        "reserved_08[6]",
        0x8,
        doc="Opaque sprite context gap before the validated glyph advance adjustment; no stable per-byte semantics have been isolated.",
    ),
    member(
        "int16_t",
        "glyph_advance_adjust",
        0xE,
        doc=(
            "Signed text/glyph advance adjustment read by UI_UpdateAndRenderSprites as the "
            "fallback/spacing word at sprite context +0x0E (PC EN)."
        ),
    ),
    member("uint8_t", "subrect_u", 0x10),
    member("uint8_t", "subrect_v", 0x11),
    member(
        "uint8_t",
        "reserved_12[2]",
        0x12,
        doc=(
            "Tail padding for the 0x14-byte runtime footprint; no independent semantic reads/writes "
            "observed. Render_TexturedSprite may read across these bytes via a masked dword load from "
            "+0x10 (PC EN)."
        ),
    ),
    size=20,
)

stable.struct(
    "Material_Entry",
    member("uint8_t", "pixel_format", 0x0),
    member("uint8_t", "flags_byte_1", 0x1),
    member("Math_Sizeu8", "dimensions_minus_1", 0x2),
    member("uint32_t", "texture_offset", 0x4),
    member("uint32_t", "palette_offset", 0x8),
    member("Math_Sizeu16", "dimensions", 0xC),
    member("uint8_t", "format", 0x10),
    member("uint8_t", "mipmap_count", 0x11),
    member("uint16_t", "reserved", 0x12),
    size=20,
)

stable.struct(
    "Material_SectionHeader",
    member("uint32_t", "node_table_offset", 0x0),
    member(
        "uint32_t",
        "reserved_04",
        0x4,
        doc=(
            "Opaque material-section header word; material fixup/loading paths use offsets/counts "
            "at +0x0 (PC EN)/+0x0C (PC EN)/+0x10 (PC EN)/+0x12 (PC EN)."
        ),
    ),
    member(
        "uint32_t",
        "reserved_08",
        0x8,
        doc="Opaque material-section header word; material loading/fixup has no validated consumer for it.",
    ),
    member("uint32_t", "material_entries_offset", 0xC),
    member("int16_t", "node_count", 0x10),
    member("int16_t", "material_entry_count", 0x12),
    member(
        "uint32_t",
        "reserved_14",
        0x14,
        doc="Opaque material-section header tail word following materialEntryCount at +0x12 (PC EN).",
    ),
    size=24,
    doc="24-byte on-disk material section header, rebased before Material_LoadAllEntries walks node/material tables.",
)

stable.struct(
    "Material_FrameData",
    member(
        "uint32_t",
        "reserved_00",
        0x0,
        doc="Zeroed by Material_BuildStructure before the 16-byte frame record is copied; no stable consumer has been observed.",
    ),
    member(
        "uint32_t",
        "reserved_04",
        0x4,
        doc="Zeroed by Material_BuildStructure before the 16-byte frame record is copied; no stable consumer has been observed.",
    ),
    member(
        "uint32_t",
        "reserved_08",
        0x8,
        doc="Zeroed by Material_BuildStructure before the 16-byte frame record is copied; no stable consumer has been observed.",
    ),
    member(
        "uint32_t",
        "material_node_ref_packed",
        0xC,
        doc="Packed material-node reference written by Material_BuildStructure and masked by Material_FindTextureByFrame to index material nodes.",
    ),
    size=16,
)

stable.struct(
    "Material_FrameSet",
    member(
        "uint16_t",
        "reserved_00",
        0x0,
        doc=(
            "Zero-initialized frame-set header word; material builders/lookups use frame_count at "
            "+0x2 (PC EN) and frame_ptr_array at +0x4 (PC EN)."
        ),
    ),
    member(
        "int16_t",
        "frame_count",
        0x2,
        doc=(
            "Number of frame pointers; Material_BuildStructure writes this at +0x2 (PC EN) and "
            "Material_FindTextureByFrame uses it to bound frame_ptr_array iteration."
        ),
    ),
    member("Material_FrameData**", "frame_ptr_array", 0x4),
    size=8,
    doc="8-byte material animation frame-set header, built for Material_FindTextureByFrame lookups.",
)


stable.struct(
    "Material_DataRef",
    member("uint32_t", "level_base_address", 0x0),
    member("uint32_t", "material_id", 0x4),
    member("Math_Sizeu16", "actual_dimensions", 0x8),
    member("uint8_t", "reserved[12]", 0xC),
    size=24,
)

stable.struct(
    "Material_NodeRef",
    member("int32_t", "material_index", 0x0),
    member("DDraw_IDirectDrawSurface7*", "texture_surface", 0x4),
    member("uint8_t*", "palette_ptr", 0x8),
    member("int16_t", "frame_index", 0xC),
    member("int16_t", "blend_mode", 0xE),
    member("uint32_t", "runtime_flags", 0x10),
    size=20,
)

stable.struct(
    "Material_RefEntry",
    member(
        "Material_SectionHeader*",
        "material_section",
        0x0,
        doc="Material section pointer written by Material_BuildStructure and read by Material_FindTextureByFrame.",
    ),
    member(
        "Material_TableEntry*",
        "material_table_entry",
        0x4,
        doc="36-byte material table entry pointer written by Material_BuildStructure.",
    ),
    member(
        "Material_FrameSet*",
        "frame_set",
        0x8,
        doc="Optional frame-set table written by Material_BuildStructure and searched by Material_FindTextureByFrame.",
    ),
    size=12,
)


stable.struct(
    "Material_State",
    member("uint32_t", "flags", 0x0),
    member("uint32_t", "material_id", 0x4),
    member("Math_Sizeu8", "actual_dimensions", 0x8),
    member("uint16_t", "padding", 0xA),
    member("DDraw_IDirectDrawSurface7*", "texture_handles[4]", 0xC),
    member("DDraw_IDirectDrawSurface7*", "backface_handles[4]", 0x1C),
    member("uint32_t", "ambient_color", 0x2C),
    member("uint32_t", "diffuse_color", 0x30),
    size=52,
)

stable.struct(
    "Material_Table",
    member("uint16_t", "material_count", 0x0),
    member("uint16_t", "flags", 0x2),
    member("Pkg_MaterialTableEntry*", "entries_ptr", 0x4),
    member("uint32_t", "reserved", 0x8),
    member("uint32_t", "materials_offset", 0xC),
    member("uint16_t", "secondary_count", 0x10),
    member("uint16_t", "entry_count", 0x12),
    size=20,
)

stable.struct(
    "Material_TableEntry",
    member("uint16_t", "flags", 0x0),
    member("uint8_t", "flags_bytes[2]", 0x2),
    member(
        "Material_Entry*",
        "material_ptr",
        0x4,
        doc="Runtime/fixed-up texture descriptor or material entry pointer for this 36-byte render material record.",
    ),
    member(
        "uint32_t",
        "material_tint",
        0x8,
        doc="Tint/color word copied into render batches by Render_SetPolygonUVs.",
    ),
    member(
        "Material_TextureInfo",
        "texture_info",
        0xC,
        doc="Texture width/height plus reserved upper bytes copied into render batches.",
    ),
    member("Pkg_UVCoord", "uv_tile_offset", 0x10),
    member(
        "uint8_t",
        "texture_info_hi_reserved[2]",
        0x12,
        doc="Upper bytes of the packed texture/UV info area; no stable semantic read has been isolated.",
    ),
    member(
        "uint8_t",
        "color_adjust_r",
        0x14,
        doc="RGB adjustment byte read by ComputeVertexColors.",
    ),
    member(
        "uint8_t",
        "color_adjust_g",
        0x15,
        doc="RGB adjustment byte read by ComputeVertexColors.",
    ),
    member(
        "uint8_t",
        "color_adjust_b",
        0x16,
        doc="RGB adjustment byte read by ComputeVertexColors.",
    ),
    member("uint8_t", "reserved_17", 0x17),
    member(
        "Material_TableEntry*",
        "next_material_entry",
        0x18,
        doc="Chained 36-byte material-table entry pointer fixed up by Material_FixupAndLoad.",
    ),
    member("uint16_t", "explicit_uv[4]", 0x1C),
    size=36,
)

stable.struct(
    "Material_TextureHashEntry",
    member("Material_Entry*", "material_entry_ptr", 0x0),
    member("Math_Sizei16", "dimensions", 0x4),
    member("DDraw_IDirectDrawSurface7*", "texture_data", 0x8),
    member("Animation_FrameData*", "anim_frame_data", 0xC),
    size=16,
)

stable.struct(
    "Math_AABB",
    member("Math_Vec3i32", "min", 0x0),
    size=12,
)

stable.struct(
    "Math_Matrix3x3i16",
    member("int16_t", "m00", 0x0),
    member("int16_t", "m01", 0x2),
    member("int16_t", "m02", 0x4),
    member("int16_t", "m10", 0x6),
    member("int16_t", "m11", 0x8),
    member("int16_t", "m12", 0xA),
    member("int16_t", "m20", 0xC),
    member("int16_t", "m21", 0xE),
    member("int16_t", "m22", 0x10),
    size=18,
)

stable.struct(
    "Actor_AnimationComponentState",
    member("int16_t", "animation_step", 0x0),
    member("int16_t", "component_counts", 0x2),
    size=4,
)

stable.struct(
    "Actor_State",
    member("Actor_State*", "list_next", 0x0),
    member("Actor_State*", "next_actor", 0x4),
    member("Entity_State*", "owner_entity", 0x8),
    member("int16_t", "anim_state", 0xC),
    member("int16_t", "anim_flags", 0xE),
    member("int16_t", "contact_tangent[6]", 0x10),
    member("int16_t", "contact_normal_z", 0x1C),
    member("int16_t", "contact_normal_z_hi", 0x1E),
    member("Math_Vec3i32", "attach_offset", 0x20),
    member(
        "Math_Matrix3x3i16",
        "rot_mat",
        0x2C,
        doc="Actor-local rotation/render transform matrix initialized by Entity_SpawnActor.",
    ),
    member("int16_t", "rot_mat_padding", 0x3E),
    member(
        "Math_Vec3i32",
        "position",
        0x40,
        doc="Logical actor position in game fixed-point units.",
    ),
    member("Math_Vec3i32", "sub_pos", 0x4C),
    member("int32_t", "attach_flags", 0x58),
    member("int16_t", "anim_seq_index", 0x5C),
    member("int16_t", "anim_seq_timer", 0x5E),
    member("int32_t", "anim_frame_time", 0x60),
    member("uint8_t", "actor_type", 0x64),
    member("uint8_t", "lifecycle_flags", 0x65),
    member("uint8_t", "attach_slot_index", 0x66),
    member("uint8_t", "render_layer", 0x67),
    member(
        "Math_Vec2i16",
        "visual_scale",
        0x68,
        doc=(
            "Unpacked XY visual scale. Fabricated values can destabilize "
            "Actor_ProcessRendering, so engine-owned spawn/render paths are safer "
            "than direct writes."
        ),
    ),
    member("Mesh_Object*", "mesh_data_ptr", 0x6C),
    member("void*", "scene_vertex_data", 0x70),
    member("void*", "scene_anim_data", 0x74),
    member("int16_t", "chain_timer", 0x78),
    member("int16_t", "chain_state", 0x7A),
    member("Animation_ControllerGroup*", "anim_ctrl_ptr", 0x7C),
    member("void*", "anim_controller_root", 0x80),
    member("void*", "collision_list_heads", 0x84),
    member("int32_t", "behavior_flags", 0x88),
    member(
        "int32_t",
        "mesh_polygon_count_or_ground_y",
        0x8C,
        doc=(
            "Render-coupled word at actor+0x8C (PC EN); mesh rendering paths pass it as the "
            "count/limit for 24-byte records reached through mesh_data_ptr, whose copies require coherent "
            "visual state."
        ),
    ),
    member("int32_t", "reserved_90", 0x90),
    member("void*", "attach_point_table", 0x94),
    member(
        "Math_Vec3i32",
        "world_render_pos",
        0x98,
        doc="Render-position mirror used by camera/render paths; transform writes require logical-position coherence.",
    ),
    member("int32_t", "height_offset", 0xA4),
    member("Actor_State*", "collision_next", 0xA8),
    member("Animation_StateTable*", "anim_asset_table", 0xAC),
    member(
        "Animation_MorphTargetVertex**",
        "visual_morph_or_skin_target_table",
        0xB0,
        doc=(
            "Visual morph/skin target pointer table used by Bone_TransformVertices_Weighted at actor+0xB0 "
            "(PC EN); it is paired with borrowed mesh/scene vertex resources."
        ),
    ),
    member("int32_t", "anim_tick", 0xB4),
    member(
        "Actor_AnimationComponentState",
        "animation_component_state",
        0xB8,
        doc=(
            "Unpacked animation/component state: Model_AdvanceAnimation reads the low int16 as per-tick "
            "animation advance, while trail/component paths use the high half around actor+0xBA (PC EN)."
        ),
    ),
    member("Component_TrailObject*", "trail_chain_ptr", 0xBC),
    member("Component_Instance*", "component_array", 0xC0),
    member("uint8_t", "collision_state_a", 0xC4),
    member("uint8_t", "collision_state_b", 0xC5),
    member(
        "uint8_t",
        "actor_collision_subtype_or_zero",
        0xC6,
        doc=(
            "Actor/actor collision subtype byte. Nonzero paths dereference "
            "collision_component_or_parent_component at actor+0xC8 (PC EN)."
        ),
    ),
    member("uint8_t", "collision_state_d", 0xC7),
    member(
        "Component_Instance*",
        "collision_component_or_parent_component",
        0xC8,
        doc="Collision component pointer used when actor_collision_subtype_or_zero is nonzero; preserve it with collision state bytes.",
    ),
    member("int16_t", "fade_timer", 0xCC),
    member(
        "int16_t",
        "scale_factor",
        0xCE,
        doc=(
            "Actor scale factor copied through runtime actor paths. Direct writes require "
            "additional visual-state handling."
        ),
    ),
    member(
        "Scene_Node**",
        "render_node_list",
        0xD0,
        doc=(
            "NULL-terminated runtime render node list read by Scene_RenderNodeTree. "
            "Runtime rendering fixes iterate this list with pointer guards."
        ),
    ),
    member("Math_Vec3i32", "velocity", 0xD4),
    member("int32_t", "spin_angle", 0xE0),
    member("int32_t", "spin_speed", 0xE4),
    member(
        "Collision_Node*",
        "ground_collision_node",
        0xE8,
        doc=(
            "Ground/contact collision or scene-geometry node pointer. Polygon access routes "
            "through the node's polygon/vertex arrays."
        ),
    ),
    member(
        "void*",
        "ground_contact_ptr",
        0xEC,
        doc=(
            "Ground-contact pointer paired with ground_collision_node and passed through polygon/contact helpers."
        ),
    ),
    member("int32_t", "rotation", 0xF0),
    member("Pkg_ActorRecord*", "record_ptr", 0xF4),
    member("Actor_State*", "child_actor", 0xF8),
    member("int32_t", "fade_alpha", 0xFC),
    member("int32_t", "path_trace_mode", 0x100),
    member(
        "Pkg_LODEntry*",
        "level_local_lod_redirect_record",
        0x104,
        doc=(
            "Level-local LOD redirect record; Actor_ProcessRendering follows this record "
            "to an alternate actor/render source tied to the current level."
        ),
    ),
    member("Actor_State*", "parent_actor", 0x108),
    member("int32_t", "collision_push_xz", 0x10C),
    member("int32_t", "collision_response_xz", 0x110),
    member("int32_t", "alloc_size", 0x114),
    member("int32_t", "camera_mat_xz", 0x118),
    member("int32_t", "camera_mat_yz", 0x11C),
    member("int32_t", "camera_pitch", 0x120),
    member("Math_Matrix3x3i16*", "camera_rot_matrix_ptr", 0x124),
    member("Actor_State*", "linked_actor", 0x128),
    member("int32_t", "camera_cos_factor", 0x12C),
    member("void*", "camera_scratch_vec_ptr", 0x130),
    member("int32_t", "camera_yaw", 0x134),
    member("int32_t", "reserved_138", 0x138),
    member("int32_t", "script_timer", 0x13C),
    member("uint8_t", "script_entity_index", 0x140),
    member(
        "uint8_t",
        "script_entity_stack[3]",
        0x141,
        doc="Three contiguous script-entity stack bytes used as an array.",
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
    member("int32_t", "default_coll_radius", 0x174),
    member("int32_t", "default_coll_height", 0x178),
    member("uint32_t", "default_flags", 0x17C),
    member(
        "int32_t",
        "transition_saved_velocity_x",
        0x180,
        doc="Saved pre-transition velocity_x copied from the live actor and restored by Actor_SnapToPosition.",
    ),
    member(
        "int32_t",
        "transition_saved_velocity_y",
        0x184,
        doc="Saved pre-transition velocity_y copied from the live actor and restored by Actor_SnapToPosition.",
    ),
    member(
        "int32_t",
        "transition_saved_velocity_z",
        0x188,
        doc="Saved pre-transition velocity_z copied from the live actor and restored by Actor_SnapToPosition.",
    ),
    member(
        "int16_t",
        "transition_timer",
        0x18C,
        doc="Transition/countdown timer set by camera/snap setup and decremented by Actor_ProcessSnapAndEntityUpdate.",
    ),
    member(
        "int16_t",
        "transition_target_angle",
        0x18E,
        doc="Target facing angle passed into Actor_CalculateRotation during snap/transition updates.",
    ),
    member(
        "int32_t",
        "transition_rotation_param",
        0x190,
        doc="Rotation step/speed parameter passed as Actor_CalculateRotation third argument; the callee scales it before applying the facing update.",
    ),
    member(
        "int16_t",
        "behavior_state",
        0x194,
        doc="Player/AI behavior substate switched by Actor_ProcessPlayerBehavior; nonzero suppresses selected collision/despawn paths.",
    ),
    member(
        "int16_t",
        "behavior_timer",
        0x196,
        doc="Behavior phase timer/frame counter set to animation/countdown durations and decremented by player/AI behavior paths.",
    ),
    member(
        "Component_Instance*",
        "runtime_component_array",
        0x198,
        doc="Pointer to runtime Component_Instance array initialized by Entity_SpawnActor; entries use the 0x224-byte public Component_Instance layout.",
    ),
    member(
        "Actor_State*",
        "tracked_actor_0",
        0x19C,
        doc="Tracked/attached actor pointer slot released by Actor_ReleaseBindings with target refcount decrement.",
    ),
    member(
        "Actor_State*",
        "tracked_actor_1",
        0x1A0,
        doc="Tracked/attached actor pointer slot released by Actor_ReleaseBindings/Actor_ReleaseAttachment with target refcount decrement.",
    ),
    member(
        "int32_t",
        "candidate_actor_distance_0",
        0x1A4,
        doc="Nearest/candidate actor distance for tracked slot 0 used by player behavior and collision scans.",
    ),
    member(
        "Actor_State*",
        "candidate_actor_0",
        0x1A8,
        doc="Candidate actor pointer for tracked slot 0 used by player behavior and collision scans.",
    ),
    member(
        "int32_t",
        "candidate_actor_distance_1",
        0x1AC,
        doc="Nearest/candidate actor distance for tracked slot 1 used by collision scans.",
    ),
    member(
        "Actor_State*",
        "candidate_actor_1",
        0x1B0,
        doc="Candidate actor pointer for tracked slot 1 used by collision scans.",
    ),
    member(
        "int32_t",
        "scratch_reserved[4]",
        0x1B4,
        doc="Zeroed actor scratch tail; no direct semantic references have been isolated for these four dwords.",
    ),
    size=452,
)

stable.struct(
    "Component_Instance",
    member("Component_Instance*", "parent_comp", 0x0),
    member("int32_t", "timer_packed", 0x4),
    member("Component_Instance*", "next_in_chain", 0x8),
    member("Component_Instance*", "prev_in_chain", 0xC),
    member("Scene_Node*", "scene_node_ptr", 0x10),
    member("Component_Definition*", "definition_ptr", 0x14),
    member("Actor_State*", "target_actor", 0x18),
    member("int32_t", "state_word", 0x1C),
    member("int16_t", "flags", 0x20),
    member("int16_t", "flags_hi", 0x22),
    member("int32_t", "collision_flags", 0x24),
    member("int32_t", "behavior_state", 0x28),
    member("int32_t", "local_rot", 0x2C),
    member("int16_t", "local_rot_02", 0x30),
    member("int16_t", "local_rot_hi", 0x32),
    member("int32_t", "local_rot_11", 0x34),
    member("int32_t", "local_rot_20", 0x38),
    member("int32_t", "local_rot_22", 0x3C),
    member("Math_Vec2i32", "local_pos_xy", 0x40),
    member("uint8_t", "spawn_count_byte_0", 0x48),
    member("uint8_t", "active_count", 0x49),
    member("uint8_t", "spawn_count_byte_2", 0x4A),
    member("uint8_t", "active_max", 0x4B),
    member("int32_t", "spawn_interval", 0x4C),
    member("int16_t", "spawn_delay", 0x50),
    member("int16_t", "lifetime", 0x52),
    member("Math_Vec3i32", "spawn_offset", 0x54),
    member("Scene_Node*", "lod_node_ptrs[3]", 0x60),
    member("Scene_Node*", "shadow_node_ptr", 0x6C),
    member("void*", "sound_slot_table", 0x70),
    member("Math_Vec3i32", "local_scale", 0x74),
    member("Math_Vec3i32", "bone_offset", 0x80),
    member("int32_t", "transform_flags", 0x8C),
    member("int32_t", "render_distance_sq", 0x90),
    member("int32_t", "visibility_mask", 0x94),
    member("Math_Vec3i32", "world_pos", 0x98),
    member("Math_Vec3i32", "prev_world_pos", 0xA4),
    member("Math_Vec3i32", "velocity", 0xB0),
    member("int32_t", "acceleration", 0xBC),
    member("int32_t", "ground_height", 0xC0),
    member("int32_t", "projectile_state", 0xC4),
    member("int32_t", "world_pos_ref", 0xC8),
    member("int32_t", "projectile_timer", 0xCC),
    member(
        "Actor_State**",
        "owner_actor_ref",
        0xD0,
        doc="Pointer to an Actor_State* slot; projectile logic dereferences it and checks the owner's lifecycle_flags.",
    ),
    member("Math_Vec3i32", "homing_vel", 0xD4),
    member("int16_t", "yaw_angle", 0xE0),
    member("int16_t", "yaw_angle_prev", 0xE2),
    member("int16_t", "pitch_angle", 0xE4),
    member("int16_t", "pitch_angle_prev", 0xE6),
    member("int32_t", "angular_vel_yaw", 0xE8),
    member("int32_t", "angular_vel_pitch", 0xEC),
    member("int32_t", "spin_rate", 0xF0),
    member("Component_SpawnParams*", "spawn_context", 0xF4),
    member("int32_t", "height_ref_comp", 0xF8),
    member("Component_TrailObject*", "trail_effect_ptr", 0xFC),
    member("int32_t", "collision_group", 0x100),
    member("int32_t", "damage_cooldown_timer", 0x104),
    member("int32_t", "script_vars[71]", 0x108),
    size=548,
)

stable.struct(
    "Entity_State",
    member("uint32_t", "flags", 0x0),
    member(
        "Math_Vec3i32",
        "bonus_respawn_pos",
        0x4,
        doc="Inactive/pre-spawn entity position; also mirrors Entity_State bonus respawn position.",
    ),
    member(
        "int32_t",
        "collision_radius_sq",
        0x10,
        doc="Squared visibility/collision radius used by Entity_UpdateVisibilityAndSpawn.",
    ),
    member(
        "int32_t",
        "collision_height_sq",
        0x14,
        doc="Squared visibility/collision height used by Entity_UpdateVisibilityAndSpawn.",
    ),
    member("uint8_t", "active_flag", 0x18),
    member(
        "uint8_t",
        "bonus_respawn_mode",
        0x19,
        doc="Entity respawn mode byte; Player_RespawnAfterDeath checks entity-slot state at this offset.",
    ),
    member(
        "uint16_t",
        "reserved_1a",
        0x1A,
        doc="Opaque entity record pad.",
    ),
    member("int32_t*", "bonus_respawn_target_pos", 0x1C),
    member("int16_t", "attach_offset_x", 0x20),
    member(
        "int16_t",
        "default_anim_state",
        0x22,
        doc="Default animation state reset to 0xFFFF by Entity_UpdateVisibilityAndSpawn/despawn paths.",
    ),
    member("int32_t", "local_vars[9]", 0x24),
    member("void*", "script_base_ptr", 0x48),
    member("void*", "actor_template_header_ref", 0x4C),
    member("void*", "shared_ref_50", 0x50),
    member("void*", "shared_ref_54", 0x54),
    member(
        "int32_t*",
        "runtime_target_pos",
        0x58,
        doc="Runtime/default target-position pointer copied into cached_actor_defaults by Entity_SpawnActor.",
    ),
    member(
        "uint8_t",
        "reserved_5c[44]",
        0x5C,
        doc="Opaque portion of the 0x68 source block copied by Entity_SpawnActor into cached_actor_defaults; no stable per-field semantics have been isolated.",
    ),
    member(
        "int32_t",
        "respawn_transition_speed",
        0x88,
        doc="Default/source-block transition speed mirrored from Entity_State layout.",
    ),
    member(
        "uint8_t",
        "reserved_8c[52]",
        0x8C,
        doc="Continuation of the copied actor-default block; no stable per-field semantics have been isolated.",
    ),
    member(
        "uint8_t",
        "cached_actor_defaults[104]",
        0xC0,
        doc="Entity_SpawnActor caches 0x68 bytes here; Entity_CopyDataToActor copies this block into Pkg_ActorRecord+0x30 (PC EN).",
    ),
    member("Actor_State*", "active_actor", 0x128),
    member("uint8_t", "padding_12c[16]", 0x12C),
    member("int32_t", "script_timer", 0x13C),
    member("uint8_t", "behavior_index", 0x140),
    member("uint8_t", "behavior_stack[3]", 0x141),
    member(
        "uint8_t",
        "reserved_144[40]",
        0x144,
        doc="Runtime entity scratch before target_offset_phase; no stable per-field semantics have been isolated.",
    ),
    member(
        "int32_t",
        "target_offset_phase",
        0x16C,
        doc="Runtime target-offset accumulator advanced by Entity_UpdateVisibilityAndSpawn.",
    ),
    member(
        "int32_t",
        "target_offset_step",
        0x170,
        doc="Signed per-update target-offset delta; Entity_UpdateVisibilityAndSpawn negates it at bounds.",
    ),
    member(
        "int32_t",
        "default_collision_radius_sq",
        0x174,
        doc="Default collision radius restored into collision_radius_sq by Actor_SetProperty/despawn reset paths.",
    ),
    member(
        "int32_t",
        "default_collision_height_sq",
        0x178,
        doc="Default collision height restored into collision_height_sq by Actor_SetProperty/despawn reset paths.",
    ),
    member(
        "uint32_t",
        "default_flags",
        0x17C,
        doc="Default flags restored by despawn/reset and used as the default flag mask by Actor_SetProperty.",
    ),
    size=384,
    doc=(
        "Runtime entity slot record used by spawn and visibility paths; current_level_data->entity_array "
        "supplies the slot index."
    ),
)

stable.struct(
    "Math_Quaternioni16",
    member("int16_t", "w", 0x0, doc="Scalar component in signed Q14 fixed-point form."),
    member(
        "int16_t", "x", 0x2, doc="X vector component in signed Q14 fixed-point form."
    ),
    member(
        "int16_t", "y", 0x4, doc="Y vector component in signed Q14 fixed-point form."
    ),
    member(
        "int16_t", "z", 0x6, doc="Z vector component in signed Q14 fixed-point form."
    ),
    size=8,
    doc="Four signed Q14 fixed-point quaternion components, used by animation rotation tracks.",
)

stable.struct(
    "Math_OBB",
    member("Math_Vec3i32", "center", 0x0),
    member("Math_Vec3i32", "half", 0xC),
    member("Math_Vec3i32", "axis_0", 0x18),
    member("int32_t", "axis_1_x", 0x24),
    member("int32_t", "axis_1_y", 0x28),
    size=44,
)

stable.struct(
    "Math_Vec3i32",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "z", 0x8),
    size=12,
)

stable.struct(
    "Math_Vec3i32XZY",
    member("int32_t", "x", 0x0),
    member("int32_t", "z", 0x4),
    member("int32_t", "y", 0x8),
    size=12,
    doc="Three signed 32-bit vector components stored in the engine's X/Z/Y camera order.",
)


stable.struct(
    "Math_Vec3u",
    member("uint32_t", "x", 0x0),
    member("uint32_t", "y", 0x4),
    member("uint32_t", "z", 0x8),
    size=12,
)

stable.struct(
    "Math_Vec3f",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "z", 0x8),
    size=12,
)


stable.struct(
    "Math_Vec3i16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "z", 0x4),
    size=6,
)


stable.struct(
    "Math_Vec2i16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    size=4,
)

stable.struct(
    "Math_Vec2i32",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    size=8,
)

stable.struct(
    "Math_BoundingSphereu16",
    member("uint16_t", "x", 0x0),
    member("uint16_t", "y", 0x2),
    member("uint16_t", "z", 0x4),
    member("uint16_t", "radius", 0x6),
    size=8,
)

stable.struct(
    "Math_Sizeu8",
    member("uint8_t", "width", 0x0),
    member("uint8_t", "height", 0x1),
    size=2,
)

stable.struct(
    "Math_Sizei16",
    member("int16_t", "width", 0x0),
    member("int16_t", "height", 0x2),
    size=4,
)

stable.struct(
    "Math_Recti16",
    member("int16_t", "min_x", 0x0),
    member("int16_t", "min_y", 0x2),
    member("int16_t", "max_x", 0x4),
    member("int16_t", "max_y", 0x6),
    size=8,
)

stable.struct(
    "Math_Sizeu16",
    member("uint16_t", "width", 0x0),
    member("uint16_t", "height", 0x2),
    size=4,
)

stable.struct(
    "Math_Sizeu32",
    member("uint32_t", "width", 0x0),
    member("uint32_t", "height", 0x4),
    size=8,
)

stable.struct(
    "Mesh_Command",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("int16_t", "signal_id", 0x2),
    member("int32_t", "progress_q12", 0x4),
    member("int32_t", "limit_q12", 0x8),
    member(
        "uint8_t",
        "payload[4]",
        0xC,
        doc="Start of the command-specific payload bytes inside the 16-byte command prefix; type 0 passes this region to Animation_ProcessController and type 1 uses the same header as a vertex-color command.",
    ),
    size=16,
    doc=(
        "16-byte-aligned, variable-sized mesh animation/render command prefix read by "
        "Render_ProcessMeshCommands and Render_UpdateMeshCommandFlags; both read the common "
        "header through +0x0 (PC EN)..+0x0F (PC EN) and treats payload as command-specific data."
    ),
)

stable.struct(
    "Mesh_CmdList",
    member("char", "type", 0x0),
    member("char", "flags", 0x1),
    member("int16_t", "count", 0x2),
    member("Mesh_Command**", "cmd_ptrs", 0x4),
    size=8,
)

stable.struct(
    "Mesh_RuntimePolygon",
    member(
        "Material_TableEntry*",
        "material_entry",
        0x0,
        doc="Runtime material-table entry pointer; Resource_FixUpMaterialRefs rewrites material indices to 36-byte Material_TableEntry records.",
    ),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("void*", "p_normal_plane", 0xC),
    member("uint16_t", "face_flags", 0x10),
    member("int16_t", "uv_index", 0x12),
    member("uint16_t", "reserved_14", 0x14),
    member("int16_t", "normal_offset", 0x16),
    size=24,
)

stable.struct(
    "Mesh_RuntimeVertex",
    member("Math_Vec3i16", "pos", 0x0),
    member("int16_t", "normal_group_index", 0x6),
    member("uint8_t", "r", 0x8),
    member("uint8_t", "g", 0x9),
    member("uint8_t", "b", 0xA),
    member("uint8_t", "padding", 0xB),
    size=12,
)

stable.struct(
    "Mesh_AccumulatedNormal",
    member("int32_t", "nx", 0x0),
    member("int32_t", "ny", 0x4),
    size=8,
)

stable.struct(
    "Mesh_FaceNormal",
    member("int32_t", "plane_dist", 0x0),
    member("int16_t", "nx", 0x4),
    member("int16_t", "ny", 0x6),
    member("int16_t", "nz", 0x8),
    member("int16_t", "padding", 0xA),
    size=12,
)

stable.struct(
    "Mesh_MaterialRef",
    member("Material_Table*", "material_table_ptr", 0x0),
    member("int32_t", "uv_offset", 0x4),
    member("int32_t", "color_index", 0x8),
    member("void*", "texture_data_ptr", 0xC),
    member("int32_t", "blend_mode", 0x10),
    member("int32_t", "render_flags", 0x14),
    size=24,
)

stable.struct(
    "Mesh_RenderNodeEntry",
    member(
        "uint8_t",
        "reserved_00[10]",
        0x0,
        doc=(
            "Unresolved prefix before render count/flags; Resource_FixUpMeshNode fixes descriptor data at "
            "+0x0C (PC EN) and relocated tail pointer at +0x1C (PC EN)."
        ),
    ),
    member(
        "uint8_t",
        "render_entry_count",
        0xA,
        doc=(
            "Byte count read by Scene_RenderSubMesh from mesh render-node entry +0x0A (PC EN) before "
            "iterating/rendering the submesh span."
        ),
    ),
    member(
        "uint8_t",
        "render_entry_flags",
        0xB,
        doc="Render-entry flag byte tested by Scene_RenderSubMesh at +0x0B (PC EN).",
    ),
    member(
        "Material_RefEntry",
        "material_descriptor",
        0xC,
        doc="0x0C material/command descriptor fixed by Resource_FixUpSpriteEntry when Resource_FixUpMeshNode walks the 0x20-stride render-node table.",
    ),
    member(
        "uint8_t",
        "reserved_18[4]",
        0x18,
        doc=(
            "Opaque gap between the 0x0C material descriptor and the relocated tail pointer; "
            "Resource_FixUpMeshNode's 0x20-stride helper skips these bytes."
        ),
    ),
    member(
        "void*",
        "relocated_tail_ptr",
        0x1C,
        doc="Entry tail pointer rebased by the 0x20-stride render-node fixup helper called from Resource_FixUpMeshNode.",
    ),
    size=32,
    doc="32-byte mesh render-node entry rebased by Resource_FixUpMeshNode's entry-table helper.",
)

stable.struct(
    "Mesh_Node",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3i16", "position", 0x10),
    member("Math_Vec3i16", "rotation", 0x16),
    member("uint16_t", "flags", 0x1C),
    member("uint16_t", "reserved", 0x1E),
    size=32,
)

stable.struct(
    "Mesh_NodeExtended",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3i16", "position", 0x10),
    member("int16_t", "rotation_x", 0x16),
    member("int16_t", "rotation_y", 0x18),
    member("uint16_t", "node_flags_2", 0x1A),
    member("Mesh_Node*", "child_ptr", 0x1C),
    member("Mesh_Node*", "parent_ref", 0x20),
    member("Mesh_Node*", "sibling_ref", 0x24),
    member("void*", "aux_ptr", 0x28),
    member("int16_t", "rot_matrix[9]", 0x2C),
    member("int16_t", "matrix_padding", 0x3E),
    member("Math_Vec3i32", "world_pos", 0x40),
    member("Math_Vec3i32", "velocity", 0x4C),
    member("int16_t", "material_flags", 0x58),
    member("Math_Vec3i16", "bound_extent", 0x5A),
    member("uint8_t", "node_flags", 0x60),
    member("uint8_t", "padding_61[1]", 0x61),
    member("int16_t", "scale_y", 0x62),
    member("Mesh_RuntimePolygon*", "polygon_data_ptr", 0x64),
    member("Mesh_RuntimeVertex*", "vertex_array_ptr", 0x68),
    member("Resource_Manager*", "resource_manager_ref", 0x6C),
    member("Mesh_RuntimeVertex*", "scene_vertex_data", 0x70),
    member("Mesh_VertexNormal*", "normal_source_ptr", 0x74),
    member("uint8_t", "mesh_flags", 0x78),
    member("uint8_t", "mesh_align_padding[3]", 0x79),
    member("int32_t", "lod_distance_threshold", 0x7C),
    member("Material_Entry*", "material_entry_array", 0x80),
    member("int32_t", "vertex_weights_data[6]", 0x84),
    member("Animation_MorphTargetVertex**", "morph_target_array_ptr", 0x9C),
    member("Mesh_Node*", "parent_node_ptr", 0xA0),
    member("int32_t", "render_flags", 0xA4),
    member("Mesh_Node*", "linked_list_ptr", 0xA8),
    member("Math_Vec3i32", "bounding_box_min", 0xAC),
    member("Math_Vec3i32", "bounding_box_max", 0xB8),
    member("int32_t", "bounding_box_radius", 0xC4),
    member("int32_t", "bounding_box_flags", 0xC8),
    member("Mesh_Object*", "special_mesh_data_ptr", 0xCC),
    member("Material_DataRef*", "data_material_ptr", 0xD0),
    member("int32_t", "sort_key", 0xD4),
    member("Material_Entry*", "material_list_ptr", 0xD8),
    size=220,
)

stable.struct(
    "Mesh_NodeFull",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3i16", "position", 0x10),
    member("Math_Vec3i16", "rotation", 0x16),
    member("uint16_t", "flags", 0x1C),
    member("uint16_t", "material_count", 0x1E),
    member("Mesh_Node*", "parent_ref", 0x20),
    member("Mesh_Node*", "sibling_ref", 0x24),
    member("void*", "aux_ptr", 0x28),
    member("int16_t", "rot_matrix[9]", 0x2C),
    member("int16_t", "matrix_padding", 0x3E),
    member("Math_Vec3i32", "world_pos", 0x40),
    member("Math_Vec3i32", "velocity", 0x4C),
    member("int16_t", "material_flags", 0x58),
    member("Math_Vec3i16", "bound_extent", 0x5A),
    member("int16_t", "reserved_60", 0x60),
    member("uint8_t", "node_flags", 0x62),
    member("uint8_t", "render_mode", 0x63),
    member("uint8_t", "subtype_id", 0x64),
    member("uint8_t", "subtype_flags", 0x65),
    member("int16_t", "subtype_count", 0x66),
    member("Material_Entry*", "material_array", 0x68),
    member("Mesh_RuntimeVertex*", "vertex_data", 0x6C),
    member("Mesh_VertexNormal*", "normal_data", 0x70),
    member("Mesh_NodeFull*", "uv_array_ptr", 0x74),
    member("Mesh_RuntimePolygon*", "polygon_data", 0x78),
    member("uint8_t", "controller_slots[4]", 0x7C),
    member("Animation_DataBlock*", "animation_data", 0x80),
    member(
        "void*",
        "group_list_0",
        0x84,
        doc="First of six group-node fixup/list heads rebased by Resource_FixUpGroupNode from +0x84 (PC EN) through +0x98 (PC EN).",
    ),
    member(
        "void*",
        "group_list_1",
        0x88,
        doc="Group-node fixup/list head rebased by Resource_FixUpGroupNode.",
    ),
    member(
        "void*",
        "group_list_2",
        0x8C,
        doc="Group-node fixup/list head rebased by Resource_FixUpGroupNode.",
    ),
    member(
        "void*",
        "group_list_3",
        0x90,
        doc="Group-node fixup/list head rebased by Resource_FixUpGroupNode.",
    ),
    member(
        "void*",
        "group_list_4",
        0x94,
        doc="Group-node fixup/list head rebased by Resource_FixUpGroupNode.",
    ),
    member(
        "void*",
        "group_list_5",
        0x98,
        doc="Sixth group-node fixup/list head rebased by Resource_FixUpGroupNode.",
    ),
    member("void*", "group_linked_list_a", 0x9C),
    member(
        "int32_t",
        "group_reserved_a0",
        0xA0,
        doc=(
            "Opaque 32-bit slot between group_linked_list_a and group_linked_list_b; "
            "Resource_FixUpGroupNode rebases adjacent list pointers at +0x9C (PC EN)/+0xA4 (PC EN) but "
            "intentionally skips +0xA0 (PC EN)."
        ),
    ),
    member("void*", "group_linked_list_b", 0xA4),
    size=168,
)

stable.struct(
    "Mesh_Object",
    member("void*", "node_table_ptr", 0x0),
    member("int16_t", "frame_range_lo", 0x4),
    member("int16_t", "frame_range_hi", 0x6),
    member("int16_t", "anim_frame_start", 0x8),
    member("int16_t", "anim_frame_end", 0xA),
    member("void*", "morph_target_table", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "node_count", 0x12),
    member("uint32_t", "reserved_14", 0x14),
    size=24,
)

stable.struct(
    "Mesh_ObjectNodeEntry",
    member("void*", "scene_node_ptr", 0x0),
    member("int16_t", "index_a", 0x4),
    member("int16_t", "index_b", 0x6),
    member("int16_t", "index_c", 0x8),
    member("int16_t", "index_d", 0xA),
    size=12,
)

stable.struct(
    "Mesh_Polygon",
    member("uint32_t", "material_ptr", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("uint32_t", "face_normal_ptr", 0xC),
    member("uint16_t", "flags", 0x10),
    member("int16_t", "uv_ambient_index", 0x12),
    member("int16_t", "uv_offset", 0x14),
    member("int16_t", "normal_offset", 0x16),
    size=24,
)


stable.struct(
    "Mesh_Vertex",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "z", 0x8),
    size=12,
)

stable.struct(
    "Mesh_Vertex3D",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "z", 0x4),
    member("int16_t", "padding", 0x6),
    size=8,
)

stable.struct(
    "Mesh_Vertex3DNormal",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "z", 0x8),
    member("float", "nx", 0xC),
    member("float", "ny", 0x10),
    member("float", "nz", 0x14),
    member("uint32_t", "color", 0x18),
    size=28,
)

stable.struct(
    "Mesh_VertexColorRGB",
    member("int32_t", "r", 0x0),
    member("int32_t", "g", 0x4),
    member("int32_t", "b", 0x8),
    size=12,
)

stable.struct(
    "Mesh_VertexNormal",
    member("int16_t", "nx", 0x0),
    member("int16_t", "ny", 0x2),
    member("int16_t", "nz", 0x4),
    member("int16_t", "normal_count", 0x6),
    size=8,
)

stable.struct(
    "Mesh_VertexUV", member("int16_t", "u", 0x0), member("int16_t", "v", 0x2), size=4
)

stable.struct(
    "Mesh_WorkingVertex",
    member("Math_Vec3i16", "pos", 0x0),
    member("int16_t", "bone_index", 0x6),
    member("Math_Vec3i16", "normal", 0x8),
    member("int16_t", "padding_0e", 0xE),
    member("int16_t", "u", 0x10),
    member("int16_t", "v", 0x12),
    member("uint32_t", "color", 0x14),
    size=24,
)


stable.struct(
    "Nav_Command",
    member("uint32_t", "command_type", 0x0),
    member("int16_t", "target_x", 0x4),
    member("int16_t", "target_y", 0x6),
    member("int32_t", "target_z", 0x8),
    member("Math_Vec3i32", "pos", 0xC),
    size=24,
)

stable.struct(
    "Nav_NeighborEntry",
    member("uint16_t", "packed_id", 0x0),
    member("int16_t", "cost", 0x2),
    size=4,
)

stable.struct(
    "Nav_Network",
    member("int32_t", "node_count", 0x0),
    member("Nav_Node*", "nodes", 0x4),
    size=8,
)

stable.struct(
    "Nav_Node",
    member("Math_Vec3i32", "pos", 0x0),
    member("uint16_t", "parent_link", 0xC),
    member("int16_t", "neighbor_count", 0xE),
    member("int32_t*", "pathfind_state", 0x10),
    member("Nav_NeighborEntry*", "neighbor_list", 0x14),
    size=24,
)

stable.struct(
    "Nav_PathState",
    member("uint32_t", "cost", 0x0),
    member("uint16_t", "node_id", 0x4),
    member("uint16_t", "parent_backlink", 0x6),
    member("int16_t", "step_count", 0x8),
    member("int16_t", "reserved", 0xA),
    size=12,
)

stable.struct(
    "Physics_State",
    member("int32_t", "gravity", 0x0),
    member("int32_t", "friction", 0x4),
    member("int32_t", "max_velocity", 0x8),
    member("int32_t", "max_fall_speed", 0xC),
    member("Math_Vec3i32", "acceleration", 0x10),
    member("Actor_State*", "ground_object", 0x1C),
    member("Collision_Polygon*", "ground_polygon", 0x20),
    size=36,
)

stable.struct(
    "Pkg_SplashScreen",
    member("uint32_t", "format_tag", 0x0),
    member("uint8_t", "pixel_data[1228800]", 0x4),
    size=1228804,
)

stable.struct(
    "Pkg_SplashScreenEx",
    member("uint32_t", "type_tag", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "reserved", 0x8),
    member("char", "name[16]", 0xC),
    member("uint8_t", "pixel_data[1228800]", 0x1C),
    size=1228828,
)

stable.struct(
    "Pkg_ActorTemplate",
    member("Mesh_Node*", "lod_nodes[3]", 0x0),
    member("uint32_t", "reserved_0c", 0xC),
    member("Animation_StateTable*", "anim_table", 0x10),
    size=20,
)

stable.struct(
    "Pkg_CameraDef",
    member("uint8_t", "camera_type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("int16_t", "transition_speed", 0x2),
    member("int16_t", "fov_distance", 0x4),
    member("int16_t", "orbit_yaw", 0x6),
    member("int32_t", "orbit_pitch", 0x8),
    member("Math_Vec3i32", "cam_offset", 0xC),
    member("Math_Vec3i32", "target_offset", 0x18),
    size=36,
)

stable.struct(
    "Pkg_CollisionFace",
    member("int16_t", "material_index", 0x0),
    member("int16_t", "surface_type", 0x2),
    member(
        "int16_t",
        "vertex_indices_front[3]",
        0x4,
        doc=(
            "Storage-order vertex index run before face_flags. Native suffix order is "
            "0, 1, 3 here; the remaining index lives after the flag bytes."
        ),
    ),
    member("uint8_t", "face_flags", 0xA),
    member("uint8_t", "face_flags_hi", 0xB),
    member(
        "int16_t",
        "vertex_indices_after_flags[1]",
        0xC,
        doc="Final storage-order vertex index after the interleaved face flag bytes.",
    ),
    member("int16_t", "adj_face_index", 0xE),
    member("int32_t", "plane_offset", 0x10),
    size=20,
)

stable.struct(
    "Pkg_CollisionFacePlane",
    member("int32_t", "plane_distance", 0x0),
    member("Math_Vec3i16", "normal", 0x4),
    member("int16_t", "padding_0a", 0xA),
    member("int16_t", "adj_edges[4]", 0xC),
    size=20,
)

stable.struct(
    "Pkg_CollisionHeader",
    member("Math_Sizeu32", "dimensions", 0x0),
    member("uint32_t", "cell_size", 0x8),
    member("uint32_t", "data_offset", 0xC),
    member("uint8_t", "collision_reserved[16]", 0x10),
    size=32,
)

stable.struct(
    "Pkg_CollisionShape",
    member("uint32_t", "face_count", 0x0),
    member("uint32_t", "flags_packed", 0x4),
    member("uint32_t", "grid_params", 0x8),
    member("void*", "material_base_ptr", 0xC),
    member("uint32_t", "type_flags", 0x10),
    member("Math_Vec3i32", "extent", 0x14),
    member("void*", "grid_cell_array", 0x20),
    member("Pkg_CollisionFace*", "face_array", 0x24),
    member("Pkg_CollisionFacePlane*", "plane_array", 0x28),
    member("Pkg_CollisionVertex*", "vertex_array", 0x2C),
    member("int32_t", "sentinel", 0x30),
    member("int32_t", "face_array_count", 0x34),
    member("int32_t", "vertex_count", 0x38),
    member("uint32_t", "reserved_3c", 0x3C),
    size=64,
)

stable.struct(
    "Pkg_CollisionVertex",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "z", 0x4),
    member("int16_t", "nx", 0x6),
    member("int16_t", "ny", 0x8),
    member("int16_t", "nz", 0xA),
    size=12,
)

stable.struct(
    "Collision_Vertex",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "z", 0x4),
    member(
        "int16_t",
        "reserved_06",
        0x6,
        doc="Opaque collision-vertex tail word following x/y/z coordinates.",
    ),
    member(
        "int16_t",
        "reserved_08",
        0x8,
        doc="Opaque collision-vertex tail word; no stable direct semantics have been isolated beyond xyz.",
    ),
    member(
        "int16_t",
        "reserved_0a",
        0xA,
        doc="Opaque collision-vertex tail word; no stable direct semantics have been isolated beyond xyz.",
    ),
    size=12,
)

stable.struct(
    "Collision_Node",
    member(
        "Collision_Node*",
        "next_in_list",
        0x0,
        doc="List link used by static collision-scene node lists.",
    ),
    member(
        "uint8_t",
        "reserved_04[40]",
        0x4,
        doc="Opaque prefix before the node transform matrix.",
    ),
    member(
        "Math_Matrix3x3i16",
        "transform_matrix",
        0x2C,
        doc="Q12 node rotation/scale matrix used when transformed-node flags are set.",
    ),
    member("int16_t", "transform_padding", 0x3E),
    member("Math_Vec3i32", "origin", 0x40),
    member(
        "uint8_t",
        "reserved_4c[28]",
        0x4C,
        doc="Opaque collision-node payload before vertex and face counts.",
    ),
    member(
        "uint16_t",
        "vertex_count",
        0x68,
        doc="Number of 12-byte Collision_Vertex records in vertices.",
    ),
    member(
        "uint16_t",
        "polygon_count",
        0x6A,
        doc="Number of 24-byte Collision_Polygon records in polygons.",
    ),
    member(
        "Collision_Polygon*",
        "polygons",
        0x6C,
        doc="Collision polygon/face array; vertex indices live at offsets +4/+6/+8/+10 in each record.",
    ),
    member(
        "Collision_Vertex*",
        "vertices",
        0x70,
        doc="Collision vertex array with signed int16 x/y/z at offsets +0/+2/+4 and 12-byte stride.",
    ),
    member(
        "uint8_t",
        "reserved_74[20]",
        0x74,
        doc="Opaque node payload between geometry arrays and flags.",
    ),
    member(
        "uint32_t",
        "flags",
        0x88,
        doc="Node flags; collision polygon tests use transformed coordinates when bits 0x22 are set.",
    ),
    size=140,
    doc=(
        "Runtime collision/scene-geometry node layout used by ground and static collision "
        "paths. Vertices are signed int16 triples scaled by the owning level/caller; "
        "matrix transforms are Q12 when enabled by flags."
    ),
)

stable.struct(
    "Pkg_ComponentData",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "component_bytes_01[51]", 0x1),
    member("int32_t", "parent_ref_value", 0x34),
    member("uint8_t", "component_bytes_38[10]", 0x38),
    member("int16_t", "delay_base", 0x42),
    member("uint8_t", "component_bytes_44[2]", 0x44),
    member("int16_t", "delay_random", 0x46),
    member("uint8_t", "component_byte_48", 0x48),
    member("uint8_t", "spawn_count", 0x49),
    member("uint8_t", "num_lod_variants", 0x4A),
    member("uint8_t", "component_byte_4_b", 0x4B),
    member("int16_t", "delay_between", 0x4C),
    member("uint8_t", "component_bytes_4_e[4]", 0x4E),
    member("int16_t", "init_value", 0x52),
    member("uint8_t", "component_bytes_54[12]", 0x54),
    member("Mesh_Node*", "sub_nodes[4]", 0x60),
    size=112,
)

stable.struct(
    "Pkg_FaceNormal",
    member("int32_t", "plane_distance", 0x0),
    member("Math_Vec3i16", "normal", 0x4),
    member("uint8_t", "padding_0a[2]", 0xA),
    size=12,
)

stable.struct(
    "Pkg_GeometryChunk",
    member("uint32_t", "vertex_offset", 0x0),
    member("uint32_t", "vertex_count", 0x4),
    member("uint32_t", "polygon_offset", 0x8),
    member("uint32_t", "polygon_count", 0xC),
    member("uint32_t", "material_ref", 0x10),
    member("uint32_t", "flags", 0x14),
    size=24,
)

stable.struct(
    "Pkg_GeometryResource",
    member("uint32_t", "node_count", 0x0),
    member("uint32_t", "resource_mgr_offset", 0x4),
    member("uint32_t", "material_table_offset", 0x8),
    member("uint32_t", "material_data_size", 0xC),
    member("uint32_t", "total_data_size", 0x10),
    member("uint32_t", "geometry_flags", 0x14),
    member("uint32_t", "secondary_data_size", 0x18),
    member("uint32_t", "flags", 0x1C),
    member("uint32_t", "version", 0x20),
    member("uint8_t", "geometry_reserved[12]", 0x24),
    size=48,
)


stable.struct(
    "Pkg_LevelHeader",
    member("uint32_t", "cam_default_offset", 0x0),
    member("int16_t", "actor_record_count", 0x4),
    member("int16_t", "padding_06", 0x6),
    member("int16_t", "initial_entity_index", 0x8),
    member("int16_t", "entity_count", 0xA),
    member("uint32_t", "entity_array_offset", 0xC),
    member("uint32_t", "sound_definition_list_offset", 0x10),
    member("int32_t", "sound_definition_count", 0x14),
    member("uint32_t", "var_list_offset", 0x18),
    member("int16_t", "powerup_count", 0x1C),
    member("int16_t", "powerup_type_count", 0x1E),
    member("uint32_t", "powerup_list_offset", 0x20),
    member(
        "uint32_t",
        "powerup_slots[16]",
        0x24,
        doc="Fixed 16-slot powerup table following powerup_list_offset; entries retain the original 0x24..0x60 (PC EN) span.",
    ),
    member("uint32_t", "theme_0_offset", 0x64),
    member("uint32_t", "theme_1_offset", 0x68),
    member("uint32_t", "theme_2_offset", 0x6C),
    member("uint32_t", "theme_3_offset", 0x70),
    member("uint32_t", "theme_4_offset", 0x74),
    member("int32_t", "theme_count", 0x78),
    member("uint32_t", "trail_list_offset", 0x7C),
    member("uint32_t", "sprite_list_offset", 0x80),
    member("uint32_t", "nav_net_offset", 0x84),
    member("uint32_t", "usable_materials_offset", 0x88),
    size=140,
    doc="Package level header containing actor, entity-slot, sound, powerup, theme, trail, sprite, nav, and material relative offsets. Modeled fields cover this header's offset table.",
)

stable.struct(
    "Pkg_LevelResource",
    member("uint32_t", "node_count", 0x0),
    member("uint32_t", "resource_mgr_offset", 0x4),
    member("uint32_t", "material_table_offset", 0x8),
    member("uint32_t", "material_count", 0xC),
    size=16,
    doc="Package level-resource header used as the relocation base for resource-manager and material-table offsets. Only those offsets are modeled.",
)

stable.struct(
    "Pkg_MaterialRef",
    member("uint32_t", "texture_id", 0x0),
    member("Material_Descriptor*", "texture_desc_ptr", 0x4),
    member("uint32_t", "properties", 0x8),
    size=12,
)

stable.struct(
    "Pkg_MaterialTableEntry",
    member("uint32_t", "texture_offset", 0x0),
    member("Material_Descriptor*", "texture_desc_ptr", 0x4),
    member("Math_Sizeu16", "dimensions", 0x8),
    member("uint32_t", "runtime_surface", 0xC),
    member("uint32_t", "pixel_data_offset", 0x10),
    member("uint16_t", "format_flags", 0x14),
    member("uint16_t", "mipmap_count", 0x16),
    member("uint32_t", "palette_offset", 0x18),
    member("uint32_t", "extra_flags", 0x1C),
    member("uint32_t", "extra_data", 0x20),
    size=36,
)

stable.struct(
    "Pkg_MenuTextureResource",
    member("uint32_t", "format", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "reserved", 0x8),
    member("char", "filename[16]", 0xC),
    size=28,
)


stable.struct(
    "Pkg_PolygonData",
    member("uint32_t", "flags", 0x0),
    member("Material_Entry*", "material_ptr", 0x4),
    member("uint32_t", "material_tint", 0x8),
    member("Material_TextureInfo", "texture_info", 0xC),
    member("Pkg_UVCoord", "uv_tile_offset", 0x10),
    member("uint8_t", "padding_12[2]", 0x12),
    member("uint8_t", "color_adjust_r", 0x14),
    member("uint8_t", "color_adjust_g", 0x15),
    member("uint8_t", "color_adjust_b", 0x16),
    member("uint8_t", "padding_17", 0x17),
    member("Pkg_PolygonData*", "next_polygon_data_ptr", 0x18),
    member("uint16_t", "explicit_uv[4]", 0x1C),
    size=36,
)

stable.struct(
    "Pkg_PolygonDataRaw",
    member("uint32_t", "material_index", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("uint32_t", "uv_data_offset", 0xC),
    member("uint32_t", "render_flags", 0x10),
    member("uint32_t", "polygon_flags", 0x14),
    size=24,
)

stable.struct(
    "Pkg_PolygonListEntry",
    member("uint32_t", "material_index", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("Pkg_FaceNormal*", "face_normal_ptr", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "tex_coord_index", 0x12),
    member("uint16_t", "reserved", 0x14),
    member("int16_t", "sort_bias", 0x16),
    size=24,
)


stable.struct(
    "Pkg_ResourceDirectory",
    member("uint32_t", "material_blob_offset", 0x0),
    member("uint32_t", "mesh_node_root_offset", 0x4),
    member("uint32_t", "level_data_offset", 0x8),
    size=12,
    doc=(
        "Package resource directory of relative offsets to material, mesh-node, and level-data "
        "sections. Payload data for those sections is not modeled here."
    ),
)

stable.struct(
    "Pkg_ResourceHeader",
    member("uint32_t", "resource_type", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "data_size", 0x8),
    member("uint32_t", "extra_offset", 0xC),
    member("uint32_t", "flags", 0x10),
    member("uint32_t", "secondary_size", 0x14),
    member("uint32_t", "info_1", 0x18),
    member("uint32_t", "info_2", 0x1C),
    member("uint32_t", "reserved", 0x20),
    size=36,
    doc="Package header for one resource: type, relative offsets, sizes, and flags. Resource-specific payloads are still undocumented.",
)

stable.struct(
    "Pkg_ScriptHeader",
    member("uint8_t", "size_b0", 0x0),
    member("uint8_t", "size_b1", 0x1),
    member("uint8_t", "size_b2", 0x2),
    member("uint8_t", "size_b3", 0x3),
    member("uint8_t", "end_b0", 0x4),
    member("uint8_t", "end_b1", 0x5),
    member("uint8_t", "end_b2", 0x6),
    member("uint8_t", "end_b3", 0x7),
    size=8,
)

stable.struct("Pkg_SoundResource", member("uint32_t", "data_offset", 0x0), size=4)

stable.struct("Pkg_SpriteAtlasResource", member("uint32_t", "data_offset", 0x0), size=4)


stable.struct("Pkg_StringEntry", member("uint32_t", "offset", 0x0), size=4)

stable.struct("Pkg_TextureResource", member("uint32_t", "data_offset", 0x0), size=4)

stable.struct(
    "Pkg_TOCEntry",
    member("uint32_t", "offset", 0x0),
    member("uint32_t", "size", 0x4),
    size=8,
    doc="One 8-byte package TOC entry; the package table contains 0x8a file offset/size pairs, with size-lane aliases sharing this storage.",
)


stable.struct(
    "Pkg_UILayoutEntry",
    member("uint32_t", "element_id", 0x0),
    member("uint32_t", "element_type", 0x4),
    member("int16_t", "param_a", 0x8),
    member("int16_t", "param_b", 0xA),
    member("uint32_t", "reserved", 0xC),
    size=16,
)


stable.struct(
    "Pkg_UVCoord", member("uint8_t", "u", 0x0), member("uint8_t", "v", 0x1), size=2
)

stable.struct(
    "Material_TextureInfo",
    member("Math_Sizeu8", "dimensions", 0x0),
    member(
        "uint8_t",
        "reserved[2]",
        0x2,
        doc="Upper bytes of the packed texture-info word; no stable semantic read has been isolated.",
    ),
    size=4,
)

stable.struct(
    "Render_TexWrapMode",
    member("Pkg_UVCoord", "mode", 0x0),
    member(
        "uint8_t",
        "reserved[2]",
        0x2,
        doc="Upper bytes of the packed texture wrap/mode word; no stable semantic read has been isolated.",
    ),
    size=4,
)

stable.struct(
    "Pkg_VertexData",
    member("Math_Vec3i16", "pos", 0x0),
    member("int16_t", "normal_group_index", 0x6),
    member("uint8_t", "r", 0x8),
    member("uint8_t", "g", 0x9),
    member("uint8_t", "b", 0xA),
    member("uint8_t", "a", 0xB),
    size=12,
)

stable.struct(
    "Pkg_VertexNormalGroup",
    member("Math_Vec3i16", "normal", 0x0),
    member("int16_t", "padding", 0x6),
    size=8,
)


stable.struct(
    "Render_Batch",
    member("uint32_t", "flags", 0x0),
    member("uint32_t", "material_index", 0x4),
    member("uint32_t", "texture_id", 0x8),
    member("uint32_t", "batch_flags", 0xC),
    member("uint32_t", "screen_coords[12]", 0x10),
    member("float", "view_space_pos[12]", 0x40),
    member("float", "fog_depth", 0x70),
    member("uint32_t", "vertex_colors[4]", 0x74),
    size=132,
)

stable.struct(
    "Render_FrustumClipPlane",
    member("int32_t", "distance", 0x0),
    member("Math_Vec3i16", "normal", 0x4),
    member(
        "int16_t",
        "padding_0a",
        0xA,
        doc="Alignment/pad word in the 12-byte frustum/clip-plane record.",
    ),
    size=12,
    doc="12-byte Render_ListState frustum/clip-plane record, used by actor visibility checks.",
)

stable.struct(
    "Render_ClipPlane",
    member("Math_Vec3f", "normal", 0x0),
    member("float", "distance", 0xC),
    size=16,
)

stable.struct(
    "Render_ClipUVData",
    member("Math_Vec3i32", "screen", 0x0),
    member("int32_t", "rhw", 0xC),
    member("uint32_t", "color", 0x10),
    member("float", "u", 0x14),
    member("float", "v", 0x18),
    size=28,
)

stable.struct(
    "Render_ClipVertex",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "z", 0x8),
    member("float", "w", 0xC),
    member("float", "color_or_data", 0x10),
    member("float", "u", 0x14),
    member("float", "v", 0x18),
    size=28,
)

stable.struct(
    "Render_ClipAttribute",
    member("float", "components[3]", 0x0),
    size=12,
)


stable.struct(
    "Render_Color32",
    member("uint8_t", "b", 0x0),
    member("uint8_t", "g", 0x1),
    member("uint8_t", "r", 0x2),
    member("uint8_t", "a", 0x3),
    size=4,
)

stable.struct(
    "Render_ComponentData",
    member("uint32_t", "flags", 0x0),
    member("Actor_State*", "parent_actor", 0x4),
    member("Mesh_Node*", "mesh_data_ptr", 0x8),
    member("Mesh_RuntimeVertex*", "vertex_buffer", 0xC),
    member("Mesh_RuntimePolygon*", "index_buffer", 0x10),
    member("int32_t", "mesh_offset", 0x14),
    member("int32_t", "material_offset", 0x18),
    member("int16_t", "lod_level", 0x1C),
    member("int16_t", "render_priority", 0x1E),
    member("Material_Entry*", "texture_ptr", 0x20),
    member("uint32_t", "vertex_count", 0x24),
    member("uint32_t", "polygon_count", 0x28),
    member("int32_t*", "transform_matrix", 0x2C),
    size=48,
)

stable.struct(
    "Render_GradientState",
    member("int32_t", "start_color", 0x0),
    member("int32_t", "end_color", 0x4),
    size=8,
)


stable.struct(
    "Render_Polygon",
    member("Material_Entry*", "material", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("Mesh_FaceNormal*", "face_normal", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "padding", 0x12),
    member("uint16_t", "uv_indices[4]", 0x14),
    size=28,
)

stable.struct(
    "Render_Polygon3D",
    member(
        "uint16_t",
        "vertex_indices_front[3]",
        0x0,
        doc="Storage-order vertex indices before the interleaved flags field.",
    ),
    member("uint16_t", "flags", 0x6),
    member(
        "uint16_t",
        "vertex_indices_after_flags[1]",
        0x8,
        doc="Final storage-order vertex index after flags.",
    ),
    member("uint16_t", "material_index", 0xA),
    size=12,
)

stable.struct(
    "Render_ProjectedVertex",
    member("int16_t", "screen_x", 0x0),
    member("int16_t", "screen_y", 0x2),
    member("int32_t", "screen_z", 0x4),
    size=8,
)

stable.struct(
    "Render_ViewVertex",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "z", 0x8),
    size=12,
)

stable.struct(
    "Render_TexCoord8",
    member("uint8_t", "u", 0x0),
    member("uint8_t", "v", 0x1),
    size=2,
)

stable.struct(
    "Render_PolygonBatchRecord",
    member("uint32_t", "flags", 0x0),
    member("DDraw_IDirectDrawSurface7*", "texture_ptr", 0x4),
    member("uint32_t", "material_tint", 0x8),
    member(
        "Render_TexWrapMode",
        "tex_wrap_mode",
        0xC,
        doc="Texture wrap/mode bytes copied from Pkg_PolygonData texture info, plus reserved upper bytes.",
    ),
    member("Render_PolygonBatchRecord*", "next_in_bucket", 0x10),
    member("Render_ProjectedVertex", "screen_vertices[4]", 0x14),
    member("Render_ViewVertex", "view_vertices[4]", 0x34),
    member("float", "depth_bias", 0x64),
    member(
        "int32_t",
        "face_normal_dot",
        0x68,
        doc="Signed face-normal/dot value used by draw and clipping paths; Render_QuadClipped tests it against zero.",
    ),
    member("uint32_t", "vertex_colors[4]", 0x6C),
    member("Render_TexCoord8", "tex_coords[4]", 0x7C),
    member("uint32_t", "render_state_flags", 0x84),
    size=136,
)

stable.struct(
    "Render_PolygonRenderRef",
    member(
        "Material_TableEntry*",
        "material_entry",
        0x0,
        doc="Runtime material-table entry pointer read by Render_PolygonBatch/Render_SetPolygonUVs.",
    ),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("uint32_t", "normal_offset", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "uv_index", 0x12),
    member("int16_t", "depth_bias", 0x14),
    member(
        "int16_t",
        "depth_bias_q12",
        0x16,
        doc="Fixed-point companion to depth_bias in polygon render refs; validated as depth-bias data for render ref records.",
    ),
    size=24,
)

stable.struct(
    "Render_QuadData",
    member("uint32_t", "flags", 0x0),
    member("Material_Entry*", "material_ptr", 0x4),
    member("uint32_t", "color", 0x8),
    member(
        "uint8_t",
        "texture_width",
        0xC,
        doc="Quad texture width byte read by Render_DrawQuad/quad rendering paths.",
    ),
    member(
        "uint8_t",
        "texture_height",
        0xD,
        doc="Quad texture height byte read by Render_DrawQuad/quad rendering paths.",
    ),
    member(
        "uint16_t",
        "quad_padding",
        0xE,
        doc=(
            "Alignment/pad word between texture dimensions and quad_state; Render_DrawQuad reads "
            "texture_width at +0x0C (PC EN) and texture_height at +0x0D (PC EN), then skips to quad_state "
            "at +0x10 (PC EN)."
        ),
    ),
    member("uint32_t", "quad_state", 0x10),
    member("int16_t", "vertex_x[4]", 0x14),
    member("int16_t", "vertex_y[4]", 0x1C),
    member("float", "vertex_z[4]", 0x24),
    member("uint32_t", "quad_type", 0x34),
    member("uint32_t", "vertex_data[16]", 0x38),
    member("uint8_t", "texture_coords[16]", 0x78),
    member("uint32_t", "vertex_colors[4]", 0x88),
    size=152,
)

stable.struct(
    "Render_QuadRenderData",
    member("uint32_t", "render_flags[5]", 0x0),
    member("Render_ProjectedVertex", "projected_vertices[3]", 0x14),
    member("int16_t", "vertex_3_screen_x", 0x2C),
    member("int16_t", "vertex_3_screen_y", 0x2E),
    member("uint32_t", "sort_data[2]", 0x30),
    member("float", "vertex_0_x", 0x38),
    member("float", "vertex_0_z", 0x3C),
    member("float", "vertex_0_y", 0x40),
    member("float", "vertex_1_x", 0x44),
    member("float", "vertex_1_z", 0x48),
    member("float", "vertex_1_y", 0x4C),
    member("float", "vertex_2_x", 0x50),
    member("float", "vertex_2_y", 0x54),
    member("float", "vertex_3_z", 0x58),
    member("float", "z_offset", 0x5C),
    member("int32_t", "depth_bias_sign", 0x60),
    member("uint32_t", "vertex_tint", 0x64),
    size=104,
)

stable.struct("Render_RGB555Palette", member("uint16_t", "colors[256]", 0x0), size=512)

stable.struct(
    "Render_SpriteData",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "texture_id", 0x8),
    size=12,
)

stable.struct(
    "Render_SpriteLayer",
    member("int32_t", "texture_handle", 0x0),
    member("int32_t", "sprite_count", 0x4),
    member("Render_SpriteData*", "sprite_list_ptr", 0x8),
    member("Render_SpriteLayer*", "next_layer_ptr", 0xC),
    size=16,
)

stable.struct(
    "Render_SpriteNodeData",
    member("uint8_t", "node_type", 0x0),
    member("uint8_t", "sprite_flags", 0x1),
    member("int16_t", "frame_index", 0x2),
    member("Material_Entry*", "material_ptr", 0x4),
    member("Math_Vec3i16", "offset", 0x8),
    member("int16_t", "sort_key", 0xE),
    member("Math_Vec2i16", "bound_extent", 0x10),
    size=20,
)

stable.struct(
    "Render_SpriteVertexData",
    member("Math_Vec3i16", "pos", 0x0),
    member("uint8_t", "color_r", 0x6),
    member("uint8_t", "color_g", 0x7),
    member("uint8_t", "color_b", 0x8),
    member("uint8_t", "normal_x", 0x9),
    member("uint8_t", "normal_y", 0xA),
    member("uint8_t", "vertex_state", 0xB),
    size=12,
)

stable.struct(
    "Render_FloatColorRGB",
    member("float", "r", 0x0),
    member("float", "g", 0x4),
    member("float", "b", 0x8),
    size=12,
)

stable.struct(
    "Render_WorkArea",
    member("uint8_t", "transform_scratch[144]", 0x0),
    member("float", "clip_double_buffer_0[160]", 0x90),
    member("float", "clip_double_buffer_1[160]", 0x310),
    member("uint8_t", "vertex_scratch[104]", 0x590),
    member("float", "color_data[20]", 0x5F8),
    member("Render_FloatColorRGB", "color_channels[4]", 0x648),
    size=1656,
)

stable.struct(
    "SaveGame_Data",
    member(
        "int32_t",
        "version_marker",
        0x0,
        doc="32-bit save-file marker set to 2 before SaveGame_InitOperation writes the 0x1dc-byte buffer; SaveGame_LoadState compares the full dword to 2.",
    ),
    member(
        "int32_t",
        "game_state",
        0x4,
        doc="Copied from game_state by SaveGame_SaveToSlot and restored by SaveGame_LoadState.",
    ),
    member(
        "int32_t",
        "game_settings",
        0x8,
        doc="Copied from game_settings by SaveGame_SaveToSlot and restored by SaveGame_LoadState.",
    ),
    member(
        "int32_t",
        "player_lives_state",
        0xC,
        doc="Copied from player_lives by SaveGame_SaveToSlot and restored by SaveGame_LoadState.",
    ),
    size=16,
)

stable.struct(
    "SaveGame_Slot",
    member(
        "char",
        "max_world_reached",
        0x0,
        doc="Highest world/progress index used by Level_CalculateCompletionPercent as the base completion contribution.",
    ),
    member(
        "uint8_t",
        "is_valid",
        0x1,
        doc="Per-slot valid flag set to 1 by SaveGame_SaveToSlot after copying the 0x5c-byte progress block.",
    ),
    member(
        "uint8_t",
        "puppy_count_backup",
        0x2,
        doc="Backed-up puppy/life count copied from backup_puppy_count by SaveGame_BackupPuppyCount.",
    ),
    member(
        "uint8_t",
        "save_game_init_flag",
        0x3,
        doc="Initialization/progress flag set to 4 by SaveGame_InitializeState.",
    ),
    member(
        "uint8_t",
        "game_complete_flag",
        0x4,
        doc="Game-complete flag written by SaveGame_SetGameComplete.",
    ),
    member(
        "uint8_t",
        "slot_padding[3]",
        0x5,
        doc="Unused alignment bytes before the per-level progress arrays.",
    ),
    member(
        "uint8_t",
        "level_completion_flags[16]",
        0x8,
        doc="Per-level puppy/bone completion bitfields updated by SaveGame_SaveLevelCompletion.",
    ),
    member(
        "uint8_t",
        "level_bonus_item_flags[16]",
        0x18,
        doc="Per-level dalmatian/bonus bitfields updated by SaveGame_SaveLevelCompletion.",
    ),
    member(
        "uint8_t",
        "level_best_scores[16]",
        0x28,
        doc="Per-level best completion scores; Level_CalculateCompletionPercent treats 100 as full level score.",
    ),
    member(
        "uint16_t",
        "bonus_level_data[5]",
        0x38,
        doc="Packed bonus-level parameters read by Level_InitializeBonusData.",
    ),
    member(
        "uint16_t",
        "level_best_time",
        0x42,
        doc="Best time/value for the TOB bonus level, written from menu_items by SaveGame_SaveLevelCompletion.",
    ),
    member(
        "uint8_t",
        "bonus_name_entry_buffer[24]",
        0x44,
        doc="Name-entry/bonus scratch data copied as part of the 0x5c-byte save slot payload.",
    ),
    size=92,
)


stable.struct(
    "Scene_NodeEntry",
    member("uint32_t", "material_ptr", 0x0),
    member("uint32_t", "type_and_flags", 0x4),
    member("uint32_t", "data_offset", 0x8),
    member("uint32_t", "child_table_ptr", 0xC),
    size=16,
)

stable.struct(
    "Scene_Header",
    member("int32_t", "node_count", 0x0),
    member("Scene_Node*", "root_node_ptr", 0x4),
    member("Scene_Node**", "scene_node_list_ptr", 0x8),
    size=12,
)

stable.struct(
    "Scene_LocalTransform",
    member("Math_Vec3i16", "pos", 0x0),
    member("Math_Vec3i16", "rot", 0x6),
    member("uint8_t", "flags", 0xC),
    member("uint8_t", "padding_0d[1]", 0xD),
    size=14,
)

stable.struct(
    "Camera_Runtime",
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
    member("Camera_FrustumPlane", "frustum_planes[3]", 0x44),
    member("int32_t", "frustum_plane_3_x", 0x68),
    size=108,
)

stable.struct(
    "Scene_Node",
    member(
        "Scene_Node*",
        "next_in_resource_list",
        0x0,
        doc="Resource-side list link used to chain Scene_Node records during load/fixup.",
    ),
    member(
        "int32_t",
        "node_list_skip_count",
        0x4,
        doc=(
            "Scene_RenderNodeTree treats this as a node-list cursor skip count when traversal "
            "flags hide a subtree."
        ),
    ),
    member(
        "Scene_Node*",
        "transform_parent_node",
        0x8,
        doc=(
            "Parent transform node used by the renderer for traversal flags and parent world "
            "matrix/position fields. Runtime render fixes cannot treat it as a child-list head."
        ),
    ),
    member(
        "Math_Matrix3x3i16",
        "local_rot_matrix",
        0xC,
        doc="Local rotation matrix read by Scene_UpdateNodeAnimation/node-transform update paths.",
    ),
    member("int16_t", "local_rot_reserved", 0x1E),
    member("Math_Vec3i32", "local_pos", 0x20),
    member("Math_Matrix3x3i16", "world_rot_matrix", 0x2C),
    member(
        "int16_t",
        "world_rot_reserved",
        0x3E,
        doc=(
            "Alignment/reserved word after world_rot_matrix; Scene_UpdateNodeAnimation writes the "
            "surrounding matrix fields, but no direct semantic access to +0x3E (PC EN) has been isolated."
        ),
    ),
    member("Math_Vec3i32", "world_pos", 0x40),
    member(
        "Math_Vec3i32",
        "world_delta_pos",
        0x4C,
        doc="Per-update world position delta computed by Scene_UpdateNodeAnimation.",
    ),
    member(
        "int8_t",
        "position_anim_channel_index_or_minus_one",
        0x58,
        doc="Signed animation channel index read by Scene_UpdateNodeAnimation; negative values skip sampling.",
    ),
    member(
        "int8_t",
        "rotation_anim_channel_index_or_minus_one",
        0x59,
        doc="Signed animation channel index read by Scene_UpdateNodeAnimation; negative values skip sampling.",
    ),
    member(
        "int8_t",
        "visibility_anim_channel_index_or_minus_one",
        0x5A,
        doc="Signed animation channel index read by visibility/render traversal; negative values skip sampling.",
    ),
    member(
        "int8_t",
        "scale_anim_channel_index_or_minus_one",
        0x5B,
        doc="Signed animation channel index read by Scene_UpdateNodeAnimation; negative values skip sampling.",
    ),
    member(
        "uint16_t",
        "anim_update_sentinel",
        0x5C,
        doc="Scene_UpdateNodeAnimation skips animation update when this word is 0xFFFF.",
    ),
    member("uint16_t", "padding_5e", 0x5E),
    member(
        "int32_t",
        "cached_anim_tick",
        0x60,
        doc="Compared/stored against actor animation tick by Scene_UpdateNodeAnimation.",
    ),
    member(
        "uint8_t",
        "node_type",
        0x64,
        doc="Node type byte read by Scene_UpdateNodeAnimation.",
    ),
    member(
        "uint8_t",
        "traversal_flags",
        0x65,
        doc="Traversal/visibility flags byte read by Scene_TraverseNodeTree.",
    ),
    member(
        "int8_t",
        "render_entry_index_for_type_6",
        0x66,
        doc="Type-specific signed render/model-entry index; node_type 6 render paths index actor+0x94 (PC EN) with this byte.",
    ),
    member(
        "int8_t",
        "type_specific_secondary_render_or_sound_index",
        0x67,
        doc="Type-specific secondary byte. For node_type 0x0b sound nodes this is a level-local sound index passed to Audio_PlayLevelSoundIndexAtPosition.",
    ),
    member(
        "int32_t",
        "render_param_1",
        0x68,
        doc=(
            "Type-specific render/collision parameter. For collision-style nodes, the low/high "
            "words can act as vertex and polygon counts."
        ),
    ),
    member(
        "void*",
        "variant_payload_ptr",
        0x6C,
        doc=(
            "Type-specific pointer: model/mesh sprite table, type-8 simple-node payload, "
            "or collision/scene polygon array; rebased by Resource_FixUp*Node paths."
        ),
    ),
    member(
        "Collision_Vertex*",
        "collision_vertices",
        0x70,
        doc="Collision/scene vertex array with signed int16 x/y/z at offsets +0/+2/+4 and 12-byte stride.",
    ),
    member("int32_t", "actor_world_pos_z", 0x74),
    member("uint8_t", "visibility_flags[6]", 0x78),
    member(
        "uint8_t",
        "reserved_7e[2]",
        0x7E,
        doc="Reserved model-node tail bytes before rebased model pointers.",
    ),
    member(
        "void*",
        "model_relocated_ptr_80",
        0x80,
        doc=("Model-node pointer rebased by Resource_FixUpModelNode at +0x80 (PC EN)."),
    ),
    member(
        "void*",
        "model_relocated_ptr_84",
        0x84,
        doc=("Model-node pointer rebased by Resource_FixUpModelNode at +0x84 (PC EN)."),
    ),
    member(
        "uint32_t",
        "model_runtime_flags",
        0x88,
        doc=(
            "Flags read by traversal/render/fixup paths; Resource_FixUpModelNode tests bit 1 at +0x88 (PC "
            "EN), and collision polygon tests use transformed coordinates when bits 0x22 are set."
        ),
    ),
    member(
        "uint32_t",
        "variant_transform_tail",
        0x8C,
        doc="Opaque tail dword before trail_effects_ptr.",
    ),
    member("Trail_BoneEffect*", "trail_effects_ptr", 0x90),
    member("Submesh_Entry*", "submesh_entry_table", 0x94),
    member(
        "uint8_t",
        "padding_98[4]",
        0x98,
        doc="Opaque tail gap used by render/finalize/bone paths.",
    ),
    member("int32_t", "sort_keys[2]", 0x9C),
    member("Scene_Node*", "child_node_list_ptr", 0xA4),
    member(
        "uint8_t",
        "reserved_a8[4]",
        0xA8,
        doc="Reserved model-node tail gap before the rebased animation-data pointer.",
    ),
    member(
        "void*",
        "model_animation_data_ptr",
        0xAC,
        doc="Model animation-data pointer rebased by Resource_FixUpModelNode and read by Scene_TraverseNodeTree for visibility animation data.",
    ),
    member(
        "void*",
        "offset_fixup_list_ptr",
        0xB0,
        doc="Rebased model-node offset/fixup list pointer walked by Resource_FixUpModelNode.",
    ),
    member(
        "int32_t",
        "child_rot_x",
        0xB4,
        doc="Name retained as inferred; current audit did not find a direct stable accessor for this variant-tail field.",
    ),
    member(
        "uint16_t",
        "padding_b8",
        0xB8,
        doc="Opaque child-transform gap; no stable read/write access has been observed.",
    ),
    member(
        "uint16_t",
        "mesh_node_count",
        0xBA,
        doc="Count for mesh_node_table read by Resource_FixUpModelNode/Resource_FixUpMeshNode.",
    ),
    member(
        "Mesh_RenderNodeEntry*",
        "mesh_node_table",
        0xBC,
        doc=(
            "32-byte mesh render-node entry table; count is read from mesh_node_count at +0xBA (PC EN) "
            "and entries are fixed by Resource_FixUpMeshNode before Scene_RenderSubMesh indexes them."
        ),
    ),
    member(
        "int32_t",
        "child_scale_x",
        0xC0,
        doc="Name retained as inferred; current audit did not find a direct stable accessor for this variant-tail field.",
    ),
    member(
        "uint8_t",
        "reserved_c4",
        0xC4,
        doc="Reserved byte before model-node type-1 dispatch metadata.",
    ),
    member(
        "int8_t",
        "type_1_dispatch_index",
        0xC5,
        doc="Signed dispatch index used by Scene_TraverseNodeTree and tested by Resource_FixUpModelNode before rebasing LOD/config data.",
    ),
    member(
        "int8_t",
        "type_1_attach_point_index",
        0xC6,
        doc="For Scene_UpdateNodeAnimation node type 1, indexes actor attach_point_table entries.",
    ),
    member(
        "int8_t",
        "type_1_bone_channel_index",
        0xC7,
        doc="For Scene_UpdateNodeAnimation node type 1, indexes the type-1 bone/animation channel table.",
    ),
    member(
        "int32_t",
        "child_scale_z",
        0xC8,
        doc="Name retained as inferred; current audit did not find a direct stable accessor for this variant-tail field.",
    ),
    member(
        "int32_t",
        "render_offset_x",
        0xCC,
        doc="Model render offset added to world_pos_x by Scene_TraverseNodeTree to produce render-space position.",
    ),
    member(
        "int32_t",
        "render_offset_y",
        0xD0,
        doc="Model render offset added to world_pos_y by Scene_TraverseNodeTree to produce render-space position.",
    ),
    member(
        "int32_t",
        "render_offset_z",
        0xD4,
        doc=(
            "Model render offset added to world_pos_z by Scene_TraverseNodeTree to produce render-space "
            "position; has no confirmed is_active meaning."
        ),
    ),
    member(
        "Scene_Node*",
        "lod_config_ptr",
        0xD8,
        doc="Name retained as inferred; current audit did not find a direct stable accessor for this variant-tail pointer.",
    ),
    size=220,
)

stable.struct("Scene_NodeType", member("uint32_t", "type", 0x0), size=4)


stable.struct(
    "Script_Context",
    member("uint8_t*", "script_data", 0x0),
    member("uint8_t*", "script_pc", 0x4),
    member("int32_t*", "stack_ptr", 0x8),
    member("int32_t*", "var_base", 0xC),
    member("uint32_t", "flags", 0x10),
    size=20,
)

stable.struct(
    "Script_OpcodeTable",
    member(
        "Script_CommandCallback",
        "handlers[45]",
        0x0,
        doc=(
            "45-entry script opcode handler array. Entry 0 is null in the "
            "table; non-null entries use the Script_CommandCallback ABI."
        ),
    ),
    size=180,
    doc=(
        "Script opcode dispatch table reached from ScriptCmd_WithActor at pcdogs.exe+0x51988 (PC EN). "
        "Table shape is stable; individual handler slots dispatch native script opcodes."
    ),
)


stable.struct(
    "Trail_BoneEffect",
    member("uint32_t", "spawn_frame", 0x0),
    member("int32_t", "waypoint_index", 0x4),
    member("Math_Vec3i32", "pos", 0x8),
    member("int32_t", "offset_x", 0x14),
    member("int32_t", "offset_z", 0x18),
    member("uint16_t", "rotation", 0x1C),
    member("uint16_t", "scale", 0x1E),
    size=32,
)

stable.struct(
    "Trail_Entry",
    member("uint16_t", "spawn_interval", 0x0),
    member("uint16_t", "data_type", 0x2),
    member("void*", "data_ptr", 0x4),
    size=8,
)

stable.struct(
    "Trail_Segment",
    member("int32_t", "active", 0x0),
    member("uint8_t", "world_transform[32]", 0x4),
    member("Math_Vec3i16", "start", 0x24),
    member(
        "int16_t",
        "start_coord_hi",
        0x2A,
        doc="High half of the packed start coordinate word written by Trail_UpdateEffect; not observed as independently read by Trail_RenderAnimated.",
    ),
    member("Math_Vec3i16", "end", 0x2C),
    member(
        "int16_t",
        "end_coord_hi",
        0x32,
        doc="High half of the packed end coordinate word written by Trail_UpdateEffect; not observed as independently read by Trail_RenderAnimated.",
    ),
    size=52,
)

stable.struct(
    "UI_SpotEntry",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "timer", 0x8),
    size=12,
)

stable.struct(
    "Menu_LevelProgressInfo",
    member("int16_t", "level_puppy_count", 0x0),
    member("int16_t", "level_bone_count", 0x2),
    member("int16_t", "player_bone_count", 0x4),
    member("int16_t", "player_lives", 0x6),
    size=8,
)

stable.struct(
    "UI_LivesIconState",
    member("uint8_t", "target_visible", 0x0),
    member("uint8_t", "animating", 0x1),
    member("uint8_t", "visible", 0x2),
    member("uint8_t", "animation_frame", 0x3),
    size=4,
)

stable.struct("UI_StringTableEntry", member("uint32_t", "offset", 0x0), size=4)


stable.type_alias("Win32_GUID", "GUID")

stable.callback_type(
    "DDraw_EnumCallbackExA",
    ret="BOOL",
    params=[
        param("Win32_GUID*", "guid"),
        param("char*", "description"),
        param("char*", "name"),
        param("void*", "context"),
        param("HMONITOR", "monitor"),
    ],
    calling="CALLBACK",
)

stable.callback_type(
    "D3D_DriverAcceptCallback",
    ret="int32_t",
    params=[param("D3D_DriverInfo*", "driver_info")],
)

stable.callback_type(
    "Actor_BehaviorCallback",
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    calling=CallingConvention.CDECL,
    doc="Runtime behavior/movement callback slot signature used by Render_InitDispatchTables for behavior_target_actor and behavior_param0..2.",
)

stable.callback_type(
    "Actor_DefaultUpdateCallback",
    ret="int32_t",
    params=[],
    calling=CallingConvention.CDECL,
    doc="Default actor update callback signature, with no arguments.",
)

stable.callback_type(
    "Script_CommandCallback",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t**", "ip")],
    calling=CallingConvention.CDECL,
    doc=(
        "ScriptCmd_WithActor uses this callback type for opcode handlers in the native script dispatch table."
    ),
)


stable.callback_type(
    "Sort_CompareCallback",
    ret="int32_t",
    params=[param("void const*", "lhs"), param("void const*", "rhs")],
    calling=CallingConvention.CDECL,
    doc="CRT qsort/shortsort comparator ABI; returns negative, zero, or positive for lhs vs rhs ordering.",
)

stable.callback_type(
    "TreeMap_CompareCallback",
    ret="int32_t",
    params=[param("void*", "lhs_payload"), param("void*", "rhs_payload")],
    calling=CallingConvention.CDECL,
    doc="TreeMap payload comparator stored in the 12-byte tree header and called with two node payload pointers.",
)

stable.callback_type(
    "Powerup_UpdateCallback",
    ret="uint8_t",
    params=[param("Actor_State*", "actor")],
    calling=CallingConvention.CDECL,
    doc="Powerup actor update callback slot initialized by Powerup_InitializeSystem.",
)

stable.callback_type(
    "Powerup_CollisionCallback",
    ret="int32_t",
    params=[
        param("Actor_State*", "powerup_actor"),
        param("Actor_State*", "other_actor"),
        param(
            "int32_t",
            "reserved_zero",
            doc="Reserved argument slot observed as zero at the powerup collision dispatch site.",
        ),
        param("int32_t", "collision_result"),
    ],
    calling=CallingConvention.CDECL,
    doc=(
        "Four-argument powerup collision filter callback slot initialized by Powerup_InitializeSystem; "
        "Powerup_CollisionFilter consumes the fourth collision_result argument and ignores the observed-zero third slot."
    ),
)

stable.callback_type(
    "Collision_ActorResponseCallback",
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    calling=CallingConvention.CDECL,
    doc="Actor-vs-actor collision response callback slot initialized to Actor_ProcessCollisionResponse.",
)

stable.callback_type(
    "Collision_ComponentResponseCallback",
    ret="int32_t",
    params=[
        param("Component_Instance*", "component"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    calling=CallingConvention.CDECL,
    doc="Component/projectile collision response callback slot initialized to Collision_ProcessProjectileHit.",
)

stable.callback_type(
    "Collision_ProcessCallback",
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "collision_depth"),
    ],
    calling=CallingConvention.CDECL,
    doc="Engine-owned scalar actor collision processing callback slot initialized to Physics_ProcessActorCollision and aliased by collision_state_handler_table slot 2.",
)


stable.enum(
    "Actor_PropId",
    enum_value("PROP_RENDER_Y", 0),
    enum_value("PROP_SPEED", 1),
    enum_value("PROP_ROTATION", 2),
    enum_value("PROP_PARENT", 3),
    enum_value("PROP_PUSH_XZ", 4),
    enum_value("PROP_RESPONSE_XZ", 5),
    enum_value("PROP_LIVE_SPEED", 6),
    enum_value("PROP_CAMERA_XZ", 7),
    enum_value("PROP_COLL_RADIUS", 8),
    enum_value("PROP_COLL_HEIGHT", 9),
    enum_value("PROP_TRACE_MODE_0", 100),
    enum_value("PROP_TRACE_MODE_1", 101),
    enum_value("PROP_TRACE_MODE_2", 102),
    enum_value("PROP_TRACE_MODE_3", 103),
)

stable.enum("Camera_TransitionMode", enum_value("CAMERA_SNAP", 0))

stable.sig("ddraw_object_anchor", "A1 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81 EC 9C 00 00 00")

stable.sig("game_initialized_anchor", "89 35 ?? ?? ?? ?? C6 44 24 ?? 10")

stable.sig(
    "gamepad_button_flags", "8B 15 ?? ?? ?? ?? 8B 06 0B C2 89 06 81 FB BC 02 00 00"
)

stable.sig("joystick_available_anchor", "A0 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? A1")

stable.sig("keyboard_mappinbuttons", "8B 15 ?? ?? ?? ?? 50 56")

stable.sig("keyboard_mappinkeys", "A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 8D 14 8D")

stable.sig("main_window_handle_anchor", "A1 ?? ?? ?? ?? 83 C4 08 6A 03")

stable.sig("main_window_handle2_anchor", "A3 ?? ?? ?? ?? FF D7")

stable.sig("mappincount", "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 50")

stable.sig("rendering_enabled_anchor", "39 35 ?? ?? ?? ?? 74 ?? E8")

stable.sig("should_quit_anchor", "39 35 ?? ?? ?? ?? 75 ?? 39 35")

stable.sig("directory_anchor", "68 ?? ?? ?? ?? 68 04 01 00 00 FF 15")

stable.sig("audio_digital_driver_anchor", "A1 ?? ?? ?? ?? 6A 7F 50 FF 15")

stable.sig("movie_file_names_and_path_prefix", "8B 04 B5 ?? ?? ?? ?? 50 68")

stable.sig(
    "title_resource_cleanup_bundle",
    "6A 01 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ??",
)

stable.sig("find_and_load_game_pkfile", "81 EC 10 01 00 00 57 ??")

stable.sig("initialize_game_engine", "E8 ?? ?? ?? ?? 85 C0 75 ?? 32 C0")

stable.sig("initialize_graphics_subsystem", "E8 ?? ?? ?? ?? 8B 44 24 ?? 8B 4C 24 ?? 50")

stable.sig("initialize_capabilities", "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 A3")

stable.sig("initialize_window_handle", "A1 ?? ?? ?? ?? 56 57 8B 7C 24")

stable.sig("initialize_game_systems", "E8 ?? ?? ?? ?? 8D 54 24 ?? 56")

stable.sig("sig_render_frame", "51 53 E8 ?? ?? ?? ?? A1")

stable.sig("is_key_pressed", "A1 ?? ?? ?? ?? 33 C9 85 C0 53")

stable.sig("reset_input_and_state", "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 25 DF F4 FF FF")

stable.sig("take_screenshot", "81 EC 04 01 00 00 56 ??")

stable.sig("malloc", "FF 35 ?? ?? ?? ?? FF 74 24")

stable.sig("sig_audio_shutdown_system", "A1 ?? ?? ?? ?? 85 C0 74 ?? 53 8B 1D")

stable.sig(
    "sig_camera_check_actor_distance",
    "8B 44 24 04 85 C0 75 ?? 33 C0 C3 8B 15 ??",
    required=Required.EN,
)

stable.sig(
    "sig_scene_render_frame",
    "55 8B EC 83 EC 44 F6 05 ?? ?? ?? ?? 08 0F 85 ??",
    required=Required.EN,
)

stable.sig(
    "sig_timer_get_raw_tick_count",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 55",
    required=Required.EN,
)

stable.sig(
    "sig_audio_play_positional_sound", "8B 0D ?? ?? ?? ?? 56 85", required=Required.EN
)

stable.sig("sig_actor_set_property", "00 00 83 F9 09 0F 87 ??", required=Required.EN)

stable.sig(
    "sig_render_adjust_level_scale", "A1 ?? ?? ?? ?? 85 C0 7C", required=Required.EN
)

stable.fn(
    "Camera_CheckActorDistance",
    "8B 44 24 04 85 C0 75 ?? 33 C0 C3 8B 15 ??",
    required=Required.EN,
    hook=6,
    ret="BOOL",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Returns TRUE when actor is non-null and its maximum saved-camera axis "
        "distance passes the current game-mode filter. Mode 0 rejects distance "
        "0x8381; mode 0x11 rejects distances in the open range 0x30d40..0x493e0."
    ),
)

stable.fn(
    "Scene_InitNodeState",
    "56 65 74 ?? 48 0F 84 ??",
    match=-18,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Scene_TraverseNodeTree",
    "7C ?? 1C 85 F6 0F 84 ??",
    match=-24,
    hook=8,
    ret="Scene_Node*",
    params=[
        param("Scene_Node*", "node"),
        param("Actor_State*", "actor"),
        param("Scene_Node*", "parent_node"),
    ],
    doc=(
        "Traverses the scene-node tree for rendering/visibility side effects, dispatching node_type "
        "1..7 through scene_node_type_dispatch_table. Type 1 uses Scene_Node+0xC5 (PC EN) for "
        "model-like variants, while +0x8 (PC EN) and +0x4 (PC EN) act as traversal links."
    ),
)

stable.fn(
    "Scene_RenderBillboard",
    "8B 44 24 04 8D 50 6C 8D 48 2C 52 50 89 0D ??",
    hook=7,
    ret="int32_t",
    params=[param("Scene_Node*", "node")],
)

stable.fn(
    "Trail_ProcessComponents",
    "00 6A 00 6A 00 56 E8 ??",
    match=-41,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Processes the actor trail chain at actor+0xBC (PC EN) for actor+0xBA (PC EN) entries, "
        "refreshing pending Component_TrailObject records and clearing each processed_flag byte."
    ),
)

stable.fn(
    "Trail_UpdateEffect",
    "0B 56 F6 C1 02 0F 85 ??",
    match=-34,
    hook=8,
    ret="char",
    params=[
        param("Component_TrailObject*", "trail"),
        param(
            "Actor_State*",
            "actor",
            doc=(
                "Actor supplying mesh vertex data at actor+0x70 (PC EN) when a live segment is written; null "
                "is only valid for inactive/expiry updates."
            ),
        ),
        param(
            "void*",
            "component_record",
            doc="Component/node record whose 16-bit fields at +0x4 (PC EN) and +0x0A (PC EN) select the start/end mesh vertices.",
        ),
        param("char", "active"),
    ],
    doc="Advances the Component_TrailObject ring segment, writes the active flag, copies the cached transform, and for live segments stores start/end mesh vertex positions. Returns 0 when an inactive segment expires, otherwise 1.",
)

stable.fn(
    "Scene_RenderSubMesh",
    "8B 5D 10 83 C0 2C A3 ??",
    match=-10,
    hook=6,
    ret="int16_t*",
    params=[
        param("Scene_Node*", "node"),
        param("int16_t*", "matrix"),
        param("Render_WorkArea*", "render_ctx"),
    ],
)

stable.fn(
    "Scene_RenderFrame",
    "55 8B EC 83 EC 44 F6 05 ?? ?? ?? ?? 08 0F 85 ??",
    required=Required.EN,
    hook=6,
    ret="void",
    params=[],
)

stable.fn(
    "Scene_RenderStaticGeometry",
    "A1 ?? ?? ?? ?? 56 57 C7",
    match=-35,
    hook=6,
    ret="int32_t",
    params=[],
    doc="Renders static scene geometry for side effects and returns the native status value.",
)

stable.fn(
    "Render_SpriteObjectNode",
    "50 8D 46 40 51 50 E8 ??",
    match=-24,
    ret="int32_t",
    params=[param("Scene_Node*", "node")],
)

stable.fn(
    "Actor_UpdateList", "56 57 E8 ?? ?? ?? ?? BF", hook=7, ret="Scene_Node*", params=[]
)

stable.fn(
    "Actor_ProcessRendering",
    "53 56 8B 74 24 0C 57 56 89 35 ??",
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "During actor rendering, sets player_actor, updates animation/visibility, renders "
        "the actor scene-node tree, processes trail/mesh command flags, then clears the "
        "render-scoped globals before return. player_actor is render-scoped state."
    ),
)

stable.fn(
    "Scene_RenderNodeTree",
    "55 8B EC 81 EC 90 00 00 00 A1 ??",
    hook=9,
    ret="void",
    params=[],
    doc="Renders the active scene node tree and runs render/finalizer side effects.",
)

stable.fn(
    "Actor_UpdateAnimationAndVisibility",
    "F6 D3 80 E3 01 0F 84 ??",
    match=-20,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_UpdateActivePowerups",
    "56 8B 35 ?? ?? ?? ?? 57 BF ?? ?? ?? ?? 85 F6 74",
    required=Required.EN_EU,
    hook=7,
    ret="void",
    params=[],
)

stable.fn(
    "Actor_UpdateProjectileList",
    "56 8B 35 ?? ?? ?? ?? 57 BF ?? ?? ?? ?? 85 F6 0F",
    required=Required.EN_EU,
    hook=7,
    ret="void",
    params=[],
    doc=(
        "Walks the live projectile_actor_list_head runtime actor list, updates projectile "
        "actors through physics/animation/render paths, and removes actors whose lifecycle "
        "state indicates destruction."
    ),
)

stable.fn(
    "Actor_GetStateIndex",
    "8B 44 24 04 85 C0 74 ?? 66 8B 40 0C 66 85 C0 7C ?? 8B 4C 24 08 0F BF C0 8D 44 08 9C C3 83 C8 FF C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t", "state_base")],
)

stable.fn(
    "Navigation_AddCommand",
    "8B 0D ?? ?? ?? ?? 8B 54 24 04 81",
    hook=6,
    ret="void*",
    params=[
        param("char", "command_type"),
        param("int16_t", "target_x"),
        param("int16_t", "target_y"),
        param("int32_t", "speed"),
        param("int32_t*", "position"),
    ],
)

stable.fn(
    "Navigation_HandleDamageResponse",
    "53 8B 5C 24 0C 55 56 57 53 E8 ??",
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Component_Instance*", "damage_component"),
    ],
)

stable.fn(
    "Audio_PlayLevelSoundIndexAtPosition",
    "8B 0D ?? ?? ?? ?? 56 85",
    hook=6,
    ret="void",
    params=[
        param("int32_t", "level_sound_index"),
        param("Math_Vec3i32*", "position"),
    ],
    doc=(
        "Resolves a level-local sound index through current_level_data->sound_definition_list "
        "using 20-byte Audio_SoundDefinition stride, then forwards to "
        "Audio_PlaySoundDefinition3D."
    ),
)

stable.fn(
    "Effect_TriggerSoundAtPosition",
    "A1 ?? ?? ?? ?? 53 55 56 85",
    ret="int32_t",
    params=[
        param("int32_t", "sound_type_index"),
        param("Math_Vec3i32*", "position"),
        param("char", "stop_only_if_playing"),
    ],
)

stable.fn(
    "Script_CheckCollisionBit",
    "83 C1 02 89 08 8B 35 ??",
    match=-19,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
)

stable.fn(
    "Script_PollSignal",
    "66 89 54 24 10 50 E8 ??",
    match=-90,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
)

stable.fn(
    "Input_CheckButtonState",
    "FA 00 00 00 20 0F 84 ??",
    match=-39,
    hook=8,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "state_index",
            doc="Input state slot to query; invalid or empty slots fall back to slot zero.",
        ),
        param(
            "Input_State*",
            "state_table",
            doc="Two-entry input state table used by scripts and menus.",
        ),
        param(
            "int32_t",
            "control_code",
            doc="Button bit index or extended axis/control selector.",
        ),
        param(
            "int32_t",
            "mode_or_threshold",
            doc="Button expected-state flag or axis threshold percentage, depending on control_code.",
        ),
    ],
    doc=(
        "Evaluates a button or axis control code against sampled input state. "
        "Button queries return 0/100; axis queries return scaled Q12 magnitude or threshold results."
    ),
)

stable.fn(
    "Actor_CalculateRotation",
    "FF FF 89 45 10 0F 84 ??",
    match=-77,
    hook=6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t", "target_actor_id"),
        param("int32_t", "angle_delta"),
    ],
)

stable.fn(
    "Actor_ResetVelocityAndSnap",
    "8B 4C 24 10 51 52 50 E8 ??",
    match=-61,
    hook=6,
    ret="int32_t*",
    params=[
        param(
            "int32_t*",
            "camera_script_state",
            doc="Camera/script move state: target actor is at dword index 74 and target position starts at index 82.",
        ),
        param(
            "int16_t",
            "target_angle",
            doc="Angle forwarded to Camera_MoveToTarget for the selected target actor.",
        ),
        param(
            "int32_t",
            "transition_speed",
            doc="Transition speed/control value forwarded to Camera_MoveToTarget.",
        ),
        param(
            "int32_t",
            "duration_q12",
            doc="Script duration value scaled by 30 and shifted from Q12 before the move call.",
        ),
    ],
    doc=(
        "Reads a target actor from camera_script_state[74], clears its velocity slots, "
        "then moves it toward camera_script_state[82..84] through Camera_MoveToTarget."
    ),
)

stable.fn(
    "Camera_MoveToTarget",
    "00 84 C0 7E ?? 56 E8 ??",
    match=-23,
    hook=6,
    ret="int32_t*",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t*", "target_pos"),
        param("int16_t", "target_angle"),
        param("int32_t", "transition_speed"),
        param("int32_t", "duration"),
    ],
)

stable.fn(
    "Actor_ReleaseAttachment",
    "00 00 8B 48 24 51 E8 ??",
    match=-9,
    ret="int32_t",
    params=[param("Pkg_ActorRecord*", "record")],
)

stable.fn(
    "Actor_SnapToPosition",
    "01 00 00 3B D0 0F 84 ??",
    match=-54,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_SetEntityProperties",
    "8B 44 24 04 56 85 C0 0F 84 ??",
    ret="void*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_StartTransition",
    "C6 00 00 00 89 88 E8 ??",
    match=-60,
    hook=10,
    ret="int32_t*",
    params=[param("Actor_State*", "actor"), param("Pkg_ActorRecord*", "record")],
)

stable.fn(
    "Script_SetPlayerState",
    "00 88 4F 0C 7D ?? E8 ??",
    match=-90,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
)

stable.fn(
    "Actor_TracePath",
    "D2 3B F2 57 75 ?? BE ??",
    match=-12,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose path-trace fields are read and updated.",
        ),
        param(
            "int32_t",
            "target_selector",
            doc="Signed script/actor selector; positive values address actor slots and negative sentinel values select special path targets.",
        ),
        param(
            "int32_t",
            "max_path_distance",
            doc="Primary distance cap used when accepting or rejecting a candidate path target.",
        ),
        param(
            "int32_t",
            "best_path_distance",
            doc="Current best/pruning distance carried through recursive trace attempts.",
        ),
        param(
            "int32_t",
            "trace_flags",
            doc="Bitfield controlling recursive trace/result modes; observed values include 0, 0x20, and 0x300.",
        ),
    ],
    doc=(
        "Traces/selects an actor path target and mutates actor path fields around +0x144 (PC "
        "EN)..+0x170 (PC EN). Returns the selected path/actor index, or 0 when no valid path is "
        "found."
    ),
)

stable.fn(
    "Entity_IsInActiveList",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 48",
    hook=10,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "entity_index_one_based",
            doc="1-based entity index checked against the current level entity table.",
        ),
    ],
    doc=(
        "Returns 1 when entity_index_one_based is present in the active entity work list. "
        "The function converts the 1-based index to an entity-slot pointer and compares it "
        "against the active-entity pointer list populated by Camera_UpdateFollow."
    ),
)

stable.fn(
    "Entity_CopyDataToActor",
    "68 01 00 00 20 0F 85 ??",
    match=-13,
    ret="Pkg_ActorRecord*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_SetProperty",
    "00 00 83 F9 09 0F 87 ??",
    match=-51,
    ret="void",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Entity-slot input; the routine bridges through the active actor/record link before writing actor property/default fields.",
        ),
        param("Actor_PropId", "prop_id"),
        param("int32_t", "value"),
    ],
    doc="Applies a property update through entity/active-actor state. Observed callers ignore the residual register value; no stable pointer return is modeled.",
)

stable.fn(
    "Timer_GetRawTickCount",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 55",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Entity_SpawnActor",
    "8D 73 58 85 C0 0F 85 ??",
    match=-15,
    hook=6,
    ret="Actor_State*",
    params=[param("Entity_State*", "source_entity")],
)

stable.fn(
    "Shadow_CheckRequirement",
    "A4 00 00 00 7D ?? 8B ??",
    match=-67,
    hook=10,
    ret="void*",
    params=[param("Actor_State*", "actor")],
    doc="Returns actor->linked_actor and, when that linked actor owns a parent component plus a valid child actor, sets behavior_flags bit 0x40 if the child shadow height/scale falls below the linked-actor threshold fields.",
)

stable.fn(
    "Entity_DestroyActor",
    "00 00 53 50 6A FE E8 ??",
    match=-23,
    hook=6,
    ret="uint32_t",
    params=[
        param("Actor_State*", "actor", doc="Actor slot to detach and clear."),
        param(
            "uint32_t",
            "restore_defaults",
            doc="Nonzero restores default collision radius/height and default flags with bit 0x800 set after teardown.",
        ),
    ],
    doc=(
        "Tears down an actor's entity-slot state. It detaches or marks any linked actor, clears "
        "attachment and script entity-slot fields, resets component_array to actor-local storage, and "
        "optionally restores default collision state when restore_defaults is nonzero."
    ),
)

stable.fn(
    "Render_UpdateScreenFade",
    "55 8B EC 83 EC 10 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Level_TriggerTransition",
    "8B 44 24 08 8B 0D ?? ?? ?? ?? 53",
    hook=10,
    ret="int32_t",
    params=[
        param("int32_t", "target_level_index"),
        param("uint32_t", "transition_flags"),
        param("int32_t*", "transition_data"),
    ],
)

stable.fn(
    "Audio_TriggerMusicTransition",
    "55 8B EC 8B 45 0C 53 85 C0 56 7D ?? 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[
        param("int32_t", "track_index"),
        param("int32_t", "fade_speed"),
        param("int32_t*", "track_data"),
        param("int32_t", "loop_flag"),
    ],
)

stable.fn(
    "Script_PauseToggle",
    "A1 ?? ?? ?? ?? 53 33 DB 55",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Entity_UpdateVisibilityAndSpawn",
    "40 01 00 00 51 50 E8 ??",
    match=-75,
    hook=6,
    ret="int32_t",
    params=[
        param("Entity_State*", "source_entity"),
        param("Actor_State*", "actor_list"),
    ],
    doc=(
        "Updates level-local Entity_State visibility/spawn state, active actor ownership, and active "
        "entity work-list membership; native code at pcdogs.exe+0x06D50 (PC EN) reads "
        "current_level_data entity slots and Entity_State.active_actor."
    ),
)

stable.fn(
    "Camera_UpdateFollow",
    "55 8B EC 83 EC 64 53 56 57 E8 ??",
    hook=6,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera")],
    doc=(
        "Camera follow/update path that swaps active entity/navigation work-list buffers and queues "
        "entity slots for visibility/update work; pcdogs.exe+0x55D30 (PC EN), pcdogs.exe+0x55D3C (PC "
        "EN), pcdogs.exe+0x55ED8 (PC EN), and pcdogs.exe+0x56850 (PC EN) are engine-owned pointer cells."
    ),
)

stable.fn(
    "Camera_InterpolateTransition",
    "55 8B EC 83 EC 10 A1 ??",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Camera_TransitionState*",
            "current_camera",
            doc="Live camera state whose embedded pose at +0x04 (PC EN) is eased toward target_pose.",
        ),
        param(
            "Camera_Pose*",
            "target_pose",
            doc="36-byte target/scratch pose built by Camera_UpdateFollow or Camera_UpdateFromDefinition.",
        ),
        param(
            "int32_t",
            "duration_frames",
            doc="Total transition frame count; observed callers pass 30, a remaining scripted span, or camera definition duration.",
        ),
        param(
            "int32_t",
            "recompute_eye_from_angles",
            doc="Nonzero interpolates the first two angle words and recomputes eye position from target distance.",
        ),
    ],
    doc=(
        "Eases an active camera transition from current_camera->pose toward target_pose using "
        "camera_transition_frame_counter as the remaining countdown. It interpolates fov, "
        "target position, and orbit yaw with a Q12 ease curve; when requested it also "
        "wrap-interpolates the first two angle words and calls Camera_CalculatePosition."
    ),
)

stable.fn(
    "Camera_CalculatePosition",
    "55 8B EC 56 8B 75 08 66 8B 06 50 E8 ??",
    hook=7,
    ret="int32_t",
    params=[
        param(
            "Camera_Pose*",
            "camera_pose",
            doc="Pose whose eye/source components are recomputed from its target components and angles.",
        ),
        param(
            "int32_t",
            "target_distance",
            doc="Distance from target/look-at point used to recompute the eye/source position.",
        ),
    ],
    doc=(
        "Recomputes camera_pose eye/source position from target_pos fields, the first two "
        "wrapped angle words, and target_distance using Math_SinCos_FP12."
    ),
)

stable.fn(
    "Camera_UpdateFromDefinition",
    "55 8B EC 83 EC 20 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[
        param("Camera_Runtime*", "camera"),
        param("int32_t*", "position"),
        param("int16_t*", "angles"),
    ],
)

stable.fn(
    "Camera_CalculateFollowAngles",
    "55 8B EC 81 EC 78 01 00 00 A1 ??",
    hook=9,
    ret="int32_t",
    params=[
        param("Camera_Runtime*", "camera"),
        param(
            "Actor_State*",
            "target_actor",
            doc="Actor that the follow-camera code is targeting for this update.",
        ),
        param(
            "int16_t*",
            "angles",
            doc="Caller-owned angle output/input buffer used by follow-camera calculations.",
        ),
        param("int32_t", "flags"),
    ],
    doc="Calculates camera follow angles for targetActor and updates the packed camera_yaw_angle global used by movement.",
)

stable.fn(
    "Physics_CheckGroundFriction",
    "71 ?? C9 75 ?? 50 E8 ??",
    match=-13,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose record/ground-friction state is checked and updated.",
        )
    ],
    doc=(
        "Checks and updates the actor's ground-friction state. It reads the actor record at "
        "actor+0xF4 (PC EN), calls Physics_CalculateFrictionForce when the record+0x71 (PC EN) byte "
        "is clear, stores the friction result at record+0x10 (PC EN), and writes status 3 or 0 to "
        "record+0x0E (PC EN)."
    ),
)

stable.fn(
    "Trail_SpawnFromEntry",
    "8B 15 ?? ?? ?? ?? 83 EC 24",
    hook=6,
    ret="int32_t*",
    params=[param("Actor_State*", "actor"), param("int32_t", "trail_index")],
)

stable.fn(
    "Entity_GetActiveActorFromList",
    "8B 15 ?? ?? ?? ?? 85 D2 74 42 66 83 7A 0A 00 76 3B",
    hook=6,
    ret="Actor_State*",
    params=[],
    doc=(
        "Returns the active actor from the current level entity slots; pcdogs.exe+0x09560 (PC "
        "EN) checks current_level_data->current_entity_index and Entity_State.active_actor, then uses "
        "pcdogs.exe+0x55D30 (PC EN) as a count bound."
    ),
)

stable.fn(
    "Camera_UpdateShakeOffset",
    "55 8B EC 51 8B 4D 08 66 8B 81 D4 00 01 00",
    hook=6,
    ret="void",
    params=[param("Camera_Runtime*", "camera")],
    doc=(
        "Applies late camera shake/offset state after Camera_UpdateFollow by using the camera+0x100D4 "
        "(PC EN) countdown, +0x100D6 (PC EN) intensity, and shake lookup table before adjusting "
        "camera/source and target vector components."
    ),
)

stable.fn(
    "Physics_CheckGroundSlopeDirection",
    "D0 0C 83 F8 1C 0F 8E ??",
    match=-42,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("Math_Vec3i16*", "out_normal"),
        param("Math_Vec3i32*", "delta_vec"),
    ],
)

stable.fn(
    "Render_AdjustLevelScale",
    "A1 ?? ?? ?? ?? 85 C0 7C",
    ret="int32_t",
    params=[
        param(
            "float",
            "measured_fps",
            doc="Averaged frame rate measured by Render_Frame before adjusting the level/render scale.",
        )
    ],
    doc=(
        "Adjusts the global level/render scale from measured_fps during Render_Frame. For eligible "
        "game modes and measured_fps in the 10..30 range, it stores int(reciprocal_lookup_table * "
        "level_scale_factor * measured_fps) into the global level scale and render_list_state +0xB8 "
        "(PC EN); above 30 FPS it restores int(level_scale_factor)."
    ),
)

stable.fn(
    "Level_InitializeActorSystem",
    "51 53 33 DB 57 89 1D ??",
    ret="void",
    params=[],
    doc=(
        "Initializes actor/entity runtime lists, dispatch/collision/movement callback globals, "
        "and the active entity/navigation work-list backing buffers for the loaded level. "
        "The pcdogs.exe+0x09750 (PC EN) path seeds the navigation/work-list pointer cells and clears the backing-buffer span."
    ),
)

stable.fn(
    "Actor_ProcessSnapAndEntityUpdate",
    "00 00 00 75 ?? 51 E8 ??",
    match=-51,
    ret="int32_t*",
    params=[param("Actor_State*", "actor")],
    doc=("Snap/entity update callback used by movement/render dispatch paths."),
)

stable.fn(
    "Player_ProcessMovement",
    "55 8B EC 83 EC 2C 53 8B 5D 08 56 A1 ??",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc=(
                "Actor whose input-driven movement state is processed. Gameplay timing is cross-checked "
                "with render, behavior, camera, and collision signals."
            ),
        )
    ],
    doc=(
        "Processes camera-relative player movement for actor using actor->record_ptr at +0xF4 (PC "
        "EN), player_facing_angle - camera_yaw_angle movement, entity movement "
        "state, friction/velocity integration, and collision/trigger transitions."
    ),
)

stable.fn(
    "Actor_ProcessPlayerBehavior",
    "83 EC 1C 53 55 56 57 8B 7C 24 30 8B 0D ??",
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor being processed for player/AI behavior; state mutation depends on validating that this is the intended player.",
        )
    ],
)

stable.fn(
    "Actor_UpdateAnimationState",
    "83 EC 08 55 56 57 8B 7C 24 18 57 E8 ??",
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Pkg_ActorRecord*", "record"),
        param("int32_t", "update_from_input"),
    ],
)

stable.fn(
    "Player_RespawnAfterDeath",
    "53 56 8B 74 24 14 33 DB 66 89 1D ??",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Entity_State*",
            "current_entity",
            doc="Current entity whose respawn target and flags are used.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Player actor being reset and moved to the respawn target.",
        ),
        param(
            "Pkg_ActorRecord*",
            "record",
            doc="Player actor record containing backup-puppy and respawn state fields.",
        ),
    ],
    doc=(
        "Handles the player-death respawn transition. It decrements the record backup-puppy/life "
        "count at +0x172 (PC EN), saves it, and either enters pause/game-over state or clears "
        "loading/fade/death fields before reinitializing player placement and sound."
    ),
)

stable.fn(
    "Physics_ProcessActorCollision",
    "85 C9 89 75 F8 0F 8C ??",
    match=-21,
    hook=6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param(
            "int32_t",
            "collision_depth",
            doc="Signed contact/penetration scalar used by collision response.",
        ),
    ],
    doc=(
        "Resolves collision overlap between actor and otherActor. collisionDepth is a signed "
        "contact/penetration scalar: negative values short-circuit handling, while non-negative "
        "values are shifted down by 6 and compared against actor radius/overlap terms before "
        "displacement and collision response state are applied."
    ),
)

stable.fn(
    "Actor_ProcessCollisionResponse",
    "24 34 83 FF FE 0F 85 ??",
    match=-19,
    hook=8,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    doc=(
        "Actor-vs-actor collision response dispatcher. It receives a contextual "
        "Collision_Polygon from the collision query, handles sentinel collision_depth "
        "values, dispatches collision callback slots, and mutates actor response state."
    ),
)

stable.fn(
    "Actor_ProcessPuppyInteraction",
    "50 6A 67 89 7D F8 E8 ??",
    match=-56,
    hook=6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "puppy_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
)

stable.fn(
    "Level_CleanupActors",
    "A1 ?? ?? ?? ?? 57 85 C0 0F",
    ret="void",
    params=[],
    doc="Destroys/cleans level actor state and returns transition status.",
)

stable.fn(
    "Anim_InterpolateKeyframe_Vec3_Blend",
    "?? ?? 8B 44 24 2C 8B 74",
    match=-22,
    hook=8,
    ret="Math_Vec3i32*",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param(
            "int32_t",
            "blend_weight_q12",
            doc="Q12 blend amount applied to each sampled vector component.",
        ),
        param(
            "Animation_SplineChannel*",
            "vec3_track",
            doc="Position/vector keyframe track descriptor.",
        ),
        param(
            "Math_Vec3i32*",
            "inout_vec3",
            doc="Destination vector blended in place and returned.",
        ),
    ],
    doc=(
        "Samples vec3_track at frame_time with Anim_InterpolateVec3, then blends the sampled "
        "x/y/z into inout_vec3 in place using ((sample - current) * blend_weight_q12) >> 12."
    ),
)

stable.fn(
    "Anim_InterpolateVec3",
    "08 03 D0 8B 07 C1 E8 ??",
    match=-79,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param(
            "Animation_SplineChannel*",
            "vec3_track",
            doc="Position/vector keyframe track descriptor.",
        ),
        param(
            "Math_Vec3i32*",
            "out_vec3",
            doc="Receives the interpolated three-component vector.",
        ),
    ],
    doc=(
        "Samples a position/vector animation spline channel into out_vec3. Constant channels copy "
        "three int16 components from the track descriptor; keyed channels locate neighboring 10-dword "
        "keyframes and write interpolated 32-bit x/y/z components using Q12 coefficients selected by "
        "channel flags."
    ),
)

stable.fn(
    "Animation_CalculateSplineParameter",
    "8B 75 08 8B C7 C1 E8 ??",
    match=-12,
    ret="int32_t",
    params=[
        param(
            "uint32_t",
            "sample_time",
            doc="Current animation sample time in 1/64-frame keyframe units.",
        ),
        param(
            "uint32_t",
            "next_key_time",
            doc="Absolute next-key time in the same 1/64-frame units.",
        ),
        param(
            "uint32_t",
            "prev_key_packed",
            doc="Previous keyframe timing/easing word; high bits encode previous-key time and low bits encode easing metadata.",
        ),
        param(
            "uint8_t",
            "next_ease_index",
            doc="Low-byte easing/control index from the next keyframe timing word.",
        ),
        param(
            "uint8_t*",
            "keyframe_data",
            doc="Base of the keyframe data block; easing/control records are addressed before this pointer.",
        ),
    ],
    doc=(
        "Computes the normalized Q12 spline/easing parameter between two animation keyframes, using "
        "the previous/next packed key times, an interval reciprocal lookup when available, and "
        "easing/control records stored before keyframe_data. The returned Q12 weight feeds "
        "vector/quaternion keyframe interpolation."
    ),
)

stable.fn(
    "Anim_InterpolateKeyframe_Quat_Blend",
    "45 F8 56 50 51 52 E8 ??",
    match=-14,
    hook=6,
    ret="int16_t",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param(
            "int32_t",
            "blend_weight_q14",
            doc="Q14 blend amount used to mix the sampled quaternion into the destination.",
        ),
        param("int32_t*", "quat_track", doc="Quaternion keyframe track descriptor."),
        param(
            "Math_Quaternioni16*",
            "inout_quat",
            doc="Destination quaternion that is blended in place.",
        ),
    ],
    doc=(
        "Samples a quaternion animation track and blends the result into an existing "
        "Q14 quaternion."
    ),
)

stable.fn(
    "Anim_InterpolateQuat",
    "06 03 D0 8B 06 C1 E8 ??",
    match=-71,
    hook=6,
    ret="int32_t*",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param("int32_t*", "quat_track", doc="Quaternion keyframe track descriptor."),
        param(
            "Math_Quaternioni16*",
            "out_quat",
            doc="Receives the interpolated four-component Q14 quaternion.",
        ),
    ],
    doc="Samples/interpolates a quaternion animation track into a Q14 quaternion.",
)

stable.fn(
    "Animation_InterpolateQuaternionSlerp",
    "55 8B EC 53 56 57 8B 7D 18 66 85 FF 0F 84 ??",
    ret="int32_t",
    params=[
        param(
            "Math_Quaternioni16*",
            "out_quat",
            doc="Receives the interpolated Q14 quaternion.",
        ),
        param("Math_Quaternioni16*", "from_quat", doc="Starting Q14 quaternion."),
        param("Math_Quaternioni16*", "to_quat", doc="Ending Q14 quaternion."),
        param("int32_t", "blend_weight_q14", doc="Q14 interpolation weight."),
        param(
            "int32_t",
            "angle_fp12",
            doc="Angular distance/phase in the game's 12-bit sine-table domain.",
        ),
        param(
            "int32_t",
            "spin_phase",
            doc="Additional phase term folded into the interpolation angle.",
        ),
    ],
    doc=(
        "Interpolates two Q14 quaternions, using sine-weighted spherical interpolation "
        "when an angle is available and linear interpolation as a fallback."
    ),
)

stable.fn(
    "Anim_QuatToRotMatrix",
    "24 18 57 50 51 52 E8 ??",
    match=-16,
    hook=7,
    ret="Math_Matrix3x3i16*",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param("int32_t*", "quat_track", doc="Quaternion keyframe track descriptor."),
        param(
            "Math_Matrix3x3i16*",
            "out_matrix",
            doc="Receives the 3x3 signed fixed-point rotation matrix.",
        ),
    ],
    doc="Samples a quaternion animation track and converts it to a 3x3 int16 rotation matrix.",
)

stable.fn(
    "Anim_CheckKeyframeActive",
    "00 3B FE 72 ?? 0F 84 ??",
    match=-80,
    ret="uint8_t",
    params=[
        param(
            "uint32_t",
            "sample_time",
            doc="Animation sample time in the caller's channel time domain.",
        ),
        param(
            "Animation_SplineChannel*",
            "channel",
            doc="Spline channel descriptor whose packed keyframes are tested.",
        ),
    ],
    doc=(
        "Returns whether a spline channel is active at sample_time, with constant channels using "
        "packed_first_key as a start threshold and keyed channels locating the active packed keyframe "
        "by scan or binary search. Low channel_flags bits control end-of-channel wrapping."
    ),
)

stable.fn(
    "Anim_SplineInterpolate",
    "C0 FF 03 00 3B C8 72 ?? 75 13",
    match=-78,
    hook=6,
    ret="int16_t",
    params=[
        param(
            "uint32_t",
            "sample_time",
            doc="Animation sample time in the caller's channel time domain.",
        ),
        param(
            "Animation_SplineChannel*",
            "channel",
            doc="Scalar spline channel descriptor to sample.",
        ),
        param("int16_t*", "out_value", doc="Receives the sampled int16 channel value."),
    ],
    doc=(
        "Samples a scalar animation spline channel at sample_time and writes the int16 result to "
        "out_value. Constant channels write packed_first_key >> 12, while keyed channels find "
        "neighboring keys, compute the Q12 easing parameter, and evaluate a cubic Hermite-style "
        "fixed-point blend."
    ),
)

stable.fn(
    "Bone_TransformVertices_Morphed",
    "8B 01 83 F8 FF 0F 85 ??",
    match=-12,
    hook=6,
    ret="void",
    params=[
        param(
            "uint32_t",
            "sample_time",
            doc="Animation sample time used to evaluate morph channels.",
        ),
        param(
            "Animation_SplineChannel*",
            "morph_channels",
            doc="Morph channel array; a -1 keyframe_table_or_minus_one_sentinel sentinel selects the base-copy path.",
        ),
        param(
            "void*",
            "actor_or_mesh_state",
            doc="Render/actor mesh state whose +0x100 (PC EN) morph table feeds output buffers at +0x90 (PC EN) and +0x70 (PC EN).",
        ),
    ],
    doc=(
        "Transforms or copies morphed vertex and normal buffers for a render/actor mesh state. The "
        "sentinel path copies the base morph table selected through actor_or_mesh_state+0x100 (PC "
        "EN), while the keyed path evaluates active morph weights and accumulates weighted deltas "
        "into output buffers at +0x90 (PC EN) and +0x70 (PC EN)."
    ),
)

stable.fn(
    "Bone_TransformVertices_Weighted",
    "53 F6 40 0A 40 0F 84 ??",
    match=-9,
    hook=6,
    ret="void",
    params=[
        param(
            "uint32_t",
            "sample_time",
            doc="Animation sample time used to evaluate bone-weight channels.",
        ),
        param(
            "Animation_SplineChannel*",
            "bone_channels",
            doc="Bone/skin channel array; channel_flags bit 0x40 selects this weighted vertex path.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state whose skin table at +0xB0 (PC EN) and scene vertex buffer at +0x70 (PC EN) are updated.",
        ),
        param(
            "void*",
            "mesh_piece",
            doc="20-byte mesh piece descriptor; +4 is first vertex and +6 is vertex count for the affected span.",
        ),
    ],
    doc=(
        "Transforms weighted/skinned vertices for a mesh piece using animation bone channels. The "
        "sentinel path blends one skin table from actor->visual_morph_or_skin_target_table+0xB0 (PC "
        "EN), while the keyed path evaluates active spline channels, updates "
        "actor->scene_vertex_data+0x70 (PC EN), and recomputes normals."
    ),
)

stable.fn(
    "Bone_ComputeNormals_PostTransform",
    "?? 80 7B 64 03 0F 84 ??",
    match=-22,
    hook=6,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc=(
                "Actor/render state containing transformed vertices at +0x70 (PC EN), face records at +0x6C "
                "(PC EN), and normal accumulators at +0x74 (PC EN)."
            ),
        ),
        param(
            "void*",
            "mesh_piece",
            doc="20-byte mesh piece descriptor; +4/+6 select vertices and +8/+10 select faces.",
        ),
    ],
    doc=(
        "Computes post-transform face normals for a mesh piece, with transformed vertices from "
        "actor->scene_vertex_data+0x70 (PC EN). When mesh_piece flags bit0 is set, face normals "
        "accumulate into actor normal storage at +0x74 (PC EN) before affected vertices are "
        "normalized."
    ),
)

stable.fn(
    "Bone_BlendVertices_MultiWeight",
    "40 8D 04 8A 50 56 E8 ??",
    match=-43,
    ret="void",
    params=[
        param(
            "void*",
            "mesh_piece",
            doc="20-byte mesh piece descriptor; +4 is first vertex and +6 is vertex count.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state whose transformed vertex buffer at +0x70 (PC EN) and normal accumulator at +0x74 (PC EN) are updated.",
        ),
        param(
            "int16_t**",
            "skin_tables",
            doc="Array of source skin/bone xyz delta tables, each stored as 4 int16 values per vertex.",
        ),
        param(
            "int32_t*",
            "weights",
            doc="Q12 blend weights; up to four nonzero entries are consumed.",
        ),
    ],
    doc=(
        "Blends up to four weighted skin/bone vertex tables into the actor transformed vertex buffer "
        "for one mesh piece, then recomputes normals. A zero second weight copies the first skin "
        "table directly; otherwise the routine writes the first weighted table, accumulates remaining "
        "nonzero weights, clears touched normal accumulators, and calls "
        "Bone_ComputeNormals_PostTransform."
    ),
)

stable.fn(
    "Actor_CheckAnimationComplete",
    "53 56 8B 74 24 0C 57 85 F6 0F 84 ??",
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t", "anim_state")],
)

stable.fn(
    "Animation_QueueStateChange",
    "A1 ?? ?? ?? ?? 8B 4C 24 04 89",
    ret="int32_t",
    params=[param("int32_t", "anim_state")],
)

stable.fn(
    "Model_AdvanceAnimation",
    "?? 03 C5 89 46 60 A1 ??",
    match=-144,
    hook=7,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Bone_UpdateJointTracking",
    "83 C0 2C 50 51 52 E8 ??",
    match=-21,
    hook=6,
    ret="int16_t*",
    params=[
        param("Camera_Runtime*", "camera"),
        param("int32_t", "target_index"),
        param("int32_t*", "target_pos"),
    ],
)

stable.fn(
    "Scene_UpdateNodeAnimation",
    "81 7E 5C FF FF 0F 84 ??",
    match=-11,
    hook=6,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state providing animation tick, animation data table, behavior flags, and mesh-piece table.",
        ),
        param(
            "Scene_Node*",
            "parent_node",
            doc="Parent node transform source; +0x2C (PC EN) matrix and +0x40 (PC EN) position are used for world composition.",
        ),
        param(
            "Scene_Node*",
            "node",
            doc="Scene node whose local animated channels and world transform outputs are updated.",
        ),
    ],
    doc=(
        "Updates one scene node's animated local channels and composes its world transform from the "
        "parent node. It skips nodes with anim_seq_index == -1, samples position, rotation, "
        "scale/scalar channels when present, handles type 1/6 weighted vertex animation through "
        "Bone_TransformVertices_Weighted, handles type 8 scalar pairs, then writes the node world "
        "position, velocity delta, and world rotation matrix from the parent transform."
    ),
)

stable.fn(
    "Actor_ApplyVerticalVelocity",
    "7C ?? 10 57 50 56 E8 ??",
    match=-49,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t", "velocity")],
)

stable.fn(
    "Actor_ApplySplineMovement",
    "45 F4 8B 42 04 C1 E8 ??",
    match=-44,
    hook=6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Animation_SplineChannel*", "spline_track"),
        param("int32_t", "velocity"),
    ],
)

stable.fn(
    "Video_InitializeAVIPlayer",
    "6A 00 6A 00 6A 00 68 ?? ?? ?? ?? C7",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "char*",
            "window_handle_text",
            doc="Decimal HWND string passed by Movie_PlayIntro and parsed into avi_window_handle on successful MCI AVI initialization.",
        )
    ],
    doc=(
        "Initializes the Windows MCI AVI video subsystem by resetting avi_movie_counter, sending the "
        "global open-avivideo command, and storing the success flag in avi_player_initialized. On "
        "success it parses window_handle_text into retained AVI window-handle state."
    ),
)

stable.fn(
    "Video_ShutdownAVIPlayer",
    "?? FF 15 ?? ?? ?? ?? C7",
    match=-10,
    hook=6,
    ret="int32_t",
    params=[],
    doc='Sends the MCI "close avivideo" command, clears avi_player_initialized, and returns the MCI status/result.',
)

stable.fn(
    "Video_OpenAVIFile",
    "A1 ?? ?? ?? ?? 85 C0 75 ?? 33",
    ret="int32_t",
    params=[param("int32_t", "file_handle")],
)

stable.fn(
    "Video_CloseAVIFile", "01 FF 15 ?? ?? ?? ?? C3", match=-34, ret="int32_t", params=[]
)

stable.fn(
    "Video_PlayAVIFullscreen",
    "0C 6A 00 6A 00 6A 00 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? F7",
    match=-23,
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_IsAVIPlaying",
    "83 EC 50 68 ?? ?? ?? ?? 68 ??",
    required=Required.EN,
    hook=8,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Script_CheckTerminator",
    "FF 48 83 F8 03 0F 87 ??",
    match=-15,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc=(
        "Handles script terminator opcodes 1..4: clears *ip for hard end cases, skips "
        "nested variable-length blocks for opcode 2, and rotates actor script nesting state bytes "
        "at offsets 0x140..0x143 for opcodes 3/4."
    ),
)

stable.fn(
    "ScriptCmd_SetEntityIndex",
    "8B 44 24 08 8B 08 41 89 08 8B C1 8B 4C 24 04 8A 40 ?? 88 81 ?? ?? ?? ?? C3",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
    doc=(
        "Consumes one script byte from *script_cursor_inout and stores it in the actor entity-slot "
        "selector byte at offset +0x140 (PC EN)."
    ),
)

stable.fn(
    "ScriptCmd_ConditionalJump",
    "51 8B 4C 24 0C 33 D2 8B 01 83 C0 02 89 01 8A 70 ?? 8A 50 ?? 03 D0 40 89 01",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
    doc=(
        "Reads a 16-bit relative target and selector byte, then restores *script_cursor_inout to the "
        "target when the selector differs from actor+0x140 (PC EN); an unset actor entity-slot "
        "selector compares as 1."
    ),
)

stable.fn(
    "Script_ResolveVariableRef",
    "8B 44 24 0C 8B 4C 24 08 83 F8 09 7D ?? 8B ??",
    hook=8,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t* *", "value_ref"),
        param("int32_t", "ref_id"),
    ],
)

stable.fn(
    "Animation_GetProgress",
    "25 ?? ?? ?? ?? 50 51 6A 00 E8 ?? ?? ?? ?? 83",
    match=-10,
    hook=8,
    ret="int32_t",
    params=[
        param(
            "uint8_t",
            "progress_var_index",
            doc="Selector byte offset by +9 before resolving the backing script/global progress variable.",
        )
    ],
    doc=(
        "Resolves and returns an animation/movement progress variable by selector. The selector is "
        "stored as a byte by Actor_ProcessMovementBehavior; this wrapper adds 9 and passes that refId "
        "to Script_ResolveVariableRef with a null actor, so selectors map to level/global script "
        "variables."
    ),
)

stable.fn(
    "Animation_SetProgress",
    "?? ?? 8B 44 24 10 8B 54",
    match=-22,
    hook=8,
    ret="int32_t*",
    params=[
        param(
            "uint8_t",
            "progress_var_index",
            doc="Selector byte offset by +9 before resolving the backing script/global progress variable.",
        ),
        param(
            "int32_t",
            "progress_value",
            doc="Value written to the resolved progress variable.",
        ),
    ],
    doc=(
        "Resolves an animation/movement progress variable by selector, writes progress_value to the "
        "resolved int32 storage, and returns that resolved pointer. The selector is byte-sized, "
        "offset by +9 (PC EN), and resolved through Script_ResolveVariableRef with a null actor, "
        "matching Animation_GetProgress."
    ),
)

stable.fn(
    "Script_SetVariable",
    "C1 E3 08 57 0B DA E8 ??",
    match=-91,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_DecrementVariable",
    "?? ?? 8B 4C 24 14 83 C4",
    match=-30,
    hook=8,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_IndirectCall",
    "8B 44 24 08 50 8B 10 42 89 10 8B 44 24 08 8B CA 33 D2 50 8A 51 FF FF 14 95 ?? ?? ?? ??",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc="Consumes one script opcode byte and dispatches through the script command table.",
)

stable.fn(
    "Script_MoveToTarget",
    "66 81 FF FF 7F 0F 84 ??",
    match=-124,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_WalkToTarget",
    "89 4C 24 1C 75 ?? A1 ??",
    match=-98,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_RunToTarget",
    "03 C8 83 C0 02 8B E9 ??",
    match=-30,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_RotateActor",
    "83 C0 02 89 06 8B E9 ??",
    match=-29,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_MoveToTargetWithCamera",
    "08 8B 74 24 18 8B 1D ??",
    match=-100,
    hook=8,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_WaitForAnimation",
    "83 C0 02 89 06 8B 3D ??",
    match=-29,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_SetActorProperty",
    "08 0B CA 8B C1 8B 0D ??",
    match=-65,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_ClearActorProperty",
    "8B 44 24 08 6A FF 8B 08 41 89 08 8B C1 8B 0D ??",
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_PathfindToEntity",
    "3D FF 7F 74 ?? 8B 35 ??",
    match=-42,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_ActorPathTrace",
    "83 C0 04 89 07 8B 35 ??",
    match=-75,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc="Parses a path-trace command from *ip, resolves special entity-slot/actor selectors against current_level_data->entity_array, snapshots transient actor path-result state, and invokes Actor_TracePath before advancing the script pointer.",
)

stable.fn(
    "Script_AddNavigationCommand",
    "88 54 24 10 75 ?? 8B ??",
    match=-19,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc="Parses one navigation opcode from *ip and forwards it into the actor navigation queue. Opcode 0x67 derives the target entity slot from the current actor entry in current_level_data->entity_array before calling Navigation_AddCommand.",
)

stable.fn(
    "Script_TestPathTrace",
    "58 FF 8B EB 72 ?? 80 ??",
    match=-78,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_EmitSignal",
    "C2 3C FE 75 ?? C1 E8 ??",
    match=-75,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptOp_PlaySoundIndex",
    "?? 83 C4 08 85 C0 7C ?? 8B 15 ?? ?? ?? ?? 8B",
    match=-70,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
    doc=(
        "Script sound opcode that reads one byte from *script_cursor_inout, advances the "
        "cursor by one byte, and plays it as a level-local sound index."
    ),
)

stable.fn(
    "Script_StopSound",
    "?? 83 C4 08 85 C0 7C ?? 8B 15 ?? ?? ?? ?? 6A",
    match=-77,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_RemoveActor",
    "66 85 C0 7E ?? 8B 15 ?? ?? ?? ?? 68",
    match=-41,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_WithActor",
    "83 C0 02 89 06 8B 1D ??",
    match=-33,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Entity_GetOrSpawnCameraActor",
    "00 04 00 00 74 ?? 80 ??",
    match=-26,
    hook=10,
    ret="Actor_State*",
    params=[
        param(
            "Entity_State*",
            "source_entity",
            doc="Source entity whose active actor pointer at +0x128 (PC EN) is returned or spawned when camera activation requires it.",
        )
    ],
    doc=(
        "Ensures a source entity has an active actor for script/camera activation paths. Existing "
        "active actors are returned directly; otherwise the function toggles the source entity "
        "activation bits and calls Entity_SpawnActor(source_entity), returning null when the entity "
        "is already marked active without an actor."
    ),
)

stable.fn(
    "Entity_EnsureCameraActive",
    "5A 04 51 8B 6A 08 E8 ??",
    match=-66,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_TriggerLevelTransition",
    "03 C8 40 89 06 8B E9 ??",
    match=-28,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_TriggerMusicTransition",
    "83 EC 10 8B 44 24 18 56 C7 05 ??",
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Script_TriggerMusicFade",
    "EF C1 FA 05 8B CA C1 E9 ??",
    match=-129,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptOp_PlaySoundBlockOrWait",
    "03 D8 40 89 06 8B 3D ??",
    match=-27,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "script_cursor_inout")],
    doc=(
        "Length-prefixed script sound block opcode. It reads the block length and sound "
        "operand from *script_cursor_inout, may hold the cursor while playback is active, "
        "and uses Audio_PlaySoundDefinition3D with a current-level sound definition."
    ),
)

stable.fn(
    "Script_CheckButtonState",
    "?? 83 C4 10 83 F8 64 7D ?? 89",
    match=-106,
    hook=8,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "MiniGame_InitializeRoundParams",
    "66 8B 44 24 08 66 C7 05 ??",
    ret="void",
    params=[
        param(
            "Math_Vec3i32*",
            "position",
            doc="Actor position vector passed by both known callers; unused by the callee.",
        ),
        param(
            "int16_t",
            "round_param",
            doc="Round/minigame parameter stored in the global word at pcdogs.exe+0x234416 (PC EN).",
        ),
    ],
    doc="Initializes mini-game round globals by setting the round counter/default at pcdogs.exe+0x234414 (PC EN) to 10 and storing roundParam at pcdogs.exe+0x234416 (PC EN).",
)

stable.fn(
    "MiniGame_SetScoreValues",
    "8B 44 24 08 66 A3 ??",
    hook=10,
    ret="int32_t",
    params=[param("int16_t", "player_1_score"), param("int16_t", "player_2_score")],
)

stable.fn(
    "Camera_UpdateEffects",
    "62 00 E8 ?? ?? ?? ?? 83 C4 0C",
    match=-23,
    ret="int16_t",
    params=[],
)

stable.fn(
    "Camera_UpdateFade",
    "E0 0C 99 F7 F9 50 E8 ??",
    match=-67,
    ret="int16_t",
    params=[param("Camera_Runtime*", "camera")],
)

stable.fn(
    "Checkers_UpdateStateMachine",
    "55 8B EC 81 EC 54 05 00 00 A1 ??",
    hook=9,
    ret="void",
    params=[],
    doc=(
        "Processes the frame-driven checkers/minigame state machine. Updates global board, "
        "selection, current-player, move-result, animation, camera, and AI-search state; "
        "called from the render frame when checkers mode is active."
    ),
)

stable.fn(
    "Checkers_UpdateCameraPositions",
    "83 EC 0C 8B 0D ?? ?? ?? ?? 53 8B",
    hook=9,
    ret="void*",
    params=[],
)

stable.fn(
    "Checkers_ProcessInputAndRender",
    "56 8B 35 ?? ?? ?? ?? 57 33",
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Checkers_CenterCameraOnCell",
    "83 EC 0C 8B 0D ?? ?? ?? ?? A1",
    hook=9,
    ret="void*",
    params=[],
)

stable.fn(
    "Checkers_BuildMoveList",
    "E7 01 83 FF 08 0F 8D ??",
    match=-50,
    hook=7,
    ret="int32_t",
    params=[
        param("int32_t", "board_state"),
        param("int32_t", "player"),
        param("int32_t*", "move_list"),
    ],
)

stable.fn(
    "Checkers_CheckMoveValid",
    "8B 54 24 10 8B 44 24 14 8B CA 56 0B C8 57 F7 C1 ?? ?? ?? ?? 75 ??",
    ret="int32_t",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-playable-square checkers board, addressed as four stored cells per row.",
        ),
        param("int32_t", "from_row"),
        param("int32_t", "from_col"),
        param("int32_t", "to_row"),
        param("int32_t", "to_col"),
    ],
    doc="Validates a one-square checkers move against board bounds, destination occupancy, and piece direction/king rules.",
)

stable.fn(
    "Checkers_CheckCapturePossible",
    "8B 44 24 08 53 55 56 57 8D 78 FF 83 FF 05 ??",
    ret="int32_t",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-playable-square checkers board, addressed as four stored cells per row.",
        ),
        param(
            "int32_t",
            "piece",
            doc="Piece code at the queried square; observed move logic treats 1/2 as men and 5/6 as kings.",
        ),
        param(
            "int32_t",
            "row",
            doc="Zero-based source row to inspect for available captures.",
        ),
        param(
            "int32_t",
            "col",
            doc="Zero-based source column to inspect for available captures.",
        ),
    ],
    doc=(
        "Checks whether the checkers piece at (row, col) has any legal capture "
        "available in the directions allowed by its piece type. Board storage is "
        "the packed 32-square checkers layout used throughout the mini-game."
    ),
)

stable.fn(
    "Checkers_FindValidMoves",
    "F0 89 74 24 04 0F 8F ??",
    match=-15,
    hook=7,
    ret="void",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-square checkers board copied before recursive capture expansion.",
        ),
        param("int32_t", "from_row"),
        param("int32_t", "from_col"),
        param(
            "int32_t* *",
            "move_cursor",
            doc="Pointer to the current four-int move record cursor; advanced as legal moves are emitted.",
        ),
    ],
    doc=(
        "Enumerates legal moves from one checkers board coordinate into a cursor of "
        "four-int move records. The function reads a board pointer, source row/column, and an "
        "int32_t** cursor, then copies the 32-byte board for recursive capture expansion."
    ),
)

stable.fn(
    "Checkers_ValidateMove",
    "C1 F8 FF FF FF 0F 85 ??",
    match=-17,
    ret="int32_t",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-playable-square checkers board to validate against.",
        ),
        param("int32_t", "from_row", doc="Zero-based source row."),
        param("int32_t", "from_col", doc="Zero-based source column."),
        param("int32_t", "to_row", doc="Zero-based destination row."),
        param("int32_t", "to_col", doc="Zero-based destination column."),
    ],
    doc=(
        "Validates a checkers move on the supplied packed 32-square board: returns "
        "2 for a legal simple move, 1 for a legal capture, and 0 for invalid moves "
        "or blocked mandatory-capture cases. When the mandatory-capture rule is "
        "enabled, simple moves are rejected if any same-side piece can capture."
    ),
)

stable.fn(
    "Checkers_ExecuteMove",
    "24 57 50 53 51 56 E8 ??",
    match=-23,
    hook=8,
    ret="int32_t",
    params=[
        param("int32_t", "from_row"),
        param("int32_t", "from_col"),
        param("int32_t", "to_row"),
        param("int32_t", "to_col"),
        param("int32_t", "player"),
    ],
)

stable.fn(
    "Checkers_RecursiveSolverStep",
    "A1 ?? ?? ?? ?? 83 EC 40",
    ret="int32_t",
    params=[
        param("int32_t", "depth"),
        param("uint32_t", "player"),
        param("int32_t", "alpha"),
    ],
)

stable.fn(
    "Checkers_ExecuteMoveSequence",
    "07 57 56 52 51 50 E8 ??",
    match=-32,
    ret="int32_t",
    params=[param("int32_t", "move_count"), param("int32_t*", "move_sequence")],
)

stable.fn(
    "Checkers_SearchBestMove",
    "A1 ?? ?? ?? ?? 81 EC EC",
    ret="int32_t",
    params=[
        param("Checkers_Board*", "board"),
        param("int32_t", "depth"),
        param(
            "int32_t*",
            "out_move_4",
            doc="Optional four-int output record receiving the selected move.",
        ),
        param("int32_t", "player"),
        param("int32_t", "alpha"),
        param("int32_t", "beta"),
    ],
    doc="Recursive alpha-beta checkers AI search over a 32-byte board; writes an optional four-int best-move record.",
)

stable.fn(
    "Checkers_EvaluateBoardScore",
    "E0 01 83 F8 08 0F 8D ??",
    match=-16,
    hook=6,
    ret="int32_t",
    params=[param("Checkers_Board*", "board")],
)

stable.fn(
    "Checkers_AnimateMoveSequence",
    "C6 89 74 24 08 0F 8E ??",
    match=-16,
    hook=7,
    ret="int32_t",
    params=[param("int32_t*", "move_data")],
)

stable.fn(
    "Checkers_InitializeBoard",
    "56 8B 74 24 08 57 33 C9 33 C0 8D 14 08 F6 C2 ??",
    ret="void",
    params=[param("Checkers_Board*", "board")],
    doc="Initializes the 32-byte checkers board: playable dark squares in rows 0-2 become player 1 pieces, rows 3-4 become empty, and rows 5-7 become player 2 pieces.",
)

stable.fn(
    "Checkers_HighlightPlayerPieces",
    "E6 01 83 FE 08 0F 8D ??",
    match=-13,
    ret="void",
    params=[param("int32_t", "player"), param("void*", "highlight_data")],
)

stable.fn(
    "Actor_DestroyAll",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85",
    ret="int32_t*",
    params=[],
)

stable.fn(
    "Resource_UnloadGameData",
    "56 33 F6 56 E8 ??",
    hook=9,
    ret="void",
    params=[],
    doc="Unloads active game data, clears runtime callbacks/flags, and returns cleanup status.",
)

stable.fn(
    "Scene_ResetState",
    "8A 0D ?? ?? ?? ?? B8 02 00 00 00 84 C8 0F 85 ??",
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Animation_ProcessController",
    "8B 18 83 FB FF 0F 84 ??",
    match=-12,
    hook=6,
    ret="int32_t*",
    params=[
        param("int32_t*", "controller_data"),
        param("int32_t*", "state_data"),
        param("int32_t", "frame_count"),
    ],
)

stable.fn(
    "Render_ProcessMeshCommands",
    "06 33 C9 8A 08 83 E9 ??",
    match=-46,
    ret="void",
    params=[
        param("Mesh_CmdList*", "cmd_list"),
        param("int32_t*", "controller_data"),
        param("void*", "owner_context"),
    ],
    doc="Processes dirty mesh command entries, advancing animation/controller state and vertex-color commands.",
)

stable.fn(
    "Animation_ProcessVertexColor",
    "53 0C 57 85 D2 0F 84 ??",
    match=-17,
    hook=9,
    ret="void",
    params=[
        param("Animation_VertexColorController*", "color_controller"),
        param("Mesh_NodeExtended*", "mesh_node"),
    ],
    doc=(
        "Samples a vertex-color animation controller and writes clamped RGB bytes to the "
        "Mesh_RuntimeVertex buffer at meshNode+0x70 (PC EN). Keyframes store signed RGB deltas and "
        "Q12 interpolation factors; target rows provide vertex index, optional frame offset, and base "
        "RGB."
    ),
)

stable.fn(
    "Render_UpdateMeshCommandFlags",
    "A1 ?? ?? ?? ?? 83 EC 0C F6",
    ret="void",
    params=[param("Mesh_CmdList*", "cmd_list")],
    doc="Polls mesh command signal ids and updates command progress/flags; marks changed commands dirty.",
)

stable.fn(
    "Material_ReleaseTextureArray",
    "06 00 00 00 00 8B 0D ??",
    match=-18,
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7**", "texture_handles")],
    doc="Releases up to 25 cached DirectDraw texture surfaces and clears the bound texture if needed.",
)

stable.fn(
    "Material_ClearTextureCache",
    "75 ?? 8D 46 0C 50 E8 ??",
    match=-10,
    ret="HRESULT",
    params=[param("Material_RuntimeDescriptor*", "descriptor")],
    doc="Clears or releases a material descriptor's 25 cached texture handles and invalidates its cached transparent color.",
)

stable.fn(
    "Material_ReleaseSingleTexture",
    "A1 ?? ?? ?? ?? 48 A3",
    ret="uint32_t",
    params=[param("DDraw_IDirectDrawSurface7*", "texture_surface")],
    doc="Decrements active texture count and returns the DirectDraw surface Release result.",
)

stable.fn(
    "Texture_LoadAndUpload",
    "05 ?? ?? ?? ?? 00 E8 ?? ?? ?? ?? 83 C4 08 A3 ?? ?? ?? ?? 85 C0 75 ??",
    match=-11,
    required=Required.EN,
    ret="Material_BlendTextureSet*",
    params=[param("uint8_t*", "pixel_data")],
    doc="Creates the four loading-screen texture quadrants and uploads a 640x480 4-byte RGBx source buffer into them.",
)

stable.fn(
    "D3D_CreateTextureSurface",
    "81 EC 6C 01 00 00 A1 ??",
    hook=6,
    ret="DDraw_IDirectDrawSurface7*",
    params=[param("int32_t", "width"), param("int32_t", "height")],
)

stable.fn(
    "D3D_CreateWorkSurface",
    "24 00 89 44 24 0C A1 ??",
    match=-143,
    hook=6,
    ret="DDraw_IDirectDrawSurface7*",
    params=[
        param("DDraw_IDirectDrawSurface7*", "source_surface"),
        param("int32_t", "width"),
        param("int32_t", "height"),
    ],
)

stable.fn(
    "Material_ComputeAvgTransparentColor",
    "D2 88 5C 24 40 0F 85 ??",
    match=-59,
    ret="uint32_t",
    params=[
        param("uint8_t*", "pixel_data"),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
        param("uint8_t*", "out_red"),
        param("uint8_t*", "out_green"),
        param("uint8_t*", "out_blue"),
    ],
    doc="Computes average non-black horizontal neighbor RGB for black/transparent pixels in a 4-byte RGBx buffer.",
)

stable.fn(
    "Material_CopyPixelDataToTexture",
    "89 74 24 34 75 ?? 68 ??",
    match=-42,
    hook=6,
    ret="int32_t",
    params=[
        param("DDraw_IDirectDrawSurface7*", "texture_surface"),
        param("char*", "pixel_data"),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
    ],
)

stable.fn(
    "Texture_BlitToQuadrants",
    "89 5C 24 2C 75 ?? 68 ??",
    match=-42,
    hook=6,
    ret="int32_t",
    params=[
        param("DDraw_IDirectDrawSurface7**", "quadrant_surfaces"),
        param("uint8_t*", "pixel_data"),
        param("int32_t", "pixel_count"),
        param("int32_t", "width"),
        param("int32_t", "height"),
    ],
    doc="Converts a 4-byte RGBx image to a temporary 16-bit work surface and blits it into four hardcoded 640x480 quadrant surfaces.",
)

stable.fn(
    "Material_LoadTexture",
    "B8 48 00 08 00 E8 ??",
    ret="void",
    params=[param("Material_TableEntry*", "material_entry")],
    doc=(
        "Loads a material table entry's runtime descriptor into DirectDraw texture surfaces; cdecl "
        "void despite decompiler return artifacts."
    ),
)

stable.fn(
    "D3D_SetBlendMode",
    "53 56 8B 74 24 0C 85 F6 0F 85 ??",
    hook=6,
    ret="uint8_t",
    params=[param("int32_t", "blend_mode")],
    doc="Configures D3D blend/render state and returns an AL alpha/color byte for the selected blend mode.",
)

stable.fn(
    "D3D_SetTextureColorOperation",
    "51 A1 ?? ?? ?? ?? 8D",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "color_operation",
            doc="D3DTEXTUREOP value for texture stage 0 D3DTSS_COLOROP; observed callers pass 1 (disable) or 4 (modulate).",
        )
    ],
    doc=(
        "Ensures texture stage 0 D3DTSS_COLOROP equals colorOperation on the global IDirect3DDevice7. "
        "The routine reads GetTextureStageState(0, 1), compares the current value, and calls "
        "SetTextureStageState(0, 1, colorOperation) only when it differs."
    ),
)

stable.fn(
    "Render_TexturedQuad",
    "?? ?? ?? C0 88 44 24 10",
    match=-78,
    hook=6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "Material_TableEntry*",
            "material",
            doc="Runtime material record; offset 0 supplies flags and offset 4 supplies the texture descriptor/handle record.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Render flags; bits 24-26 request blend mode, falling back to material flags bit 0x80 when absent.",
        ),
        param(
            "uint32_t",
            "top_color",
            doc="Packed color used for the top two quad vertices; alpha is supplied by D3D_SetBlendMode.",
        ),
        param(
            "uint32_t",
            "bottom_color",
            doc="Packed color used for the bottom two quad vertices; alpha is supplied by D3D_SetBlendMode.",
        ),
        param(
            "int32_t",
            "quad_size",
            doc="Width/height in pixels added to packedXY to form the bottom-right corner.",
        ),
    ],
    doc="Draws a screen-space textured quad as a Direct3D triangle strip using FVF 0x144 and four transformed/lit textured vertices.",
)

stable.fn(
    "Render_TexturedQuadMaterialSize",
    "33 C9 8A 48 02 8B E9 ??",
    match=-51,
    hook=6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "Material_TableEntry*",
            "material",
            doc="Material/render entry; texture/frame data at +4 supplies width/height bytes.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Render flags; high blend bits can override material blend mode.",
        ),
        param(
            "uint32_t",
            "top_color",
            doc="Packed color used for the top two quad vertices after blend-byte insertion.",
        ),
        param(
            "uint32_t",
            "bottom_color",
            doc="Packed color used for the bottom two quad vertices after blend-byte insertion.",
        ),
    ],
    doc=(
        "Draws a textured screen-space quad using the material's texture/frame dimensions as width "
        "and height; used by font rendering callers but not font-specific."
    ),
)

stable.fn(
    "Render_TexturedSprite",
    "F6 57 3B DE 75 ?? BB ??",
    match=-17,
    hook=6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "uint32_t",
            "packed_wh",
            doc="Packed signed 16-bit width/height; non-positive components fall back to texture dimensions or 1.",
        ),
        param(
            "Render_SpriteContext*",
            "sprite",
            doc="Sprite/font render context; null uses the default font_glyph_render_state.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Blend/color/UV-orientation flags; bits 24-26 select blend, 0x18000000 permutes UVs, 0x40000000 requests gradient colors, 0x80000000 supplies packed RGB.",
        ),
        param(
            "uint32_t",
            "rotation_angle",
            doc="Low 12 bits are fixed-angle units; zero disables rotation.",
        ),
        param(
            "uint32_t",
            "rotation_pivot_xy",
            doc="Packed signed 16-bit x/y pivot used when rotationAngle is nonzero.",
        ),
    ],
    doc=(
        "Draws a screen-space sprite/font glyph as a Direct3D triangle strip. "
        "Builds four transformed/lit vertices, derives missing dimensions from the texture descriptor, "
        "sets texture/color state, optionally permutes UVs, applies gradient color mode, and rotates around "
        "rotationPivotXY when rotationAngle is nonzero."
    ),
)

stable.fn(
    "D3D_SetZWriteEnable",
    "A1 ?? ?? ?? ?? F6 C4 20 74 ?? A1 ?? ?? ?? ?? 8B",
    ret="void",
    params=[
        param(
            "BOOL",
            "enable",
            doc="Passed to D3DRENDERSTATE_ZWRITEENABLE; callers use 0 then 1 around depth-write-suppressed draws.",
        ),
    ],
    doc="If the D3D render-state capability flag is set, calls IDirect3DDevice7::SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, enable); otherwise no-ops.",
)

stable.fn(
    "Render_ClipPolygonByPlane",
    "C9 89 4C 24 18 0F 8E ??",
    match=-88,
    hook=7,
    ret="int32_t",
    params=[
        param("Render_ClipVertex*", "input_vertices"),
        param("Render_ClipAttribute*", "input_attributes"),
        param("Render_ClipPlane*", "clip_plane"),
        param("Render_ClipVertex*", "output_vertices"),
        param("Render_ClipAttribute*", "output_attributes"),
        param("int32_t*", "in_out_vertex_count"),
    ],
    doc=(
        "Clips a polygon against one plane, updating inOutVertexCount while copying inside vertices "
        "and emitting interpolated edge intersections into the output vertex and attribute buffers. "
        "Returns 1 when at least three vertices remain, otherwise 0."
    ),
)

stable.fn(
    "Render_ClipPolygonByCameraPyramid",
    "24 14 8B 74 24 18 B8 ??",
    match=-18,
    ret="int32_t",
    params=[
        param("Render_ClipVertex*", "input_vertices"),
        param("Render_ClipAttribute*", "input_attributes"),
        param("Render_ClipVertex*", "output_vertices"),
        param("Render_ClipAttribute*", "output_attributes"),
        param("int32_t*", "in_out_vertex_count"),
    ],
    doc="Clips a polygon through the camera clipping plane slab using Render_ClipPolygonByPlane and scratch temp buffers. Returns 0 as soon as a clipping pass leaves fewer than three vertices; otherwise writes final clipped vertices/attributes to the caller buffers.",
)

stable.fn(
    "Math_CalculateFaceNormal",
    "44 24 14 D8 CB DE E9 ??",
    match=-62,
    ret="float*",
    params=[
        param("float*", "out_normal"),
        param("float*", "point_0"),
        param("float*", "point_1"),
        param("float*", "point_2"),
    ],
    doc="Computes and normalizes the face normal from three 3D points, writes it to outNormal, and returns outNormal.",
)

stable.fn(
    "Camera_SetupClipPlanes",
    "83 EC 34 DB 44 24 3C 8B 44 24 38 8B 15 ??",
    hook=7,
    ret="void",
    params=[
        param("float", "projection_depth"),
        param("int32_t", "screen_half_width"),
        param("int32_t", "screen_half_height"),
    ],
    doc="Initializes the near plane and the four camera-pyramid side clip-plane globals from projection depth and screen half extents; no semantic return value.",
)

stable.fn(
    "Math_SnapVertexToNearestPoint",
    "53 8B 5C 24 14 55 33 C9 56 57 85 DB 0F 8E ??",
    ret="void",
    params=[
        param("float*", "x"),
        param("float*", "y"),
        param("Render_ProjectedVertex*", "screen_vertices"),
        param("int32_t", "vertex_count"),
    ],
    doc="Snaps x/y to the first nearby Render_ProjectedVertex in the 8-byte-stride screen-vertex array whose screen_x/screen_y are both within 2 pixels.",
)

stable.fn(
    "Render_QuadClipped",
    "D1 38 D9 41 3C D8 1D ??",
    match=-50,
    hook=7,
    ret="void",
    params=[
        param("Render_PolygonBatchRecord*", "batch"),
        param("void*", "vertex_buffer_base"),
        param("uint8_t", "clip_color_byte"),
        param("int32_t", "unused_or_mode"),
        param("int32_t", "brighten_colors"),
    ],
    doc=(
        "Clipped quad helper used by Render_DrawQuad, building and clipping vertices from batch data. "
        "brightenColors doubles/clamps vertex RGB when nonzero; the fourth argument is passed by "
        "callers but has no observed effect."
    ),
)

stable.fn(
    "Render_DrawQuad",
    "00 00 00 74 ?? 39 3D ??",
    match=-33,
    hook=8,
    ret="int32_t",
    params=[param("Render_PolygonBatchRecord*", "batch")],
    doc="Issues or queues draw work for one transformed polygon batch and returns 1 on normal exit.",
)

stable.fn(
    "Texture_SelectLOD",
    "A1 ?? ?? ?? ?? 8B 54 24 04 8B",
    ret="int32_t",
    params=[param("Render_PolygonBatchRecord*", "batch")],
    doc="Selects a texture LOD/render bucket from graphics capability flags and the polygon batch screen-depth fields; returns -1 when no LOD bucket applies.",
)

stable.fn(
    "Debug_Log",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 44 24 08",
    ret="int32_t",
    params=[param("char const*", "message"), param("char const*", "prefix")],
    doc="Writes prefix, then message, as separate newline-terminated strings to the debug log file when open; NULL arguments are skipped.",
)

stable.fn(
    "D3D_InitDirectDrawAndDirect3D",
    "A1 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81",
    ret="int32_t",
    params=[param("HWND", "hwnd")],
)

stable.fn(
    "D3D_EnumZBufferFormatCallback",
    "56 8B 74 24 08 81 7E 04 00 04 00 00 75 ?? 57 8B 7C 24 10 B9 08 00 00 00 33 C0 F3 A5 5F 5E C2 08 00 B8 01 00 00 00 5E C2 08 00 90 90 90 90 90 90 A1 ??",
    cc=CallingConvention.STDCALL,
    ret="int32_t",
    params=[
        param("DDraw_PixelFormat*", "pixel_format"),
        param("DDraw_PixelFormat*", "selected_format"),
    ],
    doc=(
        "IDirect3D7::EnumZBufferFormats callback: when the DDraw_PixelFormat flags include "
        "DDPF_ZBUFFER (0x400), copies the 0x20-byte format to selectedFormat and returns "
        "D3DENUMRET_CANCEL/0; otherwise returns D3DENUMRET_OK/1 to continue enumeration."
    ),
)

stable.fn(
    "D3D_ReleaseAllAndReportLeaks",
    "A1 ?? ?? ?? ?? 56 33 F6 3B C6 74 ?? 8B",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Render_DrawRectangle",
    "D3 89 74 24 10 C1 E8 ??",
    match=-48,
    hook=7,
    ret="void",
    params=[
        param(
            "int32_t", "packed_origin_xy", doc="Packed signed 16-bit screen x/y origin."
        ),
        param(
            "int32_t",
            "packed_size_wh",
            doc="Packed signed 16-bit width/height added to the origin.",
        ),
        param(
            "int32_t",
            "blend_flags",
            doc="Low word is a 0..0x1000 color scale; high bits select blend behavior.",
        ),
        param(
            "uint32_t",
            "rgb_color",
            doc="Packed RGB source color; final diffuse alpha is supplied by blend state.",
        ),
    ],
    doc="Draws a filled screen-space rectangle through Direct3D DrawPrimitive.",
)

stable.fn(
    "Render_DrawFilledRectangleGradient",
    "00 89 54 24 14 C1 E8 ??",
    match=-76,
    hook=7,
    ret="int16_t",
    params=[
        param(
            "int32_t",
            "packed_origin_xy",
            doc="Packed signed 16-bit screen-space x/y origin.",
        ),
        param(
            "int32_t",
            "packed_size_wh",
            doc="Packed signed 16-bit width/height added to the origin.",
        ),
        param(
            "int32_t",
            "top_left_color",
            doc="Packed color for the top-left vertex; its low byte is reused as the shared alpha byte.",
        ),
        param(
            "int32_t", "top_right_color", doc="Packed color for the top-right vertex."
        ),
        param(
            "int32_t",
            "bottom_left_color",
            doc="Packed color for the bottom-left vertex.",
        ),
        param(
            "int32_t",
            "bottom_right_color",
            doc="Packed color for the bottom-right vertex.",
        ),
    ],
    doc=(
        "Draws a filled screen-space rectangle as a Direct3D triangle strip with "
        "per-corner diffuse colors."
    ),
)

stable.fn(
    "Render_DrawFadeOverlay",
    "7C ?? 18 8B C8 C7 05 ??",
    match=-44,
    hook=7,
    ret="void",
    params=[
        param(
            "int32_t", "packed_origin_xy", doc="Packed signed 16-bit screen x/y origin."
        ),
        param(
            "int32_t",
            "packed_size_wh",
            doc="Packed signed 16-bit width/height added to the origin.",
        ),
        param(
            "int32_t",
            "opacity_12",
            doc="0..0x1000 fixed-point opacity; 0x1000 maps to alpha 255.",
        ),
    ],
    doc="Draws a black alpha-blended rectangle overlay.",
)

stable.fn(
    "Render_SetFadeLevel",
    "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 8B 44 24 04",
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "fade_level")],
)

stable.fn(
    "D3D_CheckDeviceLost",
    "32 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 81 EC 80 00 00 00 A1 ??",
    hook=hook(2, kind=HookKind.HOTPATCH),
    ret="int32_t",
    params=[],
)

stable.fn(
    "D3D_RenderTexturedQuad",
    "81 EC 80 00 00 00 A1 ??",
    hook=6,
    ret="int32_t",
    params=[param("Material_BlendTextureSet*", "blend_textures")],
)

stable.fn(
    "D3D_ClearViewport",
    "?? 6A 00 68 00 00 80 3F 6A 00 8B 08 6A 03",
    match=-4,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Render_ClearDepthBuffer",
    "?? 6A 00 68 00 00 80 3F 6A 00 8B 08 6A 02",
    match=-4,
    ret="int32_t",
    params=[],
)

stable.fn(
    "D3D_SignalHandler",
    "E8 ?? ?? ?? ?? 83 C4 1C C3",
    match=-55,
    ret="int32_t",
    params=[],
)

stable.fn(
    "D3D_InitializeDirectDraw",
    "81 EC 7C 02 00 00 C7 05 ??",
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "D3D_ReleaseDirectDrawDevice",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 08",
    ret="uint32_t",
    params=[],
    doc="Releases the global IDirectDraw7 interface, clears the global pointer, and returns the COM Release refcount or 0 when no interface was present.",
)

stable.fn(
    "Render_ClearScreenWithColor",
    "56 8B 35 ?? ?? ?? ?? 57 8B 3D ?? ?? ?? ?? 6A",
    hook=7,
    ret="void",
    params=[param("uint32_t", "rgb_color")],
    doc="Fills the current viewport with a solid color by drawing a full-screen rectangle while depth writes are disabled.",
)

stable.fn(
    "Camera_SetupProjection",
    "0C 89 44 24 0C F7 35 ??",
    match=-10,
    hook=6,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera")],
)

stable.fn(
    "Render_TakeScreenshot",
    "81 EC 04 01 00 00 56 33 F6 56 8D 44 24 08 68 ??",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "DDraw_UnlockSurface",
    "8B 44 24 04 6A 00 50 8B 08 FF 91 80 00 00 00 C3 83 EC 18 53 55 56 57 6A 28 6A 40 FF 15 ??",
    hook=6,
    ret="int32_t",
    params=[param("DDraw_IDirectDrawSurface7*", "surface")],
)

stable.fn(
    "File_SaveScreenshot",
    "83 EC 18 53 55 56 57 6A 28 6A 40 FF 15 ??",
    ret="void",
    params=[param("const char*", "path")],
    doc="Writes the current 640x480 backbuffer to a BMP file at path; the original game writes a larger pixel payload than the header size field reports.",
)

stable.fn(
    "D3D_SetGammaRamp",
    "D9 44 24 04 D8 1D ?? ?? ?? ?? 8B",
    hook=10,
    ret="int32_t",
    params=[param("float", "gamma_scale")],
    doc="Caches the requested gamma scale, clamps the applied gamma to 0.1 for inputs below 0.1 and to 5.0 for inputs above 10.0, builds a 256-entry RGB DirectDraw gamma ramp through IDirectDrawGammaControl, and returns the residual HRESULT/status from the DirectDraw calls.",
)


stable.fn(
    "DDraw_CompareDisplayModes",
    "8B 54 24 04 56 8B 74 24 0C 8B 42 0C 8B 4E 0C 3B C1 73 ?? 83 C8 FF 5E C3 76 ?? B8 01 00 00 00 5E C3 8B 42 08",
    ret="int32_t",
    params=[
        param("const DDraw_SurfaceDesc2*", "left"),
        param("const DDraw_SurfaceDesc2*", "right"),
    ],
    doc="qsort comparator for enumerated display modes: sorts by width, then height, then pixel-format RGB bit count using DDraw_SurfaceDesc2 offsets 0x0c, 0x08, and 0x54.",
)

stable.fn(
    "D3D_EnumerateDirectDrawDevices",
    "8B 44 24 04 56 6A 07 6A 00 68 ??",
    ret="int32_t",
    params=[param("D3D_DriverAcceptCallback", "accept_driver")],
    doc="Runs DirectDrawEnumerateExA with DDraw_EnumerateCallback and stores the caller-provided driver acceptance callback for enumeration filtering; returns 0 on accepted devices, 0x81000002 when no devices/modes were enumerated, or 0x81000003 when no enumerated devices were accepted.",
)

stable.fn(
    "DDraw_AddDisplayMode",
    "8B 44 24 08 56 8B 74 24 08 57 8B 88 ?? ?? ?? ?? 8B D1 C1 E2 ??",
    cc=CallingConvention.STDCALL,
    ret="BOOL",
    params=[
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("void*", "enum_context"),
    ],
    doc="IDirectDraw7::EnumDisplayModes callback that appends each 0x7c-byte DDraw_SurfaceDesc2 to the driver enumeration context, increments the mode count, and returns TRUE while count is <= 0x4f.",
)

stable.fn(
    "DDraw_EnumerateCallback",
    "B8 94 2B 00 00 E8 ??",
    cc=CallingConvention.STDCALL,
    ret="BOOL",
    params=[
        param("Win32_GUID*", "guid"),
        param("const char*", "driver_name"),
        param("char*", "driver_description"),
        param("void*", "context"),
        param("HMONITOR", "monitor"),
    ],
    doc="DirectDrawEnumerateExA callback: creates DirectDraw/Direct3D interfaces for a driver, records display modes through DDraw_AddDisplayMode, sorts them with DDraw_CompareDisplayModes, enumerates D3D devices, and returns TRUE to continue enumeration.",
)

stable.fn(
    "D3D_EnumDeviceCallback",
    "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81",
    cc=CallingConvention.STDCALL,
    hook=6,
    ret="BOOL",
    params=[
        param("const char*", "device_description"),
        param("const char*", "device_name"),
        param("D3D_DeviceDesc7*", "device_desc"),
        param("D3D_DriverInfo*", "enum_context"),
    ],
    doc="IDirect3D7::EnumDevices callback: filters/copies accepted D3D_DeviceDesc7 records into the global enumerated-device list and returns TRUE to continue enumeration.",
)

stable.fn(
    "D3D_GetDriverEnumerationData",
    "08 85 C0 74 08 8B 0D ??",
    match=-17,
    hook=6,
    ret="void",
    params=[
        param("D3D_DriverInfo* *", "driver_list_out"),
        param("int32_t*", "count_out"),
    ],
    doc="Writes the global accepted DirectDraw/Direct3D driver list and accepted-device count to caller-provided output pointers.",
)

stable.fn(
    "D3D_SelectBestDriver",
    "8D 4C 24 20 50 51 E8 ??",
    match=-50,
    ret="int32_t",
    params=[
        param("D3D_DriverInfo* *", "selected_driver_out"),
        param("int32_t", "flags"),
    ],
)

stable.fn(
    "D3D_AlwaysAcceptDriver",
    "B8 01 00 00 00 C3 90 90 90 90 90 90 90 90 90 90 51 68 ??",
    ret="int32_t",
    params=[param("D3D_DriverInfo*", "driver_info")],
    doc="D3D_DriverAcceptCallback implementation that ignores driverInfo and always returns accepted/nonzero.",
)

stable.fn(
    "D3D_GetSelectedDriverInfo",
    "51 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8D",
    hook=6,
    ret="D3D_DriverInfo*",
    params=[],
    doc="Enumerates DirectDraw/Direct3D drivers with D3D_AlwaysAcceptDriver, selects the default/best driver with flags 0, and returns the selected D3D_DriverInfo pointer.",
)

stable.fn(
    "D3D_FormatDirectXError",
    "8B 44 24 04 3D E0 01 76 88 0F 8F ??",
    hook=9,
    ret="char*",
    params=[param("HRESULT", "error_code"), param("char*", "out_buffer")],
    doc=(
        "Formats a DirectX/DirectDraw HRESULT into a static message buffer using sparse "
        "range dispatch/index tables, optionally copies it to outBuffer, and returns the "
        "static buffer pointer."
    ),
)

stable.fn(
    "DInput_CreateInterface",
    "68 00 07 00 00 51 E8 ??",
    match=-11,
    hook=8,
    ret="DInput_IDirectInputA*",
    params=[param("HINSTANCE", "h_instance")],
    doc="Calls DirectInputCreateA(hInstance, 0x700, &directInput, NULL) and returns the created IDirectInputA pointer on success or NULL on failure.",
)

stable.fn(
    "D3D_QueryAndGetCapabilities",
    "?? 8B F0 8B 44 24 0C 83",
    match=-39,
    hook=8,
    ret="int32_t*",
    params=[param("D3D_IDirect3DDevice7*", "device")],
)

stable.fn(
    "D3D_GetDeviceCapabilities",
    "8B 44 24 04 8D 54 24 04 52 68 ??",
    hook=8,
    ret="int32_t*",
    params=[param("D3D_IDirect3DDevice7*", "device")],
)

stable.fn(
    "Input_SetDeviceDataFormat",
    "8B 44 24 04 8B 54 24 08 52 50 8B 08 FF 51 ?? F7 D8 1B C0 40 C3",
    hook=8,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("DInput_DataFormat*", "data_format"),
    ],
    doc="Calls IDirectInputDevice::SetDataFormat and returns 1 on success, 0 on failure.",
)

stable.fn(
    "Input_AcquireDevice",
    "8B 44 24 04 50 8B 08 FF 51 1C F7 D8 1B C0 40 C3 56 57 8B 7C 24 10 57 E8 ??",
    ret="int32_t",
    params=[param("DInput_IDirectInputDevice*", "device")],
    doc="Calls IDirectInputDevice::Acquire and returns 1 on success, 0 on failure.",
)

stable.fn(
    "Input_GetDeviceData",
    "56 57 8B 7C 24 10 57 E8 ?? ?? ?? ?? 83 C4 04 8B",
    hook=6,
    ret="void*",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("uint32_t", "state_size"),
    ],
    doc="Allocates a state buffer, calls IDirectInputDevice::GetDeviceState(stateSize, buffer), and returns the buffer on success or NULL on failure.",
)

stable.fn(
    "DInput_EnumJoystickDeviceCallback",
    "08 B8 01 00 00 00 A3 ??",
    cc=CallingConvention.STDCALL,
    match=-57,
    hook=6,
    ret="BOOL",
    params=[
        param("const DInput_DeviceInstanceA*", "device_instance"),
        param("int32_t*", "enum_state"),
    ],
    doc="DirectInput EnumDevices callback: copies deviceInstance->guidInstance into enumState[1 + 4*enumState[0]], increments the count, marks that a DirectInput device was seen, and returns DIENUM_CONTINUE; returns 0 for null inputs.",
)

stable.fn(
    "Input_SetJoystickXAxisRange",
    "04 50 51 6A 00 52 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc="Sets DIPROP_RANGE for joystick X axis offset 0 and returns 1 on success.",
)

stable.fn(
    "Input_SetJoystickYAxisRange",
    "04 50 51 6A 04 52 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc="Sets DIPROP_RANGE for joystick Y axis offset 4 and returns 1 on success.",
)

stable.fn(
    "Input_SetJoystickZAxisRange",
    "04 50 51 6A 08 52 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc="Sets DIPROP_RANGE for joystick Z axis offset 8 and returns 1 on success.",
)

stable.fn(
    "Input_SetJoystickRzAxisRange",
    "04 50 51 6A 14 52 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputDevice*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc="Sets DIPROP_RANGE for joystick Rz axis offset +0x14 (PC EN) and returns 1 on success.",
)

stable.fn(
    "DInput_EnumerateForceFeedbackJoysticks",
    "01 00 00 52 8B 08 68 ??",
    match=-21,
    hook=7,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("void*", "enum_state_buffer"),
    ],
    doc="Enumerates attached force-feedback joysticks via IDirectInputA::EnumDevices(DIDEVTYPE_JOYSTICK, DInput_EnumJoystickDeviceCallback, enumStateBuffer, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK), stores GUID entries, records whether any were found, and returns the count.",
)

stable.fn(
    "DInput_EnumerateAttachedJoysticks",
    "0C 6A 01 52 8B 08 68 ??",
    match=-18,
    hook=7,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("void*", "enum_state_buffer"),
    ],
    doc="Enumerates attached joysticks via IDirectInputA::EnumDevices(DIDEVTYPE_JOYSTICK, DInput_EnumJoystickDeviceCallback, enumStateBuffer, DIEDFL_ATTACHEDONLY), stores GUID entries, and returns the count.",
)

stable.fn(
    "Input_SetExclusiveForegroundCooperativeLevel",
    "8B 44 24 08 8B 4C 24 04 6A 05 50 51 E8 ??",
    hook=8,
    ret="int32_t",
    params=[param("DInput_IDirectInputDevice*", "device"), param("HWND", "hwnd")],
    doc="Calls IDirectInputDevice::SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND).",
)

stable.fn(
    "Input_SetJoystickDataFormat",
    "8B 44 24 04 68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 83 C4 08 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B",
    hook=9,
    ret="int32_t",
    params=[param("DInput_IDirectInputDevice*", "device")],
    doc="Sets the device data format to the 0x50-byte joystick state DInput_DataFormat at pcdogs.exe+0x4D198 (PC EN).",
)

stable.fn(
    "Input_SetJoystick2DataFormat",
    "56 8B 06 FF 50 64 6A 50 56 E8 ?? ?? ?? ?? 83 C4 08 85 C0",
    match=-37,
    hook=9,
    ret="int32_t",
    params=[param("DInput_IDirectInputDevice*", "device")],
    doc="Sets the device data format to the 0x110-byte extended joystick state DInput_DataFormat at pcdogs.exe+0x4D1B0 (PC EN).",
)

stable.fn(
    "Input_PollJoystickState",
    "56 8B 06 FF 50 64 6A 50 56 E8 ?? ?? ?? ?? 83 C4 08 85 C0",
    match=-5,
    ret="DInput_JoystickState*",
    params=[param("DInput_IDirectInputDevice*", "device")],
)

stable.fn(
    "JoyState_GetAxisX",
    "8B 44 24 04 8B 00 C3 90 90 90 90 90 90 90 90 90 8B 44 24 04 8B 40 ?? C3",
    hook=7,
    ret="int32_t",
    params=[param("DInput_JoystickState*", "state")],
    doc="Returns DInput_JoystickState.lX (offset 0).",
)

stable.fn(
    "JoyState_GetAxisY",
    "8B 44 24 04 8B 40 04 C3 90 90 90 90 90 90 90 90 8B 44 24 04 8B 40 14 C3 90 90 90 90 90 90 90 90 8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=7,
    ret="int32_t",
    params=[param("DInput_JoystickState*", "state")],
    doc="Returns DInput_JoystickState.lY (offset 4), the raw vertical axis sampled by Input_ReadGamepad.",
)

stable.fn(
    "JoyState_GetAxisRz",
    "8B 44 24 04 8B 40 14 C3 90 90 90 90 90 90 90 90 8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=7,
    ret="int32_t",
    params=[param("DInput_JoystickState*", "state")],
    doc=(
        "Returns DInput_JoystickState.lRz (offset +0x14 (PC EN)), the raw twist/alternate horizontal "
        "axis sampled by Input_ReadGamepad."
    ),
)

stable.fn(
    "JoyState_GetButtonByte",
    "8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=8,
    ret="uint8_t",
    params=[param("DInput_JoystickState*", "state"), param("int32_t", "button_index")],
)

stable.fn(
    "JoyState_IsButtonPressed",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 25",
    hook=8,
    ret="BOOL",
    params=[param("DInput_JoystickState*", "state"), param("int32_t", "button_index")],
)

stable.fn(
    "DInput_CreateConfiguredJoystickDevice",
    "00 00 00 57 50 56 E8 ??",
    match=-16,
    hook=6,
    ret="DInput_IDirectInputDevice*",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("HWND", "hwnd"),
        param("int32_t", "device_index"),
        param("int32_t", "setup_mode"),
    ],
    doc="Enumerates DirectInput joystick GUIDs, creates the selected device, sets joystick or extended data format according to setupMode, sets exclusive foreground cooperative level, optionally creates a constant-force effect, acquires the device, and returns it on success.",
)

stable.fn(
    "DInput_SetConstantForceEffect",
    "A1 ?? ?? ?? ?? 83 EC 44",
    ret="HRESULT",
    params=[
        param("LONG", "direction_x"),
        param("LONG", "direction_y"),
        param("DWORD", "duration"),
    ],
    doc="Updates the global DirectInput constant-force effect with duration, two-axis direction, and a scaled magnitude, then calls SetParameters with DIEP_START | DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION | DIEP_DURATION; returns 0 when no effect object is available.",
)

stable.fn(
    "D3D_SetRenderTarget",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? A1 ?? ?? ?? ?? 6A 00 6A 01 50 8B 08 FF 51 1C C3",
    ret="int32_t",
    params=[],
)

stable.fn(
    "D3D_SetDataScaleFactor",
    "8B 44 24 04 A3 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 44",
    hook=9,
    ret="int32_t",
    params=[param("int32_t", "scale_factor")],
)

stable.fn(
    "DInput_CreateJoystickDevice",
    "04 6A 01 50 51 52 E8 ??",
    match=-11,
    hook=8,
    ret="DInput_IDirectInputDevice*",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("HWND", "hwnd"),
        param("int32_t", "device_index"),
    ],
    doc="Creates/configures a DirectInput joystick device through the shared device setup helper, enabling force-feedback setup when supported.",
)

stable.fn(
    "DInput_InitializeJoystickInput",
    "8B 4C 24 08 8B 44 24 04 81 EC 80 00 00 00 A3 ??",
    hook=8,
    ret="BOOL",
    params=[param("HWND", "hwnd"), param("HINSTANCE", "h_instance")],
    doc="Initializes DirectInput joystick support: creates the DirectInput interface, enumerates devices, creates the first joystick, and sets X/Y/Z/Rz axis ranges to -1000..1000.",
)

stable.fn(
    "Video_InitPlayer",
    "?? ?? ?? 33 C0 5E C3 56",
    match=-27,
    ret="BOOL",
    params=[param("HWND", "hwnd")],
    doc="Initializes the video playback subsystem, then the sound playback subsystem, for the supplied window handle; shuts both systems down and returns 0 on either initialization failure.",
)

stable.fn(
    "Video_CloseMovieFile",
    "A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 68",
    ret="BOOL",
    params=[],
    doc="Stops the active movie timer, restores playback mode, shuts down sound/video/movie handles, and returns 1.",
)

stable.fn(
    "Video_OpenMovieFile",
    "?? ?? ?? 8A 44 24 10 3C",
    match=-35,
    ret="BOOL",
    params=[
        param("HWND", "hwnd"),
        param("const char*", "movie_path"),
        param("DDraw_IDirectDraw7*", "unused_ddraw"),
        param("char", "use_alt_video_rect"),
    ],
    doc="Opens an RPL/movie file, selects the default or alternate video rectangle, initializes movie/video/playback/sound state, maps video, and starts the playback timer; the wrapper preserves the DirectDraw ABI argument.",
)

stable.fn(
    "Video_PlayMovieLoop",
    "A1 ?? ?? ?? ?? 83 EC 0C 53",
    ret="int32_t",
    params=[],
    doc="Runs movie playback until the movie ends, an error or joystick input occurs, or ESC/ENTER/Alt+F4 is pressed; returns 1 for normal/enter/joystick stop, 2 for ESC, and 3 for Alt+F4.",
)

stable.fn(
    "Video_ShutdownPlayerSystems",
    "E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 90 90 90 90 90 8B",
    ret="void",
    params=[],
    doc="Shuts down the video playback subsystem and then the sound playback subsystem.",
)

stable.fn(
    "Render_SetPolygonUVs",
    "8B 4D 04 3B CA 0F 84 ??",
    match=-66,
    hook=7,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Render_PolygonBatchRecord*", "out_batch"),
        param("Pkg_PolygonData*", "polygon_data"),
        param("int16_t", "uv_index_or_mode"),
        param("Mesh_RuntimeVertex**", "polygon_vertices"),
    ],
    doc="Copies polygon material/render fields into outBatch and writes the four packed texture UV pairs. Supports explicit, indexed, tiled, rotated/flipped, and environment/camera-based UV modes.",
)

stable.fn(
    "Render_PolygonMesh",
    "55 8B EC 83 EC 70 A1 ??",
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Transforms an actor/render-state quad into view/screen space, fills a polygon batch record, assigns UVs, and links it into render-list buckets. Callers ignore the residual return register.",
)

stable.fn(
    "Render_MeshNode",
    "55 8B EC 83 EC 64 A1 ??",
    hook=6,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_SpriteNodeData*", "sprite_ctx"),
    ],
    doc="Renders a scene node sprite/mesh quad from node transform/extents and Render_SpriteNodeData material/offset data, emits polygon batch records, and links them into render buckets. Return register is residual/not semantically validated.",
)

stable.fn(
    "Bone_ProcessExternalRef",
    "55 8B EC 83 EC 68 A1 ?? ?? ?? ?? 8B 0D ??",
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_SpriteNodeData*", "sprite_ctx"),
    ],
    doc="Builds/enqueues billboard-like sprite polygon batch records for a scene node using Render_SpriteNodeData extents/material; handles material external-ref chaining and vertex colors.",
)

stable.fn(
    "Render_IsPolygonInDebugList",
    "A1 ?? ?? ?? ?? 53 56 57 85 C0 7E",
    ret="int32_t",
    params=[
        param(
            "Render_PolygonRenderRef*",
            "polygon_ref",
            doc="Polygon render-reference address compared against the debug polygon buckets.",
        )
    ],
    doc="Returns 1 when polygon_ref is present in any active debug polygon bucket; buckets are 0x18-byte Render_PolygonRenderRef entries.",
)

stable.fn(
    "Render_PolygonBatch",
    "53 56 57 8B 58 70 A1 ?? ?? ?? ?? 89 5D CC",
    match=-12,
    hook=9,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Builds/enqueues Render_PolygonBatchRecord entries into the global polygon batch buffer from 0x18-byte polygon render references; callers ignore the residual return register.",
)

stable.fn(
    "Render_SceneGeometry_Wrapper",
    "54 24 04 50 51 52 E8 ??",
    match=-9,
    hook=8,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Thin wrapper that forwards scene polygon render references to Render_PolygonBatch.",
)

stable.fn(
    "Render_SceneGeometry",
    "55 8B EC 81 EC 88 00 00 00 F7 05 ??",
    required=Required.EN,
    hook=9,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Converts/culls 0x18-byte scene polygon render references into global Render_PolygonBatchRecord entries and returns the residual/updated batch count.",
)

stable.fn(
    "Render_SceneGeometry_Alt",
    "00 00 F6 C2 04 0F 84 ??",
    match=-127,
    hook=6,
    ret="int32_t",
    params=[
        param("Scene_Node*", "node"),
        param("Render_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
)

stable.fn(
    "Trail_RenderAnimated",
    "BF 77 1A 3B C6 0F 84 ??",
    match=-33,
    hook=6,
    ret="void",
    params=[param("Component_TrailObject*", "trail")],
    doc="Builds camera-facing animated trail polygon batches from Component_TrailObject/Trail_Segment data and links them into render buckets.",
)

stable.fn(
    "Bone_TransformWeightedVerts_ForRender",
    "66 83 7A 0A 00 0F 8F ??",
    match=-12,
    hook=6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param(
            "void*",
            "weighted_span",
            doc="Shared 0x14-byte weighted/submesh render span; callers pass submesh-like or sprite-node-data-like records.",
        ),
        param("int16_t*", "matrix"),
    ],
    doc=(
        "Transforms weighted/skinned vertices for render using Scene_Node+0x90 (PC EN) runtime vertex "
        "records. Return is residual and ignored by callers."
    ),
)

stable.fn(
    "Scene_FinalizeNodeRender",
    "53 56 57 A8 80 0F 84 ??",
    match=-11,
    hook=6,
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc="Finalizes a rendered scene node, using node render/material fields and invoking Render_SpritePolygons(node, transformedVerts, projectedVerts).",
)

stable.fn(
    "Render_SpritePolygons",
    "8D 04 90 84 C9 0F 84 ??",
    match=-71,
    hook=6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Render_SpriteVertexData*", "transformed_vertices"),
        param("Render_ProjectedVertex*", "projected_vertices"),
    ],
    doc="Projects and enqueues sprite polygon references from a scene node using transformed-vertex and projected-vertex scratch buffers. Callers ignore the residual return register.",
)

stable.fn(
    "Mesh_CalculateVertexNormals",
    "55 8B EC 83 EC ?? 8B 45 08 53 56 57 8D 70 ??",
    hook=6,
    ret="void",
    params=[
        param("Render_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "polygon_count"),
        param("Render_SpriteVertexData*", "transformed_vertices"),
        param("int32_t", "vertex_count"),
    ],
    doc=(
        "When graphics bit 0x1000 is active, accumulates per-vertex normals for 0x18-byte render "
        "polygon refs into the Mesh_VertexNormal scratch array at pcdogs.exe+0x94848 (PC EN). Quads "
        "average two triangle normals, triangles touch three vertices, and the only observed caller "
        "ignores residual return state."
    ),
)

stable.fn(
    "Math_Distance3D_FP12",
    "8B 4C 24 04 85 C9 7D ?? F7 D9 8B 44 24 08 85 C0",
    hook=6,
    ret="int32_t",
    params=[
        param("int32_t", "dx"),
        param("int32_t", "dy"),
        param("int32_t", "dz"),
    ],
    doc="Returns a fast weighted approximation of 3D distance from absolute dx/dy/dz components.",
)

stable.fn(
    "Math_Sqrt_FP12",
    "56 8B 74 24 08 57 33 C9 33 C0 BF 16 ?? ?? ??",
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "value")],
    doc="Integer/fixed-point square-root helper used by geometry normalization paths.",
)

stable.fn(
    "Math_IntegerSquareRoot",
    "56 8B 74 24 08 57 33 C9 33 C0 BF 17 ?? ?? ??",
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "value")],
    doc="Integer square-root helper.",
)

stable.fn(
    "Physics_CalculateMovementWithCollision",
    "00 8B 08 89 94 24 E8 ??",
    match=-174,
    hook=10,
    ret="int32_t",
    params=[
        param("int32_t*", "from_pos"),
        param("int32_t*", "to_pos"),
        param("void*", "basis_or_actor"),
        param("int16_t", "collision_radius"),
        param("int32_t", "collision_mode"),
    ],
    doc=(
        "Builds a stack collision query from fromPos/toPos, optional matrix/basis fields at +0x2C (PC "
        "EN), and radius thresholds, calls Collision_DetectActorCollisions, and returns 0x1000 minus "
        "the clipped travel fraction."
    ),
)

stable.fn(
    "Model_UpdateBoundingSphere",
    "55 8B EC 83 EC 64 53 8B 5D 08 56 57 8B 83 E8 ??",
    hook=6,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Math_TransformBarycentricToWorldCoords",
    "41 20 55 56 8B B1 E8 ??",
    match=-13,
    hook=7,
    ret="int32_t*",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t*", "out_world_pos"),
    ],
    doc=(
        "Transforms actor ground-contact barycentric state into world coordinates, writes "
        "outWorldPos[0..2], and returns outWorldPos. actor+0xE8 (PC EN) ground_collision_node "
        "and actor+0xEC (PC EN) ground_contact_ptr form paired native collision context."
    ),
)

stable.fn(
    "Model_TransformByCollisionNode",
    "55 8B EC 83 EC 30 56 8B 75 08 85 F6 0F 84 ??",
    hook=6,
    ret="void",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("int32_t*", "world_pos"),
        param("int32_t*", "out_world_pos"),
    ],
    doc="Transforms worldPos into collision-node local space, solves local Y over the polygon or selected triangle, then writes outWorldPos back in world coordinates.",
)

stable.fn(
    "Math_CalculateTriangleHeight",
    "2C 66 8B 42 02 D8 0D ?? ?? ?? ??",
    match=-105,
    hook=7,
    ret="int32_t",
    params=[
        param("int32_t", "local_x_fp12"),
        param("int32_t", "local_z_fp12"),
        param("Collision_Vertex*", "tri_a"),
        param("Collision_Vertex*", "tri_b"),
        param("Collision_Vertex*", "tri_c"),
    ],
    doc="Solves the triangle-plane local Y value for fixed-point local X/Z. Falls back to the highest vertex Y when the projected triangle area is zero.",
)

stable.fn(
    "Collision_GetAdjacentPolygon",
    "74 ?? 08 8B C8 C1 E9 ??",
    match=-26,
    hook=8,
    ret="Collision_Polygon*",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("int32_t*", "edge_index_in_out"),
    ],
    doc=(
        "Decodes polygon edge adjacency from adj_face_ptr +0x0C (PC EN) + edge*2, where a zero "
        "adjacency clears edgeIndexInOut to -1 and returns NULL. Nonzero adjacency returns the "
        "neighboring polygon only when its flags include bit 0x4."
    ),
)

stable.fn(
    "Collision_FindGroundPolygonUnderActor",
    "83 EC 2C 53 8B 5C 24 34 55 56 8B B3 E8 ??",
    hook=8,
    ret="Collision_Node*",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon* *", "out_polygon"),
    ],
    doc=(
        "Finds or walks to the ground/contact polygon under actor and returns "
        "actor->ground_collision_node; the paired ground_contact_ptr context stays opaque."
    ),
)

stable.fn(
    "Collision_IsPointInsidePolygon",
    "88 00 00 00 22 0F 85 ??",
    match=-90,
    ret="int32_t",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("int16_t", "point_x"),
        param("int16_t", "point_z"),
    ],
    doc="Odd/even point-in-polygon test in X/Z space. Uses transformed vertex coordinates when collisionNode flags 0x22 are set.",
)

stable.fn(
    "Collision_FindIntersectingPolygonEdge",
    "F8 0C 83 F8 FC 0F 8E ??",
    match=-56,
    hook=7,
    ret="int32_t",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("int16_t*", "from_point"),
        param("int16_t*", "to_point"),
    ],
    doc="Returns the crossed polygon edge index in range 0..3, or -1 for no eligible edge crossing.",
)

stable.fn(
    "Model_FindCollisionTarget",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 83",
    hook=8,
    ret="int16_t*",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon* *", "out_polygon"),
    ],
    doc=(
        "Thin wrapper around Collision_FindGroundPolygonUnderActor for actor/out_polygon. "
        "This forwards directly to the ground-contact helper while Actor_State.ground_contact_ptr stays opaque."
    ),
)

stable.fn(
    "Model_ResolveCollision",
    "55 8B EC 83 EC 18 56 57 8B 7D 08 8B B7 E8 ??",
    hook=6,
    ret="void*",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Resolves actor collision against collision nodes, polygon arrays, and paired "
        "ground/contact state. Native writes update actor+0xE8 (PC EN) and actor+0xEC (PC EN) together, "
        "or clear both when contact ends; ground_contact_ptr stays opaque."
    ),
)

stable.fn(
    "Collision_DetectDynamicObject",
    "55 8B EC 83 EC 18 A1 ??",
    hook=6,
    ret="int32_t*",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose position and current ground-contact state are tested against dynamic collision objects.",
        ),
        param(
            "int32_t*",
            "in_out_ground_y",
            doc="Current best ground/contact Y value; read for comparison and updated when a dynamic object polygon is selected.",
        ),
    ],
    doc="Search enabled dynamic collision objects for actor ground contact and update the actor ground object/polygon state.",
)

stable.fn(
    "SaveGame_GetSlotIndex", "A1 ?? ?? ?? ?? 25 FF FF 00", ret="uint32_t", params=[]
)

stable.fn(
    "SaveGame_SetSlotIndex",
    "66 8B 44 24 04 66 A3 ?? ?? ?? ?? C3 90 90 90 90 33",
    ret="int32_t",
    params=[param("int16_t", "slot_index")],
    doc="Stores the current save slot index in the save-state globals and returns the stored value.",
)

stable.fn(
    "SaveGame_GetCurrentLevel",
    "33 C0 66 A1 ?? ??",
    hook=8,
    ret="int32_t",
    params=[],
    doc="Read the current saved level index from the game save-state globals.",
)

stable.fn(
    "SaveGame_SetCurrentLevel",
    "66 8B 44 24 04 66 A3 ?? ?? ?? ?? C3 90 90 90 90 E8",
    ret="int32_t",
    params=[
        param("int16_t", "level_index", doc="Level_State index to store in save state.")
    ],
    doc="Store the current level index in the game save-state globals.",
)

stable.fn(
    "GameState_IsSoundEnabled",
    "E8 ?? ?? ?? ?? F7 D8 1B C0 40 C3",
    ret="int32_t",
    params=[],
    doc="Checks whether sound effects are enabled.",
)

stable.fn(
    "GameState_SetSoundEnabled",
    "51 A2 ?? ?? ?? ?? E8 ??",
    match=-11,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "enabled", doc="Non-zero to enable game sound effects.")],
    doc="Update the game sound-effects enabled flag.",
)

stable.fn(
    "Player_SetLives",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ??",
    required=Required.EN,
    hook=9,
    ret="int32_t",
    params=[
        param(
            "char",
            "lives",
            doc="Player life-count byte to store in the player-lives global.",
        )
    ],
    doc="Store the player lives byte and return the stored value.",
)

stable.fn(
    "SaveGame_SetGameComplete",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ?? ?? ?? ?? 8D 0C 85 04 00 00 00 2B D1 8B 02 C3",
    hook=9,
    ret="int32_t",
    params=[param("char", "complete_flag")],
    doc="Stores the game-complete flag in the save-state globals and returns the stored value.",
)

stable.fn(
    "SaveGame_IsGameComplete",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ??",
    required=Required.EN,
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Level_GetDataPointer",
    "8B 44 24 04 BA ??",
    hook=9,
    ret="int32_t",
    params=[param("int32_t", "data_index")],
)

stable.fn(
    "Level_NormalizeIndex",
    "8B 44 24 04 C3 90 90 90 90 90 90 90 90 90 90 90 8B 44 24 04 56 50 E8 ??",
    hook=hook(4, kind=HookKind.HOTPATCH, entry_patch_size=2),
    ret="int32_t",
    params=[param("int32_t", "level_id")],
)

stable.fn(
    "Level_SetMenuProgressState",
    "8B 44 24 04 56 50 E8 ?? ?? ?? ?? 8B F0 56 E8 ?? ?? ?? ?? 83 C4 08 83 FE 07 7C ?? 83 FE 1B 7D ?? 83 FE 0B 74 ?? 83 FE 10 74 ?? 83 FE 15 74 ?? 83 FE 1A 74 ?? 0F BE 88 ?? ?? ?? ?? 0F BE 80 ?? ?? ?? ?? 33 D2",
    ret="int32_t",
    params=[param("int32_t", "level_id")],
    doc="Normalizes the requested level id and updates the menu progress state global.",
)

stable.fn(
    "Level_GetArrayIndex",
    "83 F9 1A 74 ?? 83 E9 ??",
    match=-29,
    hook=7,
    ret="int32_t",
    params=[param("int32_t", "level_id")],
)

stable.fn(
    "Level_InitializeBonusData",
    "24 8D ?? ?? ?? ?? B9 ??",
    match=-40,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "level_id"), param("int32_t*", "array_index")],
)

stable.fn(
    "SaveGame_SaveLevelCompletion",
    "0F BF 05 ?? ?? ?? ?? 53 56 50 E8 ??",
    required=Required.EN,
    hook=7,
    ret="void",
    params=[param("char", "include_current_puppy")],
    doc=(
        "Commit the current level's completion, puppy, bone, and bonus-item progress into the "
        "save-state globals. A zero includeCurrentPuppy value stores one less than the current level "
        "puppy count; non-zero stores the full count."
    ),
)

stable.fn(
    "SaveGame_SaveBonusProgress",
    "A3 ?? ?? ?? ?? 89 0D ?? ?? ?? ?? C3",
    match=-45,
    ret="int32_t",
    params=[],
)

stable.fn(
    "SaveGame_BackupPuppyCount",
    "A0 ?? ?? ?? ?? A2 ?? ?? ?? ?? C3 90 90 90 90 90",
    ret="uint8_t",
    params=[],
    doc="Copies the active SaveGame_Slot+2 puppy_count_backup byte into the backup_puppy_count global and returns it.",
)

stable.fn(
    "SaveGame_SetBackupPuppyCount",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BF 0D ?? ?? ?? ?? A0 ?? ?? ?? ??",
    ret="int32_t",
    params=[param("uint8_t", "puppy_count")],
    doc="Stores puppyCount in the active SaveGame_Slot+2 puppy_count_backup byte and returns the stored value.",
)

stable.fn(
    "Level_InitializeSaveState",
    "?? ?? 8B F0 52 4E E8 ??",
    match=-51,
    hook=7,
    ret="void",
    params=[],
    doc="Initializes level save/progress state and returns helper status.",
)

stable.fn(
    "Level_BuildCompletionTable",
    "33 F6 83 F8 04 0F 8D ??",
    match=-44,
    ret="int32_t",
    params=[],
)

stable.fn(
    "SaveGame_InitializeNewGame",
    "68 00 10 00 00 C6 05 ?? ?? ?? ?? 01 E8 ??",
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "SaveGame_LoadState",
    "8B 44 24 04 85 C0 75 ?? 68 DC 01 00 00 68 ??",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "operation_step",
            doc="Zero starts the async save-file read; nonzero finalizes the completed read and restores or initializes state.",
        )
    ],
    doc="Starts or finalizes loading the 0x1dc-byte save file into save_file_buffer. Returns 0 while the async file op is pending or invalid, 1 when the save header is empty/incompatible and new-game state is used, and 2 after restoring game_state, game_settings, and player_lives from the file header.",
)

stable.fn(
    "SaveGame_GetHighestWorld",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 57 B9 17 00 00 00 33 C0 BF ??",
    required=Required.EN,
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "SaveGame_InitializeState",
    "57 B9 17 00 00 00 33 C0 BF ??",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "SaveGame_ClearBonusProgressData",
    "A3 ?? ?? ?? ?? A3 ?? ?? ?? ?? C3",
    match=-17,
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Shared_LoadCommonResources",
    "A1 ?? ?? ?? ?? 85 C0 0F 85 ?? ?? ?? ?? 6A 00",
    required=Required.EN,
    ret="void*",
    params=[],
)

stable.fn(
    "Menu_ShutdownResources",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? A1 ?? ?? ?? ?? 6A 00 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ??",
    ret="BOOL",
    params=[],
)

stable.fn(
    "Menu_ClearTransitionFlags",
    "B8 42 00 00 00 33 C9 66 39 05 ??",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_UpdatePauseMenu",
    "A1 ?? ?? ?? ?? 66 8B 0D",
    ret="BOOL",
    params=[],
    doc="Updates in-level pause/save menu state and returns a scalar handled/continue status.",
)

stable.fn(
    "UI_UpdateBoneCounter",
    "?? ?? ?? 53 56 57 8B 59",
    match=-19,
    hook=6,
    ret="void",
    params=[param("int32_t", "target_bone_count")],
    doc="Animate the HUD bone counter toward targetBoneCount and render the counter at the left HUD anchor.",
)

stable.fn(
    "UI_RenderCenteredNumber",
    "8B 0D ?? ?? ?? ?? 83 EC 10",
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "packed_center_xy"), param("int32_t", "number")],
    doc="Render a decimal HUD number centered on the packed x/y anchor and return the final sprite render result.",
)

stable.fn(
    "UI_UpdatePuppyCounter",
    "?? ?? ?? 53 55 56 8B 59",
    match=-19,
    hook=6,
    ret="void",
    params=[param("int32_t", "target_puppy_count")],
    doc="Animate the HUD puppy counter toward targetPuppyCount and render the counter at the right HUD anchor.",
)

stable.fn(
    "UI_UpdateLives",
    "83 EC 24 66 83 3D ??",
    hook=11,
    ret="int32_t",
    params=[param("int32_t", "icon_count"), param("int32_t", "lives")],
    doc="Animate up to four life icons toward iconCount, render the icons and life number, and return the icon x anchor.",
)

stable.fn(
    "Menu_AnimateSlots",
    "83 EC 0C 8B 0D ?? ?? ?? ?? 53 55",
    hook=9,
    ret="int32_t*",
    params=[],
)

stable.fn(
    "Menu_RenderConfirmPrompt",
    "?? A9 00 00 08 00 75 ?? 25 00",
    match=-17,
    ret="int32_t",
    params=[param("int32_t", "prompt_id")],
)

stable.fn(
    "Menu_GetPlayerLevelInfo",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 4C",
    ret="void",
    params=[param("Menu_LevelProgressInfo*", "out_info")],
    doc="Fill outInfo with current level puppy/bone totals and the active player's bone/life counters for menu display.",
)

stable.fn(
    "Menu_IsInGame",
    "A0 ?? ?? ?? ?? 3C 0B 74 07 3C 0C 74 03 33 C0 C3 B8 01 00 00 00 C3",
    required=Required.EN,
    ret="BOOL",
    params=[],
    doc="EN-only standalone helper used by Menu_UpdatePauseMenu; returns true when menu_state is 11 or 12, the active in-level states used by pause/save UI paths. EU/SC use a different inlined/nearby state check and omit this exact helper body.",
)

stable.fn(
    "Player_InitializeState",
    "80 28 01 00 00 50 E8 ??",
    match=-44,
    hook=8,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Script/player controller actor; actor->linked_actor receives refreshed completion flags.",
        ),
        param(
            "Pkg_ActorRecord*",
            "record",
            doc="Player actor record whose puppy/count fields are initialized from the backup puppy-count global.",
        ),
    ],
    doc="Initialize the player record's saved puppy/count fields and refresh completion flags on actor->linked_actor.",
)

stable.fn(
    "Player_SetCompletionFlags",
    "00 00 FF FF FF 3F E8 ??",
    match=-9,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Player actor whose behavior_flags completion-state bits are refreshed.",
        )
    ],
    doc="Clear actor completion-state bits, then set the game-complete or in-progress flag from SaveGame_IsGameComplete.",
)

stable.fn(
    "Player_ResetBoneCount",
    "8B 44 24 08 C7 40 74 04 00 00 00 C3 90 90 90 90 56 8B 74 24 08 83 FE 03 7F ?? A1 ??",
    hook=11,
    ret="void*",
    params=[
        param(
            "Entity_State*",
            "current_entity",
            doc="Unused ABI slot; the respawn caller passes the current entity before the record.",
        ),
        param(
            "Pkg_ActorRecord*",
            "record",
            doc="Player record whose +0x74 (PC EN) counter is reset to 4.",
        ),
    ],
    doc="Resets the player record+0x74 (PC EN) counter to 4 and returns the record pointer.",
)

stable.fn(
    "Player_CollectPowerup",
    "56 8B 74 24 08 83 FE 03 7F ?? A1 ??",
    ret="Actor_State*",
    params=[
        param(
            "int32_t",
            "powerup_type",
            doc="Powerup kind byte from the collected powerup actor record, promoted to int32_t by the caller.",
        )
    ],
    doc="Apply collection side effects for the supplied powerup type, including counter updates and puppy backup-count updates.",
)

stable.fn(
    "Level_CheckBonusUnlock",
    "00 C6 44 24 31 00 E8 ??",
    match=-101,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "int32_t*",
            "player_position",
            doc="Pointer to the player actor position_x/position_y/position_z vector used for the bonus-unlock trace.",
        )
    ],
    doc="When the active checker slot changes, trace from playerPosition and latch the bonus-unlock/menu-reset globals on hit.",
)

stable.fn(
    "Level_ResetBonusState",
    "FA B5 49 00 C3 C7 05 ??",
    match=-26,
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_ResetState",
    "FF FF 83 C8 FF 66 A3 ??",
    match=-15,
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Level_CalculateCompletionPercent",
    "00 00 00 D3 E5 85 E8 ??",
    match=-54,
    ret="int32_t",
    params=[param("char*", "save_data")],
)

stable.fn(
    "Menu_RenderSaveGame",
    "44 24 20 1C 00 0F 84 ??",
    match=-82,
    required=Required.EN,
    hook=8,
    ret="int32_t",
    params=[
        param("int32_t", "slot_index"),
        param("int32_t", "y"),
        param(
            "int32_t",
            "allow_save",
            doc="Save/load mode flag; slot data is read from the global save_game_buffer, not from this parameter.",
        ),
    ],
    doc=(
        "Render the save/load slot list from the global save-file buffer. The third "
        "argument is a mode flag, while the native return is render-helper status "
        "ignored by known callers."
    ),
)

stable.fn(
    "Menu_RenderSimpleOptionsA",
    "E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 83 C4 04 50 8D 04 CD ?? ?? ?? ?? 2B C1 8D 04 80 C1 E0 02 99 F7 F9 8D",
    match=-166,
    required=Required.EN,
    ret="void",
    params=[
        param("int32_t", "title_string_id"),
        param("int32_t", "selected_index"),
        param("int32_t", "highlight_color"),
    ],
    doc="Render a lower-screen two-choice menu title plus Yes/No rows, tinting selectedIndex with highlightColor.",
)

stable.fn(
    "Menu_UpdateInput",
    "8A 0D ?? ?? ?? ?? 33 C0 A3 ??",
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
    doc=(
        "Updates menu one-shot pulse dwords at pcdogs.exe+0x9BAA4 (PC EN)..pcdogs.exe+0x9BAB8 (PC EN) "
        "and held/debounce bytes at pcdogs.exe+0x9BABC (PC EN)..pcdogs.exe+0x9BAC1 (PC EN) from "
        "player-1 input and keyboard state."
    ),
)

stable.fn(
    "Menu_HandleSaveGameLogic",
    "?? ?? 84 C0 74 27 E8 ??",
    match=-16,
    ret="void",
    params=[
        param("char*", "out_result"),
        param("int32_t", "selected_slot"),
        param("int32_t", "allow_save"),
    ],
    doc=(
        "Advance the save/load menu state machine, update outResult[0..2], and "
        "start or poll save-game file operations over the 0x1dc save-file span. "
        "Returns the selected level id through the public result."
    ),
)

stable.fn(
    "Menu_RenderButtonPrompt",
    "53 8B 5C 24 0C 55 8B 6C 24 0C 56 57 8D 7D ?? 33 D2 89 7C 24 ?? 8B 37",
    ret="int32_t",
    params=[
        param("int32_t*", "mapping_table"),
        param("uint8_t*", "out_prompt_a"),
        param("uint8_t*", "out_prompt_b"),
    ],
    doc=(
        "Compares configured button-mapping rows and fills two 2-byte prompt "
        "descriptors, or writes 0xff markers when no pairing is found; the return "
        "is matched-index/-1 helper result."
    ),
)

stable.fn(
    "SaveGame_SaveToSlot",
    "F3 A5 6A ??",
    match=-86,
    hook=11,
    ret="BOOL",
    params=[param("int32_t", "slot_index")],
    doc=(
        "Copies the active save-state and collectible values into one save slot, "
        "marks it valid, then starts save operation 9 over the 0x1dc file span. "
        "Known menu callers ignore the BOOL/native return metadata from the "
        "operation initializer."
    ),
)

stable.fn(
    "Menu_HandleOptionsLogic",
    "A1 ?? ?? ?? ?? ?? 56 48 57 C6 05 ?? ?? ?? ?? FF 0F 84",
    ret="int32_t",
    params=[],
    doc="Advance the controls-configuration submenu state and return non-zero when the options menu is exiting.",
)

stable.fn(
    "GameState_BackupSettings",
    "89 15 ?? ?? ?? ?? 33 C0 C3",
    match=-28,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_ProcessOptionsInput",
    "88 4E 02 3C 03 0F 84 ??",
    match=-11,
    hook=6,
    ret="void",
    params=[
        param(
            "char*",
            "out_result",
            doc="Three-byte menu result buffer updated as out_result[0..2].",
        ),
        param("int32_t", "selected_row", doc="Options-menu row being processed."),
    ],
    doc=(
        "Process options-menu input for selected_row and update out_result[0..2]. "
        ""
        "the public SDK contract stays void."
    ),
)

stable.fn(
    "Menu_RenderControlsConfiguration",
    "83 EC 08 53 55 56 57 6A 00 6A 00 6A 76 E8 ??",
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_RenderMusicSelection",
    "04 8B E8 ?? ?? ?? ?? 00",
    match=-16,
    hook=9,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_RenderDifficultySelection",
    "04 8B E8 ?? ?? ?? ?? 68",
    match=-16,
    required=Required.EN,
    hook=9,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_RenderOptionsMenu",
    "83 EC 64 A1 ?? ?? ?? ?? 56",
    hook=8,
    ret="int32_t",
    params=[param("int32_t", "menu_id"), param("int32_t", "selected_index")],
)

stable.fn(
    "Menu_ProcessMenuState",
    "83 EC 08 53 55 56 57 E8 ??",
    ret="BOOL",
    params=[],
    doc="Processes the active menu state and returns nonzero when menu handling consumes/skips the normal frame path.",
)

stable.fn(
    "Menu_HandleSelection",
    "8B 15 ?? ?? ?? ?? 33 C0 A2",
    hook=6,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "target_menu",
            doc="Menu/state id or sentinel: -1 skips transition, 1 resumes music, and 3 restores the stored fade target.",
        )
    ],
    doc="Clears transient menu/input state, handles special audio-resume targets, or transitions to targetMenu.",
)

stable.fn(
    "Menu_CheckPauseInput",
    "A0 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 66 A1",
    required=Required.EN,
    ret="Actor_State*",
    params=[param("char", "allow_pause")],
)

stable.fn(
    "Menu_RenderSimpleOptionsB",
    "00 01 50 E8 ?? ?? ?? ?? 83",
    match=-13,
    ret="void",
    params=[
        param("int32_t", "title_string_id"),
        param("int32_t", "selected_index"),
        param("int32_t", "highlight_color"),
    ],
    doc="Render a centered two-choice menu title plus Yes/No rows, tinting selectedIndex with highlightColor.",
)

stable.fn(
    "Menu_ProcessNameEntryInput",
    "A0 ?? ?? ?? ?? 53 56 3C 01 57 0F 85 ??",
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Menu_RenderNameEntry",
    "01 53 55 56 57 0F 85 ??",
    match=-9,
    ret="int32_t",
    params=[param("int32_t", "cursor_pos")],
)

stable.fn(
    "Color_InitializeGradient",
    "88 06 88 4E 03 C1 E8 ??",
    match=-18,
    hook=8,
    ret="char",
    params=[
        param("char*", "gradient_state"),
        param("int32_t", "start_color"),
        param("int32_t", "end_color"),
        param("int32_t", "steps"),
    ],
)

stable.fn(
    "Color_ComputeGradient",
    "8B 4C 24 0C F7 C1 00 00 00 80 0F 84 ??",
    hook=10,
    ret="int32_t*",
    params=[
        param("int32_t*", "start_color"),
        param("int32_t*", "end_color"),
        param("int32_t", "t"),
    ],
)

stable.fn(
    "UI_RenderButtonPrompts",
    "C3 06 74 ?? 40 8B 35 ??",
    match=-21,
    required=Required.EN,
    ret="void",
    params=[param("uint32_t", "button_prompt_flags")],
    doc="Render bottom-center accept/cancel button prompts selected by buttonPromptFlags bits.",
)

stable.fn(
    "Menu_RenderFormattedText",
    "D8 53 E8 ?? ?? ?? ?? 8B",
    match=-21,
    hook=10,
    ret="int32_t",
    params=[
        param("int32_t", "x"),
        param("int32_t", "y"),
        param("int32_t", "string_id"),
    ],
)

stable.fn(
    "Text_ComputeStringWidth",
    "8B 0D ?? ?? ?? ?? 56 8B 41",
    hook=6,
    ret="int32_t",
    params=[param("char*", "text")],
)

stable.fn(
    "Resource_InitializeGameEngine",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? 32",
    ret="int32_t",
    params=[],
    doc=(
        "Initialize the core memory/resource and DirectDraw-backed game engine subsystems; returns nonzero on "
        "initialization success even though native WinMain ignores the status."
    ),
)

stable.fn(
    "Resource_ShutdownGameSubsystems",
    "E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 90 90 90 90 90 51",
    ret="int32_t",
    params=[],
    doc="Shuts down core game resource subsystems by releasing the DirectDraw device and tail-calling the memory extent leak/free sweep.",
)

stable.fn(
    "Font_RenderText",
    "51 6A 00 C7 05 ??",
    hook=13,
    ret="int32_t",
    params=[param("char*", "text"), param("int32_t", "color")],
)

stable.fn(
    "Text_RenderStringWithFormatting",
    "?? ?? ?? C6 44 24 10 01",
    match=-30,
    required=Required.EN,
    hook=12,
    ret="int32_t",
    params=[param("int32_t", "x"), param("char*", "text"), param("int32_t", "format")],
)

stable.fn("Render_DrawSortedLists", "A1 ?? ?? ?? ?? 05 CC", ret="int32_t", params=[])

stable.fn(
    "Render_BeginRendering",
    "E8 ?? ?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 8B 0D",
    ret="int32_t",
    params=[param("int32_t", "clear_flags")],
)

stable.fn(
    "Render_ClearScreenAndRenderRectangle",
    "E8 ?? ?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 8B 44",
    ret="int32_t",
    params=[
        param("int32_t", "x"),
        param("int32_t", "y"),
        param("int32_t", "width"),
        param("int32_t", "height"),
    ],
)

stable.fn(
    "Render_IncrementPassCounter",
    "E4 BA 49 00 75 05 E9 ??",
    match=-12,
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Render_EndRendering",
    "E4 BA 49 00 75 0E E8 ??",
    match=-12,
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Resource_FreeAndReturnNull",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 83 C4 04 32",
    ret="BOOL",
    params=[
        param(
            "void*",
            "mem_ptr",
            doc="Resource-memory data pointer forwarded to Resource_FreeMemory.",
        )
    ],
    doc="Frees memPtr through Resource_FreeMemory and returns FALSE.",
)

stable.fn(
    "Texture_InitializeBlendTextures",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 83 C4 04 C3",
    ret="Material_BlendTextureSet*",
    params=[param("void*", "resource_data")],
)

stable.fn(
    "Texture_FreeBlend",
    "56 8B 74 24 08 8B 06 50 E8 ??",
    ret="int32_t",
    params=[param("Material_BlendTextureSet*", "blend_textures")],
)

stable.fn(
    "Resource_CleanupGameState",
    "A1 ?? ?? ?? ?? 53 33 DB 3B C3 74",
    ret="BOOL",
    params=[],
    doc="Unloads the latched level_resource_handle when present, runs the title/menu cleanup branch, clears object/current-level state including current_level_data, and resets the current level id.",
)

stable.fn(
    "Menu_ProcessMenuTransition",
    "60 85 F6 75 ?? 57 E8 ??",
    match=-33,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "target_menu")],
)

stable.fn(
    "UI_Update",
    "A1 ?? ?? ?? ?? 8A ?? ?? ?? ?? ?? 53 85 C0 0F BE ?? 74 ?? C6 05 ?? ?? ?? ?? 05 BB",
    ret="void",
    params=[],
    doc="Advance title/loading UI state, fade delays, and related menu transition side effects.",
)

stable.fn(
    "Input_CheckCheatCodeSequence",
    "?? A3 ?? ?? ?? ?? 3B C1",
    match=-10,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Pkg_LoadRandomSplashScreen",
    "A0 ?? ?? ?? ?? 53 33",
    required=Required.EN,
    ret="BOOL",
    params=[
        param(
            "int32_t",
            "level_index",
            doc="Level index used to choose loading text and splash image state.",
        )
    ],
    doc="Advances the loading-screen state machine and returns nonzero when loading may continue.",
)

stable.fn(
    "DemoReplay_LoadBonusReplay",
    "56 8B 74 24 08 85 F6 0F 84 ??",
    ret="BOOL",
    params=[
        param(
            "uint8_t*",
            "bonus_replay_resource",
            doc="Base of the packed bonus-replay resource; null only queries whether the replay buffer already exists.",
        )
    ],
    doc="Allocates/updates the demo replay buffer, selects the next bonus replay, copies its 0xc80-byte payload, and returns whether replay data is available.",
)

stable.fn(
    "Level_Load",
    "0F BE 05 ?? ?? ?? ?? 83 F8 0A 0F 87 ??",
    required=Required.EN,
    hook=7,
    ret="int32_t",
    params=[],
    doc=(
        "Top-level level/menu loading state machine driven by level_menu_load_state. Dispatches "
        "states 0..10 through level_load_state_dispatch_table: title/common-resource setup, "
        "inter-level menu, world select, splash loading, and Game_TransitionToLevel; state 9 "
        "falls through to the idle/return path."
    ),
)

stable.fn(
    "Game_TransitionToLevel",
    "8B 54 24 04 8D 42 F9 A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85 C0 75 ?? 52 E8 ?? ?? ?? ?? 83 C4 04 A3 ?? ?? ?? ?? 32 C0 C3 80 3D ?? ?? ?? ?? 03 75 ?? 32 C0 C3 57 B9 2C 00 00 00 33 C0 BF ?? ?? ?? ?? F3 AB",
    hook=7,
    ret="int32_t",
    params=[param("int32_t", "level_id")],
    doc="Begins or completes the transition into the requested level id, including menu/loading-state setup.",
)

stable.fn(
    "Level_UpdateWorldSelectMenu",
    "51 A0 ?? ?? ?? ??",
    required=Required.EN,
    cc=CallingConvention.FASTCALL,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "menu_context")],
)

stable.fn(
    "Level_UpdateInterLevelMenu",
    "A0 ?? ?? ?? ?? 83 EC 08 3C 03 53 56 0F 85 ??",
    required=Required.EN,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Material_BuildStructure",
    "8B 47 04 85 C0 0F 84 ??",
    match=-40,
    ret="int32_t",
    params=[
        param(
            "Material_SectionHeader*",
            "material_section",
            doc="Material section whose node/material tables are referenced while building the frame record.",
        ),
        param(
            "Material_RefEntry*",
            "ref_out",
            doc="Receives the material section pointer, selected 36-byte material table entry pointer, and frame-set pointer.",
        ),
        param(
            "Material_FrameSet*",
            "optional_output_table",
            doc="Optional output buffer for the 8-byte Material_FrameSet header followed by frame pointers and 0x10-byte frame records.",
        ),
    ],
    doc=(
        "Builds a 12-byte Material_RefEntry and optional Material_FrameSet for one material; frame "
        "records are zero-initialized except for the packed material-node reference at +0x0C (PC EN)."
    ),
)

stable.fn(
    "Material_BuildTextureArray",
    "C0 56 8B 4D 0C 8B 35 ??",
    match=-12,
    ret="int32_t",
    params=[
        param(
            "Material_Table*",
            "material_table",
            doc="Material table/base whose +0x0C (PC EN) material-entry offset addresses 36-byte material records.",
        ),
        param(
            "Material_RefEntry*",
            "material_refs",
            doc="First 12-byte material reference record to initialize.",
        ),
        param(
            "int32_t",
            "material_count",
            doc="Number of consecutive material reference records to build.",
        ),
        param(
            "int32_t",
            "first_material_index",
            doc="Starting source material index used to compute each 36-byte material record offset.",
        ),
    ],
    doc="Builds material reference/frame data for materialCount entries and advances material_buffer_offset.",
)

stable.fn(
    "Material_FindTextureByFrame",
    "8B 4C 24 04 53 56 8B 41 08 57 85 C0 0F 84 ??",
    ret="BOOL",
    params=[
        param(
            "Material_RefEntry*",
            "material_ref",
            doc="Material reference whose frame set is searched.",
        ),
        param("int32_t", "frame_index", doc="Requested animation/frame index."),
        param(
            "Render_SpriteContext* *",
            "out_sprite",
            doc="Receives the render sprite context; its texture descriptor is updated to the selected frame or frame-zero fallback.",
        ),
    ],
    doc="Finds the render texture for frameIndex in materialRef, falling back to frame zero when the requested frame is unavailable.",
)

stable.fn(
    "Matrix_BuildRotationY",
    "?? C1 F8 02 83 C4 08 8B",
    match=-29,
    hook=6,
    ret="Math_Matrix3x3i16*",
    params=[
        param(
            "Math_Matrix3x3i16*",
            "out_matrix",
            doc="Receives the Y-axis rotation matrix.",
        ),
        param(
            "Math_Matrix3x3i16*",
            "post_multiply",
            doc="Optional matrix multiplied after the Y rotation.",
        ),
        param(
            "int32_t",
            "angle_y",
            doc="Y angle in the game's 0x1000-turn sine-table domain.",
        ),
    ],
    doc="Builds a signed Q12 Y-axis 3x3 rotation matrix, optionally post-multiplied by post_multiply.",
)

stable.fn(
    "Matrix_BuildRotationXY",
    "7C ?? 2C 2B C7 50 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("Math_Matrix3x3i16*", "out_matrix"),
        param("Math_Matrix3x3i16*", "post_multiply"),
        param("int32_t", "angle_x"),
        param("int32_t", "angle_y"),
    ],
)

stable.fn(
    "Math_BuildRotationMatrix",
    "7C ?? 3C 2B C7 50 E8 ??",
    match=-11,
    hook=8,
    ret="int32_t",
    params=[
        param("Math_Matrix3x3i16*", "out_matrix"),
        param("Math_Matrix3x3i16*", "post_multiply"),
        param("int32_t", "angle_x"),
        param("int32_t", "angle_y"),
        param("int32_t", "angle_z"),
    ],
)

stable.fn(
    "Math_BuildRotationMatrixDirect",
    "8B 44 24 0C 8B 4C 24 10 66 A3 ??",
    hook=8,
    ret="int32_t",
    params=[
        param(
            "Math_Matrix3x3i16*",
            "out_matrix",
            doc="Receives the combined X/Y/Z rotation matrix.",
        ),
        param(
            "Math_Matrix3x3i16*",
            "post_multiply",
            doc="Optional matrix composed before the rotations.",
        ),
        param(
            "int32_t",
            "sin_x",
            doc="Precomputed X sine in signed Q12/Q14 table form; low word is stored into the X rotation template.",
        ),
        param(
            "int16_t",
            "cos_x",
            doc="Precomputed X cosine stored into the X rotation template.",
        ),
        param(
            "int32_t",
            "sin_y",
            doc="Precomputed Y sine in signed Q12/Q14 table form; low word is stored into the Y rotation template.",
        ),
        param(
            "int16_t",
            "cos_y",
            doc="Precomputed Y cosine stored into the Y rotation template.",
        ),
        param(
            "int32_t",
            "angle_z",
            doc="Z angle in the game's 0x1000-turn sine-table domain; zero skips the final Z multiply.",
        ),
    ],
    doc="Builds a signed Q12 3x3 rotation matrix from precomputed X/Y sine/cosine inputs and a Z angle.",
)

stable.fn(
    "Resource_AllocateMemory",
    "8B 44 24 04 85 C0 75 ?? C3 50 E8 ?? ?? ?? ?? 83 C4 04 C3",
    ret="void*",
    params=[param("int32_t", "size")],
    doc="Allocates a resource-memory block with the game resource header and returns the data pointer.",
)

stable.fn(
    "Resource_AllocateWithHeader",
    "8B 44 24 04 56 83 C0 04 50 E8 ??",
    ret="uint32_t*",
    params=[
        param(
            "int32_t",
            "size",
            doc="Requested resource data size, excluding the hidden leading handle dword.",
        )
    ],
    doc="Allocates size + 4 bytes, stores the allocation handle in the hidden leading dword, and returns the resource data pointer after that header; returns null if pointer lookup fails.",
)

stable.fn(
    "Resource_FreeMemory",
    "8B 44 24 04 85 C0 74 ?? 56 8B 70 FC 56 E8 ??",
    hook=6,
    ret="BOOL",
    params=[
        param(
            "void*",
            "mem_ptr",
            doc="Resource data pointer returned after the hidden handle dword.",
        )
    ],
    doc="Releases a resource data pointer allocated by Resource_AllocateMemory/Resource_AllocateWithHeader by reading the hidden handle at mem_ptr - 4.",
)

stable.fn(
    "Actor_TriggerCollisionCallback",
    "50 04 8B 00 52 50 E8 ??",
    match=-16,
    hook=8,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("Actor_State*", "other_actor")],
    doc=(
        "Triggers the component/actor collision callback path for comp against other_actor. "
        "Uses engine-owned callback globals and actor/component state; direct callback-slot "
        "writes belong in hook/patch flows, not generated data Write()."
    ),
)

stable.fn(
    "Actor_CheckCollisionType",
    "8B 54 24 04 8B 44 24 0C 3B D0 0F 84 ??",
    hook=8,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "other_actor"),
    ],
    doc="Checks whether actor/component collision metadata can collide with otherActor.",
)

stable.fn(
    "Actor_CheckAnimationTrigger",
    "74 ?? 8B D8 48 83 E9 ??",
    match=-64,
    hook=6,
    ret="BOOL",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "trigger_actor")],
    doc="Returns whether triggerActor satisfies actor's animation/collision trigger conditions.",
)

stable.fn(
    "Component_CalculateFrameDuration",
    "00 66 8B 42 44 D1 E8 ??",
    match=-40,
    hook=8,
    ret="int32_t",
    params=[param("Component_Instance*", "comp")],
)

stable.fn(
    "Component_UpdateTimers",
    "7E ?? 00 75 ?? 50 E8 ??",
    match=-50,
    ret="BOOL",
    params=[
        param(
            "int32_t*",
            "timer_slots",
            doc="Array of 36-byte component timer/collision slots; each slot starts with a Component_Definition pointer.",
        ),
        param("int32_t", "slot_count", doc="Number of timer slots to update."),
    ],
    doc="Updates component slot cooldowns and next timer deadlines, using Component_CalculateFrameDuration for active definitions.",
)

stable.fn(
    "Component_UpdateCollisionDetection",
    "8B 45 18 85 C0 0F 84 ??",
    match=-24,
    hook=6,
    ret="int32_t*",
    params=[
        param(
            "int32_t*",
            "collision_slots",
            doc="Array of 36-byte component collision slots; negative slotCount processes the current slot only.",
        ),
        param(
            "int32_t",
            "slot_count",
            doc="Number of collision slots to scan; negative forces a single-slot probe path.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose component collision slots are evaluated.",
        ),
        param(
            "Actor_State*",
            "other_actor",
            doc="Optional target actor; when present, its world position at +0x98 (PC EN) is used as the probe position.",
        ),
        param(
            "int32_t*",
            "probe_position",
            doc="Optional xyz world-position vector used when otherActor is null.",
        ),
    ],
    doc="Tests component collision slots against another actor or probe position and records hit position, distance, and target actor state in matching slots.",
)

stable.fn(
    "Component_GetSpeedRange",
    "8B 44 24 04 33 D2 8B 88 F4 00 00 00 8B 49 04 8B 41 1C 66 8B 51 18 C1 E0 10 0B C2 C3 90 90 90 90 8B 44 24 04 8B 88 F4 00 00 00 8B 51 04 8B 02 C1 E8 ??",
    hook=6,
    ret="int32_t",
    params=[param("Component_Instance*", "comp")],
)

stable.fn(
    "Component_IsAirborneTarget",
    "8B 51 04 8B 02 C1 E8 ??",
    match=-10,
    hook=10,
    ret="BOOL",
    params=[param("Component_Instance*", "comp")],
)

stable.fn(
    "Component_SpawnFromDefinition",
    "0C 89 44 24 14 0F 84 ??",
    match=-45,
    hook=7,
    ret="Component_Instance*",
    params=[
        param("Actor_State*", "owner_actor"),
        param("Actor_State*", "target_actor"),
        param("Component_Definition*", "component_def"),
    ],
)

stable.fn(
    "Component_CalculateOrientation",
    "8B 07 F6 C4 80 0F 84 ??",
    match=-23,
    hook=6,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("int32_t*", "orientation_vec")],
)

stable.fn(
    "Component_TrackTarget",
    "2B CA 51 89 4D F8 E8 ??",
    match=-101,
    hook=6,
    ret="void",
    params=[
        param("Component_Instance*", "comp"),
        param("int32_t*", "target_pos"),
        param("int16_t", "turn_rate"),
    ],
)

stable.fn(
    "Component_PlayPositionalSound",
    "?? 83 C1 40 51 50 E8 ??",
    match=-23,
    hook=6,
    ret="void",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "actor"),
        param("int32_t", "sound_slot"),
    ],
    doc="Plays a positional sound from a component definition at the actor's current position.",
)

stable.fn(
    "Component_InitializeProjectile",
    "03 66 8B 57 44 52 E8 ??",
    match=-79,
    hook=6,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("int32_t*", "spawn_vec")],
    doc=(
        "Initializes Component_Instance projectile runtime fields including projectile_state, "
        "projectile_timer, and homing velocity slots before calling Component_CalculateOrientation. "
        "Projectile tail layout stays opaque."
    ),
)

stable.fn(
    "Component_CreateActor",
    "53 56 57 8B 7C 24 1C 83 FF 02 75 ?? A1 ??",
    hook=7,
    ret="Actor_State*",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "spawn_source"),
        param("Actor_State*", "owner_actor"),
        param("int32_t", "actor_kind"),
    ],
)

stable.fn(
    "Collision_InitializeFunctionPointers",
    "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C3 90",
    required=Required.EN,
    hook=10,
    ret="void",
    params=[],
    doc=(
        "Initializes projectile/collision function pointer globals: projectile_logic_func, "
        "collision_response_actor_func, and behavior_process_projectile_func; return register is setup status."
    ),
)

stable.fn(
    "Collision_ProcessProjectileHit",
    "4C 24 28 55 50 51 E8 ??",
    match=-21,
    ret="int32_t",
    params=[
        param("Component_Instance*", "comp"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    doc=(
        "Component/projectile collision-response callback target. It consumes the contextual "
        "Collision_Polygon and collision_depth from the collision query, handles sentinel "
        "collisionDepth values -2/-1, updates projectile runtime state, records a six-entry "
        "hit cache in spawn-context storage, and dispatches hit behavior; projectile tail "
        "layout stays opaque."
    ),
)

stable.fn(
    "Collision_RegisterHitEvent",
    "8B 48 2C 51 57 56 E8 ??",
    match=-16,
    hook=6,
    ret="BOOL",
    params=[param("Collision_HitEvent*", "hit_events"), param("Actor_State*", "actor")],
    doc="Registers actor in the hit-event ring/list when no active matching event exists.",
)

stable.fn(
    "Collision_CheckHitEventExists",
    "5F 5E B0 01 C3 8B 0D ??",
    match=-42,
    ret="BOOL",
    params=[
        param("Collision_HitEvent*", "hit_events"),
        param("Actor_State*", "actor"),
        param("int32_t", "actor_collision_key"),
    ],
    doc="Returns whether hitEvents already contains an active event for actor/collision key.",
)

stable.fn(
    "Collision_HandleComponentCollision",
    "FF FF 5D C3 55 57 E8 ??",
    match=-45,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("Actor_State*", "other_actor")],
    doc=(
        "Handles component-to-actor collision side effects before the projectile/component "
        "response callback path. Uses Component_Instance and Actor_State runtime fields."
    ),
)

stable.fn(
    "Component_CalculateHomingVelocity",
    "8B 40 08 89 41 08 E9 ??",
    match=-159,
    hook=6,
    ret="int32_t",
    params=[param("Component_Instance*", "comp")],
)

stable.fn(
    "Component_SetVelocityFromDirection",
    "7E ?? 51 6A 00 57 E8 ??",
    match=-28,
    ret="int16_t",
    params=[param("Component_Instance*", "comp"), param("int32_t", "speed_scale")],
)

stable.fn(
    "Collision_ProcessProjectileLifecycle",
    "?? 74 ?? 6A 00 57 E8 ??",
    match=-28,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Actor-level projectile lifecycle callback. Drives projectile state transitions, may "
        "reinitialize via Component_InitializeProjectile, spawn follow-up projectiles, and "
        "emit trails; callback slot remains engine-owned."
    ),
)

stable.fn(
    "Component_SpawnFollowupProjectile",
    "?? 50 8D 47 40 50 E8 ??",
    match=-36,
    ret="Actor_State*",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "source_actor"),
        param("Actor_State*", "parent_actor"),
    ],
    doc=(
        "Spawns a follow-up projectile actor from a component definition and source actor, "
        "optionally attaching it to parent_actor with Actor_AttachToParent. Copies source "
        "direction into child yaw/pitch and transform state."
    ),
)

stable.fn(
    "Component_AttachToOwner",
    "46 04 F6 00 C0 0F 85 ??",
    match=-13,
    ret="char*",
    params=[
        param("Actor_State*", "owner_actor"),
        param("Component_Instance*", "comp"),
        param("int32_t*", "local_pos"),
    ],
)

stable.fn(
    "Actor_InitializeDirectionTables", "53 55 56 33 DB B9 ??", ret="int32_t", params=[]
)

stable.fn(
    "Collision_DetectAndResolve3DCollision",
    "55 8B EC 81 EC 2C 02 00 00 E8 ??",
    hook=9,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc=(
                "Actor/context pointer supplied by the collision pipeline. Validate identity and mutability "
                "before writing actor fields from hooks."
            ),
        ),
        param(
            "int32_t*",
            "velocity",
            doc="Caller-owned three-int velocity payload; preserving the native pointer is safest unless mutation has been proven safe.",
        ),
        param(
            "int16_t*",
            "surface_normal",
            doc="Caller-owned surface-normal payload filled by the collision resolver.",
        ),
        param(
            "int16_t*",
            "contact_point",
            doc="Caller-owned contact-point payload; validate readability before dereferencing in hooks.",
        ),
        param(
            "int32_t*",
            "result",
            doc="Caller-owned result/contact payload passed independently from this function's return value.",
        ),
    ],
    doc=(
        "Detects and resolves 3D collision for an actor/context and caller-supplied "
        "scratch payloads. velocity, surfaceNormal, contactPoint, and result are "
        "native raw payload pointers unless a specific call site proves they are "
        "safe to dereference or mutate."
    ),
)

stable.fn(
    "Collision_DetectActorCollisions",
    "85 C0 57 74 ?? 8B 2D ??",
    match=-46,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t*", "collision_list")],
)

stable.fn(
    "Collision_ProcessActorGroundCheck",
    "00 53 89 44 24 04 A1 ??",
    match=-12,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Ground-check collision pass over actor collision nodes and contextual polygon/contact "
        "state. Updates collision_ground_* globals as query output; the paired "
        "actor ground-contact state remains contextual runtime collision data."
    ),
)

stable.fn(
    "Collision_BuildWallCollisionPlane",
    "C2 22 89 4D 10 0F 84 ??",
    match=-76,
    hook=6,
    ret="int16_t*",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon*", "polygon"),
        param("int16_t*", "normal"),
        param("int32_t", "distance"),
        param("Collision_Plane*", "plane"),
        param("int32_t", "offset"),
        param("Collision_Response*", "out_result"),
    ],
)

stable.fn(
    "Collision_DetectActorObstacles",
    "8B 4F 4C 52 50 51 E8 ??",
    match=-47,
    hook=7,
    ret="int32_t*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Collision_DetectObjectNodeCollisions",
    "?? ?? ?? C1 E2 07 8B 40",
    match=-55,
    hook=7,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose collision callbacks, bounds, and object-node collision lists are being tested.",
        ),
        param(
            "int32_t",
            "collision_type",
            doc="Actor collision type/list index; type 2 may recurse into type 4 for special object-node checks.",
        ),
    ],
    doc="Walks the object-node collision list for collisionType and resolves matching collisions against actor.",
)

stable.fn(
    "Collision_TestLineSphereIntersection",
    "55 8B EC 83 EC 14 8B 45 10 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[
        param("Math_Vec3i32*", "line_start"),
        param("Math_Vec3i32*", "line_end"),
        param("uint32_t", "sphere_radius"),
    ],
)

stable.fn(
    "Collision_ResolveObjectNodeCollision",
    "E1 06 8B 45 0C 8B 35 ??",
    match=-86,
    hook=9,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "object_actor")],
)

stable.fn(
    "Collision_ProcessActorToActorCollisions",
    "83 EC 30 55 8B 2D ??",
    hook=10,
    ret="void",
    params=[],
    doc=(
        "Frame collision pass over live actor/entity runtime lists. It drives actor-to-actor "
        "collision checks and dispatches response callbacks through engine-owned globals; "
        "entity slots and actor pointers are level-local runtime identities."
    ),
)

stable.fn(
    "Physics_UpdateActorPreprocess",
    "42 65 01 75 ?? 8B 0D ??",
    match=-26,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Collision_ProcessPowerupCollisions",
    "83 EC 1C A1 ?? ??",
    hook=8,
    ret="int32_t",
    public=False,
    params=[param("Actor_State*", "actor")],
    doc=(
        "Scans the powerup actor list against actor and dispatches powerup_collision_handler as "
        "(powerup_actor, actor, 0, -2). Unconsumed pairs may fall through to swept/sphere distance "
        "tests and Collision_ResolveActorToActorCollision(actor, powerup_actor, -1, 0)."
    ),
)

stable.fn(
    "Model_UpdateShadow",
    "0F 00 00 51 53 50 E8 ??",
    match=-53,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Physics_UpdateActorBehavior",
    "?? 83 C4 04 84 C0 74 ?? 56",
    match=-21,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Model_UpdateTransformAndPhysics",
    "00 3B C1 74 ?? 83 3D ??",
    match=-19,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Model_UpdateAttachment",
    "85 FF 89 7D FC 0F 84 ??",
    match=-16,
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Sound_CheckCooldownExpired",
    "?? B0 01 5E C3 8B 0D ??",
    match=-32,
    hook=8,
    ret="BOOL",
    params=[
        param("int32_t*", "cooldown_entries"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "sound_key"),
    ],
    doc="Checks whether a collision sound cooldown entry for otherActor/soundKey has expired.",
)

stable.fn(
    "Sound_TriggerCollisionSound",
    "00 00 00 55 53 56 E8 ??",
    match=-43,
    ret="BOOL",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "sound_key"),
        param("uint32_t", "packed_impact_cooldown"),
    ],
    doc="Triggers a collision/impact sound for actor vs otherActor when the packed cooldown check permits it.",
)

stable.fn(
    "Collision_GetValidCollisionTarget",
    "3B CA 7E ?? 8B 8E E8 ??",
    match=-78,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor"), param("int32_t", "collision_slot")],
)

stable.fn(
    "Actor_CheckCollisionConditions",
    "24 9D ?? ?? ?? ?? A1 ??",
    match=-40,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "other_actor")],
    doc=(
        "Sentinel collision-depth condition check used by Actor_ProcessCollisionResponse "
        "when collision_depth is -1 and the other actor subtype requires condition "
        "checks. Dispatches by other_actor subtype fields and selector masks; native "
        "return values are 0 or -1 sentinel results."
    ),
)

stable.fn(
    "Actor_HandleCollisionResponse",
    "00 00 83 FF 06 0F 87 ??",
    match=-23,
    ret="int32_t",
    public=False,
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

stable.fn(
    "Physics_ApplyGroundReaction",
    "3B CA 89 4D F4 0F 84 ??",
    match=-17,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("void*", "physics_state")],
)

stable.fn(
    "Physics_CalculateFrictionForce",
    "55 8B EC 51 53 56 57 8B 7D 08 33 C0 8B 8F E8 ??",
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Physics_CalculateActorVelocity",
    "F4 00 00 00 8B 8F E8 ??",
    match=-21,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose physics component and velocity fields are updated.",
        ),
        param(
            "Math_Vec3i32*",
            "inout_velocity",
            doc="Three-int velocity vector normalized, clamped, and copied back into the actor velocity fields.",
        ),
        param(
            "Math_Vec3i32*",
            "steering_vector",
            doc="Three-int caller-provided steering/environment vector used by the velocity integration check.",
        ),
    ],
    doc="Calculates and applies the actor velocity from physics state, caller velocity, steering, ground, and slope inputs.",
)

stable.fn(
    "Physics_ApplyMovingPlatformForce",
    "8B 5E 78 85 DB 0F 84 ??",
    match=-12,
    hook=6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("void*", "physics_state"),
        param("Actor_State*", "platform_actor"),
        param("int32_t*", "velocity"),
        param("int32_t*", "out_applied_speed"),
    ],
    doc="Applies moving-platform velocity/force from platformActor into actor physics and reports the applied speed.",
)

stable.fn(
    "Actor_UpdateRotationFromVelocity",
    "?? 50 8B 41 08 50 E8 ??",
    match=-30,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t*", "velocity_xz"),
        param("int32_t", "turn_step"),
    ],
)

stable.fn(
    "Actor_ProcessHazardsAndDamage",
    "75 ?? 8B 7D 08 57 E8 ??",
    match=-29,
    hook=6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("void*", "physics_state")],
)

stable.fn(
    "Audio_InitializeSystem",
    "00 55 56 57 FF 15 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 35",
    match=-5,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Audio_ShutdownSystem",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 53 8B",
    ret="int32_t",
    params=[],
    doc=(
        "Shuts down Miles audio when audio_digital_driver is active, releasing non-null sample "
        "handles across Audio_SoundSlot entries, clearing slot sample/base-rate fields, calling "
        "AIL_shutdown, and clearing audio_digital_driver. Returns 0 when already inactive."
    ),
)

stable.fn(
    "Audio_StopAllSamples",
    "?? ?? ?? BE ?? ?? ?? ?? 8B 06 50 FF D7 83 C6 14",
    match=-5,
    hook=8,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Audio_StopAllSounds",
    "6A 00 50 FF 15 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90",
    match=-5,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Audio_InitializeLevelAudio",
    "6A 7F 50 FF 15 ?? ?? ?? ?? C3 90",
    match=-5,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Audio_StartSoundPlayback",
    "53 55 8B 6C 24 10 56 57 81 7D 00 52 49 46 46 0F 85 ??",
    hook=6,
    ret="void",
    params=[
        param("int32_t", "slot_index"),
        param("int32_t*", "wave_data"),
        param("int32_t", "pitch_scale_q12"),
        param("int32_t", "volume"),
        param("int32_t", "pan"),
        param("char", "loop_flag"),
    ],
    doc=(
        "Starts a RIFF-backed sample in the selected sound slot after resetting the Miles sample "
        "handle, caching its base playback rate, applying pitch/pan/volume, and forcing the "
        "Miles loop count to zero when loopFlag is nonzero."
    ),
)

stable.fn(
    "Audio_SetSampleVolume",
    "?? 8B 4C 24 08 C1 E9 ??",
    match=-19,
    hook=7,
    ret="int32_t",
    params=[param("int32_t", "slot_index"), param("int32_t", "volume")],
    doc=(
        "Set a sound slot's sample volume, scaling the game volume down to Miles' 0..128 range before "
        "calling AIL_set_sample_volume."
    ),
)

stable.fn(
    "Audio_SetSamplePitch",
    "0F AC D0 0C 3B 81 ?? ??",
    match=-31,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "slot_index"), param("int32_t", "pitch_scale_q12")],
    doc="Set a sound slot's sample playback rate to base_playback_rate * pitchScaleQ12 / 4096.",
)

stable.fn(
    "Audio_OpenStream",
    "A1 ?? ?? ?? ?? 81 EC 04",
    ret="void",
    params=[
        param(
            "int32_t*",
            "stream_record",
            doc="Record whose first dword receives the Audio_AIL_HStream; bytes at +4 hold the music filename.",
        )
    ],
    doc=(
        "Open streamRecord[0] from the music filename stored at streamRecord+4 under the data/music directory; "
        "clears the handle when audio is unavailable or the filename is empty."
    ),
)

stable.fn(
    "Audio_IsStreamPlaying",
    "8B 44 24 04 8B 00 85 C0 74 ?? 50 FF 15 ??",
    hook=6,
    ret="int32_t",
    params=[param("Audio_AIL_HStream*", "stream_handle_ptr")],
)

stable.fn(
    "Audio_SetStreamVolume",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 44 24 04 C1",
    hook=6,
    ret="void",
    params=[param("int32_t", "volume")],
    doc=(
        "Set the current music stream volume, scaling the game volume down to Miles' 0..128 range "
        "before calling AIL_set_stream_volume."
    ),
)

stable.fn(
    "Audio_PlayMusicStream",
    "56 8B 74 24 08 85 F6 74 ?? 56 E8 ?? ?? ?? ?? 8B 06 83 C4 04 A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 5E 85 C0 74 ?? 8B 4C 24 08 51 E8 ?? ?? ?? ??",
    ret="void",
    params=[param("int32_t*", "stream_record"), param("int32_t", "volume")],
    doc="Optionally opens streamRecord, publishes its handle as the active music stream, sets volume, starts playback, and sets the stream loop count to zero.",
)

stable.fn(
    "Audio_PauseStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 01 50 FF 15 ?? ?? ?? ?? C3",
    ret="void",
    params=[],
    doc="Pauses the active Miles music stream when one is published.",
)

stable.fn(
    "Audio_ResumeStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 00 50",
    ret="void",
    params=[],
    doc="Resumes the active Miles music stream when one is published.",
)

stable.fn(
    "Audio_CloseMusicStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 01 50 FF 15 ?? ?? ?? ?? A1",
    ret="void",
    params=[],
    doc="Pauses and closes the active Miles music stream, clears music_stream_handle, and decrements open_stream_count; residual Miles/counter return is ignored.",
)

stable.fn(
    "Audio_SetEnabledFlag",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 55 8B EC 51",
    required=Required.EN,
    hook=9,
    ret="int32_t",
    params=[param("char", "enabled_flag")],
    doc="Stores the one-byte global audio enabled flag and returns the written value.",
)

stable.fn(
    "Audio_SetMusicFadeTarget",
    "55 8B EC 51 8B 45 08 0F BF 0D ?? ?? ?? ?? 85 C0 7D ??",
    hook=7,
    ret="int32_t",
    params=[
        param("int32_t", "target_volume_q12"),
        param("int16_t", "frame_count_minus_one"),
    ],
    doc="Sets the music fade target volume and stores frameCountMinusOne + 1 as the fade frame count; negative targets scale the current target volume by -target/4096.",
)

stable.fn(
    "Audio_FadeOutMusic",
    "F6 05 ?? ?? ?? ?? 06 75 ?? 66 A1 ??",
    required=Required.EN,
    hook=7,
    ret="void",
    params=[],
)

stable.fn(
    "Audio_FadeInMusic",
    "BE 49 00 6A 0F 50 E8 ??",
    match=-13,
    required=Required.EN,
    hook=7,
    ret="void",
    params=[],
)

stable.fn(
    "Audio_StopMusicAndPause",
    "8A 0D ?? ?? ?? ?? B0 02 84 C8 75 ?? 0A C8 88 0D ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 44 24 04 A3 ??",
    required=Required.EN,
    hook=6,
    ret="void",
    params=[],
    doc="Sets the music stop/pause flag and closes the active music stream; tail-call/counter residual is ignored by observed callers.",
)

stable.fn(
    "Audio_ResetMusicState",
    "8B 44 24 04 A3 ?? ?? ?? ?? A0",
    hook=9,
    ret="void",
    params=[param("int32_t*", "stream_record")],
    doc=(
        "Stores the selected music stream record pointer for the fade/playback path and "
        "clears the low nibble of sound_system_flags; the previous scalar return was flag status."
    ),
)

stable.fn(
    "Audio_StartMusicWithFade",
    "6A 1E 68 00 10 00 00 66 C7 05 ??",
    hook=7,
    ret="void",
    params=[param("int32_t*", "stream_record")],
    doc=(
        "Arms faded playback for the selected music stream record with a 0x1000 target "
        "volume over 31 fade frames, then resets music state; residual flag return is ignored."
    ),
)

stable.fn(
    "Audio_CheckStreamStatus",
    "8B 44 24 04 85 C0 74 ?? 50 E8 ??",
    hook=6,
    ret="int32_t",
    params=[param("Audio_AIL_HStream*", "stream_handle_ptr")],
    doc="Returns nonzero when stream_handle_ptr is non-null and the pointed Miles stream is currently playing.",
)

stable.fn(
    "Audio_ProcessMusicFade",
    "55 8B EC 83 EC 08 E8 ??",
    hook=6,
    ret="void",
    params=[],
    doc="Processes one frame of music fade/stream state: advances the fade target, starts playback when allowed, pauses at zero volume, and recalculates stream volume; observed callers use side effects only.",
)

stable.fn(
    "Audio_UpdateSoundSystemWrapper",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 A1",
    ret="void",
    params=[],
    doc="Tail wrapper around the audio system update path; observed callers use side effects only and model no public return.",
)

stable.fn(
    "BoneTrail_CheckAvailable", "A1 ?? ?? ?? ?? 8B 88 84", ret="int32_t", params=[]
)

stable.fn(
    "BoneTrail_Reset",
    "33 C9 C7 05 ?? ??",
    hook=12,
    ret="void",
    params=[],
    doc="Clears bone-trail state and entry buffers.",
)

stable.fn(
    "BoneTrail_UpdateAndRender",
    "55 8B EC 83 EC 34 A1 ??",
    hook=6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("int32_t*", "movement_vec")],
    doc="Updates/renders bone-trail movement effects; movement_vec carries the output and caller overwrites native return metadata.",
)

stable.fn(
    "BoneTrail_FindPath",
    "55 8B EC 81 EC 60 01 00 00 A1 ??",
    hook=9,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3i32*",
            "start_pos",
            doc="Starting world position used to seed the bone-trail path search.",
        ),
        param(
            "Nav_Network*",
            "nav_network",
            doc="Navigation network whose nodes and neighbor lists are searched.",
        ),
        param(
            "int32_t",
            "skip_dynamic_targets",
            doc="Nonzero skips type 4/5 dynamic target refs while expanding path nodes.",
        ),
    ],
    doc="Builds a path through the navigation network from startPos and returns the number of path points.",
)

stable.fn(
    "Navigation_CalculatePolygonCenter",
    "57 F6 40 10 01 0F 84 ??",
    match=-11,
    hook=6,
    ret="int32_t",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("int32_t*", "out_center"),
    ],
    doc="Calculates the integer center point for a collision polygon in a collision/object node.",
)

stable.fn(
    "Navigation_ProcessPathNode",
    "54 C1 0E 85 D2 0F 84 ??",
    match=-29,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "void*",
            "open_set",
            doc="Opaque priority/open-set handle allocated by the pathfinding queue helpers.",
        ),
        param(
            "Nav_Network*",
            "nav_network",
            doc="Navigation network whose current node neighbors are expanded.",
        ),
        param(
            "Nav_PathState*",
            "current_state",
            doc="Open-set state entry for the node being processed.",
        ),
        param(
            "int32_t",
            "skip_dynamic_targets",
            doc="Nonzero skips type 4/5 dynamic target refs while expanding neighbors.",
        ),
    ],
    doc="Expands one pathfinding node, updates neighbor costs/backlinks, and queues reachable nodes.",
)

stable.fn(
    "Model_ResetState",
    "00 00 8B 46 7C 50 E8 ??",
    match=-17,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_ClearReferences",
    "56 8B 35 ?? ?? ?? ?? 85 F6 74 ?? 57 C7 05 ?? ?? A8",
    hook=7,
    ret="void",
    params=[],
)

stable.fn(
    "Actor_CloneTemplateWithTemplateRelativeFixups",
    "C3 F6 43 65 08 0F 85 ??",
    match=-22,
    ret="Actor_State*",
    params=[
        param(
            "Actor_State*",
            "source_actor_template",
            doc=(
                "Source actor/template whose pointer fields are relative to the cloned actor allocation; "
                "fixed-up package or borrowed runtime pointers are unsafe."
            ),
        ),
        param(
            "Actor_State**",
            "actor_list_head",
            doc="Optional linked-list head; when non-null the instantiated actor is pushed onto this list.",
        ),
    ],
    doc=(
        "Allocates/copies an Actor_State template, then rewrites selected pointer fields as "
        "cloned_actor + source_field - source_actor. This template-relative clone path leaves "
        "external package pointers subject to source lifetime."
    ),
)

stable.fn(
    "Actor_Destroy",
    "08 88 41 65 C3 51 E8 ??",
    match=-21,
    hook=11,
    ret="BOOL",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Actor_AttachToParent",
    "53 55 56 57 8B 7C 24 14 57 E8 ??",
    hook=8,
    ret="Actor_State*",
    params=[
        param("Actor_State*", "child_actor"),
        param("Actor_State*", "parent_actor"),
        param("void*", "attach_point"),
        param("int32_t", "attach_flags"),
    ],
)

stable.fn(
    "Actor_FindNearestAttachPoint",
    "7D ?? 89 45 F8 0F 84 ??",
    match=-24,
    hook=6,
    ret="void* *",
    params=[
        param("Actor_State*", "parent_actor"),
        param("Actor_State*", "child_actor"),
    ],
)

stable.fn(
    "Actor_SetAlphaFade",
    "C6 85 FF 75 ?? 81 A1 ??",
    match=-110,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t", "fade_steps"),
        param("int32_t", "fade_in"),
    ],
)

stable.fn(
    "Actor_UpdateFadeOut",
    "BF C0 6A 00 50 56 E8 ??",
    match=-32,
    ret="BOOL",
    params=[param("Actor_State*", "actor"), param("int16_t", "fade_ticks")],
    doc="Advances an actor fade-out/lifecycle step and returns a low-byte boolean completion/status value.",
)

stable.fn(
    "Actor_AddToCollisionList",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 89 08 A3",
    hook=10,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Shadow_ClearList",
    "C7 05 ?? ?? ?? ?? 00 00 00 00 C3 90 90 90 90 90 55",
    hook=10,
    ret="void",
    params=[],
)

stable.fn(
    "Shadow_RenderAll",
    "7E ?? 6A 00 51 57 E8 ??",
    match=-43,
    hook=9,
    ret="void",
    params=[],
    doc="Renders queued shadows; Scene_RenderFrame calls it for side effects and ignores render-list status.",
)

stable.fn(
    "Resource_FixUpObjectNode",
    "56 8B 74 24 08 8B 46 04 85 C0 74 ?? 8B 0D ??",
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc=(
        "Rebases the node+0x04 (PC EN) sibling/cursor link, dispatches by node_type at node+0x64 (PC "
        "EN), and recurses child siblings. Type 2 returns before the node+0x08 (PC EN) child-link "
        "rebase, while handled non-type-2 payloads are fixed before recursion."
    ),
)

stable.fn(
    "Resource_FixUpModelNode",
    "?? ?? ?? 03 C2 89 86 80",
    match=-78,
    ret="void",
    params=[
        param(
            "void*",
            "node",
            doc="Type-1 model node record; fields are level-blob-relative offsets rebased in place.",
        )
    ],
    doc="Type-1 mesh/actor-like object-node fixup; mutates model-node fields in place.",
)

stable.fn(
    "Resource_FixUpAnimationData",
    "51 8B 4C 24 08 8A 41 07 A8 02 0F 85 ??",
    ret="void",
    params=[
        param(
            "Animation_DataBlock*",
            "animation_data",
            doc="Animation block whose channel tables are marked/fixed in place.",
        ),
        param(
            "int32_t",
            "base_address",
            doc="Base added to relative animation/table offsets.",
        ),
    ],
    doc="Marks/fixes animation channel tables in place; return was loop/flag status propagated by decompilers, not public data.",
)

stable.fn(
    "Resource_FixUpMaterialRefs",
    "8B 54 24 08 8B 44 24 04 56 8B 08 8B 35 ??",
    hook=8,
    ret="void",
    params=[
        param(
            "int32_t*",
            "material_refs",
            doc="Array of 0x18-byte material-reference records fixed in place.",
        ),
        param(
            "int32_t",
            "ref_count",
            doc="Number of material-reference records to process.",
        ),
    ],
    doc="Walks and fixes ref_count 0x18-byte material-reference records in place.",
)

stable.fn(
    "Resource_FixUpMaterialIndices",
    "8B 54 24 04 56 F6 02 80 75 ?? A1 ??",
    ret="void",
    params=[
        param(
            "char*",
            "index_block",
            doc="Pointer-list block with flag byte at +0 (PC EN), count at +0x02 (PC EN), and relative list pointer at +0x04 (PC EN).",
        )
    ],
    doc=(
        "Fixes a material index/pointer-list block in place. Flag bit 0x80 skips the block; "
        "otherwise the routine reads the count at +0x02 (PC EN), rebases the pointer list at +0x04 (PC EN), "
        "and fixes type-1 entries by rebasing their +0x10 (PC EN)/+0x14 (PC EN) side pointers."
    ),
)

stable.fn(
    "Resource_FixUpRenderNodeEntries",
    "72 ?? 8D 46 F0 50 E8 ??",
    match=-21,
    hook=6,
    ret="void",
    params=[
        param(
            "Mesh_RenderNodeEntry*",
            "entries",
            doc="Array base; each 0x20-byte entry has a sprite/material descriptor at +0x0C (PC EN) and a tail pointer at +0x1C (PC EN) rebased by the level blob base.",
        ),
        param("int32_t", "entry_count", doc="Number of render-node entries to fix."),
    ],
    doc="Formerly misidentified as a vertex-normal fixup; walks render-node entries, fixes sprite/material descriptors, and rebases the entry tail pointer at +0x1C (PC EN) in place.",
)

stable.fn(
    "Resource_FixUpSpriteEntry",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 89 08 8B 48",
    hook=10,
    ret="void",
    params=[
        param(
            "int32_t*",
            "sprite_material_desc",
            doc="0x0C sprite/material descriptor fixed in place: material table base, material entry/index, optional index block.",
        ),
    ],
    doc="Fixes a sprite/material descriptor by publishing the material table, resolving its material entry, and rebasing/fixing an optional index block.",
)

stable.fn(
    "Resource_FixUpMeshNode",
    "8E 84 00 00 00 8B 15 ??",
    match=-73,
    ret="void",
    params=[
        param("void*", "node", doc="Type-3 scene mesh node record fixed in place.")
    ],
    doc="Type-3/complex object-node fixup; mutates node sidecars in place.",
)

stable.fn(
    "Resource_FixUpSimpleNode",
    "8B 4C 24 04 8B 41 6C 85 C0 74 ?? 8B 15 ??",
    hook=7,
    ret="void",
    params=[
        param(
            "void*",
            "node",
            doc="Type-8 simple node; only the payload pointer at +0x6C (PC EN) is rebased.",
        )
    ],
    doc="Type-8 pointer-only object-node fixup; mutates node +0x6C (PC EN) in place.",
)

stable.fn(
    "Resource_FixUpObjectNode_Type7_SpriteEntry",
    "8B 44 24 04 83 C0 6C 50 E8 ??",
    hook=7,
    ret="void",
    params=[
        param(
            "void*",
            "node",
            doc="Type-7 compact object node; passes the sprite/material descriptor payload at node +0x6C (PC EN) to Resource_FixUpSpriteEntry.",
        )
    ],
    doc="Formerly misidentified as a face-pointer/descriptor helper; this wrapper fixes the type-7 object-node sprite/material payload at node +0x6C (PC EN) in place.",
)

stable.fn(
    "Resource_FixUpGroupNode",
    "56 57 8B 7C 24 0C 8B 47 70 85 C0 74 ?? 8B 0D ??",
    hook=6,
    ret="void",
    params=[
        param(
            "void*",
            "node",
            doc="Group/type-0 hierarchy node record; rebases nested lists and fixes child/object/polygon sidecars.",
        )
    ],
    doc="Type-0 hierarchy object-node fixup; mutates linked child/object/polygon sidecars in place.",
)

stable.fn(
    "Resource_FixUpPolygonList",
    "56 57 8B 7C 24 0C 8B 37 85 F6 0F 84 ??",
    hook=6,
    ret="void",
    params=[
        param(
            "int32_t*",
            "list_head",
            doc="Relative polygon-list head/link field fixed in place.",
        ),
        param(
            "int32_t",
            "advance_by_link_slot",
            doc="Nonzero advances through consecutive link slots; zero follows each rebased node link.",
        ),
    ],
)

stable.fn(
    "Resource_FixUpLevelPointers",
    "?? ?? ?? ?? 83 C4 08 53",
    match=-17,
    ret="void",
    params=[
        param(
            "Pkg_LevelHeader*",
            "level",
            doc="Level header whose relative resource lists are rebased in place.",
        )
    ],
    doc="Rebases package level-header pointers in place; Level_LoadStateMachine ignores the debug-log/status native return value.",
)

stable.fn(
    "Resource_FixUpActorPointers",
    "56 8B 74 24 08 57 8B 46 10 85 C0 74 ?? 8B 0D ??",
    ret="void",
    params=[
        param(
            "Pkg_ActorTemplate*",
            "actor_template",
            doc="Actor template with three node refs at +0/+4/+8 and a 43-entry animation state table at +0x10 (PC EN).",
        )
    ],
    doc="Fixes actor-template animation and object-node references in place.",
)

stable.fn(
    "Resource_FixUpComponentNodes",
    "06 85 C0 74 ?? 8B 0D ??",
    match=-16,
    hook=7,
    ret="void",
    params=[
        param(
            "Pkg_ComponentData*",
            "component_data",
            doc="Component record; fixes four sub-node pointers at +0x60 (PC EN).",
        )
    ],
    doc="Fixes four component sub-node pointers, owner back-links, and optional animation data in place.",
)

stable.fn(
    "Material_FixupAndLoad",
    "8B 4C 24 10 51 50 E8 ??",
    match=-116,
    ret="void",
    params=[
        param(
            "Material_SectionHeader*",
            "material_section",
            doc="Material section header whose node/material-entry offsets are rebased in-place before loading.",
        ),
        param(
            "int32_t",
            "texture_base_addr",
            doc="Base passed through to the material loading path after section fixups.",
        ),
    ],
    doc="Rebases the material section header, fixes material-entry frame and node table pointers in-place, then hands the fixed-up section to the material loading path.",
)

stable.fn(
    "Material_GetDataTextureRefs",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 44 24 04 89",
    hook=6,
    ret="void",
    params=[
        param(
            "int32_t*",
            "data_ref_out",
            doc="Three-word output copied from the shared global texture reference block.",
        )
    ],
)

stable.fn(
    "SaveGame_ProcessOperation",
    "0F BE 05 ?? ?? ?? ?? 53 32 DB 83 E8 ??",
    required=Required.EN,
    hook=7,
    ret="BOOL",
    params=[param("uint32_t*", "status_out")],
    doc="Polls the active save-game operation state. Operation 8 reads savegame.dat, operation 9 writes it, and operation 12 verifies by reading and comparing buffers; writes the packed operation/status word to statusOut and returns whether the underlying file action succeeded.",
)

stable.fn(
    "SaveGame_ReadFile",
    "14 56 57 6A 01 50 E8 ??",
    match=-34,
    required=Required.EN,
    hook=6,
    ret="BOOL",
    params=[param("void*", "buffer"), param("uint32_t", "size")],
    doc="Opens savegame.dat in rb mode, reads exactly size bytes into buffer, then verifies the file length equals size before returning TRUE.",
)

stable.fn(
    "SaveGame_WriteFile",
    "08 56 6A 01 50 51 E8 ??",
    match=-32,
    required=Required.EN,
    hook=6,
    ret="BOOL",
    params=[param("void const*", "buffer"), param("uint32_t", "size")],
    doc="Opens savegame.dat in wb mode, writes one size-byte record from buffer, closes the file, and returns TRUE if the file was opened.",
)

stable.fn(
    "SaveGame_InitOperation",
    "A1 ?? ?? ?? ?? 53 8B 5C",
    ret="void",
    params=[
        param("uint8_t", "operation"),
        param("void*", "buffer"),
        param("uint32_t", "size"),
    ],
    doc=(
        "Initialize, reset, or free the global save-game operation state for the requested operation code. "
        "Operation 0x0b frees the verify-buffer scratch allocation; other operations allocate or reuse it, "
        "store the active buffer and size globals, and leave only verify-buffer/allocation native return metadata."
    ),
)

stable.fn(
    "Movie_PlayFile",
    "0D ?? ?? ?? ?? 8B 54 24 04 56 50 A1 ?? ?? ?? ?? 51 52 50 E8",
    match=-5,
    hook=10,
    ret="BOOL",
    params=[
        param(
            "char const*",
            "movie_path",
            doc="Fully formatted movie path passed to Video_OpenMovieFile.",
        ),
        param(
            "char",
            "use_alt_video_rect",
            doc="Non-zero selects the alternate video rectangle in Video_OpenMovieFile.",
        ),
    ],
    doc="Open and play the supplied movie path; closes playback on normal stop and requests shutdown on skip/Alt+F4 paths.",
)

stable.fn(
    "Movie_PlayIntro",
    "56 8B 74 24 08 C7 05 ??",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "movie_index",
            doc="Index into the four-entry startup movie filename/type tables.",
        )
    ],
    doc="Build and play one startup movie path selected by movieIndex; movie 0 initializes the player and movie 2 selects the alternate video rectangle.",
)

stable.fn(
    "Timer_GetGameTime", "83 EC 08 FF 15 ??", hook=9, ret="long double", params=[]
)

stable.fn(
    "Debug_RenderOverlay",
    "81 EC 0C 01 00 00 8B 0D ??",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Input_RegisterButtonMapping",
    "56 8B 74 24 08 81 FE E8 ??",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "control_code",
            doc="Virtual-key code or gamepad control code; gamepad codes start at 0x3e8.",
        ),
        param(
            "uint32_t",
            "button_mask",
            doc="Input_State.button_bits mask produced by this control.",
        ),
    ],
    doc=(
        "Registers a keyboard/gamepad control-to-button-mask binding. Codes below 0x3e8 "
        "append to the keyboard mapping arrays and refresh the button-name cache; codes "
        "0x3e8 and above write the direct gamepad control lookup table."
    ),
)

stable.fn(
    "D3D_SetFogDistance",
    "DB 44 24 04 D8 15 ??",
    hook=10,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "gamma_setting",
            doc="Integer menu gamma setting centered at 5; values below/above 5 map by 0.2 gamma-scale steps.",
        )
    ],
    doc="Converts the 0..10 menu gamma setting to a D3D gamma scale, clamps low values to 0.1, and forwards the scale to D3D_SetGammaRamp.",
)

stable.fn(
    "Config_ApplySettings",
    "8B 0D ?? ?? ?? ?? 56 57 8B",
    hook=6,
    ret="void",
    params=[],
    doc="Clear cached input bindings and rebuild keyboard/gamepad button masks from the loaded pcdogs.ini settings.",
)

stable.fn(
    "Config_LoadFromINI",
    "83 EC 08 57 68 ??",
    hook=9,
    ret="int32_t",
    params=[],
    doc="Loads pcdogs.ini when present and checksum-valid, clamps the display setting, restores default special-button binding when missing, reads player control bindings, then applies the resulting input mapping.",
)

stable.fn(
    "Config_SaveSettingsToINI",
    "A5 A2 ?? ?? ?? ?? E8 ??",
    match=-24,
    required=Required.EN,
    hook=7,
    ret="int32_t",
    params=[param("void const*", "config_data")],
    doc="Copies the supplied 0x6c-byte settings block into the global config while preserving the current display setting, reapplies input mappings, then writes pcdogs.ini with the PCDOGS header and control bindings.",
)

stable.fn(
    "Input_InitializeButtonMappings",
    "?? ?? 00 00 00 00 68 80",
    match=-50,
    ret="int32_t",
    params=[],
    doc=(
        "Free and rebuild the keyboard mapping arrays, register the default control masks, and initialize the "
        "player binding blocks."
    ),
)

stable.fn(
    "Input_InitializeControllerMappings",
    "53 55 56 57 68 ?? ?? ?? ?? 68",
    hook=9,
    ret="int32_t",
    params=[],
    doc=(
        "Initialize built-in controller preset names and 10-button mapping tables for Hammerhead FX, "
        "Microsoft Sidewinder, Gravis Gamepad Pro, and Wingman RumblePad."
    ),
)

stable.fn(
    "D3D_InitializeGraphicsSubsystem",
    "E8 ?? ?? ?? ?? 8B 44 24 08 8B",
    ret="int32_t",
    params=[
        param(
            "HWND",
            "hwnd",
            doc="Main game window handle forwarded to DirectInput joystick initialization.",
        ),
        param(
            "HINSTANCE",
            "hinstance",
            doc="Application instance handle forwarded to DirectInput joystick initialization.",
        ),
    ],
    doc="Clears input state, initializes joystick/force-feedback support for the game window, and sets the global D3D scale factor.",
)

stable.fn(
    "Config_LoadFromINI_Alternate",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 A1",
    ret="int32_t",
    params=[],
    doc="Alternate config-loading entry point that initializes controller and button mappings before tail-calling Config_LoadFromINI.",
)

stable.fn(
    "Input_ReadKeyboard",
    "A1 ?? ?? ?? ?? 53 8B 1D",
    ret="int32_t",
    params=[
        param(
            "Input_State*",
            "state",
            doc="Input state structure updated from keyboard state.",
        )
    ],
    doc=(
        "Sample keyboard mappings into the per-frame input state record, ORing mapped masks into button_bits; "
        "F10/VK121 triggers the screenshot path and the int32_t return is GetAsyncKeyState/screenshot "
        "native return metadata ignored by Input_ReadDevices."
    ),
)

stable.fn(
    "Input_ReadGamepad",
    "A0 ?? ?? ?? ?? 83 EC 08 84 C0 0F 84 ??",
    required=Required.EN,
    ret="void",
    params=[
        param(
            "Input_State*",
            "state",
            doc="Input state structure updated from gamepad state.",
        )
    ],
    doc=(
        "Samples DirectInput gamepad state into the per-frame input record with lX/lY +/-700 and lRz "
        "+/-600 thresholds. Live analog hooks sample JoyState_GetAxis* in the same frame to keep input "
        "frame-local, and native callers ignore the return register."
    ),
)

stable.fn(
    "Input_ReadDevices",
    "46 06 66 89 46 04 E8 ??",
    match=-20,
    ret="void",
    params=[
        param(
            "int32_t",
            "player_index",
            doc="Player/input slot index preserved for the Input_Update ABI.",
        ),
        param(
            "Input_State*",
            "state",
            doc="Input state structure cleared, then updated from keyboard and gamepad devices.",
        ),
    ],
    doc="Clears one Input_State and combines keyboard and gamepad sampling into it; player_index is preserved for the ABI.",
)

stable.fn(
    "Input_ReleaseDirectInputResources",
    "A1 ?? ?? ?? ?? 56 33 F6 3B C6 74 ?? 50",
    ret="void",
    params=[],
    doc="Releases DirectInput/input-owned resources: joystick state buffer, joystick device, DirectInput interface, and keyboard mapping arrays; clears each global after release/free.",
)

stable.fn(
    "Game_HandleGameOver",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? DB",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "effect_source_id",
            doc="Caller-supplied actor/type id preserved by the ABI.",
        ),
        param(
            "int32_t",
            "strong_feedback",
            doc="Nonzero doubles the force-feedback magnitude scale from 5000 to 10000.",
        ),
        param(
            "int32_t",
            "force_magnitude_fixed",
            doc="Fixed-point 12-bit force magnitude; 0x1000 represents 1.0 before scaling.",
        ),
        param(
            "int32_t",
            "duration_units",
            doc="Duration multiplier converted to DirectInput effect duration by multiplying by 100000.",
        ),
    ],
    doc="If player lives are exhausted, computes and plays a constant-force feedback effect, then restarts the DirectInput effect/render-target helper.",
)

stable.fn(
    "Input_GetButtonIndex",
    "00 40 00 00 74 ?? 3D ??",
    match=-113,
    hook=7,
    ret="int32_t",
    params=[param("uint32_t", "control_mask")],
    doc=(
        "Maps an input button/control bitmask to the compact button-name index used by "
        "Input_FormatButtonName. Low masks 1..0x20 dispatch through "
        "input_button_mask_index_table; 0x40, 0x80, 0x400, 0x800, 0x4000, and 0x8000 "
        "are handled by direct compares; returns 12 for unrecognized masks."
    ),
)

stable.fn(
    "Input_FormatButtonName",
    "8B 44 24 08 56 50 E8 ??",
    ret="char*",
    params=[
        param(
            "int32_t",
            "control_code",
            doc="Virtual-key or gamepad control code; codes above 0xff are normalized by subtracting 0x2e8 for the string-id table.",
        ),
        param(
            "uint32_t",
            "button_mask",
            doc="Input button mask used to choose the destination name-cache slot.",
        ),
    ],
    doc=(
        "Caches the localized display name for a control binding in the slot selected by button_mask "
        "and lazily allocates the shared 'No key assigned' string. Known native callers use the side "
        "effect and ignore the mixed pointer/sprintf-count native return value."
    ),
)

stable.fn(
    "Input_GetButtonString",
    "8B 44 24 04 3D FF 00 00 00 7E ?? 2D E8 ??",
    hook=9,
    ret="char*",
    params=[param("int32_t", "button_code")],
)

stable.fn(
    "Input_IsKeyPressedAsync",
    "8B 44 24 04 50 FF 15 ??",
    ret="BOOL",
    params=[
        param(
            "int32_t",
            "virtual_key",
            doc="Win32 virtual-key code passed to GetAsyncKeyState.",
        )
    ],
    doc="Calls GetAsyncKeyState(virtualKey) and returns 1 when the high-order key-down bit is set.",
)

stable.fn(
    "Input_GetPressedButton",
    "56 57 8B 3D ?? ?? ?? ?? 33 F6",
    hook=8,
    ret="int32_t",
    params=[],
    doc="Returns the first pressed keyboard virtual-key code, or gamepad codes 0x3e8..0x3fa for axis/button input, or -1 when no control is pressed.",
)

stable.fn("Graphics_HasFogCapability", "A1 ?? ?? ?? ?? 25 00", ret="int32_t", params=[])

stable.fn(
    "Input_ProcessWindowMessages", "A1 ?? ?? ?? ?? 83 EC 1C", ret="int32_t", params=[]
)

stable.fn(
    "Render_Frame",
    "51 53 E8 ?? ?? ?? ?? A1",
    hook=7,
    ret="uint8_t",
    params=[],
    doc=(
        "Main frame boundary: begins/ends the D3D scene, runs Game_UpdateAndRenderScene, "
        "handles surface restore/flip, enforces the 30 FPS limiter, and updates FPS counters. "
        "Return is an AL status byte: 1 when the frame was skipped/aborted, otherwise 0."
    ),
)

stable.fn(
    "Input_ResetState", "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 25", ret="int32_t", params=[]
)

stable.fn(
    "Game_WindowProc",
    "FF 00 01 00 00 C7 05 ??",
    cc=CallingConvention.STDCALL,
    match=-11,
    ret="int32_t",
    params=[
        param("HWND", "hwnd"),
        param("uint32_t", "u_msg"),
        param("uint32_t", "w_param"),
        param("int32_t", "l_param"),
    ],
    doc=(
        "Main Win32 window procedure: handles destroy/close shutdown, key down/up "
        "messages, blocks selected system-menu commands, and defers unhandled messages "
        "to DefWindowProcA."
    ),
)

stable.fn(
    "Resource_FinalGameCleanup",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E9",
    ret="int32_t",
    params=[],
    doc="Final game shutdown chain: shuts down audio, closes the package file handle, then tail-calls Resource_ShutdownGameSubsystems.",
)

stable.fn(
    "Input_GetWindowHandle",
    "A1 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90",
    ret="HWND",
    params=[],
)

stable.fn(
    "WinMain",
    "3C 89 5C 24 40 FF 15 ??",
    cc=CallingConvention.STDCALL,
    match=-39,
    hook=8,
    ret="int32_t",
    params=[
        param("HINSTANCE", "h_instance"),
        param("HINSTANCE", "h_prev_instance"),
        param("LPSTR", "lp_cmd_line"),
        param("int32_t", "n_cmd_show"),
    ],
)

stable.fn(
    "Pkg_InitializeSystem",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1",
    ret="int32_t",
    params=[],
    doc="Bootstraps package/resource startup by opening and reading the package TOC, initializing audio, initializing render dispatch tables, and seeding graphics flags/capabilities; returns the resulting graphics capability word/status value.",
)

stable.fn(
    "Window_RequestClose",
    "51 8D 44 24 00 6A 00 50 6A 00 6A 61 FF 15 ??",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Resource_ReleaseManager",
    "F6 C5 08 75 ?? 50 E8 ??",
    match=-27,
    hook=7,
    ret="void*",
    params=[
        param(
            "Material_Table*",
            "material_table",
            doc="Material table whose loaded descriptors/surfaces are released or unmarked.",
        )
    ],
)

stable.fn(
    "Material_MarkReferencedByParent",
    "40 02 40 75 ?? 3B E9 ??",
    match=-40,
    ret="int32_t*",
    params=[
        param("Material_Table*", "material_table"),
        param("Material_TableEntry*", "entry"),
        param("int32_t", "parent_texture_ref"),
        param("int32_t", "entry_count"),
    ],
)

stable.fn(
    "Material_PropagateAlphaFlags",
    "8B 16 51 50 52 57 E8 ??",
    match=-114,
    hook=7,
    ret="void",
    params=[param("Material_Table*", "material_table")],
)

stable.fn(
    "Material_LoadAllEntries",
    "03 C2 89 46 08 57 E8 ??",
    match=-67,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "Material_Table*",
            "material_table",
            doc="Material table whose fixed-up Material_SectionHeader and 0x24-byte entry records are walked for upload/runtime initialization.",
        ),
        param(
            "int32_t",
            "texture_base_addr",
            doc="Base added to texture/palette offsets before surfaces are loaded or shared.",
        ),
    ],
    doc="Walks materialEntryCount 0x24-byte records, rebases texture/palette data, resolves shared references, uploads runtime surfaces, and honors the entry flag 0x80 path.",
)

stable.fn(
    "SceneNode_FixupMaterialRefs",
    "30 A8 01 74 0C D1 E8 ??",
    match=-34,
    ret="void",
    params=[
        param(
            "int32_t*",
            "entries",
            doc="Array of 0x14-byte entries containing tagged relative/material pointers.",
        ),
        param("int32_t", "entry_count"),
        param(
            "int32_t",
            "resource_base",
            doc="Base added to embedded material/texture offsets.",
        ),
    ],
)

stable.fn(
    "Resource_CleanupHandle",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 8B",
    required=Required.EN,
    ret="BOOL",
    params=[param("void*", "resource_data")],
    doc="Thin wrapper around Resource_FreeData for resource data pointers.",
)

stable.fn(
    "PKG_LoadEntryAlloc",
    "8B 44 24 04 6A 01 6A 00 50 E8 ??",
    hook=6,
    ret="void*",
    params=[
        param(
            "int32_t",
            "toc_index",
            doc="Package TOC index to load with destination allocation enabled.",
        )
    ],
    doc="Wrapper around PKG_LoadEntry(toc_index, NULL) that allocates destination storage for one package TOC entry and returns the loaded buffer pointer.",
)

stable.fn(
    "Level_LoadStateMachine",
    "8B 44 24 04 56 57 33 FF 8D 4C 40 24 A1 ??",
    required=Required.EN,
    ret="uint32_t*",
    params=[param("int32_t", "level_index")],
    doc=(
        "Incremental level package loader keyed by package level_index. Level resources are "
        "loaded from TOC entries 0x24 + level_index * 3 + {0,1,2}; "
        "stages 0,1,2,4,5,7 load texture A, wait, load texture B, wait, load the level blob, "
        "then fix up materials/nodes/level pointers and reset level_stream_load_state to 0."
    ),
)

stable.fn(
    "Level_UnloadResources",
    "A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 4C",
    ret="BOOL",
    params=[param("void*", "level_resource_data")],
    doc="Release the level material section and free the level resource blob plus cached level texture data buffers.",
)

stable.fn(
    "PKG_LoadEntry",
    "33 C0 5B C3 8B C7 25 ??",
    match=-20,
    ret="void*",
    params=[
        param("int32_t", "toc_index", doc="Index into package_toc."),
        param(
            "void*",
            "dest_buffer",
            doc="Optional caller-supplied destination; allocated when NULL.",
        ),
    ],
)

stable.fn(
    "Resource_FreeData",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 B0",
    ret="BOOL",
    params=[param("void*", "resource_data")],
    doc="Thin wrapper around Resource_FreeMemory for resource data pointers.",
)

stable.fn(
    "PKG_FindAndOpenFile",
    "81 EC 10 01 00 00 57 ??",
    hook=6,
    ret="int32_t",
    params=[],
    doc="Package-file lookup/open routine. EN enters directly into the shared body; EU/SC include a nearby pre-open path-resolution divergence that the sidecar patches via its game hook AOB. The SDK entry signature anchors the common function prologue.",
)

stable.fn(
    "PKG_OpenAndReadTOC",
    "?? ?? 68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8D 4C 24 0C 68 ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83",
    match=-13,
    hook=6,
    ret="BOOL",
    params=[],
    doc="Opens the located pcdogs.pkg path, stores the package file handle globally, reads the 0x800-byte package header into a temporary buffer, copies the 0x450-byte TOC to the global package table, frees the temporary buffer, and returns FALSE only when the package cannot be opened.",
)

stable.fn(
    "Pkg_Close",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 50 E8 ?? ?? ?? ?? 83 C4 04 C7 05 ?? ?? ?? ?? 00 00 00 00 C3 90 90 90 83",
    ret="File_Handle*",
    params=[],
    doc="Closes the open package file handle when present and clears pkg_file_handle.",
)

stable.fn(
    "Render_CheckActorVisibilityAndFrustum",
    "83 EC ?? 8B 54 24 14 53 55 56 8B 02 8B 4A ?? C1 F8 ??",
    hook=7,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3i32*",
            "position",
            doc="World-space fixed-point position tested against the active camera frustum.",
        ),
        param(
            "int32_t",
            "cull_radius",
            doc="Object radius/margin scaled by the frustum tests before near/far comparisons.",
        ),
        param(
            "uint8_t", "cull_flags", doc="Bit 2 bypasses the far-depth rejection path."
        ),
    ],
    doc=(
        "Tests position against the active camera frustum planes. Returns 0 when culled; "
        "otherwise returns the near-plane depth with bit 0 set and updates the edge-clipping flag."
    ),
)

stable.fn(
    "Render_FrustumCullCheck",
    "8B 44 24 0C 8B 4C 24 04 56 8B 74 24 0C 50 56 51 E8 ??",
    hook=8,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3i32*",
            "position",
            doc="World-space fixed-point position forwarded to Render_CheckActorVisibilityAndFrustum.",
        ),
        param(
            "int32_t",
            "cull_radius",
            doc="Object radius/margin used for frustum and distance checks.",
        ),
        param(
            "uint8_t",
            "cull_flags",
            doc="Flag byte forwarded to Render_CheckActorVisibilityAndFrustum.",
        ),
    ],
    doc="Visibility wrapper that toggles render-state visibility bits from Render_CheckActorVisibilityAndFrustum and returns 1 when visible.",
)

stable.fn(
    "Collision_CheckActorGround",
    "83 EC 18 53 55 8B 2D ??",
    ret="BOOL",
    params=[
        param(
            "Math_Vec3i32*",
            "position",
            doc="World-space fixed-point position tested against active ground/shadow collision planes.",
        ),
        param(
            "int32_t",
            "cull_radius",
            doc="Radius/clearance value; the function uses radius >> 2 for plane-distance thresholds.",
        ),
        param(
            "uint32_t",
            "collision_flags",
            doc="Flags/mask used to skip matching collision entries; bit 0x8000 selects the positive-depth threshold path.",
        ),
    ],
    doc="Tests a position/radius against the active collision/shadow plane list and returns nonzero when the point is rejected by the plane set.",
)

stable.fn(
    "Powerup_Cleanup",
    "56 8B 35 ?? ?? ?? ?? 85 F6 74 ?? 57 C7 05 ?? ?? 63",
    hook=7,
    ret="void",
    params=[],
)

stable.fn(
    "Powerup_CloneActor",
    "68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 83 C4 08 85",
    match=-10,
    required=Required.EN,
    hook=6,
    ret="Actor_State*",
    params=[
        param(
            "Pkg_ActorTemplate*",
            "actor_template",
            doc=(
                "Powerup actor-template/clone-source selected from "
                "Level_RuntimeData.powerup_actor_slots[0..15]; nullptr or a failed clone returns nullptr."
            ),
        )
    ],
    doc=(
        "Clones a level-owned powerup actor template into the live powerup actor list. "
        "The source template comes from the fixed 16-slot powerup_actor_slot table; "
        "spawned live actors are linked through powerup_actor_list_head."
    ),
)

stable.fn(
    "Actor_SpawnFromCollision",
    "33 C0 8A 47 0C 8B 0D ??",
    match=-24,
    hook=6,
    ret="Actor_State*",
    params=[
        param(
            "int32_t*",
            "spawn_record",
            doc="28-byte powerup spawn record containing flags, type id, local/world position, and optional parent actor pointer.",
        )
    ],
    doc="Spawns/clones the actor selected by a powerup spawn record, resolves attached/local positions when needed, initializes runtime actor flags, and stores the source record on the spawned actor.",
)

stable.fn(
    "Powerup_InitializeSystem", "8B 15 ?? ?? ?? ?? 56 85", hook=6, ret="void", params=[]
)

stable.fn(
    "Powerup_UpdateActorState",
    "4E 65 20 EB ?? 56 E8 ??",
    match=-26,
    ret="uint8_t",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Powerup_HandleCollection",
    "00 F6 47 0D 20 0F 84 ??",
    match=-12,
    required=Required.EN,
    hook=6,
    ret="uint32_t",
    params=[
        param("Actor_State*", "powerup_actor"),
        param("Actor_State*", "collector_actor"),
    ],
)

stable.fn(
    "TreeMap_InitializeNode",
    "12 8B 44 24 08 50 E8 ??",
    match=-27,
    hook=6,
    ret="void* *",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header: root node, allocation size, and compare callback.",
        ),
        param(
            "void* *",
            "node_payload",
            doc="Payload pointer returned by TreeMap_AllocateNode; the 0x14-byte node header begins immediately before it.",
        ),
    ],
    doc="Clears the hidden node header for node_payload and inserts the node into tree.",
)

stable.fn(
    "TreeMap_InsertNode",
    "56 57 8B 7C 24 0C 8B 07 85 C0 75 ?? 8B 44 24 10 89 40 ?? 89 00 89 07 5F 5E C3",
    ret="void* *",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header whose root/list links and compare callback control insertion.",
        ),
        param(
            "int32_t*",
            "node_header",
            doc="Internal 0x14-byte node header to link into the tree/list.",
        ),
    ],
    doc="Links node_header into the TreeMap circular root list, updating the root through the tree comparator when required.",
)

stable.fn(
    "TreeMap_DetachNode",
    "8B 4C 24 08 85 C9 74 ?? 8B 44 24 04 56 8B 71 ?? 8B 00 8B 50 ?? 89 70 ??",
    ret="void",
    params=[
        param("int32_t*", "tree", doc="Tree header whose root/list links are updated."),
        param(
            "void*",
            "node_header",
            doc="Internal node header to detach; nullptr is accepted as a no-op.",
        ),
    ],
    doc="Detaches/relinks a TreeMap node from the circular list headed by tree[0].",
)

stable.fn(
    "TreeMap_RemoveNode",
    "56 8D 70 EC 56 57 E8 ??",
    match=-13,
    ret="void* *",
    params=[
        param("int32_t*", "tree", doc="Tree header whose root/list links are updated."),
        param(
            "void* *",
            "node_payload",
            doc="Payload pointer for the node to remove; node metadata is stored 0x14 bytes before it.",
        ),
    ],
    doc="Removes node_payload from tree, detaches/rethreads child links, and rebalances the remaining tree when needed.",
)

stable.fn(
    "TreeMap_Rebalance",
    "8B 44 24 04 53 55 56 8B 28 57 33 DB 8B FD 8B 6D 04 0F BF 4F 10 8B 34 8D ?? ?? ?? ?? 3B F3 74 ?? 0F BF 57 10 8D 47 14 8D 4E 14 89 1C 95 ?? ?? ?? ??",
    required=Required.EN,
    ret="void* *",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header whose root chain is bucketized and rebuilt.",
        )
    ],
    doc="Rebuilds/rebalances tree using the compare callback and the temporary tree_map_buckets array.",
)

stable.fn(
    "TreeMap_RotateAndDetach",
    "8B 08 89 0A 50 53 E8 ??",
    match=-60,
    ret="int32_t*",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header passed back to TreeMap_InsertNode while rotating detached nodes.",
        ),
        param(
            "int32_t*",
            "node_header",
            doc="Internal node header pointer, 0x14 bytes before the user payload.",
        ),
    ],
    doc="Walks upward from node_header, detaches affected parent links, toggles side bits, and reinserts nodes into tree.",
)

stable.fn(
    "TreeMap_GetFirst",
    "?? 83 C6 14 56 50 E8 ??",
    match=-14,
    ret="int32_t*",
    params=[
        param(
            "int32_t*", "tree", doc="Tree header to pop from; nullptr returns nullptr."
        )
    ],
    doc="Returns and removes the first/root payload from tree, or nullptr when the tree is empty.",
)

stable.fn(
    "TreeMap_FixupAfterInsert",
    "33 C0 5B C3 57 56 E8 ??",
    match=-42,
    ret="int32_t",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header containing root, allocation size, and compare callback.",
        ),
        param(
            "void*",
            "node_payload",
            doc="Payload pointer for the newly inserted/adjusted node; returns -1 when nullptr.",
        ),
    ],
    doc="Fixes TreeMap ordering after insertion or priority update by comparing node_payload against parent/root links, detaching/reinserting when needed, and returning -1 for null payload.",
)

stable.fn(
    "TreeMap_AllocateNode",
    "8B 44 24 04 8B 48 04 51 E8 ??",
    hook=7,
    ret="void*",
    params=[
        param(
            "void*",
            "tree",
            doc="Tree header whose allocation-size field at offset 4 controls the node allocation size.",
        )
    ],
    doc="Allocates one tree node block and returns the user payload pointer at node + 0x14.",
)

stable.fn(
    "TreeMap_FreeNode",
    "8B 44 24 04 85 C0 74 ?? 83 C0 EC 50 E8 ??",
    hook=6,
    ret="BOOL",
    params=[
        param(
            "void*",
            "node_payload",
            doc="Payload pointer returned by TreeMap_AllocateNode; nullptr is accepted and returned unchanged.",
        )
    ],
    doc="Frees the full tree node allocation by subtracting the hidden 0x14-byte node header from node_payload.",
)

stable.fn(
    "TreeMap_CreateTreeNode",
    "6A 0C E8 ?? ?? ??",
    hook=7,
    ret="uint32_t*",
    params=[
        param(
            "int32_t",
            "payload_size",
            doc="Size of each user payload; the stored allocation size is payload_size + 0x14.",
        ),
        param(
            "TreeMap_CompareCallback",
            "compare_func",
            doc="Compare callback stored in the tree header and later called with two payload pointers.",
        ),
    ],
    doc="Allocates and initializes a 12-byte tree header: empty root, node allocation size, and compare callback.",
)

stable.fn(
    "TreeMap_Destroy",
    "37 8B 46 0C 50 57 E8 ??",
    match=-17,
    ret="void",
    params=[
        param(
            "int32_t*",
            "tree",
            doc="Tree header to destroy; all linked node headers are freed before the header itself.",
        )
    ],
    doc="Destroys every node in tree by detaching list links and freeing each node block, then frees the tree header.",
)

stable.fn(
    "Input_ClearEventQueue",
    "C6 05 ?? ?? ?? ?? 00 C3 90 90 90 90 90 90 90 90 A0 ?? ?? ?? ?? 84 C0 74 ?? 8B 54 24 04 55 56 57 32 C0 0F BF 32 85 F6 74 ?? C6 42 02 00 B9 ??",
    required=Required.EN,
    hook=7,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Signal_Poll",
    "74 ?? C6 42 02 00 B9 ??",
    match=-23,
    ret="int32_t",
    params=[param("int16_t*", "event_buffer"), param("int32_t", "max_events")],
)

stable.fn(
    "Timer_ClearEventList",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 83",
    hook=6,
    ret="void",
    params=[],
)

stable.fn(
    "Effect_ResetPlayback",
    "78 ?? FF 75 ?? 8B 0D ??",
    match=-23,
    hook=6,
    ret="void*",
    params=[param("Actor_State*", "actor"), param("int32_t", "chain_index")],
)

stable.fn(
    "Game_TriggerPause",
    "05 ?? ?? ?? ?? 0A A1 ??",
    match=-9,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[param("int32_t", "pause_type")],
)

stable.fn(
    "Game_UpdateAndRenderScene",
    "A0 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? E8",
    required=Required.EN,
    ret="BOOL",
    params=[],
    doc="Top-level per-frame scene/update/render driver returning native scalar status.",
)

stable.fn(
    "Game_UpdateLogic", "53 56 8B 35 ?? ?? ?? ?? 33", hook=8, ret="void", params=[]
)

stable.fn(
    "Render_ClearBackground",
    "66 81 3D ?? ?? ?? ?? 00 10 7D ?? A1 ??",
    required=Required.EN,
    hook=9,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Render_UpdateFadeCounters",
    "?? ?? ?? 83 C4 04 C3 A0",
    match=-49,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Game_RenderOverlays",
    "E8 ?? ?? ?? ?? 8B 44 24 04",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "frame_arg",
            doc="Callback/frame argument forwarded to Render_UpdateFadeCounters; the current callee ignores it.",
        )
    ],
    doc="Updates screen fade state, forwards the frame argument to the fade-counter callback path, renders UI sprites, and tail-calls the pause menu renderer.",
)

stable.fn(
    "Math_GenerateRandom",
    "8B 0D ?? ?? ?? ?? 56 8B C1",
    hook=6,
    ret="uint32_t",
    params=[
        param(
            "int32_t",
            "max_value",
            doc="Exclusive upper bound for the scaled random result.",
        )
    ],
    doc="Advances the Park-Miller MINSTD seed and returns a scaled value in [0, max_value).",
)

stable.fn(
    "Random_SetSeed",
    "8B 4C 24 04 A1 ??",
    hook=9,
    ret="uint32_t",
    params=[
        param(
            "uint32_t",
            "seed",
            doc="New Park-Miller seed; zero is normalized to 1, values above 0x7fffffff subtract 0x7fffffff once.",
        )
    ],
    doc="Stores a normalized random seed and returns the previous seed value.",
)

stable.fn(
    "Script_ExecuteBehaviorScript",
    "56 8B 74 24 08 85 F6 57 89 35 ??",
    ret="void",
    params=[param("Actor_State*", "actor")],
)

stable.fn(
    "Render_ComputeVertexColors",
    "83 EC 08 A1 ?? ?? ?? ?? 8B",
    hook=8,
    ret="void*",
    params=[
        param("int32_t", "color_shift"),
        param("Mesh_VertexColorRGB*", "input_colors"),
        param("Mesh_VertexColorRGB*", "output_colors"),
    ],
)

stable.fn(
    "Color_AdjustQuadRGB",
    "83 EC 08 A1 ?? ?? ?? ?? 33",
    hook=8,
    ret="int32_t",
    params=[
        param(
            "const uint32_t*",
            "input_quad_rgb",
            doc="Four packed 0x00BBGGRR source colors.",
        ),
        param(
            "uint32_t*",
            "output_quad_rgb",
            doc="Four packed destination colors receiving clamped RGB adjustments.",
        ),
    ],
    doc="Applies the global RGB adjustment bytes at offsets 0x14..0x16, centered at 0x80, to four packed RGB colors with per-channel 0..255 saturation.",
)

stable.fn(
    "Math_AtanToCosTransform",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 0F",
    hook=8,
    ret="int32_t",
    params=[param("int32_t", "sin_value"), param("int32_t", "cos_value")],
)

stable.fn(
    "Math_BuildQuaternionFromMatrix",
    "DF 03 C3 85 C0 0F 8E ??",
    match=-33,
    hook=6,
    ret="void",
    params=[
        param(
            "Math_Matrix3x3i16*",
            "matrix",
            doc="Input row-major signed fixed-point 3x3 rotation matrix.",
        ),
        param(
            "Math_Quaternioni16*",
            "out_quat",
            doc="Receives the converted Q14 quaternion as w/x/y/z.",
        ),
    ],
    doc="Converts a 3x3 fixed-point rotation matrix to a Q14 quaternion using the trace-positive path or largest-diagonal fallback.",
)

stable.fn(
    "Math_BuildRotationFromVectors",
    "FF FF 89 55 0C 0F 8F ??",
    match=-86,
    hook=6,
    ret="void",
    params=[
        param(
            "Math_Vec3i16*",
            "up_vector",
            doc="First normalized signed fixed-point vector.",
        ),
        param(
            "Math_Vec3i16*",
            "forward_vector",
            doc="Second normalized signed fixed-point vector.",
        ),
        param(
            "Math_Matrix3x3i16*",
            "out_matrix",
            doc="Receives the rotation matrix mapping up_vector toward forward_vector.",
        ),
    ],
    doc="Builds a Q12 rotation matrix from two 3-component vectors; emits identity for near-equal vectors and a 180-degree fallback for opposing vectors.",
)

stable.fn(
    "Audio_StopSound",
    "08 84 C0 74 ?? 56 E8 ??",
    match=-15,
    ret="int32_t",
    params=[param("int32_t", "slot_index")],
    doc="Frees the sound slot with resource cleanup enabled, then releases the Miles sample handle when the slot free succeeds.",
)

stable.fn(
    "Audio_FreeSoundSlot",
    "75 ?? 8B 48 04 89 0D ??",
    match=-46,
    hook=7,
    ret="BOOL",
    params=[param("int32_t", "slot_index"), param("BOOL", "clear_resource_flag")],
)

stable.fn(
    "Audio_PlaySoundDefinition3D",
    "56 8B 74 24 08 83 3E 00 74 ?? A1 ?? ?? ?? ??",
    ret="int32_t",
    params=[
        param("Audio_SoundDefinition*", "sound_def"),
        param(
            "Math_Vec3i32*",
            "position_or_actor_position_ptr",
            doc=(
                "Position pointer for 3D playback. Actor sound-node calls often pass &actor->position_x, but "
                "script/cutscene paths may pass non-actor globals."
            ),
        ),
    ],
    doc=(
        "Starts playback from an already-resolved Audio_SoundDefinition. Script/dialogue "
        "paths may call this directly and bypass Audio_PlayLevelSoundIndexAtPosition."
    ),
)

stable.fn(
    "Audio_AllocateSoundSlot",
    "74 ?? 6A 00 6A 08 BE ?? ?? ?? ??",
    match=-11,
    ret="int32_t",
    params=[param("Audio_SoundDefinition*", "sound_def")],
)

stable.fn(
    "Audio_FindSoundByType",
    "?? 85 DB 75 ?? 8B 35 ?? ?? ?? ??",
    match=-15,
    ret="int32_t",
    params=[
        param("int32_t", "slot_index"),
        param("Audio_SoundDefinition*", "sound_def"),
        param("Math_Vec3i32*", "position"),
    ],
)

stable.fn(
    "Audio_UpdateSoundChannels",
    "53 8B 5C 24 08 56 8B 35 ??",
    ret="void",
    params=[param("int32_t", "channel_mask")],
)

stable.fn(
    "Audio_ProcessSoundQueue", "A1 ?? ?? ?? ?? 83 EC 18", ret="int32_t", params=[]
)

stable.fn(
    "Audio_CalculateSpatialVolumeAndPan",
    "8B 45 10 85 C0 0F 84 ??",
    match=-28,
    hook=6,
    ret="uint8_t",
    params=[
        param(
            "Math_Vec3i32*",
            "source_pos",
            doc="Nullable source/world position; null treats distance and x/z delta as zero.",
        ),
        param(
            "uint32_t",
            "audible_radius",
            doc="Maximum audible distance. Sources at or beyond this radius write volume 0 and return 0.",
        ),
        param(
            "int32_t*",
            "out_volume_q12",
            doc="Optional 0..0x1000 attenuation output; null skips volume calculation.",
        ),
        param(
            "int16_t*",
            "out_pan_angle",
            doc="Output 12-bit pan/facing angle relative to player_facing_angle.",
        ),
    ],
    doc=(
        "Computes positional-audio attenuation and pan from source_pos relative to the selected listener/camera position. "
        "The listener base switches between player1_camera_pos and player2_camera_pos via input_system_flags bit 0x10."
    ),
)

stable.fn(
    "Audio_PauseAllSounds",
    "56 57 33 FF BE ?? ?? ?? ?? 57",
    hook=9,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Audio_ResumeAllSounds",
    "56 57 33 FF BE ?? ?? ?? ?? 66",
    hook=9,
    ret="int32_t",
    params=[],
)

stable.fn(
    "UI_UpdateAndRenderSprites",
    "55 8B EC 83 EC 4C A1 ??",
    required=Required.EN,
    hook=6,
    ret="void",
    params=[],
)

stable.fn(
    "ScriptCmd_AnimateRotation",
    "83 EC ?? 53 55 56 57 8B 7C 24 ?? 33 C9 33 DB 8B 2F 83 C5 ?? 8B C5 89 2F",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc="Reads camera rotation animation target/duration/easing bytes and loops by restoring *ip until the computed end tick.",
)

stable.fn(
    "ScriptCmd_AnimateZoom",
    "33 D2 8A 50 FF 8B E9 ??",
    match=-125,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_ProcessSpriteRotation",
    "55 8B EC 83 EC ?? 53 56 57 8B 7D ?? 33 C9 33 DB 8B 37 83 C6 ?? 8B C6 89 37",
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc="Reads sprite rotation animation target/duration/easing bytes, updates rotation fields at camera-entry offsets 0x84..0x98, and loops until the end tick.",
)

stable.fn(
    "ScriptCmd_AnimateTarget",
    "14 52 8D 04 50 8B 15 ??",
    match=-152,
    hook=7,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "ScriptCmd_SetCameraProperty",
    "0C 49 8D 04 48 8B 0D ??",
    match=-71,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
)

stable.fn(
    "Input_CalculateMovementVector",
    "0F 49 83 F9 09 0F 87 ??",
    match=-30,
    ret="int32_t",
    params=[
        param(
            "int32_t*",
            "out_move_vec",
            doc="Output vector with at least three int32_t components written by this helper.",
        ),
        param("int32_t", "player_index"),
        param(
            "int32_t",
            "heading_angle",
            doc=(
                "Camera-relative heading angle consumed by Math_SinCos_FP12; observed callers use the low 16 "
                "bits / 12-bit fixed-point angle domain, while the ABI remains int32_t."
            ),
        ),
    ],
    doc=(
        "Writes out_move_vec[0..2] from D-pad or analog movement input, using heading_angle in the "
        "low-16-bit / 12-bit fixed-point angle domain for Math_SinCos_FP12. The native return is "
        "ignored arithmetic status."
    ),
)

stable.fn(
    "Recording_StartPlayback",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 48",
    ret="int32_t*",
    params=[],
    doc="Starts demo playback from loaded replay data by installing the input frame pointer, saving/restoring random seed state, and setting the input replay flag.",
)

stable.fn(
    "Recording_StopPlayback",
    "8B 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 83 E2 DF 85 C0 89 15 ?? ?? ?? ?? 74 ??",
    hook=6,
    ret="void*",
    params=[],
    doc="Clears demo playback input mode, frees loaded replay data when present, clears the replay data pointer, and restores the saved random seed.",
)

stable.fn(
    "Input_Update",
    "8B 49 08 89 48 08 E8 ??",
    match=-36,
    hook=7,
    ret="int32_t",
    params=[],
    doc=(
        "Per-frame input updater: copies current/previous player input state, reads "
        "keyboard/gamepad devices, applies toggles, and handles replay input. The "
        "return register is loop/replay status ignored by the observed caller."
    ),
)

stable.fn(
    "Actor_ValidateDirectionAndProcessInput",
    "F6 C4 04 75 ?? 56 E8 ??",
    match=-17,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t", "direction_mode")],
    doc=(
        "Movement/input check for actor-local direction processing. It validates the "
        "requested direction mode against actor movement state and funnels accepted input "
        "through movement-vector processing."
    ),
)

stable.fn(
    "Render_InitDispatchTables",
    "6A 00 E8 ?? ?? ?? ?? B8",
    match=-14,
    hook=7,
    ret="int32_t",
    params=[],
    doc=(
        "Initializes render/behavior dispatch globals after setting the default 640x480 "
        "camera viewport: actor_default_update_handler, movement_handler_table, "
        "model_physics_callback_table, behavior_target_actor, and behavior_param0..2, then builds "
        "direction/collision plane tables."
    ),
)

stable.fn(
    "Actor_DefaultUpdateHandler",
    "33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 4C 24 04 8B 81 88 00 00 00 A9 00 10 08 00 74 ?? A9 00 00 02 00 0F BF 81 B8 00 00 00 74 ?? F7 D8 50 51 E8 ??",
    hook=hook(2, kind=HookKind.HOTPATCH),
    ret="int32_t",
    params=[],
    doc=(
        "Default no-op actor update callback installed into "
        "actor_default_update_handler by Render_InitDispatchTables; returns 0."
    ),
)

stable.fn(
    "Actor_ApplyVerticalMovement",
    "74 ?? F7 D8 50 51 E8 ??",
    match=-29,
    hook=10,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc="Vertical movement callback for actor runtime physics/movement state; return is native movement status.",
)

stable.fn(
    "Actor_FollowAttachedMovement",
    "F6 C4 04 75 ?? 51 E8 ??",
    match=-24,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Attached/follow movement callback for actors whose runtime transform follows "
        "another actor/component context."
    ),
)

stable.fn(
    "Actor_ProcessMovementCommands",
    "00 00 FF F7 F7 FF E8 ??",
    match=-12,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Processes actor-local movement command opcodes through the 11-entry "
        "movement_command_opcode_dispatch_table for opcode values 0..10. Per-opcode "
        "record layout and enum names stay internal."
    ),
)

stable.fn(
    "Actor_ProcessMovementBehavior",
    "E8 ?? ?? ?? ?? 8B 8E E0",
    match=-37,
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Behavior/movement callback installed by Render_InitDispatchTables. Coordinates "
        "vertical movement, animation progress, and input-validation flow for the actor; "
        "the return is native movement behavior status."
    ),
)

stable.fn(
    "String_SetTable",
    "8B 44 24 04 A3 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 54",
    required=Required.EN_SC,
    hook=9,
    ret="void*",
    params=[
        param(
            "void*",
            "string_table",
            doc="Base pointer to the loaded string table block used by String_GetByIndex.",
        ),
    ],
    doc="Stores the active string table pointer and returns the same pointer.",
)

stable.fn(
    "String_GetByIndex",
    "BA 09 04 00 00 8B 0D ??",
    match=-24,
    required=Required.EN_SC,
    hook=7,
    ret="void*",
    params=[param("int32_t", "string_index")],
)

stable.fn(
    "Spots_Initialize",
    "51 53 55 56 33 DB 57 66 89 1D ??",
    cc=CallingConvention.FASTCALL,
    hook=6,
    ret="int32_t",
    params=[param("void*", "effect_data")],
)

stable.fn(
    "Spots_Update",
    "53 55 8B 6C 24 0C 85 ED 75 ?? 66 A1 ??",
    hook=6,
    ret="void",
    params=[param("int32_t*", "effect_state")],
)

stable.fn(
    "Pkg_LoadTitleScreenResources",
    "6A 00 6A 00 E8 ?? ?? ?? ?? 83 C4 08",
    required=Required.EN,
    hook=9,
    ret="BOOL",
    params=[],
    doc="Loads title packages/materials/sound refs and returns nonzero on success.",
)

stable.fn(
    "TitleScreen_CleanupResources",
    "FF FF E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8",
    match=-5,
    hook=7,
    ret="BOOL",
    params=[],
)

stable.fn(
    "TitleScreen_UpdateAndRender",
    "8B 44 24 04 53 33 DB 83 F8 FF 0F 84 ??",
    required=Required.EN,
    ret="BOOL",
    params=[
        param(
            "void*",
            "update_token",
            doc="Normal update callers pass nullptr; (void*)-1 forces the title-screen shutdown/reset path. The stack slot is reused internally as a Render_SpriteContext* scratch.",
        ),
    ],
    doc="Advances the title-screen state machine, draws title sprites/text/spots, and returns nonzero while the title screen remains active.",
)

stable.fn(
    "Camera_UpdateProjection",
    "2C 8D 4E 30 50 51 E8 ??",
    match=-20,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera"), param("int32_t", "focal_distance")],
)

stable.fn(
    "Camera_BuildViewMatrix",
    "D1 89 45 E0 89 45 E8 ??",
    match=-74,
    hook=6,
    ret="int32_t",
    params=[
        param("Math_Matrix3x3i16*", "view_matrix"),
        param("int16_t*", "screen_half_size"),
        param("int32_t", "focal_distance"),
    ],
)

stable.fn(
    "Camera_CalculateClipDistance",
    "04 00 00 2B C6 50 E8 ??",
    match=-19,
    hook=7,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera"), param("int16_t", "fov")],
)

stable.fn(
    "Camera_SetDefaultFOV",
    "66 C7 40 0A 0F 03 E8 ??",
    match=-10,
    hook=9,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera")],
)

stable.fn(
    "Camera_SetViewport",
    "66 8B 54 24 0C 66 A3 ??",
    match=-10,
    ret="int32_t",
    params=[
        param("int16_t", "viewport_x"),
        param("int16_t", "viewport_y"),
        param("int16_t", "viewport_w"),
        param("int16_t", "viewport_h"),
        param("int32_t", "apply_projection"),
    ],
)

stable.fn(
    "Camera_LookAt",
    "?? ?? ?? 8B 4D EC 8B 55",
    match=-98,
    hook=6,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera")],
)

stable.fn(
    "Camera_Initialize",
    "00 00 00 F0 FF 7F E8 ??",
    match=-10,
    ret="int32_t",
    params=[
        param(
            "Camera_Runtime*",
            "camera",
            doc="Camera runtime initialized with default clip distance, viewport, and fade fields.",
        )
    ],
    doc=(
        "Initializes camera defaults, applies the 640x480 viewport, and clears the two "
        "fade/transition counters at +0x100D4 (PC EN)/+0x100D8 (PC EN)."
    ),
)

stable.fn(
    "Memory_MallocWithRetry",
    "53 56 57 8B 7C 24 10 57 E8 ??",
    hook=7,
    ret="void*",
    params=[param("uint32_t", "size"), param("char const*", "context")],
)

stable.fn(
    "CRT_Malloc",
    "FF 35 ?? ?? ?? ?? FF 74 24",
    hook=6,
    ret="void*",
    params=[param("uint32_t", "size")],
)

stable.fn(
    "UI_ShowConfirmDialog",
    "00 56 57 50 51 52 E8 ??",
    match=-20,
    hook=10,
    ret="BOOL",
    params=[param("char const*", "message")],
)

stable.fn(
    "Memory_AllocateHandle",
    "56 8B 35 ?? ?? ?? ?? 85 F6 57",
    hook=7,
    ret="uint32_t",
    params=[param("uint32_t", "size")],
)

stable.fn(
    "UI_ShowErrorMessage",
    "?? ?? 8D 44 24 10 50 68",
    match=-28,
    hook=10,
    ret="int32_t",
    params=[param("char const*", "message")],
)

stable.fn(
    "Memory_InitializeAllocator",
    "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8 ?? ?? ?? ?? 33",
    required=Required.EN,
    hook=10,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Memory_FreeAllExtents", "56 BE ?? ?? ?? ?? 8B 46", hook=6, ret="int32_t", params=[]
)

stable.fn(
    "Memory_ReleaseHandle",
    "8B 4C 24 04 56 49 0F 88 ??",
    ret="BOOL",
    params=[param("uint32_t", "handle")],
)

stable.fn(
    "Memory_IsValidHandle",
    "8B 44 24 04 85 C0 7E ?? 3D A0 86 01 00 7F ?? 8B C8 C1 E1 04 39 81 ?? ?? ?? ?? 75 ?? B8 01 00 00 00 C3 33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 A1 ??",
    required=Required.EN,
    hook=6,
    ret="BOOL",
    params=[param("uint32_t", "handle")],
)

stable.fn(
    "Timer_GetElapsedTickCount",
    "A1 ?? ?? ?? ?? 56 8B 35 ?? ?? ?? ?? 83",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Input_ClearState",
    "57 B9 40 00 00 00 33 C0 BF ?? ?? ?? ?? F3 AB 5F C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 A1 ??",
    required=Required.EN,
    hook=6,
    ret="void",
    params=[],
    doc="Zeros the 0x100-byte input_state_buffer raw input/VK clear buffer; native callers ignore the constant-zero native return metadata.",
)

stable.fn(
    "Input_IsKeyPressed",
    "A1 ?? ?? ?? ?? 33 C9 85 C0 53",
    ret="BOOL",
    params=[param("uint8_t", "scan_code")],
)

stable.fn(
    "Input_SetKeyUp",
    "8B 44 24 04 25 FF 00 00 00 C6 80 ?? ?? ?? ?? 00 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 0D ??",
    required=Required.EN,
    hook=9,
    ret="uint32_t",
    params=[param("uint8_t", "scan_code")],
)

stable.fn(
    "Display_IsActive",
    "8B 0D ?? ?? ?? ?? 33 C0 85 C9 0F 95",
    hook=6,
    ret="BOOL",
    params=[],
)

stable.fn(
    "Display_SetMode",
    "A1 ?? ?? ?? ?? 56 57 8B 7C",
    ret="int32_t",
    params=[param("HWND", "hwnd")],
)

stable.fn(
    "Display_ReleaseMode",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 90 90 90",
    ret="int32_t",
    params=[],
)

stable.fn(
    "Window_SetResolution", "C7 05 ?? ?? ?? ?? 80 02", hook=10, ret="int32_t", params=[]
)

stable.fn(
    "DDraw_CreateEx",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC ?? ?? ?? ?? 00 00 00 00 03 FF FF 80 00 01 00 00",
    required=Required.EN,
    cc=CallingConvention.STDCALL,
    match=-12,
    hook=6,
    ret="HRESULT",
    params=[
        param("Win32_GUID*", "lp_guid"),
        param("void* *", "lplp_dd"),
        param("Win32_GUID*", "iid"),
        param("COM_IUnknown*", "p_unk_outer"),
    ],
    doc="Import thunk for ddraw!DirectDrawCreateEx; used by graphics initialization to create the primary DirectDraw7 interface.",
)

stable.fn(
    "DirectX_DirectDrawEnumerateExA",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC ?? ?? ?? ?? 00 00 00 00 03 FF FF 80 00 01 00 00",
    cc=CallingConvention.STDCALL,
    match=-6,
    hook=6,
    ret="HRESULT",
    params=[
        param("DDraw_EnumCallbackExA", "lp_callback"),
        param("LPVOID", "lp_context"),
        param("DWORD", "dw_flags"),
    ],
    doc="Import thunk for ddraw!DirectDrawEnumerateExA; used by D3D_EnumerateDirectDrawDevices with DDraw_EnumerateCallback and enumeration flags.",
)

stable.fn(
    "DirectX_DirectInputCreateA",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC",
    cc=CallingConvention.STDCALL,
    hook=6,
    ret="HRESULT",
    params=[
        param("HINSTANCE", "hinst"),
        param("DWORD", "dw_version"),
        param("DInput_IDirectInputA**", "pp_di"),
        param("COM_IUnknown*", "p_unk_outer"),
    ],
    doc="Import thunk for dinput!DirectInputCreateA; used by DInput_CreateInterface to create the DirectInput 7 interface.",
)

stable.fn(
    "Movie_SetSyncAdjust",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=16,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetSoundRate",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=22,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetSoundPrecision",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=28,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetSoundChannels",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=34,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetXSize",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=40,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetYSize",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=46,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetCurrentFrame",
    "08 D5 A8 01 FF 25 ?? ??",
    match=-8,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_GetTotalFrames",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitSoundSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitVideoSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ShutdownMovie",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ShutdownVideo",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ShutdownSound",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ReturnPlaybackMode",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_StopTimer",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_StartTimer",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_MapVideo",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitMoviePlayback",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitSound",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitPlaybackMode",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitVideo",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_InitMovie",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_PlayFrame",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-7,
    required=Required.EN,
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ShutdownSoundSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Player_ShutdownVideoSystem",
    "FF 25 ?? ?? ?? ?? E8",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "String_ParseInt",
    "53 55 56 57 8B 7C 24 14 83 3D ?? ?? ?? ?? 01 7E",
    ret="int32_t",
    params=[param("char const*", "text")],
    doc="CRT atol-style signed decimal parser for a NUL-terminated text string.",
)

stable.fn(
    "String_Atoi",
    "FF 74 24 04 E8 ?? ?? ?? ?? 59 C3",
    hook=9,
    ret="int32_t",
    params=[param("char const*", "text")],
    doc="Thin atoi wrapper around String_ParseInt.",
)

stable.fn(
    "File_FlushBuffer",
    "?? 57 50 FF 76 10 E8 ??",
    match=-38,
    hook=6,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
)

stable.fn(
    "File_Close",
    "?? A8 83 74 ?? 56 E8 ??",
    match=-20,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
)

stable.fn(
    "File_OpenWithMode",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? C3",
    ret="File_Handle*",
    params=[
        param("char const*", "filename"),
        param("char const*", "mode"),
        param("uint8_t", "sharing_flag"),
    ],
)

stable.fn(
    "File_Open",
    "0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3 55 8B EC 81 EC F8 00 00 00 53 56 8B 75",
    match=-5,
    hook=6,
    ret="File_Handle*",
    params=[param("char const*", "filename"), param("char const*", "mode")],
)

stable.fn(
    "Sort_QuickSort",
    "55 8B EC 81 EC F8 00 00 00 53 56 8B 75 0C 57 83 FE 02 0F 82 ?? ?? ?? ??",
    hook=9,
    ret="void",
    params=[
        param("void*", "base"),
        param("uint32_t", "count"),
        param("uint32_t", "element_size"),
        param(
            "Sort_CompareCallback",
            "compare",
            doc="Comparator callback used by the CRT qsort implementation.",
        ),
    ],
    doc="CRT qsort-style quicksort over count elements of elementSize bytes using the comparator callback.",
)

stable.fn(
    "Sort_InsertionSort",
    "55 8B EC 8B 45 08 57 8B 7D 0C 3B F8 76 ?? 8B 4D 10 53 03 C1 56 89 45 0C",
    ret="char*",
    params=[
        param("char*", "first_element"),
        param("char*", "last_element"),
        param("uint32_t", "element_size"),
        param("Sort_CompareCallback", "compare"),
    ],
    doc="Insertion-sort helper used by Sort_QuickSort for small partitions; returns a residual element/swap cursor ignored by the qsort caller.",
)

stable.fn(
    "Memory_SwapBytes",
    "8B 44 24 04 8B 4C 24 08 3B C1 56 74 ?? 8B 54 24 10 8B F2 4A 85 F6 74 ??",
    ret="void",
    params=[
        param("char*", "left"),
        param("char*", "right"),
        param("uint32_t", "byte_count"),
    ],
    doc="Swaps byteCount bytes between two element buffers.",
)

stable.fn(
    "File_SeekAndGetPosition",
    "5F 04 6A 01 53 56 E8 ??",
    match=-26,
    hook=6,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
)

stable.fn(
    "File_WriteElements",
    "8B DF 75 ?? 33 C0 E9 ??",
    match=-25,
    ret="uint32_t",
    params=[
        param("void const*", "buffer"),
        param("uint32_t", "size"),
        param("uint32_t", "count"),
        param("File_Handle*", "stream"),
    ],
    doc="fwrite-like buffered writer: writes count elements of size bytes from buffer to stream and returns the element count written.",
)

stable.fn(
    "Memory_Realloc",
    "75 ?? FF 74 24 18 E8 ??",
    match=-10,
    ret="void*",
    params=[param("void*", "ptr"), param("uint32_t", "size")],
)

stable.fn(
    "Float_CheckPrecision",
    "55 8B EC 83 EC 18 DD 05 ??",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Float_LoadFPU",
    "68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 85 C0 74",
    ret="int32_t",
    params=[],
)

stable.fn(
    "String_FormatFloat",
    "65 59 74 ?? 46 83 3D ??",
    match=-16,
    ret="void",
    params=[
        param(
            "char*",
            "buffer",
            doc="NUL-terminated float string buffer modified in place.",
        )
    ],
    doc="CRT _forcdecpt-style helper: inserts the locale decimal-point character before the exponent/non-digit suffix by shifting the remainder right.",
)

stable.fn(
    "Float_ConvertToExponential",
    "55 8B EC 80 3D ??",
    hook=10,
    ret="char*",
    params=[
        param("double*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param(
            "int32_t",
            "precision",
            doc="Number of fractional digits before the exponent suffix.",
        ),
        param(
            "int32_t",
            "uppercase",
            doc="Nonzero selects an uppercase E exponent marker.",
        ),
    ],
    doc="Formats value into scientific notation using the shared CRT float state, rounding digits and appending e/E+000.",
)

stable.fn(
    "Float_ConvertToFixed",
    "80 3D ?? ?? ?? ?? 00 53 55",
    hook=7,
    ret="char*",
    params=[
        param("double*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "precision", doc="Number of digits after the decimal point."),
    ],
    doc="Formats value into fixed-point notation using the shared CRT float state and returns buffer.",
)

stable.fn(
    "Float_ConvertGeneral",
    "51 DD 07 DD 1C 24 E8 ??",
    match=-10,
    ret="char*",
    params=[
        param("double*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "precision", doc="Requested significant-digit precision."),
        param(
            "int32_t",
            "uppercase",
            doc="Nonzero selects an uppercase E exponent marker if scientific notation is used.",
        ),
    ],
    doc="CRT general-format conversion: chooses fixed or scientific notation from the decimal exponent and returns buffer.",
)

stable.fn(
    "Float_FormatScientific",
    "24 10 E8 ?? ?? ?? ?? 80",
    match=-21,
    hook=11,
    ret="char*",
    params=[
        param("double*", "value"),
        param("char*", "buffer"),
        param("int32_t", "precision"),
        param("int32_t", "uppercase"),
    ],
    doc="Scientific-format helper that forwards value, buffer, precision, and uppercase selector into the CRT float conversion path.",
)

stable.fn(
    "Float_ConvertToFixedWrapper",
    "24 0C E8 ?? ?? ?? ?? 80",
    match=-17,
    hook=11,
    ret="char*",
    params=[
        param("double*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "precision", doc="Number of digits after the decimal point."),
    ],
    doc="Sets the shared fixed-format flag, calls Float_ConvertToFixed, and returns buffer.",
)

stable.fn(
    "String_InsertSpace",
    "56 8B 74 24 0C 56 E8 ??",
    match=-9,
    ret="void",
    params=[
        param(
            "char*",
            "buffer",
            doc="NUL-terminated string whose tail is shifted right in place.",
        ),
        param(
            "int32_t",
            "insert_count",
            doc="Number of bytes to make room for at buffer; zero leaves the string unchanged.",
        ),
    ],
    doc="Shifts the NUL-terminated string right by insertCount bytes using a backward memmove.",
)

stable.fn(
    "Char_GetCharacterType",
    "01 00 00 77 ?? 8B 0D ??",
    match=-13,
    hook=7,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "ch",
            doc="Character code; multibyte characters are handled as a packed low-word value.",
        ),
        param("int32_t", "mask", doc="CRT ctype mask to test."),
    ],
    doc="CRT _isctype-style helper: returns mask bits for ch using the ctype table or __crtGetStringTypeA for multibyte characters.",
)

stable.fn(
    "File_WriteChar",
    "8B 5E 10 A8 82 0F 84 ??",
    match=-11,
    ret="int32_t",
    params=[param("int32_t", "ch"), param("File_Handle*", "stream")],
    doc="fputc-like writer: writes ch to stream, flushing or filling the stream buffer as needed; returns the written byte or -1 on failure.",
)

stable.fn(
    "File_WriteCharWithCounter",
    "EB ?? 51 FF 75 08 E8 ??",
    match=-23,
    hook=6,
    ret="int32_t*",
    params=[
        param("int32_t", "ch"),
        param("File_Handle*", "stream"),
        param("int32_t*", "written_count"),
    ],
    doc="Buffered putc helper: writes ch to stream, increments *writtenCount on success, or sets it to -1 on failure; returns writtenCount.",
)

stable.fn(
    "Exception_CaptureContext",
    "53 51 BB ?? ?? ?? ?? 8B",
    cc=CallingConvention.STDCALL,
    callable=False,
    hook=7,
    ret="void",
    params=[
        param(
            "int32_t",
            "notification_code",
            doc="Stack-passed NLG notification code popped by ret 4. The helper also consumes EAX as the handler/continuation address and EBP as the establisher frame.",
        ),
    ],
    doc="MSVC _NLG_Notify-style unwind helper; saves EAX, EBP, and *(EBP+8) into the CRT NLG context. EAX and EBP are implicit inputs, while the stack notification code is popped by ret 4.",
)

stable.fn(
    "Exception_IsAccessViolation",
    "8B 45 EC 8B 00 8B 00 33 C9 3D 05 00 00 C0 0F 94 C1 8B C1 C3 8B 65 E8 ??",
    callable=False,
    ret="BOOL",
    params=[
        param(
            "void*",
            "exception_frame",
            doc="Implicit EBP-framed SEH filter state containing exception pointers at -0x14 (PC EN).",
        )
    ],
    doc="Non-normal-callable SEH filter fragment inside _rt_probe_read4; returns true when the exception code is 0xC0000005 (access violation).",
)

stable.fn(
    "File_FlushToDisk",
    "8B 44 24 04 3B 05 ?? ?? ?? ?? 73 ?? 8B C8 8B",
    hook=10,
    ret="int32_t",
    params=[param("int32_t", "file_no")],
    doc="Flushes the OS handle for a CRT file descriptor with FlushFileBuffers; returns 0 on success and -1 on invalid descriptor or Win32 failure.",
)

stable.fn(
    "File_WriteBytes",
    "55 8B EC 81 EC 14 04 00 00 8B 4D 08 53 3B 0D ??",
    hook=9,
    ret="int32_t",
    params=[
        param("int32_t", "file_no"),
        param("void const*", "buffer"),
        param("int32_t", "byte_count"),
    ],
)

stable.fn(
    "File_AllocateBuffer",
    "56 8B 74 24 08 FF 76 10 E8 ??",
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
    doc="Allocates or assigns a buffered I/O area for stdout/stderr-like streams; returns 1 when a buffer is installed, otherwise 0.",
)

stable.fn(
    "File_FlushAndClear",
    "46 0D 10 74 ?? 56 E8 ??",
    match=-13,
    ret="void",
    params=[param("int32_t", "clear_buffer"), param("File_Handle*", "stream")],
    doc="Flushes a buffered stream when needed; when clearBuffer is nonzero, also clears the stream buffer pointers and count.",
)

stable.fn(
    "File_CloseHandle",
    "53 55 56 57 8B 7C 24 14 3B 3D ??",
    hook=8,
    ret="int32_t",
    params=[param("int32_t", "file_no")],
    doc="Closes the OS handle associated with a CRT file descriptor and clears its descriptor-table flags; returns 0 on success or -1 on failure.",
)

stable.fn(
    "Heap_InitializeAllocator",
    "68 40 01 00 00 6A 00 FF 35 ??",
    ret="int32_t",
    params=[],
    doc="Allocates the custom small-block heap segment table, clears the last-freed segment cache, and initializes segment counters.",
)

stable.fn(
    "Heap_FindBlockByAddress",
    "A1 ?? ?? ?? ?? 8D 0C 80",
    ret="void*",
    params=[
        param(
            "void*",
            "address",
            doc="Allocation address to locate within the custom heap's 1 MiB segments.",
        )
    ],
    doc="Scans the 0x14-byte heap segment table and returns the segment entry whose base contains address, or NULL when not owned by the custom heap.",
)

stable.fn(
    "Heap_FreeBlock",
    "C2 FC 57 C1 EE 0F 8B ??",
    match=-26,
    hook=6,
    ret="void",
    params=[
        param(
            "void*",
            "heap_segment",
            doc="0x14-byte heap segment entry returned by Heap_FindBlockByAddress.",
        ),
        param("void*", "block", doc="Allocated small-block pointer to release."),
    ],
    doc="Frees a custom small-block allocation, coalesces adjacent free blocks, updates size-class bitmaps, and releases empty pages/segments back to the OS.",
)

stable.fn(
    "CRT_CopyString",
    "8B 4C 24 04 57 F7 ?? 03 00 00 00 74 ?? 8A ??",
    ret="char*",
    params=[param("char*", "dest"), param("char const*", "src")],
)

stable.fn(
    "Float_RoundAndCopyDigits",
    "10 40 FF 4D 08 75 ?? ??",
    match=-51,
    hook=6,
    ret="char*",
    params=[
        param(
            "char*",
            "out_digits",
            doc="Destination digit buffer; initialized with a leading '0'.",
        ),
        param(
            "int32_t",
            "digit_count",
            doc="Number of digits to copy and round from the CRT float state.",
        ),
        param(
            "void*",
            "float_state",
            doc="CRT float conversion state; +4 is adjusted when rounding carries and +0x0C (PC EN) points at source digits.",
        ),
    ],
    doc="Copies digit_count decimal digits from the CRT float state, rounds on the next digit, shifts away the leading guard zero, and bumps the decimal exponent on carry.",
)

stable.fn(
    "Float_ConvertToDecimalString",
    "C6 45 E7 CC C6 45 E8 ??",
    match=-45,
    hook=6,
    ret="int32_t",
    params=[
        param(
            "int16_t",
            "exponent_sign",
            doc="x87 80-bit exponent/sign word; sign bit selects leading space versus minus.",
        ),
        param(
            "int32_t",
            "digit_count",
            doc="Requested decimal digit count, adjusted by exponent when flags bit 0 is set.",
        ),
        param(
            "char",
            "flags",
            doc="Conversion flags; bit 0 makes digit_count relative to the computed decimal exponent.",
        ),
        param(
            "char*",
            "out_decimal",
            doc="Output decimal-record buffer receiving exponent, sign, digit length, digits, and terminator.",
        ),
    ],
    doc="Converts the split x87 extended-precision value on the stack into the CRT decimal record, handling zero, INF/IND/QNAN/SNAN, rounding, and digit trimming.",
)


stable.fn(
    "RtlUnwind",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC 55",
    hook=6,
    ret="int32_t",
    params=[],
)

stable.data("max_fps_threshold", xref("Render_AdjustLevelScale", 50, 2), type="int32_t")


# Function promoted with the data rows that use it as an xref resolver.
stable.fn(
    "ScriptCmd_ConditionalBranch",
    "0B DA 80 F9 06 0F 84 ??",
    match=-104,
    ret="void",
    params=[param("Actor_State*", "actor"), param("uint8_t* *", "ip")],
    doc=(
        "Decodes a conditional branch bytecode record, resolves one or two operands through "
        "Script_ResolveVariableRef/random/button-state helpers, applies optional arithmetic, and "
        "advances *ip to the branch target when the selected comparison fails."
    ),
)

stable.data(
    "ddraw_object",
    xref("D3D_CreateTextureSurface", 726, 1),
    type="DDraw_IDirectDraw7*",
    doc="Primary IDirectDraw7 interface used for texture/work/z-buffer surface creation and released during DirectDraw shutdown.",
)
stable.data(
    "game_initialized",
    xref("Game_WindowProc", 95, 1),
    type="int32_t",
    doc="Non-zero after the main game window and runtime initialization have completed.",
)
stable.data(
    "joystick_available",
    xref("Input_GetPressedButton", 31, 1),
    type="uint8_t",
    doc="Non-zero when joystick/gamepad input is available; allows gamepad polling in Input_GetPressedButton.",
)
stable.data(
    "main_window_handle",
    xref("D3D_InitDirectDrawAndDirect3D", 5, 2),
    type="HWND",
    doc="Primary game window handle captured during DirectDraw/Direct3D initialization.",
)
stable.data(
    "main_window_handle_2",
    xref("WinMain", 315, 1),
    type="HWND",
    doc=(
        "Write-only secondary copy of the HWND returned by CreateWindowExA in WinMain; "
        "main_window_handle is the runtime window handle read by input/movie/DirectDraw paths."
    ),
)
stable.data(
    "rendering_enabled",
    xref("WinMain", 355, 2),
    type="int32_t",
    doc="Flag checked by game rendering paths before drawing world content.",
)
stable.data(
    "should_quit",
    xref("Input_ProcessWindowMessages", 0, 1),
    type="int32_t",
    doc="Flag set by window-message processing for game exit.",
)
stable.data(
    "audio_digital_driver",
    xref("Audio_InitializeSystem", 127, 1),
    type="Audio_AIL_HDigitalDriver",
    doc="Miles digital driver handle opened by AIL_waveOutOpen and cleared by Audio_ShutdownSystem.",
)
stable.data(
    "movie_file_names",
    xref("Movie_PlayIntro", 15, 3),
    doc="First entry/base of the four-entry movie filename pointer table used by intro and movie playback routines.",
)
stable.data(
    "movie_path_prefix",
    xref("Movie_PlayIntro", 23, 1),
    type="char",
    doc="First byte/base of the NUL-terminated data/movies path prefix used by movie-loading routines.",
)
stable.data(
    "title_bonus_replay_resource",
    xref("TitleScreen_CleanupResources", 12, 1),
    type="void*",
    doc=(
        "Title-screen bonus replay resource pointer freed during "
        "TitleScreen_CleanupResources and assigned/used by title-screen load/update paths."
    ),
)
stable.data(
    "title_resource_handle_1",
    xref("TitleScreen_CleanupResources", 23, 2),
    type="void*",
    doc=(
        "Title-screen resource handle slot 1 cleaned by Resource_CleanupHandle during "
        "TitleScreen_CleanupResources."
    ),
)
stable.data(
    "title_resource_handle_0",
    xref("TitleScreen_CleanupResources", 35, 2),
    type="void*",
    doc=(
        "Title-screen resource handle slot 0 cleaned by Resource_CleanupHandle during "
        "TitleScreen_CleanupResources."
    ),
)
stable.data(
    "title_material_base",
    xref("TitleScreen_CleanupResources", 47, 1),
    type="void*",
    doc=(
        "Title-screen material/resource manager base released by Resource_ReleaseManager "
        "during TitleScreen_CleanupResources."
    ),
)
stable.data(
    "title_resource_package",
    xref("TitleScreen_CleanupResources", 58, 2),
    type="void*",
    doc=(
        "Title-screen resource package pointer freed by Resource_FreeData during "
        "TitleScreen_CleanupResources."
    ),
)
stable.data(
    "scene_node_type_dispatch_table",
    xref("Scene_TraverseNodeTree", 890, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Scene_TraverseNodeTree to dispatch child scene-node type values 1..7 from node +0x64 (PC EN); node_type 3..5 share target pcdogs.exe+0x016D2 (PC EN) and out-of-range values default to pcdogs.exe+0x016D2 (PC EN).",
)
stable.data(
    "input_control_code_dispatch_table",
    xref("Input_CheckButtonState", 130, 3),
    type="uint32_t",
    doc=(
        "Ten-entry uint32_t jump table used by Input_CheckButtonState for control codes 0x20..0x47. "
        "Slots 0..8 handle aggregate direction axes and signed axis thresholds; slot 9 is "
        "the default return-zero path for unused codes 0x24..0x3f."
    ),
)
stable.data(
    "input_control_code_dispatch_index_table",
    xref("Input_CheckButtonState", 124, 2),
    type="uint8_t",
    doc=(
        "0x28-byte uint8_t lookup table mapping control-code offsets 0x20..0x47 onto "
        "input_control_code_dispatch_table slots."
    ),
)
stable.data(
    "actor_path_target_selector_dispatch_table",
    xref("Actor_TracePath", 704, 3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Actor_TracePath for negative target-selector sentinel values -0x8000..-0x7FF7.",
)
stable.data(
    "actor_property_id_dispatch_table",
    xref("Actor_SetProperty", 62, 3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Actor_SetProperty for property ids 0..9.",
)
stable.data(
    "player_behavior_state_dispatch_table",
    xref("Actor_ProcessPlayerBehavior", 623, 3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Actor_ProcessPlayerBehavior for player behavior state values 0..4.",
)
stable.data(
    "collision_response_node_type_dispatch_table",
    xref("Actor_ProcessCollisionResponse", 335, 3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Actor_ProcessCollisionResponse for collided actor/node type values 0..4.",
)
stable.data(
    "script_terminator_opcode_table",
    xref("Script_CheckTerminator", 26, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table for Script_CheckTerminator opcodes 1 through 4.",
)
stable.data(
    "script_branch_arithmetic_op_table",
    xref("ScriptCmd_ConditionalBranch", 267, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by ScriptCmd_ConditionalBranch for arithmetic/combine opcodes before comparison.",
)
stable.data(
    "script_branch_comparison_op_table",
    xref("ScriptCmd_ConditionalBranch", 391, 3),
    type="uint32_t",
    doc="Six-entry uint32_t jump table used by ScriptCmd_ConditionalBranch for comparison opcodes.",
)
stable.data(
    "script_set_variable_op_jump_table",
    xref("Script_SetVariable", 273, 3),
    type="uint32_t",
    doc=(
        "Four-entry uint32_t jump table used by Script_SetVariable for arithmetic opcode dispatch; "
        "validated as data."
    ),
)
stable.data(
    "checkers_capture_piece_type_dispatch_table",
    xref("Checkers_CheckCapturePossible", 32, 3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t jump table for Checkers_CheckCapturePossible piece values 1..6. "
        "Pieces 1/2 are men, 5/6 are kings, and 3/4 fall through to the no-capture path."
    ),
)
stable.data(
    "checkers_simple_move_piece_type_dispatch_table",
    xref("Checkers_ValidateMove", 104, 3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t Checkers_ValidateMove jump table for one-square moves by piece value "
        "1..6; pieces 3/4 share the invalid/default path and 5/6 share king movement."
    ),
)
stable.data(
    "checkers_capture_move_piece_type_dispatch_table",
    xref("Checkers_ValidateMove", 271, 3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t Checkers_ValidateMove jump table for two-square captures by piece value "
        "1..6; pieces 3/4 share the invalid/default path and 5/6 share king capture logic."
    ),
)
stable.data(
    "mesh_command_signal_dispatch_table",
    xref("Render_UpdateMeshCommandFlags", 119, 3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Render_UpdateMeshCommandFlags for mesh command signal ids 0..10.",
)
stable.data(
    "d3d_error_88760028_range_dispatch_table",
    xref("D3D_FormatDirectXError", 223, 3),
    type="uint32_t",
    doc="Eight-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760028..0x88760078 DirectDraw error range.",
)
stable.data(
    "d3d_error_88760028_range_index_table",
    xref("D3D_FormatDirectXError", 217, 2),
    type="uint8_t",
    doc="0x51-byte uint8_t lookup table that maps sparse 0x88760028..0x88760078 HRESULT offsets to D3D_FormatDirectXError jump-table slots.",
)
stable.data(
    "d3d_error_88760091_range_dispatch_table",
    xref("D3D_FormatDirectXError", 332, 3),
    type="uint32_t",
    doc="Fifteen-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760091..0x887600E1 DirectDraw error range.",
)
stable.data(
    "d3d_error_88760091_range_index_table",
    xref("D3D_FormatDirectXError", 326, 2),
    type="uint8_t",
    doc="0x51-byte uint8_t lookup table that maps sparse 0x88760091..0x887600E1 HRESULT offsets to D3D_FormatDirectXError jump-table slots.",
)
stable.data(
    "d3d_error_887600f0_range_dispatch_table",
    xref("D3D_FormatDirectXError", 513, 3),
    type="uint32_t",
    doc="Thirty-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x887600F0..0x887601D6 DirectDraw error range.",
)
stable.data(
    "d3d_error_887600f0_range_index_table",
    xref("D3D_FormatDirectXError", 507, 2),
    type="uint8_t",
    doc="0xE7-byte uint8_t lookup table that maps sparse 0x887600F0..0x887601D6 HRESULT offsets to D3D_FormatDirectXError jump-table slots.",
)
stable.data(
    "d3d_error_887601ea_range_dispatch_table",
    xref("D3D_FormatDirectXError", 910, 3),
    type="uint32_t",
    doc="Thirty-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x887601EA..0x88760245 DirectDraw error range.",
)
stable.data(
    "d3d_error_887601ea_range_index_table",
    xref("D3D_FormatDirectXError", 904, 2),
    type="uint8_t",
    doc="0x5C-byte uint8_t lookup table that maps sparse 0x887601EA..0x88760245 HRESULT offsets to D3D_FormatDirectXError jump-table slots.",
)
stable.data(
    "d3d_error_88760247_range_dispatch_table",
    xref("D3D_FormatDirectXError", 1274, 3),
    type="uint32_t",
    doc="Sixteen-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760247..0x8876026C DirectDraw error range.",
)
stable.data(
    "d3d_error_88760247_range_index_table",
    xref("D3D_FormatDirectXError", 1268, 2),
    type="uint8_t",
    doc="0x26-byte uint8_t lookup table that maps sparse 0x88760247..0x8876026C HRESULT offsets to D3D_FormatDirectXError jump-table slots; 0x88760276 is handled as a separate singleton.",
)
stable.data(
    "d3d_error_88760280_range_dispatch_table",
    xref("D3D_FormatDirectXError", 1495, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760280..0x887602B4 DirectDraw error range.",
)
stable.data(
    "d3d_error_88760280_range_index_table",
    xref("D3D_FormatDirectXError", 1489, 2),
    type="uint8_t",
    doc="0x35-byte uint8_t lookup table that maps sparse 0x88760280..0x887602B4 HRESULT offsets to D3D_FormatDirectXError jump-table slots.",
)
stable.data(
    "d3d_error_887602b6_range_dispatch_table",
    xref("D3D_FormatDirectXError", 1567, 3),
    type="uint32_t",
    doc="Six-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the contiguous 0x887602B6..0x887602BB DirectDraw error range.",
)
stable.data(
    "bonus_level_code_dispatch_table",
    xref("Level_InitializeBonusData", 39, 3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Level_InitializeBonusData for bonus level ids 27..31.",
)
stable.data(
    "level_completion_slot_dispatch_table",
    xref("Level_BuildCompletionTable", 376, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Level_BuildCompletionTable to store four packed completion masks.",
)
stable.data(
    "options_menu_render_item_dispatch_table",
    xref("Menu_RenderOptionsMenu", 259, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Menu_RenderOptionsMenu to render options menu rows 0..6.",
)
stable.data(
    "menu_state_dispatch_table",
    xref("Menu_ProcessMenuState", 177, 3),
    type="uint32_t",
    doc="Thirteen-entry uint32_t jump table used by Menu_ProcessMenuState for menu state values 1..13.",
)
stable.data(
    "level_load_state_dispatch_table",
    xref("Level_Load", 16, 3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Level_Load for level-loading state values 0..10; state 9 maps to the idle/default return path.",
)
stable.data(
    "projectile_hit_node_type_dispatch_table",
    xref("Collision_ProcessProjectileHit", 93, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_ProcessProjectileHit for hit actor/node type values 1..4.",
)
stable.data(
    "collision_3d_axis_dispatch_table",
    xref("Collision_DetectAndResolve3DCollision", 3579, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_DetectAndResolve3DCollision to select one of four collision-normal/contact axes.",
)
stable.data(
    "object_node_collision_axis_dispatch_table",
    xref("Collision_ResolveObjectNodeCollision", 346, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_ResolveObjectNodeCollision to select one of four object-node collision axes.",
)
stable.data(
    "collision_condition_subtype_dispatch_table",
    xref("Actor_CheckCollisionConditions", 39, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Actor_CheckCollisionConditions after remapping collision subtype values 0x0D..0x17.",
)
stable.data(
    "collision_condition_subtype_index_table",
    xref("Actor_CheckCollisionConditions", 33, 2),
    type="uint8_t",
    doc="0x0B-byte uint8_t lookup table mapping collision subtype values 0x0D..0x17 onto collision_condition_subtype_dispatch_table slots; max slot is 3.",
)
stable.data(
    "collision_condition_flag_mask_dispatch_table",
    xref("Actor_CheckCollisionConditions", 291, 3),
    type="uint32_t",
    doc="Eight-entry uint32_t jump table used by Actor_CheckCollisionConditions to test selector values 2..9 against masks 0x10,0x20,0x40,0x80,1,2,4,8.",
)
stable.data(
    "collision_response_subtype_dispatch_table",
    xref("Actor_HandleCollisionResponse", 34, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Actor_HandleCollisionResponse for collision subtype values 0x0D..0x13.",
)
stable.data(
    "input_button_mask_dispatch_table",
    xref("Input_GetButtonIndex", 25, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table mapping low input button bitmasks 1,2,4,8,0x10,0x20 through input_button_mask_index_table; slot 6 is the default unrecognized-mask path.",
)
stable.data(
    "input_button_mask_index_table",
    xref("Input_GetButtonIndex", 19, 2),
    type="uint8_t",
    doc="0x20-byte uint8_t lookup table for Input_GetButtonIndex masks 1..0x20; larger recognized masks are handled by direct compares.",
)
stable.data(
    "window_low_message_dispatch_table",
    xref("Game_WindowProc", 64, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Game_WindowProc for sparse low Win32 messages 0x02..0x10, including destroy/size/close handling; WM_KEYDOWN (0x100) is handled by a direct branch.",
)
stable.data(
    "window_low_message_index_table",
    xref("Game_WindowProc", 58, 2),
    type="uint8_t",
    doc="0x0f-byte uint8_t lookup table mapping Win32 message IDs 0x02..0x10 to window_low_message_dispatch_table slots.",
)
stable.data(
    "window_high_message_dispatch_table",
    xref("Game_WindowProc", 361, 3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Game_WindowProc for Win32 messages 0x101..0x112, including WM_KEYUP, system key messages, and WM_SYSCOMMAND filtering.",
)
stable.data(
    "window_high_message_index_table",
    xref("Game_WindowProc", 355, 2),
    type="uint8_t",
    doc="0x12-byte uint8_t lookup table mapping sparse high Win32 message IDs 0x101..0x112 to window_high_message_dispatch_table slots.",
)
stable.data(
    "ui_sprite_anchor_dispatch_table",
    xref("UI_UpdateAndRenderSprites", 1259, 3),
    type="uint32_t",
    doc="Eight-entry uint32_t jump table used by UI_UpdateAndRenderSprites for sprite anchor codes 1..8.",
)
stable.data(
    "movement_direction_dispatch_table",
    xref("Input_CalculateMovementVector", 41, 3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Input_CalculateMovementVector to map low-nibble direction masks to heading offsets; kept as read-only scalar/base table metadata.",
)
stable.data(
    "movement_command_opcode_dispatch_table",
    xref("Actor_ProcessMovementCommands", 79, 3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Actor_ProcessMovementCommands for movement command opcodes 0..10; kept as read-only scalar/base table metadata.",
)
stable.data(
    "title_screen_state_dispatch_table",
    xref("TitleScreen_UpdateAndRender", 32, 3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by TitleScreen_UpdateAndRender for title-screen state values 0..6.",
)
stable.data("max_gamma_clamp", xref("Render_AdjustLevelScale", 33, 2))
stable.data("rhw_depth_mul_2", xref("Level_InitializeActorSystem", 417, 2))
stable.data("degenerate_tri_area", xref("Level_InitializeActorSystem", 350, 2))
stable.data("camera_init_dist", xref("Level_InitializeActorSystem", 342, 2))
stable.data("z_depth_scale", xref("Render_QuadClipped", 1677, 2))
stable.data(
    "one",
    xref("Render_TexturedQuad", 194, 2),
    xref("DInput_SetConstantForceEffect", 23, 2),
)
stable.data(
    "zero",
    xref("Render_ClipPolygonByPlane", 279, 2),
    xref("Video_OpenMovieFile", 27, 2),
)
stable.data("fov_angle_scale", xref("Camera_SetupClipPlanes", 23, 2))
stable.data("fps_update_interval", xref("Math_SnapVertexToNearestPoint", 105, 2))
stable.data("bottom_edge_clamp", xref("Render_QuadClipped", 1881, 2))
stable.data("right_edge_clamp", xref("Render_QuadClipped", 1836, 2))
stable.data("max_z_depth_clamp", xref("Render_QuadClipped", 1687, 2))
stable.data(
    "project_screen_height_half",
    xref("Render_QuadClipped", 1622, 2),
    xref("Render_DrawQuad", 2772, 2),
)
stable.data("project_screen_width_half", xref("Render_QuadClipped", 1585, 2))
stable.data("neg_z_bias", xref("Render_DrawQuad", 2764, 2))
stable.data("alt_uv_offset", xref("Render_DrawQuad", 609, 2))
stable.data("fixed_to_float", xref("Camera_SetupProjection", 161, 2))
stable.data("aspect_correction", xref("Camera_SetupProjection", 96, 2))
stable.data("debug_pos_scale", xref("Camera_SetupProjection", 54, 2))
stable.data(
    "window_width",
    xref("Render_PolygonBatch", 6219, 2),
    xref("D3D_InitDirectDrawAndDirect3D", 92, 2),
    type="int32_t",
    doc="Active render/window width in pixels; initialized during DirectDraw/Direct3D setup and read by render projection/batching paths.",
)
stable.data("min_fog_dist", xref("Render_PolygonBatch", 6193, 2))
stable.data("ms_to_sec", xref("Timer_GetGameTime", 25, 2))
stable.data("game_over_fade_mul", xref("Game_HandleGameOver", 19, 2))
stable.data("gamma_step", xref("D3D_SetFogDistance", 58, 2))
stable.data("default_gamma", xref("D3D_SetFogDistance", 4, 2))
stable.data(
    "gamepad_axis_scale",
    xref("Input_ReadGamepad", 219, 2),
    type="float",
    doc="Float constant -4096.0 used to convert post-deadzone DirectInput axis values into signed Q12 input axes.",
)
stable.data(
    "gamepad_axis_inv_range",
    xref("Input_ReadGamepad", 213, 2),
    type="float",
    doc="Float constant 1/600 used with gamepad_axis_scale; the observed deadzone is the literal +/-100 check in Input_ReadGamepad.",
)
stable.data("difficulty_normal", xref("Game_HandleGameOver", 35, 2))
stable.data("difficulty_easy", xref("Game_HandleGameOver", 27, 2))
stable.data("direct_draw_gamma_control_guid", xref("D3D_SetGammaRamp", 130, 1))
stable.data("direct_draw7_guid", xref("D3D_InitializeDirectDraw", 503, 1))
stable.data("direct3d_tnl_hal_device_iid", xref("D3D_CreateTextureSurface", 134, 1))
stable.data("direct3dhal_device_iid", xref("D3D_SelectBestDriver", 120, 1))
stable.data("direct3drgb_device_iid", xref("D3D_CreateTextureSurface", 67, 1))
stable.data("direct_input_device2_a_guid", xref("D3D_GetDeviceCapabilities", 9, 1))
stable.data("crt_zero", xref("Float_CheckPrecision", 42, 2))
stable.data("crt_negative_two", xref("Float_CheckPrecision", 15, 2))
stable.data("crt_kernel32", xref("Float_LoadFPU", 0, 1))
stable.data("crt_exponent_suffix", xref("Float_ConvertToExponential", 147, 1))
stable.data("crt_qnan", xref("Float_ConvertToDecimalString", 245, 1))
stable.data("crt_inf", xref("Float_ConvertToDecimalString", 216, 1))
stable.data("crt_ind", xref("Float_ConvertToDecimalString", 199, 1))
stable.data("crt_snan", xref("Float_ConvertToDecimalString", 173, 1))
stable.data("actor_spawn_params", xref("Level_InitializeActorSystem", 315, 4))
stable.data("max_level_scale", xref("Level_InitializeActorSystem", 300, 2))
stable.data("reciprocal_lookup_table", xref("Render_AdjustLevelScale", 80, 2))
stable.data(
    "mci_open_avi_video",
    xref("Video_InitializeAVIPlayer", 6, 1),
    xref("Video_OpenAVIFile", 51, 1),
    type="char",
    doc='NUL-terminated MCI command fragment "open" used by AVI/movie playback setup.',
)
stable.data("mci_close_avi_video", xref("Video_ShutdownAVIPlayer", 6, 1))
stable.data("mci_movie_id", xref("Video_OpenAVIFile", 21, 1))
stable.data("mci_close_device", xref("Video_CloseAVIFile", 5, 1))
stable.data("mci_play_fullscreen", xref("Video_PlayAVIFullscreen", 5, 1))
stable.data("mci_status_playing", xref("Video_IsAVIPlaying", 51, 1))
stable.data("mci_status_mode", xref("Video_IsAVIPlaying", 8, 1))
stable.data("font_glyph_render_state", xref("Render_TexturedSprite", 23, 1))
stable.data("cooperative_level_set", xref("D3D_InitDirectDrawAndDirect3D", 17, 2))
stable.data("camera_projection_divisor", xref("Camera_SetupProjection", 15, 2))
stable.data("tex_err_tex_2_null", xref("Texture_LoadAndUpload", 129, 1))
stable.data("tex_err_tex_1_null", xref("Texture_LoadAndUpload", 84, 1))
stable.data("tex_err_tex_0_null", xref("Texture_LoadAndUpload", 39, 1))
stable.data("error_string", xref("Texture_LoadAndUpload", 34, 1))
stable.data("d3d_err_create_texture", xref("D3D_CreateTextureSurface", 753, 1))
stable.data("d3d_err_no4444_rgba", xref("D3D_CreateTextureSurface", 364, 1))
stable.data("d3d_texture_error", xref("D3D_CreateTextureSurface", 359, 1))
stable.data("d3d_err_no16_bit_rgb", xref("D3D_CreateTextureSurface", 352, 1))
stable.data("d3d_err_create_work_surface", xref("D3D_CreateWorkSurface", 180, 1))
stable.data("d3d_err_blt", xref("Material_CopyPixelDataToTexture", 749, 1))
stable.data("d3d_err_get_surface_ptr", xref("Material_CopyPixelDataToTexture", 214, 1))
stable.data("d3d_error_message_buffer", xref("Material_CopyPixelDataToTexture", 209, 1))
stable.data("d3d_err_lock", xref("Material_CopyPixelDataToTexture", 158, 1))
stable.data("d3d_err_work_null", xref("Material_CopyPixelDataToTexture", 115, 1))
stable.data("d3d_err_copy_mem_tex_null", xref("Material_CopyPixelDataToTexture", 53, 1))
stable.data("d3d_err_work_surface_null", xref("Texture_BlitToQuadrants", 137, 1))
stable.data("app_window_class_name", xref("Debug_Log", 18, 1))
stable.data("d3d_err_z_enable", xref("D3D_InitDirectDrawAndDirect3D", 1419, 1))
stable.data("d3d_err_set_viewport", xref("D3D_InitDirectDrawAndDirect3D", 1066, 1))
stable.data("d3d_err_create_rgb_device", xref("D3D_InitDirectDrawAndDirect3D", 951, 1))
stable.data("d3d_err_create_hal_device", xref("D3D_InitDirectDrawAndDirect3D", 895, 1))
stable.data(
    "d3d_err_create_driver_device", xref("D3D_InitDirectDrawAndDirect3D", 839, 1)
)
stable.data("d3d_err_create_device7", xref("D3D_InitDirectDrawAndDirect3D", 774, 1))
stable.data(
    "d3d_err_add_attached_surface", xref("D3D_InitDirectDrawAndDirect3D", 763, 1)
)
stable.data("d3d_software_device", xref("D3D_InitDirectDrawAndDirect3D", 687, 1))
stable.data("d3d_tnl_device", xref("D3D_InitDirectDrawAndDirect3D", 664, 1))
stable.data("d3d_hal_device", xref("D3D_InitDirectDrawAndDirect3D", 623, 1))
stable.data("d3d_err_create_z_buffer", xref("D3D_InitDirectDrawAndDirect3D", 477, 1))
stable.data("d3d_err_query_direct3d7", xref("D3D_InitDirectDrawAndDirect3D", 412, 1))
stable.data(
    "d3d_err_get_attached_surface", xref("D3D_InitDirectDrawAndDirect3D", 378, 1)
)
stable.data("d3d_err_create_surface", xref("D3D_InitDirectDrawAndDirect3D", 312, 1))
stable.data("d3d_err_set_display_mode", xref("D3D_InitDirectDrawAndDirect3D", 262, 1))
stable.data(
    "d3d_err_set_cooperative_level", xref("D3D_InitDirectDrawAndDirect3D", 46, 1)
)
stable.data("d3d_textures_still_active", xref("D3D_ReleaseAllAndReportLeaks", 143, 1))
stable.data("d3d_closing_log", xref("D3D_SignalHandler", 0, 1))
stable.data("d3d_log_separator", xref("D3D_InitializeDirectDraw", 437, 1))
stable.data("d3d_can_use_color_key", xref("D3D_InitializeDirectDraw", 375, 1))
stable.data("d3d_selected_driver_header", xref("D3D_InitializeDirectDraw", 125, 1))
stable.data("d3d_open_log", xref("D3D_InitializeDirectDraw", 46, 1))
stable.data("d3d_log", xref("D3D_InitializeDirectDraw", 21, 1))
stable.data("d3d_log_file_mode", xref("D3D_InitializeDirectDraw", 16, 1))
stable.data("file_mode_read_binary", xref("Render_TakeScreenshot", 29, 1))
stable.data("screenshot_fmt", xref("Render_TakeScreenshot", 14, 1))
stable.data("d3d_err_set_gamma_control", xref("D3D_SetGammaRamp", 280, 1))
stable.data("d3d_err_query_gamma_control", xref("D3D_SetGammaRamp", 144, 1))
stable.data("d3d_gamma_not_supported", xref("D3D_SetGammaRamp", 89, 1))
stable.data("d3d_try_ref_rasterizer", xref("D3D_EnumerateDirectDrawDevices", 101, 1))
stable.data("d3d_no_devices_accepted", xref("D3D_EnumerateDirectDrawDevices", 76, 1))
stable.data("d3d_no_devices_enumerated", xref("D3D_EnumerateDirectDrawDevices", 35, 1))
stable.data("d3d_err_query_during_enum", xref("DDraw_EnumerateCallback", 86, 1))
stable.data("d3d_err_create_during_enum", xref("DDraw_EnumerateCallback", 41, 1))
stable.data(
    "ddraw_error_unknown_message",
    xref("D3D_FormatDirectXError", 1616, 1),
    type="char",
    doc=(
        'NUL-terminated fallback DirectDraw error string "Unknown Error." used by '
        "D3D_FormatDirectXError; name aligned to Binary Ninja's ddraw_error_unknown_message data "
        "symbol."
    ),
)
stable.data("dd_err_ok", xref("D3D_FormatDirectXError", 1609, 1))
stable.data("dd_err_not_initialized", xref("D3D_FormatDirectXError", 1574, 1))
stable.data("dd_err_test_finished", xref("D3D_FormatDirectXError", 1544, 1))
stable.data("dd_err_page_lock_failed", xref("D3D_FormatDirectXError", 1502, 1))
stable.data("dd_err_non_local_vid_mem", xref("D3D_FormatDirectXError", 1454, 1))
stable.data("dd_err_cant_duplicate", xref("D3D_FormatDirectXError", 1281, 1))
stable.data("dd_err_not_flippable", xref("D3D_FormatDirectXError", 1225, 1))
stable.data("dd_err_width_too_large", xref("D3D_FormatDirectXError", 917, 1))
stable.data("dd_err_size_too_large", xref("D3D_FormatDirectXError", 861, 1))
stable.data("dd_err_out_of_memory", xref("D3D_FormatDirectXError", 680, 1))
stable.data("dd_err_no_gdi", xref("D3D_FormatDirectXError", 520, 1))
stable.data("dd_err_no_flip_hardware", xref("D3D_FormatDirectXError", 479, 1))
stable.data("dd_err_invalid_pixel_format", xref("D3D_FormatDirectXError", 339, 1))
stable.data("dd_err_invalid_object", xref("D3D_FormatDirectXError", 300, 1))
stable.data("dd_err_unsupported", xref("D3D_FormatDirectXError", 230, 1))
stable.data("dd_err_can_not_detach", xref("D3D_FormatDirectXError", 191, 1))
stable.data("dd_err_invalid_params", xref("D3D_FormatDirectXError", 181, 1))
stable.data("dd_err_already_initialized", xref("D3D_FormatDirectXError", 171, 1))
stable.data("dd_err_can_not_attach", xref("D3D_FormatDirectXError", 161, 1))
stable.data("dd_err_not_supported", xref("D3D_FormatDirectXError", 126, 1))
stable.data("dd_err_generic_failure", xref("D3D_FormatDirectXError", 116, 1))
stable.data(
    "error_message_interface_already_associated", xref("D3D_FormatDirectXError", 106, 1)
)
stable.data("movie_playback_state", xref("Video_OpenMovieFile", 17, 1))
stable.data("default_screen_width", xref("Video_OpenMovieFile", 44, 2))
stable.data("default_screen_height", xref("Video_OpenMovieFile", 33, 1))
stable.data("video_default_rect_left", xref("Video_OpenMovieFile", 0, 1))
stable.data("video_default_rect_top", xref("Video_OpenMovieFile", 5, 2))
stable.data("video_default_rect_right", xref("Video_OpenMovieFile", 11, 2))
stable.data("video_default_rect_bottom", xref("Video_OpenMovieFile", 22, 1))
stable.data("video_alt_rect_left", xref("Video_OpenMovieFile", 52, 2))
stable.data("video_alt_rect_top", xref("Video_OpenMovieFile", 58, 2))
stable.data("video_alt_rect_right", xref("Video_OpenMovieFile", 64, 1))
stable.data("video_alt_rect_bottom", xref("Video_OpenMovieFile", 75, 2))
stable.data("level_index_dalmatians", xref("SaveGame_SaveLevelCompletion", 333, 3))
stable.data("name_entry_char_a", xref("Menu_ProcessNameEntryInput", 55, 4))
stable.data("difficulty_option_easy", xref("Menu_RenderDifficultySelection", 62, 1))
stable.data("menu_difficulty_tob", xref("Level_InitializeBonusData", 82, 1))
stable.data("level_bonus_rff", xref("Level_InitializeBonusData", 46, 1))
stable.data("cheater", xref("Menu_UpdatePauseMenu", 543, 1))
stable.data("menu_save_percent", xref("Menu_RenderSaveGame", 840, 1))
stable.data("format_string_and_int", xref("Menu_RenderSaveGame", 661, 1))
stable.data("menu_cancel", xref("Menu_RenderControlsConfiguration", 872, 1))
stable.data("menu_accept", xref("Menu_RenderControlsConfiguration", 807, 1))
stable.data(
    "format_string_two_strings", xref("Menu_RenderControlsConfiguration", 366, 1)
)
stable.data("current_level_id", xref("Player_ProcessMovement", 321, 2))
stable.data("cheat_code_sequence", xref("Input_CheckCheatCodeSequence", 30, 3))
stable.data("bonus_replay_level_ids", xref("DemoReplay_LoadBonusReplay", 97, 3))
stable.data("bonus_replay_index", xref("DemoReplay_LoadBonusReplay", 13, 1))
stable.data("matrix_fixed_one_1", xref("Matrix_BuildRotationXY", 164, 1))
stable.data("matrix_fixed_zero_1", xref("Math_BuildRotationMatrix", 102, 1))
stable.data("matrix_fixed_one_2", xref("Matrix_BuildRotationXY", 119, 3))
stable.data("matrix_fixed_zero_2", xref("Math_BuildRotationMatrix", 117, 2))
stable.data("matrix_fixed_one_short", xref("Matrix_BuildRotationXY", 126, 3))
stable.data("matrix_fixed_one_3", xref("Matrix_BuildRotationY", 50, 3))
stable.data("matrix_fixed_zero_3", xref("Matrix_BuildRotationY", 38, 2))
stable.data("matrix_fixed_one_4", xref("Matrix_BuildRotationY", 118, 2))
stable.data("matrix_fixed_zero_4", xref("Matrix_BuildRotationY", 65, 3))
stable.data("matrix_fixed_one_short_2", xref("Matrix_BuildRotationY", 57, 3))
stable.data("matrix_fixed_one_5", xref("Math_BuildRotationMatrix", 304, 1))
stable.data("matrix_fixed_one_6", xref("Math_BuildRotationMatrix", 326, 3))
stable.data(
    "vertex_index_remap_table_1", xref("Actor_InitializeDirectionTables", 10, 1)
)
stable.data("audio_emulated", xref("Audio_InitializeSystem", 154, 1))
stable.data("audio_active_waves_themes", xref("Audio_ShutdownSystem", 74, 1))
stable.data(
    "audio_music_path",
    xref("Audio_OpenStream", 43, 1),
    type="char",
    doc="First byte/base of the data/music path format literal used by Audio_OpenStream.",
)
stable.data("fix_up_end", xref("Resource_FixUpLevelPointers", 1376, 1))
stable.data("fix_up_usable_materials", xref("Resource_FixUpLevelPointers", 1239, 1))
stable.data("fix_up_nav_net", xref("Resource_FixUpLevelPointers", 1126, 1))
stable.data("fix_up_powerup_ct", xref("Resource_FixUpLevelPointers", 1056, 1))
stable.data("fix_up_trail_list", xref("Resource_FixUpLevelPointers", 881, 1))
stable.data("fix_up_max_themes", xref("Resource_FixUpLevelPointers", 813, 1))
stable.data("fix_up_sprite_list", xref("Resource_FixUpLevelPointers", 695, 1))
stable.data("fix_up_powerup_ct_ellipsis", xref("Resource_FixUpLevelPointers", 619, 1))
stable.data("fix_up_var_list", xref("Resource_FixUpLevelPointers", 577, 1))
stable.data("fix_up_sound_definition_list", xref("Resource_FixUpLevelPointers", 535, 1))
stable.data("fix_up_cycle_actor_list", xref("Resource_FixUpLevelPointers", 295, 1))
stable.data("fix_up_actor_list_not_null", xref("Resource_FixUpLevelPointers", 268, 1))
stable.data("fix_up_actor_list_null", xref("Resource_FixUpLevelPointers", 250, 1))
stable.data("fix_up_actor_ct_zero", xref("Resource_FixUpLevelPointers", 209, 1))
stable.data("fix_up_actor_ct", xref("Resource_FixUpLevelPointers", 184, 1))
stable.data("fix_up_cam_default_null", xref("Resource_FixUpLevelPointers", 160, 1))
stable.data("fix_up_cam_default_not_null", xref("Resource_FixUpLevelPointers", 147, 1))
stable.data("fix_up_cam_default_get_addr", xref("Resource_FixUpLevelPointers", 112, 1))
stable.data("fix_up_cam_default_abs_addr", xref("Resource_FixUpLevelPointers", 83, 1))
stable.data("fix_up_level_base_null", xref("Resource_FixUpLevelPointers", 59, 1))
stable.data("fix_up_level_null", xref("Resource_FixUpLevelPointers", 35, 1))
stable.data("fix_up_start", xref("Resource_FixUpLevelPointers", 11, 1))
stable.data(
    "savegame_dat",
    xref("SaveGame_ReadFile", 6, 1),
    type="char",
    doc='First byte/base of the "savegame.dat" path literal shared by SaveGame_ReadFile and SaveGame_WriteFile.',
)
stable.data("file_mode_write_binary", xref("SaveGame_WriteFile", 1, 1))
stable.data("movie_err_play", xref("Movie_PlayIntro", 249, 1))
stable.data("movie_err_open", xref("Movie_PlayIntro", 186, 1))
stable.data("string_concat_3", xref("Movie_PlayIntro", 33, 1))
stable.data("debug_fps_format", xref("Debug_RenderOverlay", 163, 1))
stable.data("debug_pos_format", xref("Debug_RenderOverlay", 117, 1))
stable.data("max_primitives_per_batch", xref("Render_QuadClipped", 2004, 2))
stable.data("window_class_pcdogs", xref("Config_SaveSettingsToINI", 64, 1))
stable.data(
    "config_file_checksum",
    xref("Config_LoadFromINI", 67, 1),
    type="int32_t",
    doc="Scalar checksum/header accumulator used by Config_LoadFromINI to validate the PCDOGS pcdogs.ini header.",
)
stable.data(
    "input_button_name_buffer",
    xref("Input_FormatButtonName", 60, 3),
    type="int32_t",
    doc=(
        "First entry/base of the input button-name string-id table consumed by Input_FormatButtonName and "
        "Input_GetButtonString; not the heap-allocated input_button_name_buffers pointer array."
    ),
)
stable.data("pcdogs_ini", xref("Config_LoadFromINI", 9, 1))
stable.data(
    "controller_wingman_rumblepad",
    xref("Input_InitializeControllerMappings", 237, 1),
)
stable.data(
    "controller_gravis_gamepad",
    xref("Input_InitializeControllerMappings", 138, 1),
)
stable.data(
    "controller_ms_sidewinder", xref("Input_InitializeControllerMappings", 39, 1)
)
stable.data(
    "controller_hammerhead_fx", xref("Input_InitializeControllerMappings", 4, 1)
)
stable.data(
    "input_no_key_assigned",
    xref("Input_FormatButtonName", 112, 1),
    type="char",
    doc='First byte/base of the inline "No key assigned" string literal used by Input_FormatButtonName.',
)
stable.data("game_title_102_dalmatians", xref("WinMain", 82, 4))
stable.data(
    "window_height",
    xref("D3D_InitDirectDrawAndDirect3D", 86, 2),
    type="int32_t",
    doc="Active render/window height in pixels initialized during DirectDraw/Direct3D setup.",
)
stable.data("d3d_err_begin_scene", xref("Render_Frame", 121, 1))
stable.data("d3d_err_restore_all_surfaces", xref("Render_Frame", 82, 1))
stable.data("shutdown_complete", xref("Game_WindowProc", 258, 1))
stable.data("shutdown_destroy_window", xref("Game_WindowProc", 207, 1))
stable.data("shutdown_uninit_game", xref("Game_WindowProc", 173, 1))
stable.data("shutdown_direct_input_release", xref("Game_WindowProc", 156, 1))
stable.data("shutdown_kill_game", xref("Game_WindowProc", 136, 1))
stable.data("shutdown_uninit_game_interface", xref("Game_WindowProc", 119, 1))
stable.data("shutdown_unload_data", xref("Game_WindowProc", 85, 1))
stable.data("shutdown_begin", xref("Game_WindowProc", 73, 1))
stable.data("win_main_requires_nt", xref("WinMain", 117, 1))
stable.data("file_cant_find_pkg", xref("PKG_FindAndOpenFile", 321, 1))
stable.data("file_setup_path", xref("PKG_FindAndOpenFile", 213, 1))
stable.data("pkg_search_pattern", xref("PKG_FindAndOpenFile", 171, 1))
stable.data("file_dalms_setup_path", xref("PKG_FindAndOpenFile", 147, 1))
stable.data("file_drive_letter", xref("PKG_FindAndOpenFile", 96, 1))
stable.data("pcdogs_pkg", xref("PKG_FindAndOpenFile", 69, 1))
stable.data("random_seed", xref("Math_GenerateRandom", 0, 2))
stable.data(
    "script_command_table",
    xref("ScriptCmd_WithActor", 174, 3),
    type="Script_OpcodeTable",
    doc=(
        "45-entry script opcode handler table resolved through ScriptCmd_WithActor. "
        "Table identity is cross-tool corroborated; per-opcode semantics still require "
        "case-by-case audit before naming individual handlers."
    ),
)
stable.data("input_landing_flags_ptr", xref("Player_ProcessMovement", 74, 1))
stable.data("strin_no_string", xref("String_GetByIndex", 733, 1))
stable.data("strin_two_strings", xref("String_GetByIndex", 428, 1))
stable.data(
    "render_list_state",
    xref("Render_AdjustLevelScale", 101, 2),
    doc="Data pointer to active Render_ListState; Render_AdjustLevelScale writes dynamic level scale at +0xB8 (PC EN).",
)
stable.data("debug_killed_by_player", xref("Memory_MallocWithRetry", 96, 1))
stable.data("ui_confirm_stop_game", xref("Memory_MallocWithRetry", 78, 1))
stable.data("out_of_memory", xref("Memory_MallocWithRetry", 49, 1))
stable.data("mem_malloc_failed", xref("Memory_MallocWithRetry", 26, 1))
stable.data("ui_programmer_message", xref("UI_ShowConfirmDialog", 90, 1))
stable.data("mem_alloc_debug", xref("Memory_AllocateHandle", 120, 1))
stable.data("mem_alloc_failed", xref("Memory_AllocateHandle", 59, 1))
stable.data("mem_alloc_prefix", xref("Memory_AllocateHandle", 34, 1))
stable.data("mem_out_of_extents", xref("Memory_AllocateHandle", 12, 1))
stable.data("leak_unreleased_extent", xref("Memory_FreeAllExtents", 22, 1))
stable.data("mem_leak_invalid_extent", xref("Memory_ReleaseHandle", 158, 1))
stable.data("mem_free_debug", xref("Memory_ReleaseHandle", 76, 1))
stable.data("mem_leak_unallocated", xref("Memory_ReleaseHandle", 41, 1))
stable.data("game_start_time", xref("Timer_GetElapsedTickCount", 0, 1))
stable.data("key_mapping_table_size", xref("Input_IsKeyPressed", 44, 3))
stable.data("key_mapping_table_ptr", xref("Input_IsKeyPressed", 0, 1))
stable.data("char_type_table", xref("String_ParseInt", 35, 2))
stable.data("decimal_point_char", xref("String_FormatFloat", 64, 2))
stable.data("scene_traversal_depth", xref("Scene_TraverseNodeTree", 1025, 2))
stable.data("graphics_capability_flags", xref("Render_MeshNode", 3958, 1))
stable.data(
    "current_entity_camera",
    xref("Camera_UpdateFollow", 720, 2),
    doc=(
        "Camera_UpdateFollow scratch/current entity-camera cell. This is transient "
        "camera-owned runtime state used across level/entity transitions; current_level_data "
        "entity slots are the actor/entity enumeration source, not this global."
    ),
)
stable.data("screen_border_state_flag", xref("Script_PauseToggle", 102, 1))
stable.data(
    "active_entity_work_list",
    xref("Entity_UpdateVisibilityAndSpawn", 232, 1),
    xref("Camera_UpdateFollow", 2157, 1),
    doc=(
        "Active entity/navigation pointer cells: pcdogs.exe+0x55D30 (PC EN) is current, "
        "pcdogs.exe+0x55D3C (PC EN) is staging, and pcdogs.exe+0x56850 (PC EN)/pcdogs.exe+0x55ED8 (PC "
        "EN) are alternating 0x978-byte backing buffers. Entity_GetActiveActorFromList loads "
        "pcdogs.exe+0x55D30 (PC EN) as an entity-array scan bound."
    ),
)
stable.data("fade_counter", xref("Script_PauseToggle", 141, 1))
stable.data("music_transition_volume", xref("Audio_TriggerMusicTransition", 153, 2))
stable.data(
    "navigation_command_queue",
    xref("Camera_UpdateFollow", 2147, 1),
    doc=(
        "Camera_UpdateFollow navigation command/work queue pointer cell at pcdogs.exe+0x55D3C (PC "
        "EN); writes are staged into the pcdogs.exe+0x55ED8 (PC EN)/pcdogs.exe+0x56850 (PC EN) "
        "backing-buffer pair."
    ),
)
stable.data("path_trace_work_buffer", xref("Actor_TracePath", 18, 1))
stable.data(
    "path_trace_state_2",
    xref("Actor_TracePath", 39, 2),
    doc=(
        "Actor_TracePath byte/cell used when the source actor is NULL. Native evidence "
        "initializes this to zero while using path_trace_work_buffer, current_level_data "
        "entity slots, and active_entity_work_list; the exact byte meaning is still unknown."
    ),
)
stable.data("path_trace_state_1", xref("Actor_TracePath", 29, 2))
stable.data("path_trace_state_3", xref("Actor_TracePath", 23, 2))
stable.data("camera_transition_trigger", xref("Camera_UpdateFollow", 668, 1))
stable.data("camera_transition_countdown", xref("Script_PauseToggle", 379, 2))
stable.data(
    "music_fade_frame_count",
    xref("Audio_TriggerMusicTransition", 102, 2),
    xref("Audio_InitializeSystem", 279, 2),
)
stable.data("music_fade_start_frame", xref("Audio_TriggerMusicTransition", 199, 1))
stable.data("music_transition_end_frame", xref("Audio_TriggerMusicTransition", 226, 2))
stable.data("music_transition_pending", xref("Audio_TriggerMusicTransition", 207, 2))
stable.data(
    "navigation_queue_head",
    xref("Camera_UpdateFollow", 2162, 1),
    doc=(
        "Active entity/navigation backing buffer at pcdogs.exe+0x55ED8 (PC EN); Camera_UpdateFollow "
        "selects pcdogs.exe+0x56850 (PC EN) as the alternate buffer while the shared backing-buffer "
        "layout stays opaque."
    ),
)
stable.data(
    "level_transition_flag",
    xref("Script_PauseToggle", 372, 2),
    xref("Script_PauseToggle", 399, 2),
)
stable.data("level_transition_start_frame", xref("Level_InitializeActorSystem", 195, 1))
stable.data(
    "game_settings",
    xref("Camera_UpdateFollow", 1978, 1),
    xref("GameState_SetSoundEnabled", 12, 1),
)
stable.data("level_transition_end_frame", xref("Level_InitializeActorSystem", 217, 1))
stable.data("camera_transition_frame_counter", xref("Camera_UpdateFollow", 1983, 2))
stable.data("camera_previous_yaw", xref("Camera_CalculateFollowAngles", 151, 3))
stable.data("camera_transition_paused", xref("Camera_InterpolateTransition", 22, 1))
stable.data("animation_timer_state", xref("Script_PauseToggle", 247, 2))
stable.data("anim_queued_state_change", xref("Animation_QueueStateChange", 9, 3))
stable.data("animation_state_queue_count", xref("Animation_QueueStateChange", 0, 1))
stable.data(
    "avi_player_initialized",
    xref("Video_InitializeAVIPlayer", 32, 1),
    xref("Video_InitializeAVIPlayer", 54, 1),
)
stable.data("avi_window_handle", xref("Video_InitializeAVIPlayer", 49, 1))
stable.data("avi_movie_counter", xref("Video_InitializeAVIPlayer", 11, 2))
stable.data("checkers_camera_pos_1_x", xref("Checkers_UpdateStateMachine", 230, 2))
stable.data("checkers_camera_pos_1_y", xref("Checkers_UpdateStateMachine", 236, 1))
stable.data("checkers_camera_pos_1_z", xref("Checkers_UpdateStateMachine", 241, 2))
stable.data("checkers_camera_pos_2_x", xref("Checkers_UpdateStateMachine", 161, 2))
stable.data("checkers_logic_state", xref("Checkers_UpdateStateMachine", 189, 2))
stable.data("checkers_camera_pos_2_z", xref("Checkers_UpdateStateMachine", 195, 1))
stable.data("checkers_selected_col_1", xref("Checkers_UpdateStateMachine", 66, 2))
stable.data("checkers_selected_row_1", xref("Checkers_UpdateStateMachine", 60, 2))
stable.data("checkers_selected_col_2", xref("Checkers_UpdateStateMachine", 54, 2))
stable.data("checkers_selected_row_2", xref("Checkers_UpdateStateMachine", 48, 2))
stable.data(
    "checkers_board",
    xref("Checkers_UpdateStateMachine", 225, 1),
    type="Checkers_Board",
    doc="32-byte checkers board at pcdogs.exe+0x572A0 (PC EN); passed to board init, move generation, move execution, and AI search.",
)
stable.data("checkers_ai_node_counter", xref("Checkers_UpdateStateMachine", 1470, 2))
stable.data("checkers_move_result", xref("Checkers_UpdateStateMachine", 611, 1))
stable.data("checkers_ai_move_from_col", xref("Checkers_UpdateStateMachine", 1458, 2))
stable.data("checkers_ai_move_from_row", xref("Checkers_UpdateStateMachine", 1452, 2))
stable.data("checkers_ai_move_to_col", xref("Checkers_UpdateStateMachine", 1446, 2))
stable.data("checkers_ai_move_to_row", xref("Checkers_UpdateStateMachine", 1437, 2))
stable.data(
    "checkers_ai_search_jmp_buf",
    xref("Checkers_UpdateStateMachine", 1415, 1),
    type="uint32_t",
    doc="First word of a 64-byte setjmp/longjmp buffer used to abort/pause checkers AI search from input/render polling; SDK generator cannot expose this raw array as a typed global.",
)
stable.data("checkers_ai_think_timeout", xref("Checkers_UpdateStateMachine", 1464, 2))
stable.data("quad_vertex_0", xref("Render_DrawQuad", 972, 6))
stable.data("quad_vertex_0_u", xref("Render_DrawQuad", 648, 2))
stable.data("quad_vertex_0_v", xref("Render_DrawQuad", 669, 2))
stable.data("quad_vertex_1", xref("Render_DrawQuad", 982, 6))
stable.data("quad_vertex_1_u", xref("Render_DrawQuad", 692, 2))
stable.data("quad_vertex_1_v", xref("Render_DrawQuad", 713, 2))
stable.data("quad_vertex_2", xref("Render_DrawQuad", 992, 6))
stable.data("quad_vertex_2_u", xref("Render_DrawQuad", 748, 2))
stable.data("quad_vertex_2_v", xref("Render_DrawQuad", 772, 2))
stable.data("quad_vertex_3", xref("Render_DrawQuad", 1002, 6))
stable.data("quad_vertex_3_u", xref("Render_DrawQuad", 858, 2))
stable.data("quad_vertex_3_v", xref("Render_DrawQuad", 882, 2))
stable.data("d3d_device_init_0", xref("D3D_InitDirectDrawAndDirect3D", 138, 2))
stable.data("d3d_device_init_1", xref("D3D_InitDirectDrawAndDirect3D", 133, 1))
stable.data("d3d_device_init_2", xref("D3D_InitDirectDrawAndDirect3D", 144, 2))
stable.data("d3d_device_init_3", xref("D3D_InitDirectDrawAndDirect3D", 104, 2))
stable.data("vertex_color_buffer", xref("Render_QuadClipped", 859, 2))
stable.data("vertex_color_buffer_g", xref("Render_QuadClipped", 473, 1))
stable.data("vertex_color_buffer_b1", xref("Render_QuadClipped", 871, 1))
stable.data("vertex_color_buffer_b0", xref("Render_QuadClipped", 876, 2))
stable.data("vertex_color_buffer_r_0", xref("Render_QuadClipped", 906, 2))
stable.data("vertex_color_buffer_g_1", xref("Render_QuadClipped", 929, 1))
stable.data("vertex_color_buffer_r_1", xref("Render_QuadClipped", 894, 2))
stable.data("vertex_color_buffer_b", xref("Render_QuadClipped", 918, 2))
stable.data("vertex_color_buffer_b2", xref("Render_QuadClipped", 882, 2))
stable.data("vertex_color_buffer_g_2", xref("Render_QuadClipped", 1088, 2))
stable.data("vertex_color_buffer_r_2", xref("Render_QuadClipped", 1107, 1))
stable.data("vertex_color_buffer_g_3", xref("Render_QuadClipped", 1148, 2))
stable.data("vertex_work_buffer", xref("Render_QuadClipped", 1038, 1))
stable.data("vertex_work_buffer_v_1_base", xref("Render_QuadClipped", 1055, 1))
stable.data("vertex_work_buffer_v_1_alt", xref("Render_QuadClipped", 1193, 1))
stable.data("clip_quad_src_u", xref("Render_QuadClipped", 828, 1))
stable.data("clip_quad_src_v", xref("Render_QuadClipped", 839, 2))
stable.data("vertex_work_buffer_v_2_base", xref("Render_QuadClipped", 1078, 1))
stable.data("clip_quad_dst_u", xref("Render_QuadClipped", 847, 2))
stable.data("clip_quad_dst_v", xref("Render_QuadClipped", 853, 2))
stable.data("clip_temp_buffer", xref("Render_QuadClipped", 1443, 1))
stable.data("driver_guid", xref("Render_QuadClipped", 1515, 1))
stable.data("clip_work_buffer_6", xref("D3D_InitDirectDrawAndDirect3D", 453, 3))
stable.data("ddraw_enum_driver_data", xref("D3D_InitializeDirectDraw", 299, 3))
stable.data("ddraw_init_state_0", xref("D3D_InitializeDirectDraw", 231, 3))
stable.data("ddraw_init_state_1", xref("D3D_InitializeDirectDraw", 363, 3))
stable.data("ddraw_init_param_0", xref("D3D_InitializeDirectDraw", 137, 2))
stable.data("ddraw_init_param_1", xref("D3D_InitializeDirectDraw", 156, 2))
stable.data("ddraw_init_param_2", xref("D3D_InitializeDirectDraw", 165, 2))
stable.data("ddraw_init_param_3", xref("D3D_InitializeDirectDraw", 179, 2))
stable.data("ddraw_init_param_4", xref("D3D_InitializeDirectDraw", 174, 1))
stable.data("driver_initialized", xref("D3D_InitializeDirectDraw", 115, 2))
stable.data(
    "loading_screen_texture",
    xref("Texture_LoadAndUpload", 25, 1),
    type="Material_BlendTextureSet",
)
stable.data("clip_input_buffer", xref("Render_QuadClipped", 1043, 1))
stable.data("clip_input_buffer_y", xref("Render_QuadClipped", 1060, 1))
stable.data("clip_input_buffer_v_2", xref("Render_QuadClipped", 1083, 1))
stable.data("render_clip_min_x", xref("Render_QuadClipped", 774, 2))
stable.data("render_clip_min_y", xref("Render_QuadClipped", 800, 2))
stable.data("clip_input_buffer_vertex_2_w", xref("Render_QuadClipped", 780, 2))
stable.data("render_clip_max_x", xref("Render_QuadClipped", 806, 2))
stable.data("d3d_device_caps_0", xref("D3D_InitDirectDrawAndDirect3D", 127, 1))
stable.data("client_rect_top", xref("D3D_InitDirectDrawAndDirect3D", 156, 1))
stable.data("d3d_device_caps_1", xref("D3D_InitDirectDrawAndDirect3D", 161, 2))
stable.data("client_rect_bottom", xref("D3D_InitDirectDrawAndDirect3D", 110, 2))
stable.data(
    "active_texture_count", xref("Material_ReleaseTextureArray", 23, 2), type="uint32_t"
)
stable.data("untextured_vertex_buffer", xref("Render_DrawQuad", 1261, 1))
stable.data("untextured_vertex_buffer_vertex_1", xref("Render_DrawQuad", 1298, 6))
stable.data("untextured_vertex_buffer_vertex_2", xref("Render_DrawQuad", 1325, 6))
stable.data("untextured_vertex_buffer_vertex_3", xref("Render_DrawQuad", 1315, 6))
stable.data("proj_matrix_x_scale", xref("Camera_SetupProjection", 64, 1))
stable.data("proj_matrix_y_scale", xref("Camera_SetupProjection", 140, 2))
stable.data("proj_matrix_near_w", xref("Camera_SetupProjection", 106, 2))
stable.data("proj_matrix_one", xref("Camera_SetupProjection", 120, 2))
stable.data("proj_matrix_far_w", xref("Camera_SetupProjection", 130, 2))
stable.data("transformed_vertices", xref("Render_QuadClipped", 2054, 1))
stable.data("transformed_vertices_y", xref("Render_QuadClipped", 1600, 2))
stable.data("clip_output_buffer", xref("Render_QuadClipped", 1448, 1))
stable.data("clip_plane_count", xref("Render_QuadClipped", 888, 2))
stable.data("clip_plane_coeffs_0", xref("Render_QuadClipped", 900, 2))
stable.data("clip_plane_coeffs_1", xref("Render_QuadClipped", 924, 1))
stable.data("clip_plane_coeffs_2", xref("Render_QuadClipped", 944, 2))
stable.data("clip_plane_coeffs_3", xref("Render_QuadClipped", 950, 2))
stable.data("clip_plane_coeffs_4", xref("Render_QuadClipped", 956, 1))
stable.data("clip_plane_coeffs_5", xref("Render_QuadClipped", 912, 2))
stable.data("clip_plane_coeffs_6", xref("Render_QuadClipped", 934, 2))
stable.data("clip_plane_coeffs_7", xref("Render_QuadClipped", 961, 2))
stable.data("d3d_vertex_buffer", xref("Render_QuadClipped", 2022, 1))
stable.data("d3d_vertex_buffer_y", xref("Render_QuadClipped", 1949, 1))
stable.data("clip_temp_vertex_buffer", xref("Render_ClipPolygonByCameraPyramid", 24, 1))
stable.data("clip_temp_uv_buffer", xref("Render_ClipPolygonByCameraPyramid", 29, 1))
stable.data(
    "current_bound_texture",
    xref("Material_ReleaseTextureArray", 48, 1),
    type="DDraw_IDirectDrawSurface7*",
)
stable.data("current_blend_mode", xref("D3D_SetBlendMode", 152, 1), type="int32_t")
stable.data("quad_vertex_ptrs", xref("Render_DrawQuad", 108, 2))
stable.data("quad_vertex_1_ptr", xref("Render_DrawQuad", 982, 2))
stable.data("quad_vertex_2_ptr", xref("Render_DrawQuad", 992, 2))
stable.data("quad_vertex_3_ptr", xref("Render_DrawQuad", 1002, 2))
stable.data("selected_driver_index", xref("D3D_InitDirectDrawAndDirect3D", 433, 1))
stable.data("tex_format_is_software", xref("D3D_CreateTextureSurface", 387, 2))
stable.data("tex_needs_alpha", xref("Texture_LoadAndUpload", 10, 2), type="uint8_t")
stable.data("gamma_control", xref("D3D_SetGammaRamp", 110, 2))
stable.data("ddraw_enum_device_list", xref("D3D_EnumDeviceCallback", 58, 3))
stable.data("ddraw_enum_device_count", xref("D3D_EnumerateDirectDrawDevices", 14, 1))
stable.data("enum_device_count", xref("D3D_EnumerateDirectDrawDevices", 26, 1))
stable.data("accepted_device_count", xref("D3D_EnumerateDirectDrawDevices", 67, 1))
stable.data(
    "direct_input",
    xref("DInput_InitializeJoystickInput", 28, 1),
    type="DInput_IDirectInputA*",
    doc="DirectInput interface created by DInput_CreateInterface; used for joystick enumeration/creation and released by Input_ReleaseDirectInputResources.",
)
stable.data(
    "joystick_state",
    xref("Input_ReadGamepad", 32, 1),
    type="DInput_JoystickState*",
    doc=(
        "Current DirectInput joystick state buffer read by Input_ReadGamepad and released "
        "during input shutdown. This is a frame-local sample source; JoyState_GetAxis* "
        "near Input_ReadGamepad gives live analog freshness."
    ),
)
stable.data(
    "direct_input_joystick_device",
    xref("DInput_InitializeJoystickInput", 116, 1),
    type="DInput_IDirectInputDevice*",
    doc="DirectInput joystick device created by DInput_CreateConfiguredJoystickDevice; acquired, polled, and released by input shutdown.",
)
stable.data(
    "dinput_constant_force_effect",
    xref("DInput_CreateConfiguredJoystickDevice", 370, 1),
    type="void*",
    doc="DirectInput constant-force effect object returned by IDirectInputDevice::CreateEffect when force feedback is available.",
)
stable.data(
    "video_movie_handle",
    xref("Video_CloseMovieFile", 0, 1),
    type="int32_t",
    doc="winplay/RPL movie handle initialized by Player_InitMovie and shut down by Player_ShutdownMovie.",
)
stable.data(
    "video_surface_handle",
    xref("Video_CloseMovieFile", 26, 1),
    type="int32_t",
    doc="winplay video surface handle initialized by Player_InitVideo, mapped for playback, and shut down by Player_ShutdownVideo.",
)
stable.data(
    "video_sound_handle",
    xref("Video_CloseMovieFile", 16, 1),
    type="int32_t",
    doc="winplay sound handle initialized by Player_InitSound and passed into movie playback.",
)
stable.data("polygon_render_flags", xref("Render_PolygonBatch", 985, 2))
stable.data("max_primitives_per_batch_d3d", xref("Render_PolygonBatch", 973, 2))
stable.data("polygon_batch_work_value_1", xref("Render_PolygonBatch", 526, 3))
stable.data("polygon_batch_work_value_2", xref("Render_PolygonBatch", 519, 3))
stable.data("polygon_batch_work_value_3", xref("Render_PolygonBatch", 512, 3))
stable.data("render_batch_vertex_base", xref("Render_PolygonBatch", 1091, 1))
stable.data("poly_batch_vertex_count", xref("Render_PolygonBatch", 1099, 2))
stable.data("poly_batch_tri_count", xref("Render_PolygonBatch", 1112, 2))
stable.data("normal_accumulator_x", xref("Mesh_CalculateVertexNormals", 1526, 4))
stable.data("vertex_normal_accum_y", xref("Scene_FinalizeNodeRender", 774, 1))
stable.data("vertex_normal_accum_z", xref("Mesh_CalculateVertexNormals", 1554, 4))
stable.data("vertex_normal_count", xref("Mesh_CalculateVertexNormals", 1512, 4))
stable.data("poly_batch_texture_state", xref("Render_PolygonBatch", 194, 1))
stable.data("backface_vertex_1_ptr", xref("Render_PolygonBatch", 160, 2))
stable.data("backface_vertex_2_ptr", xref("Render_PolygonBatch", 178, 2))
stable.data("transformed_vertex_1_screen_xy", xref("Render_PolygonBatch", 1826, 3))
stable.data("transformed_vertex_1_view_z", xref("Render_PolygonBatch", 1726, 1))
stable.data("transformed_vertex_2_screen_xy", xref("Render_PolygonBatch", 2185, 3))
stable.data("transformed_vertex_2_view_z", xref("Render_PolygonBatch", 2085, 1))
stable.data("transformed_vertex_3_screen_xy", xref("Render_PolygonBatch", 2544, 3))
stable.data("transformed_vertex_3_view_z", xref("Render_PolygonBatch", 2444, 1))
stable.data("transformed_vertex_4_screen_x", xref("Render_PolygonBatch", 2596, 3))
stable.data("transformed_vertex_4_screen_y", xref("Render_PolygonBatch", 4375, 2))
stable.data("transformed_vertex_4_view_z", xref("Render_PolygonBatch", 2828, 1))
stable.data("poly_batch_render_flags", xref("Render_PolygonBatch", 199, 1))
stable.data(
    "sprite_vertex_flags", xref("Bone_TransformWeightedVerts_ForRender", 1491, 2)
)
stable.data(
    "sprite_last_position_x", xref("Bone_TransformWeightedVerts_ForRender", 768, 2)
)
stable.data(
    "sprite_last_position_y", xref("Bone_TransformWeightedVerts_ForRender", 777, 2)
)
stable.data(
    "sprite_last_position_z", xref("Bone_TransformWeightedVerts_ForRender", 786, 2)
)
stable.data("sprite_anim_frame", xref("Bone_TransformWeightedVerts_ForRender", 1398, 4))
stable.data("sprite_anim_flags", xref("Bone_TransformWeightedVerts_ForRender", 1406, 4))
stable.data("sprite_anim_state", xref("Bone_TransformWeightedVerts_ForRender", 1151, 3))
stable.data("puppy_counter_anim_state", xref("Menu_ClearTransitionFlags", 23, 3))
stable.data("bonus_level_flags", xref("Level_GetDataPointer", 4, 1))
stable.data("string_format_buffer", xref("Menu_RenderControlsConfiguration", 45, 1))
stable.data("pause_menu_state", xref("SaveGame_SaveLevelCompletion", 188, 2))
stable.data("confirm_prompt_frame_counter", xref("Menu_RenderConfirmPrompt", 32, 2))
stable.data(
    "pause_transition_timer",
    xref("Menu_UpdatePauseMenu", 210, 3),
    xref("Menu_ResetState", 2, 1),
)
stable.data("game_state", xref("SaveGame_GetSlotIndex", 0, 1))
stable.data("player_lives", xref("Player_SetLives", 4, 1))
stable.data("options_menu_value_1", xref("Menu_HandleOptionsLogic", 149, 1))
stable.data("pause_menu_transition_timer_1", xref("Menu_ClearTransitionFlags", 7, 3))
stable.data(
    "save_file_buffer",
    xref("SaveGame_LoadState", 13, 1),
    type="SaveGame_Data",
    doc="0x10-byte save-file header followed by save-slot payloads; passed to SaveGame_InitOperation with total size 0x1dc.",
)
stable.data(
    "save_file_game_state",
    xref("SaveGame_LoadState", 112, 2),
    type="int32_t",
    doc="Save-file header game_state dword restored to game_state by SaveGame_LoadState.",
)
stable.data(
    "save_file_game_settings",
    xref("SaveGame_LoadState", 118, 2),
    type="int32_t",
    doc="Save-file header game_settings dword restored to game_settings by SaveGame_LoadState.",
)
stable.data(
    "save_file_player_lives",
    xref("SaveGame_LoadState", 107, 1),
    type="int32_t",
    doc="Save-file header player-lives dword restored to player_lives by SaveGame_LoadState.",
)
stable.data(
    "save_game_buffer",
    xref("Menu_HandleSaveGameLogic", 792, 2),
    type="SaveGame_Slot",
    doc=(
        "First 0x5c-byte save-slot record immediately after the 0x10-byte "
        "SaveGame_Data header. Native file operations cover 0x1dc bytes, consistent "
        "with five slot-sized records."
    ),
)
stable.data(
    "save_slot_valid_flags",
    xref("Menu_HandleSaveGameLogic", 316, 3),
    type="uint8_t",
    doc="First save-slot valid byte at SaveGame_Slot+1; SaveGame_SaveToSlot sets saveSlots[slotIndex].is_valid to 1.",
)
stable.data(
    "options_menu_backup_data",
    xref("Menu_HandleOptionsLogic", 169, 1),
    type="uint8_t[0x6c]",
    doc="Editable 0x6c-byte options/config backup block copied before controls remapping and passed to Config_SaveSettingsToINI.",
)
stable.data(
    "keyboard_mappings",
    xref("Menu_HandleOptionsLogic", 1061, 3),
    type="int32_t",
    doc=(
        "First entry/base of the 13-dword player-1/keyboard binding range inside "
        "options_menu_backup_data at offset +4 (PC EN); options UI compares 11 keyboard-side "
        "entries. Scalar alias only; the parent 0x6c block remains authoritative."
    ),
)
stable.data(
    "gamepad_mappings",
    xref("Menu_HandleOptionsLogic", 154, 1),
    type="int32_t",
    doc=(
        "First entry/base of the 13-dword player-2/gamepad binding range inside "
        "options_menu_backup_data at offset +0x38 (PC EN); options UI uses 10 gamepad-side "
        "entries. Scalar alias only; the parent 0x6c block remains authoritative."
    ),
)
stable.data(
    "resource_data_buffer",
    xref("Shared_LoadCommonResources", 77, 1),
    type="void*",
    doc="Resource/common-data buffer pointer populated by Shared_LoadCommonResources.",
)
stable.data("menu_state", xref("Menu_ProcessMenuState", 160, 3))
stable.data(
    "menu_selection",
    xref("Level_SetMenuProgressState", 68, 2),
    xref("Menu_ProcessMenuState", 210, 1),
)
stable.data("menu_skip_background_render", xref("Menu_ProcessMenuState", 592, 1))
stable.data("menu_display_flags", xref("Menu_ProcessMenuState", 565, 1))
stable.data("menu_transition_delay", xref("Menu_ProcessMenuState", 111, 1))
stable.data("menu_sound_effect", xref("Menu_HandleOptionsLogic", 9, 2))
stable.data("menu_post_transition_action", xref("Menu_ProcessMenuState", 120, 3))
stable.data("menu_context", xref("Menu_ProcessMenuState", 927, 1))
stable.data(
    "menu_fade_counter",
    xref("Menu_ProcessMenuState", 1980, 2),
    xref("Menu_ProcessMenuState", 91, 1),
)
stable.data("menu_stored_fade_level", xref("Menu_ProcessMenuState", 2441, 2))
stable.data("menu_option_index", xref("Menu_ProcessMenuState", 254, 1))
stable.data("name_entry_active", xref("Menu_ProcessMenuState", 2122, 2))
stable.data("name_entry_row", xref("GameState_BackupSettings", 17, 1))
stable.data("name_entry_column", xref("Menu_ProcessMenuState", 2094, 2))
stable.data("saved_game_settings", xref("GameState_BackupSettings", 22, 2))
stable.data("saved_player_lives", xref("GameState_BackupSettings", 28, 2))
stable.data("lives_ui_current_value", xref("Menu_UpdatePauseMenu", 283, 2))
stable.data("menu_reset_flag", xref("Level_CheckBonusUnlock", 38, 3))
stable.data(
    "collectibles_data",
    xref("SaveGame_SaveLevelCompletion", 171, 3),
    type="SaveGame_Slot",
    doc="Active 0x5c-byte save progress payload copied into save_game_buffer by SaveGame_SaveToSlot.",
)
stable.data(
    "puppy_count_backup",
    xref("Level_InitializeSaveState", 7, 1),
    type="uint8_t",
    doc="Active SaveGame_Slot+2 puppy/life backup byte, seeded to 3 by SaveGame_InitializeState and updated by SaveGame_BackupPuppyCount.",
)
stable.data(
    "save_game_init_flag",
    xref("SaveGame_InitializeState", 22, 2),
    type="uint8_t",
    doc="Active SaveGame_Slot+3 initialization flag set to 4 by SaveGame_InitializeState.",
)
stable.data(
    "game_complete_flag",
    xref("SaveGame_SetGameComplete", 4, 1),
    type="uint8_t",
    doc="Active SaveGame_Slot+4 game-complete flag written by SaveGame_SetGameComplete.",
)
stable.data(
    "bonus_level_data",
    xref("Level_InitializeBonusData", 6, 4),
    type="uint16_t",
    doc=(
        "First entry of the active SaveGame_Slot+0x38 (PC EN) packed bonus-level parameter table read "
        "by Level_InitializeBonusData."
    ),
)
stable.data(
    "level_best_time",
    xref("SaveGame_SaveLevelCompletion", 131, 3),
    type="uint16_t",
    doc=(
        "Active SaveGame_Slot+0x42 (PC EN) best time/value for the TOB bonus level, written from "
        "menu_items by SaveGame_SaveLevelCompletion."
    ),
)
stable.data(
    "bonus_name_entry_buffer",
    xref("Level_InitializeBonusData", 18, 3),
    type="uint8_t",
    doc="First byte of active SaveGame_Slot+0x44 (PC EN) bonus/name-entry payload copied with each 0x5c-byte save slot.",
)
stable.data("lives_ui_state", xref("UI_UpdateLives", 303, 3))
stable.data("lives_ui_counter_1", xref("UI_UpdateLives", 81, 1))
stable.data("timer_state", xref("Shared_LoadCommonResources", 50, 1))
stable.data("pause_menu_timer", xref("Menu_UpdatePauseMenu", 379, 3))
stable.data("resource_handle_1", xref("Shared_LoadCommonResources", 25, 1))
stable.data("saved_world_0_completion_bits", xref("SaveGame_SaveBonusProgress", 17, 1))
stable.data("saved_world_1_completion_bits", xref("SaveGame_SaveBonusProgress", 27, 2))
stable.data("saved_world_2_completion_bits", xref("SaveGame_SaveBonusProgress", 39, 2))
stable.data("saved_world_3_completion_bits", xref("SaveGame_SaveBonusProgress", 45, 1))
stable.data("saved_world_4_completion_bits", xref("SaveGame_SaveBonusProgress", 50, 2))
stable.data(
    "save_operation_step",
    xref("Menu_RenderSaveGame", 269, 3),
    type="uint8_t",
    doc="Save/load async operation step byte used by Menu_RenderSaveGame, Menu_HandleSaveGameLogic, and SaveGame_SaveToSlot.",
)
stable.data(
    "save_load_mode_flag",
    xref("Menu_RenderSaveGame", 276, 1),
    type="uint8_t",
    doc="Save/load mode byte in the save-menu state cluster.",
)
stable.data(
    "save_operation_result",
    xref("Menu_HandleSaveGameLogic", 147, 2),
    type="uint8_t",
    doc="Save operation result/status byte written by Menu_HandleSaveGameLogic.",
)
stable.data(
    "save_menu_active",
    xref("Menu_RenderSaveGame", 24, 1),
    type="uint8_t",
    doc="Save-menu active byte flag read by Menu_RenderSaveGame.",
)
stable.data(
    "save_overwrite_choice",
    xref("Menu_RenderSaveGame", 308, 3),
    type="uint8_t",
    doc="Overwrite-confirmation choice byte in the save/load menu dialog state cluster.",
)
stable.data(
    "save_dialog_state",
    xref("Menu_RenderSaveGame", 197, 1),
    type="uint8_t",
    doc="Save/load dialog substate byte consumed by Menu_RenderSaveGame.",
)
stable.data(
    "save_game_dirty_flag",
    xref("Menu_HandleSaveGameLogic", 433, 2),
    type="uint8_t",
    doc="Async save dirty/completion byte set while save-slot data is copied and operation 9 is queued.",
)
stable.data(
    "save_game_menu_state",
    xref("Menu_RenderSaveGame", 249, 2),
    type="int32_t",
    doc="Packed save-menu transition/countdown dword; native code accesses individual byte lanes.",
)
stable.data("menu_pause_delay", xref("Menu_CheckPauseInput", 90, 1))
stable.data(
    "controls_menu_key_index",
    xref("Menu_HandleOptionsLogic", 164, 1),
    type="int16_t",
    doc="Two-byte controls prompt descriptor filled by Menu_RenderButtonPrompt.",
)
stable.data(
    "controls_menu_button_index",
    xref("Menu_HandleOptionsLogic", 159, 1),
    type="int16_t",
    doc="Second two-byte controls prompt descriptor filled by Menu_RenderButtonPrompt for duplicate/conflict checks.",
)
stable.data(
    "backup_puppy_count",
    xref("Level_InitializeSaveState", 15, 1),
    xref("Level_InitializeSaveState", 20, 2),
)
stable.data("menu_selection_2", xref("SaveGame_SaveLevelCompletion", 269, 2))
stable.data("puppy_counter_ui_state", xref("Menu_UpdatePauseMenu", 248, 3))
stable.data(
    "menu_input_up",
    xref("Menu_UpdateInput", 18, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input up pulse dword at pcdogs.exe+0x9BAA4 (PC EN) in the pcdogs.exe+0x9BAA4 "
        "(PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_down",
    xref("Menu_UpdateInput", 23, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input down pulse dword at pcdogs.exe+0x9BAA8 (PC EN) in the pcdogs.exe+0x9BAA4 "
        "(PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_left",
    xref("Menu_UpdateInput", 13, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input left pulse dword at pcdogs.exe+0x9BAAC (PC EN) in the pcdogs.exe+0x9BAA4 "
        "(PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_right",
    xref("Menu_UpdateInput", 8, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input right pulse dword at pcdogs.exe+0x9BAB0 (PC EN) in the "
        "pcdogs.exe+0x9BAA4 (PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_confirm",
    xref("Menu_UpdateInput", 33, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input confirm pulse dword at pcdogs.exe+0x9BAB4 (PC EN) in the "
        "pcdogs.exe+0x9BAA4 (PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_cancel",
    xref("Menu_UpdateInput", 28, 1),
    type="int32_t",
    doc=(
        "One-shot menu-input cancel pulse dword at pcdogs.exe+0x9BAB8 (PC EN) in the "
        "pcdogs.exe+0x9BAA4 (PC EN)..pcdogs.exe+0x9BAB8 (PC EN) pulse cluster."
    ),
)
stable.data(
    "menu_input_up_held",
    xref("Menu_UpdateInput", 190, 1),
    doc=(
        "Held/debounce up byte at pcdogs.exe+0x9BABE (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "menu_input_down_held",
    xref("Menu_UpdateInput", 165, 2),
    doc=(
        "Held/debounce down byte at pcdogs.exe+0x9BABF (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "menu_input_left_held",
    xref("Menu_UpdateInput", 53, 2),
    doc=(
        "Held/debounce left byte at pcdogs.exe+0x9BAC1 (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "menu_input_right_held",
    xref("Menu_UpdateInput", 90, 2),
    doc=(
        "Held/debounce right byte at pcdogs.exe+0x9BAC0 (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "menu_input_cancel_held",
    xref("Menu_UpdateInput", 115, 2),
    doc=(
        "Held/debounce cancel byte at pcdogs.exe+0x9BABD (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "menu_input_confirm_held",
    xref("Menu_UpdateInput", 140, 2),
    doc=(
        "Held/debounce confirm byte at pcdogs.exe+0x9BABC (PC EN) in the pcdogs.exe+0x9BABC (PC "
        "EN)..pcdogs.exe+0x9BAC1 (PC EN) cluster."
    ),
)
stable.data(
    "options_menu_column",
    xref("Menu_HandleOptionsLogic", 225, 1),
    type="int32_t",
    doc="Options/control-remap column cursor dword used by Menu_HandleOptionsLogic.",
)
stable.data(
    "options_menu_selection",
    xref("Menu_HandleOptionsLogic", 58, 2),
    xref("Level_UpdateInterLevelMenu", 181, 3),
    type="int32_t",
    doc="Selected options-menu row dword at pcdogs.exe+0x9BAC8 (PC EN).",
)
stable.data(
    "button_remapping_active",
    xref("Menu_HandleOptionsLogic", 515, 1),
    type="int32_t",
    doc="Control-remapping active/latch dword in the options submenu state cluster.",
)
stable.data(
    "options_menu_state",
    xref("Menu_HandleOptionsLogic", 327, 2),
    type="int32_t",
    doc="Auxiliary options-menu UI state dword at pcdogs.exe+0x9BADC (PC EN).",
)
stable.data(
    "menu_ui_state_5",
    xref("Menu_HandleOptionsLogic", 46, 2),
    type="int32_t",
    doc="Auxiliary options-menu UI state dword read by Menu_HandleOptionsLogic.",
)
stable.data(
    "options_menu_sub_state",
    xref("Menu_HandleOptionsLogic", 0, 1),
    type="int32_t",
    doc="Options/control-remap substate dword read at Menu_HandleOptionsLogic entry.",
)
stable.data("sorted_render_list_flags", xref("Render_DrawSortedLists", 10, 1))
stable.data("menu_render_state", xref("Render_DrawSortedLists", 21, 1))
stable.data("rendering_frame_counter", xref("Render_IncrementPassCounter", 0, 1))
stable.data("loading_screen_state", xref("Pkg_LoadRandomSplashScreen", 0, 1))
stable.data("level_select_state", xref("Level_UpdateWorldSelectMenu", 1, 1))
stable.data("level_select_slot", xref("Level_UpdateWorldSelectMenu", 70, 3))
stable.data("level_select_fade_counter", xref("Level_UpdateWorldSelectMenu", 10, 1))
stable.data("cheat_code_progress", xref("Input_CheckCheatCodeSequence", 11, 1))
stable.data("loading_blend_texture_ptr", xref("Pkg_LoadRandomSplashScreen", 67, 1))
stable.data(
    "level_resource_handle",
    xref("Resource_CleanupGameState", 0, 1),
    doc="Global latch for the completed level resource/blob handle returned by Level_LoadStateMachine; Resource_CleanupGameState passes the non-null handle to Level_UnloadResources and then clears it.",
)
stable.data("loading_fade_counter", xref("Pkg_LoadRandomSplashScreen", 79, 2))
stable.data("main_menu_state", xref("Level_Load", 175, 2))
stable.data("main_menu_selection", xref("Level_UpdateInterLevelMenu", 266, 1))
stable.data("menu_fade_timer", xref("Level_UpdateInterLevelMenu", 18, 1))
stable.data("last_loading_image_index", xref("Pkg_LoadRandomSplashScreen", 19, 3))
stable.data("loading_image_resource_ptr", xref("Pkg_LoadRandomSplashScreen", 53, 1))
stable.data("loading_fade_delay", xref("UI_Update", 52, 2))
stable.data("skip_title_screen", xref("Menu_ProcessMenuTransition", 143, 2))
stable.data("cheat_sequence_index", xref("Input_CheckCheatCodeSequence", 20, 2))
stable.data("cheat_previous_button", xref("Input_CheckCheatCodeSequence", 5, 2))
stable.data("collision_object_list", xref("Collision_DetectActorCollisions", 65, 1))
stable.data(
    "collision_ground_check_result", xref("Collision_ProcessActorGroundCheck", 68, 1)
)
stable.data(
    "collision_test_pos_x", xref("Collision_DetectObjectNodeCollisions", 217, 2)
)
stable.data(
    "collision_test_pos_y", xref("Collision_DetectObjectNodeCollisions", 244, 2)
)
stable.data(
    "collision_test_pos_z", xref("Collision_DetectObjectNodeCollisions", 258, 2)
)
stable.data(
    "collision_test_radius", xref("Collision_DetectObjectNodeCollisions", 143, 1)
)
stable.data(
    "collision_test_vel_x", xref("Collision_DetectObjectNodeCollisions", 131, 2)
)
stable.data(
    "collision_test_vel_y", xref("Collision_DetectObjectNodeCollisions", 153, 2)
)
stable.data(
    "collision_test_vel_z", xref("Collision_DetectObjectNodeCollisions", 187, 2)
)
stable.data(
    "collision_response_buffer", xref("Collision_DetectAndResolve3DCollision", 6854, 3)
)
stable.data("collision_plane_pointers", xref("Actor_InitializeDirectionTables", 5, 1))
stable.data(
    "collision_plane_buffer", xref("Collision_ResolveObjectNodeCollision", 508, 1)
)
stable.data(
    "collision_ground_normal_y", xref("Collision_ProcessActorGroundCheck", 79, 2)
)
stable.data(
    "collision_object_count",
    xref("Render_IsPolygonInDebugList", 12, 2),
    xref("Collision_DetectActorCollisions", 51, 2),
)
stable.data(
    "collision_ground_normal_x", xref("Collision_ProcessActorGroundCheck", 52, 1)
)
stable.data(
    "collision_ground_normal_z", xref("Collision_ProcessActorGroundCheck", 57, 2)
)
stable.data("collision_response_planes", xref("Actor_InitializeDirectionTables", 15, 1))
stable.data("collision_ground_dist", xref("Collision_ProcessActorGroundCheck", 96, 2))
stable.data(
    "actor_default_update_callback_slot",
    xref("Render_InitDispatchTables", 29, 2),
    xref("Collision_DetectAndResolve3DCollision", 6038, 3),
    type="Actor_DefaultUpdateCallback",
    doc=(
        "Slot 0 of the sparse collision-state callback vector, initialized to "
        "Actor_DefaultUpdateHandler by Render_InitDispatchTables; slots at +8 (PC EN)/+12 (PC EN)/+16 (PC EN) "
        "hold actor, component/projectile, and powerup collision callbacks."
    ),
)
stable.data(
    "collision_dispatch_actor_func",
    xref("Level_InitializeActorSystem", 17, 2),
    type="Collision_ActorResponseCallback",
    doc="Actor collision response callback slot initialized to Actor_ProcessCollisionResponse; part of the sparse collision-state callback vector.",
)
stable.data(
    "collision_response_actor_func",
    xref("Collision_InitializeFunctionPointers", 10, 2),
    type="Collision_ComponentResponseCallback",
    doc="Component/projectile collision response callback slot initialized to Collision_ProcessProjectileHit.",
)
stable.data(
    "powerup_collision_handler",
    xref("Collision_ProcessPowerupCollisions", 3, 1),
    type="Powerup_CollisionCallback",
    doc=(
        "Four-argument Powerup collision filter callback slot initialized by "
        "Powerup_InitializeSystem; Collision_ProcessPowerupCollisions passes "
        "(powerup_actor, actor, 0, -2)."
    ),
)
stable.data(
    "audio_system_flag",
    xref("Audio_SetEnabledFlag", 4, 1),
    type="uint8_t",
    doc="Byte-sized audio enabled flag stored by Audio_SetEnabledFlag and read by Audio_GetEnabledFlag.",
)
stable.data(
    "open_stream_count",
    xref("Audio_ShutdownSystem", 61, 1),
    type="int32_t",
    doc="Count of open Miles streams, incremented by Audio_OpenStream and decremented by Audio_CloseMusicStream.",
)
stable.data(
    "sound_slots",
    xref("Audio_StartSoundPlayback", 112, 2),
    type="Audio_SoundSlot",
    doc=(
        "Base entry of the nine-slot Audio_SoundSlot array at pcdogs.exe+0x9BDE0 (PC EN). "
        "Audio_InitializeSystem seeds the allocation cursor at pcdogs.exe+0x9BDE8 (PC EN), eight bytes after base, "
        "then writes the leading fields at cursor - 8/cursor - 4; SDK typed globals "
        "resolve to the array base."
    ),
)
stable.data(
    "sound_slot_0_sample_handle",
    xref("Audio_ShutdownSystem", 17, 1),
    type="Audio_AIL_HSample",
    doc="Sample-handle field of sound_slots[0]; Audio_ShutdownSystem iterates this field with Audio_SoundSlot stride 0x14 through all nine sound slots.",
)
stable.data(
    "music_stream_handle",
    xref("Audio_SetStreamVolume", 0, 2),
    type="Audio_AIL_HStream",
    doc="Active Miles music stream handle consumed by set-volume/play/pause/resume/close helpers.",
)
stable.data(
    "music_selected_stream_record",
    xref("Audio_ResetMusicState", 4, 1),
    type="int32_t*",
    doc="Selected/pending music stream record pointer stored by Audio_ResetMusicState and later passed to Audio_PlayMusicStream by Audio_ProcessMusicFade.",
)
stable.data("active_wave_count", xref("Audio_ShutdownSystem", 66, 2))
stable.data(
    "sound_system_flags",
    xref("Audio_FadeOutMusic", 0, 2),
    type="uint8_t",
    doc="Byte-sized audio/music state flags used by music fade/transition routines.",
)
stable.data(
    "music_transition_target",
    xref("Audio_ShutdownSystem", 47, 2),
    type="int16_t",
    doc="Signed 16-bit music transition target/state value cleared during audio shutdown.",
)
stable.data(
    "music_transition_state",
    xref("Audio_StartMusicWithFade", 7, 3),
    type="int16_t",
    doc="Signed 16-bit current music fade/transition volume state advanced by Audio_ProcessMusicFade.",
)
stable.data(
    "music_fade_target_volume",
    xref("Audio_SetMusicFadeTarget", 7, 3),
    type="int16_t",
    doc="Signed 16-bit target volume used by Audio_SetMusicFadeTarget and Audio_ProcessMusicFade.",
)
stable.data(
    "bone_trail_current_entity",
    xref("BoneTrail_Reset", 18, 2),
    doc=(
        "Bone-trail current nav/entity/powerup selector. BoneTrail_FindPath writes it from "
        "nav node metadata, and BoneTrail_UpdateAndRender maps the value into current_level_data "
        "powerup/entity runtime data."
    ),
)
stable.data("bone_trail_path_node_count", xref("BoneTrail_Reset", 24, 2))
stable.data("bone_trail_path_end", xref("BoneTrail_FindPath", 607, 1))
stable.data("bone_trail_timer", xref("BoneTrail_CheckAvailable", 19, 2))
stable.data("bone_trail_entries", xref("BoneTrail_UpdateAndRender", 1175, 1))
stable.data("bone_trail_entry_0_timestamp", xref("BoneTrail_Reset", 30, 1))
stable.data("bone_trail_entry_0_pos_x", xref("BoneTrail_UpdateAndRender", 1035, 1))
stable.data("bone_trail_path_nodes", xref("BoneTrail_UpdateAndRender", 949, 6))
stable.data("bone_trail_path_buffer_y", xref("BoneTrail_UpdateAndRender", 1099, 2))
stable.data("bone_trail_path_buffer_z", xref("BoneTrail_UpdateAndRender", 1054, 1))
stable.data("bone_trail_target_position", xref("BoneTrail_UpdateAndRender", 311, 2))
stable.data("bone_trail_anim_time", xref("BoneTrail_Reset", 40, 1))
stable.data("bone_trail_path_buffer_z_1", xref("BoneTrail_UpdateAndRender", 343, 2))
stable.data("bone_trail_path_buffer_x_2", xref("BoneTrail_FindPath", 154, 1))
stable.data("bone_trail_path_buffer_y_2", xref("BoneTrail_FindPath", 1881, 1))
stable.data("bone_trail_path_buffer_z_2", xref("BoneTrail_FindPath", 1888, 2))
stable.data("special_node_processing_flag", xref("Resource_FixUpMeshNode", 573, 2))
stable.data(
    "save_game_operation_state",
    xref("SaveGame_ProcessOperation", 61, 2),
    type="uint32_t",
    doc=(
        "Packed active save I/O operation/status word initialized by SaveGame_InitOperation "
        "and polled by SaveGame_ProcessOperation; byte 0 carries the result/status, byte 1 the success flag, "
        "byte 2 the requested operation, and byte 3 the file operation code."
    ),
)
stable.data(
    "save_game_operation_buffer",
    xref("SaveGame_ProcessOperation", 93, 2),
    type="uint8_t*",
    doc=(
        "Async save-operation buffer pointer at pcdogs.exe+0x9C41C (PC EN). The static "
        "save-file/slot span rooted at pcdogs.exe+0x9B798 (PC EN) is consistent with "
        "five slot-sized records."
    ),
)
stable.data(
    "save_game_size",
    xref("SaveGame_ProcessOperation", 41, 1),
    type="uint32_t",
    doc="Byte count passed to save read/write/verify helpers by SaveGame_ProcessOperation.",
)
stable.data(
    "save_game_verify_buffer",
    xref("SaveGame_ProcessOperation", 46, 2),
    type="uint8_t*",
    doc="Comparison buffer used by SaveGame_ProcessOperation's operation 12 verify path.",
)
stable.data(
    "video_skip_requested",
    xref("Movie_PlayFile", 60, 2),
    type="int32_t",
    doc="Movie playback skip/shutdown flag cleared by Movie_PlayIntro and set by Movie_PlayFile when the user skips or closes playback.",
)
stable.data(
    "avi_playback_started",
    xref("Movie_PlayIntro", 274, 2),
    type="int32_t",
    doc="AVI playback-start latch toggled by Movie_PlayIntro around Video_IsAVIPlaying during the AVI fallback path.",
)
stable.data("window_message_state", xref("Game_WindowProc", 16, 2))
stable.data("show_fps_counter", xref("Render_DrawSortedLists", 159, 1))
stable.data(
    "keyboard_mapping_keys",
    xref("Input_RegisterButtonMapping", 32, 1),
    type="int32_t*",
    doc="Heap array of registered keyboard virtual-key codes, grown by Input_RegisterButtonMapping.",
)
stable.data(
    "keyboard_mapping_buttons",
    xref("Input_RegisterButtonMapping", 62, 1),
    type="uint32_t*",
    doc="Heap array parallel to keyboard_mapping_keys; each entry is the Input_State.button_bits mask for that key.",
)
stable.data("last_frame_tick", xref("Render_Frame", 165, 2))
stable.data("input_processed_flag", xref("Render_DrawQuad", 906, 2))
stable.data(
    "frame_start_time_sec",
    xref("Render_Frame", 14, 2),
    type="float",
    doc="Timer_GetGameTime value captured at the start of Render_Frame.",
)
stable.data(
    "frame_end_time_sec",
    xref("Render_Frame", 450, 2),
    type="float",
    doc="Timer_GetGameTime value captured after rendering/flip handling in Render_Frame.",
)
stable.data(
    "current_fps",
    xref("Debug_RenderOverlay", 147, 2),
    type="float",
    doc="Frames-per-second value rendered by the debug overlay after Render_Frame updates the accumulator.",
)
stable.data(
    "fps_accumulated_frame_time",
    xref("Render_Frame", 460, 2),
    type="float",
    doc="Accumulated frame-time seconds used with fps_frame_count to refresh current_fps.",
)
stable.data(
    "fps_frame_count",
    xref("Render_Frame", 491, 1),
    type="int32_t",
    doc="Number of frames accumulated since the last current_fps refresh.",
)
stable.data(
    "level_menu_load_state",
    xref("Resource_CleanupGameState", 29, 2),
    type="uint8_t",
    doc=(
        "Top-level menu/load dispatch state byte used by Level_Load, UI_Update, Resource_CleanupGameState, "
        "and transition helpers."
    ),
)
stable.data(
    "level_stream_load_state",
    xref("Level_LoadStateMachine", 12, 1),
    type="int32_t",
    doc=(
        "Async package-stream loader stage dword used only by Level_LoadStateMachine; stages 0,1,2,4,5,7 "
        "load level TOC entries and then reset this state to zero."
    ),
)
stable.data(
    "level_tex_data_a",
    xref("Level_LoadStateMachine", 32, 1),
    type="void*",
    doc="Opaque first level package-entry buffer loaded from TOC entry 0x24 + level_index * 3 by Level_LoadStateMachine stage 0.",
)
stable.data(
    "level_tex_data_b",
    xref("Level_LoadStateMachine", 104, 1),
    type="void*",
    doc="Opaque second level package-entry buffer loaded from TOC entry 0x25 + level_index * 3 by Level_LoadStateMachine stage 2.",
)
stable.data("debug_logging_enabled", xref("Resource_FixUpLevelPointers", 0, 1))
stable.data(
    "pkg_file_handle",
    xref("PKG_LoadEntry", 76, 1),
    type="File_Handle*",
    doc="Open package File_Handle consumed by PKG_LoadEntry while reading aligned package entries.",
)
stable.data(
    "pkg_toc",
    xref("PKG_LoadEntry", 69, 3),
    type="Pkg_TOCEntry",
    doc=(
        "Base of the 0x8a-entry / 0x450-byte package table of contents at "
        "pcdogs.exe+0x9CA80 (PC EN); each 8-byte Pkg_TOCEntry stores file offset and size."
    ),
)
stable.data(
    "package_toc_file_sizes",
    xref("PKG_LoadEntry", 7, 3),
    type="uint32_t",
    doc=(
        "Size-field view at pkg_toc + 4 (PC EN) used by PKG_LoadEntry; this overlaps the "
        "Pkg_TOCEntry.size lane within the package TOC allocation."
    ),
)
stable.data("tree_map_buckets", xref("TreeMap_Rebalance", 21, 3))
stable.data("timed_signal_list_head", xref("Timer_ClearEventList", 0, 2))
stable.data("signal_queue_count", xref("Input_ClearEventQueue", 0, 2))
stable.data("signal_queue", xref("Signal_Poll", 29, 1))
stable.data(
    "sound_playback_rate_table",
    xref("Audio_ProcessSoundQueue", 498, 4),
    type="uint16_t",
    doc="First halfword/base of the 9-entry sound pitch/playback-rate cache used before AIL_set_sample_playback_rate.",
)
stable.data(
    "sound_entries",
    xref("Audio_FreeSoundSlot", 10, 1),
    type="Audio_SoundEntry",
    doc="Base of the nine-entry active/free sound-entry pool; Audio_AllocateSoundSlot computes slot indices from this base with stride 0x30.",
)
stable.data(
    "sound_slot_reserved",
    xref("Audio_AllocateSoundSlot", 17, 1),
    type="Audio_SoundEntry",
    doc="Ninth Audio_SoundEntry at sound_entries[8], reserved for Audio_SoundDefinition flags bit 0x80 before being linked into the active list.",
)
stable.data(
    "active_sound_list",
    xref("Audio_FreeSoundSlot", 51, 2),
    type="Audio_SoundEntry*",
    doc="Head pointer for the doubly linked active Audio_SoundEntry list maintained by Audio_AllocateSoundSlot and Audio_FreeSoundSlot.",
)
stable.data(
    "sound_slot_tail",
    xref("Audio_FreeSoundSlot", 74, 2),
    type="Audio_SoundEntry*",
    doc="Tail pointer for the active Audio_SoundEntry list; new allocated entries are appended here.",
)
stable.data(
    "sound_slot_list_ptr",
    xref("Audio_FreeSoundSlot", 92, 2),
    type="Audio_SoundEntry*",
    doc="Head pointer for the free Audio_SoundEntry list read by Audio_AllocateSoundSlot and replenished by Audio_FreeSoundSlot.",
)
stable.data(
    "sound_timer",
    xref("Audio_UpdateSoundChannels", 163, 1),
    type="int32_t",
    doc="Last Timer_GetElapsedTickCount value captured by Audio_UpdateSoundChannels for per-channel sound timing updates.",
)
stable.data(
    "demo_replay_input_ptr",
    xref("Recording_StartPlayback", 14, 2),
    type="int32_t*",
    doc="Pointer to the current demo replay input-frame stream, loaded from demo_replay_data[1] by Recording_StartPlayback when replay playback begins.",
)
stable.data(
    "frame_counter",
    xref("Audio_TriggerMusicTransition", 96, 2),
    type="int32_t",
    doc="Data frame counter used by music fade/transition timing and other frame-based game state checks.",
)
stable.data(
    "input_system_flags",
    xref("Audio_TriggerMusicTransition", 12, 2),
    type="uint32_t",
    doc=(
        "Shared frame/input/audio transition bitfield: bit 0x20 marks demo replay playback, bit 0x10 selects "
        "alternate 3D-audio listener camera data, bit 0x400 is set by Audio_TriggerMusicTransition, bit 0x08 "
        "requests unload before being cleared after Resource_UnloadGameData, bit 0x1000 allows cleanup/load "
        "rendering, bit 0x4000 requests Level_Load, bit 0x04 marks post-load actor/audio initialization, and "
        "bit 0x02 allows active scene update/render."
    ),
)
stable.data(
    "string_table",
    xref("String_SetTable", 4, 1),
    type="int16_t*",
    doc="Active package/localization string table pointer consumed by String_GetByIndex; split from input button-name buffers.",
)
stable.data(
    "input_button_name_buffers",
    xref("Menu_RenderFormattedText", 50, 3),
    type="char*",
    doc="First entry/base of the heap-allocated input button-name buffer pointer array.",
)
stable.data("spots_active_count", xref("Spots_Initialize", 14, 3))
stable.data(
    "title_music_data",
    xref("Pkg_LoadTitleScreenResources", 151, 2),
    type="int32_t*",
    doc="Title-screen music stream-record pointer loaded from the title package and armed by Audio_StartMusicWithFade.",
)
stable.data("title_material_base", xref("Pkg_LoadTitleScreenResources", 85, 2))
stable.data("spots_cycle_length", xref("Spots_Initialize", 422, 2))
stable.data("title_screen_counter", xref("TitleScreen_UpdateAndRender", 130, 3))
stable.data("spots_material_index", xref("Spots_Initialize", 63, 1))
stable.data("title_resource_package", xref("Pkg_LoadTitleScreenResources", 62, 1))
stable.data("title_screen_fade_level", xref("TitleScreen_UpdateAndRender", 168, 3))
stable.data("spots_data_array", xref("Spots_Initialize", 23, 1))
stable.data("spots_timer_array", xref("Spots_Initialize", 338, 4))
stable.data("title_bonus_replay_resource", xref("TitleScreen_CleanupResources", 12, 1))
stable.data("title_screen_state", xref("TitleScreen_UpdateAndRender", 16, 3))
stable.data("title_resource_handle_1", xref("Pkg_LoadTitleScreenResources", 36, 1))
stable.data("title_resource_handle_0", xref("Pkg_LoadTitleScreenResources", 12, 1))
stable.data("spots_sound_id", xref("Spots_Update", 92, 1))
stable.data("spots_frame_counter", xref("Spots_Initialize", 7, 3))
stable.data("memory_handle_pool", xref("Memory_AllocateHandle", 82, 2))
stable.data("mem_handle_pool_handle_id", xref("Memory_InitializeAllocator", 10, 1))
stable.data("mem_allocator_initialized", xref("Memory_InitializeAllocator", 45, 2))
stable.data("memory_handle_pool_head", xref("Memory_AllocateHandle", 1, 2))
stable.data(
    "input_state_buffer",
    xref("Input_ClearState", 8, 1),
    type="uint8_t[0x100]",
    doc=(
        "0x100-byte raw input/VK state clear buffer zeroed by Input_ClearState. "
        "Public fixed-array accessors use pointer-to-array read/write signatures."
    ),
)
stable.data("current_display_mode", xref("Display_IsActive", 0, 2))
stable.data("float_format_precision", xref("Float_ConvertToExponential", 17, 1))
stable.data("float_format_flags", xref("Float_ConvertToExponential", 3, 2))
stable.data("mem_debug_enabled", xref("Memory_AllocateHandle", 105, 2))
stable.data("camera_data", xref("Level_InitializeActorSystem", 509, 1))
stable.data("player_facing_angle", xref("Player_ProcessMovement", 345, 3))
stable.data(
    "camera_yaw_angle",
    xref("Player_ProcessMovement", 354, 3),
    type="uint32_t",
    doc=(
        "Packed camera yaw global used by Player_ProcessMovement. The high 16 bits hold "
        "the signed yaw angle; the low 16 bits are reserved, and a zero high word is "
        "a valid yaw sample."
    ),
)
stable.data("camera_viewport_height", xref("Camera_SetViewport", 54, 2))
stable.data("camera_far_clip_plane", xref("Camera_SetViewport", 60, 2))
stable.data(
    "player_1_camera_pos",
    xref("Checkers_UpdateStateMachine", 115, 6),
    type="Math_Vec3i32",
    doc="Primary listener/camera position vector at pcdogs.exe+0x224350 (PC EN); used by positional-audio panning when input_system_flags bit 0x10 is set.",
)
stable.data(
    "player_2_camera_pos",
    xref("Audio_PlaySoundDefinition3D", 103, 1),
    type="Math_Vec3i32",
    doc="Alternate listener/camera position vector at pcdogs.exe+0x22435C (PC EN); used by positional-audio panning when input_system_flags bit 0x10 is clear.",
)
stable.data("graphics_flags", xref("Pkg_InitializeSystem", 20, 2))
stable.data(
    "level_init_callback_2",
    xref("Resource_UnloadGameData", 67, 2),
    doc=(
        "Untyped native level cleanup/init callback slot touched by Resource_UnloadGameData. "
        "Kept engine-owned and untyped until the callback signature is proven."
    ),
)
stable.data(
    "level_init_callback_1",
    xref("Resource_UnloadGameData", 61, 2),
    doc=(
        "Untyped native level cleanup/init callback slot touched by Resource_UnloadGameData. "
        "Kept engine-owned and untyped until the callback signature is proven."
    ),
)
stable.data("vertex_batch_buffer", xref("Scene_RenderFrame", 1402, 1))
stable.data(
    "input_state_previous_p_1",
    xref("Menu_UpdateInput", 0, 2),
    doc=(
        "First/base byte of the two-player previous Input_State snapshot rows at pcdogs.exe+0x234420 "
        "(PC EN); player 2 follows at +0x0C (PC EN). Scalar aliases overlap the row."
    ),
)
stable.data("input_toggle_mask_p_1", xref("Menu_ProcessMenuState", 418, 2))
stable.data("demo_replay_saved_random_seed", xref("Recording_StartPlayback", 30, 1))
stable.data(
    "input_state_current_p_1",
    xref("Camera_CalculateFollowAngles", 828, 2),
    doc=(
        "First/base byte of the two-player current Input_State snapshot rows at pcdogs.exe+0x234460 "
        "(PC EN); player 2 follows at +0x0C (PC EN). Scalar aliases overlap the row."
    ),
)
stable.data(
    "current_input_x",
    xref("Input_CalculateMovementVector", 173, 3),
    xref("Input_Update", 158, 1),
    doc=(
        "Input/movement scratch value referenced by Input_Update and "
        "Input_CalculateMovementVector."
    ),
)
stable.data("demo_replay_data", xref("DemoReplay_LoadBonusReplay", 36, 1))
stable.data("render_color_adjustment_flag", xref("Render_MeshNode", 1752, 2))
stable.data(
    "script_current_actor",
    xref("Script_MoveToTarget", 135, 2),
    type="Actor_State*",
    doc=(
        "Script-dispatch current actor context written by Script_ExecuteBehaviorScript "
        "and read by script command handlers; engine-owned transient state."
    ),
)
stable.data("script_entity_index", xref("Script_CheckTerminator", 278, 1))
stable.data(
    "model_physics_callback_table",
    xref("Model_UpdateTransformAndPhysics", 2297, 3),
    type="Actor_BehaviorCallback",
    doc="First slot/base of the eight-slot model/physics callback vector at pcdogs.exe+0x2344A0 (PC EN)..pcdogs.exe+0x2344BC (PC EN).",
)
stable.data(
    "behavior_process_actor_func",
    xref("Level_InitializeActorSystem", 47, 2),
    type="Actor_BehaviorCallback",
    doc="Actor behavior dispatch callback initialized by Level_InitializeActorSystem.",
)
stable.data(
    "behavior_process_projectile_func",
    xref("Collision_InitializeFunctionPointers", 20, 2),
    type="Actor_BehaviorCallback",
    doc=(
        "Projectile behavior lifecycle callback slot initialized to "
        "Collision_ProcessProjectileLifecycle; the native return is data-var status."
    ),
)
stable.data(
    "behavior_process_snap_func",
    xref("Level_InitializeActorSystem", 57, 2),
    doc="Snap/entity update callback slot initialized to Actor_ProcessSnapAndEntityUpdate.",
)
stable.data(
    "behavior_target_actor",
    xref("Render_InitDispatchTables", 49, 2),
    type="Actor_BehaviorCallback",
    doc="Behavior/movement callback slot initialized to Actor_ApplyVerticalMovement by Render_InitDispatchTables.",
)
stable.data(
    "behavior_param_0",
    xref("Render_InitDispatchTables", 59, 2),
    type="Actor_BehaviorCallback",
    doc="Behavior/movement callback slot initialized to Actor_ProcessMovementCommands by Render_InitDispatchTables.",
)
stable.data(
    "behavior_param_1",
    xref("Render_InitDispatchTables", 69, 2),
    type="Actor_BehaviorCallback",
    doc="Behavior/movement callback slot initialized to Actor_FollowAttachedMovement by Render_InitDispatchTables.",
)
stable.data(
    "behavior_param_2",
    xref("Render_InitDispatchTables", 79, 2),
    type="Actor_BehaviorCallback",
    doc="Behavior/movement callback slot initialized to Actor_ProcessMovementBehavior by Render_InitDispatchTables.",
)
stable.data(
    "powerup_update_func",
    xref("Powerup_InitializeSystem", 9, 2),
    type="Powerup_UpdateCallback",
    doc="Powerup actor update callback slot initialized by Powerup_InitializeSystem.",
)
stable.data(
    "movement_handler_table",
    xref("Physics_UpdateActorPreprocess", 141, 3),
    type="Actor_BehaviorCallback",
    doc="Read-only base of the movement handler callback table indexed by actor movement/collision state at pcdogs.exe+0x2344D0 (PC EN). unload_delay_counter at pcdogs.exe+0x2344DC (PC EN) bounds the known range.",
)
stable.data(
    "player_movement_func",
    xref("Level_InitializeActorSystem", 37, 2),
    type="Actor_BehaviorCallback",
    doc="Player movement callback slot initialized by Level_InitializeActorSystem.",
)
stable.data(
    "projectile_logic_func",
    xref("Collision_InitializeFunctionPointers", 0, 2),
    doc="Projectile behavior lifecycle callback slot initialized to Collision_ProcessProjectileLifecycle.",
)
stable.data("unload_delay_counter", xref("Menu_RenderSaveGame", 93, 1))
stable.data(
    "powerup_actor_list_head",
    xref("Collision_ProcessPowerupCollisions", 9, 2),
    doc=(
        "Live powerup actor list head linked by Powerup_CloneActor. Powerup collision "
        "handling and clone-source templates use separate SDK symbols."
    ),
)
stable.data("powerup_spawn_delay", xref("Powerup_HandleCollection", 92, 2))
stable.data("graphics_capabilities", xref("Render_ProcessMeshCommands", 59, 2))
stable.data(
    "pkg_base_path",
    xref("Audio_OpenStream", 34, 1),
    type="char[0x104]",
    doc="Mutable NUL-terminated package base path buffer used to build package and data/music stream paths.",
)
stable.data("render_batch_triangle_count", xref("Render_QuadClipped", 2077, 2))
stable.data("win_main_instance_handle", xref("WinMain", 540, 1))
stable.data("render_batch_primitive_count", xref("Render_QuadClipped", 1999, 1))
stable.data("accelerator_table", xref("Input_ProcessWindowMessages", 139, 1))
stable.data("win_main_show_cmd", xref("WinMain", 396, 2))
stable.data("render_frame_count", xref("Render_DrawQuad", 2973, 2))
stable.data("localization_language_id", xref("String_GetByIndex", 161, 1))
stable.data("string_table_loaded", xref("String_GetByIndex", 124, 1))
stable.data("string_buffer_ptr", xref("Menu_RenderConfirmPrompt", 80, 2))
stable.data("string_table_size", xref("String_GetByIndex", 97, 2))
stable.data(
    "no_key_assigned_string",
    xref("Input_FormatButtonName", 92, 1),
    type="char*",
    doc=(
        'Cached heap string for "No key assigned"; slot 12 of input_button_name_buffers aliases this '
        "address, but the label remains a separate symbol."
    ),
)
stable.data(
    "ui_initialized_flag", xref("Menu_AnimateSlots", 292, 1), xref("UI_Update", 0, 1)
)
stable.data("render_frame_index", xref("Render_Frame", 290, 2))
stable.data(
    "mapping_count",
    xref("Input_RegisterButtonMapping", 26, 2),
    type="int32_t",
    doc="Number of entries in the keyboard_mapping_keys/keyboard_mapping_buttons arrays.",
)
stable.data(
    "gamepad_axis_y_negative_mask",
    xref("Input_ReadGamepad", 134, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3e8; ORed when DInput_JoystickState.lY < -700.",
)
stable.data(
    "gamepad_axis_y_positive_mask",
    xref("Input_ReadGamepad", 154, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3e9; ORed when DInput_JoystickState.lY > 700.",
)
stable.data(
    "gamepad_axis_x_negative_mask",
    xref("Input_ReadGamepad", 95, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ea; ORed when DInput_JoystickState.lX < -700.",
)
stable.data(
    "gamepad_axis_x_positive_mask",
    xref("Input_ReadGamepad", 114, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3eb; ORed when DInput_JoystickState.lX > 700.",
)
stable.data(
    "gamepad_axis_rz_negative_mask",
    xref("Input_ReadGamepad", 501, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ec; ORed when DInput_JoystickState.lRz < -600.",
)
stable.data(
    "gamepad_axis_rz_positive_mask",
    xref("Input_ReadGamepad", 521, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ed; ORed when DInput_JoystickState.lRz > 600.",
)
stable.data(
    "gamepad_button_0_mask",
    xref("Input_ReadGamepad", 553, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ee; ORed when DInput_JoystickState.rgbButtons[0] is pressed.",
)
stable.data(
    "gamepad_button_1_mask",
    xref("Input_ReadGamepad", 586, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ef; ORed when DInput_JoystickState.rgbButtons[1] is pressed.",
)
stable.data(
    "gamepad_button_2_mask",
    xref("Input_ReadGamepad", 618, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f0; ORed when DInput_JoystickState.rgbButtons[2] is pressed.",
)
stable.data(
    "gamepad_button_3_mask",
    xref("Input_ReadGamepad", 650, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f1; ORed when DInput_JoystickState.rgbButtons[3] is pressed.",
)
stable.data(
    "gamepad_button_4_mask",
    xref("Input_ReadGamepad", 683, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f2; ORed when DInput_JoystickState.rgbButtons[4] is pressed.",
)
stable.data(
    "gamepad_button_5_mask",
    xref("Input_ReadGamepad", 715, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f3; ORed when DInput_JoystickState.rgbButtons[5] is pressed.",
)
stable.data(
    "gamepad_button_6_mask",
    xref("Input_ReadGamepad", 747, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f4; ORed when DInput_JoystickState.rgbButtons[6] is pressed.",
)
stable.data(
    "gamepad_button_7_mask",
    xref("Input_ReadGamepad", 780, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f5; ORed when DInput_JoystickState.rgbButtons[7] is pressed.",
)
stable.data(
    "gamepad_button_8_mask",
    xref("Input_ReadGamepad", 812, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f6; ORed when DInput_JoystickState.rgbButtons[8] is pressed.",
)
stable.data(
    "gamepad_button_9_mask",
    xref("Input_ReadGamepad", 844, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f7; ORed when DInput_JoystickState.rgbButtons[9] is pressed.",
)
stable.data(
    "gamepad_button_10_mask",
    xref("Input_ReadGamepad", 877, 1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f8; ORed when DInput_JoystickState.rgbButtons[10] is pressed.",
)
stable.data(
    "gamepad_button_11_mask",
    xref("Input_ReadGamepad", 909, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f9; ORed when DInput_JoystickState.rgbButtons[11] is pressed.",
)
stable.data(
    "gamepad_button_12_mask",
    xref("Input_ReadGamepad", 941, 2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3fa; ORed when DInput_JoystickState.rgbButtons[12] is pressed.",
)
stable.data(
    "display_setting",
    xref("Menu_HandleOptionsLogic", 895, 1),
    type="uint8_t",
    doc="Saved display/detail setting byte from pcdogs.ini; clamped to 0..10 before being applied.",
)
stable.data(
    "player_1_controls",
    xref("Config_ApplySettings", 119, 2),
    type="int32_t",
    doc=(
        "First dword/base of the 13-dword player-1 pcdogs.ini binding block; Config_ApplySettings applies "
        "the first 10 normal mappings."
    ),
)
stable.data("player_1_down_key", xref("Input_InitializeButtonMappings", 359, 2))
stable.data(
    "input_button_map",
    xref("Input_InitializeButtonMappings", 369, 2),
    type="int32_t[8]",
    doc="Eight adjacent player-1 button mapping dwords after the down-key entry.",
)
stable.data(
    "special_button",
    xref("Config_ApplySettings", 143, 2),
    type="int32_t",
    doc="Additional pcdogs.ini button binding mapped to Input_State mask 0x4000; defaults to VK_SPACE (0x20) when unset.",
)
stable.data(
    "player_2_controls",
    xref("Config_ApplySettings", 87, 2),
    type="int32_t",
    doc=(
        "First dword/base of the 13-dword player-2/gamepad pcdogs.ini binding block; Config_ApplySettings "
        "applies the first 10 normal mappings."
    ),
)
stable.data("player_2_down_button", xref("Input_InitializeButtonMappings", 469, 2))
stable.data(
    "input_button_map_alt",
    xref("Input_InitializeButtonMappings", 479, 2),
    type="int32_t[8]",
    doc="Eight adjacent player-2/gamepad button mapping dwords after the down-button entry.",
)
stable.data(
    "config_file_path",
    xref("Movie_PlayIntro", 142, 2),
    type="int32_t",
    doc="Post-config-block dword used as the exclusive end sentinel for pcdogs.ini control binding loops and zeroed on the AVI movie path.",
)
stable.data(
    "level_material_section",
    xref("Resource_FixUpModelNode", 277, 1),
    type="Material_SectionHeader*",
    doc="Active level material section header/base used while rebasing model-node material references.",
)
stable.data(
    "level_blob_ptr",
    xref("Resource_FixUpObjectNode", 12, 2),
    type="uint32_t*",
    doc=(
        "Active level blob relocation base loaded from TOC entry 0x26 + level_index * 3; used while rebasing "
        "material/object/level relative offsets and returned by Level_LoadStateMachine after final fixups."
    ),
)
stable.data("shadow_render_list", xref("Shadow_ClearList", 0, 2))
stable.data("powerup_collision_list_head", xref("Script_PauseToggle", 467, 1))
stable.data(
    "collision_state_handler_table",
    xref("Collision_ProcessActorToActorCollisions", 198, 3),
    type="Collision_ProcessCallback",
    doc="Read-only first entry/base of the collision-state callback table indexed by actor collision subtype; slot 2 aliases collision_process_func.",
)
stable.data(
    "collision_process_func",
    xref("Level_InitializeActorSystem", 27, 2),
    type="Collision_ProcessCallback",
    doc="Engine-owned scalar actor collision processing callback slot initialized to Physics_ProcessActorCollision and aliased by collision_state_handler_table slot 2.",
)
stable.data("debug_polygon_counts", xref("Render_IsPolygonInDebugList", 0, 1))
stable.data("powerup_active_list_head", xref("Render_IsPolygonInDebugList", 39, 3))
stable.data(
    "material_buffer_offset",
    xref("Shared_LoadCommonResources", 196, 2),
    type="Material_FrameSet*",
    doc="Cursor into shared material frame-set storage advanced by Material_BuildTextureArray.",
)
stable.data("screen_fade_duration", xref("Player_RespawnAfterDeath", 105, 2))
stable.data("screen_fade_counter", xref("Audio_ProcessMusicFade", 71, 1))
stable.data("is_loading_level", xref("Player_RespawnAfterDeath", 88, 2))
stable.data("level_index", xref("Menu_ProcessMenuTransition", 87, 2))
stable.data("rendering_state_flag", xref("Level_InitializeActorSystem", 258, 3))
stable.data(
    "shared_material_section",
    xref("Shared_LoadCommonResources", 0, 1),
    type="Material_SectionHeader*",
    doc="Shared/common material section header/base loaded by Shared_LoadCommonResources.",
)
stable.data("controller_hammerhead_name", xref("Menu_RenderMusicSelection", 26, 1))
stable.data(
    "controller_hammerhead_buttons",
    xref("Menu_HandleOptionsLogic", 340, 3),
    type="int32_t[10]",
    doc="Ten adjacent Hammerhead button preset dwords inside a 0x8c-byte controller profile record.",
)
stable.data(
    "controller_sidewinder_buttons",
    xref("Input_InitializeControllerMappings", 148, 2),
    type="int32_t[10]",
    doc="Ten adjacent SideWinder button preset dwords inside a 0x8c-byte controller profile record.",
)
stable.data(
    "controller_gravis_buttons",
    xref("Input_InitializeControllerMappings", 247, 2),
    type="int32_t[10]",
    doc="Ten adjacent Gravis button preset dwords inside a 0x8c-byte controller profile record.",
)
stable.data(
    "controller_wingman_button_ref",
    xref("Input_InitializeControllerMappings", 366, 2),
    type="int32_t",
    doc=(
        "One validated WingMan button preset dword. The surrounding WingMan writes are "
        "not grouped because their xref order does not prove a contiguous public array."
    ),
)
stable.data("font_data_ptr", xref("Shared_LoadCommonResources", 184, 2))
stable.data("texture_data_refs_ptr", xref("Shared_LoadCommonResources", 176, 1))
stable.data("current_usable_materials", xref("Menu_AnimateSlots", 22, 2))
stable.data("menu_gradient_color_value", xref("Menu_ProcessMenuState", 2531, 2))
stable.data(
    "render_transform_matrix", xref("Bone_TransformWeightedVerts_ForRender", 28, 1)
)
stable.data("render_matrix_01", xref("Bone_TransformWeightedVerts_ForRender", 35, 3))
stable.data("render_matrix_02", xref("Bone_TransformWeightedVerts_ForRender", 60, 2))
stable.data("render_matrix_10", xref("Bone_TransformWeightedVerts_ForRender", 53, 3))
stable.data("render_matrix_11", xref("Bone_TransformWeightedVerts_ForRender", 161, 3))
stable.data("render_matrix_12", xref("Bone_TransformWeightedVerts_ForRender", 87, 3))
stable.data("render_matrix_20", xref("Bone_TransformWeightedVerts_ForRender", 80, 3))
stable.data("render_matrix_21", xref("Bone_TransformWeightedVerts_ForRender", 98, 3))
stable.data("render_matrix_22", xref("Bone_TransformWeightedVerts_ForRender", 205, 3))
stable.data("polygon_highlight_mode", xref("Render_PolygonBatch", 6830, 1))
stable.data("view_direction_x", xref("Render_PolygonBatch", 60, 3))
stable.data("view_direction_y", xref("Render_PolygonBatch", 50, 3))
stable.data("view_direction_z", xref("Render_PolygonBatch", 29, 3))
stable.data("current_polygon_batch_index", xref("Render_PolygonMesh", 1503, 1))
stable.data("camera_transform_matrix", xref("Render_SetPolygonUVs", 347, 3))
stable.data("transform_matrix_element_1", xref("Render_SetPolygonUVs", 318, 3))
stable.data("camera_matrix_m02", xref("Render_SetPolygonUVs", 359, 3))
stable.data("camera_matrix_m10", xref("Render_SetPolygonUVs", 304, 3))
stable.data("camera_matrix_m11", xref("Render_SetPolygonUVs", 371, 3))
stable.data("camera_matrix_m12", xref("Render_SetPolygonUVs", 383, 3))
stable.data("camera_matrix_m20", xref("Render_SetPolygonUVs", 395, 3))
stable.data("camera_matrix_m21", xref("Render_SetPolygonUVs", 405, 3))
stable.data("camera_matrix_m22", xref("Render_SetPolygonUVs", 415, 3))
stable.data("node_view_translation_x", xref("Render_PolygonMesh", 98, 2))
stable.data("node_view_translation_y", xref("Render_PolygonMesh", 174, 2))
stable.data("node_view_translation_z", xref("Render_PolygonMesh", 260, 2))
stable.data("polygon_batch_records", xref("Render_PolygonMesh", 1522, 3))
stable.data(
    "video_player_error_code",
    xref("Video_InitPlayer", 16, 1),
    type="int32_t",
    doc="Last winplay video/sound/movie initialization or playback status code.",
)
stable.data(
    "dinput_force_feedback_available",
    xref("DInput_EnumerateForceFeedbackJoysticks", 57, 2),
    type="int32_t",
    doc="Set to 1 when force-feedback joystick enumeration finds at least one attached device; allows constant-force effect creation and playback.",
)
stable.data(
    "window_handle",
    xref("DInput_InitializeJoystickInput", 14, 1),
    type="HWND",
    doc="Main game window handle used by DirectInput cooperative-level setup.",
)
stable.data(
    "dinput_enum_device_seen",
    xref("DInput_EnumJoystickDeviceCallback", 63, 1),
    type="int32_t",
    doc="Set by the DirectInput joystick enumeration callback after copying an enumerated device GUID into the caller-provided list.",
)
stable.data("graphics_initialized", xref("D3D_InitializeGraphicsSubsystem", 25, 1))
stable.data("text_rendering_mode", xref("D3D_SetBlendMode", 14, 1), type="int32_t")
stable.data("quad_color_blue", xref("Render_QuadClipped", 620, 3))
stable.data("temp_vertex_color_blue_vertex_1", xref("Render_DrawQuad", 1979, 2))
stable.data("temp_vertex_color_blue_vertex_2", xref("Render_DrawQuad", 2173, 2))
stable.data("temp_vertex_color_blue_vertex_3", xref("Render_DrawQuad", 152, 2))
stable.data("quad_color_green", xref("Render_QuadClipped", 583, 3))
stable.data("temp_vertex_color_green_vertex_1", xref("Render_DrawQuad", 1956, 2))
stable.data("temp_vertex_color_green_vertex_2", xref("Render_DrawQuad", 2150, 2))
stable.data("temp_vertex_color_green_vertex_3", xref("Render_DrawQuad", 141, 2))
stable.data("quad_color_red", xref("Render_QuadClipped", 553, 3))
stable.data("temp_vertex_color_red_vertex_1", xref("Render_DrawQuad", 1937, 1))
stable.data("temp_vertex_color_red_vertex_2", xref("Render_DrawQuad", 2129, 2))
stable.data("temp_vertex_color_red_vertex_3", xref("Render_DrawQuad", 404, 2))
stable.data("can_flip_surfaces", xref("D3D_InitializeDirectDraw", 594, 2))
stable.data(
    "direct3d7_interface",
    xref("D3D_InitDirectDrawAndDirect3D", 393, 1),
    type="D3D_IDirect3D7*",
    doc="IDirect3D7 interface obtained from IDirectDraw7::QueryInterface; used for device enumeration and device creation.",
)
stable.data("blue_mask", xref("D3D_CreateTextureSurface", 451, 2))
stable.data(
    "texture_pow2_width",
    xref("D3D_CreateTextureSurface", 308, 2),
    type="uint32_t",
    doc="Power-of-two texture width computed by D3D_CreateTextureSurface: rounds requested width up, clamps to 256, and mirrors height when device caps require square textures.",
)
stable.data(
    "ddraw_back_buffer",
    xref("D3D_InitDirectDrawAndDirect3D", 333, 1),
    type="DDraw_IDirectDrawSurface7*",
    doc="Attached DirectDraw back buffer used as the active D3D render target and screenshot source.",
)
stable.data(
    "ddraw_primary_surface",
    xref("D3D_InitDirectDrawAndDirect3D", 296, 1),
    type="DDraw_IDirectDrawSurface7*",
    doc="Primary/front DirectDraw surface created during D3D initialization and flipped/presented by frame rendering.",
)
stable.data("d3d_state", xref("Camera_SetupClipPlanes", 91, 2))
stable.data("clip_plane_left_0", xref("Render_ClipPolygonByCameraPyramid", 109, 1))
stable.data("clip_plane_left_1", xref("Camera_SetupClipPlanes", 198, 1))
stable.data("clip_plane_left_2", xref("Camera_SetupClipPlanes", 207, 2))
stable.data("clip_plane_right_0", xref("Camera_SetupClipPlanes", 241, 2))
stable.data("clip_plane_right_1", xref("Camera_SetupClipPlanes", 344, 2))
stable.data("clip_plane_right_2", xref("Camera_SetupClipPlanes", 358, 1))
stable.data("clip_plane_top_0", xref("Camera_SetupClipPlanes", 367, 2))
stable.data("clip_plane_top_1", xref("Camera_SetupClipPlanes", 401, 2))
stable.data("clip_plane_top_2", xref("Camera_SetupClipPlanes", 431, 2))
stable.data("clip_plane_bottom_0", xref("Camera_SetupClipPlanes", 437, 1))
stable.data("clip_plane_bottom_1", xref("Camera_SetupClipPlanes", 442, 2))
stable.data("clip_plane_bottom_2", xref("Camera_SetupClipPlanes", 448, 2))
stable.data(
    "camera_near_clip_distance", xref("Render_ClipPolygonByCameraPyramid", 161, 2)
)
stable.data("clip_plane_near_0", xref("Camera_SetupClipPlanes", 278, 1))
stable.data("clip_plane_near_1", xref("Camera_SetupClipPlanes", 287, 2))
stable.data("clip_plane_near_2", xref("Camera_SetupClipPlanes", 321, 2))
stable.data("red_shift", xref("D3D_CreateTextureSurface", 457, 2))
stable.data("red_bits_to_discard", xref("D3D_CreateTextureSurface", 505, 2))
stable.data(
    "d3d_device7",
    xref("Material_ReleaseTextureArray", 59, 1),
    type="D3D_IDirect3DDevice7*",
)
stable.data("current_vertex_format", xref("Render_QuadClipped", 421, 1))
stable.data("red_mask", xref("D3D_CreateTextureSurface", 480, 1))
stable.data("green_mask", xref("D3D_CreateTextureSurface", 487, 2))
stable.data("near_clip_plane", xref("Camera_SetupClipPlanes", 11, 2))
stable.data(
    "texture_pow2_height",
    xref("D3D_CreateTextureSurface", 294, 1),
    type="uint32_t",
    doc="Power-of-two texture height computed by D3D_CreateTextureSurface: rounds requested height up, clamps to 256, and mirrors width when device caps require square textures.",
)
stable.data(
    "texture_surface_desc",
    xref("D3D_CreateTextureSurface", 430, 1),
    type="uint32_t",
    doc="First word of the cached 0x7c-byte DDraw_SurfaceDesc2 texture surface descriptor copied by D3D_CreateTextureSurface before IDirectDraw7::CreateSurface; SDK typed globals expose the base word.",
)
stable.data("script_variable_ref_base", xref("Script_ResolveVariableRef", 69, 3))
stable.data(
    "ddraw_z_buffer",
    xref("D3D_InitDirectDrawAndDirect3D", 712, 1),
    type="DDraw_IDirectDrawSurface7*",
    doc="DirectDraw z-buffer surface attached to the D3D render target and released during D3D/DirectDraw shutdown.",
)
stable.data("blue_bits_to_discard", xref("D3D_CreateTextureSurface", 517, 2))
stable.data("debug_log_file", xref("Debug_Log", 0, 1))
stable.data("blue_shift", xref("D3D_CreateTextureSurface", 499, 2))
stable.data("reciprocal_z", xref("Render_QuadClipped", 1553, 2))
stable.data("level_actor_system_state", xref("Level_InitializeActorSystem", 495, 2))
stable.data("alpha_bits_to_discard", xref("D3D_CreateTextureSurface", 523, 2))
stable.data("alpha_shift", xref("D3D_CreateTextureSurface", 463, 2))
stable.data("alpha_mask", xref("D3D_CreateTextureSurface", 435, 2))
stable.data("green_shift", xref("D3D_CreateTextureSurface", 493, 2))
stable.data("green_bits_to_discard", xref("D3D_CreateTextureSurface", 511, 2))
stable.data("object_node_root", xref("Level_InitializeActorSystem", 423, 1))
stable.data("dalmatian_spawn_states", xref("Checkers_UpdateStateMachine", 9, 1))
stable.data("submenu_count", xref("Level_SetMenuProgressState", 74, 2))
stable.data("menu_items", xref("SaveGame_SaveLevelCompletion", 123, 3))
stable.data("menu_slots", xref("Level_InitializeSaveState", 246, 2))
stable.data("confirm_text_enabled", xref("Actor_UpdateAnimationState", 760, 1))
stable.data("menu_initial_entry_flag", xref("Level_ResetBonusState", 12, 3))
stable.data("checkers_player_is_human", xref("Checkers_UpdateStateMachine", 977, 3))
stable.data("bonus_level_progress_value", xref("Level_InitializeSaveState", 485, 1))
stable.data("save_state_init_flag", xref("Level_InitializeSaveState", 496, 2))
stable.data(
    "checkers_enforce_capture_rule", xref("Checkers_UpdateStateMachine", 621, 2)
)
stable.data("checkers_ai_difficulty", xref("Checkers_UpdateStateMachine", 1432, 1))
stable.data(
    "checkers_current_player",
    xref("Checkers_UpdateStateMachine", 261, 2),
    doc="Current checkers side: live play uses player values 1 and 2, toggled with xor 3, and is set to 0 for the no-move/end state.",
)
stable.data("camera_transition_counter", xref("Script_PauseToggle", 409, 1))
stable.data("pause_camera_rotation_angle", xref("Script_PauseToggle", 414, 1))
stable.data("pause_target_rotation_angle", xref("Script_PauseToggle", 419, 2))
stable.data("pause_target_y_offset", xref("Script_PauseToggle", 425, 2))
stable.data("pause_target_distance", xref("Script_PauseToggle", 431, 2))
stable.data("pause_camera_fov", xref("Script_PauseToggle", 451, 2))
stable.data("world_0_completion_bits", xref("Checkers_UpdateStateMachine", 1304, 2))
stable.data("world_1_completion_bits", xref("SaveGame_SaveBonusProgress", 5, 2))
stable.data("world_2_completion_bits", xref("SaveGame_SaveBonusProgress", 11, 2))
stable.data("world_3_completion_bits", xref("SaveGame_SaveBonusProgress", 22, 1))
stable.data("bonus_level_unlocked", xref("Level_InitializeSaveState", 169, 2))
stable.data("bone_trail_path_count", xref("BoneTrail_Reset", 12, 2))
stable.data("world_4_completion_bits", xref("SaveGame_SaveBonusProgress", 33, 2))
stable.data("bonus_level_parameter_1", xref("Level_InitializeBonusData", 123, 2))
stable.data("bonus_level_parameter_2", xref("Level_InitializeBonusData", 129, 2))
stable.data("bonus_level_parameter_3", xref("Level_InitializeBonusData", 135, 1))
stable.data(
    "current_level_data",
    xref("Audio_TriggerMusicTransition", 65, 1),
    type="Level_Data*",
    doc=(
        "Active stable Level_Data pointer at pcdogs.exe+0x168ADA0 (PC EN); actor enumeration uses "
        "Level_RuntimeData.entity_array/entity_count plus Entity_State.active_actor through "
        "dttr_pcdogs_unstable.h conversion helpers."
    ),
)
stable.data("level_scale_factor", xref("Render_AdjustLevelScale", 63, 2))
stable.data("level_render_distance", xref("Level_InitializeActorSystem", 389, 2))
stable.data("level_transition_state", xref("Level_InitializeActorSystem", 228, 2))
stable.data("current_game_mode", xref("Script_PauseToggle", 0, 1))
stable.data(
    "level_render_distance_quarter", xref("Level_InitializeActorSystem", 431, 2)
)
stable.data("level_render_distance_third", xref("Level_InitializeActorSystem", 449, 2))
stable.data("actor_behavior_ai_state_0", xref("Actor_ProcessPlayerBehavior", 2147, 1))
stable.data("rendering_depth_mode", xref("Actor_ProcessPlayerBehavior", 2158, 2))
stable.data(
    "dynamic_level_scale",
    xref("Camera_UpdateFollow", 2571, 1),
    type="int32_t",
    doc=(
        "Integer runtime level/render scale at pcdogs.exe+0x168A7A0 (PC EN). Camera_UpdateFollow "
        "writes this value shifted left by 12 into Render_ListState.dynamic_level_scale at offset "
        "+0xB8 (PC EN)."
    ),
)
stable.data(
    "music_transition_active",
    xref("Audio_TriggerMusicTransition", 41, 2),
    type="uint8_t",
    doc="Byte-sized music transition active/selected-state flag read by Audio_TriggerMusicTransition.",
)
stable.data("render_world_transform_ptr", xref("Render_MeshNode", 11, 2))
stable.data(
    "player_actor",
    xref("Actor_ProcessRendering", 8, 2),
    type="Actor_State*",
    doc=(
        "Render-scoped actor pointer published and cleared by Actor_ProcessRendering. "
        "Movement/collision hooks use DTTR_Util_GetActiveActor or "
        "current_level_data->Level_RuntimeData.entity_array->Entity_State.active_actor for "
        "current-player authority."
    ),
)
stable.data("script_pause_toggle_state", xref("Script_PauseToggle", 27, 1))
stable.data(
    "pause_saved_camera_x",
    xref("Script_PauseToggle", 41, 2),
    doc="Saved active actor world_render_pos_x used for pause/menu camera distance checks.",
)
stable.data(
    "pause_saved_camera_y",
    xref("Script_PauseToggle", 50, 2),
    doc="Saved active actor world_render_pos_y used for pause/menu camera distance checks.",
)
stable.data(
    "pause_saved_camera_z",
    xref("Script_PauseToggle", 59, 1),
    doc="Saved active actor world_render_pos_z used for pause/menu camera distance checks.",
)
stable.data(
    "projectile_actor_list_head",
    xref("Trail_SpawnFromEntry", 92, 1),
    doc=(
        "Live projectile actor linked-list head walked by Actor_UpdateProjectileList. "
        "Use generated read access for inspection."
    ),
)
stable.data("file_descriptor_table", xref("File_SeekAndGetPosition", 92, 3))
stable.data("io_buffer_high_water_mark", xref("File_FlushToDisk", 4, 2))
stable.data("heap_max_segments", xref("Heap_InitializeAllocator", 50, 2))
stable.data("heap_last_freed_page_index", xref("Heap_FreeBlock", 569, 2))
stable.data("heap_segment_table_cached", xref("Heap_InitializeAllocator", 45, 1))
stable.data("heap_last_freed_segment", xref("Heap_InitializeAllocator", 29, 2))
stable.data("heap_segment_count", xref("Heap_InitializeAllocator", 36, 2))
stable.data("heap_allocator_state", xref("Heap_InitializeAllocator", 21, 1))
stable.data("get_async_key_state", xref("Video_PlayMovieLoop", 41, 2))
stable.data("ail_release_sample_handle", xref("Audio_ShutdownSystem", 10, 2))
stable.data("ail_shutdown", xref("Audio_ShutdownSystem", 55, 2))

BLUEPRINT = stable
