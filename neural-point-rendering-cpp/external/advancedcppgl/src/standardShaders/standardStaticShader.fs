#version 430

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : enable

#include "../../src/shaderStructs.h"
#include "__shading_functions.shaderinclude"
#include "__colorspace_functions.shaderinclude"

uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;

layout(std430, binding = MATERIAL_DATA_BINDPOINT) buffer MaterialBuffer
{
	MaterialData material_data[];
};

layout(std430, binding = LIGHTING_DATA_BINDPOINT) buffer LightingDataBuffer
{
	LightingData lighting_data;
	PointLight point_lights[];
};


in Data{
	flat int matID;
	flat int dynamicModID;
	vec3 normal;
	vec2 tc;
	vec4 pM;
	vec4 pMV;
	vec4 pMVP;
};

out vec4 out_col;

void main() {

	MaterialData mat = material_data[matID];

	/////////////////////////////////////
	//alpha discard
	float alpha = 1;
	const float ALPHA_TEST_VALUE = 0.25;
	if (mat.texAlpha.x != 0)
	{
		sampler2D sa = sampler2D(mat.texAlpha);
		vec4 texAlpha = texture (sa, tc);
		//first alpha test via alpha tex
		if (length(texAlpha.xyz) < ALPHA_TEST_VALUE)
			alpha = 0;
	}

	//alpha test on diff tex
	vec4 materialDiffuseColor = vec4((mat.kDiffuse), 1);
	if (mat.texDiff.x != 0) {
		sampler2D sd = sampler2D(mat.texDiff);
		materialDiffuseColor = texture(sd, tc);
		//second alpha test via alpha value of diffuse tex
		if (materialDiffuseColor.a < ALPHA_TEST_VALUE)
			alpha = 0;
	}
	if (alpha == 0) {
		discard;
		return;
	}

	////////////////////////////////////////
	// shading

	vec3 materialSpecularColor = toLinear3(mat.kSpecular);
	if (mat.texSpec.x != 0) {
		sampler2D ss = sampler2D(mat.texSpec);
		vec4 texSpecular = texture (ss, tc);
		materialSpecularColor = texSpecular.xyz;
		materialSpecularColor = toLinear3(materialSpecularColor);
	}

	//HERE: SHADOWS

	//important: Normalize normal, rasterization interpolation does not guarantee length==1
	vec3 N = normalize(normal);

	//standard vectors
	vec3 view_dir = normalize(-pMV.xyz);
	vec3 light_dir = normalize(mat3(view)*normalize(-lighting_data.directionalLightDir));
	vec3 half_vector = normalize(.5*(view_dir + light_dir));

	vec3 diffuseColor = vec3(0);
	vec3 ambientColor = vec3(0);
	vec3 specularColor = vec3(0);

	//other values
	float dotNL = max(dot(N, light_dir), 0);
	float dotNH = max(dot(N, half_vector), 0);
	float dotVN = max(0.0, dot(normalize(view_dir), N));
	float dotLH = max(0.0, dot(light_dir, half_vector));
	vec3 lightPower = normalize(lighting_data.directionalLightColor) * vec3(lighting_data.directionalLightStrength);

	//colors with dot
	diffuseColor  += lightPower * diffuseFunc(dotNL, materialDiffuseColor.xyz);
	ambientColor  += lightPower * ambientFunc(materialDiffuseColor.xyz, dotNL);
	specularColor += lightPower * specularFunc(dotNL, dotLH, dotNH, mat.roughness, F0_fixed) * materialSpecularColor;

	//compute lighting for all point lights
	for(int i=0; i<lighting_data.numPointLights;++i){
		vec4 pos_light_view = view * vec4(point_lights[i].pointLightPos,1);
		vec3 light_dir = (pos_light_view.xyz - pMV.xyz);

		//falloff based on the length between light and shaded point
		const float falloff = 1.0 / (length(light_dir)*length(light_dir)*point_lights[i].pointLightFalloffFactor*point_lights[i].pointLightFalloffFactor);
		light_dir = normalize(light_dir);
		
		vec3 half_vector = normalize(.5*(view_dir + light_dir));

		float dotNL = max(dot(N, light_dir), 0.0);
		float dotNH = max(dot(N, half_vector), 0.0);
		float dotLH = max(0.0, dot(light_dir, half_vector));
		vec3 powerPL = point_lights[i].pointLightColor * vec3(point_lights[i].pointLightPower);

		//colors with dot
		diffuseColor += powerPL *falloff*diffuseFunc(dotNL, materialDiffuseColor.xyz);
		//ambientColor += power*falloff*ambientFunc(materialDiffuseColor.xyz, dotNL);
		specularColor += powerPL *falloff*specularFunc(dotNL, dotLH, dotNH, mat.roughness, F0_fixed)* materialSpecularColor;
	}

	//HERE: REFLECTIONS


	////////////////////////////////////////
	//composit all
	vec3 colorOut = ambientColor;
	colorOut += diffuseColor;
	colorOut += specularColor;
	colorOut *= lighting_data.luminance;

    out_col = vec4(colorOut, 1);
}
