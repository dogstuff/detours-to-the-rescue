// OpenGL 3.3 fallback backend for environments where SDL GPU is unavailable.
// (e.g. Parallels on Darwin ARM64)

#include "backend_opengl_internal.h"
#include "graphics_internal.h"

#define S_DRIVER_DISPLAY_OPENGL "OpenGL 3.3"
#include "log.h"

#include <dttr_config.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DTTR_RendererVtbl s_renderer;

#include "gen/opengl_shaders.h"

static GLuint s_compile_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status) {
		char info[512];
		glGetShaderInfoLog(shader, sizeof(info), NULL, info);
		log_error(
			"GL shader compile failed (%s): %s",
			type == GL_VERTEX_SHADER ? "vert" : "frag",
			info
		);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint s_create_program(void) {
	GLuint vert = s_compile_shader(GL_VERTEX_SHADER, S_OPENGL_BASIC_VERT_SOURCE);

	if (!vert) {
		return 0;
	}

	GLuint frag = s_compile_shader(GL_FRAGMENT_SHADER, S_OPENGL_BASIC_FRAG_SOURCE);

	if (!frag) {
		glDeleteShader(vert);
		return 0;
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	glDeleteShader(vert);
	glDeleteShader(frag);

	GLint status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (!status) {
		char info[512];
		glGetProgramInfoLog(program, sizeof(info), NULL, info);
		log_error("GL program link failed: %s", info);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

static bool s_create_fbo(S_OpenglBackendData *gl, int width, int height) {
	glGenFramebuffers(1, &gl->m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gl->m_fbo);

	glGenTextures(1, &gl->m_fbo_color_tex);
	glBindTexture(GL_TEXTURE_2D, gl->m_fbo_color_tex);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D,
		gl->m_fbo_color_tex,
		0
	);

	glGenRenderbuffers(1, &gl->m_fbo_depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->m_fbo_depth_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		gl->m_fbo_depth_rbo
	);

	GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		log_error("GL framebuffer incomplete: 0x%x", fb_status);
		return false;
	}

	gl->m_fbo_width = width;
	gl->m_fbo_height = height;
	return true;
}

static void s_create_samplers(S_OpenglBackendData *gl) {
	glGenSamplers(DTTR_SAMPLER_COUNT, gl->m_gl_samplers);

	const GLint min_filter = g_dttr_config.m_generate_texture_mipmaps
								 ? GL_LINEAR_MIPMAP_LINEAR
								 : GL_LINEAR;

	for (int cu = 0; cu < 2; cu++) {
		for (int cv = 0; cv < 2; cv++) {
			GLuint s = gl->m_gl_samplers[cu * 2 + cv];
			GLint wrap_s = cu ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			GLint wrap_t = cv ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			glSamplerParameteri(s, GL_TEXTURE_WRAP_S, wrap_s);
			glSamplerParameteri(s, GL_TEXTURE_WRAP_T, wrap_t);
			glSamplerParameteri(s, GL_TEXTURE_MIN_FILTER, min_filter);
			glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
}

static int s_select_gl_msaa_samples(void) {
	int requested = g_dttr_config.m_msaa_samples;

	if (requested <= 1) {
		return 0;
	}

	GLint max_samples = 0;
	glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

	if (max_samples <= 1) {
		return 0;
	}

	if (requested > max_samples) {
		requested = max_samples;
	}

	return requested;
}

static bool s_create_msaa_fbo(S_OpenglBackendData *gl, int w, int h, int samples) {
	glGenFramebuffers(1, &gl->m_msaa_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gl->m_msaa_fbo);

	glGenRenderbuffers(1, &gl->m_msaa_color_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->m_msaa_color_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, w, h);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_RENDERBUFFER,
		gl->m_msaa_color_rbo
	);

	glGenRenderbuffers(1, &gl->m_msaa_depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->m_msaa_depth_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		gl->m_msaa_depth_rbo
	);

	GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		log_error("GL MSAA framebuffer incomplete: 0x%x", fb_status);
		return false;
	}

	gl->m_msaa_samples = samples;
	return true;
}

static void s_destroy_msaa_fbo(S_OpenglBackendData *gl) {
	if (gl->m_msaa_fbo) {
		glDeleteFramebuffers(1, &gl->m_msaa_fbo);
		gl->m_msaa_fbo = 0;
	}

	if (gl->m_msaa_color_rbo) {
		glDeleteRenderbuffers(1, &gl->m_msaa_color_rbo);
		gl->m_msaa_color_rbo = 0;
	}

	if (gl->m_msaa_depth_rbo) {
		glDeleteRenderbuffers(1, &gl->m_msaa_depth_rbo);
		gl->m_msaa_depth_rbo = 0;
	}

	gl->m_msaa_samples = 0;
}

static void s_release_deferred_gl_destroys(
	DTTR_BackendState *state,
	S_OpenglBackendData *gl
) {
	if (!state->m_texture_mutex) {
		return;
	}

	SDL_LockMutex(state->m_texture_mutex);

	for (int i = 0; i < gl->m_deferred_gl_destroy_count; i++) {
		if (gl->m_deferred_gl_destroys[i]) {
			glDeleteTextures(1, &gl->m_deferred_gl_destroys[i]);
			gl->m_deferred_gl_destroys[i] = 0;
		}
	}

	gl->m_deferred_gl_destroy_count = 0;
	SDL_UnlockMutex(state->m_texture_mutex);
}

static void s_defer_texture_destroy(DTTR_BackendState *state, int texture_index) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl || texture_index < 0 || texture_index >= DTTR_MAX_STAGED_TEXTURES) {
		return;
	}

	if (!gl->m_gl_textures[texture_index]) {
		return;
	}

	if (gl->m_deferred_gl_destroy_count < DTTR_MAX_STAGED_TEXTURES) {
		gl->m_deferred_gl_destroys[gl->m_deferred_gl_destroy_count++] = gl->m_gl_textures
																			[texture_index];
	}

	gl->m_gl_textures[texture_index] = 0;
}

