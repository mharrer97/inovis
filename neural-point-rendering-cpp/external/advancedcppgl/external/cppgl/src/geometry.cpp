#include <geometry.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GeometryImpl::GeometryImpl(const std::string& name) : name(name), bb_min(FLT_MAX), bb_max(FLT_MIN) {}

GeometryImpl::GeometryImpl(const std::string& name, const aiMesh* mesh_ai) : GeometryImpl(name) {
    add(mesh_ai);
}

GeometryImpl::GeometryImpl(const std::string& name, const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices,
            const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords) : GeometryImpl(name) {
    add(positions, indices, normals, texcoords);
}

GeometryImpl::~GeometryImpl() {}

void GeometryImpl::add(const aiMesh* mesh_ai) {
    // conversion helper
    const auto to_glm = [](const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); };
    // extract vertices, normals and texture coords
    positions.reserve(positions.size() + mesh_ai->mNumVertices);
    normals.reserve(normals.size() + mesh_ai->HasNormals() ? mesh_ai->mNumVertices : 0);
    texcoords.reserve(texcoords.size() + mesh_ai->HasTextureCoords(0) ? mesh_ai->mNumVertices : 0);
    for (uint32_t i = 0; i < mesh_ai->mNumVertices; ++i) {
        positions.emplace_back(to_glm(mesh_ai->mVertices[i]));
        if (mesh_ai->HasNormals())
            normals.emplace_back(to_glm(mesh_ai->mNormals[i]));
        if (mesh_ai->HasTextureCoords(0))
            texcoords.emplace_back(glm::vec2(to_glm(mesh_ai->mTextureCoords[0][i])));
        // update AABB
        bb_min = glm::min(bb_min, to_glm(mesh_ai->mVertices[i]));
        bb_max = glm::max(bb_max, to_glm(mesh_ai->mVertices[i]));
    }
    // extract faces
    indices.reserve(indices.size() + mesh_ai->mNumFaces*3);
    for (uint32_t i = 0; i < mesh_ai->mNumFaces; ++i) {
        const aiFace &face = mesh_ai->mFaces[i];
        if (face.mNumIndices == 3) {
            indices.emplace_back(face.mIndices[0]);
            indices.emplace_back(face.mIndices[1]);
            indices.emplace_back(face.mIndices[2]);
        } else
            std::cerr << "WARN: Geometry: skipping non-triangle face!" << std::endl;
    }
}

void GeometryImpl::add(const GeometryImpl& other) {
    add(other.positions, other.indices, other.normals, other.texcoords);
}

void GeometryImpl::add(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices,
        const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texcoords) {
    // add vertices, normals and texture coords
    this->positions.reserve(this->positions.size() + positions.size());
    this->normals.reserve(this->normals.size() + normals.size());
    this->texcoords.reserve(this->texcoords.size() + texcoords.size());
    for (uint32_t i = 0; i < positions.size(); ++i) {
        this->positions.emplace_back(positions[i]);
        if (i < normals.size())
            this->normals.emplace_back(normals[i]);
        if (i < texcoords.size())
            this->texcoords.emplace_back(texcoords[i]);
        // update AABB
        bb_min = glm::min(bb_min, positions[i]);
        bb_max = glm::max(bb_max, positions[i]);
    }
    // add indices
    this->indices.reserve(this->indices.size() + indices.size());
    for (uint32_t i = 0; i < indices.size(); ++i)
        this->indices.emplace_back(indices[i]);
}

void GeometryImpl::clear() {
    bb_min = bb_max = glm::vec3(0);
    positions.clear();
    indices.clear();
    normals.clear();
    texcoords.clear();
}

void GeometryImpl::recompute_aabb() {
    bb_min = glm::vec3(FLT_MAX);
    bb_max = glm::vec3(FLT_MIN);;
    for (const auto& pos : positions) {
        bb_min = glm::min(bb_min, pos);
        bb_max = glm::max(bb_max, pos);
    }
}

void GeometryImpl::fit_into_aabb(const glm::vec3& aabb_min, const glm::vec3& aabb_max) {
    // compute offset to origin and scale factor
    const glm::vec3 center = (bb_min + bb_max) * .5f;
    const glm::vec3 scale_v = (aabb_max - aabb_min) / (bb_max - bb_min);
    const float scale_f = std::min(scale_v.x, std::min(scale_v.y, scale_v.z));
    // apply
    for (uint32_t i = 0; i < positions.size(); ++i)
        positions[i] = (positions[i] - center) * scale_f;
}

void GeometryImpl::translate(const glm::vec3& by) {
    for (uint32_t i = 0; i < positions.size(); ++i)
        positions[i] += by;
}

void GeometryImpl::scale(const glm::vec3& by) {
    // scale positions
    for (uint32_t i = 0; i < positions.size(); ++i)
        positions[i] *= by;
    // scale normals
    const glm::mat4 mat_norm = glm::transpose(glm::inverse(glm::scale(glm::mat4(1), by)));
    for (uint32_t i = 0; i < normals.size(); ++i)
        normals[i] = glm::normalize(glm::vec3(mat_norm * glm::vec4(normals[i], 0)));
}

void GeometryImpl::rotate(float angle_degrees, const glm::vec3& axis) {
    // rotate positions
    const glm::mat4 rot = glm::rotate(glm::mat4(1), glm::radians(angle_degrees), axis);
    for (uint32_t i = 0; i < positions.size(); ++i)
        positions[i] = glm::vec3(rot * glm::vec4(positions[i], 1));
    // rotate normals
    const glm::mat4 rot_inv_tra = glm::transpose(glm::inverse(rot));
    for (uint32_t i = 0; i < normals.size(); ++i)
        normals[i] = glm::normalize(glm::vec3(rot_inv_tra * glm::vec4(normals[i], 0)));
}
