#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "platform.h"
#include <assimp/mesh.h>
#include "named_handle.h"

// ------------------------------------------
// Geometry

class GeometryImpl {
public:
    GeometryImpl(const std::string& name);
    GeometryImpl(const std::string& name, const aiMesh* mesh_ai);
    GeometryImpl(const std::string& name, const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices,
            const std::vector<glm::vec3>& normals = std::vector<glm::vec3>(), const std::vector<glm::vec2>& texcoords = std::vector<glm::vec2>());
    virtual ~GeometryImpl();

    explicit inline operator bool() const  { return positions.size() > 0 && indices.size() > 0; }

    void add(const aiMesh* mesh_ai);
    void add(const GeometryImpl& other);
    void add(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices,
            const std::vector<glm::vec3>& normals = std::vector<glm::vec3>(), const std::vector<glm::vec2>& texcoords = std::vector<glm::vec2>());
    void clear();

    inline bool has_normals() const { return !normals.empty(); }
    inline bool has_texcoords() const { return !texcoords.empty(); }

    // O(n) geometry operations
    void recompute_aabb();
    void fit_into_aabb(const glm::vec3& aabb_min, const glm::vec3& aabb_max);
    void translate(const glm::vec3& by);
    void scale(const glm::vec3& by);
    void rotate(float angle_degrees, const glm::vec3& axis);

    // data
    const std::string name;
    glm::vec3 bb_min, bb_max;
    std::vector<glm::vec3> positions;
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
};

using Geometry = NamedHandle<GeometryImpl>;
template class _API NamedHandle<GeometryImpl>; //needed for Windows DLL export
