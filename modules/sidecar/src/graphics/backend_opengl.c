// OpenGL 3.3 fallback backend for environments where SDL GPU is unavailable.
// (e.g. Parallels on Darwin ARM64)

#include "backend_opengl_private.h"
#include "graphics_private.h"

#include <dttr_log.h>

#include <dttr_config.h>

#define DRIVER_DISPLAY_OPENGL "OpenGL 3.3"

#ifdef DTTR_MODS_ENABLED
#include "../mods/mods_private.h"
#include "imgui_overlay_private.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const DTTR_RendererVtbl renderer;

#include "gen/opengl_shaders.h"

static GLuint compile_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (!status) {
		char info[512];
		glGetShaderInfoLog(shader, sizeof(info), NULL, info);
		DTTR_LOG_ERROR(
			"GL shader compile failed (%s): %s",
			type == GL_VERTEX_SHADER ? "vert" : "frag",
			info
		);
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

static GLuint create_program() {
	GLuint vert = compile_shader(GL_VERTEX_SHADER, OPENGL_BASIC_VERT_SOURCE);

	if (!vert) {
		return 0;
	}

	GLuint frag = compile_shader(GL_FRAGMENT_SHADER, OPENGL_BASIC_FRAG_SOURCE);

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
		DTTR_LOG_ERROR("GL program link failed: %s", info);
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

static bool create_fbo(opengl_backend_data *gl, int width, int height) {
	glGenFramebuffers(1, &gl->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gl->fbo);

	glGenTextures(1, &gl->fbo_color_tex);
	glBindTexture(GL_TEXTURE_2D, gl->fbo_color_tex);
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
		gl->fbo_color_tex,
		0
	);

	glGenRenderbuffers(1, &gl->fbo_depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->fbo_depth_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		gl->fbo_depth_rbo
	);

	GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		DTTR_LOG_ERROR("GL framebuffer incomplete: 0x%x", fb_status);
		return false;
	}

	gl->fbo_width = width;
	gl->fbo_height = height;
	return true;
}

static void destroy_fbo(opengl_backend_data *gl) {
	if (gl->fbo) {
		glDeleteFramebuffers(1, &gl->fbo);
		gl->fbo = 0;
	}

	if (gl->fbo_color_tex) {
		glDeleteTextures(1, &gl->fbo_color_tex);
		gl->fbo_color_tex = 0;
	}

	if (gl->fbo_depth_rbo) {
		glDeleteRenderbuffers(1, &gl->fbo_depth_rbo);
		gl->fbo_depth_rbo = 0;
	}
}

