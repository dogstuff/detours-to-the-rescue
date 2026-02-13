#include "graphics_internal.h"

#include "log.h"

#include <stddef.h>

#include "gen/shaders.h"

// Holds shader bytecode pointer/size pairs for SDL pipeline creation
typedef struct {
	const Uint8 *code;
	size_t size;
} S_GraphicsShaderBlob;

// Returns a readable shader-stage name for diagnostics
static const char *s_shader_stage_name(SDL_GPUShaderStage stage) {
	switch (stage) {
	case SDL_GPU_SHADERSTAGE_VERTEX:
		return "vertex";
	case SDL_GPU_SHADERSTAGE_FRAGMENT:
		return "fragment";
	default:
		return "compute";
	}
}

// Returns the embedded basic vertex shader blob for the selected format
static S_GraphicsShaderBlob s_get_basic_vert_blob(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return (S_GraphicsShaderBlob){basic_vert_spv, (size_t)basic_vert_spv_len};
	case SDL_GPU_SHADERFORMAT_DXIL:
		return (S_GraphicsShaderBlob){basic_vert_dxil, (size_t)basic_vert_dxil_len};
	case SDL_GPU_SHADERFORMAT_MSL:
		return (S_GraphicsShaderBlob){basic_vert_msl, (size_t)basic_vert_msl_len};
	default:
		return (S_GraphicsShaderBlob){NULL, 0};
	}
}

// Returns the embedded basic fragment shader blob for the selected format
static S_GraphicsShaderBlob s_get_basic_frag_blob(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return (S_GraphicsShaderBlob){basic_frag_spv, (size_t)basic_frag_spv_len};
	case SDL_GPU_SHADERFORMAT_DXIL:
		return (S_GraphicsShaderBlob){basic_frag_dxil, (size_t)basic_frag_dxil_len};
	case SDL_GPU_SHADERFORMAT_MSL:
		return (S_GraphicsShaderBlob){basic_frag_msl, (size_t)basic_frag_msl_len};
	default:
		return (S_GraphicsShaderBlob){NULL, 0};
	}
}

// Returns the embedded buffer-to-texture compute shader blob for the selected
// format
static S_GraphicsShaderBlob s_get_buf2tex_comp_blob(SDL_GPUShaderFormat format) {
	switch (format) {
	case SDL_GPU_SHADERFORMAT_SPIRV:
		return (S_GraphicsShaderBlob){
			buf2tex_comp_spv,
			(size_t)buf2tex_comp_spv_len,
		};
	case SDL_GPU_SHADERFORMAT_DXIL:
		return (S_GraphicsShaderBlob){
			buf2tex_comp_dxil,
			(size_t)buf2tex_comp_dxil_len,
		};
	case SDL_GPU_SHADERFORMAT_MSL:
		return (S_GraphicsShaderBlob){buf2tex_comp_msl, (size_t)buf2tex_comp_msl_len};
	default:
		return (S_GraphicsShaderBlob){NULL, 0};
	}
}

// Creates one graphics shader object from an embedded blob
static SDL_GPUShader *
s_create_shader(const S_GraphicsShaderBlob *blob, SDL_GPUShaderStage stage, uint32_t num_samplers) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!blob || !blob->code || blob->size == 0) {
		SDL_SetError(
			"No precompiled %s shader blob for %s format",
			s_shader_stage_name(stage),
			dttr_graphics_shader_format_name(state->m_shader_format)
		);
		return NULL;
	}

	const SDL_GPUShaderCreateInfo info = {
		.code = blob->code,
		.code_size = blob->size,
		.entrypoint = "main",
		.format = state->m_shader_format,
		.stage = stage,
		.num_samplers = num_samplers,
		.num_uniform_buffers = 1,
	};

	return SDL_CreateGPUShader(state->m_device, &info);
}

// Creates the compute pipeline used to expand upload buffers into textures
static SDL_GPUComputePipeline *s_create_compute_pipeline(const S_GraphicsShaderBlob *blob) {
	DTTR_BackendState *state = &g_dttr_backend;

	if (!blob || !blob->code || blob->size == 0) {
		SDL_SetError(
			"No precompiled compute shader blob for %s format", dttr_graphics_shader_format_name(state->m_shader_format)
		);
		return NULL;
	}

	const SDL_GPUComputePipelineCreateInfo info = {
		.code = blob->code,
		.code_size = blob->size,
		.entrypoint = "main",
		.format = state->m_shader_format,
		.num_readonly_storage_buffers = 1,
		.num_readwrite_storage_textures = 1,
		.num_uniform_buffers = 1,
		.threadcount_x = DTTR_COMPUTE_WORKGROUP_X,
		.threadcount_y = DTTR_COMPUTE_WORKGROUP_Y,
		.threadcount_z = DTTR_COMPUTE_WORKGROUP_Z,
	};

	return SDL_CreateGPUComputePipeline(state->m_device, &info);
}

// Releases a temporary vertex/fragment shader pair after pipeline creation
static void s_release_shader_pair(DTTR_BackendState *state, SDL_GPUShader *vert, SDL_GPUShader *frag) {
	if (vert)
		SDL_ReleaseGPUShader(state->m_device, vert);

	if (frag)
		SDL_ReleaseGPUShader(state->m_device, frag);
}