bool dttr_graphics_opengl_init(DTTR_BackendState *state) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	S_OpenglBackendData *gl = calloc(1, sizeof(S_OpenglBackendData));

	if (!gl) {
		return false;
	}

	gl->m_gl_context = SDL_GL_CreateContext(state->m_window);

	if (!gl->m_gl_context) {
		log_error("SDL_GL_CreateContext failed: %s", SDL_GetError());
		free(gl);
		return false;
	}

	if (!SDL_GL_MakeCurrent(state->m_window, gl->m_gl_context)) {
		log_error("SDL_GL_MakeCurrent failed: %s", SDL_GetError());
		SDL_GL_DestroyContext(gl->m_gl_context);
		free(gl);
		return false;
	}

	SDL_GL_SetSwapInterval(0);

	if (!s_load_functions()) {
		log_error("Failed to load OpenGL 3.3 functions");
		SDL_GL_DestroyContext(gl->m_gl_context);
		free(gl);
		return false;
	}

	gl->m_program = s_create_program();

	if (!gl->m_program) {
		SDL_GL_DestroyContext(gl->m_gl_context);
		free(gl);
		return false;
	}

	glUseProgram(gl->m_program);
	gl->m_loc_mvp = glGetUniformLocation(gl->m_program, "u_mvp");
	gl->m_loc_screen_size = glGetUniformLocation(gl->m_program, "u_screen_size");
	gl->m_loc_is_2d = glGetUniformLocation(gl->m_program, "u_is_2d");
	gl->m_loc_has_texture = glGetUniformLocation(gl->m_program, "u_has_texture");
	gl->m_loc_texture = glGetUniformLocation(gl->m_program, "u_texture");

	// Create the vertex array and buffer objects.
	glGenVertexArrays(1, &gl->m_vao);
	glBindVertexArray(gl->m_vao);
	glGenBuffers(1, &gl->m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gl->m_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		(GLsizeiptr)(DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE),
		NULL,
		GL_DYNAMIC_DRAW
	);

	// Vertex layout matches DTTR_Vertex
	const GLsizei stride = (GLsizei)DTTR_VERTEX_SIZE;
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void *)(4 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void *)(8 * sizeof(float)));
	glEnableVertexAttribArray(3);

	// Create the framebuffer.
	if (!s_create_fbo(gl, state->m_width, state->m_height)) {
		glDeleteProgram(gl->m_program);
		glDeleteVertexArrays(1, &gl->m_vao);
		glDeleteBuffers(1, &gl->m_vbo);
		SDL_GL_DestroyContext(gl->m_gl_context);
		free(gl);
		return false;
	}

	s_create_samplers(gl);

	int msaa = s_select_gl_msaa_samples();
	if (msaa > 0) {
		if (s_create_msaa_fbo(gl, state->m_width, state->m_height, msaa)) {
			log_info("OpenGL MSAA enabled (%dx samples)", msaa);
		} else {
			log_warn("OpenGL MSAA %dx failed, falling back to no MSAA", msaa);
			s_destroy_msaa_fbo(gl);
		}
	}

	// Create a 1-pixel white fallback texture.
	glGenTextures(1, &gl->m_dummy_texture);
	glBindTexture(GL_TEXTURE_2D, gl->m_dummy_texture);
	const uint32_t white = 0xFFFFFFFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

	// Allocate the (CPU) vertex staging buffer.
	gl->m_vertex_staging = malloc((size_t)DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE);

	if (!gl->m_vertex_staging) {
		log_error("Failed to allocate OpenGL vertex staging buffer");
		SDL_GL_DestroyContext(gl->m_gl_context);
		free(gl);
		return false;
	}

	state->m_backend_data = gl;
	state->m_backend_type = DTTR_BACKEND_OPENGL;
	state->m_renderer = &s_renderer;

