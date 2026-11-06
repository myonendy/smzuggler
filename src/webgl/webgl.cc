/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Native WebGL2 context over GLES 3.0 / GL 3.3 core (spec section 4.2).
 *
 * Why: real PIXI and UltraMode7 call canvas.getContext('webgl2') and drive the
 * GL state machine directly. This file owns the context, C state and V8 objects
 * because the hot calls must be v8::CFunction fast calls rather than only jsb
 * slow callbacks, and each wrapper owns exactly one GL name (spec section
 * 15.8). Textures, framebuffers, renderbuffers, readback, the shader pipeline
 * (including the desktop-GL GLSL ES rewrite) and driver-probed extensions live
 * here.
 *
 * Rules: main thread only. No v8::Local is stored in C; explicit deletes free
 * names now, GC only queues them (and a collected context) for a main-thread
 * drain outside GC, because spec section 15.8 forbids GL calls in a finalizer.
 */
#include "webgl/webgl.h"

#include "jsb/jsb.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_egl.h>
#include <SDL3/SDL_opengl.h>
#include <v8-fast-api-calls.h>
#include <v8.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

namespace {

using v8::Array;
using v8::ArrayBuffer;
using v8::ArrayBufferView;
using v8::Boolean;
using v8::CFunction;
using v8::Context;
using v8::Exception;
using v8::External;
using v8::FastApiCallbackOptions;
using v8::Float32Array;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Global;
using v8::Int32Array;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Name;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::PropertyCallbackInfo;
using v8::String;
using v8::TryCatch;
using v8::Value;
using v8::WeakCallbackInfo;

/* ---- GL entry points -------------------------------------------------- */

/* One resolved entry point per call; avoids the PFN name split between APIs. */
struct GL {
    GLenum (APIENTRY *GetError)(void);
    void (APIENTRY *GetIntegerv)(GLenum, GLint *);
    void (APIENTRY *GetFloatv)(GLenum, GLfloat *);
    const GLubyte *(APIENTRY *GetString)(GLenum);
    const GLubyte *(APIENTRY *GetStringi)(GLenum, GLuint);
    void (APIENTRY *Viewport)(GLint, GLint, GLsizei, GLsizei);
    void (APIENTRY *ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (APIENTRY *Clear)(GLbitfield);
    void (APIENTRY *Enable)(GLenum);
    void (APIENTRY *Disable)(GLenum);
    GLboolean (APIENTRY *IsEnabled)(GLenum);
    void (APIENTRY *GenBuffers)(GLsizei, GLuint *);
    void (APIENTRY *DeleteBuffers)(GLsizei, const GLuint *);
    void (APIENTRY *BindBuffer)(GLenum, GLuint);
    void (APIENTRY *BufferData)(GLenum, GLsizeiptr, const void *, GLenum);
    void (APIENTRY *BufferSubData)(GLenum, GLintptr, GLsizeiptr, const void *);
    void (APIENTRY *GenVertexArrays)(GLsizei, GLuint *);
    void (APIENTRY *DeleteVertexArrays)(GLsizei, const GLuint *);
    void (APIENTRY *BindVertexArray)(GLuint);
    void (APIENTRY *EnableVertexAttribArray)(GLuint);
    void (APIENTRY *DisableVertexAttribArray)(GLuint);
    void (APIENTRY *VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean,
                                         GLsizei, const void *);
    void (APIENTRY *DrawArrays)(GLenum, GLint, GLsizei);
    void (APIENTRY *DrawElements)(GLenum, GLsizei, GLenum, const void *);
    void (APIENTRY *ActiveTexture)(GLenum);
    void (APIENTRY *BindTexture)(GLenum, GLuint);
    void (APIENTRY *UseProgram)(GLuint);
    void (APIENTRY *Uniform1f)(GLint, GLfloat);
    void (APIENTRY *Uniform1i)(GLint, GLint);
    void (APIENTRY *Uniform4f)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);
    void (APIENTRY *Uniform1fv)(GLint, GLsizei, const GLfloat *);
    void (APIENTRY *Uniform2fv)(GLint, GLsizei, const GLfloat *);
    void (APIENTRY *Uniform3fv)(GLint, GLsizei, const GLfloat *);
    void (APIENTRY *Uniform4fv)(GLint, GLsizei, const GLfloat *);
    void (APIENTRY *UniformMatrix2fv)(GLint, GLsizei, GLboolean,
                                      const GLfloat *);
    void (APIENTRY *UniformMatrix3fv)(GLint, GLsizei, GLboolean,
                                      const GLfloat *);
    void (APIENTRY *UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void (APIENTRY *Uniform2f)(GLint, GLfloat, GLfloat);
    void (APIENTRY *Uniform3f)(GLint, GLfloat, GLfloat, GLfloat);
    void (APIENTRY *BindFramebuffer)(GLenum, GLuint);
    /* Texture, framebuffer and renderbuffer entry points (card M2-T002). */
    void (APIENTRY *GenTextures)(GLsizei, GLuint *);
    void (APIENTRY *DeleteTextures)(GLsizei, const GLuint *);
    void (APIENTRY *TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint,
                                GLenum, GLenum, const void *);
    void (APIENTRY *TexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei,
                                   GLenum, GLenum, const void *);
    void (APIENTRY *TexParameteri)(GLenum, GLenum, GLint);
    void (APIENTRY *PixelStorei)(GLenum, GLint);
    void (APIENTRY *GenFramebuffers)(GLsizei, GLuint *);
    void (APIENTRY *DeleteFramebuffers)(GLsizei, const GLuint *);
    void (APIENTRY *FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
    void (APIENTRY *FramebufferRenderbuffer)(GLenum, GLenum, GLenum, GLuint);
    GLenum (APIENTRY *CheckFramebufferStatus)(GLenum);
    void (APIENTRY *GenRenderbuffers)(GLsizei, GLuint *);
    void (APIENTRY *DeleteRenderbuffers)(GLsizei, const GLuint *);
    void (APIENTRY *BindRenderbuffer)(GLenum, GLuint);
    void (APIENTRY *RenderbufferStorage)(GLenum, GLenum, GLsizei, GLsizei);
    void (APIENTRY *ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,
                                void *);
    void (APIENTRY *StencilFunc)(GLenum, GLint, GLuint);
    void (APIENTRY *StencilOp)(GLenum, GLenum, GLenum);
    void (APIENTRY *StencilMask)(GLuint);
    void (APIENTRY *ClearStencil)(GLint);
    void (APIENTRY *DepthFunc)(GLenum);
    void (APIENTRY *DepthMask)(GLboolean);
    void (APIENTRY *ColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
    /* Shader pipeline and introspection; GLSL ES is rewritten on desktop GL
     * core before it reaches the driver. */
    GLuint (APIENTRY *CreateShader)(GLenum);
    void (APIENTRY *ShaderSource)(GLuint, GLsizei, const GLchar *const *,
                                  const GLint *);
    void (APIENTRY *CompileShader)(GLuint);
    GLuint (APIENTRY *CreateProgram)(void);
    void (APIENTRY *AttachShader)(GLuint, GLuint);
    void (APIENTRY *LinkProgram)(GLuint);
    void (APIENTRY *GetShaderiv)(GLuint, GLenum, GLint *);
    void (APIENTRY *GetProgramiv)(GLuint, GLenum, GLint *);
    void (APIENTRY *GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
    void (APIENTRY *GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
    void (APIENTRY *DeleteShader)(GLuint);
    void (APIENTRY *DeleteProgram)(GLuint);
    void (APIENTRY *BindAttribLocation)(GLuint, GLuint, const GLchar *);
    GLint (APIENTRY *GetUniformLocation)(GLuint, const GLchar *);
    GLint (APIENTRY *GetAttribLocation)(GLuint, const GLchar *);
    void (APIENTRY *GetActiveUniform)(GLuint, GLuint, GLsizei, GLsizei *,
                                      GLint *, GLenum *, GLchar *);
    void (APIENTRY *GetActiveAttrib)(GLuint, GLuint, GLsizei, GLsizei *,
                                     GLint *, GLenum *, GLchar *);
    void (APIENTRY *TexParameterf)(GLenum, GLenum, GLfloat);
};

static GL gl;

/* Loads one entry point through SDL; `missing` counts failures. */
#define LOAD(name, field)                                                      \
    do {                                                                       \
        SDL_FunctionPointer proc = SDL_GL_GetProcAddress(#name);               \
        memcpy(&gl.field, &proc, sizeof(proc));                                \
        if (!gl.field) missing++;                                              \
    } while (0)

/* Loads context, buffer, vertex and uniform entry points; returns misses. */
static int gl_load_context(void)
{
    int missing = 0;
    LOAD(glGetError, GetError);
    LOAD(glGetIntegerv, GetIntegerv);
    LOAD(glGetFloatv, GetFloatv);
    LOAD(glGetString, GetString);
    LOAD(glGetStringi, GetStringi);
    LOAD(glViewport, Viewport);
    LOAD(glClearColor, ClearColor);
    LOAD(glClear, Clear);
    LOAD(glEnable, Enable);
    LOAD(glDisable, Disable);
    LOAD(glIsEnabled, IsEnabled);
    LOAD(glGenBuffers, GenBuffers);
    LOAD(glDeleteBuffers, DeleteBuffers);
    LOAD(glBindBuffer, BindBuffer);
    LOAD(glBufferData, BufferData);
    LOAD(glBufferSubData, BufferSubData);
    LOAD(glGenVertexArrays, GenVertexArrays);
    LOAD(glDeleteVertexArrays, DeleteVertexArrays);
    LOAD(glBindVertexArray, BindVertexArray);
    LOAD(glEnableVertexAttribArray, EnableVertexAttribArray);
    LOAD(glDisableVertexAttribArray, DisableVertexAttribArray);
    LOAD(glVertexAttribPointer, VertexAttribPointer);
    LOAD(glDrawArrays, DrawArrays);
    LOAD(glDrawElements, DrawElements);
    LOAD(glActiveTexture, ActiveTexture);
    LOAD(glBindTexture, BindTexture);
    LOAD(glUseProgram, UseProgram);
    LOAD(glUniform1f, Uniform1f);
    LOAD(glUniform1i, Uniform1i);
    LOAD(glUniform4f, Uniform4f);
    LOAD(glUniform1fv, Uniform1fv);
    LOAD(glUniform2fv, Uniform2fv);
    LOAD(glUniform3fv, Uniform3fv);
    LOAD(glUniform4fv, Uniform4fv);
    LOAD(glUniformMatrix2fv, UniformMatrix2fv);
    LOAD(glUniformMatrix3fv, UniformMatrix3fv);
    LOAD(glUniformMatrix4fv, UniformMatrix4fv);
    LOAD(glUniform2f, Uniform2f);
    LOAD(glUniform3f, Uniform3f);
    return missing;
}
/* Loads texture, framebuffer, renderbuffer and raster entry points. */
static int gl_load_objects(void)
{
    int missing = 0;
    LOAD(glBindFramebuffer, BindFramebuffer);
    LOAD(glGenTextures, GenTextures);
    LOAD(glDeleteTextures, DeleteTextures);
    LOAD(glTexImage2D, TexImage2D);
    LOAD(glTexSubImage2D, TexSubImage2D);
    LOAD(glTexParameteri, TexParameteri);
    LOAD(glTexParameterf, TexParameterf);
    LOAD(glPixelStorei, PixelStorei);
    LOAD(glGenFramebuffers, GenFramebuffers);
    LOAD(glDeleteFramebuffers, DeleteFramebuffers);
    LOAD(glFramebufferTexture2D, FramebufferTexture2D);
    LOAD(glFramebufferRenderbuffer, FramebufferRenderbuffer);
    LOAD(glCheckFramebufferStatus, CheckFramebufferStatus);
    LOAD(glGenRenderbuffers, GenRenderbuffers);
    LOAD(glDeleteRenderbuffers, DeleteRenderbuffers);
    LOAD(glBindRenderbuffer, BindRenderbuffer);
    LOAD(glRenderbufferStorage, RenderbufferStorage);
    LOAD(glReadPixels, ReadPixels);
    LOAD(glStencilFunc, StencilFunc);
    LOAD(glStencilOp, StencilOp);
    LOAD(glStencilMask, StencilMask);
    LOAD(glClearStencil, ClearStencil);
    LOAD(glDepthFunc, DepthFunc);
    LOAD(glDepthMask, DepthMask);
    LOAD(glColorMask, ColorMask);
    return missing;
}
/* Loads shader/program creation, linking and introspection entry points. */
static int gl_load_shaders(void)
{
    int missing = 0;
    LOAD(glCreateShader, CreateShader);
    LOAD(glShaderSource, ShaderSource);
    LOAD(glCompileShader, CompileShader);
    LOAD(glCreateProgram, CreateProgram);
    LOAD(glAttachShader, AttachShader);
    LOAD(glLinkProgram, LinkProgram);
    LOAD(glGetShaderiv, GetShaderiv);
    LOAD(glGetProgramiv, GetProgramiv);
    LOAD(glGetShaderInfoLog, GetShaderInfoLog);
    LOAD(glGetProgramInfoLog, GetProgramInfoLog);
    LOAD(glDeleteShader, DeleteShader);
    LOAD(glDeleteProgram, DeleteProgram);
    LOAD(glBindAttribLocation, BindAttribLocation);
    LOAD(glGetUniformLocation, GetUniformLocation);
    LOAD(glGetAttribLocation, GetAttribLocation);
    LOAD(glGetActiveUniform, GetActiveUniform);
    LOAD(glGetActiveAttrib, GetActiveAttrib);
    return missing;
}
/* Resolves the entry points this card needs after a context is current. */
static bool gl_load(void)
{
    int missing = gl_load_context() + gl_load_objects() + gl_load_shaders();
    if (missing) fprintf(stderr, "webgl: %d GL entry point(s) missing\n", missing);
    return missing == 0;
}

/* The window and GL context the module draws on. `s_borrowed_context` belongs
 * to the platform's window and is never destroyed here; `s_owned_context` is
 * the hidden fallback window's context, which this module created. An EGL
 * offscreen context (pbuffer or surfaceless) is preferred on the GPU backend
 * so a second SDL window does not appear to the compositor. */
static bool s_backend_checked;
static bool s_backend_ready;
static SDL_Window *s_backend_window;
static SDL_GLContext s_borrowed_context;
static SDL_GLContext s_owned_context;
static bool s_egl_offscreen;
static EGLDisplay s_egl_display = EGL_NO_DISPLAY;
static EGLContext s_egl_context = EGL_NO_CONTEXT;
static EGLSurface s_egl_surface = EGL_NO_SURFACE;

/* Returns the context the module draws on, owned or borrowed, or NULL. */
static SDL_GLContext backend_context(void)
{
    return s_owned_context ? s_owned_context : s_borrowed_context;
}
/* Makes the module's GL context current; needed before deleting GL names. */
static bool backend_make_current(void)
{
    if (s_egl_offscreen) {
        PFNEGLMAKECURRENTPROC makeCurrent =
            (PFNEGLMAKECURRENTPROC)SDL_EGL_GetProcAddress("eglMakeCurrent");
        return makeCurrent && s_egl_display != EGL_NO_DISPLAY &&
               s_egl_context != EGL_NO_CONTEXT &&
               makeCurrent(s_egl_display, s_egl_surface, s_egl_surface,
                           s_egl_context) == EGL_TRUE;
    }
    SDL_GLContext context = backend_context();
    return s_backend_window && context &&
           SDL_GL_MakeCurrent(s_backend_window, context);
}
/* Creates a GLES 3 / GL 3.3 context with no extra SDL window: surfaceless
 * when the driver allows it, otherwise a 16x16 pbuffer. */
static bool backend_try_egl_offscreen(void)
{
    PFNEGLGETDISPLAYPROC getDisplay =
        (PFNEGLGETDISPLAYPROC)SDL_EGL_GetProcAddress("eglGetDisplay");
    PFNEGLINITIALIZEPROC initialize =
        (PFNEGLINITIALIZEPROC)SDL_EGL_GetProcAddress("eglInitialize");
    PFNEGLBINDAPIPROC bindApi =
        (PFNEGLBINDAPIPROC)SDL_EGL_GetProcAddress("eglBindAPI");
    PFNEGLCHOOSECONFIGPROC chooseConfig =
        (PFNEGLCHOOSECONFIGPROC)SDL_EGL_GetProcAddress("eglChooseConfig");
    PFNEGLCREATECONTEXTPROC createContext =
        (PFNEGLCREATECONTEXTPROC)SDL_EGL_GetProcAddress("eglCreateContext");
    PFNEGLCREATEPBUFFERSURFACEPROC createPbuffer =
        (PFNEGLCREATEPBUFFERSURFACEPROC)SDL_EGL_GetProcAddress(
            "eglCreatePbufferSurface");
    PFNEGLMAKECURRENTPROC makeCurrent =
        (PFNEGLMAKECURRENTPROC)SDL_EGL_GetProcAddress("eglMakeCurrent");
    PFNEGLDESTROYCONTEXTPROC destroyContext =
        (PFNEGLDESTROYCONTEXTPROC)SDL_EGL_GetProcAddress("eglDestroyContext");
    PFNEGLDESTROYSURFACEPROC destroySurface =
        (PFNEGLDESTROYSURFACEPROC)SDL_EGL_GetProcAddress("eglDestroySurface");
    if (!getDisplay || !initialize || !bindApi || !chooseConfig ||
        !createContext || !makeCurrent)
        return false;

    EGLDisplay dpy = getDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY || !initialize(dpy, NULL, NULL)) return false;

    const EGLint cfg_es[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    const EGLint cfg_gl[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    const EGLint ctx_es[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    const EGLint ctx_gl[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
#ifdef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#endif
        EGL_NONE
    };
    const EGLint pbuf[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };

    struct Attempt { EGLenum api; const EGLint *cfg; const EGLint *ctx; };
    const Attempt attempts[] = {
        { EGL_OPENGL_ES_API, cfg_es, ctx_es },
        { EGL_OPENGL_API, cfg_gl, ctx_gl },
    };

    for (size_t i = 0; i < sizeof(attempts) / sizeof(attempts[0]); i++) {
        if (!bindApi(attempts[i].api)) continue;
        EGLConfig config;
        EGLint n = 0;
        if (!chooseConfig(dpy, attempts[i].cfg, &config, 1, &n) || n < 1)
            continue;
        EGLContext ctx = createContext(dpy, config, EGL_NO_CONTEXT,
                                       attempts[i].ctx);
        if (ctx == EGL_NO_CONTEXT) continue;
        /* A pbuffer has a default framebuffer, which WebGL clear/readPixels
         * and the existing tests need; surfaceless does not. Prefer pbuffer
         * so we never create a second compositor window. */
        if (createPbuffer) {
            EGLSurface surface = createPbuffer(dpy, config, pbuf);
            if (surface != EGL_NO_SURFACE &&
                makeCurrent(dpy, surface, surface, ctx) == EGL_TRUE) {
                s_egl_display = dpy;
                s_egl_context = ctx;
                s_egl_surface = surface;
                s_egl_offscreen = true;
                fprintf(stderr, "webgl: using an EGL pbuffer context\n");
                return true;
            }
            if (surface != EGL_NO_SURFACE && destroySurface)
                destroySurface(dpy, surface);
        }
        if (makeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) == EGL_TRUE) {
            s_egl_display = dpy;
            s_egl_context = ctx;
            s_egl_surface = EGL_NO_SURFACE;
            s_egl_offscreen = true;
            fprintf(stderr, "webgl: using an EGL surfaceless context\n");
            return true;
        }
        if (destroyContext) destroyContext(dpy, ctx);
    }
    return false;
}
/* Uses an existing OpenGL window, an EGL offscreen context, or a hidden
 * utility window that should stay out of the compositor's task list. */
static bool backend_create(void)
{
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_Window *window = NULL;
    for (int i = 0; windows && i < count; i++)
        if (SDL_GetWindowFlags(windows[i]) & SDL_WINDOW_OPENGL) {
            window = windows[i];
            break;
        }
    if (windows) SDL_free(windows);
    if (window) {
        s_borrowed_context = SDL_GL_GetCurrentContext();
        s_backend_window = window;
        return s_borrowed_context &&
               SDL_GL_MakeCurrent(window, s_borrowed_context);
    }

    if (backend_try_egl_offscreen()) return true;

    s_backend_window = SDL_CreateWindow(
        "smzuggler WebGL", 16, 16,
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_UTILITY |
            SDL_WINDOW_NOT_FOCUSABLE);
    if (!s_backend_window) return false;
    fprintf(stderr,
            "webgl: created a hidden fallback window "
            "(EGL offscreen unavailable)\n");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    s_owned_context = SDL_GL_CreateContext(s_backend_window);
    if (!s_owned_context) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        s_owned_context = SDL_GL_CreateContext(s_backend_window);
    }
    return s_owned_context &&
           SDL_GL_MakeCurrent(s_backend_window, s_owned_context);
}
/* True when the resolved backend is OpenGL ES rather than desktop GL core.
 * WebGL2 shaders need rewriting only on the desktop-core path (spec §4.2). */
static bool s_gl_es;
/* Extensions this backend really provides; PIXI 5.3.12 probes these names.
 * `getExtension` must never answer with one that is not listed here. */
enum ExtensionId {
    EXT_ANISOTROPIC = 0,
    EXT_COLOR_BUFFER_FLOAT = 1,
    EXT_FLOAT_LINEAR = 2,
    EXT_COUNT = 3,
};
static const char *const kExtensionNames[EXT_COUNT] = {
    "EXT_texture_filter_anisotropic",
    "EXT_color_buffer_float",
    "OES_texture_float_linear",
};
static bool s_ext_supported[EXT_COUNT];

/* Reports whether the driver's extension string lists `name`. GLES exposes
 * the whole space-separated list through GL_EXTENSIONS; a desktop core
 * context makes that query invalid and requires the indexed form instead. */
static bool gl_extension_listed(const char *name)
{
    if (s_gl_es) {
        const char *list = (const char *)gl.GetString(GL_EXTENSIONS);
        if (!list) return false;
        const char *p = list;
        while (*p) {
            while (*p == ' ') p++;
            const char *start = p;
            while (*p && *p != ' ') p++;
            size_t length = (size_t)(p - start);
            if (strlen(name) == length && !strncmp(start, name, length))
                return true;
        }
        return false;
    }
    GLint count = 0;
    gl.GetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; i++) {
        const char *extension = (const char *)gl.GetStringi(GL_EXTENSIONS,
                                                            (GLuint)i);
        if (extension && !strcmp(extension, name)) return true;
    }
    return false;
}
/* Records the extensions the current driver can back. Desktop GL 3.3 core has
 * float colour buffers and float linear filtering in core, while GLES needs
 * the matching extensions. Anisotropic filtering is always optional. */
static void detect_extensions(void)
{
    s_ext_supported[EXT_ANISOTROPIC] =
        gl_extension_listed("GL_EXT_texture_filter_anisotropic");
    if (s_gl_es) {
        s_ext_supported[EXT_COLOR_BUFFER_FLOAT] =
            gl_extension_listed("GL_EXT_color_buffer_float");
        s_ext_supported[EXT_FLOAT_LINEAR] =
            gl_extension_listed("GL_OES_texture_float_linear");
    } else {
        s_ext_supported[EXT_COLOR_BUFFER_FLOAT] = true;
        s_ext_supported[EXT_FLOAT_LINEAR] = true;
    }
}
/* Brings up SDL video, a GL context and entry points exactly once. */
static bool backend_ready(void)
{
    if (s_backend_checked) return s_backend_ready;
    s_backend_checked = true;
    if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO))
        return false;
    if (!SDL_GL_LoadLibrary(NULL)) return false;
    s_backend_ready = backend_create() && gl_load();
    if (s_backend_ready) {
        const char *version = (const char *)gl.GetString(GL_VERSION);
        s_gl_es = version && strstr(version, "OpenGL ES") != NULL;
        detect_extensions();
    }
    return s_backend_ready;
}

#define MAX_CONTEXTS 8

/* One queued GC delete, drained on the main thread (spec section 15.8). */
struct Pending { Pending *next; GLuint name; int kind; };
/* Kinds of GL name a wrapper can own; each wrapper owns exactly one. */
enum GLObjectKind {
    GL_OBJECT_BUFFER = 0,
    GL_OBJECT_VERTEX_ARRAY = 1,
    GL_OBJECT_TEXTURE = 2,
    GL_OBJECT_FRAMEBUFFER = 3,
    GL_OBJECT_RENDERBUFFER = 4,
    GL_OBJECT_SHADER = 5,
    GL_OBJECT_PROGRAM = 6,
};
/* One wrapper-owned GL name; `kind` selects the delete entry point. */
struct GLObject;

/* One WebGL2 context, its drawing-buffer size and the wrappers it owns. */
struct GLContext {
    uint32_t id;
    int width;
    int height;
    Pending *pending;
    GLObject *objects;
    GLenum synth_error;
    /* Unpack state applied to Canvas2D/image sources only; typed arrays keep
     * the GL defaults (WebGL2 spec 5.14.8 pixel storage parameters). */
    GLint unpack_flip_y;
    GLint unpack_premultiply_alpha;
    GLint unpack_alignment;
    GLint pack_alignment;
    GLint unpack_row_length;
    GLint pack_row_length;
};

/* A wrapper-owned GL name; `context` is NULL once the context is gone.
 * `source` keeps a shader's original text so a compile failure can log the
 * author's lines (spec section 15.8). `compiled` is what the driver received:
 * on desktop GL core it is the rewritten GLSL, and on GLES it is a copy of
 * `source`. Program wrappers leave both NULL. */
struct GLObject {
    GLContext *context;
    GLObject *next;
    GLuint name;
    int kind;
    bool deleted;
    int shader_type;
    char *source;
    char *compiled;
};

/* Frees one GL name of `kind`; only called on the main thread with a current
 * context. A zero name is ignored, as the delete entry points do. */
static void delete_name(int kind, GLuint name)
{
    if (!name) return;
    switch (kind) {
    case GL_OBJECT_BUFFER: gl.DeleteBuffers(1, &name); break;
    case GL_OBJECT_VERTEX_ARRAY: gl.DeleteVertexArrays(1, &name); break;
    case GL_OBJECT_TEXTURE: gl.DeleteTextures(1, &name); break;
    case GL_OBJECT_FRAMEBUFFER: gl.DeleteFramebuffers(1, &name); break;
    case GL_OBJECT_RENDERBUFFER: gl.DeleteRenderbuffers(1, &name); break;
    case GL_OBJECT_SHADER: gl.DeleteShader(name); break;
    case GL_OBJECT_PROGRAM: gl.DeleteProgram(name); break;
    default: break;
    }
}
/* Creates one GL name of `kind`, or 0 when the driver refuses. */
static GLuint gen_name(int kind)
{
    GLuint name = 0;
    switch (kind) {
    case GL_OBJECT_BUFFER: gl.GenBuffers(1, &name); break;
    case GL_OBJECT_VERTEX_ARRAY: gl.GenVertexArrays(1, &name); break;
    case GL_OBJECT_TEXTURE: gl.GenTextures(1, &name); break;
    case GL_OBJECT_FRAMEBUFFER: gl.GenFramebuffers(1, &name); break;
    case GL_OBJECT_RENDERBUFFER: gl.GenRenderbuffers(1, &name); break;
    default: break;
    }
    return name;
}

static GLContext *s_contexts[MAX_CONTEXTS];
/* Contexts whose JS wrapper was collected, drained outside GC. */
static GLContext *s_destroy_queue[MAX_CONTEXTS];
static size_t s_destroy_count;
/* True when a GC finalizer queued at least one GL name. Kept separate so the
 * hot paths pay one predictable branch instead of walking every context. */
static bool s_pending_any;

/* Returns `ctx` while it is registered, else NULL. */
static GLContext *live(GLContext *ctx)
{
    if (!ctx || ctx->id == 0 || ctx->id > MAX_CONTEXTS) return NULL;
    return s_contexts[ctx->id - 1] == ctx ? ctx : NULL;
}
/* Applies WebGL2's documented initial state to a new context. */
static void default_state(GLContext *ctx)
{
    gl.Viewport(0, 0, ctx->width, ctx->height);
    gl.ClearColor(0, 0, 0, 0);
    gl.Disable(GL_DEPTH_TEST);
    gl.Disable(GL_STENCIL_TEST);
    gl.Disable(GL_BLEND);
    gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_SCISSOR_TEST);
    gl.Disable(GL_POLYGON_OFFSET_FILL);
    gl.Enable(GL_DITHER);
    /* WebGL2 pixel-storage defaults: alignment 4, no flip, no premultiply. */
    ctx->unpack_flip_y = 0;
    ctx->unpack_premultiply_alpha = 0;
    ctx->unpack_alignment = 4;
    ctx->pack_alignment = 4;
    ctx->unpack_row_length = 0;
    ctx->pack_row_length = 0;
}
/* Creates a context in the first free slot; NULL when GL is unavailable. */
static GLContext *context_create(int width, int height)
{
    if (!backend_ready()) return NULL;
    int slot = -1;
    for (int i = 0; i < MAX_CONTEXTS; i++)
        if (!s_contexts[i]) { slot = i; break; }
    if (slot < 0) return NULL;
    GLContext *ctx = (GLContext *)calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    ctx->id = (uint32_t)slot + 1;
    ctx->width = width > 0 ? width : 1;
    ctx->height = height > 0 ? height : 1;
    s_contexts[slot] = ctx;
    default_state(ctx);
    if (gl.GetError() != GL_NO_ERROR) {
        s_contexts[slot] = NULL;
        free(ctx);
        return NULL;
    }
    return ctx;
}
/* Deletes names queued by collected wrappers and frees the queue nodes. */
static void drain_pending(GLContext *ctx)
{
    Pending *node = ctx->pending;
    ctx->pending = NULL;
    while (node) {
        Pending *next = node->next;
        delete_name(node->kind, node->name);
        free(node);
        node = next;
    }
}
/* Drains every context's GC queue. Only the main thread calls this, and only
 * outside a finalizer: spec section 15.8 lets a finalizer queue a name but not
 * touch GL. A missing current context leaves the queue for the next drain. */
static void drain_pending_all(void)
{
    if (!s_pending_any) return;
    if (!backend_make_current()) return;
    s_pending_any = false;
    for (int i = 0; i < MAX_CONTEXTS; i++)
        if (s_contexts[i]) drain_pending(s_contexts[i]);
}
/* Deletes the names live wrappers still hold, then detaches the wrappers.
 * The records themselves belong to the JS wrappers and are not freed here. */
static void release_objects(GLContext *ctx)
{
    for (GLObject *object = ctx->objects; object; object = object->next) {
        if (!object->deleted && object->name) {
            delete_name(object->kind, object->name);
        }
        free(object->source);
        free(object->compiled);
        object->source = NULL;
        object->compiled = NULL;
        object->name = 0;
        object->deleted = true;
        object->context = NULL;
    }
    ctx->objects = NULL;
}
/* Forgets the live wrappers' names without GL, for a lost context. */
static void forget_objects(GLContext *ctx)
{
    for (GLObject *object = ctx->objects; object; object = object->next) {
        free(object->source);
        free(object->compiled);
        object->source = NULL;
        object->compiled = NULL;
        object->name = 0;
        object->deleted = true;
        object->context = NULL;
    }
    ctx->objects = NULL;
}
/* Frees the queued name nodes without GL, for a lost context. */
static void forget_pending(GLContext *ctx)
{
    Pending *node = ctx->pending;
    ctx->pending = NULL;
    while (node) {
        Pending *next = node->next;
        free(node);
        node = next;
    }
}
/* Destroys a context; runs on the main thread, never inside a GC callback. */
static void context_destroy(GLContext *ctx)
{
    if (!ctx) return;
    if (ctx->id > 0 && ctx->id <= MAX_CONTEXTS &&
        s_contexts[ctx->id - 1] == ctx)
        s_contexts[ctx->id - 1] = NULL;
    if (backend_make_current()) {
        release_objects(ctx);
        drain_pending(ctx);
    } else {
        /* Context lost or window gone: forget names, never call GL blindly. */
        forget_objects(ctx);
        forget_pending(ctx);
    }
    free(ctx);
}
/* Queues a collected context for the next main-thread drain; no GL here. */
static void context_enqueue(GLContext *ctx)
{
    if (s_destroy_count < MAX_CONTEXTS) s_destroy_queue[s_destroy_count++] = ctx;
}
/* Destroys every queued context; the only place GL can run after GC. */
static void drain_destroy_queue(void)
{
    size_t count = s_destroy_count;
    s_destroy_count = 0;
    for (size_t i = 0; i < count; i++) context_destroy(s_destroy_queue[i]);
}
/* Wraps an existing GL name of `kind` in a record linked into `ctx`. */
static GLObject *object_wrap(GLContext *ctx, int kind, GLuint name)
{
    GLObject *object = name ? (GLObject *)calloc(1, sizeof(*object)) : NULL;
    if (!object) {
        if (name) delete_name(kind, name);
        return NULL;
    }
    object->context = ctx;
    object->next = ctx->objects;
    ctx->objects = object;
    object->name = name;
    object->kind = kind;
    return object;
}
/* Creates one wrapper record for a new GL name and links it into its context. */
static GLObject *object_create(GLContext *ctx, int kind)
{
    return object_wrap(ctx, kind, gen_name(kind));
}
/* Unlinks `object` from its context's live-wrapper list, if linked. */
static void object_unlink(GLObject *object)
{
    GLContext *ctx = object->context;
    if (!ctx) return;
    GLObject **link = &ctx->objects;
    while (*link && *link != object) link = &(*link)->next;
    if (*link) *link = object->next;
    object->context = NULL;
    object->next = NULL;
}
/* Explicit delete: releases the name now and marks the wrapper dead. */
static void object_delete(GLObject *object)
{
    if (!object || object->deleted) return;
    if (object->context && object->name) {
        delete_name(object->kind, object->name);
    }
    object_unlink(object);
    free(object->source);
    free(object->compiled);
    object->source = NULL;
    object->compiled = NULL;
    object->name = 0;
    object->deleted = true;
}
/* GC delete: queue the name for the context drain, then free the record. */
static void object_gc(GLObject *object)
{
    if (!object) return;
    GLContext *ctx = object->context;
    if (ctx && !object->deleted && object->name) {
        Pending *node = (Pending *)malloc(sizeof(*node));
        if (node) {
            node->name = object->name;
            node->kind = object->kind;
            node->next = ctx->pending;
            ctx->pending = node;
            s_pending_any = true;
        }
    }
    object_unlink(object);
    free(object->source);
    free(object->compiled);
    free(object);
}
static const char *g_fast_names[32];
static size_t g_fast_count;

/* Records a CFunction registration once for the fast-api test. */
static void record_fast(const char *name)
{
    for (size_t i = 0; i < g_fast_count; i++)
        if (!strcmp(g_fast_names[i], name)) return;
    if (g_fast_count < 32) g_fast_names[g_fast_count++] = name;
}
/* Returns a V8 string for borrowed UTF-8 `text`. */
static Local<String> utf8(Isolate *isolate, const char *text)
{
    return String::NewFromUtf8(isolate, text).ToLocalChecked();
}
/* Throws a TypeError with `message`. */
static void throw_type(Isolate *isolate, const char *message)
{
    isolate->ThrowException(Exception::TypeError(utf8(isolate, message)));
}
/* Throws the receiver error both paths share, as Chrome does. */
static void throw_illegal(Isolate *isolate)
{
    throw_type(isolate, "Illegal invocation");
}
/* Reads ToInt32; missing -> 0. */
static int32_t arg_i32(Isolate *isolate, const FunctionCallbackInfo<Value> &info,
                       int index)
{
    if (index >= info.Length()) return 0;
    return info[index]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0);
}
/* Reads ToUint32; missing -> 0. */
static uint32_t arg_u32(Isolate *isolate, const FunctionCallbackInfo<Value> &info,
                        int index)
{
    if (index >= info.Length()) return 0;
    return info[index]->Uint32Value(isolate->GetCurrentContext()).FromMaybe(0);
}
/* Reads a float; missing -> 0. */
static float arg_f32(Isolate *isolate, const FunctionCallbackInfo<Value> &info,
                     int index)
{
    if (index >= info.Length()) return 0;
    return (float)info[index]->NumberValue(isolate->GetCurrentContext())
        .FromMaybe(0);
}
/* Reads an int64; missing -> 0. */
static int64_t arg_i64(Isolate *isolate, const FunctionCallbackInfo<Value> &info,
                       int index)
{
    if (index >= info.Length()) return 0;
    return info[index]->IntegerValue(isolate->GetCurrentContext()).FromMaybe(0);
}
/* Reads truthiness; missing -> false. */
static bool arg_bool(Isolate *isolate, const FunctionCallbackInfo<Value> &info,
                     int index)
{
    return index < info.Length() && info[index]->BooleanValue(isolate);
}
/* Returns the context behind a receiver, or NULL when it is not ours. */
static GLContext *context_of(Local<Object> receiver)
{
    if (receiver->InternalFieldCount() < 1) return NULL;
    return static_cast<GLContext *>(
        receiver->GetAlignedPointerFromInternalField(0));
}
/* Returns the wrapper record behind a JS object value, or NULL. */
static GLObject *object_of(Local<Value> value)
{
    if (value->IsNullOrUndefined() || !value->IsObject()) return NULL;
    Local<Object> object = value.As<Object>();
    if (object->InternalFieldCount() < 1) return NULL;
    return static_cast<GLObject *>(
        object->GetAlignedPointerFromInternalField(0));
}
/* Resolves null or a wrapper to a GL name without allocating, so fast calls
 * may use it. A dead or foreign-context wrapper records INVALID_OPERATION, as
 * Chrome reports for use after delete (spec section 15.8). */
static uint32_t gl_name(GLContext *ctx, Local<Value> value)
{
    if (value->IsNullOrUndefined()) return 0;
    if (value->IsObject()) {
        GLObject *object = object_of(value);
        if (!object) return 0;
        if (object->deleted || object->context != ctx) {
            ctx->synth_error = GL_INVALID_OPERATION;
            return 0;
        }
        return object->name;
    }
    if (value->IsUint32()) return (uint32_t)value.As<Integer>()->Value();
    if (value->IsInt32()) return (uint32_t)value.As<Integer>()->Value();
    return 0;
}
/* Bytes borrowed from a JS ArrayBuffer/view. `storage` receives V8's on-heap
 * typed arrays (at most typed_array_max_size_in_heap, 64 in this pin) so a fast
 * call never allocates a backing store. */
struct Bytes {
    const uint8_t *data = NULL;
    size_t len = 0;
    uint8_t storage[64];
};

/* Fills `bytes` from a buffer or view with no JS-heap allocation; false for
 * any other value. The bytes stay valid while the JS value is reachable. */
static bool bytes_of(Local<Value> value, Bytes *bytes)
{
    bytes->data = bytes->storage;
    bytes->len = 0;
    if (value->IsArrayBuffer()) {
        Local<ArrayBuffer> buffer = value.As<ArrayBuffer>();
        const void *data = buffer->Data();
        if (!data) return true;
        bytes->data = (const uint8_t *)data;
        bytes->len = buffer->ByteLength();
        return true;
    }
    if (!value->IsArrayBufferView()) return false;
    Local<ArrayBufferView> view = value.As<ArrayBufferView>();
    if (view->HasBuffer()) {
        /* Off-heap already: point at the backing store and change nothing.
         * Buffer() would externalise an on-heap typed array (allocates). */
        const void *base = view->Buffer()->Data();
        if (base) {
            bytes->data = (const uint8_t *)base + view->ByteOffset();
            bytes->len = view->ByteLength();
        }
        return true;
    }
    bytes->len = view->CopyContents(bytes->storage, sizeof(bytes->storage));
    return true;
}

/* ---- Texture upload sources ------------------------------------------- */

/* Reads one JS property; an empty handle means the read threw or failed.
 * Slow path only, so building the name string is fine. */
static Local<Value> property_value(Local<Context> context, Local<Object> object,
                                   const char *name)
{
    Local<Value> value;
    if (!object->Get(context, utf8(context->GetIsolate(), name)).ToLocal(&value))
        return Local<Value>();
    return value;
}
/* Reads a property as ToUint32; missing or failed reads become 0. */
static uint32_t property_u32(Local<Context> context, Local<Object> object,
                             const char *name)
{
    Local<Value> value = property_value(context, object, name);
    if (value.IsEmpty()) return 0;
    return value->Uint32Value(context).FromMaybe(0);
}

/* One pixel source for texture uploads. Canvas, image, and ImageData sources
 * are copied into JS-owned ArrayBuffers or an allocated buffer; `keep` in the
 * caller pins the converted buffer until the upload finishes, while `owned`
 * covers ImageData copies and flip/premultiply transforms (freed by
 * `pixel_source_release`). */
struct PixelSource {
    const uint8_t *pixels;
    int width;
    int height;
    uint8_t *owned;
};

/* Calls the existing Canvas2D/image binding to get an ArrayBuffer copy of a
 * host object's pixels. Going through those bindings keeps src/webgl free of a
 * link dependency on the renderer and image cache. The returned handle must be
 * kept in the caller's scope until the GL upload finishes. */
static Local<ArrayBuffer> native_pixels(Isolate *isolate, const char *module,
                                        const char *method, uint32_t handle)
{
    Local<Context> context = isolate->GetCurrentContext();
    Local<Value> module_value;
    if (!context->Global()->Get(context, utf8(isolate, module))
             .ToLocal(&module_value) || !module_value->IsObject())
        return Local<ArrayBuffer>();
    Local<Object> object = module_value.As<Object>();
    Local<Value> function;
    if (!object->Get(context, utf8(isolate, method)).ToLocal(&function) ||
        !function->IsFunction())
        return Local<ArrayBuffer>();
    Local<Value> argv[1] = { Integer::NewFromUnsigned(isolate, handle) };
    TryCatch try_catch(isolate);
    Local<Value> result;
    if (!function.As<Function>()->Call(context, object, 1, argv)
             .ToLocal(&result)) {
        try_catch.Reset();
        return Local<ArrayBuffer>();
    }
    if (!result->IsArrayBuffer()) return Local<ArrayBuffer>();
    return result.As<ArrayBuffer>();
}
/* Fills `source` from an HTMLCanvasElement's live Canvas2D raster. */
static bool canvas_source(Local<Context> context, Local<Object> object,
                          PixelSource *source, Local<ArrayBuffer> *keep)
{
    Local<Value> context2d = property_value(context, object, "_context2d");
    if (context2d.IsEmpty() || !context2d->IsObject()) return false;
    uint32_t handle = property_u32(context, context2d.As<Object>(), "_handle");
    uint32_t width = property_u32(context, object, "width");
    uint32_t height = property_u32(context, object, "height");
    if (!handle || !width || !height) return false;
    Local<ArrayBuffer> buffer = native_pixels(
        context->GetIsolate(), "__native_canvas2d", "getPixels", handle);
    size_t size = (size_t)width * (size_t)height * 4u;
    if (buffer.IsEmpty() || !buffer->Data() ||
        buffer->ByteLength() < size) return false;
    *keep = buffer;
    source->pixels = (const uint8_t *)buffer->Data();
    source->width = (int)width;
    source->height = (int)height;
    return true;
}
/* Fills `source` from an HTMLImageElement's decoded RGBA pixels. */
static bool image_source(Local<Context> context, Local<Object> object,
                         PixelSource *source, Local<ArrayBuffer> *keep)
{
    uint32_t handle = property_u32(context, object, "_imageHandle");
    uint32_t width = property_u32(context, object, "width");
    uint32_t height = property_u32(context, object, "height");
    if (!width) width = property_u32(context, object, "naturalWidth");
    if (!height) height = property_u32(context, object, "naturalHeight");
    if (!handle || !width || !height) return false;
    Local<ArrayBuffer> buffer = native_pixels(
        context->GetIsolate(), "__native_image", "getImagePixels", handle);
    size_t size = (size_t)width * (size_t)height * 4u;
    if (buffer.IsEmpty() || !buffer->Data() ||
        buffer->ByteLength() < size) return false;
    *keep = buffer;
    source->pixels = (const uint8_t *)buffer->Data();
    source->width = (int)width;
    source->height = (int)height;
    return true;
}
/* Fills `source` from an ImageData-like object ({width, height, data}).
 * The pixels are copied because the JS typed array may live on the V8 heap. */
static bool image_data_source(Local<Context> context, Local<Object> object,
                              PixelSource *source)
{
    Local<Value> data = property_value(context, object, "data");
    if (data.IsEmpty() || !data->IsArrayBufferView()) return false;
    uint32_t width = property_u32(context, object, "width");
    uint32_t height = property_u32(context, object, "height");
    Bytes bytes;
    if (!width || !height || !bytes_of(data, &bytes)) return false;
    size_t size = (size_t)width * (size_t)height * 4u;
    if (bytes.len < size) return false;
    uint8_t *copy = (uint8_t *)malloc(size);
    if (!copy) return false;
    memcpy(copy, bytes.data, size);
    source->pixels = copy;
    source->width = (int)width;
    source->height = (int)height;
    source->owned = copy;
    return true;
}
/* Resolves any supported host upload source; false for other values. */
static bool pixel_source(Local<Context> context, Local<Value> value,
                         PixelSource *source, Local<ArrayBuffer> *keep)
{
    memset(source, 0, sizeof(*source));
    if (!value->IsObject()) return false;
    Local<Object> object = value.As<Object>();
    if (canvas_source(context, object, source, keep)) return true;
    if (image_source(context, object, source, keep)) return true;
    return image_data_source(context, object, source);
}
/* Releases the copy an ImageData or transform made, never borrowed pixels. */
static void pixel_source_release(PixelSource *source)
{
    free(source->owned);
    source->owned = NULL;
}
/* Bytes per component for the GL types this binding accepts. */
static size_t type_size(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE: return 1;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
    case GL_HALF_FLOAT: return 2;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT: return 4;
    default: return 0;
    }
}
/* Component count for the pixel formats PIXI and Canvas2D upload. */
static size_t format_components(GLenum format)
{
    switch (format) {
    case GL_RGBA: return 4;
    case GL_RGB: return 3;
    case GL_ALPHA:
    case GL_DEPTH_COMPONENT: return 1;
    default: return 0;
    }
}
/* Checks the last byte GL can access, including row padding but excluding
 * padding after the final row. Main-thread only; no caller memory is retained.
 * Unknown formats/types, invalid dimensions and overflow fail before GL. */
static bool pixel_bytes(GLContext *ctx, GLenum format, GLenum type, int width,
                        int height, bool pack, size_t *need)
{
    size_t components = format_components(format);
    size_t unit = type_size(type);
    if (!components || !unit) {
        ctx->synth_error = GL_INVALID_ENUM;
        return false;
    }
    if (width < 0 || height < 0) {
        ctx->synth_error = GL_INVALID_VALUE;
        return false;
    }
    *need = 0;
    if (!width || !height) return true;
    size_t alignment = pack ? ctx->pack_alignment : ctx->unpack_alignment;
    size_t row_length = pack ? ctx->pack_row_length : ctx->unpack_row_length;
    size_t pixel = components * unit;
    size_t columns = row_length ? row_length : (size_t)width;
    if (columns > (SIZE_MAX - alignment + 1) / pixel ||
        (size_t)width > SIZE_MAX / pixel) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return false;
    }
    size_t stride = (columns * pixel + alignment - 1) / alignment * alignment;
    size_t last = (size_t)width * pixel;
    if (stride && (size_t)(height - 1) > (SIZE_MAX - last) / stride) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return false;
    }
    *need = stride * (size_t)(height - 1) + last;
    return true;
}
/* Host buffers are packed RGBA8, independent of the user's array layout.
 * Main-thread only; restore the tracked array state after either host upload. */
