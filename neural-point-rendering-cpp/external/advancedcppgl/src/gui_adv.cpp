#include "gui_adv.h"
#include <cppgl.h>
#include "sceneComponent.h"
#include "scene.h"

void gui_display_supermodels() {
	static bool showSceneComponentData = false;

	if (ImGui::BeginMainMenuBar()) {
		ImGui::Separator();
		ImGui::Checkbox("supermodels", &showSceneComponentData);
		ImGui::EndMainMenuBar();
	}
	if (showSceneComponentData) {
		if (ImGui::Begin(std::string("SceneComponent (" + std::to_string(SceneComponent::map.size()) + ")").c_str(), &showSceneComponentData)) {
			for (auto& [name, elem] : SceneComponent::map)
				if (ImGui::CollapsingHeader(name.c_str())) {
					ImGui::Indent();
					ImGui::Text("name: %s", elem->name.c_str());
					if (ImGui::CollapsingHeader("modelmatrices")) {
						ImGui::Indent();
						ImGui::Text("Manip all");
						{
							static glm::mat4 manip = glm::mat4(1.f);
							bool modified = false;
							if (ImGui::DragFloat4("row0", &manip[0][0], .01f)) modified = true;
							if (ImGui::DragFloat4("row1", &manip[1][0], .01f)) modified = true;
							if (ImGui::DragFloat4("row2", &manip[2][0], .01f)) modified = true;
							if (ImGui::DragFloat4("row3", &manip[3][0], .01f)) modified = true;
							if (modified) {
								for (int i = 0; i < elem->modelMatrices.size(); ++i)
									elem->modelMatrices[i] = glm::transpose(manip);
							}
						}
						ImGui::Unindent();

						for (int i = 0; i < elem->m_geometries.size(); ++i)
						{
							ImGui::Indent();
							if (ImGui::CollapsingHeader(elem->m_geometries[i]->name.c_str()))
								gui_display_mat4(elem->modelMatrices[i]);
							ImGui::Unindent();

						}
					}
					ImGui::Unindent();
				}
		}
		ImGui::End();
	}
}



void gui_display_lighting() {
	static bool showLighting = false;

	if (ImGui::BeginMainMenuBar()) {
		ImGui::Separator();
		ImGui::Checkbox("lighting", &showLighting);
		ImGui::EndMainMenuBar();
	}
	if (showLighting) {
		if (ImGui::Begin("Lighting", &showLighting)) {
			for (auto& [name, elem] : Scene::map) {
				ImGui::Text(elem->lightSystem->name.c_str());

				ImGui::Indent();
				bool modified = false;
				if (ImGui::DragFloat("luminance##", &elem->lightSystem->luminance, 0.001f)) modified = true;

				if (ImGui::DragFloat3("directionLightDir##", &elem->lightSystem->directionalLightDir.x, 0.001f, -1.f, 1.f)) modified = true;
				if (ImGui::DragFloat3("directionLightColor##", &elem->lightSystem->directionalLightColor.x, 0.001f, 0.f, 100.f)) modified = true;
				if (ImGui::DragFloat("directionalLightStrength##", &elem->lightSystem->directionalLightStrength, 0.001f)) modified = true;

				ImGui::Unindent();

				ImGui::Indent();

				ImGui::Text("Total Point Lights: %ld", elem->lightSystem->pointLightManagement.size());
				ImGui::Indent();
				
				ImGui::Text("Add New Point Light");
				{
					static glm::vec3 pPos = glm::vec3(0.f, 0.f, 0.f);
					static glm::vec3 pColor = glm::vec3(1.f, 1.f, 1.f);
					static float pPower = 1.f;
					static float pFalloff = 1.f;
					ImGui::DragFloat3("pointLightPos##", &pPos.x, 0.001f);
					ImGui::DragFloat3("pointLightColor##", &pColor.x, 0.001f, 0.f, 100.f);
					ImGui::DragFloat("pointLightPower##", &pPower, 0.001f);
					ImGui::DragFloat("pointLightFalloff##", &pFalloff, 0.001f);
					if (ImGui::Button("Add Point Light")) {
						PointLight p;
						p.pointLightColor = pColor;
						p.pointLightFalloffFactor = pFalloff;
						p.pointLightPos = pPos;
						p.pointLightPower = pPower;
						elem->lightSystem->addPointLight(p);
					}
					if (ImGui::Button("Remove Last Point Light")) {
						elem->lightSystem->removeLastPointLight();
					}
				}
				ImGui::Unindent();
				for (int i = 0; i < elem->lightSystem->pointLightManagement.size(); ++i) {

					ImGui::Indent();
					if (ImGui::CollapsingHeader(("PointLight_" + std::to_string(i)).c_str())) {
						if (ImGui::DragFloat3("pointLightPos##", &elem->lightSystem->pointLightManagement[i].pointLightsData.pointLightPos.x, 0.001f)) modified = true;
						if (ImGui::DragFloat3("pointLightColor##", &elem->lightSystem->pointLightManagement[i].pointLightsData.pointLightColor.x, 0.001f, 0.f, 100.f)) modified = true;
						if (ImGui::DragFloat("pointLightPower##", &elem->lightSystem->pointLightManagement[i].pointLightsData.pointLightPower, 0.001f)) modified = true;
						if (ImGui::DragFloat("pointLightFalloff##", &elem->lightSystem->pointLightManagement[i].pointLightsData.pointLightFalloffFactor, 0.001f)) modified = true;
						if (ImGui::Checkbox("PointLight On", &elem->lightSystem->pointLightManagement[i].pointLightOn)) modified = true;
					}
					ImGui::Unindent();
				}
				ImGui::Unindent();

				if (modified)elem->lightSystem->dirtyFlag = true;
			}
		}
		ImGui::End();

	}

}