#ifdef DTTR_COMPONENTS_ENABLED
	int ow = 0, oh = 0;
	uint8_t *overlay_pixels = dttr_components_overlay_decode_bitmap(&ow, &oh);

	if (overlay_pixels) {
		glGenTextures(1, &gl->m_overlay_texture);
		glBindTexture(GL_TEXTURE_2D, gl->m_overlay_texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			ow,
			oh,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			overlay_pixels
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		free(overlay_pixels);

		state->m_components_overlay_w = ow;
		state->m_components_overlay_h = oh;
		log_info("Components overlay texture created (%dx%d)", ow, oh);
	}
#endif

	log_info(
		"OpenGL 3.3 backend initialized (vendor: %s, renderer: %s)",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER)
	);

	return true;
}

static bool s_resize_fbo(DTTR_BackendState *state, int width, int height) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl) {
		return false;
	}

	if (width == gl->m_fbo_width && height == gl->m_fbo_height) {
		return true;
	}

	if (gl->m_fbo_color_tex) {
		glDeleteTextures(1, &gl->m_fbo_color_tex);
	}

	if (gl->m_fbo_depth_rbo) {
		glDeleteRenderbuffers(1, &gl->m_fbo_depth_rbo);
	}

	if (gl->m_fbo) {
		glDeleteFramebuffers(1, &gl->m_fbo);
	}

	if (!s_create_fbo(gl, width, height)) {
		log_error("Failed to recreate OpenGL FBO at %dx%d", width, height);
		return false;
	}

	if (gl->m_msaa_samples > 0) {
		int prev_samples = gl->m_msaa_samples;
		s_destroy_msaa_fbo(gl);

		if (!s_create_msaa_fbo(gl, width, height, prev_samples)) {
			log_warn("MSAA FBO resize failed, disabling MSAA");
		}
	}

	state->m_width = width;
	state->m_height = height;

	log_info("GL FBO resized to %dx%d", width, height);
	return true;
}