static void host_unpack(GLContext *ctx, bool restore)
{
    gl.PixelStorei(GL_UNPACK_ROW_LENGTH, restore ? ctx->unpack_row_length : 0);
    gl.PixelStorei(GL_UNPACK_ALIGNMENT, restore ? ctx->unpack_alignment : 1);
}
/* Applies UNPACK_FLIP_Y_WEBGL and UNPACK_PREMULTIPLY_ALPHA_WEBGL to a host
 * RGBA8 source, returning either the borrowed pixels or a malloc'ed copy.
 * WebGL2 ignores both flags for ArrayBufferView uploads (spec 5.14.8). */
static const uint8_t *transform_rgba(GLContext *ctx, const PixelSource *source,
                                     uint8_t **owned)
{
    *owned = NULL;
    size_t row = (size_t)source->width * 4u;
    bool flip = ctx->unpack_flip_y != 0;
    bool premultiply = ctx->unpack_premultiply_alpha != 0;
    if (!flip && !premultiply) return source->pixels;
    uint8_t *copy = (uint8_t *)malloc(row * (size_t)source->height);
    if (!copy) return source->pixels;
    for (int y = 0; y < source->height; y++) {
        int sy = flip ? source->height - 1 - y : y;
        const uint8_t *src = source->pixels + (size_t)sy * row;
        uint8_t *dst = copy + (size_t)y * row;
        if (!premultiply) {
            memcpy(dst, src, row);
            continue;
        }
        for (int x = 0; x < source->width; x++) {
            uint8_t alpha = src[x * 4 + 3];
            dst[x * 4 + 0] = (uint8_t)((src[x * 4 + 0] * alpha + 127u) / 255u);
            dst[x * 4 + 1] = (uint8_t)((src[x * 4 + 1] * alpha + 127u) / 255u);
            dst[x * 4 + 2] = (uint8_t)((src[x * 4 + 2] * alpha + 127u) / 255u);
            dst[x * 4 + 3] = alpha;
        }
    }
    *owned = copy;
    return copy;
}
/* Uploads borrowed RGBA8 host pixels on the main thread; unsupported layouts
 * record INVALID_OPERATION rather than interpreting the raster as another type. */
