#pragma once

#include "named_handle.h"
#include "mesh.h"
#include "shader.h"

// -----------------------------------------------
// Drawelement (object instance for rendering)

class DrawelementImpl {
public:
    DrawelementImpl(const std::string& name, const Shader& shader = Shader(), const Mesh& mesh = Mesh());
    virtual ~DrawelementImpl();

    void bind() const;
    void draw() const;
    void unbind() const;

    // data
    const std::string name;
    glm::mat4 model;
    Shader shader;
    Mesh mesh;
};

using Drawelement = NamedHandle<DrawelementImpl>;
template class _API NamedHandle<DrawelementImpl>; //needed for Windows DLL export
