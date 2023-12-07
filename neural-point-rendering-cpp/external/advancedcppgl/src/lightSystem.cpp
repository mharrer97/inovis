#include "lightSystem.h"
#include "glm/gtx/string_cast.hpp"

LightSystemImpl::LightSystemImpl(std::string name) : name(name), lightingDataBuffer(name + "_lightingDataBuffer") {
	dirtyFlag = true;

}

void LightSystemImpl::addPointLight(PointLight p) {
	pointLightManagement.emplace_back(p, true);
	dirtyFlag = true;
}

void LightSystemImpl::removeLastPointLight() {
	if (pointLightManagement.size() <= 0)
		return;

	pointLightManagement.pop_back(); 
	dirtyFlag = true; 
}


void LightSystemImpl::bindLights() {

	//reupload only if something changed
	if (dirtyFlag) {

		//buffer always allocates space for all pointLights, even if they are off
		if (lightingDataBuffer->size_bytes != (sizeof(LightingData) + sizeof(PointLight) * pointLightManagement.size()))
			lightingDataBuffer->resize(sizeof(LightingData) + sizeof(PointLight) * pointLightManagement.size());

		LightingData lightingData;
		lightingData.directionalLightDir = directionalLightDir;
		lightingData.directionalLightColor = directionalLightColor;
		lightingData.luminance = luminance;
		lightingData.directionalLightStrength = directionalLightStrength;
		lightingData.numPointLights = 0;

		lightingDataBuffer->bind();
		GLbyte* lightsBufferGPU = (GLbyte*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, lightingDataBuffer->size_bytes, GL_MAP_WRITE_BIT);
		PointLight* pointLightGPUpointer =(PointLight*) (lightsBufferGPU + sizeof(LightingData));

		//only pointLights that are on are uploaded
		for (auto& plm : pointLightManagement) {
			if (plm.pointLightOn) {
				pointLightGPUpointer[lightingData.numPointLights] = plm.pointLightsData;
				lightingData.numPointLights += 1;
			}
		}

		//afterwards: fill the first X bytes with directional light data
		memcpy(lightsBufferGPU, &lightingData, sizeof(LightingData));

		lightingDataBuffer->unmap();
		dirtyFlag = false;
	}

	lightingDataBuffer->bind_base(LIGHTING_DATA_BINDPOINT);

}

void LightSystemImpl::unbindLights() {

	lightingDataBuffer->unbind_base(LIGHTING_DATA_BINDPOINT);
}