static void tex_image_source(GLContext *ctx, GLenum target, GLint level,
                             GLint internalformat, GLenum format, GLenum type,
                             const PixelSource *source)
{
    if (format != GL_RGBA || type != GL_UNSIGNED_BYTE) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return;
    }
    uint8_t *owned = NULL;
    const uint8_t *pixels = source->pixels;
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        pixels = transform_rgba(ctx, source, &owned);
    host_unpack(ctx, false);
    gl.TexImage2D(target, level, internalformat, source->width, source->height,
                  0, format, type, pixels);
    host_unpack(ctx, true);
    free(owned);
}
/* Uploads a host source into an existing texture region. */
static void tex_sub_image_source(GLContext *ctx, GLenum target, GLint level,
                                 GLint xoffset, GLint yoffset, GLenum format,
                                 GLenum type, const PixelSource *source)
{
    if (format != GL_RGBA || type != GL_UNSIGNED_BYTE) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return;
    }
    uint8_t *owned = NULL;
    const uint8_t *pixels = source->pixels;
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE)
        pixels = transform_rgba(ctx, source, &owned);
    host_unpack(ctx, false);
    gl.TexSubImage2D(target, level, xoffset, yoffset, source->width,
                     source->height, format, type, pixels);
    host_unpack(ctx, true);
    free(owned);
}

