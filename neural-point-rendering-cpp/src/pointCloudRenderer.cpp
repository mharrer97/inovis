#include "pointCloudRenderer.h"

#include <ctime>



// ------------------------------------------
// helper funcs and callbacks

void pcr_blit(const Texture2D tex) {
	static Shader blit_shader("blit", "shader/quad.vs", "shader/blit.fs");
	blit_shader->bind();
	blit_shader->uniform("tex", tex, 0);
	Quad::draw();
	blit_shader->unbind();
}
void pcr_blit(const vec3& col) {
	static Shader blit_shader_col("blitCol", "shader/quad.vs", "shader/blitCol.fs");
	//glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	blit_shader_col->bind();
	blit_shader_col->uniform("col", col);
	Quad::draw();
	blit_shader_col->unbind();
	//glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}


void pcr_blitNoise(glm::ivec2 resolution, std::chrono::time_point<std::chrono::system_clock> start) {
	static Shader blit_shader_col("blitNoise", "shader/quad.vs", "shader/blitNoise.fs");
	//glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	blit_shader_col->bind();
	std::chrono::duration<float> diff = std::chrono::system_clock::now() - start;
	blit_shader_col->uniform("time", diff.count());
	blit_shader_col->uniform("resolution", resolution);
	Quad::draw();
	blit_shader_col->unbind();
	//glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}
void pcr_resize_callback(int w, int h) {
	float mod = 2.0 * glm::pow(0.5, gui_params_pcr.resolution_modifier);
	Framebuffer::find("example_fbo")->resize(w*mod, h*mod);
}

void pcr_keyboard_callback(int key, int scancode, int action, int mods) {
	if (ImGui::GetIO().WantCaptureKeyboard) return;
	if (mods == GLFW_MOD_SHIFT && key == GLFW_KEY_R && action == GLFW_PRESS)
		reload_modified_shaders();
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		Context::screenshot("screenshot.png");
	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		gui_params_pcr.draw_gui = (gui_params_pcr.draw_gui + 1) % 4;
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		gui_params_pcr.startCapturing = true;
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		gui_params_pcr.animationRunning = !gui_params_pcr.animationRunning;
}

void pcr_mouse_button_callback(int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
}

void pcr_gui_callback(void) {
	ImGui::ShowMetricsWindow();
}


void PointCloudRenderer::setCurrentCam(ReferenceFrameRGBCamera refCam, RGBCameras rgbCam, int view_in_cp) {
	// load ocam parameters from refcam
	ocam_model model;
	model.image_size = refCam->cams[gui_params_pcr.num].model.image_size;
	model.c = refCam->cams[gui_params_pcr.num].model.c;
	model.d = refCam->cams[gui_params_pcr.num].model.d;
	model.e = refCam->cams[gui_params_pcr.num].model.e;
	model.cx = refCam->cams[gui_params_pcr.num].model.cx;
	model.cy = refCam->cams[gui_params_pcr.num].model.cy;
	model.world2cam_size = refCam->cams[gui_params_pcr.num].model.world2cam.size();
	assert((refCam->cams[gui_params_pcr.num].model.world2cam.size() <= kMaxOcamParams, "TOO MANY OCAM ARGUMENTS; INCREASE kMaxOcamParams"));
	for (int i = 0; i < refCam->cams[gui_params_pcr.num].model.world2cam.size(); ++i) {
		model.world2cam[i] = refCam->cams[gui_params_pcr.num].model.world2cam[i];

	}

	// upload ocam model of current num 
	ocamModelShaderStorageBuffer->bind();
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ocam_model), &model, GL_DYNAMIC_DRAW);
	ocamModelShaderStorageBuffer->unbind();

	// load position and rotation from rgbcam
	glm::vec3 pos;
	glm::quat q;
	if (gui_params_pcr.use_darius_optimized_positions && rgbCam->darius_optimized_pose_present) {
		//std::cerr << "[PointCloudRenderer::setCurrentCam] optimized poses present -> use optimized pose!" << std::endl;
		q = ((rgbCam->cams[view_in_cp].optimized_pose.orientation));
		pos = rgbCam->cams[view_in_cp].optimized_pose.position;
	}
	else {
		//if(rgbCam->darius_optimized_pose_present) std::cerr << "[PointCloudRenderer::setCurrentCam] No optimized poses activated -> use default pose!" << std::endl;
		//else std::cerr << "[PointCloudRenderer::setCurrentCam] No optimized poses present -> use default pose!" << std::endl;
		q = ((rgbCam->cams[view_in_cp].pose.orientation));
		pos = rgbCam->cams[view_in_cp].pose.position;
	}
	glm::mat4 mat_v = (glm::mat4_cast(q));
	current_camera()->pos = pos;
	current_camera()->dir = glm::vec3(mat_v[2]);
	current_camera()->up = -glm::vec3(mat_v[0]);
	current_camera()->update();
}

void PointCloudRenderer::generatePointClouds() {
	std::cerr << "[PointCloudRenderer] Try to parse PLY file" << std::endl;
	// "LOD 0\0LOD 1 - 0.005\0LOD 2 - 0.01\0LOD 3 - 0.02\0LoD 4 - 0.05\0LOD 5 - 0.1"
	//PLYPointCloudParser plyParser(setFolder[dataset] + "pointcloud.ply", true);
	//plyParser.transformPointCloudToNavvis(); // not needed anymore! parser automatically transforms every popint cloud to gl space on every load -> transform back for saving and reducing
	//plyParser.reducePointCloudByBoundingBox(vec3(-10, -10, -2), vec3(20, 10, 10)); // set0 big room
	//plyParser.reducePointCloudByBoundingBox(vec3(-25, -10, -2), vec3(20, 10, 10)); // set1 smaller rooms
	//plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_0.ply");
	
	PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) + "_lod_0.ply", nearest_views, "NavVis", true);
	PLYPointCloudParser plyParser2("../../../set" + std::to_string(dataset) + "_lod_2.ply", nearest_views, "NavVis", true);
	PLYPointCloudParser plyParser3("../../../set" + std::to_string(dataset) + "_lod_3.ply", nearest_views, "NavVis", true);
	/*plyParser.reducePointCloudByFactor(0, 4);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_0_part3.ply");*/
	
	/*PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) + "_lod_0.ply", true);
	plyParser.reducePointCloudByRadius(0.005f);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_1.ply");*/
	//PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) + "_lod_1.ply", true);
	/*plyParser.reducePointCloudByRadius(0.01f);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_2.ply");
	plyParser.reducePointCloudByRadius(0.02f);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_3.ply");
	plyParser.reducePointCloudByRadius(0.05f);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_4.ply");
	plyParser.reducePointCloudByRadius(0.1f);
	plyParser.savePointCloud("../../../set" + std::to_string(dataset) + "_lod_5.ply");*/
	// // for now only load a minor part of the pointcloud for faster debugging
	////PLYPointCloudParser plyParser("../../../reduced_cell_0_1.ply", true);
	////std::cerr << "pointcloud has " << plyParser.points.size() << " points " << std::endl;
	////plyParser.savePointCloud("../../../smallsmall.ply", 1000);
	std::cerr << "[PointCloudRenderer] Finished parsing PLY file" << std::endl;
}

void PointCloudRenderer::loadPointClouds(std::vector<PointCloud>& pcs) {
	//----------------------------------------------------------------------
	// load point clouds
	// extract pointclouddata to single vectors
	std::vector<vec3> pc_position;
	std::vector<vec3> pc_color;
	std::vector<vec3> pc_normal;
	std::vector<float> pc_curvature;
	std::vector<uint32_t> pc_index;

	bool load_lod = true;
	std::pair<vec3, vec3> aabb;
	if (load_lod) {

		for (int i = 0; i < LOD_LEVELS; ++i) {
			std::cerr << "[PointCloudRenderer] Try to parse PLY file lod " << i << std::endl;
			//PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) + "_lod_0_part" + std::to_string(i) + ".ply");
			PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) +  "_lod" + std::to_string(i) + ".ply",nearest_views, "NavVis", true);
			plyParser.createBoundingStructureGrid(2.f);
			if (i == 0)aabb = plyParser.getBoundingBox(); // get bounding box for the bigegst point cloud
			int index = 0;
			//float min_c = std::numeric_limits<float>::max();
			//float max_c = std::numeric_limits<float>::min();
			for (auto& p : plyParser.points) {
				vec3 pos = p.pos;// vec3(p.pos[1], p.pos[2], p.pos[0]); // axis correction now in the parser
				pc_position.push_back(pos);
				pc_color.push_back(p.color);
				vec3 norm = p.normal;// vec3(p.normal[1], p.normal[2], p.normal[0]); // axis correction now in the parser
				pc_normal.push_back(norm);
				pc_curvature.push_back(p.curvature);
				pc_index.push_back(index);
				++index;
				//if (min_c > p.curvature) min_c = p.curvature;
				//if (max_c < p.curvature) max_c = p.curvature;
			}

			// load pointcloud to model
			pcs.emplace_back("PointCloud_LOD" + std::to_string(i));
			// old try without geometry wrapper
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_position.size(), pc_position.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_color.size(), pc_color.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_normal.size(), pc_normal.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 1, pc_curvature.size(), pc_curvature.data());
			pcs[i]->add_index_buffer(pc_index.size(), pc_index.data());
			//std::cerr << bool(pcs[i]->drawCommandBuffer) << std::endl;
			pcs[i]->add_bounding_structure(plyParser.bounding_structure);
			//std::cerr << bool(pcs[i]->drawCommandBuffer) << std::endl;


			pcs[i]->set_primitive_type(GL_POINTS);

			//clear ram in parser
			plyParser.clear();
			pc_position.clear();
			pc_color.clear();
			pc_normal.clear();
			pc_curvature.clear();
			pc_index.clear();

			std::cerr << "[PointCloudRenderer] Finished parsing PLY file lod " << i << std::endl;
		}
	}
	gui_params_pcr.aabb_extend = length(aabb.first - aabb.second);
}