static void create_samplers(opengl_backend_data *gl) {
	glGenSamplers(DTTR_SAMPLER_COUNT, gl->gl_samplers);

	const GLint min_filter = dttr_config.generate_texture_mipmaps
								 ? GL_LINEAR_MIPMAP_LINEAR
								 : GL_LINEAR;

	for (int cu = 0; cu < 2; cu++) {
		for (int cv = 0; cv < 2; cv++) {
			GLuint s = gl->gl_samplers[cu * 2 + cv];
			GLint wrap_s = cu ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			GLint wrap_t = cv ? GL_CLAMP_TO_EDGE : GL_REPEAT;
			glSamplerParameteri(s, GL_TEXTURE_WRAP_S, wrap_s);
			glSamplerParameteri(s, GL_TEXTURE_WRAP_T, wrap_t);
			glSamplerParameteri(s, GL_TEXTURE_MIN_FILTER, min_filter);
			glSamplerParameteri(s, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
}

static int select_gl_msaa_samples() {
	int requested = dttr_config.msaa_samples;

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

static bool create_msaa_fbo(opengl_backend_data *gl, int w, int h, int samples) {
	glGenFramebuffers(1, &gl->msaa_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, gl->msaa_fbo);

	glGenRenderbuffers(1, &gl->msaa_color_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->msaa_color_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, w, h);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		GL_RENDERBUFFER,
		gl->msaa_color_rbo
	);

	glGenRenderbuffers(1, &gl->msaa_depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl->msaa_depth_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, w, h);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_RENDERBUFFER,
		gl->msaa_depth_rbo
	);

	GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (fb_status != GL_FRAMEBUFFER_COMPLETE) {
		DTTR_LOG_ERROR("GL MSAA framebuffer incomplete: 0x%x", fb_status);
		return false;
	}

	gl->msaa_samples = samples;
	return true;
}

static void destroy_msaa_fbo(opengl_backend_data *gl) {
	if (gl->msaa_fbo) {
		glDeleteFramebuffers(1, &gl->msaa_fbo);
		gl->msaa_fbo = 0;
	}

	if (gl->msaa_color_rbo) {
		glDeleteRenderbuffers(1, &gl->msaa_color_rbo);
		gl->msaa_color_rbo = 0;
	}

	if (gl->msaa_depth_rbo) {
		glDeleteRenderbuffers(1, &gl->msaa_depth_rbo);
		gl->msaa_depth_rbo = 0;
	}

	gl->msaa_samples = 0;
}

static void release_deferred_gl_destroys(
	DTTR_BackendState *state,
	opengl_backend_data *gl
) {
	if (!state->texture_mutex) {
		return;
	}

	SDL_LockMutex(state->texture_mutex);

	for (int i = 0; i < gl->deferred_gl_destroy_count; i++) {
		if (gl->deferred_gl_destroys[i]) {
			glDeleteTextures(1, &gl->deferred_gl_destroys[i]);
			gl->deferred_gl_destroys[i] = 0;
		}
	}

	gl->deferred_gl_destroy_count = 0;
	SDL_UnlockMutex(state->texture_mutex);
}

static void defer_texture_destroy(DTTR_BackendState *state, int texture_index) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl || texture_index < 0 || texture_index >= DTTR_MAX_STAGED_TEXTURES) {
		return;
	}

	if (!gl->gl_textures[texture_index]) {
		return;
	}

	if (gl->deferred_gl_destroy_count < DTTR_MAX_STAGED_TEXTURES) {
		gl->deferred_gl_destroys[gl->deferred_gl_destroy_count++] = gl->gl_textures
																		[texture_index];
	}

	gl->gl_textures[texture_index] = 0;
}