enum Op {
    OP_VIEWPORT, OP_CLEAR_COLOR, OP_CLEAR, OP_ENABLE, OP_DISABLE, OP_IS_ENABLED,
    OP_GET_ERROR, OP_GET_PARAMETER,
    OP_CREATE_BUFFER, OP_DELETE_BUFFER, OP_BIND_BUFFER, OP_BUFFER_DATA,
    OP_BUFFER_SUB_DATA, OP_CREATE_VAO, OP_DELETE_VAO, OP_BIND_VAO,
    OP_ENABLE_ATTRIB, OP_DISABLE_ATTRIB, OP_VERTEX_ATTRIB, OP_DRAW_ARRAYS,
    OP_DRAW_ELEMENTS, OP_ACTIVE_TEXTURE, OP_BIND_TEXTURE, OP_USE_PROGRAM,
    OP_BIND_FRAMEBUFFER, OP_UNIFORM1F, OP_UNIFORM1I, OP_UNIFORM4F,
    OP_UNIFORM2F, OP_UNIFORM3F, OP_UNIFORM1FV, OP_UNIFORM2FV, OP_UNIFORM3FV,
    OP_UNIFORM4FV, OP_UNIFORM_MATRIX2FV, OP_UNIFORM_MATRIX3FV,
    OP_UNIFORM_MATRIX4FV,
    /* Card M2-T002: textures, framebuffers, renderbuffers and readback. */
    OP_CREATE_TEXTURE, OP_DELETE_TEXTURE, OP_TEX_IMAGE_2D,
    OP_TEX_SUB_IMAGE_2D, OP_TEX_PARAMETERI, OP_PIXEL_STOREI,
    OP_CREATE_FRAMEBUFFER, OP_DELETE_FRAMEBUFFER, OP_FRAMEBUFFER_TEXTURE_2D,
    OP_FRAMEBUFFER_RENDERBUFFER, OP_CHECK_FRAMEBUFFER_STATUS,
    OP_CREATE_RENDERBUFFER, OP_DELETE_RENDERBUFFER, OP_BIND_RENDERBUFFER,
    OP_RENDERBUFFER_STORAGE, OP_READ_PIXELS,
    OP_STENCIL_FUNC, OP_STENCIL_OP, OP_STENCIL_MASK, OP_CLEAR_STENCIL,
    OP_DEPTH_FUNC, OP_DEPTH_MASK, OP_COLOR_MASK,
    /* Minimal shader pipeline; the stencil test needs one real draw call. */
    OP_CREATE_SHADER, OP_SHADER_SOURCE, OP_COMPILE_SHADER, OP_CREATE_PROGRAM,
    OP_ATTACH_SHADER, OP_LINK_PROGRAM, OP_GET_SHADER_PARAMETER,
    OP_GET_PROGRAM_PARAMETER, OP_GET_SHADER_INFO_LOG,
    OP_GET_PROGRAM_INFO_LOG, OP_DELETE_SHADER, OP_DELETE_PROGRAM,
    OP_BIND_ATTRIB_LOCATION, OP_GET_UNIFORM_LOCATION, OP_GET_ATTRIB_LOCATION,
    OP_GET_ACTIVE_UNIFORM, OP_GET_ACTIVE_ATTRIB,
    /* Card M2-T003: extensions and the Chrome-117 parameter answers. */
    OP_GET_EXTENSION, OP_GET_SUPPORTED_EXTENSIONS
};

/* One JS method: name, `length` and its switch case. */
struct Method { const char *name; int length; Op op; };

static const Method kMethods[] = {
    {"viewport", 4, OP_VIEWPORT}, {"clearColor", 4, OP_CLEAR_COLOR},
    {"clear", 1, OP_CLEAR}, {"enable", 1, OP_ENABLE},
    {"disable", 1, OP_DISABLE}, {"isEnabled", 1, OP_IS_ENABLED},
    {"getError", 0, OP_GET_ERROR}, {"getParameter", 1, OP_GET_PARAMETER},
    {"createBuffer", 0, OP_CREATE_BUFFER},
    {"deleteBuffer", 1, OP_DELETE_BUFFER}, {"bindBuffer", 2, OP_BIND_BUFFER},
    {"bufferData", 3, OP_BUFFER_DATA}, {"bufferSubData", 3, OP_BUFFER_SUB_DATA},
    {"createVertexArray", 0, OP_CREATE_VAO}, {"deleteVertexArray", 1, OP_DELETE_VAO},
    {"bindVertexArray", 1, OP_BIND_VAO}, {"enableVertexAttribArray", 1, OP_ENABLE_ATTRIB},
    {"disableVertexAttribArray", 1, OP_DISABLE_ATTRIB},
    {"vertexAttribPointer", 6, OP_VERTEX_ATTRIB},
    {"drawArrays", 3, OP_DRAW_ARRAYS}, {"drawElements", 4, OP_DRAW_ELEMENTS},
    {"activeTexture", 1, OP_ACTIVE_TEXTURE}, {"bindTexture", 2, OP_BIND_TEXTURE},
    {"useProgram", 1, OP_USE_PROGRAM}, {"bindFramebuffer", 2, OP_BIND_FRAMEBUFFER},
    {"uniform1f", 2, OP_UNIFORM1F}, {"uniform1i", 2, OP_UNIFORM1I},
    {"uniform2f", 3, OP_UNIFORM2F}, {"uniform3f", 4, OP_UNIFORM3F},
    {"uniform4f", 5, OP_UNIFORM4F}, {"uniform1fv", 2, OP_UNIFORM1FV},
    {"uniform2fv", 2, OP_UNIFORM2FV}, {"uniform3fv", 2, OP_UNIFORM3FV},
    {"uniform4fv", 2, OP_UNIFORM4FV},
    {"uniformMatrix2fv", 3, OP_UNIFORM_MATRIX2FV},
    {"uniformMatrix3fv", 3, OP_UNIFORM_MATRIX3FV},
    {"uniformMatrix4fv", 3, OP_UNIFORM_MATRIX4FV},
    {"createTexture", 0, OP_CREATE_TEXTURE},
    {"deleteTexture", 1, OP_DELETE_TEXTURE},
    {"texImage2D", 6, OP_TEX_IMAGE_2D},
    {"texSubImage2D", 7, OP_TEX_SUB_IMAGE_2D},
    {"texParameteri", 3, OP_TEX_PARAMETERI},
    {"pixelStorei", 2, OP_PIXEL_STOREI},
    {"createFramebuffer", 0, OP_CREATE_FRAMEBUFFER},
    {"deleteFramebuffer", 1, OP_DELETE_FRAMEBUFFER},
    {"framebufferTexture2D", 5, OP_FRAMEBUFFER_TEXTURE_2D},
    {"framebufferRenderbuffer", 4, OP_FRAMEBUFFER_RENDERBUFFER},
    {"checkFramebufferStatus", 1, OP_CHECK_FRAMEBUFFER_STATUS},
    {"createRenderbuffer", 0, OP_CREATE_RENDERBUFFER},
    {"deleteRenderbuffer", 1, OP_DELETE_RENDERBUFFER},
    {"bindRenderbuffer", 2, OP_BIND_RENDERBUFFER},
    {"renderbufferStorage", 4, OP_RENDERBUFFER_STORAGE},
    {"readPixels", 7, OP_READ_PIXELS},
    {"stencilFunc", 3, OP_STENCIL_FUNC}, {"stencilOp", 3, OP_STENCIL_OP},
    {"stencilMask", 1, OP_STENCIL_MASK},
    {"clearStencil", 1, OP_CLEAR_STENCIL},
    {"depthFunc", 1, OP_DEPTH_FUNC}, {"depthMask", 1, OP_DEPTH_MASK},
    {"colorMask", 4, OP_COLOR_MASK},
    {"createShader", 1, OP_CREATE_SHADER},
    {"shaderSource", 2, OP_SHADER_SOURCE},
    {"compileShader", 1, OP_COMPILE_SHADER},
    {"createProgram", 0, OP_CREATE_PROGRAM},
    {"attachShader", 2, OP_ATTACH_SHADER},
    {"linkProgram", 1, OP_LINK_PROGRAM},
    {"getShaderParameter", 2, OP_GET_SHADER_PARAMETER},
    {"getProgramParameter", 2, OP_GET_PROGRAM_PARAMETER},
    {"getShaderInfoLog", 1, OP_GET_SHADER_INFO_LOG},
    {"getProgramInfoLog", 1, OP_GET_PROGRAM_INFO_LOG},
    {"deleteShader", 1, OP_DELETE_SHADER},
    {"deleteProgram", 1, OP_DELETE_PROGRAM},
    {"bindAttribLocation", 3, OP_BIND_ATTRIB_LOCATION},
    {"getUniformLocation", 2, OP_GET_UNIFORM_LOCATION},
    {"getAttribLocation", 2, OP_GET_ATTRIB_LOCATION},
    {"getActiveUniform", 2, OP_GET_ACTIVE_UNIFORM},
    {"getActiveAttrib", 2, OP_GET_ACTIVE_ATTRIB},
    {"getExtension", 1, OP_GET_EXTENSION},
    {"getSupportedExtensions", 0, OP_GET_SUPPORTED_EXTENSIONS},
};

/* Uploads a float uniform vector of `components` width; shared by both paths.
 * Returns false after throwing when `data` is not an ArrayBuffer/view. */
static bool upload_fv(Isolate *isolate, int32_t location, Local<Value> data,
                      int components)
{
    Bytes bytes;
    if (!bytes_of(data, &bytes)) {
        throw_type(isolate, "expected a float array");
        return false;
    }
    int32_t count = (int32_t)(bytes.len / (sizeof(float) * components));
    const float *values = (const float *)bytes.data;
    if (components == 1) gl.Uniform1fv(location, count, values);
    else if (components == 2) gl.Uniform2fv(location, count, values);
    else if (components == 3) gl.Uniform3fv(location, count, values);
    else gl.Uniform4fv(location, count, values);
    return true;
}
/* Creates a JS wrapper for a buffer/VAO record; defined with its finalizers. */
static Local<Object> make_object(Isolate *isolate, Local<Context> context,
                                 GLObject *object);
/* Reads one getParameter value with the JS type Chrome uses (defined below). */
static void set_parameter(const FunctionCallbackInfo<Value> &info, GLenum pname);
/* Handles the context-state calls; false when `op` is another group's. */
static bool slow_state(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                       Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_VIEWPORT:
        gl.Viewport(arg_i32(isolate, info, 0), arg_i32(isolate, info, 1),
                    arg_i32(isolate, info, 2), arg_i32(isolate, info, 3));
        return true;
    case OP_CLEAR_COLOR:
        gl.ClearColor(arg_f32(isolate, info, 0), arg_f32(isolate, info, 1),
                      arg_f32(isolate, info, 2), arg_f32(isolate, info, 3));
        return true;
    case OP_CLEAR: gl.Clear(arg_u32(isolate, info, 0)); return true;
    case OP_ENABLE: gl.Enable(arg_u32(isolate, info, 0)); return true;
    case OP_DISABLE: gl.Disable(arg_u32(isolate, info, 0)); return true;
    case OP_IS_ENABLED:
        info.GetReturnValue().Set(
            gl.IsEnabled(arg_u32(isolate, info, 0)) == GL_TRUE);
        return true;
    case OP_GET_ERROR: {
        /* A synthetic error from a dead/foreign wrapper comes first, as it
         * was recorded before any GL error. */
        GLenum error = ctx->synth_error;
        if (error) ctx->synth_error = GL_NO_ERROR;
        else error = gl.GetError();
        info.GetReturnValue().Set(Integer::NewFromUnsigned(isolate, error));
        return true;
    }
    case OP_GET_PARAMETER:
        set_parameter(info, arg_u32(isolate, info, 0));
        return true;
    case OP_ACTIVE_TEXTURE: gl.ActiveTexture(arg_u32(isolate, info, 0)); return true;
    case OP_BIND_FRAMEBUFFER:
        gl.BindFramebuffer(arg_u32(isolate, info, 0), gl_name(ctx, info[1]));
        return true;
    default: return false;
    }
}
/* Handles buffer creation, binding and uploads; false for another group. */
static bool slow_buffer(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                        Op op)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    switch (op) {
    case OP_CREATE_BUFFER: {
        GLObject *object = object_create(ctx, GL_OBJECT_BUFFER);
        if (!object) { info.GetReturnValue().SetNull(); return true; }
        info.GetReturnValue().Set(make_object(isolate, context, object));
        return true;
    }
    case OP_DELETE_BUFFER: {
        GLObject *object = object_of(info[0]);
        if (object && object->kind == GL_OBJECT_BUFFER) object_delete(object);
        return true;
    }
    case OP_BIND_BUFFER:
        gl.BindBuffer(arg_u32(isolate, info, 0), gl_name(ctx, info[1]));
        return true;
    case OP_BUFFER_DATA: {
        Local<Value> source = info[1];
        Bytes bytes;
        size_t size = 0;
        if (source->IsNumber()) {
            int64_t n = source->IntegerValue(context).FromMaybe(0);
            if (n < 0) {
                isolate->ThrowException(Exception::RangeError(
                    utf8(isolate, "buffer size must not be negative")));
                return true;
            }
            size = (size_t)n;
        } else if (bytes_of(source, &bytes)) {
            size = bytes.len;
        } else {
            throw_type(isolate, "expected an ArrayBuffer, a view or a size");
            return true;
        }
        gl.BufferData(arg_u32(isolate, info, 0), (GLsizeiptr)size, bytes.data,
                      arg_u32(isolate, info, 2));
        return true;
    }
    case OP_BUFFER_SUB_DATA: {
        Bytes bytes;
        if (!bytes_of(info[2], &bytes)) {
            throw_type(isolate, "bufferSubData expects an ArrayBuffer or a view");
            return true;
        }
        gl.BufferSubData(arg_u32(isolate, info, 0),
                         (GLintptr)arg_i64(isolate, info, 1),
                         (GLsizeiptr)bytes.len, bytes.data);
        return true;
    }
    default: return false;
    }
}
/* Handles VAO creation, attribute layout and the draw calls. */
static bool slow_vao(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                     Op op)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    switch (op) {
    case OP_CREATE_VAO: {
        GLObject *object = object_create(ctx, GL_OBJECT_VERTEX_ARRAY);
        if (!object) { info.GetReturnValue().SetNull(); return true; }
        info.GetReturnValue().Set(make_object(isolate, context, object));
        return true;
    }
    case OP_DELETE_VAO: {
        GLObject *object = object_of(info[0]);
        if (object && object->kind == GL_OBJECT_VERTEX_ARRAY) object_delete(object);
        return true;
    }
    case OP_BIND_VAO: gl.BindVertexArray(gl_name(ctx, info[0])); return true;
    case OP_ENABLE_ATTRIB:
        gl.EnableVertexAttribArray(arg_u32(isolate, info, 0));
        return true;
    case OP_DISABLE_ATTRIB:
        gl.DisableVertexAttribArray(arg_u32(isolate, info, 0));
        return true;
    case OP_VERTEX_ATTRIB:
        gl.VertexAttribPointer(arg_u32(isolate, info, 0),
                               arg_i32(isolate, info, 1),
                               arg_u32(isolate, info, 2),
                               arg_bool(isolate, info, 3) ? GL_TRUE : GL_FALSE,
                               arg_i32(isolate, info, 4),
                               (const void *)(intptr_t)arg_i64(isolate, info, 5));
        return true;
    case OP_DRAW_ARRAYS:
        gl.DrawArrays(arg_u32(isolate, info, 0), arg_i32(isolate, info, 1),
                      arg_i32(isolate, info, 2));
        return true;
    case OP_DRAW_ELEMENTS:
        gl.DrawElements(arg_u32(isolate, info, 0), arg_i32(isolate, info, 1),
                        arg_u32(isolate, info, 2),
                        (const void *)(intptr_t)arg_i64(isolate, info, 3));
        return true;
    default: return false;
    }
}
/* Uploads a square float matrix of `side` width; shared by the 2/3/4 cases.
 * `transpose` is WebGL's boolean and is passed through unchanged. */