void PointCloudRenderer::loadPointCloudParts(std::vector<PointCloud>& pcs) {
	//----------------------------------------------------------------------
	// load point clouds
	// extract pointclouddata to single vectors
	std::vector<vec3> pc_position;
	std::vector<vec3> pc_color;
	std::vector<vec3> pc_normal;
	std::vector<float> pc_curvature;
	std::vector<uint32_t> pc_index;

	bool load_lod = true;
	std::pair<vec3, vec3> aabb;
	if (load_lod) {

		for (int i = 0; i < gui_params_pcr.pointCloudPartAmount; ++i) {
			std::cerr << "[PointCloudRenderer] Try to parse PLY file part " << i << std::endl;
			PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset) + "_lod_0_part" + std::to_string(i) + ".ply", nearest_views);
			plyParser.createBoundingStructureGrid(2.f);
			if (i == 0)aabb = plyParser.getBoundingBox(); // get bounding box for the bigegst point cloud
			int index = 0;
			//float min_c = std::numeric_limits<float>::max();
			//float max_c = std::numeric_limits<float>::min();
			for (auto& p : plyParser.points) {
				vec3 pos = p.pos;// vec3(p.pos[1], p.pos[2], p.pos[0]); // axis correction now in the parser
				pc_position.push_back(pos);
				pc_color.push_back(p.color);
				vec3 norm = p.normal;// vec3(p.normal[1], p.normal[2], p.normal[0]); // axis correction now in the parser
				pc_normal.push_back(norm);
				pc_curvature.push_back(p.curvature);
				pc_index.push_back(index);
				++index;
				//if (min_c > p.curvature) min_c = p.curvature;
				//if (max_c < p.curvature) max_c = p.curvature;
			}

			// load pointcloud to model
			pcs.emplace_back("PointCloud_Part" + std::to_string(i));
			// old try without geometry wrapper
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_position.size(), pc_position.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_color.size(), pc_color.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_normal.size(), pc_normal.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 1, pc_curvature.size(), pc_curvature.data());
			pcs[i]->add_index_buffer(pc_index.size(), pc_index.data());
			//std::cerr << bool(pcs[i]->drawCommandBuffer) << std::endl;
			pcs[i]->add_bounding_structure(plyParser.bounding_structure);
			//std::cerr << bool(pcs[i]->drawCommandBuffer) << std::endl;


			pcs[i]->set_primitive_type(GL_POINTS);

			//clear ram in parser
			plyParser.clear();
			pc_position.clear();
			pc_color.clear();
			pc_normal.clear();
			pc_curvature.clear();
			pc_index.clear();

			std::cerr << "[PointCloudRenderer] Finished parsing PLY file part " << i << std::endl;
		}
	}
	gui_params_pcr.aabb_extend = length(aabb.first - aabb.second);
}
// ------------------------------------------
// run
void PointCloudRenderer::run(int argc, char** argv) {
	// adjust these parameters to change main functionalities

	int vertical_resolution = VERTICAL_RESOLUTION; // 768; //1536;//990;

	//----------------------------------------------------------------------
	// init GL
	std::cerr << "[PointCloudRenderer] Init GL" << std::endl;
	ContextParameters params;
	if (gui_params_pcr.cameraMode == 0) {
		params.width = 960;// 16 * vertical_resolution / 9;
		params.height = 544;// vertical_resolution;
	}
	else {
		params.width = 2 * vertical_resolution / 3; //scaled down for correct aspect ratio, should be 3648 
		params.height = vertical_resolution;//scaled down for correct aspect ratio, should be 5472
	}
	params.title = "Inovis";
	params.swap_interval = 0;
	Context::init(params);	
	Context::set_keyboard_callback(pcr_keyboard_callback);
	Context::set_mouse_button_callback(pcr_mouse_button_callback);
	gui_add_callback("example_gui_callback", pcr_gui_callback);

	TimerQuery timer = TimerQuery("renderTimer");
	enable_gl_debug_output();
	//----------------------------------------------------------------------


	// example scene
   /* Scene the_scene = Scene("the_scene");*/
	// parse cmd line args
	//for (int i = 1; i < argc; ++i) {
	//    const std::string arg = argv[i];
	//    if (arg == "-w")
	//        Context::resize(std::stoi(argv[++i]), Context::resolution().y);
	//    else if (arg == "-h")
	//        Context::resize(Context::resolution().x, std::stoi(argv[++i]));
	//    else if (arg == "-pos") {
	//        current_camera()->pos.x = std::stof(argv[++i]);
	//        current_camera()->pos.y = std::stof(argv[++i]);
	//        current_camera()->pos.z = std::stof(argv[++i]);
	//    } else if (arg == "-dir") {
	//        current_camera()->dir.x = std::stof(argv[++i]);
	//        current_camera()->dir.y = std::stof(argv[++i]);
	//        current_camera()->dir.z = std::stof(argv[++i]);
	//    } else if (arg == "-fov")
	//        current_camera()->fov_degree = std::stof(argv[++i]);
	//    else {
	//        //for (auto& mesh : load_meshes_gpu(argv[i], true))
	//        //    Drawelement(mesh->name, Shader::find("draw"), mesh);
	//        the_scene->load_model_and_assign_automatically(argv[i],true,false);
	//    }
	//}
	//the_scene->load_model_and_assign_automatically("../../../sponza/sponza.obj", true, false); // hard coded path

	//----------------------------------------------------------------------
	// load clouds
	std::vector<PointCloud> pointClouds;
	loadPointClouds(pointClouds);
	//std::vector<PointCloud> pointCloudParts;
	//loadPointCloudParts(pointCloudParts);
	//----------------------------------------------------------------------

	//----------------------------------------------------------------------
	// shader 
	// default camera shader
	std::vector<Shader> pcShader;
	pcShader.push_back(Shader("drawPC", "shader/drawPC.vs", "shader/drawPC.fs"));
	pcShader.push_back(Shader("drawPCnormal", "shader/drawPC.vs", "shader/drawPCnormal.fs"));
	pcShader.push_back(Shader("drawPCcurvature", "shader/drawPC.vs", "shader/drawPCcurvature.fs"));
	pcShader.push_back(Shader("drawPCdepthmask", "shader/drawPC.vs", "shader/drawPCdepthmask.fs"));
	pcShader.push_back(Shader("drawPCmotion", "shader/drawPCmotion.vs", "shader/drawPCmotion.fs")); 
	pcShader.push_back(Shader("drawPCOQ", "shader/drawPCoq.vs", "shader/drawPCoq.gs", "shader/drawPCoq.fs"));
	pcShader.push_back(Shader("drawPCOQnormal", "shader/drawPCoq.vs", "shader/drawPCoq.gs", "shader/drawPCoqNormal.fs"));
	pcShader.push_back(Shader("drawPCOQcurvature", "shader/drawPCoq.vs", "shader/drawPCoq.gs", "shader/drawPCoqCurvature.fs"));
	pcShader.push_back(Shader("drawPCOQdepthmask", "shader/drawPCoq.vs", "shader/drawPCoq.gs", "shader/drawPCoqDepthmask.fs"));
	pcShader.push_back(Shader("drawPCOQmotion", "shader/drawPCoq.vs", "shader/drawPCoq.gs", "shader/drawPCoq.fs")); 
	// ocam shader
	/*Shader ocamShader("ocamShader", "shader/ocamPC.vs", "shader/ocamPC.fs");
	Shader ocamShaderNormal("ocamShaderNormal", "shader/ocamPC.vs", "shader/ocamPCnormal.fs");
	Shader ocamShaderCurvature("ocamShaderCurvature", "shader/ocamPC.vs", "shader/ocamPCcurvature.fs");
	Shader ocamShaderOQ("ocamShaderOQ", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoq.fs");*/
	std::vector<Shader> pcOcamShader;
	pcOcamShader.push_back(Shader("ocamShader", "shader/ocamPC.vs", "shader/ocamPC.fs"));
	pcOcamShader.push_back(Shader("ocamShaderNormal", "shader/ocamPC.vs", "shader/ocamPCnormal.fs"));
	pcOcamShader.push_back(Shader("ocamShaderCurvature", "shader/ocamPC.vs", "shader/ocamPCcurvature.fs"));
	pcOcamShader.push_back(Shader("ocamShaderDepthmask", "shader/ocamPC.vs", "shader/ocamPCdepthmask.fs"));
	pcOcamShader.push_back(Shader("ocamShadermotion", "shader/ocamPCmotion.vs", "shader/ocamPCmotion.fs"));
	pcOcamShader.push_back(Shader("ocamShaderOQ", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoq.fs"));
	pcOcamShader.push_back(Shader("ocamShaderOQnormal", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoqNormal.fs"));
	pcOcamShader.push_back(Shader("ocamShaderOQcurvature", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoqCurvature.fs"));
	pcOcamShader.push_back(Shader("ocamShaderOQdepthmask", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoqDepthmask.fs"));
	pcOcamShader.push_back(Shader("ocamShaderOQmotion", "shader/ocamPCoq.vs", "shader/ocamPCoq.gs", "shader/ocamPCoq.fs")); 
	// culling compute Shader
	Shader computeFrustumCullingShader("computeFrustumCulling", "shader/computeFrustumCulling.glcs");
	Shader computeOcamCullingShader("computeOcamCulling", "shader/computeOcamCulling.glcs");
	//----------------------------------------------------------------------

	//----------------------------------------------------------------------
	// setup gui params
	for (int i = 0; i < gui_params_pcr.navvisCamCount; ++i) {
		std::string id = std::to_string(i);
		while (id.size() < 5) id = "0" + id;
		gui_params_pcr.navvisCameraSelection.push_back(id);
	}

	glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL * gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod])));
	//----------------------------------------------------------------------
	// setup camera
	current_camera()->near = 0.1f;
	current_camera()->far = 35.f;
	current_camera()->fix_up_vector = false;
	current_camera()->fov_degree = gui_params_pcr.fov;

	//----------------------------------------------------------------------
	Context::set_resize_callback(pcr_resize_callback);
	// setup fbo
	float mod = 2.0 * glm::pow(0.5, gui_params_pcr.resolution_modifier);
	glm::ivec2 res = ivec2(params.width*mod, params.height*mod);// Context::resolution();
	Framebuffer fbo = Framebuffer("example_fbo", res.x, res.y);
	fbo->attach_depthbuffer(Texture2D("example_fbo/depth", res.x, res.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo->attach_colorbuffer(Texture2D("example_fbo/col", res.x, res.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo->check();

	//----------------------------------------------------------------------

	//----------------------------------------------------------------------
	// parse ocam model
	std::cerr << "[PointCloudRenderer] Start Parsing OCAM Model " << std::endl;

	// parse the 4 ocam models as reference
	std::filesystem::path path_sensor_frame(setFolder[dataset] + "sensor_frame.xml");
	refCam = Parser::parseReferenceFile(path_sensor_frame);
	// parse the cam positions
	std::filesystem::path path_navvis(setFolder[dataset]);
	RGBCameras::clear();
	
	// parse all cams at once into rgbCams vector
	std::cerr << "[PointCloudRenderer] Cam Positions " << std::endl;
	for (int i = 0; i < gui_params_pcr.navvisCamCount; ++i) {
		//std::cerr << "[PointCloudRenderer] Parse Cam " << i << std::endl;
		std::string id = std::to_string(i);
		while (id.size() < 5) id = "0" + id;
		std::cout << "Parse cam " << id << "\r";
		rgbCams.emplace_back(Parser::parseCams(path_navvis, id, refCam, 4, !gui_params_pcr.loadGroundtruth)); // here the first position is initialized as well

		if (rgbCams[i]->darius_optimized_pose_present) {
			nearest_views.emplace_back( i, 0, rgbCams[i]->cams[0].optimized_pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[0].optimized_pose.orientation)[2]));
			nearest_views.emplace_back( i, 1, rgbCams[i]->cams[1].optimized_pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[1].optimized_pose.orientation)[2]));
			nearest_views.emplace_back( i, 2, rgbCams[i]->cams[2].optimized_pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[2].optimized_pose.orientation)[2]));
			nearest_views.emplace_back( i, 3, rgbCams[i]->cams[3].optimized_pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[3].optimized_pose.orientation)[2]));
		}
		else {
			nearest_views.emplace_back(i, 0, rgbCams[i]->cams[0].pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[0].pose.orientation)[2]));
			nearest_views.emplace_back(i, 1, rgbCams[i]->cams[1].pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[1].pose.orientation)[2]));
			nearest_views.emplace_back(i, 2, rgbCams[i]->cams[2].pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[2].pose.orientation)[2]));
			nearest_views.emplace_back(i, 3, rgbCams[i]->cams[3].pose.position, glm::vec3(glm::mat4_cast(rgbCams[i]->cams[3].pose.orientation)[2]));
		}
	}
	/*std::cout << "nearest_views size: " << nearest_views.size() << std::endl;
	for (int i = 0; i < nearest_views.size(); ++i) {
		auto& cur = nearest_views[i];
		std::cout << cur.first.first << ":" << cur.first.second << "\t" << cur.second.first << " , " << cur.second.second << std::endl;
	}*/
	
	

	//init SSBO before loading to it in setCurrentCamera
	ocamModelShaderStorageBuffer = SSBO("OCamShaderStorageBuffer");
	ocamModelShaderStorageBuffer->resize(sizeof(ocam_model));

	

	// parse position and orientation
	setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num);
	gui_params_pcr.fov = current_camera()->fov_degree;
	gui_params_pcr.campos = current_camera()->pos;
	

	std::cerr << "[PointCloudRenderer] Finished Parsing OCAM Model " << std::endl;
	//----------------------------------------------------------------------

	//old view matrix for motion vecs
	view_old = current_camera()->view;

	//----------------------------------------------------------------------
	// Animations for automatic random camera movement for dataset creation
	// example animation
	myAnimation = Animation("myAnimation");
	/*myAnimation->push_node(vec3(-6.021261, 1.086725, 0.880409), glm::quat(0.040602, { -0.035238, 0.754961, 0.653562 }));
	myAnimation->push_node(vec3(-5.349366, -4.661439, 1.361215), glm::quat(0.329532, { -0.141117, 0.699628, 0.618074 }));
	myAnimation->push_node(vec3(-1.597957, -5.036160, 1.172142), glm::quat(0.459745, { -0.502792, 0.541570, 0.492480 }));
	myAnimation->push_node(vec3(6.313894, -5.510762, 0.921037), glm::quat(0.465339, { -0.504957, 0.536702, 0.490336 }));
	myAnimation->push_node(vec3(12.469159, -5.463311, 1.407214), glm::quat(-0.591276, { 0.615826, -0.392570, -0.342111 }));
	myAnimation->push_node(vec3(13.200774, -1.574515, 1.524403), glm::quat(-0.696599, { 0.717020, -0.003997, -0.024803 }));
	myAnimation->push_node(vec3(12.583897, 4.055170, 0.963354), glm::quat(-0.632696, { 0.695604, -0.077151, 0.331479 }));
	myAnimation->push_node(vec3(6.055538, 4.921814, 0.919154), glm::quat(0.525487, { -0.489946, -0.502538, -0.480908 }));
	myAnimation->push_node(vec3(1.098742, 5.087135, 0.934151), glm::quat(0.523360, { -0.497280, -0.505573, -0.472444 }));
	myAnimation->push_node(vec3(-1.479190, 5.070765, 0.862661), glm::quat(-0.221718, { 0.272721, 0.689119, 0.633703 }));
	myAnimation->push_node(vec3(-0.890523, 1.190104, 1.156271), glm::quat(0.373584, { -0.336908, 0.607672, 0.614542 }));
	myAnimation->push_node(vec3(6.844116, 0.217109, 1.117900), glm::quat(0.373584, { -0.336908, 0.607672, 0.614542 }));
	myAnimation->push_node(vec3(3.592030, -1.741320, 1.122301), glm::quat(0.132715, { -0.162700, 0.734644, 0.645147 }));*/

	myAnimation->push_node(vec3(2.665753, -0.076543, 0.913379), glm::quat(0.414161, { -0.534316, 0.591737, 0.439119 }));
	myAnimation->push_node(vec3(6.391758, -0.401054, 1.336774), glm::quat(0.472657, { -0.477869, 0.537325, 0.509429 }));
	myAnimation->push_node(vec3(6.914591, -3.419446, 1.262339), glm::quat(0.039058, { -0.028144, 0.725134, 0.686923 }));
	myAnimation->push_node(vec3(9.152531, -5.970881, 1.218955), glm::quat(0.502979, { -0.493643, 0.497307, 0.505978 }));
	myAnimation->push_node(vec3(13.368887, -2.637278, 1.245593), glm::quat(-0.699451, { 0.714669, 0.003374, 0.002382 }));
	myAnimation->push_node(vec3(13.535895, 3.623169, 1.364535), glm::quat(-0.667468, { 0.668829, 0.229090, 0.233822 }));
	myAnimation->push_node(vec3(7.069435, 0.990500, 0.939071), glm::quat(-0.349542, { 0.367205, 0.624332, 0.594298 }));

	//----------------------------------------------------------------------
	// render loop
	std::cerr << "[PointCloudRenderer] Start Render Loop " << std::endl;
	//glDisable(GL_CULL_FACE); 
	// run
	while (Context::running()) {
		// handle input
		glfwPollEvents();
		float frame_time = Context::frame_time();
		CameraImpl::default_input_handler(frame_time);

		//std::cout << "fov cam: " << current_camera()->fov_degree << " fov gui: " << gui_params_pcr.fov << std::endl;

		//----------------------------------------------------------------------
		// Animation handling
		// example animation
		if (gui_params_pcr.animationRunning && !createData) { // only do the animation if no data is captured currently
			//std::cerr << "animation step in ms: " << gui_params_pcr.animationSpeed * frame_time << std::endl;
			myAnimation->update(gui_params_pcr.animationSpeed*frame_time);
			if (!myAnimation->running) {
				//myAnimation->reset();
				myAnimation->play();
				//std::cerr << "Restarting Animation" << std::endl;
			}
		}
		// alternate point clouds if wanted
		if (gui_params_pcr.alternatePointClouds && !createData) {
			gui_params_pcr.currentCloudIndex = (gui_params_pcr.currentCloudIndex + 1) % gui_params_pcr.pointCloudPartAmount;
		}

		//----------------------------------------------------------------------
		// Nearest View Sorting
		// get similarity_descriptor
		if (!createData) {
			get_capture_view_similarity(nearest_views, current_camera()->pos, current_camera()->dir);
			std::sort(nearest_views.begin(), nearest_views.end(), [](Capture_View& i, Capture_View& j) {
				return i.similarity_descriptor < j.similarity_descriptor;
				});
			/*std::cout << "nearest views: " << std::endl;
			for (int i = 0; i <3; ++i) {
				auto& cur = nearest_views[i];
				std::cout << cur.id << ":" << cur.num << " dist: " << length(cur.pos- current_camera()->pos) << "\t" << cur.pos << " , " << cur.dir << std::endl;
			}*/
			if(gui_params_pcr.captureGroundtruthAsPrevious)
				std::cout << "\r" << nearest_views[0].id << ":" << nearest_views[0].num << ", " << nearest_views[0].similarity_descriptor << ", dist " << length(nearest_views[0].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[0].dir), normalize(current_camera()->dir))
						  << "       " << nearest_views[1].id << ":" << nearest_views[1].num << ", " << nearest_views[1].similarity_descriptor << ", dist " << length(nearest_views[1].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[1].dir), normalize(current_camera()->dir))
						  << "       " << nearest_views[2].id << ":" << nearest_views[2].num << ", " << nearest_views[2].similarity_descriptor << ", dist " << length(nearest_views[2].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[2].dir), normalize(current_camera()->dir))
						  << "                              ";
			
		}
		//----------------------------------------------------------------------
		
		//----------------------------------------------------------------------
		// update and reload shaders
		current_camera()->update();
		static uint32_t frame_counter = 0;
		if (frame_counter++ % 100 == 0)
			reload_modified_shaders();

		timer->begin();
		// render all drawelements into fbo
		fbo->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// set bg if movecs are used to place dummy values while backward warping
		//if (gui_params_pcr.currentRenderInfo == 4) blit(vec3(-2.0 * res.x, -2.0 * res.y, 0.0));
		/*the_scene->bindSupermodel(SceneImpl::staticElements);
		the_scene->drawSupermodel(SceneImpl::staticElements);
		the_scene->unbindSupermodel(SceneImpl::staticElements);*/

		if (gui_params_pcr.showGroundtruth) {
			if (gui_params_pcr.cameraMode == 0) { // default camera -> remap
				static Shader blit_shader("blitOcamToProj", "shader/quad.vs", "shader/blitOcamToProj.fs");
				blit_shader->bind();
				ocamModelShaderStorageBuffer->bind_base(0);
				blit_shader->uniform("tex", rgbCams[gui_params_pcr.currentNavvisCam]->cams[gui_params_pcr.num].rgbImage.tex_gpu, 0);
				blit_shader->uniform("view", current_camera()->view);
				blit_shader->uniform("view_normal", current_camera()->view_normal);
				blit_shader->uniform("proj", current_camera()->proj);
				blit_shader->uniform("aabb_extend", gui_params_pcr.aabb_extend);
				blit_shader->uniform("resolution", res);

				Quad::draw();
				ocamModelShaderStorageBuffer->unbind_base(0);
				blit_shader->unbind();
				
			}
			else { // ocam -> simply show groundtruth
				pcr_blit(rgbCams[gui_params_pcr.currentNavvisCam]->cams[gui_params_pcr.num].rgbImage.tex_gpu);
			}
		}
		else {
			//----------------------------------------------------------------------
			if (gui_params_pcr.cameraMode == 0) { //default camera

				// choose shader according to gui
				// if rendermode is 2 (oriented quads), use that shader, else choose the fitting Shader for selected render Info
				Shader& curShader = pcShader[(gui_params_pcr.renderMode != 2) ? gui_params_pcr.currentRenderInfo : 5 + gui_params_pcr.currentRenderInfo];//(renderMode == 2) ? pcShaderOQ : (currentRenderInfo == 0) ? pcShader : (currentRenderInfo == 1) ? pcShaderNormal : pcShaderCurvature;



				//if (gui_params_pcr.alternatePointClouds) { // use different distinct parts of the full clouds present in pointCloudParts
				//	if (gui_params_pcr.enableCulling) pointCloudParts[gui_params_pcr.currentCloudIndex]->cull(computeFrustumCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
				//		current_camera()->near, current_camera()->far, current_camera()->fov_degree, current_camera()->aspect_ratio());
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->bind(curShader);
				//}
				//else { // use the full clouds present in pointClouds
				if (gui_params_pcr.enableCulling) pointClouds[gui_params_pcr.lod]->cull(computeFrustumCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
					current_camera()->near, current_camera()->far, current_camera()->fov_degree, current_camera()->aspect_ratio());
				pointClouds[gui_params_pcr.lod]->bind(curShader);
				//}
				curShader->bind();
				curShader->uniform("proj", current_camera()->proj);
				curShader->uniform("view", current_camera()->view);
				curShader->uniform("view_normal", current_camera()->view_normal);

				//load old view if motion vecs are rendered
				if (gui_params_pcr.currentRenderInfo == 4) {
					curShader->uniform("resolution", res);
					curShader->uniform("view_old", view_old);
					view_old = current_camera()->view;
				}

				if (gui_params_pcr.renderMode == 2) {
					curShader->uniform("point_size", gui_params_pcr.pointSizeOQ * gui_params_pcr.lodPointSizesOQ[gui_params_pcr.lod]);
				}
				//if (gui_params_pcr.alternatePointClouds) { // use different distinct parts of the full clouds present in pointCloudParts
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->draw();
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->unbind();
				//}
				//else { // use the full clouds present in pointClouds
				// draw PointCloud here
				pointClouds[gui_params_pcr.lod]->draw();
				pointClouds[gui_params_pcr.lod]->unbind();
				//}
			}
			else if (gui_params_pcr.cameraMode == 1) { // OCam camera
				//glEnable(GL_PROGRAM_POINT_SIZE); // would enable gl_PointSize in the shader
				//glEnable(GL_POINT_SPRITE); maybe for custom sprite
				//glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);

				//Shader& curShader = (gui_params_pcr.renderMode == 2) ? ocamShaderOQ : (gui_params_pcr.currentRenderInfo == 0) ? ocamShader : (gui_params_pcr.currentRenderInfo == 1) ? ocamShaderNormal : ocamShaderCurvature;
				Shader& curShader = pcOcamShader[(gui_params_pcr.renderMode != 2) ? gui_params_pcr.currentRenderInfo : 5 + gui_params_pcr.currentRenderInfo];


				//if (gui_params_pcr.alternatePointClouds) { // use different distinct parts of the full clouds present in pointCloudParts
				//	if (gui_params_pcr.enableCulling) pointCloudParts[gui_params_pcr.currentCloudIndex]->cull(computeOcamCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
				//		current_camera()->near, current_camera()->far, current_camera()->fov_degree, current_camera()->aspect_ratio());
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->bind(curShader);
				//}
				//else { // use the full clouds present in pointClouds
				if (gui_params_pcr.enableCulling) pointClouds[gui_params_pcr.lod]->cull(computeOcamCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
					current_camera()->near, current_camera()->far, current_camera()->fov_degree, current_camera()->aspect_ratio());
				pointClouds[gui_params_pcr.lod]->bind(curShader);
				//}
				curShader->bind();
				ocamModelShaderStorageBuffer->bind_base(0);
				//shader->uniform("model", model);
				curShader->uniform("view", current_camera()->view);
				curShader->uniform("view_normal", current_camera()->view_normal);
				curShader->uniform("proj", current_camera()->proj);
				curShader->uniform("aabb_extend", gui_params_pcr.aabb_extend);

				//load old view if motion vecs are rendered
				if (gui_params_pcr.currentRenderInfo == 4) {
					curShader->uniform("resolution", res);
					curShader->uniform("view_old", view_old);
					view_old = current_camera()->view;
				}

				if (gui_params_pcr.renderMode == 2) {
					curShader->uniform("point_size", gui_params_pcr.pointSizeOQ * gui_params_pcr.lodPointSizesOQ[gui_params_pcr.lod]);
				}

				//if (gui_params_pcr.alternatePointClouds) { // use different distinct parts of the full clouds present in pointCloudParts
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->draw();
				//	pointCloudParts[gui_params_pcr.currentCloudIndex]->unbind();
				//}
				//else { // use the full clouds present in pointClouds
					// draw PointCloud here
				pointClouds[gui_params_pcr.lod]->draw();
				pointClouds[gui_params_pcr.lod]->unbind();
				//}

				ocamModelShaderStorageBuffer->unbind_base(0);

				curShader->unbind();

				//glDisable(GL_PROGRAM_POINT_SIZE);
				//glDisable(GL_POINT_SPRITE);
			}
		}
		//----------------------------------------------------------------------
		//blitNoise(res, time_start);
		//----------------------------------------------------------------------
		//Data Creation
		// first check if capturing is running, secondly start if necessary -> prevent the first captuiring from being wrong
		if (createData) { // create data here if specified in the previous frame
			captureTrainingData();
		}
		if (gui_params_pcr.startCapturing) { // start capturing if specifies in gui_params_pcr. this is accessed via keyboard callback
			startCaptureTrainingData();
			gui_params_pcr.startCapturing = false;
		}
		
		//----------------------------------------------------------------------

		//----------------------------------------------------------------------
		// draw general cppgl gui
		if (gui_params_pcr.draw_gui == 2 || gui_params_pcr.draw_gui == 3) {
			gui_draw();
		}
		if (gui_params_pcr.draw_gui == 1 || gui_params_pcr.draw_gui == 3) {
			custom_gui_draw();
		}
		//----------------------------------------------------------------------
		
		fbo->unbind();

		timer->end();

		// draw
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
		pcr_blit(fbo->color_textures[0]);
		// finish frame
		Context::swap_buffers();

		//----------------------------------------------------------------------
		// resize if camera mode swap requires resizing
		if (gui_params_pcr.resized) {
			float mod = 2.0 * glm::pow(0.5, gui_params_pcr.resolution_modifier);
			if (gui_params_pcr.cameraMode == 0) { // resize to 16-9
				int new_width = 960;// 16 * VERTICAL_RESOLUTION / 9;
				int new_height = 544;//VERTICAL_RESOLUTION;
				//Context::resize(1820, 1024);
				//Context::resize(1760, 990);
				Context::resize(new_width, new_height);
				//Framebuffer::find("example_fbo")->resize(1820, 1024);
				//Framebuffer::find("example_fbo")->resize(1760, 990);
				Framebuffer::find("example_fbo")->resize(new_width*mod, new_height*mod);
				res = ivec2(new_width * mod, new_height * mod);
			}
			else { // resize to 2-3
				int new_width = 2 * vertical_resolution / 3;
				int new_height = VERTICAL_RESOLUTION;
				//Context::resize(682, 1024);
				//Context::resize(660, 990);
				Context::resize(new_width, new_height);
				//Framebuffer::find("example_fbo")->resize(682, 1024);
				//Framebuffer::find("example_fbo")->resize(660, 990);
				Framebuffer::find("example_fbo")->resize(new_width*mod, new_height*mod);
				res = ivec2(new_width * mod, new_height * mod);
			}
			gui_params_pcr.resized = false;
		}
		//----------------------------------------------------------------------
	}
	//----------------------------------------------------------------------

}

