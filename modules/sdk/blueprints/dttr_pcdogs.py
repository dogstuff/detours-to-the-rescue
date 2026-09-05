#!/usr/bin/env python3
from __future__ import annotations

from blueprint import (
    UNKNOWN_PARAMS,
    AbiStatus,
    Blueprint,
    CallingConvention,
    HookKind,
    Required,
    WritePolicy,
    enum_value,
    hook,
    member,
    param,
    xref,
)

stable = Blueprint("stable")


stable.type_alias("Audio_AILHSample", "void*")

stable.type_alias("Audio_AILHStream", "void*")

stable.type_alias(
    "Audio_AILHDigitalDriver",
    "void*",
    doc="Miles digital-driver handle stored by audio initialization and used by playback guards.",
    stable=True,
)

stable.struct(
    "CRT_LongDouble12",
    member("uint8_t", "ld12[12]", 0x0),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Private CRT 12-byte container for an x87 extended value and two bytes of alignment padding.",
)

stable.struct(
    "CRT_FloatConversionRecord",
    member("int32_t", "sign_character", 0x0),
    member("int32_t", "decimal_point_position", 0x4),
    member(
        "BOOL",
        "is_finite",
        0x8,
        doc="FALSE only for INF, QNAN, SNAN, and IND tokens. TRUE for all finite values including zero.",
    ),
    member("char*", "mantissa_digits", 0xC),
    size=0x10,
    incomplete=False,
    unstable=True,
    doc="Private CRT conversion record carrying sign, decimal-point position, finiteness, and mantissa digits.",
)

stable.struct(
    "DInput_DeviceEnumContext",
    member("uint32_t", "count", 0x0, doc="Count of discovered joystick device GUIDs."),
    member(
        "Win32_GUID*",
        "guid_list",
        0x4,
        doc="Caller-owned destination array for discovered device GUIDs.",
    ),
    size=0x8,
    doc="DirectInput joystick enumeration context for discovered device GUIDs.",
    stable=True,
    incomplete=False,
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
    size=0x20,
)

stable.struct(
    "Animation_ChannelHeader",
    member("uint8_t", "channel_params[7]", 0x0),
    member("uint8_t", "loop_flags", 0x7),
    member("Math_RangeI16", "keyframe_range", 0x8),
    size=0xC,
)

stable.struct(
    "Animation_ColorKeyframe",
    member("uint16_t", "frame_number", 0x0),
    member("uint16_t", "interpolation_flag", 0x2),
    member("uint32_t", "color", 0x4),
    member("uint32_t", "interpolation_mode", 0x8),
    size=0xC,
)

stable.struct(
    "Animation_VertexColorTarget",
    member("int16_t", "vertex_index", 0x0),
    member("int16_t", "frame_offset", 0x2),
    member("Math_ColorRGB8", "base_color", 0x4),
    member(
        "uint8_t",
        "reserved_07",
        0x7,
        doc="Reserved byte after base RGB used by Animation_ProcessVertexColor target reads.",
    ),
    size=0x8,
    unstable=True,
)

stable.struct(
    "Animation_VertexColorKeyframe",
    member("Math_ColorRGBI16", "delta", 0x0),
    member("int16_t", "frame_number", 0x6),
    member("int32_t", "inv_frame_delta_q12", 0x8),
    size=0xC,
)

stable.struct(
    "Animation_VertexColorController",
    member(
        "Mesh_CommandType",
        "type",
        0x0,
        doc="MESH_COMMAND_VERTEX_COLOR identifies vertex-color commands.",
    ),
    member(
        "Mesh_CommandFlags",
        "flags",
        0x1,
        doc="State flags and MESH_CMD_MODE_MASK timing mode.",
    ),
    member(
        "int16_t",
        "signal_id",
        0x2,
        doc="Signal id polled by Graphics_UpdateMeshCommandState, then used for controller updates.",
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
        doc="Alignment byte between keyframe_count and vertex_count; no documented semantic use in vertex-color paths.",
    ),
    member("int16_t", "vertex_count", 0xE),
    member(
        "Animation_VertexColorTarget*",
        "target_array",
        0x10,
        doc="Vertex-color target array with vertex index, frame position, and base RGB bytes.",
    ),
    member(
        "Animation_VertexColorKeyframe*",
        "keyframe_array",
        0x14,
        doc="Signed RGB delta keyframe array, sampled by Animation_ProcessVertexColor.",
    ),
    size=0x18,
    incomplete=False,
    unstable=True,
)

stable.type_alias(
    "Animation_PackedControllerState",
    "uint32_t",
    doc="Fixed-width packed controller state. Symbolic masks are declared by Animation_PackedControllerStateValue.",
    stable=True,
)

stable.enum(
    "Animation_PackedControllerStateValue",
    enum_value("ANIM_CTRL_STATE_SLOT_MASK", 0x000001FF),
    enum_value("ANIM_CTRL_STATE_PHASE_MASK", 0x0001FE00),
    enum_value("ANIM_CTRL_STATE_RANDOM_FRAME_MASK", 0x01FE0000),
    enum_value("ANIM_CTRL_STATE_REVERSE", 0x02000000),
    enum_value("ANIM_CTRL_STATE_PINGPONG", 0x04000000),
    enum_value("ANIM_CTRL_STATE_RANDOM", 0x08000000),
    enum_value("ANIM_CTRL_STATE_ONCE", 0x10000000),
    enum_value("ANIM_CTRL_STATE_MODE_MASK", 0x1E000000),
    enum_value("ANIM_CTRL_STATE_DISABLED", 0xFFFFFFFF),
    doc="Masks, playback flags, and disabled sentinel stored in Animation_PackedControllerState.",
    unstable=True,
)

stable.struct(
    "Animation_ControllerCommand",
    member("Mesh_CommandType", "type", 0x0),
    member("Mesh_CommandFlags", "flags", 0x1),
    member("int16_t", "signal_id", 0x2),
    member("int32_t", "progress_q12", 0x4),
    member("int32_t", "limit_q12", 0x8),
    member(
        "Animation_PackedControllerState",
        "packed_controller_state",
        0xC,
        doc="Persistent slot, phase, random-frame, and playback-mode state updated in place.",
    ),
    size=0x10,
    doc="Type-0 mesh command passed to Animation_ProcessController.",
    incomplete=False,
    unstable=True,
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
    size=0x20,
)

stable.struct(
    "Animation_ControllerGroup",
    member("uint8_t", "group_flags", 0x0),
    member(
        "uint8_t",
        "active_controller_count",
        0x1,
        doc="Temporary controller_count override used for one Graphics_ProcessMeshCommands call; the original 16-bit count is restored afterward.",
    ),
    member("int16_t", "controller_count", 0x2),
    member("Animation_ControllerSlot**", "controller_slot_array", 0x4),
    size=0x8,
    unstable=True,
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
            "Reserved controller word used near the fields read by Animation_ProcessController."
        ),
    ),
    member(
        "void*",
        "controller_payload_ptr",
        0x8,
        doc="Variant controller payload pointer containing type-specific byte/dword data interpreted by Animation_ProcessController.",
    ),
    member("Animation_FrameHeader*", "frame_sequence_ptr", 0xC),
    size=0x10,
    unstable=True,
)

stable.struct(
    "Animation_Data",
    member("uint8_t", "frame_count", 0x0),
    member("uint8_t", "bone_count", 0x1),
    member("uint8_t", "flags", 0x2),
    member("uint8_t", "reserved", 0x3),
    member("uint32_t", "keyframe_data[6]", 0x4),
    size=0x1C,
)

stable.type_alias(
    "Animation_DataBlockFixupFlags",
    "uint8_t",
    doc="Byte-width animation block fixup flags. Values are declared by Animation_DataBlockFixupFlagValue.",
    unstable=True,
)

stable.enum(
    "Animation_DataBlockFixupFlagValue",
    enum_value("ANIMATION_DATA_RELOCATION_VISITED", 0x02),
    doc="Animation block relocation marker. Remaining flag bits are opaque.",
    unstable=True,
)

stable.type_alias(
    "Animation_SplineChannelFlags",
    "uint16_t",
    doc="Word-width spline channel flags. Values are declared by Animation_SplineChannelFlagValue.",
    unstable=True,
)

stable.enum(
    "Animation_SplineChannelFlagValue",
    enum_value("ANIMATION_SPLINE_NESTED_CHANNEL_ARRAY", 0x0040),
    enum_value("ANIMATION_SPLINE_RELOCATION_VISITED", 0x8000),
    doc="Spline relocation flags. Remaining flag bits are opaque.",
    unstable=True,
)

stable.type_alias(
    "Animation_KeyframeTimingPackedView",
    "uint32_t",
    doc="Packed key timing uses bits 0 through 7 for the ease record, bits 8 through 19 for curve shape, and bits 20 through 31 for the Q6 time index.",
    unstable=True,
)

stable.type_alias(
    "Animation_VisibilityKeyframePackedView",
    "uint32_t",
    doc="Packed visibility timing leaves bits 0 through 19 opaque, uses bits 20 through 30 for the Q6 time index, and stores visibility in bit 31.",
    unstable=True,
)

stable.struct(
    "Animation_SplineChannelRaw",
    member("uint32_t", "blob_relative_channel_data_offset_or_minus_one_sentinel", 0x0),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Serialized channel descriptor. Direct data offset is blob-relative, while 0xFFFFFFFF preserves the constant-channel sentinel.",
)

stable.struct(
    "Animation_DataBlockRaw",
    member("int8_t", "position_channel_count", 0x0),
    member("int8_t", "rotation_channel_count", 0x1),
    member("int8_t", "scale_channel_count", 0x2),
    member("int8_t", "visibility_channel_count", 0x3),
    member("int8_t", "color_channel_count", 0x4),
    member("int8_t", "nested_channel_count", 0x5),
    member("uint8_t", "reserved_06", 0x6),
    member("Animation_DataBlockFixupFlags", "fixup_flags", 0x7),
    member("Math_RangeI16", "frame_range", 0x8),
    member("uint32_t", "blob_relative_position_channels_offset", 0xC),
    member("uint32_t", "blob_relative_rotation_channels_offset", 0x10),
    member("uint32_t", "blob_relative_scale_channels_offset", 0x14),
    member("uint32_t", "blob_relative_visibility_channels_offset", 0x18),
    member("uint32_t", "blob_relative_color_channels_offset", 0x1C),
    member("uint32_t", "blob_relative_nested_channels_offset", 0x20),
    size=0x24,
    incomplete=False,
    unstable=True,
    doc="Serialized animation block overlay whose six channel-array offsets are rebased from the package blob base.",
)

stable.struct(
    "Animation_DataBlock",
    member("int8_t", "num_position_channels", 0x0),
    member("int8_t", "num_rotation_channels", 0x1),
    member("int8_t", "num_visibility_channels", 0x2),
    member("int8_t", "num_scale_channels", 0x3),
    member("int8_t", "num_morph_channels", 0x4),
    member("int8_t", "num_scalar_channels", 0x5),
    member(
        "uint8_t",
        "reserved_06",
        0x6,
        doc=(
            "Reserved byte before fixup_flags; PKG_FixUpResourceAnimationData reads the channel pointers and fixup flags."
        ),
    ),
    member(
        "Animation_DataBlockFixupFlags",
        "fixup_flags",
        0x7,
        doc="Bit 0x02 marks the block's channel pointer tables as relocated.",
    ),
    member("Math_RangeI16", "keyframe_range", 0x8),
    member("Animation_SplineChannelVec3*", "position_channels", 0xC),
    member("Animation_SplineChannelQuat*", "rotation_channels", 0x10),
    member("Animation_SplineChannelVis*", "visibility_channels", 0x14),
    member("Animation_SplineChannelVec3*", "scale_channels", 0x18),
    member("Animation_SplineChannelMorph*", "morph_channels", 0x1C),
    member("Animation_SplineChannelScalar*", "scalar_channels", 0x20),
    size=0x24,
    unstable=True,
)


stable.struct(
    "Animation_FrameData",
    member("uint8_t*", "data", 0x0),
    member("int16_t", "vertex_count", 0x4),
    member("uint8_t", "reserved_06[2]", 0x6),
    member("uint8_t", "morph_keyframe_count", 0x8),
    member("uint8_t", "reserved_09[3]", 0x9),
    member("uint8_t", "frame_extra_data[10]", 0xC),
    member("uint8_t", "reserved_16[2]", 0x16),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Animation_FrameHeader",
    member("uint8_t", "fixup_flags", 0x0),
    member("uint8_t", "padding_01", 0x1),
    member("int16_t", "frame_count", 0x2),
    member("Animation_FrameData**", "frame_ptrs", 0x4),
    size=0x8,
    unstable=True,
)

stable.struct(
    "Animation_FrameVertex",
    member("Math_Vec3I16", "pos", 0x0),
    member("int16_t", "padding", 0x6),
    size=0x8,
)

stable.struct(
    "Animation_MorphKeyframe",
    member("uint32_t", "timing_flags", 0x0),
    member("int16_t", "morph_target_index", 0x4),
    member("int16_t", "blend_weight", 0x6),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_id", 0x9),
    member("int16_t", "flags", 0xA),
    size=0xC,
)

stable.struct(
    "Animation_MorphTargetVertex",
    member("Math_Vec3I16", "delta", 0x0),
    member("int16_t", "flags", 0x6),
    size=0x8,
)

stable.struct(
    "Animation_MorphVertex",
    member("Math_Vec3I16", "delta", 0x0),
    member("int16_t", "flags", 0x6),
    member("Math_Vec3I16", "normal", 0x8),
    member("int16_t", "vertex_index", 0xE),
    size=0x10,
)

stable.struct(
    "Animation_PositionKeyframe",
    member("Animation_KeyframeTimingPackedView", "timing", 0x0),
    member("Math_Vec3I32", "position", 0x4),
    member("Math_Vec3I32", "tangent_out", 0x10),
    member("Math_Vec3I32", "tangent_in", 0x1C),
    size=0x28,
)

stable.struct(
    "Animation_Vec3Keyframe",
    member("Animation_KeyframeTimingPackedView", "timing", 0x0),
    member("Math_Vec3I32", "value", 0x4),
    member("Math_Vec3I32", "tangent_out", 0x10),
    member("Math_Vec3I32", "tangent_in", 0x1C),
    size=0x28,
    incomplete=False,
    unstable=True,
    doc="Generic position-or-scale spline keyframe. Animation_InterpolateVec3 advances records at a 0x28-byte stride.",
)

stable.struct(
    "Animation_RotationKeyframe",
    member("Animation_KeyframeTimingPackedView", "timing", 0x0),
    member("Math_QuaternionI16", "rotation", 0x4),
    member("uint32_t", "segment_flags", 0xC),
    member("Math_QuaternionI16", "tangent_out", 0x10),
    member("Math_QuaternionI16", "tangent_in", 0x18),
    size=0x20,
)

stable.struct(
    "Animation_ScalarKeyframe",
    member("Animation_KeyframeTimingPackedView", "timing", 0x0),
    member("int32_t", "value", 0x4),
    member("int32_t", "tangent_out", 0x8),
    member("int32_t", "tangent_in", 0xC),
    size=0x10,
)

stable.struct(
    "Animation_SplineChannel",
    member(
        "uint8_t*",
        "channel_data_or_minus_one_sentinel",
        0x0,
        doc=(
            "Byte-addressed polymorphic channel data, null when absent, or exactly "
            "0xFFFFFFFF for the preserved sentinel case."
        ),
    ),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member(
        "uint8_t",
        "keyframe_or_nested_channel_count",
        0x8,
        doc="Unsigned channel record count. Zero is valid during package fixup.",
    ),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Dynamic spline descriptor with the time bias in the low 16 bits at offset 4 and the end time in bits 20 through 31.",
)

stable.struct(
    "Animation_SplineChannelMorph",
    member(
        "Animation_SplineChannelScalar*",
        "nested_scalar_channels_or_minus_one_sentinel",
        0x0,
    ),
    member("int32_t", "packed_timing_or_direct_target_index", 0x4),
    member("uint8_t", "nested_scalar_channel_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Morph view whose payload is null, 0xFFFFFFFF, or nested 0x0C scalar channels. Direct/static tails overlay inline values.",
)

stable.struct(
    "Animation_SplineChannelQuat",
    member(
        "Animation_RotationKeyframe*",
        "keyframes_or_minus_one_sentinel",
        0x0,
    ),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Rotation view over 0x20-byte keyframes, null, or 0xFFFFFFFF. Direct/static tails overlay inline values.",
)

stable.struct(
    "Animation_SplineChannelScalar",
    member(
        "Animation_ScalarKeyframe*",
        "keyframes_or_minus_one_sentinel",
        0x0,
    ),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Scalar view over 0x10-byte keyframes, null, or 0xFFFFFFFF. Direct/static tails overlay inline values.",
)

stable.struct(
    "Animation_SplineChannelVec3",
    member(
        "Animation_Vec3Keyframe*",
        "keyframes_or_minus_one_sentinel",
        0x0,
    ),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Vector view over 0x28-byte keyframes, null, or 0xFFFFFFFF. Direct/static tails overlay inline values.",
)

stable.struct(
    "Animation_SplineChannelVec3ConstantView",
    member("uint8_t*", "channel_data_minus_one_sentinel", 0x0),
    member("Math_Vec3I16", "value_q12", 0x4),
    member("uint16_t", "reserved_0a", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Constant Vec3 sentinel overlay. The 0xFFFFFFFF pointer selects three inline signed Q12 values.",
)

stable.struct(
    "Animation_SplineChannelQuatConstantView",
    member("uint8_t*", "channel_data_minus_one_sentinel", 0x0),
    member("Math_QuaternionI16", "value_q14", 0x4),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Constant quaternion sentinel overlay. The 0xFFFFFFFF pointer selects four inline signed Q14 components.",
)

stable.struct(
    "Animation_SplineChannelScalarConstantView",
    member("uint8_t*", "channel_data_minus_one_sentinel", 0x0),
    member("int32_t", "packed_value", 0x4),
    member("uint32_t", "reserved_08", 0x8),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Constant scalar sentinel overlay. The interpolator consumes packed_value shifted right by 12.",
)

stable.struct(
    "Animation_SplineChannelVisibilityConstantView",
    member("uint8_t*", "channel_data_minus_one_sentinel", 0x0),
    member("int32_t", "activation_time", 0x4),
    member("uint32_t", "reserved_08", 0x8),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Constant visibility sentinel overlay. Activation compares the signed sample time against activation_time.",
)

stable.struct(
    "Animation_SplineChannelVis",
    member(
        "Animation_VisibilityKeyframe*",
        "keyframes_or_minus_one_sentinel",
        0x0,
    ),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Visibility view over 4-byte keyframes, null, or 0xFFFFFFFF. Direct/static tails overlay inline values.",
)

stable.struct(
    "Animation_NestedSplineChannelRaw",
    member("uint32_t", "blob_relative_channel_data_offset", 0x0),
    member("int32_t", "packed_time_bias_and_end_time", 0x4),
    member("uint8_t", "keyframe_count", 0x8),
    member("uint8_t", "target_index", 0x9),
    member("Animation_SplineChannelFlags", "channel_flags", 0xA),
    size=0xC,
    incomplete=False,
    unstable=True,
    doc="Raw nested 0x0C spline descriptor whose byte-addressed channel-data offset is rebased when the outer channel has flag 0x40.",
)

stable.struct(
    "Animation_StateTable",
    member("Animation_DataBlock*", "anim_ptrs[43]", 0x0),
    size=0xAC,
)


stable.struct(
    "Animation_VisibilityKeyframe",
    member("Animation_VisibilityKeyframePackedView", "timing_and_visibility", 0x0),
    size=0x4,
)

stable.struct(
    "Audio_SampleHandle",
    member("int32_t", "sound_id", 0x0),
    member("int32_t", "volume", 0x4),
    member("int32_t", "frequency", 0x8),
    member("int32_t*", "sample_handle", 0xC),
    member("int32_t", "status_flags", 0x10),
    size=0x14,
)

stable.struct(
    "Audio_SampleHandleEntry",
    member("Audio_SampleHandleEntry*", "prev_ptr", 0x0),
    member("Audio_SampleHandleEntry*", "next_ptr", 0x4),
    member("int32_t*", "ail_handle", 0x8),
    member("Audio_SoundEntry*", "sound_entry", 0xC),
    member("uint32_t", "additional_data", 0x10),
    size=0x14,
)


stable.struct(
    "Audio_SoundDescriptor",
    member("int32_t", "sound_id", 0x0),
    member("int32_t", "flags", 0x4),
    size=0x8,
)

stable.struct(
    "Audio_SoundEntry",
    member("Audio_SoundEntry*", "prev", 0x0),
    member("Audio_SoundEntry*", "next", 0x4),
    member("Audio_SoundDefinition*", "sound_def_ptr", 0x8),
    member("Math_Vec3I32*", "position_ptr", 0xC),
    member("Math_Vec3I32", "listener_pos", 0x10),
    member("Math_Vec3I32", "sound_pos", 0x1C),
    member("int16_t", "volume", 0x28),
    member("int16_t", "pitch", 0x2A),
    member("int16_t", "pan", 0x2C),
    member("uint8_t", "flags", 0x2E),
    member("uint8_t", "padding_2f", 0x2F),
    size=0x30,
)

stable.struct(
    "Audio_CollisionSoundCooldownEntry",
    member("Actor_State*", "other_actor", 0x0),
    member("Math_Vec3I32", "velocity", 0x4),
    member("Math_Vec3I32", "impact_velocity", 0x10),
    member("int32_t", "sound_key", 0x1C),
    member("uint32_t", "start_frame", 0x20),
    member("uint32_t", "expires_frame", 0x24),
    size=0x28,
)

stable.struct(
    "Audio_SoundDefinition",
    member("void**", "sample_table_ptr", 0x0),
    member(
        "uint8_t",
        "flags",
        0x4,
        doc=(
            "Audio definition flags. Script_OpPlaySoundBlockOrWait mutates bit 0x40 as "
            "a script_playback_active_or_wait_latch around direct playback."
        ),
    ),
    member("uint8_t", "pan", 0x5),
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
        doc=("Reserved audio definition byte before the volume and pitch fields."),
    ),
    member("int32_t", "volume_fp12", 0x8),
    member("int32_t", "pitch_fp12", 0xC),
    member("int32_t", "spatial_range_fp12", 0x10),
    size=0x14,
    doc="Sound definition used by Audio_PlaySoundDefinition3D and Audio_AllocateSoundSlot. It stores the sample table, flags, eviction priority, Q12 volume/pitch/range, and the bit 0x40 script-playback latch.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Audio_SoundSlot",
    member("int32_t", "cached_volume", 0x0),
    member("int32_t", "cached_pan", 0x4),
    member("int32_t", "cached_playback_rate", 0x8),
    member("int32_t", "base_playback_rate", 0xC),
    member("Audio_AILHSample", "sample_handle", 0x10),
    size=0x14,
)

stable.struct(
    "Audio_WaveFormat",
    member("int16_t", "format_tag", 0x0),
    member("int16_t", "channels", 0x2),
    member("int16_t", "bits_per_sample", 0x4),
    member("int16_t", "block_align", 0x6),
    size=0x8,
)

stable.struct(
    "Bone_JointTrackState",
    member("int16_t", "angle_param[4]", 0x0),
    member("int16_t", "current_yaw", 0x8),
    member("int16_t", "current_pitch", 0xA),
    member("int16_t", "max_angle_step", 0xC),
    member("int16_t", "pad_0e", 0xE),
    member("Math_Matrix3x3I16", "base_rotation", 0x10),
    member("int16_t", "pad_22", 0x22),
    member("Scene_Node*", "node", 0x24),
    size=0x28,
    doc="Joint head/eye tracking state advanced by Bone_UpdateJointTracking: angle limits, current yaw/pitch, base rotation, and the tracked scene node.",
)

stable.struct(
    "Bone_WeightedVertex",
    member("Math_Vec3I16", "local_pos", 0x0),
    member("int16_t", "reserved_06", 0x6),
    member("int16_t", "vertex_index", 0x8),
    member("int16_t", "weight_q12", 0xA),
    size=0xC,
)

stable.struct(
    "Camera_Frustum",
    member("Math_Vec3I32", "clip_plane_a_normal", 0x0),
    member("int32_t", "clip_plane_a_dist", 0xC),
    member("Math_Vec3I32", "clip_plane_b_normal", 0x10),
    member("int32_t", "clip_plane_b_dist", 0x1C),
    member("Math_Vec3I32", "clip_plane_c_normal", 0x20),
    member("int32_t", "clip_plane_c_dist", 0x2C),
    member("Math_Vec3I32", "clip_plane_d_normal", 0x30),
    member("int32_t", "clip_plane_d_dist", 0x3C),
    member("Math_Vec3I32", "clip_plane_e_normal", 0x40),
    member("int32_t", "clip_plane_e_dist", 0x4C),
    member("int32_t", "state_flags[2]", 0x50),
    member("D3D_IDirect3DDevice7*", "d3d_device", 0x58),
    member("int32_t", "viewport_x", 0x5C),
    member("int32_t", "viewport_y", 0x60),
    member("int32_t", "viewport_w", 0x64),
    size=0x68,
)

stable.struct(
    "Camera_FrustumDirEntry",
    member("Math_Vec3I16", "dir", 0x0),
    member("int16_t", "pad", 0x6),
    size=0x8,
    doc="One int16 frustum direction triple plus padding (stride 8).",
)

stable.struct(
    "Camera_FrustumDirTable",
    member("Camera_FrustumDirEntry", "v[5]", 0x0),
    size=0x28,
    doc="Five frustum direction entries written by Camera_BuildViewMatrix and embedded in Camera_Runtime at +0x30.",
)

stable.struct(
    "Camera_FrustumPlane",
    member("Math_Vec3I32", "normal", 0x0),
    size=0xC,
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
    member("Math_Vec2I32", "position_xy", 0x10),
    member("Math_Vec2I32XZ", "target", 0x18),
    member("Math_Vec2I16", "viewport_pos", 0x20),
    member("int16_t", "view_matrix[20]", 0x24),
    member("uint8_t", "combined_view_matrix[48]", 0x4C),
    member("uint8_t", "viewport_clip_data[12]", 0x7C),
    member(
        "int16_t",
        "world_rot_reserved",
        0x88,
        doc="Reserved word after camera view matrix data.",
    ),
    member(
        "uint8_t",
        "reserved_8a[2]",
        0x8A,
        doc="Reserved bytes before view translation fields. It is reserved for internal use.",
    ),
    member("Math_Vec2I32", "view_translation_xy", 0x8C),
    member("int32_t", "position_z", 0x94),
    member("int32_t", "near_clip_distance", 0x98),
    member("int32_t", "max_render_distance", 0x9C),
    member("uint8_t", "projection_extra[8]", 0xA0),
    member("void*", "scene_setup_callback", 0xA8),
    member(
        "uint8_t",
        "reserved_ac[24]",
        0xAC,
        doc="Reserved trailing camera-render scratch block after scene_setup_callback.",
    ),
    size=0xC4,
    unstable=True,
)

stable.struct(
    "Camera_ViewRecord",
    member("uint32_t", "flags", 0x0),
    member("Math_Vec3I32", "pos", 0x4),
    member("uint32_t", "activation_radius", 0x10),
    member("uint32_t", "actor_activation_radius", 0x14),
    member("int16_t", "camera_def_index", 0x18),
    member("int16_t", "entity_index", 0x1A),
    member("uint8_t", "neighbor_entity_links[8]", 0x1C),
    member("uint8_t", "zone_boundaries[32]", 0x24),
    member("PKG_ActorTemplate*", "actor_template", 0x44),
    member("uint32_t", "action_data[2]", 0x48),
    member("int32_t*", "camera_path_data", 0x50),
    member("uint8_t", "path_parameters[16]", 0x54),
    member("uint8_t", "camera_collision_volume[84]", 0x64),
    member("int32_t*", "transform_pointer", 0xB8),
    member("uint8_t", "view_transform_matrix[44]", 0xBC),
    member("int32_t", "rotation_calc", 0xE8),
    member("uint8_t", "camera_anim_state[52]", 0xEC),
    member(
        "Actor_State*",
        "spawned_actor",
        0x120,
        doc=(
            "References the spawned live actor when present; actor lifetime and list "
            "ownership remain with actor/entity systems."
        ),
    ),
    member("uint8_t", "linked_actor_data[16]", 0x124),
    member("uint32_t", "timestamp", 0x134),
    member("uint8_t", "transition_timers[20]", 0x138),
    member("Math_Vec3I32", "target_pos", 0x14C),
    member("uint8_t", "look_at_target_data[32]", 0x158),
    size=0x178,
    doc=(
        "Package/runtime camera-view record keyed by an entity index. It owns camera/link data "
        "and can reference a spawned live actor; actor/entity systems own actor lifetime and "
        "list membership."
    ),
    unstable=True,
)

stable.struct(
    "Camera_State",
    member("uint8_t", "camera_type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("int16_t", "transition_speed", 0x2),
    member("int16_t", "fov_distance", 0x4),
    member("int16_t", "orbit_yaw", 0x6),
    member("int32_t", "orbit_pitch", 0x8),
    member("Math_Vec3I32", "cam_offset", 0xC),
    member("Math_Vec3I32", "target_offset", 0x18),
    size=0x24,
)

stable.struct(
    "Camera_Pose",
    member("int16_t", "angle_vertical", 0x0),
    member("int16_t", "angle_horizontal", 0x2),
    member("int16_t", "view_roll", 0x4),
    member("int16_t", "fov", 0x6),
    member("int32_t", "distance_or_clip", 0x8),
    member("Math_Vec3I32XZY", "eye_pos", 0xC),
    member("Math_Vec3I32XZY", "target_pos", 0x18),
    size=0x24,
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
    size=0x28,
    doc="Live camera-state prefix read by Camera_InterpolateTransition.",
)

stable.type_alias(
    "Checkers_Piece",
    "uint8_t",
    doc="Byte-width checkers piece code. Values are declared by Checkers_PieceValue.",
    stable=True,
)

stable.enum(
    "Checkers_Player",
    enum_value("CHECKERS_PLAYER_1", 1),
    enum_value("CHECKERS_PLAYER_2", 2),
    doc="Player identifiers stored in the low two bits of a Checkers_Piece.",
    stable=True,
)

stable.enum(
    "Checkers_PieceValue",
    enum_value("CHECKERS_PIECE_EMPTY", 0),
    enum_value("CHECKERS_PIECE_PLAYER1_MAN", 1),
    enum_value("CHECKERS_PIECE_PLAYER2_MAN", 2),
    enum_value("CHECKERS_PIECE_PLAYER1_KING", 5),
    enum_value("CHECKERS_PIECE_PLAYER2_KING", 6),
    doc="Values stored in Checkers_Piece board cells. Low two bits identify the player.",
    stable=True,
)

stable.enum(
    "Checkers_MoveValidation",
    enum_value("CHECKERS_MOVE_INVALID", 0),
    enum_value("CHECKERS_MOVE_CAPTURE", 1),
    enum_value("CHECKERS_MOVE_SIMPLE", 2),
    doc="Result of validating one proposed checkers move.",
    stable=True,
)

stable.enum(
    "Checkers_MoveExecutionResult",
    enum_value("CHECKERS_EXEC_INVALID", 0),
    enum_value("CHECKERS_EXEC_CAPTURE_MUST_CONTINUE", 1),
    enum_value("CHECKERS_EXEC_TURN_COMPLETE", 2),
    enum_value("CHECKERS_EXEC_TURN_COMPLETE_PROMOTED", 3),
    doc="A move can be invalid, require another capture, complete the turn, or complete a promoted turn.",
    stable=True,
)

stable.enum(
    "Checkers_MinGameState",
    enum_value("CHECKERS_STATE_IDLE", 0),
    enum_value("CHECKERS_STATE_SIMPLE_MOVE", 1),
    enum_value("CHECKERS_STATE_SIMPLE_MOVE_PROMOTED", 2),
    enum_value("CHECKERS_STATE_CAPTURE_SEQUENCE", 3),
    enum_value("CHECKERS_STATE_CAPTURE_SEQUENCE_PROMOTED", 4),
    enum_value("CHECKERS_STATE_ANIMATION_COMPLETE", 5),
    enum_value("CHECKERS_STATE_AI_RENDER_TICK", 6),
    enum_value("CHECKERS_STATE_NO_MOVES_PLAYER2", 100),
    enum_value("CHECKERS_STATE_NO_MOVES_PLAYER1", 101),
    enum_value("CHECKERS_STATE_INITIALIZE", 1000),
    enum_value("CHECKERS_STATE_INITIALIZED", 1001),
    enum_value("CHECKERS_STATE_BEGIN_PLAY", 1002),
    doc=(
        "Internal checkers presentation and move-animation states. ANIMATION_COMPLETE is consumed "
        "after rendering, but its indirect producer remains unresolved."
    ),
    unstable=True,
)

stable.struct(
    "Checkers_Board",
    member("Checkers_Piece", "playable_by_col[8][4]", 0x0),
    size=0x20,
    doc="Packed checkers board indexed as playable_by_col[col][row >> 1].",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Checkers_MoveSequence",
    member("int32_t", "from_col", 0x0),
    member("int32_t", "from_row", 0x4),
    member("uint32_t", "packed_to_cols", 0x8),
    member("uint32_t", "packed_to_rows", 0xC),
    size=0x10,
    doc="One checkers move with successive destination columns and rows packed into 3-bit fields.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Checkers_MoveList",
    member("int32_t", "count", 0x0),
    member("Checkers_MoveSequence", "records[60]", 0x4),
    size=0x3C4,
    doc="Count-prefixed fixed-capacity list of sixty checkers move sequences.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Collision_BoundingSphere",
    member("Math_Vec3I32", "center", 0x0),
    member("int32_t", "radius", 0xC),
    size=0x10,
)

stable.struct(
    "Actor_CollisionSphereExtentView",
    member("int32_t", "radius_q12", 0x0),
    member("int16_t", "reserved_6c", 0x4),
    member("int16_t", "collision_probe_radius_q6", 0x6),
    size=0x8,
    incomplete=False,
    unstable=True,
    doc="Mode zero uses a signed Q12 sphere radius and a signed Q6 probe radius.",
)

stable.struct(
    "Actor_CollisionBoxExtentView",
    member("int16_t", "extent_x_q6", 0x0),
    member("int16_t", "extent_y_q6", 0x2),
    member("int16_t", "extent_z_q6", 0x4),
    member("int16_t", "collision_probe_radius_q6", 0x6),
    size=0x8,
    incomplete=False,
    unstable=True,
    doc="Nonzero modes use three signed Q6 half extents and a signed Q6 probe radius.",
)

stable.struct(
    "Actor_CollisionShapeExtentOverlay",
    member("uint8_t", "raw[0x8]", 0x0),
    size=0x8,
    incomplete=False,
    unstable=True,
    doc="Eight-byte union storage. View as Actor_CollisionSphereExtentView or Actor_CollisionBoxExtentView according to the mode byte.",
)

stable.struct(
    "Actor_CollisionShapeNodeView",
    member("uint8_t", "reserved_00[0x20]", 0x0),
    member("Math_Vec3I32", "local_pos", 0x20),
    member("uint8_t", "reserved_2c[0x3A]", 0x2C),
    member("uint8_t", "extent_encoding_mode", 0x66),
    member("uint8_t", "reserved_67", 0x67),
    member("Actor_CollisionShapeExtentOverlay", "shape", 0x68),
    size=0x70,
    doc="Internal Scene_Node-compatible shape/render-origin pointee reached through class-specific, allocation-relative Actor +0xC8. Other layouts use +0xCA as animation frame count.",
    unstable=True,
)

stable.struct(
    "Collision_SphereNodeBoundsView",
    member("uint8_t", "reserved_00[8]", 0x0),
    member("Math_Vec3I32", "center", 0x8),
    member("uint8_t", "reserved_14[6]", 0x14),
    member("int16_t", "radius", 0x1A),
    size=0x1C,
    doc="Internal node_type 1 tail view. Center is at node +0x98 and signed radius at +0xAA.",
    unstable=True,
)

stable.struct(
    "Collision_OrientedBoxNodeBoundsView",
    member("Math_Matrix3x3I16", "orientation_q12", 0x0),
    member("int16_t", "orientation_padding", 0x12),
    member("Math_Vec3I32", "center", 0x14),
    member("Math_Vec3I16", "half_extents_q6", 0x20),
    member("int16_t", "polygon_count", 0x26),
    size=0x28,
    doc="Internal node_type 2 tail view with Q12 orientation, Q6 extents, and signed count through node +0xB6.",
    unstable=True,
)

stable.struct(
    "Collision_NodeBoundsOverlay",
    member("uint8_t", "raw[0x28]", 0x0),
    size=0x28,
    doc="Tagged collision-node tail storage. node_type selects the sphere or oriented-box view.",
    unstable=True,
)

stable.struct(
    "Collision_TemporaryNodeContext",
    member("Collision_Node*", "next_in_list", 0x0),
    member("uint8_t", "reserved_04[0x28]", 0x4),
    member("Math_Matrix3x3I16", "transform_matrix", 0x2C, doc="Initialized to the identity matrix."),
    member("int16_t", "transform_padding", 0x3E),
    member("Math_Vec3I32", "origin", 0x40, doc="Initialized for the synthesized wall source."),
    member("Math_Vec3I32", "collision_offset", 0x4C, doc="Copied from the source collision node."),
    member("uint8_t", "reserved_58[0xC]", 0x58),
    member("uint8_t", "node_type", 0x64, doc="Initialized to type 0 for temporary wall resolution."),
    member("uint8_t", "node_cull_flags", 0x65),
    member("uint8_t", "reserved_66[2]", 0x66),
    size=0x68,
    doc="Minimum stack-only collision-source prefix for synthesized edge walls. Only the documented transform, origin, offset, and type fields are initialized before type-0 resolution.",
    unstable=True,
)

stable.struct(
    "Collision_FacePlane",
    member("int32_t", "plane_distance", 0x0, doc="Signed Q10 plane distance."),
    member("Math_Vec3I16", "normal", 0x4, doc="Signed Q12 face normal."),
    member("uint16_t", "normal_padding", 0xA),
    size=0xC,
    doc="Packed runtime face plane. Loads may span +0xA, but collision logic consumes only normal.z.",
    unstable=True,
)

stable.struct(
    "Collision_HitEvent",
    member("Actor_State*", "actor", 0x0, doc="Stored actor. Collision keys are derived through its record."),
    member("uint32_t", "start_frame", 0x4),
    member("uint32_t", "expire_frame", 0x8),
    size=0xC,
    doc="Hit-cooldown entry layout. Registration tables contain eight slots.",
)

stable.struct(
    "Collision_Plane",
    member("Math_Vec3I32", "normal", 0x0),
    member("int32_t", "distance", 0xC),
    member("int32_t", "edge_index", 0x10),
    member("int32_t", "polygon_index", 0x14),
    member(
        "int32_t",
        "surface_type",
        0x18,
        doc=("This surface classifier is copied from package collision face data."),
    ),
    size=0x1C,
)

stable.struct(
    "Collision_Polygon",
    member("Collision_Plane*", "plane_data", 0x0),
    member("uint16_t", "vertex_idx[4]", 0x4),
    member("Collision_FacePlane*", "face_plane", 0xC),
    member(
        "uint16_t",
        "flags",
        0x10,
        doc="Packed collision behavior flags. 0x0001 selects a triangle (clear selects a quad), 0x0004 marks collision-active/participating surfaces, and 0x0040 enables the stricter Q12 normal-Y threshold.",
    ),
    member("int16_t", "material_index", 0x12),
    member("int16_t", "adj_edge0", 0x14),
    member("int16_t", "pad_16", 0x16),
    size=0x18,
)

stable.struct(
    "Collision_Response",
    member("Math_Vec3I16", "surface_normal", 0x0),
    member("int16_t", "padding", 0x6),
    member("Math_Vec3I32", "response_vel", 0x8),
    member("int16_t", "penetration_depth", 0x14),
    member("uint16_t", "landing_state", 0x16),
    size=0x18,
    doc="Collision response record containing normal, velocity, depth, and landing-state fields.",
)

stable.struct(
    "Collision_Slot",
    member("Math_Vec3I32", "normal", 0x0),
    member("uint8_t", "contact_position[4]", 0xC),
    member(
        "uint8_t",
        "response_data[20]",
        0x10,
        doc="Reserved response payload; ownership and lifecycle are internal.",
    ),
    size=0x24,
    doc="Collision/contact slot containing normal data, contact-position data, and response "
    "payload storage.",
    unstable=True,
)

stable.struct(
    "Component_CollisionRecord",
    member("Component_Definition*", "definition", 0x0),
    member("uint16_t", "attachment_joint_index", 0x4),
    member("uint16_t", "damage_cooldown", 0x6),
    member(
        "Actor_State*",
        "position_source_actor",
        0x8,
        doc="Actor supplying the collision position. Defaults to the owner actor.",
    ),
    member("Math_Vec3I32", "position", 0xC),
    member("Actor_State*", "reference_actor", 0x18),
    member("int32_t", "previous_distance", 0x1C),
    member("int32_t", "current_distance", 0x20),
    size=0x24,
)

stable.type_alias(
    "Component_CollisionTargetMask",
    "uint8_t",
    doc="Byte-width collision-target mask with target and projectile-mode values.",
    unstable=True,
)

stable.enum(
    "Component_CollisionTargetMaskValue",
    enum_value("COMPONENT_COLLISION_CLASS2_FLAGGED_ACTOR", 0x01),
    enum_value("COMPONENT_COLLISION_CLASS2_UNFLAGGED_ACTOR", 0x02),
    enum_value("COMPONENT_COLLISION_CLASS3_FLAGGED_OWNER", 0x08),
    enum_value("COMPONENT_COLLISION_CLASS3_OTHER_OWNER", 0x10),
    enum_value("COMPONENT_COLLISION_PROJECTILE_BEHAVIOR_MODE", 0x40),
    enum_value("COMPONENT_COLLISION_POWERUP", 0x80),
    enum_value("COMPONENT_COLLISION_ALL_FLAGS", 0xFF),
    doc="Component_CollisionTargetMask bits selected by Actor_CheckCollisionType. FLAGGED/UNFLAGGED names describe the tested behavior bit, not a gameplay identity.",
    unstable=True,
)


stable.struct(
    "Component_CollisionVolume",
    member("Math_Vec3I16", "half_extents", 0x0),
    member("uint8_t", "is_box", 0x6),
    size=0x8,
    doc=(
        "Compact component collision-volume descriptor with half extents and a shape discriminator."
    ),
    unstable=True,
)

stable.struct(
    "Component_Definition",
    member("uint32_t", "flags", 0x0),
    member("Math_Vec2I32XZ", "initial_vel_xz", 0x4),
    member("int32_t", "gravity", 0xC),
    member("int32_t", "homing_strength", 0x10),
    member("int32_t", "lifetime_range", 0x14),
    member("int16_t", "speed_min", 0x18),
    member("int16_t", "speed_variance", 0x1A),
    member("int32_t", "speed_max", 0x1C),
    member("int32_t", "collision_damage_type", 0x20),
    member("Math_Vec2I32XZ", "scatter_angle_xz", 0x24),
    member("int32_t", "bounce_factor", 0x2C),
    member("int32_t", "trail_effect_id", 0x30),
    member("int32_t", "return_to_owner", 0x34),
    member("int32_t", "collision_radius", 0x38),
    member("int32_t", "collision_height", 0x3C),
    member("uint16_t", "type0_duration_base", 0x40),
    member("uint16_t", "type2_duration_base", 0x42),
    member("uint16_t", "type0_duration_range", 0x44),
    member("uint16_t", "type2_duration_range", 0x46),
    member("uint8_t", "team_id", 0x48),
    member("uint8_t", "active_instance_count", 0x49),
    member("uint8_t", "spawn_count", 0x4A),
    member("uint8_t", "max_active_instances", 0x4B),
    member("uint16_t", "type2_duration_stagger", 0x4C),
    member("Component_CollisionTargetMask", "collision_neutral_mask", 0x4E),
    member("Component_CollisionTargetMask", "collision_positive_mask", 0x4F),
    member("uint16_t", "damage_amount", 0x50),
    member("uint16_t", "damage_cooldown", 0x52),
    member("int32_t", "particle_count", 0x54),
    member("int32_t", "particle_spread", 0x58),
    member("int32_t", "particle_lifetime", 0x5C),
    member("Actor_State*", "actor_templates[4]", 0x60),
    member("int32_t", "sound_ids[3]", 0x70),
    size=0x7C,
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
    size=0x80,
    unstable=True,
)

stable.struct(
    "Component_HitEvent",
    member("Actor_State*", "actor", 0x0, doc="Stored actor. Collision keys are derived through its record."),
    member("Math_RangeI32", "frame_range", 0x4, doc="Start and expiry frames for duplicate admission."),
    size=0xC,
    doc="One entry in the fixed eight-slot component hit-cooldown table.",
)

stable.struct(
    "Component_SpawnParams",
    member("Actor_State*", "owner", 0x0),
    member("Component_Definition*", "definition", 0x4),
    member("uint32_t", "spawn_value_08", 0x8),
    member("uint32_t", "spawn_value_0c", 0xC),
    member("Math_Vec3I32", "target_pos", 0x10),
    member("Actor_State*", "attached_actor", 0x1C),
    member("int32_t", "reserved_20", 0x20),
    member("int32_t", "reserved_24", 0x24),
    member("uint8_t", "state_flags", 0x28),
    member("uint8_t", "pad_29[3]", 0x29),
    member("int32_t", "spawn_frame", 0x2C),
    member(
        "Component_HitEvent",
        "hit_events[8]",
        0x30,
        doc="Best-effort cooldown table. Admission may succeed when all eight slots are occupied.",
    ),
    member("Actor_State*", "hit_actors[6]", 0x90),
    member("int16_t", "hit_depths[6]", 0xA8),
    member("Math_Vec3I32", "impact_velocities[6]", 0xB4),
    member("uint8_t", "hit_count", 0xFC),
    member("uint8_t", "pad_0FD", 0xFD),
    member("Math_Vec3I16", "impact_normal", 0xFE),
    member("uint8_t", "pad_104[4]", 0x104),
    member("Math_Vec3I32", "impact_delta", 0x108),
    member("int16_t", "homing_response_scale", 0x114),
    member("int16_t", "pad_116", 0x116),
    size=0x118,
)

stable.struct(
    "Component_TrailObject",
    member("Math_Vec3I16", "bone_offset", 0x0),
    member("int16_t", "bone_index", 0x6),
    member("uint8_t", "max_segments", 0x8),
    member("uint8_t", "processed_flag", 0x9),
    member(
        "uint8_t",
        "expiry_countdown",
        0xA,
        doc="Remaining inactive overwrites before the trail history fully expires.",
    ),
    member(
        "uint8_t",
        "flags",
        0xB,
        doc="Bit 0 is a template-preserved per-trail force-active-sample override. Bit 1 skips sampling and expiry work while the ring advances.",
    ),
    member("Math_ColorRGBA8", "color", 0xC),
    member("Math_RangeI16", "width_range", 0x10),
    member(
        "Animation_ControllerGroup*",
        "color_controller_group",
        0x14,
        doc="Controller group used by Trail_RenderAnimated for trail color animation.",
    ),
    member("int16_t", "head_index", 0x18),
    member("int16_t", "segment_index", 0x1A),
    member("Trail_Segment*", "segment_array", 0x1C),
    size=0x20,
    unstable=True,
)

stable.struct(
    "DDraw_ColorKey",
    member("uint32_t", "color_space_low_value", 0x0),
    member("uint32_t", "color_space_high_value", 0x4),
    size=0x8,
    doc="DirectDraw DDCOLORKEY range.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "DDraw_PixelFormat",
    member("uint32_t", "size", 0x0),
    member("uint32_t", "flags", 0x4),
    member("uint32_t", "four_cc", 0x8),
    member("uint32_t", "bit_count", 0xC),
    member("uint32_t", "mask_0", 0x10),
    member("uint32_t", "mask_1", 0x14),
    member("uint32_t", "mask_2", 0x18),
    member("uint32_t", "mask_3", 0x1C),
    size=0x20,
    doc="DirectDraw DDPIXELFORMAT. Bit-count and mask fields are unioned by format flags.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "DDraw_Caps2",
    member("uint32_t", "caps", 0x0),
    member("uint32_t", "caps_2", 0x4),
    member("uint32_t", "caps_3", 0x8),
    member("uint32_t", "caps_4", 0xC),
    size=0x10,
    doc="DirectDraw DDSCAPS2 capability words.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "DDSCAPS",
    member("DWORD", "dwCaps", 0x0),
    size=0x4,
    doc="DirectDraw 7 legacy DDSCAPS capability word. The native SDK declares the structure tag as _DDSCAPS and typedefs it as DDSCAPS.",
    stable=True,
    incomplete=False,
)

stable.type_alias(
    "DDSCAPS2",
    "DDraw_Caps2",
    doc="DirectDraw 7 four-word surface-capability structure.",
    stable=True,
)

stable.struct(
    "DDCAPS_DX7",
    member("DWORD", "dwSize", 0x0),
    member("DWORD", "dwCaps", 0x4),
    member("DWORD", "dwCaps2", 0x8),
    member("DWORD", "dwCKeyCaps", 0xC),
    member("DWORD", "dwFXCaps", 0x10),
    member("DWORD", "dwFXAlphaCaps", 0x14),
    member("DWORD", "dwPalCaps", 0x18),
    member("DWORD", "dwSVCaps", 0x1C),
    member("DWORD", "dwAlphaBltConstBitDepths", 0x20),
    member("DWORD", "dwAlphaBltPixelBitDepths", 0x24),
    member("DWORD", "dwAlphaBltSurfaceBitDepths", 0x28),
    member("DWORD", "dwAlphaOverlayConstBitDepths", 0x2C),
    member("DWORD", "dwAlphaOverlayPixelBitDepths", 0x30),
    member("DWORD", "dwAlphaOverlaySurfaceBitDepths", 0x34),
    member("DWORD", "dwZBufferBitDepths", 0x38),
    member("DWORD", "dwVidMemTotal", 0x3C),
    member("DWORD", "dwVidMemFree", 0x40),
    member("DWORD", "dwMaxVisibleOverlays", 0x44),
    member("DWORD", "dwCurrVisibleOverlays", 0x48),
    member("DWORD", "dwNumFourCCCodes", 0x4C),
    member("DWORD", "dwAlignBoundarySrc", 0x50),
    member("DWORD", "dwAlignSizeSrc", 0x54),
    member("DWORD", "dwAlignBoundaryDest", 0x58),
    member("DWORD", "dwAlignSizeDest", 0x5C),
    member("DWORD", "dwAlignStrideAlign", 0x60),
    member("DWORD", "dwRops[8]", 0x64),
    member("DDSCAPS", "ddsOldCaps", 0x84),
    member("DWORD", "dwMinOverlayStretch", 0x88),
    member("DWORD", "dwMaxOverlayStretch", 0x8C),
    member("DWORD", "dwMinLiveVideoStretch", 0x90),
    member("DWORD", "dwMaxLiveVideoStretch", 0x94),
    member("DWORD", "dwMinHwCodecStretch", 0x98),
    member("DWORD", "dwMaxHwCodecStretch", 0x9C),
    member("DWORD", "dwReserved1", 0xA0),
    member("DWORD", "dwReserved2", 0xA4),
    member("DWORD", "dwReserved3", 0xA8),
    member("DWORD", "dwSVBCaps", 0xAC),
    member("DWORD", "dwSVBCKeyCaps", 0xB0),
    member("DWORD", "dwSVBFXCaps", 0xB4),
    member("DWORD", "dwSVBRops[8]", 0xB8),
    member("DWORD", "dwVSBCaps", 0xD8),
    member("DWORD", "dwVSBCKeyCaps", 0xDC),
    member("DWORD", "dwVSBFXCaps", 0xE0),
    member("DWORD", "dwVSBRops[8]", 0xE4),
    member("DWORD", "dwSSBCaps", 0x104),
    member("DWORD", "dwSSBCKeyCaps", 0x108),
    member("DWORD", "dwSSBFXCaps", 0x10C),
    member("DWORD", "dwSSBRops[8]", 0x110),
    member("DWORD", "dwMaxVideoPorts", 0x130),
    member("DWORD", "dwCurrVideoPorts", 0x134),
    member("DWORD", "dwSVBCaps2", 0x138),
    member("DWORD", "dwNLVBCaps", 0x13C),
    member("DWORD", "dwNLVBCaps2", 0x140),
    member("DWORD", "dwNLVBCKeyCaps", 0x144),
    member("DWORD", "dwNLVBFXCaps", 0x148),
    member("DWORD", "dwNLVBRops[8]", 0x14C),
    member("DDSCAPS2", "ddsCaps", 0x16C),
    size=0x17C,
    doc="DirectDraw 7 DDCAPS_DX7 layout used by GetCaps on IDirectDraw7. DwCaps2 is at +0x8, legacy ddsOldCaps is DDSCAPS at +0x84, and ddsCaps is DDSCAPS2 at +0x16C.",
    stable=True,
    incomplete=False,
)

stable.type_alias(
    "DDCAPS",
    "DDCAPS_DX7",
    doc="DirectDraw 7 DDCAPS alias for the 0x17C-byte DDCAPS_DX7 layout.",
    stable=True,
)

stable.struct(
    "DDraw_SurfaceDesc2",
    member("uint32_t", "size", 0x0),
    member("uint32_t", "flags", 0x4),
    member("uint32_t", "height", 0x8),
    member("uint32_t", "width", 0xC),
    member("int32_t", "pitch_or_linear_size", 0x10),
    member("uint32_t", "back_buffer_count_or_depth", 0x14),
    member("uint32_t", "mipmap_count_or_refresh_rate", 0x18),
    member("uint32_t", "alpha_bit_depth", 0x1C),
    member("uint32_t", "reserved", 0x20),
    member("void*", "surface", 0x24),
    member("DDraw_ColorKey", "dest_overlay_color_key", 0x28),
    member("DDraw_ColorKey", "dest_blt_color_key", 0x30),
    member("DDraw_ColorKey", "src_overlay_color_key", 0x38),
    member("DDraw_ColorKey", "src_blt_color_key", 0x40),
    member("DDraw_PixelFormat", "pixel_format", 0x48),
    member("DDraw_Caps2", "caps", 0x68),
    member("uint32_t", "texture_stage", 0x78),
    size=0x7C,
    doc="DirectDraw 7 DDSURFACEDESC2 layout used for surfaces and enumerated display modes.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "DDraw_BltFx",
    member("uint32_t", "size", 0x0),
    member("uint32_t", "ddfx", 0x4),
    member("uint32_t", "rop", 0x8),
    member("uint32_t", "ddrop", 0xC),
    member("uint32_t", "rotation_angle", 0x10),
    member("uint32_t", "z_buffer_opcode", 0x14),
    member("uint32_t", "z_buffer_low", 0x18),
    member("uint32_t", "z_buffer_high", 0x1C),
    member("uint32_t", "z_buffer_base_dest", 0x20),
    member("uint32_t", "z_dest_const_bit_depth", 0x24),
    member("uint32_t", "z_dest_const_or_surface", 0x28),
    member("uint32_t", "z_src_const_bit_depth", 0x2C),
    member("uint32_t", "z_src_const_or_surface", 0x30),
    member("uint32_t", "alpha_edge_blend_bit_depth", 0x34),
    member("uint32_t", "alpha_edge_blend", 0x38),
    member("uint32_t", "reserved", 0x3C),
    member("uint32_t", "alpha_dest_const_bit_depth", 0x40),
    member("uint32_t", "alpha_dest_const_or_surface", 0x44),
    member("uint32_t", "alpha_src_const_bit_depth", 0x48),
    member("uint32_t", "alpha_src_const_or_surface", 0x4C),
    member("uint32_t", "fill_color", 0x50),
    member("DDraw_ColorKey", "dest_color_key", 0x54),
    member("DDraw_ColorKey", "src_color_key", 0x5C),
    size=0x64,
    doc="DirectDraw 7 DDBLTFX layout. fill_color is the union member used by color-fill blits.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "D3D_PrimCaps",
    member("uint32_t", "size", 0x0),
    member("uint32_t", "misc_caps", 0x4),
    member("uint32_t", "raster_caps", 0x8),
    member("uint32_t", "z_compare_caps", 0xC),
    member("uint32_t", "src_blend_caps", 0x10),
    member("uint32_t", "dest_blend_caps", 0x14),
    member("uint32_t", "alpha_compare_caps", 0x18),
    member("uint32_t", "shade_caps", 0x1C),
    member("uint32_t", "texture_caps", 0x20),
    member("uint32_t", "texture_filter_caps", 0x24),
    member("uint32_t", "texture_blend_caps", 0x28),
    member("uint32_t", "texture_address_caps", 0x2C),
    member("uint32_t", "stipple_width", 0x30),
    member("uint32_t", "stipple_height", 0x34),
    size=0x38,
    doc="Direct3D 7 D3DPRIMCAPS layout.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "D3D_DeviceDesc7",
    member("uint32_t", "device_caps", 0x0),
    member("D3D_PrimCaps", "line_caps", 0x4),
    member("D3D_PrimCaps", "triangle_caps", 0x3C),
    member("uint32_t", "device_render_bit_depth", 0x74),
    member("uint32_t", "device_z_buffer_bit_depth", 0x78),
    member("uint32_t", "min_texture_width", 0x7C),
    member("uint32_t", "min_texture_height", 0x80),
    member("uint32_t", "max_texture_width", 0x84),
    member("uint32_t", "max_texture_height", 0x88),
    member("uint32_t", "max_texture_repeat", 0x8C),
    member("uint32_t", "max_texture_aspect_ratio", 0x90),
    member("uint32_t", "max_anisotropy", 0x94),
    member("float", "guard_band_left", 0x98),
    member("float", "guard_band_top", 0x9C),
    member("float", "guard_band_right", 0xA0),
    member("float", "guard_band_bottom", 0xA4),
    member("float", "extents_adjust", 0xA8),
    member("uint32_t", "stencil_caps", 0xAC),
    member("uint32_t", "fvf_caps", 0xB0),
    member("uint32_t", "texture_op_caps", 0xB4),
    member("uint16_t", "max_texture_blend_stages", 0xB8),
    member("uint16_t", "max_simultaneous_textures", 0xBA),
    member("uint32_t", "max_active_lights", 0xBC),
    member("float", "max_vertex_w", 0xC0),
    member("Win32_GUID", "device_guid", 0xC4),
    member("uint16_t", "max_user_clip_planes", 0xD4),
    member("uint16_t", "max_vertex_blend_matrices", 0xD6),
    member("uint32_t", "vertex_processing_caps", 0xD8),
    member("uint32_t", "reserved_1", 0xDC),
    member("uint32_t", "reserved_2", 0xE0),
    member("uint32_t", "reserved_3", 0xE4),
    member("uint32_t", "reserved_4", 0xE8),
    size=0xEC,
    doc="Direct3D 7 D3DDEVICEDESC7 layout with its embedded 16-byte device GUID.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "D3D_DriverInfo",
    member("char", "driver_name[39]", 0x0),
    member("uint8_t", "pad_27", 0x27),
    member("Win32_GUID*", "device_guid_ptr", 0x28),
    member("D3D_DeviceDesc7", "device_capabilities", 0x2C),
    member("uint32_t", "has_hardware_accel", 0x118),
    member("Win32_GUID*", "lp_guid", 0x11C),
    member("DDCAPS", "driver_caps", 0x120),
    member("DDCAPS", "hel_caps", 0x29C),
    member("uint8_t", "reserved_418[124]", 0x418),
    member("uint32_t", "selected_marker", 0x494),
    member("uint8_t", "reserved_498[4]", 0x498),
    member("Win32_GUID", "device_guid", 0x49C),
    member("Win32_GUID", "driver_guid", 0x4AC),
    member("DDraw_SurfaceDesc2", "display_modes[80]", 0x4BC),
    member("uint32_t", "mode_count", 0x2B7C),
    member("int32_t", "preferred_mode_index", 0x2B80),
    member("int32_t", "selected_mode_index", 0x2B84),
    member("uint32_t", "mode_has_hardware_caps", 0x2B88),
    size=0x2B8C,
    doc="DirectDraw/Direct3D driver enumeration record. The hardware and HEL capability blocks are 0x17C-byte DDCAPS values at +0x120 and +0x29C.",
    unstable=True,
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
    size=0x20,
    doc="CRT-compatible file handle layout, used by package and asset loading streams.",
    stable=True,
)

stable.struct(
    "File_OpenMode",
    member("int32_t", "access", 0x0),
    member("int32_t", "share", 0x4),
    size=0x8,
    doc="Access/share-mode pair, passed through file-open wrappers.",
    stable=True,
    incomplete=False,
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
    size=0x8,
    doc="Compact input event record, passed through game input processing.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Input_JoystickState",
    member(
        "Math_Vec3I32",
        "pos",
        0x0,
        doc="DIJOYSTATE lX/lY/lZ axes; input paths threshold X/Y for gamepad controls.",
    ),
    member(
        "Math_Vec3I32",
        "rot",
        0xC,
        doc="DIJOYSTATE lRx/lRy/lRz axes; Rz is thresholded for gamepad controls.",
    ),
    member("int32_t", "sliders[2]", 0x18),
    member("uint32_t", "pov_hat[4]", 0x20),
    member(
        "uint8_t",
        "rgb_buttons[32]",
        0x30,
        doc="DIJOYSTATE.rgbButtons prefix read by Input_GetJoystickButtonByte for button lookup.",
    ),
    size=0x50,
    doc=(
        "DirectInput joystick snapshot read by Input_ReadGamepad and Input_GetJoystickAxis* "
        "helpers. Values are frame-local input samples."
    ),
)

stable.struct(
    "Input_State",
    member("uint32_t", "button_bits", 0x0),
    member("Math_Vec2I16", "axis", 0x4),
    member("Math_Vec2I16", "axis_aux", 0x8),
    size=0xC,
)

stable.struct(
    "Level_BlobHeader",
    member("uint32_t", "material_section_offset", 0x0),
    member("uint32_t", "scene_graph_offset", 0x4),
    member("uint32_t", "mesh_collision_offset", 0x8),
    size=0xC,
    doc="Package level-blob header whose relocation fields are rebased from the loaded blob base.",
    stable=True,
    incomplete=False,
)


stable.struct(
    "Level_DataHeader",
    member("int32_t", "resource_manager_offset", 0x0),
    member("int32_t", "object_tree_offset", 0x4),
    member("int32_t", "level_data_offset", 0x8),
    size=0xC,
    doc="Level data header containing relocation fields fixed up against the loaded level-data relocation base.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Level_Header",
    member("uint32_t", "magic", 0x0),
    member("uint32_t", "version", 0x4),
    member("uint32_t", "material_table_off", 0x8),
    member("uint32_t", "object_tree_off", 0xC),
    member("uint32_t", "collision_off", 0x10),
    member("uint32_t", "data_size", 0x14),
    size=0x18,
    doc="Level-file header for material, object-tree, and collision data. The payload after the header is still undocumented.",
    stable=True,
    incomplete=False,
)


stable.struct(
    "Level_TransitionTimingData",
    member("int32_t", "start_delay_fp12", 0x0),
    member("int32_t", "mid_duration_fp12", 0x4),
    member("int32_t", "end_duration_fp12", 0x8),
    size=0xC,
    doc=(
        "Three consecutive Q12 phase durations shared by level and music transitions. Both paths "
        "borrow the record as const input and convert the values to 30 Hz frame deadlines without "
        "sign or range clamping."
    ),
    stable=True,
    incomplete=False,
)

stable.struct(
    "Material_BlendTextureSet",
    member("DDraw_IDirectDrawSurface7*", "quadrants[4]", 0x0),
    size=0x10,
)

stable.struct(
    "Material_Descriptor",
    member("uint16_t", "flags", 0x0),
    member("Math_SizeU8", "dimensions_minus_1", 0x2),
    member("uint32_t", "pixel_data_ofs", 0x4),
    member("uint32_t", "palette_ofs", 0x8),
    size=0xC,
)

stable.struct(
    "Material_TextureDescriptorRaw",
    member("uint16_t", "flags", 0x0),
    member("Math_SizeU8", "dimensions_minus_1", 0x2),
    member("uint32_t", "texture_entry_relative_pixel_data_offset", 0x4),
    member("uint32_t", "texture_entry_relative_palette_offset", 0x8),
    member(
        "uint32_t",
        "runtime_texture_handle_slots[25]",
        0xC,
        doc="Opaque raw storage later used as 25 DDraw_IDirectDrawSurface7 pointers.",
    ),
    member("uint32_t", "average_transparent_color", 0x70),
    size=0x74,
    incomplete=False,
    unstable=True,
    doc="Raw texture descriptor. +4/+8 offsets are rebased from the separate texture-data package entry into runtime pointers.",
)

stable.struct(
    "Material_RuntimeDescriptor",
    member("uint16_t", "flags", 0x0),
    member("Math_SizeU8", "dimensions_minus_1", 0x2),
    member("uint8_t*", "pixel_data", 0x4),
    member("uint16_t*", "palette", 0x8),
    member(
        "DDraw_IDirectDrawSurface7*",
        "texture_handles[25]",
        0xC,
        doc="Twenty-five runtime surface pointers overlaid on the raw descriptor's +0x0C..+0x6F storage.",
    ),
    member(
        "uint32_t",
        "average_transparent_color",
        0x70,
        doc=(
            "Packed average RGB fill color for transparent or black pixels. Material_LoadTexture resets it to 0xffffffff, and Material_CopyPixelDataToTexture substitutes the computed average RGB when alpha processing sees fully black pixels."
        ),
    ),
    size=0x74,
)

stable.struct(
    "Graphics_SpriteContext",
    member("uint32_t", "flags", 0x0),
    member("Material_RuntimeDescriptor*", "texture_descriptor", 0x4),
    member(
        "uint8_t",
        "reserved_08[6]",
        0x8,
        doc="Reserved sprite context gap before the documented glyph advance adjustment.",
    ),
    member(
        "int16_t",
        "glyph_advance_adjust",
        0xE,
        doc=(
            "Signed text and glyph advance adjustment read by UI_UpdateAndRenderSprites as the fallback spacing word."
        ),
    ),
    member("Math_UV8", "subrect_uv", 0x10),
    member(
        "uint8_t",
        "reserved_12[2]",
        0x12,
        doc=(
            "Runtime tail padding reserved for internal use. Graphics_RenderTexturedSprite may read across these bytes with a masked load."
        ),
    ),
    member("Math_ColorRGB8", "color_mod", 0x14),
    member("uint8_t", "pad_17", 0x17),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Material_Entry",
    member("uint8_t", "pixel_format", 0x0),
    member("uint8_t", "flags_byte_1", 0x1),
    member("Math_SizeU8", "dimensions_minus_1", 0x2),
    member("uint32_t", "texture_offset", 0x4),
    member("uint32_t", "palette_offset", 0x8),
    member("Math_SizeU16", "dimensions", 0xC),
    member("uint8_t", "format", 0x10),
    member("uint8_t", "mipmap_count", 0x11),
    member("uint16_t", "reserved", 0x12),
    size=0x14,
)

stable.struct(
    "Material_Node",
    member("Material_TableEntry*", "material", 0x0),
    member("uint8_t", "node_type", 0x4),
    member("uint8_t", "unknown_05", 0x5),
    member("uint8_t", "unknown_06", 0x6),
    member("uint8_t", "unknown_07", 0x7),
    member("uint8_t*", "animation_record", 0x8),
    member("Material_RuntimeDescriptor**", "frame_table", 0xC),
    size=0x10,
)

stable.struct(
    "Material_NodeAnimRecord",
    member("Math_ColorRGB8", "base_color", 0x0),
    member("uint8_t", "parameter_03", 0x3),
    member("uint8_t", "padding_04", 0x4),
    member("uint8_t", "frame_count_minus_1", 0x5),
    member("uint8_t", "unknown_06", 0x6),
    member("uint8_t", "loop_07", 0x7),
    size=0x8,
)

stable.struct(
    "Material_SectionHeader",
    member("Material_Node*", "node_table", 0x0),
    member(
        "uint32_t",
        "reserved_04",
        0x4,
        doc=(
            "Reserved material section header word used near the count and pointer fields consumed by material fixup and loading paths."
        ),
    ),
    member(
        "uint32_t",
        "reserved_08",
        0x8,
        doc="Reserved material section header word; material loading and fixup has no documented consumer for it.",
    ),
    member("Material_TableEntry*", "material_entries", 0xC),
    member("int16_t", "node_count", 0x10),
    member("int16_t", "material_entry_count", 0x12),
    member(
        "uint32_t",
        "anim_record_count",
        0x14,
    ),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Material_SectionHeaderRaw",
    member("uint32_t", "node_table_offset", 0x0),
    member("uint32_t", "reserved_04", 0x4),
    member("uint32_t", "reserved_08", 0x8),
    member("uint32_t", "material_entries_offset", 0xC),
    member("int16_t", "node_count", 0x10),
    member("int16_t", "material_entry_count", 0x12),
    member("uint32_t", "anim_record_count", 0x14),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Material_SectionLoadView",
    member("Material_Node*", "node_table", 0x0),
    member("uint32_t", "reserved_04", 0x4),
    member("uint32_t", "reserved_08", 0x8),
    member("Material_TableEntry*", "material_entries", 0xC),
    member("int16_t", "node_count", 0x10),
    member("int16_t", "material_entry_count", 0x12),
    member("uint32_t", "anim_record_count", 0x14),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Material_FrameData",
    member(
        "uint32_t",
        "reserved_00",
        0x0,
        doc="Set to zero by Material_BuildStructure before the frame record is copied; no documented consumer uses it.",
    ),
    member(
        "uint32_t",
        "reserved_04",
        0x4,
        doc="Set to zero by Material_BuildStructure before the frame record is copied; no documented consumer uses it.",
    ),
    member(
        "uint32_t",
        "reserved_08",
        0x8,
        doc="Set to zero by Material_BuildStructure before the frame record is copied; no documented consumer uses it.",
    ),
    member(
        "uint32_t",
        "material_node_ref_packed",
        0xC,
        doc="Packed material-node reference written by Material_BuildStructure and masked by Material_FindTextureByFrame to index material nodes.",
    ),
    size=0x10,
    unstable=True,
)

stable.struct(
    "Material_FrameSet",
    member(
        "uint16_t",
        "reserved_00",
        0x0,
        doc="Header word zeroed by Material_BuildStructure.",
    ),
    member(
        "int16_t",
        "frame_count",
        0x2,
        doc=(
            "Number of frame pointers used by Material_FindTextureByFrame to bound frame_ptr_array iteration."
        ),
    ),
    member("Material_FrameData**", "frame_ptr_array", 0x4),
    size=0x8,
    doc="Material animation frame-set header, built for Material_FindTextureByFrame lookups.",
    stable=True,
    incomplete=False,
)


stable.struct(
    "Material_DataRef",
    member("uint32_t", "level_base_address", 0x0),
    member("uint32_t", "material_id", 0x4),
    member("uint32_t", "actual_dimensions", 0x8),
    member("uint8_t", "reserved[12]", 0xC),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Material_NodeRef",
    member("int32_t", "material_index", 0x0),
    member("DDraw_IDirectDrawSurface7*", "texture_surface", 0x4),
    member("uint8_t*", "palette_ptr", 0x8),
    member("int16_t", "frame_index", 0xC),
    member("int16_t", "blend_mode", 0xE),
    member("uint32_t", "runtime_flags", 0x10),
    size=0x14,
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
        doc="Material table entry pointer written by Material_BuildStructure.",
    ),
    member(
        "Material_FrameSet*",
        "frame_set",
        0x8,
        doc="Optional frame-set table written by Material_BuildStructure and searched by Material_FindTextureByFrame.",
    ),
    size=0xC,
    unstable=True,
)


stable.struct(
    "Material_State",
    member("uint32_t", "flags", 0x0),
    member("uint32_t", "material_id", 0x4),
    member("Math_SizeU8", "actual_dimensions", 0x8),
    member("uint16_t", "padding", 0xA),
    member("DDraw_IDirectDrawSurface7*", "texture_handles[4]", 0xC),
    member("DDraw_IDirectDrawSurface7*", "backface_handles[4]", 0x1C),
    member("uint32_t", "ambient_color", 0x2C),
    member("uint32_t", "diffuse_color", 0x30),
    size=0x34,
)

stable.struct(
    "Math_UV8",
    member("uint8_t", "u", 0x0),
    member("uint8_t", "v", 0x1),
    size=0x2,
)

stable.struct(
    "Math_UV16",
    member("int16_t", "u", 0x0),
    member("int16_t", "v", 0x2),
    size=0x4,
)

stable.struct(
    "Math_UVFloat",
    member("float", "u", 0x0),
    member("float", "v", 0x4),
    size=0x8,
)

stable.struct(
    "Math_ColorRGB8",
    member("uint8_t", "r", 0x0),
    member("uint8_t", "g", 0x1),
    member("uint8_t", "b", 0x2),
    size=0x3,
)

stable.struct(
    "Math_ColorRGBI16",
    member("int16_t", "r", 0x0),
    member("int16_t", "g", 0x2),
    member("int16_t", "b", 0x4),
    size=0x6,
)

stable.struct(
    "Math_ColorRGBI32",
    member("int32_t", "r", 0x0),
    member("int32_t", "g", 0x4),
    member("int32_t", "b", 0x8),
    size=0xC,
)

stable.struct(
    "Math_ColorRGBAI32",
    member("int32_t", "r", 0x0),
    member("int32_t", "g", 0x4),
    member("int32_t", "b", 0x8),
    member("int32_t", "a", 0xC),
    size=0x10,
)

stable.struct(
    "Material_TableEntry",
    member("uint32_t", "flags", 0x0),
    member("Material_RuntimeDescriptor*", "texture_descriptor", 0x4),
    member("uint32_t", "material_tint", 0x8),
    member("Material_TextureInfo", "texture_info", 0xC),
    member("Math_UV8", "uv_tile_offset", 0x10),
    member("uint8_t", "texture_info_hi_reserved[2]", 0x12),
    member("Math_ColorRGB8", "color_adjust", 0x14),
    member("uint8_t", "reserved_17", 0x17),
    member("Material_TableEntry*", "next_material_entry", 0x18),
    member("Math_UV8", "explicit_uvs[4]", 0x1C),
    size=0x24,
)

stable.struct(
    "Material_TableEntryRaw",
    member("uint32_t", "flags", 0x0),
    member(
        "uint32_t",
        "section_relative_texture_descriptor_offset",
        0x4,
    ),
    member("uint32_t", "material_tint", 0x8),
    member("Material_TextureInfo", "texture_info", 0xC),
    member("Math_UV8", "uv_tile_offset", 0x10),
    member("uint8_t", "texture_info_hi_reserved[2]", 0x12),
    member("Math_ColorRGB8", "color_adjust", 0x14),
    member("uint8_t", "reserved_17", 0x17),
    member("int32_t", "next_material_index_1based", 0x18),
    member("Math_UV8", "explicit_uvs[4]", 0x1C),
    size=0x24,
)

stable.struct(
    "PKG_MaterialIndexList",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "pad_01", 0x1),
    member("int16_t", "entry_count", 0x2),
    member("uint32_t", "entries_offset", 0x4),
    size=0x8,
)

stable.struct(
    "PKG_MaterialIndexEntry",
    member("int32_t", "type_id", 0x0),
    member("uint32_t", "pad_04", 0x4),
    member("uint32_t", "pixel_count", 0x8),
    member("uint32_t", "sort_index", 0xC),
    member("void*", "payload_10", 0x10),
    member("void*", "payload_14", 0x14),
    size=0x18,
)

stable.struct(
    "Pkg_MaterialIndexList",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "pad_01", 0x1),
    member("int16_t", "entry_count", 0x2),
    member("PKG_MaterialIndexEntry**", "entries", 0x4),
    size=0x8,
)

stable.struct(
    "Tibbs_LevelSpriteMaterialDescriptor",
    member("Material_SectionHeader*", "texture_db", 0x0),
    member("Material_TableEntry*", "material", 0x4),
    member("Pkg_MaterialIndexList*", "material_indices", 0x8),
    size=0xC,
)

stable.struct(
    "PKG_SpriteMaterialLayerRaw",
    member("Material_Entry*", "texture_db", 0x0),
    member("int32_t", "material", 0x4),
    member("uint32_t", "material_indices_rel", 0x8),
    size=0xC,
)

stable.struct(
    "PKG_UsableMaterialEntry",
    member("Material_FrameSet*", "data", 0x0),
    member("Material_TableEntry*", "material", 0x4),
    member("uint32_t", "reserved_08", 0x8),
    size=0xC,
)

stable.struct(
    "PKG_SharedPackage",
    member("uint32_t", "material_section_offset", 0x0),
    member("uint32_t", "sound_table_offset", 0x4),
    member("uint32_t", "string_section_offset", 0x8),
    size=0xC,
)

stable.struct(
    "PKG_TitlePackage",
    member("uint32_t", "material_section_offset", 0x0),
    member("uint32_t", "sound_table_offset", 0x4),
    member("uint32_t", "music_data_offset", 0x8),
    size=0xC,
)

stable.struct(
    "Material_TextureHashEntry",
    member("Material_Entry*", "material_entry_ptr", 0x0),
    member("Math_SizeI16", "dimensions", 0x4),
    member("DDraw_IDirectDrawSurface7*", "texture_data", 0x8),
    member("Animation_FrameData*", "anim_frame_data", 0xC),
    size=0x10,
)

stable.struct(
    "Math_BoundsVector",
    member(
        "Math_Vec3I32",
        "bounds_vec",
        0x0,
        doc="Single bounds/corner vector record.",
    ),
    size=0xC,
    doc=("Single-corner bounds vector record with a vector footprint."),
    unstable=True,
)

stable.struct(
    "Math_Vec2I32XZ",
    member("int32_t", "x", 0x0),
    member("int32_t", "z", 0x4),
    size=0x8,
)

stable.struct(
    "Math_RectF",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "w", 0x8),
    member("float", "h", 0xC),
    size=0x10,
)

stable.struct(
    "Math_ColorBGRA8",
    member("uint8_t", "b", 0x0),
    member("uint8_t", "g", 0x1),
    member("uint8_t", "r", 0x2),
    member("uint8_t", "a", 0x3),
    size=0x4,
)

stable.struct(
    "Math_ColorRGBA8",
    member("uint8_t", "r", 0x0),
    member("uint8_t", "g", 0x1),
    member("uint8_t", "b", 0x2),
    member("uint8_t", "a", 0x3),
    size=0x4,
)

stable.struct(
    "Math_ColorRGBF",
    member("float", "r", 0x0),
    member("float", "g", 0x4),
    member("float", "b", 0x8),
    size=0xC,
)

stable.struct(
    "Math_ScreenPointI16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    size=0x4,
)

stable.struct(
    "Math_EasePairI32",
    member("int32_t", "start", 0x0),
    member("int32_t", "target", 0x4),
    size=0x8,
)

stable.struct(
    "Math_RangeI16",
    member("int16_t", "min", 0x0),
    member("int16_t", "max", 0x2),
    size=0x4,
)

stable.struct(
    "Math_RangeI32",
    member("int32_t", "min", 0x0),
    member("int32_t", "max", 0x4),
    size=0x8,
)

stable.struct(
    "Graphics_Render_ScreenVertexI16Z",
    member("Math_ScreenPointI16", "screen", 0x0),
    member("int32_t", "z", 0x4),
    size=0x8,
)

stable.struct(
    "Math_Matrix3x3I16",
    member("int16_t", "m00", 0x0),
    member("int16_t", "m01", 0x2),
    member("int16_t", "m02", 0x4),
    member("int16_t", "m10", 0x6),
    member("int16_t", "m11", 0x8),
    member("int16_t", "m12", 0xA),
    member("int16_t", "m20", 0xC),
    member("int16_t", "m21", 0xE),
    member("int16_t", "m22", 0x10),
    size=0x12,
)

stable.struct(
    "Scene_TransformNodePrefix",
    member("void*", "next_link", 0x0),
    member("void*", "sibling_or_next_actor", 0x4),
    member("Scene_TransformNodePrefix*", "first_child_or_attachment", 0x8),
    member("Math_Matrix3x3I16", "local_rot_matrix", 0xC),
    member("int16_t", "local_rot_padding", 0x1E),
    member("Math_Vec3I32", "local_position", 0x20),
    member("Math_Matrix3x3I16", "world_rot_matrix", 0x2C),
    member("int16_t", "world_rot_padding", 0x3E),
    member("Math_Vec3I32", "world_position", 0x40),
    member("Math_Vec3I32", "world_delta_position", 0x4C),
    member("uint32_t", "variant_flags_58", 0x58),
    member("uint16_t", "animation_state_5c", 0x5C),
    member("uint16_t", "animation_state_5e", 0x5E),
    member("uint32_t", "animation_tick_60", 0x60),
    member("uint8_t", "node_type", 0x64),
    member("uint8_t", "lifecycle_or_traversal_flags", 0x65),
    member("uint8_t", "subtype_index", 0x66),
    member("uint8_t", "subtype_secondary", 0x67),
    size=0x68,
    incomplete=False,
    unstable=True,
    doc="Shared 0x68 transform prefix used when attachment APIs accept either Scene_Node or Actor_State records.",
)

stable.struct(
    "Actor_ContactSlot",
    member("int32_t", "distance", 0x0),
    member("Actor_State*", "target", 0x4),
    size=0x8,
    doc="One actor-contact tracking slot: squared/scored distance plus the contacted actor.",
    stable=True,
    incomplete=False,
)

stable.struct(
    "Actor_GroundBarycentricQ12",
    member(
        "uint16_t",
        "triangle_selector",
        0x0,
        doc="Selector 0 uses polygon vertices (3,0,1). Selector 1 uses (3,2,1), including true triangles.",
    ),
    member("int16_t", "weight_a_q12", 0x2, doc="Signed Q12 weight for polygon vertex 3."),
    member(
        "int16_t",
        "weight_b_q12",
        0x4,
        doc="Signed Q12 weight for polygon vertex 0 when selector is 0, or vertex 2 when it is 1.",
    ),
    member("int16_t", "weight_c_q12", 0x6, doc="Signed Q12 weight for vertex 1. Equals 0x1000 minus weights A and B."),
    size=0x8,
    doc="Ground-contact interpretation of Actor_State+0x20. The three signed Q12 weights sum to 0x1000.",
    unstable=True,
    incomplete=False,
)

stable.struct(
    "Actor_State",
    member("Actor_State*", "list_next", 0x0),
    member("Actor_State*", "next_actor", 0x4),
    member(
        "Scene_TransformNodePrefix*",
        "attachment_transform_node",
        0x8,
        doc="Runtime attachment transform. May point to a Scene_Node socket or an Actor_State fallback sharing the prefix.",
    ),
    member(
        "Math_Matrix3x3I16",
        "contact_basis",
        0xC,
        doc="Ground/contact orientation basis matrix.",
    ),
    member("int16_t", "contact_basis_pad", 0x1E),
    member(
        "Math_Vec3I32",
        "attach_offset",
        0x20,
        doc="Signed Q6 attachment offset. While grounded its first eight bytes hold Actor_GroundBarycentricQ12.",
    ),
    member(
        "Math_Matrix3x3I16",
        "rot_mat",
        0x2C,
        doc="Actor-local rotation/render transform matrix initialized by Entity_SpawnActor.",
    ),
    member("int16_t", "rot_mat_padding", 0x3E),
    member(
        "Math_Vec3I32",
        "position",
        0x40,
        doc="Logical actor position in game fixed-point units.",
    ),
    member("Math_Vec3I32", "sub_pos", 0x4C),
    member(
        "uint32_t",
        "attachment_options",
        0x58,
        doc="Bit 0 composes child rotation. Bits 1+ encode a one-based 0x18-byte Mesh_Polygon selector.",
    ),
    member("int16_t", "anim_seq_index", 0x5C),
    member("int16_t", "anim_seq_timer", 0x5E),
    member("int32_t", "anim_frame_time", 0x60),
    member("uint8_t", "actor_type", 0x64),
    member("uint8_t", "lifecycle_flags", 0x65),
    member("uint8_t", "attach_slot_index", 0x66),
    member("uint8_t", "render_layer", 0x67),
    member(
        "Math_Vec2I16",
        "visual_scale",
        0x68,
        doc=(
            "Unpacked XY visual scale. Fabricated values can destabilize "
            "Actor_ProcessRendering, so engine-managed spawn/render paths are safer "
            "than direct writes."
        ),
    ),
    member("Mesh_Polygon*", "mesh_polygon_array", 0x6C),
    member("Mesh_RuntimeVertex*", "runtime_vertex_array", 0x70),
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
            "Render-coupled word used as the count or limit for records reached through mesh_polygon_array. Copies require coherent visual state."
        ),
    ),
    member(
        "void*",
        "morph_normal_buffer",
        0x90,
        doc="Per-actor morphed-normal scratch buffer used by skinned/morph render paths.",
    ),
    member(
        "Graphics_Render_SpriteNodeData*",
        "attach_point_table",
        0x94,
        doc="Array of 0x14-byte render/attachment slots. Type-6 entries encode attachment face selection through their offset fields.",
    ),
    member(
        "Math_Vec3I32",
        "world_render_pos",
        0x98,
        doc="Render-position mirror used by camera/render paths; transform writes require logical-position coherence.",
    ),
    member("int16_t", "collision_height", 0xA4),
    member("int16_t", "collision_height_hi", 0xA6),
    member("int16_t", "collision_radius", 0xA8),
    member("int16_t", "cull_radius", 0xAA),
    member("Animation_StateTable*", "anim_asset_table", 0xAC),
    member(
        "Animation_MorphTargetVertex**",
        "visual_morph_or_skin_target_table",
        0xB0,
        doc=(
            "Visual morph and skin target pointer table used by Bone_TransformVerticesWeighted. It is paired with borrowed mesh and scene vertex resources."
        ),
    ),
    member("int32_t", "anim_tick", 0xB4),
    member(
        "int16_t",
        "move_anim_speed",
        0xB8,
        doc="Per-tick animation advance read by Model_AdvanceAnimation.",
    ),
    member("uint8_t", "trail_count", 0xBA),
    member("uint8_t", "component_count", 0xBB),
    member("Component_TrailObject*", "trail_chain_ptr", 0xBC),
    member("Bone_JointTrackState*", "component_array", 0xC0),
    member("uint8_t", "movement_handler_index", 0xC4),
    member("uint8_t", "transition_phase", 0xC5),
    member(
        "uint8_t",
        "collision_class",
        0xC6,
        doc=(
            "Actor-to-actor collision class byte. Nonzero paths dereference collision_component_or_parent_component."
        ),
    ),
    member("uint8_t", "attach_refcount", 0xC7),
    member(
        "Component_Instance*",
        "collision_component_or_parent_component",
        0xC8,
        doc="Collision component pointer used when collision_class is nonzero; preserve it with collision state bytes.",
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
    member(
        "Math_Vec3I32",
        "velocity",
        0xD4,
        doc="Actor velocity vector; bytes +0xD8..+0xDF double as a Signal_QueueEntry[3]/follow-target overlay for some behaviors.",
    ),
    member("int16_t", "facing_current", 0xE0),
    member("int16_t", "facing_target", 0xE2),
    member("int16_t", "facing_blend", 0xE4),
    member("int16_t", "roll_angle", 0xE6),
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
        "Collision_Polygon*",
        "ground_contact_polygon",
        0xEC,
        doc=(
            "Ground-contact polygon paired with ground_collision_node and passed through polygon/contact helpers."
        ),
    ),
    member("int32_t", "rotation", 0xF0),
    member("PKG_ActorRecord*", "record_ptr", 0xF4),
    member("Actor_State*", "child_actor", 0xF8),
    member("int32_t", "fade_alpha", 0xFC),
    member(
        "int16_t**",
        "morph_table_list",
        0x100,
        doc="Pointer list of int16 morph tables used by morph/skin animation paths.",
    ),
    member("PKG_LODEntry*", "level_local_lod_redirect_record", 0x104),
    member("Actor_State*", "parent_actor", 0x108),
    member("int32_t", "collision_push_xz", 0x10C),
    member("int32_t", "collision_response_xz", 0x110),
    member("int32_t", "alloc_size", 0x114),
    member("int32_t", "camera_mat_xz", 0x118),
    member("int32_t", "camera_mat_yz", 0x11C),
    member("int32_t", "camera_pitch", 0x120),
    member("Math_Matrix3x3I16*", "camera_rot_matrix_ptr", 0x124),
    member("Actor_State*", "linked_actor", 0x128),
    member("int32_t", "camera_cos_factor", 0x12C),
    member("void*", "camera_scratch_vec_ptr", 0x130),
    member("int32_t", "camera_yaw", 0x134),
    member("int32_t", "reserved_138", 0x138),
    member("int32_t", "script_timer", 0x13C),
    member(
        "uint8_t",
        "script_entity_slot_index",
        0x140,
        doc="Script-visible level entity-slot index.",
    ),
    member(
        "uint8_t",
        "script_entity_stack[3]",
        0x141,
        doc="Three contiguous script-entity stack bytes used as an array.",
    ),
    member("int32_t", "path_best_distance", 0x144),
    member("Math_Vec3I32", "path_target", 0x148),
    member("Math_Vec3I32", "path_result", 0x154),
    member("Math_Vec3I32XZY", "path_waypoint", 0x160),
    member("int16_t", "owner_entity_index", 0x16C),
    member("uint8_t", "knockback_timer", 0x16E),
    member("int8_t", "attachment_counter", 0x16F),
    member("int32_t", "live_velocity", 0x170),
    member(
        "int32_t",
        "default_coll_radius",
        0x174,
        doc="Actor_State default collision radius restored by reset/despawn paths.",
    ),
    member(
        "int32_t",
        "default_coll_height",
        0x178,
        doc="Actor_State default collision height restored by reset/despawn paths.",
    ),
    member(
        "uint32_t",
        "default_flags",
        0x17C,
        doc="Actor_State default flag mask restored by reset/despawn paths.",
    ),
    member("int32_t", "runtime_state_0", 0x180),
    member("int32_t", "runtime_state_1", 0x184),
    member("int32_t", "runtime_state_2", 0x188),
    member("int32_t", "runtime_state_3", 0x18C),
    member("int16_t", "runtime_jump_state", 0x190),
    member("int16_t", "runtime_state_4_hi", 0x192),
    member("int16_t", "runtime_counter", 0x194),
    member("int16_t", "runtime_state_5_hi", 0x196),
    member("int32_t", "runtime_state_6", 0x198),
    member("Component_Instance*", "attached_component_a", 0x19C),
    member("Component_Instance*", "attached_component_b", 0x1A0),
    member(
        "Actor_ContactSlot",
        "contact_slots[4]",
        0x1A4,
        doc="Four actor-contact tracking slots holding distance plus contacted actor.",
    ),
    size=0x1C4,
    doc="Live actor record. Class 0x0D overlays +0x68..+0x7A with force-emitter state.",
    unstable=True,
)

stable.struct(
    "Component_LocalPositionState",
    member("Math_Vec3I32", "local_pos", 0x0),
    size=0xC,
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
    member(
        "int32_t",
        "collision_flags",
        0x24,
        doc=(
            "These runtime collision flags describe the object itself, separate "
            "from level polygon flags."
        ),
    ),
    member("int32_t", "behavior_state", 0x28),
    member("int32_t", "local_rot", 0x2C),
    member("int16_t", "local_rot_02", 0x30),
    member("int16_t", "local_rot_hi", 0x32),
    member("int32_t", "local_rot_11", 0x34),
    member("int32_t", "local_rot_20", 0x38),
    member("int32_t", "local_rot_22", 0x3C),
    member("Component_LocalPositionState", "local_state", 0x40),
    member("int32_t", "spawn_interval", 0x4C),
    member("int16_t", "spawn_delay", 0x50),
    member("int16_t", "lifetime", 0x52),
    member("Math_Vec3I32", "spawn_offset", 0x54),
    member("int16_t", "collision_packed_state[8]", 0x60),
    member("void*", "sound_slot_table", 0x70),
    member("Math_Vec3I32", "local_scale", 0x74),
    member("Math_Vec3I32", "bone_offset", 0x80),
    member("int32_t", "transform_flags", 0x8C),
    member("int32_t", "render_distance_sq", 0x90),
    member("int32_t", "visibility_mask", 0x94),
    member("Math_Vec3I32", "world_pos", 0x98),
    member("Math_Vec3I32", "prev_world_pos", 0xA4),
    member("Math_Vec3I32", "velocity", 0xB0),
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
    member("Math_Vec3I32", "homing_vel", 0xD4),
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
    size=0x224,
    unstable=True,
)

stable.struct(
    "Entity_State",
    member("uint32_t", "flags", 0x0),
    member("Math_Vec3I32", "bonus_respawn_pos", 0x4),
    member("int32_t", "collision_radius_sq", 0x10),
    member("int32_t", "collision_height_sq", 0x14),
    member("uint8_t", "active_flag", 0x18),
    member("uint8_t", "bonus_respawn_mode", 0x19),
    member("uint16_t", "reserved_1A", 0x1A),
    member("Math_Vec3I32*", "bonus_respawn_target_pos", 0x1C),
    member("int16_t", "attach_offset_x", 0x20),
    member("int16_t", "default_anim_state", 0x22),
    member("int32_t", "local_vars[9]", 0x24),
    member("uint8_t*", "script_base_ptr", 0x48),
    member("Actor_State*", "actor_template_header_ref", 0x4C),
    member("void*", "shared_ref_50", 0x50),
    member("void*", "shared_ref_54", 0x54),
    member("Math_Vec3I32*", "runtime_target_pos", 0x58),
    member("uint8_t", "reserved_5C[44]", 0x5C),
    member("int32_t", "respawn_transition_speed", 0x88),
    member("uint8_t", "reserved_8C[52]", 0x8C),
    member("Math_Vec3I32*", "position_ptr", 0xC0),
    member("int32_t", "param_gravity_friction", 0xC4),
    member("int32_t", "param_c8", 0xC8),
    member("int32_t", "param_cc", 0xCC),
    member("int32_t", "param_max_speed_0", 0xD0),
    member("int32_t", "param_max_speed_1", 0xD4),
    member("int32_t", "param_accel_0", 0xD8),
    member("int32_t", "param_max_speed_2", 0xDC),
    member("int32_t", "param_accel_1", 0xE0),
    member("int32_t", "param_accel_2", 0xE4),
    member("int32_t", "param_accel_3", 0xE8),
    member("int32_t", "param_accel_4", 0xEC),
    member("int32_t", "param_f0", 0xF0),
    member("int32_t", "param_turn_step", 0xF4),
    member("int32_t", "param_mass", 0xF8),
    member("int32_t", "param_fc", 0xFC),
    member("uint8_t", "param_jump_frames", 0x100),
    member("uint8_t", "param_jump_strength", 0x101),
    member("uint8_t", "param_direction_mode", 0x102),
    member("uint8_t", "param_pad_103", 0x103),
    member("int32_t", "param_max_health", 0x104),
    member("int32_t", "param_108", 0x108),
    member("int32_t", "param_10c", 0x10C),
    member("int32_t", "param_110", 0x110),
    member("int32_t", "param_114", 0x114),
    member("int32_t", "param_118", 0x118),
    member("int32_t", "param_11c", 0x11C),
    member("int32_t", "param_120", 0x120),
    member("int32_t", "param_124", 0x124),
    member("Actor_State*", "runtime_actor", 0x128),
    member(
        "uint8_t",
        "team_bitmask[16]",
        0x12C,
        doc="Per-team membership/relation bitmask bytes.",
    ),
    member("int32_t", "script_timer", 0x13C),
    member("uint8_t", "behavior_index", 0x140),
    member("uint8_t", "behavior_stack[3]", 0x141),
    member("int32_t", "path_best_distance", 0x144),
    member("Math_Vec3I32", "path_target", 0x148),
    member("Math_Vec3I32", "path_result", 0x154),
    member("uint8_t", "reserved_160[12]", 0x160),
    member("int32_t", "target_offset_phase", 0x16C),
    member("int32_t", "target_offset_step", 0x170),
    member("int32_t", "saved_collision_radius_sq", 0x174),
    member("int32_t", "saved_collision_height_sq", 0x178),
    member("uint32_t", "saved_flags", 0x17C),
    size=0x180,
)

stable.struct(
    "Math_QuaternionI16",
    member("int16_t", "w", 0x0),
    member("int16_t", "x", 0x2),
    member("int16_t", "y", 0x4),
    member("int16_t", "z", 0x6),
    size=0x8,
)

stable.struct(
    "Math_OrientedBoundsRecord",
    member(
        "Math_Vec3I32",
        "anchor_vec",
        0x0,
        doc="First vector in the internal oriented-bounds record.",
    ),
    member(
        "Math_Vec3I32",
        "extent_vec",
        0xC,
        doc="Second extent-like vector; the remaining collision-shape semantics are internal.",
    ),
    member(
        "Math_Vec3I32",
        "orientation_vec_0",
        0x18,
        doc="First orientation-like vector in the internal oriented-bounds record.",
    ),
    member("Math_Vec2I32", "orientation_vec_1", 0x24),
    size=0x2C,
    doc=(
        "Oriented-bounds record containing center, extent, and orientation-vector fields."
    ),
    unstable=True,
)

stable.struct(
    "Math_Vec3I32",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "z", 0x8),
    size=0xC,
)

stable.struct(
    "Math_TransformQ12Q6",
    member("Math_Matrix3x3I16", "rotation_q12", 0x0),
    member("int16_t", "reserved_12", 0x12),
    member("Math_Vec3I32", "translation_q6", 0x14),
    size=0x20,
    doc="Compact transform containing a Q12 rotation matrix and Q6 translation vector.",
    unstable=True,
    incomplete=False,
)

stable.struct(
    "Math_Vec3I32XZY",
    member("int32_t", "x", 0x0),
    member("int32_t", "z", 0x4),
    member("int32_t", "y", 0x8),
    size=0xC,
    doc="Three signed 32-bit vector components stored in X/Z/Y order.",
    stable=True,
    incomplete=False,
)


stable.struct(
    "Math_Vec3U",
    member("uint32_t", "x", 0x0),
    member("uint32_t", "y", 0x4),
    member("uint32_t", "z", 0x8),
    size=0xC,
)

stable.struct(
    "Math_Vec3F",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    member("float", "z", 0x8),
    size=0xC,
)


stable.struct(
    "Math_Vec3I16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "z", 0x4),
    size=0x6,
)


stable.struct(
    "Math_Vec2I16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    size=0x4,
)

stable.struct(
    "Math_Vec2I32",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    size=0x8,
)

stable.struct(
    "Math_Vec2F",
    member("float", "x", 0x0),
    member("float", "y", 0x4),
    size=0x8,
)

stable.struct(
    "Math_SizeU8",
    member("uint8_t", "width", 0x0),
    member("uint8_t", "height", 0x1),
    size=0x2,
)

stable.struct(
    "Math_SizeI16",
    member("int16_t", "width", 0x0),
    member("int16_t", "height", 0x2),
    size=0x4,
)

stable.struct(
    "Math_RectI16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "w", 0x4),
    member("int16_t", "h", 0x6),
    size=0x8,
)

stable.struct(
    "Math_ViewportI16",
    member("int16_t", "x", 0x0),
    member("int16_t", "y", 0x2),
    member("int16_t", "w", 0x4),
    member("int16_t", "h", 0x6),
    size=0x8,
)

stable.struct(
    "Math_RectI32",
    member("int32_t", "x", 0x0),
    member("int32_t", "y", 0x4),
    member("int32_t", "w", 0x8),
    member("int32_t", "h", 0xC),
    size=0x10,
)

stable.struct(
    "Math_SizeU16",
    member("uint16_t", "width", 0x0),
    member("uint16_t", "height", 0x2),
    size=0x4,
)

stable.struct(
    "Math_SizeU32",
    member("uint32_t", "width", 0x0),
    member("uint32_t", "height", 0x4),
    size=0x8,
)

stable.type_alias(
    "Mesh_CommandType",
    "uint8_t",
    doc="Byte-width mesh command discriminator. Values are declared by Mesh_CommandTypeValue.",
    stable=True,
)

stable.enum(
    "Mesh_CommandTypeValue",
    enum_value("MESH_COMMAND_CONTROLLER", 0),
    enum_value("MESH_COMMAND_VERTEX_COLOR", 1),
    doc="Mesh command handlers selected by Mesh_Command.type.",
    stable=True,
)

stable.type_alias(
    "Mesh_CommandFlags",
    "uint8_t",
    doc="Byte-width mesh command state flags. Values are declared by Mesh_CommandFlagValue.",
    stable=True,
)

stable.enum(
    "Mesh_CommandFlagValue",
    enum_value("MESH_CMD_MODE_ONCE", 0),
    enum_value("MESH_CMD_MODE_LOOP", 1),
    enum_value("MESH_CMD_MODE_PINGPONG", 2),
    enum_value("MESH_CMD_MODE_MASK", 0x07),
    enum_value("MESH_CMD_ACTIVE", 0x08),
    enum_value("MESH_CMD_LATCHED", 0x10),
    enum_value("MESH_CMD_REVERSE", 0x20),
    enum_value("MESH_CMD_AUTOPLAY", 0x40),
    enum_value("MESH_CMD_DIRTY", 0x80),
    doc="Mesh command timing modes and state bits. Individual meanings for mode values 3 through 7 remain unresolved.",
    stable=True,
)

stable.type_alias(
    "Mesh_CommandEvent",
    "uint8_t",
    doc="Byte-width mesh signal event. Values are declared by Mesh_CommandEventValue.",
    unstable=True,
)

stable.enum(
    "Mesh_CommandEventValue",
    enum_value("MESH_EVENT_RESET", 0),
    enum_value("MESH_EVENT_RESERVED_1", 1),
    enum_value("MESH_EVENT_RESERVED_2", 2),
    enum_value("MESH_EVENT_PLAY_FORWARD", 3),
    enum_value("MESH_EVENT_STEP_FORWARD", 4),
    enum_value("MESH_EVENT_PLAY_REVERSE", 5),
    enum_value("MESH_EVENT_STEP_REVERSE", 6),
    enum_value("MESH_EVENT_RESUME", 7),
    enum_value("MESH_EVENT_PAUSE", 8),
    enum_value("MESH_EVENT_SKIP_UPDATE", 9),
    enum_value("MESH_EVENT_LATCH_STOP_IF_ACTIVE", 10),
    doc=(
        "Events consumed by Graphics_UpdateMeshCommandState. Values 1 and 2 consume the signal; "
        "their other external effects remain unknown."
    ),
    unstable=True,
)

stable.struct(
    "Mesh_Command",
    member("Mesh_CommandType", "type", 0x0),
    member(
        "Mesh_CommandFlags",
        "flags",
        0x1,
        doc="Timing mode under MESH_CMD_MODE_MASK plus ACTIVE/LATCHED/REVERSE/AUTOPLAY/DIRTY state bits.",
    ),
    member("int16_t", "signal_id", 0x2),
    member("int32_t", "progress_q12", 0x4),
    member("int32_t", "limit_q12", 0x8),
    member(
        "uint8_t",
        "payload[4]",
        0xC,
        doc="Opaque command-specific payload through offset 0xF. MESH_COMMAND_CONTROLLER passes it to Animation_ProcessController, while MESH_COMMAND_VERTEX_COLOR uses the header as a color command.",
    ),
    size=0x10,
    doc=(
        "Mesh animation/render command prefix shared by Graphics_ProcessMeshCommands and Graphics_UpdateMeshCommandState."
    ),
    unstable=True,
)

stable.struct(
    "Mesh_CmdList",
    member("uint8_t", "flags", 0x0),
    member("uint8_t", "reserved_01", 0x1),
    member("int16_t", "count", 0x2),
    member("Mesh_Command**", "cmd_ptrs", 0x4),
    size=0x8,
    doc="Mesh command-list header. List flags are stored at +0 and byte +1 is unused.",
    unstable=True,
    incomplete=False,
)

stable.struct(
    "Mesh_RuntimePolygon",
    member("void*", "material_ref", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("void*", "p_normal_plane", 0xC),
    member("uint16_t", "face_flags", 0x10),
    member("int16_t", "uv_index", 0x12),
    member("uint16_t", "reserved_14", 0x14),
    member("int16_t", "normal_offset", 0x16),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Mesh_RuntimeVertex",
    member("Math_Vec3I16", "pos", 0x0),
    member("int16_t", "normal_group_index", 0x6),
    member("Math_ColorRGB8", "color", 0x8),
    member("uint8_t", "padding", 0xB),
    size=0xC,
)

stable.struct(
    "Mesh_AccumulatedNormal",
    member("Math_Vec3I16", "normal", 0x0),
    member("int16_t", "pad_06", 0x6),
    size=0x8,
)

stable.struct(
    "Mesh_FaceNormal",
    member("int32_t", "plane_dist", 0x0),
    member("Math_Vec3I16", "normal", 0x4),
    member("int16_t", "padding", 0xA),
    size=0xC,
    doc="Mesh face-plane normal record referenced by Graphics_Polygon.face_normal; kept in the Mesh domain because it is stored with mesh polygon data.",
)

stable.struct(
    "Mesh_MaterialRef",
    member("Material_TableEntry*", "material_table_ptr", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("void*", "texture_data_ptr", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "uv_index", 0x12),
    member("int16_t", "depth_bias", 0x14),
    member("int16_t", "depth_bias_q12", 0x16),
    size=0x18,
)

stable.struct(
    "Mesh_RenderNodeEntry",
    member(
        "uint8_t",
        "reserved_00[10]",
        0x0,
        doc=(
            "Reserved prefix before render count and flags. PKG_FixUpResourceObjectNodeType3ComplexActorLike fixes descriptor data and the relocated tail pointer."
        ),
    ),
    member(
        "uint8_t",
        "render_entry_count",
        0xA,
        doc=(
            "Byte count read by Scene_RenderSubMesh from the mesh render-node entry before rendering the submesh span."
        ),
    ),
    member(
        "uint8_t",
        "render_entry_flags",
        0xB,
        doc="Render-entry flag byte tested by Scene_RenderSubMesh.",
    ),
    member(
        "Material_RefEntry",
        "material_descriptor",
        0xC,
        doc="0x0C material/command descriptor fixed by PKG_FixUpResourceSpriteEntry when PKG_FixUpResourceObjectNodeType3ComplexActorLike walks the render-node table.",
    ),
    member(
        "uint8_t",
        "reserved_18[4]",
        0x18,
        doc=(
            "Reserved gap between the 0x0C material descriptor and the relocated tail pointer; PKG_FixUpResourceObjectNodeType3ComplexActorLike's helper skips these bytes."
        ),
    ),
    member(
        "void*",
        "relocated_tail_ptr",
        0x1C,
        doc="Entry tail pointer rebased by the render-node fixup helper called from PKG_FixUpResourceObjectNodeType3ComplexActorLike.",
    ),
    size=0x20,
    doc="Mesh render-node entry rebased by PKG_FixUpResourceObjectNodeType3ComplexActorLike's entry-table helper.",
    unstable=True,
)

stable.struct(
    "Submesh_Entry",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("char", "bone_index", 0x2),
    member("uint8_t", "pad_03", 0x3),
    member("uint16_t", "vert_start_index", 0x4),
    member("uint16_t", "vert_count", 0x6),
    member("int16_t", "poly_start_index", 0x8),
    member("int16_t", "poly_count", 0xA),
    member("int16_t", "unknown_0c", 0xC),
    member("int16_t", "effect_count", 0xE),
    member("Math_Vec2I16", "scale", 0x10),
    size=0x14,
    doc="Skinned mesh-piece descriptor selecting vertex and polygon index ranges; consumed by Bone_BlendVerticesMultiWeight and Bone_ComputeNormalsPostTransform.",
)

stable.struct(
    "Submesh_RenderSpan",
    member("uint16_t", "unk_00", 0x0),
    member("char", "render_node_entry_index", 0x2),
    member("uint8_t", "unk_03", 0x3),
    member("uint32_t", "unk_04", 0x4),
    member("int16_t", "polygon_ref_start_index", 0x8),
    member("int16_t", "polygon_ref_count", 0xA),
    member("int16_t", "weighted_vertex_start_index", 0xC),
    member("int16_t", "weighted_vertex_count", 0xE),
    member("int16_t", "cached_offset_10", 0x10),
    member("int16_t", "cached_offset_12", 0x12),
    size=0x14,
    doc="Submesh render span selecting polygon-ref and weighted-vertex ranges; consumed by Scene_RenderSubMesh and Bone_TransformWeightedVerticesForRender.",
)

stable.struct(
    "Mesh_Node",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3I16", "position", 0x10),
    member("Math_Vec3I16", "rotation", 0x16),
    member("uint16_t", "flags", 0x1C),
    member("uint16_t", "reserved", 0x1E),
    size=0x20,
)

stable.struct(
    "Mesh_NodeExtended",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3I16", "position", 0x10),
    member("Math_Vec3I16", "rotation", 0x16),
    member("Mesh_Node*", "child_ptr", 0x1C),
    member("Mesh_Node*", "parent_ref", 0x20),
    member("Mesh_Node*", "sibling_ref", 0x24),
    member("void*", "aux_ptr", 0x28),
    member("Math_Matrix3x3I16", "rot_matrix", 0x2C),
    member("int16_t", "matrix_padding", 0x3E),
    member("Math_Vec3I32", "world_pos", 0x40),
    member("Math_Vec3I32", "velocity", 0x4C),
    member("int16_t", "material_flags", 0x58),
    member("Math_Vec3I16", "bound_extent", 0x5A),
    member("uint8_t", "node_flags", 0x60),
    member("uint8_t", "padding_61[1]", 0x61),
    member("int16_t", "scale_y", 0x62),
    member("Mesh_RuntimePolygon*", "polygon_data_ptr", 0x64),
    member("Mesh_RuntimeVertex*", "vertex_array_ptr", 0x68),
    member("PKG_ResourceManager*", "resource_manager_ref", 0x6C),
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
    member("Math_Vec3I32", "bounding_box_min", 0xAC),
    member("Math_Vec3I32", "bounding_box_max", 0xB8),
    member("int32_t", "bounding_box_radius", 0xC4),
    member("int32_t", "bounding_box_flags", 0xC8),
    member("Mesh_Object*", "special_mesh_data_ptr", 0xCC),
    member("Material_DataRef*", "data_material_ptr", 0xD0),
    member("int32_t", "sort_key", 0xD4),
    member("Material_Entry*", "material_list_ptr", 0xD8),
    size=0xDC,
    unstable=True,
)

stable.struct(
    "Mesh_NodeFull",
    member("uint32_t", "node_type", 0x0),
    member("Mesh_Node*", "next_sibling", 0x4),
    member("Mesh_Node*", "first_child", 0x8),
    member("Mesh_Node*", "parent", 0xC),
    member("Math_Vec3I16", "position", 0x10),
    member("Math_Vec3I16", "rotation", 0x16),
    member("uint16_t", "flags", 0x1C),
    member("uint16_t", "material_count", 0x1E),
    member("Mesh_Node*", "parent_ref", 0x20),
    member("Mesh_Node*", "sibling_ref", 0x24),
    member("void*", "aux_ptr", 0x28),
    member("Math_Matrix3x3I16", "rot_matrix", 0x2C),
    member("int16_t", "matrix_padding", 0x3E),
    member("Math_Vec3I32", "world_pos", 0x40),
    member("Math_Vec3I32", "velocity", 0x4C),
    member("int16_t", "material_flags", 0x58),
    member("Math_Vec3I16", "bound_extent", 0x5A),
    member("int16_t", "reserved_60", 0x60),
    member("uint8_t", "node_flags", 0x62),
    member("uint8_t", "render_mode", 0x63),
    member("uint8_t", "subtype_id", 0x64),
    member("uint8_t", "subtype_flags", 0x65),
    member("int16_t", "subtype_count", 0x66),
    member("Material_Entry*", "material_array", 0x68),
    member("void*", "vertex_data", 0x6C),
    member("Mesh_VertexNormal*", "normal_data", 0x70),
    member("Mesh_NodeFull*", "uv_array_ptr", 0x74),
    member("void*", "polygon_data", 0x78),
    member("uint8_t", "controller_slots[4]", 0x7C),
    member("Animation_DataBlock*", "animation_data", 0x80),
    member("void*", "group_lists[6]", 0x84),
    member("void*", "group_linked_list_a", 0x9C),
    member(
        "int32_t",
        "group_reserved_a0",
        0xA0,
        doc=(
            "Reserved slot between group_linked_list_a and group_linked_list_b. PKG_FixUpResourceObjectNodeType0Hierarchy rebases the adjacent list pointers but intentionally skips this slot."
        ),
    ),
    member("void*", "group_linked_list_b", 0xA4),
    size=0xA8,
    unstable=True,
)

stable.struct(
    "Mesh_Object",
    member("void*", "node_table_ptr", 0x0),
    member("Math_RangeI16", "frame_range", 0x4),
    member("Math_RangeI16", "anim_frame_range", 0x8),
    member("void*", "morph_target_table", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "node_count", 0x12),
    member("uint32_t", "reserved_14", 0x14),
    size=0x18,
    unstable=True,
)

stable.struct(
    "Mesh_ObjectNodeEntry",
    member("void*", "scene_node_ptr", 0x0),
    member("int16_t", "index_a", 0x4),
    member("int16_t", "index_b", 0x6),
    member("int16_t", "index_c", 0x8),
    member("int16_t", "index_d", 0xA),
    size=0xC,
    unstable=True,
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
    size=0x18,
    unstable=True,
)


stable.struct(
    "Mesh_Vertex",
    member("Math_Vec3I32", "pos", 0x0),
    size=0xC,
)

stable.struct(
    "Mesh_Vertex3DNormal",
    member("Math_Vec3F", "normal", 0x0),
    member("Math_Vec3F", "normal_2", 0xC),
    member("uint32_t", "color", 0x18),
    size=0x1C,
)

stable.struct(
    "Mesh_VertexColorRGB",
    member("Math_ColorRGBI32", "color", 0x0),
    size=0xC,
)

stable.struct(
    "Mesh_VertexNormal",
    member("Math_Vec3I16", "normal", 0x0),
    member("int16_t", "normal_count", 0x6),
    size=0x8,
)

stable.struct(
    "Mesh_VertexUV",
    member("Math_UV16", "uv", 0x0),
    size=0x4,
)

stable.struct(
    "Mesh_WorkingVertex",
    member("Math_Vec3I16", "pos", 0x0),
    member("int16_t", "bone_index", 0x6),
    member("Math_Vec3I16", "normal", 0x8),
    member("int16_t", "padding_0e", 0xE),
    member("Math_UV16", "uv", 0x10),
    member("uint32_t", "color", 0x14),
    size=0x18,
)


stable.struct(
    "EntityNavigationWorkListBuffer",
    member("int32_t", "active_count", 0x0),
    member("Entity_State*", "active_entities[4]", 0x4),
    member("int32_t", "nav_command_count", 0x14),
    member("Nav_Command", "nav_commands[100]", 0x18),
    size=0x978,
    doc="Double-buffered next-pass work queue with four active-entity slots and 100 navigation commands. Append and scan paths trust both counts without local bounds checks.",
)

stable.struct(
    "Nav_Command",
    member("uint32_t", "command_type", 0x0),
    member("int16_t", "target_selector_a", 0x4),
    member("int16_t", "target_selector_b", 0x6),
    member("int32_t", "speed", 0x8),
    member("Math_Vec3I32", "pos", 0xC),
    size=0x18,
)

stable.struct(
    "Nav_NeighborEntry",
    member("uint16_t", "packed_id", 0x0),
    member("int16_t", "cost", 0x2),
    size=0x4,
    unstable=True,
)

stable.struct(
    "Nav_Network",
    member("int32_t", "node_count", 0x0),
    member("Nav_Node*", "nodes", 0x4),
    size=0x8,
    unstable=True,
)

stable.struct(
    "Nav_Node",
    member("Math_Vec3I32", "pos", 0x0),
    member("uint16_t", "parent_link", 0xC),
    member("int16_t", "neighbor_count", 0xE),
    member("Nav_PathState*", "pathfind_state", 0x10),
    member("Nav_NeighborEntry*", "neighbor_list", 0x14),
    size=0x18,
)

stable.struct(
    "Nav_PathState",
    member("uint32_t", "cost", 0x0),
    member("uint16_t", "node_id", 0x4),
    member("uint16_t", "parent_backlink", 0x6),
    member("int16_t", "step_count", 0x8),
    member("int16_t", "reserved", 0xA),
    size=0xC,
    unstable=True,
)

stable.enum(
    "Physics_ForceEmitterModeFlags",
    enum_value(
        "PHYSICS_FORCE_EMITTER_AXIAL_MODE",
        0,
        doc="Whole mode value zero applies the primary force along emitter local +Y.",
    ),
    enum_value(
        "PHYSICS_FORCE_EMITTER_RADIAL_CANONICAL_MODE",
        1,
        doc="Known nonzero radial mode. Other nonzero values remain opaque.",
    ),
    enum_value(
        "PHYSICS_FORCE_EMITTER_ATTENUATE_BY_MAX_EXTENT",
        0x8,
        doc="Flag bit enabling maximum-extent cutoff and linear radial-force attenuation.",
    ),
    unstable=True,
    doc="Mode zero is axial. Any nonzero value is radial, bit 0x8 enables attenuation, and all other bits are opaque.",
)

stable.struct(
    "Physics_CollisionForceEmitterActorView",
    member("Scene_TransformNodePrefix", "transform", 0x0),
    member("int16_t", "radius_extent_x_q6", 0x68),
    member("int16_t", "radius_extent_y_q6", 0x6A),
    member("int16_t", "radius_extent_z_q6", 0x6C),
    member("int16_t", "collision_probe_radius_q6", 0x6E),
    member("int32_t", "axial_or_inward_force_q12", 0x70),
    member("int32_t", "orbit_force_q12", 0x74),
    member(
        "Physics_ForceEmitterModeFlags",
        "mode_and_attenuation_flags",
        0x78,
    ),
    size=0x7C,
    doc="Class-0x0D force emitter with signed Q6 extents/probe radius and Q12 forces. Radial flag 0x8 uses the maximum extent.",
    unstable=True,
)

stable.struct(
    "Physics_State",
    member("int32_t", "gravity", 0x0),
    member("int32_t", "friction", 0x4),
    member("int32_t", "max_velocity", 0x8),
    member("int32_t", "max_fall_speed", 0xC),
    member("Math_Vec3I32", "acceleration", 0x10),
    member("Actor_State*", "ground_object", 0x1C),
    member("Collision_Polygon*", "ground_polygon", 0x20),
    size=0x24,
    unstable=True,
)

stable.struct(
    "PKG_SplashScreen",
    member("uint32_t", "format_tag", 0x0),
    member("uint8_t", "pixel_data[1228800]", 0x4),
    size=0x12C004,
    unstable=True,
)

stable.struct(
    "PKG_SplashScreenEx",
    member("uint32_t", "type_tag", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "reserved", 0x8),
    member("char", "name[16]", 0xC),
    member("uint8_t", "pixel_data[1228800]", 0x1C),
    size=0x12C01C,
    unstable=True,
)

stable.struct(
    "PKG_ActorTemplate",
    member("PKG_ObjectNodeFixupView*", "lod_nodes", 0x0),
    member("uint32_t", "reserved_0c", 0xC),
    member("Animation_StateTable*", "anim_table", 0x10),
    size=0x14,
)

stable.struct(
    "PKG_CameraDef",
    member("uint8_t", "camera_type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("int16_t", "transition_speed", 0x2),
    member("int16_t", "fov_distance", 0x4),
    member("int16_t", "orbit_yaw", 0x6),
    member("int32_t", "orbit_pitch", 0x8),
    member("Math_Vec3I32", "cam_offset", 0xC),
    member("Math_Vec3I32", "target_offset", 0x18),
    size=0x24,
)

stable.struct(
    "PKG_CollisionFace",
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
    size=0x14,
)

stable.struct(
    "PKG_CollisionFacePlane",
    member("int32_t", "plane_distance", 0x0, doc="Signed Q10 plane distance."),
    member("Math_Vec3I16", "normal", 0x4, doc="Signed Q12 face normal."),
    member("uint16_t", "navigation_offset_present", 0xA),
    member("uint16_t", "adj_edges[4]", 0xC),
    member("Math_Vec3I16", "navigation_offset", 0x14),
    member("uint16_t", "navigation_link_id", 0x1A),
    size=0x1C,
    doc="Package-only face plane. Nonzero +0xA gates the navigation offset stored at +0x14.",
)

stable.struct(
    "PKG_CollisionHeader",
    member("Math_SizeU32", "grid_dimensions", 0x0),
    member("uint32_t", "cell_size", 0x8),
    member("uint32_t", "data_offset", 0xC),
    member("uint8_t", "collision_reserved[16]", 0x10),
    size=0x20,
)

stable.struct(
    "PKG_CollisionShape",
    member("uint32_t", "face_count", 0x0),
    member("uint32_t", "flags_packed", 0x4),
    member("uint32_t", "grid_params", 0x8),
    member("void*", "material_base_ptr", 0xC),
    member("uint32_t", "type_flags", 0x10),
    member("Math_Vec3I32", "extents", 0x14),
    member("void*", "grid_cell_array", 0x20),
    member("PKG_CollisionFace*", "face_array", 0x24),
    member("PKG_CollisionFacePlane*", "plane_array", 0x28),
    member("PKG_CollisionVertex*", "vertex_array", 0x2C),
    member("int32_t", "sentinel", 0x30),
    member("int32_t", "face_array_count", 0x34),
    member("int32_t", "vertex_count", 0x38),
    member("uint32_t", "reserved_3c", 0x3C),
    size=0x40,
)

stable.struct(
    "PKG_CollisionVertex",
    member("Math_Vec3I16", "pos", 0x0, doc="Signed Q0 local-space position."),
    member("Math_Vec3I16", "normal", 0x6),
    size=0xC,
)

stable.struct(
    "Collision_Vertex",
    member("Math_Vec3I16", "pos", 0x0, doc="Signed Q0 local-space position."),
    member(
        "int16_t",
        "reserved_06",
        0x6,
        doc="Reserved collision-vertex tail word following x/y/z coordinates.",
    ),
    member(
        "int16_t",
        "reserved_08",
        0x8,
        doc="Reserved collision-vertex tail word. Only the xyz components are named.",
    ),
    member(
        "int16_t",
        "reserved_0a",
        0xA,
        doc=(
            "The high byte (offset 0xB) is the per-vertex wall tag, and the "
            "low byte is unused. Collision_BuildAndResolveGroundEdgeWalls only "
            "considers a polygon edge as a wall candidate when at least one "
            "endpoint has a nonzero wall tag."
        ),
    ),
    size=0xC,
    unstable=True,
)

stable.struct(
    "Collision_Node",
    member("Collision_Node*", "next_in_list", 0x0),
    member("uint8_t", "reserved_04[40]", 0x4),
    member("Math_Matrix3x3I16", "transform_matrix", 0x2C, doc="Signed Q12 local-to-world rotation matrix."),
    member("int16_t", "transform_padding", 0x3E),
    member("Math_Vec3I32", "origin", 0x40, doc="Signed Q6 world-space origin."),
    member("Math_Vec3I32", "collision_offset", 0x4C),
    member("uint8_t", "reserved_58[0xC]", 0x58),
    member("uint8_t", "node_type", 0x64),
    member("uint8_t", "node_cull_flags", 0x65),
    member("uint8_t", "reserved_66[2]", 0x66),
    member("int16_t", "vertex_count", 0x68),
    member("int16_t", "polygon_count", 0x6A),
    member("Collision_Polygon*", "polygons", 0x6C),
    member("Collision_Vertex*", "vertices", 0x70),
    member("uint8_t", "reserved_74[20]", 0x74),
    member(
        "uint32_t",
        "flags",
        0x88,
        doc="Bit 0x2000 makes the central resolver reinterpret its vertices input as a +0x70 provider proxy.",
    ),
    member("int32_t", "collision_radius", 0x8C),
    member("Collision_NodeBoundsOverlay", "bounds", 0x90),
    size=0xB8,
    doc="Packed collision subtype view over in-place scene/package data with a tagged bounds tail. 0xB8 is the observed access extent, not an allocation stride.",
)

stable.struct(
    "PKG_ComponentData",
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
    size=0x70,
    unstable=True,
)

stable.struct(
    "PKG_FaceNormal",
    member("int32_t", "plane_distance", 0x0),
    member("Math_Vec3I16", "normal", 0x4),
    member("uint8_t", "padding_0a[2]", 0xA),
    size=0xC,
)

stable.struct(
    "PKG_GeometryChunk",
    member("uint32_t", "vertex_offset", 0x0),
    member("uint32_t", "vertex_count", 0x4),
    member("uint32_t", "polygon_offset", 0x8),
    member("uint32_t", "polygon_count", 0xC),
    member("uint32_t", "material_ref", 0x10),
    member("uint32_t", "flags", 0x14),
    size=0x18,
)

stable.struct(
    "PKG_GeometryResource",
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
    size=0x30,
)


stable.struct(
    "PKG_LevelHeader",
    member("uint32_t", "cam_default_offset", 0x0),
    member("int16_t", "actor_record_count", 0x4),
    member("int16_t", "padding_06", 0x6),
    member("int16_t", "initial_entity_index", 0x8),
    member("int16_t", "entity_count", 0xA),
    member("uint32_t", "entity_array_offset", 0xC),
    member("uint32_t", "sound_definition_list_offset", 0x10),
    member("int16_t", "sound_or_var_count", 0x14),
    member("int16_t", "var_count", 0x16),
    member("uint32_t", "var_list_offset", 0x18),
    member(
        "int16_t",
        "powerup_count",
        0x1C,
        doc="Number of Powerup_Entry spawn records in powerup_list_offset.",
    ),
    member(
        "int16_t",
        "sound_definition_count",
        0x1E,
    ),
    member(
        "uint32_t",
        "powerup_list_offset",
        0x20,
        doc="Package-relative position to the Powerup_Entry spawn-record list.",
    ),
    member(
        "uint32_t",
        "powerup_actor_template_offsets[16]",
        0x24,
        doc=(
            "Fixed 16-slot table of package-relative PKG_ActorTemplate positions used as powerup clone sources. Runtime Level_RuntimeData.powerup_actor_template_slots contains the fixed-up PKG_ActorTemplate* sources."
        ),
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
    size=0x8C,
    doc="Package level header containing actor, entity-slot, sound, powerup, theme, trail, sprite, nav, and material relocation fields.",
)

stable.struct(
    "PKG_LevelResource",
    member("uint32_t", "node_count", 0x0),
    member("uint32_t", "resource_mgr_offset", 0x4),
    member("uint32_t", "material_table_offset", 0x8),
    member("uint32_t", "material_count", 0xC),
    size=0x10,
    doc="Package level-resource header used as the relocation base for resource-manager and material-table fields.",
)

stable.struct(
    "PKG_MaterialRef",
    member("uint32_t", "texture_id", 0x0),
    member("Material_Descriptor*", "texture_desc_ptr", 0x4),
    member("uint32_t", "properties", 0x8),
    size=0xC,
)

stable.struct(
    "PKG_MaterialTableEntry",
    member("uint32_t", "texture_offset", 0x0),
    member("Material_Descriptor*", "texture_desc_ptr", 0x4),
    member("Math_SizeU16", "dimensions", 0x8),
    member("uint32_t", "runtime_surface", 0xC),
    member("uint32_t", "pixel_data_offset", 0x10),
    member("uint16_t", "format_flags", 0x14),
    member("uint16_t", "mipmap_count", 0x16),
    member("uint32_t", "palette_offset", 0x18),
    member("uint32_t", "extra_flags", 0x1C),
    member("uint32_t", "extra_data", 0x20),
    size=0x24,
)

stable.struct(
    "PKG_MenuTextureResource",
    member("uint32_t", "format", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "reserved", 0x8),
    member("char", "filename[16]", 0xC),
    size=0x1C,
)

stable.struct(
    "PKG_ObjectNodeFixupView",
    member("uint32_t", "node_type", 0x0),
    member("PKG_ObjectNodeFixupView*", "next_sibling", 0x4),
    member("PKG_ObjectNodeFixupView*", "first_child", 0x8),
    member("Math_Matrix3x3I16", "local_matrix", 0xC),
    member("int16_t", "pad_1e", 0x1E),
    member("Math_Vec3I32", "local_pos", 0x20),
    member("Math_Matrix3x3I16", "world_matrix", 0x2C),
    member("int16_t", "pad_3e", 0x3E),
    member("Math_Vec3I32", "world_pos", 0x40),
    member("uint32_t", "unk_4c[3]", 0x4C),
    member("int16_t", "sentinel_58[4]", 0x58),
    member("uint32_t", "unk_60", 0x60),
    member("uint8_t", "node_fixup_type", 0x64),
    member("uint8_t", "flags_65", 0x65),
    member("int16_t", "material_ref_count", 0x66),
    member("int16_t", "vertex_count", 0x68),
    member("uint16_t", "count_6a", 0x6A),
    member("void*", "material_refs", 0x6C),
    member("void*", "normals_or_list", 0x70),
    member("void*", "uv_or_child_list", 0x74),
    member("void*", "material_section_or_list", 0x78),
    member("void*", "material_indices_or_polygons", 0x7C),
    member("void*", "aux_80", 0x80),
    member("void*", "aux_84", 0x84),
    member("uint32_t", "flags_88", 0x88),
    member("uint32_t", "field_8c", 0x8C),
    member("void*", "entries_90", 0x90),
    member("void*", "entries_94", 0x94),
    member("uint32_t", "list_head_98", 0x98),
    member("int32_t*", "list_head_9c", 0x9C),
    member("uint32_t", "field_a0", 0xA0),
    member("int32_t*", "list_head_a4", 0xA4),
    member("uint8_t", "reserved_a8[4]", 0xA8),
    member("Animation_StateTable*", "animation_state_table", 0xAC),
    member("int32_t*", "relocation_list_b0", 0xB0),
    member("uint8_t", "reserved_b4[6]", 0xB4),
    member("uint8_t", "render_entry_count", 0xBA),
    member("uint8_t", "secondary_entry_count", 0xBB),
    member("Mesh_RenderNodeEntry*", "render_entries", 0xBC),
    member("void*", "secondary_entries", 0xC0),
    member("uint8_t", "reserved_c4", 0xC4),
    member("uint8_t", "subtype_c5", 0xC5),
    member("uint8_t", "reserved_c6[2]", 0xC6),
    member("void*", "extra_c8", 0xC8),
    member("uint8_t", "reserved_cc[4]", 0xCC),
    member("int32_t*", "linked_records", 0xD0),
    member("uint8_t", "reserved_d4[4]", 0xD4),
    member("void*", "extra_d8", 0xD8),
    member("uint8_t", "reserved_dc[22]", 0xDC),
    member("int16_t", "entry_count_f2", 0xF2),
    member("void*", "component_or_owner", 0xF4),
    member("void*", "global_texture_refs", 0xF8),
    member("uint8_t", "reserved_fc[4]", 0xFC),
    member("int32_t*", "relocation_list_100", 0x100),
    member("PKG_ActorTemplate*", "owner_template", 0x104),
    size=0x108,
    incomplete=True,
    doc=(
        "Fixup-time view of a packed object node used by the PKG_FixUpResourceObjectNode* "
        "family. Offset fields hold blob-relative positions before fixup and absolute "
        "pointers after; the per-type handlers only touch the fields their node_type uses."
    ),
)


stable.struct(
    "PKG_PolygonDataRaw",
    member("uint32_t", "material_index", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("uint32_t", "uv_data_offset", 0xC),
    member("uint32_t", "render_flags", 0x10),
    member("uint32_t", "polygon_flags", 0x14),
    size=0x18,
)

stable.struct(
    "PKG_PolygonListEntry",
    member("uint32_t", "material_index", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("PKG_FaceNormal*", "face_normal_ptr", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "tex_coord_index", 0x12),
    member("uint16_t", "reserved", 0x14),
    member("int16_t", "sort_bias", 0x16),
    size=0x18,
)


stable.struct(
    "PKG_ResourceDirectory",
    member("uint32_t", "material_blob_offset", 0x0),
    member("uint32_t", "mesh_node_root_offset", 0x4),
    member("uint32_t", "level_data_offset", 0x8),
    size=0xC,
    doc=(
        "Package resource directory for material, mesh-node, and level-data sections."
    ),
)

stable.struct(
    "PKG_ResourceHeader",
    member("uint32_t", "resource_type", 0x0),
    member("uint32_t", "data_offset", 0x4),
    member("uint32_t", "data_size", 0x8),
    member("uint32_t", "extra_offset", 0xC),
    member("uint32_t", "flags", 0x10),
    member("uint32_t", "secondary_size", 0x14),
    member("uint32_t", "info_1", 0x18),
    member("uint32_t", "info_2", 0x1C),
    member("uint32_t", "reserved", 0x20),
    size=0x24,
    doc="Package header for one resource, including type, relocation fields, sizes, and flags. Resource-specific payloads are still undocumented.",
    unstable=True,
)

stable.struct(
    "PKG_ScriptHeader",
    member("uint8_t", "size_bytes[4]", 0x0),
    member("uint8_t", "end_bytes[4]", 0x4),
    size=0x8,
)

stable.struct(
    "PKG_SoundResource",
    member("uint32_t", "data_offset", 0x0),
    size=0x4,
)

stable.struct(
    "PKG_SpriteAtlasResource",
    member("uint32_t", "data_offset", 0x0),
    size=0x4,
)


stable.struct(
    "PKG_StringEntry",
    member("uint32_t", "offset", 0x0),
    size=0x4,
)

stable.struct(
    "PKG_TextureResource",
    member("uint32_t", "data_offset", 0x0),
    size=0x4,
)

stable.struct(
    "PKG_TOCEntry",
    member("uint32_t", "offset", 0x0),
    member("uint32_t", "size", 0x4),
    size=0x8,
    doc="Package TOC entry used by the package table. Size-lane aliases share this storage.",
)


stable.struct(
    "PKG_UILayoutEntry",
    member("uint32_t", "element_id", 0x0),
    member("uint32_t", "element_type", 0x4),
    member("int16_t", "param_a", 0x8),
    member("int16_t", "param_b", 0xA),
    member("uint32_t", "reserved", 0xC),
    size=0x10,
    unstable=True,
)


stable.struct(
    "PKG_UVCoord",
    member("Math_UV8", "uv", 0x0),
    size=0x2,
)

stable.struct(
    "Material_TextureInfo",
    member("Math_SizeU8", "dimensions", 0x0),
    member(
        "uint8_t",
        "reserved[2]",
        0x2,
        doc="Upper bytes of the packed texture-info word. No named read path uses these bits.",
    ),
    size=0x4,
    unstable=True,
)

stable.struct(
    "PKG_VertexData",
    member("Math_Vec3I16", "pos", 0x0),
    member("int16_t", "normal_group_index", 0x6),
    member("uint8_t", "r", 0x8),
    member("uint8_t", "g", 0x9),
    member("uint8_t", "b", 0xA),
    member("uint8_t", "a", 0xB),
    size=0xC,
)

stable.struct(
    "PKG_VertexNormalGroup",
    member("Math_Vec3I16", "normal", 0x0),
    member("int16_t", "padding", 0x6),
    size=0x8,
)


stable.struct(
    "Graphics_Batch",
    member("uint32_t", "flags", 0x0),
    member("uint32_t", "material_index", 0x4),
    member("uint32_t", "texture_id", 0x8),
    member("uint32_t", "batch_flags", 0xC),
    member("uint32_t", "screen_coords[12]", 0x10),
    member("float", "view_space_pos[12]", 0x40),
    member("float", "fog_depth", 0x70),
    member("uint32_t", "vertex_colors[4]", 0x74),
    size=0x84,
    unstable=True,
)

stable.struct(
    "Graphics_FrustumClipPlane",
    member("int32_t", "distance", 0x0),
    member("Math_Vec3I16", "normal", 0x4),
    member(
        "int16_t",
        "padding_0a",
        0xA,
        doc="Alignment/pad word in the frustum/clip-plane record.",
    ),
    size=0xC,
    doc="Graphics_ListState frustum/clip-plane record, used by actor visibility checks.",
    unstable=True,
)

stable.struct(
    "Graphics_ClipPlane",
    member("Math_Vec3F", "normal", 0x0),
    member("float", "distance", 0xC),
    size=0x10,
)

stable.struct(
    "Graphics_ClipPos",
    member("Math_Vec3F", "pos", 0x0),
    member("float", "rhw", 0xC),
    size=0x10,
)

stable.struct(
    "Graphics_ClipUVData",
    member("Math_Vec3I32", "screen", 0x0),
    member("int32_t", "rhw", 0xC),
    member("uint32_t", "color", 0x10),
    member("Math_UVFloat", "uv", 0x14),
    size=0x1C,
)

stable.struct(
    "Graphics_ClipVertex",
    member("Graphics_ClipPos", "pos", 0x0),
    member("float", "color_or_data", 0x10),
    member("Math_UVFloat", "uv", 0x14),
    size=0x1C,
    unstable=True,
)

stable.struct(
    "Graphics_ClipAttribute",
    member("float", "components[3]", 0x0),
    size=0xC,
)


stable.struct(
    "Graphics_Color32",
    member("uint8_t", "b", 0x0),
    member("uint8_t", "g", 0x1),
    member("uint8_t", "r", 0x2),
    member("uint8_t", "a", 0x3),
    size=0x4,
)

stable.struct(
    "Graphics_ComponentData",
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
    size=0x30,
)

stable.struct(
    "Graphics_GradientState",
    member("int32_t", "start_color", 0x0),
    member("int32_t", "end_color", 0x4),
    size=0x8,
)

stable.struct(
    "Menu_ColorGradientState",
    member("uint8_t", "low_hold_frames", 0x0),
    member("uint8_t", "high_hold_frames", 0x1),
    member("uint8_t", "step_count", 0x2),
    member("uint8_t", "low_value", 0x3),
    member("uint8_t", "high_value", 0x4),
    member("uint8_t", "current_value", 0x5),
    member("int8_t", "signed_step", 0x6),
    member("uint8_t", "hold_timer", 0x7),
    size=0x8,
    doc="Packed ping-pong intensity gradient with endpoint hold timers.",
)


stable.struct(
    "Graphics_Polygon",
    member("Material_Entry*", "material", 0x0),
    member("uint16_t", "vertex_indices[4]", 0x4),
    member("Mesh_FaceNormal*", "face_normal", 0xC),
    member("uint16_t", "flags", 0x10),
    member("uint16_t", "padding", 0x12),
    member("uint16_t", "uv_indices[4]", 0x14),
    size=0x1C,
)

stable.struct(
    "Graphics_Polygon3D",
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
    size=0xC,
)

stable.struct(
    "Graphics_ProjectedVertex",
    member("Math_ScreenPointI16", "screen", 0x0),
    member("int32_t", "z", 0x4),
    size=0x8,
)

stable.struct(
    "Graphics_PolygonBatchRecord",
    member("uint32_t", "flags", 0x0),
    member("Material_RuntimeDescriptor*", "texture_desc", 0x4),
    member("uint32_t", "material_tint", 0x8),
    member("uint32_t", "tex_wrap_mode", 0xC),
    member("Graphics_PolygonBatchRecord*", "next_in_bucket", 0x10),
    member("Graphics_Render_ScreenVertexI16Z", "screen_vertices[4]", 0x14),
    member("Math_Vec3F", "view_vertices[4]", 0x34),
    member("float", "depth_bias", 0x64),
    member("int32_t", "face_normal_dot", 0x68),
    member("uint32_t", "vertex_colors[4]", 0x6C),
    member("Math_UV8", "tex_uvs[4]", 0x7C),
    member("uint32_t", "render_state_flags", 0x84),
    size=0x88,
    doc="Shared 0x88-byte polygon-batch record. All eight known producers increment the frame index without a capacity guard.",
)

stable.struct(
    "Graphics_PolygonRenderRef",
    member(
        "Material_Entry*",
        "material_ref",
        0x0,
        doc="Runtime material entry pointer read by Graphics_RenderPolygonBatch/Graphics_SetPolygonUVs.",
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
        doc="Fixed-point companion to depth_bias in polygon render refs; used as depth-bias data for render ref records.",
    ),
    size=0x18,
)

stable.struct(
    "Graphics_QuadData",
    member("uint32_t", "flags", 0x0),
    member("Material_Entry*", "material_ptr", 0x4),
    member("uint32_t", "color", 0x8),
    member(
        "uint8_t",
        "texture_width",
        0xC,
        doc="Quad texture width byte read by Graphics_DrawQuad/quad rendering paths.",
    ),
    member(
        "uint8_t",
        "texture_height",
        0xD,
        doc="Quad texture height byte read by Graphics_DrawQuad/quad rendering paths.",
    ),
    member(
        "uint16_t",
        "quad_padding",
        0xE,
        doc=(
            "Reserved word between texture dimensions and quad_state. Graphics_DrawQuad reads texture dimensions before using quad_state."
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
    size=0x98,
    unstable=True,
)

stable.struct(
    "Graphics_QuadRenderData",
    member("uint32_t", "render_flags[5]", 0x0),
    member("Graphics_ProjectedVertex", "projected_vertices[3]", 0x14),
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
    size=0x68,
    unstable=True,
)

stable.struct(
    "Graphics_RGB555Palette",
    member("uint16_t", "colors[256]", 0x0),
    size=0x200,
)

stable.struct(
    "Graphics_SpriteData",
    member("Math_Vec2I32", "pos", 0x0),
    member("int32_t", "texture_id", 0x8),
    size=0xC,
)

stable.struct(
    "Graphics_SpriteLayer",
    member("int32_t", "texture_handle", 0x0),
    member("int32_t", "sprite_count", 0x4),
    member("Graphics_SpriteData*", "sprite_list_ptr", 0x8),
    member("Graphics_SpriteLayer*", "next_layer_ptr", 0xC),
    size=0x10,
)

stable.struct(
    "Graphics_Render_SpriteNodeData",
    member("uint8_t", "node_type", 0x0),
    member("uint8_t", "sprite_flags", 0x1),
    member("int16_t", "frame_index", 0x2),
    member("Material_Entry*", "material_ptr", 0x4),
    member("Math_Vec3I16", "offset", 0x8),
    member("int16_t", "sort_key", 0xE),
    member("Math_Vec2I16", "bound_extent", 0x10),
    size=0x14,
)

stable.struct(
    "Actor_AttachmentSpriteNodeSlotView",
    member("uint8_t", "reserved_00[2]", 0x0),
    member("int8_t", "selection_gate", 0x2),
    member("uint8_t", "reserved_03[5]", 0x3),
    member("int16_t", "base_face_index", 0x8),
    member("int16_t", "random_face_count", 0xA),
    member("uint8_t", "reserved_0c[8]", 0xC),
    size=0x14,
    incomplete=False,
    unstable=True,
    doc="Attachment overlay for a render slot. Automatic selection requires a nonpositive selection value and positive random_face_count.",
)

stable.type_alias(
    "Graphics_SpriteNodeData",
    "Graphics_Render_SpriteNodeData",
    doc="Compatibility alias for the 0x14-byte render sprite-node and attachment-slot record.",
    unstable=True,
)

stable.struct(
    "Graphics_SpriteVertexData",
    member("Math_Vec3I16", "pos", 0x0),
    member("Math_ColorRGB8", "color", 0x6),
    member("uint8_t", "normal_x", 0x9),
    member("uint8_t", "normal_y", 0xA),
    member("uint8_t", "vertex_state", 0xB),
    size=0xC,
)

stable.struct(
    "Graphics_WorkArea",
    member("uint8_t", "transform_scratch[144]", 0x0),
    member("float", "clip_double_buffer[2][160]", 0x90),
    member("uint8_t", "vertex_scratch[104]", 0x590),
    member("float", "color_data[20]", 0x5F8),
    member("Math_ColorRGBF", "colors[4]", 0x648),
    size=0x678,
    unstable=True,
)

stable.struct(
    "Options_BindingLocation",
    member(
        "uint8_t",
        "device_kind",
        0x0,
        doc="Binding device. 0 for keyboard, 1 for joystick.",
    ),
    member(
        "uint8_t",
        "binding_index",
        0x1,
        doc="Binding index, or 0xff when no conflict exists.",
    ),
    size=0x2,
)

stable.struct(
    "Config_Data",
    member("char", "gamma_setting", 0x0),
    member("uint8_t", "pad[3]", 0x1),
    member("int32_t", "keyboard_bindings[13]", 0x4),
    member("int32_t", "joystick_bindings[13]", 0x38),
    size=0x6C,
)

stable.struct(
    "Config_GameSettings",
    member("uint8_t", "sound_enabled", 0x0),
    member("uint8_t", "difficulty", 0x1),
    member(
        "uint8_t",
        "language",
        0x2,
        doc=(
            "Persisted language ID; 0 is English. EU/SC builds write it through Settings_SetLanguage "
            "and check it against the boot-selected language group in Save_CheckContinueSlotLanguage; "
            "EN builds leave it 0."
        ),
    ),
    member("uint8_t", "initialized", 0x3),
    size=0x4,
)

stable.struct(
    "Save_VolumeSettings",
    member("uint16_t", "sfx_volume", 0x0, doc="Sound-effect volume in Q12 units."),
    member("uint16_t", "music_volume", 0x2, doc="Music volume in Q12 units."),
    size=0x4,
    doc="Persisted Q12 SFX and music volumes exposed by the Settings volume accessors.",
)

stable.struct(
    "Save_OperationStatus",
    member("uint8_t", "status_code", 0x0),
    member("uint8_t", "success", 0x1),
    member("uint8_t", "requested_operation", 0x2),
    member("uint8_t", "completed_operation", 0x3),
    size=0x4,
)

stable.struct(
    "Menu_ActionResult",
    member("uint8_t", "action", 0x0),
    member("uint8_t", "selected_index", 0x1),
    member("uint8_t", "sound_effect", 0x2),
    member("uint8_t", "pad", 0x3),
    size=0x4,
)

stable.struct(
    "Save_GameData",
    member("int16_t", "version_marker", 0x0),
    member("uint8_t", "pad_02[2]", 0x2),
    member(
        "Save_VolumeSettings",
        "volume_settings",
        0x4,
        doc="Persisted SFX/music volumes read by Settings_GetSfxVolume and Settings_GetMusicVolume.",
    ),
    member("Config_GameSettings", "game_settings", 0x8),
    member("uint32_t", "rumble_suppress_flag", 0xC),
    size=0x10,
)

stable.struct(
    "Save_GameSlot",
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
        doc="Per-slot valid flag set to 1 by Save_SaveGameToSlot after copying the progress block.",
    ),
    member(
        "uint8_t",
        "save_game_puppy_count_backup",
        0x2,
        doc="Backed-up puppy/life count copied from backup_puppy_count by Save_BackupGamePuppyCount.",
    ),
    member(
        "uint8_t",
        "save_game_init_flag",
        0x3,
        doc="Initialization/progress flag set to 4 by Save_InitializeGameState.",
    ),
    member(
        "uint8_t",
        "save_game_complete_flag",
        0x4,
        doc="Game-complete flag written by Save_SetGameComplete.",
    ),
    member(
        "uint8_t",
        "slot_padding[3]",
        0x5,
        doc="Reserved alignment bytes before the per-level progress arrays.",
    ),
    member(
        "uint8_t",
        "level_completion_flags[16]",
        0x8,
        doc="Per-level puppy/bone completion bitfields updated by Save_SaveGameLevelCompletion.",
    ),
    member(
        "uint8_t",
        "level_bonus_item_flags[16]",
        0x18,
        doc="Per-level dalmatian/bonus bitfields updated by Save_SaveGameLevelCompletion.",
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
        doc="Packed initials for bonus IDs 27 through 31. ID 32 maps to index 5 at +0x42, an unreachable shipped-flow alias of save_game_level_best_time rather than a live sixth slot.",
    ),
    member(
        "uint16_t",
        "save_game_level_best_time",
        0x42,
        doc="Live level-31 best-time word. Generic level-32 slot-5 arithmetic aliases it, but the shipped name-entry flow cannot reach that path.",
    ),
    member(
        "uint8_t",
        "bonus_name_entry_buffer[24]",
        0x44,
        doc="Name-entry/bonus scratch data copied as part of the save slot payload.",
    ),
    size=0x5C,
    doc="Per-slot progress payload, used by save/load UI and completion calculations.",
    unstable=True,
)


stable.struct(
    "Scene_NodeEntry",
    member("uint32_t", "material_ptr", 0x0),
    member("uint32_t", "type_and_flags", 0x4),
    member("uint32_t", "data_offset", 0x8),
    member("uint32_t", "child_table_ptr", 0xC),
    size=0x10,
    unstable=True,
)

stable.struct(
    "Scene_Header",
    member("int32_t", "node_count", 0x0),
    member("Scene_Node*", "root_node_ptr", 0x4),
    member("Scene_Node**", "scene_node_list_ptr", 0x8),
    size=0xC,
)

stable.struct(
    "Scene_LocalTransform",
    member("Math_Vec3I16", "pos", 0x0),
    member("Math_Vec3I16", "rot", 0x6),
    member("uint8_t", "flags", 0xC),
    member("uint8_t", "padding_0d[1]", 0xD),
    size=0xE,
    unstable=True,
)

# Camera_Runtime and Graphics_ListState alias the same camera-state layout.

stable.struct(
    "Camera_Runtime",
    member(
        "int16_t",
        "pose_state_flags",
        0x0,
        doc="Camera pose/state flag word (previously misread as a yaw angle).",
    ),
    member("int16_t", "pitch", 0x2),
    member("int16_t", "look_pitch", 0x4),
    member("int16_t", "look_yaw", 0x6),
    member("int16_t", "view_roll", 0x8),
    member("int16_t", "fov", 0xA),
    member("int32_t", "focal_distance", 0xC),
    member("Math_Vec3I32XZY", "eye_pos", 0x10),
    member("Math_Vec3I32XZY", "target_pos", 0x1C),
    member("Math_ViewportI16", "viewport", 0x28),
    member(
        "Camera_FrustumDirTable",
        "frustum_dirs",
        0x30,
        doc=(
            "Five int16 frustum direction triples (stride 8) written by "
            "Camera_BuildViewMatrix; previously misread as a view matrix plus planes."
        ),
    ),
    member("int32_t", "frustum_plane_1_z", 0x58),
    member("Math_Vec3I32", "frustum_plane_2_normal", 0x5C),
    member("int32_t", "frustum_plane_3_x", 0x68),
    size=0x6C,
    unstable=True,
)

stable.struct(
    "Scene_Node",
    member(
        "Scene_Node*",
        "next_in_resource_list",
        0x0,
        doc="Resource-side list link used to chain Scene_Node records during load and fixup.",
    ),
    member("Scene_Node*", "next_sibling", 0x4),
    member("Scene_Node*", "first_child", 0x8),
    member(
        "Math_Matrix3x3I16",
        "local_rot_matrix",
        0xC,
        doc="Local rotation matrix read by Scene_UpdateNodeAnimation/node-transform update paths.",
    ),
    member("int16_t", "local_rot_reserved", 0x1E),
    member("Math_Vec3I32", "local_pos", 0x20),
    member("Math_Matrix3x3I16", "world_rot_matrix", 0x2C),
    member(
        "int16_t",
        "world_rot_reserved",
        0x3E,
        doc=(
            "Reserved word after world_rot_matrix. Scene_UpdateNodeAnimation writes the surrounding matrix fields, but this word is reserved for internal use."
        ),
    ),
    member("Math_Vec3I32", "world_pos", 0x40),
    member(
        "Math_Vec3I32",
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
        doc="Type-specific signed render and model-entry index used by node_type 6 render paths.",
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
            "or collision/scene polygon array; rebased by PKG_FixUpResource*Node paths."
        ),
    ),
    member(
        "Mesh_RuntimeVertex*",
        "runtime_vertices",
        0x70,
        doc="Runtime mesh vertex array for this node; the root node repurposes this slot as the collision list head.",
    ),
    member("Mesh_CmdList*", "mesh_cmd_list", 0x74),
    member("uint8_t", "visibility_flags[4]", 0x78),
    member(
        "Scene_Node*",
        "shadow_list_link",
        0x7C,
        doc="Next-node link in the shadow render list.",
    ),
    member(
        "void*",
        "model_relocated_ptr_80",
        0x80,
        doc=(
            "Model-node pointer rebased by PKG_FixUpResourceObjectNodeType1MeshActorLike ."
        ),
    ),
    member(
        "void*",
        "model_relocated_ptr_84",
        0x84,
        doc=(
            "Model-node pointer rebased by PKG_FixUpResourceObjectNodeType1MeshActorLike ."
        ),
    ),
    member(
        "uint32_t",
        "model_runtime_flags",
        0x88,
        doc=(
            "Flags read by traversal/render/fixup paths; PKG_FixUpResourceObjectNodeType1MeshActorLike tests bit 1 , and collision polygon tests use transformed coordinates when bits 0x22 are set."
        ),
    ),
    member(
        "uint32_t",
        "variant_transform_tail",
        0x8C,
        doc="Reserved tail dword before trail_effects_ptr.",
    ),
    member("Bone_WeightedVertex*", "weighted_vertices", 0x90),
    member("Submesh_Entry*", "submesh_entry_table", 0x94),
    member(
        "uint8_t",
        "padding_98[4]",
        0x98,
        doc="Reserved tail gap used by render finalize and bone paths.",
    ),
    member("int32_t", "sort_keys[2]", 0x9C),
    member("Scene_Node*", "child_node_list_ptr", 0xA4),
    member(
        "Actor_State*",
        "deferred_actor_list",
        0xA8,
        doc="Head of the deferred actor render list chained through this node.",
    ),
    member(
        "void*",
        "model_animation_data_ptr",
        0xAC,
        doc="Model animation-data pointer rebased by PKG_FixUpResourceObjectNodeType1MeshActorLike and read by Scene_TraverseNodeTree for visibility animation data.",
    ),
    member(
        "void*",
        "offset_fixup_list_ptr",
        0xB0,
        doc="Rebased model-node fixup list pointer walked by PKG_FixUpResourceObjectNodeType1MeshActorLike.",
    ),
    member(
        "int32_t",
        "child_rot_x",
        0xB4,
        doc="Reserved variant-tail field.",
    ),
    member(
        "uint16_t",
        "anim_step_q6",
        0xB8,
        doc="Per-node animation step in Q6 fixed-point units.",
    ),
    member(
        "uint16_t",
        "mesh_node_count",
        0xBA,
        doc="Count for mesh_node_table read by PKG_FixUpResourceObjectNodeType1MeshActorLike/PKG_FixUpResourceObjectNodeType3ComplexActorLike.",
    ),
    member(
        "Mesh_RenderNodeEntry*",
        "mesh_node_table",
        0xBC,
        doc=(
            "Mesh render-node entry table; count is read from mesh_node_count and entries are fixed by PKG_FixUpResourceObjectNodeType3ComplexActorLike before Scene_RenderSubMesh indexes them."
        ),
    ),
    member(
        "int32_t",
        "child_scale_x",
        0xC0,
        doc="Reserved variant-tail field.",
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
        doc="Signed dispatch index used by Scene_TraverseNodeTree and tested by PKG_FixUpResourceObjectNodeType1MeshActorLike before rebasing LOD/config data.",
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
        doc="Reserved variant-tail field.",
    ),
    member("Math_Vec3I32", "render_offset", 0xCC),
    member(
        "Scene_Node*",
        "lod_config_ptr",
        0xD8,
        doc="Reserved variant-tail pointer.",
    ),
    size=0xDC,
    doc=(
        "Scene graph node. The root node repurposes +0x70 as the collision list head, "
        "+0x78 as the active node array, and +0xB4 as the background color."
    ),
    unstable=True,
)

stable.struct(
    "Scene_NodeType",
    member("uint32_t", "type", 0x0),
    size=0x4,
)


stable.struct(
    "Script_Context",
    member("uint8_t*", "script_data", 0x0),
    member("uint8_t*", "script_pc", 0x4),
    member("int32_t*", "stack_ptr", 0x8),
    member("int32_t*", "var_base", 0xC),
    member("uint32_t", "flags", 0x10),
    size=0x14,
    doc="Script interpreter context for instruction state and game-script execution data.",
)

stable.struct(
    "Trail_BoneEffect",
    member("uint32_t", "spawn_frame", 0x0),
    member("int32_t", "waypoint_index", 0x4),
    member("Math_Vec3I32", "pos", 0x8),
    member("Math_Vec2I32XZ", "offset_xz", 0x14),
    member("uint16_t", "rotation", 0x1C),
    member("uint16_t", "scale", 0x1E),
    size=0x20,
)

stable.struct(
    "Trail_Entry",
    member("uint16_t", "spawn_interval", 0x0),
    member("uint16_t", "data_type", 0x2),
    member("void*", "data_ptr", 0x4),
    size=0x8,
)

stable.struct(
    "Trail_Segment",
    member("int32_t", "active", 0x0),
    member("Math_TransformQ12Q6", "world_transform", 0x4),
    member("Math_Vec3I16", "start", 0x24),
    member(
        "int16_t",
        "start_coord_hi",
        0x2A,
        doc="High half of the packed start coordinate word written by Trail_UpdateEffect; Trail_RenderAnimated reads it as part of the packed word.",
    ),
    member("Math_Vec3I16", "end", 0x2C),
    member(
        "int16_t",
        "end_coord_hi",
        0x32,
        doc="High half of the packed end coordinate word written by Trail_UpdateEffect; Trail_RenderAnimated reads it as part of the packed word.",
    ),
    size=0x34,
    unstable=True,
)

stable.struct(
    "Title_SpotEntry",
    member("Math_Vec2I32", "pos", 0x0),
    member("int32_t", "timer", 0x8),
    size=0xC,
)

stable.struct(
    "Menu_LevelProgressInfo",
    member("int16_t", "level_puppy_count", 0x0),
    member("int16_t", "level_bone_count", 0x2),
    member("int16_t", "player_bone_count", 0x4),
    member("int16_t", "player_lives", 0x6),
    size=0x8,
)

stable.struct(
    "UI_LivesIconState",
    member("uint8_t", "target_visible", 0x0),
    member("uint8_t", "animating", 0x1),
    member("uint8_t", "visible", 0x2),
    member("uint8_t", "animation_frame", 0x3),
    size=0x4,
)

stable.struct(
    "UI_StringTableEntry",
    member("uint32_t", "offset", 0x0),
    size=0x4,
)

stable.struct(
    "Tree_MapNode",
    member("Tree_MapNode*", "next", 0x0),
    member("Tree_MapNode*", "prev", 0x4),
    member("Tree_MapNode*", "parent", 0x8),
    member("Tree_MapNode*", "child", 0xC),
    member("int16_t", "rank", 0x10),
    member("uint8_t", "marked", 0x12),
    member("uint8_t", "pad13", 0x13),
    size=0x14,
)

stable.struct(
    "Signal_QueueEntry",
    member("int16_t", "signal_id", 0x0),
    member("uint8_t", "event_type", 0x2),
    member("uint8_t", "reserved_03", 0x3, doc="Reserved byte."),
    member("int16_t", "payload_04", 0x4),
    member("int16_t", "payload_06", 0x6),
    size=0x8,
    doc="Queued signal record with signal ID, event type, and two payload words.",
    stable=True,
    incomplete=False,
)


stable.type_alias(
    "Win32_GUID",
    "GUID",
    doc="Win32 GUID value used by DirectDraw import entries.",
    stable=True,
)

stable.struct(
    "DIDEVICEINSTANCEA",
    member("DWORD", "dwSize", 0x0),
    member("GUID", "guidInstance", 0x4),
    member("GUID", "guidProduct", 0x14),
    member("DWORD", "dwDevType", 0x24),
    member("char", "tszInstanceName[260]", 0x28),
    member("char", "tszProductName[260]", 0x12C),
    member("GUID", "guidFFDriver", 0x230),
    member("WORD", "wUsagePage", 0x240),
    member("WORD", "wUsage", 0x242),
    size=0x244,
    doc="DX5+ ANSI DirectInput device-instance layout used by joystick enumeration.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DIEFFECTINFOA",
    member("DWORD", "dwSize", 0x0),
    member("GUID", "guid", 0x4),
    member("DWORD", "dwEffType", 0x14),
    member("DWORD", "dwStaticParams", 0x18),
    member("DWORD", "dwDynamicParams", 0x1C),
    member("char", "tszName[260]", 0x20),
    size=0x124,
    doc="DX7 ANSI DirectInput effect-information layout.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DIEFFESCAPE",
    member("DWORD", "dwSize", 0x0),
    member("DWORD", "dwCommand", 0x4),
    member("void*", "lpvInBuffer", 0x8),
    member("DWORD", "cbInBuffer", 0xC),
    member("void*", "lpvOutBuffer", 0x10),
    member("DWORD", "cbOutBuffer", 0x14),
    size=0x18,
    doc="DX7 DirectInput force-feedback escape layout.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDGAMMARAMP",
    member("WORD", "red[256]", 0x0),
    member("WORD", "green[256]", 0x200),
    member("WORD", "blue[256]", 0x400),
    size=0x600,
    doc="DirectDraw gamma-ramp layout. The native C tag is _DDGAMMARAMP.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDraw_DeviceIdentifier2",
    member("char", "driver[512]", 0x0),
    member("char", "description[512]", 0x200),
    member("LARGE_INTEGER", "driver_version", 0x400),
    member("DWORD", "vendor_id", 0x408),
    member("DWORD", "device_id", 0x40C),
    member("DWORD", "subsystem_id", 0x410),
    member("DWORD", "revision", 0x414),
    member("GUID", "device_identifier", 0x418),
    member("DWORD", "whql_level", 0x428),
    member(
        "uint8_t",
        "tail_padding[4]",
        0x42C,
        doc="Native i686 tail padding from the structure's eight-byte alignment.",
    ),
    size=0x430,
    doc="i686 DX7 DDDEVICEIDENTIFIER2 layout used by GetDeviceIdentifier on IDirectDraw7.",
    incomplete=False,
    unstable=True,
)

stable.callback_type(
    "LPDIENUMDEVICESCALLBACKA",
    ret="BOOL",
    params=[
        param("DIDEVICEINSTANCEA const*", "device_instance"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawGammaControl_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawGammaControl*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawGammaControl_AddRefFn",
    ret="ULONG",
    params=[param("DDraw_IDirectDrawGammaControl*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawGammaControl_ReleaseFn",
    ret="ULONG",
    params=[param("DDraw_IDirectDrawGammaControl*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawGammaControl_GetGammaRampFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawGammaControl*", "self"),
        param("DWORD", "flags"),
        param("DDGAMMARAMP*", "gamma_ramp"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawGammaControl_SetGammaRampFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawGammaControl*", "self"),
        param("DWORD", "flags"),
        param("DDGAMMARAMP*", "gamma_ramp"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_AddRefFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputEffect*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_ReleaseFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputEffect*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_InitializeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("HINSTANCE", "instance"),
        param("DWORD", "version"),
        param("Win32_GUID const*", "effect_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_GetEffectGuidFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("Win32_GUID*", "effect_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_GetParametersFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("void*", "effect"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="The effect pointer is an ABI-sized stand-in for LPDIEFFECT. Its layout is not exposed.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_SetParametersFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("void const*", "effect"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="The effect pointer is an ABI-sized stand-in for LPCDIEFFECT. Its layout is not exposed.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_StartFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("DWORD", "iterations"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_NoArgsFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputEffect*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by Stop, Download, and Unload.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_GetEffectStatusFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("DWORD*", "status"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputEffect_EscapeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputEffect*", "self"),
        param("DIEFFESCAPE*", "escape_data"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_AddRefFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputA*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_ReleaseFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputA*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_CreateDeviceFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("Win32_GUID const*", "device_guid"),
        param("DInput_IDirectInputDeviceA**", "out_device"),
        param("void*", "outer_unknown"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_EnumDevicesFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("DWORD", "device_type"),
        param("LPDIENUMDEVICESCALLBACKA", "callback"),
        param("void*", "context"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_GetDeviceStatusFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("Win32_GUID const*", "device_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_RunControlPanelFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("HWND", "owner"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputA_InitializeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputA*", "self"),
        param("HINSTANCE", "instance"),
        param("DWORD", "version"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_AddRefFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputDeviceA*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_ReleaseFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputDeviceA*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetCapabilitiesFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDeviceA*", "self"), param("void*", "caps")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_EnumObjectsFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("void*", "callback"),
        param("void*", "context"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetPropertyFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("Win32_GUID const*", "property_guid"),
        param("void*", "property_header"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_SetPropertyFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("Win32_GUID const*", "property_guid"),
        param("void const*", "property_header"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_NoArgsFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDeviceA*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by Acquire and Unacquire.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetDeviceStateFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("DWORD", "data_size"),
        param("void*", "data"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetDeviceDataFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("DWORD", "object_data_size"),
        param("void*", "object_data"),
        param("DWORD*", "inout_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_SetDataFormatFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("void const*", "data_format"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_SetEventNotificationFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDeviceA*", "self"), param("HANDLE", "event")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_SetCooperativeLevelFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("HWND", "window"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetObjectInfoFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("void*", "object_info"),
        param("DWORD", "object"),
        param("DWORD", "how"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_GetDeviceInfoFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("DIDEVICEINSTANCEA*", "device_info"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_RunControlPanelFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("HWND", "owner"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDeviceA_InitializeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDeviceA*", "self"),
        param("HINSTANCE", "instance"),
        param("DWORD", "version"),
        param("Win32_GUID const*", "device_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_AddRefFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputDevice2A*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_ReleaseFn",
    ret="ULONG",
    params=[param("DInput_IDirectInputDevice2A*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetCapabilitiesFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDevice2A*", "self"), param("void*", "caps")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_EnumObjectsFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("void*", "callback"),
        param("void*", "context"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetPropertyFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("Win32_GUID const*", "property_guid"),
        param("void*", "property_header"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SetPropertyFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("Win32_GUID const*", "property_guid"),
        param("void const*", "property_header"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_NoArgsFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDevice2A*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by Acquire, Unacquire, and Poll.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetDeviceStateFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DWORD", "data_size"),
        param("void*", "data"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetDeviceDataFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DWORD", "object_data_size"),
        param("void*", "object_data"),
        param("DWORD*", "inout_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SetDataFormatFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("void const*", "data_format"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SetEventNotificationFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDevice2A*", "self"), param("HANDLE", "event")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SetCooperativeLevelFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("HWND", "window"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetObjectInfoFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("void*", "object_info"),
        param("DWORD", "object"),
        param("DWORD", "how"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetDeviceInfoFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DIDEVICEINSTANCEA*", "device_info"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_RunControlPanelFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("HWND", "owner"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_InitializeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("HINSTANCE", "instance"),
        param("DWORD", "version"),
        param("Win32_GUID const*", "device_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_CreateEffectFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("Win32_GUID const*", "effect_guid"),
        param("void const*", "effect"),
        param("DInput_IDirectInputEffect**", "out_effect"),
        param("void*", "outer_unknown"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="Device2A CreateEffect ABI. The DIEFFECT input remains an opaque pointer.",
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_EnumEffectsFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("void*", "callback"),
        param("void*", "context"),
        param("DWORD", "effect_type"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetEffectInfoFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DIEFFECTINFOA*", "effect_info"),
        param("Win32_GUID const*", "effect_guid"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_GetForceFeedbackStateFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DWORD*", "state"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SendForceFeedbackCommandFn",
    ret="HRESULT",
    params=[param("DInput_IDirectInputDevice2A*", "self"), param("DWORD", "flags")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_EnumCreatedEffectObjectsFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("void*", "callback"),
        param("void*", "context"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_EscapeFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DIEFFESCAPE*", "escape_data"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DInput_IDirectInputDevice2A_SendDeviceDataFn",
    ret="HRESULT",
    params=[
        param("DInput_IDirectInputDevice2A*", "self"),
        param("DWORD", "object_data_size"),
        param("void const*", "object_data"),
        param("DWORD*", "inout_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDrawGammaControlVtbl",
    member("DDraw_IDirectDrawGammaControl_QueryInterfaceFn", "query_interface", 0x0),
    member("DDraw_IDirectDrawGammaControl_AddRefFn", "add_ref", 0x4),
    member("DDraw_IDirectDrawGammaControl_ReleaseFn", "release", 0x8),
    member("DDraw_IDirectDrawGammaControl_GetGammaRampFn", "get_gamma_ramp", 0xC),
    member("DDraw_IDirectDrawGammaControl_SetGammaRampFn", "set_gamma_ramp", 0x10),
    size=0x14,
    doc="Five-slot IDirectDrawGammaControl COM vtable.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDrawGammaControl",
    member("DDraw_IDirectDrawGammaControlVtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirectDrawGammaControl COM interface wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputEffectVtbl",
    member("DInput_IDirectInputEffect_QueryInterfaceFn", "query_interface", 0x0),
    member("DInput_IDirectInputEffect_AddRefFn", "add_ref", 0x4),
    member("DInput_IDirectInputEffect_ReleaseFn", "release", 0x8),
    member("DInput_IDirectInputEffect_InitializeFn", "initialize", 0xC),
    member("DInput_IDirectInputEffect_GetEffectGuidFn", "get_effect_guid", 0x10),
    member("DInput_IDirectInputEffect_GetParametersFn", "get_parameters", 0x14),
    member("DInput_IDirectInputEffect_SetParametersFn", "set_parameters", 0x18),
    member("DInput_IDirectInputEffect_StartFn", "start", 0x1C),
    member("DInput_IDirectInputEffect_NoArgsFn", "stop", 0x20),
    member("DInput_IDirectInputEffect_GetEffectStatusFn", "get_effect_status", 0x24),
    member("DInput_IDirectInputEffect_NoArgsFn", "download", 0x28),
    member("DInput_IDirectInputEffect_NoArgsFn", "unload", 0x2C),
    member("DInput_IDirectInputEffect_EscapeFn", "escape", 0x30),
    size=0x34,
    doc="Thirteen-slot DX7 IDirectInputEffect COM vtable.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputEffect",
    member("DInput_IDirectInputEffectVtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirectInputEffect COM interface wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputAVTable",
    member("DInput_IDirectInputA_QueryInterfaceFn", "query_interface", 0x0),
    member("DInput_IDirectInputA_AddRefFn", "add_ref", 0x4),
    member("DInput_IDirectInputA_ReleaseFn", "release", 0x8),
    member("DInput_IDirectInputA_CreateDeviceFn", "create_device", 0xC),
    member("DInput_IDirectInputA_EnumDevicesFn", "enum_devices", 0x10),
    member("DInput_IDirectInputA_GetDeviceStatusFn", "get_device_status", 0x14),
    member("DInput_IDirectInputA_RunControlPanelFn", "run_control_panel", 0x18),
    member("DInput_IDirectInputA_InitializeFn", "initialize", 0x1C),
    size=0x20,
    doc="Eight-slot DX7 IDirectInputA COM vtable.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputA",
    member("DInput_IDirectInputAVTable*", "vtable", 0x0),
    size=0x4,
    doc="IDirectInputA COM interface wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputDeviceAVTable",
    member("DInput_IDirectInputDeviceA_QueryInterfaceFn", "query_interface", 0x0),
    member("DInput_IDirectInputDeviceA_AddRefFn", "add_ref", 0x4),
    member("DInput_IDirectInputDeviceA_ReleaseFn", "release", 0x8),
    member("DInput_IDirectInputDeviceA_GetCapabilitiesFn", "get_capabilities", 0xC),
    member("DInput_IDirectInputDeviceA_EnumObjectsFn", "enum_objects", 0x10),
    member("DInput_IDirectInputDeviceA_GetPropertyFn", "get_property", 0x14),
    member("DInput_IDirectInputDeviceA_SetPropertyFn", "set_property", 0x18),
    member("DInput_IDirectInputDeviceA_NoArgsFn", "acquire", 0x1C),
    member("DInput_IDirectInputDeviceA_NoArgsFn", "unacquire", 0x20),
    member("DInput_IDirectInputDeviceA_GetDeviceStateFn", "get_device_state", 0x24),
    member("DInput_IDirectInputDeviceA_GetDeviceDataFn", "get_device_data", 0x28),
    member("DInput_IDirectInputDeviceA_SetDataFormatFn", "set_data_format", 0x2C),
    member("DInput_IDirectInputDeviceA_SetEventNotificationFn", "set_event_notification", 0x30),
    member("DInput_IDirectInputDeviceA_SetCooperativeLevelFn", "set_cooperative_level", 0x34),
    member("DInput_IDirectInputDeviceA_GetObjectInfoFn", "get_object_info", 0x38),
    member("DInput_IDirectInputDeviceA_GetDeviceInfoFn", "get_device_info", 0x3C),
    member("DInput_IDirectInputDeviceA_RunControlPanelFn", "run_control_panel", 0x40),
    member("DInput_IDirectInputDeviceA_InitializeFn", "initialize", 0x44),
    size=0x48,
    doc="Eighteen-slot DX7 IDirectInputDeviceA COM vtable.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputDeviceA",
    member("DInput_IDirectInputDeviceAVTable*", "vtable", 0x0),
    size=0x4,
    doc="IDirectInputDeviceA base COM interface returned by CreateDevice on IDirectInputA before the owned helper upgrades it to IDirectInputDevice2A.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputDevice2AVTable",
    member("DInput_IDirectInputDevice2A_QueryInterfaceFn", "query_interface", 0x0),
    member("DInput_IDirectInputDevice2A_AddRefFn", "add_ref", 0x4),
    member("DInput_IDirectInputDevice2A_ReleaseFn", "release", 0x8),
    member("DInput_IDirectInputDevice2A_GetCapabilitiesFn", "get_capabilities", 0xC),
    member("DInput_IDirectInputDevice2A_EnumObjectsFn", "enum_objects", 0x10),
    member("DInput_IDirectInputDevice2A_GetPropertyFn", "get_property", 0x14),
    member("DInput_IDirectInputDevice2A_SetPropertyFn", "set_property", 0x18),
    member("DInput_IDirectInputDevice2A_NoArgsFn", "acquire", 0x1C),
    member("DInput_IDirectInputDevice2A_NoArgsFn", "unacquire", 0x20),
    member("DInput_IDirectInputDevice2A_GetDeviceStateFn", "get_device_state", 0x24),
    member("DInput_IDirectInputDevice2A_GetDeviceDataFn", "get_device_data", 0x28),
    member("DInput_IDirectInputDevice2A_SetDataFormatFn", "set_data_format", 0x2C),
    member("DInput_IDirectInputDevice2A_SetEventNotificationFn", "set_event_notification", 0x30),
    member("DInput_IDirectInputDevice2A_SetCooperativeLevelFn", "set_cooperative_level", 0x34),
    member("DInput_IDirectInputDevice2A_GetObjectInfoFn", "get_object_info", 0x38),
    member("DInput_IDirectInputDevice2A_GetDeviceInfoFn", "get_device_info", 0x3C),
    member("DInput_IDirectInputDevice2A_RunControlPanelFn", "run_control_panel", 0x40),
    member("DInput_IDirectInputDevice2A_InitializeFn", "initialize", 0x44),
    member("DInput_IDirectInputDevice2A_CreateEffectFn", "create_effect", 0x48),
    member("DInput_IDirectInputDevice2A_EnumEffectsFn", "enum_effects", 0x4C),
    member("DInput_IDirectInputDevice2A_GetEffectInfoFn", "get_effect_info", 0x50),
    member("DInput_IDirectInputDevice2A_GetForceFeedbackStateFn", "get_force_feedback_state", 0x54),
    member("DInput_IDirectInputDevice2A_SendForceFeedbackCommandFn", "send_force_feedback_command", 0x58),
    member("DInput_IDirectInputDevice2A_EnumCreatedEffectObjectsFn", "enum_created_effect_objects", 0x5C),
    member("DInput_IDirectInputDevice2A_EscapeFn", "escape", 0x60),
    member("DInput_IDirectInputDevice2A_NoArgsFn", "poll", 0x64),
    member("DInput_IDirectInputDevice2A_SendDeviceDataFn", "send_device_data", 0x68),
    size=0x6C,
    doc=(
        "Twenty-seven-slot DX7 IDirectInputDevice2A vtable. Opaque DirectInput record pointers retain "
        "their pointer ABI without exposing unmodeled layouts."
    ),
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DInput_IDirectInputDevice2A",
    member("DInput_IDirectInputDevice2AVTable*", "vtable", 0x0),
    size=0x4,
    doc="IDirectInputDevice2A COM interface wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "D3D_Viewport7",
    member("uint32_t", "x", 0x0),
    member("uint32_t", "y", 0x4),
    member("uint32_t", "width", 0x8),
    member("uint32_t", "height", 0xC),
    member("float", "min_z", 0x10),
    member("float", "max_z", 0x14),
    size=0x18,
    doc="Direct3D 7 D3DVIEWPORT7 layout used by SetViewport on IDirect3DDevice7.",
    incomplete=False,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_ReleaseFn",
    ret="ULONG",
    params=[param("DDraw_IDirectDraw7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_CreateSurfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("DDraw_IDirectDrawSurface7**", "out_surface"),
        param("void*", "outer_unknown"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_GetCapsFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("DDCAPS*", "driver_caps"),
        param("DDCAPS*", "hel_caps"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="GetCaps on IDirectDraw7 ABI for two full DX7 DDCAPS outputs.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_RestoreDisplayModeFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDraw7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_SetCooperativeLevelFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("HWND", "hwnd"),
        param("uint32_t", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_SetDisplayModeFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
        param("uint32_t", "bits_per_pixel"),
        param("uint32_t", "refresh_rate"),
        param("uint32_t", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_TestCooperativeLevelFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDraw7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_GetDeviceIdentifierFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("DDraw_DeviceIdentifier2*", "device_identifier"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_StartModeTestFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("SIZE*", "modes"),
        param("DWORD", "mode_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDraw7_EvaluateModeFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDraw7*", "self"),
        param("DWORD", "flags"),
        param("DWORD*", "timeout"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_AddRefFn",
    ret="ULONG",
    params=[param("DDraw_IDirectDrawSurface7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_ReleaseFn",
    ret="ULONG",
    params=[param("DDraw_IDirectDrawSurface7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_AddAttachedSurfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_IDirectDrawSurface7*", "attached_surface"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_RectFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("RECT*", "rect")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by AddOverlayDirtyRect.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_BltFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("RECT*", "destination_rect"),
        param("DDraw_IDirectDrawSurface7*", "source_surface"),
        param("RECT*", "source_rect"),
        param("uint32_t", "flags"),
        param("DDraw_BltFx*", "effects"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_BltBatchFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("void*", "batch"),
        param("DWORD", "count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="BltBatch parameter list. DDBLTBATCH records remain opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_BltFastFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DWORD", "x"),
        param("DWORD", "y"),
        param("DDraw_IDirectDrawSurface7*", "source_surface"),
        param("RECT*", "source_rect"),
        param("DWORD", "transparency"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_DeleteAttachedSurfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DWORD", "flags"),
        param("DDraw_IDirectDrawSurface7*", "attached_surface"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_EnumSurfacesCallback7",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "surface"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_EnumAttachedSurfacesFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("void*", "context"),
        param("DDraw_EnumSurfacesCallback7", "callback"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_EnumOverlayZOrdersFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DWORD", "flags"),
        param("void*", "context"),
        param("DDraw_EnumSurfacesCallback7", "callback"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_FlipFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_IDirectDrawSurface7*", "target_override"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetAttachedSurfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_Caps2*", "caps"),
        param("DDraw_IDirectDrawSurface7**", "out_attached_surface"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_DwordFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("DWORD", "value")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by single-DWORD Surface7 methods.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetCapsFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("DDraw_Caps2*", "caps")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_OutObjectFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("void**", "out_object")],
    calling=CallingConvention.CALLBACK,
    doc="Shared pointer ABI for GetClipper, GetPalette, and GetDDInterface. Pointees remain opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_ColorKeyFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DWORD", "flags"),
        param("void*", "color_key"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="GetColorKey/SetColorKey parameter list. DDCOLORKEY remains opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetDCFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("HDC*", "out_dc")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetOverlayPositionFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("LONG*", "out_x"),
        param("LONG*", "out_y"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetPixelFormatFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_PixelFormat*", "pixel_format"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetSurfaceDescFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_InitializeFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("void*", "direct_draw"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="Initialize parameter list. The older IDirectDraw pointee remains opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_NoArgsFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by IsLost, Restore, and ChangeUniquenessValue.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_LockFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("RECT*", "rect"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("uint32_t", "flags"),
        param("HANDLE", "event_handle"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_ReleaseDCFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("HDC", "dc")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_SetOpaqueInterfaceFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("void*", "object")],
    calling=CallingConvention.CALLBACK,
    doc="Shared pointer ABI for SetClipper and SetPalette. Auxiliary COM pointees remain opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_SetOverlayPositionFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("LONG", "x"),
        param("LONG", "y"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_UnlockFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("RECT*", "surface_data"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_UpdateOverlayFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("RECT*", "source_rect"),
        param("DDraw_IDirectDrawSurface7*", "destination_surface"),
        param("RECT*", "destination_rect"),
        param("DWORD", "flags"),
        param("void*", "overlay_effects"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="UpdateOverlay parameter list. DDOVERLAYFX remains opaque.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_DwordSurfaceFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DWORD", "flags"),
        param("DDraw_IDirectDrawSurface7*", "reference_surface"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="UpdateOverlayZOrder ABI.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_SetSurfaceDescFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_SetPrivateDataFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("Win32_GUID const*", "tag"),
        param("void*", "data"),
        param("DWORD", "data_size"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GetPrivateDataFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("Win32_GUID const*", "tag"),
        param("void*", "buffer"),
        param("DWORD*", "inout_buffer_size"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_GuidFn",
    ret="HRESULT",
    params=[
        param("DDraw_IDirectDrawSurface7*", "self"),
        param("Win32_GUID const*", "tag"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="FreePrivateData ABI.",
    unstable=True,
)

stable.callback_type(
    "DDraw_IDirectDrawSurface7_DwordOutFn",
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "self"), param("DWORD*", "out_value")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by GetUniquenessValue, GetPriority, and GetLOD.",
    unstable=True,
)

stable.callback_type(
    "D3D_EnumTextureFormatsCallback",
    ret="HRESULT",
    params=[
        param("DDraw_PixelFormat*", "pixel_format"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_EnumZBufferFormatsCallback",
    ret="HRESULT",
    params=[
        param("DDraw_PixelFormat*", "pixel_format"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_AddRefFn",
    ret="ULONG",
    params=[param("D3D_IDirect3DDevice7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_ReleaseFn",
    ret="ULONG",
    params=[param("D3D_IDirect3DDevice7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetCapsFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("D3D_DeviceDesc7*", "caps"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="GetCaps on IDirect3DDevice7 with a full D3DDEVICEDESC7 output.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_EnumTextureFormatsFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("D3D_EnumTextureFormatsCallback", "callback"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_NoArgsFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3DDevice7*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by BeginScene, EndScene, and BeginStateBlock.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetDirect3DFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3DDevice7*", "self"), param("D3D_IDirect3D7**", "out_d3d")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SetRenderTargetFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DDraw_IDirectDrawSurface7*", "render_target"),
        param("uint32_t", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetRenderTargetFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DDraw_IDirectDrawSurface7**", "out_render_target"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_ClearFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "rect_count"),
        param("void*", "rects"),
        param("DWORD", "flags"),
        param("DWORD", "color"),
        param("float", "z"),
        param("DWORD", "stencil"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="Clear parameter list and scalar ABI. D3DRECT records remain opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DwordOpaqueFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "selector"),
        param("void*", "data"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by transform and light methods whose payload layouts remain opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SetViewportFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("D3D_Viewport7*", "viewport"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_OpaqueFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3DDevice7*", "self"), param("void*", "data")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by material and clip-status methods whose payload layouts remain opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SetRenderStateFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("uint32_t", "state"),
        param("uint32_t", "value"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DwordDwordOutFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "selector"),
        param("DWORD*", "out_value"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by GetRenderState and CreateStateBlock.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DwordOutFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3DDevice7*", "self"), param("DWORD*", "out_value")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by EndStateBlock and ValidateDevice.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SurfaceFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DDraw_IDirectDrawSurface7*", "surface"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="PreLoad ABI.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DrawPrimitiveFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("uint32_t", "primitive_type"),
        param("uint32_t", "vertex_format"),
        param("void*", "vertices"),
        param("uint32_t", "vertex_count"),
        param("uint32_t", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DrawIndexedPrimitiveFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "primitive_type"),
        param("DWORD", "vertex_format"),
        param("void*", "vertices"),
        param("DWORD", "vertex_count"),
        param("WORD*", "indices"),
        param("DWORD", "index_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DrawPrimitiveVBFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "primitive_type"),
        param("void*", "vertex_buffer"),
        param("DWORD", "start_vertex"),
        param("DWORD", "vertex_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="DrawPrimitiveVB parameter list. IDirect3DVertexBuffer7 remains opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DrawIndexedPrimitiveVBFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "primitive_type"),
        param("void*", "vertex_buffer"),
        param("DWORD", "start_vertex"),
        param("DWORD", "vertex_count"),
        param("WORD*", "indices"),
        param("DWORD", "index_count"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="DrawIndexedPrimitiveVB parameter list. IDirect3DVertexBuffer7 remains opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_ComputeSphereVisibilityFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("void*", "centers"),
        param("float*", "radii"),
        param("DWORD", "sphere_count"),
        param("DWORD", "flags"),
        param("DWORD*", "results"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="Scalar/pointer ABI. D3DVECTOR centers remain opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetTextureFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "stage"),
        param("DDraw_IDirectDrawSurface7**", "out_texture"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SetTextureFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("uint32_t", "stage"),
        param("DDraw_IDirectDrawSurface7*", "texture"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetTextureStageStateFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("uint32_t", "stage"),
        param("uint32_t", "state"),
        param("uint32_t*", "out_value"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_SetTextureStageStateFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("uint32_t", "stage"),
        param("uint32_t", "state"),
        param("uint32_t", "value"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_DwordFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3DDevice7*", "self"), param("DWORD", "value")],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by ApplyStateBlock, CaptureStateBlock, and DeleteStateBlock.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_LoadFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DDraw_IDirectDrawSurface7*", "destination_surface"),
        param("POINT*", "destination_point"),
        param("DDraw_IDirectDrawSurface7*", "source_surface"),
        param("RECT*", "source_rect"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_LightEnableFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "light_index"),
        param("BOOL", "enabled"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetLightEnableFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "light_index"),
        param("BOOL*", "out_enabled"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_ClipPlaneFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "plane_index"),
        param("float*", "plane_equation"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="ABI used by SetClipPlane and GetClipPlane.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3DDevice7_GetInfoFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3DDevice7*", "self"),
        param("DWORD", "info_id"),
        param("void*", "info"),
        param("DWORD", "info_size"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_EnumDevicesCallback7",
    ret="HRESULT",
    params=[
        param("char*", "device_description"),
        param("char*", "device_name"),
        param("D3D_DeviceDesc7*", "device_desc"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_QueryInterfaceFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3D7*", "self"),
        param("Win32_GUID const*", "interface_id"),
        param("void**", "out_object"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_AddRefFn",
    ret="ULONG",
    params=[param("D3D_IDirect3D7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_ReleaseFn",
    ret="ULONG",
    params=[param("D3D_IDirect3D7*", "self")],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_EnumDevicesFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3D7*", "self"),
        param("D3D_EnumDevicesCallback7", "callback"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_CreateDeviceFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3D7*", "self"),
        param("Win32_GUID const*", "device_guid"),
        param("DDraw_IDirectDrawSurface7*", "render_target"),
        param("D3D_IDirect3DDevice7**", "out_device"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_CreateVertexBufferFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3D7*", "self"),
        param("void*", "vertex_buffer_desc"),
        param("void**", "out_vertex_buffer"),
        param("DWORD", "flags"),
    ],
    calling=CallingConvention.CALLBACK,
    doc="CreateVertexBuffer parameter list. Auxiliary descriptor/interface pointees remain opaque.",
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_EnumZBufferFormatsFn",
    ret="HRESULT",
    params=[
        param("D3D_IDirect3D7*", "self"),
        param("Win32_GUID const*", "device_guid"),
        param("D3D_EnumZBufferFormatsCallback", "callback"),
        param("void*", "context"),
    ],
    calling=CallingConvention.CALLBACK,
    unstable=True,
)

stable.callback_type(
    "D3D_IDirect3D7_NoArgsFn",
    ret="HRESULT",
    params=[param("D3D_IDirect3D7*", "self")],
    calling=CallingConvention.CALLBACK,
    doc="EvictManagedTextures ABI.",
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDraw7Vtbl",
    member("DDraw_IDirectDraw7_QueryInterfaceFn", "query_interface", 0x0),
    member("void*", "unmodeled_04", 0x4),
    member("DDraw_IDirectDraw7_ReleaseFn", "release", 0x8),
    member("void*", "unmodeled_0c", 0xC),
    member("void*", "unmodeled_10", 0x10),
    member("void*", "unmodeled_14", 0x14),
    member("DDraw_IDirectDraw7_CreateSurfaceFn", "create_surface", 0x18),
    member("void*", "unmodeled_1c", 0x1C),
    member("void*", "unmodeled_20", 0x20),
    member("void*", "unmodeled_24", 0x24),
    member("void*", "unmodeled_28", 0x28),
    member("DDraw_IDirectDraw7_GetCapsFn", "get_caps", 0x2C),
    member("void*", "unmodeled_30", 0x30),
    member("void*", "unmodeled_34", 0x34),
    member("void*", "unmodeled_38", 0x38),
    member("void*", "unmodeled_3c", 0x3C),
    member("void*", "unmodeled_40", 0x40),
    member("void*", "unmodeled_44", 0x44),
    member("void*", "unmodeled_48", 0x48),
    member("DDraw_IDirectDraw7_RestoreDisplayModeFn", "restore_display_mode", 0x4C),
    member("DDraw_IDirectDraw7_SetCooperativeLevelFn", "set_cooperative_level", 0x50),
    member("DDraw_IDirectDraw7_SetDisplayModeFn", "set_display_mode", 0x54),
    member("void*", "unmodeled_58", 0x58),
    member("void*", "unmodeled_5c", 0x5C),
    member("void*", "unmodeled_60", 0x60),
    member("void*", "unmodeled_64", 0x64),
    member("DDraw_IDirectDraw7_TestCooperativeLevelFn", "test_cooperative_level", 0x68),
    member("DDraw_IDirectDraw7_GetDeviceIdentifierFn", "get_device_identifier", 0x6C),
    member("DDraw_IDirectDraw7_StartModeTestFn", "start_mode_test", 0x70),
    member("DDraw_IDirectDraw7_EvaluateModeFn", "evaluate_mode", 0x74),
    size=0x78,
    doc="Complete 0x78-byte DX7 IDirectDraw7 slot layout. Seven owned-use slots and the four DX4/DX7 tail methods are typed.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDraw7",
    member("DDraw_IDirectDraw7Vtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirectDraw7 COM interface pointer wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDrawSurface7Vtbl",
    member("DDraw_IDirectDrawSurface7_QueryInterfaceFn", "query_interface", 0x0),
    member("DDraw_IDirectDrawSurface7_AddRefFn", "add_ref", 0x4),
    member("DDraw_IDirectDrawSurface7_ReleaseFn", "release", 0x8),
    member("DDraw_IDirectDrawSurface7_AddAttachedSurfaceFn", "add_attached_surface", 0xC),
    member("DDraw_IDirectDrawSurface7_RectFn", "add_overlay_dirty_rect", 0x10),
    member("DDraw_IDirectDrawSurface7_BltFn", "blt", 0x14),
    member("DDraw_IDirectDrawSurface7_BltBatchFn", "blt_batch", 0x18),
    member("DDraw_IDirectDrawSurface7_BltFastFn", "blt_fast", 0x1C),
    member("DDraw_IDirectDrawSurface7_DeleteAttachedSurfaceFn", "delete_attached_surface", 0x20),
    member("DDraw_IDirectDrawSurface7_EnumAttachedSurfacesFn", "enum_attached_surfaces", 0x24),
    member("DDraw_IDirectDrawSurface7_EnumOverlayZOrdersFn", "enum_overlay_z_orders", 0x28),
    member("DDraw_IDirectDrawSurface7_FlipFn", "flip", 0x2C),
    member("DDraw_IDirectDrawSurface7_GetAttachedSurfaceFn", "get_attached_surface", 0x30),
    member("DDraw_IDirectDrawSurface7_DwordFn", "get_blt_status", 0x34),
    member("DDraw_IDirectDrawSurface7_GetCapsFn", "get_caps", 0x38),
    member("DDraw_IDirectDrawSurface7_OutObjectFn", "get_clipper", 0x3C),
    member("DDraw_IDirectDrawSurface7_ColorKeyFn", "get_color_key", 0x40),
    member("DDraw_IDirectDrawSurface7_GetDCFn", "get_dc", 0x44),
    member("DDraw_IDirectDrawSurface7_DwordFn", "get_flip_status", 0x48),
    member("DDraw_IDirectDrawSurface7_GetOverlayPositionFn", "get_overlay_position", 0x4C),
    member("DDraw_IDirectDrawSurface7_OutObjectFn", "get_palette", 0x50),
    member("DDraw_IDirectDrawSurface7_GetPixelFormatFn", "get_pixel_format", 0x54),
    member("DDraw_IDirectDrawSurface7_GetSurfaceDescFn", "get_surface_desc", 0x58),
    member("DDraw_IDirectDrawSurface7_InitializeFn", "initialize", 0x5C),
    member("DDraw_IDirectDrawSurface7_NoArgsFn", "is_lost", 0x60),
    member("DDraw_IDirectDrawSurface7_LockFn", "lock", 0x64),
    member("DDraw_IDirectDrawSurface7_ReleaseDCFn", "release_dc", 0x68),
    member("DDraw_IDirectDrawSurface7_NoArgsFn", "restore", 0x6C),
    member("DDraw_IDirectDrawSurface7_SetOpaqueInterfaceFn", "set_clipper", 0x70),
    member("DDraw_IDirectDrawSurface7_ColorKeyFn", "set_color_key", 0x74),
    member("DDraw_IDirectDrawSurface7_SetOverlayPositionFn", "set_overlay_position", 0x78),
    member("DDraw_IDirectDrawSurface7_SetOpaqueInterfaceFn", "set_palette", 0x7C),
    member("DDraw_IDirectDrawSurface7_UnlockFn", "unlock", 0x80),
    member("DDraw_IDirectDrawSurface7_UpdateOverlayFn", "update_overlay", 0x84),
    member("DDraw_IDirectDrawSurface7_DwordFn", "update_overlay_display", 0x88),
    member("DDraw_IDirectDrawSurface7_DwordSurfaceFn", "update_overlay_z_order", 0x8C),
    member("DDraw_IDirectDrawSurface7_OutObjectFn", "get_dd_interface", 0x90),
    member("DDraw_IDirectDrawSurface7_DwordFn", "page_lock", 0x94),
    member("DDraw_IDirectDrawSurface7_DwordFn", "page_unlock", 0x98),
    member("DDraw_IDirectDrawSurface7_SetSurfaceDescFn", "set_surface_desc", 0x9C),
    member("DDraw_IDirectDrawSurface7_SetPrivateDataFn", "set_private_data", 0xA0),
    member("DDraw_IDirectDrawSurface7_GetPrivateDataFn", "get_private_data", 0xA4),
    member("DDraw_IDirectDrawSurface7_GuidFn", "free_private_data", 0xA8),
    member("DDraw_IDirectDrawSurface7_DwordOutFn", "get_uniqueness_value", 0xAC),
    member("DDraw_IDirectDrawSurface7_NoArgsFn", "change_uniqueness_value", 0xB0),
    member("DDraw_IDirectDrawSurface7_DwordFn", "set_priority", 0xB4),
    member("DDraw_IDirectDrawSurface7_DwordOutFn", "get_priority", 0xB8),
    member("DDraw_IDirectDrawSurface7_DwordFn", "set_lod", 0xBC),
    member("DDraw_IDirectDrawSurface7_DwordOutFn", "get_lod", 0xC0),
    size=0xC4,
    doc="Forty-nine-slot DX7 IDirectDrawSurface7 vtable through GetLOD. Each slot uses the CALLBACK calling convention and its listed parameter count.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "DDraw_IDirectDrawSurface7",
    member("DDraw_IDirectDrawSurface7Vtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirectDrawSurface7 COM interface pointer wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "D3D_IDirect3DDevice7Vtbl",
    member("D3D_IDirect3DDevice7_QueryInterfaceFn", "query_interface", 0x0),
    member("D3D_IDirect3DDevice7_AddRefFn", "add_ref", 0x4),
    member("D3D_IDirect3DDevice7_ReleaseFn", "release", 0x8),
    member("D3D_IDirect3DDevice7_GetCapsFn", "get_caps", 0xC),
    member("D3D_IDirect3DDevice7_EnumTextureFormatsFn", "enum_texture_formats", 0x10),
    member("D3D_IDirect3DDevice7_NoArgsFn", "begin_scene", 0x14),
    member("D3D_IDirect3DDevice7_NoArgsFn", "end_scene", 0x18),
    member("D3D_IDirect3DDevice7_GetDirect3DFn", "get_direct3d", 0x1C),
    member("D3D_IDirect3DDevice7_SetRenderTargetFn", "set_render_target", 0x20),
    member("D3D_IDirect3DDevice7_GetRenderTargetFn", "get_render_target", 0x24),
    member("D3D_IDirect3DDevice7_ClearFn", "clear", 0x28),
    member("D3D_IDirect3DDevice7_DwordOpaqueFn", "set_transform", 0x2C),
    member("D3D_IDirect3DDevice7_DwordOpaqueFn", "get_transform", 0x30),
    member("D3D_IDirect3DDevice7_SetViewportFn", "set_viewport", 0x34),
    member("D3D_IDirect3DDevice7_DwordOpaqueFn", "multiply_transform", 0x38),
    member("D3D_IDirect3DDevice7_SetViewportFn", "get_viewport", 0x3C),
    member("D3D_IDirect3DDevice7_OpaqueFn", "set_material", 0x40),
    member("D3D_IDirect3DDevice7_OpaqueFn", "get_material", 0x44),
    member("D3D_IDirect3DDevice7_DwordOpaqueFn", "set_light", 0x48),
    member("D3D_IDirect3DDevice7_DwordOpaqueFn", "get_light", 0x4C),
    member("D3D_IDirect3DDevice7_SetRenderStateFn", "set_render_state", 0x50),
    member("D3D_IDirect3DDevice7_DwordDwordOutFn", "get_render_state", 0x54),
    member("D3D_IDirect3DDevice7_NoArgsFn", "begin_state_block", 0x58),
    member("D3D_IDirect3DDevice7_DwordOutFn", "end_state_block", 0x5C),
    member("D3D_IDirect3DDevice7_SurfaceFn", "preload", 0x60),
    member("D3D_IDirect3DDevice7_DrawPrimitiveFn", "draw_primitive", 0x64),
    member("D3D_IDirect3DDevice7_DrawIndexedPrimitiveFn", "draw_indexed_primitive", 0x68),
    member("D3D_IDirect3DDevice7_OpaqueFn", "set_clip_status", 0x6C),
    member("D3D_IDirect3DDevice7_OpaqueFn", "get_clip_status", 0x70),
    member("D3D_IDirect3DDevice7_DrawPrimitiveFn", "draw_primitive_strided", 0x74),
    member("D3D_IDirect3DDevice7_DrawIndexedPrimitiveFn", "draw_indexed_primitive_strided", 0x78),
    member("D3D_IDirect3DDevice7_DrawPrimitiveVBFn", "draw_primitive_vb", 0x7C),
    member("D3D_IDirect3DDevice7_DrawIndexedPrimitiveVBFn", "draw_indexed_primitive_vb", 0x80),
    member("D3D_IDirect3DDevice7_ComputeSphereVisibilityFn", "compute_sphere_visibility", 0x84),
    member("D3D_IDirect3DDevice7_GetTextureFn", "get_texture", 0x88),
    member("D3D_IDirect3DDevice7_SetTextureFn", "set_texture", 0x8C),
    member("D3D_IDirect3DDevice7_GetTextureStageStateFn", "get_texture_stage_state", 0x90),
    member("D3D_IDirect3DDevice7_SetTextureStageStateFn", "set_texture_stage_state", 0x94),
    member("D3D_IDirect3DDevice7_DwordOutFn", "validate_device", 0x98),
    member("D3D_IDirect3DDevice7_DwordFn", "apply_state_block", 0x9C),
    member("D3D_IDirect3DDevice7_DwordFn", "capture_state_block", 0xA0),
    member("D3D_IDirect3DDevice7_DwordFn", "delete_state_block", 0xA4),
    member("D3D_IDirect3DDevice7_DwordDwordOutFn", "create_state_block", 0xA8),
    member("D3D_IDirect3DDevice7_LoadFn", "load", 0xAC),
    member("D3D_IDirect3DDevice7_LightEnableFn", "light_enable", 0xB0),
    member("D3D_IDirect3DDevice7_GetLightEnableFn", "get_light_enable", 0xB4),
    member("D3D_IDirect3DDevice7_ClipPlaneFn", "set_clip_plane", 0xB8),
    member("D3D_IDirect3DDevice7_ClipPlaneFn", "get_clip_plane", 0xBC),
    member("D3D_IDirect3DDevice7_GetInfoFn", "get_info", 0xC0),
    size=0xC4,
    doc="Forty-nine-slot DX7 IDirect3DDevice7 vtable through GetInfo. Each slot uses the CALLBACK calling convention and its listed parameter count.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "D3D_IDirect3DDevice7",
    member("D3D_IDirect3DDevice7Vtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirect3DDevice7 COM interface pointer wrapper.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "D3D_IDirect3D7Vtbl",
    member("D3D_IDirect3D7_QueryInterfaceFn", "query_interface", 0x0),
    member("D3D_IDirect3D7_AddRefFn", "add_ref", 0x4),
    member("D3D_IDirect3D7_ReleaseFn", "release", 0x8),
    member("D3D_IDirect3D7_EnumDevicesFn", "enum_devices", 0xC),
    member("D3D_IDirect3D7_CreateDeviceFn", "create_device", 0x10),
    member("D3D_IDirect3D7_CreateVertexBufferFn", "create_vertex_buffer", 0x14),
    member("D3D_IDirect3D7_EnumZBufferFormatsFn", "enum_z_buffer_formats", 0x18),
    member("D3D_IDirect3D7_NoArgsFn", "evict_managed_textures", 0x1C),
    size=0x20,
    doc="Eight-slot DX7 IDirect3D7 vtable through EvictManagedTextures. Each slot uses the CALLBACK calling convention and its listed parameter count.",
    incomplete=False,
    unstable=True,
)

stable.struct(
    "D3D_IDirect3D7",
    member("D3D_IDirect3D7Vtbl*", "vtable", 0x0),
    size=0x4,
    doc="IDirect3D7 COM interface pointer wrapper.",
    incomplete=False,
    unstable=True,
)

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
    doc="DirectDrawEnumerateExA callback signature used when enumerating DirectDraw devices.",
    stable=True,
)

stable.callback_type(
    "D3D_DeviceAcceptCallback",
    ret="HRESULT",
    params=[
        param("DDCAPS*", "driver_caps"),
        param("D3D_DeviceDesc7*", "device_desc"),
    ],
    calling=CallingConvention.CDECL,
    doc="Cdecl DirectX 7 device-policy callback ABI: HRESULT (*)(DDCAPS*, D3DDEVICEDESC7*).",
)

stable.callback_type(
    "Actor_BehaviorCallback",
    ret="void",
    params=[param("Actor_State*", "actor")],
    calling=CallingConvention.CDECL,
    doc="Semantic void callback used by the nine-entry actor-phase table and compatible preprocess slots. All indexed dispatchers ignore EAX.",
)

stable.callback_type(
    "Graphics_Render_ListCallback",
    ret="void",
    params=[param("Graphics_ListState*", "render_list_state")],
    calling=CallingConvention.CDECL,
    doc="Render-list callback invocation contract. The installed Graphics_ClearBackground target ignores the supplied state pointer.",
)

stable.callback_type(
    "Graphics_RenderPolygonCallbackType",
    ret="void",
    params=[
        param(
            "void*",
            "render_owner",
            doc=(
                "Actor_State or static Scene_Node-like owner exposing render fields at common "
                "offsets +0x64, +0x70, and +0x88."
            ),
        ),
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "polygon_count"),
    ],
    calling=CallingConvention.CDECL,
    doc="Cdecl polygon-batch callback. Installed targets are void.",
)

stable.callback_type(
    "Script_CommandCallback",
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    calling=CallingConvention.CDECL,
    doc=(
        "Script_OpRunWithActor uses this callback type for opcode handlers in the native script dispatch table."
    ),
)


stable.callback_type(
    "Sort_CompareCallback",
    ret="int32_t",
    params=[param("void const*", "lhs"), param("void const*", "rhs")],
    calling=CallingConvention.CDECL,
    doc="CRT qsort/shortsort comparator ABI. Returns negative, zero, or positive for lhs vs rhs ordering.",
)

stable.callback_type(
    "Tree_MapCompareCallback",
    ret="int32_t",
    params=[param("void*", "lhs_payload"), param("void*", "rhs_payload")],
    calling=CallingConvention.CDECL,
    doc="TreeMap payload comparator stored in the tree header and called with two node payload pointers.",
)

stable.struct(
    "Tree_Map",
    member("Tree_MapNode*", "min_root", 0x0),
    member("int32_t", "node_alloc_size", 0x4),
    member("Tree_MapCompareCallback", "compare", 0x8),
    size=0xC,
)

stable.callback_type(
    "Powerup_UpdateCallback",
    ret="void",
    params=[param("Actor_State*", "actor")],
    calling=CallingConvention.CDECL,
    doc="Actor-phase slot-8 callback initialized by Powerup_InitializeSystem. Both indexed phase dispatchers ignore EAX.",
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
            doc="Reserved argument slot used as zero at the powerup collision dispatch site.",
        ),
        param("int32_t", "collision_result"),
    ],
    calling=CallingConvention.CDECL,
    doc="Four-argument powerup collision filter callback initialized by Powerup_InitializeSystem. Powerup_CollisionFilter consumes the fourth collision_result argument and ignores the zero third slot.",
    unstable=True,
)

stable.callback_type(
    "ActorCollisionProbeCallback",
    ret="int32_t",
    params=[
        param("void*", "subject"),
        param("void*", "other_object"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    calling=CallingConvention.CDECL,
)

stable.callback_type(
    "Component_ProjectileLogicCallback",
    ret="void",
    params=[param("Component_Instance*", "comp")],
    calling=CallingConvention.CDECL,
    doc="Semantic void projectile preprocess callback. The indexed dispatcher ignores EAX.",
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
    doc="Engine-managed scalar actor collision processing callback slot initialized to Physics_ProcessActorCollision and aliased by collision_state_handler_table slot 2.",
)


stable.enum(
    "Actor_PropID",
    enum_value("PROP_RENDERY", 0),
    enum_value("PROP_SPEED", 1),
    enum_value("PROP_ROTATION", 2),
    enum_value("PROP_PARENT", 3),
    enum_value("PROP_PUSHXZ", 4),
    enum_value("PROP_RESPONSEXZ", 5),
    enum_value("PROP_LIVESPEED", 6),
    enum_value("PROP_CAMERAXZ", 7),
    enum_value("PROP_COLLRADIUS", 8),
    enum_value("PROP_COLLHEIGHT", 9),
    enum_value("PROP_TRACEMODE0", 100),
    enum_value("PROP_TRACEMODE1", 101),
    enum_value("PROP_TRACEMODE2", 102),
    enum_value("PROP_TRACEMODE3", 103),
)

stable.enum("Camera_TransitionMode", enum_value("CAMERA_SNAP", 0))

stable.sig("DDraw_ObjectAnchor", "A1 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81 EC 9C 00 00 00")

stable.sig("Game_InitializedAnchor", "89 35 ?? ?? ?? ?? C6 44 24 ?? 10")

stable.sig(
    "Input_GamepadButtonFlags", "8B 15 ?? ?? ?? ?? 8B 06 0B C2 89 06 81 FB BC 02 00 00"
)

stable.sig("Input_JoystickAvailableAnchor", "A0 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? A1")

stable.sig("Input_KeyboardMappingButtons", "8B 15 ?? ?? ?? ?? 50 56")

stable.sig("Input_KeyboardMappingKeys", "A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 8D 14 8D")

stable.sig("Window_MainHandleAnchor", "A1 ?? ?? ?? ?? 83 C4 08 6A 03")

stable.sig("Window_RunWinMain_SecondaryWindowHandleAnchor", "A3 ?? ?? ?? ?? FF D7")

stable.sig("Input_MappingCount", "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 50")

stable.sig("Graphics_RenderingEnabledAnchor", "39 35 ?? ?? ?? ?? 74 ?? E8")

stable.sig("Window_ShouldQuitAnchor", "39 35 ?? ?? ?? ?? 75 ?? 39 35")

stable.sig("File_DirectoryAnchor", "68 ?? ?? ?? ?? 68 04 01 00 00 FF 15")

stable.sig("Audio_DigitalDriverAnchor", "A1 ?? ?? ?? ?? 6A 7F 50 FF 15")

stable.sig("Video_MovieFileNamesAndPathPrefix", "8B 04 B5 ?? ?? ?? ?? 50 68")

stable.sig(
    "Title_ResourceCleanupBundle",
    "6A 01 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ??",
)

stable.sig("PKG_FindAndLoadGamePKFile", "81 EC 10 01 00 00 57 ??")

stable.sig("Game_InitializeEngine", "E8 ?? ?? ?? ?? 85 C0 75 ?? 32 C0")

stable.sig("Graphics_InitializeSubsystem", "E8 ?? ?? ?? ?? 8B 44 24 ?? 8B 4C 24 ?? 50")

stable.sig("D3D_InitializeCapabilities", "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 A3")

stable.sig(
    "Display_SetMode_Anchor",
    "A1 ?? ?? ?? ?? 56 57 8B 7C 24",
    doc="Signature anchor for Display_SetMode, the HWND display-mode initializer.",
    stable=True,
)

stable.sig("Game_InitializeSystems", "E8 ?? ?? ?? ?? 8D 54 24 ?? 56")

stable.sig("Graphics_RenderFrame", "51 53 E8 ?? ?? ?? ?? A1")

stable.sig("Input_IsKeyPressed", "A1 ?? ?? ?? ?? 33 C9 85 C0 53")

stable.sig("Input_ResetInputAndState", "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 25 DF F4 FF FF")

stable.sig("Graphics_TakeScreenshot", "81 EC 04 01 00 00 56 ??")

stable.sig("Mem_Malloc", "FF 35 ?? ?? ?? ?? FF 74 24")

stable.sig("Audio_ShutdownSystem", "A1 ?? ?? ?? ?? 85 C0 74 ?? 53 8B 1D")

stable.sig(
    "Actor_CheckSavedActiveActorCameraDistance",
    "8B 44 24 04 85 C0 75 ?? 33 C0 C3 8B 15 ??",
    required=Required.EN,
)

stable.sig(
    "Scene_RenderFrame",
    "55 8B EC 83 EC 44 F6 05 ?? ?? ?? ?? 08 0F 85 ??",
)

stable.sig(
    "Timer_GetElapsedTickCountThunk",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 55",
)

stable.sig(
    "Audio_PlayLevelSoundIndexAtPositionAnchor",
    "8B 0D ?? ?? ?? ?? 56 85",
)

stable.sig("Entity_SetActorProperty", "00 00 83 F9 09 0F 87 ??")

stable.sig("Graphics_AdjustLevelScale", "A1 ?? ?? ?? ?? 85 C0 7C")

stable.fn(
    "Actor_CheckSavedActiveActorCameraDistance",
    "8B 44 24 04 85 C0 75 ?? 33 C0 C3 8B 15 ??",
    required=Required.EN,
    hook=0x6,
    ret="BOOL",
    params=[param("Actor_State*", "actor")],
    doc="Returns TRUE when actor is non-null and the native pause/menu camera-distance condition allows processing. The distance scalar is computed from the saved active-actor world_render_pos snapshot captured by Script_OpPauseToggle and the current Graphics_ListState eye position.",
    stable=True,
)

stable.fn(
    "Scene_InitNodeState",
    "56 65 74 ?? 48 0F 84 ??",
    match=-0x12,
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc=(
        "Resets per-node traversal and render state before ordinary traversal or activation of a "
        "newly selected type-8 transition node."
    ),
    stable=True,
)

stable.fn(
    "Scene_TraverseNodeTree",
    "7C ?? 1C 85 F6 0F 84 ??",
    match=-0x18,
    hook=0x8,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Actor_State*", "actor"),
        param("uint32_t", "traversal_flags"),
    ],
    doc="Traverses the scene-node tree for rendering and visibility side effects, dispatching node_type 1 through 7 through scene_node_type_dispatch_table. It produces transient Actor behavior bit 0x8 for actor-level trail sampling.",
    stable=True,
)

stable.fn(
    "Scene_SubmitSpriteNode",
    "8B 44 24 04 8D 50 6C 8D 48 2C 52 50 89 0D ??",
    hook=0x7,
    public=False,
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc="Publishes the node world transform and submits its type-7 Graphics_Render_SpriteNodeData payload via Graphics_RenderMeshNode.",
    stable=True,
)

stable.fn(
    "Trail_UpdateAndRenderActorTrails",
    "00 6A 00 6A 00 56 E8 ??",
    match=-0x29,
    hook=0x6,
    public=False,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Updates each actor trail once, renders active histories outside frame transition 0x200000, then clears processed flags.",
    stable=True,
)

stable.fn(
    "Trail_UpdateEffect",
    "0B 56 F6 C1 02 0F 85 ??",
    match=-0x22,
    hook=0x8,
    public=False,
    ret="char",
    params=[
        param("Component_TrailObject*", "trail"),
        param(
            "Actor_State*",
            "actor",
            doc="Actor supplying scene vertices for active samples. Null is valid only for inactive or bit-1-suppressed updates.",
        ),
        param(
            "Graphics_PolygonRenderRef*",
            "polygon_ref",
            doc="Polygon reference whose vertex indices 0 and 3 select the trail endpoints.",
        ),
        param("char", "active"),
    ],
    doc="Advances the trail ring and resets expiry_countdown for active samples. Returns zero only when inactive decay expires the final history.",
    stable=True,
)

stable.fn(
    "Scene_RenderSubMesh",
    "8B 5D 10 83 C0 2C A3 ??",
    match=-0xA,
    hook=0x6,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("Scene_Node*", "node"),
        param("Submesh_RenderSpan*", "span"),
    ],
    doc="Submits one actor submesh span through the active polygon-render callback using the scene-node world transform. Trail flag bit 0 is a template-preserved per-trail force-active-sample override of actor behavior bit 0x8.",
    stable=True,
)

stable.fn(
    "Scene_RenderFrame",
    "55 8B EC 83 EC 44 F6 05 ?? ?? ?? ?? 08 0F 85 ??",
    hook=0x6,
    ret="void",
    params=[],
)

stable.fn(
    "Scene_RenderStaticGeometry",
    "A1 ?? ?? ?? ?? 56 57 C7",
    match=-0x23,
    hook=0x6,
    ret="void",
    params=[],
    doc="Walks visible static-scene records and submits their geometry. Far-distance flag 0x40000000 selects Graphics_RenderSceneGeometryWrapper.",
    stable=True,
)

stable.fn(
    "Graphics_RenderSpriteObjectNode",
    "50 8D 46 40 51 50 E8 ??",
    match=-0x18,
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc=(
        "Applies visibility and frustum gates to a sprite-object node, processes its mesh commands, "
        "and submits its payload for rendering."
    ),
    stable=True,
)

stable.fn(
    "Actor_UpdateList",
    "56 57 E8 ?? ?? ?? ?? BF",
    hook=0x7,
    ret="void",
    params=[],
    doc=(
        "Processes actor collisions and rendering for Actor_CollisionListHead, including deferred "
        "nodes and safe destruction of lifecycle-0x20 actors."
    ),
    stable=True,
)

stable.fn(
    "Actor_ProcessRendering",
    "53 56 8B 74 24 0C 57 56 89 35 ??",
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Renders one actor. Publishes actor in Actor_ProcessRendering_CurrentRenderActor for render helpers, "
        "updates animation/visibility, renders the actor scene-node tree, processes trail/mesh command "
        "flags, and clears render-scoped globals before return."
    ),
    stable=True,
)

stable.fn(
    "Scene_RenderNodeTree",
    "55 8B EC 81 EC 90 00 00 00 A1 ??",
    hook=0x9,
    ret="void",
    params=[],
    doc="Renders the active scene node tree and runs render and finalizer side effects. Trail flag bit 0 is a template-preserved per-trail force-active-sample override of actor behavior bit 0x8.",
)

stable.fn(
    "Actor_UpdateAnimationAndVisibility",
    "F6 D3 80 E3 01 0F 84 ??",
    match=-0x14,
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Updates actor animation and render flags, applies distance and visibility gates, and "
        "prepares scene-node geometry for submission."
    ),
    stable=True,
)

stable.fn(
    "Powerup_UpdateLiveActorList",
    "56 8B 35 ?? ?? ?? ?? 57 BF ?? ?? ?? ?? 85 F6 74",
    hook=0x7,
    ret="void",
    params=[],
    doc=(
        "Walks Powerup_LiveActorListHead, updates live powerup Actor_State nodes through physics/"
        "render paths, and removes collected or expired powerup actors using the powerup "
        "lifecycle policy."
    ),
)

stable.fn(
    "Actor_UpdateProjectileList",
    "56 8B 35 ?? ?? ?? ?? 57 BF ?? ?? ?? ?? 85 F6 0F",
    hook=0x7,
    ret="void",
    params=[],
    doc=(
        "Walks Projectile_LiveActorListHead, updating live projectile actors and rendering them. "
        "Lifecycle 0x20 entries are detached and have positional sound stopped, then remain while "
        "referenced or are unlinked and destroyed."
    ),
    stable=True,
)

stable.fn(
    "Audio_ResolveActorSoundIndex",
    "8B 44 24 04 85 C0 74 ?? 66 8B 40 0C 66 85 C0 7C ?? 8B 4C 24 08 0F BF C0 8D 44 08 9C C3 83 C8 FF C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 0D ??",
    hook=0x6,
    ret="int32_t",
    params=[
        param("PKG_ActorTemplate const*", "actor_template"),
        param("int32_t", "encoded_sound_selector"),
    ],
    doc="Resolves selectors 100 through 199 through the signed sound-index base at actor_template +0x0C. Returns -1 for no usable base.",
    stable=True,
)

stable.fn(
    "Nav_AddCommand",
    "8B 0D ?? ?? ?? ?? 8B 54 24 04 81",
    hook=0x6,
    ret="void",
    params=[
        param("uint8_t", "command_type"),
        param("int16_t", "target_selector_a"),
        param("int16_t", "target_selector_b"),
        param("int32_t", "speed"),
        param("Math_Vec3I32 const*", "position"),
    ],
    doc=(
        "Copies one command and position into the staging buffer for the next pass. The fixed "
        "100-command array is indexed without a local capacity check."
    ),
    stable=True,
)

stable.fn(
    "Navigation_HandleDamageResponse",
    "53 8B 5C 24 0C 55 56 57 53 E8 ??",
    ret="char",
    params=[
        param("Actor_State*", "actor"),
        param("Component_Instance*", "damage_component"),
    ],
    doc="Queues navigation response work for an eligible damage component after owner, team, and entity-state checks. Returns 1 when the response is accepted.",
    stable=True,
)

stable.fn(
    "Audio_PlayLevelSoundIndexAtPosition",
    "8B 0D ?? ?? ?? ?? 56 85",
    hook=0x6,
    public=False,
    ret="void",
    params=[
        param("int32_t", "sound_index"),
        param("Math_Vec3I32*", "position"),
    ],
    doc="Indexes sound_list without checking sound_def_count. Loop-bit definitions deduplicate by matching sound-definition and position pointers.",
    stable=True,
)

stable.fn(
    "Audio_StopOrUpdateSoundsAtPosition",
    "A1 ?? ?? ?? ?? 53 55 56 85",
    public=False,
    ret="void",
    params=[
        param("int32_t", "sound_index_or_mode"),
        param("Math_Vec3I32*", "position"),
        param("char", "looping_only"),
    ],
    doc="Matches active sounds by definition and position-pointer identity. Nonnegative indices are unchecked.",
    stable=True,
)

stable.fn(
    "Script_OpUpdateCollisionAndSetFlag",
    "83 C1 02 89 08 8B 35 ??",
    match=-0x13,
    hook=0x7,
    public=False,
    ret="void",
    params=[param("Entity_State*", "entity"), param("uint8_t**", "script_cursor_inout")],
    doc="Opcode 0x15 callback. Updates the selected collision record, restores path_result, then sets the requested entity flag bit.",
    stable=True,
)

stable.fn(
    "Script_OpPollSignal",
    "66 89 54 24 10 50 E8 ??",
    match=-0x5A,
    public=False,
    ret="void",
    params=[param("Entity_State*", "entity"), param("uint8_t**", "script_cursor_inout")],
    doc="Opcode 0x19 callback. Polls a queued signal.",
    stable=True,
)

stable.fn(
    "Input_CheckButtonState",
    "FA 00 00 00 20 0F 84 ??",
    match=-0x27,
    hook=0x8,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "state_index",
            doc="Input state slot to query. Invalid or empty slots fall back to slot zero.",
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
    doc="Evaluates a button or axis control code against sampled input state. Button queries return 0/100.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Actor_SetFacingTarget",
    "FF FF 89 45 10 0F 84 ??",
    match=-0x4D,
    hook=0x6,
    ret="bool",
    params=[
        param("Entity_State*", "entity"),
        param(
            "int32_t",
            "target_selector",
            doc="Target selector. 1-based entity index or sentinel 0x8008/0x800A, not an actor id.",
        ),
        param("int32_t", "angle_degrees_q12"),
    ],
    doc="Sets the runtime actor facing target from a script selector and Q12 degree offset. Returns whether the previous facing already matched the target.",
    stable=True,
)

stable.fn(
    "Actor_ResetVelocityAndMoveToTarget",
    "8B 4C 24 10 51 52 50 E8 ??",
    match=-0x3D,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Entity whose runtime actor is stopped and whose path target supplies the destination.",
        ),
        param(
            "int16_t",
            "target_selector",
            doc="Target selector forwarded to Actor_MoveToTarget.",
        ),
        param(
            "int32_t",
            "transition_speed",
            doc="Transition speed forwarded to Actor_MoveToTarget.",
        ),
        param(
            "int32_t",
            "duration_q12",
            doc="Script duration value scaled by 30 and shifted from Q12 before the move call.",
        ),
    ],
    doc="Stops the entity's runtime actor and starts movement toward its path_target. Converts duration_q12 from Q12 seconds to 30 Hz ticks with (duration_q12 * 30) >> 12.",
    stable=True,
)

stable.fn(
    "Actor_MoveToTarget",
    "00 84 C0 7E ?? 56 E8 ??",
    match=-0x17,
    hook=0x6,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("Math_Vec3I32 const*", "target_pos"),
        param("int16_t", "target_selector"),
        param("int32_t", "transition_speed"),
        param("int32_t", "duration_frames"),
    ],
    doc="Reads nullable target_pos as borrowed const input, releases the prior attachment, and stores the low 16 bits of duration_frames at PKG_ActorRecord +0x18C and target_selector at +0x18E. Zero duration snaps immediately.",
    stable=True,
)

stable.fn(
    "Actor_ReleaseAttachment",
    "00 00 8B 48 24 51 E8 ??",
    match=-0x9,
    ret="void",
    params=[param("PKG_ActorRecord*", "record")],
    doc=(
        "Releases the record attachment at +0x1A0, updates its target and reference count, and clears "
        "the active transitioning type-0 camera when owned by the current entity. Also stores the "
        "negated collision-component lifetime byte."
    ),
    stable=True,
)

stable.fn(
    "Actor_SnapToPosition",
    "01 00 00 3B D0 0F 84 ??",
    match=-0x36,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Completes a pending actor movement transition by applying recorded target and velocity, "
        "rebuilding collision/model state, resolving ground and parent collision, updating the linked "
        "actor, and clearing transition bookkeeping. Also applies and finalizes the facing target."
    ),
    stable=True,
)

stable.fn(
    "Actor_ApplyEntitySlotFlags",
    "8B 44 24 04 56 85 C0 0F 84 ??",
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Applies current-level entity-slot state to an actor's movement, collision, and behavior "
        "flags after movement, property, or spawn-state changes."
    ),
    stable=True,
)

stable.fn(
    "Actor_StartTransition",
    "C6 00 00 00 89 88 E8 ??",
    match=-0x3C,
    hook=0xA,
    ret="void",
    params=[param("Actor_State*", "actor"), param("PKG_ActorRecord*", "record")],
    doc=(
        "Marks an actor active and in transition, clears movement and collision state, optionally "
        "clears per-component transition data, and sets entity flag 0x2000 for the record target."
    ),
    stable=True,
)

stable.fn(
    "Script_OpSetPlayerState",
    "00 88 4F 0C 7D ?? E8 ??",
    match=-0x5A,
    public=False,
    ret="void",
    params=[param("Entity_State*", "entity"), param("uint8_t**", "script_cursor_inout")],
    doc="Opcode 0x11 callback. Selects player-control state and sets Entity bit 0x4 as the current pass's active-worklist request.",
    stable=True,
)

stable.fn(
    "Actor_TracePath",
    "D2 3B F2 57 75 ?? BE ??",
    match=-0xC,
    hook=0x6,
    ret="int32_t",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Entity whose actor path-trace fields are read and updated.",
        ),
        param(
            "int32_t",
            "target_selector",
            doc="Signed script/actor selector. Positive values address actor slots and negative sentinel values select special path targets.",
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
            doc="Bitfield controlling recursive trace/result modes. Known values include 0, 0x20, and 0x300.",
        ),
    ],
    doc="Traces and selects an entity path target. Returns the selected path or actor index, or zero when none is available.",
)

stable.fn(
    "Entity_IsInActiveList",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 48",
    hook=0xA,
    ret="BOOL",
    params=[
        param(
            "int32_t",
            "entity_index_one_based",
            doc="1-based entity index checked against the current level entity table.",
        ),
    ],
    doc="Returns TRUE when entity_index_one_based appears before active_count in active_entities. The four slot array trusts active_count to stay at or below four.",
)

stable.fn(
    "Actor_CopyEntityDefaultsToRecord",
    "68 01 00 00 20 0F 85 ??",
    match=-0xD,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Refreshes the actor record from its owning entity defaults while preserving selected "
        "runtime fields and active position overrides."
    ),
    stable=True,
)

stable.fn(
    "Entity_SetActorProperty",
    "00 00 83 F9 09 0F 87 ??",
    match=-0x33,
    ret="void",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Entity-slot input. The routine bridges through the active actor/record link before writing actor property/default fields.",
        ),
        param("Actor_PropID", "prop_id"),
        param("int32_t", "value"),
    ],
    doc="Applies a property update through entity and active-actor state. Callers ignore the residual register value.",
    unstable=True,
)

stable.fn(
    "Timer_GetElapsedTickCountThunk",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 55",
    ret="int32_t",
    params=[],
    doc="Tail-call thunk that returns Timer_GetElapsedTickCount().",
    stable=True,
)

stable.fn(
    "Entity_SpawnActor",
    "8D 73 58 85 C0 0F 85 ??",
    match=-0xF,
    hook=0x6,
    ret="Actor_State*",
    params=[param("Entity_State*", "source_entity")],
)

stable.fn(
    "Entity_EnableChildGroundOrientation",
    "A4 00 00 00 7D ?? 8B ??",
    match=-0x43,
    hook=0xA,
    public=False,
    ret="void",
    params=[param("Entity_State*", "entity")],
    doc="Enables child-derived ground orientation after clone initialization clears parent behavior bit 0x4000. Ground-orientation update computes and arms child quaternion state.",
    stable=True,
)

stable.fn(
    "Entity_DestroyActor",
    "00 00 53 50 6A FE E8 ??",
    match=-0x17,
    hook=0x6,
    ret="uint32_t",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Entity slot whose active_actor is detached and cleared.",
        ),
        param(
            "uint32_t",
            "restore_defaults",
            doc="Nonzero restores default collision radius/height and default flags with bit 0x800 set after teardown.",
        ),
    ],
    doc="Tears down an entity slot's active actor state. A collision-list-unlinked actor (flag 0x4000) is reinserted before teardown.",
)

stable.fn(
    "Graphics_UpdateScreenTransitionEffects",
    "55 8B EC 83 EC 10 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[],
    doc=(
        "Advances the 10-frame Q12 fade counter, draws curtain overlays, and handles pause-edge audio "
        "fade and pause. Level transitions use eased progress to draw either a full-screen fade or a "
        "radial iris assembled from border rectangles and four mirrored sprites."
    ),
    stable=True,
)

stable.fn(
    "Level_TriggerTransition",
    "8B 44 24 08 8B 0D ?? ?? ?? ?? 53",
    hook=0xA,
    ret="BOOL",
    params=[
        param("int32_t", "target_level_index"),
        param("int32_t", "transition_mode"),
        param(
            "Level_TransitionTimingData const*",
            "transition_timing",
            doc="Borrowed const record. Nullable only when negative transition_mode resumes the current transition.",
        ),
    ],
    doc="Selects or updates linked type-8 transitions. Returns TRUE while transition work remains pending.",
    stable=True,
)

stable.fn(
    "Audio_TriggerMusicTransition",
    "55 8B EC 8B 45 0C 53 85 C0 56 7D ?? 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[
        param("int32_t", "track_index"),
        param("int32_t", "transition_mode"),
        param(
            "Level_TransitionTimingData const*",
            "transition_timing",
            doc="Borrowed const record. Nullable only when negative transition_mode resumes the current music transition.",
        ),
        param("int32_t", "volume_percent"),
    ],
    doc="Selects or updates a music transition. A new track indexes themes directly without checking theme_count or the five-entry array bound, then maps the const timing record to unclamped Q12 fade-out, silent-hold, and fade-in durations.",
    stable=True,
)

stable.fn(
    "Script_OpPauseToggle",
    "A1 ?? ?? ?? ?? 53 33 DB 55",
    public=False,
    ret="void",
    params=[param("Entity_State*", "entity"), param("uint8_t**", "script_cursor_inout")],
    doc="Opcode 0x23 callback. Pause sets Entity flag 0x2000 for script-paused state and sets flag 0x4000 when unlinking a live actor from the collision list.",
    stable=True,
)

stable.fn(
    "Entity_UpdateVisibilityAndSpawn",
    "40 01 00 00 51 50 E8 ??",
    match=-0x4B,
    hook=0x6,
    public=False,
    ret="BOOL",
    params=[
        param("Entity_State*", "entity"),
        param(
            "Entity_State*",
            "reactivation_requester_or_null",
            doc="Non-null nested-script requester that forces collision-list relinking for the target.",
        ),
    ],
    doc="Updates visibility/spawn state. A non-null reactivation requester bypasses the frustum test for a collision-list-unlinked target, clears bit 0x4000, and relinks its actor.",
    stable=True,
)

stable.fn(
    "Camera_UpdateFollow",
    "55 8B EC 83 EC 64 53 56 57 E8 ??",
    hook=0x6,
    public=False,
    ret="void",
    params=[param("Camera_Runtime*", "camera")],
    doc="Updates the camera, promotes the staging entity/navigation buffer, selects the alternate buffer, and clears its counts before producing next-pass work. Entity bit 0x4 appends to the fixed four-slot active list without a local bounds check.",
    stable=True,
)

stable.fn(
    "Camera_InterpolateTransition",
    "55 8B EC 83 EC 10 A1 ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "Camera_TransitionState*",
            "transition_state",
            doc="Live camera transition state whose embedded pose is eased toward target_pose.",
        ),
        param(
            "Camera_Pose*",
            "target_pose",
            doc="Target/scratch pose built by Camera_UpdateFollow or Camera_UpdateFromDefinition.",
        ),
        param(
            "int32_t",
            "duration_frames",
            doc="Total transition frame count. Known callers pass 30, a remaining scripted span, or camera definition duration.",
        ),
        param(
            "int32_t",
            "recompute_eye_from_angles",
            doc="Nonzero interpolates the first two angle words and recomputes eye position from target distance.",
        ),
    ],
    doc="Eases an active camera transition from transition_state.pose toward target_pose using camera_transition_frame_counter as the remaining countdown. It interpolates fov, target position, and orbit yaw with a Q12 ease curve.",
    stable=True,
)

stable.fn(
    "Camera_CalculatePosition",
    "55 8B EC 56 8B 75 08 66 8B 06 50 E8 ??",
    hook=0x7,
    ret="void",
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
        "wrapped angle words, and target_distance using Math_SinCosFP12."
    ),
    stable=True,
)

stable.fn(
    "Camera_UpdateFromDefinition",
    "55 8B EC 83 EC 20 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[
        param("Camera_Runtime*", "camera"),
        param("Entity_State*", "entity"),
        param("Camera_Pose*", "pose"),
    ],
    doc=(
        "Builds a camera pose from entity and default camera definitions, applies overrides, and "
        "starts or interpolates camera transitions."
    ),
    stable=True,
)

stable.fn(
    "Camera_CalculateFollowAngles",
    "55 8B EC 81 EC 78 01 00 00 A1 ??",
    hook=0x9,
    ret="bool",
    params=[
        param("Camera_Runtime*", "camera"),
        param(
            "Actor_State*",
            "target_actor",
            doc="Actor that the follow-camera code is targeting for this update.",
        ),
        param(
            "Camera_Pose*",
            "pose",
            doc="Caller-owned pose output/input record used by follow-camera calculations.",
        ),
        param("PKG_CameraDef*", "cam_def"),
    ],
    doc="Calculates follow-camera yaw, orbit, and target adjustments. Returns false only when the initial global camera condition blocks processing.",
    stable=True,
)

stable.fn(
    "Physics_CheckGroundFriction",
    "71 ?? C9 75 ?? 50 E8 ??",
    match=-0xD,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose record/ground-friction state is checked and updated.",
        )
    ],
    doc="Reads record_ptr from actor at offset 0xF4. When record offset 0x71 is clear, calls Physics_CalculateFrictionForce and updates the friction fields.",
    stable=True,
)

stable.fn(
    "Trail_SpawnFromEntry",
    "8B 15 ?? ?? ?? ?? 83 EC 24",
    hook=0x6,
    ret="int32_t*",
    params=[param("Actor_State*", "actor"), param("int32_t", "trail_index")],
)

stable.fn(
    "Entity_GetActiveActorFromList",
    "8B 15 ?? ?? ?? ?? 85 D2 74 42 66 83 7A 0A 00 76 3B",
    hook=0x6,
    ret="Actor_State*",
    params=[],
    doc=("Returns the active actor from the current level entity slots."),
)

stable.fn(
    "Camera_UpdateShakeOffset",
    "55 8B EC 51 8B 4D 08 66 8B 81 D4 00 01 00",
    hook=0x6,
    ret="void",
    params=[param("Camera_Runtime*", "camera")],
    doc=(
        "Applies late camera shake after Camera_UpdateFollow using the countdown at +0x100D4, "
        "intensity at +0x100D6, and the shake lookup table, adjusting camera eye and target fields."
    ),
    stable=True,
)

stable.fn(
    "Physics_CheckGroundSlopeDirection",
    "D0 0C 83 F8 1C 0F 8E ??",
    match=-0x2A,
    ret="bool",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("Math_Vec3I16*", "out_normal"),
        param("Math_Vec3I32*", "delta_vec"),
    ],
    doc=(
        "When squared delta length exceeds 28, rotates the collision-polygon face normal into actor "
        "space and returns whether its dot product with delta_vec is negative. Leaves out_normal "
        "untouched below the threshold."
    ),
    stable=True,
)

stable.fn(
    "Graphics_AdjustLevelScale",
    "A1 ?? ?? ?? ?? 85 C0 7C",
    ret="void",
    params=[
        param(
            "float",
            "measured_fps",
            doc="Averaged frame rate measured by Graphics_RenderFrame before adjusting the level/render scale.",
        )
    ],
    doc="For game modes 0 through 18 except 4/9/14 and measured_fps >= 10, updates the dynamic scale global. At fps <= 30 also writes the value to render-list state +0xB8.",
    stable=True,
)

stable.fn(
    "Level_InitializeActorSystem",
    "51 53 33 DB 57 89 1D ??",
    ret="void",
    params=[],
    doc="Registers collision slot 2, preprocess-table slot 1, and actor-phase-table slots 1 and 3, then clears the entity work buffer and spawns level actors. The preprocess table has three slots selected by Actor_State+0xC4.",
)

stable.fn(
    "Actor_ProcessSnapAndEntityUpdate",
    "00 00 00 75 ?? 51 E8 ??",
    match=-0x33,
    ret="void",
    params=[param("Actor_State*", "actor")],
    public=False,
    callable=False,
    doc="Actor-phase slot 3 in the nine-entry table selected by Actor_State+0xC5. Its return value is ignored. Maintains record +0x18C countdown, snaps at zero, then clears entity-state bit 0x20.",
)

stable.fn(
    "Player_ProcessMovement",
    "55 8B EC 83 EC 2C 53 8B 5D 08 56 A1 ??",
    hook=0x6,
    ret="void",
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
    public=False,
    callable=False,
    doc="Preprocess slot 1. Its return value is ignored while it processes input, camera-relative steering, actor velocity, and player record state.",
)

stable.fn(
    "Actor_ProcessPlayerBehavior",
    "83 EC 1C 53 55 56 57 8B 7C 24 30 8B 0D ??",
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor being processed for player/AI behavior. State mutation depends on validating that this is the intended player.",
        )
    ],
    public=False,
    callable=False,
    doc="Actor-phase slot 1 selected by Actor_State+0xC5. Its return value is ignored while it runs input, trail, navigation, animation, audio, collision, and respawn transitions.",
)

stable.fn(
    "Actor_UpdateAnimationState",
    "83 EC 08 55 56 57 8B 7C 24 18 57 E8 ??",
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("PKG_ActorRecord*", "record"),
        param("int32_t", "update_from_input"),
    ],
    doc="Selects and queues player animation state from current progress, entity defaults, movement input, behavior flags, attachment state, and forced record state. States 4/6/8/9 wait for Q12 progress 0x1000.",
    stable=True,
)

stable.fn(
    "Player_RespawnAfterDeath",
    "53 56 8B 74 24 14 33 DB 66 89 1D ??",
    hook=0x6,
    ret="void",
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
            "PKG_ActorRecord*",
            "record",
            doc="Player actor record containing backup-puppy and respawn state fields.",
        ),
    ],
    doc=(
        "Handles the player-death respawn transition by updating life state, saving progress, and "
        "either entering pause/game-over state or reinitializing placement and sound."
    ),
)

stable.fn(
    "Physics_ProcessActorCollision",
    "85 C9 89 75 F8 0F 8C ??",
    match=-0x15,
    hook=0x6,
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
    doc="Resolves collision overlap between actor and otherActor. CollisionDepth is a signed contact/penetration scalar.",
)

stable.fn(
    "Actor_ProcessCollisionResponse",
    "24 34 83 FF FE 0F 85 ??",
    match=-0x13,
    hook=0x8,
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
    match=-0x38,
    hook=0x6,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "puppy_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    doc="Processes asymmetric puppy or companion contact after parent and team filtering. Returns -1 when accepted and zero when rejected.",
    stable=True,
)

stable.fn(
    "Level_CleanupActors",
    "A1 ?? ?? ?? ?? 57 85 C0 0F",
    ret="void",
    params=[],
    doc="Destroys/cleans level actor state and returns transition status.",
)

stable.fn(
    "Animation_InterpolateKeyframeVec3Blend",
    "?? ?? 8B 44 24 2C 8B 74",
    match=-0x16,
    hook=0x8,
    ret="void",
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
            "Math_Vec3I32*",
            "inout_vec3",
            doc="Destination vector blended in place.",
        ),
    ],
    doc="Samples vec3_track, then updates each inout_vec3 component as current + (((sample-current)*blend_weight_q12)>>12). All callers use only the output buffer.",
    stable=True,
)

stable.fn(
    "Animation_InterpolateVec3",
    "08 03 D0 8B 07 C1 E8 ??",
    match=-0x4F,
    hook=0x6,
    ret="void",
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
            "Math_Vec3I32*",
            "out_vec3",
            doc="Receives the interpolated three-component vector.",
        ),
    ],
    doc="Writes a three-int32 vector from a constant or keyed 0x28-byte animation channel. Keyed paths bracket time and evaluate fixed-point Hermite/linear terms.",
    stable=True,
)

stable.fn(
    "Animation_CalculateSplineParameter",
    "8B 75 08 8B C7 C1 E8 ??",
    match=-0xC,
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
            doc="Previous keyframe timing/easing word. High bits encode previous-key time and low bits encode easing metadata.",
        ),
        param(
            "uint8_t",
            "next_ease_index",
            doc="Low-byte easing/control index from the next keyframe timing word.",
        ),
        param(
            "uint8_t*",
            "keyframe_data",
            doc="Base of the keyframe data block. Easing/control records are addressed before this pointer.",
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
    "Animation_InterpolateKeyframeQuatBlend",
    "45 F8 56 50 51 52 E8 ??",
    match=-0xE,
    hook=0x6,
    ret="void",
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
        param(
            "Animation_SplineChannel*",
            "quat_track",
            doc="Quaternion keyframe track descriptor.",
        ),
        param(
            "Math_QuaternionI16*",
            "inout_quat",
            doc="Destination quaternion that is blended in place.",
        ),
    ],
    doc="Samples a Q14 quaternion, chooses normalized LERP or trigonometric interpolation from the dot product, and overwrites four int16 components in inout_quat. Both callers discard EAX.",
    stable=True,
)

stable.fn(
    "Animation_InterpolateQuat",
    "06 03 D0 8B 06 C1 E8 ??",
    match=-0x47,
    hook=0x6,
    ret="void",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param(
            "Animation_SplineChannel*",
            "quat_track",
            doc="Quaternion keyframe track descriptor.",
        ),
        param(
            "Math_QuaternionI16*",
            "out_quat",
            doc="Receives the interpolated four-component Q14 quaternion.",
        ),
    ],
    doc="Writes a four-int16 Q14 quaternion for constant, endpoint, or keyed channel paths. Keyed paths bracket time and may call Animation_InterpolateQuaternionSlerp.",
    stable=True,
)

stable.fn(
    "Animation_InterpolateQuaternionSlerp",
    "55 8B EC 53 56 57 8B 7D 18 66 85 FF 0F 84 ??",
    ret="void",
    params=[
        param(
            "Math_QuaternionI16*",
            "out_quat",
            doc="Receives the interpolated Q14 quaternion.",
        ),
        param("Math_QuaternionI16*", "from_quat", doc="Starting Q14 quaternion."),
        param("Math_QuaternionI16*", "to_quat", doc="Ending Q14 quaternion."),
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
    doc="Writes out_quat using SLERP when angle_fp12 has a usable sine, with normalized/linear fallbacks for degenerate cases. All three callsites discard EAX and use the output buffer.",
    stable=True,
)

stable.fn(
    "Animation_ConvertQuatToRotMatrix",
    "24 18 57 50 51 52 E8 ??",
    match=-0x10,
    hook=0x7,
    ret="void",
    params=[
        param(
            "uint32_t",
            "frame_time",
            doc="Animation frame/time value in the track time domain.",
        ),
        param(
            "Animation_SplineChannel*",
            "quat_track",
            doc="Quaternion keyframe track descriptor.",
        ),
        param(
            "Math_Matrix3x3I16*",
            "out_matrix",
            doc="Receives the 3x3 signed fixed-point rotation matrix.",
        ),
    ],
    doc="Samples a Q14 quaternion and writes all nine int16 entries of out_matrix using fixed-point quaternion-to-matrix products. Both callers discard EAX.",
    stable=True,
)

stable.fn(
    "Animation_CheckKeyframeActive",
    "00 3B FE 72 ?? 0F 84 ??",
    match=-0x50,
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
    doc="Applies +4 low 16 bits of time bias and bits 20 through 31 end time, then locates the active keyframe. Constant sentinel uses inline +4 time.",
)

stable.fn(
    "Animation_InterpolateSpline",
    "C0 FF 03 00 3B C8 72 ?? 75 13",
    match=-0x4E,
    hook=0x6,
    ret="void",
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
    doc="Writes one int16 sample from a constant or keyed scalar channel. Keyed paths bracket time, calculate a Q12 spline parameter, and evaluate cubic fixed-point terms.",
    stable=True,
)

stable.fn(
    "Bone_TransformVerticesMorphed",
    "8B 01 83 F8 FF 0F 85 ??",
    match=-0xC,
    hook=0x6,
    ret="void",
    params=[
        param("uint32_t", "sample_time"),
        param("Animation_SplineChannel*", "morph_channels"),
        param("void*", "actor_or_mesh_state"),
    ],
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Bone_TransformVerticesWeighted",
    "53 F6 40 0A 40 0F 84 ??",
    match=-0x9,
    hook=0x6,
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
            doc="Bone/skin channel array. channel_flags bit 0x40 selects this weighted vertex path.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state whose skin table and scene vertex buffer are updated.",
        ),
        param(
            "Submesh_Entry*",
            "mesh_piece",
            doc="Mesh piece descriptor. +4 is first vertex and +6 is vertex count for the affected span.",
        ),
    ],
    doc=(
        "Transforms weighted/skinned vertices for a mesh piece using animation bone channels, "
        "then updates scene vertex data and recomputes normals."
    ),
)

stable.fn(
    "Bone_ComputeNormalsPostTransform",
    "?? 80 7B 64 03 0F 84 ??",
    match=-0x16,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state containing transformed vertices, face records, and normal accumulators.",
        ),
        param(
            "Submesh_Entry*",
            "mesh_piece",
            doc="Mesh piece descriptor. vert_start_index/vert_count select vertices and poly_start_index/poly_count select faces.",
        ),
    ],
    doc=(
        "Computes post-transform face normals for a mesh piece. When mesh_piece flags bit0 is "
        "set, face normals accumulate into actor normal storage before affected vertices are "
        "normalized."
    ),
)

stable.fn(
    "Bone_BlendVerticesMultiWeight",
    "40 8D 04 8A 50 56 E8 ??",
    match=-0x2B,
    ret="void",
    params=[
        param(
            "Submesh_Entry*",
            "mesh_piece",
            doc="Mesh piece descriptor. vert_start_index selects the first vertex and vert_count the vertex count.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor/render state whose transformed vertex buffer and normal accumulator are updated.",
        ),
        param(
            "int16_t**",
            "skin_tables",
            doc="Array of source skin/bone xyz delta tables, each stored as 4 int16 values per vertex.",
        ),
        param(
            "int32_t*",
            "weights",
            doc="Q12 blend weights. Up to four nonzero entries are consumed.",
        ),
    ],
    doc="Blends up to four weighted skin/bone vertex tables into the actor transformed vertex buffer for one mesh piece, then recomputes normals. A zero second weight copies the first skin table directly.",
)

stable.fn(
    "Actor_GetAnimationWaitStatus",
    "53 56 8B 74 24 0C 57 85 F6 0F 84 ??",
    hook=0x6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("int32_t", "anim_seq_index")],
    doc="Returns zero when complete, 1 while active, and -1 when activation is impossible. The wait opcode retries both nonzero results without a timeout.",
    stable=True,
)

stable.fn(
    "Animation_QueueStateChange",
    "A1 ?? ?? ?? ?? 8B 4C 24 04 89",
    ret="void",
    params=[param("int32_t", "anim_state")],
    doc="Queues an animation state change without checking capacity or actor identity. More than 10 pending states overwrite adjacent data.",
    stable=True,
)

stable.fn(
    "Model_AdvanceAnimation",
    "?? 03 C5 89 46 60 A1 ??",
    match=-0x90,
    hook=0x7,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Advances the supplied actor and drains the entire global queue into it in LIFO order. At count 11 the first read treats the aliased queue[10]/count as state 11.",
    stable=True,
)

stable.fn(
    "Bone_UpdateJointTracking",
    "83 C0 2C 50 51 52 E8 ??",
    match=-0x15,
    hook=0x6,
    ret="void",
    params=[
        param("Bone_JointTrackState*", "track"),
        param("Actor_State*", "target_actor"),
        param("int32_t", "direct_tracking_mode"),
    ],
    doc="Updates Bone_JointTrackState current yaw/pitch and node world rotation toward target_actor. The third stack argument is only tested for zero.",
    stable=True,
)

stable.fn(
    "Scene_UpdateNodeAnimation",
    "81 7E 5C FF FF 0F 84 ??",
    match=-0xB,
    hook=0x6,
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
            doc="Parent node transform source. Matrix and position are used for world composition.",
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
        "Bone_TransformVerticesWeighted, handles type 8 scalar pairs, then writes the node world "
        "position, velocity delta, and world rotation matrix from the parent transform."
    ),
)

stable.fn(
    "Actor_ApplyVerticalVelocity",
    "7C ?? 10 57 50 56 E8 ??",
    match=-0x31,
    ret="void",
    params=[param("Actor_State*", "actor"), param("int32_t", "velocity")],
    doc="Advances actor vertical animation tick directly or through Actor_ApplySplineMovement, applies loop/clamp behavior, and sets actor+0x65 bit 0x40. Sole caller discards EAX and Ghidra recovers void.",
    stable=True,
)

stable.fn(
    "Actor_ApplySplineMovement",
    "45 F4 8B 42 04 C1 E8 ??",
    match=-0x2C,
    hook=0x6,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("Animation_SplineChannel*", "spline_track"),
        param("int32_t", "velocity"),
    ],
    doc="Advances anim_tick over a vec3 spline and moves attach_offset within the actor's move_anim_speed distance budget. Partial segments scale both spatial delta and tick advance.",
    stable=True,
)

stable.fn(
    "Video_InitializeAVIPlayer",
    "6A 00 6A 00 6A 00 68 ?? ?? ?? ?? C7",
    hook=0x6,
    ret="int32_t",
    params=[
        param(
            "char*",
            "window_handle_text",
            doc="Decimal HWND string passed by Video_PlayMovieIntro and parsed into avi_window_handle on successful MCI AVI initialization.",
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
    match=-0xA,
    hook=0x6,
    ret="int32_t",
    params=[],
    doc='Sends the MCI "close avivideo" command, clears avi_player_initialized, and returns the MCI status/result.',
)

stable.fn(
    "Video_OpenAVIFile",
    "A1 ?? ?? ?? ?? 85 C0 75 ?? 33",
    ret="int32_t",
    params=[param("int32_t", "file_handle")],
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Video_CloseAVIFile",
    "01 FF 15 ?? ?? ?? ?? C3",
    match=-0x22,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_PlayAVIFullscreen",
    "0C 6A 00 6A 00 6A 00 68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? F7",
    match=-0x17,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_IsAVIPlaying",
    "83 EC 50 68 ?? ?? ?? ?? 68 ??",
    hook=0x8,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Script_OpCheckTerminator",
    "FF 48 83 F8 03 0F 87 ??",
    match=-0xF,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Handles script terminator opcodes 1 through 4. Clears *ip for hard end cases, skips nested variable-length blocks for opcode 2, and rotates actor script-nesting state bytes for opcodes 3/4.",
)

stable.fn(
    "Script_OpSetEntityIndex",
    "8B 44 24 08 8B 08 41 89 08 8B C1 8B 4C 24 04 8A 40 ?? 88 81 ?? ?? ?? ?? C3",
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "script_cursor_inout")],
    doc=(
        "Consumes one script byte from *script_cursor_inout and stores it as the actor "
        "entity-slot selector."
    ),
)

stable.fn(
    "Script_OpJumpConditional",
    "51 8B 4C 24 0C 33 D2 8B 01 83 C0 02 89 01 8A 70 ?? 8A 50 ?? 03 D0 40 89 01",
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "script_cursor_inout")],
    doc=(
        "Reads a 16-bit relative target and selector byte, then restores *script_cursor_inout to "
        "the target when the selector differs from the actor entity-slot selector."
    ),
)

stable.fn(
    "Script_ResolveVariableRef",
    "8B 44 24 0C 8B 4C 24 08 83 F8 09 7D ?? 8B ??",
    hook=0x8,
    ret="int32_t",
    params=[
        param("Entity_State*", "actor"),
        param("int32_t* *", "value_storage_inout"),
        param("int32_t", "ref_id"),
    ],
    doc="Resolves refs 0 through 8 to entity fields, later ordinary refs to level/global storage, and normally replaces value_storage_inout with that storage. Ref 0xFC returns zero without writing it.",
)

stable.fn(
    "Script_GetVariableById",
    "25 ?? ?? ?? ?? 50 51 6A 00 E8 ?? ?? ?? ?? 83",
    match=-0xA,
    hook=0x8,
    ret="int32_t",
    params=[
        param(
            "uint8_t",
            "progress_var_index",
            doc="Selector byte position by +9 before resolving the backing script/global progress variable.",
        )
    ],
    doc="Resolves and returns a script variable by id. Nothing animation-specific.",
)

stable.fn(
    "Script_SetVariableById",
    "?? ?? 8B 44 24 10 8B 54",
    match=-0x16,
    hook=0x8,
    ret="void",
    params=[
        param(
            "uint8_t",
            "progress_var_index",
            doc="Selector byte position by +9 before resolving the backing script/global progress variable.",
        ),
        param(
            "int32_t",
            "progress_value",
            doc="Value written to the resolved progress variable.",
        ),
    ],
    doc="Converts progress_var_index to script ref ID index+9, resolves the storage pointer, and writes progress_value. Sole caller discards EAX.",
    stable=True,
)

stable.fn(
    "Script_OpSetVariable",
    "C1 E3 08 57 0B DA E8 ??",
    match=-0x5B,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc=(
        "Consumes destination ref, RHS ref kind, arithmetic opcode, and a four-byte big-endian RHS. "
        "Opcodes add, saturating-subtract, multiply, or guarded-divide the resolved destination."
    ),
)

stable.fn(
    "Script_OpDecrementVariable",
    "?? ?? 8B 4C 24 14 83 C4",
    match=-0x1E,
    hook=0x8,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpCallIndirect",
    "8B 44 24 08 50 8B 10 42 89 10 8B 44 24 08 8B CA 33 D2 50 8A 51 FF FF 14 95 ?? ?? ?? ??",
    ret="void",
    params=[
        param("Entity_State*", "entity"),
        param("uint8_t**", "script_cursor_inout"),
    ],
    doc="Reads one raw nested opcode and calls Script_ActorOpcodeDispatchTable[opcode]. The table has 45 slots (0 through 44), slot 0 is NULL, and this, Script_ExecuteBehaviorScript, and Script_OpRunWithActor perform no range or null check.",
    stable=True,
)

stable.fn(
    "Script_OpMoveToTarget",
    "66 81 FF FF 7F 0F 84 ??",
    match=-0x7C,
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Consumes a rel16 failure target, i16 selector, and u32 big-endian best-path-distance bound. Selector 0x7FFF exits.",
)

stable.fn(
    "Script_OpWalkToTarget",
    "89 4C 24 1C 75 ?? A1 ??",
    match=-0x62,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Consumes a failure target, target selector, animation selector, and maximum path distance. Calls Actor_TracePath and rewinds the script pointer on failure.",
)

stable.fn(
    "Script_OpRunToTarget",
    "03 C8 83 C0 02 8B E9 ??",
    match=-0x1E,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Consumes a rel16 failure target, i16 target selector, i16 attach/animation selector, and u32 big-endian best-path-distance bound. Calls Actor_TracePath(g_scriptCurrentEntity, selector, INT_MAX, bound, 0x40), treating zero bound as INT_MAX.",
)

stable.fn(
    "Script_OpRotateActor",
    "83 C0 02 89 06 8B E9 ??",
    match=-0x1D,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpMoveToTargetWithCamera",
    "08 8B 74 24 18 8B 1D ??",
    match=-0x64,
    hook=0x8,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpWaitForAnimation",
    "83 C0 02 89 06 8B 3D ??",
    match=-0x1D,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpSetActorProperty",
    "08 0B CA 8B C1 8B 0D ??",
    match=-0x41,
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpClearActorProperty",
    "8B 44 24 08 6A FF 8B 08 41 89 08 8B C1 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpPathfindToEntity",
    "3D FF 7F 74 ?? 8B 35 ??",
    match=-0x2A,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpTraceActorPath",
    "83 C0 04 89 07 8B 35 ??",
    match=-0x4B,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Parses a path-trace command from *ip and resolves special entity or actor selectors against current_level_data.entity_array. It saves transient path state, calls Actor_TracePath, and advances the script pointer.",
)

stable.fn(
    "Script_OpAddNavigationCommand",
    "88 54 24 10 75 ?? 8B ??",
    match=-0x13,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Parses one navigation opcode from *ip. Opcode 0x67 updates the current entity index and skips its payload, while other opcodes enqueue a navigation command.",
)

stable.fn(
    "Script_OpTestPathTrace",
    "58 FF 8B EB 72 ?? 80 ??",
    match=-0x4E,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpEmitSignal",
    "C2 3C FE 75 ?? C1 E8 ??",
    match=-0x4B,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpPlaySoundIndex",
    "?? 83 C4 08 85 C0 7C ?? 8B 15 ?? ?? ?? ?? 8B",
    match=-0x46,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "script_cursor_inout")],
    doc=(
        "Script sound opcode that reads one byte from *script_cursor_inout, advances the "
        "cursor by one byte, and plays it as a level-local sound index."
    ),
)

stable.fn(
    "Script_OpStopSound",
    "?? 83 C4 08 85 C0 7C ?? 8B 15 ?? ?? ?? ?? 6A",
    match=-0x4D,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpRemoveActor",
    "66 85 C0 7E ?? 8B 15 ?? ?? ?? ?? 68",
    match=-0x29,
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpRunWithActor",
    "83 C0 02 89 06 8B 1D ??",
    match=-0x21,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Entity_GetOrSpawnCameraActor",
    "00 04 00 00 74 ?? 80 ??",
    match=-0x1A,
    hook=0xA,
    ret="Actor_State*",
    params=[
        param(
            "Entity_State*",
            "source_entity",
            doc="Source entity whose active actor pointer is returned or spawned when camera activation requires it.",
        )
    ],
    doc="Returns a source entity's active actor, creating one when needed for script or camera activation. Existing active actors are returned directly.",
)

stable.fn(
    "Script_OpEnsureCameraActive",
    "5A 04 51 8B 6A 08 E8 ??",
    match=-0x42,
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpTriggerLevelTransition",
    "03 C8 40 89 06 8B E9 ??",
    match=-0x1C,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpTriggerMusicTransition",
    "83 EC 10 8B 44 24 18 56 C7 05 ??",
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpTriggerMusicFade",
    "EF C1 FA 05 8B CA C1 E9 ??",
    match=-0x81,
    hook=0x6,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "Script_OpPlaySoundBlockOrWait",
    "03 D8 40 89 06 8B 3D ??",
    match=-0x1B,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "script_cursor_inout")],
    doc=(
        "Length-prefixed script sound block opcode. It reads the block length and sound operand "
        "from *script_cursor_inout, can hold the cursor while playback is active, and uses "
        "Audio_PlaySoundDefinition3D with a current-level sound definition."
    ),
)

stable.fn(
    "Script_OpCheckButtonState",
    "?? 83 C4 10 83 F8 64 7D ?? 89",
    match=-0x6A,
    hook=0x8,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
)

stable.fn(
    "MiniGame_InitializeRoundParams",
    "66 8B 44 24 08 66 C7 05 ??",
    ret="void",
    params=[
        param(
            "Math_Vec3I32*",
            "position",
            doc="Actor position vector passed by both known callers. Unused by the callee.",
        ),
        param(
            "int16_t",
            "round_param",
            doc="Round/mini-game parameter stored in the global word at pcdogs.exe.",
        ),
    ],
    doc="Sets the default mini-game round counter to 10 and stores round_param. Position is an unused ABI slot.",
    stable=True,
)

stable.fn(
    "MiniGame_SetScoreValues",
    "8B 44 24 08 66 A3 ??",
    hook=0xA,
    ret="void",
    params=[param("int16_t", "player_1_score"), param("int16_t", "player_2_score")],
    doc=(
        "Stores player_2_score in two adjacent mini-game state words and player_1_score in the "
        "following score word."
    ),
    stable=True,
)

stable.fn(
    "Camera_UpdateEffects",
    "62 00 E8 ?? ?? ?? ?? 83 C4 0C",
    match=-0x17,
    ret="void",
    params=[],
    doc=(
        "Runs Camera_UpdateFollow, Camera_UpdateShakeOffset, and Camera_UpdateRollEffect in order on "
        "the global camera state."
    ),
    stable=True,
)

stable.fn(
    "Camera_UpdateRollEffect",
    "E0 0C 99 F7 F9 50 E8 ??",
    match=-0x43,
    ret="void",
    params=[param("Camera_Runtime*", "camera")],
    doc=(
        "Applies the active signed roll magnitude to camera roll using the remaining countdown, "
        "duration, and eased progress, then clears roll when the effect completes."
    ),
    stable=True,
)

stable.fn(
    "Checkers_UpdateStateMachine",
    "55 8B EC 81 EC 54 05 00 00 A1 ??",
    hook=0x9,
    ret="void",
    params=[],
    doc="Runs the Checkers_MinGameState lifecycle from INITIALIZE through BEGIN_PLAY, input, simple or capture animation, promotion, completed turns, no-move endings, and AI search. Move result 1 forces same-piece capture continuation.",
)

stable.fn(
    "Checkers_QueueSelectedCellTargets",
    "83 EC 0C 8B 0D ?? ?? ?? ?? 53 8B",
    hook=0x9,
    ret="void",
    params=[],
    doc="Queues navigation targets for the selected source and destination board cells. It does not update camera state.",
    stable=True,
)

stable.fn(
    "Checkers_ProcessInputAndRender",
    "56 8B 35 ?? ?? ?? ?? 57 33",
    hook=0x7,
    ret="bool",
    params=[],
    doc="Snapshots Checkers_MinGameState, renders one frame, and consumes ANIMATION_COMPLETE. Simple states finish.",
    stable=True,
)

stable.fn(
    "Checkers_QueueMoveMidpointTarget",
    "83 EC 0C 8B 0D ?? ?? ?? ?? A1",
    hook=0x9,
    ret="void",
    params=[],
    doc="Queues navigation opcode 0x73 at the midpoint of the selected move. It does not write camera state.",
    stable=True,
)

stable.fn(
    "Checkers_BuildMoveList",
    "E7 01 83 FF 08 0F 8D ??",
    match=-0x32,
    hook=0x7,
    ret="BOOL",
    params=[
        param("Checkers_Board*", "board_state"),
        param("Checkers_Player", "player"),
        param("Checkers_MoveList*", "move_list"),
    ],
    doc="Builds a count-prefixed move list for CHECKERS_PLAYER_1 or CHECKERS_PLAYER_2 without a local capacity check. Mandatory captures discard accumulated simple moves.",
    stable=True,
)

stable.fn(
    "Checkers_ValidateSimpleMove",
    "8B 54 24 10 8B 44 24 14 8B CA 56 0B C8 57 F7 C1 ?? ?? ?? ?? 75 ??",
    ret="Checkers_MoveValidation",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed board addressed as playable_by_col[col][row >> 1], with four stored cells per column.",
        ),
        param("int32_t", "from_col"),
        param("int32_t", "from_row"),
        param("int32_t", "to_col"),
        param("int32_t", "to_row"),
    ],
    doc="Returns CHECKERS_MOVE_SIMPLE for a legal one-column diagonal step in the piece's allowed direction, otherwise CHECKERS_MOVE_INVALID.",
    stable=True,
)

stable.fn(
    "Checkers_CheckCapturePossible",
    "8B 44 24 08 53 55 56 57 8D 78 FF 83 FF 05 ??",
    ret="BOOL",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed board addressed as playable_by_col[col][row >> 1], with four stored cells per column.",
        ),
        param(
            "Checkers_Piece",
            "piece",
            doc="Piece code at the queried square. Men and kings use the declared Checkers_Piece values.",
        ),
        param(
            "int32_t",
            "col",
            doc="Zero-based source column to inspect for available captures.",
        ),
        param(
            "int32_t",
            "row",
            doc="Zero-based source row to inspect for available captures.",
        ),
    ],
    doc=(
        "Returns TRUE when the piece at (col, row) has an allowed capture over an opponent into an "
        "empty landing square."
    ),
    stable=True,
)

stable.fn(
    "Checkers_AppendCaptureMoveSequences",
    "F0 89 74 24 04 0F 8F ??",
    match=-0xF,
    hook=0x7,
    ret="void",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-square checkers board copied before recursive capture expansion.",
        ),
        param("int32_t", "from_col"),
        param("int32_t", "from_row"),
        param(
            "Checkers_MoveSequence**",
            "move_cursor_in_out",
            doc="Cursor advanced as terminal capture sequences are appended.",
        ),
    ],
    doc=(
        "Seeds capture-only move sequences from one square, copies the board for each legal jump, "
        "and recursively appends multi-jump continuations."
    ),
    stable=True,
)

stable.fn(
    "Checkers_ValidateMove",
    "C1 F8 FF FF FF 0F 85 ??",
    match=-0x11,
    ret="Checkers_MoveValidation",
    params=[
        param(
            "Checkers_Board*",
            "board",
            doc="Packed 32-playable-square checkers board to validate against.",
        ),
        param("int32_t", "from_col", doc="Zero-based source column."),
        param("int32_t", "from_row", doc="Zero-based source row."),
        param("int32_t", "to_col", doc="Zero-based destination column."),
        param("int32_t", "to_row", doc="Zero-based destination row."),
    ],
    doc=(
        "Returns CHECKERS_MOVE_SIMPLE for a legal step, CHECKERS_MOVE_CAPTURE for a legal jump, and "
        "CHECKERS_MOVE_INVALID for invalid or blocked mandatory-capture cases. When that rule is "
        "enabled, simple moves are rejected if any same-side piece can capture."
    ),
    stable=True,
)

stable.fn(
    "Checkers_ExecuteMove",
    "24 57 50 53 51 56 E8 ??",
    match=-0x17,
    hook=0x8,
    ret="Checkers_MoveExecutionResult",
    params=[
        param("Checkers_Board*", "board"),
        param("int32_t", "from_col"),
        param("int32_t", "from_row"),
        param("int32_t", "to_col"),
        param("int32_t", "to_row"),
    ],
    doc="Validates and applies one move, removes a jumped piece, tests for another capture, and promotes men on terminal rows. A capture with another legal jump returns CHECKERS_EXEC_CAPTURE_MUST_CONTINUE.",
    stable=True,
)

stable.fn(
    "Checkers_ExpandCaptureSequences",
    "A1 ?? ?? ?? ?? 83 EC 40",
    ret="void",
    params=[
        param("Checkers_Board*", "board"),
        param("Checkers_MoveSequence**", "move_cursor_in_out"),
        param("uint32_t", "pack_shift"),
    ],
    doc="Recursively expands legal multi-jump capture sequences on copied boards. It advances the output cursor without checking capacity.",
    stable=True,
)

stable.fn(
    "Checkers_ExecuteMoveSequence",
    "07 57 56 52 51 50 E8 ??",
    match=-0x20,
    ret="Checkers_MoveExecutionResult",
    params=[
        param("Checkers_Board*", "board"),
        param("Checkers_MoveSequence const*", "move_sequence"),
    ],
    doc="Decodes successive 3-bit destination column and row pairs from move_sequence[2.3], applies each leg until both packed words are exhausted, and returns the final CHECKERS_EXEC continuation or completed-turn result.",
    stable=True,
)

stable.fn(
    "Checkers_SearchBestMove",
    "A1 ?? ?? ?? ?? 81 EC EC",
    ret="int32_t",
    params=[
        param("Checkers_Board*", "board"),
        param("int32_t", "depth"),
        param(
            "Checkers_MoveSequence*",
            "out_best_move",
            doc="Optional output receiving the selected 16-byte move sequence.",
        ),
        param("Checkers_Player", "player"),
        param("int32_t", "alpha"),
        param("int32_t", "beta"),
    ],
    doc="Runs recursive alpha-beta search, alternating sides with player ^ 3, and optionally writes the selected move sequence. Every 64 nodes it temporarily enters AI_RENDER_TICK to render and poll cancellation.",
    stable=True,
)

stable.fn(
    "Checkers_EvaluateBoardScore",
    "E0 01 83 F8 08 0F 8D ??",
    match=-0x10,
    hook=0x6,
    ret="int32_t",
    params=[param("Checkers_Board*", "board")],
    doc=(
        "Scores men by advancement and kings by base/edge value, adds difficulty-scaled noise, and "
        "normalizes the sign for the active player."
    ),
    stable=True,
)

stable.fn(
    "Checkers_QueueMoveListTargets",
    "C6 89 74 24 08 0F 8E ??",
    match=-0x10,
    hook=0x7,
    ret="void",
    params=[param("Checkers_MoveList*", "move_list")],
    doc=(
        "Walks every move record in the count-prefixed list and queues opcode-0x74 world targets "
        "for changed cells."
    ),
    stable=True,
)

stable.fn(
    "Checkers_InitializeBoard",
    "56 8B 74 24 08 57 33 C9 33 C0 8D 14 08 F6 C2 ??",
    ret="void",
    params=[param("Checkers_Board*", "board")],
    doc="Initializes playable rows 0-2 with CHECKERS_PIECE_PLAYER1_MAN, rows 3-4 empty, and rows 5-7 with CHECKERS_PIECE_PLAYER2_MAN.",
)

stable.fn(
    "Checkers_HighlightPlayerPieces",
    "E6 01 83 FE 08 0F 8D ??",
    match=-0xD,
    ret="void",
    params=[param("Checkers_Board*", "board"), param("Checkers_Player", "player")],
    doc=(
        "Maps pieces whose low owner bits match player to world space and emits Nav_AddCommand "
        "opcode 0x75. The opcode's meaning remains uncertain."
    ),
    stable=True,
)

stable.fn(
    "Actor_DestroyAll",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85",
    ret="void",
    params=[],
    doc=(
        "Runs powerup cleanup, clears actor references, detaches Actor_CollisionListHead, and "
        "destroys every actor in the detached list."
    ),
    stable=True,
)

stable.fn(
    "PKG_UnloadResourceGameData",
    "56 33 F6 56 E8 ??",
    hook=0x9,
    ret="void",
    params=[],
    doc="Unloads active game data, clears runtime callbacks/flags, and returns cleanup status.",
)

stable.fn(
    "Scene_ResetState",
    "8A 0D ?? ?? ?? ?? B8 02 00 00 00 84 C8 0F 85 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc=(
        "Clears scene and root-node state, including the active-node array and actor-list heads, then "
        "initializes actor and powerup systems. Root-node storage overlaps fields used differently by "
        "regular scene nodes."
    ),
    stable=True,
)

stable.fn(
    "Animation_ProcessController",
    "8B 18 83 FB FF 0F 84 ??",
    match=-0xC,
    hook=0x6,
    ret="bool",
    params=[
        param("Animation_ControllerSlot**", "controller_slots"),
        param(
            "Animation_PackedControllerState*",
            "packed_controller_state_inout",
            doc="Persistent packed state, or ANIM_CTRL_STATE_DISABLED.",
        ),
        param("int32_t", "frame_count_q12"),
    ],
    doc="Leaves ANIM_CTRL_STATE_DISABLED unchanged. Otherwise decodes and repacks the slot, phase, saved random frame, and playback flags.",
    stable=True,
)

stable.fn(
    "Graphics_ProcessMeshCommands",
    "06 33 C9 8A 08 83 E9 ??",
    match=-0x2E,
    ret="void",
    params=[
        param("Mesh_CmdList*", "cmd_list"),
        param("Animation_ControllerSlot**", "controller_slots"),
        param("void*", "owner_context"),
    ],
    doc="Dispatches dirty Animation_ControllerCommand and Animation_VertexColorController records, clearing MESH_CMD_DIRTY before each handler. Controller state at +0xC persists across frames, with ANIM_CTRL_STATE_DISABLED suppressing updates.",
    stable=True,
)

stable.fn(
    "Animation_ProcessVertexColor",
    "53 0C 57 85 D2 0F 84 ??",
    match=-0x11,
    hook=0x9,
    ret="void",
    params=[
        param("Animation_VertexColorController*", "color_controller"),
        param("Mesh_NodeExtended*", "mesh_node"),
    ],
    doc="Samples up to 20 local keyframes using signed Q12 time and writes clamped RGB bytes to mesh vertices. MESH_CMD_MODE_LOOP wraps shared time and MODE_PINGPONG reflects it.",
)

stable.fn(
    "Graphics_UpdateMeshCommandState",
    "A1 ?? ?? ?? ?? 83 EC 0C F6",
    ret="void",
    params=[param("Mesh_CmdList*", "cmd_list")],
    doc="Consumes Mesh_CommandEvent values and updates Q12 playback. RESET clears progress/state.",
    stable=True,
)

stable.fn(
    "Material_ReleaseTextureArray",
    "06 00 00 00 00 8B 0D ??",
    match=-0x12,
    ret="void",
    params=[param("DDraw_IDirectDrawSurface7**", "texture_handles")],
    doc="Releases up to 25 cached DirectDraw texture surfaces and clears the bound texture if needed.",
    stable=True,
)

stable.fn(
    "Material_ClearTextureCache",
    "75 ?? 8D 46 0C 50 E8 ??",
    match=-0xA,
    ret="void",
    params=[param("Material_RuntimeDescriptor*", "descriptor")],
    doc="Clears or releases a material descriptor's 25 cached texture handles and invalidates its cached transparent color.",
    stable=True,
)

stable.fn(
    "Material_ReleaseSingleTexture",
    "A1 ?? ?? ?? ?? 48 A3",
    ret="void",
    params=[param("DDraw_IDirectDrawSurface7*", "texture_surface")],
    doc="Decrements the active texture count and releases the DirectDraw surface, discarding the COM reference count.",
    stable=True,
)

stable.fn(
    "Graphics_LoadAndUploadTexture",
    "05 ?? ?? ?? ?? 00 E8 ?? ?? ?? ?? 83 C4 08 A3 ?? ?? ?? ?? 85 C0 75 ??",
    match=-0xB,
    ret="Material_BlendTextureSet*",
    params=[param("uint8_t*", "pixel_data")],
    doc=(
        "Creates four 256x256 loading-screen texture surfaces, uploads a 640x480 RGBx buffer, and "
        "returns the global quadrant set."
    ),
    stable=True,
)

stable.fn(
    "DDraw_FillSurfaceColor",
    "83 EC 64 8B 44 24 6C 8D 54 24 00 89 44 24 50 8B 44 24 68 52 68 00 04 00 00 8B 08 6A 00 6A 00 6A 00 50 C7 44 24 18 64 00 00 00 FF 51 14 F7 D8 1B C0 40 83 C4 64 C3",
    hook=0x7,
    ret="BOOL",
    params=[
        param("DDraw_IDirectDrawSurface7*", "surface"),
        param("uint32_t", "fill_color"),
    ],
    doc=(
        "Writes fill_color to DDraw_BltFx +0x50, fills the surface with DDBLT_COLORFILL, and returns "
        "TRUE when Blt returns DD_OK."
    ),
    stable=True,
)

stable.fn(
    "D3D_CreateTextureSurface",
    "81 EC 6C 01 00 00 A1 ??",
    hook=0x6,
    ret="DDraw_IDirectDrawSurface7*",
    params=[param("uint32_t", "width"), param("uint32_t", "height")],
    doc="Rounds dimensions up to powers of two, clamps them to 256, and forces a square when D3D_DeviceDesc7.triangle_caps.texture_caps has bit 0x20. Selects a 16-bit RGB or alpha-capable format and creates a cleared texture surface.",
    stable=True,
)

stable.fn(
    "D3D_HandleTextureFormatCallback",
    "53 56 8B 74 24 0C 57 85 F6 C6 05 ?? ?? ?? ?? 00 74 ?? 8B 7C 24 14 85 FF 74 ?? 8B 5E 04 F7 C3 00 00 0E 00",
    cc=CallingConvention.STDCALL,
    ret="HRESULT",
    params=[
        param("DDraw_PixelFormat*", "pixel_format"),
        param("void*", "selected_format_context"),
    ],
    doc=(
        "EnumTextureFormats callback. Copies a supported RGB16 or 0xF000-alpha format into the "
        "opaque context and returns HRESULT D3DENUMRET_CANCEL/OK."
    ),
    public=False,
    unstable=True,
)

stable.fn(
    "D3D_CreateWorkSurface",
    "24 00 89 44 24 0C A1 ??",
    match=-0x8F,
    hook=0x6,
    ret="DDraw_IDirectDrawSurface7*",
    params=[
        param("DDraw_IDirectDrawSurface7*", "source_surface"),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
    ],
    doc=(
        "Creates and clears a scratch surface with the requested dimensions and caps 0x1800, copying "
        "the source pixel format in hardware mode. Logs the DirectX error and returns NULL on failure."
    ),
    stable=True,
)

stable.fn(
    "Material_ComputeAvgTransparentColor",
    "D2 88 5C 24 40 0F 85 ??",
    match=-0x3B,
    ret="void",
    params=[
        param("uint8_t*", "pixel_data"),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
        param("uint8_t*", "out_red"),
        param("uint8_t*", "out_green"),
        param("uint8_t*", "out_blue"),
    ],
    doc=(
        "Computes the average non-black horizontal-neighbor color around black or transparent RGBx "
        "pixels and writes the three channel averages to out_red, out_green, and out_blue."
    ),
    stable=True,
)

stable.fn(
    "Material_CopyPixelDataToTexture",
    "89 74 24 34 75 ?? 68 ??",
    match=-0x2A,
    hook=0x6,
    ret="void",
    params=[
        param("DDraw_IDirectDrawSurface7*", "texture_surface"),
        param("uint8_t*", "pixel_data"),
        param(
            "uint32_t",
            "unused_pixel_count",
            doc="Present in the native signature but never read by the routine.",
        ),
        param("uint32_t", "width"),
        param("uint32_t", "height"),
    ],
    doc="Creates and locks a format-compatible work surface, using its DDSURFACEDESC2 pitch and surface pointer to convert RGBx pixels into the selected 16-bit channel layout. It then unlocks, clears/blits into texture_surface, and releases the work surface.",
    stable=True,
)

stable.fn(
    "Graphics_BlitTextureToQuadrants",
    "89 5C 24 2C 75 ?? 68 ??",
    match=-0x2A,
    hook=0x6,
    ret="void",
    params=[
        param("DDraw_IDirectDrawSurface7**", "quadrant_surfaces"),
        param("uint8_t*", "pixel_data"),
        param("int32_t", "pixel_count"),
        param("int32_t", "width"),
        param("int32_t", "height"),
    ],
    doc=(
        "Converts the caller's 640x480, 307200-pixel RGBx image through a locked work surface, using "
        "separate DDSURFACEDESC2 records for the work and source surfaces. It reuses one RECT "
        "to blit (0,0,320,240), (321,0,640,240), (321,241,640,480), and (0,241,320,480) into the "
        "four quadrant surfaces, excluding the one-pixel seams."
    ),
    stable=True,
)

stable.fn(
    "Material_CheckHasTransparency",
    "53 8B 5C 24 10 56 57 33 C0 33 FF 85 DB 7E ?? 8B 74 24 14 8B 4C 24 10 33 D2 85 F6 7E ?? 80 3C 08 00",
    ret="bool",
    params=[
        param("uint8_t*", "pixel_data"),
        param("int32_t", "width"),
        param("int32_t", "height"),
    ],
    doc="Returns true when any RGBx pixel has all three color channels equal to zero.",
    stable=True,
)

stable.fn(
    "Material_LoadTexture",
    "B8 48 00 08 00 E8 ??",
    ret="void",
    params=[param("Material_TableEntry*", "material_entry")],
    doc=(
        "Expands indexed or 16-bit material pixels into RGBA storage, computes average transparent "
        "color, uploads two texture surfaces, and sets flag 0x00400000 when transparency is found."
    ),
    stable=True,
)

stable.fn(
    "D3D_SetBlendMode",
    "53 56 8B 74 24 0C 85 F6 0F 85 ??",
    hook=0x6,
    ret="uint8_t",
    params=[param("int32_t", "blend_mode")],
    doc=(
        "Configures cached D3D7 blend and texture-stage state. Returns diffuse alpha 0xFF for modes "
        "0/2, 0x40 for modes 3/4, and 0x80 for other nonzero modes."
    ),
    stable=True,
)

stable.fn(
    "D3D_SetTextureColorOperation",
    "51 A1 ?? ?? ?? ?? 8D",
    hook=0x6,
    ret="void",
    params=[
        param(
            "int32_t",
            "color_operation",
            doc="D3DTEXTUREOP value for texture stage 0 D3DTSS_COLOROP. Known callers pass 1 (disable) or 4 (modulate).",
        )
    ],
    doc="Sets texture-stage-0 D3DTSS_COLOROP only when its current value differs from color_operation.",
    stable=True,
)

stable.fn(
    "Graphics_RenderTexturedQuad",
    "?? ?? ?? C0 88 44 24 10",
    match=-0x4E,
    hook=0x6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "Material_TableEntry*",
            "material",
            doc="Runtime material record. Position 0 supplies flags and position 4 supplies the texture descriptor/handle record.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Render flags. Bits 24-26 request blend mode, falling back to material flags bit 0x80 when absent.",
        ),
        param(
            "uint32_t",
            "top_color",
            doc="Packed color used for the top two quad vertices. Alpha is supplied by D3D_SetBlendMode.",
        ),
        param(
            "uint32_t",
            "bottom_color",
            doc="Packed color used for the bottom two quad vertices. Alpha is supplied by D3D_SetBlendMode.",
        ),
        param(
            "int32_t",
            "quad_size",
            doc="Width/height in pixels added to packedXY to form the bottom-right corner.",
        ),
    ],
    doc="Draws an FVF-0x144 textured quad. Selected blend mode supplies diffuse alpha for all four vertices.",
)

stable.fn(
    "Graphics_RenderTexturedQuadMaterialSize",
    "33 C9 8A 48 02 8B E9 ??",
    match=-0x33,
    hook=0x6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "Material_TableEntry*",
            "material",
            doc="Material/render entry. Texture/frame data at +4 supplies width/height bytes.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Render flags. High blend bits can override material blend mode.",
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
    doc="Draws a textured quad sized from material dimensions. Selected blend mode supplies vertex diffuse alpha.",
)

stable.fn(
    "Graphics_RenderTexturedSprite",
    "F6 57 3B DE ?? ?? BB ??",
    match=-0x11,
    hook=0x6,
    ret="void",
    params=[
        param(
            "uint32_t", "packed_xy", doc="Packed signed 16-bit screen-space x/y origin."
        ),
        param(
            "uint32_t",
            "packed_wh",
            doc="Packed signed 16-bit width/height. Non-positive components fall back to texture dimensions or 1.",
        ),
        param(
            "Graphics_SpriteContext*",
            "sprite",
            doc="Sprite/font render context. Null uses the default font_glyph_render_state.",
        ),
        param(
            "uint32_t",
            "render_flags",
            doc="Blend/color/UV-orientation flags. Bits 24-26 select blend, 0x18000000 permutes UVs, 0x40000000 requests gradient colors, 0x80000000 supplies packed RGB.",
        ),
        param(
            "uint32_t",
            "rotation_angle",
            doc="Low 12 bits are fixed-angle units. Zero disables rotation.",
        ),
        param(
            "uint32_t",
            "rotation_pivot_xy",
            doc="Packed signed 16-bit x/y pivot used when rotation_angle is nonzero.",
        ),
    ],
    doc=(
        "Draws a sprite or glyph strip, using blend-mode return as diffuse alpha. Supports fallback "
        "dimensions, UV permutation, gradients, and rotation around rotation_pivot_xy."
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
            doc="Passed to D3DRENDERSTATE_ZWRITEENABLE. Callers use 0 then 1 around depth-write-suppressed draws.",
        ),
    ],
    doc="If the D3D render-state capability flag is set, calls SetRenderState on IDirect3DDevice7(D3DRENDERSTATE_ZWRITEENABLE, enable). Otherwise no-ops.",
)

stable.fn(
    "Graphics_ClipPolygonByPlane",
    "C9 89 4C 24 18 0F 8E ??",
    match=-0x58,
    hook=0x7,
    ret="BOOL",
    params=[
        param("Graphics_ClipVertex*", "input_vertices"),
        param("Graphics_ClipAttribute*", "input_attributes"),
        param("Graphics_ClipPlane*", "clip_plane"),
        param("Graphics_ClipVertex*", "output_vertices"),
        param("Graphics_ClipAttribute*", "output_attributes"),
        param("int32_t*", "in_out_vertex_count"),
    ],
    doc=(
        "Clips a polygon against one plane, updating in_out_vertex_count while copying inside vertices "
        "and emitting interpolated edge intersections into the output vertex and attribute buffers. "
        "Returns 1 when at least three vertices remain, otherwise 0."
    ),
    stable=True,
)

stable.fn(
    "Graphics_ClipPolygonByCameraPyramid",
    "24 14 8B 74 24 18 B8 ??",
    match=-0x12,
    ret="BOOL",
    params=[
        param("Graphics_ClipVertex*", "input_vertices"),
        param("Graphics_ClipAttribute*", "input_attributes"),
        param("Graphics_ClipVertex*", "output_vertices"),
        param("Graphics_ClipAttribute*", "output_attributes"),
        param("int32_t*", "in_out_vertex_count"),
    ],
    doc="Clips a polygon through the camera clipping plane slab using Graphics_ClipPolygonByPlane "
    "and local temporary buffers. Returns false as soon as fewer than three vertices remain.",
    stable=True,
)

stable.fn(
    "Math_CalculateFaceNormal",
    "44 24 14 D8 CB DE E9 ??",
    match=-0x3E,
    ret="Math_Vec3F*",
    params=[
        param("Math_Vec3F*", "out_normal"),
        param("Math_Vec3F const*", "point_0"),
        param("Math_Vec3F const*", "point_1"),
        param("Math_Vec3F const*", "point_2"),
    ],
    doc="Computes and normalizes the face normal from three 3D points, writes it to outNormal, and returns outNormal.",
)

stable.fn(
    "Camera_SetupClipPlanes",
    "83 EC 34 DB 44 24 3C 8B 44 24 38 8B 15 ??",
    hook=0x7,
    ret="void",
    params=[
        param("float", "projection_depth_or_focal"),
        param("int32_t", "viewport_w"),
        param("int32_t", "viewport_h"),
    ],
    doc="Builds the near and four side clip planes from the projection parameter and viewport dimensions.",
    stable=True,
)

stable.fn(
    "Math_SnapVertexToNearestPoint",
    "53 8B 5C 24 14 55 33 C9 56 57 85 DB 0F 8E ??",
    ret="void",
    params=[
        param("float*", "x"),
        param("float*", "y"),
        param("Graphics_ProjectedVertex*", "screen_vertices"),
        param("int32_t", "vertex_count"),
    ],
    doc="Snaps x/y to the first nearby Graphics_ProjectedVertex in the screen-vertex array whose screen_x/screen_y are both within 2 pixels.",
)

stable.fn(
    "Graphics_ClipAndDrawPolygon",
    "D1 38 D9 41 3C D8 1D ??",
    match=-0x32,
    hook=0x7,
    ret="void",
    params=[
        param("Graphics_PolygonBatchRecord*", "batch"),
        param("void*", "unused_vertex_buffer"),
        param("uint8_t", "alpha_byte"),
        param("int32_t", "unused_batch_alias"),
        param("int32_t", "brighten_colors"),
    ],
    doc=(
        "Builds, clips, and draws polygon vertices from batch data. alpha_byte supplies diffuse "
        "alpha, brighten_colors doubles and clamps RGB, and parameters 2 and 4 are unused."
    ),
    stable=True,
)

stable.fn(
    "Graphics_DrawQuad",
    "00 00 00 74 ?? 39 3D ??",
    match=-0x21,
    hook=0x8,
    ret="int32_t",
    params=[param("Graphics_PolygonBatchRecord*", "batch")],
    doc="Draws one transformed polygon batch, propagating blend-mode alpha through terminal/clipped vertices. Returns 1.",
)

stable.fn(
    "Graphics_SelectTextureLOD",
    "A1 ?? ?? ?? ?? 8B 54 24 04 8B",
    ret="int32_t",
    params=[param("Graphics_PolygonBatchRecord*", "batch")],
    doc="Selects a texture LOD/render bucket from graphics capability flags and the polygon batch screen-depth fields. Returns -1 when no LOD bucket applies.",
)

stable.fn(
    "Debug_Log",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 44 24 08",
    ret="void",
    params=[param("char const*", "category"), param("char const*", "message")],
    doc="Writes each non-null category and message as a newline-terminated entry to the open debug log, then flushes it.",
    stable=True,
)

stable.fn(
    "D3D_InitDirectDrawAndDirect3D",
    "A1 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81",
    ret="BOOL",
    params=[param("HWND", "hwnd")],
    doc="Initializes DirectDraw and Direct3D for the main game window, creating the primary/back/z-buffer surfaces and selected Direct3DDevice7. One internal 32-byte stack slot is reused first for the callback-selected DDraw_PixelFormat and later for the D3D_Viewport7 passed to SetViewport.",
    stable=True,
)

stable.fn(
    "D3D_EnumZBufferFormatCallback",
    "56 8B 74 24 08 81 7E 04 00 04 00 00 75 ?? 57 8B 7C 24 10 B9 08 00 00 00 33 C0 F3 A5 5F 5E C2 08 00 B8 01 00 00 00 5E C2 08 00 90 90 90 90 90 90 A1 ??",
    cc=CallingConvention.STDCALL,
    ret="HRESULT",
    params=[
        param("DDraw_PixelFormat*", "pixel_format"),
        param("void*", "selected_format_context"),
    ],
    doc=(
        "EnumZBufferFormats callback. Copies a format whose flags equal 0x400 into the opaque context "
        "and returns HRESULT D3DENUMRET_CANCEL/OK."
    ),
    public=False,
    unstable=True,
)

stable.fn(
    "D3D_ReleaseAllAndReportLeaks",
    "A1 ?? ?? ?? ?? 56 33 F6 3B C6 74 ?? 8B",
    ret="void",
    params=[],
    doc="Releases and clears D3D interfaces plus Z, back, and primary surfaces, restoring display mode and capture state when needed. Logs the active texture count.",
    stable=True,
)

stable.fn(
    "Graphics_DrawRectangle",
    "D3 89 74 24 10 C1 E8 ??",
    match=-0x30,
    hook=0x7,
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
            doc="Low word is a 0 through 0x1000 color scale. High bits select blend behavior.",
        ),
        param(
            "uint32_t",
            "rgb_color",
            doc="Packed RGB source color. Final diffuse alpha is supplied by blend state.",
        ),
    ],
    doc=(
        "Unpacks the rectangle, derives color scale and blend state from blend_flags, and draws a "
        "D3DPT_TRIANGLESTRIP with FVF 0x44. It assumes the global device is valid, ignores render "
        "HRESULTs, and restores depth writes to enabled rather than restoring the prior value."
    ),
)

stable.fn(
    "Graphics_DrawFilledRectangleGradient",
    "00 89 54 24 14 C1 E8 ??",
    match=-0x4C,
    hook=0x7,
    ret="void",
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
            "uint32_t",
            "top_left_rgb",
            doc="Packed RGB color for the top-left vertex. Alpha comes from blend state.",
        ),
        param(
            "uint32_t", "top_right_rgb", doc="Packed RGB color for the top-right vertex."
        ),
        param(
            "uint32_t", "bottom_left_rgb", doc="Packed RGB color for the bottom-left vertex."
        ),
        param(
            "uint32_t", "bottom_right_rgb", doc="Packed RGB color for the bottom-right vertex."
        ),
    ],
    doc="Draws a filled screen-space rectangle as D3DPT_TRIANGLESTRIP/FVF 0x44 with top-left, top-right, bottom-left, and bottom-right diffuse colors. Alpha comes from blend state.",
)

stable.fn(
    "Graphics_DrawFadeOverlay",
    "7C ?? 18 8B C8 C7 05 ??",
    match=-0x2C,
    hook=0x7,
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
            doc="0 through 0x1000 fixed-point opacity. 0x1000 maps to alpha 255.",
        ),
    ],
    doc="Draws a black alpha-blended rectangle strip. opacity_12 is shifted and truncated to an 8-bit alpha without clamping, so values outside 0 through 0x1000 wrap.",
)

stable.fn(
    "Graphics_DrawFullscreenFadeOverlay",
    "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 8B 44 24 04",
    hook=0x6,
    ret="void",
    params=[param("int32_t", "opacity_12")],
    doc=(
        "Draws a full-window software fade overlay from origin (0,0) using the current window "
        "dimensions and caller-supplied Q12 opacity."
    ),
    stable=True,
)

stable.fn(
    "Graphics_DisabledFadeBackendStub",
    "32 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 81 EC 80 00 00 00 A1 ??",
    hook=hook(0x2, kind=HookKind.HOTPATCH),
    ret="BOOL",
    params=[param("int32_t", "fade_level")],
    doc="Three-byte no-op stub that ignores fade_level and always returns FALSE. Loading-screen and world-select callers continue through the software fade-overlay path.",
)

stable.fn(
    "Graphics_DrawFullscreenTextureQuadrants",
    "81 EC 80 00 00 00 A1 ??",
    hook=0x6,
    ret="void",
    params=[param("Material_BlendTextureSet*", "texture_set")],
    doc=(
        "Renders the four pre-uploaded 256x256 surfaces in texture_set as D3DPT_TRIANGLESTRIP "
        "quadrants with FVF 0x144. The loading pipeline fills them from one 640x480 source image."
    ),
    stable=True,
)

stable.fn(
    "D3D_ClearViewport",
    "?? 6A 00 68 00 00 80 3F 6A 00 8B 08 6A 03",
    match=-0x4,
    ret="int32_t",
    params=[],
    doc="Clears the current viewport target and Z buffer through Clear on IDirect3DDevice7 and forwards its HRESULT. Requires an initialized global device.",
)

stable.fn(
    "Graphics_ClearDepthBuffer",
    "?? 6A 00 68 00 00 80 3F 6A 00 8B 08 6A 02",
    match=-0x4,
    ret="int32_t",
    params=[],
    doc="Clears only the current viewport Z buffer through Clear on IDirect3DDevice7 and forwards its HRESULT. Requires an initialized global device.",
)

stable.fn(
    "D3D_CloseDebugLog",
    "E8 ?? ?? ?? ?? 83 C4 1C C3",
    match=-0x37,
    ret="int32_t",
    params=[],
    doc=(
        "Writes the closing D3D.log entry and closes the global log handle without validating or "
        "clearing it. D3D initialization registers this handler each time, so repeated setup also "
        "registers repeated closes."
    ),
)

stable.fn(
    "D3D_InitializeDirectDraw",
    "81 EC 7C 02 00 00 C7 05 ??",
    required=Required.EN,
    hook=0x6,
    ret="void",
    params=[],
    doc="Opens D3D.log and registers its close handler, then enumerates/selects a driver, creates IDirectDraw7, queries a full 0x17C-byte DDCAPS, and records gamma support from dwCaps2 & DDCAPS2_PRIMARYGAMMA. Log-open, selector, and GetCaps results are unchecked.",
)

stable.fn(
    "D3D_ReleaseDirectDrawDevice",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 08",
    ret="ULONG",
    params=[],
    doc="Releases the global IDirectDraw7 interface, clears the global pointer, and returns the COM Release refcount or 0 when no interface was present.",
)

stable.fn(
    "Graphics_ClearScreenWithColor",
    "56 8B 35 ?? ?? ?? ?? 57 8B 3D ?? ?? ?? ?? 6A",
    hook=0x7,
    ret="void",
    params=[param("uint32_t", "rgb_color")],
    doc=(
        "Draws a solid rectangle from the viewport origin through width+1,height+1 while depth "
        "writes are disabled. It restores depth writes to enabled rather than the prior value."
    ),
    stable=True,
)

stable.fn(
    "Camera_SetupProjection",
    "0C 89 44 24 0C F7 35 ??",
    match=-0xA,
    hook=0x6,
    ret="void",
    params=[param("Camera_Runtime*", "camera")],
    doc=(
        "Builds and submits the camera projection transform, then updates clip-plane inputs "
        "and reciprocal-Z state. Callers ignore the residual result of clip-plane setup."
    ),
)

stable.fn(
    "Graphics_TakeScreenshot",
    "81 EC 04 01 00 00 56 33 F6 56 8D 44 24 08 68 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc=(
        "Finds the first unused sshot%03d.bmp filename and passes it to File_SaveScreenshot."
    ),
)

stable.fn(
    "Graphics_ConvertSurface565ToBGR24",
    "81 EC 84 00 00 00 55 56 57 B9 1F 00 00 00 33 C0",
    hook=0x6,
    public=False,
    ret="uint8_t*",
    params=[param("DDraw_IDirectDrawSurface7*", "surface")],
    doc="Locks surface without checking the HRESULT, assumes positive-pitch 640x480 RGB565, and writes 0xE1000 tightly packed BGR24 bytes. Source traversal starts at lpSurface + lPitch*480 and then subtracts one pitch per row, reading logical rows 480 through 1.",
)

stable.fn(
    "DDraw_UnlockSurface",
    "8B 44 24 04 6A 00 50 8B 08 FF 91 80 00 00 00 C3 83 EC 18 53 55 56 57 6A 28 6A 40 FF 15 ??",
    hook=0x6,
    ret="HRESULT",
    params=[param("DDraw_IDirectDrawSurface7*", "surface")],
    doc="Calls Unlock on IDirectDrawSurface7(NULL) for the whole surface and forwards its HRESULT. Surface must be a valid locked interface.",
)

stable.fn(
    "File_SaveScreenshot",
    "83 EC 18 53 55 56 57 6A 28 6A 40 FF 15 ??",
    ret="void",
    params=[param("const char*", "path")],
    doc="Allocates BMP headers, converts and unlocks the 640x480 backbuffer, then writes 14+40+0xE1000 BGR24 bytes. BfOffBits=0x36 and row alignment are correct, but biSizeImage=0x70800 and bfSize=0x70836 are exactly half the actual 0xE1000 payload and 0xE1036 file sizes.",
    stable=True,
)

stable.fn(
    "D3D_SetGammaRamp",
    "D9 44 24 04 D8 1D ?? ?? ?? ?? 8B",
    hook=0xA,
    public=False,
    ret="int32_t",
    params=[param("float", "linear_ramp_scale")],
    doc="Caches linear_ramp_scale, clamps values below 0.1 to 0.1, and replaces direct values above 10.0 with 5.0 before building an equal-channel DDGAMMARAMP. Each channel entry is clamp(i * scale - 1, 0, 65535).",
    stable=True,
)


stable.fn(
    "DDraw_CompareDisplayModes",
    "8B 54 24 04 56 8B 74 24 0C 8B 42 0C 8B 4E 0C 3B C1 73 ?? 83 C8 FF 5E C3 76 ?? B8 01 00 00 00 5E C3 8B 42 08",
    ret="int32_t",
    params=[
        param("DDraw_SurfaceDesc2 const*", "left"),
        param("DDraw_SurfaceDesc2 const*", "right"),
    ],
    doc=(
        "Qsort comparator ordering display modes ascending by width, then height, then RGB bit "
        "count from DDSURFACEDESC2."
    ),
    stable=True,
)

stable.fn(
    "D3D_EnumerateDirectDrawDevices",
    "8B 44 24 04 56 6A 07 6A 00 68 ??",
    ret="HRESULT",
    params=[param("D3D_DeviceAcceptCallback", "unused_device_accept_callback")],
    doc="Invokes DirectDrawEnumerateExA with flags 7 and returns its device status. Enumeration counters persist between calls, and the 20-record accepted list has no capacity guard.",
)

stable.fn(
    "D3D_EnumDisplayModeCallback",
    "8B 44 24 08 56 8B 74 24 08 57 8B 88 ?? ?? ?? ?? 8B D1 C1 E2 ??",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    ret="HRESULT",
    params=[
        param("DDraw_SurfaceDesc2*", "surface_desc"),
        param("D3D_DriverInfo*", "parent_driver_info"),
    ],
    doc="Internal EnumDisplayModes on IDirectDraw7 HRESULT callback. Copies one DDSURFACEDESC2 into slots 0 through 79, increments mode_count, and returns DDENUMRET_CANCEL exactly when the new count reaches 80.",
    stable=True,
)

stable.fn(
    "D3D_EnumDriverCallback",
    "B8 94 2B 00 00 E8 ??",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    ret="BOOL",
    params=[
        param("Win32_GUID*", "guid"),
        param("char*", "driver_description"),
        param("char*", "driver_name"),
        param("void*", "context"),
        param("HMONITOR", "monitor"),
    ],
    doc="Internal DirectDrawEnumerateExA callback. Acquires DirectDraw7/Direct3D7, calls GetCaps on IDirectDraw7 at vtable +0x2c, stages up to 80 modes via EnumDisplayModes at +0x20, sorts them, then calls EnumDevices on IDirect3D7 at +0x0c.",
    stable=True,
)

stable.fn(
    "D3D_EnumDeviceCallback",
    "8B 0D ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 81",
    cc=CallingConvention.STDCALL,
    hook=0x6,
    callable=False,
    public=False,
    ret="HRESULT",
    params=[
        param("char*", "device_description"),
        param("char*", "device_name"),
        param("D3D_DeviceDesc7*", "device_desc"),
        param("D3D_DriverInfo*", "parent_driver_info"),
    ],
    doc="Handles IDirect3D7 device enumeration and records usable devices. The 20-record list has no capacity guard, so a later accepted device can write past it.",
    stable=True,
)

stable.fn(
    "D3D_GetEnumeratedDevices",
    "08 85 C0 74 08 8B 0D ??",
    match=-0x11,
    hook=0x6,
    public=False,
    ret="void",
    params=[
        param("D3D_DriverInfo**", "device_list_out"),
        param("uint32_t*", "device_count_out"),
    ],
    doc="Writes the 20-record device-list base and unchecked accepted_count DWORD to independently optional output pointers. The count can exceed 20 after repeated enumeration or callback overflow.",
    stable=True,
)

stable.fn(
    "D3D_SelectDefaultDevice",
    "8D 4C 24 20 50 51 E8 ??",
    match=-0x32,
    public=False,
    ret="HRESULT",
    params=[
        param("D3D_DriverInfo**", "selected_device_out"),
        param("uint32_t", "flags"),
    ],
    doc="Selects desktop-compatible hardware-TnL, hardware, software, then reference devices, returning E_INVALIDARG for a null output or 0x81000004 when none exists. Flags bit 0 is D3DENUM_SOFTWAREONLY and skips both hardware classes.",
)

stable.fn(
    "D3D_AcceptAnyDevice",
    "B8 01 00 00 00 C3 90 90 90 90 90 90 90 90 90 90 51 68 ??",
    callable=False,
    public=False,
    ret="HRESULT",
    params=[
        param("DDCAPS*", "driver_caps"),
        param("D3D_DeviceDesc7*", "device_desc"),
    ],
    doc="Internal cdecl accept-all device-policy callback. Ignores the DDCAPS and D3DDEVICEDESC7 pointers and returns D3DENUMRET_OK.",
    stable=True,
)

stable.fn(
    "D3D_EnumerateAndSelectDefaultDevice",
    "51 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8D",
    hook=0x6,
    public=False,
    ret="D3D_DriverInfo*",
    params=[],
    doc="Enumerates and selects the default device while ignoring both results. Failure can return an indeterminate pointer, and repeated calls retain enumeration counters.",
    stable=True,
)

stable.fn(
    "D3D_FormatDirectXError",
    "8B 44 24 04 3D E0 01 76 88 0F 8F ??",
    hook=0x9,
    ret="char*",
    params=[param("HRESULT", "error_code"), param("char*", "out_buffer")],
    doc="Formats a DirectX HRESULT into one mutable global buffer, using empty text for success and an unknown-error fallback when unmapped. A non-null out_buffer receives an unbounded copy.",
    stable=True,
)

stable.fn(
    "DInput_CreateInterface",
    "68 00 07 00 00 51 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="DInput_IDirectInputA*",
    params=[param("HINSTANCE", "h_instance")],
    doc="Calls DirectInputCreateA(h_instance, 0x700, &local, NULL) and returns the interface only for DI_OK. Other HRESULTs are normalized to NULL.",
    stable=True,
)

stable.fn(
    "DInput_CreateDevice2A",
    "?? 8B F0 8B 44 24 0C 83",
    match=-0x27,
    hook=0x8,
    ret="DInput_IDirectInputDevice2A*",
    params=[
        param("DInput_IDirectInputA*", "dinput_interface"),
        param(
            "Win32_GUID",
            "device_guid",
            doc="Native helper receives the GUID by value, then passes its stack address to CreateDevice on IDirectInputA.",
        ),
    ],
    doc="Receives a DirectInput GUID by value and returns NULL if CreateDevice fails. CreateDevice first outputs an IDirectInputDeviceA base interface.",
    stable=True,
)

stable.fn(
    "DInput_QueryDevice2AInterface",
    "8B 44 24 04 8D 54 24 04 52 68 ??",
    hook=0x8,
    ret="DInput_IDirectInputDevice2A*",
    params=[param("DInput_IDirectInputDeviceA*", "base_device")],
    doc=(
        "Queries IID_IDirectInputDevice2A on base_device using the incoming parameter stack slot "
        "as the Device2A output, ignores the HRESULT, and returns that slot without validation."
    ),
    stable=True,
)

stable.fn(
    "Input_SetDeviceDataFormat",
    "8B 44 24 04 8B 54 24 08 52 50 8B 08 FF 51 ?? F7 D8 1B C0 40 C3",
    hook=0x8,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("void const*", "data_format"),
    ],
    doc="Calls SetDataFormat on IDirectInputDevice and normalizes DI_OK to 1 and every other HRESULT to 0. Device and data_format must be valid.",
    stable=True,
)

stable.fn(
    "Input_AcquireDevice",
    "8B 44 24 04 50 8B 08 FF 51 1C F7 D8 1B C0 40 C3 56 57 8B 7C 24 10 57 E8 ??",
    ret="BOOL",
    params=[param("DInput_IDirectInputDeviceA*", "device")],
    doc="Calls Acquire on IDirectInputDevice and normalizes DI_OK to TRUE and every other HRESULT to FALSE. Device must be valid.",
    stable=True,
)

stable.fn(
    "Input_GetDeviceStateBuffer",
    "56 57 8B 7C 24 10 57 E8 ?? ?? ?? ?? 83 C4 04 8B",
    hook=0x6,
    ret="void*",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("DWORD", "state_size"),
    ],
    doc=(
        "Allocates state_size bytes, passes the allocation to GetDeviceState without a null check, "
        "and transfers caller ownership on success. A negative GetDeviceState HRESULT returns NULL "
        "without freeing the allocation."
    ),
)

stable.fn(
    "DInput_EnumJoystickDeviceCallback",
    "08 B8 01 00 00 00 A3 ??",
    cc=CallingConvention.STDCALL,
    match=-0x39,
    hook=0x6,
    ret="BOOL",
    params=[
        param("DIDEVICEINSTANCEA const*", "device_instance"),
        param("DInput_DeviceEnumContext*", "enum_context"),
    ],
    doc="Copies device_instance.guidInstance into enum_context.guid_list, increments the count, marks that a device was seen, and returns DIENUM_CONTINUE. The context has no capacity check.",
)

stable.fn(
    "Input_SetJoystickAxisRangeProperty",
    "83 EC 18 8B 44 24 20 8B 54 24 28 8B 4C 24 24 89 44 24 08 8B 44 24 1C 89 54 24 14 8D 54 24 00 89 4C 24 10 8B 08 52 6A 04 50",
    public=False,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("DWORD", "axis_offset"),
        param("LONG", "min_value"),
        param("LONG", "max_value"),
    ],
    doc="Builds DIPROPRANGE for axis_offset with DIPH_BYOFFSET, calls SetProperty on IDirectInputDevice(DIPROP_RANGE), and returns TRUE only for DI_OK. The helper does not validate device or min/max ordering.",
    unstable=True,
)

stable.fn(
    "Input_SetJoystickXAxisRange",
    "04 50 51 6A 00 52 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc=(
        "Sets DIPROP_RANGE by DIJOFS_X and returns 1 only for DI_OK. The helper does not validate "
        "device or min/max ordering."
    ),
)

stable.fn(
    "Input_SetJoystickYAxisRange",
    "04 50 51 6A 04 52 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc=(
        "Sets DIPROP_RANGE by DIJOFS_Y and returns 1 only for DI_OK. The helper does not validate "
        "device or min/max ordering."
    ),
)

stable.fn(
    "Input_SetJoystickZAxisRange",
    "04 50 51 6A 08 52 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc=(
        "Sets DIPROP_RANGE by DIJOFS_Z and returns 1 only for DI_OK. The helper does not validate "
        "device or min/max ordering."
    ),
)

stable.fn(
    "Input_SetJoystickRzAxisRange",
    "04 50 51 6A 14 52 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="BOOL",
    params=[
        param("DInput_IDirectInputDeviceA*", "device"),
        param("int32_t", "min_value"),
        param("int32_t", "max_value"),
    ],
    doc=(
        "Sets DIPROP_RANGE by DIJOFS_RZ and returns 1 only for DI_OK. The helper does not validate "
        "device or min/max ordering."
    ),
)

stable.fn(
    "DInput_EnumerateForceFeedbackJoysticks",
    "01 00 00 52 8B 08 68 ??",
    match=-0x15,
    hook=0x7,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("Win32_GUID*", "device_guid_buffer"),
    ],
    doc="Enumerates attached force-feedback joysticks with flags 0x101, stores GUID entries, and returns their count while ignoring EnumDevices HRESULT. It sets availability from count != 0 before device/effect setup.",
)

stable.fn(
    "DInput_EnumerateAttachedJoysticks",
    "0C 6A 01 52 8B 08 68 ??",
    match=-0x12,
    hook=0x7,
    ret="int32_t",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("Win32_GUID*", "device_guid_buffer"),
    ],
    doc=(
        "Enumerates attached joysticks with flag 1 through DInput_EnumJoystickDeviceCallback, "
        "stores GUID entries, and returns their count."
    ),
)

stable.fn(
    "Input_SetExclusiveForegroundCooperativeLevel",
    "8B 44 24 08 8B 4C 24 04 6A 05 50 51 E8 ??",
    hook=0x8,
    ret="BOOL",
    params=[param("DInput_IDirectInputDeviceA*", "device"), param("HWND", "hwnd")],
    doc="Calls SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND) and returns the normalized BOOL result. Device and hwnd must be valid.",
)

stable.fn(
    "Input_SetJoystickDataFormat",
    "8B 44 24 04 68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 83 C4 08 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B",
    hook=0x9,
    ret="BOOL",
    params=[param("DInput_IDirectInputDeviceA*", "device")],
    doc="Sets c_dfDIJoystick and returns 1 only for DI_OK. Device must be valid.",
)

stable.fn(
    "Input_SetJoystick2DataFormat",
    "56 8B 06 FF 50 64 6A 50 56 E8 ?? ?? ?? ?? 83 C4 08 85 C0",
    match=-0x25,
    hook=0x9,
    ret="BOOL",
    params=[param("DInput_IDirectInputDeviceA*", "device")],
    doc="Sets c_dfDIJoystick2 and returns 1 only for DI_OK. Device must be valid.",
)

stable.fn(
    "Input_PollJoystickState",
    "56 8B 06 FF 50 64 6A 50 56 E8 ?? ?? ?? ?? 83 C4 08 85 C0",
    match=-0x5,
    ret="Input_JoystickState*",
    params=[param("DInput_IDirectInputDevice2A*", "device")],
    doc="Polls and reads a caller-owned joystick state buffer, retrying once after acquisition. The first failed read leaks its allocation, and final failure returns NULL.",
)

stable.fn(
    "Input_GetJoystickAxisX",
    "8B 44 24 04 8B 00 C3 90 90 90 90 90 90 90 90 90 8B 44 24 04 8B 40 ?? C3",
    hook=0x7,
    ret="int32_t",
    params=[param("Input_JoystickState*", "state")],
    doc="Returns DIJOYSTATE.lX without validating state.",
)

stable.fn(
    "Input_GetJoystickAxisY",
    "8B 44 24 04 8B 40 04 C3 90 90 90 90 90 90 90 90 8B 44 24 04 8B 40 14 C3 90 90 90 90 90 90 90 90 8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=0x7,
    ret="int32_t",
    params=[param("Input_JoystickState*", "state")],
    doc="Returns DIJOYSTATE.lY without validating state. Input_ReadGamepad uses it as vertical input.",
)

stable.fn(
    "Input_GetJoystickAxisRz",
    "8B 44 24 04 8B 40 14 C3 90 90 90 90 90 90 90 90 8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=0x7,
    ret="int32_t",
    params=[param("Input_JoystickState*", "state")],
    doc=(
        "Returns DIJOYSTATE.lRz without validating state. Input_ReadGamepad treats it as an "
        "alternate horizontal axis, though physical mapping depends on the controller."
    ),
)

stable.fn(
    "Input_GetJoystickButtonByte",
    "8B 44 24 08 8B 4C 24 04 8A 44 01 30 C3 90 90 90 8B 44 24 08 8B 4C 24 04 50 51 E8 ??",
    hook=0x8,
    ret="uint8_t",
    params=[param("Input_JoystickState*", "state"), param("int32_t", "button_index")],
    doc="Returns state.rgbButtons[button_index] without pointer or 0 through 31 index validation.",
)

stable.fn(
    "Input_IsJoystickButtonPressed",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 25",
    hook=0x8,
    ret="BOOL",
    params=[
        param("Input_JoystickState*", "joy_state"),
        param("int32_t", "button_index"),
    ],
    doc="Returns the high pressed-state bit from joy_state.rgb_buttons[button_index]. Pointer and button-index preconditions are inherited from Input_GetJoystickButtonByte.",
    stable=True,
)

stable.fn(
    "DInput_CreateConfiguredJoystickDevice",
    "00 00 00 57 50 56 E8 ??",
    match=-0x10,
    hook=0x6,
    ret="DInput_IDirectInputDevice2A*",
    params=[
        param("DInput_IDirectInputA*", "direct_input"),
        param("HWND", "hwnd"),
        param("int32_t", "device_index"),
        param("int32_t", "setup_mode"),
    ],
    doc="Enumerates into a local GUID[8], selects device_index, configures DIJOYSTATE/DIJOYSTATE2, cooperation, optional constant force, and acquisition. Enumeration has no eight-entry capacity check and negative indices address before the buffer.",
)

stable.fn(
    "DInput_SetConstantForceEffect",
    "A1 ?? ?? ?? ?? 83 EC 44",
    ret="HRESULT",
    params=[
        param("LONG", "direction_x"),
        param("LONG", "direction_y_and_base_magnitude"),
        param("DWORD", "duration_microseconds"),
    ],
    doc="Updates the global constant-force effect and calls SetParameters. Availability can remain true after setup fails, leaving the effect pointer NULL.",
)

stable.fn(
    "DInput_StartConstantForceEffect",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? A1 ?? ?? ?? ?? 6A 00 6A 01 50 8B 08 FF 51 1C C3",
    ret="HRESULT",
    params=[],
    doc="Returns zero when force feedback is unavailable, otherwise starts the global effect. Availability can remain true while the effect pointer is NULL.",
)

stable.fn(
    "Input_SetForceFeedbackScale",
    "8B 44 24 04 A3 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 44",
    hook=0x9,
    ret="void",
    params=[param("float", "scale")],
    doc="Stores the process-global force-feedback magnitude scale without validation.",
)

stable.fn(
    "DInput_CreateJoystickDevice",
    "04 6A 01 50 51 52 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="DInput_IDirectInputDevice2A*",
    params=[
        param("DInput_IDirectInputA*", "dinput_interface"),
        param("HWND", "hwnd"),
        param("int32_t", "device_index"),
    ],
    doc=(
        "Calls DInput_CreateConfiguredJoystickDevice with setup_mode 1, selecting DIJOYSTATE and "
        "force-feedback setup when available. Ownership and failure behavior come from that helper."
    ),
)

stable.fn(
    "DInput_InitializeJoystickInput",
    "8B 4C 24 08 8B 44 24 04 81 EC 80 00 00 00 A3 ??",
    hook=0x8,
    ret="BOOL",
    params=[param("HWND", "hwnd"), param("HINSTANCE", "h_instance")],
    doc="Creates the DirectInput interface and first joystick, then sets axis ranges while ignoring property failures. Setup continues after failures, which can leave availability true with a NULL effect.",
)

stable.fn(
    "Video_InitPlayer",
    "?? ?? ?? 33 C0 5E C3 56",
    match=-0x1B,
    ret="BOOL",
    params=[param("HWND", "hwnd")],
    doc=(
        "Initializes video then sound for hwnd. Either failure calls both subsystem shutdown paths "
        "and returns FALSE, relying on those shutdown routines to tolerate partial initialization."
    ),
)

stable.fn(
    "Video_CloseMovieFile",
    "A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 68",
    ret="BOOL",
    params=[],
    doc=(
        "Stops the movie timer, restores playback mode, then closes sound, playback, video, and "
        "movie state in fixed order. It performs no local handle/global validation and returns TRUE."
    ),
)

stable.fn(
    "Video_OpenMovieFile",
    "?? ?? ?? 8A 44 24 10 3C",
    match=-0x23,
    ret="BOOL",
    params=[
        param("HWND", "hwnd"),
        param("const char*", "movie_path"),
        param("DDraw_IDirectDraw7*", "unused_ddraw"),
        param("char", "use_alt_video_rect"),
    ],
    doc="Opens and maps an RPL movie, selects the alternate rectangle only when requested, gates audio setup on nonzero format fields, and starts playback. unused_ddraw is ABI-retained but unread.",
    unstable=True,
)

stable.fn(
    "Video_PlayMovieLoop",
    "A1 ?? ?? ?? ?? 83 EC 0C 53",
    ret="int32_t",
    params=[],
    doc=(
        "Runs playback until completion, playback error, joystick input, ESC, ENTER, or Alt+F4. "
        "Returns 2 for ESC, 3 for Alt+F4, and 1 for normal completion, playback error, ENTER, or "
        "joystick stop."
    ),
    stable=True,
)

stable.fn(
    "Video_ShutdownPlayerSystems",
    "E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 90 90 90 90 90 8B",
    ret="void",
    params=[],
    doc="Shuts down the video playback subsystem and then the sound playback subsystem.",
)

stable.fn(
    "Graphics_SetPolygonUVs",
    "8B 4D 04 3B CA 0F 84 ??",
    match=-0x42,
    hook=0x7,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_PolygonBatchRecord*", "out_batch"),
        param("Material_TableEntry*", "material_entry"),
        param("int16_t", "uv_index_or_sentinel"),
        param("Mesh_RuntimeVertex**", "polygon_vertices"),
    ],
    doc="Copies material and render state, then writes four packed UV pairs. A null texture exits early, while other branches trust all input pointers and indexes.",
)

stable.fn(
    "Graphics_RenderPolygonMesh",
    "55 8B EC 83 EC 70 A1 ??",
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Transforms actor geometry and links material batches into render buckets without drawing directly. Producers share a frame arena and do not check its capacity.",
    unstable=True,
)

stable.fn(
    "Graphics_RenderMeshNode",
    "55 8B EC 83 EC 64 A1 ??",
    hook=0x6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_SpriteNodeData*", "sprite_ctx"),
    ],
    doc="Transforms a scene billboard and inserts material-linked polygon batches into depth-sorted buckets. Material chains can emit several records, and the frame arena index has no guard.",
    unstable=True,
)

stable.fn(
    "Bone_ProcessExternalRef",
    "55 8B EC 83 EC 68 A1 ?? ?? ?? ?? 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_SpriteNodeData*", "sprite_ctx"),
    ],
    doc="Handles attachment table tag 7 by transforming a billboard and enqueueing material-linked polygon batches. Material chains can emit several records, and the frame arena index has no guard.",
)

stable.fn(
    "Graphics_IsPolygonInDebugList",
    "A1 ?? ?? ?? ?? 53 56 57 85 C0 7E",
    ret="BOOL",
    params=[param("Graphics_PolygonRenderRef*", "polygon_ref")],
    doc=(
        "Searches registered debug-polygon lists in Graphics_PolygonRenderRef-sized steps and returns "
        "TRUE when polygon_ref matches an entry. Encountering any list with a nonpositive count stops "
        "the outer search, so registered lists are expected to form a dense prefix."
    ),
    stable=True,
)

stable.fn(
    "Graphics_RenderPolygonBatch",
    "53 56 57 8B 58 70 A1 ?? ?? ?? ?? 89 5D CC",
    match=-0xC,
    hook=0x9,
    ret="void",
    params=[
        param(
            "void*",
            "render_owner",
            doc="Actor_State or static Scene_Node-like owner with the shared render-field prefix.",
        ),
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Transforms polygon references and links their material batches into sorted buckets. Callers must pass valid inputs and a positive count, while secondary materials can emit extra records without an arena guard.",
    unstable=True,
)

stable.fn(
    "Graphics_RenderSceneGeometryWrapper",
    "54 24 04 50 51 52 E8 ??",
    match=-0x9,
    hook=0x8,
    ret="void",
    params=[
        param(
            "void*",
            "render_owner",
            doc="Actor_State or static Scene_Node-like owner forwarded without reinterpretation.",
        ),
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Forwards the generic render owner and polygon batch to Graphics_RenderPolygonBatch as a callback-compatible alias.",
    stable=True,
)

stable.fn(
    "Graphics_RenderSceneGeometry",
    "55 8B EC 81 EC 88 00 00 00 F7 05 ??",
    required=Required.EN,
    hook=0x9,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc=(
        "Transforms and culls scene polygon references into depth-bucketed render-batch records. "
        "Delegates to Graphics_RenderSceneGeometryAlt when the alternate rendering capability is active."
    ),
    stable=True,
)

stable.fn(
    "Graphics_RenderSceneGeometryAlt",
    "00 00 F6 C2 04 0F 84 ??",
    match=-0x7F,
    hook=0x6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "count"),
    ],
    doc="Transforms scene polygon references into depth-bucketed render records using the alternate capability-derived vertex colors.",
    stable=True,
)

stable.fn(
    "Trail_RenderAnimated",
    "BF 77 1A 3B C6 0F 84 ??",
    match=-0x21,
    hook=0x6,
    ret="void",
    params=[param("Component_TrailObject*", "trail")],
    doc="Builds camera-facing animated trail polygon batches from Component_TrailObject/Trail_Segment data and links them into render buckets.",
)

stable.fn(
    "Bone_TransformWeightedVerticesForRender",
    "66 83 7A 0A 00 0F 8F ??",
    match=-0xC,
    hook=0x6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param(
            "Submesh_RenderSpan*",
            "weighted_span",
            doc="Weighted submesh render span selecting the weighted-vertex range to transform.",
        ),
        param(
            "Math_TransformQ12Q6 const*",
            "world_transform",
            doc="Q12 rotation and Q6 translation transform read from the owning scene node.",
        ),
    ],
    doc="Transforms weighted/skinned vertices for rendering with the node's compact world transform.",
    unstable=True,
)

stable.fn(
    "Scene_FinalizeNodeRender",
    "53 56 57 A8 80 0F 84 ??",
    match=-0xB,
    hook=0x6,
    ret="void",
    params=[param("Scene_Node*", "node")],
    doc="Finalizes a rendered scene node, using node render/material fields and invoking Graphics_RenderSpritePolygons(node, transformedVerts, projectedVerts).",
)

stable.fn(
    "Graphics_RenderSpritePolygons",
    "8D 04 90 84 C9 0F 84 ??",
    match=-0x47,
    hook=0x6,
    ret="void",
    params=[
        param("Scene_Node*", "node"),
        param("Graphics_SpriteVertexData*", "graphics_transformed_vertices"),
        param("Graphics_ProjectedVertex*", "projected_vertices"),
    ],
    doc="Projects and enqueues sprite polygon references from a scene node using "
    "transformed-vertex and projected-vertex local buffers. Callers ignore the residual "
    "return register.",
    unstable=True,
)

stable.fn(
    "Mesh_CalculateVertexNormals",
    "55 8B EC 83 EC ?? 8B 45 08 53 56 57 8D 70 ??",
    hook=0x6,
    ret="void",
    params=[
        param("Graphics_PolygonRenderRef*", "polygon_refs"),
        param("int32_t", "polygon_count"),
        param("Graphics_SpriteVertexData*", "graphics_transformed_vertices"),
        param("int32_t", "vertex_count"),
    ],
    doc=(
        "Accumulates per-vertex normals for render polygon refs when graphics bit 0x1000 is "
        "active. Quads average two triangle normals, triangles touch three vertices, and callers "
        "ignore the residual return value."
    ),
    unstable=True,
)

stable.fn(
    "Math_ApproxDistance3DWeighted",
    "8B 4C 24 04 85 C9 7D ?? F7 D9 8B 44 24 08 85 C0",
    hook=0x6,
    ret="int32_t",
    params=[
        param("int32_t", "dx"),
        param("int32_t", "dy"),
        param("int32_t", "dz"),
    ],
    doc="Returns (15*max/16) + (3*mid/8) + (9*min/32) for the absolute components, preserving the input coordinate scale.",
    stable=True,
)

stable.fn(
    "Math_SqrtFP12",
    "56 8B 74 24 08 57 33 C9 33 C0 BF 16 ?? ?? ??",
    hook=0x6,
    ret="int32_t",
    params=[param("uint32_t", "value")],
    doc="Returns floor(sqrt(value << 12)), producing a Q12 square root from an unsigned squared magnitude.",
    stable=True,
)

stable.fn(
    "Math_SqrtFP14",
    "56 8B 74 24 08 57 33 C9 33 C0 BF 17 ?? ?? ??",
    hook=0x6,
    ret="int32_t",
    params=[param("uint32_t", "value")],
    doc="Returns floor(sqrt(value << 14)), producing a Q14 square root from an unsigned squared magnitude.",
    stable=True,
)

stable.fn(
    "Physics_CalculateMovementWithCollision",
    "00 8B 08 89 94 24 E8 ??",
    match=-0xAE,
    hook=0xA,
    ret="int32_t",
    params=[
        param("Math_Vec3I32 const*", "start_pos"),
        param("Math_Vec3I32 const*", "end_pos"),
        param("Actor_State const*", "collision_actor"),
        param("int16_t", "probe_radius"),
        param("int32_t", "collision_flags"),
    ],
    doc=(
        "Sweeps the read-only Q6 segment from start_pos to end_pos using collision_actor's rotation "
        "when supplied. Returns a Q12 blocked/retraction fraction in [0, 0x1000], or zero when clear."
    ),
    stable=True,
)

stable.fn(
    "Collision_UpdateActorGroundBarycentric",
    "55 8B EC 83 EC 64 53 8B 5D 08 56 57 8B 83 E8 ??",
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Builds the uint16 selector and three signed Q12 weights in the grounded +0x20 overlay, then reconstructs actor Q6 world position. Triangles use (3,2,1).",
    stable=True,
)

stable.fn(
    "Collision_TransformGroundBarycentricToWorld",
    "41 20 55 56 8B B1 E8 ??",
    match=-0xD,
    hook=0x7,
    ret="void",
    params=[
        param("Actor_State const*", "actor_state"),
        param("Math_Vec3I32*", "out_world_pos"),
    ],
    doc="Combines signed Q0 vertices (3,0,1) or (3,2,1) with cached signed Q12 weights, then applies the node Q12 matrix. The Q0 result is shifted to Q6, translated by the Q6 node origin, and written to out_world_pos.",
    stable=True,
)

stable.fn(
    "Collision_ProjectPointOntoPolygonSurface",
    "55 8B EC 83 EC 30 56 8B 75 08 85 F6 0F 84 ??",
    hook=0x6,
    ret="void",
    params=[
        param("Collision_Node const*", "collision_node"),
        param("Collision_Polygon const*", "contact_poly"),
        param("Math_Vec3I32 const*", "world_pos"),
        param("Math_Vec3I32*", "out_world_pos"),
    ],
    doc="Projects signed Q6 world XZ onto the collision surface and writes signed Q6 world output. A null collision_node leaves output untouched.",
    stable=True,
)

stable.fn(
    "Math_CalculateTriangleHeight",
    "2C 66 8B 42 02 D8 0D ?? ?? ?? ??",
    match=-0x69,
    hook=0x7,
    ret="int32_t",
    params=[
        param("int32_t", "local_x_q6"),
        param("int32_t", "local_z_q6"),
        param("Collision_Vertex const*", "tri_a"),
        param("Collision_Vertex const*", "tri_b"),
        param("Collision_Vertex const*", "tri_c"),
    ],
    doc=(
        "Interpolates signed Q6 local X/Z over three signed Q0 vertices and returns signed Q6 Y with "
        "x87-to-integer truncation. A zero XZ determinant returns the maximum signed vertex Y shifted to Q6."
    ),
    stable=True,
)

stable.fn(
    "Collision_GetAdjacentPolygon",
    "74 ?? 08 8B C8 C1 E9 ??",
    match=-0x1A,
    hook=0x8,
    ret="Collision_Polygon*",
    params=[
        param("Collision_Node const*", "collision_node"),
        param("Collision_Polygon const*", "polygon"),
        param("int32_t*", "edge_index_in_out"),
    ],
    doc="Decodes packed uint16 adjacency. Bits 0 through 1 select the reverse edge and bits 2 through 15 encode a one-based polygon index.",
    stable=True,
)

stable.fn(
    "Collision_FindGroundPolygonUnderActor",
    "83 EC 2C 53 8B 5C 24 34 55 56 8B B3 E8 ??",
    hook=0x8,
    ret="Collision_Node*",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon**", "out_polygon"),
    ],
    doc="Finds or walks packed adjacency to the ground polygon under actor and returns its collision node. When out_polygon is null, current or newly entered polygons with flags & 0x1800 are skipped.",
    unstable=True,
)

stable.fn(
    "Collision_IsPointInsidePolygon",
    "88 00 00 00 22 0F 85 ??",
    match=-0x5A,
    ret="int32_t",
    params=[
        param("Collision_Node const*", "collision_node"),
        param("Collision_Polygon const*", "polygon"),
        param("int16_t", "point_x"),
        param("int16_t", "point_z"),
    ],
    doc="Tests local integer XZ using three edges when polygon flag 0x0001 is set and four when clear, honoring node transform flags 0x22. Across 88,174 shipped polygons, all 7,420 bit-set records repeat vertex index 0 in slot 3, while all 80,754 clear records keep slots 0 and 3 distinct.",
)

stable.fn(
    "Collision_FindIntersectingPolygonEdge",
    "F8 0C 83 F8 FC 0F 8E ??",
    match=-0x38,
    hook=0x7,
    ret="int32_t",
    params=[
        param("Collision_Node const*", "collision_node"),
        param("Collision_Polygon const*", "polygon"),
        param("Math_Vec3I16 const*", "from_point"),
        param("Math_Vec3I16 const*", "to_point"),
    ],
    doc="Returns the crossed local-space polygon edge index 0 through 3, or -1. It rejects Q12 normal-Y values at or below -4.",
)

stable.fn(
    "Collision_FindActorGroundContact",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 83",
    hook=0x8,
    ret="Collision_Node*",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Polygon* *", "out_polygon"),
    ],
    doc="Returns the actor's ground collision node and writes its ground polygon through out_polygon. Wall contacts are outside this wrapper.",
    unstable=True,
)

stable.fn(
    "Collision_ResolveActorGroundContact",
    "55 8B EC 83 EC 18 56 57 8B 7D 08 8B B7 E8 ??",
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Resolves cached, dynamic, and static ground contacts, seeding a missing-contact Q6 ground Y with -0x3FFFF800 and using a 0x555-Q6 vertical replacement band. Behavior bit 0x40 suppresses world-Y snapping.",
    stable=True,
)

stable.fn(
    "Collision_DetectDynamicObject",
    "55 8B EC 83 EC 18 A1 ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose position and current ground-contact state are tested against dynamic collision objects.",
        ),
        param(
            "int32_t*",
            "in_out_ground_y",
            doc="Current best signed Q6 ground Y. Updated with 0x555-Q6 replacement hysteresis when a dynamic polygon is selected.",
        ),
    ],
    doc="Scans enabled type-2 dynamic collision objects, rejecting cull, radius, vertical, and node-flag 0x0200 failures. vertex_count <= 0 substitutes the opaque pointer stored at node+0x70 as context.",
    stable=True,
)

stable.fn(
    "Settings_GetSfxVolume",
    "A1 ?? ?? ?? ?? 25 FF FF 00",
    ret="uint16_t",
    params=[],
    doc="Returns the persisted SFX volume in Q12 units, where 0x1000 is full volume.",
    stable=True,
)

stable.fn(
    "Settings_SetSfxVolume",
    "66 8B 44 24 04 66 A3 ?? ?? ?? ?? C3 90 90 90 90 33",
    ret="void",
    params=[param("uint16_t", "volume_fp12")],
    doc="Stores the persisted Q12 SFX volume.",
    stable=True,
)

stable.fn(
    "Settings_GetMusicVolume",
    "33 C0 66 A1 ?? ??",
    hook=0x8,
    ret="uint16_t",
    params=[],
    doc="Returns the persisted music volume in Q12 units, where 0x1000 is full volume.",
    stable=True,
)

stable.fn(
    "Settings_SetMusicVolume",
    "66 8B 44 24 04 66 A3 ?? ?? ?? ?? C3 90 90 90 90 E8",
    ret="void",
    params=[param("uint16_t", "volume_fp12")],
    doc="Stores the persisted Q12 music volume.",
    stable=True,
)

stable.fn(
    "Settings_IsSoundEnabled",
    "E8 ?? ?? ?? ?? F7 D8 1B C0 40 C3",
    ret="int32_t",
    params=[],
    doc="Returns nonzero when sound is enabled, inverting the audio backend's suppression flag.",
    stable=True,
)

stable.fn(
    "Settings_SetSoundEnabled",
    "51 A2 ?? ?? ?? ?? E8 ??",
    match=-0xB,
    hook=0x6,
    ret="void",
    params=[param("int32_t", "enabled")],
    doc="Stores the requested sound-enabled state after converting it to the backend suppression flag.",
    stable=True,
)

stable.fn(
    "Settings_SetRumbleSuppressFlag",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ??",
    hook=0x9,
    ret="void",
    params=[param("uint8_t", "suppress_rumble")],
    doc="Stores the rumble-suppression flag. 1 suppresses vibration and 0 allows it.",
    stable=True,
)

stable.fn(
    "Settings_GetRumbleSuppressFlag",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA",
    hook=0x7,
    ret="int32_t",
    params=[],
    doc="Returns the signed controller-vibration suppress flag. The pattern includes the two trailing setters and the byte-identical Save_IsGameComplete getter so it anchors the rumble getter instead of colliding with that routine.",
)

stable.fn(
    "Settings_GetLanguage",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA",
    required=Required.EU_SC,
    hook=0x7,
    ret="int32_t",
    params=[],
    doc=(
        "Returns the signed persisted language ID (Config_GameSettings.language). The pattern spans "
        "the byte-identical accessor neighborhood through the String_GetRuntimeByNegativeIndex head so it anchors "
        "the language getter instead of a twin."
    ),
)

stable.fn(
    "Settings_SetLanguage",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA",
    required=Required.EU_SC,
    hook=0x9,
    ret="void",
    params=[
        param(
            "char",
            "language_id",
            doc="Language ID. 0 English.",
        )
    ],
    doc=(
        "Stores the persisted language ID (Config_GameSettings.language). Called by the "
        "multi-language boot flow with the selected language and zeroed by Save_InitializeNewGame."
    ),
)

stable.fn(
    "Save_SetGameComplete",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ?? ?? ?? ?? 8D 0C 85 04 00 00 00 2B D1 8B 02 C3",
    hook=0x9,
    ret="void",
    params=[param("uint8_t", "complete")],
    doc="Stores the active save slot's campaign-complete flag.",
    stable=True,
)

stable.fn(
    "Save_IsGameComplete",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8B 44 24 04 BA ??",
    hook=0x7,
    ret="int32_t",
    params=[],
    doc="Returns the active save slot's campaign-complete flag as a signed value.",
    stable=True,
)

stable.fn(
    "String_GetRuntimeByNegativeIndex",
    "8B 44 24 04 BA ??",
    hook=0x9,
    public=False,
    ret="char*",
    params=[param("int32_t", "negative_string_id")],
    doc="Resolves the intentional dynamic-text token -1 through g_dynamicStringSlot0. The 29 nonempty shipped sprite lists contain 912 payload records plus 29 terminators.",
    stable=True,
)

stable.fn(
    "Level_PassthroughIndex",
    "8B 44 24 04 C3 90 90 90 90 90 90 90 90 90 90 90 8B 44 24 04 56 50 E8 ??",
    hook=hook(0x4, kind=HookKind.HOTPATCH, entry_patch_size=0x2),
    ret="int32_t",
    params=[param("int32_t", "level_id")],
    doc="Identity passthrough. Returns level_id unchanged.",
    stable=True,
)

stable.fn(
    "Menu_LoadLevelProgressState",
    "8B 44 24 04 56 50 E8 ?? ?? ?? ?? 8B F0 56 E8 ?? ?? ?? ?? 83 C4 08 83 FE 07 7C ?? 83 FE 1B 7D ?? 83 FE 0B 74 ?? 83 FE 10 74 ?? 83 FE 15 74 ?? 83 FE 1A 74 ?? 0F BE 88 ?? ?? ?? ?? 0F BE 80 ?? ?? ?? ?? 33 D2",
    ret="void",
    params=[param("int32_t", "level_id")],
    doc="Loads the selected level's saved score and completion count into menu state, delegating bonus levels to Level_InitializeBonusData.",
    stable=True,
)

stable.fn(
    "Level_GetSaveArrayIndex",
    "83 F9 1A 74 ?? 83 E9 ??",
    match=-0x1D,
    hook=0x7,
    ret="int32_t",
    params=[param("int32_t", "level_id")],
    doc="Maps campaign and bonus level IDs to compact save indices, returning -1 when unsupported. Bonus ID 32 arithmetically maps to slot 5 at +0x42, but the shipped name-entry check cannot reach this alias.",
    stable=True,
)

stable.fn(
    "Level_InitializeBonusData",
    "24 8D ?? ?? ?? ?? B9 ??",
    match=-0x28,
    hook=0x6,
    public=False,
    ret="void",
    params=[param("int32_t", "level_id"), param("int32_t", "save_index")],
    doc="Loads packed bonus initials or RFF/PR3/RFF/TOB defaults for IDs 27/28/29/31, publishes the selected name through g_dynamicStringSlot0, and unpacks its 6/6/4-bit codes. Shipped levels 28/31 each consume the slot through a type-2 -1 record.",
    stable=True,
)

stable.fn(
    "Save_SaveGameLevelCompletion",
    "0F BF 05 ?? ?? ?? ?? 53 56 50 E8 ??",
    hook=0x7,
    ret="void",
    params=[param("uint8_t", "completion_flag")],
    doc="Commits the current level's best score, completion, puppy, bone, and bonus progress to the active save slot. Level 31 owns the live +0x42 best-time write.",
    stable=True,
)

stable.fn(
    "Save_SnapshotWorldCompletionBits",
    "A3 ?? ?? ?? ?? 89 0D ?? ?? ?? ?? C3",
    match=-0x2D,
    ret="void",
    params=[],
    doc="Copies the five runtime world-completion bitfields into the save-staging snapshot globals.",
    stable=True,
)

stable.fn(
    "Save_BackupGamePuppyCount",
    "A0 ?? ?? ?? ?? A2 ?? ?? ?? ?? C3 90 90 90 90 90",
    ret="void",
    params=[],
    doc="Copies the runtime backup puppy count into the active save slot's puppy-count snapshot byte.",
    stable=True,
)

stable.fn(
    "Save_SetGameBackupPuppyCount",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BF 0D ?? ?? ?? ?? A0 ?? ?? ?? ??",
    ret="void",
    params=[param("uint8_t", "puppy_count")],
    doc="Stores the runtime backup puppy count later committed by Save_BackupGamePuppyCount.",
    stable=True,
)

stable.fn(
    "Level_InitializeSaveState",
    "?? ?? 8B F0 52 4E E8 ??",
    match=-0x33,
    hook=0x7,
    ret="void",
    params=[],
    doc="Loads persistent progress for the current level, including bonus initials for every ID 27 through 32. Level 31 reads +0x42 as best time.",
    unstable=True,
)

stable.fn(
    "Level_BuildCompletionTable",
    "33 F6 83 F8 04 0F 8D ??",
    match=-0x2C,
    ret="bool",
    params=[],
    doc="Builds four 24-bit per-world completion masks and a fifth aggregate mask, then writes the five cached masks. Returns true through AL only when the first four masks are 0xFFFFFF and the fifth is 0x3F.",
    stable=True,
)

stable.fn(
    "Save_InitializeNewGame",
    "68 00 10 00 00 C6 05 ?? ?? ?? ?? 01 E8 ??",
    ret="void",
    params=[],
    doc="Applies the default volume, sound, rumble, character, and difficulty settings for a new game.",
    stable=True,
)

stable.fn(
    "Save_LoadGameState",
    "8B 44 24 04 85 C0 75 ?? 68 DC 01 00 00 68 ??",
    hook=0x6,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "operation_step",
            doc="Zero starts the async save-file read. Nonzero finalizes the completed read and restores or initializes state.",
        )
    ],
    doc=(
        "With operation_step 0, starts an asynchronous 0x1DC-byte save-file read and returns 0. "
        "Later calls poll and close the operation, returning 2 after restoring a valid version-2 "
        "payload or 1 after preserving initialized settings or creating new-game state."
    ),
)

stable.fn(
    "Save_GetGameHighestWorld",
    "0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 57 B9 17 00 00 00 33 C0 BF ??",
    hook=0x7,
    ret="int32_t",
    params=[],
    doc="Returns the signed highest-world progression marker from the active save state.",
    stable=True,
)

stable.fn(
    "Save_InitializeGameState",
    "57 B9 17 00 00 00 33 C0 BF ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Clears active game progress, seeds the backup count and initialization marker, and resets highest-world and completion state.",
    stable=True,
)

stable.fn(
    "Save_ClearGameBonusProgressData",
    "A3 ?? ?? ?? ?? A3 ?? ?? ?? ?? C3",
    match=-0x11,
    hook=0x7,
    ret="void",
    params=[],
    doc="Clears the five cached world-completion masks used while initializing game state.",
    stable=True,
)

stable.fn(
    "Shared_LoadCommonResources",
    "A1 ?? ?? ?? ?? 85 C0 0F 85 ?? ?? ?? ?? 6A 00",
    required=Required.EN,
    ret="void",
    params=[],
    doc=(
        "Loads and fixes up the shared menu audio, strings, fonts, materials, and texture arrays. "
        "Returns early when already loaded or when a required resource fails."
    ),
    stable=True,
)

stable.fn(
    "String_InstallLocaleOverlay",
    "8B 0D ?? ?? ?? ?? 53 56 8B 74 24 0C 8B 41 08 57 03 C1 8B 08 8D 50 04 8B D9 8B FA C1 E9 02 F3 A5 8B CB 83 E1 03 F3 A4",
    required=Required.EU_SC,
    ret="void",
    params=[
        param(
            "void*",
            "locale_data",
            doc="Locale payload copied over the loaded overlay resource blob.",
        )
    ],
    abi_status=AbiStatus.PLACEHOLDER,
    doc=(
        "Copies the supplied locale payload into the loaded overlay resource at the blob's stored "
        "offset and installs the localized string table that follows the copied block."
    ),
)

stable.fn(
    "Save_CheckContinueSlotLanguage",
    "66 83 3D ?? ?? ?? ?? 00 74 0C 66 C7 05 ?? ?? ?? ?? 00 00 32 C0 C3 E8 ?? ?? ?? ?? 0F BE 0D ?? ?? ?? ?? 49 83 F9 03 77 EB FF 24 8D ?? ?? ?? ?? 85 C0 75 E0 B0 01 C3",
    required=Required.EU_SC,
    hook=0x8,
    ret="BOOL",
    params=[],
    doc="Returns TRUE when the saved language belongs to the language group selected at startup.",
)

stable.fn(
    "Menu_ShutdownResources",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? A1 ?? ?? ?? ?? 6A 00 50 E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 52 E8 ?? ?? ?? ??",
    ret="void",
    params=[],
    doc="Releases the shared menu materials, package data, and resource handles when they are loaded.",
    stable=True,
)

stable.fn(
    "Menu_ClearTransitionFlags",
    "B8 42 00 00 00 33 C9 66 39 05 ??",
    ret="void",
    params=[],
    doc="Rearms completed bone, puppy, and lives HUD transitions by changing terminal state 0x42 to zero.",
    stable=True,
)

stable.fn(
    "Menu_UpdatePauseMenu",
    "A1 ?? ?? ?? ?? 66 8B 0D",
    ret="void",
    params=[],
    doc="Updates the in-level pause/save menu state. Callers discard native result-register residue.",
)

stable.fn(
    "UI_UpdateBoneCounter",
    "?? ?? ?? 53 56 57 8B 59",
    match=-0x13,
    hook=0x6,
    ret="void",
    params=[param("int32_t", "bone_count")],
    doc="Advances the bone HUD slide/count/hold/exit states and renders the icon plus centered total.",
    stable=True,
)

stable.fn(
    "UI_RenderCenteredNumber",
    "8B 0D ?? ?? ?? ?? 83 EC 10",
    hook=0x6,
    ret="void",
    params=[param("uint32_t", "packed_center_xy"), param("int32_t", "number")],
    doc="Formats and renders a decimal HUD number centered on the packed x/y anchor.",
)

stable.fn(
    "UI_UpdatePuppyCounter",
    "?? ?? ?? 53 55 56 8B 59",
    match=-0x13,
    hook=0x6,
    ret="void",
    params=[param("int32_t", "puppy_count")],
    doc="Advances the puppy HUD slide/count/hold/exit states and renders the icon plus centered total.",
    stable=True,
)

stable.fn(
    "UI_UpdateLives",
    "83 EC 24 66 83 3D ??",
    hook=0xB,
    ret="int32_t",
    params=[param("int32_t", "icon_count"), param("int32_t", "lives")],
    doc="Clamps icon_count to 0 through 4, animates and renders the life icons plus lives-1, and returns their X anchor. Returns -1 while the animation is not drawable or lives is negative.",
)

stable.fn(
    "Menu_AnimateSlots",
    "83 EC 0C 8B 0D ?? ?? ?? ?? 53 55",
    hook=0x9,
    ret="void",
    params=[],
    doc="Advances the eight menu-slot animations. The terminal slot pointer is not a return contract.",
)

stable.fn(
    "Menu_RenderConfirmPrompt",
    "?? A9 00 00 08 00 75 ?? 25 00",
    match=-0x11,
    ret="void",
    params=[param("int32_t", "right_edge_x")],
    doc="Blinks and right-aligns the buffered confirmation text near the bottom of the screen.",
    stable=True,
)

stable.fn(
    "Menu_GetPlayerLevelInfo",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 4C",
    ret="void",
    params=[param("Menu_LevelProgressInfo*", "out_info")],
    doc="Writes current player and level progress into out_info and refreshes the active level's completion bits.",
    stable=True,
)

stable.fn(
    "Menu_IsInGame",
    "A0 ?? ?? ?? ?? 3C 0B 74 07 3C 0C 74 03 33 C0 C3 B8 01 00 00 00 C3",
    required=Required.EN,
    ret="BOOL",
    params=[],
    doc="EN-only helper used by Menu_UpdatePauseMenu. Returns full-EAX TRUE exactly for menu states 11 and 12 and full-EAX FALSE otherwise.",
)

stable.fn(
    "Player_InitializeState",
    "80 28 01 00 00 50 E8 ??",
    match=-0x2C,
    hook=0x8,
    ret="void",
    params=[
        param(
            "Entity_State*",
            "entity",
            doc="Player entity. Its linked actor receives refreshed completion flags.",
        ),
        param(
            "PKG_ActorRecord*",
            "record",
            doc="Player actor record whose puppy/count fields are initialized from the backup puppy-count global.",
        ),
    ],
    doc="Initializes the player record's saved puppy and count fields, then refreshes completion flags on actor.linked_actor.",
)

stable.fn(
    "Player_SetCompletionFlags",
    "00 00 FF FF FF 3F E8 ??",
    match=-0x9,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Player actor whose behavior_flags completion-state bits are refreshed.",
        )
    ],
    doc="Clear actor completion-state bits, then set the game-complete or in-progress flag from Save_IsGameComplete.",
)

stable.fn(
    "Player_ResetBoneCount",
    "8B 44 24 08 C7 40 74 04 00 00 00 C3 90 90 90 90 56 8B 74 24 08 83 FE 03 7F ?? A1 ??",
    hook=0xB,
    ret="void",
    params=[
        param(
            "Entity_State*",
            "current_entity",
            doc="Unused ABI slot. The respawn caller passes the current entity before the record.",
        ),
        param(
            "PKG_ActorRecord*",
            "record",
            doc="Player record whose counter is reset to 4.",
        ),
    ],
    doc="Resets the player bone counter to 4. The record-pointer residue is not returned.",
    stable=True,
)

stable.fn(
    "Player_CollectPowerup",
    "56 8B 74 24 08 83 FE 03 7F ?? A1 ??",
    ret="void",
    params=[
        param(
            "int32_t",
            "powerup_type",
            doc="Powerup kind byte from the collected powerup actor record, promoted to int32_t by the caller.",
        )
    ],
    doc="Applies score, inventory, bone-count, or puppy-count side effects for the supplied powerup type.",
    stable=True,
)

stable.fn(
    "Level_CheckBonusUnlock",
    "00 C6 44 24 31 00 E8 ??",
    match=-0x65,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Math_Vec3I32*",
            "player_position",
            doc="Player actor position vector used for the bonus-unlock trace.",
        )
    ],
    doc=(
        "When the active checker slot changes, traces from player_position and latches the "
        "bonus-unlock/menu-reset globals on hit."
    ),
)

stable.fn(
    "Level_ResetBonusState",
    "FA B5 49 00 C3 C7 05 ??",
    match=-0x1A,
    required=Required.EN,
    ret="int32_t",
    params=[],
    doc="If the menu-reset flag is clear, writes state 3 to the first bonus-world slot and returns EAX=-1. Otherwise writes state 2 to the indexed slot and sign-extends the adjacent word into EAX.",
    stable=True,
)

stable.fn(
    "Menu_ResetState",
    "FF FF 83 C8 FF 66 A3 ??",
    match=-0xF,
    hook=0x7,
    ret="void",
    params=[],
    doc="Clears pause/menu transition state, rearms HUD counters, and resets their displayed values to -1.",
    stable=True,
)

stable.fn(
    "Level_CalculateCompletionPercent",
    "00 00 00 D3 E5 85 E8 ??",
    match=-0x36,
    ret="int32_t",
    params=[param("Save_GameSlot*", "save")],
    doc="Scans exactly 16 level entries, starting with max_world_reached * 20 and adding five for each complete level, best score of 100, and bonus bit 1 through 3. A total of 800 returns 100.",
)

stable.fn(
    "Menu_RenderSaveGame",
    "44 24 20 1C 00 0F 84 ??",
    match=-0x52,
    required=Required.EN,
    hook=0x8,
    ret="void",
    params=[
        param("int32_t", "slot_index"),
        param("uint32_t", "highlight_tint"),
        param(
            "BOOL",
            "allow_save",
            doc="Save/load mode flag. The renderer reads slot data from the global save_game_buffer.",
        ),
    ],
    doc="Renders save/load slots and applies highlight_tint to the selected row. Prompt choices use 0=affirm and 1=cancel.",
)

stable.fn(
    "Menu_RenderSaveYesNoPrompt",
    "E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 83 C4 04 50 8D 04 CD ?? ?? ?? ?? 2B C1 8D 04 80 C1 E0 02 99 F7 F9 8D",
    match=-0xA6,
    required=Required.EN,
    ret="void",
    params=[
        param("int32_t", "title_string_id"),
        param("int32_t", "selected_index"),
        param("uint32_t", "highlight_tint"),
    ],
    doc="Renders the save menu's lower Yes/No prompt and highlights the selected row. The English package table assigns ID 3 to Yes and ID 4 to No.",
)

stable.fn(
    "Menu_UpdateInput",
    "8A 0D ?? ?? ?? ?? 33 C0 A3 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc=(
        "Updates menu one-shot pulse dwords and held/debounce bytes from player-1 input and "
        "keyboard state."
    ),
)

stable.fn(
    "Menu_HandleSaveGameLogic",
    "?? ?? 84 C0 74 27 E8 ??",
    match=-0x10,
    ret="void",
    params=[
        param("char*", "out_result"),
        param("int32_t", "selected_slot"),
        param("int32_t", "allow_save"),
    ],
    doc="Advances the save/load menu state machine, updates out_result[0 through 2], and starts or polls save-game file operations over the 0x1dc file span. Prompt choices use 0=affirm and 1=cancel.",
)

stable.fn(
    "Options_FindBindingConflict",
    "53 8B 5C 24 0C 55 8B 6C 24 0C 56 57 8D 7D ?? 33 D2 89 7C 24 ?? 8B 37",
    ret="void",
    params=[
        param("Config_Data*", "settings"),
        param("Options_BindingLocation*", "out_first_conflict"),
        param("Options_BindingLocation*", "out_second_conflict"),
    ],
    doc="Finds the first duplicate keyboard or joystick binding and writes its two locations. 0xff marks no conflict.",
)

stable.fn(
    "Save_SaveGameToSlot",
    "F3 A5 6A ??",
    match=-0x56,
    hook=0xB,
    ret="void",
    params=[param("int32_t", "slot_index")],
    doc="Copies live settings and collectible data into slot_index. Valid slots are 0 through 4, but the function does not check the range.",
)

stable.fn(
    "Menu_HandleOptionsLogic",
    "A1 ?? ?? ?? ?? ?? 56 48 57 C6 05 ?? ?? ?? ?? FF 0F 84",
    ret="int32_t",
    params=[],
    doc="Runs the options/control-binding submenu, including conflict detection and keyboard or joystick remapping. Returns int32_t 1 on accepted exit/save paths and 0 while continuing or backing between substates.",
)

stable.fn(
    "Game_BackupSettings",
    "89 15 ?? ?? ?? ?? 33 C0 C3",
    match=-0x1C,
    ret="void",
    params=[],
    doc="Snapshots the live volume, game, and rumble settings so the options menu can restore them after cancellation.",
    stable=True,
)

stable.fn(
    "Menu_ProcessOptionsInput",
    "88 4E 02 3C 03 0F 84 ??",
    match=-0xB,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Menu_ActionResult*",
            "out_result",
            doc="Menu action, selection, and sound result updated by the input dispatcher.",
        ),
        param("int32_t", "selected_row", doc="Options-menu row being processed."),
    ],
    doc="Processes options-menu navigation and setting changes for selected_row and writes the resulting action, selection, and sound fields.",
    stable=True,
)

stable.fn(
    "Menu_RenderControlsConfiguration",
    "83 EC 08 53 55 56 57 6A 00 6A 00 6A 76 E8 ??",
    required=Required.EN,
    ret="void",
    params=[],
    doc="Renders the keyboard and joystick binding tables, conflict highlighting, remapping status, and accept/cancel controls.",
    stable=True,
)

stable.fn(
    "Menu_RenderMusicSelection",
    "04 8B E8 ?? ?? ?? ?? 00",
    match=-0x10,
    hook=0x9,
    ret="void",
    params=[],
    doc="Updates the highlight gradient and renders four music-selection rows.",
    stable=True,
)

stable.fn(
    "Menu_RenderDifficultySelection",
    "04 8B E8 ?? ?? ?? ?? 68",
    match=-0x10,
    required=Required.EN,
    hook=0x9,
    ret="void",
    params=[],
    doc="Renders the difficulty title and two choices, highlighting the selected row.",
    stable=True,
)

stable.fn(
    "Menu_RenderOptionsMenu",
    "83 EC 64 A1 ?? ?? ?? ?? 56",
    hook=0x8,
    ret="int32_t",
    params=[
        param("int32_t", "selected_option_index"),
        param("uint32_t", "highlight_tint"),
    ],
    doc="Renders the active options page and returns the button-prompt set appropriate for the selected row.",
    stable=True,
)

stable.fn(
    "Menu_ProcessMenuState",
    "83 EC 08 53 55 56 57 E8 ??",
    ret="bool",
    params=[],
    doc="Processes the active menu state and returns one-byte truthiness through AL when menu handling consumes or skips the normal frame path. Yes/No choice is exactly 0=Yes and 1=No.",
    unstable=True,
)

stable.fn(
    "Menu_HandleSelection",
    "8B 15 ?? ?? ?? ?? 33 C0 A2",
    hook=0x6,
    ret="void",
    params=[
        param(
            "int32_t",
            "target_menu",
            doc="Menu/state id or sentinel. -1 skips transition, 1 resumes music, and 3 restores the stored fade target.",
        )
    ],
    doc="Side-effect-only menu-exit dispatcher. Clears transient menu/input state.",
)

stable.fn(
    "Menu_CheckPauseInput",
    "A0 ?? ?? ?? ?? 84 C0 0F 85 ?? ?? ?? ?? 66 A1",
    ret="void",
    params=[param("char", "allow_pause")],
    doc="Updates an active pause transition or starts one from eligible input when pausing is allowed.",
    stable=True,
)

stable.fn(
    "Menu_RenderYesNoPrompt",
    "00 01 50 E8 ?? ?? ?? ?? 83",
    match=-0xD,
    ret="void",
    params=[
        param("int32_t", "title_string_id"),
        param("int32_t", "selected_index"),
        param("uint32_t", "highlight_tint"),
    ],
    doc="Renders a centered Yes/No prompt and highlights the selected row. The English package table assigns ID 3 to Yes and ID 4 to No.",
)

stable.fn(
    "Menu_ProcessNameEntryInput",
    "A0 ?? ?? ?? ?? 53 56 3C 01 57 0F 85 ??",
    ret="void",
    params=[],
    doc="Processes the initials-entry editor and its post-entry choice menu.",
    stable=True,
)

stable.fn(
    "Menu_RenderNameEntry",
    "01 53 55 56 57 0F 85 ??",
    match=-0x9,
    ret="void",
    params=[param("uint32_t", "highlight_tint")],
    doc="Renders the initials-entry grid or post-entry choices with the active-cell highlight tint.",
    stable=True,
)

stable.fn(
    "Graphics_InitializeColorGradient",
    "88 06 88 4E 03 C1 E8 ??",
    match=-0x12,
    hook=0x8,
    ret="void",
    params=[
        param("Menu_ColorGradientState*", "gradient_state"),
        param("uint8_t", "step_count"),
        param("uint32_t", "packed_hold_frames"),
        param("uint32_t", "packed_intensity_bounds"),
    ],
    doc="Initializes an eight-byte ping-pong color-gradient state from its step count, endpoint holds, and intensity bounds.",
    unstable=True,
)

stable.fn(
    "Graphics_ComputeColorGradient",
    "8B 4C 24 0C F7 C1 00 00 00 80 0F 84 ??",
    hook=0xA,
    ret="void",
    params=[
        param("uint32_t*", "out_top_color"),
        param("uint32_t*", "out_bottom_color"),
        param("uint32_t", "text_flags"),
    ],
    doc="Computes top and bottom text colors from text_flags, or writes the default pair when tinting is disabled.",
    stable=True,
)

stable.fn(
    "UI_RenderButtonPrompts",
    "C3 06 74 ?? 40 8B 35 ??",
    match=-0x15,
    required=Required.EN,
    ret="void",
    params=[param("int32_t", "button_set")],
    doc="Renders the bottom-centered confirm/back prompt groups selected by button_set.",
    stable=True,
)

stable.fn(
    "Menu_RenderFormattedText",
    "D8 53 E8 ?? ?? ?? ?? 8B",
    match=-0x15,
    hook=0xA,
    ret="void",
    params=[
        param("uint32_t", "packed_xy"),
        param("int32_t", "label_string_id"),
        param("int32_t", "button_prompt_id"),
    ],
    doc="Centers and renders one localized label/button prompt row at packed_xy.",
)

stable.fn(
    "UI_ComputeStringWidth",
    "8B 0D ?? ?? ?? ?? 56 8B 41",
    hook=0x6,
    ret="int32_t",
    params=[param("char const*", "text")],
    doc="Walks NUL-terminated signed text bytes through the initialized current glyph table without guards. Returns visible glyph widths plus one pixel between glyphs.",
)

stable.fn(
    "PKG_InitializeResourceGameEngine",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? 32",
    ret="bool",
    params=[],
    doc="Initializes core memory, resource, and DirectDraw subsystems and returns one-byte bool status through AL. The BN database has analysis-only EB 03 at 0x42C817, so the allocator result is ignored and control always reaches the continuation, which sets AL=1 after DirectDraw initialization.",
    stable=True,
)

stable.fn(
    "PKG_ShutdownResourceGameSubsystems",
    "E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 90 90 90 90 90 51",
    ret="void",
    params=[],
    doc="Shuts down core game resource subsystems by releasing the DirectDraw device and tail-calling the memory extent leak/free sweep.",
)

stable.fn(
    "UI_RenderText",
    "51 6A 00 C7 05 ??",
    hook=0xD,
    ret="void",
    params=[
        param("uint32_t", "packed_xy"),
        param("char const*", "text"),
        param("uint32_t", "text_flags"),
        param("void*", "font_context"),
    ],
    doc="Renders a NUL-terminated string at packed_xy using the selected color and font context.",
)

stable.fn(
    "UI_RenderStringWithFormatting",
    "?? ?? ?? C6 44 24 10 01",
    match=-0x1E,
    required=Required.EN,
    hook=0xC,
    ret="void",
    params=[
        param("uint32_t", "packed_xy"),
        param("char const*", "text"),
        param("uint32_t", "text_flags"),
        param("void*", "font_context"),
        param("uint32_t", "glyph_advance"),
    ],
    doc="Renders NUL-terminated text at packed_xy using fixed-advance glyph_advance spacing.",
)

stable.fn(
    "Graphics_DrawSortedLists",
    "A1 ?? ?? ?? ?? 05 CC",
    ret="void",
    params=[],
    doc="Draws queued render-list records in sorted order.",
)

stable.fn(
    "Graphics_BeginRendering",
    "E8 ?? ?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 8B 0D",
    ret="void",
    params=[param("uint32_t", "clear_color")],
    doc="Begins a full-screen clear-color rendering pass using clear_color.",
)

stable.fn(
    "Graphics_ClearScreenAndRenderRectangle",
    "E8 ?? ?? ?? ?? 6A 00 E8 ?? ?? ?? ?? 8B 44",
    ret="void",
    params=[
        param("uint32_t", "top_left_color"),
        param("uint32_t", "top_right_color"),
        param("uint32_t", "bottom_left_color"),
        param("uint32_t", "bottom_right_color"),
    ],
    doc="Runs a nested full-screen four-corner gradient pass with depth writes disabled, then closes the pass.",
    stable=True,
)

stable.fn(
    "Graphics_IncrementPassCounter",
    "E4 BA 49 00 75 05 E9 ??",
    match=-0xC,
    required=Required.EN,
    ret="void",
    params=[],
    doc="Increments the nested rendering-pass counter.",
)

stable.fn(
    "Graphics_EndRendering",
    "E4 BA 49 00 75 0E E8 ??",
    match=-0xC,
    required=Required.EN,
    ret="void",
    params=[],
    doc="Closes one nested rendering pass and runs the final fog/no-op backend path when the nesting counter reaches zero.",
    stable=True,
)

stable.fn(
    "Pkg_FreeResourceAndReturnFalse",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 83 C4 04 32",
    ret="bool",
    params=[
        param(
            "void*",
            "mem_ptr",
            doc="Resource-memory data pointer forwarded to PKG_FreeResourceMemory.",
        )
    ],
    doc="Frees mem_ptr through PKG_FreeResourceMemory, then deliberately returns false by clearing AL only. Upper EAX remains undefined residue.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Graphics_InitializeTextureBlendTextures",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 83 C4 04 C3",
    ret="Material_BlendTextureSet*",
    params=[param("uint8_t*", "pixel_data")],
    doc="Uploads pixel_data as the loading-screen texture and returns the global blend-texture set.",
    stable=True,
)

stable.fn(
    "Graphics_FreeTextureBlend",
    "56 8B 74 24 08 8B 06 50 E8 ??",
    ret="void",
    params=[param("Material_BlendTextureSet*", "blend_textures")],
    doc="Releases the four quadrant texture handles in blend_textures.",
    stable=True,
)

stable.fn(
    "PKG_CleanupResourceGameState",
    "A1 ?? ?? ?? ?? 53 33 DB 3B C3 74",
    ret="void",
    params=[],
    doc="Unloads the latched level_resource_handle when present, runs the title/menu cleanup branch, clears object/current-level state including current_level_data, and resets the current level id.",
)

stable.fn(
    "Menu_ProcessMenuTransition",
    "60 85 F6 75 ?? 57 E8 ??",
    match=-0x21,
    required=Required.EN,
    hook=0x6,
    ret="void",
    params=[param("int32_t", "target_menu")],
    doc="Side-effect-only converter from target_menu to title, level, reload, or quit state. It clears pause/title flags.",
    stable=True,
)

stable.fn(
    "UI_Update",
    "A1 ?? ?? ?? ?? 8A ?? ?? ?? ?? ?? 53 85 C0 0F BE ?? 74 ?? C6 05 ?? ?? ?? ?? 05 BB",
    ret="void",
    params=[],
    doc="Advances title/loading UI state, fade delays, and menu transitions. BN database-only branch edits bypass the local state-8 handler at 0x42CFD6 (EB 3A) and state-0xA handler at 0x42D015 (90 E9 70 FF FF FF).",
)

stable.fn(
    "Input_CheckCheatCodeSequence",
    "?? A3 ?? ?? ?? ?? 3B C1",
    match=-0xA,
    ret="bool",
    params=[],
    doc="Recognizes changed nonnegative button codes in the edge-debounced sequence abccba. A match advances the index, a mismatch resets it, and completion returns AL=1 once.",
    stable=True,
)

stable.fn(
    "PKG_UpdateLoadingScreen",
    "A0 ?? ?? ?? ?? 53 33",
    ret="bool",
    params=[param("int32_t", "level_index")],
    doc="Drives the four-state loading-image load, 15-tick fade, resource wait, fade-out, and teardown lifecycle. Returns one-byte bool true once state reaches 2.",
    stable=True,
)

stable.fn(
    "Replay_LoadBonusReplay",
    "56 8B 74 24 08 85 F6 0F 84 ??",
    ret="bool",
    params=[
        param(
            "uint8_t*",
            "bonus_replay_source",
            doc="Base of the packed bonus-replay source. Null only queries whether the replay buffer already exists.",
        )
    ],
    doc=(
        "A null source returns existing replay-buffer availability through AL. Otherwise rotates "
        "through six replay indices, allocates the 0xC90-byte buffer when needed, sets replay/title "
        "state, copies the selected 0xC80-byte payload, and returns one-byte bool availability."
    ),
)

stable.fn(
    "Level_Load",
    "0F BE 05 ?? ?? ?? ?? 83 F8 0A 0F 87 ??",
    required=Required.EN,
    hook=0x7,
    ret="bool",
    params=[],
    doc="Runs the level and menu loading state machine for states 0 through 10. Every exit returns a one-byte boolean value.",
)

stable.fn(
    "Game_TransitionToLevel",
    "8B 54 24 04 8D 42 F9 A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 85 C0 75 ?? 52 E8 ?? ?? ?? ?? 83 C4 04 A3 ?? ?? ?? ?? 32 C0 C3 80 3D ?? ?? ?? ?? 03 75 ?? 32 C0 C3 57 B9 2C 00 00 00 33 C0 BF ?? ?? ?? ?? F3 AB",
    hook=0x7,
    ret="bool",
    params=[param("int32_t", "level_id")],
    doc="Starts the requested level-load state machine and returns false through AL while loading begins or the loading screen remains in state 3. Once resources exist, initializes level, replay, and frame state and returns AL=1.",
)

stable.fn(
    "Level_UpdateWorldSelectMenu",
    "51 A0 ?? ?? ?? ??",
    hook=0x6,
    ret="int32_t",
    params=[],
    doc="Runs the world/save-slot selection transition. Returns 0 while active, 1 on cancel, and 2 after accepted cleanup.",
    stable=True,
)

stable.fn(
    "Level_UpdateInterLevelMenu",
    "A0 ?? ?? ?? ?? 83 EC 08 3C 03 53 56 0F 85 ??",
    ret="int32_t",
    params=[],
    doc="Runs the inter-level menu and options flow, returning 3 while active, 1 for world selection, and 0 to continue loading.",
    stable=True,
)

stable.fn(
    "Material_BuildStructure",
    "8B 47 04 85 C0 0F 84 ??",
    match=-0x28,
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
            doc="Receives the material section pointer, selected material table entry pointer, and frame-set pointer.",
        ),
        param(
            "Material_FrameSet*",
            "optional_output_table",
            doc="Optional output buffer for the Material_FrameSet header followed by frame pointers and frame records.",
        ),
    ],
    doc="Builds one material reference and compact frame table, returning the number of emitted dwords or zero when no material nodes match.",
    stable=True,
)

stable.fn(
    "Material_BuildTextureArray",
    "C0 56 8B 4D 0C 8B 35 ??",
    match=-0xC,
    ret="void",
    params=[
        param(
            "Material_SectionHeader*",
            "material_section",
            doc="Material section containing the source material-entry table.",
        ),
        param(
            "Material_RefEntry*",
            "material_refs",
            doc="First material reference record to initialize.",
        ),
        param(
            "int32_t",
            "material_count",
            doc="Number of consecutive material reference records to build.",
        ),
        param(
            "int32_t",
            "first_material_index",
            doc="Starting source material index used to compute each material record position.",
        ),
    ],
    doc="Builds material reference and frame-table data for material_count source entries starting at first_material_index.",
    stable=True,
)

stable.fn(
    "Material_FindTextureByFrame",
    "8B 4C 24 04 53 56 8B 41 08 57 85 C0 0F 84 ??",
    ret="int32_t",
    params=[
        param(
            "Material_RefEntry*",
            "material_ref",
            doc="Material reference whose frame set is searched.",
        ),
        param("int32_t", "frame_index", doc="Requested animation/frame index."),
        param(
            "Graphics_SpriteContext**",
            "out_sprite",
            doc="Receives the render sprite context. Its texture descriptor is updated to the selected frame or frame-zero fallback.",
        ),
    ],
    doc="Resolves material_ref at frame_index into out_sprite, falling back to frame zero when the requested frame is unavailable.",
    stable=True,
)

stable.fn(
    "Math_BuildMatrixRotationY",
    "?? C1 F8 02 83 C4 08 8B",
    match=-0x1D,
    hook=0x6,
    ret="Math_Matrix3x3I16*",
    params=[
        param(
            "Math_Matrix3x3I16*",
            "out_matrix",
            doc="Receives the Y-axis rotation matrix.",
        ),
        param(
            "Math_Matrix3x3I16*",
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
    "Math_BuildMatrixRotationXY",
    "7C ?? 2C 2B C7 50 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="void",
    params=[
        param("Math_Matrix3x3I16*", "out_matrix"),
        param("Math_Matrix3x3I16*", "post_multiply"),
        param("int32_t", "angle_x"),
        param("int32_t", "angle_y"),
    ],
    doc="Builds the combined Q12 X/Y rotation matrix and optionally composes it with post_multiply.",
    stable=True,
)

stable.fn(
    "Math_BuildRotationMatrix",
    "7C ?? 3C 2B C7 50 E8 ??",
    match=-0xB,
    hook=0x8,
    ret="void",
    params=[
        param("Math_Matrix3x3I16*", "out_matrix"),
        param("Math_Matrix3x3I16*", "post_multiply"),
        param("int32_t", "angle_x"),
        param("int32_t", "angle_y"),
        param("int32_t", "angle_z"),
    ],
    doc="Builds a signed Q12 XYZ rotation matrix and optionally composes it with post_multiply.",
    stable=True,
)

stable.fn(
    "Math_BuildRotationMatrixDirect",
    "8B 44 24 0C 8B 4C 24 10 66 A3 ??",
    hook=0x8,
    ret="void",
    params=[
        param(
            "Math_Matrix3x3I16*",
            "out_matrix",
            doc="Receives the combined X/Y/Z rotation matrix.",
        ),
        param(
            "Math_Matrix3x3I16*",
            "post_multiply",
            doc="Optional matrix composed before the rotations.",
        ),
        param(
            "int32_t",
            "sin_x_q12",
            doc="Precomputed X sine in signed Q12/Q14 table form. Low word is stored into the X rotation template.",
        ),
        param(
            "int16_t",
            "cos_x_q12",
            doc="Precomputed X cosine stored into the X rotation template.",
        ),
        param(
            "int32_t",
            "sin_y_q12",
            doc="Precomputed Y sine in signed Q12/Q14 table form. Low word is stored into the Y rotation template.",
        ),
        param(
            "int16_t",
            "cos_y_q12",
            doc="Precomputed Y cosine stored into the Y rotation template.",
        ),
        param(
            "int32_t",
            "angle_z",
            doc="Z angle in the game's 0x1000-turn sine-table domain. Zero skips the final Z multiply.",
        ),
    ],
    doc="Builds a signed Q12 3x3 rotation matrix from precomputed X/Y sine/cosine inputs and a Z angle.",
    stable=True,
)

stable.fn(
    "PKG_AllocateResourceMemory",
    "8B 44 24 04 85 C0 75 ?? C3 50 E8 ?? ?? ?? ?? 83 C4 04 C3",
    ret="void*",
    params=[param("int32_t", "size")],
    doc="Size == 0 returns null. Otherwise forwards to PKG_AllocateResourceWithHeader.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "PKG_AllocateResourceWithHeader",
    "8B 44 24 04 56 83 C0 04 50 E8 ??",
    ret="uint32_t*",
    params=[
        param(
            "int32_t",
            "size",
            doc="Requested resource data size, excluding the hidden leading handle dword.",
        )
    ],
    doc="Allocates size + 4 bytes, stores the allocation handle in the hidden leading dword, and returns the payload pointer after it, or null if pointer lookup fails. The size + 4 arithmetic has no explicit overflow guard.",
)

stable.fn(
    "PKG_FreeResourceMemory",
    "8B 44 24 04 85 C0 74 ?? 56 8B 70 FC 56 E8 ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "void*",
            "mem_ptr",
            doc="Resource data pointer returned after the hidden handle dword.",
        )
    ],
    doc="Returns immediately for null. Otherwise reads the hidden allocation handle at mem_ptr-4, checks it, and releases it.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Component_CheckActorCollisionTypeAdapter",
    "50 04 8B 00 52 50 E8 ??",
    match=-0x10,
    hook=0x8,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("Actor_State*", "other_actor")],
    doc="Forwards the component owner, definition, and other_actor to Actor_CheckCollisionType and returns its tri-state result.",
    stable=True,
)

stable.fn(
    "Actor_CheckCollisionType",
    "8B 54 24 04 8B 44 24 0C 3B D0 0F 84 ??",
    hook=0x8,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "other_actor"),
    ],
    doc="Maps collision classes 2, 3, and 4 to their target mask bits. Returns zero for a neutral match, 1 for a positive match, and -1 otherwise.",
    stable=True,
)

stable.fn(
    "Actor_CheckAnimationTrigger",
    "74 ?? 8B D8 48 83 E9 ??",
    match=-0x40,
    hook=0x6,
    ret="bool",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "other_actor")],
    doc="Walks actor's trigger slots backward. A slot matches when its threshold word is at most actor state word +0xCE and its actor pointer equals other_actor.",
    stable=True,
)

stable.fn(
    "Component_CalculateCollisionRange",
    "00 66 8B 42 44 D1 E8 ??",
    match=-0x28,
    hook=0x8,
    ret="int32_t",
    params=[param("Component_Definition*", "definition")],
    doc="Estimates collision travel range from velocity and average type-0 lifetime. The special duration tuple returns 0x3E8000.",
)

stable.fn(
    "Component_RefreshCollisionRanges",
    "7E ?? 00 75 ?? 50 E8 ??",
    match=-0x32,
    ret="void",
    params=[
        param(
            "Component_CollisionRecord*",
            "collision_records",
            doc="Writable array of 0x24-byte collision records.",
        ),
        param("int32_t", "slot_count", doc="Positive number of collision records to refresh."),
    ],
    doc="Refreshes collision ranges while decrementing cooldowns and clearing reference actors. Callers must pass a positive slot_count.",
    stable=True,
)

stable.fn(
    "Component_UpdateCollisionDetection",
    "8B 45 18 85 C0 0F 84 ??",
    match=-0x18,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Component_CollisionRecord*",
            "collision_records",
            doc="Array of 0x24-byte component collision records.",
        ),
        param(
            "int32_t",
            "slot_count",
            doc="Positive record count, or negative to select one current record when actor collision_class != 3. Zero and negative class-3 counts violate the loop contract.",
        ),
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose component collision records are tested.",
        ),
        param(
            "Actor_State*",
            "other_actor",
            doc="Optional input actor used for filtering and collision radius. When nonnull its world position overrides test_pos.",
        ),
        param(
            "Math_Vec3I32*",
            "test_pos",
            doc="Explicit world position required when other_actor is null.",
        ),
    ],
    doc="Processes collision records against an actor or explicit position. A null other_actor requires a nonnull test_pos, and zero or some negative slot counts violate the loop contract.",
    stable=True,
)

stable.fn(
    "Component_GetSpeedRange",
    "8B 44 24 04 33 D2 8B 88 F4 00 00 00 8B 49 04 8B 41 1C 66 8B 51 18 C1 E0 10 0B C2 C3 90 90 90 90 8B 44 24 04 8B 88 F4 00 00 00 8B 51 04 8B 02 C1 E8 ??",
    hook=0x6,
    ret="uint32_t",
    params=[param("Actor_State*", "actor")],
    doc="Returns a uint32 carrier packing definition +0x18/+0x1C low words. Callers sign-extend each half and field labels remain uncertain.",
    stable=True,
)

stable.fn(
    "Component_IsAirborneTarget",
    "8B 51 04 8B 02 C1 E8 ??",
    match=-0xA,
    hook=0xA,
    ret="BOOL",
    params=[param("Actor_State*", "actor")],
    doc="Returns definition flag bit 14 for a spawned component actor.",
    stable=True,
)

stable.fn(
    "Component_SpawnFromCollisionRecord",
    "0C 89 44 24 14 0F 84 ??",
    match=-0x2D,
    hook=0x7,
    ret="Actor_State*",
    params=[
        param("Actor_State*", "owner_actor"),
        param("Actor_State*", "target_actor"),
        param(
            "Component_CollisionRecord*",
            "collision_record",
            doc="Mutable record. Cooldown at +0x6 is refreshed and the remaining fields seed the spawned component.",
        ),
    ],
    doc="Creates component actors from a collision record, updates its cooldown, initializes ownership/transforms/effects, and returns the last actor.",
    unstable=True,
)

stable.fn(
    "Component_CalculateOrientation",
    "8B 07 F6 C4 80 0F 84 ??",
    match=-0x17,
    hook=0x6,
    ret="void",
    params=[
        param("Component_Instance*", "comp"),
        param("Component_Definition*", "definition"),
    ],
    doc="Computes projectile yaw, pitch, and local rotation from definition defaults, targeting, gravity, and scatter.",
    stable=True,
)

stable.fn(
    "Component_TrackTarget",
    "2B CA 51 89 4D F8 E8 ??",
    match=-0x65,
    hook=0x6,
    ret="void",
    params=[
        param("Component_Instance*", "comp"),
        param("Component_Definition*", "definition"),
        param("int16_t", "turn_rate"),
    ],
    doc="Turns component yaw and pitch toward its target. Definition flag 0x8000 suppresses pitch tracking.",
)

stable.fn(
    "Component_PlayPositionalSound",
    "?? 83 C1 40 51 50 E8 ??",
    match=-0x17,
    hook=0x6,
    ret="void",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "actor"),
        param("int32_t", "sound_slot"),
    ],
    doc="Plays definition sound_ids[sound_slot_index] at actor position. Returns without effect when actor is null.",
)

stable.fn(
    "Component_InitializeProjectile",
    "03 66 8B 57 44 52 E8 ??",
    match=-0x4F,
    hook=0x6,
    ret="void",
    params=[
        param("Component_Instance*", "comp"),
        param("Component_Definition*", "definition"),
    ],
    doc="Activates projectile state, resets hit, timer, and velocity data, starts launch audio, and computes orientation.",
    stable=True,
)

stable.fn(
    "Component_CreateActor",
    "53 56 57 8B 7C 24 1C 83 FF 02 75 ?? A1 ??",
    hook=0x7,
    ret="Actor_State*",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "spawn_source"),
        param("Actor_State*", "owner_actor"),
        param("int32_t", "actor_kind"),
    ],
    doc="Creates a component actor of actor_kind, initializes its ownership and lifecycle state, and inherits the source actor's transform.",
    stable=True,
)

stable.fn(
    "Collision_InitializeFunctionPointers",
    "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C3 90",
    hook=0xA,
    ret="void",
    params=[],
    doc=(
        "Installs projectile logic and collision-response callbacks, including actor-phase slot 2 "
        "as Collision_ProcessProjectileLifecycle."
    ),
)

stable.fn(
    "Collision_ProcessProjectileHit",
    "4C 24 28 55 50 51 E8 ??",
    match=-0x15,
    ret="int32_t",
    params=[
        param("Component_Instance*", "comp"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_depth"),
    ],
    doc="Handles projectile collision probes and contacts, records accepted hits, and dispatches actor-class-specific responses.",
    stable=True,
)

stable.fn(
    "Collision_RegisterHitEvent",
    "8B 48 2C 51 57 56 E8 ??",
    match=-0x10,
    hook=0x6,
    ret="uint8_t",
    params=[
        param("Component_HitEvent*", "hit_events", doc="Base of the fixed eight-slot cooldown table."),
        param("Actor_State*", "actor"),
    ],
    doc="Returns zero when admission is denied. Otherwise returns 1 after using the first expired slot, though a full table also returns 1 without inserting anything.",
)

stable.fn(
    "Collision_CanRegisterHitEvent",
    "5F 5E B0 01 C3 8B 0D ??",
    match=-0x2A,
    public=False,
    ret="uint8_t",
    params=[
        param("Component_HitEvent*", "hit_events", doc="Base of the fixed eight-slot cooldown table."),
        param("Actor_State*", "actor"),
        param("int32_t", "actor_collision_key"),
    ],
    doc="Scans eight slots and returns 0 only for a matching prior-frame cooldown still in the future. No match, same-frame match, or expiry returns 1.",
)

stable.fn(
    "Collision_HandleComponentCollision",
    "FF FF 5D C3 55 57 E8 ??",
    match=-0x2D,
    ret="int32_t",
    params=[param("Component_Instance*", "comp"), param("Actor_State*", "other_actor")],
    doc="Filters component hits, returning 0 when accepted, -2 for owner self-collision, and -1 for other rejections.",
    stable=True,
)

stable.fn(
    "Component_CalculateHomingVelocity",
    "8B 40 08 89 41 08 E9 ??",
    match=-0x9F,
    hook=0x6,
    ret="void",
    params=[param("Component_Instance*", "comp")],
    doc="Writes post-impact velocity and updates yaw, pitch, and local rotation when gravity is disabled.",
    stable=True,
)

stable.fn(
    "Component_SetVelocityFromDirection",
    "7E ?? 51 6A 00 57 E8 ??",
    match=-0x1C,
    ret="void",
    params=[
        param("Component_Instance*", "comp"),
        param("Component_Definition*", "definition"),
    ],
    doc="Builds local rotation and initializes velocity from the definition's launch speed and direction flags.",
    stable=True,
)

stable.fn(
    "Collision_ProcessProjectileLifecycle",
    "?? 74 ?? 6A 00 57 E8 ??",
    match=-0x1C,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Actor-phase slot 2. Updates projectile ownership, activation, follow-up spawns, attachments, fade, and destruction while indexed dispatchers ignore EAX.",
    stable=True,
)

stable.fn(
    "Component_SpawnFollowupProjectile",
    "?? 50 8D 47 40 50 E8 ??",
    match=-0x24,
    ret="Actor_State*",
    params=[
        param("Component_Definition*", "component_def"),
        param("Actor_State*", "source_actor"),
        param("Actor_State*", "parent_actor"),
    ],
    doc=(
        "Spawns a follow-up component actor from source_actor, optionally attaches it to "
        "parent_actor, and initializes its sound and orientation."
    ),
    stable=True,
)

stable.fn(
    "Component_AttachToOwner",
    "46 04 F6 00 C0 0F 85 ??",
    match=-0xD,
    ret="void",
    params=[
        param("Actor_State*", "owner_actor"),
        param("Component_Instance*", "comp"),
        param("Math_Vec3I32*", "local_pos"),
    ],
    doc="Rehomes an eligible component to owner_actor at local_pos, updates references, and reinitializes projectile behavior.",
    stable=True,
)

stable.fn(
    "Collision_InitializePlaneLookupTables",
    "53 55 56 33 DB B9 ??",
    ret="void",
    params=[],
    doc="Builds the paired collision-plane and direction lookup tables with Q12 axis normals.",
    stable=True,
)

stable.fn(
    "Collision_DetectAndResolve3DCollision",
    "55 8B EC 81 EC 2C 02 00 00 E8 ??",
    hook=0x9,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param(
            "Collision_Node*",
            "collision_source",
            doc="Tagged layout-compatible source. Collision_Node, temporary prefix, or Actor_State alias.",
        ),
        param(
            "Collision_Vertex*",
            "vertices",
            doc="Vertex array. Source flag 0x2000 instead treats it as a node-compatible proxy and reloads +0x70.",
        ),
        param("Collision_Polygon*", "polygons"),
        param("int32_t", "polygon_count"),
    ],
    doc="Resolves meshes from tagged Collision_Node, temporary-prefix, or Actor_State source views. Flag 0x2000 makes vertices a +0x70 vertex-provider proxy.",
    stable=True,
)

stable.fn(
    "Collision_DetectActorCollisions",
    "85 C0 57 74 ?? 8B 2D ??",
    match=-0x2E,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("BOOL", "use_cached_collision_list"),
    ],
    doc="Fresh traversal uses a tag-specific root +0x70 list and type-1 sphere tails. Cached replay uses signed count +0xB6 for type 2 or +0x6A otherwise.",
)

stable.fn(
    "Collision_BuildAndResolveGroundEdgeWalls",
    "00 53 89 44 24 04 A1 ??",
    match=-0xC,
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Builds edge walls using a 0x68 temporary source with identity transform, origin, copied collision offset, and type 0 initialized. Other prefix bytes are unused on this path.",
)

stable.fn(
    "Collision_BuildWallCollisionPlane",
    "C2 22 89 4D 10 0F 84 ??",
    match=-0x4C,
    hook=0x6,
    ret="uint8_t",
    params=[
        param("Actor_State*", "actor"),
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "source_polygon"),
        param("int32_t", "edge_index"),
        param("Collision_Polygon*", "out_polygon"),
        param("Collision_Vertex*", "out_vertices"),
        param("int32_t", "out_vertex_base"),
    ],
    doc="Emits one wall quad and its packed face-plane distance/normal. +0xA padding is not initialized.",
)

stable.fn(
    "Collision_DetectActorObstacles",
    "8B 4F 4C 52 50 51 E8 ??",
    match=-0x2F,
    hook=0x7,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Scans obstacles and resolves qualifying type-2 nodes. Node flag 0x1000 admits collision, and signed polygon count is at +0xB6.",
)

stable.fn(
    "Collision_DetectObjectNodeCollisions",
    "?? ?? ?? C1 E2 07 8B 40",
    match=-0x37,
    hook=0x7,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t", "object_list_index"),
    ],
    doc="Scans object-list slot object_list_index for swept actor collisions. Slot 2 may also scan slots 1 and 4.",
)

stable.fn(
    "Collision_TestLineSphereIntersection",
    "55 8B EC 83 EC 14 8B 45 10 8B 0D ??",
    hook=0x6,
    ret="uint8_t",
    params=[
        param("Math_Vec3I32*", "line_start"),
        param("Math_Vec3I32*", "segment_delta"),
        param("uint32_t", "sphere_radius"),
    ],
    doc="Tests a segment against the global swept-sphere state and returns the byte-sized intersection result.",
)

stable.fn(
    "Collision_ResolveObjectNodeCollision",
    "E1 06 8B 45 0C 8B 35 ??",
    match=-0x56,
    hook=0x9,
    ret="void",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "other_actor")],
    doc=(
        "Resolves the class-specific Actor +0xC8 shape overlay, then passes other_actor as a "
        "node-compatible collision source sharing transform, position, sub_pos, and type offsets."
    ),
)

stable.fn(
    "Collision_ProcessActorToActorCollisions",
    "83 EC 30 55 8B 2D ??",
    hook=0xA,
    ret="void",
    params=[],
    doc="Runs frame actor collisions. +0xC8 is a collision-layout pointer overlay, while movement layouts use +0xCA as animation count.",
    stable=True,
)

stable.fn(
    "Physics_UpdateActorPreprocess",
    "42 65 01 75 ?? 8B 0D ??",
    match=-0x1A,
    hook=0x6,
    ret="uint8_t",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Returns the preprocess predicate and clears transient movement/contact bit 0x1000 each "
        "frame. Analyst label ACTOR_BEHAVIOR_MOVEMENT_ACTIVE has medium source-label confidence."
    ),
    stable=True,
)

stable.fn(
    "Actor_UpdateGroundOrientation",
    "0F 00 00 51 53 50 E8 ??",
    match=-0x35,
    public=False,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Aligns to contact normal. Transient movement/contact bit 0x1000 forces this path and also feeds movement-animation advance.",
)

stable.fn(
    "Physics_UpdateActorBehavior",
    "?? 83 C4 04 84 C0 74 ?? 56",
    match=-0x15,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Runs movement/collision integration directly for flag 0x800000, otherwise only after preprocessing succeeds.",
    stable=True,
)

stable.fn(
    "Actor_IntegrateMovementAndCollision",
    "00 3B C1 74 ?? 83 3D ??",
    match=-0x13,
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Integrates movement/collision. Accepted contact asserts transient movement/contact bit 0x1000, shared with movement-animation advance.",
)

stable.fn(
    "Actor_UpdateAttachmentTransform",
    "85 FF 89 7D FC 0F 84 ??",
    match=-0x10,
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Updates from an attachment transform or selected parent mesh face. Bit 0 composes rotation, and missing attachments mark actor for destruction.",
)

stable.fn(
    "Audio_CanRegisterCollisionSound",
    "?? B0 01 5E C3 8B 0D ??",
    match=-0x20,
    hook=0x8,
    ret="uint8_t",
    params=[
        param("Audio_CollisionSoundCooldownEntry*", "cooldown_entries"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "sound_key"),
    ],
    doc="Returns 0 only for a live matching prior-frame cooldown. No match, same-frame match, or expiry returns 1.",
)

stable.fn(
    "Audio_RegisterCollisionSoundEvent",
    "00 00 00 55 53 56 E8 ??",
    match=-0x2B,
    ret="uint8_t",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "sound_key"),
        param("uint32_t", "packed_impact_cooldown"),
    ],
    doc="Admits and records collision-sound cooldown state without playing audio. A full live table still returns 1 without replacement.",
)

stable.fn(
    "Collision_GetValidCollisionTarget",
    "3B CA 7E ?? 8B 8E E8 ??",
    match=-0x4E,
    ret="Actor_State*",
    params=[param("Actor_State*", "actor"), param("int32_t", "collision_slot")],
    doc="Consumes and validates one collision-target slot, returning the eligible actor or null.",
    stable=True,
)

stable.fn(
    "Actor_EvaluateCollisionConditionSelector",
    "24 9D ?? ?? ?? ?? A1 ??",
    match=-0x28,
    hook=0x6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("Actor_State*", "other_actor")],
    doc="Returns exactly 0 or -1 after testing collision selectors, ground/motion state, frame/camera state, and mapped input bits.",
)

stable.fn(
    "Physics_ApplyGroundReaction",
    "3B CA 89 4D F4 0F 84 ??",
    match=-0x11,
    hook=0x6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("PKG_ActorRecord*", "record")],
    doc="Consumes the recorded ground contact, applies its velocity reaction, clears it, and returns 0, -1, or 1 for none, rejected, or applied.",
    stable=True,
)

stable.fn(
    "Physics_CalculateFrictionForce",
    "55 8B EC 51 53 56 57 8B 7D 08 33 C0 8B 8F E8 ??",
    ret="int32_t",
    params=[param("Actor_State*", "actor")],
    doc="Returns a Q12 ground-friction scalar from the active collision normal and component multiplier.",
)

stable.fn(
    "Physics_CalculateActorVelocity",
    "F4 00 00 00 8B 8F E8 ??",
    match=-0x15,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "actor",
            doc="Actor whose physics component and velocity fields are updated.",
        ),
        param(
            "Math_Vec3I32*",
            "inout_velocity",
            doc="Three-int velocity vector normalized, clamped, and copied back into the actor velocity fields.",
        ),
        param(
            "Math_Vec3I32*",
            "steering_vec",
            doc="Three-int caller-provided steering/environment vector used by the velocity integration check.",
        ),
    ],
    doc="Integrates actor velocity and applies deferred collision responses, including the selector-0x0D specialized force-emitter view.",
    stable=True,
)

stable.fn(
    "Actor_UpdateGroundedState",
    "8B 4C 24 10 8B 44 24 0C 8B 11",
    hook=0x8,
    public=False,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("PKG_ActorRecord*", "record"),
        param("int32_t", "vertical_delta"),
        param("int32_t*", "inout_vertical_accum"),
    ],
    doc="Accumulates a vertical delta and updates the actor and record grounded-state flags when support changes.",
    stable=True,
)

stable.fn(
    "Physics_ApplyCollisionForceEmitter",
    "8B 5E 78 85 DB 0F 84 ??",
    match=-0xC,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("Actor_State*", "affected_actor"),
        param("PKG_ActorRecord*", "unused_record"),
        param("Physics_CollisionForceEmitterActorView*", "force_emitter"),
        param("Math_Vec3I32*", "inout_velocity_q12"),
        param("int32_t*", "inout_speed_limit_q12"),
    ],
    doc="Returns axial world-Y delta (radial zero). Zero maps local +Y to XYZ.",
    unstable=True,
)

stable.fn(
    "Actor_UpdateRotationFromVelocity",
    "?? 50 8B 41 08 50 E8 ??",
    match=-0x1E,
    ret="BOOL",
    params=[
        param("Actor_State*", "actor"),
        param("Math_Vec3I32 const*", "velocity"),
        param("int32_t", "turn_step"),
    ],
    doc="Turns the actor toward horizontal velocity and returns whether the pre-step yaw error exceeds the settled threshold.",
)

stable.fn(
    "Actor_ProcessHazardsAndDamage",
    "75 ?? 8B 7D 08 57 E8 ??",
    match=-0x1D,
    hook=0x6,
    ret="int32_t",
    params=[param("Actor_State*", "actor"), param("PKG_ActorRecord*", "record")],
    doc="Processes hazard and contact damage for actor, accumulating signed damage and knockback into record while enforcing its cooldown.",
    stable=True,
)

stable.fn(
    "Audio_InitializeSystem",
    "00 55 56 57 FF 15 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 8B 35",
    match=-0x5,
    hook=0x6,
    ret="int32_t",
    params=[],
    doc="Initializes the Miles audio system and opens the digital driver handle used by game audio playback.",
    stable=True,
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
    stable=True,
)

stable.fn(
    "Audio_StopAllSamples",
    "?? ?? ?? BE ?? ?? ?? ?? 8B 06 50 FF D7 83 C6 14",
    match=-0x5,
    hook=0x8,
    ret="int32_t",
    params=[],
    doc="Stops all active Miles sample playback slots while leaving the audio system initialized.",
    stable=True,
)

stable.fn(
    "Audio_StopAllSounds",
    "6A 00 50 FF 15 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90",
    match=-0x5,
    ret="int32_t",
    params=[],
    doc="Stops all currently playing game sounds through the active Miles digital driver.",
    stable=True,
)

stable.fn(
    "Audio_InitializeLevelAudio",
    "6A 7F 50 FF 15 ?? ?? ?? ?? C3 90",
    match=-0x5,
    ret="int32_t",
    params=[],
    doc="Initializes level audio playback state using the active Miles digital driver.",
    stable=True,
)

stable.fn(
    "Audio_StartSoundPlayback",
    "53 55 8B 6C 24 10 56 57 81 7D 00 52 49 46 46 0F 85 ??",
    hook=0x6,
    ret="void",
    params=[
        param("int32_t", "slot_index"),
        param("uint32_t const*", "wave_data"),
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
    stable=True,
)

stable.fn(
    "Audio_SetSampleVolume",
    "?? 8B 4C 24 08 C1 E9 ??",
    match=-0x13,
    hook=0x7,
    ret="int32_t",
    params=[param("int32_t", "slot_index"), param("int32_t", "volume")],
    doc="Set a sound slot's sample volume, scaling the game volume down to Miles' 0 through 128 range before calling AIL_set_sample_volume.",
    stable=True,
)

stable.fn(
    "Audio_SetSamplePitch",
    "0F AC D0 0C 3B 81 ?? ??",
    match=-0x1F,
    hook=0x6,
    ret="int32_t",
    params=[param("int32_t", "slot_index"), param("int32_t", "pitch_scale_q12")],
    doc="Set a sound slot's sample playback rate to base_playback_rate * pitchScaleQ12 / 4096.",
    stable=True,
)

stable.fn(
    "Audio_OpenStream",
    "A1 ?? ?? ?? ?? 81 EC 04",
    ret="void",
    params=[
        param(
            "int32_t*",
            "stream_record",
            doc="Record whose first dword receives the Audio_AILHStream. Bytes at +4 hold the music filename.",
        )
    ],
    doc="Open streamRecord[0] from the music filename stored at streamRecord+4 under the data/music directory. Clears the handle when audio is unavailable or the filename is empty.",
    stable=True,
)

stable.fn(
    "Audio_IsStreamPlaying",
    "8B 44 24 04 8B 00 85 C0 74 ?? 50 FF 15 ??",
    hook=0x6,
    ret="int32_t",
    params=[param("Audio_AILHStream*", "stream_handle_ptr")],
)

stable.fn(
    "Audio_SetStreamVolume",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 44 24 04 C1",
    hook=0x6,
    ret="void",
    params=[param("int32_t", "volume")],
    doc="Set the current music stream volume, scaling the game volume down to Miles' 0 through 128 range before calling AIL_set_stream_volume.",
    stable=True,
)

stable.fn(
    "Audio_PlayMusicStream",
    "56 8B 74 24 08 85 F6 74 ?? 56 E8 ?? ?? ?? ?? 8B 06 83 C4 04 A3 ?? ?? ?? ?? A1 ?? ?? ?? ?? 5E 85 C0 74 ?? 8B 4C 24 08 51 E8 ?? ?? ?? ??",
    ret="void",
    params=[param("int32_t*", "stream_record"), param("int32_t", "volume")],
    doc="Optionally opens streamRecord, publishes its handle as the active music stream, sets volume, starts playback, and sets the stream loop count to zero.",
    stable=True,
)

stable.fn(
    "Audio_PauseStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 01 50 FF 15 ?? ?? ?? ?? C3",
    ret="void",
    params=[],
    doc="Pauses the active Miles music stream when one is published.",
    stable=True,
)

stable.fn(
    "Audio_ResumeStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 00 50",
    ret="void",
    params=[],
    doc="Resumes the active Miles music stream when one is published.",
    stable=True,
)

stable.fn(
    "Audio_CloseMusicStream",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 6A 01 50 FF 15 ?? ?? ?? ?? A1",
    ret="void",
    params=[],
    doc="Pauses and closes the active Miles music stream, clears music_stream_handle, and decrements open_stream_count. Residual Miles/counter return is ignored.",
    stable=True,
)

stable.fn(
    "Audio_SetEnabledFlag",
    "8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 55 8B EC 51",
    hook=0x9,
    ret="void",
    params=[param("uint8_t", "enabled")],
    doc="Stores the global audio-system enabled flag.",
    stable=True,
)

stable.fn(
    "Audio_SetMusicFadeTarget",
    "55 8B EC 51 8B 45 08 0F BF 0D ?? ?? ?? ?? 85 C0 7D ??",
    hook=0x7,
    ret="int32_t",
    params=[
        param("int32_t", "target_volume_q12"),
        param("int16_t", "frame_count_minus_one"),
    ],
    doc="Sets the music fade target volume and stores frameCountMinusOne + 1 as the fade frame count. Negative targets scale the current target volume by -target/4096.",
    stable=True,
)

stable.fn(
    "Audio_FadeOutMusic",
    "F6 05 ?? ?? ?? ?? 06 75 ?? 66 A1 ??",
    hook=0x7,
    ret="void",
    params=[],
)

stable.fn(
    "Audio_FadeInMusic",
    "BE 49 00 6A 0F 50 E8 ??",
    match=-0xD,
    required=Required.EN,
    hook=0x7,
    ret="void",
    params=[],
    doc="Restores the saved fade target, clears fade/stop flags, and resumes the active music "
    "stream if one exists.",
    stable=True,
)

stable.fn(
    "Audio_StopMusicAndPause",
    "8A 0D ?? ?? ?? ?? B0 02 84 C8 75 ?? 0A C8 88 0D ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 44 24 04 A3 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Sets the music stop/pause flag and closes the active music stream. Callers ignore the "
    "residual return value.",
)

stable.fn(
    "Audio_ResetMusicState",
    "8B 44 24 04 A3 ?? ?? ?? ?? A0",
    hook=0x9,
    ret="void",
    params=[param("int32_t*", "stream_record")],
    doc="Stores the selected music stream record pointer for the fade/playback path and clears the low nibble of sound_system_flags. The previous scalar return was flag status.",
)

stable.fn(
    "Audio_StartMusicWithFade",
    "6A 1E 68 00 10 00 00 66 C7 05 ??",
    hook=0x7,
    ret="void",
    params=[param("int32_t*", "stream_record")],
    doc="Arms faded playback for the selected music stream record with a 0x1000 target volume over 31 fade frames, then resets music state. Residual flag return is ignored.",
    unstable=True,
)

stable.fn(
    "Audio_CheckStreamStatus",
    "8B 44 24 04 85 C0 74 ?? 50 E8 ??",
    hook=0x6,
    ret="int32_t",
    params=[param("Audio_AILHStream*", "stream_handle_ptr")],
    doc="Returns nonzero when stream_handle_ptr is non-null and the pointed Miles stream is currently playing.",
)

stable.fn(
    "Audio_ProcessMusicFade",
    "55 8B EC 83 EC 08 E8 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Processes one frame of music fade/stream state, advances the fade target, starts "
    "playback when allowed, pauses at zero volume, and recalculates stream volume.",
)

stable.fn(
    "Audio_StopMusicWrapper",
    "E9 ?? ?? ?? ?? 90 90 90 90 90 90 90 90 90 90 90 A1",
    ret="void",
    params=[],
    doc="Pure tail-call wrapper around the stop-music path. Callers use side effects only and no "
    "public return is modeled.",
)

stable.fn(
    "Trail_CheckBoneAvailable", "A1 ?? ?? ?? ?? 8B 88 84", ret="int32_t", params=[]
)

stable.fn(
    "Trail_ResetBone",
    "33 C9 C7 05 ?? ??",
    hook=0xC,
    ret="void",
    params=[],
    doc="Clears bone-trail state and entry buffers.",
)

stable.fn(
    "Trail_UpdateAndRenderBone",
    "55 8B EC 83 EC 34 A1 ??",
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("int32_t*", "movement_vec")],
    doc="Updates/renders bone-trail movement effects. movement_vec carries the output and caller overwrites native return metadata.",
)

stable.fn(
    "Trail_FindBonePath",
    "55 8B EC 81 EC 60 01 00 00 A1 ??",
    hook=0x9,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3I32*",
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
    "Nav_CalculatePolygonCenter",
    "57 F6 40 10 01 0F 84 ??",
    match=-0xB,
    hook=0x6,
    ret="void",
    params=[
        param("Collision_Node*", "collision_node"),
        param("Collision_Polygon*", "polygon"),
        param("Math_Vec3I32*", "out_center"),
    ],
    doc="Calculates the integer center point for a collision polygon in a collision/object node.",
)

stable.fn(
    "Nav_ProcessPathNode",
    "54 C1 0E 85 D2 0F 84 ??",
    match=-0x1D,
    hook=0x6,
    ret="int32_t",
    params=[
        param(
            "void*",
            "open_set",
            doc="Internal priority/open-set handle allocated by the pathfinding queue helpers.",
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
    unstable=True,
)

stable.fn(
    "Model_ResetState",
    "00 00 8B 46 7C 50 E8 ??",
    match=-0x11,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Clears actor behavior state and resets its animation, trail-chain, and component state.",
    stable=True,
)

stable.fn(
    "Projectile_DestroyAllLiveActors",
    "56 8B 35 ?? ?? ?? ?? 85 F6 74 ?? 57 C7 05 ?? ?? A8",
    hook=0x7,
    ret="void",
    params=[],
    doc="Detaches and destroys every live projectile actor after stopping its positional sound.",
    stable=True,
)

stable.fn(
    "Actor_CloneTemplateWithTemplateRelativeFixups",
    "C3 F6 43 65 08 0F 85 ??",
    match=-0x16,
    public=False,
    ret="Actor_State*",
    params=[
        param(
            "Actor_State*",
            "template_actor",
            doc="Actor template to clone or reuse.",
        ),
        param(
            "Actor_State**",
            "list_head",
            doc="Optional linked-list head that receives the result.",
        ),
    ],
    doc="Clones or reuses a template, rebases template pointers, clears runtime attachment_transform_node, resets state, and optionally links the result.",
)

stable.fn(
    "Actor_Destroy",
    "08 88 41 65 C3 51 E8 ??",
    match=-0x15,
    hook=0xB,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Defers destruction for owned actors when required. Otherwise frees the actor resource immediately.",
)

stable.fn(
    "Actor_AttachToParent",
    "53 55 56 57 8B 7C 24 14 57 E8 ??",
    hook=0x8,
    public=False,
    ret="Scene_TransformNodePrefix*",
    params=[
        param("Actor_State*", "child_actor"),
        param("Actor_State*", "parent_actor"),
        param(
            "Scene_TransformNodePrefix*",
            "attach_point_or_auto_sentinel",
            doc="Transform prefix. Null requests automatic selection, while -1 skips it before fallback.",
        ),
        param(
            "uint32_t",
            "attachment_flags_and_face_selector",
            doc="Bit 0 composes basis. Bits 1+ encode a one-based Mesh_Polygon selector.",
        ),
    ],
    doc="Zero auto-selects a socket. -1 skips auto.",
    unstable=True,
)

stable.fn(
    "Actor_SelectNearestAttachPoint",
    "7D ?? 89 45 F8 0F 84 ??",
    match=-0x18,
    hook=0x6,
    public=False,
    ret="void",
    params=[
        param("Actor_State*", "parent_actor"),
        param("Actor_State*", "child_actor"),
    ],
    doc="Selects the nearest type-6 socket through Actor_AttachmentSpriteNodeSlotView. Nonnegative slot, selection_gate <=0, random_face_count >0.",
    unstable=True,
)

stable.fn(
    "Actor_SetAlphaFade",
    "C6 85 FF 75 ?? 81 A1 ??",
    match=-0x6E,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("int32_t", "fade_steps"),
        param("int32_t", "fade_in"),
    ],
    doc="Configures and advances the actor's color and alpha fade toward transparent or opaque state.",
    stable=True,
)

stable.fn(
    "Actor_UpdateFadeOut",
    "BF C0 6A 00 50 56 E8 ??",
    match=-0x20,
    ret="BOOL",
    params=[param("Actor_State*", "actor"), param("int16_t", "fade_ticks")],
    doc="Advances an actor fade-out/lifecycle step and returns a low-byte boolean completion/status value.",
)

stable.fn(
    "Actor_AddToCollisionList",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 89 08 A3",
    hook=0xA,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc=(
        "Prepends actor to Actor_CollisionListHead using Actor_State.list_next for actor-to-actor collision processing."
    ),
)

stable.fn(
    "Shadow_ClearList",
    "C7 05 ?? ?? ?? ?? 00 00 00 00 C3 90 90 90 90 90 55",
    hook=0xA,
    ret="void",
    params=[],
    doc="Clears the per-frame shadow render-list head.",
    stable=True,
)

stable.fn(
    "Graphics_RenderAllShadows",
    "7E ?? 6A 00 51 57 E8 ??",
    match=-0x2B,
    hook=0x9,
    ret="void",
    params=[],
    doc="Renders queued shadows. Scene_RenderFrame calls it for side effects and ignores render-list status.",
)

stable.fn(
    "PKG_FixUpResourceObjectNodeDispatchByType",
    "56 8B 74 24 08 8B 46 04 85 C0 74 ?? 8B 0D ??",
    ret="void",
    params=[param("PKG_ObjectNodeFixupView*", "node")],
    doc=(
        "Rebases node sibling and cursor links, dispatches by node_type, and recurses "
        "through child siblings. Type 2 returns before child-link rebase, while handled "
        "non-type-2 payloads are fixed before recursion."
    ),
)

stable.fn(
    "PKG_FixUpResourceObjectNodeType1MeshActorLike",
    "?? ?? ?? 03 C2 89 86 80",
    match=-0x4E,
    ret="void",
    params=[
        param(
            "PKG_ObjectNodeFixupView*",
            "node",
            doc="Type-1 model node record whose pointer fields are rebased in place.",
        )
    ],
    doc="Type-1 mesh/actor-like node fixup. Mutates model-node fields in place.",
)

stable.fn(
    "PKG_FixUpResourceAnimationData",
    "51 8B 4C 24 08 8A 41 07 A8 02 0F 85 ??",
    ret="void",
    params=[
        param(
            "Animation_DataBlock*",
            "animation_data",
            doc="In-place block viewed as Animation_DataBlockRaw before its six channel-array offsets are rebased.",
        ),
        param(
            "uint8_t*",
            "blob_base",
            doc="Byte-addressed base added to relative channel and keyframe offsets.",
        ),
    ],
    doc="Rebases raw block/channel offsets. Marks 0x02/0x8000 and preserves direct -1.",
)

stable.fn(
    "PKG_FixUpResourceMaterialRefs",
    "8B 54 24 08 8B 44 24 04 56 8B 08 8B 35 ??",
    hook=0x8,
    ret="void",
    params=[
        param(
            "Mesh_MaterialRef*",
            "material_refs",
            doc="Array of material-reference records fixed in place.",
        ),
        param(
            "int32_t",
            "ref_count",
            doc="Positive number of material-reference records to process.",
        ),
    ],
    doc="Fixes ref_count material references in place. ref_count must be positive because the native loop always processes its first record.",
)

stable.fn(
    "PKG_FixUpResourceMaterialIndices",
    "8B 54 24 04 56 F6 02 80 75 ?? A1 ??",
    ret="void",
    params=[
        param(
            "PKG_MaterialIndexList*",
            "index_list_raw",
            doc="Pointer-list block with flag byte at +0, count, and relative list pointer.",
        )
    ],
    doc="Relocates a material pointer list and type-1 side pointers without validation. Flag bit 0x80 skips the block.",
)

stable.fn(
    "PKG_FixUpResourceRenderNodeEntries",
    "72 ?? 8D 46 F0 50 E8 ??",
    match=-0x15,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Mesh_RenderNodeEntry*",
            "entries",
            doc="Array base. Each entry has a sprite/material descriptor and a tail pointer rebased by the level blob base.",
        ),
        param("int32_t", "entry_count", doc="Number of render-node entries to fix."),
    ],
    doc="Fixes entry_count render-node records in place, including material descriptors and tail pointers. Zero count is a valid no-op.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "PKG_FixUpResourceSpriteEntry",
    "8B 44 24 04 8B 0D ?? ?? ?? ?? 89 08 8B 48",
    hook=0xA,
    ret="void",
    params=[
        param(
            "PKG_SpriteMaterialLayerRaw*",
            "sprite_material_raw",
            doc="0x0C sprite/material descriptor fixed in place. Material table base, material entry/index, optional index block.",
        ),
    ],
    doc="Resolves a signed material index and rebases the optional index list in place. Negative indices become null.",
)

stable.fn(
    "PKG_FixUpResourceObjectNodeType3ComplexActorLike",
    "8E 84 00 00 00 8B 15 ??",
    match=-0x49,
    ret="void",
    params=[
        param(
            "PKG_ObjectNodeFixupView*",
            "node",
            doc="Type-3 scene mesh node record fixed in place.",
        )
    ],
    doc="Type-3/complex node fixup. Mutates node sidecars in place.",
)

stable.fn(
    "PKG_FixUpResourceObjectNodeType8Ptr6COnly",
    "8B 4C 24 04 8B 41 6C 85 C0 74 ?? 8B 15 ??",
    hook=0x7,
    ret="void",
    params=[
        param(
            "PKG_ObjectNodeFixupView*",
            "node",
            doc="Type-8 simple node. Only the payload pointer is rebased.",
        )
    ],
    doc="Performs the type-8 pointer-only node fixup in place.",
)

stable.fn(
    "PKG_FixUpResourceObjectNodeType7SpriteEntry",
    "8B 44 24 04 83 C0 6C 50 E8 ??",
    hook=0x7,
    ret="void",
    params=[
        param(
            "PKG_ObjectNodeFixupView*",
            "node",
            doc="Type-7 compact object node. Passes the sprite or material descriptor at node.material_refs_offset to PKG_FixUpResourceSpriteEntry.",
        )
    ],
    doc="Fixes the type-7 node sprite/material payload in place.",
)

stable.fn(
    "PKG_FixUpResourceObjectNodeType0Hierarchy",
    "56 57 8B 7C 24 0C 8B 47 70 85 C0 74 ?? 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "PKG_ObjectNodeFixupView*",
            "node",
            doc="Group/type-0 hierarchy node record. Rebases nested lists and fixes child/object/polygon sidecars.",
        )
    ],
    doc="Type-0 hierarchy node fixup. Mutates linked child/object/polygon sidecars in place.",
)

stable.fn(
    "PKG_FixUpResourcePolygonList",
    "56 57 8B 7C 24 0C 8B 37 85 F6 0F 84 ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "int32_t*",
            "rel_node_list",
            doc="Relative node-list head/link field fixed in place.",
        ),
        param(
            "int32_t",
            "advance_by_link_slot",
            doc="Nonzero advances through consecutive link slots. Zero follows each rebased node link.",
        ),
    ],
)

stable.fn(
    "PKG_FixUpResourceLevelPointers",
    "?? ?? ?? ?? 83 C4 08 53",
    match=-0x11,
    ret="void",
    params=[
        param(
            "PKG_LevelHeader*",
            "level",
            doc="Level header whose relative resource lists are rebased in place.",
        )
    ],
    doc="Rebases package level-header pointers in place. Level_LoadStateMachine ignores the debug-log/status native return value.",
)

stable.fn(
    "PKG_FixUpResourceActorRecordPointers",
    "56 8B 74 24 08 57 8B 46 10 85 C0 74 ?? 8B 0D ??",
    ret="void",
    params=[
        param(
            "PKG_ActorTemplate*",
            "actor_template",
            doc="Actor template with three node refs at +0/+4/+8 and an animation state table.",
        )
    ],
    doc="Fixes actor-template animation and node references in place.",
)

stable.fn(
    "PKG_FixUpResourceComponentNodes",
    "06 85 C0 74 ?? 8B 0D ??",
    match=-0x10,
    hook=0x7,
    ret="void",
    params=[
        param(
            "PKG_ComponentData*",
            "component_data",
            doc="Component record. Fixes four sub-node pointers.",
        )
    ],
    doc="Relocates four component sub-node slots, owner links, and optional animation data, including nested spline descriptors.",
)

stable.fn(
    "Material_LoadAndFixup",
    "8B 4C 24 10 51 50 E8 ??",
    match=-0x74,
    public=False,
    ret="void",
    params=[
        param(
            "Material_SectionHeaderRaw*",
            "material_section_raw",
            doc="Raw material section header. The loader rebases its node and material-entry offsets in place.",
        ),
        param(
            "uint8_t*",
            "texture_data_entry_base",
            doc="Base of the separate texture-data package entry used to relocate descriptor pixel and palette offsets.",
        ),
    ],
    doc="Rebases links and texture entry offsets 4 and 8. Offsets 0xC through 0x6F become 25 surface pointers.",
)

stable.fn(
    "Material_GetGlobalTextureRefs",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 8B 44 24 04 89",
    hook=0x6,
    public=False,
    ret="void",
    params=[
        param(
            "Material_RefEntry*",
            "out_ref",
            doc="Receives the shared global material reference.",
        )
    ],
    doc="Copies the initialized shared material section, table entry, and frame-set pointers into out_ref.",
)

stable.fn(
    "Save_ProcessGameOperation",
    "0F BE 05 ?? ?? ?? ?? 53 32 DB 83 E8 ??",
    hook=0x7,
    ret="BOOL",
    params=[param("Save_OperationStatus*", "status_out")],
    doc="Polls the active save-game operation state. Operation 8 reads savegame.dat, operation 9 writes it, and operation 12 verifies by reading and comparing buffers.",
)

stable.fn(
    "Save_ReadGameFile",
    "14 56 57 6A 01 50 E8 ??",
    match=-0x22,
    required=Required.EN,
    hook=0x6,
    ret="BOOL",
    params=[param("void*", "buffer"), param("uint32_t", "size")],
    doc="Opens savegame.dat in rb mode, reads exactly size bytes into buffer, then verifies the file length equals size before returning TRUE.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Save_WriteGameFile",
    "08 56 6A 01 50 51 E8 ??",
    match=-0x20,
    required=Required.EN,
    hook=0x6,
    ret="BOOL",
    params=[param("void const*", "buffer"), param("uint32_t", "size")],
    doc="Writes one size-byte record from buffer to savegame.dat in wb mode, closes the file, and "
    "reports whether the write succeeded.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Save_InitGameOperation",
    "A1 ?? ?? ?? ?? 53 8B 5C",
    ret="void",
    params=[
        param("uint8_t", "operation"),
        param("void*", "buffer"),
        param("uint32_t", "size"),
    ],
    doc="Initializes, resets, or frees the global save-game operation state for the requested operation code. Operation 0x0b frees the verify-buffer local allocation.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Video_PlayMovieFile",
    "0D ?? ?? ?? ?? 8B 54 24 04 56 50 A1 ?? ?? ?? ?? 51 52 50 E8",
    match=-0x5,
    hook=0xA,
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
    doc="Open and play the supplied movie path. Closes playback on normal stop and requests shutdown on skip/Alt+F4 paths.",
    stable=True,
)

stable.fn(
    "Video_PlayMovieIntro",
    "56 8B 74 24 08 C7 05 ??",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "movie_index",
            doc="Index into the four-entry startup movie filename/type tables.",
        )
    ],
    doc="Build and play one startup movie path selected by movieIndex. Movie 0 initializes the player and movie 2 selects the alternate video rectangle.",
    stable=True,
)

stable.fn(
    "Timer_GetGameTime",
    "83 EC 08 FF 15 ??",
    hook=0x9,
    ret="float",
    params=[],
    doc="Returns the current GetTickCount-based game clock in seconds.",
    stable=True,
)

stable.fn(
    "Debug_RenderOverlay",
    "81 EC 0C 01 00 00 8B 0D ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Renders the developer position and frame-rate overlay.",
    stable=True,
)

stable.fn(
    "Input_RegisterButtonMapping",
    "56 8B 74 24 08 81 FE E8 ??",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "control_code",
            doc="Virtual-key code or gamepad control code. Gamepad codes start at 0x3e8.",
        ),
        param(
            "uint32_t",
            "button_mask",
            doc="Input_State.button_bits mask produced by this control.",
        ),
    ],
    doc="Registers a keyboard/gamepad control-to-button-mask binding. Codes below 0x3e8 append to the keyboard mapping arrays and refresh the button-name cache.",
)

stable.fn(
    "D3D_SetGammaFromMenuSetting",
    "DB 44 24 04 D8 15 ??",
    hook=0xA,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "gamma_setting",
            doc="Integer menu gamma setting centered at 5. Values below/above 5 map by 0.2 gamma-scale steps.",
        )
    ],
    doc="Converts the 0 through 10 menu gamma setting to a D3D gamma scale, clamps low values to 0.1, and forwards the scale to D3D_SetGammaRamp.",
)

stable.fn(
    "Config_ApplySettings",
    "8B 0D ?? ?? ?? ?? 56 57 8B",
    hook=0x6,
    ret="void",
    params=[],
    doc="Clear cached input bindings and rebuild keyboard/gamepad button masks from the loaded pcdogs.ini settings.",
)

stable.fn(
    "Config_LoadFromINI",
    "83 EC 08 57 68 ??",
    hook=0x9,
    ret="int32_t",
    params=[],
    doc="Loads pcdogs.ini when present and checksum-valid, clamps the display setting, restores default special-button binding when missing, reads player control bindings, then applies the resulting input mapping.",
)

stable.fn(
    "Config_SaveSettingsToINI",
    "A5 A2 ?? ?? ?? ?? E8 ??",
    match=-0x18,
    hook=0x7,
    ret="int32_t",
    params=[param("Config_Data const*", "config_data")],
    doc="Copies the supplied settings block into the global config while preserving the current display setting, reapplies input mappings, then writes pcdogs.ini with the PCDOGS header and control bindings.",
)

stable.fn(
    "Input_InitializeButtonMappings",
    "?? ?? 00 00 00 00 68 80",
    match=-0x32,
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
    hook=0x9,
    ret="int32_t",
    params=[],
    doc=(
        "Initialize built-in controller preset names and 10-button mapping tables for Hammerhead FX, "
        "Microsoft Sidewinder, Gravis Gamepad Pro, and Wingman RumblePad."
    ),
)

stable.fn(
    "Input_InitializeInputSubsystem",
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
    doc="Clears input state, initializes DirectInput joystick/force-feedback support for the game window, and sets the global force-feedback scale.",
    stable=True,
)

stable.fn(
    "Config_LoadAlternateFromINI",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 90 A1",
    ret="int32_t",
    params=[],
    doc="Alternate config-loading entry point that initializes controller and button mappings before tail-calling Config_LoadFromINI.",
    stable=True,
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
        "Samples keyboard mappings into the per-frame input state record, ORs configured masks into "
        "button_bits, triggers the screenshot path for F10/VK121, and ignores the return "
        "metadata."
    ),
)

stable.fn(
    "Input_ReadGamepad",
    "A0 ?? ?? ?? ?? 83 EC 08 84 C0 0F 84 ??",
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
        "+/-600 thresholds. Live analog hooks sample Input_GetJoystickAxis* in the same frame to keep input "
        "frame-local, and native callers ignore the return register."
    ),
)

stable.fn(
    "Input_ReadDevices",
    "46 06 66 89 46 04 E8 ??",
    match=-0x14,
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
    doc="Clears one Input_State and combines keyboard and gamepad sampling into it. player_index is preserved for the ABI.",
    unstable=True,
)

stable.fn(
    "DInput_ReleaseResources",
    "A1 ?? ?? ?? ?? 56 33 F6 3B C6 74 ?? 50",
    ret="void",
    params=[],
    doc="Releases DirectInput/input-owned resources. Joystick state buffer, joystick device, DirectInput interface, and keyboard mapping arrays.",
)

stable.fn(
    "Input_TriggerRumbleIfAllowed",
    "E8 ?? ?? ?? ?? 85 C0 75 ?? DB",
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "effect_source_id",
            doc="Caller-supplied effect source id preserved by the ABI.",
        ),
        param(
            "int32_t",
            "strong_feedback",
            doc="Nonzero doubles the force-feedback magnitude scale from 5000 to 10000.",
        ),
        param(
            "int32_t",
            "force_magnitude_fixed",
            doc="Fixed-point 12-bit force magnitude. 0x1000 represents 1.0 before scaling.",
        ),
        param(
            "int32_t",
            "duration_units",
            doc="Duration in units of 100000 microseconds.",
        ),
    ],
    doc="Returns immediately when rumble is suppressed. Otherwise computes DirectInput constant-force magnitude/duration and starts the effect.",
)

stable.fn(
    "Input_GetButtonIndex",
    "00 40 00 00 74 ?? 3D ??",
    match=-0x71,
    hook=0x7,
    ret="int32_t",
    params=[param("uint32_t", "control_mask")],
    doc="Maps an input button/control bitmask to the compact button-name index used by Input_FormatButtonName. Low masks 1 through 0x20 dispatch through input_button_mask_index_table.",
)

stable.fn(
    "Input_FormatButtonName",
    "8B 44 24 08 56 50 E8 ??",
    ret="char*",
    params=[
        param(
            "int32_t",
            "control_code",
            doc="Virtual-key or gamepad control code. Codes above 0xff are normalized by subtracting 0x2e8 for the string-id table.",
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
    hook=0x9,
    ret="char*",
    params=[param("int32_t", "button_code")],
    doc="Copies the localized label for button_code into the shared controls-menu buffer and returns it.",
    stable=True,
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
    doc="Calls GetAsyncKeyState and returns whether the high-order key-down bit is set.",
    stable=True,
)

stable.fn(
    "Input_GetPressedButton",
    "56 57 8B 3D ?? ?? ?? ?? 33 F6",
    hook=0x8,
    ret="int32_t",
    params=[],
    doc="Returns the first pressed keyboard virtual-key code, or gamepad codes 0x3e8 through 0x3fa for axis/button input, or -1 when no control is pressed.",
    stable=True,
)

stable.fn(
    "Graphics_HasFogCapability",
    "A1 ?? ?? ?? ?? 25 00",
    ret="int32_t",
    params=[],
    doc="Returns graphics capability mask 0x200 when fog support is available, otherwise zero.",
    stable=True,
)

stable.fn(
    "Input_ProcessWindowMessages",
    "A1 ?? ?? ?? ?? 83 EC 1C",
    ret="int32_t",
    params=[],
    doc="Pumps pending Win32 messages and updates the game quit flag when window processing requests shutdown.",
    stable=True,
)

stable.fn(
    "Graphics_RenderFrame",
    "51 53 E8 ?? ?? ?? ?? A1",
    hook=0x7,
    ret="uint8_t",
    params=[],
    doc=(
        "Runs the frame update/present boundary and returns its explicitly constructed byte status. "
        "Persistent EndScene or non-surface-lost Flip failures can spin indefinitely."
    ),
    stable=True,
)

stable.fn(
    "Input_ResetState",
    "E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 25",
    ret="void",
    params=[],
    doc="Clears raw input state, updates frame-transition flags, and records the current elapsed tick.",
    stable=True,
)

stable.fn(
    "Window_ProcessGameProc",
    "FF 00 01 00 00 C7 05 ??",
    cc=CallingConvention.STDCALL,
    match=-0xB,
    ret="int32_t",
    params=[
        param("HWND", "hwnd"),
        param("uint32_t", "u_msg"),
        param("uint32_t", "w_param"),
        param("int32_t", "l_param"),
    ],
    doc=(
        "Dispatches Win32 key, system, and window messages. WM_CLOSE tears down package, menu, "
        "display, input, and audio state, posts quit, and terminates through exit(0)."
    ),
    stable=True,
)

stable.fn(
    "PKG_CleanupFinalResourceGame",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E9",
    ret="void",
    params=[],
    doc="Performs final process-level audio, package-file, and resource-subsystem teardown.",
    stable=True,
)

stable.fn(
    "Window_GetMainHandle",
    "A1 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90",
    ret="HWND",
    params=[],
    doc=(
        "Returns the borrowed cached main HWND used by input, D3D, and UI paths. "
        "The accessor performs no validation or lifecycle operation and does not transfer ownership."
    ),
)

stable.fn(
    "Window_RunWinMain",
    "3C 89 5C 24 40 FF 15 ??",
    cc=CallingConvention.STDCALL,
    match=-0x27,
    hook=0x8,
    ret="int32_t",
    params=[
        param("HINSTANCE", "h_instance"),
        param("HINSTANCE", "h_prev_instance"),
        param("LPSTR", "lp_cmd_line"),
        param("int32_t", "n_cmd_show"),
    ],
    doc="Registers and creates the game window, initializes package/input/display systems, plays the intro movies, and owns the message/render loop through process exit. Startup continues after PKG_LocatePackagePath reports failure.",
    stable=True,
)

stable.fn(
    "PKG_InitializeSystem",
    "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1",
    ret="void",
    params=[param("HWND", "hwnd"), param("HINSTANCE", "h_instance")],
    doc=(
        "Bootstraps package/resource startup from the legacy hwnd and h_instance call ABI, "
        "although the EN body ignores both arguments. Opens the package TOC, initializes audio "
        "and render dispatch, and seeds graphics capability state."
    ),
    unstable=True,
)

stable.fn(
    "PKG_InitializeSystemMultiLanguage",
    "81 0D ?? ?? ?? ?? 00 00 00 04 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A1",
    required=Required.EU_SC,
    hook=0xA,
    ret="void",
    params=[param("HWND", "hwnd"), param("HINSTANCE", "h_instance")],
    doc=(
        "EU/SC entry that sets the multi-language game flag before entering "
        "PKG_InitializeSystem through the same legacy two-argument startup ABI."
    ),
    unstable=True,
)

stable.fn(
    "Window_RequestClose",
    "51 8D 44 24 00 6A 00 50 6A 00 6A 61 FF 15 ??",
    ret="void",
    params=[],
    doc="Requests orderly shutdown by sending WM_CLOSE to the main game window.",
    stable=True,
)

stable.fn(
    "Material_ReleaseSectionTextures",
    "F6 C5 08 75 ?? 50 E8 ??",
    match=-0x1B,
    hook=0x7,
    public=False,
    ret="void",
    params=[
        param(
            "Material_SectionHeader*",
            "material_section",
            doc="Material section whose loaded texture state is cleared.",
        )
    ],
    doc=(
        "Clears loaded texture state for material entries and type-3 node animation frames. "
        "A non-null node table is expected to have a positive node count."
    ),
    stable=True,
)

stable.fn(
    "Material_MarkReferencedByParent",
    "40 02 40 75 ?? 3B E9 ??",
    match=-0x28,
    ret="void",
    params=[
        param("Material_SectionHeader*", "material_section"),
        param("int32_t*", "material_ref"),
        param("int32_t", "parent_texture_descriptor"),
        param("int32_t", "material_count"),
    ],
    doc="Marks material_ref as shared when its texture descriptor matches an already-marked parent material entry.",
    stable=True,
)

stable.fn(
    "Material_MarkNodeTextureReferences",
    "8B 16 51 50 52 57 E8 ??",
    match=-0x72,
    hook=0x7,
    public=False,
    ret="void",
    params=[param("Material_SectionHeader*", "section")],
    doc="Marks texture references used by type-3 node frames. The earlier O(n^2) self-OR pass is state-neutral.",
    unstable=True,
)

stable.fn(
    "Material_LoadAllEntries",
    "03 C2 89 46 08 57 E8 ??",
    match=-0x43,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Material_SectionLoadView*",
            "section",
            doc="Load view over the raw material entries initialized for runtime use.",
        ),
        param(
            "uint8_t*",
            "texture_blob_base",
            doc="Byte-addressed base added to texture and palette offsets before loading or sharing.",
        ),
    ],
    doc=(
        "Rebases material descriptors, loads unshared textures, fixes type-3 frame pointers, and "
        "marks node references in a package section whose bounds are assumed valid."
    ),
)

stable.fn(
    "Audio_FixupSoundDefinitions",
    "30 A8 01 74 0C D1 E8 ??",
    match=-0x22,
    public=False,
    ret="void",
    params=[
        param(
            "Audio_SoundDefinition*",
            "sound_defs",
            doc="Sound-definition table fixed up in place.",
        ),
        param("int32_t", "count"),
        param("void*", "sample_data_base"),
    ],
    doc="Fixes a sound-definition table in place without validating its bounds. Tagged odd values become aliases, while even offsets and nested sample pointers are rebased to their runtime bases.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "PKG_CleanupResourceHandle",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 8B",
    required=Required.EN,
    ret="void",
    params=[param("void*", "resource_data")],
    doc="Releases resource_data through PKG_FreeResourceData. Callers receive no status result.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "PKG_LoadEntryAlloc",
    "8B 44 24 04 6A 01 6A 00 50 E8 ??",
    hook=0x6,
    public=False,
    ret="void*",
    params=[
        param(
            "int32_t",
            "toc_index",
            doc="Package TOC index to load with destination allocation enabled.",
        ),
        param(
            "int32_t",
            "unused_zero",
            doc="Unused ABI slot. All known callers pass zero.",
        ),
    ],
    doc="Allocating wrapper for PKG_LoadEntry. Retains the ignored second cdecl slot.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Level_LoadStateMachine",
    "8B 44 24 04 56 57 33 FF 8D 4C 40 24 A1 ??",
    required=Required.EN,
    ret="Level_DataHeader*",
    params=[param("int32_t", "level_index")],
    doc="Resumable package loader keyed by unchecked level_index. States 0/2/5 load TOC entries 0x24 + level_index * 3 + (0,1,2).",
)

stable.fn(
    "Level_UnloadResources",
    "A1 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 4C",
    ret="void",
    params=[param("Level_DataHeader*", "level_resource_data")],
    doc="Releases the level material section, level blob, and cached texture buffers.",
    stable=True,
)

stable.fn(
    "PKG_LoadEntry",
    "33 C0 5B C3 8B C7 25 ??",
    match=-0x14,
    ret="void*",
    params=[
        param("int32_t", "toc_index", doc="Index into package_toc."),
        param(
            "void*",
            "dest_buffer",
            doc="Optional caller-supplied destination. Allocated when NULL.",
        ),
        param(
            "int32_t",
            "unused_mode",
            doc="Unused third ABI slot retained by every known three-argument callsite.",
        ),
    ],
    doc="Loads one of 138 raw TOC entries while retaining all three cdecl slots. Index, file bounds, destination capacity, seek, and read results are unchecked.",
    stable=True,
)

stable.fn(
    "PKG_FreeResourceData",
    "8B 44 24 04 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 B0",
    ret="void",
    params=[param("void*", "resource_data")],
    doc="Releases resource_data through PKG_FreeResourceMemory. Callers receive no status result.",
    stable=True,
)

stable.fn(
    "PKG_LocatePackagePath",
    "81 EC 10 01 00 00 57 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Builds package candidates in fixed buffers from the current directory and setup paths on drives C through Y. Appends do not check capacity, and final failure reports an error but returns normally.",
    stable=True,
)

stable.fn(
    "PKG_OpenAndReadTOC",
    "?? ?? 68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8D 4C 24 0C 68 ?? ?? ?? ?? 51 E8 ?? ?? ?? ?? 83",
    match=-0xD,
    hook=0x6,
    ret="void",
    params=[],
    doc="Opens the package, reads one 0x800-byte sector, and copies its first 0x450 bytes as 138 raw (offset,size) pairs. The reference header has 135 valid aligned entries, sentinels 27/29/34, and zeros from 0x450 through 0x7FF.",
    stable=True,
)

stable.fn(
    "PKG_Close",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 50 E8 ?? ?? ?? ?? 83 C4 04 C7 05 ?? ?? ?? ?? 00 00 00 00 C3 90 90 90 83",
    ret="void",
    params=[],
    doc=(
        "Idempotently closes the open package file handle when present and clears pkg_file_handle. "
        "The native File_Close residue is not a status or ownership-transfer contract."
    ),
)

stable.fn(
    "Graphics_CheckActorVisibilityAndFrustum",
    "83 EC ?? 8B 54 24 14 53 55 56 8B 02 8B 4A ?? C1 F8 ??",
    hook=0x7,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3I32*",
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
    doc="Tests position against the active camera frustum planes. Returns 0 when culled.",
)

stable.fn(
    "Graphics_CheckFrustumCull",
    "8B 44 24 0C 8B 4C 24 04 56 8B 74 24 0C 50 56 51 E8 ??",
    hook=0x8,
    ret="int32_t",
    params=[
        param(
            "Math_Vec3I32*",
            "position",
            doc="World-space fixed-point position forwarded to Graphics_CheckActorVisibilityAndFrustum.",
        ),
        param(
            "int32_t",
            "cull_radius",
            doc="Object radius/margin used for frustum and distance checks.",
        ),
        param(
            "uint8_t",
            "cull_flags",
            doc="Flag byte forwarded to Graphics_CheckActorVisibilityAndFrustum.",
        ),
    ],
    doc="Visibility wrapper that toggles render-state visibility bits from Graphics_CheckActorVisibilityAndFrustum and returns 1 when visible.",
)

stable.fn(
    "Collision_CheckActorGround",
    "83 EC 18 53 55 8B 2D ??",
    ret="BOOL",
    params=[
        param(
            "Math_Vec3I32*",
            "position",
            doc="World-space fixed-point position tested against active ground/shadow collision planes.",
        ),
        param(
            "int32_t",
            "cull_radius",
            doc="Radius/clearance value. The function uses radius >> 2 for plane-distance thresholds.",
        ),
        param(
            "uint32_t",
            "collision_flags",
            doc="Flags/mask used to skip matching collision entries. Bit 0x8000 selects the positive-depth threshold path.",
        ),
    ],
    doc="Tests a position/radius against the active collision/shadow plane list and returns nonzero when the point is rejected by the plane set.",
)

stable.fn(
    "Powerup_Cleanup",
    "56 8B 35 ?? ?? ?? ?? 85 F6 74 ?? 57 C7 05 ?? ?? 63",
    hook=0x7,
    ret="void",
    params=[],
    doc="Destroys every live powerup actor after stopping its position-bound audio state.",
    stable=True,
)

stable.fn(
    "Powerup_CloneActorFromTemplate",
    "68 ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 83 C4 08 85",
    match=-0xA,
    hook=0x6,
    ret="Actor_State*",
    params=[
        param(
            "PKG_ActorTemplate*",
            "actor_template",
            doc="Powerup actor-template/clone-source selected from Level_RuntimeData.powerup_actor_template_slots[0 through 15]. Nullptr or a failed clone returns nullptr.",
        )
    ],
    doc="Clones a level-owned powerup actor template into the live powerup actor list. The source template comes from the fixed 16-slot powerup_actor_template_slots table.",
)

stable.fn(
    "Powerup_SpawnActorFromEntry",
    "33 C0 8A 47 0C 8B 0D ??",
    match=-0x18,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Powerup_Entry*",
            "powerup_entry",
            doc="Powerup spawn entry containing flags, type id, local/world position, and optional parent actor record pointer.",
        )
    ],
    doc="Clones level powerup template slot 15 when flag 0x20 is set, otherwise the unchecked uint8 powerup_type, then initializes attachment, position, selectors, and behavior state. Runtime requires unflagged types <=14, a populated selected template, and a nonnull parent for flag 0x08 without checking them.",
)

stable.fn(
    "Powerup_InitializeSystem",
    "8B 15 ?? ?? ?? ?? 56 85",
    hook=0x6,
    ret="void",
    params=[],
    doc=(
        "Always installs actor-phase slot 8 and collision callbacks, then marks powerup entries pending "
        "only when current level data is nonnull and both entity_count and powerup_count are positive. "
        "The entity-count condition is treated as a format invariant, not a powerup-list bound."
    ),
)

stable.fn(
    "Powerup_UpdateActorState",
    "4E 65 20 EB ?? 56 E8 ??",
    match=-0x1A,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "powerup_actor",
            doc=(
                "Live powerup actor. In this path the actor record/source slot at Actor_State can point back to the Powerup_Entry that spawned the actor."
            ),
        )
    ],
    doc="Actor-phase slot 8 callback. Its return value is ignored. It resets transition state and respawns or removes the powerup actor.",
)

stable.fn(
    "Powerup_FilterCollision",
    "83 7C 24 10 FE 75 ?? 8B 44 24 04 8B 88 F4 00 00 00 F6 41 0D 12 74 ??",
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    public=False,
    ret="int32_t",
    params=[
        param("Actor_State*", "powerup_actor"),
        param("Actor_State*", "other_actor"),
        param("int32_t", "reserved_zero", doc="Unused slot. The dispatcher passes zero."),
        param("int32_t", "collision_result"),
    ],
    doc="Powerup_CollisionCallback returning -2 to suppress flagged, disabled, or non-player collisions and 0 for player collection. Flags 0x12 also clear collision class and request lifecycle removal.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Powerup_HandleCollection",
    "00 F6 47 0D 20 0F 84 ??",
    match=-0xC,
    required=Required.EN,
    hook=0x6,
    ret="void",
    params=[
        param("Actor_State*", "powerup_actor"),
        param("Actor_State*", "collector_actor"),
    ],
    doc="Applies a collected powerup or advances its trail-spawn countdown, then clears collision class, selects transition phase 8, and detaches the actor. The sole caller discards EAX.",
)

stable.fn(
    "Tree_InitializeMapNode",
    "12 8B 44 24 08 50 E8 ??",
    match=-0x1B,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header. Root node, allocation size, and compare callback.",
        ),
        param(
            "void*",
            "node_payload",
            doc="Payload pointer returned by Tree_AllocateMapNode. The node header begins immediately before it.",
        ),
    ],
    doc="Clears the hidden node header for node_payload and inserts the node into tree.",
    stable=True,
)

stable.fn(
    "Tree_InsertMapNode",
    "56 57 8B 7C 24 0C 8B 07 85 C0 75 ?? 8B 44 24 10 89 40 ?? 89 00 89 07 5F 5E C3",
    ret="void",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header whose root/list links and compare callback control insertion.",
        ),
        param(
            "Tree_MapNode*",
            "node_header",
            doc="Internal node header to link into the tree/list.",
        ),
    ],
    doc="Links node_header into the TreeMap circular root list, updating the root through the tree comparator when required.",
)

stable.fn(
    "Tree_SpliceRingIntoRootList",
    "8B 4C 24 08 85 C9 74 ?? 8B 44 24 04 56 8B 71 ?? 8B 00 8B 50 ?? 89 70 ??",
    ret="void",
    params=[
        param(
            "Tree_Map*", "tree", doc="Tree header whose root/list links are updated."
        ),
        param(
            "Tree_MapNode*",
            "node_header",
            doc="Internal node header to detach. Nullptr is accepted as a no-op.",
        ),
    ],
    doc="Splices node_header's circular sibling ring into the root list headed by tree[0], concatenating the child ring into the root ring.",
)

stable.fn(
    "Tree_RemoveMapNode",
    "56 8D 70 EC 56 57 E8 ??",
    match=-0xD,
    ret="void",
    params=[
        param(
            "Tree_Map*", "tree", doc="Tree header whose root/list links are updated."
        ),
        param(
            "void*",
            "node_payload",
            doc="Payload pointer for the node to remove. Node metadata is stored immediately before it.",
        ),
    ],
    doc="Removes node_payload from tree, detaches/rethreads child links, and rebalances the remaining tree when needed.",
)

stable.fn(
    "Tree_RebalanceMap",
    "8B 44 24 04 53 55 56 8B 28 57 33 DB 8B FD 8B 6D 04 0F BF 4F 10 8B 34 8D ?? ?? ?? ?? 3B F3 74 ?? 0F BF 57 10 8D 47 14 8D 4E 14 89 1C 95 ?? ?? ?? ??",
    ret="void",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header whose root chain is bucketized and rebuilt.",
        )
    ],
    doc="Rebuilds/rebalances tree using the compare callback and the temporary tree_map_buckets array.",
)

stable.fn(
    "Tree_RotateAndDetachMapNode",
    "8B 08 89 0A 50 53 E8 ??",
    match=-0x3C,
    ret="void",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header passed back to Tree_InsertMapNode while rotating detached nodes.",
        ),
        param(
            "Tree_MapNode*",
            "node_header",
            doc="Internal node header pointer, storage before the user payload.",
        ),
    ],
    doc="Walks upward from node_header, detaches affected parent links, toggles side bits, and reinserts nodes into tree.",
)

stable.fn(
    "Tree_GetFirstMapNode",
    "?? 83 C6 14 56 50 E8 ??",
    match=-0xE,
    ret="void*",
    params=[
        param(
            "Tree_Map*", "tree", doc="Tree header to pop from. Nullptr returns nullptr."
        )
    ],
    doc="Returns and removes the first/root payload from tree, or nullptr when the tree is empty.",
)

stable.fn(
    "Tree_FixupMapAfterInsert",
    "33 C0 5B C3 57 56 E8 ??",
    match=-0x2A,
    ret="int32_t",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header containing root, allocation size, and compare callback.",
        ),
        param(
            "void*",
            "node_payload",
            doc="Payload pointer for the newly inserted/adjusted node. Returns -1 when nullptr.",
        ),
    ],
    doc="Fixes TreeMap ordering after insertion or priority update by comparing node_payload against parent/root links, detaching/reinserting when needed, and returning -1 for null payload.",
)

stable.fn(
    "Tree_AllocateMapNode",
    "8B 44 24 04 8B 48 04 51 E8 ??",
    hook=0x7,
    ret="void*",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header whose allocation-size field in the record controls the node allocation size.",
        )
    ],
    doc="Allocates one tree node block and returns the user payload pointer 0x14 bytes after the node header.",
)

stable.fn(
    "Tree_FreeMapNode",
    "8B 44 24 04 85 C0 74 ?? 83 C0 EC 50 E8 ??",
    hook=0x6,
    ret="BOOL",
    params=[
        param(
            "void*",
            "node_payload",
            doc="Payload pointer returned by Tree_AllocateMapNode. Nullptr is accepted and returned unchanged.",
        )
    ],
    doc="Frees the full tree node allocation by subtracting the hidden node header from node_payload.",
)

stable.fn(
    "Tree_CreateMap",
    "6A 0C E8 ?? ?? ??",
    hook=0x7,
    ret="Tree_Map*",
    params=[
        param(
            "int32_t",
            "payload_size",
            doc="Size of each user payload. The stored allocation size is payload_size + 0x14.",
        ),
        param(
            "Tree_MapCompareCallback",
            "compare_func",
            doc="Compare callback stored in the tree header and later called with two payload pointers.",
        ),
    ],
    doc="Allocates and initializes a tree header. Empty root, node allocation size, and compare callback.",
)

stable.fn(
    "Tree_DestroyMap",
    "37 8B 46 0C 50 57 E8 ??",
    match=-0x11,
    ret="void",
    params=[
        param(
            "Tree_Map*",
            "tree",
            doc="Tree header to destroy. All linked node headers are freed before the header itself.",
        )
    ],
    doc="Destroys every node in tree by detaching list links and freeing each node block, then frees the tree header.",
)

stable.fn(
    "Signal_ClearQueue",
    "C6 05 ?? ?? ?? ?? 00 C3 90 90 90 90 90 90 90 90 A0 ?? ?? ?? ?? 84 C0 74 ?? 8B 54 24 04 55 56 57 32 C0 0F BF 32 85 F6 74 ?? C6 42 02 00 B9 ??",
    hook=0x7,
    ret="int32_t",
    params=[],
    doc="Clears the queued signal/event count in signal queue state.",
)

stable.fn(
    "Signal_Poll",
    "74 ?? C6 42 02 00 B9 ??",
    match=-0x17,
    ret="int32_t",
    params=[param("int16_t*", "event_buffer"), param("int32_t", "max_events")],
)

stable.fn(
    "Signal_ClearTimedEventList",
    "8B 0D ?? ?? ?? ?? 85 C9 74 ?? 83",
    hook=0x6,
    ret="void",
    params=[],
)

stable.fn(
    "Actor_ResetChainState",
    "78 ?? FF 75 ?? 8B 0D ??",
    match=-0x17,
    hook=0x6,
    ret="void",
    params=[param("Actor_State*", "actor"), param("int32_t", "chain_index")],
    doc="Clears the selected actor chain fields at +0x78 and +0x7A. When +0x7C is the -1 sentinel, links the actor into Signal_TimedEventListHead.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Game_TriggerPause",
    "05 ?? ?? ?? ?? 0A A1 ??",
    match=-0x9,
    hook=0x6,
    ret="void",
    params=[param("int32_t", "pause_type")],
    doc="Begins a pause transition, stops active sounds, and seeds the pause and unload-delay counters.",
    stable=True,
)

stable.fn(
    "Game_UpdateAndRenderScene",
    "A0 ?? ?? ?? ?? 84 C0 0F 84 ?? ?? ?? ?? E8",
    ret="void",
    params=[],
    doc="Runs the scene update/render pipeline. Resource activation installs Graphics_ClearBackground and Game_RenderOverlays in the fixed nullable +0xC8 pre-list and +0xC4 scene post-sort slots.",
    stable=True,
)

stable.fn(
    "Signal_UpdateTimedEvents",
    "53 56 8B 35 ?? ?? ?? ?? 33",
    hook=0x8,
    ret="void",
    params=[],
    doc="Updates queued timed signal events for the current frame.",
)

stable.fn(
    "Graphics_ClearBackground",
    "66 81 3D ?? ?? ?? ?? 00 10 7D ?? A1 ??",
    hook=0x9,
    ret="void",
    params=[],
    doc="Clears to black or the level background color. Both exits are side-effect-only.",
)

stable.fn(
    "Graphics_UpdateFadeCounters",
    "?? ?? ?? 83 C4 04 C3 A0",
    match=-0x31,
    ret="void",
    params=[],
    doc="Advances the pause and screen-fade counters and updates their Q12 fade values.",
    stable=True,
)

stable.fn(
    "Game_RenderOverlays",
    "E8 ?? ?? ?? ?? 8B 44 24 04",
    ret="void",
    params=[
        param(
            "int32_t",
            "reserved_arg",
            doc="Legacy callback slot with no observed semantic effect.",
        )
    ],
    doc="Side-effect-only overlay pass that updates transition/fade state and renders UI sprites. The tail-called pause-menu helper has no semantic result.",
)

stable.fn(
    "Math_GenerateRandom",
    "8B 0D ?? ?? ?? ?? 56 8B C1",
    hook=0x6,
    ret="uint32_t",
    params=[
        param(
            "uint32_t",
            "range",
            doc="Exclusive upper bound for the scaled random result.",
        )
    ],
    doc="Advances the Park Miller MINSTD seed and returns a value from zero up to but excluding range.",
    stable=True,
)

stable.fn(
    "Math_SetRandomSeed",
    "8B 4C 24 04 A1 ??",
    hook=0x9,
    ret="uint32_t",
    params=[
        param(
            "uint32_t",
            "seed",
            doc="New Park-Miller seed. Zero is normalized to 1, values above 0x7fffffff subtract 0x7fffffff once.",
        )
    ],
    doc="Stores a normalized unsigned Park-Miller seed and returns the previous seed.",
    stable=True,
)

stable.fn(
    "Script_ExecuteBehaviorScript",
    "56 8B 74 24 08 85 F6 57 89 35 ??",
    ret="void",
    params=[param("Entity_State*", "entity")],
)

stable.fn(
    "Graphics_ComputeVertexColors",
    "83 EC 08 A1 ?? ?? ?? ?? 8B",
    hook=0x8,
    ret="void",
    params=[
        param("Mesh_RuntimeVertex*", "runtime_vertices"),
        param("Graphics_PolygonRenderRef*", "polygon_ref"),
        param("uint32_t*", "out_colors"),
    ],
    doc="Writes three or four adjusted packed RGB colors for a polygon render reference.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Graphics_AdjustColorQuadRGB",
    "83 EC 08 A1 ?? ?? ?? ?? 33",
    hook=0x8,
    ret="void",
    params=[
        param(
            "uint32_t const*",
            "input_quad_rgb",
            doc="Four packed 0x00BBGGRR colors.",
        ),
        param(
            "uint32_t*",
            "output_quad_rgb",
            doc="Four packed destination colors receiving clamped RGB adjustments.",
        ),
    ],
    doc="Applies the global RGB adjustment bytes, centered at 0x80, to four packed RGB colors with per-channel 0 through 255 saturation.",
)

stable.fn(
    "Math_Atan2ToCosPhaseFP12",
    "8B 44 24 08 8B 4C 24 04 50 51 E8 ?? ?? ?? ?? 0F",
    hook=0x8,
    ret="int32_t",
    params=[param("int32_t", "sin_value"), param("int32_t", "cos_value")],
    doc="Converts the Q12 atan2 lookup angle for sin_value and cos_value into its wrapped complementary cosine phase.",
    stable=True,
)

stable.fn(
    "Math_BuildQuaternionFromMatrix",
    "DF 03 C3 85 C0 0F 8E ??",
    match=-0x21,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Math_Matrix3x3I16*",
            "matrix",
            doc="Input row-major signed fixed-point 3x3 rotation matrix.",
        ),
        param(
            "Math_QuaternionI16*",
            "out_quat",
            doc="Receives the converted Q14 quaternion as w/x/y/z.",
        ),
    ],
    doc="Converts a 3x3 fixed-point rotation matrix to a Q14 quaternion using the trace-positive path or largest-diagonal fallback.",
)

stable.fn(
    "Math_BuildRotationFromVectors",
    "FF FF 89 55 0C 0F 8F ??",
    match=-0x56,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Math_Vec3I16*",
            "up_vector",
            doc="First normalized signed fixed-point vector.",
        ),
        param(
            "Math_Vec3I16*",
            "forward_vector",
            doc="Second normalized signed fixed-point vector.",
        ),
        param(
            "Math_Matrix3x3I16*",
            "out_matrix",
            doc="Receives the rotation matrix mapping up_vector toward forward_vector.",
        ),
    ],
    doc="Builds a Q12 rotation matrix from two 3-component vectors. Emits a neutral matrix for near-equal vectors and a 180-degree fallback for opposing vectors.",
)

stable.fn(
    "Audio_StopSound",
    "08 84 C0 74 ?? 56 E8 ??",
    match=-0xF,
    ret="void",
    params=[param("int32_t", "slot_index")],
    doc="Frees the sound slot with resource cleanup enabled, then releases the Miles sample handle when the slot free succeeds.",
    stable=True,
)

stable.fn(
    "Audio_FreeSoundSlot",
    "75 ?? 8B 48 04 89 0D ??",
    match=-0x2E,
    hook=0x7,
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
            "Math_Vec3I32*",
            "position_or_actor_position_ptr",
            doc="Position pointer for 3D playback. Actor sound-node calls often pass &actor to position_x, but script/cutscene paths may pass non-actor globals.",
        ),
    ],
    doc=(
        "Starts playback from an already-resolved Audio_SoundDefinition. Script/dialogue paths "
        "can call this directly and bypass Audio_PlayLevelSoundIndexAtPosition."
    ),
)

stable.fn(
    "Audio_AllocateSoundSlot",
    "74 ?? 6A 00 6A 08 BE ?? ?? ?? ??",
    match=-0xB,
    ret="int32_t",
    params=[param("Audio_SoundDefinition*", "sound_def")],
)

stable.fn(
    "Audio_FindSoundByType",
    "?? 85 DB 75 ?? 8B 35 ?? ?? ?? ??",
    match=-0xF,
    ret="int32_t",
    params=[
        param("int32_t", "slot_index"),
        param("Audio_SoundDefinition*", "sound_def"),
        param("Math_Vec3I32*", "position"),
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
    match=-0x1C,
    hook=0x6,
    ret="uint8_t",
    params=[
        param(
            "Math_Vec3I32*",
            "source_pos",
            doc="Nullable source/world position. Null treats distance and x/z delta as zero.",
        ),
        param(
            "uint32_t",
            "audible_radius",
            doc="Maximum audible distance. Sources at or beyond this radius write volume 0 and return 0.",
        ),
        param(
            "int32_t*",
            "out_volume_q12",
            doc="Optional 0 through 0x1000 attenuation output. Null skips volume calculation.",
        ),
        param(
            "int16_t*",
            "out_pan_angle",
            doc="Output 12-bit pan/facing angle relative to player_facing_angle.",
        ),
    ],
    doc=(
        "Computes positional-audio attenuation and pan from source_pos relative to the selected listener/camera position. "
        "Game_FrameTransitionFlags bit 0x10 selects Audio_ListenerCameraPos_Flag10Set when set and Audio_ListenerCameraPos_Flag10Clear when clear."
    ),
)

stable.fn(
    "Audio_PauseAllSounds",
    "56 57 33 FF BE ?? ?? ?? ?? 57",
    hook=0x9,
    ret="void",
    params=[],
    doc="Pauses active sound slots by setting their pitch to zero while preserving cached pitches.",
    stable=True,
)

stable.fn(
    "Audio_ResumeAllSounds",
    "56 57 33 FF BE ?? ?? ?? ?? 66",
    hook=0x9,
    ret="void",
    params=[],
    doc="Restores each active sound slot from its cached nonzero pitch.",
    stable=True,
)

stable.fn(
    "UI_UpdateAndRenderSprites",
    "55 8B EC 83 EC 4C A1 ??",
    required=Required.EN,
    hook=0x6,
    public=False,
    ret="void",
    params=[],
    doc="Updates and renders a sentinel-terminated package sprite list without validating its bounds. No known list-bound check protects the 0xAC-byte record selected by link_index.",
    stable=True,
)

stable.fn(
    "Script_OpAnimateSpriteMove",
    "83 EC ?? 53 55 56 57 8B 7C 24 ?? 33 C9 33 DB 8B 2F 83 C5 ?? 8B C5 89 2F",
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Sprite move-tween opcode. Reads target/duration/easing bytes and loops by restoring *ip until the computed end tick.",
    unstable=True,
)

stable.fn(
    "Script_OpAnimateZoom",
    "33 D2 8A 50 FF 8B E9 ??",
    match=-0x7D,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    unstable=True,
)

stable.fn(
    "Script_OpProcessSpriteRotation",
    "55 8B EC 83 EC ?? 53 56 57 8B 7D ?? 33 C9 33 DB 8B 37 83 C6 ?? 8B C6 89 37",
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Reads sprite rotation animation target/duration/easing bytes, updates actor rotation fields, and loops until the end tick.",
    unstable=True,
)

stable.fn(
    "Script_OpAnimateTarget",
    "14 52 8D 04 50 8B 15 ??",
    match=-0x98,
    hook=0x7,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    unstable=True,
)

stable.fn(
    "Script_OpSetSpriteProperty",
    "0C 49 8D 04 48 8B 0D ??",
    match=-0x47,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc="Script opcode that mutates PKG_SpriteEntry fields.",
)

stable.fn(
    "Input_CalculateMovementVector",
    "0F 49 83 F9 09 0F 87 ??",
    match=-0x1E,
    ret="void",
    params=[
        param(
            "Math_Vec3I32*",
            "out_move_vec",
            doc="Output vector with at least three int32_t components written by this helper.",
        ),
        param("int32_t", "player_index"),
        param(
            "int32_t",
            "heading_angle",
            doc="Camera-relative heading angle consumed by Math_SinCosFP12. Known callers use the low 16 bits / 12-bit fixed-point angle domain, while the ABI remains int32_t.",
        ),
    ],
    doc="Writes out_move_vec[0 through 2] from D-pad or analog movement input, using heading_angle in the low-16-bit / 12-bit fixed-point angle domain for Math_SinCosFP12.",
    unstable=True,
)

stable.fn(
    "Replay_StartDemoPlayback",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 8B 48",
    ret="void",
    params=[],
    doc="Starts demo playback from loaded replay data by installing the input frame pointer, saving/restoring random seed state, and setting the input replay flag.",
    stable=True,
)

stable.fn(
    "Replay_StopDemoPlayback",
    "8B 15 ?? ?? ?? ?? A1 ?? ?? ?? ?? 83 E2 DF 85 C0 89 15 ?? ?? ?? ?? 74 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Clears demo playback input mode, frees loaded replay data when present, clears the replay data pointer, and restores the saved random seed.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Input_Update",
    "8B 49 08 89 48 08 E8 ??",
    match=-0x24,
    hook=0x7,
    ret="void",
    params=[],
    doc=(
        "Per-frame input updater that copies current/previous player input state, reads "
        "keyboard/gamepad devices, applies toggles, and handles replay input."
    ),
    stable=True,
)

stable.fn(
    "Actor_PollMovementSignals",
    "F6 C4 04 75 ?? 56 E8 ??",
    match=-0x11,
    ret="int32_t",
    params=[
        param("Actor_State*", "actor"),
        param(
            "int32_t",
            "max_events",
            doc="Signal_Poll capacity. 3 in Actor_ProcessMovementCommands and 1 in Actor_ProcessMovementBehavior.",
        ),
    ],
    doc="Normalizes right-moving state when required, then polls movement signals into the Actor_State +0xD8 scratch overlay. Returns the Signal_Poll event count.",
)

stable.fn(
    "Graphics_InitializeDispatchTables",
    "6A 00 E8 ?? ?? ?? ?? B8",
    match=-0xE,
    hook=0x7,
    ret="void",
    params=[],
    doc="Initializes the nine actor phase callback slots. Dispatchers use a signed selector without checking its range.",
    stable=True,
)

stable.fn(
    "Actor_RunNullCallback",
    "C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 4C 24 04",
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    callable=False,
    public=False,
    ret="void",
    params=[
        param(
            "Actor_State*",
            "unused_actor",
            doc="Actor passed by both callback tables. Intentionally unused.",
        )
    ],
    doc="Pure single-RET callback installed in phase and preprocess slot 0. Scene_TraverseNodeTree can call phase slot 0 directly.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Actor_HandleDefaultUpdate",
    "33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 4C 24 04 8B 81 88 00 00 00 A9 00 10 08 00 74 ?? A9 00 00 02 00 0F BF 81 B8 00 00 00 74 ?? F7 D8 50 51 E8 ??",
    hook=hook(0x2, kind=HookKind.HOTPATCH),
    ret="int32_t",
    params=[
        param("void*", "subject", doc="Collision-dispatch subject. Intentionally unused."),
        param("void*", "other_object", doc="Other collision object. Intentionally unused."),
        param("Collision_Polygon*", "collision_poly", doc="Collision polygon. Intentionally unused."),
        param("int32_t", "collision_depth", doc="Collision depth. Intentionally unused."),
    ],
    doc=(
        "Default ActorCollisionProbeCallback installed in collision slot 0. It ignores all four "
        "cdecl arguments and returns constant zero to the result-consuming dispatcher."
    ),
    unstable=True,
)

stable.fn(
    "Actor_ApplyVerticalMovement",
    "74 ?? F7 D8 50 51 E8 ??",
    match=-0x1D,
    hook=0xA,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Actor-phase slot 4. Applies signed vertical speed when movement flags enable that path, and indexed dispatchers ignore EAX.",
    stable=True,
)

stable.fn(
    "Actor_FollowAttachedMovement",
    "F6 C4 04 75 ?? 51 E8 ??",
    match=-0x18,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Actor-phase slot 6 that follows an attached actor/component. Indexed dispatchers ignore EAX.",
    unstable=True,
)

stable.fn(
    "Actor_ProcessMovementCommands",
    "00 00 FF F7 F7 FF E8 ??",
    match=-0xC,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Actor-phase slot 5 that dispatches the three-entry movement-command queue. Indexed dispatchers ignore EAX.",
)

stable.fn(
    "Actor_ProcessMovementBehavior",
    "E8 ?? ?? ?? ?? 8B 8E E0",
    match=-0x25,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Actor-phase slot 7 coordinating vertical movement and script progress. Indexed dispatchers ignore EAX.",
)

stable.fn(
    "String_SetTable",
    "8B 44 24 04 A3 ?? ?? ?? ?? C3 90 90 90 90 90 90 8B 54",
    required=Required.EN_SC,
    hook=0x9,
    ret="void",
    params=[
        param(
            "int16_t*",
            "string_table",
            doc="Base pointer to the loaded string table block used by String_GetByIndex.",
        ),
    ],
    doc="Stores the active package/localization string table pointer.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "String_GetByIndex",
    "BA 09 04 00 00 8B 0D ??",
    match=-0x18,
    required=Required.EN_SC,
    hook=0x7,
    ret="char*",
    params=[param("int32_t", "string_index")],
    doc="Scans the active string table and returns its matching string. Special IDs share a non-reentrant scratch buffer, and ID 0x453 copies into unchecked local buffers.",
    abi_status=AbiStatus.VERIFIED,
)

stable.fn(
    "Title_InitializeSpots",
    "51 53 55 56 33 DB 57 66 89 1D ??",
    hook=0x6,
    ret="void",
    params=[],
)

stable.fn(
    "Title_UpdateSpots",
    "53 55 8B 6C 24 0C 85 ED 75 ?? 66 A1 ??",
    hook=0x6,
    ret="void",
    params=[
        param(
            "BOOL",
            "hold_animation",
            doc="Nonzero freezes animation timing and sound while still rendering current spots.",
        )
    ],
    doc="Updates and renders title spots. hold_animation freezes frame advance and timed sound.",
    stable=True,
)

stable.fn(
    "PKG_LoadTitleScreenResources",
    "6A 00 6A 00 E8 ?? ?? ?? ?? 83 C4 08",
    required=Required.EN,
    hook=0x9,
    public=False,
    ret="uint8_t",
    params=[],
    doc=(
        "Loads and fixes the three title package resources and returns an AL boolean. "
        "A partial failure does not unwind resources loaded earlier in the sequence."
    ),
    stable=True,
)

stable.fn(
    "Title_CleanupScreenResources",
    "FF FF E8 ?? ?? ?? ?? A1 ?? ?? ?? ?? 50 E8",
    match=-0x5,
    hook=0x7,
    ret="void",
    params=[],
    doc=(
        "Stops title audio and releases replay data, material textures, package data, and two "
        "resource handles. It does not clear the corresponding globals, so callers treat it as one-shot."
    ),
    stable=True,
)

stable.fn(
    "Title_UpdateAndRenderScreen",
    "8B 44 24 04 53 33 DB 83 F8 FF 0F 84 ??",
    required=Required.EN,
    public=False,
    ret="uint8_t",
    params=[
        param(
            "int32_t",
            "command",
            doc="Normal callers pass 0. -1 forces the title-screen shutdown/reset path.",
        ),
    ],
    doc=(
        "Advances or tears down the title-screen state machine for command 0 or -1 and returns "
        "its continuation result in AL."
    ),
    stable=True,
)

stable.fn(
    "Camera_UpdateProjection",
    "2C 8D 4E 30 50 51 E8 ??",
    match=-0x14,
    ret="void",
    params=[param("Camera_Runtime*", "camera"), param("int32_t", "focal_distance")],
    doc="Updates camera focal distance and rebuilds projection state when the value changes.",
)

stable.fn(
    "Camera_BuildViewMatrix",
    "D1 89 45 E0 89 45 E8 ??",
    match=-0x4A,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Camera_FrustumDirTable*",
            "frustum_dirs",
            doc="Receives five int16 direction triples (stride 8), not a 3x3 view matrix.",
        ),
        param("int16_t*", "screen_half_size"),
        param("int32_t", "focal_distance"),
    ],
    doc="Writes the five frustum direction entries derived from the current view angles, screen half size, and focal distance.",
)

stable.fn(
    "Camera_CalculateClipDistance",
    "04 00 00 2B C6 50 E8 ??",
    match=-0x13,
    hook=0x7,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera"), param("int16_t", "fov")],
)

stable.fn(
    "Camera_SetDefaultFOV",
    "66 C7 40 0A 0F 03 E8 ??",
    match=-0xA,
    hook=0x9,
    ret="int32_t",
    params=[param("Camera_Runtime*", "camera")],
)

stable.fn(
    "Camera_SetViewport",
    "66 8B 54 24 0C 66 A3 ??",
    match=-0xA,
    ret="void",
    params=[
        param("int16_t", "viewport_x"),
        param("int16_t", "viewport_y"),
        param("int16_t", "viewport_w"),
        param("int16_t", "viewport_h"),
        param("int32_t", "apply_projection"),
    ],
    doc="Stores the global viewport and optionally refreshes camera projection state.",
    stable=True,
)

stable.fn(
    "Camera_RecomputePitchYawFromEyeTarget",
    "?? ?? ?? 8B 4D EC 8B 55",
    match=-0x62,
    hook=0x6,
    ret="int32_t",
    params=[param("Camera_Pose*", "pose")],
    doc="Recomputes pose pitch/yaw from the pose eye and target positions (the inverse of a look-at).",
)

stable.fn(
    "Camera_Initialize",
    "00 00 00 F0 FF 7F E8 ??",
    match=-0xA,
    ret="void",
    params=[
        param(
            "Graphics_ListState*",
            "render_state",
            doc="Render-list state initialized with camera defaults, viewport, shake, and roll.",
        )
    ],
    doc=(
        "Initializes camera defaults, applies the 640x480 viewport, and clears the two "
        "fade/transition counters."
    ),
)

stable.fn(
    "Mem_MallocWithRetry",
    "53 56 57 8B 7C 24 10 57 E8 ??",
    hook=0x7,
    ret="void*",
    params=[param("uint32_t", "size"), param("char const*", "context")],
    doc="Allocates size bytes, prompting to retry after failures and exiting when the user declines.",
    stable=True,
)

stable.fn(
    "Mem_MallocCRT",
    "FF 35 ?? ?? ?? ?? FF 74 24",
    hook=0x6,
    ret="void*",
    params=[param("uint32_t", "size")],
    doc="Allocates size bytes through the game's CRT malloc path and returns the allocated pointer.",
    stable=True,
)

stable.fn(
    "Mem_FreeMemoryExCRT",
    "56 8B 74 24 08 85 F6 74 24 56 E8 ?? ?? ?? ?? 59 85 C0 56 74 0A 50 E8",
    ret="void",
    params=[param("void*", "ptr")],
    doc="Frees a pointer through the CRT small-block allocator or process heap. NULL is ignored.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "UI_ShowConfirmDialog",
    "00 56 57 50 51 52 E8 ??",
    match=-0x14,
    hook=0xA,
    callable=False,
    public=False,
    ret="BOOL",
    params=[param("char const*", "format")],
    doc="Formats C varargs into a 256-byte local while the generic formatter uses INT32_MAX as its effective capacity, so expansions over 255 bytes can overwrite the stack. Current internal calls are fixed literals and consume at most 77 bytes including NUL.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "Mem_AllocateHandle",
    "56 8B 35 ?? ?? ?? ?? 85 F6 57",
    hook=0x7,
    ret="uint32_t",
    params=[param("uint32_t", "size")],
)

stable.fn(
    "UI_ShowErrorMessage",
    "?? ?? 8D 44 24 10 50 68",
    match=-0x1C,
    hook=0xA,
    callable=False,
    public=False,
    ret="void",
    params=[param("char const*", "format")],
    doc="Formats C varargs into a 256-byte local while the generic formatter uses INT32_MAX as its effective capacity, so expansions over 255 bytes can overwrite the stack. Current internal calls consume at most 50 bytes including NUL, even for the %u maximum.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "Mem_InitializeAllocator",
    "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8 ?? ?? ?? ?? 33",
    hook=0xA,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Mem_FreeAllExtents",
    "56 BE ?? ?? ?? ?? 8B 46",
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Mem_ReleaseHandle",
    "8B 4C 24 04 56 49 0F 88 ??",
    ret="BOOL",
    params=[param("uint32_t", "handle")],
    doc="Checks a 1-based extent handle, frees its backing block, and returns BOOL status.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Mem_IsValidHandle",
    "8B 44 24 04 85 C0 7E ?? 3D A0 86 01 00 7F ?? 8B C8 C1 E1 04 39 81 ?? ?? ?? ?? 75 ?? B8 01 00 00 00 C3 33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 A1 ??",
    hook=0x6,
    ret="BOOL",
    params=[param("uint32_t", "handle")],
    doc="Checks the range and generation of a 1-based extent handle.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Timer_GetElapsedTickCount",
    "A1 ?? ?? ?? ?? 56 8B 35 ?? ?? ?? ?? 83",
    ret="uint32_t",
    params=[],
    doc="Returns wrap-safe milliseconds elapsed since the game timer was initialized.",
    stable=True,
)

stable.fn(
    "Input_ClearState",
    "57 B9 40 00 00 00 33 C0 BF ?? ?? ?? ?? F3 AB 5F C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 A1 ??",
    hook=0x6,
    ret="void",
    params=[],
    doc="Zeros the input_state_buffer raw input/VK clear buffer. Native callers ignore the constant-zero native return metadata.",
)

stable.fn(
    "Input_IsKeyPressed",
    "A1 ?? ?? ?? ?? 33 C9 85 C0 53",
    ret="BOOL",
    params=[param("uint8_t", "scan_code")],
    doc="Returns the mapped logical-key state for scan_code, or false when no mapping exists.",
    stable=True,
)

stable.fn(
    "Input_SetKeyUp",
    "8B 44 24 04 25 FF 00 00 00 C6 80 ?? ?? ?? ?? 00 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 0D ??",
    hook=0x9,
    ret="void",
    params=[param("uint8_t", "scan_code")],
    doc="Clears the raw input-state byte indexed by scan_code.",
    stable=True,
)

stable.fn(
    "Display_IsActive",
    "8B 0D ?? ?? ?? ?? 33 C0 85 C9 0F 95",
    hook=0x6,
    ret="BOOL",
    params=[],
    doc="Returns TRUE when Display_CurrentWindowHandle is non-null, meaning a display/D3D mode is currently bound to a game window.",
)

stable.fn(
    "Display_SetMode",
    "A1 ?? ?? ?? ?? 56 57 8B 7C",
    ret="int32_t",
    params=[
        param(
            "HWND",
            "hwnd",
            doc="Game window handle to bind to the active display/D3D mode.",
        )
    ],
    doc="Initializes or reuses the active display/D3D mode for hwnd, stores Display_CurrentWindowHandle on success, and resets input state.",
    stable=True,
)

stable.fn(
    "Display_ReleaseMode",
    "A1 ?? ?? ?? ?? 85 C0 74 ?? 50 E8 ?? ?? ?? ?? 59 C3 90 90 90 90 90 90 90",
    ret="void",
    params=[],
    doc="Releases the active display/D3D mode when Display_CurrentWindowHandle is set.",
    stable=True,
)

stable.fn(
    "Window_SetDefaultResolution",
    "C7 05 ?? ?? ?? ?? 80 02",
    hook=0xA,
    public=False,
    ret="void",
    params=[
        param(
            "int32_t",
            "reserved_zero",
            doc="Legacy argument. The sole caller passes zero and the body ignores it.",
        )
    ],
    doc="Sets the default game resolution to 640 by 480 pixels. reserved_zero is ignored.",
    stable=True,
)

stable.fn(
    "DDraw_CreateEx",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC ?? ?? ?? ?? 00 00 00 00 03 FF FF 80 00 01 00 00",
    cc=CallingConvention.STDCALL,
    match=-0xC,
    hook=0x6,
    ret="HRESULT",
    params=[
        param("Win32_GUID*", "lp_guid"),
        param("void* *", "lplp_dd"),
        param("Win32_GUID*", "iid"),
        param("COM_IUnknown*", "p_unk_outer"),
    ],
    doc="Import entry for ddraw!DirectDrawCreateEx. Used by graphics initialization to create the primary DirectDraw7 interface.",
    stable=True,
)

stable.fn(
    "DDraw_EnumerateExA",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC CC CC CC ?? ?? ?? ?? 00 00 00 00 03 FF FF 80 00 01 00 00",
    cc=CallingConvention.STDCALL,
    match=-0x6,
    hook=0x6,
    ret="HRESULT",
    params=[
        param("DDraw_EnumCallbackExA", "lp_callback"),
        param("LPVOID", "lp_context"),
        param("DWORD", "dw_flags"),
    ],
    doc="Import entry for ddraw!DirectDrawEnumerateExA. Used by D3D_EnumerateDirectDrawDevices with D3D_EnumDriverCallback and enumeration flags.",
    stable=True,
)

stable.fn(
    "DInput_CreateA",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC CC",
    cc=CallingConvention.STDCALL,
    hook=0x6,
    ret="HRESULT",
    params=[
        param("HINSTANCE", "hinst"),
        param("DWORD", "dw_version"),
        param("DInput_IDirectInputA**", "pp_di"),
        param("COM_IUnknown*", "p_unk_outer"),
    ],
    doc="Terminal dinput!DirectInputCreateA import thunk ending at 0x445132. INT3 padding spans 0x445132 through 0x44513F, 0x445140 begins non-code table data, and the next function is Video_SetMovieSyncAdjust at 0x445E40.",
)

stable.fn(
    "Video_SetMovieSyncAdjust",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x10,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieSoundRate",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x16,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieSoundPrecision",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x1C,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieSoundChannels",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x22,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieXSize",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x28,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieYSize",
    "?? ?? ?? ?? 1C 00 00 00 03 FF FF 80 00 04 00 00 FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ??",
    match=0x2E,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Video_GetMovieCurrentFrame",
    "08 D5 A8 01 FF 25 ?? ??",
    match=-0x8,
    required=Required.EN,
    hook=0x6,
    ret="int32_t",
    params=[param("int32_t", "movie_handle")],
    doc="Returns the current frame index for a movie handle through the winplay import thunk.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Video_GetMovieTotalFrames",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    ret="int32_t",
    params=[param("int32_t", "movie_handle")],
    doc="Returns the total frame count for a movie handle through the winplay import thunk.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.callback_type(
    "Movie_StreamingCallback",
    ret="void",
    params=[],
    calling=CallingConvention.CDECL,
    doc="No-argument callback invoked after each configured interval of streamed movie bytes.",
    unstable=True,
)

stable.fn(
    "Movie_InitSoundSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[param("HWND", "hwnd")],
    doc="Initializes shared DirectSound at priority cooperative level for hwnd. Returns 0 or WINPLAY error 0x3F8.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitVideoSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("HWND", "hwnd"),
        param(
            "BOOL",
            "fullscreen_exclusive",
            doc="Nonzero selects fullscreen-exclusive DirectDraw cooperation. Zero selects normal windowed cooperation.",
        ),
    ],
    doc="Initializes WINPLAY DirectDraw for normal or fullscreen-exclusive cooperation and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_ShutdownMovie",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[param("int32_t*", "movie_handle")],
    doc="Shuts down the WINPLAY movie handle passed by pointer and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_ShutdownVideo",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[param("int32_t*", "surface_handle")],
    doc="Shuts down the WINPLAY video-surface handle passed by pointer and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_ShutdownSound",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[param("int32_t*", "sound_handle")],
    doc="Shuts down the WINPLAY sound handle passed by pointer and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_ReturnPlaybackMode",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_StopTimer",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[param("int32_t", "movie_handle")],
    doc="Stops the WINPLAY timer for a movie handle and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_StartTimer",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    ret="int32_t",
    params=[param("int32_t", "movie_handle")],
    doc="Thunk forwarding the movie handle to the movie player's start-timer entry.",
)

stable.fn(
    "Movie_MapVideo",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("int32_t", "surface_handle"),
        param("int32_t", "flags", doc="Opaque WINPLAY mapping flags word."),
    ],
    doc="Maps the WINPLAY video surface with an opaque flags word and returns imported status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitMoviePlayback",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("int32_t", "movie_handle"),
        param("int32_t", "surface_handle"),
        param("int32_t", "sound_handle"),
    ],
    doc="Binds the WINPLAY movie, video-surface, and sound handles for playback.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitSound",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("int32_t*", "sound_handle_out"),
        param("uint32_t", "buffer_bytes"),
        param("uint32_t", "bytes_per_sample"),
        param("BOOL", "precision_is_not_four_bytes"),
        param("uint32_t", "block_bytes"),
        param("int32_t", "channels"),
        param("int32_t", "sample_rate"),
        param("int32_t", "sample_precision"),
        param(
            "uint32_t",
            "sound_sync_mode",
            doc="Opaque synchronization-policy value. Behavior is proved for values 2 and 4, but the enum domain is unknown.",
        ),
    ],
    doc="Initializes WINPLAY audio metadata and stores the opaque audio/video synchronization policy.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitPlaybackMode",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("HWND", "hwnd"),
        param("int32_t", "surface_handle"),
        param(
            "BOOL",
            "force_640x480x16",
            doc="Nonzero enforces a 640x480x16 display mode before surface setup.",
        ),
        param(
            "BOOL",
            "attach_window_clipper",
            doc="Nonzero creates a window clipper and attaches it to the playback surface.",
        ),
    ],
    doc="Configures WINPLAY display mode and optional window clipping for a playback surface.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitVideo",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("int32_t*", "surface_handle_out"),
        param("int32_t", "movie_handle"),
        param("int32_t", "movie_width"),
        param("int32_t", "movie_height"),
        param("int32_t", "mapping_x"),
        param("int32_t", "mapping_y"),
        param("uint32_t", "mapping_arg7"),
        param("uint32_t", "mapping_arg8"),
        param("uint32_t", "output_width"),
        param("uint32_t", "output_height"),
        param(
            "BOOL",
            "zero_mapping_origin",
            doc="Nonzero substitutes zero for mapping_x and mapping_y.",
        ),
        param(
            "BOOL",
            "preserve_destination_mapping",
            doc="Nonzero suppresses later destination remapping.",
        ),
        param(
            "uint32_t",
            "streamer_flags",
            doc="Opaque WINSTR flags word. Bits 0x8, 0x20, and 0x80 are tested but their meanings remain unresolved.",
        ),
    ],
    doc="Initializes WINPLAY video mapping and output geometry through its 13-slot ABI.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_InitMovie",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    public=False,
    ret="int32_t",
    params=[
        param("int32_t*", "movie_handle_out"),
        param(
            "Movie_StreamingCallback",
            "streaming_callback",
            doc="Nullable no-argument callback invoked after each configured byte interval.",
        ),
        param(
            "uint32_t",
            "callback_interval_bytes",
            doc="Processed-byte countdown between streaming callback invocations. Zero is used with a null callback.",
        ),
        param("char const*", "movie_path"),
        param("uint32_t", "buffer_size"),
    ],
    doc="Initializes a WINPLAY movie handle with an optional byte-interval streaming callback and buffer size.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Movie_PlayFrame",
    "25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    match=-0x7,
    hook=0x6,
    ret="int32_t",
    params=[
        param("int32_t", "movie_handle"),
        param("int32_t", "surface_handle"),
        param("int32_t", "sound_handle"),
        param(
            "void*",
            "alpha_effects_handle",
            doc="Nullable opaque object forwarded to WINSTR Alpha_GetBuffer.",
        ),
        param("RECT const*", "destination_rect"),
        param("uint32_t", "codec_frame_arg6"),
        param("uint32_t", "codec_frame_arg7"),
        param("uint32_t", "codec_start_arg15"),
    ],
    doc="Forwards the WINPLAY frame ABI. Codec Frame arguments 6/7 and Start argument 15 remain opaque words.",
)

stable.fn(
    "Movie_ShutdownSoundSystem",
    "FF 25 ?? ?? ?? ?? FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "Movie_ShutdownVideoSystem",
    "FF 25 ?? ?? ?? ?? E8",
    hook=0x6,
    ret="int32_t",
    params=[],
)

stable.fn(
    "String_ParseInt",
    "53 55 56 57 8B 7C 24 14 83 3D ?? ?? ?? ?? 01 7E",
    ret="int32_t",
    params=[param("char const*", "string")],
    doc="Parses whitespace, sign, and decimal digits from a read-only NUL-terminated string.",
)

stable.fn(
    "String_ParseAtoi",
    "FF 74 24 04 E8 ?? ?? ?? ?? 59 C3",
    hook=0x9,
    ret="int32_t",
    params=[param("char const*", "string")],
    doc="Forwards a read-only string to String_ParseInt and returns its signed result.",
)

stable.fn(
    "File_FlushBuffer",
    "?? 57 50 FF 76 10 E8 ??",
    match=-0x26,
    hook=0x6,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
    doc="Drains pending bytes from a writable CRT stream buffer and resets its buffer cursor.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "File_Close",
    "?? A8 83 74 ?? 56 E8 ??",
    match=-0x14,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
    doc="Flushes and closes a CRT stream, releasing its owned buffer and temporary filename.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
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
    doc="Opens a file with an explicit mode string and sharing flag, returning the game's CRT-compatible file handle.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "File_Open",
    "0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3 55 8B EC 81 EC F8 00 00 00 53 56 8B 75",
    match=-0x5,
    hook=0x6,
    ret="File_Handle*",
    params=[param("char const*", "filename"), param("char const*", "mode")],
    doc="Opens a file with the game's default sharing behavior and returns the CRT-compatible file handle.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Sort_RunQuickSort",
    "55 8B EC 81 EC F8 00 00 00 53 56 8B 75 0C 57 83 FE 02 0F 82 ?? ?? ?? ??",
    hook=0x9,
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
    "Sort_RunInsertionSort",
    "55 8B EC 8B 45 08 57 8B 7D 0C 3B F8 76 ?? 8B 4D 10 53 03 C1 56 89 45 0C",
    ret="char*",
    params=[
        param("char*", "first_element"),
        param("char*", "last_element"),
        param("uint32_t", "element_size"),
        param("Sort_CompareCallback", "compare"),
    ],
    doc="Insertion-sort helper used by Sort_RunQuickSort for small partitions. Returns a residual element/swap cursor ignored by the qsort caller.",
    unstable=True,
)

stable.fn(
    "Mem_SwapBytes",
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
    match=-0x1A,
    hook=0x6,
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
    doc="Returns the logical CRT stream position, correcting for buffered read-ahead and text-mode CRLF translation.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "File_WriteElements",
    "8B DF 75 ?? 33 C0 E9 ??",
    match=-0x19,
    ret="uint32_t",
    params=[
        param("void const*", "buffer"),
        param("uint32_t", "size"),
        param("uint32_t", "count"),
        param("File_Handle*", "stream"),
    ],
    doc="Fwrite-like buffered writer. Writes count elements of size bytes from buffer to stream and returns the element count written.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "Mem_Realloc",
    "75 ?? FF 74 24 18 E8 ??",
    match=-0xA,
    ret="void*",
    params=[param("void*", "ptr"), param("uint32_t", "size")],
    doc="Implements CRT realloc behavior. Allocates for a null ptr, frees on zero size, and otherwise resizes without losing contents.",
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "CRT_TestFdivBugFallback",
    "55 8B EC 83 EC 18 DD 05 ??",
    hook=0x6,
    callable=False,
    public=False,
    ret="int32_t",
    params=[],
    doc="Runs the x87 arithmetic fallback used to detect the Pentium FDIV precision erratum.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_TestPentiumFdivBug",
    "68 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 85 C0 74",
    callable=False,
    public=False,
    ret="int32_t",
    params=[],
    doc="Checks the OS Pentium FDIV-erratum feature flag, falling back to an x87 arithmetic test.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "String_FormatFloat",
    "65 59 74 ?? 46 83 3D ??",
    match=-0x10,
    ret="void",
    params=[
        param(
            "char*",
            "buffer",
            doc="NUL-terminated float string buffer modified in place.",
        )
    ],
    doc="CRT _forcdecpt-style helper. Inserts the locale decimal-point character before the exponent/non-digit suffix by shifting the remainder right.",
)

stable.fn(
    "String_ConvertToExponentialFloat",
    "55 8B EC 80 3D ??",
    hook=0xA,
    callable=False,
    public=False,
    ret="char*",
    params=[
        param("double*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "digit_count"),
        param("int32_t", "precision"),
    ],
    doc="Formats sign and mantissa digits, printing decimal_point_position - 1 as the scientific exponent.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "String_ConvertFloatToFixed",
    "80 3D ?? ?? ?? ?? 00 53 55",
    hook=0x7,
    ret="char*",
    params=[
        param("double const*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "precision", doc="Number of digits after the decimal point."),
    ],
    doc="Uses decimal_point_position to place digits or leading fractional zeroes and returns buffer.",
)

stable.fn(
    "String_ConvertFloatGeneral",
    "51 DD 07 DD 1C 24 E8 ??",
    match=-0xA,
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
    doc="Uses decimal_point_position - 1 and precision flags to choose fixed or scientific formatting.",
)

stable.fn(
    "String_FormatScientificFloat",
    "24 10 E8 ?? ?? ?? ?? 80",
    match=-0x15,
    hook=0xB,
    callable=False,
    public=False,
    ret="char*",
    params=[
        param("double*", "value"),
        param("char*", "buffer"),
        param("int32_t", "digit_count"),
        param("int32_t", "precision"),
    ],
    doc="Forwards value, buffer, digit count, and precision to the CRT exponent-conversion core.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "String_ConvertFloatToFixedWrapper",
    "24 0C E8 ?? ?? ?? ?? 80",
    match=-0x11,
    hook=0xB,
    ret="char*",
    params=[
        param("double const*", "value", doc="Input double to convert."),
        param("char*", "buffer", doc="Destination NUL-terminated output buffer."),
        param("int32_t", "precision", doc="Number of digits after the decimal point."),
    ],
    doc="Sets the shared fixed-format flag, calls String_ConvertFloatToFixed, and returns buffer.",
)

stable.fn(
    "String_InsertSpace",
    "56 8B 74 24 0C 56 E8 ??",
    match=-0x9,
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
            doc="Number of bytes to make room for at buffer. Zero leaves the string unchanged.",
        ),
    ],
    doc="Shifts the NUL-terminated string right by insertCount bytes using a backward memmove.",
)

stable.fn(
    "String_GetCharType",
    "01 00 00 77 ?? 8B 0D ??",
    match=-0xD,
    hook=0x7,
    ret="int32_t",
    params=[
        param(
            "int32_t",
            "ch",
            doc="Character code. Multibyte characters are handled as a packed low-word value.",
        ),
        param("int32_t", "mask", doc="CRT ctype mask to test."),
    ],
    doc="CRT _isctype-style helper. Returns mask bits for ch using the ctype table or CRT_GetStringTypeA for multibyte characters.",
)

stable.fn(
    "File_WriteChar",
    "8B 5E 10 A8 82 0F 84 ??",
    match=-0xB,
    ret="int32_t",
    params=[param("int32_t", "ch"), param("File_Handle*", "stream")],
    doc="Fputc-like writer. Writes ch to stream, flushing or filling the stream buffer as needed.",
)

stable.fn(
    "File_WriteCharWithCounter",
    "EB ?? 51 FF 75 08 E8 ??",
    match=-0x17,
    hook=0x6,
    ret="void",
    params=[
        param("int32_t", "ch"),
        param("File_Handle*", "stream"),
        param("int32_t*", "written_count"),
    ],
    doc="Writes ch to stream, increments written_count on success, or stores -1 on failure.",
    abi_status=AbiStatus.VERIFIED,
)

stable.fn(
    "CRT_NLGNotify1",
    "53 51 BB ?? ?? ?? ?? 8B",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    hook=0x7,
    ret="void",
    params=[
        param(
            "int32_t",
            "notification_code",
            doc="Stack-passed NLG notification code popped by ret 4. The helper also consumes EAX as the handler/continuation address and EBP as the establisher frame.",
        ),
    ],
    doc="Records implicit EAX/EBP unwind state and pops the stack notification code with ret 4. Compiler-private custom ABI.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "File_FlushToDisk",
    "8B 44 24 04 3B 05 ?? ?? ?? ?? 73 ?? 8B C8 8B",
    hook=0xA,
    ret="int32_t",
    params=[param("int32_t", "file_no")],
    doc="Flushes the OS handle for a CRT file descriptor with FlushFileBuffers. Returns 0 on success and -1 on invalid descriptor or Win32 failure.",
)

stable.fn(
    "File_WriteBytes",
    "55 8B EC 81 EC 14 04 00 00 8B 4D 08 53 3B 0D ??",
    hook=0x9,
    ret="int32_t",
    params=[
        param("int32_t", "file_no"),
        param("void const*", "buffer"),
        param("int32_t", "byte_count"),
    ],
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "File_AllocateBuffer",
    "56 8B 74 24 08 FF 76 10 E8 ??",
    ret="int32_t",
    params=[param("File_Handle*", "stream")],
    doc="Allocates or assigns a buffered I/O area for stdout/stderr-like streams. Returns 1 when a buffer is installed, otherwise 0.",
)

stable.fn(
    "File_FlushAndClear",
    "46 0D 10 74 ?? 56 E8 ??",
    match=-0xD,
    ret="void",
    params=[param("int32_t", "clear_buffer"), param("File_Handle*", "stream")],
    doc="Flushes a buffered stream when needed. When clearBuffer is nonzero, also clears the stream buffer pointers and count.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "File_CloseHandle",
    "53 55 56 57 8B 7C 24 14 3B 3D ??",
    hook=0x8,
    ret="int32_t",
    params=[param("int32_t", "file_no")],
    doc="Closes the OS handle associated with a CRT file descriptor and clears its descriptor-table flags. Returns 0 on success or -1 on failure.",
)

stable.fn(
    "Mem_InitializeHeapAllocator",
    "68 40 01 00 00 6A 00 FF 35 ??",
    callable=False,
    public=False,
    ret="int32_t",
    params=[],
    doc="Allocates the custom small-block heap segment table, clears the last-freed segment cache, and initializes segment counters.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "Mem_FindHeapBlockByAddress",
    "A1 ?? ?? ?? ?? 8D 0C 80",
    ret="void*",
    params=[
        param(
            "void*",
            "address",
            doc="Allocation address to locate within the custom heap's 1 MiB segments.",
        )
    ],
    doc="Scans the heap segment table and returns the segment entry whose base contains address, or NULL for addresses outside the custom heap.",
    abi_status=AbiStatus.PLACEHOLDER,
)

stable.fn(
    "Mem_FreeHeapBlock",
    "C2 FC 57 C1 EE 0F 8B ??",
    match=-0x1A,
    hook=0x6,
    ret="void",
    params=[
        param(
            "void*",
            "heap_segment",
            doc="Heap segment entry returned by Mem_FindHeapBlockByAddress.",
        ),
        param("void*", "block", doc="Allocated small-block pointer to release."),
    ],
    doc="Frees a custom small-block allocation, coalesces adjacent free blocks, updates size-class bitmaps, and releases empty pages/segments back to the OS.",
)

stable.fn(
    "String_ConcatCRTString",
    "8B 4C 24 04 57 F7 ?? 03 00 00 00 74 ?? 8A ??",
    ret="char*",
    params=[param("char*", "dest"), param("char const*", "src")],
    doc="Appends src at the NUL terminator in dest, copies the source terminator, and returns dest.",
    stable=True,
)

stable.fn(
    "String_RoundFloatAndCopyDigits",
    "10 40 FF 4D 08 75 ?? ??",
    match=-0x33,
    hook=0x6,
    public=False,
    ret="char*",
    params=[
        param(
            "char*",
            "output",
            doc="Destination digit buffer. Initialized with a leading '0'.",
        ),
        param(
            "int32_t",
            "digit_count",
            doc="Number of digits to copy and round from the CRT float state.",
        ),
        param(
            "CRT_FloatConversionRecord*",
            "conversion_state",
            doc="Conversion record whose mantissa pointer and decimal-point position are consumed and updated.",
        ),
    ],
    doc="Copies and rounds mantissa digits into output, increments decimal_point_position on carry, and returns output.",
    abi_status=AbiStatus.VERIFIED,
)

stable.fn(
    "String_ConvertToDecimalStringFloat",
    "C6 45 E7 CC C6 45 E8 ??",
    match=-0x2D,
    hook=0x6,
    callable=False,
    public=False,
    ret="BOOL",
    params=[
        param(
            "CRT_LongDouble12",
            "value",
            doc="Twelve-byte by-value CRT container holding the x87 extended input.",
        ),
        param("int32_t", "digit_count"),
        param(
            "uint32_t",
            "format_flags",
            doc="Bit 0 makes digit_count relative to the computed decimal-point position.",
        ),
        param(
            "uint8_t*",
            "output",
            doc="Receives int16 point position at +0, sign at +2, length at +3, and NUL-terminated digits or token at +4.",
        ),
    ],
    doc="Returns FALSE only for INF, QNAN, SNAN, or IND output tokens. Every finite value, including zero, returns TRUE.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)


stable.callback_type(
    'CRT_ExitCallback',
    ret='void',
    params=[],
    calling=CallingConvention.CDECL,
    doc='CRT process-exit callback registered by CRT_OnExit and CRT_AtExit.',
    unstable=True,
)

# 2026-08-14 decomp-pass functions (strictly sorted by EN VA).
stable.fn(
    'Actor_ReleaseBindings',
    '8B 44 24 04 56 BE 02 00 00 00',
    public=False,
    ret='void',
    params=[param('PKG_ActorRecord*', 'record')],
    doc="Clears two actor-record binding pointers and decrements each referenced object's refcount when nonzero.",
    unstable=True,
)

stable.fn(
    'Math_EaseInOutQuadraticQ12',
    '55 8B EC 8B 45 08 3D 00 08 00 00',
    hook=0x6,
    ret='int32_t',
    params=[param('int32_t', 'input_q12')],
    doc='Evaluates a normalized quadratic ease-in-out curve using Q12 input and output.',
    stable=True,
)

stable.fn(
    'Bone_InitVertexBufferFromSkin',
    '8B 4C 24 0C 8B 44 24 08',
    hook=0x8,
    public=False,
    ret='void',
    params=[
        param('Actor_State*', 'actor'),
        param('Mesh_RuntimeVertex*', 'dst_vertices'),
        param('Animation_FrameVertex*', 'skin_vertices'),
        param('int32_t', 'vertex_count'),
    ],
    doc="A do/while loop copies x/y/z int16 values from 8-byte Animation_FrameVertex entries into 12-byte Mesh_RuntimeVertex entries and clears referenced normal accumulators for nonnegative normal_group_index. Both callers discard the final advanced EAX pointer.",
    stable=True,
)

stable.fn(
    'Actor_GetAnimSeqIndex',
    '8B 4C 24 04 33 C0 66 8B 41 5C',
    hook=0x6,
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Returns actor's zero-extended current animation sequence index.",
    stable=True,
)

stable.fn(
    'Actor_GetAnimationProgress',
    '8B 54 24 04 33 C0 66 8B 42 5C',
    hook=0x6,
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Returns current animation progress in Q12. Missing or zero-span animation data yields 0x1000.",
    stable=True,
)

stable.fn(
    'Graphics_MarkMeshDirty',
    '8B 44 24 04 56 85 C0 74 ?? F6 00 80',
    public=False,
    ret='void',
    params=[param('Mesh_CmdList*', 'cmd_list')],
    doc="Resets each mesh command's progress/state and marks it dirty for reprocessing.",
    unstable=True,
)

stable.fn(
    'Input_SetDeviceCooperativeLevel',
    '8B 54 24 0C 8B 44 24 04 52',
    hook=0x8,
    public=False,
    ret='BOOL',
    params=[
        param('DInput_IDirectInputDeviceA*', 'device'),
        param('HWND', 'hwnd'),
        param('DWORD', 'flags'),
    ],
    doc="Calls SetCooperativeLevel on IDirectInputDevice(device, hwnd, flags) and normalizes DI_OK to TRUE and every other HRESULT to FALSE. Inputs are not validated.",
    unstable=True,
)

stable.fn(
    'Settings_GetDifficulty',
    '8B 44 24 04 33 C9 85 C0 0F 94 C1 51 A2 ?? ?? ?? ?? E8 ?? ?? ?? ?? 59 C3 90 90 90 90 90 90 90 90',
    match=0x20,
    hook=0x7,
    ret='int32_t',
    params=[],
    doc='Returns current signed difficulty setting used by menu and save-game logic.',
    stable=True,
)

stable.fn(
    'Settings_SetDifficulty',
    '8B 44 24 04 33 C9 85 C0 0F 94 C1 51 A2 ?? ?? ?? ?? E8 ?? ?? ?? ?? 59 C3 90 90 90 90 90 90 90 90',
    match=0x30,
    hook=0x9,
    ret='void',
    params=[param('uint8_t', 'difficulty')],
    doc='Stores the difficulty setting.',
    stable=True,
)

stable.fn(
    'Settings_SetPlayerCharacter',
    '8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 0F BE 05 ?? ?? ?? ?? C3 90 90 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90 8A 44 24 04 A2 ?? ?? ?? ?? C3 90 90 90 90 90 90',
    required=Required.EN,
    hook=0x9,
    ret='void',
    params=[param('uint8_t', 'character_id')],
    doc='Stores the selected player-character ID.',
    stable=True,
)

stable.fn(
    'Level_GetCampaignProgressOrdinal',
    '8B 44 24 04 83 F8 07 7C ??',
    hook=0x7,
    public=False,
    ret='int32_t',
    params=[param('int32_t', 'level_id')],
    doc="Maps campaign level IDs 7-26 to progress ordinals 1-20. Returns -1 for other IDs.",
    stable=True,
)

stable.fn(
    'Color_UpdateGradient',
    '8B 4C 24 04 8A 41 07 84 C0',
    hook=0x7,
    ret='uint32_t',
    params=[param('Menu_ColorGradientState*', 'gradient_state')],
    doc="Advances an eight-byte ping-pong color gradient, honoring endpoint holds, and returns the current 0 through 255 intensity.",
    unstable=True,
)

stable.fn(
    'Material_FindIndex',
    '8B 44 24 04 55 56 57 85 C0',
    public=False,
    ret='int32_t',
    params=[
        param('Material_SectionHeader*', 'section'),
        param('Material_TableEntry*', 'entry'),
        param('int32_t', 'node_type'),
    ],
    doc='Finds a material node matching an entry and type, returning its packed node reference or -1 when absent.',
    unstable=True,
)

stable.fn(
    'Component_HasSpawnCapacity',
    '8B 4C 24 04 8A 41 4B 84 C0',
    hook=0x7,
    ret='BOOL',
    params=[param('Component_Definition*', 'definition')],
    doc='Returns TRUE when the definition has no active-instance limit or its live count remains below the limit.',
    stable=True,
)

stable.fn(
    'Component_GetDamageAmount',
    '8B 44 24 04 8B 88 F4 00 00 00 8B 44 24 08',
    hook=0xA,
    ret='uint16_t',
    params=[
        param('Component_Instance*', 'comp'),
        param('int32_t*', 'out_hit_flag'),
    ],
    doc="Returns the component definition's 16-bit damage amount and writes 1 to outHitFlag when non-NULL.",
    stable=True,
)

stable.fn(
    'Collision_GetCooldownFrames',
    'B8 0F 00 00 00 C3 90 90',
    public=False,
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Returns the fixed 15-frame cooldown. Actor is ignored.",
    unstable=True,
)

stable.fn(
    'Component_GetTeamID',
    '8B 44 24 04 85 C0 75 ?? 83 C8 FF',
    hook=0x6,
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Returns -1 for null actor. Otherwise returns the spawned component actor's zero-extended definition team ID.",
    stable=True,
)

stable.fn(
    'Component_GetOwnerActor',
    '8B 44 24 04 8B 88 F4 00 00 00 8B 01',
    hook=0xA,
    ret='Actor_State*',
    params=[param('Actor_State*', 'actor')],
    doc="Returns the owner from a spawned actor's Component_SpawnParams. Actor and record_ptr must be valid.",
    stable=True,
)

stable.fn(
    'Actor_HandleNullDamage',
    '33 C0 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 44 24 04 8B 88 F4 00 00 00 8B 41 2C C3',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Three-byte null stub. Actor is unused.",
    unstable=True,
)

stable.fn(
    'Component_GetSpawnFrame',
    '8B 44 24 04 8B 88 F4 00 00 00 8B 41 2C',
    hook=0xA,
    ret='int32_t',
    params=[param('Actor_State*', 'actor')],
    doc="Returns spawn_frame from the spawned actor's Component_SpawnParams.",
    stable=True,
)

stable.fn(
    'Component_CheckHeightConstraint',
    '8B 44 24 08 8B 40 04 85 C0',
    hook=0x7,
    public=False,
    ret='BOOL',
    params=[
        param('Actor_State*', 'actor'),
        param('Component_Definition*', 'definition'),
    ],
    doc="Tests actor vertical separation against definition +0x04. The threshold field label remains uncertain.",
    unstable=True,
)

stable.fn(
    'Component_GetNextInChain',
    '8B 44 24 04 85 C0 75 ?? C3 8B 40 08',
    hook=0x6,
    public=False,
    ret='Component_Instance*',
    params=[param('Component_Instance*', 'comp')],
    doc='Returns the next component in the intrusive chain, or null when the input is null.',
    unstable=True,
)

stable.fn(
    'Component_GetFlags',
    '8B 44 24 04 85 C0 75 ?? C3 66 8B 40 20',
    hook=0x6,
    public=False,
    ret='uint16_t',
    params=[param('Component_Instance*', 'comp')],
    doc="Returns the component's 16-bit raw flags, or zero when the input is null.",
    unstable=True,
)

stable.fn(
    'Component_GetPreviousInChain',
    '8B 44 24 04 85 C0 75 ?? C3 8B 40 0C',
    hook=0x6,
    public=False,
    ret='Component_Instance*',
    params=[param('Component_Instance*', 'comp')],
    doc='Returns the previous component in the intrusive chain, or null when the input is null.',
    unstable=True,
)

stable.fn(
    'Component_GetLifetime',
    '8B 4C 24 04 85 C9 75 ?? 33 C0',
    hook=0x6,
    ret='int32_t',
    params=[param('Component_Instance*', 'comp')],
    doc="Returns zero for a null component. Otherwise returns its zero-extended 16-bit lifetime value.",
    stable=True,
)

stable.fn(
    'ComponentBinding_ResolveActorByKey',
    '8B 44 24 0C 56 85 C0 74 ?? 8B 4C 24 08 33 D2 66 8B 51 04 3B D0 74 ??',
    hook=0x5,
    public=False,
    ret='int32_t',
    params=[
        param('void*', 'bindings'),
        param('int32_t', 'binding_count'),
        param('int32_t', 'binding_key'),
        param('Actor_State**', 'out_actor'),
    ],
    doc='Scans 0x24-byte component-binding slots for binding_key and writes the live matching actor to out_actor.',
    unstable=True,
)

stable.fn(
    'Collision_CheckEnemyCanBeHit',
    '8B 44 24 08 80 78 64 04 75 ?? 80 78 67 0B 77 ?? 83 C8 FF C3 33 C0 C3',
    hook=0x8,
    public=False,
    ret='int32_t',
    params=[
        param('Component_Instance*', 'comp'),
        param('Actor_State*', 'other_actor'),
    ],
    doc="Returns -1 only for eligible type-4 other_actor targets. Comp is an unused native ABI parameter.",
    unstable=True,
)

stable.fn(
    'Collision_CheckLineSphereIntersection',
    '55 8B EC 83 EC 10 8B 45 08 8B 4D 14 53 56 0F BF 90 AA 00 00 00 03 CA 8B 75 0C 0F AF C9',
    hook=0x6,
    public=False,
    ret='uint8_t',
    params=[
        param('Actor_State*', 'actor'),
        param('Math_Vec3I32*', 'sphere_center'),
        param('Math_Vec3I32*', 'segment_delta'),
        param('uint32_t', 'sphere_radius'),
    ],
    doc='Tests actor position against a swept sphere and returns the byte-sized intersection result.',
    unstable=True,
)

stable.fn(
    'Collision_CalculateImpactVelocity',
    '55 8B EC 83 EC 10 53 8B 5D 18',
    hook=0x6,
    public=False,
    ret='void',
    params=[
        param('Actor_State*', 'actor'),
        param('Actor_State*', 'other_actor'),
        param('int32_t', 'collision_push'),
        param('Math_Vec3I32*', 'velocity'),
        param('Math_Vec3I32*', 'impact_velocity'),
    ],
    doc="Partitions impact velocity via the class-specific +0xC8 collision-shape overlay. +0x6E is a mode-dependent signed Q6 scalar.",
    unstable=True,
)

stable.fn(
    'Actor_UpdateWorldRenderPosition',
    '55 8B EC 83 EC 08 8B 45 08',
    hook=0x6,
    public=False,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc="Updates world_render_pos through a class-specific, allocation-relative Actor +0xC8 pointer to Scene_Node-compatible local_pos. Other layouts use +0xCA as animation frame count.",
    unstable=True,
)

stable.fn(
    'Collision_ResolveActorToActorCollision',
    '55 8B EC 81 EC A4 00 00 00 8B 45 0C 8B 55 08 66 8B 88 CE 00 00 00 66 3B 8A CE 00 00 00',
    hook=0x9,
    public=False,
    ret='void',
    params=[
        param('Actor_State*', 'actor_a'),
        param('Actor_State*', 'actor_b'),
        param('int32_t', 'actor_a_response'),
        param('int32_t', 'actor_b_response'),
    ],
    doc=(
        'Resolves class-specific Actor +0xC8 shape overlays, then passes actor_b as a node-compatible '
        'collision source sharing transform, position, sub_pos, and type offsets.'
    ),
    unstable=True,
)

stable.fn(
    'Actor_ApplyDamageScaling',
    '55 8B EC 8B 45 08 56 8B 75 0C',
    hook=0x6,
    public=False,
    ret='void',
    params=[
        param('Actor_State*', 'actor'),
        param('PKG_ActorRecord*', 'record'),
        param('int32_t', 'damage'),
    ],
    doc='Initialize actor-record damage response constants, then Q12-scale response motion by damage and actor scale.',
    unstable=True,
)

stable.fn(
    'Audio_ReleaseSoundHandle',
    '8B 44 24 04 56 8D 34 80',
    public=False,
    ret='void',
    params=[param('int32_t', 'slot_index')],
    doc='Stop and end an active Miles sample slot, clearing its cached base playback rate.',
    unstable=True,
)

stable.fn(
    'Actor_ResetAnimationChains',
    '8B 44 24 04 33 C9 33 D2 8A 88 BA 00 00 00',
    hook=0x6,
    public=False,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc='Reset byte flags and 16-bit cursors for every 0x20-byte actor trail-chain record.',
    unstable=True,
)

stable.fn(
    'Actor_ResetComponentAnimations',
    '8B 44 24 04 33 C9 33 D2 8A 88 BB 00 00 00',
    hook=0x6,
    public=False,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc='Zero two 16-bit animation fields in every 0x28-byte actor component record.',
    unstable=True,
)

stable.fn(
    'Actor_DetachFromParent',
    '8B 4C 24 04 33 D2 8B 81 08 01 00 00',
    hook=0x6,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc="Detaches actor, clears runtime attachment_transform_node, and releases a type-3 parent's attachment reference.",
    stable=True,
)

stable.fn(
    'Math_ClampAngleDelta',
    '8B 44 24 08 56 66 8B 74 24 10',
    ret='uint16_t',
    params=[
        param('int16_t', 'target_angle'),
        param('int16_t', 'current_angle'),
        param('int16_t', 'max_delta'),
    ],
    doc='Move a 12-bit circular angle toward its target by half the allowed delta, honoring the 0x1000 sentinel.',
    stable=True,
)

stable.fn(
    'Material_MarkSharedTextureReference',
    '8B 44 24 04 8B 54 24 0C',
    hook=0x8,
    public=False,
    ret='void',
    params=[
        param('Material_SectionLoadView*', 'section'),
        param('Material_TableEntryRaw*', 'entry'),
        param('int32_t', 'entry_index'),
    ],
    doc='Marks a material entry when an earlier entry with the same texture descriptor already carries the shared-reference flag.',
    stable=True,
)

stable.fn(
    'Math_SinFP12',
    '8B 4C 24 04 81 E1 FF 0F 00 00',
    hook=0xA,
    ret='int32_t',
    params=[param('int32_t', 'angle')],
    doc="Returns Q12 sine for a 12-bit turn angle using the engine's mirrored lookup table.",
    stable=True,
)

stable.fn(
    'Math_CalculateAtan2Fast',
    '8B 54 24 04 56 8B 74 24 0C 8B C2',
    ret='int32_t',
    params=[
        param('int32_t', 'y'),
        param('int32_t', 'x'),
    ],
    doc='Approximates atan2(y, x) as a 12-bit turn angle using downscaled inputs and reciprocal/angle lookup tables.',
    stable=True,
)

stable.fn(
    'Math_CalculateAtan2FP12',
    '53 8B 5C 24 0C 56 57 8B 7C 24 10',
    ret='int32_t',
    params=[
        param('int32_t', 'y'),
        param('int32_t', 'x'),
    ],
    doc='Computes atan2(y, x) as a 12-bit turn angle using integer division and an angle lookup table.',
    stable=True,
)

stable.fn(
    'Math_CalculateAtan2Lookup12',
    '8B 44 24 08 56 57 8B 7C 24 0C',
    public=False,
    ret='int32_t',
    params=[
        param('int32_t', 'sin_value'),
        param('int32_t', 'cos_value'),
    ],
    doc="Computes the engine's clamped signed atan-ratio angle. This is not an unrestricted atan2 implementation.",
    stable=True,
)

stable.fn(
    'UI_CompareSpriteDepth',
    '8B 44 24 04 33 D2 8B 08',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[
        param('PKG_SpriteEntry*const*', 'lhs'),
        param('PKG_SpriteEntry*const*', 'rhs'),
    ],
    doc='Compares two sprite pointers by signed 16-bit sort_key for ascending depth ordering.',
    stable=True,
)

stable.fn(
    'Math_EaseInOut',
    '55 8B EC 51 8B 4D 10 56',
    hook=0x7,
    ret='int32_t',
    params=[
        param('int32_t', 't_q12'),
        param('int32_t', 'ease_in_q12'),
        param('int32_t', 'ease_out_q12'),
    ],
    doc='Maps Q12 time through quadratic ease-in, linear travel, and quadratic ease-out segments.',
    stable=True,
)

stable.fn(
    'Actor_SetMovingRight',
    '8B 44 24 04 8B 88 88 00 00 00 80 CD 08',
    hook=0xA,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc='Sets actor rightward movement/orientation flags and clears the opposing lifecycle direction bit.',
    stable=True,
)

stable.fn(
    'Actor_SetMovingLeft',
    '8B 44 24 04 8B 88 88 00 00 00 81 E1 F7 EF FF FF',
    hook=0xA,
    ret='void',
    params=[param('Actor_State*', 'actor')],
    doc='Sets actor leftward movement/orientation flags while clearing conflicting movement flags.',
    stable=True,
)

stable.fn(
    'Input_SetKeyDown',
    '8B 44 24 04 25 FF 00 00 00 C6 80 ?? ?? ?? ?? 01',
    hook=0x9,
    public=False,
    ret='void',
    params=[param('uint8_t', 'scan_code')],
    doc='Marks one raw 8-bit scan-code slot as pressed in the engine input-state buffer.',
    stable=True,
)

stable.fn(
    'CRT_InitializeFloatingPoint',
    'E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? A3 ?? ?? ?? ?? E8 ?? ?? ?? ?? DB E2 C3',
    public=False,
    ret='void',
    params=[],
    doc='Initializes CRT floating-format dispatch and records whether the CPU has the Pentium FDIV precision erratum.',
    unstable=True,
)

stable.fn(
    'CRT_InitFloatFormatDispatch',
    'B8 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? A3 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ??',
    public=False,
    ret='void',
    params=[],
    doc='Installs the CRT function-pointer dispatch table used by printf-style floating conversion.',
    unstable=True,
)

stable.fn(
    'String_FormatCRTString',
    '55 8B EC 83 EC 20 8B 45 08 56 89 45 E8',
    hook=0x6,
    required=Required.EN,
    public=False,
    callable=False,
    ret='int32_t',
    params=[
        param('char*', 'buffer'),
        param('char const*', 'format'),
    ],
    doc='Formats into a caller buffer and terminates it. Variadic arguments are not modeled, so this row is not callable.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_Setjmp3',
    '8B 54 24 04 89 2A 89 5A 04 89 7A 08 89 72 0C 89 62 10 8B 04 24 89 42 14 C7 42 20 30 32 43 56',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint32_t*', 'jump_buffer'),
        param('int32_t', 'argument_count'),
    ],
    doc="Captures MSVC register, stack, PC, and SEH state. Optional words selected by argument_count are not modeled.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_Longjmp',
    '8B 5C 24 04 8B 2B 8B 73 18',
    hook=0x6,
    required=Required.EN,
    public=False,
    callable=False,
    ret='void',
    params=[
        param('int32_t*', 'jump_buffer'),
        param('int32_t', 'value'),
    ],
    doc="Restores a private MSVC jump buffer, unwinds SEH state, and jumps to the saved PC. It never returns to its caller.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'File_FlushCRTFile',
    '56 8B 74 24 08 85 F6 75 ??',
    ret='int32_t',
    params=[param('File_Handle*', 'stream')],
    doc="NULL stream dispatches to flush-all mode. Otherwise flushes buffered bytes and optionally calls FlushFileBuffers for commit-enabled streams.",
    stable=True,
)

stable.fn(
    'CRT_FlushAll',
    '6A 01 E8 ?? ?? ?? ?? 59 C3 53 56 57',
    public=False,
    hook=0x7,
    ret='int32_t',
    params=[],
    doc='Returns File_FlushAllCRT(1).',
    stable=True,
)

stable.fn(
    'File_FlushAllCRT',
    '53 56 57 33 F6 33 DB 33 FF',
    public=False,
    ret='int32_t',
    params=[param('int32_t', 'flush_mode')],
    doc="Scans CRT FILE table. Mode 1 flushes all active streams and counts successes, mode 0 flushes writable streams and accumulates failure.",
    unstable=True,
)

stable.fn(
    'File_WriteCRTFileFormat',
    '55 8B EC 56 57 FF 75 08',
    required=Required.EN,
    public=False,
    callable=False,
    ret='int32_t',
    params=[
        param('File_Handle*', 'stream'),
        param('char const*', 'format'),
    ],
    doc='Formats output to a CRT stream. Variadic arguments are not modeled, so this row is not callable.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'String_PrintFormatCRT',
    '53 56 BE ?? ?? ?? ?? 57 56 E8 ?? ?? ?? ?? 8B F8',
    hook=0x7,
    required=Required.EN,
    public=False,
    callable=False,
    ret='int32_t',
    params=[param('char const*', 'format')],
    doc='Formats output to the CRT stdout stream. Variadic arguments are not modeled, so this row is not callable.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_OnExit',
    '56 FF 35 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 59 8B 0D ?? ?? ?? ??',
    hook=0x7,
    public=False,
    ret='CRT_ExitCallback',
    params=[param('CRT_ExitCallback', 'callback')],
    doc='Grows heap-backed exit-callback array in 16-byte increments, appends callback, advances end pointer, returns callback or NULL on allocation.',
    unstable=True,
)

stable.fn(
    'CRT_AtExit',
    'FF 74 24 04 E8 ?? ?? ?? ?? F7 D8 1B C0',
    hook=0x9,
    public=False,
    ret='int32_t',
    params=[param('CRT_ExitCallback', 'callback')],
    doc='Registers callback through CRT_OnExit and maps non-NULL result to 0, NULL to -1.',
    unstable=True,
)

stable.fn(
    'CRT_AtExitInit',
    '68 80 00 00 00 E8 ?? ?? ?? ?? 85 C0',
    public=False,
    ret='void',
    params=[],
    doc='Allocates initial 0x80-byte callback table, aborts with runtime error 0x18 on failure, zeroes first slot, and initializes base/next globals.',
    unstable=True,
)

stable.fn(
    'CRT_NhMalloc',
    '83 7C 24 04 E0 77 ?? FF 74 24 04',
    public=False,
    ret='void*',
    params=[
        param('uint32_t', 'size'),
        param('int32_t', 'new_mode'),
    ],
    doc='Rejects sizes above 0xFFFFFFE0, retries Mem_HeapAllocCRT while new_mode is enabled and CRT_CallNewHandler(size) succeeds, otherwise returns NULL.',
    unstable=True,
)

stable.fn(
    'CRT_Strstr',
    '8B 4C 24 08 57 53 56 8A 11',
    public=False,
    ret='char*',
    params=[
        param('char const*', 'haystack'),
        param('char const*', 'needle'),
    ],
    doc="Finds the first needle occurrence in read-only haystack. The returned pointer aliases haystack.",
    stable=True,
)

stable.fn(
    'CRT_CInit',
    'A1 ?? ?? ?? ?? 85 C0 74 ?? FF D0 68 ?? ?? ?? ?? 68 ?? ?? ?? ??',
    public=False,
    ret='void',
    params=[],
    doc="Calls optional CRT_InitializeFloatingPoint, then walks.CRT initializer ranges 0x44E008 through 0x44E018 and 0x44E000 through 0x44E004 via CRT_InitTerm.",
    unstable=True,
)

stable.fn(
    'exit',
    '6A 00 6A 00 FF 74 24 0C',
    hook=0x8,
    public=False,
    ret='void',
    params=[param('int32_t', 'status')],
    doc="Runs normal CRT exit processing, then terminates through ExitProcess. It never returns.",
    stable=True,
)

stable.fn(
    'CRT_QuickExit',
    '6A 00 6A 01 FF 74 24 0C',
    hook=0x8,
    public=False,
    ret='void',
    params=[param('int32_t', 'status')],
    doc="Skips normal atexit processing and terminates through ExitProcess. It never returns.",
    stable=True,
)

stable.fn(
    'CRT_DoExit',
    '57 6A 01 5F 39 3D ?? ?? ?? ?? 75 ?? FF 74 24 08',
    hook=0xA,
    public=False,
    ret='void',
    params=[
        param('int32_t', 'status'),
        param('int32_t', 'quick_exit'),
        param('int32_t', 'return_to_caller'),
    ],
    doc='Coordinates CRT termination, runs callbacks once, flushes streams as requested, and exits or returns according to its flags.',
    unstable=True,
)

stable.fn(
    'CRT_InitTerm',
    '56 8B 74 24 08 3B 74 24 0C',
    public=False,
    ret='void',
    params=[
        param('CRT_ExitCallback*', 'first'),
        param('CRT_ExitCallback*', 'last'),
    ],
    doc="Invokes each nonnull function pointer from first up to but excluding last.",
    unstable=True,
)

stable.fn(
    'File_FlushAndCloseOnExitCRT',
    'E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? 00 74 ?? E9 ?? ?? ?? ?? C3',
    public=False,
    ret='int32_t',
    params=[],
    doc="Calls CRT_FlushAll. When g_crtExitProcessing is nonzero, tail-calls File_CloseAllOpenFilesCRT.",
    unstable=True,
)

stable.fn(
    'String_FormatCRTVString',
    '55 8B EC 83 EC 20 8B 45 08 56 FF 75 10',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[
        param('char*', 'buffer'),
        param('char const*', 'format'),
        param('uint8_t*', 'argument_cursor'),
    ],
    doc='Forwards buffer, format, and the x86 variadic byte cursor to the formatting core, then writes a trailing NUL.',
    unstable=True,
)

stable.fn(
    'CRT_AmsgExit',
    '83 3D ?? ?? ?? ?? 01 75 ?? E8 ?? ?? ?? ?? FF 74 24 04 E8 ?? ?? ?? ?? 68 FF 00 00 00',
    hook=0x7,
    callable=False,
    public=False,
    ret='void',
    params=[param('int32_t', 'runtime_error')],
    doc="Displays the selected runtime error, then invokes the immutable quick-exit callback with status 255. It never returns.",
    unstable=True,
)

stable.fn(
    'CRT_ExitWithRuntimeError',
    '83 3D ?? ?? ?? ?? 01 75 ?? E8 ?? ?? ?? ?? FF 74 24 04 E8 ?? ?? ?? ?? 59',
    hook=0x7,
    callable=False,
    public=False,
    ret='void',
    params=[param('int32_t', 'runtime_error')],
    doc="Displays the selected CRT runtime error, then calls ExitProcess with status 255. It never returns.",
    unstable=True,
)

stable.fn(
    'CRT_SetDefaultPrecision',
    '68 00 00 03 00 68 00 00 01 00',
    public=False,
    ret='void',
    params=[],
    doc="Sets the x87 precision-control field through CRT_Control87(0x10000, 0x30000). No result is consumed.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'String_FindDecimalPointFloat',
    '8B 44 24 04 8A 15 ?? ?? ?? ?? 8A 08',
    hook=0xA,
    public=False,
    ret='char*',
    params=[param('char*', 'formatted_string')],
    doc='Removes redundant fractional zeroes in place and returns the resulting decimal boundary.',
    unstable=True,
)

stable.fn(
    'Float_IsNonnegative',
    '8B 44 24 04 DD 00 DC 1D ?? ?? ?? ??',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[param('double const*', 'value')],
    doc="Returns true for an ordered value greater than or equal to zero. Negative values and NaNs return false.",
    unstable=True,
)

stable.fn(
    'CRT_CfltCvt',
    '55 8B EC 83 7D 10 65 74 ??',
    hook=0x7,
    public=False,
    ret='char*',
    params=[
        param('double*', 'value'),
        param('char*', 'buffer'),
        param('int32_t', 'format'),
        param('int32_t', 'precision'),
        param('int32_t', 'flags'),
    ],
    doc='Dispatches floating conversion by format character among exponential/fixed/general paths and writes caller buffer.',
    unstable=True,
)

stable.fn(
    'String_FormatCRTVStringAlt',
    '55 8B EC 81 EC 48 02 00 00',
    hook=0x9,
    required=Required.EN,
    public=False,
    callable=False,
    ret='int32_t',
    params=[
        param('int32_t*', 'output_context'),
        param('char const*', 'format'),
        param('uint8_t*', 'argument_cursor'),
    ],
    doc="Formats values from an x86 variadic byte cursor into a FILE-like output context. Internal and non-callable.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_VaArgInt',
    '8B 44 24 04 83 00 04 8B 00 8B 40 FC',
    hook=0x7,
    public=False,
    ret='int32_t',
    params=[param('uint8_t**', 'argument_cursor')],
    doc='Reads one 32-bit promoted slot and advances the mutable byte cursor by four bytes.',
    unstable=True,
)

stable.fn(
    'CRT_VaArgInt64',
    '8B 44 24 04 83 00 08 8B 08',
    hook=0x7,
    public=False,
    ret='int64_t',
    params=[param('uint8_t**', 'argument_cursor')],
    doc="Reads eight bytes, advances the mutable byte cursor by eight, and returns the value in EDX.EAX.",
    unstable=True,
)

stable.fn(
    'CRT_GlobalUnwind2',
    '55 8B EC 53 56 57 55 6A 00',
    required=Required.EN,
    public=False,
    callable=False,
    ret='void',
    params=[param('void*', 'target_frame')],
    doc='Runs the MSVC global SEH unwind path. Its compiler frame ABI is not callable through the SDK.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_UnwindHandler',
    '8B 4C 24 04 F7 41 04 06 00 00 00 B8 01 00 00 00 74 0F 8B 44 24 08 8B 54 24 10 89 02 B8 03 00 00 00 C3',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('void*', 'exception_record'),
        param('void*', 'establisher_frame'),
        param('void*', 'context_record'),
        param('void**', 'dispatcher_context'),
    ],
    doc='Returns ContinueSearch normally or records the establisher frame and returns CollidedUnwind during unwind.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_LocalUnwind2',
    '53 56 57 8B 44 24 10 50',
    hook=0x7,
    required=Required.EN,
    public=False,
    callable=False,
    ret='void',
    params=[
        param('void*', 'registration'),
        param('int32_t', 'stop_try_level'),
    ],
    doc='Runs local MSVC SEH termination handlers. Its compiler frame ABI is not callable through the SDK.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_AbnormalTermination',
    '33 C0 64 8B 0D 00 00 00 00',
    hook=0x9,
    required=Required.EN,
    public=False,
    callable=False,
    ret='int32_t',
    params=[],
    doc="Checks the current MSVC SEH registration for abnormal termination. Not callable through the SDK.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'Mem_GetCRTSize',
    '56 8B 74 24 08 56 E8 ?? ?? ?? ?? 85 C0',
    ret='uint32_t',
    params=[param('void*', 'ptr')],
    doc="Custom-segment allocation size comes from block header minus overhead. External heap allocation uses HeapSize.",
    stable=True,
)

stable.fn(
    'File_AllocateFileHandleCRT',
    '8B 15 ?? ?? ?? ?? 53 55 56 33 ED 33 F6',
    hook=0x6,
    public=False,
    ret='File_Handle*',
    params=[],
    doc='Scans FILE table for empty/inactive slot, allocates 0x20-byte FILE object for absent slot, then clears fields and sets descriptor -1.',
    unstable=True,
)

stable.fn(
    'CRT_CallNewHandler',
    'A1 ?? ?? ?? ?? 85 C0 74 ?? FF 74 24 04',
    public=False,
    ret='int32_t',
    params=[param('uint32_t', 'size')],
    doc="Invokes configured new-handler with requested size and returns boolean success. Absent handler returns 0.",
    stable=True,
)

stable.fn(
    'Mem_InitializeHeapCRT',
    '33 C0 6A 00 39 44 24 08',
    hook=0x8,
    public=False,
    ret='int32_t',
    params=[param('int32_t', 'mt_flag')],
    doc='Creates private Win32 heap with growable mode based on mtFlag, initializes custom allocator metadata, and destroys heap on metadata failure.',
    unstable=True,
)

stable.fn(
    'String_StrchrCRT',
    '53 8B D8 C1 E0 08 8B 54 24 08 F7 C2 03 00 00 00 74 13 8A 0A 42 38 D9 74 D1 84 C9 74 51',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='char*',
    params=[param('char const*', 'string')],
    doc="Optimized strchr entry. String is stack-passed, search character arrives in EAX, and return tail is shared.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'Mem_MemmoveBackwardACRT',
    '83 25 ?? ?? ?? ?? 00 C7 05 ?? ?? ?? ?? 09 00 00 00 83 C8 FF 5F 5E 5B C9 C3 55 8B EC 57 56 8B 75 0C 8B',
    match=0x19,
    public=False,
    ret='char*',
    params=[
        param('char*', 'dest'),
        param('char const*', 'src'),
        param('uint32_t', 'count'),
    ],
    doc="Overlap-aware byte move selects forward or backward copy and uses aligned dword/jump-table fast paths. Returns original destination.",
    unstable=True,
)

stable.fn(
    'Mem_CallocCRT',
    '53 56 8B 74 24 0C 57 0F AF 74 24 14',
    hook=0x6,
    ret='void*',
    params=[
        param('uint32_t', 'count'),
        param('uint32_t', 'elem_size'),
    ],
    doc='Checks count*elemSize overflow, allocates total bytes through Mem_MallocCRT, and zero-fills successful allocation.',
    stable=True,
)

stable.fn(
    'File_CloseAllOpenFilesCRT',
    '56 57 6A 03 33 FF 5E 39 35 ?? ?? ?? ??',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[],
    doc="Scans CRT FILE slots 3 through g_maxFileHandleCount, closes active streams, frees dynamically allocated slots >=20, clears table entries, and returns.",
    unstable=True,
)

stable.fn(
    'CRT_XcptFilter',
    '55 8B EC 53 FF 75 08 E8 ?? ?? ?? ??',
    hook=0x7,
    public=False,
    ret='int32_t',
    params=[
        param('uint32_t', 'xcptnum'),
        param('void*', 'pxcptinfoptrs'),
    ],
    doc='Maps an OS exception code through the CRT table, invokes its handler, and returns the CRT exception-filter action.',
    unstable=True,
)

stable.fn(
    'File_FindFileHandleEntryCRT',
    '8B 54 24 04 8B 0D ?? ?? ?? ?? 39 15 ?? ?? ?? ??',
    hook=0xA,
    public=False,
    ret='int32_t*',
    params=[param('int32_t', 'file_handle')],
    doc='Resolves a CRT descriptor into its 12-byte table entry after range and table-presence checks.',
    unstable=True,
)

stable.fn(
    'CRT_SkipProgramNameInCmdLine',
    '83 3D ?? ?? ?? ?? 00 75 ?? E8 ?? ?? ?? ?? 56 8B 35 ?? ?? ?? ??',
    hook=0x7,
    public=False,
    ret='char*',
    params=[],
    doc='Walks g_commandLine past quoted or unquoted executable token, handles multibyte lead bytes, then skips whitespace.',
    unstable=True,
)

stable.fn(
    'CRT_SetEnvp',
    '53 33 DB 39 1D ?? ?? ?? ?? 56 57 75 ?? E8 ?? ?? ?? ?? 8B 35 ?? ?? ?? ??',
    hook=0x9,
    public=False,
    ret='int32_t',
    params=[],
    doc="Counts non-'=' strings in copied environment block, allocates g_environ, duplicates each visible entry, frees source block, and terminates.",
    unstable=True,
)

stable.fn(
    'CRT_SetArgv',
    '55 8B EC 51 51 53 33 DB',
    public=False,
    ret='int32_t',
    params=[],
    doc='Parses the process command line, allocates argv storage, populates __argc/__argv, and returns 0 or -1 on allocation failure.',
    unstable=True,
)

stable.fn(
    'CRT_ParseCmdLine',
    '55 8B EC 8B 4D 18 8B 45 14',
    hook=0x6,
    public=False,
    ret='void',
    params=[
        param('char*', 'command_line'),
        param('char**', 'argv'),
        param('char*', 'arg_buffer'),
        param('int32_t*', 'argc_out'),
        param('int32_t*', 'char_count_out'),
    ],
    doc='Counts or populates argv and its character buffer from a Windows command line, honoring quotes and escaped quotes.',
    unstable=True,
)

stable.fn(
    'CRT_GetEnvironmentStringsA',
    '51 51 A1 ?? ?? ?? ?? 53 55 8B 2D ?? ?? ?? ?? 56',
    hook=0x7,
    public=False,
    ret='char*',
    params=[],
    doc='Returns a heap-owned ANSI environment block, converting from Unicode when only the wide environment API is available.',
    unstable=True,
)

stable.fn(
    'CRT_SEHLongjmpUnwind',
    '55 8B 4C 24 08 8B 29 8B 41 1C',
    cc=CallingConvention.STDCALL,
    required=Required.EN,
    public=False,
    callable=False,
    ret='void',
    params=[param('int32_t*', 'jump_buffer')],
    doc='Unwinds MSVC SEH state from a private jump buffer. Its compiler frame ABI is not callable through the SDK.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_ControlFP',
    '55 8B EC 51 56 9B D9 7D FC',
    public=False,
    ret='uint32_t',
    params=[
        param('uint32_t', 'new_control'),
        param('uint32_t', 'mask'),
    ],
    doc='Translates abstract control-word masks, updates x87 control state, and returns abstracted prior/current flags.',
    stable=True,
)

stable.fn(
    'CRT_Control87',
    '8B 44 24 08 25 FF FF F7 FF',
    public=False,
    hook=0x9,
    ret='uint32_t',
    params=[
        param('uint32_t', 'new_value'),
        param('uint32_t', 'mask'),
    ],
    doc='Converts abstract mask/control values to x87 form, applies FLDCW, then converts resulting control word back.',
    stable=True,
)

stable.fn(
    'CRT_AbstractCWToCW',
    '53 8B 5C 24 08 33 C0 55',
    public=False,
    ret='uint32_t',
    params=[param('uint32_t', 'abstract_control_word')],
    doc='Maps CRT abstract floating-control flags to hardware x87 control-word bits.',
    unstable=True,
)

stable.fn(
    'CRT_CWToAbstractCW',
    '53 8B 5C 24 08 33 C0 56',
    public=False,
    ret='uint32_t',
    params=[param('uint32_t', 'control_word')],
    doc='Maps x87 control-word bits to CRT abstract floating-control flags.',
    unstable=True,
)

stable.fn(
    "CRT_FAssign",
    "55 8B EC 51 51 83 7D 08 00 FF 75 10 74 1B 8D 45 F8 50 E8 ?? ?? ?? ??",
    cc=CallingConvention.CDECL,
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[
        param("int32_t", "flag"),
        param("void*", "destination"),
        param("char const*", "number"),
    ],
    doc="Dispatches a decimal string to the CRT double or float converter selected by flag.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "CRT_AllMul",
    "8B 44 24 08 8B 4C 24 10 0B C8 8B 4C 24 0C 75 ?? 8B 44 24 04 F7 E1 C2 10 00",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int64_t",
    params=[param("int64_t", "lhs"), param("int64_t", "rhs")],
    doc="Returns the low 64 bits of lhs * rhs in EDX.EAX and pops both stack operands.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "File_WriteRepeatedCharCRT",
    "56 57 8B 7C 24 10 8B C7 4F 85 C0 7E ?? 8B 74 24 18 56 FF 74 24 18",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[
        param("int32_t", "ch"),
        param("int32_t", "count"),
        param("File_Handle*", "stream"),
        param("int32_t*", "written_count"),
    ],
    doc="Emits ch count times through the CRT formatter stream, stopping after an output failure.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "String_WriteStringToOutputCRT",
    "53 8B 5C 24 0C 8B C3 4B 56 57 85 C0 7E ?? 8B 7C 24 1C 8B 74 24 10",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[
        param("char const*", "source"),
        param("int32_t", "count"),
        param("File_Handle*", "stream"),
        param("int32_t*", "written_count"),
    ],
    doc="Emits count bytes from source through the CRT formatter stream, stopping after an output failure.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_VaArgShort",
    "8B 44 24 04 83 00 04 8B 00 66 8B 40 FC C3",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int16_t",
    params=[param("uint8_t**", "argument_cursor")],
    doc="Consumes one four-byte promoted slot and returns its low signed 16 bits.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_ExceptHandler3",
    "55 8B EC 83 EC 08 53 56 57 55 FC 8B 5D 0C 8B 45 08 F7 40 04 06 00 00 00",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int32_t",
    params=[
        param("void*", "exception_record"),
        param("void*", "registration"),
        param("void*", "context"),
        param("void*", "dispatcher_context"),
    ],
    doc="Implements the MSVC scope-table filter, unwind, and handler-transfer protocol for an SEH registration.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_WriteRuntimeErrorBanner",
    "A1 ?? ?? ?? ?? 83 F8 01 74 0D 85 C0 75 2A 83 3D ?? ?? ?? ?? 01 75 21 68 FC 00 00 00",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[],
    doc="Writes the CRT runtime-error banner around the optional runtime callback.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "CRT_NmsgWrite",
    "55 8B EC 81 EC A4 01 00 00 8B 55 08 33 C9 B8 ?? ?? ?? ?? 3B 10 74 0B 83 C0 08 41 3D ?? ?? ?? ??",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[param("int32_t", "runtime_error_id")],
    doc="Looks up a CRT runtime-error message and writes it to stderr or a message box.",
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    "CRT_ToLower",
    '55 8B EC 51 83 3D ?? ?? ?? ?? 00 53',
    public=False,
    hook=0xB,
    ret='int32_t',
    params=[param('int32_t', 'ch')],
    doc='Maps ASCII A-Z to lowercase directly and otherwise uses the active locale mapping.',
    abi_status=AbiStatus.VERIFIED,
    stable=True,
)

stable.fn(
    "CRT_LD12IsZeroUpper",
    "8B 44 24 08 56 6A 20 99 59 F7 F9 6A 1F 8B F0",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int32_t",
    params=[
        param("uint32_t const*", "value"),
        param("uint32_t", "start_bit"),
    ],
    doc="Returns whether the ld12 mantissa tail beginning at start_bit is zero.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12Increment',
    '8B 44 24 08 53 56 57 6A 20',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint32_t*', 'value'),
        param('uint32_t', 'bit_index'),
    ],
    doc="Sets one ld12 mantissa bit and propagates carry through preceding dwords. Returns carry status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12RoundAndClear',
    '55 8B EC 51 51 8B 45 0C',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint32_t*', 'value'),
        param('uint32_t', 'bit_index'),
    ],
    doc='Rounds the ld12 mantissa at bit_index, propagates carry, and clears discarded low bits.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12Copy',
    '8B 44 24 08 8B 4C 24 04 56',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='void',
    params=[
        param('uint32_t*', 'dest'),
        param('uint32_t const*', 'src'),
    ],
    doc='Copies the three dwords of an internal ld12 value from src to dest.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12Clear',
    '57 8B 7C 24 08 33 C0 AB',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='void',
    params=[param('uint32_t*', 'value')],
    doc='Clears all three dwords of an internal ld12 value.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12IsZero',
    '8B 44 24 04 33 C9 83 38 00',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[param('uint32_t const*', 'value')],
    doc='Returns 1 when all three dwords of an internal ld12 value are zero.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12ShiftRight',
    '55 8B EC 83 EC 0C 8B 45 0C 53 56 57 6A 20',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='void',
    params=[
        param('uint32_t*', 'value'),
        param('uint32_t', 'bit_count'),
    ],
    doc='Shifts an ld12 mantissa right in place by bit_count with cross-dword carry.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_LD12ToIEEE",
    "55 8B EC 83 EC 18 8B 45 08 53 56 57 0F B7 48 0A 8B D9 81 E1 00 80 00 00",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int32_t",
    params=[
        param("uint16_t const*", "value"),
        param("void*", "output"),
        param("int32_t const*", "format"),
    ],
    doc="Converts an ld12 value to the IEEE representation selected by format and returns conversion status.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12ToDouble',
    '68 ?? ?? ?? ?? FF 74 24 0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3 68 ?? ?? ?? ?? FF 74 24 0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint16_t const*', 'value'),
        param('double*', 'output'),
    ],
    doc='Converts an ld12 value to an IEEE double and returns conversion status.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_LD12ToFloat',
    '68 ?? ?? ?? ?? FF 74 24 0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3 68 ?? ?? ?? ?? FF 74 24 0C FF 74 24 0C E8 ?? ?? ?? ?? 83 C4 0C C3',
    match=0x16,
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint16_t const*', 'value'),
        param('float*', 'output'),
    ],
    doc='Converts an ld12 value to an IEEE float and returns conversion status.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_AtoDbl',
    '55 8B EC 83 EC 0C 33 C0 50 50 50 50 FF 75 0C 8D 45 0C 50 8D 45 F4 50 E8 83 18 00 00',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('double*', 'output'),
        param('char const*', 'text'),
    ],
    doc='Parses text through the ld12 core, stores an IEEE double, and returns conversion status.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_AtoFlt',
    '55 8B EC 83 EC 0C 33 C0 50 50 50 50 FF 75 0C 8D 45 0C 50 8D 45 F4 50 E8 56 18 00 00',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('float*', 'output'),
        param('char const*', 'text'),
    ],
    doc='Parses text through the ld12 core, stores an IEEE float, and returns conversion status.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'String_CopyStringCRT',
    '57 8B 7C 24 08 EB ?? 8D A4 24 00 00 00 00',
    ret='char*',
    params=[
        param('char*', 'dest'),
        param('char const*', 'src'),
    ],
    doc='Copies source bytes including terminating NUL to destination and returns original destination.',
    stable=True,
)

stable.fn(
    "CRT_FPMath",
    "55 8B EC 83 EC 0C 56 8D 45 08 57 50 8D 45 F4 50 E8 ?? ?? ?? ?? 59 8D 75 F4",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="CRT_FloatConversionRecord*",
    params=[param("double", "value")],
    doc="Converts value through CRT_LongDouble12 and returns a shared record containing sign, point position, finiteness, and digits.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_DoubleToLongDouble80',
    '55 8B EC 51 8B 55 0C 53',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='void',
    params=[
        param('uint16_t*', 'destination'),
        param('double const*', 'source'),
    ],
    doc='Converts an IEEE double into the 10-byte x87 extended value at destination.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_GetStringTypeA",
    "55 8B EC 6A FF 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 18 53 56 57",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int32_t",
    params=[
        param("DWORD", "info_type"),
        param("LPCSTR", "src"),
        param("int32_t", "src_count"),
        param("uint16_t*", "char_types"),
        param("UINT", "code_page"),
        param("LCID", "locale"),
        param("int32_t", "strict"),
    ],
    doc="Classifies ANSI text directly or through a temporary UTF-16 conversion, returning Win32 character-type results.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_AullDiv",
    "53 56 8B 44 24 18 0B C0 75 ?? 8B 4C 24 14 8B 44 24 10 33 D2 F7 F1 8B D8",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="uint64_t",
    params=[param("uint64_t", "dividend"), param("uint64_t", "divisor")],
    doc="Returns the unsigned 64-bit quotient in EDX.EAX and pops both stack operands.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_AullRem",
    "53 8B 44 24 14 0B C0 75 ?? 8B 4C 24 10 8B 44 24 0C 33 D2 F7 F1 8B 44 24 08",
    cc=CallingConvention.STDCALL,
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="uint64_t",
    params=[param("uint64_t", "dividend"), param("uint64_t", "divisor")],
    doc="Returns the unsigned 64-bit remainder in EDX.EAX and pops both stack operands.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_SetMbcpUpdateTables",
    "55 8B EC 81 EC 14 05 00 00 8D 45 EC 56 50 FF 35 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 83 F8 01",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="void",
    params=[],
    doc="Rebuilds the CRT multibyte classification and case-mapping tables for the active code page.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    "CRT_LCMapStringA",
    "55 8B EC 6A FF 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 1C 53 56 57",
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret="int32_t",
    params=[
        param("LCID", "locale"),
        param("DWORD", "map_flags"),
        param("LPCSTR", "src"),
        param("int32_t", "src_count"),
        param("LPSTR", "dest"),
        param("int32_t", "dest_count"),
        param("UINT", "code_page"),
        param("int32_t", "strict"),
    ],
    doc="Maps ANSI text directly or through UTF-16 conversion using the requested locale, flags, and code page.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_Memset',
    '8B 54 24 0C 8B 4C 24 04',
    public=False,
    hook=0x8,
    ret='void*',
    params=[
        param('void*', 'dest'),
        param('int32_t', 'value'),
        param('uint32_t', 'count'),
    ],
    doc="Fills count bytes at destination with repeated low byte of value using aligned optimized stores. Returns destination.",
    stable=True,
)

stable.fn(
    'Mem_MemmoveBackwardBCRT',
    '8B 44 24 08 5F C3 8B 44 24 04 C3 CC CC CC CC CC CC CC CC 55 8B EC 57 56 8B 75 0C 8B 4D 10 8B 7D',
    match=0x13,
    public=False,
    ret='void*',
    params=[
        param('void*', 'dest'),
        param('void const*', 'src'),
        param('uint32_t', 'count'),
    ],
    doc="Second overlap-aware memmove implementation with forward/backward aligned dword fast paths. Returns original destination.",
    unstable=True,
)

stable.fn(
    'CRT_Strlen',
    '8B 4C 24 04 F7 C1 03 00 00 00',
    public=False,
    hook=0xA,
    ret='uint32_t',
    params=[param('char const*', 'str')],
    doc='Scans for NUL with aligned dword zero-byte detection and returns byte length.',
    stable=True,
)

stable.fn(
    'CRT_AbortFloatSupportNotLoaded',
    '6A 02 E8 ?? ?? ?? ?? 59 C3 55 8B EC',
    hook=0x7,
    callable=False,
    public=False,
    ret='void',
    params=[],
    doc="Calls CRT_AmsgExit with runtime error 2 when floating-point support is unavailable. It never returns.",
    unstable=True,
)

stable.fn(
    'CRT_IsLeadByteL',
    '8B 44 24 04 3B 05 ?? ?? ?? ?? 72 ?? 33 C0',
    public=False,
    hook=0xA,
    ret='int32_t',
    params=[param('int32_t', 'character')],
    doc='Checks active multibyte character table for lead-byte classification.',
    stable=True,
)

stable.fn(
    'String_ConvertCRTWideCharToMultiByte',
    '55 8B EC 8B 45 08 85 C0',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[
        param('char*', 'buffer'),
        param('uint16_t', 'wide_char'),
    ],
    doc="Converts one UTF-16 code unit into current-codepage multibyte bytes, or direct-copies <=0xFF in C locale. Sets error 0x2A on failure.",
    unstable=True,
)

stable.fn(
    'File_AllocateFileDescriptorCRT',
    '53 56 57 83 CB FF 33 FF',
    hook=0x6,
    public=False,
    ret='int32_t',
    params=[],
    doc='Scans 32-entry descriptor blocks for unused slot, lazily allocates/initializes 0x100-byte block, and returns fd or -1.',
    unstable=True,
)

stable.fn(
    'File_SetOSHandleCRT',
    '8B 44 24 04 56 3B 05 ?? ?? ?? ?? 57',
    public=False,
    ret='int32_t',
    params=[
        param('int32_t', 'file_no'),
        param('HANDLE', 'os_handle'),
    ],
    doc='Validates unused fd slot, installs OS HANDLE, mirrors stdin/stdout/stderr through SetStdHandle in console mode, else sets EBADF.',
    unstable=True,
)

stable.fn(
    'File_ClearFileDescriptorCRT',
    '8B 4C 24 04 56 3B 0D ?? ?? ?? ?? 57',
    public=False,
    ret='int32_t',
    params=[param('int32_t', 'file_no')],
    doc='Validates open fd, clears corresponding standard handle when applicable, replaces stored HANDLE with -1, else sets EBADF.',
    unstable=True,
)

stable.fn(
    'File_GetOSHandleCRT',
    '8B 44 24 04 3B 05 ?? ?? ?? ?? 73 ?? 8B C8 83 E0 1F',
    hook=0xA,
    ret='HANDLE',
    params=[param('int32_t', 'file_no')],
    doc="Validates fd range/open flag and returns stored OS HANDLE. Invalid input sets EBADF and returns -1.",
    stable=True,
)

stable.fn(
    'CRT_DosMapErr',
    '8B 4C 24 04 33 D2 89 0D ?? ?? ?? ??',
    public=False,
    hook=0x6,
    ret='int32_t',
    params=[param('uint32_t', 'os_error_code')],
    doc='Stores Win32 error, searches mapping table for errno, then applies range fallbacks for EACCES/ENOMEM/EINVAL.',
    stable=True,
)

stable.fn(
    'CRT_IsDigit',
    '6A 04 6A 00 FF 74 24 0C',
    public=False,
    hook=0x8,
    ret='int32_t',
    params=[param('int32_t', 'ch')],
    doc='Tests input against CRT digit character-type mask.',
    stable=True,
)

stable.fn(
    'CRT_SetMbcp',
    '55 8B EC 83 EC 18 53 56 57',
    public=False,
    hook=0x6,
    ret='int32_t',
    params=[param('int32_t', 'requested_code_page')],
    doc='Resolves requested codepage, resets or selects built-in lead/trail-byte ranges, otherwise queries OS CP info, then rebuilds multibyte.',
    stable=True,
)

stable.fn(
    'CRT_ResolveRequestedCodePage',
    '8B 44 24 04 83 25 ?? ?? ?? ?? 00 83 F8 FE',
    hook=0xB,
    public=False,
    ret='uint32_t',
    params=[param('int32_t', 'requested_code_page')],
    doc="Maps sentinel -2/-3/-4 to OEMCP/ACP/current codepage and marks sentinel-derived mode. Otherwise returns requested codepage.",
    unstable=True,
)

stable.fn(
    'CRT_MapCodePageToLocaleId',
    '8B 44 24 04 2D A4 03 00 00',
    hook=0x9,
    public=False,
    ret='uint32_t',
    params=[param('uint32_t', 'code_page')],
    doc='Maps codepages 932/936/949/950 to locale IDs 0x411/0x804/0x412/0x404, else 0.',
    unstable=True,
)

stable.fn(
    'CRT_ResetMbcp',
    '57 6A 40 59 33 C0 BF ?? ?? ?? ?? F3 AB',
    hook=0x6,
    public=False,
    ret='void',
    params=[],
    doc="Clears multibyte classification/case tables and codepage state. The terminal zero-fill result is unused.",
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_InitMbcTable',
    '83 3D ?? ?? ?? ?? 00 75 ?? 6A FD E8 ?? ?? ?? ?? 59',
    hook=0x7,
    public=False,
    ret='void',
    params=[],
    doc='Once-only guard calls CRT_SetMbcp(-3/ACP) then marks multibyte tables initialized.',
    unstable=True,
)

stable.fn(
    'CRT_ShowMessageBoxA',
    '53 33 DB 39 1D ?? ?? ?? ?? 56 57 75 ?? 68 ?? ?? ?? ??',
    hook=0x9,
    public=False,
    ret='int32_t',
    params=[
        param('LPCSTR', 'lp_text'),
        param('LPCSTR', 'lp_caption'),
        param('UINT', 'u_type'),
    ],
    doc='Lazily loads user32.dll, resolves MessageBoxA/GetActiveWindow/GetLastActivePopup, chooses active popup owner, and displays.',
    unstable=True,
)

stable.fn(
    'CRT_Strncpy',
    '8B 4C 24 0C 57 85 C9 74 ??',
    public=False,
    ret='char*',
    params=[
        param('char*', 'dest'),
        param('char const*', 'src'),
        param('uint32_t', 'count'),
    ],
    doc='Copies at most count bytes, pads destination with NUL when source ends, and returns destination.',
    stable=True,
)

stable.fn(
    'strnlen',
    '8B 54 24 08 8B 44 24 04 85 D2',
    hook=0x8,
    ret='uint32_t',
    params=[
        param('char const*', 'str'),
        param('uint32_t', 'max_count'),
    ],
    doc='Scans until NUL or maxCount and returns traversed length.',
    stable=True,
)

stable.fn(
    'CRT_AddWithOverflowCheck',
    '8B 54 24 04 56 8B 74 24 0C 33 C0',
    callable=False,
    public=False,
    ret='int32_t',
    params=[
        param('uint32_t', 'left'),
        param('uint32_t', 'right'),
        param('uint32_t*', 'sum'),
    ],
    doc='Stores left plus right through sum and returns 1 exactly when the unsigned addition carries.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_Add12',
    '56 8B 74 24 08 57 8B 7C 24 10',
    callable=False,
    public=False,
    ret='void',
    params=[
        param('uint32_t*', 'accumulator'),
        param('uint32_t const*', 'addend'),
    ],
    doc='Adds a 12-byte addend into accumulator with carry propagation across three dwords.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_ShiftLeft12Byte',
    '8B 44 24 04 56 57 8B 30',
    callable=False,
    public=False,
    ret='void',
    params=[param('uint32_t*', 'value')],
    doc='Shifts a 12-byte value left one bit in place across all three dwords.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_ShiftRight12Byte',
    '8B 44 24 04 56 57 8B 50 08',
    callable=False,
    public=False,
    ret='void',
    params=[param('uint32_t*', 'value')],
    doc='Shifts a 12-byte value right one bit in place across all three dwords.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_MtoLD12',
    '55 8B EC 83 EC 10 8B 45 0C 53 8B 5D 10 33 D2 3B C2 56 C7 45 FC 4E 40 00 00',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='void',
    params=[
        param('uint8_t const*', 'digits'),
        param('uint32_t', 'digit_count'),
        param('uint16_t*', 'output'),
    ],
    doc='Accumulates digit bytes into a normalized 12-byte ld12 value at output.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_StrgToLD12',
    '55 8B EC 83 EC 5C 53 56 57 8B 7D 10 8D 45 A4 6A 01 89 45 F4 33 C0 5A 89 45 D8',
    callable=False,
    public=False,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    ret='int32_t',
    params=[
        param('uint16_t*', 'output'),
        param('char**', 'end_pointer'),
        param('char const*', 'text'),
        param('int32_t', 'mult12'),
        param('int32_t', 'scale'),
        param('int32_t', 'decimal_point'),
        param('int32_t', 'implicit_exponent'),
    ],
    doc='Parses text to ld12, updates end_pointer, and returns the CRT conversion status flags.',
    abi_status=AbiStatus.PLACEHOLDER,
    unstable=True,
)

stable.fn(
    'CRT_FltMul',
    '55 8B EC 83 EC 24 53 8B 5D 0C',
    hook=0x6,
    callable=False,
    public=False,
    ret='void',
    params=[
        param('uint16_t*', 'left_and_result'),
        param('uint16_t const*', 'right'),
    ],
    doc='Multiplies two ld12 values and writes the normalized result over the left operand.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_FltPower10',
    '55 8B EC 83 EC 0C 53 BB ?? ?? ?? ??',
    hook=0x6,
    callable=False,
    public=False,
    ret='void',
    params=[
        param('uint16_t*', 'value'),
        param('int32_t', 'power'),
        param('int32_t', 'mult12'),
    ],
    doc='Scales value by a signed power of ten through table-driven ld12 multiplication.',
    abi_status=AbiStatus.VERIFIED,
    unstable=True,
)

stable.fn(
    'CRT_Strcmpi',
    '55 8B EC 57 56 53 8B 75 0C',
    public=False,
    ret='int32_t',
    params=[
        param('char const*', 's1'),
        param('char const*', 's2'),
    ],
    doc='Compares strings bytewise after locale-aware lowercase normalization, stopping at mismatch or NUL.',
    stable=True,
)



stable.fn(
    "Exception_RtlUnwindThunk",
    "FF 25 ?? ?? ?? ?? CC CC CC CC CC CC CC CC CC CC 55",
    cc=CallingConvention.STDCALL,
    hook=0x6,
    public=False,
    ret="void",
    params=[
        param("void*", "target_frame"),
        param("void*", "target_ip"),
        param("struct _EXCEPTION_RECORD*", "exception_record"),
        param("void*", "return_value"),
    ],
    doc="Stdcall import thunk to RtlUnwind with frame, target IP, exception record, and return value arguments.",
)

stable.data(
    "Graphics_AdjustLevelScale_DebugMaxFPSThreshold",
    xref("Graphics_AdjustLevelScale", 0x32, 0x2),
    type="int32_t",
    write_policy=WritePolicy.RAW_MEMORY,
)


# Function promoted with the data rows that use it as an xref resolver.
stable.fn(
    "Script_OpBranchConditional",
    "0B DA 80 F9 06 0F 84 ??",
    match=-0x68,
    ret="void",
    params=[param("Entity_State*", "actor"), param("uint8_t**", "ip")],
    doc=(
        "Consumes rel16 target, LHS ref, comparison, RHS ref kind, arithmetic opcode, and a "
        "four-byte big-endian RHS. Resolves operands and rewinds ip to the relative target when "
        "the selected equality or ordering comparison fails."
    ),
)

stable.data(
    "D3D_CreateTextureSurface_DDrawObject",
    xref("D3D_CreateTextureSurface", 0x2D6, 0x1),
    type="DDraw_IDirectDraw7*",
    doc="Primary IDirectDraw7 interface used for texture/work/z-buffer surface creation and released during DirectDraw shutdown.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Window_ProcessGameProc_Initialized",
    xref("Window_ProcessGameProc", 0x5F, 0x1),
    type="int32_t",
    doc="Non-zero after the main game window and runtime initialization have completed.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_GetPressedButton_JoystickAvailable",
    xref("Input_GetPressedButton", 0x1F, 0x1),
    type="uint8_t",
    doc="Non-zero when joystick/gamepad input is available; allows gamepad polling in Input_GetPressedButton.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Window_MainHandle",
    xref("D3D_InitDirectDrawAndDirect3D", 0x5, 0x2),
    type="HWND",
    doc="Cached main game HWND written by WinMain and read by DirectDraw/Direct3D, video, input, and window paths.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Window_RunWinMain_SecondaryWindowHandle",
    xref("Window_RunWinMain", 0x13B, 0x1),
    type="HWND",
    doc=(
        "Write-only secondary copy of the HWND returned by CreateWindowExA in Window_RunWinMain; "
        "main_window_handle is the runtime window handle read by input/movie/DirectDraw paths."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Window_RunWinMain_RenderingEnabled",
    xref("Window_RunWinMain", 0x163, 0x2),
    type="int32_t",
    doc="Flag checked by game rendering paths before drawing world content.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ProcessWindowMessages_ShouldQuit",
    xref("Input_ProcessWindowMessages", 0x0, 0x1),
    type="int32_t",
    doc="Flag set by window-message processing for game exit.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_InitializeSystem_DigitalDriver",
    xref("Audio_InitializeSystem", 0x7F, 0x1),
    type="Audio_AILHDigitalDriver",
    doc="Miles digital driver handle opened by AIL_waveOutOpen and cleared by Audio_ShutdownSystem.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Video_PlayMovieIntro_FileNames",
    xref("Video_PlayMovieIntro", 0xF, 0x3),
    type="char*[4]",
    doc="First entry/base of the four-entry movie filename pointer table used by intro and movie playback routines.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Video_PlayMovieIntro_PathPrefix",
    xref("Video_PlayMovieIntro", 0x17, 0x1),
    type="char",
    doc="First byte/base of the NUL-terminated data/movies path prefix used by movie-loading routines.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "PKG_ResourceTitleBonusReplayResource",
    xref("Title_CleanupScreenResources", 0xC, 0x1),
    type="void*",
    doc=(
        "Title-screen bonus replay resource pointer freed during "
        "Title_CleanupScreenResources and assigned/used by title-screen load/update paths."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Title_ResourceHandle1",
    xref("Title_CleanupScreenResources", 0x17, 0x2),
    xref("PKG_LoadTitleScreenResources", 0x24, 0x1),
    type="void*",
    doc=(
        "Title-screen resource handle slot 1 cleaned by PKG_CleanupResourceHandle during "
        "Title_CleanupScreenResources and loaded by PKG_LoadTitleScreenResources."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Title_ResourceHandle0",
    xref("Title_CleanupScreenResources", 0x23, 0x2),
    xref("PKG_LoadTitleScreenResources", 0xC, 0x1),
    type="void*",
    doc=(
        "Title-screen resource handle slot 0 cleaned by PKG_CleanupResourceHandle during "
        "Title_CleanupScreenResources and loaded by PKG_LoadTitleScreenResources."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "PKG_ResourceTitleMaterialBase",
    xref("Title_CleanupScreenResources", 0x2F, 0x1),
    type="Material_SectionHeader*",
    doc=(
        "Title-screen material/resource manager base released by Material_ReleaseSectionTextures "
        "during Title_CleanupScreenResources."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "PKG_ResourceTitlePackage",
    xref("Title_CleanupScreenResources", 0x3A, 0x2),
    type="PKG_TitlePackage*",
    doc=(
        "Title-screen resource package pointer freed by PKG_FreeResourceData during "
        "Title_CleanupScreenResources."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Scene_TraverseNodeTree_TypeDispatchTable",
    xref("Scene_TraverseNodeTree", 0x37A, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Scene_TraverseNodeTree to dispatch child "
    "scene-node type values 1..7.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Input_CheckButtonState_ControlCodeDispatchTable",
    xref("Input_CheckButtonState", 0x82, 0x3),
    type="uint32_t",
    doc=(
        "Ten-entry uint32_t jump table used by Input_CheckButtonState for control codes 0x20..0x47. "
        "Slots 0..8 handle aggregate direction axes and signed axis thresholds; slot 9 is "
        "the default return-zero path for unused codes 0x24..0x3f."
    ),
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Input_CheckButtonState_ControlCodeDispatchIndexTable",
    xref("Input_CheckButtonState", 0x7C, 0x2),
    type="uint8_t",
    doc=(
        "Uint8_t lookup table mapping control codes onto input_control_code_dispatch_table slots."
    ),
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_TracePath_TargetSelectorDispatchTable",
    xref("Actor_TracePath", 0x2C0, 0x3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Actor_TracePath for negative target-selector sentinel values -0x8000..-0x7FF7.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Entity_SetActorProperty_IDDispatchTable",
    xref("Entity_SetActorProperty", 0x3E, 0x3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Entity_SetActorProperty for property ids 0..9.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Actor_ProcessPlayerBehavior_StateDispatchTable",
    xref("Actor_ProcessPlayerBehavior", 0x26F, 0x3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Actor_ProcessPlayerBehavior for player behavior state values 0..4.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Actor_ProcessCollisionResponse_NodeTypeDispatchTable",
    xref("Actor_ProcessCollisionResponse", 0x14F, 0x3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Actor_ProcessCollisionResponse for collided actor/node type values 0..4.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Script_OpCheckTerminator_OpcodeTable",
    xref("Script_OpCheckTerminator", 0x1A, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table for Script_OpCheckTerminator opcodes 1 through 4.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Script_OpBranchConditional_ArithmeticOpTable",
    xref("Script_OpBranchConditional", 0x10B, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Script_OpBranchConditional for arithmetic/combine opcodes before comparison.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Script_OpBranchConditional_ComparisonOpTable",
    xref("Script_OpBranchConditional", 0x187, 0x3),
    type="uint32_t",
    doc="Six-entry uint32_t jump table used by Script_OpBranchConditional for comparison opcodes.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Script_OpSetVariable_OpJumpTable",
    xref("Script_OpSetVariable", 0x111, 0x3),
    type="uint32_t",
    doc=(
        "Four-entry uint32_t jump table used by Script_OpSetVariable for arithmetic opcode dispatch."
    ),
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Checkers_CheckCapturePossible_PieceTypeDispatchTable",
    xref("Checkers_CheckCapturePossible", 0x20, 0x3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t jump table for Checkers_CheckCapturePossible piece values 1..6. "
        "Pieces 1/2 are men, 5/6 are kings, and 3/4 fall through to the no-capture path."
    ),
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Checkers_ValidateMove_SimpleMovePieceTypeDispatchTable",
    xref("Checkers_ValidateMove", 0x68, 0x3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t Checkers_ValidateMove jump table for one-square moves by piece value "
        "1..6; pieces 3/4 share the invalid/default path and 5/6 share king movement."
    ),
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Checkers_ValidateMove_CaptureMovePieceTypeDispatchTable",
    xref("Checkers_ValidateMove", 0x10F, 0x3),
    type="uint32_t",
    doc=(
        "Six-entry uint32_t Checkers_ValidateMove jump table for two-square captures by piece value "
        "1..6; pieces 3/4 share the invalid/default path and 5/6 share king capture logic."
    ),
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Graphics_UpdateMeshCommandState_SignalDispatchTable",
    xref("Graphics_UpdateMeshCommandState", 0x77, 0x3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Graphics_UpdateMeshCommandState for Mesh_CommandEvent values 0 through 10.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "D3D_FormatDirectXError_88760028RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0xDF, 0x3),
    type="uint32_t",
    doc="Eight-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760028..0x88760078 DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "D3D_FormatDirectXError_88760028RangeIndexTable",
    xref("D3D_FormatDirectXError", 0xD9, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x88760028..0x88760078 HRESULT positions to D3D_FormatDirectXError jump-table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760091RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x14C, 0x3),
    type="uint32_t",
    doc="Fifteen-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760091..0x887600E1 DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760091RangeIndexTable",
    xref("D3D_FormatDirectXError", 0x146, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x88760091..0x887600E1 HRESULT positions to D3D_FormatDirectXError jump-table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_887600F0RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x201, 0x3),
    type="uint32_t",
    doc="Thirty-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x887600F0..0x887601D6 DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_887600F0RangeIndexTable",
    xref("D3D_FormatDirectXError", 0x1FB, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x887600F0..0x887601D6 HRESULT positions to D3D_FormatDirectXError jump-table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_887601EARangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x38E, 0x3),
    type="uint32_t",
    doc="Thirty-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x887601EA..0x88760245 DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_887601EARangeIndexTable",
    xref("D3D_FormatDirectXError", 0x388, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x887601EA..0x88760245 HRESULT positions to D3D_FormatDirectXError jump-table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760247RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x4FA, 0x3),
    type="uint32_t",
    doc="Sixteen-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760247..0x8876026C DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760247RangeIndexTable",
    xref("D3D_FormatDirectXError", 0x4F4, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x88760247..0x8876026C HRESULT positions to D3D_FormatDirectXError jump-table slots; 0x88760276 is handled as a separate singleton.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760280RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x5D7, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the 0x88760280..0x887602B4 DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_88760280RangeIndexTable",
    xref("D3D_FormatDirectXError", 0x5D1, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table that maps sparse 0x88760280..0x887602B4 HRESULT positions to D3D_FormatDirectXError jump-table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_887602B6RangeDispatchTable",
    xref("D3D_FormatDirectXError", 0x61F, 0x3),
    type="uint32_t",
    doc="Six-entry uint32_t jump table for D3D_FormatDirectXError HRESULT values in the contiguous 0x887602B6..0x887602BB DirectDraw error range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_InitializeBonusData_CodeDispatchTable",
    xref("Level_InitializeBonusData", 0x27, 0x3),
    type="uint32_t",
    doc="Five-entry uint32_t jump table used by Level_InitializeBonusData for bonus level ids 27..31.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_BuildCompletionTable_SlotDispatchTable",
    xref("Level_BuildCompletionTable", 0x178, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Level_BuildCompletionTable to store four packed completion masks.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Menu_RenderOptionsMenu_ItemDispatchTable",
    xref("Menu_RenderOptionsMenu", 0x103, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Menu_RenderOptionsMenu to render options menu rows 0..6.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Menu_ProcessMenuState_DispatchTable",
    xref("Menu_ProcessMenuState", 0xB1, 0x3),
    type="uint32_t",
    doc="Thirteen-entry uint32_t jump table used by Menu_ProcessMenuState for menu state values 1..13.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_Load_StateDispatchTable",
    xref("Level_Load", 0x10, 0x3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Level_Load for level-loading state values 0..10; state 9 maps to the idle/default return path.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Collision_ProcessProjectileHit_NodeTypeDispatchTable",
    xref("Collision_ProcessProjectileHit", 0x5D, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_ProcessProjectileHit for hit actor/node type values 1..4.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Collision_DetectAndResolve3DCollision_3dAxisDispatchTable",
    xref("Collision_DetectAndResolve3DCollision", 0xDFB, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_DetectAndResolve3DCollision to select one of four collision-normal/contact axes.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Collision_ResolveObjectNodeCollision_AxisDispatchTable",
    xref("Collision_ResolveObjectNodeCollision", 0x15A, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Collision_ResolveObjectNodeCollision to select one of four node collision axes.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_EvaluateCollisionConditionSelector_ConditionSubtypeDispatchTable",
    xref("Actor_EvaluateCollisionConditionSelector", 0x27, 0x3),
    type="uint32_t",
    doc="Four-entry jump table used after remapping collision subtype values 0x0D through 0x17.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_EvaluateCollisionConditionSelector_ConditionSubtypeIndexTable",
    xref("Actor_EvaluateCollisionConditionSelector", 0x21, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table mapping collision subtype values 0x0D..0x17 onto collision_condition_subtype_dispatch_table slots; max slot is 3.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_EvaluateCollisionConditionSelector_ConditionFlagMaskDispatchTable",
    xref("Actor_EvaluateCollisionConditionSelector", 0x123, 0x3),
    type="uint32_t",
    doc="Eight-entry jump table testing selector values 2 through 9 against masks 0x10,0x20,0x40,0x80,1,2,4,8.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_HandleCollisionResponse_SubtypeDispatchTable",
    xref("Actor_HandleCollisionResponse", 0x22, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Actor_HandleCollisionResponse for collision subtype values 0x0D..0x13.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Input_GetButtonIndex_MaskDispatchTable",
    xref("Input_GetButtonIndex", 0x19, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table mapping low input button bitmasks 1,2,4,8,0x10,0x20 through input_button_mask_index_table; slot 6 is the default unrecognized-mask path.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Input_GetButtonIndex_MaskIndexTable",
    xref("Input_GetButtonIndex", 0x13, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table for Input_GetButtonIndex masks 1..0x20; larger recognized masks are handled by direct compares.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Window_ProcessGameProc_LowMessageDispatchTable",
    xref("Window_ProcessGameProc", 0x40, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Window_ProcessGameProc for sparse low Win32 messages 0x02..0x10, including destroy/size/close handling; WM_KEYDOWN (0x100) is handled by a direct branch.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Window_ProcessGameProc_LowMessageIndexTable",
    xref("Window_ProcessGameProc", 0x3A, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table mapping Win32 message IDs 0x02..0x10 to window_low_message_dispatch_table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Window_ProcessGameProc_HighMessageDispatchTable",
    xref("Window_ProcessGameProc", 0x169, 0x3),
    type="uint32_t",
    doc="Four-entry uint32_t jump table used by Window_ProcessGameProc for Win32 messages 0x101..0x112, including WM_KEYUP, system key messages, and WM_SYSCOMMAND filtering.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Window_ProcessGameProc_HighMessageIndexTable",
    xref("Window_ProcessGameProc", 0x163, 0x2),
    type="uint8_t",
    doc="Uint8_t lookup table mapping sparse high Win32 message IDs 0x101..0x112 to window_high_message_dispatch_table slots.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "UI_UpdateAndRenderSprites_SpriteAnchorDispatchTable",
    xref("UI_UpdateAndRenderSprites", 0x4EB, 0x3),
    type="uint32_t",
    doc="Eight-entry uint32_t jump table used by UI_UpdateAndRenderSprites for sprite anchor codes 1..8.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Input_CalculateMovementVector_DirectionDispatchTable",
    xref("Input_CalculateMovementVector", 0x29, 0x3),
    type="uint32_t",
    doc="Ten-entry uint32_t jump table used by Input_CalculateMovementVector to map low-nibble direction masks to heading positions; kept as read-only scalar/base table metadata.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Actor_ProcessMovementCommands_CommandOpcodeDispatchTable",
    xref("Actor_ProcessMovementCommands", 0x4F, 0x3),
    type="uint32_t",
    doc="Eleven-entry uint32_t jump table used by Actor_ProcessMovementCommands for movement command opcodes 0..10; kept as read-only scalar/base table metadata.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Title_UpdateAndRenderScreen_StateDispatchTable",
    xref("Title_UpdateAndRenderScreen", 0x20, 0x3),
    type="uint32_t",
    doc="Seven-entry uint32_t jump table used by Title_UpdateAndRenderScreen for title-screen state values 0..6.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Graphics_AdjustLevelScale_MaxGammaClamp",
    xref("Graphics_AdjustLevelScale", 0x21, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_RHWDepthMul2",
    xref("Level_InitializeActorSystem", 0x1A1, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_DegenerateTriArea",
    xref("Level_InitializeActorSystem", 0x15E, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_CameraInitDist",
    xref("Level_InitializeActorSystem", 0x156, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ZDepthScale",
    xref("Graphics_ClipAndDrawPolygon", 0x68D, 0x2),
)
stable.data(
    "Math_One",
    xref("Graphics_RenderTexturedQuad", 0xC2, 0x2),
    xref("DInput_SetConstantForceEffect", 0x17, 0x2),
)
stable.data(
    "Math_Zero",
    xref("Graphics_ClipPolygonByPlane", 0x117, 0x2),
    xref("Video_OpenMovieFile", 0x1B, 0x2),
)
stable.data(
    "Camera_SetupClipPlanes_FOVAngleScale",
    xref("Camera_SetupClipPlanes", 0x17, 0x2),
)
stable.data(
    "Math_SnapVertexToNearestPoint_DebugFPSUpdateInterval",
    xref("Math_SnapVertexToNearestPoint", 0x69, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_BottomEdgeClamp",
    xref("Graphics_ClipAndDrawPolygon", 0x759, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_RightEdgeClamp",
    xref("Graphics_ClipAndDrawPolygon", 0x72C, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_MaxZDepthClamp",
    xref("Graphics_ClipAndDrawPolygon", 0x697, 0x2),
)
stable.data(
    "Graphics_ProjectScreenHeightHalf",
    xref("Graphics_ClipAndDrawPolygon", 0x656, 0x2),
    type="float",
    doc="Float constant 240.0, half of the fixed 480-pixel projection/screen height used by clipping math.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ProjectScreenWidthHalf",
    xref("Graphics_ClipAndDrawPolygon", 0x631, 0x2),
)
stable.data("Graphics_DrawQuad_NegZBias", xref("Graphics_DrawQuad", 0xACC, 0x2))
stable.data(
    "Graphics_DrawQuad_PosZBias",
    xref("Graphics_DrawQuad", 0xAD4, 0x2),
    type="float",
    doc="Small positive Z-bias constant loaded by Graphics_DrawQuad; split from the project-screen-height constant at 0x44D038.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data("Graphics_DrawQuad_AltUVOffset", xref("Graphics_DrawQuad", 0x261, 0x2))
stable.data(
    "Camera_SetupProjection_FixedToFloat",
    xref("Camera_SetupProjection", 0xA1, 0x2),
)
stable.data(
    "Camera_SetupProjection_AspectCorrection",
    xref("Camera_SetupProjection", 0x60, 0x2),
)
stable.data(
    "Camera_SetupProjection_DebugPosScale",
    xref("Camera_SetupProjection", 0x36, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_WindowWidthFloat",
    xref("Graphics_RenderPolygonBatch", 0x184B, 0x2),
    type="float",
    doc="Float constant 640.0 used by polygon-batch projection checks.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Graphics_WindowWidth",
    xref("D3D_InitDirectDrawAndDirect3D", 0x5C, 0x2),
    type="int32_t",
    doc="Active render/window width in pixels; initialized during DirectDraw/Direct3D setup and read by render projection/batching paths.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Graphics_RenderPolygonBatch_MinFogDist",
    xref("Graphics_RenderPolygonBatch", 0x1831, 0x2),
)
stable.data("Timer_GetGameTime_MsToSec", xref("Timer_GetGameTime", 0x19, 0x2))
stable.data(
    "Input_TriggerRumbleIfAllowed_ForceScale",
    xref("Input_TriggerRumbleIfAllowed", 0x13, 0x2),
)
stable.data(
    "D3D_SetGammaFromMenuSetting_GraphicsGammaStep",
    xref("D3D_SetGammaFromMenuSetting", 0x3A, 0x2),
)
stable.data(
    "D3D_SetGammaFromMenuSetting_GraphicsDefaultGamma",
    xref("D3D_SetGammaFromMenuSetting", 0x4, 0x2),
)
stable.data(
    "Input_ReadGamepad_AxisScale",
    xref("Input_ReadGamepad", 0xDB, 0x2),
    type="float",
    doc="Float constant -4096.0 used to convert post-deadzone DirectInput axis values into signed Q12 input axes.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Input_ReadGamepad_AxisInvRange",
    xref("Input_ReadGamepad", 0xD5, 0x2),
    type="float",
    doc="Float constant 1/600 used with gamepad_axis_scale; the deadzone check uses literal "
    "+/-100 in Input_ReadGamepad.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Input_TriggerRumbleIfAllowed_StrongScale",
    xref("Input_TriggerRumbleIfAllowed", 0x23, 0x2),
)
stable.data(
    "Input_TriggerRumbleIfAllowed_WeakScale",
    xref("Input_TriggerRumbleIfAllowed", 0x1B, 0x2),
)
stable.data(
    "D3D_SetGammaRamp_DDrawGammaControlGUID", xref("D3D_SetGammaRamp", 0x82, 0x1)
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawDirectDraw7GUID",
    xref("D3D_InitializeDirectDraw", 0x1F7, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_TnLHALDeviceIID",
    xref("D3D_CreateTextureSurface", 0x86, 0x1),
)
stable.data(
    "D3D_SelectDefaultDevice_HALDeviceIID",
    xref("D3D_SelectDefaultDevice", 0x78, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_RGBDeviceIID",
    xref("D3D_CreateTextureSurface", 0x43, 0x1),
)
stable.data(
    "DInput_Device2AIID",
    xref("DInput_QueryDevice2AInterface", 0x9, 0x1),
    type="Win32_GUID",
    doc="IID_IDirectInputDevice2A GUID passed to IDirectInputDevice::QueryInterface.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "CRT_TestFdivBugFallback_CRTZero",
    xref("CRT_TestFdivBugFallback", 0x2A, 0x2),
)
stable.data(
    "CRT_TestFdivBugFallback_CRTNegativeTwo",
    xref("CRT_TestFdivBugFallback", 0xF, 0x2),
)
stable.data(
    "CRT_TestPentiumFdivBug_CRTKernel32", xref("CRT_TestPentiumFdivBug", 0x0, 0x1)
)
stable.data(
    "String_ConvertToExponentialFloat_CRTExponentSuffix",
    xref("String_ConvertToExponentialFloat", 0x93, 0x1),
)
stable.data(
    "String_ConvertToDecimalStringFloat_CRTQNAN",
    xref("String_ConvertToDecimalStringFloat", 0xF5, 0x1),
)
stable.data(
    "String_ConvertToDecimalStringFloat_CRTINF",
    xref("String_ConvertToDecimalStringFloat", 0xD8, 0x1),
)
stable.data(
    "String_ConvertToDecimalStringFloat_CRTIND",
    xref("String_ConvertToDecimalStringFloat", 0xC7, 0x1),
)
stable.data(
    "String_ConvertToDecimalStringFloat_CRTSNaN",
    xref("String_ConvertToDecimalStringFloat", 0xAD, 0x1),
)
stable.data(
    "Level_InitializeActorSystem_ComponentSpawnStateBuffer",
    xref("Level_InitializeActorSystem", 0x13B, 0x4),
)
stable.data(
    "Level_InitializeActorSystem_MaxLevelScale",
    xref("Level_InitializeActorSystem", 0x12C, 0x2),
)
stable.data(
    "Graphics_AdjustLevelScale_OneOverThirtyFPS",
    xref("Graphics_AdjustLevelScale", 0x50, 0x2),
    type="float",
    doc="Scalar float constant 1/30 (one frame at 30 FPS); not a reciprocal lookup table.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Video_MCI_OpenAVIVideo",
    xref("Video_InitializeAVIPlayer", 0x6, 0x1),
    xref("Video_OpenAVIFile", 0x33, 0x1),
    type="char",
    doc='NUL-terminated MCI command fragment "open" used by AVI/movie playback setup.',
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Video_ShutdownAVIPlayer_MCICloseAVIVideo",
    xref("Video_ShutdownAVIPlayer", 0x6, 0x1),
)
stable.data("Video_OpenAVIFile_MCIMovieID", xref("Video_OpenAVIFile", 0x15, 0x1))
stable.data("Video_CloseAVIFile_MCICloseDevice", xref("Video_CloseAVIFile", 0x5, 0x1))
stable.data(
    "Video_PlayAVIFullscreen_MCIPlayFullscreen",
    xref("Video_PlayAVIFullscreen", 0x5, 0x1),
)
stable.data(
    "Video_IsAVIPlaying_MCIStatusPlaying", xref("Video_IsAVIPlaying", 0x33, 0x1)
)
stable.data("Video_IsAVIPlaying_MCIStatusMode", xref("Video_IsAVIPlaying", 0x8, 0x1))
stable.data(
    "Graphics_RenderTexturedSprite_FontGlyphRenderState",
    xref("Graphics_RenderTexturedSprite", 0x17, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsCooperativeLevelSet",
    xref("D3D_InitDirectDrawAndDirect3D", 0x11, 0x2),
)
stable.data(
    "Camera_SetupProjection_Divisor",
    xref("Camera_SetupProjection", 0xF, 0x2),
)
stable.data(
    "Graphics_LoadAndUploadTexture_TexErrTex2Null",
    xref("Graphics_LoadAndUploadTexture", 0x81, 0x1),
)
stable.data(
    "Graphics_LoadAndUploadTexture_TexErrTex1Null",
    xref("Graphics_LoadAndUploadTexture", 0x54, 0x1),
)
stable.data(
    "Graphics_LoadAndUploadTexture_TexErrTex0Null",
    xref("Graphics_LoadAndUploadTexture", 0x27, 0x1),
)
stable.data(
    "Graphics_LoadAndUploadTexture_ErrorString",
    xref("Graphics_LoadAndUploadTexture", 0x22, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_ErrCreateTexture",
    xref("D3D_CreateTextureSurface", 0x2F1, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_ErrNo4444RGBA",
    xref("D3D_CreateTextureSurface", 0x16C, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_Error",
    xref("D3D_CreateTextureSurface", 0x167, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_ErrNo16BitRGB",
    xref("D3D_CreateTextureSurface", 0x160, 0x1),
)
stable.data(
    "D3D_CreateWorkSurface_ErrCreateWorkSurface",
    xref("D3D_CreateWorkSurface", 0xB4, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrBlt",
    xref("Material_CopyPixelDataToTexture", 0x2ED, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrGetSurfacePtr",
    xref("Material_CopyPixelDataToTexture", 0xD6, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrorMessageBuffer",
    xref("Material_CopyPixelDataToTexture", 0xD1, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrLock",
    xref("Material_CopyPixelDataToTexture", 0x9E, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrWorkNull",
    xref("Material_CopyPixelDataToTexture", 0x73, 0x1),
)
stable.data(
    "Material_CopyPixelDataToTexture_D3DErrCopyMemTexNull",
    xref("Material_CopyPixelDataToTexture", 0x35, 0x1),
)
stable.data(
    "Graphics_BlitTextureToQuadrants_D3DErrWorkSurfaceNull",
    xref("Graphics_BlitTextureToQuadrants", 0x89, 0x1),
)
stable.data(
    "Debug_Log_LineFormat",
    xref("Debug_Log", 0x12, 0x1),
    type="char[4]",
    doc='Debug-log line format string "%s\\n" used before writing the formatted message.',
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrZEnable",
    xref("D3D_InitDirectDrawAndDirect3D", 0x58B, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrSetViewport",
    xref("D3D_InitDirectDrawAndDirect3D", 0x42A, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateRGBDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x3B7, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateHALDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x37F, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateDriverDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x347, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateDevice7",
    xref("D3D_InitDirectDrawAndDirect3D", 0x306, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrAddAttachedSurface",
    xref("D3D_InitDirectDrawAndDirect3D", 0x2FB, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_SoftwareDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x2AF, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_TnLDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x298, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_HALDevice",
    xref("D3D_InitDirectDrawAndDirect3D", 0x26F, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateZBuffer",
    xref("D3D_InitDirectDrawAndDirect3D", 0x1DD, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrQueryDirect3D7",
    xref("D3D_InitDirectDrawAndDirect3D", 0x19C, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrGetAttachedSurface",
    xref("D3D_InitDirectDrawAndDirect3D", 0x17A, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrCreateSurface",
    xref("D3D_InitDirectDrawAndDirect3D", 0x138, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrSetDisplayMode",
    xref("D3D_InitDirectDrawAndDirect3D", 0x106, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_ErrSetCooperativeLevel",
    xref("D3D_InitDirectDrawAndDirect3D", 0x2E, 0x1),
)
stable.data(
    "D3D_ReleaseAllAndReportLeaks_TexturesStillActive",
    xref("D3D_ReleaseAllAndReportLeaks", 0x8F, 0x1),
)
stable.data("D3D_CloseDebugLog_ClosingLog", xref("D3D_CloseDebugLog", 0x0, 0x1))
stable.data(
    "D3D_InitializeDirectDraw_LogSeparator",
    xref("D3D_InitializeDirectDraw", 0x1B5, 0x1),
)
stable.data(
    "D3D_InitializeDirectDraw_CanUseColorKey",
    xref("D3D_InitializeDirectDraw", 0x177, 0x1),
)
stable.data(
    "D3D_InitializeDirectDraw_SelectedDriverHeader",
    xref("D3D_InitializeDirectDraw", 0x7D, 0x1),
)
stable.data(
    "D3D_InitializeDirectDraw_OpenLog", xref("D3D_InitializeDirectDraw", 0x2E, 0x1)
)
stable.data("D3D_InitializeDirectDraw_Log", xref("D3D_InitializeDirectDraw", 0x15, 0x1))
stable.data(
    "D3D_InitializeDirectDraw_LogFileMode",
    xref("D3D_InitializeDirectDraw", 0x10, 0x1),
)
stable.data(
    "Graphics_TakeScreenshot_FileModeReadBinary",
    xref("Graphics_TakeScreenshot", 0x1D, 0x1),
)
stable.data(
    "Graphics_TakeScreenshot_FormatString",
    xref("Graphics_TakeScreenshot", 0xE, 0x1),
    doc="Screenshot filename/format string used by Graphics_TakeScreenshot.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Graphics_ConvertSurface565ToBGR24_OutputBuffer",
    xref("Graphics_ConvertSurface565ToBGR24", 0x72, 0x2),
    type="uint8_t[921600]",
    doc="Engine-owned fixed 640x480 BGR24 screenshot conversion buffer.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data("D3D_SetGammaRamp_ErrSetGammaControl", xref("D3D_SetGammaRamp", 0x118, 0x1))
stable.data(
    "D3D_SetGammaRamp_ErrQueryGammaControl", xref("D3D_SetGammaRamp", 0x90, 0x1)
)
stable.data("D3D_SetGammaRamp_NotSupported", xref("D3D_SetGammaRamp", 0x59, 0x1))
stable.data(
    "D3D_EnumerateDirectDrawDevices_TryRefRasterizer",
    xref("D3D_EnumerateDirectDrawDevices", 0x65, 0x1),
)
stable.data(
    "D3D_EnumerateDirectDrawDevices_NoDevicesAccepted",
    xref("D3D_EnumerateDirectDrawDevices", 0x4C, 0x1),
)
stable.data(
    "D3D_EnumerateDirectDrawDevices_NoDevicesEnumerated",
    xref("D3D_EnumerateDirectDrawDevices", 0x23, 0x1),
)
stable.data(
    "D3D_EnumDriverCallback_D3DErrQueryDuringEnum",
    xref("D3D_EnumDriverCallback", 0x56, 0x1),
)
stable.data(
    "D3D_EnumDriverCallback_D3DErrCreateDuringEnum",
    xref("D3D_EnumDriverCallback", 0x29, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrorUnknownMessage",
    xref("D3D_FormatDirectXError", 0x650, 0x1),
    type="char",
    doc=(
        "NUL-terminated fallback DirectDraw error string used by D3D_FormatDirectXError."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrOk", xref("D3D_FormatDirectXError", 0x649, 0x1)
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrD3DNotInitialized",
    xref("D3D_FormatDirectXError", 0x626, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNewMode",
    xref("D3D_FormatDirectXError", 0x608, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrPageLockFailed",
    xref("D3D_FormatDirectXError", 0x5DE, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNoNonLocalVidMem",
    xref("D3D_FormatDirectXError", 0x5AE, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrCantDuplicate",
    xref("D3D_FormatDirectXError", 0x501, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNotFlippable",
    xref("D3D_FormatDirectXError", 0x4C9, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrWidthTooLarge",
    xref("D3D_FormatDirectXError", 0x395, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrSizeTooLarge",
    xref("D3D_FormatDirectXError", 0x35D, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrOutOfMemory",
    xref("D3D_FormatDirectXError", 0x2A8, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNoGdi", xref("D3D_FormatDirectXError", 0x208, 0x1)
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNoFlipHardware",
    xref("D3D_FormatDirectXError", 0x1DF, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrInvalidPixelFormat",
    xref("D3D_FormatDirectXError", 0x153, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrInvalidObject",
    xref("D3D_FormatDirectXError", 0x12C, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrCurrentlyNotAvail",
    xref("D3D_FormatDirectXError", 0xE6, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrCanNotDetach",
    xref("D3D_FormatDirectXError", 0xBF, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrInvalidParams",
    xref("D3D_FormatDirectXError", 0xB5, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrAlreadyInitialized",
    xref("D3D_FormatDirectXError", 0xAB, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrCanNotAttach",
    xref("D3D_FormatDirectXError", 0xA1, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNotSupported",
    xref("D3D_FormatDirectXError", 0x7E, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrGenericFailure",
    xref("D3D_FormatDirectXError", 0x74, 0x1),
)
stable.data(
    "D3D_FormatDirectXError_DDrawErrNotInitialized",
    xref("D3D_FormatDirectXError", 0x6A, 0x1),
)
stable.data(
    "Video_OpenMovieFile_PlaybackState",
    xref("Video_OpenMovieFile", 0x11, 0x1),
)
stable.data(
    "Video_OpenMovieFile_DefaultScreenWidth",
    xref("Video_OpenMovieFile", 0x2C, 0x2),
)
stable.data(
    "Video_OpenMovieFile_DefaultScreenHeight",
    xref("Video_OpenMovieFile", 0x21, 0x1),
)
stable.data(
    "Video_OpenMovieFile_CurrentRectLeft", xref("Video_OpenMovieFile", 0x0, 0x1)
)
stable.data("Video_OpenMovieFile_CurrentRectTop", xref("Video_OpenMovieFile", 0x5, 0x2))
stable.data(
    "Video_OpenMovieFile_CurrentRectRight", xref("Video_OpenMovieFile", 0xB, 0x2)
)
stable.data(
    "Video_OpenMovieFile_CurrentRectBottom", xref("Video_OpenMovieFile", 0x16, 0x1)
)
stable.data("Video_OpenMovieFile_AltRectLeft", xref("Video_OpenMovieFile", 0x34, 0x2))
stable.data("Video_OpenMovieFile_AltRectTop", xref("Video_OpenMovieFile", 0x3A, 0x2))
stable.data("Video_OpenMovieFile_AltRectRight", xref("Video_OpenMovieFile", 0x40, 0x1))
stable.data("Video_OpenMovieFile_AltRectBottom", xref("Video_OpenMovieFile", 0x4B, 0x2))
stable.data(
    "Save_GameLevelCompletion_DalmatianBonusLevelIds",
    xref("Save_SaveGameLevelCompletion", 0x14D, 0x3),
)
stable.data(
    "Menu_ProcessNameEntryInput_Charset",
    xref("Menu_ProcessNameEntryInput", 0x37, 0x4),
)
stable.data(
    "Menu_RenderDifficultySelection_OptionEasy",
    xref("Menu_RenderDifficultySelection", 0x3E, 0x1),
)
stable.data(
    "g_sz_Level_BonusTOB",
    xref("Level_InitializeBonusData", 0x52, 0x1),
    doc='Read-only NUL-terminated "TOB" initials used as the level-31 bonus default.',
    type="char[4]",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "g_sz_Level_BonusRFF",
    xref("Level_InitializeBonusData", 0x2E, 0x1),
    doc='Read-only NUL-terminated "RFF" initials used as the level-27 and level-29 bonus defaults.',
    type="char[4]",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "g_sz_Level_BonusPR3",
    xref("Level_InitializeBonusData", 0x3A, 0x1),
    doc='Read-only NUL-terminated "PR3" initials used as the level-28 bonus default.',
    type="char[4]",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Menu_PauseCheaterText",
    xref("Menu_UpdatePauseMenu", 0x21F, 0x1),
    doc='Read-only "Cheater!" text displayed by Menu_UpdatePauseMenu.',
    type="char[9]",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data("Menu_RenderSaveGame_Percent", xref("Menu_RenderSaveGame", 0x348, 0x1))
stable.data(
    "Menu_RenderSaveGame_StringFormatStringAndInt",
    xref("Menu_RenderSaveGame", 0x295, 0x1),
)
stable.data(
    "Menu_RenderControlsConfiguration_Cancel",
    xref("Menu_RenderControlsConfiguration", 0x368, 0x1),
)
stable.data(
    "Menu_RenderControlsConfiguration_Accept",
    xref("Menu_RenderControlsConfiguration", 0x327, 0x1),
)
stable.data(
    "Menu_RenderControlsConfiguration_StringFormatStringTwoStrings",
    xref("Menu_RenderControlsConfiguration", 0x16E, 0x1),
)
stable.data(
    "g_currentLevelID",
    xref("Player_ProcessMovement", 0x141, 0x2),
    type="int16_t",
    doc="Signed runtime level ID. -1 is the cleared/no-level sentinel.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Input_CheckCheatCodeSequence_Sequence",
    xref("Input_CheckCheatCodeSequence", 0x1E, 0x3),
)
stable.data(
    "Replay_BonusReplayLevelIDs",
    xref("Replay_LoadBonusReplay", 0x61, 0x3),
    doc="Bonus replay level-id table read by Replay_LoadBonusReplay.",
)
stable.data(
    "Replay_LoadBonusReplay_Index",
    xref("Replay_LoadBonusReplay", 0xD, 0x1),
)
stable.data(
    "Math_BuildMatrixRotationXY_FixedOneScratch1",
    xref("Math_BuildMatrixRotationXY", 0xA4, 0x1),
    doc="Fixed-point one constant/local slot used by the XY rotation matrix builder; suffix "
    "disambiguates the repeated constant reference.",
    unstable=True,
)
stable.data(
    "Math_BuildRotationMatrix_FixedZero1",
    xref("Math_BuildRotationMatrix", 0x66, 0x1),
)
stable.data(
    "Math_BuildMatrixRotationXY_FixedOneScratch2",
    xref("Math_BuildMatrixRotationXY", 0x77, 0x3),
    doc="Second fixed-point one constant/local slot used by the XY rotation matrix builder; "
    "suffix disambiguates the repeated constant reference.",
    unstable=True,
)
stable.data(
    "Math_BuildRotationMatrix_FixedZero2",
    xref("Math_BuildRotationMatrix", 0x75, 0x2),
)
stable.data(
    "Math_BuildMatrixRotationXY_FixedOneShort",
    xref("Math_BuildMatrixRotationXY", 0x7E, 0x3),
)
stable.data(
    "Math_BuildMatrixRotationY_FixedOne3",
    xref("Math_BuildMatrixRotationY", 0x32, 0x3),
)
stable.data(
    "Math_BuildMatrixRotationY_FixedZero3",
    xref("Math_BuildMatrixRotationY", 0x26, 0x2),
)
stable.data(
    "Math_BuildMatrixRotationY_FixedOne4",
    xref("Math_BuildMatrixRotationY", 0x76, 0x2),
)
stable.data(
    "Math_BuildMatrixRotationY_FixedZero4",
    xref("Math_BuildMatrixRotationY", 0x41, 0x3),
)
stable.data(
    "Math_BuildMatrixRotationY_FixedOneShort2",
    xref("Math_BuildMatrixRotationY", 0x39, 0x3),
)
stable.data(
    "Math_BuildRotationMatrix_FixedOne5",
    xref("Math_BuildRotationMatrix", 0x130, 0x1),
)
stable.data(
    "Math_BuildRotationMatrix_FixedOne6",
    xref("Math_BuildRotationMatrix", 0x146, 0x3),
)
stable.data(
    "Collision_InitializePlaneLookupTables_VertexIndexRemapTable1",
    xref("Collision_InitializePlaneLookupTables", 0xA, 0x1),
)
stable.data(
    "Audio_InitializeSystem_Emulated", xref("Audio_InitializeSystem", 0x9A, 0x1)
)
stable.data(
    "Audio_ShutdownSystem_ActiveWavesThemes",
    xref("Audio_ShutdownSystem", 0x4A, 0x1),
)
stable.data(
    "g_fmt_Audio_MusicPath",
    xref("Audio_OpenStream", 0x2B, 0x1),
    type="char[0x10]",
    doc="Read-only music path format string used by Audio_OpenStream.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "PKG_FixUpResourceLevelPointers_End",
    xref("PKG_FixUpResourceLevelPointers", 0x560, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_UsableMaterials",
    xref("PKG_FixUpResourceLevelPointers", 0x4D7, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_NavNet",
    xref("PKG_FixUpResourceLevelPointers", 0x466, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_PowerupCount",
    xref("PKG_FixUpResourceLevelPointers", 0x420, 0x1, required=Required.EN),
    xref("PKG_FixUpResourceLevelPointers", 0x468, 0x1, required=Required.EU_SC),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_TrailList",
    xref("PKG_FixUpResourceLevelPointers", 0x371, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_MaxThemes",
    xref("PKG_FixUpResourceLevelPointers", 0x32D, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_SpriteList",
    xref("PKG_FixUpResourceLevelPointers", 0x2B7, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_PowerupCountEllipsis",
    xref("PKG_FixUpResourceLevelPointers", 0x26B, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_VariableList",
    xref("PKG_FixUpResourceLevelPointers", 0x241, 0x1),
    doc="Level variable-list pointer/count region processed by PKG_FixUpResourceLevelPointers.",
    unstable=True,
)
stable.data(
    "PKG_FixUpResourceLevelPointers_SoundDefinitionList",
    xref("PKG_FixUpResourceLevelPointers", 0x217, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_CycleActorList",
    xref("PKG_FixUpResourceLevelPointers", 0x127, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_ActorListNotNull",
    xref("PKG_FixUpResourceLevelPointers", 0x10C, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_ActorListNull",
    xref("PKG_FixUpResourceLevelPointers", 0xFA, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_ActorCountZero",
    xref("PKG_FixUpResourceLevelPointers", 0xD1, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_ActorCount",
    xref("PKG_FixUpResourceLevelPointers", 0xB8, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_CamDefaultNull",
    xref("PKG_FixUpResourceLevelPointers", 0xA0, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_CamDefaultNotNull",
    xref("PKG_FixUpResourceLevelPointers", 0x93, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_CamDefaultGetAddr",
    xref("PKG_FixUpResourceLevelPointers", 0x70, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_CamDefaultAbsAddr",
    xref("PKG_FixUpResourceLevelPointers", 0x53, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_BaseNull",
    xref("PKG_FixUpResourceLevelPointers", 0x3B, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_Null",
    xref("PKG_FixUpResourceLevelPointers", 0x23, 0x1),
)
stable.data(
    "PKG_FixUpResourceLevelPointers_Start",
    xref("PKG_FixUpResourceLevelPointers", 0xB, 0x1),
)
stable.data(
    "Save_ReadGameFile_Dat",
    xref("Save_ReadGameFile", 0x6, 0x1),
    type="char",
    doc='First byte/base of the "savegame.dat" path literal shared by Save_ReadGameFile and Save_WriteGameFile.',
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("Save_WriteGameFile_ModeWriteBinary", xref("Save_WriteGameFile", 0x1, 0x1))
stable.data("Video_PlayMovieIntro_ErrPlay", xref("Video_PlayMovieIntro", 0xF9, 0x1))
stable.data("Video_PlayMovieIntro_ErrOpen", xref("Video_PlayMovieIntro", 0xBA, 0x1))
stable.data(
    "Video_PlayMovieIntro_StringConcat3", xref("Video_PlayMovieIntro", 0x21, 0x1)
)
stable.data("Debug_RenderOverlay_FPSFormat", xref("Debug_RenderOverlay", 0xA3, 0x1))
stable.data("Debug_RenderOverlay_PosFormat", xref("Debug_RenderOverlay", 0x75, 0x1))
stable.data(
    "Graphics_ClipAndDrawPolygon_MaxPrimitivesPerBatch",
    xref("Graphics_ClipAndDrawPolygon", 0x7D4, 0x2),
)
stable.data(
    "Config_SaveSettingsToINI_FileHeaderPcdogs",
    xref("Config_SaveSettingsToINI", 0x40, 0x1),
    type="char[7]",
    doc="Literal PCDOGS header written for pcdogs.ini.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "g_configFileChecksum",
    xref("Config_LoadFromINI", 0x43, 0x1),
    type="int32_t",
    doc="Expected pcdogs.ini header checksum checked by Config_LoadFromINI.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Input_ButtonNameStringIDs",
    xref("Input_FormatButtonName", 0x3C, 0x3),
    type="int32_t[0x113]",
    doc=(
        "First entry/base of the input button-name string-id table consumed by Input_FormatButtonName and "
        "Input_GetButtonString."
    ),
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data("Config_LoadFromINI_FilePcdogsINI", xref("Config_LoadFromINI", 0x9, 0x1))
stable.data(
    "Input_InitializeControllerMappings_WingmanRumblepad",
    xref("Input_InitializeControllerMappings", 0xED, 0x1),
)
stable.data(
    "Input_InitializeControllerMappings_GravisGamepad",
    xref("Input_InitializeControllerMappings", 0x8A, 0x1),
)
stable.data(
    "Input_InitializeControllerMappings_MsSidewinder",
    xref("Input_InitializeControllerMappings", 0x27, 0x1),
)
stable.data(
    "Input_InitializeControllerMappings_HammerheadFx",
    xref("Input_InitializeControllerMappings", 0x4, 0x1),
)
stable.data(
    "g_sz_Input_NoKeyAssigned",
    xref("Input_FormatButtonName", 0x70, 0x1),
    type="char[0x10]",
    doc='Read-only "No key assigned" string used by Input_FormatButtonName.',
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "Window_RunWinMain_GameTitle102Dalmatians",
    xref("Window_RunWinMain", 0x52, 0x4),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsWindowHeight",
    xref("D3D_InitDirectDrawAndDirect3D", 0x56, 0x2),
    type="int32_t",
    doc="Active render/window height in pixels initialized during DirectDraw/Direct3D setup.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Graphics_RenderFrame_D3DErrBeginScene", xref("Graphics_RenderFrame", 0x79, 0x1)
)
stable.data(
    "Graphics_RenderFrame_D3DErrRestoreAllSurfaces",
    xref("Graphics_RenderFrame", 0x52, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownComplete",
    xref("Window_ProcessGameProc", 0x102, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownDestroyWindow",
    xref("Window_ProcessGameProc", 0xCF, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownUninitGame",
    xref("Window_ProcessGameProc", 0xAD, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownDirectInputRelease",
    xref("Window_ProcessGameProc", 0x9C, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownKillGame",
    xref("Window_ProcessGameProc", 0x88, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownUninitGameInterface",
    xref("Window_ProcessGameProc", 0x77, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownUnloadData",
    xref("Window_ProcessGameProc", 0x55, 0x1),
)
stable.data(
    "Window_ProcessGameProc_ShutdownBegin",
    xref("Window_ProcessGameProc", 0x49, 0x1),
)
stable.data("Window_RunWinMain_RequiresNT", xref("Window_RunWinMain", 0x75, 0x1))
stable.data("PKG_LocatePackagePath_CantFindPKG", xref("PKG_LocatePackagePath", 0x141, 0x1))
stable.data("PKG_LocatePackagePath_SetupPath", xref("PKG_LocatePackagePath", 0xD5, 0x1))
stable.data("PKG_LocatePackagePath_SearchPattern", xref("PKG_LocatePackagePath", 0xAB, 0x1))
stable.data(
    "PKG_LocatePackagePath_DalmsSetupPath", xref("PKG_LocatePackagePath", 0x93, 0x1)
)
stable.data("PKG_LocatePackagePath_DriveLetter", xref("PKG_LocatePackagePath", 0x60, 0x1))
stable.data(
    "PKG_LocatePackagePath_PcdogsPKG",
    xref("PKG_LocatePackagePath", 0x45, 0x1, required=Required.EN),
    xref("PKG_LocatePackagePath", 0x4A, 0x1, required=Required.EU_SC),
)
stable.data("Math_GenerateRandom_Seed", xref("Math_GenerateRandom", 0x0, 0x2))
stable.data(
    "Script_ActorOpcodeDispatchTable",
    xref("Script_OpRunWithActor", 0xAE, 0x3),
    type="Script_CommandCallback[45]",
    doc="Read-only 45-slot actor opcode table with a NULL first slot. Dispatchers use raw byte indexes without range or null checks.",
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Player_ProcessMovement_InputLandingFlagsPtr",
    xref("Player_ProcessMovement", 0x4A, 0x1),
)
stable.data("String_GetByIndex_NoString", xref("String_GetByIndex", 0x2DD, 0x1))
stable.data("String_GetByIndex_TwoStrings", xref("String_GetByIndex", 0x1AC, 0x1))
stable.data(
    "Graphics_AdjustLevelScale_ListState",
    xref("Graphics_AdjustLevelScale", 0x65, 0x2),
    doc="Data pointer to active Graphics_ListState used when Graphics_AdjustLevelScale writes "
    "dynamic level scale.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Mem_MallocWithRetry_DebugKilledByPlayer", xref("Mem_MallocWithRetry", 0x60, 0x1)
)
stable.data(
    "Mem_MallocWithRetry_UIConfirmStopGame", xref("Mem_MallocWithRetry", 0x4E, 0x1)
)
stable.data("Mem_MallocWithRetry_OutOfMemory", xref("Mem_MallocWithRetry", 0x31, 0x1))
stable.data("Mem_MallocWithRetry_Failed", xref("Mem_MallocWithRetry", 0x1A, 0x1))
stable.data(
    "UI_ShowConfirmDialog_ProgrammerMessage", xref("UI_ShowConfirmDialog", 0x5A, 0x1)
)
stable.data("Mem_AllocateHandle_AllocDebug", xref("Mem_AllocateHandle", 0x78, 0x1))
stable.data("Mem_AllocateHandle_AllocFailed", xref("Mem_AllocateHandle", 0x3B, 0x1))
stable.data("Mem_AllocateHandle_AllocPrefix", xref("Mem_AllocateHandle", 0x22, 0x1))
stable.data("Mem_AllocateHandle_OutOfExtents", xref("Mem_AllocateHandle", 0xC, 0x1))
stable.data(
    "Mem_FreeAllExtents_LeakUnreleasedExtent", xref("Mem_FreeAllExtents", 0x16, 0x1)
)
stable.data("Mem_ReleaseHandle_LeakInvalidExtent", xref("Mem_ReleaseHandle", 0x9E, 0x1))
stable.data("Mem_ReleaseHandle_FreeDebug", xref("Mem_ReleaseHandle", 0x4C, 0x1))
stable.data("Mem_ReleaseHandle_LeakUnallocated", xref("Mem_ReleaseHandle", 0x29, 0x1))
stable.data(
    "Timer_GetElapsedTickCount_GameStartTime",
    xref("Timer_GetElapsedTickCount", 0x0, 0x1),
)
stable.data(
    "Input_IsKeyPressed_MappingTableSize", xref("Input_IsKeyPressed", 0x2C, 0x3)
)
stable.data("Input_IsKeyPressed_MappingTablePtr", xref("Input_IsKeyPressed", 0x0, 0x1))
stable.data("String_ParseInt_CharTypeTable", xref("String_ParseInt", 0x23, 0x2))
stable.data(
    "String_FormatFloat_DecimalPointChar", xref("String_FormatFloat", 0x40, 0x2)
)
stable.data(
    "Graphics_RenderPolygonCallback",
    xref("Scene_TraverseNodeTree", 0x401, 0x2),
    type="Graphics_RenderPolygonCallbackType",
    doc=(
        "Engine-managed polygon callback installed as either Graphics_RenderPolygonBatch or its "
        "forwarding far-distance wrapper. The first argument is a shared Actor/static-scene render "
        "owner prefix, represented conservatively as void*."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Graphics_RenderMeshNode_CapabilityFlags",
    xref("Graphics_RenderMeshNode", 0xF76, 0x1),
)
stable.data(
    "Camera_UpdateFollow_CurrentCameraCell",
    xref("Camera_UpdateFollow", 0x2D0, 0x2),
    doc=(
        "Camera_UpdateFollow transient current camera cell. This camera-owned runtime state is "
        "used across level/entity transitions; current_level_data entity slots are the "
        "actor/entity enumeration source."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Script_OpPauseToggle_ScreenBorderStateFlag",
    xref("Script_OpPauseToggle", 0x66, 0x1),
)
stable.data(
    "EntityNavigationWorkList_ActiveBufferPtr",
    xref("Entity_UpdateVisibilityAndSpawn", 0xE8, 0x1),
    xref("Camera_UpdateFollow", 0x86D, 0x1),
    type="EntityNavigationWorkListBuffer*",
    doc=(
        "Active work-buffer pointer promoted from the prior staging pass. Its four entity slots and "
        "100 navigation commands are consumed until the next Camera_UpdateFollow swap."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data("Script_OpPauseToggle_FadeCounter", xref("Script_OpPauseToggle", 0x8D, 0x1))
stable.data(
    "Audio_TriggerMusicTransition_Volume",
    xref("Audio_TriggerMusicTransition", 0x99, 0x2),
)
stable.data(
    "EntityNavigationWorkList_StagingBufferPtr",
    xref("Camera_UpdateFollow", 0x863, 0x1),
    type="EntityNavigationWorkListBuffer*",
    doc="Next-pass work-buffer pointer. Camera_UpdateFollow selects the alternate buffer and clears both counts.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data("Actor_TracePath_WorkBuffer", xref("Actor_TracePath", 0x12, 0x1))
stable.data(
    "Actor_TracePath_NullActorState2",
    xref("Actor_TracePath", 0x27, 0x2),
    doc=(
        "Actor_TracePath byte/cell used when the source actor is NULL while "
        "path_trace_work_buffer, current_level_data entity slots, and "
        "EntityNavigationWorkList_ActiveBufferPtr are active."
    ),
)
stable.data("Actor_TracePath_NullActorState1", xref("Actor_TracePath", 0x1D, 0x2))
stable.data("Actor_TracePath_NullActorState3", xref("Actor_TracePath", 0x17, 0x2))
stable.data(
    "Camera_UpdateFollow_TransitionTrigger",
    xref("Camera_UpdateFollow", 0x29C, 0x1),
)
stable.data(
    "Script_OpPauseToggle_CameraTransitionCountdown",
    xref("Script_OpPauseToggle", 0x17B, 0x2),
)
stable.data(
    "Audio_MusicFadeFrameCount",
    xref("Audio_TriggerMusicTransition", 0x66, 0x2),
    xref("Audio_InitializeSystem", 0x117, 0x2),
)
stable.data(
    "Audio_TriggerMusicTransition_FadeStartFrame",
    xref("Audio_TriggerMusicTransition", 0xC7, 0x1),
)
stable.data(
    "Audio_TriggerMusicTransition_EndFrame",
    xref("Audio_TriggerMusicTransition", 0xE2, 0x2),
)
stable.data(
    "Audio_TriggerMusicTransition_Pending",
    xref("Audio_TriggerMusicTransition", 0xCF, 0x2),
)
stable.data(
    "EntityNavigationWorkList_BackingBuffer",
    xref("Camera_UpdateFollow", 0x872, 0x1),
    doc=(
        "Entity/navigation work-list backing buffer. Camera_UpdateFollow alternates it with the "
        "peer backing buffer, while the active/staging pointer cells name the current roles."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Script_OpPauseToggle_LevelTransitionFlag",
    xref("Script_OpPauseToggle", 0x174, 0x2),
    xref("Script_OpPauseToggle", 0x18F, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_TransitionStartFrame",
    xref("Level_InitializeActorSystem", 0xC3, 0x1),
)
stable.data(
    "Game_TransitionAndSettingsFlags",
    xref("Camera_UpdateFollow", 0x7BA, 0x1),
    xref("Settings_SetSoundEnabled", 0xC, 0x1),
    doc=(
        "Shared game-state/settings dword used in camera transition and sound-setting paths. The "
        "broad symbol covers multiple settings and transition bits."
    ),
)
stable.data(
    "Level_InitializeActorSystem_TransitionEndFrame",
    xref("Level_InitializeActorSystem", 0xD9, 0x1),
)
stable.data(
    "Camera_UpdateFollow_TransitionFrameCounter",
    xref("Camera_UpdateFollow", 0x7BF, 0x2),
)
stable.data(
    "Camera_CalculateFollowAngles_PreviousYaw",
    xref("Camera_CalculateFollowAngles", 0x97, 0x3),
)
stable.data(
    "Camera_InterpolateTransition_Paused",
    xref("Camera_InterpolateTransition", 0x16, 0x1),
)
stable.data(
    "Script_OpPauseToggle_AnimationTimerState", xref("Script_OpPauseToggle", 0xF7, 0x2)
)
stable.data(
    "Animation_QueueStateChange_AnimQueuedStateChange",
    xref("Animation_QueueStateChange", 0x9, 0x3),
    type="int32_t[10]",
    doc="Global int32_t[10] queue followed by its count at 0x457210. Append 11 aliases and loses its state.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Animation_QueueStateChange_Count",
    xref("Animation_QueueStateChange", 0x0, 0x1),
    type="int32_t",
    doc="Global pending count at 0x457210. No producer enforces <=10 and no independent reset exists.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Video_InitializeAVIPlayer_Initialized",
    xref("Video_InitializeAVIPlayer", 0x20, 0x1),
    xref("Video_InitializeAVIPlayer", 0x36, 0x1),
)
stable.data(
    "Video_InitializeAVIPlayer_WindowHandle",
    xref("Video_InitializeAVIPlayer", 0x31, 0x1),
)
stable.data(
    "Video_InitializeAVIPlayer_MovieCounter",
    xref("Video_InitializeAVIPlayer", 0xB, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos1X",
    xref("Checkers_UpdateStateMachine", 0xE6, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos1Y",
    xref("Checkers_UpdateStateMachine", 0xEC, 0x1),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos1Z",
    xref("Checkers_UpdateStateMachine", 0xF1, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos2X",
    xref("Checkers_UpdateStateMachine", 0xA1, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos2Y",
    xref("Checkers_UpdateStateMachine", 0xBD, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_CameraPos2Z",
    xref("Checkers_UpdateStateMachine", 0xC3, 0x1),
)
stable.data(
    "Checkers_UpdateStateMachine_SelectedCol1",
    xref("Checkers_UpdateStateMachine", 0x42, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_SelectedRow1",
    xref("Checkers_UpdateStateMachine", 0x3C, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_SelectedCol2",
    xref("Checkers_UpdateStateMachine", 0x36, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_SelectedRow2",
    xref("Checkers_UpdateStateMachine", 0x30, 0x2),
)
stable.data(
    "Checkers_GameBoard",
    xref("Checkers_UpdateStateMachine", 0xE1, 0x1),
    type="Checkers_Board",
    doc="Checkers board passed to board initialization, move generation, move execution, and AI search.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Checkers_UpdateStateMachine_AINodeCounter",
    xref("Checkers_UpdateStateMachine", 0x5BE, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_MoveResult",
    xref("Checkers_UpdateStateMachine", 0x263, 0x1),
)
stable.data(
    "Checkers_UpdateStateMachine_AIMoveFromCol",
    xref("Checkers_UpdateStateMachine", 0x5B2, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_AIMoveFromRow",
    xref("Checkers_UpdateStateMachine", 0x5AC, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_AIMoveToCol",
    xref("Checkers_UpdateStateMachine", 0x5A6, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_AIMoveToRow",
    xref("Checkers_UpdateStateMachine", 0x59D, 0x2),
)
stable.data(
    "g_checkersAISearchJumpBuffer",
    xref("Checkers_UpdateStateMachine", 0x587, 0x1),
    type="uint32_t[0x10]",
    doc="Setjmp/longjmp buffer that aborts or pauses checkers AI search during input and render polling.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Checkers_UpdateStateMachine_AIThinkTimeout",
    xref("Checkers_UpdateStateMachine", 0x5B8, 0x2),
)
stable.data("Graphics_DrawQuad_Vertex0", xref("Graphics_DrawQuad", 0x3CC, 0x6))
stable.data("Graphics_DrawQuad_Vertex0U", xref("Graphics_DrawQuad", 0x288, 0x2))
stable.data("Graphics_DrawQuad_Vertex0V", xref("Graphics_DrawQuad", 0x29D, 0x2))
stable.data("Graphics_DrawQuad_Vertex1", xref("Graphics_DrawQuad", 0x3D6, 0x6))
stable.data("Graphics_DrawQuad_Vertex1U", xref("Graphics_DrawQuad", 0x2B4, 0x2))
stable.data("Graphics_DrawQuad_Vertex1V", xref("Graphics_DrawQuad", 0x2C9, 0x2))
stable.data("Graphics_DrawQuad_Vertex2", xref("Graphics_DrawQuad", 0x3E0, 0x6))
stable.data("Graphics_DrawQuad_Vertex2U", xref("Graphics_DrawQuad", 0x2EC, 0x2))
stable.data("Graphics_DrawQuad_Vertex2V", xref("Graphics_DrawQuad", 0x304, 0x2))
stable.data("Graphics_DrawQuad_Vertex3", xref("Graphics_DrawQuad", 0x3EA, 0x6))
stable.data("Graphics_DrawQuad_Vertex3U", xref("Graphics_DrawQuad", 0x35A, 0x2))
stable.data("Graphics_DrawQuad_Vertex3V", xref("Graphics_DrawQuad", 0x372, 0x2))
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DeviceInit0",
    xref("D3D_InitDirectDrawAndDirect3D", 0x8A, 0x2),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DeviceInit1",
    xref("D3D_InitDirectDrawAndDirect3D", 0x85, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DeviceInit2",
    xref("D3D_InitDirectDrawAndDirect3D", 0x90, 0x2),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DeviceInit3",
    xref("D3D_InitDirectDrawAndDirect3D", 0x68, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBuffer",
    xref("Graphics_ClipAndDrawPolygon", 0x35B, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferG",
    xref("Graphics_ClipAndDrawPolygon", 0x1D9, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferB1",
    xref("Graphics_ClipAndDrawPolygon", 0x367, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferB0",
    xref("Graphics_ClipAndDrawPolygon", 0x36C, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferR0",
    xref("Graphics_ClipAndDrawPolygon", 0x38A, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferG1",
    xref("Graphics_ClipAndDrawPolygon", 0x3A1, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferR1",
    xref("Graphics_ClipAndDrawPolygon", 0x37E, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferB",
    xref("Graphics_ClipAndDrawPolygon", 0x396, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferB2",
    xref("Graphics_ClipAndDrawPolygon", 0x372, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferG2",
    xref("Graphics_ClipAndDrawPolygon", 0x440, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferR2",
    xref("Graphics_ClipAndDrawPolygon", 0x453, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexColorBufferG3",
    xref("Graphics_ClipAndDrawPolygon", 0x47C, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexWorkBuffer",
    xref("Graphics_ClipAndDrawPolygon", 0x40E, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexWorkBufferV1Base",
    xref("Graphics_ClipAndDrawPolygon", 0x41F, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexWorkBufferV1Alt",
    xref("Graphics_ClipAndDrawPolygon", 0x4A9, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipQuadSrcU",
    xref("Graphics_ClipAndDrawPolygon", 0x33C, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipQuadSrcV",
    xref("Graphics_ClipAndDrawPolygon", 0x347, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_VertexWorkBufferV2Base",
    xref("Graphics_ClipAndDrawPolygon", 0x436, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipQuadDstU",
    xref("Graphics_ClipAndDrawPolygon", 0x34F, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipQuadDstV",
    xref("Graphics_ClipAndDrawPolygon", 0x355, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipTempBuffer",
    xref("Graphics_ClipAndDrawPolygon", 0x5A3, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_DriverGUID",
    xref("Graphics_ClipAndDrawPolygon", 0x5EB, 0x1),
    doc="Region 0x457CA4 is dual-use: clip scratch for Graphics_ClipAndDrawPolygon and part of the D3D driver-summary records.",
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_SelectedDeviceGuidPtr",
    xref("D3D_InitDirectDrawAndDirect3D", 0x1C5, 0x3),
    type="Win32_GUID*",
    doc="Pointer to the GUID of the DirectDraw device selected during D3D initialization.",
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawEnumDriverData",
    xref("D3D_InitializeDirectDraw", 0x12B, 0x3),
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitState0",
    xref("D3D_InitializeDirectDraw", 0xE7, 0x3),
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitState1",
    xref("D3D_InitializeDirectDraw", 0x16B, 0x3),
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitParam0",
    xref("D3D_InitializeDirectDraw", 0x89, 0x2),
    doc="Driver-summary slot[1] field; the record shares storage with the graphics clip scratch region.",
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitScratchParam1",
    xref("D3D_InitializeDirectDraw", 0x9C, 0x2),
    doc="Generic DirectDraw-init local parameter cell used during setup.",
    unstable=True,
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitParam2",
    xref("D3D_InitializeDirectDraw", 0xA5, 0x2),
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitParam3",
    xref("D3D_InitializeDirectDraw", 0xB3, 0x2),
)
stable.data(
    "D3D_InitializeDirectDraw_DDrawInitParam4",
    xref("D3D_InitializeDirectDraw", 0xAE, 0x1),
    doc="Driver-summary slot[1] field; the record shares storage with the graphics clip scratch region.",
)
stable.data(
    "D3D_InitializeDirectDraw_GraphicsDriverInitialized",
    xref("D3D_InitializeDirectDraw", 0x73, 0x2),
)
stable.data(
    "Graphics_LoadAndUploadTexture_PKGLoadingScreenTexture",
    xref("Graphics_LoadAndUploadTexture", 0x19, 0x1),
    type="Material_BlendTextureSet",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipInputBuffer",
    xref("Graphics_ClipAndDrawPolygon", 0x413, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipInputBufferY",
    xref("Graphics_ClipAndDrawPolygon", 0x424, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipInputBufferV2",
    xref("Graphics_ClipAndDrawPolygon", 0x43B, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_RenderClipMinX",
    xref("Graphics_ClipAndDrawPolygon", 0x306, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_RenderClipMinY",
    xref("Graphics_ClipAndDrawPolygon", 0x320, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipInputBufferVertex2W",
    xref("Graphics_ClipAndDrawPolygon", 0x30C, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_RenderClipMaxX",
    xref("Graphics_ClipAndDrawPolygon", 0x326, 0x2),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsClientRectLeft",
    xref("D3D_InitDirectDrawAndDirect3D", 0x7F, 0x1),
    doc="Client-rectangle left coordinate local value used during DirectDraw/Direct3D "
    "initialization.",
    unstable=True,
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsClientRectTop",
    xref("D3D_InitDirectDrawAndDirect3D", 0x9C, 0x1),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsClientRectRight",
    xref("D3D_InitDirectDrawAndDirect3D", 0xA1, 0x2),
    doc="Client-rectangle right coordinate local value used during DirectDraw/Direct3D "
    "initialization.",
    unstable=True,
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsClientRectBottom",
    xref("D3D_InitDirectDrawAndDirect3D", 0x6E, 0x2),
)
stable.data(
    "Material_ReleaseTextureArray_GraphicsActiveTextureCount",
    xref("Material_ReleaseTextureArray", 0x17, 0x2),
    type="uint32_t",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Graphics_DrawQuad_UntexturedVertexBuffer",
    xref("Graphics_DrawQuad", 0x4ED, 0x1),
)
stable.data(
    "Graphics_DrawQuad_UntexturedVertexBufferVertex1",
    xref("Graphics_DrawQuad", 0x512, 0x6),
)
stable.data(
    "Graphics_DrawQuad_UntexturedVertexBufferVertex2",
    xref("Graphics_DrawQuad", 0x52D, 0x6),
)
stable.data(
    "Graphics_DrawQuad_UntexturedVertexBufferVertex3",
    xref("Graphics_DrawQuad", 0x523, 0x6),
)
stable.data(
    "Camera_SetupProjection_MatrixXScale",
    xref("Camera_SetupProjection", 0x40, 0x1),
)
stable.data(
    "Camera_SetupProjection_MatrixYScale",
    xref("Camera_SetupProjection", 0x8C, 0x2),
)
stable.data(
    "Camera_SetupProjection_MatrixNearW",
    xref("Camera_SetupProjection", 0x6A, 0x2),
)
stable.data(
    "Camera_SetupProjection_MatrixOne",
    xref("Camera_SetupProjection", 0x78, 0x2),
)
stable.data(
    "Camera_SetupProjection_MatrixFarW",
    xref("Camera_SetupProjection", 0x82, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_TransformedVertices",
    xref("Graphics_ClipAndDrawPolygon", 0x806, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_TransformedVerticesY",
    xref("Graphics_ClipAndDrawPolygon", 0x640, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipOutputBuffer",
    xref("Graphics_ClipAndDrawPolygon", 0x5A8, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ClipInputAttributes",
    xref("Graphics_ClipAndDrawPolygon", 0x378, 0x2),
    type="Graphics_ClipAttribute[3]",
    doc="Three-entry Graphics_ClipAttribute input array consumed by Graphics_ClipAndDrawPolygon; previously misread as a clip-plane count.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff0",
    xref("Graphics_ClipAndDrawPolygon", 0x384, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff1",
    xref("Graphics_ClipAndDrawPolygon", 0x39C, 0x1),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff2",
    xref("Graphics_ClipAndDrawPolygon", 0x3B0, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff3",
    xref("Graphics_ClipAndDrawPolygon", 0x3B6, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff4",
    xref("Graphics_ClipAndDrawPolygon", 0x3BC, 0x1),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff5",
    xref("Graphics_ClipAndDrawPolygon", 0x390, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff6",
    xref("Graphics_ClipAndDrawPolygon", 0x3A6, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CameraClipPlaneCoeff7",
    xref("Graphics_ClipAndDrawPolygon", 0x3C1, 0x2),
    doc="Scalar camera clip-plane coefficient entry used by Graphics_ClipAndDrawPolygon.",
    unstable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_D3DVertexBufferScreenX",
    xref("Graphics_ClipAndDrawPolygon", 0x7E6, 0x1),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_D3DVertexBufferScreenY",
    xref("Graphics_ClipAndDrawPolygon", 0x79D, 0x1),
)
stable.data(
    "Graphics_ClipPolygonByCameraPyramid_TempVertexBuffer",
    xref("Graphics_ClipPolygonByCameraPyramid", 0x18, 0x1),
)
stable.data(
    "Graphics_ClipPolygonByCameraPyramid_TempUVBuffer",
    xref("Graphics_ClipPolygonByCameraPyramid", 0x1D, 0x1),
)
stable.data(
    "Graphics_CurrentBoundTextureSurface",
    xref("Material_ReleaseTextureArray", 0x30, 0x1),
    type="DDraw_IDirectDrawSurface7*",
    doc="Shared graphics current-bound texture surface cleared by material texture-array release paths.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "D3D_SetBlendMode_GraphicsCurrentBlendMode",
    xref("D3D_SetBlendMode", 0x98, 0x1),
    type="int32_t",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("Graphics_DrawQuad_VertexPtrs", xref("Graphics_DrawQuad", 0x6C, 0x2))
stable.data("Graphics_DrawQuad_Vertex1Ptr", xref("Graphics_DrawQuad", 0x3D6, 0x2))
stable.data("Graphics_DrawQuad_Vertex2Ptr", xref("Graphics_DrawQuad", 0x3E0, 0x2))
stable.data("Graphics_DrawQuad_Vertex3Ptr", xref("Graphics_DrawQuad", 0x3EA, 0x2))
stable.data(
    "D3D_InitDirectDrawAndDirect3D_GraphicsSelectedDriverIndex",
    xref("D3D_InitDirectDrawAndDirect3D", 0x1B1, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsTexFormatIsSoftware",
    xref("D3D_CreateTextureSurface", 0x183, 0x2),
)
stable.data(
    "Graphics_LoadAndUploadTexture_TexNeedsAlpha",
    xref("Graphics_LoadAndUploadTexture", 0xA, 0x2),
    type="uint8_t",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "D3D_SetGammaRamp_GraphicsGammaControl",
    xref("D3D_SetGammaRamp", 0x6E, 0x2),
    doc="Shared pointer used locally as IDirectDrawGammaControl*. Its data-row type remains weak because the global lies outside the owned range.",
)
stable.data(
    "D3D_EnumDeviceCallback_DDrawEnumDeviceList",
    xref("D3D_EnumDeviceCallback", 0x3A, 0x3),
)
stable.data(
    "D3D_EnumerateDirectDrawDevices_UnusedDeviceAcceptCallback",
    xref("D3D_EnumerateDirectDrawDevices", 0xE, 0x1),
    doc=(
        "Stores the caller-provided device-accept callback at 0x4943D0. The global has no read or "
        "call xrefs in this binary, so it does not filter enumeration."
    ),
)
stable.data(
    "D3D_EnumerateDirectDrawDevices_GraphicsEnumDeviceCount",
    xref("D3D_EnumerateDirectDrawDevices", 0x1A, 0x1),
)
stable.data(
    "D3D_EnumerateDirectDrawDevices_GraphicsAcceptedDeviceCount",
    xref("D3D_EnumerateDirectDrawDevices", 0x43, 0x1),
)
stable.data(
    "DInput_InitializeJoystickInput_Interface",
    xref("DInput_InitializeJoystickInput", 0x1C, 0x1),
    type="DInput_IDirectInputA*",
    doc="DirectInput interface created by DInput_CreateInterface; used for joystick enumeration/creation and released by DInput_ReleaseResources.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_JoystickState",
    xref("Input_ReadGamepad", 0x20, 0x1),
    type="Input_JoystickState*",
    doc=(
        "Current DirectInput joystick state buffer read by Input_ReadGamepad and released "
        "during input shutdown. This is a frame-local sample source; Input_GetJoystickAxis* "
        "near Input_ReadGamepad gives live analog freshness."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "DInput_InitializeJoystickInput_Device",
    xref("DInput_InitializeJoystickInput", 0x74, 0x1),
    type="DInput_IDirectInputDevice2A*",
    doc="DirectInput joystick device created by DInput_CreateConfiguredJoystickDevice; acquired, polled, and released by input shutdown.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "DInput_CreateConfiguredJoystickDevice_ConstantForceEffect",
    xref("DInput_CreateConfiguredJoystickDevice", 0x172, 0x1),
    type="void*",
    doc="Shared force feedback effect pointer set after CreateEffect succeeds. Capability discovery can remain true while this pointer is NULL.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Video_CloseMovieFile_Handle",
    xref("Video_CloseMovieFile", 0x0, 0x1),
    type="int32_t",
    doc="Winplay/RPL movie handle initialized by Movie_InitMovie and shut down by Movie_ShutdownMovie.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Video_CloseMovieFile_SurfaceHandle",
    xref("Video_CloseMovieFile", 0x1A, 0x1),
    type="int32_t",
    doc="Winplay video surface handle initialized by Movie_InitVideo, prepared for playback, and "
    "shut down by Movie_ShutdownVideo.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Video_CloseMovieFile_SoundHandle",
    xref("Video_CloseMovieFile", 0x10, 0x1),
    type="int32_t",
    doc="Winplay sound handle initialized by Movie_InitSound and passed into movie playback.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Graphics_RenderPolygonBatch_Flags",
    xref("Graphics_RenderPolygonBatch", 0x3D9, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_MaxPrimitivesPerBatchD3D",
    xref("Graphics_RenderPolygonBatch", 0x3CD, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_ProjectionScratch1",
    xref("Graphics_RenderPolygonBatch", 0x20E, 0x3),
    doc="Projection/render local value in the polygon batching path.",
    unstable=True,
)
stable.data(
    "Graphics_RenderPolygonBatch_ProjectionScratch2",
    xref("Graphics_RenderPolygonBatch", 0x207, 0x3),
    doc="Projection/render local value in the polygon batching path.",
    unstable=True,
)
stable.data(
    "Graphics_RenderPolygonBatch_ProjectionScratch3",
    xref("Graphics_RenderPolygonBatch", 0x200, 0x3),
    doc="Projection/render local value in the polygon batching path.",
    unstable=True,
)
stable.data(
    "Graphics_RenderPolygonBatch_VertexBase",
    xref("Graphics_RenderPolygonBatch", 0x443, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_PolyBatchVertexCount",
    xref("Graphics_RenderPolygonBatch", 0x44B, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_PolyBatchTriCount",
    xref("Graphics_RenderPolygonBatch", 0x458, 0x2),
)
stable.data(
    "Mesh_CalculateVertexNormals_NormalAccumulatorX",
    xref("Mesh_CalculateVertexNormals", 0x5F6, 0x4),
)
stable.data(
    "Scene_FinalizeNodeRender_VertexNormalAccumY",
    xref("Scene_FinalizeNodeRender", 0x306, 0x1),
)
stable.data(
    "Mesh_CalculateVertexNormals_NormalAccumZ",
    xref("Mesh_CalculateVertexNormals", 0x612, 0x4),
)
stable.data(
    "Mesh_CalculateVertexNormals_NormalCount",
    xref("Mesh_CalculateVertexNormals", 0x5E8, 0x4),
)
stable.data(
    "Graphics_RenderPolygonBatch_PolyBatchTextureState",
    xref("Graphics_RenderPolygonBatch", 0xC2, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_BackfaceVertex1Ptr",
    xref("Graphics_RenderPolygonBatch", 0xA0, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_BackfaceVertex2Ptr",
    xref("Graphics_RenderPolygonBatch", 0xB2, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex1ScreenXY",
    xref("Graphics_RenderPolygonBatch", 0x722, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex1ViewZ",
    xref("Graphics_RenderPolygonBatch", 0x6BE, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex2ScreenXY",
    xref("Graphics_RenderPolygonBatch", 0x889, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex2ViewZ",
    xref("Graphics_RenderPolygonBatch", 0x825, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex3ScreenXY",
    xref("Graphics_RenderPolygonBatch", 0x9F0, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex3ViewZ",
    xref("Graphics_RenderPolygonBatch", 0x98C, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex4ScreenX",
    xref("Graphics_RenderPolygonBatch", 0xA24, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex4ScreenY",
    xref("Graphics_RenderPolygonBatch", 0x1117, 0x2),
)
stable.data(
    "Graphics_RenderPolygonBatch_TransformedVertex4ViewZ",
    xref("Graphics_RenderPolygonBatch", 0xB0C, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_PolyBatchRenderFlags",
    xref("Graphics_RenderPolygonBatch", 0xC7, 0x1),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteVertexFlags",
    xref("Bone_TransformWeightedVerticesForRender", 0x5D3, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteLastPositionX",
    xref("Bone_TransformWeightedVerticesForRender", 0x300, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteLastPositionY",
    xref("Bone_TransformWeightedVerticesForRender", 0x309, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteLastPositionZ",
    xref("Bone_TransformWeightedVerticesForRender", 0x312, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteAnimFrame",
    xref("Bone_TransformWeightedVerticesForRender", 0x576, 0x4),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteAnimFlags",
    xref("Bone_TransformWeightedVerticesForRender", 0x57E, 0x4),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_SpriteAnimState",
    xref("Bone_TransformWeightedVerticesForRender", 0x47F, 0x3),
)
stable.data(
    "UI_PuppyCounterAnimState",
    xref("Menu_ClearTransitionFlags", 0x17, 0x3),
)
stable.data(
    "g_dynamicStringSlot0",
    xref("String_GetRuntimeByNegativeIndex", 0x4, 0x1),
    type="char*",
    doc="Dynamic string pointer selected by the shipped type-2 text token -1. Levels 6, 28, and 31 contain four such records in total.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Menu_RenderControlsConfiguration_StringFormatBuffer",
    xref("Menu_RenderControlsConfiguration", 0x2D, 0x1),
)
stable.data(
    "Save_GameLevelCompletion_HighestWorld",
    xref("Save_SaveGameLevelCompletion", 0xBC, 0x2),
)
stable.data(
    "Menu_RenderConfirmPrompt_FrameCounter",
    xref("Menu_RenderConfirmPrompt", 0x20, 0x2),
)
stable.data(
    "Menu_PauseTransitionTimer",
    xref("Menu_UpdatePauseMenu", 0xD2, 0x3),
    xref("Menu_ResetState", 0x2, 0x1),
)
stable.data(
    "Save_VolumeSettings",
    xref("Settings_GetSfxVolume", 0x0, 0x1),
    type="Save_VolumeSettings",
    doc="Persisted SFX/music volume pair read and written by the Settings volume accessors.",
)
stable.data(
    "Settings_RumbleSuppressFlag", xref("Settings_SetRumbleSuppressFlag", 0x4, 0x1)
)
stable.data(
    "Settings_Language",
    xref("Settings_SetLanguage", 0x4, 0x1),
    doc="Persisted language ID byte (Config_GameSettings.language).",
)
stable.data(
    "Menu_HandleOptionsLogic_OptionValueScratch",
    xref("Menu_HandleOptionsLogic", 0x95, 0x1),
    doc="Options-menu local value used by Menu_HandleOptionsLogic.",
    unstable=True,
)
stable.data(
    "UI_BoneAndLivesCounterAnimState",
    xref("Menu_ClearTransitionFlags", 0x7, 0x3),
    type="uint32_t",
    doc="Packed HUD counter animation state: low word drives bone counter animation, high word drives lives icon animation.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Save_LoadGameState_FileBuffer",
    xref("Save_LoadGameState", 0xD, 0x1),
    type="Save_GameData",
    doc="Save-file header followed by save-slot payloads; passed to Save_InitGameOperation with total size 0x1dc.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Save_LoadGameState_FileGameState",
    xref("Save_LoadGameState", 0x70, 0x2),
    type="int32_t",
    doc="Save-file header game_state dword restored to game_state by Save_LoadGameState.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_LoadGameState_FileGameSettings",
    xref("Save_LoadGameState", 0x76, 0x2),
    type="int32_t",
    doc="Save-file header game_settings dword restored to game_settings by Save_LoadGameState.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_LoadGameState_FilePlayerLives",
    xref("Save_LoadGameState", 0x6B, 0x1),
    type="int32_t",
    doc="Save-file header player-lives dword restored to player_lives by Save_LoadGameState.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleSaveGameLogic_Buffer",
    xref("Menu_HandleSaveGameLogic", 0x318, 0x2),
    type="Save_GameSlot",
    doc=(
        "First save-slot record immediately after the Save_GameData header. Native file operations cover storage, consistent with five slot-sized records."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Menu_HandleSaveGameLogic_SlotValidFlags",
    xref("Menu_HandleSaveGameLogic", 0x13C, 0x3),
    type="uint8_t",
    doc="First save-slot valid byte at Save_GameSlot+1; Save_SaveGameToSlot sets saveSlots[slotIndex].is_valid to 1.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_BackupData",
    xref("Menu_HandleOptionsLogic", 0xA9, 0x1),
    type="uint8_t[0x6c]",
    doc="Editable options/config backup block copied before controls remapping and passed to Config_SaveSettingsToINI.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleOptionsLogic_InputKeyboardMappings",
    xref("Menu_HandleOptionsLogic", 0x425, 0x3),
    type="int32_t",
    doc=(
        "First entry/base of the player-1/keyboard binding range inside options_menu_backup_data. Options UI compares 11 keyboard-side entries."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_InputGamepadMappings",
    xref("Menu_HandleOptionsLogic", 0x9A, 0x1),
    type="int32_t",
    doc=(
        "First entry/base of the player-2/gamepad binding range inside options_menu_backup_data. Options UI uses 10 gamepad-side entries."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Shared_LoadCommonResources_PKGResourceDataBuffer",
    xref("Shared_LoadCommonResources", 0x4D, 0x1),
    type="PKG_SharedPackage*",
    doc="Resource/common-data buffer pointer populated by Shared_LoadCommonResources.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("Menu_ProcessMenuState_State", xref("Menu_ProcessMenuState", 0xA0, 0x3))
stable.data(
    "Menu_Selection",
    xref("Menu_LoadLevelProgressState", 0x44, 0x2),
    xref("Menu_ProcessMenuState", 0xD2, 0x1),
)
stable.data(
    "Menu_ProcessMenuState_SkipBackgroundRender",
    xref("Menu_ProcessMenuState", 0x250, 0x1),
)
stable.data(
    "Menu_ProcessMenuState_DisplayMenuFlags", xref("Menu_ProcessMenuState", 0x235, 0x1)
)
stable.data(
    "Menu_ProcessMenuState_TransitionDelay",
    xref("Menu_ProcessMenuState", 0x6F, 0x1),
)
stable.data(
    "Menu_HandleOptionsLogic_AudioMenuSoundEffect",
    xref("Menu_HandleOptionsLogic", 0x9, 0x2),
)
stable.data(
    "Menu_ProcessMenuState_PostTransitionAction",
    xref("Menu_ProcessMenuState", 0x78, 0x3),
)
stable.data("Menu_ProcessMenuState_Context", xref("Menu_ProcessMenuState", 0x39F, 0x1))
stable.data(
    "Menu_ProcessMenuState_FadeCounter",
    xref("Menu_ProcessMenuState", 0x7BC, 0x2),
    xref("Menu_ProcessMenuState", 0x5B, 0x1),
)
stable.data(
    "Menu_ProcessMenuState_StoredFadeLevel",
    xref("Menu_ProcessMenuState", 0x989, 0x2),
)
stable.data(
    "Menu_ProcessMenuState_OptionIndex",
    xref("Menu_ProcessMenuState", 0xFE, 0x1),
    doc="Menu choice index. Yes/No states use 0=affirm and 1=cancel.",
)
stable.data(
    "Menu_ProcessMenuState_NameEntryActive",
    xref("Menu_ProcessMenuState", 0x84A, 0x2),
)
stable.data(
    "Game_BackupSettings_NameEntryRow",
    xref("Game_BackupSettings", 0x11, 0x1),
    doc="Dual-use scratch cell: name-entry row state during name entry, options-backup slot while Game_BackupSettings runs.",
)
stable.data(
    "Menu_ProcessMenuState_NameEntryColumn",
    xref("Menu_ProcessMenuState", 0x82E, 0x2),
)
stable.data(
    "Game_BackupSettings_SavedGameSettings", xref("Game_BackupSettings", 0x16, 0x2)
)
stable.data(
    "Game_BackupSettings_SavedPlayerLives", xref("Game_BackupSettings", 0x1C, 0x2)
)
stable.data(
    "Menu_UpdatePauseMenu_UILivesCurrentValue", xref("Menu_UpdatePauseMenu", 0x11B, 0x2)
)
stable.data(
    "Level_CheckBonusUnlock_MenuResetFlag", xref("Level_CheckBonusUnlock", 0x26, 0x3)
)
stable.data(
    "Save_GameLevelCompletion_CollectiblesData",
    xref("Save_SaveGameLevelCompletion", 0xAB, 0x3),
    type="Save_GameSlot",
    doc="Active save progress payload copied into save_game_buffer by Save_SaveGameToSlot.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Level_InitializeSaveState_GamePuppyCountBackup",
    xref("Level_InitializeSaveState", 0x7, 0x1),
    type="uint8_t",
    doc="Active Save_GameSlot+2 puppy/life backup byte, seeded to 3 by Save_InitializeGameState and updated by Save_BackupGamePuppyCount.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_InitializeGameState_InitFlag",
    xref("Save_InitializeGameState", 0x16, 0x2),
    type="uint8_t",
    doc="Active Save_GameSlot+3 initialization flag set to 4 by Save_InitializeGameState.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_SetGameComplete_Flag",
    xref("Save_SetGameComplete", 0x4, 0x1),
    type="uint8_t",
    doc="Active Save_GameSlot+4 game-complete flag written by Save_SetGameComplete.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Level_InitializeBonusData_SaveGameBonusData",
    xref("Level_InitializeBonusData", 0x6, 0x4),
    type="uint16_t",
    doc=(
        "First entry of the active Save_GameSlot packed bonus-level parameter table read by "
        "Level_InitializeBonusData."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Save_GameLevelCompletion_BestTime",
    xref("Save_SaveGameLevelCompletion", 0x83, 0x3),
    type="uint16_t",
    doc=(
        "Active Save_GameSlot best time/value for the TOB bonus level, written from menu_items by "
        "Save_SaveGameLevelCompletion."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Level_InitializeBonusData_SaveGameBonusNameEntryBuffer",
    xref("Level_InitializeBonusData", 0x12, 0x3),
    type="uint8_t",
    doc="First byte of the active Save_GameSlot bonus/name-entry payload copied with each save slot.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("UI_UpdateLives_State", xref("UI_UpdateLives", 0x12F, 0x3))
stable.data("UI_UpdateLives_Counter1", xref("UI_UpdateLives", 0x51, 0x1))
stable.data(
    "Shared_LoadCommonResources_TimerState",
    xref("Shared_LoadCommonResources", 0x32, 0x1),
)
stable.data(
    "UI_LivesCounterLastValue",
    xref("Menu_UpdatePauseMenu", 0x17B, 0x3),
    type="int16_t",
    doc="Last/displayed lives value used to restart the HUD lives-counter animation when the live value changes.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Shared_LoadCommonResources_PKGResourceHandle1",
    xref("Shared_LoadCommonResources", 0x19, 0x1),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_SavedWorld0CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x11, 0x1),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_SavedWorld1CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x1B, 0x2),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_SavedWorld2CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x27, 0x2),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_SavedWorld3CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x2D, 0x1),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_SavedWorld4CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x32, 0x2),
)
stable.data(
    "Menu_RenderSaveGame_OperationStep",
    xref("Menu_RenderSaveGame", 0x10D, 0x3),
    type="uint8_t",
    doc="Save/load async operation step byte used by Menu_RenderSaveGame, Menu_HandleSaveGameLogic, and Save_SaveGameToSlot.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_RenderSaveGame_LoadModeFlag",
    xref("Menu_RenderSaveGame", 0x114, 0x1),
    type="uint8_t",
    doc="Save/load mode byte in the save-menu state cluster.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleSaveGameLogic_OperationResult",
    xref("Menu_HandleSaveGameLogic", 0x93, 0x2),
    type="uint8_t",
    doc="Save operation result/status byte written by Menu_HandleSaveGameLogic.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_RenderSaveGame_Active",
    xref("Menu_RenderSaveGame", 0x18, 0x1),
    type="uint8_t",
    doc="Save-menu active byte flag read by Menu_RenderSaveGame.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_RenderSaveGame_OverwriteChoice",
    xref("Menu_RenderSaveGame", 0x134, 0x3),
    type="uint8_t",
    doc="Overwrite-confirmation choice byte. 0=affirm and 1=cancel.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_RenderSaveGame_DialogState",
    xref("Menu_RenderSaveGame", 0xC5, 0x1),
    type="uint8_t",
    doc="Save/load dialog choice byte. 0=affirm, 1=cancel, and 2 is inactive.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleSaveGameLogic_DirtyFlag",
    xref("Menu_HandleSaveGameLogic", 0x1B1, 0x2),
    type="uint8_t",
    doc="Async save dirty/completion byte set while save-slot data is copied and operation 9 is queued.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_RenderSaveGame_State",
    xref("Menu_RenderSaveGame", 0xF9, 0x2),
    type="int32_t",
    doc="Packed save-menu transition/countdown dword; native code accesses individual byte lanes.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("Menu_CheckPauseInput_Delay", xref("Menu_CheckPauseInput", 0x5A, 0x1))
stable.data(
    "Menu_HandleOptionsLogic_InputMenuControlsKeyIndex",
    xref("Menu_HandleOptionsLogic", 0xA4, 0x1),
    type="int16_t",
    doc="First two-byte binding location filled by Options_FindBindingConflict.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_InputMenuControlsButtonIndex",
    xref("Menu_HandleOptionsLogic", 0x9F, 0x1),
    type="int16_t",
    doc="Second two-byte binding location filled by Options_FindBindingConflict.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "g_backupPuppyCount",
    xref("Level_InitializeSaveState", 0xF, 0x1),
    xref("Level_InitializeSaveState", 0x14, 0x2),
    type="uint8_t",
    doc="Runtime puppy-count backup later copied into the active save slot.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Save_GameLevelCompletion_CurrentLevelCompletionBits",
    xref("Save_SaveGameLevelCompletion", 0x10D, 0x2),
)
stable.data(
    "Menu_UpdatePauseMenu_PuppyCounterUIState", xref("Menu_UpdatePauseMenu", 0xF8, 0x3)
)
stable.data(
    "Menu_UpdateInput_Up",
    xref("Menu_UpdateInput", 0x12, 0x1),
    type="int32_t",
    doc=("One-shot menu-input up pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_Down",
    xref("Menu_UpdateInput", 0x17, 0x1),
    type="int32_t",
    doc=("One-shot menu-input down pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_Left",
    xref("Menu_UpdateInput", 0xD, 0x1),
    type="int32_t",
    doc=("One-shot menu-input left pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_Right",
    xref("Menu_UpdateInput", 0x8, 0x1),
    type="int32_t",
    doc=("One-shot menu-input right pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_Confirm",
    xref("Menu_UpdateInput", 0x21, 0x1),
    type="int32_t",
    doc=("One-shot menu-input confirm pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_Cancel",
    xref("Menu_UpdateInput", 0x1C, 0x1),
    type="int32_t",
    doc=("One-shot menu-input cancel pulse dword in the menu input pulse cluster."),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_UpdateInput_UpHeld",
    xref("Menu_UpdateInput", 0xBE, 0x1),
    doc=("Held/debounce up byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_UpdateInput_DownHeld",
    xref("Menu_UpdateInput", 0xA5, 0x2),
    doc=("Held/debounce down byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_UpdateInput_LeftHeld",
    xref("Menu_UpdateInput", 0x35, 0x2),
    doc=("Held/debounce left byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_UpdateInput_RightHeld",
    xref("Menu_UpdateInput", 0x5A, 0x2),
    doc=("Held/debounce right byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_UpdateInput_CancelHeld",
    xref("Menu_UpdateInput", 0x73, 0x2),
    doc=("Held/debounce cancel byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_UpdateInput_ConfirmHeld",
    xref("Menu_UpdateInput", 0x8C, 0x2),
    doc=("Held/debounce confirm byte in the menu input debounce cluster."),
)
stable.data(
    "Menu_HandleOptionsLogic_Column",
    xref("Menu_HandleOptionsLogic", 0xE1, 0x1),
    type="int32_t",
    doc="Options/control-remap column cursor dword used by Menu_HandleOptionsLogic.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Options_MenuSelection",
    xref("Menu_HandleOptionsLogic", 0x3A, 0x2),
    xref("Level_UpdateInterLevelMenu", 0xB5, 0x3),
    type="int32_t",
    doc="Selected options-menu row dword.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_InputMenuButtonRemappingActive",
    xref("Menu_HandleOptionsLogic", 0x203, 0x1),
    xref("Menu_HandleOptionsLogic", 0x202, 0x1, required=Required.EU_SC),
    type="int32_t",
    doc="Control-remapping active/latch dword in the options submenu state cluster.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_State",
    xref("Menu_HandleOptionsLogic", 0x147, 0x2),
    type="int32_t",
    doc="Auxiliary options-menu UI state dword.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleOptionsLogic_UIState5",
    xref("Menu_HandleOptionsLogic", 0x2E, 0x2),
    type="int32_t",
    doc="Auxiliary options-menu UI state dword read by Menu_HandleOptionsLogic.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_SubState",
    xref("Menu_HandleOptionsLogic", 0x0, 0x1),
    type="int32_t",
    doc="Options/control-remap substate dword read at Menu_HandleOptionsLogic entry.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Graphics_DrawSortedLists_Cursor",
    xref("Graphics_DrawSortedLists", 0xA, 0x1),
    type="Graphics_PolygonBatchRecord**",
)
stable.data(
    "Graphics_DrawSortedLists_CurrentRecord",
    xref("Graphics_DrawSortedLists", 0x15, 0x1),
    type="Graphics_PolygonBatchRecord*",
)
stable.data(
    "Graphics_IncrementPassCounter_RenderingFrameCounter",
    xref("Graphics_IncrementPassCounter", 0x0, 0x1),
)
stable.data(
    "PKG_UpdateLoadingScreen_LoadingScreenState",
    xref("PKG_UpdateLoadingScreen", 0x0, 0x1),
)
stable.data(
    "Level_UpdateWorldSelectMenu_State",
    xref("Level_UpdateWorldSelectMenu", 0x1, 0x1),
)
stable.data(
    "Level_UpdateWorldSelectMenu_Slot",
    xref("Level_UpdateWorldSelectMenu", 0x46, 0x3),
)
stable.data(
    "Level_UpdateWorldSelectMenu_FadeCounter",
    xref("Level_UpdateWorldSelectMenu", 0xA, 0x1),
)
stable.data(
    "Input_CheckCheatCodeSequence_LastPressedButton",
    xref("Input_CheckCheatCodeSequence", 0xB, 0x1, access="Write"),
    doc="Write-only latch of the most recently pressed button recorded by Input_CheckCheatCodeSequence; not a sequence-progress counter.",
)
stable.data(
    "PKG_UpdateLoadingScreen_LoadingBlendTexturePtr",
    xref("PKG_UpdateLoadingScreen", 0x43, 0x1),
)
stable.data(
    "PKG_CleanupResourceGameState_LevelHandle",
    xref("PKG_CleanupResourceGameState", 0x0, 0x1),
    type="void*",
    doc="Global latch for the completed level resource/blob handle returned by Level_LoadStateMachine; PKG_CleanupResourceGameState passes the non-null handle to Level_UnloadResources and then clears it.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "PKG_UpdateLoadingScreen_LoadingFadeCounter",
    xref("PKG_UpdateLoadingScreen", 0x4F, 0x2),
)
stable.data("Level_Load_MainMenuState", xref("Level_Load", 0xAF, 0x2))
stable.data(
    "Level_UpdateInterLevelMenu_MainMenuSelection",
    xref("Level_UpdateInterLevelMenu", 0x10A, 0x1),
)
stable.data(
    "Level_UpdateInterLevelMenu_FadeTimer",
    xref("Level_UpdateInterLevelMenu", 0x12, 0x1),
)
stable.data(
    "PKG_UpdateLoadingScreen_LastLoadingImageIndex",
    xref("PKG_UpdateLoadingScreen", 0x13, 0x3),
)
stable.data(
    "PKG_UpdateLoadingScreen_ResourceLoadingImagePtr",
    xref("PKG_UpdateLoadingScreen", 0x35, 0x1),
)
stable.data(
    "Menu_LoadingFadeDelay",
    xref("UI_Update", 0x34, 0x2),
    type="int32_t",
    doc="Countdown used by the menu/load transition state machine before advancing loading fade steps.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Menu_ProcessMenuTransition_SkipTitleScreen",
    xref("Menu_ProcessMenuTransition", 0x8F, 0x2),
)
stable.data(
    "Input_CheckCheatCodeSequence_Index",
    xref("Input_CheckCheatCodeSequence", 0x14, 0x2),
)
stable.data(
    "Input_CheckCheatCodeSequence_PreviousButton",
    xref("Input_CheckCheatCodeSequence", 0x5, 0x2),
)
stable.data(
    "Collision_DetectActorCollisions_ObjectList",
    xref("Collision_DetectActorCollisions", 0x41, 0x1),
)
stable.data(
    "Collision_BuildAndResolveGroundEdgeWalls_Result",
    xref("Collision_BuildAndResolveGroundEdgeWalls", 0x44, 0x1),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestPosX",
    xref("Collision_DetectObjectNodeCollisions", 0xD9, 0x2),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestPosY",
    xref("Collision_DetectObjectNodeCollisions", 0xF4, 0x2),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestPosZ",
    xref("Collision_DetectObjectNodeCollisions", 0x102, 0x2),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestRadius",
    xref("Collision_DetectObjectNodeCollisions", 0x8F, 0x1),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestVelX",
    xref("Collision_DetectObjectNodeCollisions", 0x83, 0x2),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestVelY",
    xref("Collision_DetectObjectNodeCollisions", 0x99, 0x2),
)
stable.data(
    "Collision_DetectObjectNodeCollisions_TestVelZ",
    xref("Collision_DetectObjectNodeCollisions", 0xBB, 0x2),
)
stable.data(
    "Collision_DetectAndResolve3DCollision_ResponseBuffer",
    xref("Collision_DetectAndResolve3DCollision", 0x1AC6, 0x3),
)
stable.data(
    "Collision_InitializePlaneLookupTables_CollisionPlanePointers",
    xref("Collision_InitializePlaneLookupTables", 0x5, 0x1),
)
stable.data(
    "Collision_ResolveObjectNodeCollision_PlaneBuffer",
    xref("Collision_ResolveObjectNodeCollision", 0x1FC, 0x1),
)
stable.data(
    "Collision_BuildAndResolveGroundEdgeWalls_NormalY",
    xref("Collision_BuildAndResolveGroundEdgeWalls", 0x4F, 0x2),
)
stable.data(
    "Collision_DebugPolygonListCount",
    xref("Graphics_IsPolygonInDebugList", 0xC, 0x2),
    xref("Collision_DetectActorCollisions", 0x33, 0x2),
    doc=(
        "Number of registered collision polygon list slices used by Graphics_IsPolygonInDebugList. "
        "Collision_DetectAndResolve3DCollision stores a polygon-list base and count into the "
        "parallel arrays, then increments this bounded index."
    ),
)
stable.data(
    "Collision_BuildAndResolveGroundEdgeWalls_NormalX",
    xref("Collision_BuildAndResolveGroundEdgeWalls", 0x34, 0x1),
)
stable.data(
    "Collision_BuildAndResolveGroundEdgeWalls_NormalZ",
    xref("Collision_BuildAndResolveGroundEdgeWalls", 0x39, 0x2),
)
stable.data(
    "Collision_InitializePlaneLookupTables_CollisionResponsePlanes",
    xref("Collision_InitializePlaneLookupTables", 0xF, 0x1),
)
stable.data(
    "Collision_BuildAndResolveGroundEdgeWalls_Dist",
    xref("Collision_BuildAndResolveGroundEdgeWalls", 0x60, 0x2),
)
stable.data(
    "Actor_DefaultUpdateCallbackSlot",
    xref("Graphics_InitializeDispatchTables", 0x1D, 0x2),
    xref("Collision_DetectAndResolve3DCollision", 0x1796, 0x3),
    type="ActorCollisionProbeCallback[4]",
    doc=(
        "Slot 0 of the sparse collision-state callback vector, initialized to "
        "Actor_HandleDefaultUpdate by Graphics_InitializeDispatchTables."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_CollisionDispatchActorFunc",
    xref("Level_InitializeActorSystem", 0x11, 0x2),
    type="ActorCollisionProbeCallback",
    doc="Actor collision response callback slot initialized to Actor_ProcessCollisionResponse; part of the sparse collision-state callback vector.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Collision_InitializeFunctionPointers_ComponentResponseFunc",
    xref("Collision_InitializeFunctionPointers", 0xA, 0x2),
    type="ActorCollisionProbeCallback",
    doc="Component and projectile collision response callback slot initialized to Collision_ProcessProjectileHit.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Powerup_CollisionFilterCallback",
    xref("Collision_ProcessPowerupCollisions", 0x3, 0x1),
    type="Powerup_CollisionCallback",
    doc=(
        "Four-argument Powerup collision filter callback slot initialized by "
        "Powerup_InitializeSystem; Collision_ProcessPowerupCollisions passes "
        "(powerup_actor, actor, 0, -2)."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Audio_SetEnabledFlag_SystemFlag",
    xref("Audio_SetEnabledFlag", 0x4, 0x1),
    type="uint8_t",
    doc="Byte-sized audio enabled flag stored by Audio_SetEnabledFlag and read by Audio_GetEnabledFlag.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_ShutdownSystem_OpenStreamCount",
    xref("Audio_ShutdownSystem", 0x3D, 0x1),
    type="int32_t",
    doc="Count of open Miles streams, incremented by Audio_OpenStream and decremented by Audio_CloseMusicStream.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_StartSoundPlayback_Slots",
    xref("Audio_StartSoundPlayback", 0x70, 0x2),
    type="Audio_SoundSlot",
    doc=(
        "Base entry of the nine-slot Audio_SoundSlot array. Audio_InitializeSystem seeds the "
        "allocation cursor after the base entry, and SDK typed globals resolve to the array base."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_ShutdownSystem_SoundSlot0SampleHandle",
    xref("Audio_ShutdownSystem", 0x11, 0x1),
    type="Audio_AILHSample",
    doc="Sample-handle field of sound_slots[0]. Audio_ShutdownSystem iterates this field across all sound slots.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Audio_SetStreamVolume_MusicStreamHandle",
    xref("Audio_SetStreamVolume", 0x0, 0x2),
    type="Audio_AILHStream",
    doc="Active Miles music stream handle consumed by set-volume/play/pause/resume/close helpers.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Audio_ResetMusicState_SelectedStreamRecord",
    xref("Audio_ResetMusicState", 0x4, 0x1),
    type="int32_t*",
    doc="Selected music stream record pointer stored by Audio_ResetMusicState and later passed to "
    "Audio_PlayMusicStream by Audio_ProcessMusicFade.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_ShutdownSystem_ActiveWaveCount", xref("Audio_ShutdownSystem", 0x42, 0x2)
)
stable.data(
    "Audio_FadeOutMusic_SoundSystemFlags",
    xref("Audio_FadeOutMusic", 0x0, 0x2),
    type="uint8_t",
    doc="Byte-sized audio/music state flags used by music fade/transition routines.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_ShutdownSystem_MusicTransitionTarget",
    xref("Audio_ShutdownSystem", 0x2F, 0x2),
    type="int16_t",
    doc="Signed 16-bit music transition target/state value cleared during audio shutdown.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_StartMusicWithFade_TransitionState",
    xref("Audio_StartMusicWithFade", 0x7, 0x3),
    type="int16_t",
    doc="Signed 16-bit current music fade/transition volume state advanced by Audio_ProcessMusicFade.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_SetMusicFadeTarget_Volume",
    xref("Audio_SetMusicFadeTarget", 0x7, 0x3),
    type="int16_t",
    doc="Signed 16-bit target volume used by Audio_SetMusicFadeTarget and Audio_ProcessMusicFade.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Trail_ResetBone_CurrentPathNodeSelector",
    xref("Trail_ResetBone", 0x12, 0x2),
    doc=(
        "Bone-trail current path-node selector. Trail_FindBonePath writes it from path-node "
        "metadata; later trail code resolves the selector through current-level runtime tables."
    ),
)
stable.data("Trail_ResetBone_PathNodeCount", xref("Trail_ResetBone", 0x18, 0x2))
stable.data("Trail_FindBonePath_End", xref("Trail_FindBonePath", 0x25F, 0x1))
stable.data(
    "Trail_CheckBoneAvailable_Timer",
    xref("Trail_CheckBoneAvailable", 0x13, 0x2),
)
stable.data(
    "Trail_UpdateAndRenderBone_Entries",
    xref("Trail_UpdateAndRenderBone", 0x497, 0x1),
)
stable.data("Trail_ResetBone_Entry0Timestamp", xref("Trail_ResetBone", 0x1E, 0x1))
stable.data(
    "Trail_UpdateAndRenderBone_Entry0PosX",
    xref("Trail_UpdateAndRenderBone", 0x40B, 0x1),
)
stable.data(
    "Trail_UpdateAndRenderBone_PathNodes",
    xref("Trail_UpdateAndRenderBone", 0x3B5, 0x6),
)
stable.data(
    "Trail_UpdateAndRenderBone_PathBufferY",
    xref("Trail_UpdateAndRenderBone", 0x44B, 0x2),
)
stable.data(
    "Trail_UpdateAndRenderBone_PathBufferZ",
    xref("Trail_UpdateAndRenderBone", 0x41E, 0x1),
)
stable.data(
    "Trail_UpdateAndRenderBone_TargetPosition",
    xref("Trail_UpdateAndRenderBone", 0x137, 0x2),
)
stable.data("Trail_ResetBone_AnimTime", xref("Trail_ResetBone", 0x28, 0x1))
stable.data(
    "Trail_UpdateAndRenderBone_PathBufferZ1",
    xref("Trail_UpdateAndRenderBone", 0x157, 0x2),
)
stable.data("Trail_FindBonePath_BufferX2", xref("Trail_FindBonePath", 0x9A, 0x1))
stable.data("Trail_FindBonePath_BufferY2", xref("Trail_FindBonePath", 0x759, 0x1))
stable.data("Trail_FindBonePath_BufferZ2", xref("Trail_FindBonePath", 0x760, 0x2))
stable.data(
    "PKG_FixUpResourceObjectNodeType3ComplexActorLike_SpecialNodeProcessingFlag",
    xref("PKG_FixUpResourceObjectNodeType3ComplexActorLike", 0x23D, 0x2),
)
stable.data(
    "Save_ProcessGameOperation_State",
    xref("Save_ProcessGameOperation", 0x3D, 0x2),
    type="uint32_t",
    doc=(
        "Packed active save I/O operation/status word initialized by Save_InitGameOperation "
        "and polled by Save_ProcessGameOperation; byte 0 carries the result/status, byte 1 the success flag, "
        "byte 2 the requested operation, and byte 3 the file operation code."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_ProcessGameOperation_Buffer",
    xref("Save_ProcessGameOperation", 0x5D, 0x2),
    type="uint8_t*",
    doc=(
        "Async save-operation buffer pointer for the static save-file and save-slot record span."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_ProcessGameOperation_Size",
    xref("Save_ProcessGameOperation", 0x29, 0x1),
    type="uint32_t",
    doc="Byte count passed to save read and write/verify helpers by Save_ProcessGameOperation.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Save_ProcessGameOperation_VerifyBuffer",
    xref("Save_ProcessGameOperation", 0x2E, 0x2),
    type="uint8_t*",
    doc="Comparison buffer used by Save_ProcessGameOperation's operation 12 verify path.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Video_PlayMovieFile_SkipRequested",
    xref("Video_PlayMovieFile", 0x3C, 0x2),
    type="int32_t",
    doc="Movie playback skip/shutdown flag cleared by Video_PlayMovieIntro and set by Video_PlayMovieFile when the user skips or closes playback.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Video_PlayMovieIntro_AVIPlaybackStarted",
    xref("Video_PlayMovieIntro", 0x112, 0x2),
    type="int32_t",
    doc="AVI playback-start latch toggled by Video_PlayMovieIntro around Video_IsAVIPlaying during the AVI fallback path.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Window_ProcessGameProc_MessageState",
    xref("Window_ProcessGameProc", 0x10, 0x2),
)
stable.data(
    "Graphics_DrawSortedLists_DebugShowFPSCounter",
    xref("Graphics_DrawSortedLists", 0x9F, 0x1),
)
stable.data(
    "Input_RegisterButtonMapping_KeyboardMappingKeys",
    xref("Input_RegisterButtonMapping", 0x20, 0x1),
    type="int32_t*",
    doc="Heap array of registered keyboard virtual-key codes, grown by Input_RegisterButtonMapping.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_RegisterButtonMapping_KeyboardMappingButtons",
    xref("Input_RegisterButtonMapping", 0x3E, 0x1),
    type="uint32_t*",
    doc="Heap array parallel to keyboard_mapping_keys; each entry is the Input_State.button_bits mask for that key.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Graphics_RenderFrame_LastFrameTick",
    xref("Graphics_RenderFrame", 0xA5, 0x2),
    type="uint32_t",
    doc=(
        "Timer_GetElapsedTickCount value captured when Graphics_RenderFrame last passed "
        "its internal 33 ms frame limiter. Writable so callers pacing the game "
        "themselves can rewind it and keep the limiter open."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Graphics_DrawQuad_InputProcessedFlag", xref("Graphics_DrawQuad", 0x38A, 0x2)
)
stable.data(
    "Graphics_RenderFrame_StartTimeSec",
    xref("Graphics_RenderFrame", 0xE, 0x2),
    type="float",
    doc="Timer_GetGameTime value captured at the start of Graphics_RenderFrame.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Graphics_RenderFrame_EndTimeSec",
    xref("Graphics_RenderFrame", 0x1C2, 0x2),
    type="float",
    doc="Timer_GetGameTime value captured after rendering/flip handling in Graphics_RenderFrame.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Debug_RenderOverlay_CurrentFPS",
    xref("Debug_RenderOverlay", 0x93, 0x2),
    type="float",
    doc="Frames-per-second value rendered by the debug overlay after Graphics_RenderFrame updates the accumulator.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Graphics_RenderFrame_DebugFPSAccumulatedFrameTime",
    xref("Graphics_RenderFrame", 0x1CC, 0x2),
    type="float",
    doc="Accumulated frame-time seconds used with fps_frame_count to refresh current_fps.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Graphics_RenderFrame_DebugFPSFrameCount",
    xref("Graphics_RenderFrame", 0x1EB, 0x1),
    type="int32_t",
    doc="Number of frames accumulated since the last current_fps refresh.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_LoadState",
    xref("PKG_CleanupResourceGameState", 0x1D, 0x2),
    type="uint8_t",
    doc=(
        "Top-level menu/load dispatch state byte used by Level_Load, UI_Update, "
        "PKG_CleanupResourceGameState, and transition helpers."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Level_LoadStateMachine_PKGLevelStreamLoadState",
    xref("Level_LoadStateMachine", 0xC, 0x1),
    type="int32_t",
    doc=(
        "Async package-stream loader stage dword used only by Level_LoadStateMachine; stages 0,1,2,4,5,7 "
        "load level TOC entries and then reset this state to zero."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Level_LoadStateMachine_PKGResourceLevelTexDataA",
    xref("Level_LoadStateMachine", 0x20, 0x1),
    type="void*",
    doc="Internal first level package-entry buffer loaded from TOC entry 0x24 + level_index * 3 by Level_LoadStateMachine stage 0.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Level_LoadStateMachine_PKGResourceLevelTexDataB",
    xref("Level_LoadStateMachine", 0x68, 0x1),
    type="void*",
    doc="Internal second level package-entry buffer loaded from TOC entry 0x25 + level_index * 3 by Level_LoadStateMachine stage 2.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "PKG_FixUpResourceLevelPointers_DebugLoggingEnabled",
    xref("PKG_FixUpResourceLevelPointers", 0x0, 0x1),
)
stable.data(
    "PKG_LoadEntry_FileHandle",
    xref("PKG_LoadEntry", 0x4C, 0x1),
    type="File_Handle*",
    doc="Open package File_Handle consumed by PKG_LoadEntry while reading aligned package entries.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "PKG_LoadEntry_Toc",
    xref("PKG_LoadEntry", 0x45, 0x3),
    type="PKG_TOCEntry",
    doc=(
        "Base of the / package table of contents; each PKG_TOCEntry stores file position and size."
    ),
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data("Tree_RebalanceMap_Buckets", xref("Tree_RebalanceMap", 0x15, 0x3))
stable.data(
    "Signal_TimedEventListHead",
    xref("Signal_ClearTimedEventList", 0x0, 0x2),
    type="void*",
    doc="Head pointer for the timed signal/event linked list cleared by Signal_ClearTimedEventList.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Signal_QueueCount",
    xref("Signal_ClearQueue", 0x0, 0x2),
    type="uint8_t",
    doc="Number of queued signal entries cleared by Signal_ClearQueue and consumed by Signal_Poll.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Signal_Queue",
    xref("Signal_Poll", 0x1D, 0x1),
    type="Signal_QueueEntry[0x28]",
    doc="Forty-entry signal queue scanned by Signal_Poll.",
    write_policy=WritePolicy.READ_ONLY,
    stable=True,
)
stable.data(
    "g_soundVolumeArray",
    xref("Audio_ProcessSoundQueue", 0x1F2, 0x4),
    type="uint16_t[0x9]",
    doc="Nine sound-volume values read by Audio_ProcessSoundQueue.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_FreeSoundSlot_Entries",
    xref("Audio_FreeSoundSlot", 0xA, 0x1),
    type="Audio_SoundEntry",
    doc="Base of the active and free sound-entry pool; Audio_AllocateSoundSlot computes slot indices from this base.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_AllocateSoundSlot_Reserved",
    xref("Audio_AllocateSoundSlot", 0x11, 0x1),
    type="Audio_SoundEntry",
    doc="Ninth Audio_SoundEntry at sound_entries[8], reserved for Audio_SoundDefinition flags bit 0x80 before being linked into the active list.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_FreeSoundSlot_ActiveSoundList",
    xref("Audio_FreeSoundSlot", 0x33, 0x2),
    type="Audio_SoundEntry*",
    doc="Head pointer for the doubly linked active Audio_SoundEntry list maintained by Audio_AllocateSoundSlot and Audio_FreeSoundSlot.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_FreeSoundSlot_Tail",
    xref("Audio_FreeSoundSlot", 0x4A, 0x2),
    type="Audio_SoundEntry*",
    doc="Tail pointer for the active Audio_SoundEntry list; new allocated entries are appended here.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_FreeSoundSlot_ListPtr",
    xref("Audio_FreeSoundSlot", 0x5C, 0x2),
    type="Audio_SoundEntry*",
    doc="Head pointer for the free Audio_SoundEntry list read by Audio_AllocateSoundSlot and replenished by Audio_FreeSoundSlot.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_UpdateSoundChannels_Timer",
    xref("Audio_UpdateSoundChannels", 0xA3, 0x1),
    type="int32_t",
    doc="Last Timer_GetElapsedTickCount value captured by Audio_UpdateSoundChannels for per-channel sound timing updates.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Replay_StartDemoPlayback_InputPtr",
    xref("Replay_StartDemoPlayback", 0xE, 0x2),
    type="int32_t*",
    doc="Pointer to the current demo replay input-frame stream, loaded from replay_data[1] by Replay_StartDemoPlayback when replay playback begins.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Audio_TriggerMusicTransition_FrameCounter",
    xref("Audio_TriggerMusicTransition", 0x60, 0x2),
    type="int32_t",
    doc="Data frame counter used by music fade/transition timing and other frame-based game state checks.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Game_PauseStateCounter",
    xref("Game_UpdateAndRenderScene", 0x0, 0x1),
    type="uint8_t",
    doc=("Pause frame counter tested at the top of Game_UpdateAndRenderScene."),
    write_policy=WritePolicy.READ_ONLY,
)
stable.data(
    "Game_FrameTransitionFlags",
    xref("Audio_TriggerMusicTransition", 0xC, 0x2),
    type="uint32_t",
    doc="Shared frame/input/audio transition bitfield. Bit 0x20 marks demo replay playback, bit 0x10 selects alternate 3D-audio listener camera data, bit 0x400 is set by Audio_TriggerMusicTransition, bit 0x08 requests unload before being cleared after PKG_UnloadResourceGameData, bit 0x1000 allows cleanup/load rendering, bit 0x4000 requests Level_Load, bit 0x04 marks post-load actor/audio initialization, and bit 0x02 allows active scene update/render.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "String_SetTable_Table",
    xref("String_SetTable", 0x4, 0x1),
    type="int16_t*",
    doc="Active package/localization string table pointer consumed by String_GetByIndex; split from input button-name buffers.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "g_inputButtonNameBuffers",
    xref("Menu_RenderFormattedText", 0x32, 0x3),
    type="char*[13]",
    doc="Thirteen input button-name buffer pointers indexed by button id. Slot 12 overlaps the "
    "no-key-assigned string at 0x634970.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Title_InitializeSpots_ActiveCount",
    xref("Title_InitializeSpots", 0xE, 0x3),
)
stable.data(
    "PKG_LoadTitleScreenResources_AudioTitleMusicData",
    xref("PKG_LoadTitleScreenResources", 0x97, 0x2),
    type="int32_t*",
    doc="Title-screen music stream-record pointer loaded from the title package and armed by Audio_StartMusicWithFade.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "PKG_ResourceTitleMaterialBase",
    xref("PKG_LoadTitleScreenResources", 0x55, 0x2),
)
stable.data(
    "Title_InitializeSpots_CycleLength",
    xref("Title_InitializeSpots", 0x1A6, 0x2),
)
stable.data(
    "Title_UpdateAndRenderScreen_Counter",
    xref("Title_UpdateAndRenderScreen", 0x82, 0x3),
)
stable.data(
    "Title_InitializeSpots_MaterialIndex",
    xref("Title_InitializeSpots", 0x3F, 0x1),
)
stable.data(
    "PKG_ResourceTitlePackage",
    xref("PKG_LoadTitleScreenResources", 0x3E, 0x1),
)
stable.data(
    "Title_UpdateAndRenderScreen_FadeLevel",
    xref("Title_UpdateAndRenderScreen", 0xA8, 0x3),
)
stable.data(
    "Title_InitializeSpots_DataArray",
    xref("Title_InitializeSpots", 0x17, 0x1),
)
stable.data(
    "Title_InitializeSpots_TimerArray",
    xref("Title_InitializeSpots", 0x152, 0x4),
)
stable.data(
    "PKG_ResourceTitleBonusReplayResource",
    xref("Title_CleanupScreenResources", 0xC, 0x1),
)
stable.data(
    "Title_UpdateAndRenderScreen_State",
    xref("Title_UpdateAndRenderScreen", 0x10, 0x3),
    type="uint8_t",
    doc=(
        "Byte-sized title-screen state machine value 0..6; state 4 is the "
        "interactive title screen."
    ),
)
stable.data(
    "Title_SoundDefTable",
    xref("Title_UpdateSpots", 0x5C, 0x1),
    type="Audio_SoundDefinition*",
    doc="Pointer to the title-screen sound-definition table; Title_UpdateSpots passes it to Audio_PlaySound3D for spot animations.",
    unstable=True,
)
stable.data(
    "Title_InitializeSpots_FrameCounter",
    xref("Title_InitializeSpots", 0x7, 0x3),
)
stable.data("Mem_AllocateHandle_Pool", xref("Mem_AllocateHandle", 0x52, 0x2))
stable.data(
    "Mem_InitializeAllocator_HandlePoolHandleID",
    xref("Mem_InitializeAllocator", 0xA, 0x1),
)
stable.data(
    "Mem_InitializeAllocator_HeapAllocatorInitialized",
    xref("Mem_InitializeAllocator", 0x2D, 0x2),
)
stable.data("Mem_AllocateHandle_PoolHead", xref("Mem_AllocateHandle", 0x1, 0x2))
stable.data(
    "Input_ClearState_Buffer",
    xref("Input_ClearState", 0x8, 0x1),
    type="uint8_t[0x100]",
    doc=(
        "Raw input/VK state clear buffer zeroed by Input_ClearState. Public fixed-array accessors use pointer-to-array read and write signatures."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Display_CurrentWindowHandle",
    xref("Display_IsActive", 0x0, 0x2),
    xref("Display_SetMode", 0x0, 0x2),
    xref("Display_ReleaseMode", 0x0, 0x2),
    type="HWND",
    doc="Current HWND bound to the active display/D3D mode; checked by Display_IsActive, written by Display_SetMode, and tested before Display_ReleaseMode cleanup.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "String_ConvertToExponentialFloat_FormatPrecision",
    xref("String_ConvertToExponentialFloat", 0x11, 0x1),
)
stable.data(
    "String_ConvertToExponentialFloat_FormatFlags",
    xref("String_ConvertToExponentialFloat", 0x3, 0x2),
)
stable.data("Mem_AllocateHandle_DebugEnabled", xref("Mem_AllocateHandle", 0x69, 0x2))
stable.data(
    "Level_InitializeActorSystem_CameraData",
    xref("Level_InitializeActorSystem", 0x1FD, 0x1),
)
stable.data(
    "Player_ProcessMovement_FacingAngle",
    xref("Player_ProcessMovement", 0x159, 0x3),
)
stable.data(
    "Player_ProcessMovement_CameraYawAngle",
    xref("Player_ProcessMovement", 0x162, 0x3),
    type="uint32_t",
    doc=(
        "Packed camera yaw global used by Player_ProcessMovement. The high 16 bits hold "
        "the signed yaw angle; the low 16 bits are reserved, and a zero high word is "
        "a valid yaw sample."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data("Camera_SetViewport_Height", xref("Camera_SetViewport", 0x36, 0x2))
stable.data("Camera_SetViewport_FarClipPlane", xref("Camera_SetViewport", 0x3C, 0x2))
stable.data(
    "Audio_ListenerCameraPos_Flag10Set",
    xref("Checkers_UpdateStateMachine", 0x73, 0x6),
    type="Math_Vec3I32",
    doc="Camera/listener position vector selected by positional-audio panning when "
    "Game_FrameTransitionFlags bit 0x10 is set.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Audio_ListenerCameraPos_Flag10Clear",
    xref("Audio_PlaySoundDefinition3D", 0x67, 0x1),
    type="Math_Vec3I32",
    doc="Camera/listener position vector selected by positional-audio panning when "
    "Game_FrameTransitionFlags bit 0x10 is clear.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "PKG_InitializeSystem_GraphicsFlags", xref("PKG_InitializeSystem", 0x14, 0x2)
)
stable.data(
    "PKG_UnloadResourceGameData_LevelInitCallback2",
    xref("PKG_UnloadResourceGameData", 0x43, 0x2),
    type="Graphics_Render_ListCallback",
    doc=(
        "Fixed nullable Graphics_ListState +0xC4 scene post-sort slot. Resource activation installs "
        "Game_RenderOverlays, Scene_RenderFrame invokes it after sorted scene lists, and resource "
        "unload clears it."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "PKG_UnloadResourceGameData_LevelInitCallback1",
    xref("PKG_UnloadResourceGameData", 0x3D, 0x2),
    type="Graphics_Render_ListCallback",
    doc="Fixed nullable Graphics_ListState +0xC8 pre-list slot. Resource activation installs Graphics_ClearBackground.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)
stable.data(
    "Scene_RenderFrame_VertexBatchBuffer", xref("Scene_RenderFrame", 0x57A, 0x1)
)
stable.data(
    "Menu_UpdateInput_StatePreviousP1",
    xref("Menu_UpdateInput", 0x0, 0x2),
    doc=(
        "First/base byte of the two-player previous Input_State snapshot rows. Player 2 follows "
        "in the paired row, and scalar aliases overlap the row."
    ),
)
stable.data(
    "Menu_ProcessMenuState_InputToggleMaskP1", xref("Menu_ProcessMenuState", 0x1A2, 0x2)
)
stable.data(
    "Replay_StartDemoPlayback_SavedRandomSeed",
    xref("Replay_StartDemoPlayback", 0x1E, 0x1),
)
stable.data(
    "Camera_CalculateFollowAngles_InputStateCurrentP1",
    xref("Camera_CalculateFollowAngles", 0x33C, 0x2),
    doc=(
        "First/base byte of the two-player current Input_State snapshot rows. Player 2 follows in "
        "the paired row, and scalar aliases overlap the row."
    ),
)
stable.data(
    "Replay_DemoBonusReplayDataTable",
    xref("Replay_LoadBonusReplay", 0x24, 0x1),
    doc="Demo bonus-replay data table consumed by replay loading; name avoids the generic Data suffix.",
    unstable=True,
)
stable.data(
    "Graphics_RenderMeshNode_ColorAdjustmentFlag",
    xref("Graphics_RenderMeshNode", 0x6D8, 0x2),
)
stable.data(
    "Script_CurrentEntity",
    xref("Script_OpMoveToTarget", 0x87, 0x2),
    type="Entity_State*",
    doc=(
        "Script-dispatch current entity context written by Script_ExecuteBehaviorScript "
        "and read by script command handlers; engine-managed transient state."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Script_OpCheckTerminator_EntityIndex",
    xref("Script_OpCheckTerminator", 0x116, 0x1),
)
stable.data(
    "Actor_IntegrateMovementAndCollision_CallbackTable",
    xref("Actor_IntegrateMovementAndCollision", 0x8F9, 0x3),
    type="Actor_BehaviorCallback",
    doc="Read-only nine-entry actor phase callback table selected by Actor_State offset 0xC5. Dispatchers trust the signed selector without checking its range.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_BehaviorProcessActorFunc",
    xref("Level_InitializeActorSystem", 0x2F, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 1 selected by Actor_State+0xC5 == 1. Initialized to Actor_ProcessPlayerBehavior.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Collision_InitializeFunctionPointers_BehaviorProcessProjectileFunc",
    xref("Collision_InitializeFunctionPointers", 0x14, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 2, initialized to Collision_ProcessProjectileLifecycle. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_BehaviorProcessSnapFunc",
    xref("Level_InitializeActorSystem", 0x39, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 3 selected by Actor_State+0xC5 == 3. Initialized to Actor_ProcessSnapAndEntityUpdate.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Graphics_InitializeDispatchTables_BehaviorTargetActor",
    xref("Graphics_InitializeDispatchTables", 0x31, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 4, initialized to Actor_ApplyVerticalMovement. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Graphics_InitializeDispatchTables_BehaviorParam0",
    xref("Graphics_InitializeDispatchTables", 0x3B, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 5, initialized to Actor_ProcessMovementCommands. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Graphics_InitializeDispatchTables_BehaviorParam1",
    xref("Graphics_InitializeDispatchTables", 0x45, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 6, initialized to Actor_FollowAttachedMovement. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Graphics_InitializeDispatchTables_BehaviorParam2",
    xref("Graphics_InitializeDispatchTables", 0x4F, 0x2),
    type="Actor_BehaviorCallback",
    doc="Actor-phase slot 7, initialized to Actor_ProcessMovementBehavior. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Powerup_InitializeSystem_UpdateCallback",
    xref("Powerup_InitializeSystem", 0x9, 0x2),
    type="Powerup_UpdateCallback",
    doc=(
        "Actor-phase slot 8 at 0x6344C0, initialized to Powerup_UpdateActorState and selected when "
        "Actor_State+0xC5 == 8. The nine-slot phase-table dispatcher ignores its return value."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Physics_UpdateActorPreprocess_MovementHandlerTable",
    xref("Physics_UpdateActorPreprocess", 0x8D, 0x3),
    type="Actor_BehaviorCallback",
    doc="Read-only base of the three-entry preprocess table selected by Actor_State+0xC4. Its indexed dispatcher ignores EAX.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_PlayerMovementFunc",
    xref("Level_InitializeActorSystem", 0x25, 0x2),
    type="Actor_BehaviorCallback",
    doc="Preprocess slot 1, initialized to Player_ProcessMovement. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Collision_InitializeFunctionPointers_ProjectileLogicFunc",
    xref("Collision_InitializeFunctionPointers", 0x0, 0x2),
    type="Actor_BehaviorCallback",
    doc="Preprocess slot 2, initialized to Component_UpdateProjectileLogic. Callers ignore its return value.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Menu_RenderSaveGame_UnloadDelayCounter", xref("Menu_RenderSaveGame", 0x5D, 0x1)
)
stable.data(
    "Powerup_LiveActorListHead",
    xref("Collision_ProcessPowerupCollisions", 0x9, 0x2),
    doc="Live powerup actor list head linked by Powerup_CloneActorFromTemplate.",
)
stable.data(
    "Powerup_HandleCollection_SpawnDelay",
    xref("Powerup_HandleCollection", 0x5C, 0x2),
)
stable.data(
    "Graphics_ProcessMeshCommands_Capabilities",
    xref("Graphics_ProcessMeshCommands", 0x3B, 0x2),
)
stable.data(
    "Audio_OpenStream_PKGBasePath",
    xref("Audio_OpenStream", 0x22, 0x1),
    type="char[0x104]",
    doc="Mutable NUL-terminated package base path buffer used to build package and data/music stream paths.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_BatchTriangleCount",
    xref("Graphics_ClipAndDrawPolygon", 0x81D, 0x2),
)
stable.data("Window_RunWinMain_InstanceHandle", xref("Window_RunWinMain", 0x21C, 0x1))
stable.data(
    "Graphics_ClipAndDrawPolygon_BatchPrimitiveCount",
    xref("Graphics_ClipAndDrawPolygon", 0x7CF, 0x1),
)
stable.data(
    "Input_ProcessWindowMessages_AcceleratorTable",
    xref("Input_ProcessWindowMessages", 0x8B, 0x1),
)
stable.data("Window_RunWinMain_ShowCmd", xref("Window_RunWinMain", 0x18C, 0x2))
stable.data("Graphics_DrawQuad_RenderFrameCount", xref("Graphics_DrawQuad", 0xB9D, 0x2))
stable.data(
    "String_GetByIndex_LocalizationLanguageID",
    xref("String_GetByIndex", 0xA1, 0x1),
)
stable.data("String_GetByIndex_TableLoaded", xref("String_GetByIndex", 0x7C, 0x1))
stable.data(
    "Menu_RenderConfirmPrompt_StringMenuBufferPtr",
    xref("Menu_RenderConfirmPrompt", 0x50, 0x2),
)
stable.data("String_GetByIndex_TableSize", xref("String_GetByIndex", 0x61, 0x2))
stable.data(
    "Input_FormatButtonName_NoKeyAssignedString",
    xref("Input_FormatButtonName", 0x5C, 0x1),
    type="char*",
    doc=(
        'Cached heap string for "No key assigned"; slot 12 of input_button_name_buffers shares '
        "this address."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "UI_InitializedFlag",
    xref("Menu_AnimateSlots", 0x124, 0x1),
    xref("UI_Update", 0x0, 0x1),
    type="BOOL",
    doc="UI initialization flag checked by UI_Update/Menu_AnimateSlots.",
)
stable.data(
    "Graphics_RenderFrame_Index",
    xref("Graphics_RenderFrame", 0x122, 0x2),
)
stable.data(
    "Input_RegisterButtonMapping_Count",
    xref("Input_RegisterButtonMapping", 0x1A, 0x2),
    type="int32_t",
    doc="Number of entries in the keyboard_mapping_keys/keyboard_mapping_buttons arrays.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisYNegativeMask",
    xref("Input_ReadGamepad", 0x86, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3e8; ORed when DIJOYSTATE.lY < -700.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisYPositiveMask",
    xref("Input_ReadGamepad", 0x9A, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3e9; ORed when DIJOYSTATE.lY > 700.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisXNegativeMask",
    xref("Input_ReadGamepad", 0x5F, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ea; ORed when DIJOYSTATE.lX < -700.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisXPositiveMask",
    xref("Input_ReadGamepad", 0x72, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3eb; ORed when DIJOYSTATE.lX > 700.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisRzNegativeMask",
    xref("Input_ReadGamepad", 0x1F5, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ec; ORed when DIJOYSTATE.lRz < -600.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_AxisRzPositiveMask",
    xref("Input_ReadGamepad", 0x209, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ed; ORed when DIJOYSTATE.lRz > 600.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_ReadGamepad_Button0Mask",
    xref("Input_ReadGamepad", 0x229, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ee; ORed when DIJOYSTATE.rgbButtons[0] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button1Mask",
    xref("Input_ReadGamepad", 0x24A, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3ef; ORed when DIJOYSTATE.rgbButtons[1] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button2Mask",
    xref("Input_ReadGamepad", 0x26A, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f0; ORed when DIJOYSTATE.rgbButtons[2] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button3Mask",
    xref("Input_ReadGamepad", 0x28A, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f1; ORed when DIJOYSTATE.rgbButtons[3] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button4Mask",
    xref("Input_ReadGamepad", 0x2AB, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f2; ORed when DIJOYSTATE.rgbButtons[4] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button5Mask",
    xref("Input_ReadGamepad", 0x2CB, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f3; ORed when DIJOYSTATE.rgbButtons[5] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button6Mask",
    xref("Input_ReadGamepad", 0x2EB, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f4; ORed when DIJOYSTATE.rgbButtons[6] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button7Mask",
    xref("Input_ReadGamepad", 0x30C, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f5; ORed when DIJOYSTATE.rgbButtons[7] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button8Mask",
    xref("Input_ReadGamepad", 0x32C, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f6; ORed when DIJOYSTATE.rgbButtons[8] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button9Mask",
    xref("Input_ReadGamepad", 0x34C, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f7; ORed when DIJOYSTATE.rgbButtons[9] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button10Mask",
    xref("Input_ReadGamepad", 0x36D, 0x1),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f8; ORed when DIJOYSTATE.rgbButtons[10] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button11Mask",
    xref("Input_ReadGamepad", 0x38D, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3f9; ORed when DIJOYSTATE.rgbButtons[11] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ReadGamepad_Button12Mask",
    xref("Input_ReadGamepad", 0x3AD, 0x2),
    type="uint32_t",
    doc="Direct gamepad lookup entry for control code 0x3fa; ORed when DIJOYSTATE.rgbButtons[12] is pressed.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Menu_HandleOptionsLogic_DisplayMenuSetting",
    xref("Menu_HandleOptionsLogic", 0x37F, 0x1),
    type="uint8_t",
    doc="Saved display/detail setting byte from pcdogs.ini; clamped to 0..10 before being applied.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Config_ApplySettings_InputPlayer1Controls",
    xref("Config_ApplySettings", 0x77, 0x2),
    xref("Config_ApplySettings", 0x86, 0x2, required=Required.EU_SC),
    type="int32_t",
    doc=(
        "First dword/base of the player-1 pcdogs.ini binding block; Config_ApplySettings applies the first 10 normal mappings."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_InitializeButtonMappings_Player1DownKey",
    xref("Input_InitializeButtonMappings", 0x167, 0x2),
)
stable.data(
    "Input_InitializeButtonMappings_Map",
    xref("Input_InitializeButtonMappings", 0x171, 0x2),
    type="int32_t[8]",
    doc="Eight adjacent player-1 button mapping dwords after the down-key entry.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Config_ApplySettings_InputSpecialButton",
    xref("Config_ApplySettings", 0x8F, 0x2),
    xref("Config_ApplySettings", 0x9E, 0x2, required=Required.EU_SC),
    type="int32_t",
    doc="Additional pcdogs.ini button binding assigned to Input_State mask 0x4000; defaults to "
    "VK_SPACE (0x20) when unset.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Config_ApplySettings_InputPlayer2Controls",
    xref("Config_ApplySettings", 0x57, 0x2),
    xref("Config_ApplySettings", 0x66, 0x2, required=Required.EU_SC),
    type="int32_t",
    doc=(
        "First dword/base of the player-2/gamepad pcdogs.ini binding block; Config_ApplySettings applies the first 10 normal mappings."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_InitializeButtonMappings_Player2DownButton",
    xref("Input_InitializeButtonMappings", 0x1D5, 0x2),
)
stable.data(
    "Input_InitializeButtonMappings_MapAlt",
    xref("Input_InitializeButtonMappings", 0x1DF, 0x2),
    type="int32_t[8]",
    doc="Eight adjacent player-2/gamepad button mapping dwords after the down-button entry.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Video_PlayMovieIntro_ConfigEndSentinel",
    xref("Video_PlayMovieIntro", 0x8E, 0x2),
    type="int32_t",
    doc=(
        "Post-config-block dword used as the exclusive end sentinel for pcdogs.ini "
        "control binding loops and zeroed on the AVI movie path."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "PKG_FixUpResourceObjectNodeType1MeshActorLike_LevelMaterialSection",
    xref("PKG_FixUpResourceObjectNodeType1MeshActorLike", 0x115, 0x1),
    type="Material_SectionHeader*",
    doc="Active level material section header/base used while rebasing model-node material references.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "g_levelBlobPtr",
    xref("PKG_FixUpResourceObjectNodeDispatchByType", 0xC, 0x2),
    type="uint8_t*",
    doc=(
        "Byte-addressed level blob base used to relocate package material, object, animation, and level offsets."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Shadow_ClearList_RenderList",
    xref("Shadow_ClearList", 0x0, 0x2),
)
stable.data(
    "Actor_CollisionListHead",
    xref("Script_OpPauseToggle", 0x1D3, 0x1),
    doc=(
        "Head of the actor-to-actor collision-processing linked list. Actor_AddToCollisionList prepends Actor_State records here via list_next; pause/entity update paths walk and prune it."
    ),
)
stable.data(
    "Collision_ProcessActorToActorCollisions_StateHandlerTable",
    xref("Collision_ProcessActorToActorCollisions", 0xC6, 0x3),
    type="Collision_ProcessCallback",
    doc="Read-only first entry/base of the collision-state callback table indexed by actor collision subtype; slot 2 aliases collision_process_func.",
    write_policy=WritePolicy.READ_ONLY,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_CollisionProcessFunc",
    xref("Level_InitializeActorSystem", 0x1B, 0x2),
    type="Collision_ProcessCallback",
    doc="Engine-managed scalar actor collision processing callback slot initialized to Physics_ProcessActorCollision and aliased by collision_state_handler_table slot 2.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Collision_DebugPolygonListCounts",
    xref("Graphics_IsPolygonInDebugList", 0x0, 0x1),
    doc=(
        "Parallel count array for collision polygon list slices registered by "
        "Collision_DetectAndResolve3DCollision and queried by Graphics_IsPolygonInDebugList."
    ),
)
stable.data(
    "Collision_DebugPolygonListBases",
    xref("Graphics_IsPolygonInDebugList", 0x27, 0x3),
    doc=(
        "Parallel base-pointer array for Collision_Polygon slices registered by Collision_DetectAndResolve3DCollision. Graphics_IsPolygonInDebugList checks whether a candidate polygon address falls inside one of these slices."
    ),
)
stable.data(
    "Shared_MaterialFrameSetCursor",
    xref("Shared_LoadCommonResources", 0xC4, 0x2),
    type="Material_FrameSet*",
    doc="Cursor into shared material frame-set storage advanced by Material_BuildTextureArray.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Player_RespawnAfterDeath_ScreenFadeDuration",
    xref("Player_RespawnAfterDeath", 0x69, 0x2),
)
stable.data(
    "Audio_ProcessMusicFade_ScreenFadeCounter",
    xref("Audio_ProcessMusicFade", 0x47, 0x1),
)
stable.data(
    "Player_RespawnAfterDeath_IsLoadingLevel",
    xref("Player_RespawnAfterDeath", 0x58, 0x2),
)
stable.data(
    "Menu_ProcessMenuTransition_LevelIndex",
    xref("Menu_ProcessMenuTransition", 0x57, 0x2),
    type="int16_t",
    doc="Menu/load level index. Cross-check against player_current_level_id before using it as live runtime state.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Level_InitializeActorSystem_RenderingStateFlag",
    xref("Level_InitializeActorSystem", 0x102, 0x3),
)
stable.data(
    "Shared_LoadCommonResources_PKGResourceSharedMaterialSection",
    xref("Shared_LoadCommonResources", 0x0, 0x1),
    type="Material_SectionHeader*",
    doc="Shared/common material section header/base loaded by Shared_LoadCommonResources.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_ControllerHammerheadProfileName",
    xref("Menu_RenderMusicSelection", 0x1A, 0x1),
    doc="Hammerhead controller profile name/preset metadata rendered from menu paths.",
    unstable=True,
)
stable.data(
    "Menu_HandleOptionsLogic_InputControllerHammerheadButtons",
    xref("Menu_HandleOptionsLogic", 0x154, 0x3),
    type="int32_t[10]",
    doc="Ten adjacent Hammerhead button preset dwords inside a controller profile record.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_InitializeControllerMappings_SidewinderButtons",
    xref("Input_InitializeControllerMappings", 0x94, 0x2),
    type="int32_t[10]",
    doc="Ten adjacent SideWinder button preset dwords inside a controller profile record.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_InitializeControllerMappings_GravisButtons",
    xref("Input_InitializeControllerMappings", 0xF7, 0x2),
    type="int32_t[10]",
    doc="Ten adjacent Gravis button preset dwords inside a controller profile record.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Input_InitializeControllerMappings_WingmanButtonRef",
    xref("Input_InitializeControllerMappings", 0x16E, 0x2),
    type="int32_t",
    doc=(
        "One WingMan button preset dword; surrounding WingMan writes are individual profile fields."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Shared_FontDataCursor",
    xref("Shared_LoadCommonResources", 0xB8, 0x2),
    doc="Shared/common font-data cursor used during common resource loading; package ownership remains unstable.",
    unstable=True,
)
stable.data(
    "Shared_TextureDataRefsCursor",
    xref("Shared_LoadCommonResources", 0xB0, 0x1),
    doc="Shared/common texture data-reference cursor used during common resource loading; package ownership remains unstable.",
    unstable=True,
)
stable.data(
    "Menu_AnimateSlots_PKGResourceCurrentUsableMaterials",
    xref("Menu_AnimateSlots", 0x16, 0x2),
)
stable.data(
    "Menu_ProcessMenuState_GradientColorValue",
    xref("Menu_ProcessMenuState", 0x9E3, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderTransformMatrix",
    xref("Bone_TransformWeightedVerticesForRender", 0x1C, 0x1),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix01",
    xref("Bone_TransformWeightedVerticesForRender", 0x23, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix02",
    xref("Bone_TransformWeightedVerticesForRender", 0x3C, 0x2),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix10",
    xref("Bone_TransformWeightedVerticesForRender", 0x35, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix11",
    xref("Bone_TransformWeightedVerticesForRender", 0xA1, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix12",
    xref("Bone_TransformWeightedVerticesForRender", 0x57, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix20",
    xref("Bone_TransformWeightedVerticesForRender", 0x50, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix21",
    xref("Bone_TransformWeightedVerticesForRender", 0x62, 0x3),
)
stable.data(
    "Bone_TransformWeightedVerticesForRender_GraphicsRenderMatrix22",
    xref("Bone_TransformWeightedVerticesForRender", 0xCD, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_HighlightMode",
    xref("Graphics_RenderPolygonBatch", 0x1AAE, 0x1),
)
stable.data(
    "Graphics_RenderPolygonBatch_ViewDirectionX",
    xref("Graphics_RenderPolygonBatch", 0x3C, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_ViewDirectionY",
    xref("Graphics_RenderPolygonBatch", 0x32, 0x3),
)
stable.data(
    "Graphics_RenderPolygonBatch_ViewDirectionZ",
    xref("Graphics_RenderPolygonBatch", 0x1D, 0x3),
)
stable.data(
    "Graphics_RenderPolygonMesh_CurrentPolygonBatchIndex",
    xref("Graphics_RenderPolygonMesh", 0x5DF, 0x1),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraTransformMatrix",
    xref("Graphics_SetPolygonUVs", 0x15B, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_TransformMatrixElement1",
    xref("Graphics_SetPolygonUVs", 0x13E, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM02",
    xref("Graphics_SetPolygonUVs", 0x167, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM10",
    xref("Graphics_SetPolygonUVs", 0x130, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM11",
    xref("Graphics_SetPolygonUVs", 0x173, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM12",
    xref("Graphics_SetPolygonUVs", 0x17F, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM20",
    xref("Graphics_SetPolygonUVs", 0x18B, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM21",
    xref("Graphics_SetPolygonUVs", 0x195, 0x3),
)
stable.data(
    "Graphics_SetPolygonUVs_CameraMatrixM22",
    xref("Graphics_SetPolygonUVs", 0x19F, 0x3),
)
stable.data(
    "Graphics_RenderPolygonMesh_NodeViewTranslationX",
    xref("Graphics_RenderPolygonMesh", 0x62, 0x2),
)
stable.data(
    "Graphics_RenderPolygonMesh_NodeViewTranslationY",
    xref("Graphics_RenderPolygonMesh", 0xAE, 0x2),
)
stable.data(
    "Graphics_RenderPolygonMesh_NodeViewTranslationZ",
    xref("Graphics_RenderPolygonMesh", 0x104, 0x2),
)
stable.data(
    "Graphics_RenderPolygonMesh_BatchRecords",
    xref("Graphics_RenderPolygonMesh", 0x5F2, 0x3),
)
stable.data(
    "Video_InitPlayer_ErrorCode",
    xref("Video_InitPlayer", 0x10, 0x1),
    type="int32_t",
    doc="Last winplay video/sound/movie initialization or playback status code.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "DInput_EnumerateForceFeedbackJoysticks_Available",
    xref("DInput_EnumerateForceFeedbackJoysticks", 0x39, 0x2),
    type="int32_t",
    doc="Set from joystick enumeration before device and effect setup. Later setup failures do not clear it, so it does not prove effect readiness.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "DInput_InitializeJoystickInput_WindowHandle",
    xref("DInput_InitializeJoystickInput", 0xE, 0x1),
    type="HWND",
    doc="Main game window handle used by DirectInput cooperative-level setup.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "DInput_EnumJoystickDeviceCallback_Seen",
    xref("DInput_EnumJoystickDeviceCallback", 0x3F, 0x1),
    type="int32_t",
    doc="Set by the DirectInput joystick enumeration callback after copying an enumerated device GUID into the caller-provided list.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Input_InitializeInputSubsystem_Initialized",
    xref("Input_InitializeInputSubsystem", 0x19, 0x1),
)
stable.data(
    "D3D_SetBlendMode_GraphicsTextRenderingMode",
    xref("D3D_SetBlendMode", 0xE, 0x1),
    type="int32_t",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ColorBlue",
    xref("Graphics_ClipAndDrawPolygon", 0x26C, 0x3),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorBlueVertex1",
    xref("Graphics_DrawQuad", 0x7BB, 0x2),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorBlueVertex2",
    xref("Graphics_DrawQuad", 0x87D, 0x2),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorBlueVertex3",
    xref("Graphics_DrawQuad", 0x98, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ColorGreen",
    xref("Graphics_ClipAndDrawPolygon", 0x247, 0x3),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorGreenVertex1",
    xref("Graphics_DrawQuad", 0x7A4, 0x2),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorGreenVertex2",
    xref("Graphics_DrawQuad", 0x866, 0x2),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorGreenVertex3",
    xref("Graphics_DrawQuad", 0x8D, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ColorRed",
    xref("Graphics_ClipAndDrawPolygon", 0x229, 0x3),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorRedVertex1",
    xref("Graphics_DrawQuad", 0x791, 0x1),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorRedVertex2",
    xref("Graphics_DrawQuad", 0x851, 0x2),
)
stable.data(
    "Graphics_DrawQuad_TempVertexColorRedVertex3",
    xref("Graphics_DrawQuad", 0x194, 0x2),
)
stable.data(
    "D3D_InitializeDirectDraw_GraphicsCanFlipSurfaces",
    xref("D3D_InitializeDirectDraw", 0x252, 0x2),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_3D7Interface",
    xref("D3D_InitDirectDrawAndDirect3D", 0x189, 0x1),
    type="D3D_IDirect3D7*",
    doc="IDirect3D7 interface obtained from IDirectDraw7::QueryInterface; used for device enumeration and device creation.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelBlueMask",
    xref("D3D_CreateTextureSurface", 0x1C3, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsTexturePow2Width",
    xref("D3D_CreateTextureSurface", 0x134, 0x2),
    type="uint32_t",
    doc="Power-of-two texture width computed by D3D_CreateTextureSurface: rounds requested width up, clamps to 256, and mirrors height when device caps require square textures.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DDrawBackBuffer",
    xref("D3D_InitDirectDrawAndDirect3D", 0x14D, 0x1),
    type="DDraw_IDirectDrawSurface7*",
    doc="Attached DirectDraw back buffer used as the active D3D render target and screenshot source.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DDrawPrimarySurface",
    xref("D3D_InitDirectDrawAndDirect3D", 0x128, 0x1),
    type="DDraw_IDirectDrawSurface7*",
    doc="Primary/front DirectDraw surface created during D3D initialization and flipped/presented by frame rendering.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Camera_SetupClipPlanes_D3DState", xref("Camera_SetupClipPlanes", 0x5B, 0x2)
)
stable.data(
    "Graphics_ClipPolygonByCameraPyramid_LeftPlaneCoeff0",
    xref("Graphics_ClipPolygonByCameraPyramid", 0x6D, 0x1),
    doc="Left clip-plane coefficient 0 used by the camera-pyramid polygon clipper.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_LeftPlaneCoeff1",
    xref("Camera_SetupClipPlanes", 0xC6, 0x1),
    doc="Left clip-plane coefficient 1 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_LeftPlaneCoeff2",
    xref("Camera_SetupClipPlanes", 0xCF, 0x2),
    doc="Left clip-plane coefficient 2 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_RightPlaneCoeff0",
    xref("Camera_SetupClipPlanes", 0xF1, 0x2),
    doc="Right clip-plane coefficient 0 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_RightPlaneCoeff1",
    xref("Camera_SetupClipPlanes", 0x158, 0x2),
    doc="Right clip-plane coefficient 1 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_RightPlaneCoeff2",
    xref("Camera_SetupClipPlanes", 0x166, 0x1),
    doc="Right clip-plane coefficient 2 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_TopPlaneCoeff0",
    xref("Camera_SetupClipPlanes", 0x16F, 0x2),
    doc="Top clip-plane coefficient 0 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_TopPlaneCoeff1",
    xref("Camera_SetupClipPlanes", 0x191, 0x2),
    doc="Top clip-plane coefficient 1 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_TopPlaneCoeff2",
    xref("Camera_SetupClipPlanes", 0x1AF, 0x2),
    doc="Top clip-plane coefficient 2 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_BottomPlaneCoeff0",
    xref("Camera_SetupClipPlanes", 0x1B5, 0x1),
    doc="Bottom clip-plane coefficient 0 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_BottomPlaneCoeff1",
    xref("Camera_SetupClipPlanes", 0x1BA, 0x2),
    doc="Bottom clip-plane coefficient 1 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_BottomPlaneCoeff2",
    xref("Camera_SetupClipPlanes", 0x1C0, 0x2),
    doc="Bottom clip-plane coefficient 2 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Graphics_ClipPolygonByCameraPyramid_NearClipDistance",
    xref("Graphics_ClipPolygonByCameraPyramid", 0xA1, 0x2),
)
stable.data(
    "Camera_SetupClipPlanes_NearClipPlaneCoeff0",
    xref("Camera_SetupClipPlanes", 0x116, 0x1),
    doc="Near clip-plane coefficient 0 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_NearClipPlaneCoeff1",
    xref("Camera_SetupClipPlanes", 0x11F, 0x2),
    doc="Near clip-plane coefficient 1 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "Camera_SetupClipPlanes_NearClipPlaneCoeff2",
    xref("Camera_SetupClipPlanes", 0x141, 0x2),
    doc="Near clip-plane coefficient 2 written while building the camera clip-plane set.",
    unstable=True,
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelBlueShift",
    xref("D3D_CreateTextureSurface", 0x1C9, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelBlueBitsToDiscard",
    xref("D3D_CreateTextureSurface", 0x1F9, 0x2),
)
stable.data(
    "Graphics_D3DDevice7",
    xref("Material_ReleaseTextureArray", 0x3B, 0x1),
    type="D3D_IDirect3DDevice7*",
    doc=(
        "Global IDirect3DDevice7 pointer used across texture, render-state, blend, "
        "and material cleanup paths."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Graphics_ClipAndDrawPolygon_CurrentVertexFormat",
    xref("Graphics_ClipAndDrawPolygon", 0x1A5, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelRedMask",
    xref("D3D_CreateTextureSurface", 0x1E0, 0x1),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelGreenMask",
    xref("D3D_CreateTextureSurface", 0x1E7, 0x2),
)
stable.data(
    "Camera_SetupClipPlanes_NearClipDistanceSource",
    xref("Camera_SetupClipPlanes", 0xB, 0x2),
    doc="Near-clip distance source used while deriving clip-plane coefficients.",
    unstable=True,
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsTexturePow2Height",
    xref("D3D_CreateTextureSurface", 0x126, 0x1),
    type="uint32_t",
    doc="Power-of-two texture height computed by D3D_CreateTextureSurface: rounds requested height up, clamps to 256, and mirrors width when device caps require square textures.",
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsTextureSurfaceDesc",
    xref("D3D_CreateTextureSurface", 0x1AE, 0x1),
    type="uint32_t",
    doc="First word of the cached DDSURFACEDESC2 texture surface descriptor copied by D3D_CreateTextureSurface before IDirectDraw7::CreateSurface; SDK typed globals expose the base word.",
    write_policy=WritePolicy.RAW_MEMORY,
    unstable=True,
)
stable.data(
    "Script_ResolveVariableRef_Base",
    xref("Script_ResolveVariableRef", 0x45, 0x3),
)
stable.data(
    "D3D_InitDirectDrawAndDirect3D_DDrawZBuffer",
    xref("D3D_InitDirectDrawAndDirect3D", 0x2C8, 0x1),
    type="DDraw_IDirectDrawSurface7*",
    doc="DirectDraw z-buffer surface attached to the D3D render target and released during D3D/DirectDraw shutdown.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelRedBitsToDiscard",
    xref("D3D_CreateTextureSurface", 0x205, 0x2),
)
stable.data("Debug_Log_File", xref("Debug_Log", 0x0, 0x1))
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelRedShift",
    xref("D3D_CreateTextureSurface", 0x1F3, 0x2),
)
stable.data(
    "Graphics_ClipAndDrawPolygon_ReciprocalZ",
    xref("Graphics_ClipAndDrawPolygon", 0x611, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_State",
    xref("Level_InitializeActorSystem", 0x1EF, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelAlphaBitsToDiscard",
    xref("D3D_CreateTextureSurface", 0x20B, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelAlphaShift",
    xref("D3D_CreateTextureSurface", 0x1CF, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelAlphaMask",
    xref("D3D_CreateTextureSurface", 0x1B3, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelGreenShift",
    xref("D3D_CreateTextureSurface", 0x1ED, 0x2),
)
stable.data(
    "D3D_CreateTextureSurface_GraphicsPixelGreenBitsToDiscard",
    xref("D3D_CreateTextureSurface", 0x1FF, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_ObjectNodeRoot",
    xref("Level_InitializeActorSystem", 0x1A7, 0x1),
)
stable.data(
    "Checkers_UpdateStateMachine_DalmatianSpawnStates",
    xref("Checkers_UpdateStateMachine", 0x9, 0x1),
)
stable.data(
    "Menu_LoadLevelProgressState_SubmenuCount",
    xref("Menu_LoadLevelProgressState", 0x4A, 0x2),
)
stable.data(
    "Save_GameLevelCompletion_MenuItems",
    xref("Save_SaveGameLevelCompletion", 0x7B, 0x3),
)
stable.data(
    "Level_InitializeSaveState_MenuSlots", xref("Level_InitializeSaveState", 0xF6, 0x2)
)
stable.data(
    "Actor_UpdateAnimationState_ConfirmTextEnabled",
    xref("Actor_UpdateAnimationState", 0x2F8, 0x1),
)
stable.data(
    "Level_ResetBonusState_MenuInitialEntryFlag",
    xref("Level_ResetBonusState", 0xC, 0x3),
)
stable.data(
    "Checkers_UpdateStateMachine_PlayerIsHuman",
    xref("Checkers_UpdateStateMachine", 0x3D1, 0x3),
)
stable.data(
    "Level_InitializeSaveState_GameBonusProgressValue",
    xref("Level_InitializeSaveState", 0x1E5, 0x1),
)
stable.data(
    "Level_InitializeSaveState_GameStateInitFlag",
    xref("Level_InitializeSaveState", 0x1F0, 0x2),
)
stable.data(
    "Checkers_UpdateStateMachine_EnforceCaptureRule",
    xref("Checkers_UpdateStateMachine", 0x26D, 0x2),
)
stable.data(
    "Checkers_AIDifficulty",
    xref("Checkers_UpdateStateMachine", 0x598, 0x1),
    doc="Checkers AI difficulty selector read by the state machine.",
    unstable=True,
)
stable.data(
    "Checkers_UpdateStateMachine_CurrentPlayer",
    xref("Checkers_UpdateStateMachine", 0x105, 0x2),
    doc="Current checkers side: live play uses player values 1 and 2, toggled with xor 3, and is set to 0 for the no-move/end state.",
)
stable.data(
    "Script_OpPauseToggle_CameraTransitionCounter",
    xref("Script_OpPauseToggle", 0x199, 0x1),
)
stable.data(
    "Script_OpPauseToggle_CameraRotationAngle",
    xref("Script_OpPauseToggle", 0x19E, 0x1),
)
stable.data(
    "Script_OpPauseToggle_TargetRotationAngle",
    xref("Script_OpPauseToggle", 0x1A3, 0x2),
)
stable.data(
    "Script_OpPauseToggle_TargetYOffset",
    xref("Script_OpPauseToggle", 0x1A9, 0x2),
)
stable.data(
    "Script_OpPauseToggle_TargetDistance",
    xref("Script_OpPauseToggle", 0x1AF, 0x2),
)
stable.data("Script_OpPauseToggle_CameraFOV", xref("Script_OpPauseToggle", 0x1C3, 0x2))
stable.data(
    "Checkers_UpdateStateMachine_SaveGameWorld0CompletionBits",
    xref("Checkers_UpdateStateMachine", 0x518, 0x2),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_World1CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x5, 0x2),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_World2CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0xB, 0x2),
)
stable.data(
    "Save_SnapshotWorldCompletionBits_World3CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x16, 0x1),
)
stable.data(
    "Level_InitializeSaveState_BonusUnlocked",
    xref("Level_InitializeSaveState", 0xA9, 0x2),
)
stable.data("Trail_ResetBone_PathCount", xref("Trail_ResetBone", 0xC, 0x2))
stable.data(
    "Save_SnapshotWorldCompletionBits_World4CompletionBits",
    xref("Save_SnapshotWorldCompletionBits", 0x21, 0x2),
)
stable.data(
    "g_bonusInitialCode0",
    xref("Level_InitializeBonusData", 0x7B, 0x2),
    type="uint32_t",
    doc="First bonus initial code unpacked from bits 0 through 5.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "g_bonusInitialCode1",
    xref("Level_InitializeBonusData", 0x81, 0x2),
    type="uint32_t",
    doc="Second bonus initial code unpacked from bits 6 through 11.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "g_bonusInitialCode2",
    xref("Level_InitializeBonusData", 0x87, 0x1),
    type="uint32_t",
    doc="Third bonus initial code unpacked from bits 12 through 15.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Audio_TriggerMusicTransition_PKGResourceCurrentLevelData",
    xref("Audio_TriggerMusicTransition", 0x41, 0x1),
    type="Level_Data*",
    doc=(
        "This holds the active stable Level_Data pointer, which can be cast "
        "to the unstable Level_RuntimeData layout."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data(
    "Graphics_AdjustLevelScale_Factor",
    xref("Graphics_AdjustLevelScale", 0x3F, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_RenderDistance",
    xref("Level_InitializeActorSystem", 0x185, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_TransitionState",
    xref("Level_InitializeActorSystem", 0xE4, 0x2),
)
stable.data(
    "Script_OpPauseToggle_CurrentGameMode", xref("Script_OpPauseToggle", 0x0, 0x1)
)
stable.data(
    "Level_InitializeActorSystem_RenderDistanceQuarter",
    xref("Level_InitializeActorSystem", 0x1AF, 0x2),
)
stable.data(
    "Level_InitializeActorSystem_RenderDistanceThird",
    xref("Level_InitializeActorSystem", 0x1C1, 0x2),
)
stable.data(
    "Actor_ProcessPlayerBehavior_AIState0",
    xref("Actor_ProcessPlayerBehavior", 0x863, 0x1),
)
stable.data(
    "Actor_ProcessPlayerBehavior_RenderingDepthMode",
    xref("Actor_ProcessPlayerBehavior", 0x86E, 0x2),
)
stable.data(
    "Camera_UpdateFollow_DynamicLevelScale",
    xref("Camera_UpdateFollow", 0xA0B, 0x1),
    type="int32_t",
    doc=(
        "Integer runtime level/render scale. Camera_UpdateFollow writes this value shifted left "
        "by 12 into Graphics_ListState.dynamic_level_scale."
    ),
    write_policy=WritePolicy.RAW_MEMORY,
    stable=True,
)
stable.data(
    "Audio_TriggerMusicTransition_Active",
    xref("Audio_TriggerMusicTransition", 0x29, 0x2),
    type="uint8_t",
    doc="Byte-sized music transition active/selected-state flag read by Audio_TriggerMusicTransition.",
    write_policy=WritePolicy.RAW_MEMORY,
)
stable.data(
    "Graphics_RenderMeshNode_WorldTransformPtr",
    xref("Graphics_RenderMeshNode", 0xB, 0x2),
    type="Math_TransformQ12Q6*",
    doc="Current compact Q12-rotation/Q6-translation transform used by render and trail paths.",
    write_policy=WritePolicy.ENGINE_MANAGED,
    unstable=True,
)
stable.data(
    "Actor_ProcessRendering_CurrentRenderActor",
    xref("Actor_ProcessRendering", 0x8, 0x2),
    type="Actor_State*",
    doc=(
        "Render-scoped actor pointer published and cleared by Actor_ProcessRendering. "
        "Movement/collision hooks use DTTR_UtilGetActiveActor or "
        "current_level_data->Level_RuntimeData.entity_array->Entity_State.active_actor for "
        "current-player/current-entity authority."
    ),
    write_policy=WritePolicy.ENGINE_MANAGED,
    stable=True,
)
stable.data("Script_OpPauseToggle_State", xref("Script_OpPauseToggle", 0x1B, 0x1))
stable.data(
    "Script_OpPauseToggle_SavedActorWorldRenderPosX",
    xref("Script_OpPauseToggle", 0x29, 0x2),
    doc="Saved active actor world_render_pos_x used for pause/menu distance checks.",
)
stable.data(
    "Script_OpPauseToggle_SavedActorWorldRenderPosY",
    xref("Script_OpPauseToggle", 0x32, 0x2),
    doc="Saved active actor world_render_pos_y used for pause/menu distance checks.",
)
stable.data(
    "Script_OpPauseToggle_SavedActorWorldRenderPosZ",
    xref("Script_OpPauseToggle", 0x3B, 0x1),
    doc="Saved active actor world_render_pos_z used for pause/menu distance checks.",
)
stable.data(
    "Projectile_LiveActorListHead",
    xref("Trail_SpawnFromEntry", 0x5C, 0x1),
    doc=(
        "Live projectile Actor_State linked-list head populated by Trail_SpawnFromEntry "
        "and walked by Actor_UpdateProjectileList."
    ),
)
stable.data(
    "File_SeekAndGetPosition_DescriptorTable",
    xref("File_SeekAndGetPosition", 0x5C, 0x3),
)
stable.data(
    "File_FlushToDisk_IoBufferHighWaterMark", xref("File_FlushToDisk", 0x4, 0x2)
)
stable.data(
    "Mem_InitializeHeapAllocator_MaxSegments",
    xref("Mem_InitializeHeapAllocator", 0x32, 0x2),
)
stable.data(
    "Mem_FreeHeapBlock_LastFreedPageIndex", xref("Mem_FreeHeapBlock", 0x239, 0x2)
)
stable.data(
    "Mem_InitializeHeapAllocator_SegmentTableCached",
    xref("Mem_InitializeHeapAllocator", 0x2D, 0x1),
)
stable.data(
    "Mem_InitializeHeapAllocator_LastFreedSegment",
    xref("Mem_InitializeHeapAllocator", 0x1D, 0x2),
)
stable.data(
    "Mem_InitializeHeapAllocator_SegmentCount",
    xref("Mem_InitializeHeapAllocator", 0x24, 0x2),
)
stable.data(
    "Mem_InitializeHeapAllocator_State",
    xref("Mem_InitializeHeapAllocator", 0x15, 0x1),
)
stable.data(
    "Video_PlayMovieLoop_GetAsyncKeyStateThunk",
    xref("Video_PlayMovieLoop", 0x29, 0x2),
    type="void*",
    doc="Movie-loop input import cell for GetAsyncKeyState-style key polling; the API binding remains unresolved.",
    unstable=True,
)
stable.data(
    "Audio_ShutdownSystem_AILReleaseSampleHandle",
    xref("Audio_ShutdownSystem", 0xA, 0x2),
    doc="Miles AIL sample-handle release callsite/data reference used during audio shutdown.",
)
stable.data("Audio_ShutdownSystem_AILShutdown", xref("Audio_ShutdownSystem", 0x37, 0x2))

# Lower-confidence rows stay in the blueprint and carry unstable=True.
_unstable_rows = Blueprint("unstable")

_unstable_rows.fn(
    "Component_UpdateProjectileLogic",
    "10 85 C0 74 ?? 57 E8 ??",
    match=-0x34,
    hook=0x6,
    ret="void",
    params=[
        param(
            "Component_Instance*",
            "comp",
            doc=(
                "Runtime component instance whose spawn_context, definition, owner_actor_ref, "
                "projectile timers, homing fields, and component state drive projectile behavior."
            ),
        )
    ],
    doc="Preprocess slot 2. Updates projectile homing, gravity, tracking, return-to-owner, and timer state while the indexed dispatcher ignores EAX.",
)


_unstable_rows.fn(
    "Collision_ProcessPowerupCollisions",
    "83 EC 1C A1 ?? ??",
    hook=0x8,
    ret="void",
    params=[param("Actor_State*", "actor")],
    doc="Scans live powerups, invokes their collision filter, and resolves accepted actor pairs.",
)

_unstable_rows.fn(
    "Actor_HandleCollisionResponse",
    "00 00 83 FF 06 0F 87 ??",
    match=-0x17,
    ret="void",
    params=[
        param("Actor_State*", "actor"),
        param("Actor_State*", "other_actor"),
        param("Collision_Polygon*", "collision_poly"),
        param("int32_t", "collision_slot"),
    ],
    doc="Records class-specific deferred state. Selector 0x0D writes force-emitter slot 0, while actor byte +0x67 is a role overlay.",
)

_unstable_rows.fn(
    "Powerup_UpdateSpawnLogic",
    "83 EC 0C A1 ?? ??",
    hook=0x8,
    ret="void",
    params=[],
    doc="Side-effect-only spawn scheduler that dereferences current level data before checking count, then walks exactly powerup_count entries. Positive count requires a nonnull list, and flag 0x08 requires a nonnull parent.",
)


stable.data(
    "Graphics_AdjustLevelScale_ListState",
    xref("Graphics_AdjustLevelScale", 0x65, 0x2),
    type="Graphics_ListState*",
    unstable=True,
    doc="Data pointer to active Graphics_ListState used when Graphics_AdjustLevelScale writes "
    "dynamic level scale.",
    write_policy=WritePolicy.ENGINE_MANAGED,
)

_unstable_rows.struct(
    "Graphics_ListState",
    member(
        "int16_t",
        "pose_state_flags",
        0x0,
        doc="Camera pose/state flag word (previously misread as a yaw angle).",
    ),
    member("int16_t", "pitch", 0x2),
    member("int16_t", "look_pitch", 0x4),
    member("int16_t", "look_yaw", 0x6),
    member("int16_t", "view_roll", 0x8),
    member("int16_t", "fov", 0xA),
    member("int32_t", "focal_distance", 0xC),
    member("Math_Vec3I32XZY", "eye_pos", 0x10),
    member("Math_Vec3I32XZY", "target_pos", 0x1C),
    member("Math_ViewportI16", "viewport", 0x28),
    member(
        "Camera_FrustumDirTable",
        "frustum_dirs",
        0x30,
        doc=(
            "Five int16 frustum direction triples (stride 8) written by "
            "Camera_BuildViewMatrix; previously misread as a view matrix plus setup prefix."
        ),
    ),
    member(
        "Graphics_FrustumClipPlane",
        "frustum_planes[5]",
        0x58,
        doc="Five plane records written by Scene_RenderFrame and read by Graphics_CheckActorVisibilityAndFrustum.",
    ),
    member(
        "Math_Matrix3x3I16",
        "node_view_matrix",
        0x94,
        doc="Per-node camera basis matrix in the engine's native int16 3x3 format.",
    ),
    member("int16_t", "node_view_matrix_padding", 0xA6),
    member("Math_Vec3I32", "node_view_translation", 0xA8),
    member(
        "int32_t",
        "projection_near_fp",
        0xB4,
        doc="Fixed-point near-plane threshold.",
    ),
    member("int32_t", "dynamic_level_scale", 0xB8),
    member("Actor_State*", "render_actor_ptr", 0xBC),
    member("uint32_t", "render_pass_flags", 0xC0),
    member(
        "Graphics_Render_ListCallback",
        "post_sorted_callback",
        0xC4,
        doc=(
            "Fixed nullable scene post-sort slot, not a selector or count. Installed as "
            "Game_RenderOverlays during resource activation, invoked after sorted scene lists, and "
            "cleared during resource unload."
        ),
    ),
    member(
        "Graphics_Render_ListCallback",
        "pre_shadow_callback",
        0xC8,
        doc=(
            "Fixed nullable pre-list slot, not a selector or count. Installed as "
            "Graphics_ClearBackground during resource activation, invoked before scene lists and "
            "before the menu's sorted-list pass, and cleared during resource unload."
        ),
    ),
    member("Graphics_PolygonBatchRecord*", "sorted_list_head", 0xCC),
    member("Graphics_PolygonBatchRecord*", "sorted_list_buckets[16384]", 0xD0),
    member(
        "uint32_t",
        "sorted_bucket_tail",
        0x100D0,
        doc="End/tail dword after the 16384 sorted bucket pointers.",
    ),
    member("uint16_t", "shake_countdown", 0x100D4),
    member("int16_t", "shake_intensity", 0x100D6),
    member("uint16_t", "roll_effect_countdown", 0x100D8),
    member("int16_t", "roll_effect_duration", 0x100DA),
    member(
        "int16_t",
        "roll_effect_magnitude",
        0x100DC,
        doc="Peak roll angle applied by Camera_UpdateRollEffect while the effect runs.",
    ),
    size=0x100DE,
    doc=(
        "Runtime state for the camera and render lists, including five frustum "
        "planes and the camera shake/roll-effect tail. eye_pos and target_pos "
        "use Math_Vec3I32XZY storage."
    ),
)

_unstable_rows.fn(
    "PKG_ReturnResourceAlwaysTrue",
    "B0 01 C3 90 90 90 90 90 90 90 90 90 90 90 90 90 81 EC 10 01 00 00 57 68 ?? ?? ?? ?? 68 04 01 00 00 FF 15 ?? ?? ?? ??",
    required=Required.EN,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    callable=False,
    public=False,
    ret="uint8_t",
    params=[],
    doc="Private unsupported package-resource readiness stub that always returns 1 in AL. Kept only to preserve internal resolver references.",
)

_unstable_rows.fn(
    "Runtime_NoOpStub",
    "C3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 8B 44 24 04 A3 ?? ?? ?? ??",
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    callable=False,
    public=False,
    ret="void",
    params=[],
    doc="Private unsupported shared no-op callback. A single RET with no standalone return value, retained only for internal resolver references.",
)

_unstable_rows.fn(
    "CRT_ProbeRead4",
    "25 ?? ?? ?? ?? 83 EC 10",
    match=-0x18,
    cc=CallingConvention.STDCALL,
    hook=hook(0x0, kind=HookKind.UNSUPPORTED),
    callable=False,
    public=False,
    ret="int32_t",
    params=[param("void const*", "address")],
    doc="Returns 1 after reading four bytes from address, or 0 on an access violation.",
    abi_status=AbiStatus.VERIFIED,
)

# Lower-confidence structured types: resolved, but not documented enough for
# stable=True.
_unstable_rows.struct(
    "Level_State",
    member("void*", "collision_data", 0x0),
    member(
        "Actor_State*",
        "actor_list",
        0x4,
        doc="Level-local live Actor_State list/table pointer.",
    ),
    member("void*", "trigger_list", 0x8),
    member("PKG_CameraDef*", "camera_data", 0xC),
    member("Material_Entry*", "material_table", 0x10),
    member("uint32_t", "reserved_01", 0x14),
    member("uint32_t", "flags", 0x18),
    member("uint16_t", "actor_count", 0x1C),
    member("uint16_t", "material_count", 0x1E),
    member("char*", "string_table", 0x20),
    size=0x24,
)

_unstable_rows.struct(
    "Level_RuntimeData",
    member("PKG_CameraDef*", "cam_default", 0x0),
    member("PKG_CameraDef*", "cam_current", 0x4),
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
            "identify Entity_State slots within the currently loaded level."
        ),
    ),
    member(
        "Audio_SoundDefinition*",
        "sound_list",
        0x10,
        doc="Current-level sound-definition array, indexed by level-local sound operands.",
    ),
    member(
        "int16_t",
        "sound_or_var_count",
        0x14,
        doc="Count interpreted as sound definitions or script variables, depending on the level-data path.",
    ),
    member(
        "int16_t",
        "var_count",
        0x16,
        doc="Script variable count halfword, stored separately from sound_definition_count.",
    ),
    member("int32_t*", "var_list", 0x18),
    member("int16_t", "powerup_count", 0x1C),
    member("int16_t", "sound_def_count", 0x1E),
    member(
        "Powerup_Entry*",
        "powerup_list",
        0x20,
        doc="Current-level powerup spawn-record list keyed by powerup_count.",
    ),
    member(
        "PKG_ActorTemplate*",
        "powerup_actor_template_slots[16]",
        0x24,
        doc=(
            "Fixed 16-slot powerup actor-template/clone-source table. "
            "PKG_FixUpResourceLevelPointers fixes each non-null slot with PKG_FixUpResourceActorRecordPointers; "
            "Powerup_CloneActorFromTemplate reads these PKG_ActorTemplate* sources when creating spawned powerup actors."
        ),
    ),
    member("char*", "themes[5]", 0x64),
    member("int32_t", "theme_count", 0x78),
    member("Trail_Entry*", "trail_list", 0x7C),
    member("PKG_SpriteEntry*", "sprite_list", 0x80),
    member("Nav_Network*", "nav_net", 0x84),
    member("PKG_UsableMaterialEntry*", "usable_materials", 0x88),
    size=0x8C,
    doc="Concrete runtime level-data block carried by Level_Data* APIs.",
)

_unstable_rows.struct(
    "Powerup_Entry",
    member(
        "Actor_State*",
        "attach_parent_actor",
        0x0,
        doc="Parent live actor this powerup entry is attached to (previously misread as a template/record pointer).",
    ),
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
    member(
        "int16_t",
        "spawn_signal_id",
        0xE,
        doc="Signal id raised on spawn (previously misread as a max spawn count).",
    ),
    member("Math_Vec3I32", "pos", 0x10),
    size=0x1C,
    doc="Level_RuntimeData.powerup_list entry walked by Powerup_UpdateSpawnLogic with attach_parent_actor, flags, spawn_signal_id, pos vector field.",
)

_unstable_rows.struct(
    "Mesh_TransformEntry",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "flags", 0x1),
    member("char", "bone_index", 0x2),
    member(
        "uint8_t",
        "signal_id_hi",
        0x3,
        doc="High byte of the generic mesh-command signal_id word. Transform-specific consumers leave it reserved.",
    ),
    member("uint32_t", "resource_ptr", 0x4),
    member("int16_t", "poly_start_index", 0x8),
    member("int16_t", "poly_count", 0xA),
    member(
        "int16_t",
        "payload_word_0_c",
        0xC,
        doc=(
            "Variant mesh-command payload start; type 0 passes cmd to Animation_ProcessController, while other command types reinterpret the payload."
        ),
    ),
    member("int16_t", "effect_count", 0xE),
    member("Math_Vec2I16", "scale", 0x10),
    size=0x14,
)

_unstable_rows.struct(
    "Video_PlaybackBuffer",
    member("uint8_t", "decoder_state[396]", 0x0),
    member("char", "movie_alias[64]", 0x18C),
    member("uint8_t", "decode_scratch[64]", 0x1CC),
    member("uint8_t", "frame_pixel_data[572]", 0x20C),
    member("int32_t*", "callback_context", 0x448),
    size=0x44C,
    doc="Movie playback buffer state, covering frame reads, input, and close/skip handling.",
)

_unstable_rows.struct(
    "PKG_MeshNodeHeader",
    member("uint32_t", "node_type", 0x0),
    member("uint32_t", "parent_index", 0x4),
    member("uint32_t", "node_data_offset", 0x8),
    member("uint32_t", "link_data", 0xC),
    member("uint32_t", "bone_transforms[12]", 0x10),
    member("Math_RectI16", "bounds", 0x40),
    member("uint32_t", "mesh_flags", 0x48),
    member("uint32_t", "mesh_config[3]", 0x4C),
    member("uint32_t", "visibility_mask", 0x58),
    member(
        "uint16_t",
        "padding_5c",
        0x5C,
        doc="Padding/internal mesh-node header word.",
    ),
    member(
        "uint16_t",
        "padding_5e",
        0x5E,
        doc="Padding/internal mesh-node header word.",
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
    member("Math_Vec3U", "cached_world_pos", 0x98),
    member("uint32_t", "bsphere_packed_xy", 0xA4),
    member("uint32_t", "bsphere_packed_zr", 0xA8),
    member("uint32_t", "bone_ref_array_ptr", 0xAC),
    member("uint32_t", "morph_target_list_ptr", 0xB0),
    member("uint32_t", "render_batch_array_ptr", 0xB4),
    member("uint16_t", "draw_order_flags", 0xB8),
    member(
        "uint8_t",
        "render_node_entry_count",
        0xBA,
        doc="Render-node entry count , passed to render-entry fixup.",
    ),
    member(
        "uint8_t",
        "lod_count",
        0xBB,
        doc="LOD entry count at lod_array_ptr, used while rebasing LOD entries.",
    ),
    member(
        "Mesh_RenderNodeEntry*",
        "render_node_entry_table_ptr",
        0xBC,
        doc="Mesh render-node entry table used by mesh-node fixup and render paths.",
    ),
    member("uint32_t", "lod_array_ptr", 0xC0),
    member("uint32_t", "default_vertex_color", 0xC4),
    member("uint32_t", "bone_data_ptr", 0xC8),
    member("uint32_t", "material_batch_base", 0xCC),
    member("uint32_t", "component_list_ptr", 0xD0),
    member("uint32_t", "init_world_pos_z", 0xD4),
    member("uint32_t", "bounding_radius", 0xD8),
    member("uint32_t", "runtime_anim_timer", 0xDC),
    member("Math_ColorRGBAI32", "runtime_transform", 0xE0),
    member("uint16_t", "strip_vertex_count", 0xF0),
    member("int16_t", "aux_entry_count", 0xF2),
    member("uint32_t", "special_node_data_ptr", 0xF4),
    member("uint32_t", "global_material_ref_ptr", 0xF8),
    member(
        "uint32_t",
        "padding_fc",
        0xFC,
        doc="Reserved mesh-node header dword. It is reserved for internal use.",
    ),
    size=0x100,
)

_unstable_rows.struct(
    "PKG_LODEntry",
    member("int16_t", "lod_level", 0x0),
    member("int16_t", "sprite_layer_count", 0x2),
    member("Actor_State*", "render_data_ptr", 0x4),
    member("int16_t", "rot_angle_x", 0x8),
    member("int16_t", "face_count", 0xA),
    member(
        "int16_t",
        "lod_reserved_0_c",
        0xC,
        doc="Internal LOD descriptor word used by render selection with lod_level, sprite_layer_count, render_data_ptr, face_count/start, and threshold.",
    ),
    member("uint16_t", "lod_distance_threshold", 0xE),
    member("int16_t", "face_start_index", 0x10),
    member(
        "int16_t",
        "lod_reserved_12",
        0x12,
        doc="Reserved LOD descriptor word reserved for internal use.",
    ),
    member(
        "int32_t",
        "lod_reserved_14",
        0x14,
        doc="Reserved LOD descriptor dword reserved for internal use.",
    ),
    member(
        "int32_t",
        "lod_reserved_18",
        0x18,
        doc="Reserved LOD descriptor dword reserved for internal use.",
    ),
    member(
        "int32_t",
        "lod_reserved_1_c",
        0x1C,
        doc="Reserved LOD descriptor dword reserved for internal use.",
    ),
    member(
        "int16_t",
        "lod_reserved_20",
        0x20,
        doc="Reserved LOD descriptor word reserved for internal use.",
    ),
    member(
        "int16_t",
        "lod_padding_22",
        0x22,
        doc="Reserved word before the relocated slot reserved for internal use.",
    ),
    member(
        "int32_t",
        "lod_relocated_ptr_24",
        0x24,
        doc="Relocated pointer and position slot in LOD data rebased by PKG_FixUpResourceObjectNodeType3ComplexActorLike and Actor_CloneTemplateWithTemplateRelativeFixups.",
    ),
    size=0x28,
)

_unstable_rows.struct(
    "PKG_MeshOffsetTable",
    member("uint32_t", "mesh_offsets[16]", 0x0),
    member("uint8_t", "offset_padding[64]", 0x40),
    size=0x80,
)

_unstable_rows.struct(
    "PKG_SpriteEntry",
    member("uint8_t", "type", 0x0),
    member("uint8_t", "layer_index", 0x1),
    member("uint16_t", "control_flags", 0x2),
    member("Material_Entry*", "texture_db1", 0x4),
    member("Graphics_SpriteContext*", "sprite_ctx1", 0x8),
    member("Animation_FrameData*", "anim_frames1", 0xC),
    member("Material_Entry*", "texture_db2", 0x10),
    member("Graphics_SpriteContext*", "sprite_ctx2", 0x14),
    member("Animation_FrameData*", "anim_frames2", 0x18),
    member("Scene_Node*", "scene_node_ref", 0x1C),
    member(
        "Math_Vec2I16",
        "move_start",
        0x20,
        doc="Move-tween source X/Y position written by Script_OpAnimateSpriteMove.",
    ),
    member("Math_Vec2I16", "move_target", 0x24, doc="Move-tween target X/Y position."),
    member("Math_RangeI32", "move_frame_range", 0x28),
    member(
        "Math_EasePairI32",
        "move_ease",
        0x30,
        doc="Move-tween ease-in/ease-out percentages, stored in fp12 units.",
    ),
    member("Math_Vec2I32", "scale_start", 0x38),
    member("Math_Vec2I32", "scale_target", 0x40),
    member("Math_RangeI32", "scale_frame_range", 0x48),
    member(
        "Math_EasePairI32",
        "scale_ease",
        0x50,
        doc="Scale-tween ease-in/ease-out percentages, stored in fp12 units.",
    ),
    member(
        "Math_ColorRGB8",
        "color_start",
        0x58,
        doc="Color-tween source RGB snapshot.",
    ),
    member("uint8_t", "color_start_reserved_high", 0x5B),
    member(
        "int32_t",
        "color_target_word",
        0x5C,
        doc="Color-tween target/current color word, with low 24-bit RGB documented.",
    ),
    member("Math_RangeI32", "color_frame_range", 0x60),
    member(
        "Math_EasePairI32",
        "color_ease",
        0x68,
        doc="Color-tween ease-in/ease-out percentages, stored in fp12 units.",
    ),
    member("int32_t", "rotation_angle_fp12", 0x70),
    member("Math_RectI16", "bounds", 0x74),
    member(
        "uint8_t",
        "link_index",
        0x7C,
        doc="Index of the linked sprite entry this entry follows.",
    ),
    member(
        "uint8_t",
        "anchor_code",
        0x7D,
        doc="Anchor/alignment code selecting how the screen position is derived.",
    ),
    member("uint8_t", "state_flags", 0x7E),
    member("uint8_t", "state_flags_reserved", 0x7F),
    member(
        "Math_ColorRGB8",
        "current_color",
        0x80,
        doc="Current/fallback render color.",
    ),
    member("uint8_t", "current_color_reserved_high", 0x83),
    member(
        "int32_t",
        "rotation_start_angle",
        0x84,
        doc="Current/start rotation angle, eased toward rotation_target_angle.",
    ),
    member(
        "int32_t",
        "rotation_target_angle",
        0x88,
        doc="Target rotation angle, stored in fp12 units.",
    ),
    member("Math_RangeI32", "rotation_frame_range", 0x8C),
    member(
        "Math_EasePairI32",
        "rotation_ease",
        0x94,
        doc="Rotation-tween ease-in/ease-out percentages, stored in fp12 units.",
    ),
    member("Math_Vec2I32", "current_scale", 0x9C),
    member("Math_ScreenPointI16", "screen", 0xA4),
    member(
        "int16_t",
        "sort_key",
        0xA8,
        doc="Sprite depth/sort key, compared by UI_CompareSpriteDepth.",
    ),
    member("int16_t", "reserved_aa", 0xAA),
    size=0xAC,
    doc=(
        "Sprite/UI entry with two texture/context/frame layers plus move, scale, color, "
        "and rotation tween blocks; mutated by Script_OpSetSpriteProperty and "
        "Script_OpAnimateSpriteMove."
    ),
)

_unstable_rows.struct(
    "PKG_TrailListEntry",
    member("uint16_t", "count", 0x0),
    member("uint16_t", "reserved", 0x2),
    member("Component_TrailObject*", "ptr", 0x4),
    size=0x8,
)

_unstable_rows.struct(
    "PKG_UILayoutResource",
    member("uint32_t", "checksum", 0x0),
    member("uint32_t", "entry_count", 0x4),
    member("uint8_t", "layout_padding[12]", 0x8),
    size=0x14,
)

_unstable_rows.struct(
    "PKG_Header",
    member("PKG_TOCEntry", "entries[138]", 0x0),
    member(
        "uint8_t",
        "header_reserved[944]",
        0x450,
        doc="Unparsed package-header tail, left after PKG_OpenAndReadTOC copies only the first storage of the header.",
    ),
    size=0x800,
)

_unstable_rows.struct(
    "Scene_NodePayload",
    member("Scene_Node*", "parent_node_ptr", 0x0),
    member("Scene_Node*", "child_list_head", 0x4),
    member("Scene_Node*", "sibling_link", 0x8),
    member("Scene_LocalTransform", "transform", 0xC),
    member("uint8_t", "node_type", 0x1A),
    member("uint8_t", "padding_1b[1]", 0x1B),
    member("uint16_t", "padding", 0x1C),
    member("uint8_t", "padding_1e[2]", 0x1E),
    size=0x20,
    doc="Compact scene-node payload/resource-record prefix shared by older Group, Model, and "
    "Object shapes.",
)

_unstable_rows.struct(
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
    size=0x24,
    doc="Compact scene sub-node payload/resource-record variant.",
)

stable.signatures.extend(_unstable_rows.signatures)
stable.functions.extend(_unstable_rows.functions)
stable.globals.extend(_unstable_rows.globals)
stable.types.extend(_unstable_rows.types)

BLUEPRINT = stable
