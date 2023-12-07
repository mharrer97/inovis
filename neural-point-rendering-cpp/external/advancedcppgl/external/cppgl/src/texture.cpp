#include "texture.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ----------------------------------------------------
// helper funcs

inline uint32_t format_to_channels(GLint format) {
    return format == GL_RGBA ? 4 : format == GL_RGB ? 3 : format == GL_RG ? 2 : 1;
}
inline GLint channels_to_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : channels == 2 ? GL_RG : GL_RED;
}
inline GLint channels_to_float_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
}
inline GLint channels_to_ubyte_format(uint32_t channels) {
    return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
}

// ----------------------------------------------------
// Texture2D

Texture2DImpl::Texture2DImpl(const std::string& name, const fs::path& path, bool mipmap) : name(name), loaded_from_path(path), id(0) {
    // load image from disk
    stbi_set_flip_vertically_on_load(1);
    int channels;
    uint8_t* data = 0;
    if (stbi_is_hdr(path.string().c_str())) {
        data = (uint8_t*)stbi_loadf(path.string().c_str(), &w, &h, &channels, 0);
        internal_format = channels_to_float_format(channels);
        format = channels_to_format(channels);
        type = GL_FLOAT;
    } else {
        data = stbi_load(path.string().c_str(), &w, &h, &channels, 0);
        internal_format = channels_to_ubyte_format(channels);
        format = channels_to_format(channels);
        type = GL_UNSIGNED_BYTE;
    }
    if (!data) {
        throw std::runtime_error("Failed to load image file: " + path.string());
        return;
    }

    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    //opengl by default needs 4 byte alignment after every row
    //stbi loaded data is not aligned that way -> pixelStore attributes need to be set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, &data[0]);
    if (mipmap) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // free data
    stbi_image_free(data);
}

Texture2DImpl::Texture2DImpl(const std::string& name, uint32_t w, uint32_t h, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
    : name(name), id(0), w(w), h(h), internal_format(internal_format), format(format), type(type) {
    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL) ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
            mipmap ? GL_LINEAR_MIPMAP_LINEAR : (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL) ? GL_NEAREST : GL_LINEAR);
    // if interpolation should be disabled,use following code
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, data);
    if (mipmap && data != 0) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

}

Texture2DImpl::~Texture2DImpl() {
    if (glIsTexture(id))
        glDeleteTextures(1, &id);
}