bool dttr_graphics_opengl_init(DTTR_BackendState *state) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	opengl_backend_data *gl = calloc(1, sizeof(opengl_backend_data));

	if (!gl) {
		return false;
	}

	gl->gl_context = SDL_GL_CreateContext(state->window);

	if (!gl->gl_context) {
		DTTR_LOG_ERROR("SDL_GL_CreateContext failed: %s", SDL_GetError());
		free(gl);
		return false;
	}

	if (!SDL_GL_MakeCurrent(state->window, gl->gl_context)) {
		DTTR_LOG_ERROR("SDL_GL_MakeCurrent failed: %s", SDL_GetError());
		SDL_GL_DestroyContext(gl->gl_context);
		free(gl);
		return false;
	}

	SDL_GL_SetSwapInterval(0);

	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		DTTR_LOG_ERROR("Failed to load OpenGL functions via glad");
		SDL_GL_DestroyContext(gl->gl_context);
		free(gl);
		return false;
	}

	gl->program = create_program();

	if (!gl->program) {
		SDL_GL_DestroyContext(gl->gl_context);
		free(gl);
		return false;
	}

	glUseProgram(gl->program);
	gl->loc_mvp = glGetUniformLocation(gl->program, "u_mvp");
	gl->loc_screen_size = glGetUniformLocation(gl->program, "u_screen_size");
	gl->loc_is_2d = glGetUniformLocation(gl->program, "u_is_2d");
	gl->loc_has_texture = glGetUniformLocation(gl->program, "u_has_texture");
	gl->loc_color_op = glGetUniformLocation(gl->program, "u_color_op");
	gl->loc_color_arg1 = glGetUniformLocation(gl->program, "u_color_arg1");
	gl->loc_color_arg2 = glGetUniformLocation(gl->program, "u_color_arg2");
	gl->loc_alpha_op = glGetUniformLocation(gl->program, "u_alpha_op");
	gl->loc_alpha_arg1 = glGetUniformLocation(gl->program, "u_alpha_arg1");
	gl->loc_alpha_arg2 = glGetUniformLocation(gl->program, "u_alpha_arg2");
	gl->loc_texture = glGetUniformLocation(gl->program, "u_texture");

	glGenVertexArrays(1, &gl->vao);
	glBindVertexArray(gl->vao);
	glGenBuffers(1, &gl->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
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

	if (!create_fbo(gl, state->width, state->height)) {
		glDeleteProgram(gl->program);
		glDeleteVertexArrays(1, &gl->vao);
		glDeleteBuffers(1, &gl->vbo);
		SDL_GL_DestroyContext(gl->gl_context);
		free(gl);
		return false;
	}

	create_samplers(gl);

	int msaa = select_gl_msaa_samples();
	if (msaa > 0) {
		if (create_msaa_fbo(gl, state->width, state->height, msaa)) {
			DTTR_LOG_INFO("OpenGL MSAA enabled (%dx samples)", msaa);
		} else {
			DTTR_LOG_WARN("OpenGL MSAA %dx failed, falling back to no MSAA", msaa);
			destroy_msaa_fbo(gl);
		}
	}

	// Create a 1-pixel white fallback texture.
	glGenTextures(1, &gl->dummy_texture);
	glBindTexture(GL_TEXTURE_2D, gl->dummy_texture);
	const uint32_t white = 0xFFFFFFFF;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

	// Allocate the (CPU) vertex staging buffer.
	gl->vertex_staging = malloc((size_t)DTTR_MAX_FRAME_VERTICES * DTTR_VERTEX_SIZE);

	if (!gl->vertex_staging) {
		DTTR_LOG_ERROR("Failed to allocate OpenGL vertex staging buffer");
		SDL_GL_DestroyContext(gl->gl_context);
		free(gl);
		return false;
	}

	state->backend_data = gl;
	state->backend_type = DTTR_BACKEND_OPENGL;
	state->renderer = &renderer;

	DTTR_LOG_INFO(
		"OpenGL 3.3 backend initialized (vendor: %s, renderer: %s)",
		glGetString(GL_VENDOR),
		glGetString(GL_RENDERER)
	);

	return true;
}

static bool resize_fbo(DTTR_BackendState *state, int width, int height) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl) {
		return false;
	}

	if (width == gl->fbo_width && height == gl->fbo_height) {
		return true;
	}

	destroy_fbo(gl);

	if (!create_fbo(gl, width, height)) {
		DTTR_LOG_ERROR("Failed to recreate OpenGL FBO at %dx%d", width, height);
		return false;
	}

	if (gl->msaa_samples > 0) {
		int prev_samples = gl->msaa_samples;
		destroy_msaa_fbo(gl);

		if (!create_msaa_fbo(gl, width, height, prev_samples)) {
			DTTR_LOG_WARN("MSAA FBO resize failed, disabling MSAA");
		}
	}

	state->width = width;
	state->height = height;

	DTTR_LOG_INFO("GL FBO resized to %dx%d", width, height);
	return true;
}

static void upload_pending_textures_gl(DTTR_BackendState *state, opengl_backend_data *gl) {
	if (!state->texture_mutex) {
		return;
	}

	SDL_LockMutex(state->texture_mutex);
	const size_t queued_count = kv_size(state->pending_upload_indices);

	gl->pending_mipmap_count = 0;

	for (size_t q = 0; q < queued_count; q++) {
		const int idx = kv_A(state->pending_upload_indices, q);

		if (idx < 0 || idx >= state->staged_texture_count) {
			continue;
		}

		DTTR_StagedTexture *st = &state->staged_textures[idx];

		if (!st->pixels) {
			st->pending_upload = false;
			continue;
		}

		st->pending_upload = false;

		bool new_texture = false;

		if (!gl->gl_textures[idx]) {
			glGenTextures(1, &gl->gl_textures[idx]);
			new_texture = true;
		}

		glBindTexture(GL_TEXTURE_2D, gl->gl_textures[idx]);

		if (new_texture) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			st->width,
			st->height,
			0,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			st->pixels
		);

		if (dttr_config.generate_texture_mipmaps) {
			gl->pending_mipmap_textures[gl->pending_mipmap_count++] = gl->gl_textures[idx];
		}

		free(st->pixels);
		st->pixels = NULL;
	}

	state->pending_upload_indices.n = 0;
	SDL_UnlockMutex(state->texture_mutex);

	for (int i = 0; i < gl->pending_mipmap_count; i++) {
		glBindTexture(GL_TEXTURE_2D, gl->pending_mipmap_textures[i]);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	gl->pending_mipmap_count = 0;
}