static void s_upload_pending_textures_gl(
	DTTR_BackendState *state,
	S_OpenglBackendData *gl
) {
	if (!state->m_texture_mutex) {
		return;
	}

	SDL_LockMutex(state->m_texture_mutex);
	const size_t queued_count = kv_size(state->m_pending_upload_indices);

	gl->m_pending_mipmap_count = 0;

	for (size_t q = 0; q < queued_count; q++) {
		const int idx = kv_A(state->m_pending_upload_indices, q);

		if (idx < 0 || idx >= state->m_staged_texture_count) {
			continue;
		}

		DTTR_StagedTexture *st = &state->m_staged_textures[idx];

		if (!st->m_pixels) {
			st->m_pending_upload = false;
			continue;
		}

		st->m_pending_upload = false;

		bool new_texture = false;

		if (!gl->m_gl_textures[idx]) {
			glGenTextures(1, &gl->m_gl_textures[idx]);
			new_texture = true;
		}

		glBindTexture(GL_TEXTURE_2D, gl->m_gl_textures[idx]);

		if (new_texture) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			st->m_width,
			st->m_height,
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			st->m_pixels
		);

		if (g_dttr_config.m_generate_texture_mipmaps) {
			gl->m_pending_mipmap_textures[gl->m_pending_mipmap_count++] = gl->m_gl_textures
																			  [idx];
		}

		free(st->m_pixels);
		st->m_pixels = NULL;
	}

	state->m_pending_upload_indices.n = 0;
	SDL_UnlockMutex(state->m_texture_mutex);

	for (int i = 0; i < gl->m_pending_mipmap_count; i++) {
		glBindTexture(GL_TEXTURE_2D, gl->m_pending_mipmap_textures[i]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	gl->m_pending_mipmap_count = 0;
}

static void s_begin_frame(DTTR_BackendState *state) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl || state->m_frame_active) {
		return;
	}

	state->m_frame_index++;

	s_release_deferred_gl_destroys(state, gl);
	s_upload_pending_textures_gl(state, gl);

	state->m_batch_records.n = 0;
	state->m_vertex_offset = 0;
	state->m_transfer_mapped = gl->m_vertex_staging;
	state->m_frame_active = true;
}

