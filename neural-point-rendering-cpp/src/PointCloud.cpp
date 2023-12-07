#pragma once
#include "PointCloud.h"
#include <iostream>
#include "../external/advancedcppgl/external/cppgl/src/platform.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include "../external/advancedcppgl/external/cppgl/src/buffer.h"


// ------------------------------------------
// helper funcs

inline uint32_t type_to_bytes(GLenum type) {
    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT:
        return 2;
    case GL_FLOAT:
    case GL_FIXED:
    case GL_INT:
    case GL_UNSIGNED_INT:
        return 4;
    case GL_DOUBLE:
        return 8;
    default:
        throw std::runtime_error("Unknown GL type!");
    }
}

// ------------------------------------------
// PointCloudImpl

PointCloudImpl::PointCloudImpl(const std::string& name, const Geometry& geometry, const Material& material)
    : name(name), geometry(geometry), material(material), vao(0), num_vertices(0), num_indices(0), primitive_type(GL_TRIANGLES) {
    glGenVertexArrays(1, &vao);
    upload_gpu();
}

PointCloudImpl::~PointCloudImpl() {
    clear_gpu();
    glDeleteVertexArrays(1, &vao);
}

void PointCloudImpl::clear_gpu() {
    ibo = IBO();
    vbos.clear();
    vbo_types.clear();
    vbo_dims.clear();
    num_vertices = num_indices = 0;
}

void PointCloudImpl::upload_gpu() {
    if (!geometry) return;
    // free gpu resources
    clear_gpu();
    // (re-)upload data to GL
    add_vertex_buffer(GL_FLOAT, 3, uint32_t(geometry->positions.size()), geometry->positions.data());
    if (geometry->has_normals())
        add_vertex_buffer(GL_FLOAT, 3, uint32_t(geometry->normals.size()), geometry->normals.data());
    if (geometry->has_texcoords())
        add_vertex_buffer(GL_FLOAT, 2, uint32_t(geometry->texcoords.size()), geometry->texcoords.data());
    add_index_buffer(uint32_t(geometry->indices.size()), geometry->indices.data());
}

void PointCloudImpl::cull(const Shader& computeShader, vec3& cam_pos, vec3& cam_dir, vec3& cam_up, float cam_near, float cam_far, float cam_fov, float cam_aspect){
    //bind the given shader
    computeShader->bind();
    //bind the command buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawCommandBuffer->originalBuffer->id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, drawCommandBuffer->frontBuffer->id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, voxelBuffer->id);
    //voxelBuffer->bind_base(0);
    computeShader->uniform("cam_pos", cam_pos);
    computeShader->uniform("cam_dir", cam_dir);
    computeShader->uniform("cam_up", cam_up);
    computeShader->uniform("cam_near", cam_near);
    computeShader->uniform("cam_far", cam_far);
    computeShader->uniform("cam_fov", cam_fov);
    computeShader->uniform("cam_aspect", cam_aspect);
    computeShader->uniform("voxel_count", int(bounding_structure.size()));
    // execute shader properly
    computeShader->dispatch_compute(bounding_structure.size(), 1, 1);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    //unbind everything
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    //voxelBuffer->unbind_base(0);
    computeShader->unbind();
    // signal that culling took place, hence indirect rendering calls should be used
    //std::cerr << "called cull" << std::endl;
    culled = true;
}

void PointCloudImpl::bind(const Shader& shader) const {
    glBindVertexArray(vao);
    if (material)
        material->bind(shader);
}