void PointCloudRenderer::custom_gui_draw() {
	// draw sample specific gui
	int height = 280; // height of a gui on the bottom
	int width = 170;  // width of a gui at the right
	glm::ivec2 res = Context::resolution();
	if (gui_params_pcr.cameraMode == 0) {
		ImGui::SetNextWindowSize(ImVec2(width, res.y));//, ImGuiCond_Once);
	}
	else {
		ImGui::SetNextWindowSize(ImVec2(res.x, height));//, ImGuiCond_Once);
	}

	//ImGui::SetNextWindowPos(ImVec2(0, res.y - height) , ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.45f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.75f));
	//ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.f, 0.f, 0.f, 0.25f));
	if (ImGui::Begin("Point Cloud Settings", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {// , ImGuiWindowFlags_NoDecoration);// | ImGuiWindowFlags_NoBackground);
	//if (ImGui::Begin("Point Cloud Settings", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
		if (gui_params_pcr.cameraMode == 0) {
			ImGui::SetWindowPos(ImVec2(res.x - width, 0));
		}
		else {
			ImGui::SetWindowPos(ImVec2(0, res.y - height));
		}
		//ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 1.0), "Point Cloud Settings");
		if (gui_params_pcr.cameraMode == 1) ImGui::Columns(3, "cols");



		//ImGui::SliderInt("debugInt", &debugInt, 1, 25);
		//ImGui::SliderFloat("debugFloat", &debugFloat, -10.f, 10.f);



		// first column for all settings regarding the navvis parsing
		ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "Navvis Settings");

		// Navvis camera frame selection
		ImGui::TextUnformatted("Current Navvis Camera");
		//ImGui::PushItemWidth(125); //set custom width
		if (ImGui::BeginCombo("##NavvisCam", gui_params_pcr.navvisCameraSelection[gui_params_pcr.currentNavvisCam].c_str())) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < gui_params_pcr.navvisCamCount; n++)
			{
				bool is_selected = (gui_params_pcr.currentNavvisCam == n); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(gui_params_pcr.navvisCameraSelection[n].c_str(), is_selected)) {
					gui_params_pcr.currentNavvisCam = n;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}
		//ImGui::PopItemWidth();

		// Buttons for loading specific cam
		bool loadCam = false;
		if (ImGui::Button("cam0", ImVec2(135, 25))) {
			gui_params_pcr.num = 0;
			loadCam = true;
		}
		if (ImGui::Button("cam1", ImVec2(135, 25))) {
			gui_params_pcr.num = 1;
			loadCam = true;
		}
		if (ImGui::Button("cam2", ImVec2(135, 25))) {
			gui_params_pcr.num = 2;
			loadCam = true;
		}
		if (ImGui::Button("cam3", ImVec2(135, 25))) {
			gui_params_pcr.num = 3;
			loadCam = true;
		}
		if (ImGui::Checkbox("Optimize Poses", &gui_params_pcr.use_darius_optimized_positions)) {
			if (rgbCams[gui_params_pcr.currentNavvisCam]->darius_optimized_pose_present) loadCam = true;
			else gui_params_pcr.use_darius_optimized_positions = false; // no optimizatrion present anyways
		}
		// apply pressed camera if necessary
		if (loadCam) { // if one of the load cam buttons was pressed, load cam
			// parse position and orientation
			setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num);
			gui_params_pcr.fov = current_camera()->fov_degree;
			gui_params_pcr.campos = current_camera()->pos;
			createRandomAnimation(gui_params_pcr.campos, glm::quat_cast(current_camera()->view));

		}

		

		ImGui::Text("Capture Start Index");
		ImGui::SliderInt("###Capture Start Index", &gui_params_pcr.capture_startIndex, 0, gui_params_pcr.navvisCamCount);
		ImGui::Text("Capture Step Size");
		ImGui::SliderInt("###Capture Step Size", &gui_params_pcr.capture_stepSize, 1, 30);
		// choose render info to be captured
		ImGui::Text("Capture Info");
		ImGui::Text("Co");
		ImGui::SameLine();
		ImGui::Checkbox("###Col", &capture_renderInfo[0]);
		ImGui::SameLine();
		ImGui::Text("No");
		ImGui::SameLine();
		ImGui::Checkbox("###Norm", &capture_renderInfo[1]);
		//ImGui::SameLine();
		ImGui::Text("Cu");
		ImGui::SameLine();
		ImGui::Checkbox("###Curv", &capture_renderInfo[2]);
		ImGui::SameLine();
		ImGui::Text("De");
		ImGui::SameLine();
		ImGui::Checkbox("###De", &capture_renderInfo[3]);
		//ImGui::SameLine();
		ImGui::Text("Mo");
		ImGui::SameLine();
		ImGui::Checkbox("###Mo", &capture_renderInfo[4]);

		// choose cam nums to be captured
		ImGui::Text("Capture Cam Nums");
		ImGui::Text("0");
		ImGui::SameLine();
		ImGui::Checkbox("###cam0", &capture_cameraNums[0]);
		ImGui::SameLine();
		ImGui::Text("1");
		ImGui::SameLine();
		ImGui::Checkbox("###cam1", &capture_cameraNums[1]);
		ImGui::SameLine();
		ImGui::Text("2");
		ImGui::SameLine();
		ImGui::Checkbox("###cam2", &capture_cameraNums[2]);
		ImGui::SameLine(); 
		ImGui::Text("3");
		ImGui::SameLine();
		ImGui::Checkbox("###cam3", &capture_cameraNums[3]);
		
		ImGui::Text("Groundtruth as Previous");
		ImGui::Checkbox("##captrure groundtruth as previous", &gui_params_pcr.captureGroundtruthAsPrevious);
		ImGui::Text("Animate Previous Frames");
		ImGui::Checkbox("##captrure animate previous", &gui_params_pcr.capturePreviousFramesAnimated);
		if (gui_params_pcr.capturePreviousFramesAnimated || gui_params_pcr.captureGroundtruthAsPrevious) {
			ImGui::SameLine();
			ImGui::SliderInt("##amount previous frames", &gui_params_pcr.capturePreviousFramesAmount, 1, 6);
		}

		if(ImGui::Button( "Capture Data", ImVec2(135, 25))) {
			startCaptureTrainingData();
		}


		if (gui_params_pcr.cameraMode == 1) ImGui::NextColumn();
		else ImGui::Separator();

		ImGui::TextColored(ImVec4(1.0, 0.5, 1.0, 1.0), "Render Settings");
		glm::ivec2 res = Context::resolution();
		ImGui::Text("Context/Window Res:\n %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("example_fbo")->w,Framebuffer::find("example_fbo")->h);
		ImGui::Text("FBO Res:\n %d,%d", res.x, res.y);

		ImGui::Text("Resolution Modifier");
		//combo for resolution modifier
		if (ImGui::Combo("##resolution Modifier", &gui_params_pcr.resolution_modifier, "2\0 1\0 0.5\0 0.25\0 0.125\0 0.0625")) {
			gui_params_pcr.resized = true;
			//float mod = 2.0 * glm::pow(0.5, gui_params_pcr.resolution_modifier);

		}
		if(gui_params_pcr.loadGroundtruth) ImGui::Checkbox("Show Groundtruth", &gui_params_pcr.showGroundtruth);
		if (!gui_params_pcr.loadGroundtruth || !gui_params_pcr.showGroundtruth) {
			// combo for render mode
			ImGui::Text("Render Mode");
			if (ImGui::Combo("##RenderModde", &gui_params_pcr.renderMode, "glPoints\0glPoints smooth\0Oriented Quads\0")) {
				if (gui_params_pcr.renderMode == 0) { // disable smooth
					glDisable(GL_POINT_SMOOTH);
					glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL * gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod])));
				}
				else if (gui_params_pcr.renderMode == 1) {// enable smooth
					glEnable(GL_POINT_SMOOTH);
					glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL * gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod])));
				}
				else { // oriented quads 
					glDisable(GL_POINT_SMOOTH);
					glPointSize(1.f);
				}
			}

			//combo for render content
			ImGui::Text("Render Content");
			ImGui::Combo("##RenderContent", &gui_params_pcr.currentRenderInfo, "Color\0Normal\0Curvature\0Depthmask\0Motion\0");

			// enable culling
			ImGui::Checkbox("Culling", &gui_params_pcr.enableCulling);

			ImGui::Checkbox("Alternate Clouds", &gui_params_pcr.alternatePointClouds);

			if (gui_params_pcr.alternatePointClouds) {
				if (gui_params_pcr.alternatePointClouds) {
					// combo for Part selection
					ImGui::Text("PC Part");
					if (ImGui::Combo("##Part", &gui_params_pcr.currentCloudIndex, "Part 0\0Part 1\0Part 2\0Part 3\0")) {

						glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL)));

					}
				}
			}
			else {


				// combo for LOD selection
				ImGui::Text("LOD");
				if (ImGui::Combo("##LOD", &gui_params_pcr.lod, "LOD 0\0LOD 1 - 0.005\0LOD 2 - 0.01\0LOD 3 - 0.02\0LoD 4 - 0.05\0LOD 5 - 0.1\0")) {
					if (gui_params_pcr.renderMode != 2) {
						glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL * gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod])));
					}
				}
			}
			// display how big points are rendered
			// slider for point size modifier
			if (gui_params_pcr.renderMode != 2) {
				ImGui::Text("Point Size: %d", std::max(1, int(gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod] * gui_params_pcr.pointSizeGL)));
				ImGui::Text("LOD Point Size: %d", gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod]);
				ImGui::Text("Point Size Modifier");
				if (ImGui::SliderFloat("##PointSize", &gui_params_pcr.pointSizeGL, 0.1f, 3.f)) {
					// dont need point size for oriented quads
					glPointSize(std::max(1, int(gui_params_pcr.pointSizeGL * gui_params_pcr.lodPointSizesGL[gui_params_pcr.lod])));
				}
			}
			else {
				ImGui::Text("Point Size: %f", gui_params_pcr.lodPointSizesOQ[gui_params_pcr.lod] * gui_params_pcr.pointSizeOQ);
				ImGui::Text("LOD Point Size: %f", gui_params_pcr.lodPointSizesOQ[gui_params_pcr.lod]);
				ImGui::Text("Point Size Modifier");
				ImGui::SliderFloat("##PointSize", &gui_params_pcr.pointSizeOQ, 0.1f, 3.f);
			}

		}

		if (gui_params_pcr.cameraMode == 1) ImGui::NextColumn();
		else ImGui::Separator();

		ImGui::TextColored(ImVec4(0.5, 1.0, 1.0, 1.0), "Camera Settings");

		// camera mode combo
		ImGui::Text("Camera Mode");
		if (ImGui::Combo("##CameraMode", &gui_params_pcr.cameraMode, "Default\0OCam")) {
			gui_params_pcr.resized = true;
		}

		if (gui_params_pcr.cameraMode == 1) {
			ImGui::Text("Bounding Box Extend");
			ImGui::SliderFloat("##aabb_extend", &gui_params_pcr.aabb_extend, 5.f, 150.f);
		}
		else if (gui_params_pcr.cameraMode == 0)
		{
			ImGui::Text("FOV");
			if (ImGui::SliderFloat("##FOV", &gui_params_pcr.fov, 40.f, 170.f)) {
				current_camera()->fov_degree = gui_params_pcr.fov;
			}
		}
		ImGui::Text("Cam Pos");
		if (ImGui::SliderFloat3("##Campos", &(gui_params_pcr.campos.x), -50.f, 50.f)) {
			current_camera()->pos = gui_params_pcr.campos;
		}
		//if (ImGui::SliderFloat("camposx", &gui_params_pcr.campos.x, -100.f, 100.f)) current_camera()->pos.x = gui_params_pcr.campos.x;
		//if (ImGui::SliderFloat("camposy", &gui_params_pcr.campos.y, -100.f, 100.f)) current_camera()->pos.y = gui_params_pcr.campos.y;
		//if (ImGui::SliderFloat("camposz", &gui_params_pcr.campos.z, -100.f, 100.f)) current_camera()->pos.z = gui_params_pcr.campos.z;
		//gui_params_pcr.campos = current_camera()->pos;

		// display camera direction
		ImGui::Text("Cam Dir");
		ImGui::Text("%.3f, %.3f, %.3f", current_camera()->dir.x, current_camera()->dir.y, current_camera()->dir.z);
		if (ImGui::Button("Print Cam")) {
			std::cerr << "Current Camera at pos: " << current_camera()->pos << " and rot " << glm::quat_cast(current_camera()->view) << std::endl;
		}

		ImGui::Text("Run Animation");
		ImGui::Checkbox("##AnimationRunning" , &gui_params_pcr.animationRunning);
		ImGui::Text("Animation Speed");
		ImGui::SliderFloat("##AnimationSpeed", &gui_params_pcr.animationSpeed, 0.1f, 2.f);
		

		if (gui_params_pcr.cameraMode == 1) ImGui::Columns();
	}
	else {
		if (gui_params_pcr.cameraMode == 0) {
			ImGui::SetWindowPos(ImVec2(res.x - width, 0), ImGuiCond_Always);
		}
		else {
			ImGui::SetWindowPos(ImVec2(0, res.y - 25), ImGuiCond_Always);
		}
	}
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);


	ImGui::End();
}

