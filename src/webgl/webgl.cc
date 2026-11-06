/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Native WebGL2 context over GLES 3.0 / GL 3.3 core (spec section 4.2).
 *
 * Why: real PIXI and UltraMode7 call canvas.getContext('webgl2') and drive the
 * GL state machine directly. This file owns the GL context, C state and V8
 * objects in one place because the hot calls must be v8::CFunction fast calls
 * rather than only jsb slow callbacks, while each JS wrapper must own exactly
 * one GL name (spec section 15.8). Textures, FBOs, shaders and extensions are
 * M2-T002/M2-T003. See docs/webgl.md.
 *
 * Rules: main thread only. No v8::Local is stored in C; explicit deletes free
 * names now and GC deletes only queue them for the end-of-frame drain.
 */
#include "webgl/webgl.h"

#include "jsb/jsb.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <v8-fast-api-calls.h>
#include <v8.h>

#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

using v8::Array;
using v8::ArrayBuffer;
using v8::ArrayBufferView;
using v8::CFunction;
using v8::Context;
using v8::Exception;
using v8::External;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::ObjectTemplate;
using v8::Persistent;
using v8::Signature;
using v8::String;
using v8::Value;
using v8::WeakCallbackInfo;

/* ---- GL entry points -------------------------------------------------- */

/* One resolved entry point per call; function-pointer fields avoid the PFN
 * name split between desktop GL and GLES headers. */
struct GL {
    GLenum (APIENTRY *GetError)(void);
    void (APIENTRY *GetIntegerv)(GLenum, GLint *);
    void (APIENTRY *GetFloatv)(GLenum, GLfloat *);
    const GLubyte *(APIENTRY *GetString)(GLenum);
    void (APIENTRY *Viewport)(GLint, GLint, GLsizei, GLsizei);
    void (APIENTRY *ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
    void (APIENTRY *Clear)(GLbitfield);
    void (APIENTRY *Enable)(GLenum);
    void (APIENTRY *Disable)(GLenum);
    GLboolean (APIENTRY *IsEnabled)(GLenum);
    void (APIENTRY *BlendFunc)(GLenum, GLenum);
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
    void (APIENTRY *Uniform4fv)(GLint, GLsizei, const GLfloat *);
    void (APIENTRY *UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat *);
    void (APIENTRY *BindFramebuffer)(GLenum, GLuint);
};

static GL gl;

/* Loads one entry point through SDL; `missing` counts failures. */
#define LOAD(name, field)                                                      \
    do {                                                                       \
        SDL_FunctionPointer proc = SDL_GL_GetProcAddress(#name);               \
        memcpy(&gl.field, &proc, sizeof(proc));                                \
        if (!gl.field) missing++;                                              \
    } while (0)

/* Resolves the entry points needed by this card after a context is current. */
static bool gl_load(void)
{
    int missing = 0;
    LOAD(glGetError, GetError);
    LOAD(glGetIntegerv, GetIntegerv);
    LOAD(glGetFloatv, GetFloatv);
    LOAD(glGetString, GetString);
    LOAD(glViewport, Viewport);
    LOAD(glClearColor, ClearColor);
    LOAD(glClear, Clear);
    LOAD(glEnable, Enable);
    LOAD(glDisable, Disable);
    LOAD(glIsEnabled, IsEnabled);
    LOAD(glBlendFunc, BlendFunc);
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
    LOAD(glUniform4fv, Uniform4fv);
    LOAD(glUniformMatrix4fv, UniformMatrix4fv);
    LOAD(glBindFramebuffer, BindFramebuffer);
    if (missing) fprintf(stderr, "webgl: %d GL entry point(s) missing\n", missing);
    return missing == 0;
}

static bool s_backend_checked;
static bool s_backend_ready;
static SDL_Window *s_hidden_window;
static SDL_GLContext s_owned_context;

/* Uses an existing OpenGL window, or creates a hidden GLES 3.0/GL 3.3 one. */
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
        s_owned_context = SDL_GL_GetCurrentContext();
        return s_owned_context && SDL_GL_MakeCurrent(window, s_owned_context);
    }

