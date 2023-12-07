#include "scene.h"
#include <cassert>
#include "cppgl.h"
#include "modelLoader.h"
#include "gui_adv.h"


SceneImpl::SceneImpl(std::string name): name(name), lightSystem(name+ "_lightSystem"){
	superModelMap[superModelStr[staticElements]] = SceneComponent(superModelStr[staticElements]);

	for (auto shaderName : standardShaderNames) {
		if (!Shader::valid(shaderName)) {
			// COMPILED_STANDARD_SHADER_PATH is compiled in, via cmake macro, to be the subdir of advcppgl/src
			std::string file_path = COMPILED_STANDARD_SHADER_PATH + std::string("/") + shaderName;
			Shader(shaderName, file_path +  ".vs", file_path + ".fs");
		}
	}

	gui_add_callback("supermodels", gui_display_supermodels);
	gui_add_callback("lighting", gui_display_lighting);

	//////DEBUG
	//PointLight t;
	//t.pointLightPos = glm::vec3(5.f, 5.f, 5.f);
	//t.pointLightColor = glm::vec3(1.f, 0.f, 0.f);
	//t.pointLightPower = 10.f;
	//t.pointLightFalloffFactor = 0.5f;
	//
	//lightSystem->addPointLight(t);

}


void SceneImpl::load_model_and_assign_automatically(const fs::path& path, bool normalized_model, bool use_binary_dump) {

	std::vector<std::pair<Geometry, Material>> loaded_data = ModelLoader::loadModel(path, normalized_model, use_binary_dump);

	std::string modelKind = superModelStr[staticElements];

	//add to supermodel
	for (auto& p : loaded_data) {
		superModelMap[modelKind]->add_geometry(p.first);
		superModelMap[modelKind]->add_material(p.second);
		superModelMap[modelKind]->dirtyFlag = true;

	}
}



void SceneImpl::bindSupermodel(SceneComponentKind supermodel_kind){
	assert(superModelMap[superModelStr[supermodel_kind]]);
	Shader shader = Shader::find(standardShaderNames[supermodel_kind]);
	bindSupermodel(supermodel_kind, shader);
}

void SceneImpl::drawSupermodel(SceneComponentKind supermodel_kind){
	assert(superModelMap[superModelStr[supermodel_kind]]);
	Shader shader = Shader::find(standardShaderNames[supermodel_kind]);
	drawSupermodel(supermodel_kind, shader);
}

void SceneImpl::unbindSupermodel(SceneComponentKind supermodel_kind){
	assert(superModelMap[superModelStr[supermodel_kind]]);
	Shader shader = Shader::find(standardShaderNames[supermodel_kind]);
	unbindSupermodel(supermodel_kind, shader);
}

void SceneImpl::bindSupermodel(SceneComponentKind supermodel_kind, const Shader& shader) {
	assert(superModelMap[superModelStr[supermodel_kind]]);

	lightSystem->bindLights();

	superModelMap[superModelStr[supermodel_kind]]->bind(shader);
}

void SceneImpl::drawSupermodel(SceneComponentKind supermodel_kind, const Shader& shader){
	assert(superModelMap[superModelStr[supermodel_kind]]);
	superModelMap[superModelStr[supermodel_kind]]->draw(shader);
}
void SceneImpl::unbindSupermodel(SceneComponentKind supermodel_kind, const Shader& shader){
	assert(superModelMap[superModelStr[supermodel_kind]]);

	superModelMap[superModelStr[supermodel_kind]]->unbind(shader);

	lightSystem->unbindLights();
}