static void s_replay_batch_records_gl(DTTR_BackendState *state, S_OpenglBackendData *gl) {
	const GLuint render_fbo = (gl->m_msaa_samples > 0) ? gl->m_msaa_fbo : gl->m_fbo;
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glViewport(0, 0, gl->m_fbo_width, gl->m_fbo_height);

	glUseProgram(gl->m_program);
	glBindVertexArray(gl->m_vao);

	int last_blend_mode = -1;
	bool last_depth_test = false;
	bool last_depth_write = false;
	GLuint last_texture = 0;
	int last_sampler_index = -1;

	for (size_t i = 0; i < kv_size(state->m_batch_records); i++) {
		const DTTR_BatchRecord *rec = &kv_A(state->m_batch_records, i);

		if (rec->type == DTTR_BATCH_CLEAR) {
			GLbitfield clear_mask = 0;

			if (rec->clear.flags & DTTR_CLEAR_COLOR) {
				glClearColor(
					rec->clear.color.r,
					rec->clear.color.g,
					rec->clear.color.b,
					rec->clear.color.a
				);
				clear_mask |= GL_COLOR_BUFFER_BIT;
			}

			if (rec->clear.flags & DTTR_CLEAR_DEPTH) {
				glClearDepth((double)rec->clear.depth);
				glDepthMask(GL_TRUE);
				clear_mask |= GL_DEPTH_BUFFER_BIT;
				last_depth_write = true;
			}

			if (clear_mask) {
				glClear(clear_mask);
			}

			last_blend_mode = -1;
			last_depth_test = false;
			last_texture = 0;
			last_sampler_index = -1;
			continue;
		}

		// Process a draw record.
		if (rec->draw.blend_mode != last_blend_mode) {
			if (rec->draw.blend_mode == DTTR_BLEND_OFF) {
				glDisable(GL_BLEND);
			} else {
				glEnable(GL_BLEND);
				glBlendEquation(GL_FUNC_ADD);

				if (rec->draw.blend_mode == DTTR_BLEND_ADDITIVE) {
					glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
				} else {
					glBlendFuncSeparate(
						GL_SRC_ALPHA,
						GL_ONE_MINUS_SRC_ALPHA,
						GL_SRC_ALPHA,
						GL_ONE_MINUS_SRC_ALPHA
					);
				}
			}
			last_blend_mode = rec->draw.blend_mode;
		}

		if (rec->draw.depth_test != last_depth_test) {
			if (rec->draw.depth_test) {
				glEnable(GL_DEPTH_TEST);
			} else {
				glDisable(GL_DEPTH_TEST);
			}
			last_depth_test = rec->draw.depth_test;
		}

		if (rec->draw.depth_write != last_depth_write) {
			glDepthMask(rec->draw.depth_write ? GL_TRUE : GL_FALSE);
			last_depth_write = rec->draw.depth_write;
		}

		// Upload per-draw uniforms.
		glUniformMatrix4fv(gl->m_loc_mvp, 1, GL_FALSE, rec->draw.uniforms.m_mvp);
		glUniform2f(
			gl->m_loc_screen_size,
			rec->draw.uniforms.m_screen_size[0],
			rec->draw.uniforms.m_screen_size[1]
		);
		glUniform1f(gl->m_loc_is_2d, rec->draw.uniforms.m_is_2d);
		glUniform1f(gl->m_loc_has_texture, rec->draw.uniforms.m_has_texture);

		// Bind the active texture or the dummy fallback.
		GLuint tex_id = gl->m_dummy_texture;

		if (rec->draw.uniforms.m_has_texture > 0.5f
			&& rec->draw.texture_index != UINT32_MAX
			&& rec->draw.texture_index < DTTR_MAX_STAGED_TEXTURES) {
			GLuint staged = gl->m_gl_textures[rec->draw.texture_index];

			if (staged) {
				tex_id = staged;
			}
		}

		if (tex_id != last_texture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glUniform1i(gl->m_loc_texture, 0);
			last_texture = tex_id;
		}

		if (rec->draw.sampler_index != last_sampler_index) {
			glBindSampler(0, gl->m_gl_samplers[rec->draw.sampler_index]);
			last_sampler_index = rec->draw.sampler_index;
		}

		glDrawArrays(
			GL_TRIANGLES,
			(GLint)rec->draw.first_vertex,
			(GLsizei)rec->draw.vertex_count
		);
	}
}

static void s_end_frame(DTTR_BackendState *state) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl || !state->m_frame_active) {
		return;
	}

	state->m_frame_active = false;
	state->m_transfer_mapped = NULL;

	// Upload staged vertices to the GPU buffer.
	if (state->m_vertex_offset > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, gl->m_vbo);
		glBufferSubData(
			GL_ARRAY_BUFFER,
			0,
			(GLsizeiptr)(state->m_vertex_offset * DTTR_VERTEX_SIZE),
			gl->m_vertex_staging
		);
	}

	// Replay all recorded draw and clear commands into the FBO.
	if (kv_size(state->m_batch_records) > 0) {
		s_replay_batch_records_gl(state, gl);
	}