static void upload_matrix(Isolate *isolate, int32_t location, bool transpose,
                          Local<Value> data, int side)
{
    Bytes bytes;
    if (!bytes_of(data, &bytes)) {
        throw_type(isolate, "uniformMatrix*fv expects a float array");
        return;
    }
    GLsizei count =
        (GLsizei)(bytes.len / ((size_t)side * side * sizeof(float)));
    GLboolean flip = transpose ? GL_TRUE : GL_FALSE;
    const GLfloat *values = (const GLfloat *)bytes.data;
    if (side == 2)
        gl.UniformMatrix2fv(location, count, flip, values);
    else if (side == 3)
        gl.UniformMatrix3fv(location, count, flip, values);
    else
        gl.UniformMatrix4fv(location, count, flip, values);
}
/* Handles texture/program binding and uniforms; false for another group. */
static bool slow_uniform(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                         Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_BIND_TEXTURE:
        gl.BindTexture(arg_u32(isolate, info, 0), gl_name(ctx, info[1]));
        return true;
    case OP_USE_PROGRAM: gl.UseProgram(gl_name(ctx, info[0])); return true;
    case OP_UNIFORM1F:
        gl.Uniform1f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1));
        return true;
    case OP_UNIFORM1I:
        gl.Uniform1i(arg_i32(isolate, info, 0), arg_i32(isolate, info, 1));
        return true;
    case OP_UNIFORM2F:
        gl.Uniform2f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1),
                     arg_f32(isolate, info, 2));
        return true;
    case OP_UNIFORM3F:
        gl.Uniform3f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1),
                     arg_f32(isolate, info, 2), arg_f32(isolate, info, 3));
        return true;
    case OP_UNIFORM4F:
        gl.Uniform4f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1),
                     arg_f32(isolate, info, 2), arg_f32(isolate, info, 3),
                     arg_f32(isolate, info, 4));
        return true;
    case OP_UNIFORM1FV: upload_fv(isolate, arg_i32(isolate, info, 0), info[1], 1);
        return true;
    case OP_UNIFORM2FV: upload_fv(isolate, arg_i32(isolate, info, 0), info[1], 2);
        return true;
    case OP_UNIFORM3FV: upload_fv(isolate, arg_i32(isolate, info, 0), info[1], 3);
        return true;
    case OP_UNIFORM4FV: upload_fv(isolate, arg_i32(isolate, info, 0), info[1], 4);
        return true;
    case OP_UNIFORM_MATRIX2FV:
    case OP_UNIFORM_MATRIX3FV:
    case OP_UNIFORM_MATRIX4FV: {
        int side = op == OP_UNIFORM_MATRIX2FV ? 2
                 : op == OP_UNIFORM_MATRIX3FV ? 3 : 4;
        upload_matrix(isolate, arg_i32(isolate, info, 0),
                      arg_bool(isolate, info, 1), info[2], side);
        return true;
    }
    default: return false;
    }
}
/* Wraps a fresh GL name of `kind` and returns it, or null when the driver
 * refused (the same answer Chrome gives when allocation fails). */
static void return_object(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                          int kind)
{
    GLObject *object = object_create(ctx, kind);
    if (!object) {
        info.GetReturnValue().SetNull();
        return;
    }
    info.GetReturnValue().Set(make_object(
        info.GetIsolate(), info.GetIsolate()->GetCurrentContext(), object));
}
/* Deletes `value` only when it is our wrapper of `kind`; other values are a
 * no-op so delete*() stays idempotent, as in WebGL. */
static void delete_object(Local<Value> value, GLContext *ctx, int kind)
{
    GLObject *object = object_of(value);
    if (object && object->context == ctx && object->kind == kind)
        object_delete(object);
}
/* Resolves a nullable wrapper of `kind`; a dead or foreign wrapper records
 * INVALID_OPERATION and reports false, as Chrome does after delete. */
static bool wrapper_name(GLContext *ctx, Local<Value> value, int kind,
                         GLuint *name)
{
    if (value->IsNullOrUndefined()) {
        *name = 0;
        return true;
    }
    if (!value->IsObject()) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return false;
    }
    GLObject *object = object_of(value);
    if (!object || object->deleted || object->context != ctx ||
        object->kind != kind) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return false;
    }
    *name = object->name;
    return true;
}
/* Stores pixel-store filters and forwards the real GL parameters. The two
 * WebGL-only enums never reach GL, where they would be INVALID_ENUM. */
static void pixel_store(const FunctionCallbackInfo<Value> &info, GLContext *ctx)
{
    GLenum pname = arg_u32(info.GetIsolate(), info, 0);
    GLint value = arg_i32(info.GetIsolate(), info, 1);
    if (((pname == GL_UNPACK_ALIGNMENT || pname == GL_PACK_ALIGNMENT) &&
         value != 1 && value != 2 && value != 4 && value != 8) ||
        ((pname == GL_UNPACK_ROW_LENGTH || pname == GL_PACK_ROW_LENGTH) &&
         value < 0)) {
        ctx->synth_error = GL_INVALID_VALUE;
        return;
    }
    switch (pname) {
    case 0x9240: ctx->unpack_flip_y = value != 0; return; /* FLIP_Y */
    case 0x9241: ctx->unpack_premultiply_alpha = value != 0; return; /* PREMUL */
    case GL_UNPACK_ALIGNMENT: ctx->unpack_alignment = value; break;
    case GL_PACK_ALIGNMENT: ctx->pack_alignment = value; break;
    case GL_UNPACK_ROW_LENGTH: ctx->unpack_row_length = value; break;
    case GL_PACK_ROW_LENGTH: ctx->pack_row_length = value; break;
    default: ctx->synth_error = GL_INVALID_ENUM; return;
    }
    gl.PixelStorei(pname, value);
}
/* texImage2D: 9 arguments allocate/upload from a view, 6 upload a host source. */
static void tex_image(const FunctionCallbackInfo<Value> &info, GLContext *ctx)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    GLenum target = arg_u32(isolate, info, 0);
    GLint level = arg_i32(isolate, info, 1);
    GLint internalformat = arg_i32(isolate, info, 2);
    if (info.Length() >= 9) {
        int32_t width = arg_i32(isolate, info, 3);
        int32_t height = arg_i32(isolate, info, 4);
        GLint border = arg_i32(isolate, info, 5);
        GLenum format = arg_u32(isolate, info, 6);
        GLenum type = arg_u32(isolate, info, 7);
        Bytes bytes;
        const void *data = NULL;
        if (!info[8]->IsNullOrUndefined()) {
            if (!bytes_of(info[8], &bytes)) {
                throw_type(isolate, "texImage2D expects a typed array");
                return;
            }
            size_t need;
            if (!pixel_bytes(ctx, format, type, width, height, false, &need))
                return;
            if (bytes.len < need) {
                ctx->synth_error = GL_INVALID_OPERATION;
                return;
            }
            data = bytes.data;
        }
        gl.TexImage2D(target, level, internalformat, width, height, border,
                      format, type, data);
        return;
    }
    GLenum format = arg_u32(isolate, info, 3);
    GLenum type = arg_u32(isolate, info, 4);
    PixelSource source;
    Local<ArrayBuffer> keep;
    if (!pixel_source(context, info[5], &source, &keep)) {
        throw_type(isolate, "texImage2D source is not a Canvas2D, image or "
                            "ImageData object");
        return;
    }
    tex_image_source(ctx, target, level, internalformat, format, type, &source);
    pixel_source_release(&source);
}
/* texSubImage2D: 9 arguments use a view, 7 upload a host source region. */
static void tex_sub_image(const FunctionCallbackInfo<Value> &info, GLContext *ctx)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    GLenum target = arg_u32(isolate, info, 0);
    GLint level = arg_i32(isolate, info, 1);
    GLint xoffset = arg_i32(isolate, info, 2);
    GLint yoffset = arg_i32(isolate, info, 3);
    if (info.Length() >= 9) {
        int32_t width = arg_i32(isolate, info, 4);
        int32_t height = arg_i32(isolate, info, 5);
        GLenum format = arg_u32(isolate, info, 6);
        GLenum type = arg_u32(isolate, info, 7);
        Bytes bytes;
        if (!bytes_of(info[8], &bytes)) {
            throw_type(isolate, "texSubImage2D expects a typed array");
            return;
        }
        size_t need;
        if (!pixel_bytes(ctx, format, type, width, height, false, &need))
            return;
        if (bytes.len < need) {
            ctx->synth_error = GL_INVALID_OPERATION;
            return;
        }
        gl.TexSubImage2D(target, level, xoffset, yoffset, width, height,
                         format, type, bytes.data);
        return;
    }
    GLenum format = arg_u32(isolate, info, 4);
    GLenum type = arg_u32(isolate, info, 5);
    PixelSource source;
    Local<ArrayBuffer> keep;
    if (!pixel_source(context, info[6], &source, &keep)) {
        throw_type(isolate, "texSubImage2D source is not a Canvas2D, image or "
                            "ImageData object");
        return;
    }
    tex_sub_image_source(ctx, target, level, xoffset, yoffset, format, type,
                         &source);
    pixel_source_release(&source);
}
/* Handles texture creation, parameters, pixel store and both uploads. */
static bool slow_texture(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                         Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_CREATE_TEXTURE: return_object(info, ctx, GL_OBJECT_TEXTURE); return true;
    case OP_DELETE_TEXTURE:
        delete_object(info[0], ctx, GL_OBJECT_TEXTURE);
        return true;
    case OP_TEX_PARAMETERI:
        gl.TexParameteri(arg_u32(isolate, info, 0), arg_u32(isolate, info, 1),
                         arg_i32(isolate, info, 2));
        return true;
    case OP_PIXEL_STOREI: pixel_store(info, ctx); return true;
    case OP_TEX_IMAGE_2D: tex_image(info, ctx); return true;
    case OP_TEX_SUB_IMAGE_2D: tex_sub_image(info, ctx); return true;
    default: return false;
    }
}
/* Handles framebuffer creation, attachments and completeness checks. */
static bool slow_framebuffer(const FunctionCallbackInfo<Value> &info,
                             GLContext *ctx, Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_CREATE_FRAMEBUFFER:
        return_object(info, ctx, GL_OBJECT_FRAMEBUFFER);
        return true;
    case OP_DELETE_FRAMEBUFFER:
        delete_object(info[0], ctx, GL_OBJECT_FRAMEBUFFER);
        return true;
    case OP_FRAMEBUFFER_TEXTURE_2D: {
        GLuint texture = 0;
        if (!wrapper_name(ctx, info[3], GL_OBJECT_TEXTURE, &texture)) return true;
        gl.FramebufferTexture2D(arg_u32(isolate, info, 0),
                                arg_u32(isolate, info, 1),
                                arg_u32(isolate, info, 2), texture,
                                arg_i32(isolate, info, 4));
        return true;
    }
    case OP_FRAMEBUFFER_RENDERBUFFER: {
        GLuint renderbuffer = 0;
        if (!wrapper_name(ctx, info[3], GL_OBJECT_RENDERBUFFER, &renderbuffer))
            return true;
        gl.FramebufferRenderbuffer(arg_u32(isolate, info, 0),
                                   arg_u32(isolate, info, 1),
                                   arg_u32(isolate, info, 2), renderbuffer);
        return true;
    }
    case OP_CHECK_FRAMEBUFFER_STATUS:
        info.GetReturnValue().Set(Integer::NewFromUnsigned(
            isolate, gl.CheckFramebufferStatus(arg_u32(isolate, info, 0))));
        return true;
    default: return false;
    }
}
/* Handles renderbuffer creation, binding and storage allocation. */
static bool slow_renderbuffer(const FunctionCallbackInfo<Value> &info,
                              GLContext *ctx, Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_CREATE_RENDERBUFFER:
        return_object(info, ctx, GL_OBJECT_RENDERBUFFER);
        return true;
    case OP_DELETE_RENDERBUFFER:
        delete_object(info[0], ctx, GL_OBJECT_RENDERBUFFER);
        return true;
    case OP_BIND_RENDERBUFFER: {
        GLuint renderbuffer = 0;
        if (!wrapper_name(ctx, info[1], GL_OBJECT_RENDERBUFFER, &renderbuffer))
            return true;
        gl.BindRenderbuffer(arg_u32(isolate, info, 0), renderbuffer);
        return true;
    }
    case OP_RENDERBUFFER_STORAGE:
        gl.RenderbufferStorage(arg_u32(isolate, info, 0),
                               arg_u32(isolate, info, 1),
                               arg_i32(isolate, info, 2),
                               arg_i32(isolate, info, 3));
        return true;
    default: return false;
    }
}
/* Copies GL pixels into a caller-supplied view; the view stays JS-owned. */
static void read_pixels(const FunctionCallbackInfo<Value> &info, GLContext *ctx)
{
    Isolate *isolate = info.GetIsolate();
    int32_t x = arg_i32(isolate, info, 0);
    int32_t y = arg_i32(isolate, info, 1);
    int32_t width = arg_i32(isolate, info, 2);
    int32_t height = arg_i32(isolate, info, 3);
    GLenum format = arg_u32(isolate, info, 4);
    GLenum type = arg_u32(isolate, info, 5);
    if (!info[6]->IsArrayBufferView()) {
        throw_type(isolate, "readPixels expects a typed array");
        return;
    }
    Local<ArrayBufferView> view = info[6].As<ArrayBufferView>();
    size_t need;
    if (!pixel_bytes(ctx, format, type, width, height, true, &need)) return;
    if (view->ByteLength() < need) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return;
    }
    Local<ArrayBuffer> buffer = view->Buffer();
    uint8_t *data = buffer.IsEmpty() ? NULL : (uint8_t *)buffer->Data();
    if (!data) return;
    gl.ReadPixels(x, y, width, height, format, type,
                  data + view->ByteOffset());
}
/* Handles stencil/depth/colour masks and the readback that snapshots them. */
static bool slow_raster(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                        Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_READ_PIXELS: read_pixels(info, ctx); return true;
    case OP_STENCIL_FUNC:
        gl.StencilFunc(arg_u32(isolate, info, 0), arg_i32(isolate, info, 1),
                       arg_u32(isolate, info, 2));
        return true;
    case OP_STENCIL_OP:
        gl.StencilOp(arg_u32(isolate, info, 0), arg_u32(isolate, info, 1),
                     arg_u32(isolate, info, 2));
        return true;
    case OP_STENCIL_MASK: gl.StencilMask(arg_u32(isolate, info, 0)); return true;
    case OP_CLEAR_STENCIL: gl.ClearStencil(arg_i32(isolate, info, 0)); return true;
    case OP_DEPTH_FUNC: gl.DepthFunc(arg_u32(isolate, info, 0)); return true;
    case OP_DEPTH_MASK:
        gl.DepthMask(arg_bool(isolate, info, 0) ? GL_TRUE : GL_FALSE);
        return true;
    case OP_COLOR_MASK:
        gl.ColorMask(arg_bool(isolate, info, 0) ? GL_TRUE : GL_FALSE,
                     arg_bool(isolate, info, 1) ? GL_TRUE : GL_FALSE,
                     arg_bool(isolate, info, 2) ? GL_TRUE : GL_FALSE,
                     arg_bool(isolate, info, 3) ? GL_TRUE : GL_FALSE);
        return true;
    default: return false;
    }
}
/* Copies one V8 string into a malloc'ed UTF-8 buffer. */
static char *copy_string(Isolate *isolate, Local<String> text)
{
    int length = text->Utf8Length(isolate);
    char *out = (char *)malloc((size_t)length + 1);
    if (!out) return NULL;
    text->WriteUtf8(isolate, out, length + 1);
    return out;
}
/* Growable output buffer for the GLSL rewriter; the caller frees `data`. */
struct StrBuf { char *data; size_t len; size_t cap; };

/* Appends `length` bytes to `buffer`, growing it when needed. Returns false on
 * allocation failure; the caller frees whatever was already allocated. */
static bool sb_add(StrBuf *buffer, const char *text, size_t length)
{
    if (buffer->len + length + 1 > buffer->cap) {
        size_t cap = buffer->cap ? buffer->cap * 2 : 256;
        while (cap < buffer->len + length + 1) cap *= 2;
        char *grown = (char *)realloc(buffer->data, cap);
        if (!grown) return false;
        buffer->data = grown;
        buffer->cap = cap;
    }
    if (length) memcpy(buffer->data + buffer->len, text, length);
    buffer->len += length;
    buffer->data[buffer->len] = '\0';
    return true;
}
/* Appends a NUL-terminated string to `buffer`. */
static bool sb_puts(StrBuf *buffer, const char *text)
{
    return sb_add(buffer, text, strlen(text));
}
/* True when the shader declares GLSL ES 3.00. WebGL2 only accepts 300 es, so
 * anything else is treated as GLSL ES 1.00 for the rewrite. */