void PointCloudRenderer::createRandomAnimation(glm::vec3 endpos, glm::quat endrot) {
	myAnimation->clear();
	
	// create random start point.
	vec3 pos(1, 1, 1);
	while (length(pos) > 1.f) {
		pos = vec3(std::rand(), std::rand(), std::rand()) / float(RAND_MAX);
		pos = pos * 2.f - vec3(1.f, 1.f, 1.f);
		//std::cerr << "length " << pos << std::endl;
	}
	pos = normalize(pos);
	// scale to range of 2.5 to 5 around the point
	float dist = 2.5f + (std::rand() / RAND_MAX) * 5.f;
	pos *= dist;
	//std::cerr << "final offset vector " << pos << std::endl;
	// random rotation
	float x, y, z, u, v, w, s;
	do {
		x = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		y = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		z = x * x + y * y;
		//std::cerr << "z: " << z << std::endl;
	} while (z > 1.f);
	do {
		u = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		v = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		w = u * u + v * v;
		//std::cerr << "w: " << w << std::endl;
	} while (w > 1.f);
	s = std::sqrt((1 - z) / w);
	glm::quat q(x,y,s*u,s*v);
	q = normalize(q);
	myAnimation->push_node(endpos+pos, q);
	myAnimation->push_node(endpos, endrot);
	//std::cerr << "pos from " << endpos + pos << " to " << endpos << std::endl;
	//std::cerr << "rot from " << q << " to " << endrot << std::endl << std::endl;
}