#ifdef DTTR_COMPONENTS_ENABLED
	if (gl->m_overlay_texture) {
		DTTR_Vertex overlay_verts[6];
		dttr_components_overlay_build_vertices(
			overlay_verts,
			state->m_width,
			state->m_height,
			state->m_components_overlay_w,
			state->m_components_overlay_h
		);

		const GLuint overlay_fbo = (gl->m_msaa_samples > 0) ? gl->m_msaa_fbo : gl->m_fbo;
		glBindFramebuffer(GL_FRAMEBUFFER, overlay_fbo);
		glViewport(0, 0, gl->m_fbo_width, gl->m_fbo_height);
		glUseProgram(gl->m_program);
		glBindVertexArray(gl->m_vao);

		float identity[16];
		dttr_graphics_mat4_identity(identity);
		glUniformMatrix4fv(gl->m_loc_mvp, 1, GL_FALSE, identity);
		glUniform2f(gl->m_loc_screen_size, (float)state->m_width, (float)state->m_height);
		glUniform1f(gl->m_loc_is_2d, 1.0f);
		glUniform1f(gl->m_loc_has_texture, 1.0f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gl->m_overlay_texture);
		glUniform1i(gl->m_loc_texture, 0);
		glBindSampler(0, 0);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFuncSeparate(
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA,
			GL_SRC_ALPHA,
			GL_ONE_MINUS_SRC_ALPHA
		);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		glBindBuffer(GL_ARRAY_BUFFER, gl->m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(overlay_verts), overlay_verts);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisable(GL_BLEND);
	}
#endif

	// Resolve MSAA FBO to the non-MSAA FBO.
	if (gl->m_msaa_samples > 0) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->m_msaa_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->m_fbo);
		glBlitFramebuffer(
			0,
			0,
			gl->m_fbo_width,
			gl->m_fbo_height,
			0,
			0,
			gl->m_fbo_width,
			gl->m_fbo_height,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST
		);
	}

	// Blit the FBO to the default framebuffer with letterbox scaling.
	int window_w = 0, window_h = 0;
	SDL_GetWindowSizeInPixels(state->m_window, &window_w, &window_h);

	if (window_w <= 0) {
		window_w = state->m_width;
	}

	if (window_h <= 0) {
		window_h = state->m_height;
	}

	const float sx = (float)window_w / (float)gl->m_fbo_width;
	const float sy = (float)window_h / (float)gl->m_fbo_height;
	float scale = sx < sy ? sx : sy;

	if (scale < 0.001f) {
		scale = 1.0f;
	}

	const int present_w = (int)((float)gl->m_fbo_width * scale);
	const int present_h = (int)((float)gl->m_fbo_height * scale);
	const int present_x = (window_w - present_w) / 2;
	const int present_y = (window_h - present_h) / 2;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->m_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// Clear the default framebuffer for letterbox bars.
	glViewport(0, 0, window_w, window_h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const GLenum blit_filter = (g_dttr_config.m_present_filter == SDL_GPU_FILTER_NEAREST)
								   ? GL_NEAREST
								   : GL_LINEAR;
	glBlitFramebuffer(
		0,
		0,
		gl->m_fbo_width,
		gl->m_fbo_height,
		present_x,
		present_y,
		present_x + present_w,
		present_y + present_h,
		GL_COLOR_BUFFER_BIT,
		blit_filter
	);

	SDL_GL_SwapWindow(state->m_window);
}