static bool glsl_is_es3(const char *source)
{
    const char *p = source;
    while (*p) {
        const char *end = strchr(p, '\n');
        size_t length = end ? (size_t)(end - p) : strlen(p);
        const char *t = p;
        while (t < p + length && (*t == ' ' || *t == '\t')) t++;
        if (!strncmp(t, "#version", 8)) return strstr(t, "300") != NULL;
        if (!end) break;
        p = end + 1;
    }
    return false;
}
/* Maps one GLSL ES word to its desktop-GL 3.3 core spelling, or NULL when the
 * word is unchanged. Only declarations and the ES fragment output differ. */
static const char *glsl_replacement(const char *word, size_t length, bool frag)
{
    struct Replacement { const char *from; const char *to; };
    static const Replacement vertex[] = {
        {"attribute", "in"}, {"varying", "out"}, {"texture2D", "texture"},
        {"textureCube", "texture"}, {"texture2DProj", "textureProj"},
        {"highp", ""}, {"mediump", ""}, {"lowp", ""},
    };
    static const Replacement fragment[] = {
        {"attribute", "in"}, {"varying", "in"}, {"texture2D", "texture"},
        {"textureCube", "texture"}, {"texture2DProj", "textureProj"},
        {"gl_FragColor", "smz_FragColor"},
        {"highp", ""}, {"mediump", ""}, {"lowp", ""},
    };
    const Replacement *table = frag ? fragment : vertex;
    size_t count = sizeof(vertex) / sizeof(vertex[0]);
    for (size_t i = 0; i < count; i++)
        if (strlen(table[i].from) == length &&
            !strncmp(table[i].from, word, length))
            return table[i].to;
    return NULL;
}
/* Rewrites one source line, preserving its newline so driver line numbers map
 * back to the author's lines. `#version` and standalone precision statements
 * are dropped; other preprocessor lines pass through unchanged. */
static bool glsl_rewrite_line(StrBuf *out, const char *line, size_t length,
                              bool frag, bool *in_block, bool *version_seen)
{
    const char *t = line;
    const char *end = line + length;
    while (t < end && (*t == ' ' || *t == '\t' || *t == '\r')) t++;
    if (!*version_seen && t + 8 <= end && !strncmp(t, "#version", 8)) {
        *version_seen = true;
        return sb_add(out, "\n", 1);
    }
    if (t + 9 <= end && !strncmp(t, "precision", 9)) {
        return sb_add(out, "\n", 1);
    }
    if (t + 10 <= end && !strncmp(t, "#extension", 10) &&
        strstr(t, "GL_OES_standard_derivatives")) {
        return sb_add(out, "\n", 1);
    }
    size_t i = 0;
    while (i < length) {
        if (*in_block) {
            char c = line[i++];
            if (!sb_add(out, &c, 1)) return false;
            if (c == '*' && i < length && line[i] == '/') {
                if (!sb_add(out, "/", 1)) return false;
                i++;
                *in_block = false;
            }
            continue;
        }
        if (line[i] == '/' && i + 1 < length && line[i + 1] == '/') {
            return sb_add(out, line + i, length - i) &&
                   sb_add(out, "\n", 1);
        }
        if (line[i] == '/' && i + 1 < length && line[i + 1] == '*') {
            if (!sb_add(out, "/*", 2)) return false;
            i += 2;
            *in_block = true;
            continue;
        }
        if (isalpha((unsigned char)line[i]) || line[i] == '_') {
            size_t start = i++;
            while (i < length &&
                   (isalnum((unsigned char)line[i]) || line[i] == '_'))
                i++;
            const char *replacement =
                glsl_replacement(line + start, i - start, frag);
            bool ok = replacement ? sb_puts(out, replacement)
                                  : sb_add(out, line + start, i - start);
            if (!ok) return false;
            continue;
        }
        if (!sb_add(out, line + i, 1)) return false;
        i++;
    }
    return sb_add(out, "\n", 1);
}
/* Returns the GLSL text to hand the driver for `source`. On GLES it is an
 * unchanged copy; on desktop GL core it rewrites GLSL ES 1.00/3.00 to 330
 * core, including `attribute`/`varying`/`texture2D`/`gl_FragColor` and
 * precision statements (spec §4.2). The caller owns the result. */
static char *glsl_for_target(const char *source, bool fragment)
{
    if (s_gl_es) {
        size_t length = strlen(source) + 1;
        char *copy = (char *)malloc(length);
        if (copy) memcpy(copy, source, length);
        return copy;
    }
    bool es3 = glsl_is_es3(source);
    StrBuf out = {NULL, 0, 0};
    bool ok = sb_puts(&out, "#version 330 core\n");
    if (ok && !es3 && fragment) ok = sb_puts(&out, "out vec4 smz_FragColor;\n");
    if (ok) ok = sb_puts(&out, "#line 1\n");
    bool in_block = false;
    bool version_seen = false;
    const char *p = source;
    while (ok && *p) {
        const char *end = strchr(p, '\n');
        size_t length = end ? (size_t)(end - p) : strlen(p);
        ok = glsl_rewrite_line(&out, p, length, fragment, &in_block,
                               &version_seen);
        if (!end) break;
        p = end + 1;
    }
    if (!ok) {
        free(out.data);
        return NULL;
    }
    return out.data;
}
/* Joins the string or string-array form of shaderSource into one C string.
 * The returned buffer is owned by the caller and freed on replacement. */
static char *shader_text(Isolate *isolate, Local<Context> context,
                         Local<Value> value)
{
    if (value->IsString()) return copy_string(isolate, value.As<String>());
    if (!value->IsArray()) return NULL;
    Local<Array> parts = value.As<Array>();
    char *out = NULL;
    size_t used = 0;
    for (uint32_t i = 0; i < parts->Length(); i++) {
        Local<Value> part;
        if (!parts->Get(context, i).ToLocal(&part)) { free(out); return NULL; }
        Local<String> text;
        if (!part->ToString(context).ToLocal(&text)) { free(out); return NULL; }
        int length = text->Utf8Length(isolate);
        char *grown = (char *)realloc(out, used + (size_t)length + 1);
        if (!grown) { free(out); return NULL; }
        out = grown;
        text->WriteUtf8(isolate, out + used, length + 1);
        used += (size_t)length;
    }
    return out;
}
/* Returns our live wrapper of `kind`, or records INVALID_OPERATION. */
static GLObject *owned_object(GLContext *ctx, Local<Value> value, int kind)
{
    GLObject *object = object_of(value);
    if (!object || object->deleted || object->context != ctx ||
        object->kind != kind) {
        ctx->synth_error = GL_INVALID_OPERATION;
        return NULL;
    }
    return object;
}
/* Wraps a driver-made name (shaders and programs have no gen_* entry point).
 * Returns the new wrapper so the caller can record the shader stage; the JS
 * return value is set here. */
static GLObject *return_named(const FunctionCallbackInfo<Value> &info,
                              GLContext *ctx, int kind, GLuint name)
{
    GLObject *object = object_wrap(ctx, kind, name);
    if (!object) {
        info.GetReturnValue().SetNull();
        return NULL;
    }
    info.GetReturnValue().Set(make_object(
        info.GetIsolate(), info.GetIsolate()->GetCurrentContext(), object));
    return object;
}
/* Appends the author's shader lines to `out`, numbered from 1, so a compile
 * failure reports the source line even when the driver log only gives a
 * number (spec section 15.8). Stops at 200 lines to bound a huge shader. */
static bool append_shader_source(StrBuf *out, const char *source)
{
    if (!sb_puts(out, "\n--- shader source ---\n")) return false;
    const char *line = source;
    for (int number = 1; line && *line && number <= 200; number++) {
        const char *end = strchr(line, '\n');
        int length = end ? (int)(end - line) : (int)strlen(line);
        char prefix[32];
        int prefix_length = snprintf(prefix, sizeof(prefix), "%4d: ", number);
        if (prefix_length > 0 &&
            !sb_add(out, prefix, (size_t)prefix_length)) return false;
        if (!sb_add(out, line, (size_t)length) || !sb_puts(out, "\n"))
            return false;
        if (!end) break;
        line = end + 1;
    }
    return true;
}
/* Logs a compile failure with its source lines so plugin authors can find the
 * bad statement, as Chrome does in the console (spec section 15.8). */
static void log_shader_failure(GLObject *shader)
{
    char log[2048];
    gl.GetShaderInfoLog(shader->name, (GLsizei)sizeof(log), NULL, log);
    log[sizeof(log) - 1] = '\0';
    fprintf(stderr, "webgl: shader compilation failed: %s\n", log);
    StrBuf out = {NULL, 0, 0};
    if (append_shader_source(&out, shader->source ? shader->source : ""))
        fprintf(stderr, "%s", out.data);
    free(out.data);
}
/* Returns the driver's shader log, plus the numbered original source on a
 * failure. Chrome returns only the driver text, but spec §15.8 requires the
 * source line to reach plugin authors, and this makes it testable. */
static Local<String> shader_info_log(Isolate *isolate, GLObject *shader)
{
    char log[2048];
    gl.GetShaderInfoLog(shader->name, (GLsizei)sizeof(log), NULL, log);
    log[sizeof(log) - 1] = '\0';
    GLint ok = GL_FALSE;
    gl.GetShaderiv(shader->name, GL_COMPILE_STATUS, &ok);
    if (ok || !shader->source) return utf8(isolate, log);
    StrBuf out = {NULL, 0, 0};
    bool built = sb_puts(&out, log) &&
                 append_shader_source(&out, shader->source);
    Local<String> text = utf8(isolate, built ? out.data : log);
    free(out.data);
    return text;
}
/* Stores the author's shader text and hands the driver a target-specific
 * version; keeps both so a failure can log the original lines. */
static void shader_set_source(const FunctionCallbackInfo<Value> &info,
                              GLObject *shader)
{
    Isolate *isolate = info.GetIsolate();
    char *text = shader_text(isolate, isolate->GetCurrentContext(), info[1]);
    if (!text) {
        throw_type(isolate, "shaderSource expects a string or string array");
        return;
    }
    char *compiled =
        glsl_for_target(text, shader->shader_type == GL_FRAGMENT_SHADER);
    if (!compiled) {
        free(text);
        isolate->ThrowException(
            Exception::Error(utf8(isolate, "shaderSource: out of memory")));
        return;
    }
    free(shader->source);
    free(shader->compiled);
    shader->source = text;
    shader->compiled = compiled;
    const GLchar *sources[1] = { compiled };
    gl.ShaderSource(shader->name, 1, sources, NULL);
}
/* Handles WebGLShader creation, source, compilation and deletion. */
static bool slow_shader(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                        Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_CREATE_SHADER: {
        GLObject *shader = return_named(
            info, ctx, GL_OBJECT_SHADER,
            gl.CreateShader(arg_u32(isolate, info, 0)));
        if (shader) shader->shader_type = (int)arg_u32(isolate, info, 0);
        return true;
    }
    case OP_SHADER_SOURCE: {
        GLObject *shader = owned_object(ctx, info[0], GL_OBJECT_SHADER);
        if (shader) shader_set_source(info, shader);
        return true;
    }
    case OP_COMPILE_SHADER: {
        GLObject *shader = owned_object(ctx, info[0], GL_OBJECT_SHADER);
        if (!shader) return true;
        gl.CompileShader(shader->name);
        GLint ok = 0;
        gl.GetShaderiv(shader->name, GL_COMPILE_STATUS, &ok);
        if (!ok) log_shader_failure(shader);
        return true;
    }
    case OP_GET_SHADER_PARAMETER: {
        GLObject *shader = owned_object(ctx, info[0], GL_OBJECT_SHADER);
        if (!shader) return true;
        GLenum pname = arg_u32(isolate, info, 1);
        GLint value = 0;
        gl.GetShaderiv(shader->name, pname, &value);
        if (pname == GL_COMPILE_STATUS || pname == GL_DELETE_STATUS)
            info.GetReturnValue().Set(value == GL_TRUE);
        else info.GetReturnValue().Set(Integer::New(isolate, value));
        return true;
    }
    case OP_GET_SHADER_INFO_LOG: {
        GLObject *shader = owned_object(ctx, info[0], GL_OBJECT_SHADER);
        if (!shader) return true;
        info.GetReturnValue().Set(shader_info_log(isolate, shader));
        return true;
    }
    case OP_DELETE_SHADER:
        delete_object(info[0], ctx, GL_OBJECT_SHADER);
        return true;
    default: return false;
    }
}
/* Handles WebGLProgram creation, attachment, linking and deletion. */
static bool slow_program(const FunctionCallbackInfo<Value> &info, GLContext *ctx,
                         Op op)
{
    Isolate *isolate = info.GetIsolate();
    switch (op) {
    case OP_CREATE_PROGRAM:
        return_named(info, ctx, GL_OBJECT_PROGRAM, gl.CreateProgram());
        return true;
    case OP_ATTACH_SHADER: {
        GLuint program = 0;
        GLuint shader = 0;
        if (!wrapper_name(ctx, info[0], GL_OBJECT_PROGRAM, &program) ||
            !wrapper_name(ctx, info[1], GL_OBJECT_SHADER, &shader)) return true;
        gl.AttachShader(program, shader);
        return true;
    }
    case OP_BIND_ATTRIB_LOCATION: {
        GLuint program = 0;
        if (!wrapper_name(ctx, info[0], GL_OBJECT_PROGRAM, &program)) return true;
        Local<String> name;
        if (!info[2]->ToString(isolate->GetCurrentContext()).ToLocal(&name)) {
            throw_type(isolate, "bindAttribLocation expects a name");
            return true;
        }
        char *text = copy_string(isolate, name);
        if (!text) return true;
        gl.BindAttribLocation(program, arg_u32(isolate, info, 1), text);
        free(text);
        return true;
    }
    case OP_LINK_PROGRAM: {
        GLuint program = 0;
        if (!wrapper_name(ctx, info[0], GL_OBJECT_PROGRAM, &program)) return true;
        gl.LinkProgram(program);
        GLint ok = 0;
        gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048];
            gl.GetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
            log[sizeof(log) - 1] = '\0';
            fprintf(stderr, "webgl: program link failed: %s\n", log);
        }
        return true;
    }
    case OP_DELETE_PROGRAM:
        delete_object(info[0], ctx, GL_OBJECT_PROGRAM);
        return true;
    default: return false;
    }
}
/* Reads program status and info logs; split from slow_program to keep each
 * switch arm's function under the project's 60-line limit. */
static bool program_query(const FunctionCallbackInfo<Value> &info,
                          GLContext *ctx, Op op)
{
    Isolate *isolate = info.GetIsolate();
    if (op != OP_GET_PROGRAM_PARAMETER && op != OP_GET_PROGRAM_INFO_LOG)
        return false;
    GLuint program = 0;
    if (!wrapper_name(ctx, info[0], GL_OBJECT_PROGRAM, &program)) return true;
    if (op == OP_GET_PROGRAM_PARAMETER) {
        GLenum pname = arg_u32(isolate, info, 1);
        GLint value = 0;
        gl.GetProgramiv(program, pname, &value);
        if (pname == GL_LINK_STATUS || pname == GL_DELETE_STATUS)
            info.GetReturnValue().Set(value == GL_TRUE);
        else info.GetReturnValue().Set(Integer::New(isolate, value));
        return true;
    }
    if (op != OP_GET_PROGRAM_INFO_LOG) return false;
    char log[4096];
    gl.GetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
    log[sizeof(log) - 1] = '\0';
    info.GetReturnValue().Set(utf8(isolate, log));
    return true;
}
/* Copies any JS value's string form into a malloc'ed UTF-8 buffer. Shader and
 * extension names arrive as DOMStrings, so ToString matches the WebIDL cast.
 * The caller frees the result; NULL means an exception or allocation failure. */
static char *value_text(Isolate *isolate, Local<Context> context,
                        Local<Value> value)
{
    Local<String> text;
    if (!value->ToString(context).ToLocal(&text)) return NULL;
    return copy_string(isolate, text);
}
/* Builds the WebGLActiveInfo shape `{size, type, name}` that PIXI reads from
 * getActiveUniform/getActiveAttrib. The object is V8-owned. */