void PointCloudImpl::draw() {
    if (culled) {
        if (ibo) {
            //std::cerr << "called draw with cull" << std::endl;
            drawCommandBuffer->frontBuffer->bind();
            glMultiDrawElementsIndirect(primitive_type, GL_UNSIGNED_INT, 0, bounding_structure.size(), 0);
            drawCommandBuffer->frontBuffer->unbind();
        }
        else {
            glDrawArrays(primitive_type, 0, num_vertices);
            std::cerr << "PointCloud: Tried to use culling with unindexed rendering. not implemented yet." << std::endl;
        }
        culled = false;
    }
    else {
        if (ibo)
            glDrawElements(primitive_type, num_indices, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(primitive_type, 0, num_vertices);
    }
}

void PointCloudImpl::unbind() const {
    glBindVertexArray(0);
    if (material)
        material->unbind();
}

uint32_t PointCloudImpl::add_vertex_buffer(GLenum type, uint32_t element_dim, uint32_t num_vertices, const void* data, GLenum hint) {
    if (this->num_vertices && this->num_vertices != num_vertices)
        throw std::runtime_error("PointCloud::add_vertex_buffer: vertex buffer size mismatch!");
    // setup vbo
    this->num_vertices = num_vertices;
    
    const uint32_t buf_id = vbos.size();
    vbos.emplace_back(name + "_vertex_buffer_" + std::to_string(buf_id));
    vbos[buf_id]->upload_data(data, type_to_bytes(type) * element_dim * num_vertices, hint);
    vbo_types.push_back(type);
    vbo_dims.push_back(element_dim);
    // setup vertex attributes
    glBindVertexArray(vao);
    vbos[buf_id]->bind();;
    glEnableVertexAttribArray(buf_id);
    if (type == GL_BYTE || type == GL_UNSIGNED_BYTE ||
            type == GL_SHORT || type == GL_UNSIGNED_SHORT ||
            type == GL_INT || type == GL_UNSIGNED_INT)
        glVertexAttribIPointer(buf_id, element_dim, type, 0, 0);
    else if (type == GL_DOUBLE)
        glVertexAttribLPointer(buf_id, element_dim, type, 0, 0);
    else
        glVertexAttribPointer(buf_id, element_dim, type, GL_FALSE, 0, 0);
    glBindVertexArray(0);
    vbos[buf_id]->unbind();
    return buf_id;
}

void PointCloudImpl::add_index_buffer(uint32_t num_indices, const uint32_t* data, GLenum hint) {
    this->num_indices = num_indices;
    ibo = IBO(name + "_index_buffer");
    ibo->upload_data(data, sizeof(uint32_t) * num_indices, hint);
    // setup vao+ibo
    glBindVertexArray(vao);
    ibo->bind();
    glBindVertexArray(0);
    ibo->unbind();
}

void PointCloudImpl::update_vertex_buffer(uint32_t buf_id, const void* data) {
    if (buf_id >= vbos.size())
        throw std::runtime_error("PointCloud::update_vertex_buffer: buffer id out of range!");
    vbos[buf_id]->upload_subdata(data, 0, type_to_bytes(vbo_types[buf_id]) * vbo_dims[buf_id] * num_vertices);
}

void PointCloudImpl::set_primitive_type(GLenum primitive_type) {
    this->primitive_type = primitive_type;
}




void PointCloudImpl::add_bounding_structure(std::vector<PointCloudVoxel> & bounding_structure) {
    this->bounding_structure = bounding_structure;
    if(!drawCommandBuffer) this->drawCommandBuffer = DrawCommandBuffer(name + "_drawCommandBuffer"); // create commandbuffer if necessary
    drawCommandBuffer->clearElements();
    std::vector<DrawElementsIndirectCommand> drawCommandElements;
    for (auto& v : bounding_structure) {
        //struct DrawElementsIndirectCommand {
        //    uint count; // -> number of indices. change tzhis to 0 in shader to prevent voxel from rendering
        //    uint instanceCount; // -> should be 1? since we do not use instanced rendering?
        //    uint first;         // -> first index used 
        //    uint baseVertex;    // -> contains where the vertices start with index 0? use 0?
        //    uint baseInstance;  // -> contains where the instance id starts? use 0?
        //};
        DrawElementsIndirectCommand newCommand;
        newCommand.count = v.size;
        newCommand.instanceCount = uint(1);
        newCommand.first = v.start;
        newCommand.baseVertex = uint(0);
        newCommand.baseInstance = uint(0);
        drawCommandElements.push_back(newCommand);
    }
    drawCommandBuffer->addElements(drawCommandElements);
    voxelBuffer = SSBO(name + "_voxelBuffer" );
    voxelBuffer->resize(sizeof(PointCloudVoxel)*this->bounding_structure.size());
    //voxelBuffer->bind();
    //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PointCloudVoxel) * this->bounding_structure.size(), this->bounding_structure.data(), GL_STATIC_DRAW);
    voxelBuffer->upload_data(this->bounding_structure.data(), sizeof(PointCloudVoxel) * this->bounding_structure.size());
    //voxelBuffer->unbind();
    /*std::cerr << "voxel size: " << sizeof(PointCloudVoxel) << std::endl;
    std::cerr << "bounding structure size: " << this->bounding_structure.size() << std::endl;
    std::cerr << "buffer size : " << this->bounding_structure.size() * sizeof(PointCloudVoxel) << std::endl;*/
}



void* PointCloudImpl::map_vbo(uint32_t buf_id, GLenum access) const {
    if (buf_id >= vbos.size())
        throw std::runtime_error("PointCloud::map_vbo: buffer id out of range!");
    return vbos[buf_id]->map(access);
}

void PointCloudImpl::unmap_vbo(uint32_t buf_id) const {
    if (buf_id >= vbos.size())
        throw std::runtime_error("PointCloud::map_vbo: buffer id out of range!");
    vbos[buf_id]->unmap();
}

void* PointCloudImpl::map_ibo(GLenum access) const {
    if (!ibo)
        throw std::runtime_error("PointCloud::map_ibo: no index buffer present!");
    return ibo->map(access);
}

void PointCloudImpl::unmap_ibo() const {
    ibo->unmap();
}

// ------------------------------------------
// PointCloud loader (Ass-Imp)

std::vector<std::pair<Geometry, Material>> load_pointclouds_cpu(const fs::path& path, bool normalize) {
    // load from disk
    Assimp::Importer importer;
    std::cout << "Loading: " << path << "..." << std::endl;
    const aiScene* scene_ai = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_GenNormals);// | aiProcess_FlipUVs);
    if (!scene_ai) // handle error
        throw std::runtime_error("ERROR: Failed to load file: " + path.string() + "!");
    const std::string base_name = path.filename().replace_extension("").string();
    // load geometries
    std::vector<Geometry> geometries;
    for (uint32_t i = 0; i < scene_ai->mNumMeshes; ++i) {
        const aiMesh* ai_mesh = scene_ai->mMeshes[i];
        geometries.push_back(Geometry(base_name + "_" + ai_mesh->mName.C_Str() + "_" + std::to_string(i), ai_mesh));
    }
    // move and scale geometry to fit into [-1, 1]x3?
    if (normalize) {
        glm::vec3 bb_min(FLT_MAX), bb_max(FLT_MIN);
        for (const auto& geom : geometries) {
            bb_min = glm::min(bb_min, geom->bb_min);
            bb_max = glm::max(bb_max, geom->bb_max);
        }
        const glm::vec3 center = (bb_min + bb_max) * 0.5f;
        const glm::vec3 max = glm::vec3(1), min = glm::vec3(-1);
        const glm::vec3 scale_v = (max - min) / (bb_max - bb_min);
        const float scale_f = std::min(scale_v.x, std::min(scale_v.y, scale_v.z));
        for (auto& geom : geometries) {
            geom->translate(-center);
            geom->scale(glm::vec3(scale_f));
        }
    }
    // load materials
    std::vector<Material> materials;
    for (uint32_t i = 0; i < scene_ai->mNumMaterials; ++i) {
        aiString name_ai;
        scene_ai->mMaterials[i]->Get(AI_MATKEY_NAME, name_ai);
        materials.push_back(Material(base_name + "_" + name_ai.C_Str(), path.parent_path(), scene_ai->mMaterials[i]));
    }
    // link geometry <-> material
    std::vector<std::pair<Geometry, Material>> result;
    for (uint32_t i = 0; i < scene_ai->mNumMeshes; ++i)
        result.push_back(std::make_pair(geometries[i], materials[scene_ai->mMeshes[i]->mMaterialIndex]));
    return result;
}

std::vector<PointCloud> load_pointclouds_gpu(const fs::path& path, bool normalize) {
    // build meshes from cpu data
    std::vector<PointCloud> meshes;
    for (const auto& [geometry, material] : load_pointclouds_cpu(path, normalize))
        meshes.push_back(PointCloud(geometry->name + "/" + material->name, geometry, material));
    return meshes;
}