static bool s_present_video_frame_bgra(
	DTTR_BackendState *state,
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl || !pixels || width <= 0 || height <= 0) {
		return false;
	}

	if (state->m_frame_active) {
		return false;
	}

	if (!gl->m_video_texture) {
		glGenTextures(1, &gl->m_video_texture);
	}

	if (gl->m_video_width != width || gl->m_video_height != height) {
		glBindTexture(GL_TEXTURE_2D, gl->m_video_texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			width,
			height,
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			pixels
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		gl->m_video_width = width;
		gl->m_video_height = height;
	} else {
		glBindTexture(GL_TEXTURE_2D, gl->m_video_texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			width,
			height,
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			pixels
		);
	}

	// Render a fullscreen quad to the default framebuffer.
	int window_w = 0, window_h = 0;
	SDL_GetWindowSizeInPixels(state->m_window, &window_w, &window_h);

	if (window_w <= 0 || window_h <= 0) {
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_w, window_h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glUseProgram(gl->m_program);
	glBindVertexArray(gl->m_vao);

	// Compute letterbox dimensions.
	const float sx = (float)window_w / (float)width;
	const float sy = (float)window_h / (float)height;
	float scale = sx < sy ? sx : sy;
	const int present_w = (int)((float)width * scale);
	const int present_h = (int)((float)height * scale);
	const float x0 = (float)(window_w - present_w) / 2.0f;
	const float y0 = (float)(window_h - present_h) / 2.0f;
	const float x1 = x0 + (float)present_w;
	const float y1 = y0 + (float)present_h;

	// Configure uniforms for 2D quad rendering.
	float identity[16];
	dttr_graphics_mat4_identity(identity);
	glUniformMatrix4fv(gl->m_loc_mvp, 1, GL_FALSE, identity);
	glUniform2f(gl->m_loc_screen_size, (float)window_w, (float)window_h);
	glUniform1f(gl->m_loc_is_2d, 1.0f);
	glUniform1f(gl->m_loc_has_texture, 1.0f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->m_video_texture);
	glUniform1i(gl->m_loc_texture, 0);
	glBindSampler(0, 0);

	// Build quad vertices and submit the draw call.
	DTTR_Vertex verts[6] = {
		{x0, y0, 0, 1, 1, 1, 1, 1, 0, 0},
		{x1, y0, 0, 1, 1, 1, 1, 1, 1, 0},
		{x0, y1, 0, 1, 1, 1, 1, 1, 0, 1},
		{x0, y1, 0, 1, 1, 1, 1, 1, 0, 1},
		{x1, y0, 0, 1, 1, 1, 1, 1, 1, 0},
		{x1, y1, 0, 1, 1, 1, 1, 1, 1, 1},
	};

	glBindBuffer(GL_ARRAY_BUFFER, gl->m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_GL_SwapWindow(state->m_window);
	return true;
}

static void s_cleanup(DTTR_BackendState *state) {
	S_OpenglBackendData *gl = (S_OpenglBackendData *)state->m_backend_data;

	if (!gl) {
		return;
	}

	if (gl->m_program) {
		glDeleteProgram(gl->m_program);
	}

	if (gl->m_vao) {
		glDeleteVertexArrays(1, &gl->m_vao);
	}

	if (gl->m_vbo) {
		glDeleteBuffers(1, &gl->m_vbo);
	}

	s_destroy_msaa_fbo(gl);

	if (gl->m_fbo) {
		glDeleteFramebuffers(1, &gl->m_fbo);
	}

	if (gl->m_fbo_color_tex) {
		glDeleteTextures(1, &gl->m_fbo_color_tex);
	}

	if (gl->m_fbo_depth_rbo) {
		glDeleteRenderbuffers(1, &gl->m_fbo_depth_rbo);
	}

	s_release_deferred_gl_destroys(state, gl);

	for (int i = 0; i < DTTR_MAX_STAGED_TEXTURES; i++) {
		if (gl->m_gl_textures[i]) {
			glDeleteTextures(1, &gl->m_gl_textures[i]);
		}
	}

	glDeleteSamplers(DTTR_SAMPLER_COUNT, gl->m_gl_samplers);

	if (gl->m_dummy_texture) {
		glDeleteTextures(1, &gl->m_dummy_texture);
	}

	if (gl->m_video_texture) {
		glDeleteTextures(1, &gl->m_video_texture);
	}

#ifdef DTTR_COMPONENTS_ENABLED
	if (gl->m_overlay_texture) {
		glDeleteTextures(1, &gl->m_overlay_texture);
	}
#endif

	free(gl->m_vertex_staging);

	SDL_GL_DestroyContext(gl->m_gl_context);
	free(gl);
	state->m_backend_data = NULL;
}

static const char *s_get_driver_name(const DTTR_BackendState *state) {
	return S_DRIVER_DISPLAY_OPENGL;
}

static const DTTR_RendererVtbl s_renderer = {
	.begin_frame = s_begin_frame,
	.end_frame = s_end_frame,
	.present_video_frame_bgra = s_present_video_frame_bgra,
	.resize = s_resize_fbo,
	.cleanup = s_cleanup,
	.get_driver_name = s_get_driver_name,
	.defer_texture_destroy = s_defer_texture_destroy,
};
