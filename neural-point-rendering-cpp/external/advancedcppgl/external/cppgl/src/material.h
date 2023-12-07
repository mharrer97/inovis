#pragma once

#include <map>
#include <string>
#include <memory>
#include <filesystem>
namespace fs = std::filesystem;
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <assimp/material.h>
#include "named_handle.h"
#include "shader.h"
#include "texture.h"

// ------------------------------------------
// Material

class MaterialImpl {
public:
    MaterialImpl(const std::string& name);
    MaterialImpl(const std::string& name, const fs::path& base_path, const aiMaterial* mat_ai);
    virtual ~MaterialImpl();

    void bind(const Shader& shader) const;
    void unbind() const;

    inline bool has_texture(const std::string& uniform_name) const { return texture_map.count(uniform_name); }
    inline Texture2D get_texture(const std::string& uniform_name) const { return texture_map.at(uniform_name); }
    inline void add_texture(const std::string& uniform_name, const Texture2D texture) { texture_map[uniform_name] = texture; }

    // data
    const std::string name;
    std::map<std::string, int> int_map;
    std::map<std::string, float> float_map;
    std::map<std::string, glm::vec2> vec2_map;
    std::map<std::string, glm::vec3> vec3_map;
    std::map<std::string, glm::vec4> vec4_map;
    std::map<std::string, Texture2D> texture_map;
};

using Material = NamedHandle<MaterialImpl>;
template class _API NamedHandle<MaterialImpl>; //needed for Windows DLL export
