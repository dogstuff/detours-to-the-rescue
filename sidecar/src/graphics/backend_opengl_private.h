#ifndef BACKEND_OPENGL_PRIVATE_H
#define BACKEND_OPENGL_PRIVATE_H

#include "graphics_private.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <stdbool.h>

typedef struct {
	SDL_GLContext m_gl_context;
	GLuint m_program;
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_fbo;
	GLuint m_fbo_color_tex;
	GLuint m_fbo_depth_rbo;
	int m_fbo_width;
	int m_fbo_height;
	GLuint m_gl_textures[DTTR_MAX_STAGED_TEXTURES];
	GLuint m_deferred_gl_destroys[DTTR_MAX_STAGED_TEXTURES];
	int m_deferred_gl_destroy_count;
	GLuint m_gl_samplers[DTTR_SAMPLER_COUNT];
	GLuint m_dummy_texture;
	GLuint m_video_texture;
	int m_video_width;
	int m_video_height;
	void *m_vertex_staging;
	GLint m_loc_mvp;
	GLint m_loc_screen_size;
	GLint m_loc_is_2d;
	GLint m_loc_has_texture;
	GLint m_loc_texture;
	GLuint m_msaa_fbo;
	GLuint m_msaa_color_rbo;
	GLuint m_msaa_depth_rbo;
	int m_msaa_samples;
	GLuint m_pending_mipmap_textures[DTTR_MAX_STAGED_TEXTURES];
	int m_pending_mipmap_count;
} S_OpenglBackendData;

#endif /* BACKEND_OPENGL_PRIVATE_H */
