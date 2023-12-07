#include "material.h"
#include <iostream>

MaterialImpl::MaterialImpl(const std::string& name) : name(name) {}

MaterialImpl::MaterialImpl(const std::string& name, const fs::path& base_path, const aiMaterial* mat_ai) : name(name) {
    // ambient, diffuse, specular and emissive color are handled via fallback 1x1 textures
    // parse assimp material parameters (http://assimp.sourceforge.net/lib_html/materials.html)

    // parse int values
    int int_value;
    if (mat_ai->Get(AI_MATKEY_TWOSIDED, int_value) == AI_SUCCESS)
        int_map["twosided"] = int_value;
    if (mat_ai->Get(AI_MATKEY_BLEND_FUNC, int_value) == AI_SUCCESS)
        int_map["blend_mode"] = int_value;

    // parse float values
    float float_value;
    if (mat_ai->Get(AI_MATKEY_OPACITY, float_value) == AI_SUCCESS)
        float_map["opacity"] = float_value;
    if (mat_ai->Get(AI_MATKEY_SHININESS, float_value) == AI_SUCCESS)
        float_map["roughness"] = sqrtf(2.f / (float_value + 2.f));
    if (mat_ai->Get(AI_MATKEY_REFRACTI, float_value) == AI_SUCCESS)
        float_map["ior"] = float_value;

    //parse vec3 values
    aiColor3D ai_color_value;
    if (mat_ai->Get(AI_MATKEY_COLOR_AMBIENT, ai_color_value) == AI_SUCCESS)
        vec3_map["ambient_color"] = glm::vec3(ai_color_value.r, ai_color_value.g, ai_color_value.b);
    if (mat_ai->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color_value) == AI_SUCCESS)
        vec3_map["diffuse_color"] = glm::vec3(ai_color_value.r, ai_color_value.g, ai_color_value.b);
    if (mat_ai->Get(AI_MATKEY_COLOR_SPECULAR, ai_color_value) == AI_SUCCESS)
        vec3_map["specular_color"] = glm::vec3(ai_color_value.r, ai_color_value.g, ai_color_value.b);

    // parse textures (http://assimp.sourceforge.net/lib_html/material_8h.html#a7dd415ff703a2cc53d1c22ddbbd7dde0)
    aiString name_ai;
    mat_ai->Get(AI_MATKEY_NAME, name_ai);
    aiColor3D vec3_value;

    // diffuse
    if (mat_ai->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_DIFFUSE, 0, &path_ai);
        texture_map["diffuse"] = Texture2D(name + "_diffuse_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    } else if (mat_ai->Get(AI_MATKEY_COLOR_DIFFUSE, vec3_value) == AI_SUCCESS) {
        // 1x1 fallback texture
        texture_map["diffuse"] = Texture2D(name + "_diffuse_" + name_ai.C_Str(), 1, 1, GL_RGB32F, GL_RGB, GL_FLOAT, &vec3_value.r);
    }
    // specular
    if (mat_ai->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_SPECULAR, 0, &path_ai);
        texture_map["specular"] = Texture2D(name + "_specular_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    } else if (mat_ai->Get(AI_MATKEY_COLOR_SPECULAR, vec3_value) == AI_SUCCESS) {
        // 1x1 fallback texture
        texture_map["specular"] = Texture2D(name + "_specular_" + name_ai.C_Str(), 1, 1, GL_RGB32F, GL_RGB, GL_FLOAT, &vec3_value.r);
    }
    // ambient
    if (mat_ai->GetTextureCount(aiTextureType_AMBIENT) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_AMBIENT, 0, &path_ai);
        texture_map["ambient"] = Texture2D(name + "_ambient_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    } else if (mat_ai->Get(AI_MATKEY_COLOR_AMBIENT, vec3_value) == AI_SUCCESS) {
        // 1x1 fallback texture
        texture_map["ambient"] = Texture2D(name + "_ambient_" + name_ai.C_Str(), 1, 1, GL_RGB32F, GL_RGB, GL_FLOAT, &vec3_value.r);
    }
    // emissive
    if (mat_ai->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_EMISSIVE, 0, &path_ai);
        texture_map["emissive"] = Texture2D(name + "_emissive_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    } else if (mat_ai->Get(AI_MATKEY_COLOR_EMISSIVE, vec3_value) == AI_SUCCESS) {
        // 1x1 fallback texture
        texture_map["emissive"] = Texture2D(name + "_emissive_" + name_ai.C_Str(), 1, 1, GL_RGB32F, GL_RGB, GL_FLOAT, &vec3_value.r);
    }

    // heightmap / normalmap (obj: map_Bump somehow is aiTextureType_HEIGHT, not aiTextureType_NORMALS) 
    if (mat_ai->GetTextureCount(aiTextureType_HEIGHT) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_HEIGHT, 0, &path_ai);
        texture_map["normalmap"] = Texture2D(name + "_normal_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    }
    // alphamap 
    if (mat_ai->GetTextureCount(aiTextureType_OPACITY) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_OPACITY, 0, &path_ai);
        texture_map["alphamap"] = Texture2D(name + "_alpha_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    }
    // roughness texture
    if (mat_ai->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_SHININESS, 0, &path_ai);
        texture_map["roughness"] = Texture2D(name + "_roughness_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    }
    // displacement map
    if (mat_ai->GetTextureCount(aiTextureType_DISPLACEMENT) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_DISPLACEMENT, 0, &path_ai);
        texture_map["displacement"] = Texture2D(name + "_displacement_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    }
    // lightmap (baked AO or something)
    if (mat_ai->GetTextureCount(aiTextureType_LIGHTMAP) > 0) {
        aiString path_ai;
        mat_ai->GetTexture(aiTextureType_LIGHTMAP, 0, &path_ai);
        texture_map["lightmap"] = Texture2D(name + "_light_" + name_ai.C_Str(), base_path / path_ai.C_Str());
    }
    // whatever
    if (mat_ai->GetTextureCount(aiTextureType_UNKNOWN) > 0)
        std::cerr << "Warn: material <" << name << "> has unknown texture: " << name_ai.C_Str() << std::endl;
}

MaterialImpl::~MaterialImpl() {}

void MaterialImpl::bind(const Shader& shader) const {
    // bind parameters as uniforms
    for (const auto& entry : int_map)
        shader->uniform(entry.first, entry.second);
    for (const auto& entry : float_map)
        shader->uniform(entry.first, entry.second);
    for (const auto& entry : vec2_map)
        shader->uniform(entry.first, entry.second);
    for (const auto& entry : vec3_map)
        shader->uniform(entry.first, entry.second);
    for (const auto& entry : vec4_map)
        shader->uniform(entry.first, entry.second);
    // bind textures as sampler2Ds
    uint32_t unit = 0;
    for (const auto& entry : texture_map)
        shader->uniform(entry.first, entry.second, unit++);
}

void MaterialImpl::unbind() const {
    // unbind textures
    for (const auto& entry : texture_map)
        entry.second->unbind();
}

