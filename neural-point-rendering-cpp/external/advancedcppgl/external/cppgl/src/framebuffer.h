#pragma once

#include <string>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GL/gl.h>
#include "named_handle.h"
#include "texture.h"

// ------------------------------------------
// Framebuffer

class FramebufferImpl {
public:
    FramebufferImpl(const std::string& name, uint32_t w, uint32_t h);
    virtual ~FramebufferImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    FramebufferImpl(const FramebufferImpl&) = delete;
    FramebufferImpl& operator=(const FramebufferImpl&) = delete;
    FramebufferImpl& operator=(const FramebufferImpl&&) = delete;

    inline operator GLuint() const { return id; }

    void bind();
    void unbind();

    void check() const;
    void resize(uint32_t w, uint32_t h);

    //attaches a depth buffer (with optional stencil component) to the framebuffer,
    //if no valid texture is given, a default one is created
    void attach_depthbuffer(Texture2D tex = Texture2D(), bool with_stencil = false);

    //attaches a texture to the latest unused color attachment
    void attach_colorbuffer(const Texture2D tex);

    // data
    const std::string name;
    GLuint id;
    uint32_t w, h;
    std::vector<Texture2D> color_textures;
    std::vector<GLenum> color_targets;
    Texture2D depth_texture;
    GLint prev_vp[4];
};

using Framebuffer = NamedHandle<FramebufferImpl>;
template class _API NamedHandle<FramebufferImpl>; //needed for Windows DLL export
