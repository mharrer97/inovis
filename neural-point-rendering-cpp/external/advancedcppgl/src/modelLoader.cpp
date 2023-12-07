#include "modelLoader.h"
#include <filesystem>
#include "serializationHelper.h"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <assimp/material.h>

std::vector<std::pair<Geometry, Material>> ModelLoader::load_meshes(const fs::path& path, bool normalize) {
    // load from disk
    Assimp::Importer importer;
    std::cout << "Loading: " << path << "..." << std::endl;
    const aiScene* scene_ai = importer.ReadFile(path.string(),
        0 | aiProcess_GenNormals 	// compute normals if none are defined ; aiProcess_GenSmoothNormals
        | aiProcess_Triangulate 			// triangulate quad or polygon faces
        | aiProcess_JoinIdenticalVertices		// assimp does not load shared vertices by default
        | aiProcess_SplitLargeMeshes
        | aiProcess_FindDegenerates

        | aiProcess_RemoveRedundantMaterials
        | aiProcess_FindInvalidData
        | aiProcess_ImproveCacheLocality);

    //     //   | aiProcess_CalcTangentSpace
    //

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



std::vector<std::pair<Geometry, Material>> ModelLoader::loadModel(const fs::path& path, bool normalized_model,bool use_binary_dump) {
	if (!use_binary_dump) {
        auto loaded_scene = load_meshes(path, normalized_model);
        assert((loaded_scene.size() > 0, "SCENE NOT PROPERLY LOADED"));
		return loaded_scene;
	}

	fs::path filename = path.filename();
	fs::path binary_dump_file = path;
	binary_dump_file = binary_dump_file.remove_filename() / ".." / filename.replace_extension("") ;
	binary_dump_file += fs::path(".binaryDump");

	std::ifstream myfile = std::ifstream(binary_dump_file.string().c_str(), std::ios::binary);

	std::vector<std::pair<Geometry, Material>> result;
	if (!myfile.is_open()) {
		std::cout << "LOADING MESH: " << filename << std::endl;
		result = load_meshes_cpu(path, normalized_model);
		std::cout << "BINARY STORING MESH: " << filename << " in " << binary_dump_file << std::endl;
		//myfile.open(binary_dump_file.string().c_str(), std::fstream::out | std::ios::binary);
		std::ofstream myOfile = std::ofstream(binary_dump_file.string().c_str(), std::ios::binary);
		storeBinary(myOfile, result);
		//result = test(binary_dump_file.string(), result);
	}
	else {
		std::cout << "LOADING DUMPED MESH: " << filename << std::endl;
		result = loadBinary(myfile);
	}

	return result;
}


std::vector<std::pair<Geometry, Material>> ModelLoader::loadBinary(std::ifstream& binary_file) {
	std::vector<std::pair<Geometry, Material>> result;
	{
		cereal::PortableBinaryInputArchive iarchive(binary_file);
		iarchive(result);
	}
	return result;

}



void ModelLoader::storeBinary(std::ofstream& binary_file, std::vector<std::pair<Geometry, Material>>& model) {
	{
		cereal::PortableBinaryOutputArchive oarchive(binary_file);
		oarchive(model);
	}
}