static Local<Object> active_info(Isolate *isolate, Local<Context> context,
                                 GLint size, GLenum type, const char *name)
{
    Local<Object> info = Object::New(isolate);
    info->Set(context, utf8(isolate, "size"), Integer::New(isolate, size))
        .FromMaybe(false);
    info->Set(context, utf8(isolate, "type"),
              Integer::NewFromUnsigned(isolate, type)).FromMaybe(false);
    info->Set(context, utf8(isolate, "name"), utf8(isolate, name))
        .FromMaybe(false);
    return info;
}
/* Answers getUniformLocation, getAttribLocation, getActiveUniform and
 * getActiveAttrib; split from slow_program to keep each function small. */
static bool program_introspection(const FunctionCallbackInfo<Value> &info,
                                  GLContext *ctx, Op op)
{
    if (op != OP_GET_UNIFORM_LOCATION && op != OP_GET_ATTRIB_LOCATION &&
        op != OP_GET_ACTIVE_UNIFORM && op != OP_GET_ACTIVE_ATTRIB)
        return false;
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    GLuint program = 0;
    if (!wrapper_name(ctx, info[0], GL_OBJECT_PROGRAM, &program)) return true;
    if (op == OP_GET_ACTIVE_UNIFORM || op == OP_GET_ACTIVE_ATTRIB) {
        GLint size = 0;
        GLenum type = 0;
        GLsizei length = 0;
        GLchar name[256] = {0};
        GLuint index = arg_u32(isolate, info, 1);
        if (op == OP_GET_ACTIVE_UNIFORM)
            gl.GetActiveUniform(program, index, (GLsizei)sizeof(name),
                                &length, &size, &type, name);
        else
            gl.GetActiveAttrib(program, index, (GLsizei)sizeof(name),
                               &length, &size, &type, name);
        if (length <= 0) info.GetReturnValue().SetNull();
        else info.GetReturnValue().Set(
            active_info(isolate, context, size, type, name));
        return true;
    }
    char *text = value_text(isolate, context, info[1]);
    if (!text) {
        info.GetReturnValue().SetNull();
        return true;
    }
    GLint location = op == OP_GET_UNIFORM_LOCATION
                         ? gl.GetUniformLocation(program, text)
                         : gl.GetAttribLocation(program, text);
    if (location < 0) info.GetReturnValue().SetNull();
    else info.GetReturnValue().Set(Integer::New(isolate, location));
    free(text);
    return true;
}
/* Builds the extension object Chrome returns. Only the constants PIXI reads
 * are exposed; the float extensions are presence-only capabilities. */
static Local<Object> extension_object(Isolate *isolate, int id)
{
    Local<Object> extension = Object::New(isolate);
    if (id == EXT_ANISOTROPIC) {
        Local<Context> context = isolate->GetCurrentContext();
        extension->Set(context, utf8(isolate, "TEXTURE_MAX_ANISOTROPY_EXT"),
                       Integer::New(isolate, 0x84FE)).FromMaybe(false);
        extension->Set(context, utf8(isolate, "MAX_TEXTURE_MAX_ANISOTROPY_EXT"),
                       Integer::New(isolate, 0x84FF)).FromMaybe(false);
    }
    return extension;
}
/* Implements getExtension and getSupportedExtensions. A name the current
 * driver cannot back returns null, and the supported list never contains one
 * (spec §4.2); Chrome 117 answers the PIXI probes the same way apart from
 * driver-specific availability, recorded in the test fixture. */
static bool slow_extension(const FunctionCallbackInfo<Value> &info,
                           GLContext *ctx, Op op)
{
    (void)ctx;
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    if (op == OP_GET_SUPPORTED_EXTENSIONS) {
        Local<Array> names = Array::New(isolate);
        uint32_t used = 0;
        for (int i = 0; i < EXT_COUNT; i++)
            if (s_ext_supported[i])
                names->Set(context, used++,
                           utf8(isolate, kExtensionNames[i])).FromMaybe(false);
        info.GetReturnValue().Set(names);
        return true;
    }
    if (op != OP_GET_EXTENSION) return false;
    char *name = value_text(isolate, context, info[0]);
    if (!name) {
        info.GetReturnValue().SetNull();
        return true;
    }
    int found = -1;
    for (int i = 0; i < EXT_COUNT; i++)
        if (!strcmp(name, kExtensionNames[i])) { found = i; break; }
    free(name);
    if (found < 0 || !s_ext_supported[found]) {
        info.GetReturnValue().SetNull();
        return true;
    }
    info.GetReturnValue().Set(extension_object(isolate, found));
    return true;
}
/* Handles the string-valued getParameter names; true when `pname` matched. */
static bool set_string_parameter(const FunctionCallbackInfo<Value> &info,
                                 GLenum pname)
{
    switch (pname) {
    case GL_VERSION:
    case GL_VENDOR:
    case GL_RENDERER:
    case GL_SHADING_LANGUAGE_VERSION: {
        const GLubyte *text = gl.GetString(pname);
        if (text)
            info.GetReturnValue().Set(
                utf8(info.GetIsolate(), (const char *)text));
        return true;
    }
    default: return false;
    }
}
/* Handles array-valued state as the typed arrays Chrome returns. */
static bool set_array_parameter(const FunctionCallbackInfo<Value> &info,
                                GLenum pname)
{
    Isolate *isolate = info.GetIsolate();
    if (pname == GL_VIEWPORT || pname == GL_SCISSOR_BOX ||
        pname == GL_MAX_VIEWPORT_DIMS) {
        GLint values[4] = {0, 0, 0, 0};
        gl.GetIntegerv(pname, values);
        int count = pname == GL_MAX_VIEWPORT_DIMS ? 2 : 4;
        Local<ArrayBuffer> buffer =
            ArrayBuffer::New(isolate, (size_t)count * sizeof(GLint));
        memcpy(buffer->Data(), values, (size_t)count * sizeof(GLint));
        info.GetReturnValue().Set(Int32Array::New(buffer, 0, (size_t)count));
        return true;
    }
    int count = (pname == GL_COLOR_CLEAR_VALUE || pname == GL_BLEND_COLOR)
                    ? 4 : (pname == GL_DEPTH_RANGE ||
                           pname == GL_ALIASED_LINE_WIDTH_RANGE ||
                           pname == GL_ALIASED_POINT_SIZE_RANGE) ? 2 : 0;
    if (!count) return false;
    GLfloat values[4] = {0, 0, 0, 0};
    gl.GetFloatv(pname, values);
    Local<ArrayBuffer> buffer =
        ArrayBuffer::New(isolate, (size_t)count * sizeof(GLfloat));
    memcpy(buffer->Data(), values, (size_t)count * sizeof(GLfloat));
    info.GetReturnValue().Set(Float32Array::New(buffer, 0, (size_t)count));
    return true;
}
/* Handles float and boolean state; the integer default is in set_parameter. */
static bool set_scalar_parameter(const FunctionCallbackInfo<Value> &info,
                                 GLenum pname)
{
    Isolate *isolate = info.GetIsolate();
    switch (pname) {
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE: {
        GLfloat value = 0;
        gl.GetFloatv(pname, &value);
        info.GetReturnValue().Set(Number::New(isolate, value));
        return true;
    }
    case GL_DEPTH_TEST:
    case GL_STENCIL_TEST:
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_SCISSOR_TEST:
    case GL_DITHER:
    case GL_POLYGON_OFFSET_FILL:
    case GL_DEPTH_WRITEMASK:
    case GL_RASTERIZER_DISCARD: {
        GLint value = 0;
        gl.GetIntegerv(pname, &value);
        info.GetReturnValue().Set(value != 0);
        return true;
    }
    case GL_COLOR_WRITEMASK: {
        GLint values[4] = {0, 0, 0, 0};
        gl.GetIntegerv(pname, values);
        Local<Array> mask = Array::New(isolate, 4);
        for (uint32_t i = 0; i < 4; i++)
            mask->Set(isolate->GetCurrentContext(), i,
                      Boolean::New(isolate, values[i] != 0))
                .FromMaybe(false);
        info.GetReturnValue().Set(mask);
        return true;
    }
    default: return false;
    }
}
/* Reads a type-correct Chrome value: strings, typed arrays, floats and
 * booleans as above, and every remaining limit as a driver integer. */
static void set_parameter(const FunctionCallbackInfo<Value> &info, GLenum pname)
{
    if (set_string_parameter(info, pname) ||
        set_array_parameter(info, pname) ||
        set_scalar_parameter(info, pname))
        return;
    GLint value = 0;
    gl.GetIntegerv(pname, &value);
    info.GetReturnValue().Set(Integer::New(info.GetIsolate(), value));
}
/* Handles every slow path; `data` is the table row being called. */
static void slow_method(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    const Method *method =
        static_cast<const Method *>(info.Data().As<External>()->Value());
    GLContext *ctx = live(context_of(info.This()));
    if (!ctx) {
        throw_illegal(isolate);
        return;
    }
    drain_pending_all();
    Op op = method->op;
    bool handled =
        slow_state(info, ctx, op) || slow_buffer(info, ctx, op) ||
        slow_vao(info, ctx, op) || slow_uniform(info, ctx, op) ||
        slow_texture(info, ctx, op) || slow_framebuffer(info, ctx, op) ||
        slow_renderbuffer(info, ctx, op) || slow_raster(info, ctx, op) ||
        slow_shader(info, ctx, op) || slow_program(info, ctx, op) ||
        program_query(info, ctx, op) ||
        program_introspection(info, ctx, op) ||
        slow_extension(info, ctx, op);
    (void)handled;
}
/* Returns the receiver's live context, or NULL after throwing exactly what
 * the slow path throws, so optimisation cannot change JS behaviour. */
