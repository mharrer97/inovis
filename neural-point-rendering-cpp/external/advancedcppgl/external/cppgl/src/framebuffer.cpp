#include "framebuffer.h"
#include <atomic>

FramebufferImpl::FramebufferImpl(const std::string& name, uint32_t w, uint32_t h) : name(name), id(0), w(w), h(h), prev_vp{0,0,0,0} {
    glGenFramebuffers(1, &id);
}

FramebufferImpl::~FramebufferImpl() {
    glDeleteFramebuffers(1, &id);
}

void FramebufferImpl::bind() {
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glViewport(0, 0, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glDrawBuffers(GLsizei(color_targets.size()), color_targets.data());
}

void FramebufferImpl::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
}

void FramebufferImpl::check() const {
    if (!(depth_texture && *depth_texture))
        throw std::runtime_error("ERROR: Framebuffer: depth buffer not present or invalid!");
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::string s;
        if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            s = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            s = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
            s = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
            s = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        throw std::runtime_error("ERROR: Framebuffer incomplete! Status: " + s);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferImpl::resize(uint32_t w, uint32_t h) {
    this->w = w;
    this->h = h;
    if (depth_texture)
        depth_texture->resize(w, h);
    for (auto& tex : color_textures)
        tex->resize(w, h);
}

void FramebufferImpl::attach_depthbuffer(Texture2D tex, bool with_stencil) {
    //depth-stencil combined buffers usually have two possible modes (with required attributes in order), here the first one is used
    //32 bit float depth and 8 bit stencil (GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
    //24 bit depth and 8 bit stencil (GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8)
    if (!tex) tex = Texture2D(name + "_default_depth_"+ (with_stencil ? "and_stencil_" : "" )+ "buffer", w, h,
                                with_stencil ? GL_DEPTH32F_STENCIL8              : GL_DEPTH_COMPONENT32F,
                                with_stencil ? GL_DEPTH_STENCIL                  : GL_DEPTH_COMPONENT,
                                with_stencil ? GL_FLOAT_32_UNSIGNED_INT_24_8_REV : GL_FLOAT);

    glBindFramebuffer(GL_FRAMEBUFFER, id);
    depth_texture = tex;
    glFramebufferTexture2D(GL_FRAMEBUFFER, with_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FramebufferImpl::attach_colorbuffer(const Texture2D tex) {
    const GLenum target = GL_COLOR_ATTACHMENT0 + GLenum(color_targets.size());
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, target, GL_TEXTURE_2D, tex->id, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    color_textures.push_back(tex);
    color_targets.push_back(target);
}