void PointCloudRenderer::startCaptureTrainingData() {
	// create list of camera indices. starting at 0 ending at navvisCamCount with step sie capture_stepSize
	capture_cameraIndices.clear();

	// if groundtruth capturings are used as previous frames, only capture movecs and copy gt and rendered depth
	if (gui_params_pcr.captureGroundtruthAsPrevious) {
		capture_renderInfo = { false,false,false,false, true };
	}

	for (int i = gui_params_pcr.capture_startIndex; i < gui_params_pcr.navvisCamCount; i += gui_params_pcr.capture_stepSize) {
		capture_cameraIndices.push_back(i);
	}

	// get init values for counter

	capture_numCounter = 0;
	capture_infoCounter = 0;

	// if previous frames should be captured, set the counter to amount of frames to be captured and count down
	capture_currentFrameCounter = 0;
	if (gui_params_pcr.capturePreviousFramesAnimated || gui_params_pcr.captureGroundtruthAsPrevious) {
		capture_currentFrameCounter = gui_params_pcr.capturePreviousFramesAmount;
		if (gui_params_pcr.alternatePointClouds) // set the current captured pointcloud part if alternation of clouds is wanted
			gui_params_pcr.currentCloudIndex = capture_currentFrameCounter % gui_params_pcr.pointCloudPartAmount;
	}




	while (!capture_cameraNums[capture_numCounter]) capture_numCounter++; // set numCounter to first camera to be captured
	while (!capture_renderInfo[capture_infoCounter]) capture_infoCounter++; // set infoCounter to first info to be captured

	if (capture_numCounter >= capture_cameraNums.size() || capture_infoCounter >= capture_renderInfo.size() || capture_cameraIndices.size() == 0) { // either no num or no info has been activated
		capture_numCounter = 0;
		capture_infoCounter = 0;
		capture_cameraIndices.clear();
		createData = false;
		std::cerr << "[PointCloudRenderer::startCaptureTrainingData] Error: either no num or no info has been chosen to be created. No Data created." << std::endl;
		return;
	}

	std::cerr << "[PointCloudRenderer::startCaptureTrainingData] Start capturing data at " << capture_cameraIndices.size() << " positions." << std::endl;

	// load first cam and renderinfo
	gui_params_pcr.num = capture_numCounter;
	gui_params_pcr.currentRenderInfo = capture_infoCounter;
	gui_params_pcr.currentNavvisCam = capture_cameraIndices[0];


	// set camera for the next
	// parse position and orientation
	setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num);
	gui_params_pcr.fov = current_camera()->fov_degree;
	gui_params_pcr.campos = current_camera()->pos;

	if(gui_params_pcr.captureGroundtruthAsPrevious){ // capture groundtruth as previous frames -> render only movecs and copy groundtruth and highest pc resolution of available depth
		// sort nearest_views
		get_capture_view_similarity(nearest_views, current_camera()->pos, current_camera()->dir);
		std::sort(nearest_views.begin(), nearest_views.end(), [](Capture_View& i, Capture_View& j) {
			return i.similarity_descriptor < j.similarity_descriptor;
			});
		// load cam of groundtruth capture pos of index capture_currentFrameCounter in nearest_views -> ignore the nearest view 0, since that would be a perfect fit
		setCurrentCam(refCam, rgbCams[nearest_views[capture_currentFrameCounter].id], nearest_views[capture_currentFrameCounter].num);
		view_old = current_camera()->view; //movec view old is one of nearest gt pos
		setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num); // load reference position again for movecs
		gui_params_pcr.fov = current_camera()->fov_degree;
		gui_params_pcr.campos = current_camera()->pos;
	} 
	else if (gui_params_pcr.capturePreviousFramesAnimated) { // capture previous frames -> set up a random animation
		// reseed the random engine to get deterministic animations
		std::srand(1);
		createRandomAnimation(gui_params_pcr.campos, glm::quat_cast(current_camera()->view));
		myAnimation->play();
		// upate first loads the current time i.e. here 0 ms. afterwards updates the time.
		// for e.g. 3 previous frames, at first update with 1000-4*16 = 936
		//		then do one update to load previous to first frame -> 16 -> t = 952, while current frame loaded is 3+1 (from t=936)
		//		now save the view matrix for motion vectors
		//		then do one update to load previous to first frame -> 16 -> t = 968, while current frame loaded is 3 (from t=952)
		//		in the loop, do update to load first frame -> 16 -> t = 984, while current frame loaded is 2 (from t=968)
		//		in the loop, do update to load first frame -> 16 -> t = 1000, while current frame loaded is 1 (from t=984)
		//		currentFrame is now 0 -> no update necessary, load parsed camera
		// -> do one update 1000 - previousframesamount *16. (16 ms per frame) 
		myAnimation->update(1000 - (gui_params_pcr.capturePreviousFramesAmount+1)*16);
		std::cerr << "animation at " << myAnimation->time << std::endl;
		myAnimation->update(16);
		std::cerr << "animation at " << myAnimation->time << std::endl;
		current_camera()->update();
		view_old = current_camera()->view;
		// -> do another update to load the pos of the first frame to be captured
		myAnimation->update(16); 
		std::cerr << "animation at " << myAnimation->time << std::endl;
	}
	else { // if no previous frames are captured, prevent motion vectors from jumping around by simulating a static camera
		current_camera()->update();
		view_old = current_camera()->view;
	}

	createData = true;

}

