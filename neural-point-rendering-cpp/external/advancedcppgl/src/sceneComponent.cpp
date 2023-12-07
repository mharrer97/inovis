#include "sceneComponent.h"
#include "bindlessTexture.h"
#include "renderData.h"
#include <algorithm>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"
#include <omp.h>

SceneComponentImpl::SceneComponentImpl(std::string name) : name(name), dirtyFlag(true), drawCommandBuffer(name + "_drawCommandBuffer"),
superModelVBO(name + "_superModelVBO"), superModelIBO(name + "_superModelIBO"), materialDataBuffer(name + "_materialDataBuffer"),
modelDataBuffer(name + "_modelDataBuffer"), dynamicModelDataBuffer(name + "_dynamicModelDataBuffer")
{
	m_geometries.clear();
	m_materials.clear();
	m_subdivided_geometries.clear();

}

void SceneComponentImpl::compute_and_upload_supermodel() {
	if (m_geometries.empty()) {
		std::cout << ">>>> SceneComponent \"" << name << "\" has no geometry (not loaded properly?)" << std::endl << std::endl;
		return;
	}

	std::cout << "Create Supermodel: " << name << std::endl;

	//clear all subdivided geometries 
	m_subdivided_geometries.clear(); 

	//precision of Vertices / struct used
	using Vertex = Vertex_fullP;

	//add per-vertex attributes for arrays-of-structs input structure
	std::vector<Vertex> vertices;
	std::vector<uint> indices;

	std::vector<ModelData> modelData;
	std::vector<MaterialData> materialData;
	std::vector<DrawElementsIndirectCommand> commandBufferData;

	//running variable to count submodels for material linking
	uint submodelID = 0;
	//running variable to count indices for command buffers
	uint totalIndicesCount = 0;

	std::vector<Material> materials_in_use;


	std::cout << "Processed Geometry: [" << 0 << "/" << m_geometries.size() << "]"<< std::flush;
	for (int geo_index = 0; geo_index < m_geometries.size(); ++geo_index) {
		std::cout << '\r' << std::flush;
		std::cout << "Processed Geometry: [" << geo_index << "/" << m_geometries.size() << "]" << std::flush;

		auto& geo = m_geometries[geo_index];

		//get a unique set of used materials and link materials via id
		Material m = m_materials[geo_index];
		int material_id = -1;
		for (int mID = 0; mID < materials_in_use.size(); ++mID) {
			if (materials_in_use[mID]->name == m->name)
				material_id = mID;
		}
		if (material_id == -1)
		{
			materials_in_use.push_back(m);
			material_id = int(materials_in_use.size() - 1);
		}

		//get tc range
		float tcMin = FLT_MAX;
		float tcMax = FLT_MIN;
		for (auto tc : geo->texcoords) {
			tcMin = std::min(tcMin, std::min(tc.x, tc.y));
			tcMax = std::max(tcMax, std::max(tc.x, tc.y));
		}

		//subdivide geometry
		std::vector<SubdividedGeometry> subd_geos = subdivideModel(geo);

		//process subdivided geo
		for (auto subd_geo : subd_geos) {

			//model data to rescale and link material
			glm::vec3 aabb_length = abs(geo->bb_max - geo->bb_min);
			ModelData data;
			data.b_min = geo->bb_min;
			data.axisLengths = aabb_length;
			data.subModel_b_min = subd_geo.bb_min;
			data.subModel_axisLengths = subd_geo.bb_max - subd_geo.bb_min;
			data.subModelID = submodelID;
			data.materialID = material_id;
			data.tcMin = tcMin;
			data.tcExtend = tcMax - tcMin;
			data.dynamicModelID = geo_index;

			modelData.push_back(data);

			//command buffer for submodel
			commandBufferData.emplace_back(totalIndicesCount, GLuint(subd_geo.indices.size()), 0);

			//add indices
			for (int j = 0; j < subd_geo.indices.size(); ++j) {
				indices.push_back((unsigned int)(vertices.size()) + subd_geo.indices[j]);
			}
			totalIndicesCount += (unsigned int)(subd_geo.indices.size());


			++submodelID;
		}

		//lastly add all vertices
		glm::vec3 axisLengths = geo->bb_max - geo->bb_min;
		float tcExtend = tcMax - tcMin;

		for (int j = 0; j < geo->positions.size(); ++j) {

			//rescale pos to [0,1] to make full use of float precision
			glm::vec3 rescaled_pos = geo->positions[j] - geo->bb_min;
			if (axisLengths.x != 0.f) rescaled_pos.x /= axisLengths.x;
			if (axisLengths.y != 0.f) rescaled_pos.y /= axisLengths.y;
			if (axisLengths.z != 0.f) rescaled_pos.z /= axisLengths.z;

			glm::vec2 rescaled_tc = vec2(0, 0);
			if (geo->texcoords.size()>j) {
				rescaled_tc = geo->texcoords[j] - tcMin;
				if (tcExtend != 0.f)
					rescaled_tc /= glm::vec2(tcExtend);
			}
			glm::vec3 rescaled_normal = vec3(0, 0, 0);
			if (geo->normals.size() > j) {
				rescaled_normal = geo->normals[j];
			}

			//ADD VERTEX
			vertices.emplace_back(rescaled_pos, rescaled_normal, rescaled_tc);
		}
		
	}

	std::cout << "Upload Supermodel to GPU: " << name << std::endl;

	//upload model, material and command buffer data
	std::vector<MaterialData> matDataToUpload;
	for (auto mat : materials_in_use) {
		//creates a MaterialData struct with a fixed function
		matDataToUpload.push_back(RenderData::createMaterialData(mat));
	}

	materialDataBuffer->resize(matDataToUpload.size() * sizeof(MaterialData));
	materialDataBuffer->bind();
	glBufferData(GL_SHADER_STORAGE_BUFFER, matDataToUpload.size() * sizeof(MaterialData), matDataToUpload.data(), GL_DYNAMIC_DRAW);
	materialDataBuffer->unbind();

	modelDataBuffer->resize(modelData.size() * sizeof(ModelData));
	modelDataBuffer->bind();
	glBufferData(GL_SHADER_STORAGE_BUFFER, modelData.size() * sizeof(ModelData), modelData.data(), GL_DYNAMIC_DRAW);
	modelDataBuffer->unbind();

	reloadDynamicModelDataBuffer();

	drawCommandBuffer->clearElements();
	drawCommandBuffer->addElements(commandBufferData);

	//upload vertex and index data
	if (!vao) {
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);

	superModelIBO->resize(sizeof(uint) * indices.size());
	superModelIBO->bind();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * indices.size(), indices.data(), GL_STATIC_DRAW);

	superModelVBO->resize(sizeof(Vertex) * vertices.size());
	superModelVBO->bind();
	glEnableVertexAttribArray(VERTEX_ATTRIBUTES::A_POSITION);
	glVertexAttribPointer(VERTEX_ATTRIBUTES::A_POSITION, 3, Vertex::pos_type, Vertex::pos_normalized, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::vPosition));
	glEnableVertexAttribArray(VERTEX_ATTRIBUTES::A_NORMAL);
	glVertexAttribPointer(VERTEX_ATTRIBUTES::A_NORMAL, 3, Vertex::normal_type, GL_TRUE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::vNormal));
	glEnableVertexAttribArray(VERTEX_ATTRIBUTES::A_TEXCOORDS);
	glVertexAttribPointer(VERTEX_ATTRIBUTES::A_TEXCOORDS, 2, Vertex::tc_type, GL_TRUE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Vertex::vTexCoord));

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	superModelVBO->unbind();
	superModelIBO->unbind();

	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
		throw std::runtime_error("error with super model loading, ending");

	std::cout << "Finished Supermodel: " << name << std::endl;

	//build finished
	dirtyFlag = false;
}

