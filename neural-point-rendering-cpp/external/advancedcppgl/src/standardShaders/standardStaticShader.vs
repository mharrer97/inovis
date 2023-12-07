#version 430
#extension GL_ARB_gpu_shader_int64  : enable
#extension GL_ARB_bindless_texture : enable
#extension GL_ARB_shader_draw_parameters : enable

#include "../../src/shaderStructs.h"

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_norm;
layout(location = 2) in vec2 in_tc;

uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;

out Data{
	flat int matID;
	flat int dynamicModID;
	vec3 normal;
	vec2 tc;
	vec4 pM;
	vec4 pMV;
	vec4 pMVP;
};


layout(std430, binding = MODEL_DATA_BINDPOINT) buffer ModelDataBuffer {
	ModelData model_data[];
};

layout(std430, binding = MATERIAL_DATA_BINDPOINT) buffer MaterialBuffer
{
	MaterialData material_data[];
};

layout(std430, binding = DYNAMIC_MODEL_DATA_BINDPOINT) buffer DynamicModelDataBuffer
{
	DynamicModelData dynamic_model_data[];
};

void main() {

	//drawID is the index of the command buffer element in use (similar to an ascending Model ID, but for submodels as well)
	int drawID = gl_DrawIDARB;

	//modeldata stores the bounding box information and material index, one for every command buffer element
	ModelData modelData = model_data[drawID];
	matID = modelData.materialID;
	//modelID is the original model, so not subdivided
	dynamicModID = modelData.dynamicModelID;

	//materials is an material map (multiple elements can have the same material)
	MaterialData mat = material_data[matID];

	//in_pos in in [0,1] to make use of floating point precision,
	//rescale with: pos = bbox.min + in_pos * bbox.length
	vec4 rescaledPos = vec4(in_pos,1.0) * vec4(modelData.axisLengths, 1.0) + vec4(modelData.b_min, 0);

	vec2 rescaledTC = (in_tc * modelData.tcExtend) + modelData.tcMin;
	tc = rescaledTC;

	mat4 modelMat = dynamic_model_data[dynamicModID].modelMatrix;

	pM = modelMat * vec4(rescaledPos.xyz,1.0);
	normal = normalize(mat3(view_normal) * in_norm);
	pMV = view * pM;
    pMVP = proj * pMV;
	gl_Position = pMVP;

}