static void upload_video_texture(
	opengl_backend_data *gl,
	const uint8_t *pixels,
	int width,
	int height
) {
	const bool resized = gl->video_width != width || gl->video_height != height;

	glBindTexture(GL_TEXTURE_2D, gl->video_texture);
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

	if (!resized) {
		return;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl->video_width = width;
	gl->video_height = height;
}

static void begin_frame(DTTR_BackendState *state) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl || state->frame_active) {
		return;
	}

	state->frame_index++;

	release_deferred_gl_destroys(state, gl);
	upload_pending_textures_gl(state, gl);

	state->batch_records.n = 0;
	state->vertex_offset = 0;
	state->transfer_mapped = gl->vertex_staging;
	state->frame_active = true;
	dttr_graphics_mod_frame_begin(state);
}

static void replay_batch_records_gl(DTTR_BackendState *state, opengl_backend_data *gl) {
	const GLuint render_fbo = (gl->msaa_samples > 0) ? gl->msaa_fbo : gl->fbo;
	glBindFramebuffer(GL_FRAMEBUFFER, render_fbo);
	glViewport(0, 0, gl->fbo_width, gl->fbo_height);

	glUseProgram(gl->program);
	glBindVertexArray(gl->vao);

	int last_blend_mode = -1;
	bool last_depth_test = false;
	bool last_depth_write = false;
	GLuint last_texture = 0;
	int last_sampler_index = -1;

	for (size_t i = 0; i < kv_size(state->batch_records); i++) {
		const DTTR_BatchRecord *rec = &kv_A(state->batch_records, i);

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
				glDepthFunc(GL_LEQUAL);
			} else {
				glDisable(GL_DEPTH_TEST);
			}

			last_depth_test = rec->draw.depth_test;
		}

		if (rec->draw.depth_write != last_depth_write) {
			glDepthMask(rec->draw.depth_write ? GL_TRUE : GL_FALSE);
			last_depth_write = rec->draw.depth_write;
		}

		glUniformMatrix4fv(gl->loc_mvp, 1, GL_FALSE, rec->draw.uniforms.mvp);
		glUniform2f(
			gl->loc_screen_size,
			rec->draw.uniforms.screen_size[0],
			rec->draw.uniforms.screen_size[1]
		);
		glUniform1f(gl->loc_is_2d, rec->draw.uniforms.is_2d);
		glUniform1f(gl->loc_has_texture, rec->draw.uniforms.has_texture);
		glUniform1f(gl->loc_color_op, rec->draw.uniforms.color_op);
		glUniform1f(gl->loc_color_arg1, rec->draw.uniforms.color_arg1);
		glUniform1f(gl->loc_color_arg2, rec->draw.uniforms.color_arg2);
		glUniform1f(gl->loc_alpha_op, rec->draw.uniforms.alpha_op);
		glUniform1f(gl->loc_alpha_arg1, rec->draw.uniforms.alpha_arg1);
		glUniform1f(gl->loc_alpha_arg2, rec->draw.uniforms.alpha_arg2);

		GLuint tex_id = gl->dummy_texture;

		if (rec->draw.uniforms.has_texture > 0.5f && rec->draw.texture_index != UINT32_MAX
			&& rec->draw.texture_index < DTTR_MAX_STAGED_TEXTURES) {
			GLuint staged = gl->gl_textures[rec->draw.texture_index];

			if (staged) {
				tex_id = staged;
			}
		}

		if (tex_id != last_texture) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glUniform1i(gl->loc_texture, 0);
			last_texture = tex_id;
		}

		if (rec->draw.sampler_index != last_sampler_index) {
			glBindSampler(0, gl->gl_samplers[rec->draw.sampler_index]);
			last_sampler_index = rec->draw.sampler_index;
		}

		glDrawArrays(
			GL_TRIANGLES,
			(GLint)rec->draw.first_vertex,
			(GLsizei)rec->draw.vertex_count
		);
	}
}