static GLContext *fast_context(Isolate *isolate, Local<Object> receiver)
{
    if (s_pending_any) drain_pending_all();
    GLContext *ctx = live(context_of(receiver));
    if (!ctx) throw_illegal(isolate);
    return ctx;
}
/* Fast path for bindBuffer; receiver and buffer wrapper are borrowed. */
void fast_bind_buffer(Local<Object> r, int32_t t, Local<Value> b,
                      FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.BindBuffer((GLenum)t, gl_name(c, b));
}
/* Fast path for bufferSubData; `d` is borrowed and stays alive for the call. */
void fast_buffer_sub_data(Local<Object> r, int32_t t, int32_t o, Local<Value> d,
                          FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (!c) return;
    Bytes bytes;
    if (!bytes_of(d, &bytes)) {
        throw_type(options.isolate,
                   "bufferSubData expects an ArrayBuffer or a view");
        return;
    }
    gl.BufferSubData((GLenum)t, o, (GLsizeiptr)bytes.len, bytes.data);
}
/* Fast path for drawElements; count is indices, offset is bytes. */
void fast_draw_elements(Local<Object> r, int32_t m, int32_t n, int32_t t,
                        int32_t o, FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.DrawElements((GLenum)m, n, (GLenum)t, (const void *)(intptr_t)o);
}
/* Fast path for bindTexture; the wrapper keeps ownership of the name. */
void fast_bind_texture(Local<Object> r, int32_t t, Local<Value> x,
                       FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.BindTexture((GLenum)t, gl_name(c, x));
}
/* Fast path for bindVertexArray; the wrapper keeps ownership of the name. */
void fast_bind_vertex_array(Local<Object> r, Local<Value> a,
                            FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.BindVertexArray(gl_name(c, a));
}
/* Fast path for useProgram; the program wrapper keeps its name. */
void fast_use_program(Local<Object> r, Local<Value> p,
                      FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.UseProgram(gl_name(c, p));
}
/* Fast path for uniform1f(location, x); no result, main thread only. */
void fast_uniform1f(Local<Object> r, int32_t l, double x,
                    FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.Uniform1f(l, (float)x);
}
/* Fast path for uniform1i(location, x); no result, main thread only. */
void fast_uniform1i(Local<Object> r, int32_t l, int32_t x,
                    FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.Uniform1i(l, x);
}
/* Fast path for uniform4f(location, x, y, z, w); main thread only. */
void fast_uniform4f(Local<Object> r, int32_t l, double x, double y, double z,
                    double w, FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) gl.Uniform4f(l, (float)x, (float)y, (float)z, (float)w);
}
/* Fast path for uniform1fv; `d` is a borrowed float ArrayBuffer/view. */
void fast_uniform1fv(Local<Object> r, int32_t l, Local<Value> d,
                     FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) upload_fv(options.isolate, l, d, 1);
}
/* Fast path for uniform4fv; `d` is a borrowed float vector array. */
void fast_uniform4fv(Local<Object> r, int32_t l, Local<Value> d,
                     FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (c) upload_fv(options.isolate, l, d, 4);
}
/* Fast path for uniformMatrix4fv; `d` is a borrowed 4x4 float matrix array. */
void fast_uniform_matrix4fv(Local<Object> r, int32_t l, bool t, Local<Value> d,
                            FastApiCallbackOptions &options)
{
    GLContext *c = fast_context(options.isolate, r);
    if (!c) return;
    Bytes bytes;
    if (!bytes_of(d, &bytes)) {
        throw_type(options.isolate, "uniformMatrix4fv expects a float array");
        return;
    }
    gl.UniformMatrix4fv(l, (GLsizei)(bytes.len / (16 * sizeof(float))),
                        t ? GL_TRUE : GL_FALSE, (const float *)bytes.data);
}
/* Returns the CFunction for a hot op, or an empty one for a slow method. */
static CFunction fast_for(Op op)
{
    switch (op) {
    case OP_BIND_BUFFER: return CFunction::Make(&fast_bind_buffer);
    case OP_BUFFER_SUB_DATA: return CFunction::Make(&fast_buffer_sub_data);
    case OP_DRAW_ELEMENTS: return CFunction::Make(&fast_draw_elements);
    case OP_BIND_TEXTURE: return CFunction::Make(&fast_bind_texture);
    case OP_BIND_VAO: return CFunction::Make(&fast_bind_vertex_array);
    case OP_USE_PROGRAM: return CFunction::Make(&fast_use_program);
    case OP_UNIFORM1F: return CFunction::Make(&fast_uniform1f);
    case OP_UNIFORM1I: return CFunction::Make(&fast_uniform1i);
    case OP_UNIFORM4F: return CFunction::Make(&fast_uniform4f);
    case OP_UNIFORM1FV: return CFunction::Make(&fast_uniform1fv);
    case OP_UNIFORM4FV: return CFunction::Make(&fast_uniform4fv);
    case OP_UNIFORM_MATRIX4FV: return CFunction::Make(&fast_uniform_matrix4fv);
    default: return CFunction();
    }
}
/* Every constant PIXI and the tests read; values are the GLES/WebGL enums. */
struct Constant { const char *name; int value; };
static const Constant kConstants[] = {
    {"DEPTH_BUFFER_BIT", GL_DEPTH_BUFFER_BIT},
    {"STENCIL_BUFFER_BIT", GL_STENCIL_BUFFER_BIT},
    {"COLOR_BUFFER_BIT", GL_COLOR_BUFFER_BIT},
    {"POINTS", GL_POINTS}, {"LINES", GL_LINES}, {"TRIANGLES", GL_TRIANGLES},
    {"TRIANGLE_STRIP", GL_TRIANGLE_STRIP}, {"TRIANGLE_FAN", GL_TRIANGLE_FAN},
    {"ZERO", GL_ZERO}, {"ONE", GL_ONE}, {"SRC_ALPHA", GL_SRC_ALPHA},
    {"ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA},
    {"ARRAY_BUFFER", GL_ARRAY_BUFFER},
    {"ELEMENT_ARRAY_BUFFER", GL_ELEMENT_ARRAY_BUFFER},
    {"STREAM_DRAW", GL_STREAM_DRAW}, {"STATIC_DRAW", GL_STATIC_DRAW},
    {"DYNAMIC_DRAW", GL_DYNAMIC_DRAW}, {"BYTE", GL_BYTE},
    {"UNSIGNED_BYTE", GL_UNSIGNED_BYTE}, {"SHORT", GL_SHORT},
    {"UNSIGNED_SHORT", GL_UNSIGNED_SHORT}, {"INT", GL_INT},
    {"UNSIGNED_INT", GL_UNSIGNED_INT}, {"FLOAT", GL_FLOAT},
    {"HALF_FLOAT", GL_HALF_FLOAT}, {"DEPTH_COMPONENT", GL_DEPTH_COMPONENT},
    {"ALPHA", GL_ALPHA}, {"RGB", GL_RGB}, {"RGBA", GL_RGBA},
    {"FRAGMENT_SHADER", GL_FRAGMENT_SHADER}, {"VERTEX_SHADER", GL_VERTEX_SHADER},
    {"CURRENT_PROGRAM", GL_CURRENT_PROGRAM}, {"NEVER", GL_NEVER},
    {"LESS", GL_LESS}, {"EQUAL", GL_EQUAL}, {"LEQUAL", GL_LEQUAL},
    {"GREATER", GL_GREATER}, {"NOTEQUAL", GL_NOTEQUAL},
    {"GEQUAL", GL_GEQUAL}, {"ALWAYS", GL_ALWAYS},
    {"NO_ERROR", GL_NO_ERROR}, {"INVALID_ENUM", GL_INVALID_ENUM},
    {"INVALID_VALUE", GL_INVALID_VALUE},
    {"INVALID_OPERATION", GL_INVALID_OPERATION},
    {"INVALID_FRAMEBUFFER_OPERATION", GL_INVALID_FRAMEBUFFER_OPERATION},
    {"OUT_OF_MEMORY", GL_OUT_OF_MEMORY}, {"CW", GL_CW}, {"CCW", GL_CCW},
    {"CULL_FACE", GL_CULL_FACE}, {"DEPTH_TEST", GL_DEPTH_TEST},
    {"STENCIL_TEST", GL_STENCIL_TEST}, {"BLEND", GL_BLEND},
    {"SCISSOR_TEST", GL_SCISSOR_TEST},
    {"POLYGON_OFFSET_FILL", GL_POLYGON_OFFSET_FILL}, {"DITHER", GL_DITHER},
    {"VIEWPORT", GL_VIEWPORT}, {"MAX_TEXTURE_SIZE", GL_MAX_TEXTURE_SIZE},
    {"MAX_VERTEX_ATTRIBS", GL_MAX_VERTEX_ATTRIBS},
    {"MAX_TEXTURE_IMAGE_UNITS", GL_MAX_TEXTURE_IMAGE_UNITS},
    {"MAX_VERTEX_TEXTURE_IMAGE_UNITS", GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS},
    {"MAX_COMBINED_TEXTURE_IMAGE_UNITS", GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS},
    {"MAX_CUBE_MAP_TEXTURE_SIZE", GL_MAX_CUBE_MAP_TEXTURE_SIZE},
    {"MAX_RENDERBUFFER_SIZE", GL_MAX_RENDERBUFFER_SIZE},
    {"MAX_VIEWPORT_DIMS", GL_MAX_VIEWPORT_DIMS},
    {"MAX_VARYING_VECTORS", GL_MAX_VARYING_VECTORS},
    {"MAX_VERTEX_UNIFORM_VECTORS", GL_MAX_VERTEX_UNIFORM_VECTORS},
    {"MAX_FRAGMENT_UNIFORM_VECTORS", GL_MAX_FRAGMENT_UNIFORM_VECTORS},
    {"MAX_SAMPLES", GL_MAX_SAMPLES},
    {"MAX_COLOR_ATTACHMENTS", GL_MAX_COLOR_ATTACHMENTS},
    {"TEXTURE_MAX_ANISOTROPY_EXT", 0x84FE},
    {"MAX_TEXTURE_MAX_ANISOTROPY_EXT", 0x84FF},
    {"COLOR_CLEAR_VALUE", GL_COLOR_CLEAR_VALUE},
    {"DEPTH_RANGE", GL_DEPTH_RANGE},
    {"LINE_WIDTH", GL_LINE_WIDTH},
    {"ALIASED_LINE_WIDTH_RANGE", GL_ALIASED_LINE_WIDTH_RANGE},
    {"ALIASED_POINT_SIZE_RANGE", GL_ALIASED_POINT_SIZE_RANGE},
    {"POLYGON_OFFSET_FACTOR", GL_POLYGON_OFFSET_FACTOR},
    {"POLYGON_OFFSET_UNITS", GL_POLYGON_OFFSET_UNITS},
    {"DEPTH_CLEAR_VALUE", GL_DEPTH_CLEAR_VALUE},
    {"DEPTH_WRITEMASK", GL_DEPTH_WRITEMASK},
    {"COLOR_WRITEMASK", GL_COLOR_WRITEMASK},
    {"SCISSOR_BOX", GL_SCISSOR_BOX},
    {"VERSION", GL_VERSION}, {"VENDOR", GL_VENDOR}, {"RENDERER", GL_RENDERER},
    {"SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION},
    {"TEXTURE0", GL_TEXTURE0}, {"TEXTURE_2D", GL_TEXTURE_2D},
    {"TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER},
    {"TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER},
    {"NEAREST", GL_NEAREST}, {"LINEAR", GL_LINEAR},
    {"CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE}, {"REPEAT", GL_REPEAT},
    {"FRAMEBUFFER", GL_FRAMEBUFFER}, {"FRAMEBUFFER_BINDING", GL_FRAMEBUFFER_BINDING},
    {"VERTEX_ARRAY_BINDING", GL_VERTEX_ARRAY_BINDING},
    {"TEXTURE_WRAP_S", GL_TEXTURE_WRAP_S},
    {"TEXTURE_WRAP_T", GL_TEXTURE_WRAP_T},
    {"TEXTURE_BINDING_2D", GL_TEXTURE_BINDING_2D},
    {"RENDERBUFFER", GL_RENDERBUFFER},
    {"RENDERBUFFER_BINDING", GL_RENDERBUFFER_BINDING},
    {"FRAMEBUFFER_COMPLETE", GL_FRAMEBUFFER_COMPLETE},
    {"FRAMEBUFFER_INCOMPLETE_ATTACHMENT", GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT},
    {"FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT",
     GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT},
    {"FRAMEBUFFER_UNSUPPORTED", GL_FRAMEBUFFER_UNSUPPORTED},
    {"COLOR_ATTACHMENT0", GL_COLOR_ATTACHMENT0},
    {"DEPTH_ATTACHMENT", GL_DEPTH_ATTACHMENT},
    {"STENCIL_ATTACHMENT", GL_STENCIL_ATTACHMENT},
    {"DEPTH_STENCIL_ATTACHMENT", GL_DEPTH_STENCIL_ATTACHMENT},
    {"DEPTH24_STENCIL8", GL_DEPTH24_STENCIL8},
    {"STENCIL_INDEX8", GL_STENCIL_INDEX8},
    {"RGBA8", GL_RGBA8}, {"RGB8", GL_RGB8},
    {"READ_FRAMEBUFFER", GL_READ_FRAMEBUFFER},
    {"DRAW_FRAMEBUFFER", GL_DRAW_FRAMEBUFFER},
    {"UNPACK_ALIGNMENT", GL_UNPACK_ALIGNMENT},
    {"PACK_ALIGNMENT", GL_PACK_ALIGNMENT},
    {"UNPACK_ROW_LENGTH", GL_UNPACK_ROW_LENGTH},
    {"PACK_ROW_LENGTH", GL_PACK_ROW_LENGTH},
    {"KEEP", GL_KEEP}, {"REPLACE", GL_REPLACE}, {"INCR", GL_INCR},
    {"DECR", GL_DECR}, {"INVERT", GL_INVERT}, {"INCR_WRAP", GL_INCR_WRAP},
    {"DECR_WRAP", GL_DECR_WRAP},
    {"COMPILE_STATUS", GL_COMPILE_STATUS}, {"LINK_STATUS", GL_LINK_STATUS},
    {"DELETE_STATUS", GL_DELETE_STATUS}, {"SHADER_TYPE", GL_SHADER_TYPE},
    {"ACTIVE_UNIFORMS", GL_ACTIVE_UNIFORMS},
    {"ACTIVE_ATTRIBUTES", GL_ACTIVE_ATTRIBUTES},
    {"UNPACK_FLIP_Y_WEBGL", 0x9240},
    {"UNPACK_PREMULTIPLY_ALPHA_WEBGL", 0x9241},
};

/* Owns the weak handle and C record of a context or object wrapper. */
template <typename T> struct WeakRecord {
    T *value;
    Persistent<Object> handle;
    WeakRecord(Isolate *isolate, Local<Object> object)
        : value(NULL), handle(isolate, object) {}
};

/* Second pass: queue a collected context; GL runs only in webgl_drain. */
void context_finalizer_second(const WeakCallbackInfo<WeakRecord<GLContext>> &info)
{
    WeakRecord<GLContext> *record = info.GetParameter();
    context_enqueue(record->value);
    record->handle.Reset();
    delete record;
}
/* First pass: defer the context's teardown to the second pass. */
void context_finalizer(const WeakCallbackInfo<WeakRecord<GLContext>> &info)
{
    info.SetSecondPassCallback(&context_finalizer_second);
}
/* Second pass: queue a collected wrapper's GL name, then free the record. */
void object_finalizer_second(const WeakCallbackInfo<WeakRecord<GLObject>> &info)
{
    WeakRecord<GLObject> *record = info.GetParameter();
    object_gc(record->value);
    record->handle.Reset();
    delete record;
}
/* First pass: defer a wrapper's GL work to the second pass. */
void object_finalizer(const WeakCallbackInfo<WeakRecord<GLObject>> &info)
{
    info.SetSecondPassCallback(&object_finalizer_second);
}
/* Creates a JS wrapper owning one buffer/VAO record (spec section 15.8). */
Local<Object> make_object(Isolate *isolate, Local<Context> context,
                          GLObject *object)
{
    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
    templ->SetInternalFieldCount(1);
    Local<Object> wrapper = templ->NewInstance(context).ToLocalChecked();
    wrapper->SetAlignedPointerInInternalField(0, object);
    WeakRecord<GLObject> *record =
        new WeakRecord<GLObject>(isolate, wrapper);
    record->value = object;
    record->handle.SetWeak(record, &object_finalizer,
                           v8::WeakCallbackType::kParameter);
    return wrapper;
}
/* Reads the drawing-buffer properties (defined beside make_context). */
void get_drawing_width(Local<Name>, const PropertyCallbackInfo<Value> &info);
void get_drawing_height(Local<Name>, const PropertyCallbackInfo<Value> &info);

/* One template per isolate; reusing it avoids rebuilding every method and
 * CFunction for each context. Allocated with new and released by
 * webgl_shutdown so a static destructor never touches a disposed isolate. */
static Global<ObjectTemplate> *s_template;
static Isolate *s_template_isolate;

/* Adds the methods, their CFunctions and the WebGL constants to `templ`. */
static void fill_template(Isolate *isolate, Local<ObjectTemplate> templ)
{
    for (size_t i = 0; i < sizeof(kMethods) / sizeof(kMethods[0]); i++) {
        const Method *method = &kMethods[i];
        CFunction fast = fast_for(method->op);
        bool hot = fast.GetAddress() != NULL;
        Local<FunctionTemplate> function = FunctionTemplate::New(
            isolate, slow_method, External::New(isolate, (void *)method),
            Local<v8::Signature>(), method->length,
            v8::ConstructorBehavior::kThrow,
            v8::SideEffectType::kHasSideEffect, hot ? &fast : NULL);
        function->SetClassName(utf8(isolate, method->name));
        templ->Set(isolate, method->name, function);
        if (hot) record_fast(method->name);
    }
    for (size_t i = 0; i < sizeof(kConstants) / sizeof(kConstants[0]); i++)
        templ->Set(isolate, kConstants[i].name,
                   Integer::New(isolate, kConstants[i].value));
}
/* Rebuilds the per-isolate template and records its fast calls. */
static void build_template(Isolate *isolate)
{
    g_fast_count = 0;
    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
    templ->SetInternalFieldCount(1);
    fill_template(isolate, templ);
    templ->SetNativeDataProperty(
        utf8(isolate, "drawingBufferWidth"), &get_drawing_width);
    templ->SetNativeDataProperty(
        utf8(isolate, "drawingBufferHeight"), &get_drawing_height);
    s_template->Reset(isolate, templ);
}
/* Returns the per-isolate context template, building it on first use. */
static Local<ObjectTemplate> context_template(Isolate *isolate)
{
    if (s_template_isolate != isolate) {
        /* A Global belongs to one isolate. Abandon the previous handle rather
         * than touch an isolate that may already be disposed. */
        s_template = new Global<ObjectTemplate>();
        s_template_isolate = isolate;
    }
    if (s_template->IsEmpty()) build_template(isolate);
    return s_template->Get(isolate);
}
/* Reads a context's drawing-buffer width; the canvas resize keeps it live. */
void get_drawing_width(Local<Name>, const PropertyCallbackInfo<Value> &info)
{
    GLContext *ctx = live(context_of(info.This()));
    if (ctx) info.GetReturnValue().Set(Integer::New(info.GetIsolate(), ctx->width));
}
/* Reads a context's drawing-buffer height; the canvas resize keeps it live. */
void get_drawing_height(Local<Name>, const PropertyCallbackInfo<Value> &info)
{
    GLContext *ctx = live(context_of(info.This()));
    if (ctx) info.GetReturnValue().Set(Integer::New(info.GetIsolate(), ctx->height));
}
/* Creates the native context object returned by createContext. */
Local<Object> make_context(Isolate *isolate, Local<Context> context,
                           GLContext *ctx)
{
    Local<Object> object =
        context_template(isolate)->NewInstance(context).ToLocalChecked();
    object->SetAlignedPointerInInternalField(0, ctx);
    WeakRecord<GLContext> *record =
        new WeakRecord<GLContext>(isolate, object);
    record->value = ctx;
    record->handle.SetWeak(record, &context_finalizer,
                           v8::WeakCallbackType::kParameter);
    return object;
}
/* createContext(width, height) -> a WebGL2 context or null. Draining here
 * gives collected contexts their main-thread GL teardown outside GC. */
void js_create_context(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    drain_destroy_queue();
    drain_pending_all();
    GLContext *ctx = context_create(arg_i32(isolate, info, 0),
                                    arg_i32(isolate, info, 1));
    if (!ctx) { info.GetReturnValue().SetNull(); return; }
    info.GetReturnValue().Set(
        make_context(isolate, isolate->GetCurrentContext(), ctx));
}
/* resize(context, width, height) -> undefined. The canvas element calls this
 * after its width/height setters so drawingBuffer* stays truthful (WebGL does
 * not change the viewport on resize, so neither does this). */
void js_resize_context(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    Local<Value> target = info[0];
    GLContext *ctx = target->IsObject()
                       ? live(context_of(target.As<Object>())) : NULL;
    if (!ctx) { throw_illegal(isolate); return; }
    int32_t width = arg_i32(isolate, info, 1);
    int32_t height = arg_i32(isolate, info, 2);
    ctx->width = width > 0 ? width : 0;
    ctx->height = height > 0 ? height : 0;
}
/* fastApiInfo() -> {count, names}. */
void js_fast_api_info(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> result = Object::New(isolate);
    result->Set(context, utf8(isolate, "count"),
                Integer::NewFromUnsigned(isolate, (uint32_t)g_fast_count)).Check();
    Local<Array> names = Array::New(isolate, (int)g_fast_count);
    for (size_t i = 0; i < g_fast_count; i++)
        names->Set(context, (uint32_t)i, utf8(isolate, g_fast_names[i])).Check();
    result->Set(context, utf8(isolate, "names"), names).Check();
    info.GetReturnValue().Set(result);
}
/* Defines the module in the current V8 context; runs inside jsb setup. */
bool define_module(jsb_ctx *ctx, void *user)
{
    (void)ctx;
    (void)user;
    Isolate *isolate = Isolate::GetCurrent();
    Local<Context> context = isolate->GetCurrentContext();

    /* Build the template before a GL device exists so the fast-api test can
     * prove CFunction use on a headless machine. */
    context_template(isolate);

    Local<Object> module = Object::New(isolate);
    Local<Function> create =
        Function::New(context, js_create_context).ToLocalChecked();
    create->SetName(utf8(isolate, "createContext"));
    module->Set(context, utf8(isolate, "createContext"), create).Check();
    Local<Function> resize =
        Function::New(context, js_resize_context).ToLocalChecked();
    resize->SetName(utf8(isolate, "resize"));
    module->Set(context, utf8(isolate, "resize"), resize).Check();
    Local<Function> fast_info =
        Function::New(context, js_fast_api_info).ToLocalChecked();
    fast_info->SetName(utf8(isolate, "fastApiInfo"));
    module->Set(context, utf8(isolate, "fastApiInfo"), fast_info).Check();
    return context->Global()
        ->Set(context, utf8(isolate, "__native_webgl"), module)
        .FromMaybe(false);
}

} // namespace

/* See webgl.h: webgl_define. */
extern "C" bool webgl_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "webgl_define: no jsb runtime\n");
        return false;
    }
    return jsb_runtime_setup(rt, define_module, NULL);
}
/* See webgl.h: webgl_drain. */
extern "C" void webgl_drain(void)
{
    drain_destroy_queue();
    drain_pending_all();
}
/* See webgl.h: webgl_shutdown. */
extern "C" void webgl_shutdown(void)
{
    drain_destroy_queue();
    if (s_template) {
        s_template->Reset();
        delete s_template;
        s_template = NULL;
    }
    s_template_isolate = NULL;
    g_fast_count = 0;
}
/* See webgl.h: webgl_fast_api_count. */
extern "C" size_t webgl_fast_api_count(void)
{
    return g_fast_count;
}
/* See webgl.h: webgl_fast_api_name. */
extern "C" const char *webgl_fast_api_name(size_t index)
{
    return index < g_fast_count ? g_fast_names[index] : NULL;
}