    s_hidden_window = SDL_CreateWindow("smzuggler WebGL", 16, 16,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!s_hidden_window) return false;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    s_owned_context = SDL_GL_CreateContext(s_hidden_window);
    if (!s_owned_context) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        s_owned_context = SDL_GL_CreateContext(s_hidden_window);
    }
    return s_owned_context &&
           SDL_GL_MakeCurrent(s_hidden_window, s_owned_context);
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
    return s_backend_ready;
}

#define MAX_CONTEXTS 8

/* One queued GC delete, drained at frame end (spec section 15.8). */
struct Pending { Pending *next; GLuint name; bool buffer; };

/* One WebGL2 context and its drawing-buffer size. */
struct GLContext {
    uint32_t id;
    int width;
    int height;
    Pending *pending;
};

/* A wrapper-owned GL name; `buffer` selects buffers vs vertex arrays. */
struct GLObject {
    uint32_t context_id;
    GLuint name;
    bool buffer;
    bool deleted;
};

static GLContext *s_contexts[MAX_CONTEXTS];

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

/* Deletes queued names and frees the pending nodes. */
static void drain(GLContext *ctx)
{
    Pending *node = ctx->pending;
    ctx->pending = NULL;
    while (node) {
        Pending *next = node->next;
        if (node->buffer) gl.DeleteBuffers(1, &node->name);
        else gl.DeleteVertexArrays(1, &node->name);
        free(node);
        node = next;
    }
}

/* Destroys a context; wrappers that outlive it become inert. */
static void context_destroy(GLContext *ctx)
{
    if (!ctx) return;
    if (ctx->id > 0 && ctx->id <= MAX_CONTEXTS &&
        s_contexts[ctx->id - 1] == ctx)
        s_contexts[ctx->id - 1] = NULL;
    drain(ctx);
    ctx->id = 0;
    free(ctx);
}

/* Creates one buffer or VAO record; NULL on allocation/GL failure. */
static GLObject *object_create(GLContext *ctx, bool buffer)
{
    if (!live(ctx)) return NULL;
    GLuint name = 0;
    if (buffer) gl.GenBuffers(1, &name); else gl.GenVertexArrays(1, &name);
    GLObject *object = name ? (GLObject *)calloc(1, sizeof(*object)) : NULL;
    if (!object) {
        if (buffer) gl.DeleteBuffers(1, &name);
        else gl.DeleteVertexArrays(1, &name);
        return NULL;
    }
    object->context_id = ctx->id;
    object->name = name;
    object->buffer = buffer;
    return object;
}

/* Explicit delete: releases the name now and marks the wrapper dead. */
static void object_delete(GLObject *object)
{
    if (!object || object->deleted) return;
    GLContext *ctx = object->context_id && object->context_id <= MAX_CONTEXTS
                       ? s_contexts[object->context_id - 1] : NULL;
    if (ctx && object->name) {
        if (object->buffer) gl.DeleteBuffers(1, &object->name);
        else gl.DeleteVertexArrays(1, &object->name);
    }
    object->name = 0;
    object->deleted = true;
}