static void end_frame(DTTR_BackendState *state) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl || !state->frame_active) {
		return;
	}

	state->frame_active = false;
	dttr_graphics_mod_before_game_frame(state);
	state->transfer_mapped = NULL;

	if (state->vertex_offset > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
		glBufferSubData(
			GL_ARRAY_BUFFER,
			0,
			(GLsizeiptr)(state->vertex_offset * DTTR_VERTEX_SIZE),
			gl->vertex_staging
		);
	}

	if (kv_size(state->batch_records) > 0) {
		replay_batch_records_gl(state, gl);
	}

#ifdef DTTR_MODS_ENABLED
	dttr_imgui_render_game_opengl();
#endif
	dttr_graphics_mod_after_game_frame(state);

	if (gl->msaa_samples > 0) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->msaa_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->fbo);
		glBlitFramebuffer(
			0,
			0,
			gl->fbo_width,
			gl->fbo_height,
			0,
			0,
			gl->fbo_width,
			gl->fbo_height,
			GL_COLOR_BUFFER_BIT,
			GL_NEAREST
		);
	}

	int window_w = 0, window_h = 0;
	SDL_GetWindowSizeInPixels(state->window, &window_w, &window_h);

	if (window_w <= 0) {
		window_w = state->width;
	}

	if (window_h <= 0) {
		window_h = state->height;
	}

	const bool
		is_internal_method = (dttr_config.scaling_method == DTTR_SCALING_METHOD_LOGICAL);
	const DTTR_PresentRect present = dttr_graphics_compute_present_rect(
		window_w,
		window_h,
		gl->fbo_width,
		gl->fbo_height,
		dttr_config.scaling_fit == DTTR_SCALING_MODE_STRETCH,
		(!is_internal_method) && (dttr_config.scaling_fit == DTTR_SCALING_MODE_INTEGER),
		1.0f
	);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glViewport(0, 0, window_w, window_h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	const GLenum blit_filter = (dttr_config.present_filter == SDL_GPU_FILTER_NEAREST)
								   ? GL_NEAREST
								   : GL_LINEAR;
	glBlitFramebuffer(
		0,
		0,
		gl->fbo_width,
		gl->fbo_height,
		present.x,
		present.y,
		present.x + present.w,
		present.y + present.h,
		GL_COLOR_BUFFER_BIT,
		blit_filter
	);

#ifdef DTTR_MODS_ENABLED
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	dttr_imgui_render_opengl(
		(uint32_t)present.x,
		(uint32_t)present.y,
		(uint32_t)present.w,
		(uint32_t)present.h
	);
#endif
	dttr_graphics_mod_present_rect_before(state, &present);

	SDL_GL_SwapWindow(state->window);
	dttr_graphics_mod_present_rect_after(state, &present, true);
	dttr_graphics_mod_frame_end(state);
}