void Texture2DImpl::resize(uint32_t w, uint32_t h) {
    this->w = w;
    this->h = h;
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, w, h, 0, format, type, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2DImpl::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture2DImpl::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2DImpl::bind_image(uint32_t unit, GLenum access, GLenum format) const {
    glBindImageTexture(unit, id, 0, GL_FALSE, 0, access, format);
}

void Texture2DImpl::unbind_image(uint32_t unit) const {
    glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}

void Texture2DImpl::save_png(const fs::path& path, bool flip) const {
    stbi_flip_vertically_on_write(flip);
    std::vector<uint8_t> pixels(w * h * format_to_channels(format));
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_write_png(path.string().c_str(), w, h, format_to_channels(format), pixels.data(), 0);
    //std::cout << path << " written." << std::endl;
}

void Texture2DImpl::save_png_rgb(const fs::path& path, bool flip) const {
    stbi_flip_vertically_on_write(flip);
    std::vector<uint8_t> pixels(w * h * format_to_channels(format));
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (int i = 0; i < w * h; ++i) {
        pixels[i * format_to_channels(format) + 3] = 255;
    }
    stbi_write_png(path.string().c_str(), w, h, format_to_channels(format), pixels.data(), 0);
    //std::cout << path << " written." << std::endl;
}

void Texture2DImpl::save_png_depth_to_bw(const fs::path& path, bool flip) const {
    stbi_flip_vertically_on_write(flip);
    std::vector<uint8_t> pixels(w * h * format_to_channels(format));
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    for (long unsigned int i = 0; i < pixels.size(); i += format_to_channels(format)) {
        pixels[i] = pixels[i + 3];
        pixels[i+1] = pixels[i + 3];
        pixels[i+2] = pixels[i + 3];
    }
    stbi_write_png(path.string().c_str(), w, h, format_to_channels(format), pixels.data(), 0);
    //std::cout << path << " written." << std::endl;
}

void Texture2DImpl::save_binary(const fs::path& path, bool flip) const {
    stbi_flip_vertically_on_write(flip);
    std::vector<int> meta{ w,h,3,sizeof(float),0 };
    std::vector<float> pixels(w * h * format_to_channels(format));
    // target format is 3 channels
    std::vector<float> pixels_reduced(w * h * 3);

    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    for (int i = 0; i < w*h; ++i) {
        for (unsigned int j = 0; j < std::max(format_to_channels(format), uint32_t(3)); ++j) {
            pixels_reduced[i * 3 + j] = pixels[i * format_to_channels(format) + j];
        }
    }
    
    std::string path_str;
    if (path.extension() == ".bin") {
        path_str = path.string();
    }
    else {
        std::cerr << "WARN: Context::screenshot: unknown extension, adding .bin!" << std::endl;
        path_str = (path.string() + std::string(".bin")).c_str();
    }
    
    std::ofstream out_stream(path_str.c_str(), std::ios::binary);
    out_stream.write(reinterpret_cast<char*>(&meta[0]), sizeof(int) * meta.size()); // write out metadata as int

    //-------------------------------------------
    // Flip image vertically
    std::vector<float> pixels_flipped(w * h * 3);
    for (int row = h - 1; row >= 0; --row) {
        for (int i = 0; i < w * 3; ++i) {
            pixels_flipped[(h - 1 - row) * w * 3 + i] = pixels_reduced[(row * w * 3 + i)];
        }
    }

    out_stream.write(reinterpret_cast<char*>(&pixels_flipped[0]), sizeof(float) * pixels_flipped.size()); // write out pixels as floats
  
}

void Texture2DImpl::save_jpg(const fs::path& path, int quality, bool flip) const {
    stbi_flip_vertically_on_write(flip);
    std::vector<uint8_t> pixels(w * h * format_to_channels(format));
    glBindTexture(GL_TEXTURE_2D, id);
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, &pixels[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_write_jpg(path.string().c_str(), w, h, format_to_channels(format), pixels.data(), quality);
    std::cout << path << " written." << std::endl;
}

// ----------------------------------------------------
// Texture3D

Texture3DImpl::Texture3DImpl(const std::string& name, uint32_t w, uint32_t h, uint32_t d, GLint internal_format, GLenum format, GLenum type, const void* data, bool mipmap)
    : name(name), id(0), w(w), h(h), d(d), internal_format(internal_format), format(format), type(type) {
    // init GL texture
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_3D, id);
    // default border color is (0, 0, 0, 0)
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, w, h, d, 0, format, type, data);
    if (mipmap && data != 0) glGenerateMipmap(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, 0);
}

Texture3DImpl::~Texture3DImpl() {
    if (glIsTexture(id))
        glDeleteTextures(1, &id);
}

void Texture3DImpl::resize(uint32_t w, uint32_t h, uint32_t d) {
    this->w = w;
    this->h = h;
    this->d = d;
    glBindTexture(GL_TEXTURE_3D, id);
    glTexImage2D(GL_TEXTURE_3D, 0, internal_format, w, h, 0, format, type, 0);
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3DImpl::bind(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, id);
}

void Texture3DImpl::unbind() const {
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3DImpl::bind_image(uint32_t unit, GLenum access, GLenum format) const {
    glBindImageTexture(unit, id, 0, GL_FALSE, 0, access, format);
}

void Texture3DImpl::unbind_image(uint32_t unit) const {
    glBindImageTexture(unit, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}