std::vector<SubdividedGeometry> SceneComponentImpl::subdivideModel(Geometry& geo) {

	std::vector<SubdividedGeometry> result = ModelSubdivision::subdivideGeometry(geo);
	m_subdivided_geometries.insert(m_subdivided_geometries.end(), result.begin(), result.end());

	return result;
}


void SceneComponentImpl::add_geometry(Geometry geo) {
	m_geometries.push_back(geo);
	m_geometries.back()->recompute_aabb();
	modelMatrices.push_back(glm::mat4(1.f));
	dirtyFlag = true;
}
void SceneComponentImpl::add_material(Material mat) {
	m_materials.push_back(mat);
	dirtyFlag = true;

}

void SceneComponentImpl::reloadDynamicModelDataBuffer() {
	std::vector<DynamicModelData> dynamicModelData;

	for (int i = 0; i < m_geometries.size(); ++i) {
		//add dynamic model data
		DynamicModelData dmd;
		dmd.modelMatrix = modelMatrices[i];
		dynamicModelData.push_back(dmd);
	}

	if (dynamicModelDataBuffer->size_bytes != dynamicModelData.size() * sizeof(DynamicModelData))
		dynamicModelDataBuffer->resize(dynamicModelData.size() * sizeof(DynamicModelData));

	GLvoid* p = dynamicModelDataBuffer->map();
	memcpy(p, dynamicModelData.data(), dynamicModelData.size() * sizeof(DynamicModelData));
	dynamicModelDataBuffer->unmap();

}

void SceneComponentImpl::bind(const Shader& shader) {
	if (dirtyFlag)
		compute_and_upload_supermodel();

	reloadDynamicModelDataBuffer();

	shader->bind();
	materialDataBuffer->bind_base(MATERIAL_DATA_BINDPOINT);
	modelDataBuffer->bind_base(MODEL_DATA_BINDPOINT);
	dynamicModelDataBuffer->bind_base(DYNAMIC_MODEL_DATA_BINDPOINT);

}

void SceneComponentImpl::draw(const Shader& shader) {

	shader->uniform("view", current_camera()->view);
	shader->uniform("view_normal", current_camera()->view_normal);
	shader->uniform("proj", current_camera()->proj);

	glBindVertexArray(vao);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawCommandBuffer->getFrontBuffer()->id);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, GLsizei(drawCommandBuffer->getSize()), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);

}

void SceneComponentImpl::unbind(const Shader& shader) {
	materialDataBuffer->unbind_base(MATERIAL_DATA_BINDPOINT);
	modelDataBuffer->unbind_base(MODEL_DATA_BINDPOINT);
	dynamicModelDataBuffer->unbind_base(DYNAMIC_MODEL_DATA_BINDPOINT);

	shader->unbind();
}