/* GC delete: queue the name for the frame drain, then free the record. */
static void object_gc(GLObject *object)
{
    if (!object) return;
    GLContext *ctx = object->context_id && object->context_id <= MAX_CONTEXTS
                       ? s_contexts[object->context_id - 1] : NULL;
    if (ctx && !object->deleted && object->name) {
        Pending *node = (Pending *)malloc(sizeof(*node));
        if (node) {
            node->name = object->name;
            node->buffer = object->buffer;
            node->next = ctx->pending;
            ctx->pending = node;
        }
    }
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

/* Returns the wrapper record behind a JS value, or NULL for null/undefined. */
static GLObject *object_of(Local<Value> value)
{
    if (value->IsNullOrUndefined() || !value->IsObject()) return NULL;
    Local<Object> object = value.As<Object>();
    if (object->InternalFieldCount() < 1) return NULL;
    return static_cast<GLObject *>(
        object->GetAlignedPointerFromInternalField(0));
}

/* Resolves null or a wrapper/number to a GL name (0 for null). */
static uint32_t gl_name(Local<Value> value)
{
    if (value->IsNullOrUndefined()) return 0;
    GLObject *object = object_of(value);
    if (object) return object->deleted ? 0 : object->name;
    if (value->IsNumber())
        return value->Uint32Value(Isolate::GetCurrent()->GetCurrentContext())
            .FromMaybe(0);
    return 0;
}

/* Borrowed bytes of an ArrayBuffer/view; `store` keeps them alive for GL. */
struct Bytes {
    std::shared_ptr<v8::BackingStore> store;
    const uint8_t *data = NULL;
    size_t len = 0;
};

/* Fills `bytes` from a buffer or view; false for any other value. */
static bool bytes_of(Local<Value> value, Bytes *bytes)
{
    if (value->IsArrayBuffer()) {
        bytes->store = value.As<ArrayBuffer>()->GetBackingStore();
        bytes->data = (const uint8_t *)bytes->store->Data();
        bytes->len = bytes->store->ByteLength();
        return true;
    }
    if (value->IsArrayBufferView()) {
        Local<ArrayBufferView> view = value.As<ArrayBufferView>();
        bytes->store = view->Buffer()->GetBackingStore();
        bytes->data = (const uint8_t *)bytes->store->Data() + view->ByteOffset();
        bytes->len = view->ByteLength();
        return true;
    }
    return false;
}

enum Op {
    OP_VIEWPORT, OP_CLEAR_COLOR, OP_CLEAR, OP_ENABLE, OP_DISABLE, OP_IS_ENABLED,
    OP_GET_ERROR, OP_GET_PARAMETER, OP_BLEND_FUNC,
    OP_CREATE_BUFFER, OP_DELETE_BUFFER, OP_BIND_BUFFER, OP_BUFFER_DATA,
    OP_BUFFER_SUB_DATA, OP_CREATE_VAO, OP_DELETE_VAO, OP_BIND_VAO,
    OP_ENABLE_ATTRIB, OP_DISABLE_ATTRIB, OP_VERTEX_ATTRIB, OP_DRAW_ARRAYS,
    OP_DRAW_ELEMENTS, OP_ACTIVE_TEXTURE, OP_BIND_TEXTURE, OP_USE_PROGRAM,
    OP_BIND_FRAMEBUFFER, OP_UNIFORM1F, OP_UNIFORM1I, OP_UNIFORM4F,
    OP_UNIFORM1FV, OP_UNIFORM4FV, OP_UNIFORM_MATRIX4FV, OP_RESIZE
};

/* One JS method: name, `length` and its switch case. */
struct Method { const char *name; int length; Op op; };

static const Method kMethods[] = {
    {"viewport", 4, OP_VIEWPORT},
    {"clearColor", 4, OP_CLEAR_COLOR},
    {"clear", 1, OP_CLEAR},
    {"enable", 1, OP_ENABLE},
    {"disable", 1, OP_DISABLE},
    {"isEnabled", 1, OP_IS_ENABLED},
    {"getError", 0, OP_GET_ERROR},
    {"getParameter", 1, OP_GET_PARAMETER},
    {"blendFunc", 2, OP_BLEND_FUNC},
    {"createBuffer", 0, OP_CREATE_BUFFER},
    {"deleteBuffer", 1, OP_DELETE_BUFFER},
    {"bindBuffer", 2, OP_BIND_BUFFER},
    {"bufferData", 3, OP_BUFFER_DATA},
    {"bufferSubData", 3, OP_BUFFER_SUB_DATA},
    {"createVertexArray", 0, OP_CREATE_VAO},
    {"deleteVertexArray", 1, OP_DELETE_VAO},
    {"bindVertexArray", 1, OP_BIND_VAO},
    {"enableVertexAttribArray", 1, OP_ENABLE_ATTRIB},
    {"disableVertexAttribArray", 1, OP_DISABLE_ATTRIB},
    {"vertexAttribPointer", 6, OP_VERTEX_ATTRIB},
    {"drawArrays", 3, OP_DRAW_ARRAYS},
    {"drawElements", 4, OP_DRAW_ELEMENTS},
    {"activeTexture", 1, OP_ACTIVE_TEXTURE},
    {"bindTexture", 2, OP_BIND_TEXTURE},
    {"useProgram", 1, OP_USE_PROGRAM},
    {"bindFramebuffer", 2, OP_BIND_FRAMEBUFFER},
    {"uniform1f", 2, OP_UNIFORM1F},
    {"uniform1i", 2, OP_UNIFORM1I},
    {"uniform4f", 5, OP_UNIFORM4F},
    {"uniform1fv", 2, OP_UNIFORM1FV},
    {"uniform4fv", 2, OP_UNIFORM4FV},
    {"uniformMatrix4fv", 3, OP_UNIFORM_MATRIX4FV},
    {"_resize", 2, OP_RESIZE},
};

/* Uploads a float uniform vector of `components` width. */
static void upload_fv(GLContext *ctx, int32_t location, Local<Value> data,
                      int components)
{
    (void)ctx;
    Bytes bytes;
    if (!bytes_of(data, &bytes)) return;
    int32_t count = (int32_t)(bytes.len / (sizeof(float) * components));
    const float *v = (const float *)bytes.data;
    if (components == 1) gl.Uniform1fv(location, count, v);
    else gl.Uniform4fv(location, count, v);
}

/* Creates a JS wrapper for a buffer/VAO record; defined with its finalizers. */
static Local<Object> make_object(Isolate *isolate, Local<Context> context,
                                 GLObject *object);

/* Handles every slow path; `data` is the table row being called. */
static void slow_method(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    Local<Context> context = isolate->GetCurrentContext();
    const Method *method =
        static_cast<const Method *>(info.Data().As<External>()->Value());
    GLContext *ctx = context_of(info.This());

    switch (method->op) {
    case OP_VIEWPORT:
        gl.Viewport(arg_i32(isolate, info, 0), arg_i32(isolate, info, 1),
                    arg_i32(isolate, info, 2), arg_i32(isolate, info, 3));
        break;
    case OP_CLEAR_COLOR:
        gl.ClearColor(arg_f32(isolate, info, 0), arg_f32(isolate, info, 1),
                      arg_f32(isolate, info, 2), arg_f32(isolate, info, 3));
        break;
    case OP_CLEAR: gl.Clear(arg_u32(isolate, info, 0)); break;
    case OP_ENABLE: gl.Enable(arg_u32(isolate, info, 0)); break;
    case OP_DISABLE: gl.Disable(arg_u32(isolate, info, 0)); break;
    case OP_IS_ENABLED:
        info.GetReturnValue().Set(
            gl.IsEnabled(arg_u32(isolate, info, 0)) == GL_TRUE);
        break;
    case OP_GET_ERROR:
        info.GetReturnValue().Set(
            Integer::NewFromUnsigned(isolate, gl.GetError()));
        break;
    case OP_GET_PARAMETER: {
        uint32_t pname = arg_u32(isolate, info, 0);
        if (pname == GL_VERSION || pname == GL_VENDOR ||
            pname == GL_RENDERER || pname == GL_SHADING_LANGUAGE_VERSION) {
            const GLubyte *text = gl.GetString(pname);
            if (text) info.GetReturnValue().Set(utf8(isolate, (const char *)text));
        } else {
            GLint value = 0;
            gl.GetIntegerv(pname, &value);
            info.GetReturnValue().Set(Integer::New(isolate, value));
        }
        break;
    }
        gl.BlendFunc(arg_u32(isolate, info, 0), arg_u32(isolate, info, 1));
        break;
    case OP_BLEND_FUNC:
        gl.BlendFunc(arg_u32(isolate, info, 0), arg_u32(isolate, info, 1));
        break;
    case OP_CREATE_BUFFER:
    case OP_CREATE_VAO: {
        GLObject *object = object_create(ctx, method->op == OP_CREATE_BUFFER);
        if (!object) { info.GetReturnValue().SetNull(); break; }
        info.GetReturnValue().Set(make_object(isolate, context, object));
        break;
    }
    case OP_DELETE_BUFFER:
    case OP_DELETE_VAO: {
        GLObject *object = object_of(info[0]);
        if (object && object->buffer == (method->op == OP_DELETE_BUFFER))
            object_delete(object);
        break;
    }
    case OP_BIND_BUFFER:
        gl.BindBuffer(arg_u32(isolate, info, 0), gl_name(info[1]));
        break;
    case OP_BUFFER_DATA: {
        Local<Value> source = info[1];
        Bytes bytes;
        size_t size = 0;
        if (source->IsNumber()) {
            int64_t n = source->IntegerValue(context).FromMaybe(0);
            if (n < 0) {
                isolate->ThrowException(Exception::RangeError(
                    utf8(isolate, "buffer size must not be negative")));
                break;
            }
            size = (size_t)n;
        } else if (bytes_of(source, &bytes)) {
            size = bytes.len;
        } else {
            throw_type(isolate, "expected an ArrayBuffer, a view or a size");
            break;
        }
        gl.BufferData(arg_u32(isolate, info, 0), (GLsizeiptr)size, bytes.data,
                      arg_u32(isolate, info, 2));
        break;
    }
    case OP_BUFFER_SUB_DATA: {
        Bytes bytes;
        if (!bytes_of(info[2], &bytes)) {
            throw_type(isolate, "bufferSubData expects an ArrayBuffer or a view");
            break;
        }
        gl.BufferSubData(arg_u32(isolate, info, 0),
                         (GLintptr)arg_i64(isolate, info, 1),
                         (GLsizeiptr)bytes.len, bytes.data);
        break;
    }
    case OP_BIND_VAO: gl.BindVertexArray(gl_name(info[0])); break;
    case OP_ENABLE_ATTRIB:
        gl.EnableVertexAttribArray(arg_u32(isolate, info, 0));
        break;
    case OP_DISABLE_ATTRIB:
        gl.DisableVertexAttribArray(arg_u32(isolate, info, 0));
        break;
    case OP_VERTEX_ATTRIB:
        gl.VertexAttribPointer(arg_u32(isolate, info, 0),
                               arg_i32(isolate, info, 1),
                               arg_u32(isolate, info, 2),
                               arg_bool(isolate, info, 3) ? GL_TRUE : GL_FALSE,
                               arg_i32(isolate, info, 4),
                               (const void *)(intptr_t)arg_i64(isolate, info, 5));
        break;
    case OP_DRAW_ARRAYS:
        gl.DrawArrays(arg_u32(isolate, info, 0), arg_i32(isolate, info, 1),
                      arg_i32(isolate, info, 2));
        break;
    case OP_DRAW_ELEMENTS:
        gl.DrawElements(arg_u32(isolate, info, 0), arg_i32(isolate, info, 1),
                        arg_u32(isolate, info, 2),
                        (const void *)(intptr_t)arg_i64(isolate, info, 3));
        break;
    case OP_ACTIVE_TEXTURE: gl.ActiveTexture(arg_u32(isolate, info, 0)); break;
    case OP_BIND_TEXTURE:
        gl.BindTexture(arg_u32(isolate, info, 0), gl_name(info[1]));
        break;
    case OP_USE_PROGRAM: gl.UseProgram(gl_name(info[0])); break;
    case OP_BIND_FRAMEBUFFER:
        gl.BindFramebuffer(arg_u32(isolate, info, 0), gl_name(info[1]));
        break;
    case OP_UNIFORM1F:
        gl.Uniform1f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1));
        break;
    case OP_UNIFORM1I:
        gl.Uniform1i(arg_i32(isolate, info, 0), arg_i32(isolate, info, 1));
        break;
    case OP_UNIFORM4F:
        gl.Uniform4f(arg_i32(isolate, info, 0), arg_f32(isolate, info, 1),
                     arg_f32(isolate, info, 2), arg_f32(isolate, info, 3),
                     arg_f32(isolate, info, 4));
        break;
    case OP_UNIFORM1FV: upload_fv(ctx, arg_i32(isolate, info, 0), info[1], 1); break;
    case OP_UNIFORM4FV: upload_fv(ctx, arg_i32(isolate, info, 0), info[1], 4); break;
    case OP_UNIFORM_MATRIX4FV: {
        Bytes bytes;
        if (!bytes_of(info[2], &bytes)) break;
        gl.UniformMatrix4fv(arg_i32(isolate, info, 0),
                            (GLsizei)(bytes.len / (16 * sizeof(float))),
                            arg_bool(isolate, info, 1) ? GL_TRUE : GL_FALSE,
                            (const float *)bytes.data);
        break;
    }
    case OP_RESIZE: {
        int width = arg_i32(isolate, info, 0);
        int height = arg_i32(isolate, info, 1);
        if (width <= 0 || height <= 0) break;
        ctx->width = width;
        ctx->height = height;
        gl.Viewport(0, 0, width, height);
        Local<Object> self = info.This();
        self->Set(context, utf8(isolate, "drawingBufferWidth"),
                  Integer::New(isolate, width)).Check();
        self->Set(context, utf8(isolate, "drawingBufferHeight"),
                  Integer::New(isolate, height)).Check();
        break;
    }
    }
}

