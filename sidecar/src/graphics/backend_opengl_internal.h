#ifndef DTTR_BACKEND_OPENGL_INTERNAL_H
#define DTTR_BACKEND_OPENGL_INTERNAL_H

#include "graphics_internal.h"

#include <GL/gl.h>
#include <SDL3/SDL.h>
#include <stdbool.h>

// OpenGL constants not in MinGW
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_DYNAMIC_DRAW
#define GL_DYNAMIC_DRAW 0x88E8
#endif
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_TEXTURE_WRAP_S
#define GL_TEXTURE_WRAP_S 0x2802
#endif
#ifndef GL_TEXTURE_WRAP_T
#define GL_TEXTURE_WRAP_T 0x2803
#endif
#ifndef GL_TEXTURE_MIN_FILTER
#define GL_TEXTURE_MIN_FILTER 0x2801
#endif
#ifndef GL_TEXTURE_MAG_FILTER
#define GL_TEXTURE_MAG_FILTER 0x2800
#endif
#ifndef GL_FUNC_ADD
#define GL_FUNC_ADD 0x8006
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif

// OpenGL types for 3.3 functions
typedef char GLchar;
typedef intptr_t GLsizeiptr;
typedef intptr_t GLintptr;

// Function pointer typedefs
// clang-format off
typedef void     (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void     (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void     (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void     (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void     (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void     (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void     (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void     (APIENTRY *PFNGLBUFFERSUBDATAPROC)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef GLuint   (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void     (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef void     (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length);
typedef void     (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void     (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void     (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint   (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void     (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void     (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void     (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void     (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void     (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void     (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint    (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void     (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void     (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void     (APIENTRY *PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void     (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void     (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void     (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);
typedef void     (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void     (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void     (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void     (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void     (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef GLenum   (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void     (APIENTRY *PFNGLGENRENDERBUFFERSPROC)(GLsizei n, GLuint *renderbuffers);
typedef void     (APIENTRY *PFNGLDELETERENDERBUFFERSPROC)(GLsizei n, const GLuint *renderbuffers);
typedef void     (APIENTRY *PFNGLBINDRENDERBUFFERPROC)(GLenum target, GLuint renderbuffer);
typedef void     (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void     (APIENTRY *PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void     (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void     (APIENTRY *PFNGLGENSAMPLERSPROC)(GLsizei count, GLuint *samplers);
typedef void     (APIENTRY *PFNGLDELETESAMPLERSPROC)(GLsizei count, const GLuint *samplers);
typedef void     (APIENTRY *PFNGLBINDSAMPLERPROC)(GLuint unit, GLuint sampler);
typedef void     (APIENTRY *PFNGLSAMPLERPARAMETERIPROC)(GLuint sampler, GLenum pname, GLint param);
typedef void     (APIENTRY *PFNGLBLENDFUNCSEPARATEPROC)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
typedef void     (APIENTRY *PFNGLBLENDEQUATIONPROC)(GLenum mode);
typedef void     (APIENTRY *PFNGLGENERATEMIPMAPPROC)(GLenum target);
typedef void     (APIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
// clang-format on

// Function pointers loaded at runtime
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLBUFFERSUBDATAPROC glBufferSubData;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLDELETEPROGRAMPROC glDeleteProgram;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUNIFORM1IPROC glUniform1i;
static PFNGLUNIFORM1FPROC glUniform1f;
static PFNGLUNIFORM2FPROC glUniform2f;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
static PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
static PFNGLACTIVETEXTUREPROC glActiveTexture;
static PFNGLGENSAMPLERSPROC glGenSamplers;
static PFNGLDELETESAMPLERSPROC glDeleteSamplers;
static PFNGLBINDSAMPLERPROC glBindSampler;
static PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri;
static PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
static PFNGLBLENDEQUATIONPROC glBlendEquation;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;

#define S_LOAD(name)                                                                     \
	name = (typeof(name))SDL_GL_GetProcAddress(#name);                                   \
	if (!name) {                                                                         \
		return false;                                                                    \
	}

static bool s_load_functions(void) {
	S_LOAD(glGenVertexArrays);
	S_LOAD(glDeleteVertexArrays);
	S_LOAD(glBindVertexArray);
	S_LOAD(glGenBuffers);
	S_LOAD(glDeleteBuffers);
	S_LOAD(glBindBuffer);
	S_LOAD(glBufferData);
	S_LOAD(glBufferSubData);
	S_LOAD(glCreateShader);
	S_LOAD(glDeleteShader);
	S_LOAD(glShaderSource);
	S_LOAD(glCompileShader);
	S_LOAD(glGetShaderiv);
	S_LOAD(glGetShaderInfoLog);
	S_LOAD(glCreateProgram);
	S_LOAD(glDeleteProgram);
	S_LOAD(glAttachShader);
	S_LOAD(glLinkProgram);
	S_LOAD(glGetProgramiv);
	S_LOAD(glGetProgramInfoLog);
	S_LOAD(glUseProgram);
	S_LOAD(glGetUniformLocation);
	S_LOAD(glUniform1i);
	S_LOAD(glUniform1f);
	S_LOAD(glUniform2f);
	S_LOAD(glUniformMatrix4fv);
	S_LOAD(glVertexAttribPointer);
	S_LOAD(glEnableVertexAttribArray);
	S_LOAD(glGenFramebuffers);
	S_LOAD(glDeleteFramebuffers);
	S_LOAD(glBindFramebuffer);
	S_LOAD(glFramebufferTexture2D);
	S_LOAD(glFramebufferRenderbuffer);
	S_LOAD(glCheckFramebufferStatus);
	S_LOAD(glGenRenderbuffers);
	S_LOAD(glDeleteRenderbuffers);
	S_LOAD(glBindRenderbuffer);
	S_LOAD(glRenderbufferStorage);
	S_LOAD(glBlitFramebuffer);
	S_LOAD(glActiveTexture);
	S_LOAD(glGenSamplers);
	S_LOAD(glDeleteSamplers);
	S_LOAD(glBindSampler);
	S_LOAD(glSamplerParameteri);
	S_LOAD(glBlendFuncSeparate);
	S_LOAD(glBlendEquation);
	S_LOAD(glGenerateMipmap);
	S_LOAD(glRenderbufferStorageMultisample);
	return true;
}

#undef S_LOAD

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
#ifdef DTTR_COMPONENTS_ENABLED
	GLuint m_overlay_texture;
#endif
} S_OpenglBackendData;

#endif /* DTTR_BACKEND_OPENGL_INTERNAL_H */
