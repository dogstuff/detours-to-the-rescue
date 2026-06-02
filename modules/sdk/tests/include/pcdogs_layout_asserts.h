#ifndef DTTR_TEST_PCDOGS_LAYOUT_ASSERTS_H
#define DTTR_TEST_PCDOGS_LAYOUT_ASSERTS_H

#include <stddef.h>

#define SDK_LAYOUT_OFFSET(type, field, value)                                            \
	_Static_assert(offsetof(type, field) == (value), #type "." #field " offset")
#define SDK_LAYOUT_SIZE(type, value)                                                     \
	_Static_assert(sizeof(type) == (value), #type " size")

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Animation_ChainEntry, 32);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Animation_ChainEntry, state_flags, 0x9);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Animation_ChainEntry, blend_mode, 0xB);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Checkers_Board, 32);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Checkers_Board, cells, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_PolygonBatchRecord, 136);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_PolygonBatchRecord, screen_vertices, 0x14);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_PolygonBatchRecord, tex_coords, 0x7C);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_WorkArea, 1656);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_WorkArea, color_channels, 0x648);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Math_Vec2I16, 4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_Vec2I16, y, 0x2);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Math_Vec2I32, 8);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_Vec2I32, y, 0x4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Math_Vec3I32XZY, 12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_Vec3I32XZY, z, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_Vec3I32XZY, y, 0x8);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Camera_FrustumClipPlane, 16);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_FrustumClipPlane, distance, 0xC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Math_BoundingSphereU16, 8);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_BoundingSphereU16, radius, 0x6);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_TextureInfo, 4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_TextureInfo, dimensions, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_TexWrapMode, 4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_TexWrapMode, mode, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Actor_AnimationComponentState, 4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_AnimationComponentState, component_counts, 0x2);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Camera_Frustum, 104);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Frustum, clip_planes, 0x0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Frustum, state_flags, 0x50);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Frustum, viewport_pos, 0x5C);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Camera_RenderData, 196);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_RenderData, position_xy, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_RenderData, target_x, 0x18);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_RenderData, viewport_pos, 0x20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_RenderData, view_translation_xy, 0x8C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_RenderData, position_z, 0x94);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Camera_Pose, 36);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Pose, eye_pos, 0xC);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Pose, target_pos, 0x18);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_D3D_DriverInfo, 11148);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_D3D_DriverInfo, capability_flags, 0x4B0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_D3D_DriverInfo, display_mode_workspace, 0x4BC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_Descriptor, 12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_Descriptor, dimensions_minus_1, 0x2);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_RuntimeDescriptor, 116);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_RuntimeDescriptor, dimensions_minus_1, 0x2);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_Entry, 20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_Entry, dimensions_minus_1, 0x2);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_DataRef, 24);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_DataRef, actual_dimensions, 0x8);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_BlendTextureSet, 16);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_BlendTextureSet, quadrants, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_ActorTemplate, 20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorTemplate, lod_nodes, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_CollisionFacePlane, 20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_CollisionFacePlane, adj_edges, 0xC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_PolygonData, 36);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_PolygonData, texture_info, 0xC);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_PolygonData, uv_tile_offset, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_PolygonData, padding_12, 0x12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_PolygonData, explicit_uv, 0x1C);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_PolygonDataRaw, 24);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_PolygonDataRaw, vertex_indices, 0x4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_ClipAttribute, 12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ClipAttribute, components, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Material_TableEntry, 36);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_TableEntry, flags_bytes, 0x2);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_TableEntry, texture_info, 0xC);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_TableEntry, uv_tile_offset, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_TableEntry, explicit_uv, 0x1C);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_PolygonBatchRecord, tex_wrap_mode, 0xC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Actor_State, 452);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, contact_tangent, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, contact_normal_z, 0x1C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, visual_scale, 0x68);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, animation_component_state, 0xB8);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Scene_Node, 220);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Scene_Node, visibility_flags, 0x78);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Scene_Node, sort_keys, 0x9C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Scene_Node, child_node_list_ptr, 0xA4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Component_TrailObject, 32);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_TrailObject, bone_offset, 0x0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_TrailObject, bone_index, 0x6);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_Instance, local_pos_xy, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_Instance, spawn_count_byte_0, 0x48);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_Instance, lod_node_ptrs, 0x60);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Collision_Node, 140);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Collision_Node, origin, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Collision_Node, reserved_4c, 0x4C);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_SpriteNodeData, 20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_SpriteNodeData, bound_extent, 0x10);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Camera_Runtime, 108);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Runtime, eye_pos, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Runtime, target_pos, 0x1C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Runtime, active_entity_slot_ptr, 0x28);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Runtime, frustum_planes, 0x44);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Camera_Runtime, frustum_plane_3_x, 0x68);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Graphics_QuadRenderData, 104);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_QuadRenderData, projected_vertices, 0x14);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_QuadRenderData, vertex_3_screen_x, 0x2C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_QuadRenderData, vertex_3_screen_y, 0x2E);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_QuadRenderData, sort_data, 0x30);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_QuadRenderData, vertex_0_x, 0x38);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Animation_PositionKeyframe, 40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Animation_PositionKeyframe, pos, 0x4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Audio_SoundEntry, 48);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Audio_SoundEntry, listener_pos, 0x10);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Component_Instance, 548);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_Instance, world_pos, 0x98);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Component_Instance, homing_vel, 0xD4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Mesh_Node, 32);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Mesh_Node, position, 0x10);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Mesh_NodeExtended, 220);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Mesh_NodeExtended, world_pos, 0x40);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_CollisionHeader, 32);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_CollisionHeader, dimensions, 0x0);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Trail_Segment, 52);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Trail_Segment, start, 0x24);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, position, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, world_render_pos, 0x98);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Entity_State, bonus_respawn_pos, 0x4);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Scene_Node, world_delta_pos, 0x4C);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Input_JoystickState, rot, 0xC);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, visual_scale, 0x68);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Actor_State, scale_factor, 0xCE);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Level_RuntimeData, themes, 0x64);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ListState, eye_pos, 0x10);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ListState, target_pos, 0x1C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ListState, active_entity_slot_ptr, 0x28);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ListState, frustum_planes, 0x58);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Graphics_ListState, node_view_translation, 0xA8);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Powerup_Entry, pos, 0x10);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Material_EntryFull, dimensions, 0xC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Math_RectI16, 8);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_RectI16, min_x, 0x0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_RectI16, min_y, 0x2);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_RectI16, max_x, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Math_RectI16, max_y, 0x6);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_SpriteMaterialLayer, 12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteMaterialLayer, texture_db, 0x0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteMaterialLayer, material, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteMaterialLayer, anim_frames, 0x8);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, 452);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, component_node_ptrs, 0x50);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, default_home, 0x68);
SDK_LAYOUT_OFFSET(
	DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout,
	default_movement_params,
	0x78
);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, default_facing_angle, 0x88);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, default_extra, 0xB4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, home_pos, 0xD0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, live_movement_params, 0xE0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, live_facing_angle, 0xF0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, live_extra, 0x11C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, path_target, 0x148);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, path_result_x, 0x154);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, runtime_state, 0x180);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, runtime_counter, 0x194);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_Mesh_TransformEntry, 20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_Mesh_TransformEntry, scale, 0x10);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_MeshNodeHeader, 260);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, bounds, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, bounding_sphere, 0xA4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, runtime_transform, 0xE0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, strip_vertex_count, 0xF0);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, bone_ref_array_ptr, 0xAC);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_SpriteLayerBinding, 12);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteLayerBinding, sprite_context_ptr, 0x4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, 172);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, material_layers, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, scene_node_ref, 0x1C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, base, 0x20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, offset, 0x24);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, src_scale, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, clip, 0x74);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, cur_scale, 0x9C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntryAltLayout, screen, 0xA4);

SDK_LAYOUT_SIZE(DTTR_PCDOGS_T_PKG_SpriteEntry, 172);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layers, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, sprite_resource_index, 0x1C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, movement_source, 0x20);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layer_0_transform, 0x28);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layer_0_anim_state, 0x3C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layer_0_scale, 0x40);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layer_1_scale, 0x9C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_SpriteEntry, layer_1_texture_ptr_2, 0xA4);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, camera_pos, 0x4);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, default_ref_pos, 0x5C);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_ActorRecordUnstableLayout, live_ref_pos, 0xC4);

SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, cached_world_pos, 0x98);
SDK_LAYOUT_OFFSET(DTTR_PCDOGS_T_PKG_MeshNodeHeader, relative_offset_list_ptr, 0x100);

#undef SDK_LAYOUT_SIZE
#undef SDK_LAYOUT_OFFSET

#endif