/* Fast bindBuffer. */
void fast_bind_buffer(Local<Object> r, int32_t t, Local<Value> b)
{ GLContext *c = context_of(r); if (c) gl.BindBuffer((GLenum)t, gl_name(b)); }
/* Fast bufferSubData. */
void fast_buffer_sub_data(Local<Object> r, int32_t t, int32_t o, Local<Value> d)
{
    GLContext *c = context_of(r);
    Bytes bytes;
    if (c && bytes_of(d, &bytes))
        gl.BufferSubData((GLenum)t, o, (GLsizeiptr)bytes.len, bytes.data);
}
/* Fast drawElements. */
void fast_draw_elements(Local<Object> r, int32_t m, int32_t n, int32_t t,
                        int32_t o)
{ GLContext *c = context_of(r); if (c) gl.DrawElements((GLenum)m, n, (GLenum)t, (const void *)(intptr_t)o); }
/* Fast bindTexture. */
void fast_bind_texture(Local<Object> r, int32_t t, Local<Value> x)
{ GLContext *c = context_of(r); if (c) gl.BindTexture((GLenum)t, gl_name(x)); }
/* Fast bindVertexArray. */
void fast_bind_vertex_array(Local<Object> r, Local<Value> a)
{ GLContext *c = context_of(r); if (c) gl.BindVertexArray(gl_name(a)); }
/* Fast useProgram. */
void fast_use_program(Local<Object> r, Local<Value> p)
{ GLContext *c = context_of(r); if (c) gl.UseProgram(gl_name(p)); }
/* Fast uniform1f. */
void fast_uniform1f(Local<Object> r, int32_t l, double x)
{ GLContext *c = context_of(r); if (c) gl.Uniform1f(l, (float)x); }
/* Fast uniform1i. */
void fast_uniform1i(Local<Object> r, int32_t l, int32_t x)
{ GLContext *c = context_of(r); if (c) gl.Uniform1i(l, x); }
/* Fast uniform4f. */
void fast_uniform4f(Local<Object> r, int32_t l, double x, double y, double z,
                    double w)
{ GLContext *c = context_of(r); if (c) gl.Uniform4f(l, (float)x, (float)y, (float)z, (float)w); }
/* Fast uniform1fv. */
void fast_uniform1fv(Local<Object> r, int32_t l, Local<Value> d)
{ GLContext *c = context_of(r); if (c) upload_fv(c, l, d, 1); }
/* Fast uniform4fv. */
void fast_uniform4fv(Local<Object> r, int32_t l, Local<Value> d)
{ GLContext *c = context_of(r); if (c) upload_fv(c, l, d, 4); }
/* Fast uniformMatrix4fv. */
void fast_uniform_matrix4fv(Local<Object> r, int32_t l, bool t, Local<Value> d)
{
    GLContext *c = context_of(r);
    Bytes bytes;
    if (!c || !bytes_of(d, &bytes)) return;
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
    {"VERSION", GL_VERSION}, {"VENDOR", GL_VENDOR}, {"RENDERER", GL_RENDERER},
    {"SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION},
    {"TEXTURE0", GL_TEXTURE0}, {"TEXTURE_2D", GL_TEXTURE_2D},
    {"TEXTURE_MIN_FILTER", GL_TEXTURE_MIN_FILTER},
    {"TEXTURE_MAG_FILTER", GL_TEXTURE_MAG_FILTER},
    {"NEAREST", GL_NEAREST}, {"LINEAR", GL_LINEAR},
    {"CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE}, {"REPEAT", GL_REPEAT},
    {"FRAMEBUFFER", GL_FRAMEBUFFER}, {"FRAMEBUFFER_BINDING", GL_FRAMEBUFFER_BINDING},
    {"VERTEX_ARRAY_BINDING", GL_VERTEX_ARRAY_BINDING},
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

/* Second pass: destroy a collected context on the main thread. */
void context_finalizer_second(const WeakCallbackInfo<WeakRecord<GLContext>> &info)
{
    WeakRecord<GLContext> *record = info.GetParameter();
    context_destroy(record->value);
    record->handle.Reset();
    delete record;
}

/* First pass: defer the context's GL work to the second pass. */
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

/* Installs every WebGL method/constant on one context instance. */
static void define_methods(Isolate *isolate, Local<Context> context,
                           Local<Object> target)
{
    for (size_t i = 0; i < sizeof(kMethods) / sizeof(kMethods[0]); i++) {
        const Method *method = &kMethods[i];
        CFunction fast = fast_for(method->op);
        bool hot = fast.GetAddress() != NULL;
        Local<FunctionTemplate> templ = FunctionTemplate::New(
            isolate, slow_method, External::New(isolate, (void *)method),
            Local<Signature>(), method->length,
            v8::ConstructorBehavior::kThrow,
            v8::SideEffectType::kHasSideEffect, hot ? &fast : NULL);
        Local<String> key = utf8(isolate, method->name);
        Local<Function> function = templ->GetFunction(context).ToLocalChecked();
        function->SetName(key);
        target->Set(context, key, function).Check();
        if (hot) record_fast(method->name);
    }
    for (size_t i = 0; i < sizeof(kConstants) / sizeof(kConstants[0]); i++)
        target->Set(context, utf8(isolate, kConstants[i].name),
                    Integer::New(isolate, kConstants[i].value))
            .Check();
}

/* Creates the native context object returned by createContext. */
Local<Object> make_context(Isolate *isolate, Local<Context> context,
                           GLContext *ctx)
{
    Local<ObjectTemplate> templ = ObjectTemplate::New(isolate);
    templ->SetInternalFieldCount(1);
    Local<Object> object = templ->NewInstance(context).ToLocalChecked();
    object->SetAlignedPointerInInternalField(0, ctx);
    define_methods(isolate, context, object);
    object->Set(context, utf8(isolate, "drawingBufferWidth"),
                Integer::New(isolate, ctx->width)).Check();
    object->Set(context, utf8(isolate, "drawingBufferHeight"),
                Integer::New(isolate, ctx->height)).Check();
    WeakRecord<GLContext> *record =
        new WeakRecord<GLContext>(isolate, object);
    record->value = ctx;
    record->handle.SetWeak(record, &context_finalizer,
                           v8::WeakCallbackType::kParameter);
    return object;
}

/* createContext(width, height) -> a WebGL2 context or null. */
void js_create_context(const FunctionCallbackInfo<Value> &info)
{
    Isolate *isolate = info.GetIsolate();
    GLContext *ctx = context_create(arg_i32(isolate, info, 0),
                                    arg_i32(isolate, info, 1));
    if (!ctx) { info.GetReturnValue().SetNull(); return; }
    info.GetReturnValue().Set(
        make_context(isolate, isolate->GetCurrentContext(), ctx));
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
    g_fast_count = 0;

    /* Register hot calls before a GL device exists so the fast-api test can
     * prove CFunction use on a headless machine. */
    Local<Object> probe = Object::New(isolate);
    define_methods(isolate, context, probe);

    Local<Object> module = Object::New(isolate);
    Local<Function> create =
        Function::New(context, js_create_context).ToLocalChecked();
    create->SetName(utf8(isolate, "createContext"));
    module->Set(context, utf8(isolate, "createContext"), create).Check();
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