static bool present_video_frame_bgra(
	DTTR_BackendState *state,
	const uint8_t *pixels,
	int width,
	int height,
	int stride
) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl || !pixels || width <= 0 || height <= 0) {
		return false;
	}

	if (state->frame_active) {
		return false;
	}

	if (!gl->video_texture) {
		glGenTextures(1, &gl->video_texture);
	}

	upload_video_texture(gl, pixels, width, height);

	int window_w = 0, window_h = 0;
	SDL_GetWindowSizeInPixels(state->window, &window_w, &window_h);

	if (window_w <= 0 || window_h <= 0) {
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window_w, window_h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glUseProgram(gl->program);
	glBindVertexArray(gl->vao);

	const DTTR_PresentRect present = dttr_graphics_compute_present_rect(
		window_w,
		window_h,
		width,
		height,
		false,
		false,
		0.0f
	);
	const float x0 = (float)present.x;
	const float y0 = (float)present.y;
	const float x1 = (float)(present.x + present.w);
	const float y1 = (float)(present.y + present.h);

	float neutral_mvp[16];
	dttr_graphics_mat4_identity(neutral_mvp);
	glUniformMatrix4fv(gl->loc_mvp, 1, GL_FALSE, neutral_mvp);
	glUniform2f(gl->loc_screen_size, (float)window_w, (float)window_h);
	glUniform1f(gl->loc_is_2d, 1.0f);
	glUniform1f(gl->loc_has_texture, 1.0f);
	glUniform1f(gl->loc_color_op, (float)DTTR_D3DTOP_MODULATE);
	glUniform1f(gl->loc_color_arg1, (float)DTTR_D3DTA_TEXTURE);
	glUniform1f(gl->loc_color_arg2, (float)DTTR_D3DTA_DIFFUSE);
	glUniform1f(gl->loc_alpha_op, (float)DTTR_D3DTOP_SELECTARG1);
	glUniform1f(gl->loc_alpha_arg1, (float)DTTR_D3DTA_TEXTURE);
	glUniform1f(gl->loc_alpha_arg2, (float)DTTR_D3DTA_DIFFUSE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl->video_texture);
	glUniform1i(gl->loc_texture, 0);
	glBindSampler(0, 0);

	DTTR_Vertex verts[6] = {
		{x0, y0, 0, 1, 1, 1, 1, 1, 0, 0},
		{x1, y0, 0, 1, 1, 1, 1, 1, 1, 0},
		{x0, y1, 0, 1, 1, 1, 1, 1, 0, 1},
		{x0, y1, 0, 1, 1, 1, 1, 1, 0, 1},
		{x1, y0, 0, 1, 1, 1, 1, 1, 1, 0},
		{x1, y1, 0, 1, 1, 1, 1, 1, 1, 1},
	};

	glBindBuffer(GL_ARRAY_BUFFER, gl->vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	SDL_GL_SwapWindow(state->window);
	return true;
}

static void cleanup(DTTR_BackendState *state) {
	opengl_backend_data *gl = (opengl_backend_data *)state->backend_data;

	if (!gl) {
		return;
	}

	if (gl->program) {
		glDeleteProgram(gl->program);
	}

	if (gl->vao) {
		glDeleteVertexArrays(1, &gl->vao);
	}

	if (gl->vbo) {
		glDeleteBuffers(1, &gl->vbo);
	}

	destroy_msaa_fbo(gl);
	destroy_fbo(gl);

	release_deferred_gl_destroys(state, gl);

	for (int i = 0; i < DTTR_MAX_STAGED_TEXTURES; i++) {
		if (gl->gl_textures[i]) {
			glDeleteTextures(1, &gl->gl_textures[i]);
		}
	}

	glDeleteSamplers(DTTR_SAMPLER_COUNT, gl->gl_samplers);

	if (gl->dummy_texture) {
		glDeleteTextures(1, &gl->dummy_texture);
	}

	if (gl->video_texture) {
		glDeleteTextures(1, &gl->video_texture);
	}

	free(gl->vertex_staging);

	SDL_GL_DestroyContext(gl->gl_context);
	free(gl);
	state->backend_data = NULL;
}

static const char *get_driver_name(const DTTR_BackendState *state) {
	return DRIVER_DISPLAY_OPENGL;
}

static const DTTR_RendererVtbl renderer = {
	.begin_frame = begin_frame,
	.end_frame = end_frame,
	.present_video_frame_bgra = present_video_frame_bgra,
	.resize = resize_fbo,
	.cleanup = cleanup,
	.get_driver_name = get_driver_name,
	.defer_texture_destroy = defer_texture_destroy,
};
