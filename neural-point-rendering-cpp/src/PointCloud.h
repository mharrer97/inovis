#pragma once

#include <string>
#include <memory>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include "../external/advancedcppgl/external/cppgl/src/buffer.h"
#include "../external/advancedcppgl/external/cppgl/src/geometry.h"
#include "../external/advancedcppgl/external/cppgl/src/material.h"
#include "../external/advancedcppgl/src/commandBuffer.h"

#include "PointCloudData.h"

// ------------------------------------------
// PointCloud
class PointCloudImpl {
public:
    PointCloudImpl(const std::string& name, const Geometry& geometry = Geometry(), const Material& material = Material());
    virtual ~PointCloudImpl();

    // prevent copies and moves, since GL buffers aren't reference counted
    PointCloudImpl(const PointCloudImpl&) = delete;
    PointCloudImpl& operator=(const PointCloudImpl&) = delete;
    PointCloudImpl& operator=(const PointCloudImpl&&) = delete;

    void clear_gpu(); // free gpu resources
    void upload_gpu(); // cpu -> gpu transfer

    // call in this order to draw
    void cull(const Shader& computeShader, vec3& cam_pos, vec3& cam_dir, vec3& cam_up, float cam_near, float cam_far, float cam_fov, float cam_aspect);// optional to use culling. only use after add_bounding_structure
    void bind(const Shader& shader) const;
    void draw();
    void unbind() const;

    // GL vertex and index buffer operations
    uint32_t add_vertex_buffer(GLenum type, uint32_t element_dim, uint32_t num_vertices, const void* data, GLenum hint = GL_STATIC_DRAW);
    void add_index_buffer(uint32_t num_indices, const uint32_t* data, GLenum hint = GL_STATIC_DRAW);
    void update_vertex_buffer(uint32_t buf_id, const void* data); // assumes matching size for buffer buf_id from add_vertex_buffer()
    void set_primitive_type(GLenum type); // default: GL_TRIANGLES
    void add_bounding_structure(std::vector<PointCloudVoxel> & bounding_structure); // enables culling w.r.t. the given bounding structure

    // map/unmap from GPU mem (https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-AsynchronousBufferTransfers.pdf)
    void* map_vbo(uint32_t buf_id, GLenum access = GL_READ_WRITE) const;
    void unmap_vbo(uint32_t buf_id) const;
    void* map_ibo(GLenum access = GL_READ_WRITE) const;
    void unmap_ibo() const;

    // CPU data
    const std::string name;
    Geometry geometry;
    Material material;
    bool culled = false; // contains if the point cloud used culling in this frame, set true by cull() and checked by draw to choose rendering call
    DrawCommandBuffer drawCommandBuffer;
    SSBO voxelBuffer;
    std::vector<PointCloudVoxel> bounding_structure;
    // GPU data
    GLuint vao;
    IBO ibo;
    uint32_t num_vertices;
    uint32_t num_indices;
    std::vector<VBO> vbos;
    std::vector<GLenum> vbo_types;
    std::vector<uint32_t> vbo_dims;
    GLenum primitive_type;
};

using PointCloud = NamedHandle<PointCloudImpl>;
template class NamedHandle<PointCloudImpl>; 

// ------------------------------------------
// PointCloud loader (Ass-Imp)

std::vector<std::pair<Geometry, Material>> load_pointclouds_cpu(const fs::path& path, bool normalize = false);
std::vector<PointCloud> load_pointclouds_gpu(const fs::path& path, bool normalize = false);
