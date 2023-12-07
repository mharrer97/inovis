#pragma once
#include "cppgl.h"
#include "platform_adv.h"
#include "shaderStructs.h"
#include <glm/glm.hpp>

class LightSystemImpl {
public: //structs

public: //variables
	std::string name;
	bool dirtyFlag = false;

	//GPU buffer for lighting information
	SSBO lightingDataBuffer;

	glm::vec3 directionalLightDir = glm::vec3(-0.5, -0.7, 0.0);
	glm::vec3 directionalLightColor = glm::vec3(1, 1, 1);
	float directionalLightStrength = 1.0f;
	float luminance = 2.f;
	
	struct PointLightManagement {
		PointLight pointLightsData;
		bool pointLightOn;
		PointLightManagement(PointLight p, bool on) :pointLightsData(p), pointLightOn(on) {}
	};
	std::vector<PointLightManagement> pointLightManagement;

public: //methods
	LightSystemImpl(std::string name);

	void bindLights();
	void unbindLights();

	void addPointLight(PointLight p);
	void removeLastPointLight();


};

using LightSystem = NamedHandle<LightSystemImpl>;
template class _API_ADV NamedHandle<LightSystemImpl>; //needed for Windows DLL export