void PointCloudRenderer::captureTrainingData() {
	std::cout << "capture (cam, num, frame, info) : (" << gui_params_pcr.currentNavvisCam << ", " << gui_params_pcr.num << "," << capture_currentFrameCounter << "," << gui_params_pcr.currentRenderInfo << ")" << std::endl;
	// make screenshot
	if (gui_params_pcr.cameraMode == 0) std::cerr << "[PointCloudRenderer] Warning: input will be Default cam but groundtruth is Ocam" << std::endl;
	std::string id = std::to_string(gui_params_pcr.currentNavvisCam);
	while (id.size() < 5) id = "0" + id;
	// save current rendering to corresponding folder
	glm::ivec2 	res = glm::ivec2(Framebuffer::find("example_fbo")->w, Framebuffer::find("example_fbo")->h);
	if (capture_currentFrameCounter == 0) {
		if (gui_params_pcr.currentRenderInfo == 0) { // save to input
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/input/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/input/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
			else { // oriented quads
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/input_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/input_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		if (gui_params_pcr.currentRenderInfo == 1) { // save to normals
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/normal/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/normal/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
			else {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/normal_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/normal_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		if (gui_params_pcr.currentRenderInfo == 2) { //  save to curvature
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/curvature/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/curvature/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
			else {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/curvature_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/curvature_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		if (gui_params_pcr.currentRenderInfo == 3) { //  save to depth
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
			else {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/depthmask_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolution("../../neural-point-rendering-training/data/depthmask_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		if (gui_params_pcr.currentRenderInfo == 4) { //  save to motion
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
					//Context::screenshotCustomResolution("../../neural-point-rendering-training/data/motion/col.png", res);
				 	//Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/motion/col.png.bin", res);
			}
			else {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		// copy ground truth
		//if (!std::filesystem::exists("../../neural-point-rendering-training/data/groundtruth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".jpg"))
			/*std::filesystem::copy(setFolder[dataset] + "cam/" + id + "-cam" + std::to_string(gui_params_pcr.num) + ".jpg",
				"../../neural-point-rendering-training/data/groundtruth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".jpg");*/
	}
	else { // save previous frames to respective folders, leading with previous#_
		
		if (gui_params_pcr.captureGroundtruthAsPrevious) { // copy groundtruth and depth if groundtruth is used as previous
			std::string prefix = "nearest" + std::to_string(capture_currentFrameCounter) + "_";
			int n_id = nearest_views[capture_currentFrameCounter].id;
			int n_num = nearest_views[capture_currentFrameCounter].num;
			std::string s_id = std::to_string(n_id);
			while (s_id.size() < 5) s_id = "0" + s_id;
			// copy ground truth to corresponding
			if (std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "groundtruth/")) {
				//std::cout << "destination folder exists" << std::endl;
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "groundtruth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".jpg"))
					//std::cout << "destination file does not exist" << std::endl;
					//std::filesystem::copy(setFolder[dataset] + "cam/" + s_id + "-cam" + std::to_string(n_num) + ".jpg",
					//	"../../neural-point-rendering-training/data/" + prefix + "groundtruth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".jpg");
					if (std::filesystem::exists("../../neural-point-rendering-training/data/set1_perspective_groundtruth_544/Image" + s_id + "cam" + std::to_string(n_num) + ".jpg")) {
						//std::cout << "source file exists exists  ->  copy" << std::endl;
						std::filesystem::copy("../../neural-point-rendering-training/data/set1_perspective_groundtruth_544/Image" + s_id + "cam" + std::to_string(n_num) + ".jpg",
							"../../neural-point-rendering-training/data/" + prefix + "groundtruth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".jpg");
					}
			}
			// copy ground truth depth to corresponding
			if (std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "depth/")) {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					std::filesystem::copy("../../neural-point-rendering-training/data/set1_perspective_depth_0_lod0/Image" + s_id + "cam" + std::to_string(n_num) + ".png",
						"../../neural-point-rendering-training/data/" + prefix + "depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png");
			}

			// save movec rendering to
			if (gui_params_pcr.renderMode != 2) { // gl points
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/" + prefix + "motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
			else {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
					Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/" + prefix + "motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
			}
		}
		else {
			std::string prefix = "previous" + std::to_string(capture_currentFrameCounter) + "_";
			if (gui_params_pcr.currentRenderInfo == 0) { // save to input
				if (gui_params_pcr.renderMode != 2) { // gl points
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "input/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "input/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
				else { // oriented quads
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "input_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "input_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
			}
			if (gui_params_pcr.currentRenderInfo == 1) { // save to normals
				if (gui_params_pcr.renderMode != 2) { // gl points
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "normal/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "normal/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
				else {
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "normal_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "normal_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
			}
			if (gui_params_pcr.currentRenderInfo == 2) { //  save to curvature
				if (gui_params_pcr.renderMode != 2) { // gl points
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "curvature/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "curvature/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
				else {
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "curvature_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "curvature_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
			}
			if (gui_params_pcr.currentRenderInfo == 3) { //  save to depth
				if (gui_params_pcr.renderMode != 2) { // gl points
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "depth/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
				else {
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "depthmask_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolution("../../neural-point-rendering-training/data/" + prefix + "depthmask_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
			}
			if (gui_params_pcr.currentRenderInfo == 4) { //  save to motion
				if (gui_params_pcr.renderMode != 2) { // gl points
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/" + prefix + "motion/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
				else {
					if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + prefix + "motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png"))
						Context::screenshotCustomResolutionBinary("../../neural-point-rendering-training/data/" + prefix + "motion_oq/Image" + id + "cam" + std::to_string(gui_params_pcr.num) + ".png", res);
				}
			}
		}
	}
	
	// load new cam and renderinfo
	// get the next render info, camera num and camera index if needed
	// look for the next render info counter
	
	bool loadNewFrame = false;
	do {
		capture_infoCounter++; //next renderinfo
		if (capture_infoCounter >= capture_renderInfo.size()) { // overstepped boundary -> set to 0 and load new frame
			loadNewFrame = true;
			capture_infoCounter = 0;
		}

	} while (!capture_renderInfo[capture_infoCounter]);
	gui_params_pcr.currentRenderInfo = capture_infoCounter;
	


	bool loadNewNum = false;
	// only necessary, if a new frame num is started
	if (loadNewFrame) {
		if (gui_params_pcr.captureGroundtruthAsPrevious) {
			capture_currentFrameCounter--; //next frame -> number names previous: 4 = 4 before groundtruth. 0 = current frame
			if (capture_currentFrameCounter == 0) { // overstepped boundary/reached current frame -> set to capture_previousFrameAmount and load new num
				loadNewNum = true;
				capture_currentFrameCounter = gui_params_pcr.capturePreviousFramesAmount;
			}
			else {
				// load cam of groundtruth capture pos of index capture_currentFrameCounter in nearest_views -> ignore the nearest view 0, since that would be a perfect fit
				setCurrentCam(refCam, rgbCams[nearest_views[capture_currentFrameCounter].id], nearest_views[capture_currentFrameCounter].num);
				view_old = current_camera()->view; //movec view old is one of nearest gt pos
				setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num); // load reference position again for movecs
				gui_params_pcr.fov = current_camera()->fov_degree;
				gui_params_pcr.campos = current_camera()->pos;
				
			}
		} 
		else if (gui_params_pcr.capturePreviousFramesAnimated) { // if previous frames are to be captured, first all previous frames are captured before goung to a new num
			capture_currentFrameCounter--; //next frame -> number names previous: 4 = 4 before groundtruth. 0 = current frame
			if (capture_currentFrameCounter == -1) { // overstepped boundary -> set to capture_previousFrameAmount and load new num
				loadNewNum = true;
				capture_currentFrameCounter = gui_params_pcr.capturePreviousFramesAmount;
			}
			else if (capture_currentFrameCounter == 0) { // current frame reached, load parsed camera to avoid interpolation and floating point errors completely
				// parse position and orientation
				setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num);
				gui_params_pcr.fov = current_camera()->fov_degree;
				gui_params_pcr.campos = current_camera()->pos;
			}
			else {
				// update the animation if necessary i.e. a new frame is to be loaded but no new cam num
				myAnimation->update(16);
				std::cerr << "animation at " << myAnimation->time << std::endl;
			}
			if (gui_params_pcr.alternatePointClouds) // set the current captured pointcloud part if alternation of clouds is wanted
				gui_params_pcr.currentCloudIndex = capture_currentFrameCounter % gui_params_pcr.pointCloudPartAmount;

		}
		else { // if no previous frames are to be captured, load a new num directly
			loadNewNum = true;
		}
	}

	// only necessary, if a new camera num is started
	bool loadNewCamIndex = false;
	if (loadNewNum) {
		do {
			capture_numCounter++; //next cam num
			if (capture_numCounter >= capture_cameraNums.size()) { // overstepped boundary -> set to 0 and load new camera index
				loadNewCamIndex = true;
				capture_numCounter = 0;
			}

		} while (!capture_cameraNums[capture_numCounter]);
	}
	gui_params_pcr.num = capture_numCounter;
	
	// only necessary if a new cam index is started
	if(loadNewCamIndex)
	{

		capture_cameraIndices.erase(capture_cameraIndices.begin());
		gui_params_pcr.currentNavvisCam = capture_cameraIndices[0];


	}
	// set camera for the next if necessary
	if (loadNewNum) {
		// parse position and orientation
		setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num);
		gui_params_pcr.fov = current_camera()->fov_degree;
		gui_params_pcr.campos = current_camera()->pos;
		
		if (gui_params_pcr.captureGroundtruthAsPrevious) { // capture groundtruth as previous frames -> render only movecs and copy groundtruth and highest pc resolution of available depth
			// sort nearest_views
			get_capture_view_similarity(nearest_views, current_camera()->pos, current_camera()->dir);
			std::sort(nearest_views.begin(), nearest_views.end(), [](Capture_View& i, Capture_View& j) {
				return i.similarity_descriptor < j.similarity_descriptor;
				});
			// load cam of groundtruth capture pos of index capture_currentFrameCounter in nearest_views -> ignore the nearest view 0, since that would be a perfect fit
			setCurrentCam(refCam, rgbCams[nearest_views[capture_currentFrameCounter].id], nearest_views[capture_currentFrameCounter].num);
			view_old = current_camera()->view; //movec view old is one of nearest gt pos
			setCurrentCam(refCam, rgbCams[gui_params_pcr.currentNavvisCam], gui_params_pcr.num); // load reference position again for movecs
			gui_params_pcr.fov = current_camera()->fov_degree;
			gui_params_pcr.campos = current_camera()->pos;
		}
		else if (gui_params_pcr.capturePreviousFramesAnimated) {
			// on loading a new num, load a new random animation
			createRandomAnimation(gui_params_pcr.campos, glm::quat_cast(current_camera()->view));
			myAnimation->play();
			// upate first loads the current time i.e. here 0 ms. afterwards updates the time.
			// for e.g. 3 previous frames, at first update with 1000-4*16 = 936
			//		then do one update to load previous to first frame -> 16 -> t = 952, while current frame loaded is 3+1 (from t=936)
			//		now save the view matrix for motion vectors
			//		then do one update to load previous to first frame -> 16 -> t = 968, while current frame loaded is 3 (from t=952)
			//		in the loop, do update to load first frame -> 16 -> t = 984, while current frame loaded is 2 (from t=968)
			//		in the loop, do update to load first frame -> 16 -> t = 1000, while current frame loaded is 1 (from t=984)
			//		currentFrame is now 0 -> no update necessary, load parsed camera
			// -> do one update 1000 - previousframesamount *16. (16 ms per frame) 
			myAnimation->update(1000 - (gui_params_pcr.capturePreviousFramesAmount + 1) * 16);
			std::cerr << "animation at " << myAnimation->time << std::endl;
			myAnimation->update(16);
			std::cerr << "animation at " << myAnimation->time << std::endl;
			current_camera()->update();
			view_old = current_camera()->view;
			// -> do another update to load the pos of the first frame to be captured
			myAnimation->update(16);
			std::cerr << "animation at " << myAnimation->time << std::endl;
		}
		else { // if no previous frames are captured, prevent motion vectors from jumping around by simulating a static camera
			current_camera()->update();
			view_old = current_camera()->view;
		}
	}

	if(capture_cameraIndices.size() == 0) createData = false; // end capturing if
}
