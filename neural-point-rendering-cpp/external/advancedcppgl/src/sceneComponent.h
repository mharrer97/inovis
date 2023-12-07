#pragma once

#include <cppgl.h>
#include "commandBuffer.h"
#include "vertexLayout.h"
#include "platform_adv.h"
#include "modelSubdivision.h"


// ------------------------------------------
// SceneComponent
//
// A SceneComponent contains batched models to allow efficient indirect draw
// 


class SceneComponentImpl {
public: //structs


public: //data
	std::string name;
	std::vector<Geometry> m_geometries;
	std::vector<Material> m_materials;

	//storage for subdivided geometry
	std::vector<SubdividedGeometry> m_subdivided_geometries;

	// flag is set if geometry/materials are added or other changes are made
	// forces reupload to the gpu
	bool dirtyFlag;

	//gpu buffers
	DrawCommandBuffer drawCommandBuffer;
	//per material buffer
	SSBO materialDataBuffer;
	//per submodel buffer
	SSBO modelDataBuffer;
	//per model buffer, updated every frame
	SSBO dynamicModelDataBuffer;
	
	VBO superModelVBO;
	IBO superModelIBO;
	GLuint vao = 0;

	std::vector<glm::mat4> modelMatrices;

public: //methods
	SceneComponentImpl(std::string name);
	~SceneComponentImpl() {}

	void add_geometry(Geometry geo);
	void add_material(Material mat);

	//used to construct the actual supermodel on the gpu
	void compute_and_upload_supermodel();

	void reloadDynamicModelDataBuffer();

	std::vector<SubdividedGeometry> subdivideModel(Geometry& geo);

	void bind(const Shader& shader);
	void draw(const Shader& shader);
	void unbind(const Shader& shader);

	// prevent copies and moves, since GL buffers aren't reference counted
	SceneComponentImpl(const SceneComponentImpl&) = delete;
	SceneComponentImpl& operator=(const SceneComponentImpl&) = delete;
	SceneComponentImpl& operator=(const SceneComponentImpl&&) = delete;

};



using SceneComponent = NamedHandle<SceneComponentImpl>;
template class _API_ADV NamedHandle<SceneComponentImpl>; //needed for Windows DLL export
