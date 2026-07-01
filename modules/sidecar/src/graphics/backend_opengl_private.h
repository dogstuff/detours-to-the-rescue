#ifndef BACKEND_OPENGL_PRIVATE_H
#define BACKEND_OPENGL_PRIVATE_H

#include "graphics_private.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <stdbool.h>

typedef struct {
	SDL_GLContext gl_context;
	GLuint program;
	GLuint vao;
	GLuint vbo;
	GLuint fbo;
	GLuint fbo_color_tex;
	GLuint fbo_depth_rbo;
	int fbo_width;
	int fbo_height;
	GLuint gl_textures[DTTR_MAX_STAGED_TEXTURES];
	GLuint *deferred_gl_destroys;
	int deferred_gl_destroy_count;
	int deferred_gl_destroy_capacity;
	GLuint gl_samplers[DTTR_SAMPLER_COUNT];
	GLuint dummy_texture;
	GLuint video_texture;
	int video_width;
	int video_height;
	void *vertex_staging;
	GLint loc_mvp;
	GLint loc_screen_size;
	GLint loc_is_2d;
	GLint loc_has_texture;
	GLint loc_color_op;
	GLint loc_color_arg1;
	GLint loc_color_arg2;
	GLint loc_alpha_op;
	GLint loc_alpha_arg1;
	GLint loc_alpha_arg2;
	GLint loc_texture;
	GLuint msaa_fbo;
	GLuint msaa_color_rbo;
	GLuint msaa_depth_rbo;
	int msaa_samples;
	GLuint pending_mipmap_textures[DTTR_MAX_STAGED_TEXTURES];
	int pending_mipmap_count;
} opengl_backend_data;

#endif // BACKEND_OPENGL_PRIVATE_H