// Builds all blend/depth graphics pipeline variants used by draw replay
static bool s_create_graphics_pipelines(DTTR_BackendState *state, SDL_GPUShader *vert, SDL_GPUShader *frag) {
	const SDL_GPUVertexBufferDescription vbuf_desc = {
		.slot = 0,
		.pitch = DTTR_VERTEX_SIZE,
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
	};

	const SDL_GPUVertexAttribute attrs[] = {
		{0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0},						   // position
		{1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, offsetof(DTTR_Vertex, rhw)}, // rhw
		{2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(DTTR_Vertex, r)},  // color
		{3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(DTTR_Vertex, u)},  // texcoord
	};

	const SDL_GPUTextureFormat swapchain_fmt = SDL_GetGPUSwapchainTextureFormat(state->m_device, state->m_window);

	const SDL_GPUColorTargetDescription color_targets[3] = {
		{.format = swapchain_fmt, .blend_state = {.enable_blend = false}},
		{.format = swapchain_fmt,
		 .blend_state =
			 {
				 .enable_blend = true,
				 .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				 .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				 .color_blend_op = SDL_GPU_BLENDOP_ADD,
				 .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
				 .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				 .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			 }},
		{.format = swapchain_fmt,
		 .blend_state = {
			 .enable_blend = true,
			 .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			 .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			 .color_blend_op = SDL_GPU_BLENDOP_ADD,
			 .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			 .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
			 .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		 }},
	};

	for (int bmode = 0; bmode < 3; bmode++) {
		for (int dtest = 0; dtest < 2; dtest++) {
			for (int dwrite = 0; dwrite < 2; dwrite++) {
				const SDL_GPUGraphicsPipelineCreateInfo pipe_info = {
					.vertex_shader = vert,
					.fragment_shader = frag,
					.vertex_input_state =
						{
							.vertex_buffer_descriptions = &vbuf_desc,
							.num_vertex_buffers = 1,
							.vertex_attributes = attrs,
							.num_vertex_attributes = DTTR_VERTEX_ATTRIBUTE_COUNT,
						},
					.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
					.rasterizer_state =
						{
							.fill_mode = SDL_GPU_FILLMODE_FILL,
							.cull_mode = SDL_GPU_CULLMODE_NONE,
							.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
						},
					.multisample_state =
						{
							.sample_count = state->m_msaa_sample_count,
							.sample_mask = 0,
							.enable_mask = false,
						},
					.depth_stencil_state =
						{
							.enable_depth_test = dtest,
							.enable_depth_write = dwrite,
							.compare_op = dtest ? SDL_GPU_COMPAREOP_LESS_OR_EQUAL
												: SDL_GPU_COMPAREOP_ALWAYS,
						},
					.target_info = {
						.color_target_descriptions = &color_targets[bmode],
						.num_color_targets = 1,
						.has_depth_stencil_target = true,
						.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
					},
				};

				const int idx = DTTR_PIPELINE_INDEX(bmode, dtest, dwrite);
				state->m_pipelines[idx] = SDL_CreateGPUGraphicsPipeline(state->m_device, &pipe_info);

				if (state->m_pipelines[idx])
					continue;

				log_error("Failed to create pipeline[%d]: %s", idx, SDL_GetError());
				return false;
			}
		}
	}

	return true;
}

// Creates the compute pipeline used by staged texture uploads
static bool s_create_buf2tex_pipeline(DTTR_BackendState *state) {
	const S_GraphicsShaderBlob buf2tex_blob = s_get_buf2tex_comp_blob(state->m_shader_format);
	state->m_buf2tex_pipeline = s_create_compute_pipeline(&buf2tex_blob);

	if (state->m_buf2tex_pipeline)
		return true;

	log_error("Failed to create buf2tex compute pipeline: %s", SDL_GetError());
	return false;
}

// Creates all graphics and compute pipelines required by the backend
bool dttr_graphics_create_pipelines(void) {
	DTTR_BackendState *state = &g_dttr_backend;

	const S_GraphicsShaderBlob vert_blob = s_get_basic_vert_blob(state->m_shader_format);
	SDL_GPUShader *vert = s_create_shader(&vert_blob, SDL_GPU_SHADERSTAGE_VERTEX, 0);

	if (!vert) {
		log_error("Failed to create vertex shader: %s", SDL_GetError());
		return false;
	}

	const S_GraphicsShaderBlob frag_blob = s_get_basic_frag_blob(state->m_shader_format);

	SDL_GPUShader *frag = s_create_shader(&frag_blob, SDL_GPU_SHADERSTAGE_FRAGMENT, 1);

	if (!frag) {
		log_error("Failed to create fragment shader: %s", SDL_GetError());
		s_release_shader_pair(state, vert, NULL);
		return false;
	}

	if (!s_create_graphics_pipelines(state, vert, frag)) {
		s_release_shader_pair(state, vert, frag);
		return false;
	}

	s_release_shader_pair(state, vert, frag);

	return s_create_buf2tex_pipeline(state);
}
