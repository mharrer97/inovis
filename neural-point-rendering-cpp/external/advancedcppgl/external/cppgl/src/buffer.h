#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include "named_handle.h"

// ----------------------------------------------------
// Generic GLBufferImpl

template <GLenum GL_TEMPLATE_BUFFER> class GLBufferImpl {
public:
    GLBufferImpl(const std::string& name, size_t size_bytes = 0) : name(name) {
        glGenBuffers(1, &id);
        resize(size_bytes);
    }
    virtual ~GLBufferImpl() {
        glDeleteBuffers(1, &id);
    }

    // prevent copies and moves, since GL buffers aren't reference counted
    GLBufferImpl(const GLBufferImpl&) = delete;
    GLBufferImpl& operator=(const GLBufferImpl&) = delete;
    GLBufferImpl& operator=(const GLBufferImpl&&) = delete;

    explicit inline operator bool() const { return glIsBuffer(id) && size_bytes > 0; }
    inline operator GLuint() const { return id; }

    // bind/unbind to/from OpenGL
    void bind() const {
        glBindBuffer(GL_TEMPLATE_BUFFER, id);
    }
    void unbind() const {
        glBindBuffer(GL_TEMPLATE_BUFFER, 0);
    }
    void bind_base(uint32_t unit) const {
        glBindBufferBase(GL_TEMPLATE_BUFFER, unit, id);
    }
    void unbind_base(uint32_t unit) const {
        glBindBufferBase(GL_TEMPLATE_BUFFER, unit, 0);
    }

    // directly upload data (discards and reallocates memory, slow!)
    void upload_data(const void* data, size_t size_bytes, GLenum hint = GL_DYNAMIC_DRAW) {
        bind();
        this->size_bytes = size_bytes;
        glBufferData(GL_TEMPLATE_BUFFER, size_bytes, data, hint);
        unbind();
    }
    // directly upload data (overwrites memory with no bounds checking, slow-ish)
    void upload_subdata(const void* data, size_t offset_bytes, size_t size_bytes) {
        bind();
        glBufferSubData(GL_TEMPLATE_BUFFER, offset_bytes, size_bytes, data);
        unbind();
    }
    // resize (discards all data!)
    void resize(size_t size_bytes, GLenum hint = GL_DYNAMIC_DRAW) {
        upload_data(0, size_bytes, hint);
    }
    // clear to 0x0
    void clear() {
        bind();
        const GLuint zero = 0;
        glClearBufferData(GL_TEMPLATE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
        unbind();
    }

    // map/unmap from GPU mem for faster memory transfer
    // https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-AsynchronousBufferTransfers.pdf
    void* map(GLenum access = GL_READ_WRITE) const {
        bind();
        return glMapBuffer(GL_TEMPLATE_BUFFER, access);
    }
    void unmap() const {
        glUnmapBuffer(GL_TEMPLATE_BUFFER);
        unbind();
    }


    // data
    const std::string name;
    GLuint id;
    size_t size_bytes;
};

// ----------------------------------------------------
// Specialized GL buffers

using VBO = NamedHandle<GLBufferImpl<GL_ARRAY_BUFFER>>;
using IBO = NamedHandle<GLBufferImpl<GL_ELEMENT_ARRAY_BUFFER>>;
using UBO = NamedHandle<GLBufferImpl<GL_UNIFORM_BUFFER>>;
using SSBO = NamedHandle<GLBufferImpl<GL_SHADER_STORAGE_BUFFER>>;
using TBO = NamedHandle<GLBufferImpl<GL_TEXTURE_BUFFER>>;
using QBO = NamedHandle<GLBufferImpl<GL_QUERY_BUFFER>>;
using ACBO = NamedHandle<GLBufferImpl<GL_ATOMIC_COUNTER_BUFFER>>;
using DIBO = NamedHandle<GLBufferImpl<GL_DRAW_INDIRECT_BUFFER>>;
using CIBO = NamedHandle<GLBufferImpl<GL_DISPATCH_INDIRECT_BUFFER>>;
using TFBO = NamedHandle<GLBufferImpl<GL_TRANSFORM_FEEDBACK_BUFFER>>;
using PUBO = NamedHandle<GLBufferImpl<GL_PIXEL_UNPACK_BUFFER>>;
using PPBO = NamedHandle<GLBufferImpl<GL_PIXEL_PACK_BUFFER>>;
using CRBO = NamedHandle<GLBufferImpl<GL_COPY_READ_BUFFER>>;
using CWBO = NamedHandle<GLBufferImpl<GL_COPY_WRITE_BUFFER>>;

// explicit instanciation needed for Windows DLL export
template class GLBufferImpl<GL_ARRAY_BUFFER>;
template class GLBufferImpl<GL_ELEMENT_ARRAY_BUFFER>;
template class GLBufferImpl<GL_UNIFORM_BUFFER>;
template class GLBufferImpl<GL_SHADER_STORAGE_BUFFER>;
template class GLBufferImpl<GL_TEXTURE_BUFFER>;
template class GLBufferImpl<GL_QUERY_BUFFER>;
template class GLBufferImpl<GL_ATOMIC_COUNTER_BUFFER>;
template class GLBufferImpl<GL_DRAW_INDIRECT_BUFFER>;
template class GLBufferImpl<GL_DISPATCH_INDIRECT_BUFFER>;
template class GLBufferImpl<GL_TRANSFORM_FEEDBACK_BUFFER>;
template class GLBufferImpl<GL_PIXEL_UNPACK_BUFFER>;
template class GLBufferImpl<GL_PIXEL_PACK_BUFFER>;
template class GLBufferImpl<GL_COPY_READ_BUFFER>;
template class GLBufferImpl<GL_COPY_WRITE_BUFFER>;

//needed for Windows DLL export 
template class _API NamedHandle<GLBufferImpl<GL_ARRAY_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_ELEMENT_ARRAY_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_UNIFORM_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_SHADER_STORAGE_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_TEXTURE_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_QUERY_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_ATOMIC_COUNTER_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_DRAW_INDIRECT_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_DISPATCH_INDIRECT_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_TRANSFORM_FEEDBACK_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_PIXEL_UNPACK_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_PIXEL_PACK_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_COPY_READ_BUFFER>>;
template class _API NamedHandle<GLBufferImpl<GL_COPY_WRITE_BUFFER>>;
