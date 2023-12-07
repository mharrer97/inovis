#pragma once 

#include <cppgl.h>
#include <iostream>
#include <filesystem>

#include "sceneComponent.h"
#include "platform_adv.h"
#include "lightSystem.h"

// ------------------------------------------
// Scene
//
// The scene is a container for \superModels\
// used as a combined access and management structure for gpu and cpu geometry
// using these \superModels\, which contain all geometry for one render pipeline 
// (e.g. static or transparent objects) and a uber shader reduces context switches
// to one switch per pipeline and allows draw-call manipulation on the gpu


class SceneImpl {
public: //structs
	enum SceneComponentKind {
		staticElements = 0,
	//	transparentElements,
		NUM_PIPELINES
	};
public: //data
	std::string name;
	std::map<std::string, SceneComponent> superModelMap;

	std::string superModelStr[NUM_PIPELINES] = { std::string("static") };// , std::string("transparent")};

	std::string standardShaderNames[NUM_PIPELINES] = { std::string("standardStaticShader") };// , std::string("asdf") };

	LightSystem lightSystem;

public: 
	SceneImpl(std::string name);
	~SceneImpl() {}

	//add models to the scene, assign on predefined variables to the pipeline {static, transparent etc}, use_binary_dump stores and uses binary dumps for faster loading
	void load_model_and_assign_automatically(const fs::path& path, bool normalized_model = false, bool use_binary_dump = false);
	
	//standard pipeline of rendering with predefined shaders, \supermodel_kind\ is an enum to idenify the pipeline {static, transparent etc}
	void bindSupermodel(SceneComponentKind supermodel_kind);
	//standard pipeline of rendering with predefined shaders, \supermodel_kind\ is an enum to idenify the pipeline {static, transparent etc}
	void drawSupermodel(SceneComponentKind supermodel_kind);
	//standard pipeline of rendering with predefined shaders, \supermodel_kind\ is an enum to idenify the pipeline {static, transparent etc}
	void unbindSupermodel(SceneComponentKind supermodel_kind);

	//specific shader, needs to use standard rendering layout, use only if you know what you are doing
	void bindSupermodel(SceneComponentKind supermodel_kind, const Shader& shader);
	//specific shader, needs to use standard rendering layout, use only if you know what you are doing
	void drawSupermodel(SceneComponentKind supermodel_kind, const Shader& shader);
	//specific shader, needs to use standard rendering layout, use only if you know what you are doing
	void unbindSupermodel(SceneComponentKind supermodel_kind, const Shader& shader);

	// prevent copies and moves, since GL buffers aren't reference counted
	SceneImpl(const SceneImpl&) = delete;
	SceneImpl& operator=(const SceneImpl&) = delete;
	SceneImpl& operator=(const SceneImpl&&) = delete;
};

using Scene = NamedHandle<SceneImpl>;
template class _API_ADV NamedHandle<SceneImpl>; //needed for Windows DLL export

