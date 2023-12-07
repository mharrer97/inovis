#pragma once

#include <memory>
#include <filesystem>
namespace fs = std::filesystem;
#include <GL/glew.h>
#include <GL/gl.h>
#include "named_handle.h"

// ----------------------------------------------------
// Texture2D

class Texture2DImpl {
public:
    // construct from image on disk
    Texture2DImpl(const std::string& name, const fs::path& path, bool mipmap = true);
    // construct empty texture or from raw data
    Texture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type,
            const void* data = 0, bool mipmap = false);
    virtual ~Texture2DImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    Texture2DImpl(const Texture2DImpl&) = delete;
    Texture2DImpl& operator=(const Texture2DImpl&) = delete;
    Texture2DImpl& operator=(const Texture2DImpl&&) = delete;

    explicit inline operator bool() const  { return w > 0 && h > 0 && glIsTexture(id); }
    inline operator GLuint() const { return id; }

    // resize (discards all data!)
    void resize(uint32_t w, uint32_t h);

    // bind/unbind to/from OpenGL
    void bind(uint32_t uint) const;
    void unbind() const;
    void bind_image(uint32_t unit, GLenum access, GLenum format) const;
    void unbind_image(uint32_t unit) const;


    // save to disk
    void save_png(const fs::path& path, bool flip = true) const;
    void save_png_rgb(const fs::path& path, bool flip = true) const;
    void save_png_depth_to_bw(const fs::path& path, bool flip = true) const;
    void save_binary(const fs::path& path, bool flip = true) const;
    void save_jpg(const fs::path& path, int quality = 100, bool flip = true) const; // quality: [1, 100]

    // data
    const std::string name;
    const fs::path loaded_from_path;
    GLuint id;
    int w, h;
    GLint internal_format;
    GLenum format, type;
};

using Texture2D = NamedHandle<Texture2DImpl>;
template class _API NamedHandle<Texture2DImpl>; //needed for Windows DLL export

// ----------------------------------------------------
// Texture3D

class Texture3DImpl {
public:
    // construct empty texture or from raw data
    Texture3DImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type,
            const void *data = 0, bool mipmap = false);
    virtual ~Texture3DImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    Texture3DImpl(const Texture3DImpl&) = delete;
    Texture3DImpl& operator=(const Texture3DImpl&) = delete;
    Texture3DImpl& operator=(const Texture3DImpl&&) = delete;

    explicit inline operator bool() const  { return w > 0 && h > 0 && d > 0 && glIsTexture(id); }
    inline operator GLuint() const { return id; }

    // resize (discards all data!)
    void resize(uint32_t w, uint32_t h, uint32_t d);

    // bind/unbind to/from OpenGL
    void bind(uint32_t uint) const;
    void unbind() const;
    void bind_image(uint32_t unit, GLenum access, GLenum format) const;
    void unbind_image(uint32_t unit) const;


    // data
    const std::string name;
    GLuint id;
    int w, h, d;
    GLint internal_format;
    GLenum format, type;
};

using Texture3D = NamedHandle<Texture3DImpl>;
template class _API NamedHandle<Texture3DImpl>; //needed for Windows DLL export

