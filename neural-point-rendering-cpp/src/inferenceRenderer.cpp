#include "inferenceRenderer.h"

#include "frustum.h"
#include "camPathRenderer.h"

#include <ctime>
#include <cmath>
////////////////////////////////////////////////////////////////
// helper funcs and callbacks

////////////////////////////////////////////////////////////////
// draw a texture to the fbo
void ir_blit(const Texture2D tex) {
	static Shader blit_shader("blit", "shader/quad.vs", "shader/blit.fs");
	blit_shader->bind();
	blit_shader->uniform("tex", tex, 0);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw a depth texture to the fbo
void ir_blit_depth(const Texture2D tex) {
	static Shader blit_shader("blit_depth", "shader/quad.vs", "shader/blitDepth.fs");
	blit_shader->bind();
	blit_shader->uniform("tex", tex, 0);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw 4 textures to the fbo
void ir_blit_multi(const Texture2D tex0, const Texture2D tex1, const Texture2D tex2, const Texture2D tex3) {
	static Shader blit_shader("blit_multi", "shader/quad.vs", "shader/blitmulti.fs");
	blit_shader->bind();
	blit_shader->uniform("tex0", tex0, 0);
	blit_shader->uniform("tex1", tex1, 1);
	blit_shader->uniform("tex2", tex2, 2);
	blit_shader->uniform("tex3", tex3, 3);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw 4 depth textures to the fbo
void ir_blit_multi_depth(const Texture2D tex0, const Texture2D tex1, const Texture2D tex2, const Texture2D tex3) {
	static Shader blit_shader("blit_multi_depth", "shader/quad.vs", "shader/blitmultiDepth.fs");
	blit_shader->bind();
	blit_shader->uniform("tex0", tex0, 0);
	blit_shader->uniform("tex1", tex1, 1);
	blit_shader->uniform("tex2", tex2, 2);
	blit_shader->uniform("tex3", tex3, 3);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw a color to the fbo
void ir_blit(const vec3& col) {
	static Shader blit_shader_col("blitCol", "shader/quad.vs", "shader/blitCol.fs");
	glDepthMask(GL_FALSE);
	blit_shader_col->bind();
	blit_shader_col->uniform("col", col);
	Quad::draw();
	blit_shader_col->unbind();
	glDepthMask(GL_TRUE);
}

////////////////////////////////////////////////////////////////
// draw a noise texture to the fbo
void ir_blitNoise(glm::ivec2 resolution, std::chrono::time_point<std::chrono::system_clock> start) {
	static Shader blit_shader_col("blitNoise", "shader/quad.vs", "shader/blitNoise.fs");
	glDepthMask(GL_FALSE);
	blit_shader_col->bind();
	std::chrono::duration<float> diff = std::chrono::system_clock::now() - start;
	blit_shader_col->uniform("time", diff.count());
	blit_shader_col->uniform("resolution", resolution);
	Quad::draw();
	blit_shader_col->unbind();
	glDepthMask(GL_TRUE);
}

////////////////////////////////////////////////////////////////
// draw a mipmapped variant of tex_rgb and tex_depth to the fbo
void ir_mipmap(const Texture2D tex_rgb, const Texture2D tex_depth, glm::ivec2 res_high, glm::ivec2 res_low) {
	static Shader blit_shader("mipmapShader", "shader/quad.vs", "shader/mipmap.fs");
	blit_shader->bind();
	blit_shader->uniform("tex_rgb", tex_rgb, 0);
	blit_shader->uniform("tex_depth", tex_depth, 1);
	blit_shader->uniform("res_high", res_high);
	blit_shader->uniform("res_low", res_low);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw a mipmapped variant of tex_rgb and tex_depth and 6 motion vectors to the fbo
void ir_mipmap_with_motion(const Texture2D tex_rgb, const Texture2D tex_motion1, const Texture2D tex_motion2, const Texture2D tex_motion3, const Texture2D tex_motion4, const Texture2D tex_motion5, const Texture2D tex_motion6, const Texture2D tex_depth, glm::ivec2 res_high, glm::ivec2 res_low, bool motion_pixelwise) {
	static Shader pixelwise_shader("mipmapMotionShader", "shader/quad.vs", "shader/mipmapMotion.fs");
	static Shader tc_shader("mipmapMotionShaderTC", "shader/quad.vs", "shader/mipmapMotionTC.fs");
	Shader& blit_shader = (motion_pixelwise) ? pixelwise_shader : tc_shader;
	blit_shader->bind();
	blit_shader->uniform("tex_rgb", tex_rgb, 0);
	blit_shader->uniform("tex_depth", tex_depth, 1);
	blit_shader->uniform("tex_motion_1", tex_motion1, 2);
	blit_shader->uniform("tex_motion_2", tex_motion2, 3);
	blit_shader->uniform("tex_motion_3", tex_motion3, 4);
	blit_shader->uniform("tex_motion_4", tex_motion3, 4);
	blit_shader->uniform("tex_motion_5", tex_motion3, 4);
	blit_shader->uniform("tex_motion_6", tex_motion3, 4);
	blit_shader->uniform("res_high", res_high);
	blit_shader->uniform("res_low", res_low);
	Quad::draw();
	blit_shader->unbind();
}

////////////////////////////////////////////////////////////////
// draw the difference of two textures to the fbo
void ir_diff(const Texture2D tex1, const Texture2D tex2) {
	static Shader blit_shader("diff", "shader/quad.vs", "shader/diff.fs");
	blit_shader->bind();
	blit_shader->uniform("tex_1", tex1, 1);
	blit_shader->uniform("tex_2", tex2, 2);
	Quad::draw();
	blit_shader->unbind();
}



////////////////////////////////////////////////////////////////
// draw a debug camera frustum
void ir_debug_frustum(Frustum& frustum, mat4& view_frustum, mat4& view, mat4& proj, const glm::vec3& col = glm::vec3(-1,-1,-1)){
    static Shader frustum_shader("frustum", "shader/frustum.vs", "shader/frustum.fs");
    frustum_shader->bind();
    frustum_shader->uniform("view", view);
    frustum_shader->uniform("view_frustum", view_frustum);
    frustum_shader->uniform("proj", proj);
    if(col.x > -0.5){
        frustum_shader->uniform("custom_col", 1);
    } else {
        frustum_shader->uniform("custom_col", 0);
    }
    frustum_shader->uniform("col", col);

    frustum.draw_frustum();
    frustum_shader->unbind();
}


////////////////////////////////////////////////////////////////
// draw a debug camera frustum
void ir_render_cam_path(CamPathRenderer & pathRenderer, mat4& view, mat4& proj, const glm::vec3& col){
    static Shader camPath_shader("camPath", "shader/camPath.vs", "shader/camPath.fs");
    camPath_shader->bind();
    camPath_shader->uniform("view", view);
    camPath_shader->uniform("proj", proj);
    camPath_shader->uniform("col", col);
    pathRenderer.draw_path();
    camPath_shader->unbind();
}



////////////////////////////////////////////////////////////////
// callback function resize -- called on window resize
// does not use the values at all. size is specified by the framebuffer size on startup + the resolution modifier
void ir_resize_callback(int w, int h) {
	//---------------------------------------------------------------
	// resize to the next smaller value divisible by 32
//	int width = w - (w%32);
//	int height = h - (h%32);
	float mod = 2.0 * glm::pow(0.5, gui_params_ir.resolution_modifier);
//	int nw = (width * mod)/2;
//	int nh = (height * mod)/2;
    int nw = gui_params_ir.initial_resolution_default.x * mod;
    int nh = gui_params_ir.initial_resolution_default.y * mod;
	//---------------------------------------------------------------
	// set resolution for all fbos of different resolution
	gui_params_ir.res0 = glm::ivec2(nw  , nh  );
	gui_params_ir.res1 = glm::ivec2(nw/2, nh/2);
	gui_params_ir.res2 = glm::ivec2(nw/4, nh/4);
	gui_params_ir.res3 = glm::ivec2(nw/8, nh/8);
	Framebuffer::find("fbo_out_1")->resize(nw, nh);
	Framebuffer::find("fbo_out_2")->resize(nw, nh);
	Framebuffer::find("fbo_res0")->resize(nw, nh);
	Framebuffer::find("fbo_res1")->resize(gui_params_ir.res1.x, gui_params_ir.res1.y);
	Framebuffer::find("fbo_res2")->resize(gui_params_ir.res2.x, gui_params_ir.res2.y);
	Framebuffer::find("fbo_res3")->resize(gui_params_ir.res3.x, gui_params_ir.res3.y);
	std::cout << "\t[ir_resize_callback()] Resize to [(" << gui_params_ir.res0 << "), "
				<< "(" << gui_params_ir.res1 << "), "
				<< "(" << gui_params_ir.res2 << "), "
				<< "(" << gui_params_ir.res3 << ")]" << std::endl;
}

////////////////////////////////////////////////////////////////
// callback function motion buffer resize -- called on movec buffer resize
void ir_resize_motion_buffer() {
	Framebuffer fbo = Framebuffer::find("fbo_motion");
	auto res = glm::ivec2(0, 0);
	switch (gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]) {
	case 0:
		res = gui_params_ir.res0;
		break;
	case 1:
		res = gui_params_ir.res1;
		break;
	case 2:
		res = gui_params_ir.res2;
		break;
	case 3:
		res = gui_params_ir.res3;
		break;
	default:
		std::cerr << "[ir_resize_motion_buffer] FATAL ERROR: feature extraction depth not recognized." << std::endl;
	}
	fbo->resize(res.x,res.y);
	std::cerr << "[ir_resize_motion_buffer] resized motion buffer to " << res << std::endl;
}

////////////////////////////////////////////////////////////////
// callback function keyboard -- called on button pressed
void ir_keyboard_callback(int key, int scancode, int action, int mods) {
	if (ImGui::GetIO().WantCaptureKeyboard) return;
	//---------------------------------------------------------------
	// SHIFT + R: Reload Shader
	if (mods == GLFW_MOD_SHIFT && key == GLFW_KEY_R && action == GLFW_PRESS)
		reload_modified_shaders();
	//---------------------------------------------------------------
	// ENTER: make screenshot of the Context
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
		Context::screenshot("screenshot.png");
	//---------------------------------------------------------------
	// G: toggle Gui modes. UI; TIMING; UI+TIMING; NOTHING
	if (key == GLFW_KEY_G && action == GLFW_PRESS)
		gui_params_ir.draw_gui = (gui_params_ir.draw_gui + 1) % 4;
	//---------------------------------------------------------------
	// P: toggle if the current animation is played
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		gui_params_ir.animationRunning = !gui_params_ir.animationRunning;
	}
	//---------------------------------------------------------------
	// N: cycle to next network
	if (key == GLFW_KEY_N && action == GLFW_PRESS) {
		gui_params_ir.prev_network_id = gui_params_ir.network_id;
		gui_params_ir.network_id = (gui_params_ir.network_id + 1) % gui_params_ir.network_amount;
		gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
		ir_resize_motion_buffer();
	}
	//---------------------------------------------------------------
	// B: cycle to previous network
	if (key == GLFW_KEY_B && action == GLFW_PRESS) {
		gui_params_ir.prev_network_id = gui_params_ir.network_id;
		gui_params_ir.network_id = (gui_params_ir.network_id - 1);
		if (gui_params_ir.network_id == -1)
			gui_params_ir.network_id = gui_params_ir.network_amount - 1;
		gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
		ir_resize_motion_buffer();
	}
	//---------------------------------------------------------------
	// O: change output mode: single render target; 4 render targets
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
		gui_params_ir.displayMode = 1 - gui_params_ir.displayMode;
		if (gui_params_ir.displayMode == 0)
			gui_params_ir.currentRenderInfo = 17;
		else
			gui_params_ir.currentRenderInfo = 8;
	}
	//---------------------------------------------------------------
	// L: cycle through loaded point clouds
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
		gui_params_ir.lod = (gui_params_ir.lod+ 1) % gui_params_ir.lod_amount;
	//---------------------------------------------------------------
	// C: cycle to next channel/rendered texture etc
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		if (gui_params_ir.displayMode == 0) {
			gui_params_ir.currentRenderInfo = (gui_params_ir.currentRenderInfo + 1) % 18;
		}
		else {
			gui_params_ir.currentRenderInfo = (gui_params_ir.currentRenderInfo + 1) % 9;

		}
	//---------------------------------------------------------------
	// X: cycle to previous channel/rendered texture etc
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
		if (gui_params_ir.displayMode == 0) {
			gui_params_ir.currentRenderInfo = (gui_params_ir.currentRenderInfo -1) % 18;
			if (gui_params_ir.currentRenderInfo == -1) gui_params_ir.currentRenderInfo = 17;
		}
		else {
			gui_params_ir.currentRenderInfo = (gui_params_ir.currentRenderInfo - 1) % 9;
			if (gui_params_ir.currentRenderInfo == -1) gui_params_ir.currentRenderInfo = 8;

		}
	//---------------------------------------------------------------
	// T: toggle between the current and previously selected network
	if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		int tmp = gui_params_ir.network_id;
		gui_params_ir.network_id = gui_params_ir.prev_network_id;
		gui_params_ir.prev_network_id = tmp;
		gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];

	}
	//---------------------------------------------------------------
	// 0: start capturing a dataset for training
	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
		gui_params_ir.startCapturing = true;
	//---------------------------------------------------------------
	// 9: start capturing by index
	if (key == GLFW_KEY_9 && action == GLFW_PRESS)
		gui_params_ir.startCaptureByIndex = true;
	// //---------------------------------------------------------------
	// // 1: network preset 1
	// if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
	// 	gui_params_ir.network_id = 9;
	// 	gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
	// 	ir_resize_motion_buffer();
	// }
	// //---------------------------------------------------------------
	// // 2: network preset 2
	// if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
	// 	gui_params_ir.network_id = 10;
	// 	gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
	// 	ir_resize_motion_buffer();
	// }
	// //---------------------------------------------------------------
	// // 3: network preset 3
	// if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
	// 	gui_params_ir.network_id = 11;
	// 	gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
	// }
	//---------------------------------------------------------------
	// I: cycle to next ground truth pose
	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		gui_params_ir.increment_image = true;
	}
	//---------------------------------------------------------------
	// cycle to previous gt pose
	if (key == GLFW_KEY_U && action == GLFW_PRESS) {
		gui_params_ir.decrement_image = true;
	}
	//---------------------------------------------------------------
	// V: start capturing images for a video
	if (key == GLFW_KEY_V && action == GLFW_PRESS) {
		if (gui_params_ir.captureVideo) {
			gui_params_ir.captureVideo = !gui_params_ir.captureVideo;
		}
		else {
			gui_params_ir.startCaptureVideo = true;
			gui_params_ir.captureVideoIndex = 0;
			gui_params_ir.animationRunning = false;
		}
	}
}

void ir_mouse_button_callback(int button, int action, int mods) {
	if (ImGui::GetIO().WantCaptureMouse) return;
}

void ir_gui_callback(void) {
	ImGui::ShowMetricsWindow();
}

void InferenceRenderer::setCurrentCam(int id, bool test) {
	if (test) {
		dataset.currentCam = id % dataset.cam_names_test.size();
	}
	else {
		dataset.currentCam = id % dataset.camCount;
	}
	// load position and rotation from rgbcam
	glm::vec3 pos;
	glm::quat q;
	glm::mat4 v;
	auto& cam_views = test ? dataset.cam_views_test : dataset.cam_views;
	if (gui_params_ir.use_darius_optimized_positions && cam_views[dataset.currentCam].darius_optimized_pose_present) {
		q = ((cam_views[dataset.currentCam].optimized_pose.orientation));
		pos = cam_views[dataset.currentCam].optimized_pose.position;
	}
	else {
		q = ((cam_views[dataset.currentCam].pose.orientation));
		pos = cam_views[dataset.currentCam].pose.position;
	}
	glm::mat4 mat_v = (glm::mat4_cast(q));
	
	current_camera()->pos = pos;
	current_camera()->dir = -glm::normalize(glm::vec3(mat_v[2]));
	current_camera()->up =  glm::normalize(glm::vec3(mat_v[1]));
	
	current_camera()->update();
	if (gui_params_ir.use_dataset_proj) {
		if(gui_params_ir.use_dataset_proj_cropped) current_camera()->proj = dataset.gt_proj_cropped;
		else current_camera()->proj = dataset.gt_proj;
	}
	current_camera()->moved = true;
}

glm::mat4 InferenceRenderer::getView(int id, bool test) {
	auto& cam_views = test ? dataset.cam_views_test : dataset.cam_views;
	if (gui_params_ir.use_darius_optimized_positions && cam_views[id].darius_optimized_pose_present) {
		return cam_views[id].optimized_pose.view;
	}
	return cam_views[id].pose.view;


	
}


void InferenceRenderer::loadPointClouds(std::vector<PointCloud>& pcs, std::vector<std::string> files) {
	//----------------------------------------------------------------------
	// load point clouds
	// extract pointclouddata to single vectors
	std::vector<vec3> pc_position;
	std::vector<vec3> pc_color;
	std::vector<vec3> pc_normal;
	std::vector<float> pc_curvature;
	std::vector<int> pc_timestamp;
	std::vector<uint32_t> pc_index;
	std::vector<PointCloudVoxel> bounding_structure;
	bool load_lod = true;
	std::pair<vec3, vec3> aabb;
	std::cerr << "##################################################################################################################" << std::endl;
	if (load_lod) {
		bool get_extent = true;
		if (setType[dataset_id] == "KITTY-360") {
			std::cerr << "[InferenceRenderer] Parse Kitty-360 ply files." << std::endl;

			pointCloud_filenames.resize(1);

			files.resize(0);
			std::string path = setFolder[dataset_id] + "/data_3d_semantics/train/" + setInfo[dataset_id] + "/dynamic/";

			for (const auto& entry : Helper::get_directory_entries_sorted(path)) {

				files.emplace_back("/static/" + entry.path().filename().string());
				files.emplace_back("/dynamic/" + entry.path().filename().string());
			}
			/*std::sort(files.begin(), files.end());
			*/for(auto& file : files){
				std::cout <<  file << std::endl;
			}
			pointCloud_filenames[0] = files[setKittyPCRange.first].substr(8, 18).c_str() + std::string("_") + files[setKittyPCRange.second*2].substr(19, 29).c_str();
			// load pointcloud to model

			int current_id = 0;
			std::pair<int, int> setKittyPCRangeDouble = std::pair<int, int>(setKittyPCRange.first * 2, setKittyPCRange.second * 2);
			PLYPointCloudParser plyParser;
			for (std::string file : files) {
				if (current_id < setKittyPCRangeDouble.first) {
					++current_id;
					continue;
				};
				if (current_id > setKittyPCRangeDouble.second + 1) break;
				std::cerr << "[InferenceRenderer] Parse PLY file " << file << std::endl;


				// load all chosen kitty pointclouds in the given range
				plyParser = PLYPointCloudParser(setFolder[dataset_id] + "/data_3d_semantics/train/" + setInfo[dataset_id] + file, nearest_views, setType[dataset_id], false);
				if (plyParser.vertexCount == 0) {
					continue;
				}
				plyParser.createBoundingStructureGrid(20.f);

				if (get_extent) {
					aabb = plyParser.getBoundingBox(); // get bounding box for the bigegst point cloud
					get_extent = false;
				}

				//manipulate boundung structure
				for (auto& bs : plyParser.bounding_structure) {
					bs.start += pc_position.size();
					bounding_structure.push_back(bs);
				}

				int index = pc_position.size();
				for (auto& p : plyParser.points) {
					vec3 pos = p.pos;
					pc_position.push_back(pos);
					pc_color.push_back(p.color);
					vec3 norm = p.normal;
					pc_normal.push_back(norm);
					pc_curvature.push_back(p.curvature);
					pc_timestamp.push_back(p.timestamp);
					pc_index.push_back(index);
					++index;
				}
				++current_id;
				plyParser.clear();
			}
			aabb = std::pair<vec3, vec3>(vec3(-10000, -10000, -10000), vec3(10000, 10000, 10000));
			// old try without geometry wrapper
			pcs.emplace_back("PointCloud" + pointCloud_filenames[0]);

			int i = pcs.size() - 1;
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_position.size(), pc_position.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_color.size(), pc_color.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_normal.size(), pc_normal.data());
			pcs[i]->add_vertex_buffer(GL_FLOAT, 1, pc_curvature.size(), pc_curvature.data());
			pcs[i]->add_vertex_buffer(GL_INT, 1, pc_timestamp.size(), pc_timestamp.data());
			pcs[i]->add_index_buffer(pc_index.size(), pc_index.data());
			pcs[i]->add_bounding_structure(bounding_structure);
			pcs[i]->set_primitive_type(GL_POINTS);

			std::cout << "[InferenceRenderer] PointCloud has " << pc_position.size() << " points." << std::endl;

			//clear ram in parser
			plyParser.clear();
			pc_position.clear();
			pc_color.clear();
			pc_normal.clear();
			pc_curvature.clear();
			pc_timestamp.clear();
			pc_index.clear();
			std::cerr << "[InferenceRenderer] Finished parsing Kitty-360 ply files." << std::endl;

		}
		else if (setType[dataset_id] == "Redwood" || setType[dataset_id] == "ScanNet" || setType[dataset_id] == "Generic") {
			if (setType[dataset_id] == "Redwood")
				files = std::vector<std::string>{ "pointcloud_timestamp_te_1_vs_0.01_jit.ply" , "pointcloud_timestamp_te_10_vs_0.01_jit.ply", "pointcloud_te_10_vs_0.01_jit.ply" }; //test.ply" , "test2.ply"};// { "pointcloud_te_10_vs_0.01_jit.ply", "pointcloud_te_20_vs_0.01_jit.ply"};
			if (setType[dataset_id] == "ScanNet")
				files = std::vector<std::string>{ "pointcloud_timestamp_te_1_vs_0.01_jit.ply" , "pointcloud_timestamp_te_10_vs_0.01_jit.ply", "pointcloud_te_1_vs_0.01_jit.ply" };
            if (setType[dataset_id] == "Generic")
                files = std::vector<std::string>{ "pointcloud_timestamp_te_1_vs_0.01_jit.ply" , "pointcloud_timestamp_te_1_vs_0.01_jit_down4.ply", "pointcloud_timestamp_te_1_vs_0.01_jit_down8.ply" };
			pointCloud_filenames = {"lod0","lod1","sparse"};

			for (std::string file : files) {
				std::cerr << "[InferenceRenderer] Parse PLY file " << file << std::endl;
				//PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset_id) + "_lod_0_part" + std::to_string(i) + ".ply");
				PLYPointCloudParser plyParser = PLYPointCloudParser(setFolder[dataset_id] + "geometry/" + file, nearest_views, setType[dataset_id], true);

				plyParser.createBoundingStructureGrid(2000.f);

				if (get_extent) {
					aabb = plyParser.getBoundingBox(); // get bounding box for the bigegst point cloud
					get_extent = false;
				}
				int index = 0;

				for (auto& p : plyParser.points) {
					vec3 pos = p.pos;// vec3(p.pos[1], p.pos[2], p.pos[0]); // axis correction now in the parser
					pc_position.push_back(pos);
					pc_color.push_back(p.color);
					vec3 norm = p.normal;// vec3(p.normal[1], p.normal[2], p.normal[0]); // axis correction now in the parser
					pc_normal.push_back(norm);
					pc_curvature.push_back(p.curvature);
					pc_timestamp.push_back(p.timestamp);
					pc_index.push_back(index);
					++index;
				}

				// load pointcloud to model
				pcs.emplace_back("PointCloud" + file);
				// old try without geometry wrapper
				int i = pcs.size() - 1;
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_position.size(), pc_position.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_color.size(), pc_color.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_normal.size(), pc_normal.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 1, pc_curvature.size(), pc_curvature.data());
				pcs[i]->add_vertex_buffer(GL_INT, 1, pc_timestamp.size(), pc_timestamp.data());
				pcs[i]->add_index_buffer(pc_index.size(), pc_index.data());
				pcs[i]->add_bounding_structure(plyParser.bounding_structure);

				pcs[i]->set_primitive_type(GL_POINTS);

				std::cout << "[InferenceRenderer] PointCloud has " << pc_position.size() << " points." << std::endl;

				//clear ram in parser
				plyParser.clear();
				pc_position.clear();
				pc_color.clear();
				pc_normal.clear();
				pc_curvature.clear();
				pc_timestamp.clear();
				pc_index.clear();

			}
		}
		else {
			for (std::string file : files) {
				std::cerr << "[InferenceRenderer] Parse PLY file " << file << std::endl;
				//PLYPointCloudParser plyParser("../../../set" + std::to_string(dataset_id) + "_lod_0_part" + std::to_string(i) + ".ply");
				PLYPointCloudParser plyParser = PLYPointCloudParser("../../../" + setName[dataset_id] + "_" + file + ".ply",nearest_views, setType[dataset_id], false);
				
				plyParser.createBoundingStructureGrid(2.f);

				if (get_extent) {
					aabb = plyParser.getBoundingBox(); // get bounding box for the bigegst point cloud
					get_extent = false;
				}
				int index = 0;
				
				for (auto& p : plyParser.points) {
					vec3 pos = p.pos;// vec3(p.pos[1], p.pos[2], p.pos[0]); // axis correction now in the parser
					pc_position.push_back(pos);
					pc_color.push_back(p.color);
					vec3 norm = p.normal;// vec3(p.normal[1], p.normal[2], p.normal[0]); // axis correction now in the parser
					pc_normal.push_back(norm);
					pc_curvature.push_back(p.curvature);
					pc_index.push_back(index);
					++index;
				}

				// load pointcloud to model
				pcs.emplace_back("PointCloud" + file);
				// old try without geometry wrapper
				int i = pcs.size() - 1;
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_position.size(), pc_position.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_color.size(), pc_color.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 3, pc_normal.size(), pc_normal.data());
				pcs[i]->add_vertex_buffer(GL_FLOAT, 1, pc_curvature.size(), pc_curvature.data());
				pcs[i]->add_index_buffer(pc_index.size(), pc_index.data());
				pcs[i]->add_bounding_structure(plyParser.bounding_structure);

				pcs[i]->set_primitive_type(GL_POINTS);

				std::cout << "[InferenceRenderer] PointCloud has " << pc_position.size() << " points." << std::endl;

				//clear ram in parser
				plyParser.clear();
				pc_position.clear();
				pc_color.clear();
				pc_normal.clear();
				pc_curvature.clear();
				pc_index.clear();

				

			}
		}
	}
	gui_params_ir.aabb_extend = length(aabb.first - aabb.second);
}

// ------------------------------------------
// run
void InferenceRenderer::run(int argc, char** argv) {
	std::filesystem::create_directory("./out");
	std::ofstream timingFile;
	timingFile.open("./out/timings.csv");
	timingFile << "Inference;PointRendering;MipMapping;Total" << std::endl;


	// adjust these parameters to change main functionalities

	//int vertical_resolution = gui_params_ir.vertical_resolution; // 768; //1536;//990;

	//----------------------------------------------------------------------
	// init GL
	std::cerr << "[InferenceRenderer] Init GL" << std::endl;
	ContextParameters params;
	
	params.width = gui_params_ir.initial_resolution_default.x;
	params.height = gui_params_ir.initial_resolution_default.y;
	params.width *= 4;
	params.height *= 4;
	
	params.title = "Inovis";
	params.swap_interval = 0;
	Context::init(params);	
	params.width /= 4;
	params.height /= 4;
	Context::set_keyboard_callback(ir_keyboard_callback);
	Context::set_mouse_button_callback(ir_mouse_button_callback);
	gui_add_callback("example_gui_callback", ir_gui_callback);

	TimerQuery timer = TimerQuery("renderTimer");
	TimerQuery timerPreprocess = TimerQuery("Preprocessing");
	TimerQueryGL timerRenderPC = TimerQueryGL("Render Point Cloud");
	TimerQueryGL timerMipMap = TimerQueryGL("MipMap");
	TimerQueryGL timerInference = TimerQueryGL("Inference");
	TimerQueryGL timerBlit = TimerQueryGL("Blit");
	enable_gl_debug_output();
	//----------------------------------------------------------------------
    std::cerr << "Working Directory: " << std::filesystem::current_path() << std::endl;

    //----------------------------------------------------------------------
	// Parse config files of datasets
	std::cerr << "##################################################################################################################\n[InferenceRenderer] Parse Datasets" << std::endl;
	std::string datasets_path = "../../datasets/";


	std::vector<std::filesystem::directory_entry> dataset_files = Helper::get_directory_entries_sorted((datasets_path));
	/*for (const auto& dataset_file : std::filesystem::directory_iterator(datasets_path)) {
		dataset_files.push_back(dataset_file);
	}
    std::cout << dataset_files << std::endl;
	std::sort(dataset_files.begin(), dataset_files.end());
    std::cout << dataset_files << std::endl;*/
	for(const auto& dataset_file : dataset_files) {	
		if (dataset_file.path().extension() != ".txt") continue;
		//load camera poses

		std::ifstream dataset_stream(dataset_file.path());

		if (!dataset_stream.is_open())
			std::cerr << "[InferenceRenderer] FATAL ERROR: failed to open filestream: " << dataset_file.path().string() << "." << std::endl;
		std::string dummy, d_name, d_folder, d_type, d_info;
		int d_size, d_width, d_height;
		dataset_stream >> dummy >> d_name;
		dataset_stream >> dummy >> d_folder;
		dataset_stream >> dummy >> d_type;
		dataset_stream >> dummy >> d_size;
		dataset_stream >> dummy >> d_info;
		dataset_stream >> dummy >> d_width;
		dataset_stream >> dummy >> d_height;
		if (!std::filesystem::exists(d_folder)) std::cerr << "[InferenceRenderer] WARNING: Dataset folder does not exists: " << d_folder << "." << std::endl;
		if (d_type != "NavVis" && d_type != "TanksAndTemples"&& d_type != "KITTY-360" && d_type != "L" && d_type != "Redwood" && d_type != "ScanNet" && d_type != "Generic") std::cerr << "[InferenceRenderer] WARNING: Dataset Type not found: " << d_type<< "." << std::endl;
		if (d_size < 0) std::cerr << "[InferenceRenderer] WARNING: Dataset size is negative: " << d_size << "." << std::endl;
		if (d_width % 32 != 0) {
			std::cerr << "[InferenceRenderer] WARNING: Dataset GT image width is not divisible by 32: " << d_width << "." << std::endl; 
		}
		if (d_height % 32 != 0) {
			std::cerr << "[InferenceRenderer] WARNING: Dataset GT image height is not divisible by 32: " << d_height << "." << std::endl;
		}

		setName.push_back(d_name);
		setFolder.push_back(d_folder);
		setType.push_back(d_type);
		setSize.push_back(d_size);
		setInfo.push_back(d_info);
		setWidth.push_back(d_width);
		setHeight.push_back(d_height);

		dataset_stream.close();

		std::cout << "[InferenceRenderer] Found dataset from file: " << dataset_file.path() << " \tname: " << d_name<< " \ttype:" << d_type << " \tsize: " << d_size<< " \twidth: " << d_width  << " \theight: " << d_height << " \tfolder: " << d_folder << " \tadditional info: " << d_info << std::endl;
	}
	targetWidth = int(setWidth[dataset_id] ) ;
	targetHeight = int(setHeight[dataset_id]);
	//----------------------------------------------------------------------
	
	//----------------------------------------------------------------------
	// Parse config files of networks
	std::cerr << "##################################################################################################################\n[InferenceRenderer] Parse Networks" << std::endl;
	std::string networks_path = "../../networks/";

	std::vector<std::filesystem::directory_entry> network_files = Helper::get_directory_entries_sorted((networks_path));
	/*for (const auto& network_file : std::filesystem::directory_iterator(networks_path)) {
		network_files.push_back(network_file);
	}
	std::sort(network_files.begin(), network_files.end());*/
	for (const auto& network_file : network_files) {

		if (network_file.path().extension() != ".txt") continue;
		//load camera poses

		std::ifstream network_stream(network_file.path());
		
		if (!network_stream.is_open())
			std::cerr << "[InferenceRenderer] FATAL ERROR: failed to open filestream: " << network_file.path().string() << "." << std::endl;
		std::string dummy, n_name;
		int n_feature_extraction_depth, n_movec_channels, n_groundtruth_amount;
		bool n_prevInitMV;
		network_stream >> dummy >> n_name;
		network_stream >> dummy >> n_feature_extraction_depth;
		network_stream >> dummy >> n_movec_channels;
		network_stream >> dummy >> n_groundtruth_amount;
		network_stream >> dummy >> n_prevInitMV;

		if (n_feature_extraction_depth !=0) std::cerr << "[InferenceRenderer] WARNING: Other extraction depths than 0 are deprecated: " << n_feature_extraction_depth << "." << std::endl;
		if (n_movec_channels < 2 || n_movec_channels > 3) std::cerr << "[InferenceRenderer] WARNING: Other movec channels than 2 or 3 are not supported: " << n_movec_channels << "." << std::endl;
		if (n_groundtruth_amount < 1 || n_groundtruth_amount > 6) std::cerr << "[InferenceRenderer] WARNING: Other ground truth amounts than 1 to 6 are not supported: " << n_groundtruth_amount << "." << std::endl;
		
		gui_params_ir.network_filenames.push_back(n_name);
		gui_params_ir.network_feature_extraction_depth.push_back(n_feature_extraction_depth);
		gui_params_ir.network_movec_channels.push_back(n_movec_channels);
		gui_params_ir.network_groundtruth_amount.push_back(n_groundtruth_amount);
		gui_params_ir.network_prevInitMV.push_back(n_prevInitMV);

		network_stream.close();

		std::cout << "[InferenceRenderer] Found network from file: " << network_file.path() << " \tname: " << n_name << " \tdepth:" << n_feature_extraction_depth << " \tmv_channels: " << n_movec_channels << " \tgt amount: " << n_groundtruth_amount << " \tpreInitMV: " << n_prevInitMV << std::endl;
	}

	//----------------------------------------------------------------------


	//----------------------------------------------------------------------
	// gui for dataset selection
	custom_gui_select_dataset();

	//----------------------------------------------------------------------
	// init resolutions etc
    std::cout << "[InferenceRenderer] Chose dataset with id " << dataset_id << ": " <<  setName[dataset_id] << " with render resolution: " << targetWidth << " x " << targetHeight << std::endl;
	gui_params_ir.initial_resolution_default = glm::ivec2(targetWidth, targetHeight);
	if (gui_params_ir.initial_resolution_default.x % 32 != 0 || gui_params_ir.initial_resolution_default.y % 32 != 0) {
		// chosen dataset with dimension not divisible by 32 -> inference would crash
		std::cout << "[InferenceRenderer] WARNING: You chose a dataset with image sizes not divisible by 32 -> no inference is done." << std::endl;
		if (enforceTargetResolution) {
			std::cout << "[InferenceRenderer] WARNING: Target Resolution is enforced -> disable Inference: " << targetWidth << " x " << targetHeight << "." << std::endl;
			doInference = false;
		}
		else {
			targetWidth = int(targetWidth / 32) * 32;
			targetHeight = int(targetHeight / 32) * 32;
			gui_params_ir.initial_resolution_default = glm::ivec2(targetWidth, targetHeight);
			doInference = true;
			std::cout << "[InferenceRenderer] WARNING: Target Resolution is not enforced -> target resolutiuon is floored to a value divisible by 32: " << targetWidth << " x " << targetHeight << "." << std::endl;

		}

	}


	gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
	//----------------------------------------------------------------------

	//----------------------------------------------------------------------
	// shader 
	// multi rendertarget shader
	Shader drawPCmultiShader("drawPCmulti", "shader/drawPCmulti.vs", "shader/drawPCmulti.fs");
	Shader drawPCmultiMotionShader("drawPCmultiMotion", "shader/drawPCmultiMotion.vs", "shader/drawPCmultiMotion.fs");
	Shader drawPCmultiMotionOnly("drawPCmultiMotionOnly", "shader/drawPCmotionMultiOnly.vs", "shader/drawPCmotionMultiOnly.fs");
	
	// Shader to preinit movecs used with screenspace quad
	Shader initMoVecs("initMoVecs", "shader/initMoVecs.vs", "shader/initMoVecs.fs"); // for standard operation
	Shader initMoVecsOnly("initMoVecsOnly", "shader/initMoVecs.vs", "shader/initMoVecsOnly.fs"); // for disabled mipmapping -> for movec only

	// culling compute Shader
	Shader computeFrustumCullingShader("computeFrustumCulling", "shader/computeFrustumCulling.glcs");
	//----------------------------------------------------------------------

	glPointSize(std::max(1, gui_params_ir.pointSizeGL));// std::max(1, int(gui_params_ir.pointSizeGL * gui_params_ir.lodPointSizesGL[gui_params_ir.lod])));
	

	//----------------------------------------------------------------------
	// Setup Framebuffer
	Context::set_resize_callback(ir_resize_callback);
	// setup fbo
	float mod = 2.0 * glm::pow(0.5, gui_params_ir.resolution_modifier);
	gui_params_ir.res0 = ivec2(gui_params_ir.initial_resolution_default.x *mod, gui_params_ir.initial_resolution_default.y *mod);

	// extra fbo for target. normally only texture is used but handy for resizing
	Framebuffer fbo_out_1 = Framebuffer("fbo_out_1", gui_params_ir.res0.x, gui_params_ir.res0.y);
	fbo_out_1->attach_depthbuffer(Texture2D("fbo_out_1/depth", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_out_1->attach_colorbuffer(Texture2D("fbo_out_1/col", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_out_1->attach_colorbuffer(Texture2D("fbo_out_1/depthcol", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_out_1->check();

	Framebuffer fbo_out_2 = Framebuffer("fbo_out_2", gui_params_ir.res0.x, gui_params_ir.res0.y);
	fbo_out_2->attach_depthbuffer(Texture2D("fbo_out_2/depth", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_out_2->attach_colorbuffer(Texture2D("fbo_out_2/col", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_out_2->attach_colorbuffer(Texture2D("fbo_out_2/depthcol", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_out_2->check();

	// 2 output buffer to switch between the two buffers and use the other one as a previous image for TAA
	Framebuffer fbo_out = fbo_out_1;
	Framebuffer fbo_prev = fbo_out_2;
	cur_out_fbo = fbo_out->name;

	Framebuffer fbo_res0 = Framebuffer("fbo_res0", gui_params_ir.res0.x, gui_params_ir.res0.y);
	fbo_res0->attach_depthbuffer(Texture2D("fbo_res0/depth", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	// maximum number of color attachments should be 8 -> 1: col, 1: depthcol, 6: motion 
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/col", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/depthcol", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion1", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion2", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion3", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion4", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion5", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->attach_colorbuffer(Texture2D("fbo_res0/motion6", gui_params_ir.res0.x, gui_params_ir.res0.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res0->check();

	gui_params_ir.res1 = glm::ivec2(gui_params_ir.res0.x / 2, gui_params_ir.res0.y / 2);
	Framebuffer fbo_res1 = Framebuffer("fbo_res1", gui_params_ir.res1.x, gui_params_ir.res1.y);
	fbo_res1->attach_depthbuffer(Texture2D("fbo_res1/depth", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/col", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/depthcol", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion1", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion2", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion3", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion4", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion5", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->attach_colorbuffer(Texture2D("fbo_res1/motion6", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res1->check();
	gui_params_ir.res2 = glm::ivec2(gui_params_ir.res1.x / 2, gui_params_ir.res1.y / 2);
	Framebuffer fbo_res2 = Framebuffer("fbo_res2", gui_params_ir.res2.x, gui_params_ir.res2.y);
	fbo_res2->attach_depthbuffer(Texture2D("fbo_res2/depth", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/col", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/depthcol", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion1", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion2", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion3", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion4", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion5", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->attach_colorbuffer(Texture2D("fbo_res2/motion6", gui_params_ir.res2.x, gui_params_ir.res2.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res2->check();
	gui_params_ir.res3 = glm::ivec2(gui_params_ir.res2.x / 2, gui_params_ir.res2.y / 2);
	Framebuffer fbo_res3 = Framebuffer("fbo_res3", gui_params_ir.res3.x, gui_params_ir.res3.y);
	fbo_res3->attach_depthbuffer(Texture2D("fbo_res3/depth", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/col", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/depthcol", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion1", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion2", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion3", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion4", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion5", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->attach_colorbuffer(Texture2D("fbo_res3/motion6", gui_params_ir.res3.x, gui_params_ir.res3.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_res3->check();

    std::cout << "[InferenceRenderer] Setup framebuffers with resolutions " << gui_params_ir.res0 << ", " << gui_params_ir.res1 << ", " << gui_params_ir.res2 << ", "  << gui_params_ir.res3 << std::endl;

	auto& res_motion = gui_params_ir.res0;
	switch (gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]) {
	case 0:
		res_motion = gui_params_ir.res0;
		break;
	case 1:
		res_motion = gui_params_ir.res1;
		break;
	case 2:
		res_motion = gui_params_ir.res2;
		break;
	case 3:
		res_motion = gui_params_ir.res3;
		break;
	default:
		std::cerr << "[ir_resize_motion_buffer] FATAL ERROR: feature extraction depth not recognized." << std::endl;
	}

	Framebuffer fbo_motion = Framebuffer("fbo_motion", res_motion.x, res_motion.y);
	fbo_motion->attach_depthbuffer(Texture2D("fbo_motion/depth", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion1", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion2", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion3", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion4", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion5", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->attach_colorbuffer(Texture2D("fbo_motion/motion6", gui_params_ir.res1.x, gui_params_ir.res1.y, GL_RGBA32F, GL_RGBA, GL_FLOAT));
	fbo_motion->check();
	//----------------------------------------------------------------------
	//Load Dataset
	dataset.load(0, setSize[dataset_id], setType[dataset_id], setFolder[dataset_id], setInfo[dataset_id], setKittyPCRange, gui_params_ir.initial_resolution_default, setGenericTestStartStep);
	nearest_views = dataset.getCaptureViewList();
	// setup test cycling if test poses were parsed
	if (dataset.cam_views_test.size() > 0) {
		gui_params_ir.parsedTestImages = true;
		gui_params_ir.cycleTestImages = true;
	}
	gui_params_ir.captureByIndexEnd = dataset.camCount;
	//----------------------------------------------------------------------
	
	//----------------------------------------------------------------------
	// load clouds
	gui_params_ir.lod_amount = pointCloud_filenames.size();
	std::vector<PointCloud> pointClouds;
	loadPointClouds(pointClouds, pointCloud_filenames);
	gui_params_ir.lod_amount = pointCloud_filenames.size();
	//----------------------------------------------------------------------
	
	//----------------------------------------------------------------------
	// parse position and orientation
	setCurrentCam(dataset.currentCam, gui_params_ir.cycleTestImages);
	//----------------------------------------------------------------------
	// setup camera
	current_camera()->near = dataset.camera_near;
	current_camera()->far = dataset.camera_far;
	current_camera()->fix_up_vector = false;
	current_camera()->fov_degree = dataset.camera_fov_degree;
	

	gui_params_ir.fov = current_camera()->fov_degree;
	gui_params_ir.campos = current_camera()->pos;
	
	//----------------------------------------------------------------------

	//old view matrix for motion vecs
	view_old = current_camera()->view;
    //----------------------------------------------------------------------
	// Animations for automatic random camera movement for dataset_id creation
	// example animation
	
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

	/*myAnimation->push_node(vec3(2.665753, -0.076543, 0.913379), glm::quat(0.414161, {-0.534316, 0.591737, 0.439119}));
	myAnimation->push_node(vec3(6.391758, -0.401054, 1.336774), glm::quat(0.472657, { -0.477869, 0.537325, 0.509429 }));
	myAnimation->push_node(vec3(6.914591, -3.419446, 1.262339), glm::quat(0.039058, { -0.028144, 0.725134, 0.686923 }));
	myAnimation->push_node(vec3(9.152531, -5.970881, 1.218955), glm::quat(0.502979, { -0.493643, 0.497307, 0.505978 }));
	myAnimation->push_node(vec3(13.368887, -2.637278, 1.245593), glm::quat(-0.699451, { 0.714669, 0.003374, 0.002382 }));
	myAnimation->push_node(vec3(13.535895, 3.623169, 1.364535), glm::quat(-0.667468, { 0.668829, 0.229090, 0.233822 }));
	myAnimation->push_node(vec3(9.596127, 4.416342, 1.007502), glm::quat(-0.480831, { 0.537281, 0.494064, 0.485831 }));
	myAnimation->push_node(vec3(7.069435, 0.990500, 0.939071), glm::quat(-0.349542, { 0.367205, 0.624332, 0.594298 }));*/
	standardAnimation = Animation("standardAnimation"); 
	captureAnimation = Animation("captureAnimation"); 
	customAnimation = Animation ("customAnimation");
	if (setType[dataset_id] == "NavVis"){
		captureAnimation->push_node(vec3(2.665753, -0.076543, 0.913379), glm::quat(0.414161, { -0.534316, 0.591737, 0.439119 }));
		captureAnimation->push_node(vec3(6.391758, -0.401054, 1.336774), glm::quat(0.472657, { -0.477869, 0.537325, 0.509429 }));
		captureAnimation->push_node(vec3(6.914591, -3.419446, 1.262339), glm::quat(0.039058, { -0.028144, 0.725134, 0.686923 }));
		captureAnimation->push_node(vec3(9.152531, -5.970881, 1.218955), glm::quat(0.502979, { -0.493643, 0.497307, 0.505978 }));
		captureAnimation->push_node(vec3(13.368887, -2.637278, 1.245593), glm::quat(-0.699451, { 0.714669, 0.003374, 0.002382 }));
		captureAnimation->push_node(vec3(13.535895, 3.623169, 1.364535), glm::quat(-0.667468, { 0.668829, 0.229090, 0.233822 }));
		captureAnimation->push_node(vec3(9.596127, 4.416342, 1.007502), glm::quat(-0.480831, { 0.537281, 0.494064, 0.485831 }));
		captureAnimation->push_node(vec3(7.069435, 0.990500, 0.939071), glm::quat(-0.349542, { 0.367205, 0.624332, 0.594298 }));
		myAnimation = captureAnimation;
	}
	else if (setType[dataset_id] == "TanksAndTemples") {
		std::cout << "create TaT Animation" << std::endl;
		captureAnimation->push_node(vec3(0.641492, -0.244242, 4.979766), glm::quat(0.007178, { -0.012478, -0.039491, 0.999116 }));
		captureAnimation->push_node(vec3(-1.591853, -0.298102, 4.693552), glm::quat(-0.001370, { -0.190917, -0.040512, 0.980769 }));
		captureAnimation->push_node(vec3(-3.633180, -0.231690, 2.703552), glm::quat(-0.004191, { -0.474984, -0.046964, 0.878731 }));
		captureAnimation->push_node(vec3(-4.247937, -0.103241, 0.274043), glm::quat(-0.005074, { 0.746414, 0.058722, -0.662866 }));
		captureAnimation->push_node(vec3(-3.419804, 0.094751, -2.621783), glm::quat(-0.005196, { 0.907850, 0.041693, -0.417185 }));
		captureAnimation->push_node(vec3(-0.731325, 0.286665, -4.564012), glm::quat(-0.002212, { 0.995996, 0.020900, -0.086893 }));
		captureAnimation->push_node(vec3(1.539815, 0.272616, -3.402226), glm::quat(-0.003118, { 0.993821, 0.008525, 0.110621 }));
		captureAnimation->push_node(vec3(3.389049, 0.252575, -2.434031), glm::quat(0.005636, { 0.942075, -0.015641, 0.334991 }));
		captureAnimation->push_node(vec3(4.270007, 0.174444, -1.031549), glm::quat(0.003241, { 0.846209, -0.030146, 0.531988 }));
		captureAnimation->push_node(vec3(3.000188, -0.164846, 2.535679), glm::quat(0.011764, { 0.434675, -0.057720, 0.898659 }));
		//captureAnimation->push_node(vec3(3.279879, -0.080976, 0.855173), glm::quat(0.006410, { 0.558298, -0.056374, 0.827698 }));
		//captureAnimation->push_node(vec3(2.502036, -0.247430, 2.588083), glm::quat(0.011532, { 0.285985, -0.058852, 0.956356 }));
		captureAnimation->push_node(vec3(0.641492, -0.244242, 4.979766), glm::quat(0.007178, { -0.012478, -0.039491, 0.999116 }));
		myAnimation = captureAnimation;

	}
	else {
		int step = 1;
		//if (setType[dataset_id] == "Redwood" || setType[dataset_id] == "ScanNet") step = 1;
		for (int anim_frame = 0; anim_frame <= dataset.camCount; anim_frame += step) {
			glm::mat4 view_of_frame = (getView(anim_frame));

			standardAnimation->push_node(dataset.cam_views[anim_frame].pose.position, glm::quat_cast(view_of_frame));
			if (anim_frame % gui_params_ir.captureVideoSkipControlPoint == 0) {
				captureAnimation->push_node(dataset.cam_views[anim_frame].pose.position, glm::quat_cast(view_of_frame));
			}
		}
		myAnimation = standardAnimation;
	}

    //customAnimation->push_node(dataset.cam_views[0].pose.position, glm::quat_cast(getView(0)));
    //customAnimation->push_node(dataset.cam_views[dataset.cam_views.size()-1].pose.position, glm::quat_cast(getView(dataset.cam_views.size()-1)));

    custom_to_standard_factor = 1.f;//float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

    customAnimation->ms_between_nodes = 1000.f / custom_to_standard_factor;

    if(setName[dataset_id] == "kitty00") {
        // kitti_00_4 animation
        /*customAnimation->push_node(vec3(1223.37, 3899.06, 116.111), glm::quat(0.73374 , {-0.650553, 0.142913, 0.134094}));
        customAnimation->push_node(vec3(1226.11, 3905.38, 116.08), glm::quat(0.734772 , {-0.653022, 0.133353, 0.12605}));
        customAnimation->push_node(vec3(1228.52, 3910.86, 116.054), glm::quat(0.734865 , {-0.649279, 0.143678, 0.133292}));
        customAnimation->push_node(vec3(1231.05, 3915.38, 116.065), glm::quat(0.722613 , {-0.641763, 0.184153, 0.179049}));*/
        customAnimation->push_node(vec3(1223.37, 3899.06, 116.111),
                                   glm::quat(0.73374, {-0.650553, 0.142913, 0.134094}));
        customAnimation->push_node(vec3(1224.8, 3902.29, 116.091),
                                   glm::quat(0.734461, {-0.652185, 0.135302, 0.130059}));
        customAnimation->push_node(vec3(1226.11, 3905.38, 116.08), glm::quat(0.734772, {-0.653022, 0.133353, 0.12605}));
        customAnimation->push_node(vec3(1227.34, 3908.24, 116.067),
                                   glm::quat(0.735614, {-0.651408, 0.134893, 0.12784}));
        customAnimation->push_node(vec3(1228.52, 3910.86, 116.054),
                                   glm::quat(0.734865, {-0.649279, 0.143678, 0.133292}));
        customAnimation->push_node(vec3(1229.72, 3913.24, 116.058),
                                   glm::quat(0.728033, {-0.649744, 0.160767, 0.148169}));
        customAnimation->push_node(vec3(1231.05, 3915.38, 116.065),
                                   glm::quat(0.722613, {-0.641763, 0.184153, 0.179049}));

        customAnimation->push_node(vec3(1232.52, 3917.2, 116.073),
                                   glm::quat(0.711502, {-0.633229, 0.209606, 0.221022}));
        customAnimation->push_node(vec3(1234.06, 3918.68, 116.088),
                                   glm::quat(0.691109, {-0.625004, 0.24564, 0.267208}));
        customAnimation->push_node(vec3(1235.8, 3919.85, 116.098), glm::quat(0.66739, {-0.605566, 0.285314, 0.326308}));
        customAnimation->push_node(vec3(1237.65, 3920.72, 116.089),
                                   glm::quat(0.638301, {-0.577403, 0.338016, 0.380687}));
        customAnimation->push_node(vec3(1239.61, 3921.21, 116.096),
                                   glm::quat(0.59951, {-0.543961, 0.391838, 0.437216}));
        customAnimation->push_node(vec3(1241.66, 3921.33, 116.094),
                                   glm::quat(0.555348, {-0.504952, 0.440468, 0.492545}));
        customAnimation->push_node(vec3(1243.74, 3921.08, 116.089), glm::quat(0.5101, {-0.461982, 0.484632, 0.53991}));
        customAnimation->push_node(vec3(1245.82, 3920.55, 116.086),
                                   glm::quat(0.471279, {-0.421056, 0.523407, 0.571536}));
        customAnimation->push_node(vec3(1247.93, 3919.76, 116.104),
                                   glm::quat(0.43908, {-0.387467, 0.551182, 0.594371}));
        customAnimation->push_node(vec3(1250.04, 3918.72, 116.116),
                                   glm::quat(0.410879, {-0.360598, 0.569501, 0.613854}));
        customAnimation->push_node(vec3(1253.31, 3916.9, 116.141), glm::quat(0.393793, {-0.351342, 0.572093, 0.62785}));
        customAnimation->push_node(vec3(1256.84, 3915.05, 116.15),
                                   glm::quat(0.399941, {-0.354385, 0.570766, 0.623445}));
        customAnimation->push_node(vec3(1260.52, 3913.2, 116.161),
                                   glm::quat(0.404858, {-0.354812, 0.570355, 0.620398}));
        customAnimation->push_node(vec3(1264.21, 3911.36, 116.182),
                                   glm::quat(0.408534, {-0.35164, 0.571676, 0.618576}));
        customAnimation->push_node(vec3(1267.83, 3909.51, 116.219),
                                   glm::quat(0.402498, {-0.350076, 0.580229, 0.615448}));
        customAnimation->push_node(vec3(1271.39, 3907.61, 116.243),
                                   glm::quat(0.404631, {-0.346434, 0.570217, 0.625387}));
        customAnimation->push_node(vec3(1274.86, 3905.88, 116.224),
                                   glm::quat(0.410613, {-0.350887, 0.571653, 0.617647}));
        customAnimation->push_node(vec3(1278.29, 3904.21, 116.243),
                                   glm::quat(0.414491, {-0.351915, 0.570627, 0.615417}));
        customAnimation->push_node(vec3(1281.58, 3902.57, 116.246),
                                   glm::quat(0.413663, {-0.340856, 0.575765, 0.61741}));
        customAnimation->push_node(vec3(1284.52, 3900.89, 116.273),
                                   glm::quat(0.395971, {-0.324987, 0.586546, 0.627339}));
        customAnimation->push_node(vec3(1287.04, 3899.1, 116.332),
                                   glm::quat(0.361126, {-0.309359, 0.599936, 0.643399}));
        /*customAnimation->push_node(vec3(1289.18, 3897.18, 116.383), glm::quat(0.357781 , {-0.330357, 0.59059, 0.643475}));
        customAnimation->push_node(vec3(1291.76, 3895.37, 116.392), glm::quat(0.340304 , {-0.346443, 0.609606, 0.626539}));
        customAnimation->push_node(vec3(1293.19, 3894.48, 116.346), glm::quat(0.342367 , {-0.344911, 0.606178, 0.629579}));
        customAnimation->push_node(vec3(1294.95, 3893.43, 116.344), glm::quat(0.342673 , {-0.354682, 0.6118, 0.618446}));
        customAnimation->push_node(vec3(1296.42, 3892.55, 116.344), glm::quat(0.342673 , {-0.354682, 0.6118, 0.618446}));
        customAnimation->push_node(vec3(1297.59, 3891.85, 116.344), glm::quat(0.342673 , {-0.354682, 0.6118, 0.618446}));
        customAnimation->push_node(vec3(1298.97, 3891.02, 116.344), glm::quat(0.342673 , {-0.354682, 0.6118, 0.618446}));
        customAnimation->push_node(vec3(1299.55, 3890.62, 116.352), glm::quat(0.409987 , {-0.421213, 0.571266, 0.57284}));
        customAnimation->push_node(vec3(1299.7, 3890.55, 116.354), glm::quat(-0.549083 , {0.567007, -0.44114, -0.42709}));
        customAnimation->push_node(vec3(1299.7, 3890.55, 116.354), glm::quat(-0.659347 , {0.674072, -0.246424, -0.223974}));
        customAnimation->push_node(vec3(1298.47, 3891.94, 116.364), glm::quat(-0.604103 , {0.615598, 0.343043, 0.372049}));
        customAnimation->push_node(vec3(1297.45, 3893.16, 116.383), glm::quat(-0.59147 , {0.603171, 0.363617, 0.392596}));*/
        // new points
        customAnimation->push_node(vec3(1290.04, 3896.54, 116.392),
                                   glm::quat(0.360756, {-0.33025, 0.582434, 0.649277}));
        customAnimation->push_node(vec3(1291.75, 3895.38, 116.392),
                                   glm::quat(0.353936, {-0.334796, 0.585553, 0.647896}));
        customAnimation->push_node(vec3(1293.18, 3894.49, 116.346), glm::quat(0.35464, {-0.33723, 0.586207, 0.645653}));
        customAnimation->push_node(vec3(1294.94, 3893.44, 116.344),
                                   glm::quat(0.357713, {-0.340551, 0.584996, 0.643309}));
        customAnimation->push_node(vec3(1296.42, 3892.55, 116.344),
                                   glm::quat(0.358659, {-0.341757, 0.584768, 0.642349}));
        customAnimation->push_node(vec3(1297.58, 3891.86, 116.344),
                                   glm::quat(0.358017, {-0.341412, 0.58533, 0.642379}));
        customAnimation->push_node(vec3(1298.96, 3891.03, 116.344),
                                   glm::quat(0.36074, {-0.342388, 0.582573, 0.642842}));
        customAnimation->push_node(vec3(1299.54, 3890.63, 116.352),
                                   glm::quat(0.425628, {-0.404097, 0.548398, 0.595656}));
        customAnimation->push_node(vec3(1299.69, 3890.56, 116.354), glm::quat(0.575418, {-0.51959, 0.421557, 0.47033}));
        customAnimation->push_node(vec3(1299.78, 3890.48, 116.354),
                                   glm::quat(0.649671, {-0.628189, 0.298641, 0.30679}));
        customAnimation->push_node(vec3(1299.72, 3890.52, 116.354),
                                   glm::quat(0.69433, {-0.643334, 0.227947, 0.228184}));
        customAnimation->push_node(vec3(1299.18, 3891.14, 116.358),
                                   glm::quat(0.722538, {-0.685131, -0.0469568, -0.0795549}));
        customAnimation->push_node(vec3(1298.48, 3891.93, 116.364),
                                   glm::quat(0.622479, {-0.601441, -0.329372, -0.37723}));
        customAnimation->push_node(vec3(1297.46, 3893.15, 116.383),
                                   glm::quat(0.606779, {-0.582976, -0.355356, -0.407038}));

        customAnimation->push_node(vec3(1296.16, 3893.77, 116.439),
                                   glm::quat(0.647062, {-0.583639, -0.333193, -0.360081}));
        customAnimation->push_node(vec3(1294.58, 3894.4, 116.438),
                                   glm::quat(0.621136, {-0.55992, -0.370701, -0.404054}));
        customAnimation->push_node(vec3(1292.63, 3895.24, 116.429),
                                   glm::quat(0.616791, {-0.555156, -0.374453, -0.41371}));
        customAnimation->push_node(vec3(1290.27, 3896.43, 116.399),
                                   glm::quat(0.62843, {-0.562363, -0.358909, -0.400009}));
        customAnimation->push_node(vec3(1287.62, 3898.07, 116.36),
                                   glm::quat(0.645826, {-0.572264, -0.342234, -0.371885}));
        customAnimation->push_node(vec3(1284.68, 3900.01, 116.312),
                                   glm::quat(0.652994, {-0.565533, -0.352103, -0.360269}));
        customAnimation->push_node(vec3(1281.31, 3901.94, 116.282),
                                   glm::quat(0.644329, {-0.558473, -0.368222, -0.370621}));
        customAnimation->push_node(vec3(1277.62, 3903.9, 116.265),
                                   glm::quat(0.637179, {-0.560496, -0.367735, -0.380286}));
        customAnimation->push_node(vec3(1273.76, 3905.93, 116.246),
                                   glm::quat(0.634774, {-0.558172, -0.370725, -0.384798}));
        customAnimation->push_node(vec3(1269.83, 3907.93, 116.257),
                                   glm::quat(0.629089, {-0.558434, -0.374939, -0.389641}));
        customAnimation->push_node(vec3(1265.78, 3909.91, 116.214),
                                   glm::quat(0.633988, {-0.545871, -0.375529, -0.398825}));
        customAnimation->push_node(vec3(1261.69, 3911.83, 116.187),
                                   glm::quat(0.627413, {-0.553474, -0.371086, -0.402882}));
        customAnimation->push_node(vec3(1257.64, 3913.8, 116.165),
                                   glm::quat(0.626882, {-0.558803, -0.366755, -0.40031}));
        customAnimation->push_node(vec3(1253.72, 3915.76, 116.146),
                                   glm::quat(0.628361, {-0.559298, -0.361953, -0.401668}));
        customAnimation->push_node(vec3(1250.05, 3917.63, 116.128),
                                   glm::quat(0.631123, {-0.558631, -0.360724, -0.399367}));
        customAnimation->push_node(vec3(1246.74, 3919.32, 116.087),
                                   glm::quat(0.632677, {-0.55998, -0.354679, -0.400431}));
        customAnimation->push_node(vec3(1243.96, 3920.9, 116.08),
                                   glm::quat(0.638204, {-0.575819, -0.338448, -0.382859}));
        customAnimation->push_node(vec3(1241.74, 3922.68, 116.1),
                                   glm::quat(0.662238, {-0.599176, -0.303115, -0.332491}));
        customAnimation->push_node(vec3(1240.16, 3924.67, 116.118),
                                   glm::quat(0.695195, {-0.626713, -0.238504, -0.258941}));
        customAnimation->push_node(vec3(1239.21, 3926.79, 116.117),
                                   glm::quat(0.721704, {-0.649196, -0.162083, -0.177248}));
        customAnimation->push_node(vec3(1238.72, 3929.03, 116.117),
                                   glm::quat(0.73372, {-0.665096, -0.0922808, -0.103859}));
        customAnimation->push_node(vec3(1238.62, 3931.65, 116.12),
                                   glm::quat(0.740492, {-0.671014, -0.0193509, -0.032225}));
        customAnimation->push_node(vec3(1239.04, 3934.61, 116.116),
                                   glm::quat(0.740924, {-0.668671, 0.0521623, 0.0344806}));
        customAnimation->push_node(vec3(1240, 3937.79, 116.116), glm::quat(0.737674, {-0.662371, 0.100333, 0.0838763}));
        customAnimation->push_node(vec3(1241.15, 3941.23, 116.125),
                                   glm::quat(0.733964, {-0.663774, 0.110544, 0.0920863}));
        customAnimation->push_node(vec3(1242.4, 3944.94, 116.142),
                                   glm::quat(0.735672, {-0.661054, 0.113837, 0.0939974}));
        customAnimation->push_node(vec3(1244.05, 3949.56, 116.141),
                                   glm::quat(0.733967, {-0.661189, 0.117992, 0.100992}));


        custom_to_standard_factor =
                float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

        customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;

        std::cout << "custom Animation: " << customAnimation->length() << " nodes, ms_per_node: "
                  << customAnimation->ms_between_nodes << std::endl;
        std::cout << "standard Animation: " << standardAnimation->length() << " nodes, ms_per_node: "
                  << standardAnimation->ms_between_nodes << std::endl;
    }
    if(setName[dataset_id] == "dataset_generic_bagger") {
        // kitti_00_4 animation
        /*customAnimation->push_node(vec3(1223.37, 3899.06, 116.111), glm::quat(0.73374 , {-0.650553, 0.142913, 0.134094}));
        customAnimation->push_node(vec3(1226.11, 3905.38, 116.08), glm::quat(0.734772 , {-0.653022, 0.133353, 0.12605}));
        customAnimation->push_node(vec3(1228.52, 3910.86, 116.054), glm::quat(0.734865 , {-0.649279, 0.143678, 0.133292}));
        customAnimation->push_node(vec3(1231.05, 3915.38, 116.065), glm::quat(0.722613 , {-0.641763, 0.184153, 0.179049}));*/
        customAnimation->push_node(vec3(0.0130162, -0.00264401, 0.0111631), glm::quat(-0.000144145 , {0.999998, 0.00163161, 0.00105759}));
        customAnimation->push_node(vec3(-0.0197618, -0.0203992, 0.0783894), glm::quat(0.000800616 , {0.99976, -0.00377492, -0.0215829}));
        customAnimation->push_node(vec3(-0.0656048, -0.038777, 0.15579), glm::quat(0.00365071 , {0.999619, -0.0113245, -0.0248923}));
        customAnimation->push_node(vec3(-0.536997, -0.176788, 0.876748), glm::quat(0.00638181 , {0.999937, -0.00523727, 0.0075521}));
        customAnimation->push_node(vec3(-1.20998, -0.409714, 2.05566), glm::quat(0.0238705 , {0.996729, 0.00215969, -0.0771858}));
        customAnimation->push_node(vec3(-3.06472, -0.614793, 3.36093), glm::quat(0.0228439 , {0.972202, 0.0455788, -0.228526}));
        customAnimation->push_node(vec3(-1.74727, -0.4783, 2.48554), glm::quat(0.020469 , {0.989211, 0.0176328, -0.143986}));
        customAnimation->push_node(vec3(-5.17541, -0.868211, 5.48711), glm::quat(0.0092177 , {0.909413, -0.0807805, 0.40787}));
        customAnimation->push_node(vec3(-6.47008, -0.937206, 5.81179), glm::quat(0.0088426 , {0.537796, -0.126109, 0.833543}));
        customAnimation->push_node(vec3(-7.40414, -0.999261, 6.13396), glm::quat(0.000748452 , {0.360869, -0.155855, 0.919501}));




        custom_to_standard_factor =
                float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

        customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;

        std::cout << "custom Animation: " << customAnimation->length() << " nodes, ms_per_node: "
                  << customAnimation->ms_between_nodes << std::endl;
        std::cout << "standard Animation: " << standardAnimation->length() << " nodes, ms_per_node: "
                  << standardAnimation->ms_between_nodes << std::endl;
    }
    if(setName[dataset_id] == "Generic_grossgundla_schloss2") {
        customAnimation->push_node(vec3(0.0203318, -0.00917344, 0.0502551), glm::quat(-0.000915151 , {0.999998, -0.000198008, -0.00183981}));
        customAnimation->push_node(vec3(0.0203318, -0.00917344, 0.0502551), glm::quat(-0.000915151 , {0.999998, -0.000198008, -0.00183981}));
        customAnimation->push_node(vec3(0.0814911, -0.0158878, 0.0626774), glm::quat(-0.00419121 , {0.996673, 0.0163476, -0.0797359}));
        customAnimation->push_node(vec3(0.141186, -0.0358579, 0.0529595), glm::quat(0.0173036 , {0.984028, 0.0308017, -0.174471}));
        customAnimation->push_node(vec3(1.82982, -0.154156, -1.08707), glm::quat(0.156731 , {0.945102, -0.0127821, -0.286451}));
        customAnimation->push_node(vec3(3.74148, 0.307249, -2.71967), glm::quat(0.194231 , {0.948601, 0.0153817, -0.249386}));
        customAnimation->push_node(vec3(9.37876, 0.0335953, -3.42615), glm::quat(0.190946 , {0.976202, 0.00136492, -0.102795}));
        customAnimation->push_node(vec3(15.6495, -0.34134, -2.87742), glm::quat(0.188725 , {0.971523, 0.0188287, -0.142027}));
        customAnimation->push_node(vec3(22.5545, -0.769613, -1.70019), glm::quat(0.216418 , {0.975659, -0.020858, 0.0285847}));
        customAnimation->push_node(vec3(27.7626, -0.267315, -2.2756), glm::quat(0.139274 , {0.988994, 0.039379, 0.0307095}));
        customAnimation->push_node(vec3(36.062, 0.499398, -3.35581), glm::quat(0.103084 , {0.993405, 0.0428164, -0.0262023}));
        customAnimation->push_node(vec3(38.003, 0.78865, -3.85532), glm::quat(0.0505524 , {0.983726, 0.0592698, -0.16191}));
        customAnimation->push_node(vec3(42.713, 1.48884, -5.26173), glm::quat(0.120521 , {0.942938, 0.0910434, -0.296739}));
        customAnimation->push_node(vec3(46.4665, 2.18764, -6.33522), glm::quat(0.0763622 , {0.906097, 0.107258, -0.402062}));
        customAnimation->push_node(vec3(49.0745, 2.63998, -7.25017), glm::quat(0.0580568 , {0.751996, 0.112255, -0.646939}));
        customAnimation->push_node(vec3(49.9634, 3.00487, -7.98241), glm::quat(0.0119278 , {-0.453901, -0.268171, 0.849656}));
        customAnimation->push_node(vec3(50.187, 3.30897, -8.96135), glm::quat(0.0412525 , {0.0244912, -0.230889, 0.971797}));
        customAnimation->push_node(vec3(49.883, 3.32155, -8.99022), glm::quat(0.0551335 , {0.430904, -0.238345, 0.868604}));
        customAnimation->push_node(vec3(49.5818, 3.20761, -8.73974), glm::quat(0.0213642 , {0.61268, -0.1859, 0.76786}));
        customAnimation->push_node(vec3(49.2672, 3.01899, -8.36194), glm::quat(0.113882 , {0.916512, -0.0864784, 0.373574}));
        customAnimation->push_node(vec3(49.2672, 3.01899, -8.36194), glm::quat(0.113882 , {0.916512, -0.0864784, 0.373574}));




        custom_to_standard_factor =
                float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

        customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;

        std::cout << "custom Animation: " << customAnimation->length() << " nodes, ms_per_node: "
                  << customAnimation->ms_between_nodes << std::endl;
        std::cout << "standard Animation: " << standardAnimation->length() << " nodes, ms_per_node: "
                  << standardAnimation->ms_between_nodes << std::endl;
    }
    if(setName[dataset_id] == "Generic_uni5") {

//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
//        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
        customAnimation->push_node(vec3(0.0314335, -0.00409136, -0.00691199), glm::quat(0.000714039 , {0.999999, 0.000543446, 0.000601179}));
        customAnimation->push_node(vec3(-0.00847373, -0.084872, 0.686865), glm::quat(0.0244725 , {0.999331, -0.0271763, 0.000211574}));
        customAnimation->push_node(vec3(-0.0370395, -0.122888, 1.6158), glm::quat(0.00751842 , {0.999925, -0.00964242, -0.000562697}));
        customAnimation->push_node(vec3(-0.0472148, -0.181995, 2.49617), glm::quat(0.0233271 , {0.999443, -0.00899882, 0.0221029}));
        customAnimation->push_node(vec3(-0.0672759, -0.21852, 3.30232), glm::quat(0.0139435 , {0.999412, -0.00799822, 0.0302769}));
        customAnimation->push_node(vec3(-0.132611, -0.264805, 4.21606), glm::quat(0.0206986 , {0.998835, -0.0210762, 0.0381648}));
        customAnimation->push_node(vec3(-0.141605, -0.318237, 4.79606), glm::quat(0.0105236 , {0.997569, -0.01167, 0.0678892}));
        customAnimation->push_node(vec3(-0.306126, -0.373592, 5.96212), glm::quat(0.00998736 , {0.998199, -0.00464075, 0.0589777}));
        customAnimation->push_node(vec3(-0.328368, -0.425297, 6.47852), glm::quat(-0.00014003 , {0.999805, 0.000785774, -0.0197521}));

        customAnimation->push_node(vec3(-0.253292, -0.485548, 7.60386), glm::quat(0.0129582 , {0.952297, 0.0136683, -0.304591}));
        customAnimation->push_node(vec3(-0.38457, -0.574125, 8.95425), glm::quat(0.0170035 , {0.913736, 0.0198011, -0.40547}));
        //customAnimation->push_node(vec3(-0.454567, -0.679854, 11.0004), glm::quat(-0.0010612 , {0.999972, -0.00399513, 0.00624304}));
        //customAnimation->push_node(vec3(-0.632195, -0.814662, 13.4806), glm::quat(0.0089079 , {0.999128, -0.0175419, 0.0368224}));
        customAnimation->push_node(vec3(-0.38457, -0.574125, 8.95425), glm::quat(0.0170035 , {0.913736, 0.0198011, -0.40547}));
        customAnimation->push_node(vec3(-0.52123, -0.617953, 9.71728), glm::quat(0.0144047 , {0.986543, 0.00874211, -0.162629}));
        customAnimation->push_node(vec3(-0.47978, -0.633852, 10.3234), glm::quat(0.00230983 , {0.99963, -0.00618847, -0.0263692}));
        customAnimation->push_node(vec3(-0.454567, -0.679854, 11.0004), glm::quat(-0.0010612 , {0.999972, -0.00399513, 0.00624304}));
        customAnimation->push_node(vec3(-0.537592, -0.753114, 12.3932), glm::quat(0.0138172 , {0.998955, -0.00794923, 0.0428345}));

//        customAnimation->push_node(vec3(-0.371158, -0.904888, 15.5843), glm::quat(0.0155641 , {0.969716, -0.00503845, -0.243685}));
//        customAnimation->push_node(vec3(0.272914, -0.916494, 16.2315), glm::quat(0.000588007 , {0.849028, 0.0023508, -0.528342}));
//        customAnimation->push_node(vec3(1.57956, -0.894898, 16.8435), glm::quat(-0.00762962 , {0.844793, 0.0123638, -0.534896}));
//        customAnimation->push_node(vec3(3.03518, -0.865869, 17.5229), glm::quat(-0.00597237 , {0.808688, 0.0192287, -0.587893}));
        customAnimation->push_node(vec3(-0.676659, -0.850569, 14.3997), glm::quat(0.0187612 , {0.999335, -0.0124213, -0.0286817}));
        customAnimation->push_node(vec3(-0.371158, -0.904888, 15.5843), glm::quat(0.0155641 , {0.969716, -0.00503845, -0.243685}));
        customAnimation->push_node(vec3(-0.0879037, -0.92498, 15.9706), glm::quat(0.0141499 , {0.909823, 0.00245489, -0.414748}));
        //customAnimation->push_node(vec3(-0.0879037, -0.92498, 15.9706), glm::quat(0.0141499 , {0.909823, 0.00245489, -0.414748}));
        customAnimation->push_node(vec3(0.272914, -0.916494, 16.2315), glm::quat(0.000588007 , {0.849028, 0.0023508, -0.528342}));
        customAnimation->push_node(vec3(0.94118, -0.90321, 16.5397), glm::quat(-0.00693583 , {0.841006, 0.00936151, -0.5409}));
        customAnimation->push_node(vec3(1.57956, -0.894898, 16.8435), glm::quat(-0.00762962 , {0.844793, 0.0123638, -0.534896}));
        customAnimation->push_node(vec3(2.32266, -0.87318, 17.2327), glm::quat(-0.00934103 , {0.827171, 0.0138192, -0.561702}));
        customAnimation->push_node(vec3(3.03518, -0.865869, 17.5229), glm::quat(-0.00597236 , {0.808688, 0.0192287, -0.587893}));

        customAnimation->push_node(vec3(4.66533, -0.866266, 17.8334), glm::quat(-0.00613153 , {0.78867, 0.00817875, -0.614732}));
        customAnimation->push_node(vec3(5.88156, -0.882905, 17.9616), glm::quat(-0.0231977 , {-0.700521, 0.0237626, 0.712859}));
        customAnimation->push_node(vec3(6.34393, -0.866228, 17.8375), glm::quat(-0.0067029 , {-0.41874, 0.032543, 0.907498}));
        customAnimation->push_node(vec3(7.06285, -0.776889, 17.0603), glm::quat(-0.00356 , {-0.255292, 0.016951, 0.966709}));
        customAnimation->push_node(vec3(7.19579, -0.773236, 16.6723), glm::quat(0.0116794 , {-0.241454, -0.00332258, 0.970336}));
        customAnimation->push_node(vec3(7.58356, -0.750743, 16.0305), glm::quat(0.00620993 , {-0.199408, -0.0110978, 0.979834}));
        customAnimation->push_node(vec3(7.71054, -0.770107, 15.9519), glm::quat(0.0294341 , {-0.58232, 0.00293017, 0.812422}));
        customAnimation->push_node(vec3(7.71054, -0.770107, 15.9519), glm::quat(0.0463737 , {-0.310355, 0.0160372, 0.949354}));
        customAnimation->push_node(vec3(7.82425, -0.78661, 15.9699), glm::quat(-0.0336569 , {-0.358104, -0.0412899, 0.932161}));
        customAnimation->push_node(vec3(7.82425, -0.78661, 15.9699), glm::quat(-0.0503669 , {-0.382933, 0.00180818, 0.9224}));
        customAnimation->push_node(vec3(7.82425, -0.78661, 15.9699), glm::quat(-0.0316485 , {-0.451451, -0.0296852, 0.89124}));
        customAnimation->push_node(vec3(7.82425, -0.78661, 15.9699), glm::quat(-0.0174769 , {-0.368249, -0.0525681, 0.928075}));
        customAnimation->push_node(vec3(7.82425, -0.78661, 15.9699), glm::quat(-0.0415198 , {-0.372717, -0.00512244, 0.927001}));

        custom_to_standard_factor =
                float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

        customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;

        std::cout << "custom Animation: " << customAnimation->length() << " nodes, ms_per_node: "
                  << customAnimation->ms_between_nodes << std::endl;
        std::cout << "standard Animation: " << standardAnimation->length() << " nodes, ms_per_node: "
                  << standardAnimation->ms_between_nodes << std::endl;
    }
    if( customAnimation->length() < 2){
        customAnimation->push_node(vec3(0,0,0), glm::quat(0.113882 , {0.916512, -0.0864784, 0.373574}));
        customAnimation->push_node(vec3(1,0,0), glm::quat(0.113882 , {0.916512, -0.0864784, 0.373574}));
    }
	//myAnimation = captureAnimation;
    customAnimation->play();
	myAnimation->play();
	//----------------------------------------------------------------------
	// Load CNN
	std::cerr << "[InferenceRenderer] Start Loading CNN" << std::endl;
	std::cout << "LibTorch version: "
		<< TORCH_VERSION_MAJOR << "."
		<< TORCH_VERSION_MINOR << "."
		<< TORCH_VERSION_PATCH << std::endl;
	std::vector<torch::jit::script::Module> renderer_traces;

//    try{
//        std::cout << "try to create a tensor" << std::endl;
//        torch::Tensor tensor_res0_rgb = texture2D_to_tensor(fbo_res0->color_textures[0], -1, -1, -1);
//        std::cout << "try to create a IValue" << std::endl;
//        std::vector<torch::jit::IValue> inputs;
//
//        inputs.push_back(c10::ivalue::Tuple::create(tensor_res0_rgb[0]));
//
//    } catch (const c10::Error &e) {
//        std::cerr << "error loading the model\n";
//        std::cerr << e.what() << std::endl << std::endl;;
//        std::cerr << e.what_without_backtrace() << std::endl;
//    }
//
//    try{
//        std::cout << "load model saved with torch.jit.save()" << std::endl;
//        torch::jit::load("../../../networks/test_model1.pt");
//    } catch (const c10::Error &e) {
//        std::cerr << "error loading the model\n";
//        std::cerr << e.what() << std::endl << std::endl;;
//        std::cerr << e.what_without_backtrace() << std::endl;
//    }
//
//    try{
//        std::cout << "load model saved with torch.jit.save()" << std::endl;
//        torch::jit::load("../../../networks/test_model2.pt");
//    } catch (const c10::Error &e) {
//        std::cerr << "error loading the model\n";
//        std::cerr << e.what() << std::endl << std::endl;;
//        std::cerr << e.what_without_backtrace() << std::endl;
//    }
    if(doInference) {
        try {
            gui_params_ir.network_amount = gui_params_ir.network_filenames.size();
            //renderer_trace = torch::jit::load("../../../networks/aliev.pt");
            for (int i = 0; i < gui_params_ir.network_amount; ++i) {
                std::string network_path = networks_path + gui_params_ir.network_filenames[i] + ".pt";
                std::cout << "\tload Net: " << network_path << std::endl;
                if (!std::filesystem::exists(network_path)) {
                    std::cerr << "\tfile not found: " << network_path << std::endl;
                }

                renderer_traces.push_back(torch::jit::load(network_path));
                renderer_traces[i].to(torch::kCUDA);
                renderer_traces[i].eval();

            }
        }
        catch (const c10::Error &e) {
            std::cerr << "error loading the model\n";
            std::cerr << e.what() << std::endl << std::endl;;
            std::cerr << e.what_without_backtrace() << std::endl;
            return;
        }
    }
	
	std::cerr << "[InferenceRenderer] Finished Loading CNN" << std::endl;

    //----------------------------------------------------------------------
    // setup debug frustum
    Frustum frustum(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
    gui_params_ir.debug_frustum_amount = dataset.camCount;
    CamPathRenderer camPathRenderer;

    int mod_target_res = (gui_params_ir.initial_resolution_default.y <= 560) ? 4 : 2;
    Context::resize(gui_params_ir.initial_resolution_default.x * mod_target_res, gui_params_ir.initial_resolution_default.y * mod_target_res);

	//----------------------------------------------------------------------
	// render loop
	std::cerr << "[InferenceRenderer] Start Render Loop " << std::endl;
	//glDisable(GL_CULL_FACE); 
	// run
	while (Context::running()){
		if (gui_params_ir.increment_image) {
			setCurrentCam(dataset.currentCam + 1, gui_params_ir.cycleTestImages);
			gui_params_ir.increment_image = false;
		}
		if (gui_params_ir.decrement_image) {
			if (dataset.currentCam == 0) dataset.currentCam = gui_params_ir.cycleTestImages ? dataset.cam_names_test.size(): dataset.camCount;
			setCurrentCam(dataset.currentCam - 1, gui_params_ir.cycleTestImages);
			gui_params_ir.decrement_image = false;
		}
		if (gui_params_ir.animationRunning) {
			// write timings while animation
			auto frametimer = TimerQuery::find("Frame-time");
			timingFile << timerInference->exp_avg << ";" << timerRenderPC->exp_avg << ";" << timerMipMap->exp_avg << ";" << frametimer->exp_avg << ";" << std::endl;
		}
		timerPreprocess->begin();
		//----------------------------------------------------------------------
		// compute current gui_params_ir.res0
		mod = 2.0 * glm::pow(0.5, gui_params_ir.resolution_modifier);
		// handle input
		glfwPollEvents();
		float frame_time = Context::frame_time();
		CameraImpl::default_input_handler(frame_time);

		//----------------------------------------------------------------------
		// Animation handling
		// example animation
		if (gui_params_ir.captureVideo) {
			int step = gui_params_ir.captureVideoFrameTime; // in ms of the capture animation
			standardAnimation->update(step * gui_params_ir.captureVideoSkipControlPoint);
			captureAnimation->update(step);
            customAnimation->update(step);
		}else if (gui_params_ir.animationRunning) {
			int forward_or_backward = (gui_params_ir.animationForward) ? 1 : -1;
			//myAnimation->update(gui_params_ir.animationSpeed * frame_time * forward_or_backward);
            customAnimation->update(gui_params_ir.animationSpeed * frame_time * forward_or_backward);
            standardAnimation->update(gui_params_ir.animationSpeed * frame_time * forward_or_backward);
            if(gui_params_ir.animation_use_secondary) {
                current_camera()->load(customAnimation->eval_pos(), customAnimation->eval_rot());
            }
			if (!myAnimation->running) {
				//myAnimation->reset();
				myAnimation->play();
                standardAnimation->play();
                customAnimation->play();

				//std::cerr << "Restarting Animation" << std::endl;
				timingFile.close();

			}
		}


		//----------------------------------------------------------------------
		// Nearest View Sorting
		// get similarity_descriptor
		get_capture_view_similarity(nearest_views, current_camera()->pos, current_camera()->dir);
		std::sort(nearest_views.begin(), nearest_views.end(), [](Capture_View& i, Capture_View& j) {
			return i.similarity_descriptor < j.similarity_descriptor;
			});
		// log nearest views of current positions if wanted
		if (gui_params_ir.log_nearest_views)
			std::cout << "\r" << /* "cam_dir: " << current_camera()->dir << " parsed_dir: " << normalize(nearest_views[0].dir) <<*/ nearest_views[0].id << ":" << nearest_views[0].num << ", " << nearest_views[0].similarity_descriptor << ", dist " << length(nearest_views[0].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[0].dir), normalize(current_camera()->dir))
			<< "       " << nearest_views[1].id << ":" << nearest_views[1].num << ", " << nearest_views[1].similarity_descriptor << ", dist " << length(nearest_views[1].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[1].dir), normalize(current_camera()->dir))
			<< "       " << nearest_views[2].id << ":" << nearest_views[2].num << ", " << nearest_views[2].similarity_descriptor << ", dist " << length(nearest_views[2].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[2].dir), normalize(current_camera()->dir))
			<< "       " << nearest_views[3].id << ":" << nearest_views[3].num << ", " << nearest_views[3].similarity_descriptor << ", dist " << length(nearest_views[3].pos - current_camera()->pos) << ", dot " << glm::dot(normalize(nearest_views[3].dir), normalize(current_camera()->dir))
			<< "                              ";


		//----------------------------------------------------------------------

		//----------------------------------------------------------------------
		// update and reload shaders
		current_camera()->update();
		if (gui_params_ir.use_dataset_proj) {
			if (gui_params_ir.use_dataset_proj_cropped) current_camera()->proj = dataset.gt_proj_cropped;
			else current_camera()->proj = dataset.gt_proj;
		}
		static uint32_t frame_counter = 0;
		if (frame_counter++ % 100 == 0)
			reload_modified_shaders();

		timerPreprocess->end();
		timer->begin();

		//----------------------------------------------------------------------
		// the lataset captured timestamp of all used gt images
		if (gui_params_ir.use_timestamp) {
			gt_timestamp_min = std::numeric_limits<int>::max();
			gt_timestamp_max = 0;

			if (debugBool) {
				gt_timestamp_max = debugInt;
				gt_timestamp_min = debugInt - 10;
			}
			else if (gui_params_ir.animationRunning && !gui_params_ir.animation_use_secondary) { // only use actual animation timestamp if the standard/capture animation is used

                gt_timestamp_max = size_t(glm::floor(myAnimation->time)) + 3;
                gt_timestamp_min = size_t(glm::floor(myAnimation->time)) - 7;

			}
			else {
				for (int nv = 0; nv < gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id]; ++nv) {
					if (gt_timestamp_max < nearest_views[nv].id) {
						gt_timestamp_max = nearest_views[nv].id;
					}
					if (gt_timestamp_min > nearest_views[nv].id) {
						gt_timestamp_min = nearest_views[nv].id;

					}
				}
				if (gt_timestamp_max - gt_timestamp_min < 10) {
					int diff = gt_timestamp_max - gt_timestamp_min;
					int to_adjust = 10 - diff;
					int adjust_max = to_adjust / 2;
					int adjust_min = to_adjust - adjust_max;
					gt_timestamp_max += adjust_max;
					gt_timestamp_min -= adjust_min;
				}
			}
			if (setType[dataset_id] == "KITTY-360") {
				gt_timestamp_min = 0;
			}
			if (setType[dataset_id] == "NavVis" || setType[dataset_id] == "TanksAndTemples" || setType[dataset_id] == "L" ) {
				gt_timestamp_min = 0;
				gt_timestamp_max = 1;
			}
		}
		//std::cout << gt_timestamp_min << " : " << gt_timestamp_max << std::endl;


		timerRenderPC->begin();
		//----------------------------------------------------------------------
		// Start Rendering
		// render all drawelements into fbo
		
		//----------------------------------------------------------------------
		// Render Motion Vectors
		if (!gui_params_ir.mipmap_motion) {
			fbo_motion->bind();
			glClearColor(-2.0, -2.0, -2.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0, 0.0, 0.0, 1.0);

			if (gui_params_ir.preInitMV) {
				glDepthMask(GL_FALSE);
				initMoVecsOnly->bind();
				initMoVecsOnly->uniform("proj", current_camera()->proj);
				initMoVecsOnly->uniform("view", current_camera()->view);
				initMoVecsOnly->uniform("view_normal", current_camera()->view_normal);
				
				initMoVecsOnly->uniform("use_taa", gui_params_ir.use_taa);

				//load old view if motion vecs are rendered
				initMoVecsOnly->uniform("resolution", gui_params_ir.res0);
				int base_index = 0;
				if (gui_params_ir.use_taa) {
					initMoVecsOnly->uniform("view_old_1", view_old);
					base_index = -1;
				}
				else {
					initMoVecsOnly->uniform("view_old_1", getView(nearest_views[base_index + int(gui_params_ir.skipNearest)].id));
				}
				initMoVecsOnly->uniform("view_old_2", getView(nearest_views[base_index + 1 + int(gui_params_ir.skipNearest)].id));
				initMoVecsOnly->uniform("view_old_3", getView(nearest_views[base_index + 2 + int(gui_params_ir.skipNearest)].id));
				initMoVecsOnly->uniform("view_old_4", getView(nearest_views[base_index + 3 + int(gui_params_ir.skipNearest)].id));
				initMoVecsOnly->uniform("view_old_5", getView(nearest_views[base_index + 4 + int(gui_params_ir.skipNearest)].id));
				initMoVecsOnly->uniform("view_old_6", getView(nearest_views[base_index + 5 + int(gui_params_ir.skipNearest)].id));

				initMoVecsOnly->uniform("proj_old", dataset.gt_proj);
				Quad::draw();
				initMoVecsOnly->unbind();
				glDepthMask(GL_TRUE);
				glClear(GL_DEPTH_BUFFER_BIT);
			}
			//else { // use the full clouds present in pointClouds
			if (gui_params_ir.enableCulling) pointClouds[gui_params_ir.lod]->cull(computeFrustumCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
				current_camera()->near, current_camera()->far, current_camera()->fov_degree, dataset.camera_aspect_ratio);
			pointClouds[gui_params_ir.lod]->bind(drawPCmultiMotionOnly);
			//}
			//----------------------------------------------------------------------
			// bind shader and uniforms
			drawPCmultiMotionOnly->bind();
			drawPCmultiMotionOnly->uniform("proj", current_camera()->proj);
			drawPCmultiMotionOnly->uniform("view", current_camera()->view);
			drawPCmultiMotionOnly->uniform("view_normal", current_camera()->view_normal);
			
			drawPCmultiMotionOnly->uniform("use_taa", gui_params_ir.use_taa);

			//load old view if motion vecs are rendered
			ivec2 motion_res = glm::ivec2(Framebuffer::find("fbo_motion")->w, Framebuffer::find("fbo_motion")->h);
			drawPCmultiMotionOnly->uniform("resolution", motion_res);
			int base_index = 0;
			if (gui_params_ir.use_taa) {
				drawPCmultiMotionOnly->uniform("view_old_1", view_old);
				base_index = -1;
			}
			else {
				drawPCmultiMotionOnly->uniform("view_old_1", getView(nearest_views[base_index + int(gui_params_ir.skipNearest)].id));
			}
			drawPCmultiMotionOnly->uniform("view_old_2", getView(nearest_views[base_index + 1 + int(gui_params_ir.skipNearest)].id));
			drawPCmultiMotionOnly->uniform("view_old_3", getView(nearest_views[base_index + 2 + int(gui_params_ir.skipNearest)].id));
			drawPCmultiMotionOnly->uniform("view_old_4", getView(nearest_views[base_index + 3 + int(gui_params_ir.skipNearest)].id));
			drawPCmultiMotionOnly->uniform("view_old_5", getView(nearest_views[base_index + 4 + int(gui_params_ir.skipNearest)].id));
			drawPCmultiMotionOnly->uniform("view_old_6", getView(nearest_views[base_index + 5 + int(gui_params_ir.skipNearest)].id));


			drawPCmultiMotionOnly->uniform("proj_old", dataset.gt_proj);

			drawPCmultiMotionOnly->uniform("gt_timestamp_max", gt_timestamp_max);
			drawPCmultiMotionOnly->uniform("gt_timestamp_min", gt_timestamp_min);
			drawPCmultiMotionOnly->uniform("use_timestamp", gui_params_ir.use_timestamp);
			//----------------------------------------------------------------------
			// draw PointCloud
			pointClouds[gui_params_ir.lod]->draw();
			pointClouds[gui_params_ir.lod]->unbind();
			//}
			drawPCmultiMotionOnly->unbind();

			fbo_motion->unbind();
		}

		fbo_res0->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




		//----------------------------------------------------------------------
		//default camera

		// preinit movecs in 
		if (gui_params_ir.mipmap_motion && gui_params_ir.preInitMV) {
			glDepthMask(GL_FALSE);
			initMoVecs->bind();
			initMoVecs->uniform("proj", current_camera()->proj);
			initMoVecs->uniform("view", current_camera()->view);
			initMoVecs->uniform("view_normal", current_camera()->view_normal);

			initMoVecs->uniform("use_taa", gui_params_ir.use_taa);

			//load old view if motion vecs are rendered
			initMoVecs->uniform("resolution", gui_params_ir.res0);
			int base_index = 0;
			if (gui_params_ir.use_taa) {
				initMoVecs->uniform("view_old_1", view_old);
				base_index = -1;
			}
			else {
				initMoVecs->uniform("view_old_1", getView(nearest_views[base_index + int(gui_params_ir.skipNearest)].id));
			}
			initMoVecs->uniform("view_old_2", getView(nearest_views[base_index + 1 + int(gui_params_ir.skipNearest)].id));
			initMoVecs->uniform("view_old_3", getView(nearest_views[base_index + 2 + int(gui_params_ir.skipNearest)].id));
			initMoVecs->uniform("view_old_4", getView(nearest_views[base_index + 3 + int(gui_params_ir.skipNearest)].id));
			initMoVecs->uniform("view_old_5", getView(nearest_views[base_index + 4 + int(gui_params_ir.skipNearest)].id));
			initMoVecs->uniform("view_old_6", getView(nearest_views[base_index + 5 + int(gui_params_ir.skipNearest)].id));

			initMoVecs->uniform("proj_old", dataset.gt_proj);
			Quad::draw();
			initMoVecs->unbind();
			glDepthMask(GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
		}


		fbo_res0->bind();
		// choose shader according to gui
		Shader& curShader = (gui_params_ir.mipmap_motion) ? drawPCmultiMotionShader : drawPCmultiShader;
		if (gui_params_ir.enableCulling) pointClouds[gui_params_ir.lod]->cull(computeFrustumCullingShader, current_camera()->pos, current_camera()->dir, current_camera()->up,
			current_camera()->near, current_camera()->far, current_camera()->fov_degree, dataset.camera_aspect_ratio);
		pointClouds[gui_params_ir.lod]->bind(curShader);
		//}
		//----------------------------------------------------------------------
		// bind shader and uniforms
		curShader->bind();
		curShader->uniform("proj", current_camera()->proj);
		curShader->uniform("view", current_camera()->view);
		curShader->uniform("view_normal", current_camera()->view_normal);

		curShader->uniform("use_taa", gui_params_ir.use_taa);

		//load old view if motion vecs are rendered
		curShader->uniform("resolution", gui_params_ir.res0);
		if (gui_params_ir.mipmap_motion) {
			int base_index = 0;
			if (gui_params_ir.use_taa) {
				curShader->uniform("view_old_1", view_old);
				base_index = -1;
			}
			else {
				curShader->uniform("view_old_1", getView(nearest_views[base_index + int(gui_params_ir.skipNearest)].id));
			}
			curShader->uniform("view_old_2", getView(nearest_views[base_index + 1 + int(gui_params_ir.skipNearest)].id));
			curShader->uniform("view_old_3", getView(nearest_views[base_index + 2 + int(gui_params_ir.skipNearest)].id));
			curShader->uniform("view_old_4", getView(nearest_views[base_index + 3 + int(gui_params_ir.skipNearest)].id));
			curShader->uniform("view_old_5", getView(nearest_views[base_index + 4 + int(gui_params_ir.skipNearest)].id));
			curShader->uniform("view_old_6", getView(nearest_views[base_index + 5 + int(gui_params_ir.skipNearest)].id));

			curShader->uniform("proj_old", dataset.gt_proj);
		}
		else {
			curShader->uniform("view_old", view_old);

		}
		
		curShader->uniform("gt_timestamp_max", gt_timestamp_max);
		curShader->uniform("gt_timestamp_min", gt_timestamp_min);
		curShader->uniform("use_timestamp", gui_params_ir.use_timestamp);


		//----------------------------------------------------------------------
		// draw PointCloud 


		pointClouds[gui_params_ir.lod]->draw();
		pointClouds[gui_params_ir.lod]->unbind();
		//}
		curShader->unbind();

		//----------------------------------------------------------------------

		//----------------------------------------------------------------------
		timerRenderPC->end();
		timerMipMap->begin();
		//----------------------------------------------------------------------

		//----------------------------------------------------------------------
		// draw general cppgl gui
		if (gui_params_ir.draw_gui == 2 || gui_params_ir.draw_gui == 3) {
			gui_draw();
		}
		if (gui_params_ir.draw_gui == 1 || gui_params_ir.draw_gui == 3) {
			custom_gui_draw();
		}

		fbo_res0->unbind();
		//----------------------------------------------------------------------
		// mip map lower resolutions
		glClearDepth(1);

		fbo_res1->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		bool motion_pixelwise = false;
		if (gui_params_ir.mipmap_motion)
			ir_mipmap_with_motion(fbo_res0->color_textures[0], fbo_res0->color_textures[2], fbo_res0->color_textures[3], fbo_res0->color_textures[4], fbo_res0->color_textures[5], fbo_res0->color_textures[6], fbo_res0->color_textures[7], fbo_res0->depth_texture, gui_params_ir.res0, gui_params_ir.res1, motion_pixelwise);
		else
			ir_mipmap(fbo_res0->color_textures[0], fbo_res0->depth_texture, gui_params_ir.res0, gui_params_ir.res1);
		fbo_res1->unbind();

		fbo_res2->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (gui_params_ir.mipmap_motion)
			ir_mipmap_with_motion(fbo_res1->color_textures[0], fbo_res1->color_textures[2], fbo_res1->color_textures[3], fbo_res1->color_textures[4], fbo_res1->color_textures[5], fbo_res1->color_textures[6], fbo_res1->color_textures[7], fbo_res1->depth_texture, gui_params_ir.res1, gui_params_ir.res2, motion_pixelwise);
		else
			ir_mipmap(fbo_res1->color_textures[0], fbo_res1->depth_texture, gui_params_ir.res1, gui_params_ir.res2);
		fbo_res2->unbind();

		fbo_res3->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (gui_params_ir.mipmap_motion)
			ir_mipmap_with_motion(fbo_res2->color_textures[0], fbo_res2->color_textures[2], fbo_res2->color_textures[3], fbo_res2->color_textures[4], fbo_res2->color_textures[5], fbo_res2->color_textures[6], fbo_res2->color_textures[7], fbo_res2->depth_texture, gui_params_ir.res2, gui_params_ir.res3, motion_pixelwise);
		else
			ir_mipmap(fbo_res2->color_textures[0], fbo_res2->depth_texture, gui_params_ir.res2, gui_params_ir.res3);
		fbo_res3->unbind();


		timerMipMap->end();

		//----------------------------------------------------------------------
		// Inference of the CNN

		timerInference->begin();
		if (doInference)
		{
			if ((gui_params_ir.displayMode == 0 && gui_params_ir.currentRenderInfo == 17) || (gui_params_ir.displayMode == 1 && gui_params_ir.currentRenderInfo >= 2)) {
				try {
					using namespace torch::indexing;
					//---------------------------------------------------------------------------
					// create input for different resolutions
					torch::Tensor tensor_res0_rgb = texture2D_to_tensor(fbo_res0->color_textures[0], -1, -1, -1);
					torch::Tensor tensor_res1_rgb = texture2D_to_tensor(fbo_res1->color_textures[0], -1, -1, -1);
					torch::Tensor tensor_res2_rgb = texture2D_to_tensor(fbo_res2->color_textures[0], -1, -1, -1);
					torch::Tensor tensor_res3_rgb = texture2D_to_tensor(fbo_res3->color_textures[0], -1, -1, -1);
					torch::Tensor tensor_res0_depth = texture2D_to_tensor(fbo_res0->color_textures[1], -1, -1, -1);
					torch::Tensor tensor_res1_depth = texture2D_to_tensor(fbo_res1->color_textures[1], -1, -1, -1);
					torch::Tensor tensor_res2_depth = texture2D_to_tensor(fbo_res2->color_textures[1], -1, -1, -1);
					torch::Tensor tensor_res3_depth = texture2D_to_tensor(fbo_res3->color_textures[1], -1, -1, -1);

					tensor_res0_rgb = tensor_res0_rgb.index({ Slice(0,3),Slice(),Slice() }).unsqueeze(0);
					tensor_res1_rgb = tensor_res1_rgb.index({ Slice(0,3),Slice(),Slice() }).unsqueeze(0);
					tensor_res2_rgb = tensor_res2_rgb.index({ Slice(0,3),Slice(),Slice() }).unsqueeze(0);
					tensor_res3_rgb = tensor_res3_rgb.index({ Slice(0,3),Slice(),Slice() }).unsqueeze(0);
					tensor_res0_depth = tensor_res0_depth.index({ Slice(0,1),Slice(),Slice() }).unsqueeze(0);
					tensor_res1_depth = tensor_res1_depth.index({ Slice(0,1),Slice(),Slice() }).unsqueeze(0);
					tensor_res2_depth = tensor_res2_depth.index({ Slice(0,1),Slice(),Slice() }).unsqueeze(0);
					tensor_res3_depth = tensor_res3_depth.index({ Slice(0,1),Slice(),Slice() }).unsqueeze(0);

					torch::Tensor tensor_res0 = torch::cat({ tensor_res0_rgb,tensor_res0_depth }, 1);
					torch::Tensor tensor_res1 = torch::cat({ tensor_res1_rgb,tensor_res1_depth }, 1);
					torch::Tensor tensor_res2 = torch::cat({ tensor_res2_rgb,tensor_res2_depth }, 1);
					torch::Tensor tensor_res3 = torch::cat({ tensor_res3_rgb,tensor_res3_depth }, 1);

					std::vector <torch::Tensor> tensors_groundtruth;
					std::vector <torch::Tensor> tensors_motion;

					torch::Tensor output_tensor;
					std::vector<torch::jit::IValue> inputs;
					if (gui_params_ir.use_taa) {
						tensors_groundtruth.push_back(texture2D_to_tensor(fbo_prev->color_textures[0], -1, -1, -1)); // rgb output
						tensors_groundtruth[0] = tensors_groundtruth[0].index({ Slice(0,3),Slice(),Slice() }).unsqueeze(0); // crop to 3 channels

						torch::Tensor tensor_groundtruth_depth = texture2D_to_tensor(fbo_prev->color_textures[1], -1, -1, -1); // depth of previous image
						tensor_groundtruth_depth = tensor_groundtruth_depth.index({ Slice(0,1),Slice(),Slice() }).unsqueeze(0); // crop to 1 channel

						tensors_groundtruth[0] = torch::cat({ tensors_groundtruth[0],tensor_groundtruth_depth }, 1); // cat prev rgb and depth

						if (gui_params_ir.mipmap_motion) {
							auto cur_fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
							tensors_motion.push_back(texture2D_to_tensor(cur_fbo->color_textures[2], -1, -1, -1));
						}
						else {
							tensors_motion.push_back(texture2D_to_tensor(fbo_motion->color_textures[0], -1, -1, -1));
						}

						tensors_motion[0] = tensors_motion[0].index({ Slice(0,gui_params_ir.network_movec_channels[gui_params_ir.network_id]),Slice(),Slice() }).unsqueeze(0).contiguous();
					}
					int start_i = (gui_params_ir.use_taa) ? 1 : 0; // if taa, use nearest images [0:gta-1] and corresponding movecs: [1:gta] 
					for (int i = start_i; i < gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id]; ++i) {
						tensors_groundtruth.push_back(texture2D_to_tensor(dataset.cam_views[nearest_views[i + int(gui_params_ir.skipNearest) - start_i].id].tex_gpu, -1, -1, -1).unsqueeze(0).contiguous());

						if (gui_params_ir.mipmap_motion) {
							auto cur_fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
							tensors_motion.push_back(texture2D_to_tensor(cur_fbo->color_textures[2 + i], -1, -1, -1));
						}
						else {
							tensors_motion.push_back(texture2D_to_tensor(fbo_motion->color_textures[i], -1, -1, -1));
						}

						tensors_motion[i] = tensors_motion[i].index({ Slice(0,gui_params_ir.network_movec_channels[gui_params_ir.network_id]),Slice(),Slice() }).unsqueeze(0).contiguous();
					}

					inputs.push_back(c10::ivalue::Tuple::create(tensor_res0, tensor_res1, tensor_res2, tensor_res3));
					switch (gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id]) {
					case 1:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0]));
						break;
					case 2:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0], tensors_groundtruth[1]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0], tensors_motion[1]));
						break;
					case 3:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0], tensors_groundtruth[1], tensors_groundtruth[2]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0], tensors_motion[1], tensors_motion[2]));
						break;
					case 4:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0], tensors_groundtruth[1], tensors_groundtruth[2], tensors_groundtruth[3]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0], tensors_motion[1], tensors_motion[2], tensors_motion[3]));
						break;
					case 5:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0], tensors_groundtruth[1], tensors_groundtruth[2], tensors_groundtruth[3], tensors_groundtruth[4]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0], tensors_motion[1], tensors_motion[2], tensors_motion[3], tensors_motion[4]));
						break;
					case 6:
						inputs.push_back(c10::ivalue::Tuple::create(tensors_groundtruth[0], tensors_groundtruth[1], tensors_groundtruth[2], tensors_groundtruth[3], tensors_groundtruth[4], tensors_groundtruth[5]));
						inputs.push_back(c10::ivalue::Tuple::create(tensors_motion[0], tensors_motion[1], tensors_motion[2], tensors_motion[3], tensors_motion[4], tensors_motion[5]));
						break;
					default:
						std::cerr << "[InferenceRenderer] FATAL ERROR: amount of ground truth imges not supported: " << gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id] << ", should be in [1,2,3,4,5,6]." << std::endl;
						return;

					}

					//---------------------------------------------------------------------------
					// do the inference
					output_tensor = renderer_traces[gui_params_ir.network_id].forward(inputs).toTensor();
					output_tensor = output_tensor.squeeze(0).contiguous();
					output_tensor = torch::cat({ output_tensor, torch::ones({1, output_tensor.sizes()[1], output_tensor.sizes()[2]}).to(torch::kCUDA) }, 0);
					tensor_to_texture2D(output_tensor.contiguous(), fbo_out->color_textures[0], gui_params_ir.res0.y, gui_params_ir.res0.x, false);
					//---------------------------------------------------------------------------


					//---------------------------------------------------------------------------
					// Copy Point Rendered Depth to fbo_out for TAA
					texture_to_texture(fbo_res0->color_textures[1]->id, fbo_out->color_textures[1]->id, gui_params_ir.res0.y, gui_params_ir.res0.x);
					//---------------------------------------------------------------------------


					//---------------------------------------------------------------------------
					// transform output back to opengl format


				}
				catch (const c10::Error& e) {
					std::cerr << "error loading the model\n";
					std::cerr << e.what() << std::endl << std::endl;;
					std::cerr << e.what_without_backtrace() << std::endl;
					return;
				}
			}
		}
		timerInference->end();

		//----------------------------------------------------------------------

		timer->end();

		timerBlit->begin();

        //----------------------------------------------------------------------
        // render debug frustum to neural view
        glLineWidth(2);
        fbo_res0->bind();
        if (gui_params_ir.debug_camera_frustum_size_adjusted){
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            gui_params_ir.debug_camera_frustum_size_adjusted = false;
        }
        if(gui_params_ir.debug_camera_frustum) {

            for (int i = 0; i < std::min(dataset.camCount, gui_params_ir.debug_frustum_amount); ++i)
                ir_debug_frustum(frustum, dataset.cam_views[i].pose.view, current_camera()->view,
                                 current_camera()->proj);


        }
        if(gui_params_ir.animation_use_secondary && gui_params_ir.animation_show_capture_frustum) {
            glLineWidth(1);
            glm::quat q = standardAnimation->eval_rot();
            glm::vec3 pos = standardAnimation->eval_pos();

            mat4 view = glm::mat4_cast(q);
            glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
            glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
            view = glm::lookAt(pos, pos + dir, up);
            ir_debug_frustum(frustum, view , current_camera()->view,
                             current_camera()->proj);

            glLineWidth(2);
        }
        if(gui_params_ir.animation_visualize_primary_path){
            camPathRenderer.set_path(standardAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(0.3,0.4,1));

            for (auto& pose : standardAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view , current_camera()->view,
                                 current_camera()->proj, glm::vec3(0.3,0.4,1));
            }
        }
        if(gui_params_ir.animation_visualize_secondary_path && customAnimation->length() > 0){
            camPathRenderer.set_path(customAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(1,0.65,0));
            for (auto& pose : customAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view, current_camera()->view,
                                 current_camera()->proj, glm::vec3(1,0.65,0));
            }
        }
        if(!gui_params_ir.animationRunning && gui_params_ir.debug_camera_frustum_currentCam) {
            glLineWidth(3);
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size_current);
            ir_debug_frustum(frustum, dataset.cam_views[dataset.currentCam].pose.view, current_camera()->view,
                             current_camera()->proj, glm::vec3(0.12, 0.46, 0.71));
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            glLineWidth(2);
        }
        fbo_res0->unbind();
        glDisable(GL_DEPTH_TEST);
        fbo_out_1->bind();
        if (gui_params_ir.debug_camera_frustum_size_adjusted){
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            gui_params_ir.debug_camera_frustum_size_adjusted = false;
        }
        if(gui_params_ir.debug_camera_frustum) {

            for (int i = 0; i < std::min(dataset.camCount, gui_params_ir.debug_frustum_amount); ++i)
                ir_debug_frustum(frustum, dataset.cam_views[i].pose.view, current_camera()->view,
                                 current_camera()->proj);


        }
        if(gui_params_ir.animation_use_secondary && gui_params_ir.animation_show_capture_frustum) {
            glLineWidth(1);
            glm::quat q = standardAnimation->eval_rot();
            glm::vec3 pos = standardAnimation->eval_pos();

            mat4 view = glm::mat4_cast(q);
            glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
            glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
            view = glm::lookAt(pos, pos + dir, up);
            ir_debug_frustum(frustum, view , current_camera()->view,
                             current_camera()->proj);

            glLineWidth(2);
        }
        if(gui_params_ir.animation_visualize_primary_path){
            camPathRenderer.set_path(standardAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(0.3,0.4,1));

            for (auto& pose : standardAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view , current_camera()->view,
                                 current_camera()->proj, glm::vec3(0.3,0.4,1));
            }
        }
        if(gui_params_ir.animation_visualize_secondary_path && customAnimation->length() > 0){
            camPathRenderer.set_path(customAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(1,0.65,0));
            for (auto& pose : customAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view, current_camera()->view,
                                 current_camera()->proj, glm::vec3(1,0.65,0));
            }
        }
        if(!gui_params_ir.animationRunning && gui_params_ir.debug_camera_frustum_currentCam) {
            glLineWidth(3);
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size_current);
            ir_debug_frustum(frustum, dataset.cam_views[dataset.currentCam].pose.view, current_camera()->view,
                             current_camera()->proj, glm::vec3(0.12, 0.46, 0.71));
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            glLineWidth(2);
        }
        fbo_out_1->unbind();
        fbo_out_2->bind();
        if (gui_params_ir.debug_camera_frustum_size_adjusted){
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            gui_params_ir.debug_camera_frustum_size_adjusted = false;
        }
        if(gui_params_ir.debug_camera_frustum) {

            for (int i = 0; i < std::min(dataset.camCount, gui_params_ir.debug_frustum_amount); ++i)
                ir_debug_frustum(frustum, dataset.cam_views[i].pose.view, current_camera()->view,
                                 current_camera()->proj);


        }
        if(gui_params_ir.animation_use_secondary && gui_params_ir.animation_show_capture_frustum) {
            glLineWidth(1);
            glm::quat q = standardAnimation->eval_rot();
            glm::vec3 pos = standardAnimation->eval_pos();

            mat4 view = glm::mat4_cast(q);
            glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
            glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
            view = glm::lookAt(pos, pos + dir, up);
            ir_debug_frustum(frustum, view , current_camera()->view,
                             current_camera()->proj);

            glLineWidth(2);
        }
        if(gui_params_ir.animation_visualize_primary_path){
            camPathRenderer.set_path(standardAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(0.3,0.4,1));

            for (auto& pose : standardAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view , current_camera()->view,
                                 current_camera()->proj, glm::vec3(0.3,0.4,1));
            }
        }
        if(gui_params_ir.animation_visualize_secondary_path && customAnimation->length() > 0){
            camPathRenderer.set_path(customAnimation->camera_path);
            ir_render_cam_path(camPathRenderer, current_camera()->view,current_camera()->proj, glm::vec3(1,0.65,0));
            for (auto& pose : customAnimation->camera_path) {
                glm::quat q = pose.second;
                glm::vec3 pos = pose.first;

                mat4 view = glm::mat4_cast(q);
                glm::vec3 dir = glm::normalize(-glm::vec3(view[0][2], view[1][2], view[2][2]));
                glm::vec3 up = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));
                view = glm::lookAt(pos, pos + dir, up);
                ir_debug_frustum(frustum, view, current_camera()->view,
                                 current_camera()->proj, glm::vec3(1,0.65,0));
            }
        }
        if(!gui_params_ir.animationRunning && gui_params_ir.debug_camera_frustum_currentCam) {
            glLineWidth(3);
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size_current);
            ir_debug_frustum(frustum, dataset.cam_views[dataset.currentCam].pose.view, current_camera()->view,
                             current_camera()->proj, glm::vec3(0.12, 0.46, 0.71));
            frustum.frustum_data(dataset.gt_proj, gui_params_ir.debug_camera_frustum_size);
            glLineWidth(2);
        }
        fbo_out_2->unbind();
        glLineWidth(1);
        glEnable(GL_DEPTH_TEST);

		// draw
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		
		//ir_blit(fbo_res0->color_textures[0]);
		ivec2 tmp_context_res = Context::resolution();
		glViewport(0, 0, tmp_context_res.x, tmp_context_res.y);
		if (gui_params_ir.displayMode == 0) { // Single output "0 Color Res0, 1:Color Res1, 2:Color Res2, 3:Color Res3, 4:Depth Res0, 5:Depth Res1, 6:Depth Res2, 7:Depth Res3, 8:Motion1, 9:Motion2, 10:Motion3, 11:Groundtruth1, 12:Groundtruth2, 13:Groundtruth3, 14:Output"
			switch (gui_params_ir.currentRenderInfo) {
			case 0: // Color Res0
				ir_blit(fbo_res0->color_textures[0]);
				break;
			case 1: // Color Res1
				ir_blit(fbo_res1->color_textures[0]);
				break;
			case 2: // Color Res2
				ir_blit(fbo_res2->color_textures[0]);
				break;
			case 3: // Color Res3
				ir_blit(fbo_res3->color_textures[0]);
				break;
			case 4: // Depth Res0
				if(gui_params_ir.show_depth_bg_white)
					ir_blit(fbo_res0->depth_texture);
				else
					ir_blit(fbo_res0->color_textures[1]);
				break;
			case 5: // Depth Res1
				if (gui_params_ir.show_depth_bg_white)
					ir_blit(fbo_res1->depth_texture);
				else
					ir_blit(fbo_res1->color_textures[1]);
				break;
			case 6: // Depth Res2
				if (gui_params_ir.show_depth_bg_white)
					ir_blit(fbo_res2->depth_texture);
				else
					ir_blit(fbo_res2->color_textures[1]);
				break;
			case 7: // Depth Res3
				if (gui_params_ir.show_depth_bg_white)
					ir_blit(fbo_res3->depth_texture);
				else
					ir_blit(fbo_res3->color_textures[1]);
				break;
			case 8: // Motion1
				if(gui_params_ir.mipmap_motion) {
					Framebuffer fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
					ir_blit(fbo->color_textures[2]);
				}
				else
					ir_blit(fbo_motion->color_textures[0]);
				break;
			case 9: // Motion2
				if (gui_params_ir.mipmap_motion) {
					Framebuffer fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
					ir_blit(fbo->color_textures[3]);
				}
				else
					ir_blit(fbo_motion->color_textures[1]);
				break;
			case 10: // Motion3
				if (gui_params_ir.mipmap_motion) {
					Framebuffer fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
					ir_blit(fbo->color_textures[4]);
				}	
				else
					ir_blit(fbo_motion->color_textures[2]);
				break;
			case 11: // Groundtruth1
				ir_blit(dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 12: // Groundtruth2
				ir_blit(dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 13: // Groundtruth3
				ir_blit(dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 14: // Groundtruth Depth 1
				ir_blit_depth(dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 15: // Groundtruth Depth 2
				ir_blit_depth(dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 16: // Groundtruth Depth 3
				ir_blit_depth(dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				break;
			case 17: // Output
				ir_blit(fbo_out->color_textures[0]);
				break;
			}
		}
		else { // Multi Output "0:Color, 1:Depth, 2:Motion, 3:Groundtruth, 4:Output"
			switch (gui_params_ir.currentRenderInfo) {
			case 0: // Color
				ir_blit_multi(fbo_res0->color_textures[0], fbo_res1->color_textures[0], fbo_res2->color_textures[0], fbo_res3->color_textures[0]);
				break;
			case 1: // Depth
				if (gui_params_ir.show_depth_bg_white) {
					ir_blit_multi(fbo_res0->depth_texture, fbo_res1->depth_texture, fbo_res2->depth_texture, fbo_res3->depth_texture);
				}
				else {
					ir_blit_multi(fbo_res0->color_textures[1], fbo_res1->color_textures[1], fbo_res2->color_textures[1], fbo_res3->color_textures[1]);
				}
				break;
			case 2: // Motion
				if (gui_params_ir.mipmap_motion) {
					Framebuffer fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
					ir_blit_multi(fbo_out->color_textures[0], fbo->color_textures[2], fbo->color_textures[3], fbo->color_textures[4]);
				}
				else
					ir_blit_multi(fbo_out->color_textures[0], fbo_motion->color_textures[0], fbo_motion->color_textures[1], fbo_motion->color_textures[2]);
				break;
			case 3: // Motion 2
				if (gui_params_ir.mipmap_motion) {
					Framebuffer fbo = Framebuffer::find("fbo_res" + std::to_string(gui_params_ir.network_feature_extraction_depth[gui_params_ir.network_id]));
					ir_blit_multi(fbo_out->color_textures[0], fbo->color_textures[5], fbo->color_textures[6], fbo->color_textures[7]);
				}
				else
					ir_blit_multi(fbo_out->color_textures[0], fbo_motion->color_textures[3], fbo_motion->color_textures[4], fbo_motion->color_textures[5]);
				break;
			case 4: // Groundtruth
				if (gui_params_ir.use_taa) {
					ir_blit_multi(fbo_out->color_textures[0], fbo_prev->color_textures[0], dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				else {
					ir_blit_multi(fbo_out->color_textures[0], dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				break;
			case 5: // Groundtruth 2
				if (gui_params_ir.use_taa) {
					ir_blit_multi(fbo_out->color_textures[0], dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[3 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[4 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				else {
					ir_blit_multi(fbo_out->color_textures[0], dataset.cam_views[nearest_views[3 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[4 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[5 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				break;
			case 6: // Groundtruth Depth
				if (gui_params_ir.use_taa) {
					ir_blit_multi_depth(fbo_out->color_textures[0], fbo_prev->color_textures[1], dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				else {
					ir_blit_multi_depth(fbo_out->color_textures[0], dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[1 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				break;
			case 7: // Groundtruth Depth 2
				if (gui_params_ir.use_taa) {
					ir_blit_multi_depth(fbo_out->color_textures[0], dataset.cam_views[nearest_views[2 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[3 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[4 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				else {
					ir_blit_multi_depth(fbo_out->color_textures[0], dataset.cam_views[nearest_views[3 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[4 + int(gui_params_ir.skipNearest)].id].tex_gpu, dataset.cam_views[nearest_views[5 + int(gui_params_ir.skipNearest)].id].tex_gpu);
				}
				break;
			case 8: // Output
				ir_blit_multi(fbo_res0->color_textures[0], dataset.cam_views[nearest_views[0 + int(gui_params_ir.skipNearest)].id].tex_gpu, fbo_res3->color_textures[0], fbo_out->color_textures[0]);
				break;
			
			}
			
		}
		if (takeScreenshot) {
			fbo_out->color_textures[0]->save_png(lastCubePos + "_Cube.png");
			takeScreenshot = false;
		}
		if (gui_params_ir.capturing) { // create data here if specified in the previous frame
			capture();
		}
		if (gui_params_ir.startCapturing) { // start capturing if specifies in gui_params_pcr. this is accessed via keyboard callback
			startCapture();
			gui_params_ir.startCapturing = false;
		}
		if (gui_params_ir.captureByIndex) { // create data here if specified in the previous frame
			captureByIndex();
		}
		if (gui_params_ir.startCaptureByIndex) { // start capturing if specifies in gui_params_pcr. this is accessed via keyboard callback
			startCaptureByIndex();
			gui_params_ir.startCaptureByIndex= false;
		}
		timerBlit->end();

		if (gui_params_ir.captureVideo) {
			captureVideoFrame();
		}
		if (gui_params_ir.startCaptureVideo) { // start capturing if specifies in gui_params_pcr. this is accessed via keyboard callback
			startCaptureVideo();
			gui_params_ir.startCaptureVideo = false;
		}

		//swap out and previous and old view matrix
		if (!gui_params_ir.taa_update_on_move || current_camera()->moved) {
			view_old = current_camera()->view;
			//swap fbo variable references 
			Framebuffer fbo_tmp = fbo_out;
			fbo_out = fbo_prev;
			fbo_prev = fbo_tmp;
			cur_out_fbo = fbo_out->name;
			current_camera()->clear_moved();
		}

		// finish frame
		Context::swap_buffers();

		//----------------------------------------------------------------------
		//break;
	}
	//----------------------------------------------------------------------

}

void InferenceRenderer::startCapture() {
	// create folder
	std::filesystem::create_directory("./out");
	std::filesystem::create_directory("./out/" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id]);

	dataset.currentCam = 0;
	setCurrentCam(0, gui_params_ir.cycleTestImages);
	gui_params_ir.fov = current_camera()->fov_degree;
	gui_params_ir.campos = current_camera()->pos;
	std::string lod_description = pointCloud_filenames[gui_params_ir.lod];
	if (setType[dataset_id] == "KITTY-360") {
		lod_description = "lod0";
	}
	// create folders if not present. location: "../../neural-point-rendering-training/data/
	if (!gui_params_ir.cycleTestImages) {
		std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/");
		std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_0_" + lod_description);
		if (!gui_params_ir.captureDepthOnly) {

			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_0_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_1_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_2_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_3_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_1_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_2_" + lod_description);
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_3_" + lod_description);
			for (int gt_i = 0; gt_i < gui_params_ir.captureGroundtruthAmount; ++gt_i) {
				std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_groundtruth_0");
				std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_depth_0_" + lod_description);
				std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_motion_0_" + lod_description);
			}
			std::filesystem::create_directory("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/groundtruth");
		}
	}
	gui_params_ir.capturing = true;
}

void InferenceRenderer::capture() {
	auto fbo_res0 = Framebuffer::find("fbo_res0");
	auto fbo_res1 = Framebuffer::find("fbo_res1");
	auto fbo_res2 = Framebuffer::find("fbo_res2");
	auto fbo_res3 = Framebuffer::find("fbo_res3");
	auto fbo_motion = Framebuffer::find("fbo_motion");
	auto fbo_out = Framebuffer::find(cur_out_fbo);
	//std::cerr << "NOT YET IMPLEMENTED SINCE BUFFER SWAP IS A THING DUE TO TAA" << std::endl;
	std::string img_prefix = "Image";
	std::string lod_description = pointCloud_filenames[gui_params_ir.lod];
	if (setType[dataset_id] == "KITTY-360") {
		img_prefix = "";
		lod_description = "lod0";
	}
	std::cout << "Capture View " << dataset.cam_names[dataset.currentCam] << "\r";
	if (gui_params_ir.cycleTestImages) {
		// write out rendered result
		if (!std::filesystem::exists(gui_params_ir.network_filenames[gui_params_ir.network_id] + "/" + dataset.cam_names_test[dataset.currentCam] + "_napr_full.png"))
			fbo_out->color_textures[0]->save_png("./out/" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/" + dataset.cam_names_test[dataset.currentCam] + "_napr_full.png");
	}
	else {
		if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
			fbo_res0->color_textures[1]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
		if (!gui_params_ir.captureDepthOnly) {
			// write out input data 0 col 1 depthcol 2 motion1 3 motion2 4 motion3
			if (!std::filesystem::exists("../../neural-point-rendering-training/" + setName[dataset_id] + "_new/data/input_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res0->color_textures[0]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_1_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res1->color_textures[0]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_1_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_2_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res2->color_textures[0]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_2_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_3_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res3->color_textures[0]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/input_3_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");


			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_1_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res1->color_textures[1]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_1_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_2_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res2->color_textures[1]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_2_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_3_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				fbo_res3->color_textures[1]->save_png("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/depth_3_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");

			for (int gt_i = 0; gt_i < gui_params_ir.captureGroundtruthAmount; ++gt_i) {
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_groundtruth_0/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
					dataset.cam_views[nearest_views[gt_i + int(gui_params_ir.skipNearest)].id].tex_gpu->save_png_rgb("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_groundtruth_0/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_depth_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
					dataset.cam_views[nearest_views[gt_i + int(gui_params_ir.skipNearest)].id].tex_gpu->save_png_depth_to_bw("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_depth_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");
				if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_motion_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
					fbo_res0->color_textures[gt_i + 2]->save_binary("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/nearest" + std::to_string(gt_i + 1) + "_motion_0_" + lod_description + "/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".bin");
			}

			//save groundtruth
			if (!std::filesystem::exists("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/groundtruth/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png"))
				dataset.cam_views[dataset.currentCam].tex_gpu->save_png_rgb("../../neural-point-rendering-training/data/" + setName[dataset_id] + "_new/groundtruth/" + img_prefix + dataset.cam_names[dataset.currentCam] + ".png");


			// write out rendered result
			if (!std::filesystem::exists(gui_params_ir.network_filenames[gui_params_ir.network_id] + "/" + dataset.cam_names[dataset.currentCam] + "_napr_full.png"))
				fbo_out->color_textures[0]->save_png("./out/" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/" + dataset.cam_names[dataset.currentCam] + "_napr_full.png");
			std::cout << "used fbo as out: " << fbo_out->name << std::endl;
		}
	}
	dataset.currentCam += 1;
	if (dataset.currentCam >= (gui_params_ir.cycleTestImages ? dataset.cam_names_test.size() :  dataset.camCount)) {
		gui_params_ir.capturing = false;
	}

	setCurrentCam(dataset.currentCam, gui_params_ir.cycleTestImages);
}

void InferenceRenderer::startCaptureByIndex() {
	std::filesystem::create_directory("./out");
	std::filesystem::create_directory("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id]);
	std::filesystem::create_directory("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural");
	std::filesystem::create_directory("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/gt");
	
	gui_params_ir.captureByIndexCurrent = gui_params_ir.captureByIndexStart;
	setCurrentCam(gui_params_ir.captureByIndexCurrent, gui_params_ir.cycleTestImages);
	gui_params_ir.captureByIndex = true;
}

void InferenceRenderer::captureByIndex() {
	auto fbo_out = Framebuffer::find(cur_out_fbo);
	if (!std::filesystem::exists("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural/" + dataset.cam_names[gui_params_ir.captureByIndexCurrent] + ".png"))
		fbo_out->color_textures[0]->save_png_rgb("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural/" + dataset.cam_names[gui_params_ir.captureByIndexCurrent] + ".png");
	if (!std::filesystem::exists("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/gt/" + dataset.cam_names[gui_params_ir.captureByIndexCurrent] + ".png"))
		dataset.cam_views[nearest_views[0].id].tex_gpu->save_png_rgb("./out/neural_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/gt/" + dataset.cam_names[gui_params_ir.captureByIndexCurrent] + ".png");
	gui_params_ir.captureByIndexCurrent += gui_params_ir.captureByIndexStep;
	if (gui_params_ir.captureByIndexCurrent >= dataset.camCount || gui_params_ir.captureByIndexCurrent > gui_params_ir.captureByIndexEnd) {
		gui_params_ir.captureByIndexCurrent = 0;
		gui_params_ir.captureByIndex = false;
	}
	setCurrentCam(gui_params_ir.captureByIndexCurrent, gui_params_ir.cycleTestImages);
}

void InferenceRenderer::startCaptureVideo() {
	if (!(setType[dataset_id] == "NavVis" || setType[dataset_id] == "TanksAndTemples")) {
		// create animation
		captureAnimation->clear();
		int step = 1;
		//if (setType[dataset_id] == "Redwood" || setType[dataset_id] == "ScanNet") step = 1;
		for (int anim_frame = 0; anim_frame <= dataset.camCount; anim_frame += step) {
			glm::mat4 view_of_frame = (getView(anim_frame));
			if (anim_frame % gui_params_ir.captureVideoSkipControlPoint == 0) {
				captureAnimation->push_node(dataset.cam_views[anim_frame].pose.position, glm::quat_cast(view_of_frame));
			}
		}
	}
    if(gui_params_ir.captureCustomAnimation){
        captureAnimation = customAnimation;
        gui_params_ir.captureVideoStartIndex = 0;
        gui_params_ir.captureVideoSkipControlPoint = 1;
    }
	captureAnimation->play();
	standardAnimation->play();
	
	captureAnimation->update(float(gui_params_ir.captureVideoStartIndex * 1000.f)/float(gui_params_ir.captureVideoSkipControlPoint));
	standardAnimation->update(float(gui_params_ir.captureVideoStartIndex) * 1000.f);

	// create folder
	std::filesystem::create_directory("./out");
	std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id]);
	std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_high");
	std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_low");
	for (int gt_i = 0; gt_i < gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id]; ++gt_i) {
		std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/n"+std::to_string(gt_i+1));
	}
	std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural");
    std::filesystem::create_directory("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/original_input_video");

	gui_params_ir.captureVideo = true;
	gui_params_ir.captureVideoIndex = 0;
	gui_params_ir.animationRunning = true;
}

void InferenceRenderer::captureVideoFrame() {
	std::string id = std::to_string(gui_params_ir.captureVideoIndex);
	while (id.size() < 5) id = "0" + id;
	auto fbo_res0 = Framebuffer::find("fbo_res0");
	auto fbo_res3 = Framebuffer::find("fbo_res3");
	auto fbo_out = Framebuffer::find(cur_out_fbo);
	auto fbo_prev = Framebuffer::find(cur_out_fbo);
	if (cur_out_fbo == "fbo_out_1") {
		fbo_prev = Framebuffer::find("fbo_out_2");
	}
	else {
		fbo_prev = Framebuffer::find("fbo_out_1");
	}
	

	// neural image
	if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural/" + id + ".png"))
		fbo_out->color_textures[0]->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/neural/" + id + ".png");
	/*// point renderings
	if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_high/" + id + ".png"))
		fbo_res0->color_textures[0]->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_high/" + id + ".png");
	if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_low/" + id + ".png"))
		fbo_res3->color_textures[0]->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/pr_low/" + id + ".png");
	// auxiliary images
	int start_gt_i = gui_params_ir.use_taa ? 1 : 0;
	for (int gt_i = start_gt_i; gt_i < gui_params_ir.network_groundtruth_amount[gui_params_ir.network_id]; ++gt_i) {
		if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/n" + std::to_string(gt_i+1) + "/" + id + ".png"))
			dataset.cam_views[nearest_views[gt_i - start_gt_i + int(gui_params_ir.skipNearest)].id].tex_gpu->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/n" + std::to_string(gt_i+1) + "/" + id + ".png");
	}
	if (gui_params_ir.use_taa) {
		if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/n1/" + id + ".png"))
			fbo_prev->color_textures[0]->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/n1/" + id + ".png");
	}*/


    if (!std::filesystem::exists("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/original_input_video/" + id + ".png")){
        int original_input_video_id = std::lroundf(standardAnimation->time);
        dataset.cam_views[original_input_video_id].tex_gpu->save_png_rgb("./out/video_" + setName[dataset_id] + "_" + gui_params_ir.network_filenames[gui_params_ir.network_id] + "/original_input_video/" + id + ".png");
    }


	gui_params_ir.captureVideoIndex++;
	if (!captureAnimation->running) {
		gui_params_ir.captureVideoIndex = 0;
		gui_params_ir.captureVideo = false;
		gui_params_ir.animationRunning = false;
	}
}

void InferenceRenderer::custom_gui_select_dataset() {
	glClearColor(0.25, 0.25, 0.25, 1.0);
	while (Context::running()) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw sample specific gui
		glm::ivec2 res = Context::resolution();

		glm::ivec2 sel_size = res / 2;



		ImGui::SetNextWindowSize(ImVec2(sel_size.x, sel_size.y));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.45f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.75f));
		//ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(1.f, 0.f, 0.f, 0.25f));
		if (ImGui::Begin("Dataset Selection", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)) {

			ImGui::SetWindowPos(ImVec2((res.x - sel_size.x) * 0.5, (res.y - sel_size.y) * 0.5));
			ImGui::SetWindowFontScale(1.5);
			auto windowWidth = ImGui::GetWindowSize().x;
			auto textWidth = windowWidth / 2; // ImGui::CalcTextSize("Dataset SelectionDataset Selection").x;
			auto tmpWidth = ImGui::CalcTextSize("Dataset Selection").x;
			ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
			ImGui::TextColored(ImVec4(1, 1, 1, 1), "\n\n\n     Inovis\nDataset Selection");
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::PushItemWidth(textWidth);
			if (ImGui::BeginCombo("##DatasetSelection", setName[dataset_id].c_str())) // The second parameter is the label previewed before opening the combo.
			{
				for (int n = 0; n < setName.size(); n++)
				{
					bool is_selected = (dataset_id == n); // You can store your selection however you want, outside or inside your objects
					if (ImGui::Selectable(setName[n].c_str(), is_selected)) {
						dataset_id = n;
						targetWidth = setWidth[dataset_id] ;
						targetHeight = setHeight[dataset_id] ;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
				}
				ImGui::EndCombo();
			}
			
			if (setType[dataset_id] == "KITTY-360") {
				tmpWidth = ImGui::CalcTextSize("PC Selection").x;
				ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
				ImGui::TextColored(ImVec4(1, 1, 1, 1), "PC Selection");

				auto ply_path = std::filesystem::path(setFolder[dataset_id]) / "data_3d_semantics/" / "train/" / setInfo[dataset_id] / "static/";
				std::vector<std::string> pointcloud_names;
                for(auto& p:  Helper::get_directory_entries_sorted((ply_path))){
                    pointcloud_names.emplace_back(p.path().filename().string());
                }
				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
				if (ImGui::BeginCombo("##startPLYSelection", pointcloud_names[setKittyPCRange.first].c_str())) // The second parameter is the label previewed before opening the combo.
				{
					for (int n = 0; n < pointcloud_names.size(); n++)
					{
						bool is_selected = (setKittyPCRange.first == n); // You can store your selection however you want, outside or inside your objects
						if (ImGui::Selectable(pointcloud_names[n].c_str(), is_selected)) {
							if(n <= setKittyPCRange.second)
								setKittyPCRange.first = n;
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
					}
					ImGui::EndCombo();
				}
				ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
				if (ImGui::BeginCombo("##endPLYSelection", pointcloud_names[setKittyPCRange.second].c_str())) // The second parameter is the label previewed before opening the combo.
				{
					for (int n = 0; n < pointcloud_names.size(); n++)
					{
						bool is_selected = (setKittyPCRange.second == n); // You can store your selection however you want, outside or inside your objects
						if (ImGui::Selectable(pointcloud_names[n].c_str(), is_selected)) {
							if (setKittyPCRange.first <= n)
								setKittyPCRange.second = n;
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
					}
					ImGui::EndCombo();
				}
				
				
			}
            if(setType[dataset_id] == "Generic" || setType[dataset_id] == "KITTY-360"){
                tmpWidth = ImGui::CalcTextSize("Test Set Start/Step").x;
                ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
                ImGui::TextColored(ImVec4(1, 1, 1, 1), "Test Set Start/Step");
                ImGui::Columns(4, "cols_test_set");
                ImGui::NextColumn();
                ImGui::InputInt("##teststart", &setGenericTestStartStep.first);
                ImGui::NextColumn();
                ImGui::InputInt("##teststep", &setGenericTestStartStep.second);
                ImGui::Columns();
            }
            tmpWidth = ImGui::CalcTextSize("Dataset Size").x;
			ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
			ImGui::TextColored(ImVec4(1, 1, 1, 1), "Dataset Size");

			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::InputInt("###number of images to load", &setSize[dataset_id]);

			std::string tmpTxt = (std::string("GT Resolution ") + std::to_string(setWidth[dataset_id]) + " x " + std::to_string(setHeight[dataset_id]));
			tmpWidth = ImGui::CalcTextSize(tmpTxt.c_str()).x;
			ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
			ImGui::TextColored(ImVec4(1, 1, 1, 1), tmpTxt.c_str());
			tmpWidth = ImGui::CalcTextSize("Target Resolution width x height").x;
			
			ImGui::Columns(4, "cols");
			ImGui::NextColumn();
			ImGui::InputInt("###targetwidth", &targetWidth, 1, 32);
			ImGui::NextColumn();
			ImGui::InputInt("###targetheight", &targetHeight, 1, 32);
			ImGui::Columns();

			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			ImGui::Checkbox("Enforce Resolution", &enforceTargetResolution);


			tmpWidth = ImGui::CalcTextSize("Network Selection").x;
			ImGui::SetCursorPosX((windowWidth - tmpWidth) * 0.5f);
			ImGui::TextColored(ImVec4(1, 1, 1, 1), "Network Selection");
			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			if (ImGui::BeginCombo("##NSelection", gui_params_ir.network_filenames[gui_params_ir.network_id].c_str())) // The second parameter is the label previewed before opening the combo.
			{
				for (int n = 0; n < gui_params_ir.network_filenames.size(); n++)
				{
					bool is_selected = (gui_params_ir.network_id== n); // You can store your selection however you want, outside or inside your objects
					if (ImGui::Selectable(gui_params_ir.network_filenames[n].c_str(), is_selected)) {
						gui_params_ir.network_id = n;
						gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[n];
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
				}
				ImGui::EndCombo();
			}

			

			ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
			if(ImGui::Button("LOAD",ImVec2(textWidth,35))){
                // ending stuff when exited with a break
                ImGui::PopItemWidth();
                ImGui::End();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);
                Context::swap_buffers();
				break;
			}

			ImGui::PopItemWidth();

		}
        ImGui::End();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(2);


		Context::swap_buffers();
	}

	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void InferenceRenderer::custom_gui_draw() {
	// draw sample specific gui
	int height = 280; // height of a gui on the bottom
	int width = 340;  // width of a gui at the right
	glm::ivec2 res = Context::resolution();
	
	ImGui::SetNextWindowSize(ImVec2(width, res.y));//, ImGuiCond_Once);
	

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.45f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.75f));
	if (ImGui::Begin("Settings", (bool*)0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove )) {// , ImGuiWindowFlags_NoDecoration);// | ImGuiWindowFlags_NoBackground);

		ImGui::SetWindowPos(ImVec2(res.x - width, 0));
		/* ImGui::Checkbox("Debug", &debugBool);
		ImGui::SliderInt("DebugInt", &debugInt,0,dataset.camCount);*/
		ImGui::TextColored(ImVec4(1.0, 0.5, 0.5, 1.0), "Capture Settings");
		ImGui::Checkbox("Depth Only", &gui_params_ir.captureDepthOnly);
		ImGui::SliderInt("Number Aux Imgs", &gui_params_ir.captureGroundtruthAmount, 1, 6);

		/*ImGui::Text("CaptureByIndex Settings");
		ImGui::InputInt("Start", &gui_params_ir.captureByIndexStart );
		ImGui::InputInt("End", &gui_params_ir.captureByIndexEnd);
		ImGui::InputInt("Step", &gui_params_ir.captureByIndexStep );
		ImGui::Separator();
		ImGui::Text("CaptureVideo Settings");
		ImGui::InputInt("Step Control Point", &gui_params_ir.captureVideoSkipControlPoint);
		ImGui::InputInt("Speed (ms)", &gui_params_ir.captureVideoFrameTime);
		ImGui::InputInt("Start Index", &gui_params_ir.captureVideoStartIndex);
        ImGui::Checkbox("Capture Custom Animation", &gui_params_ir.captureCustomAnimation);*/
		// first column for all settings regarding the navvis parsing
		ImGui::TextColored(ImVec4(1.0, 1.0, 0.5, 1.0), "Dataset Settings");
        ImGui::Checkbox("Debug Current Frustum", &gui_params_ir.debug_camera_frustum_currentCam);
		ImGui::SliderFloat("Frustum Size Current", &gui_params_ir.debug_camera_frustum_size_current, 0.01, 30.0);
		ImGui::Checkbox("Debug Camera Frusta", &gui_params_ir.debug_camera_frustum);

        if(ImGui::SliderFloat("Frustum Size", &gui_params_ir.debug_camera_frustum_size, 0.01, 1.0)){
            gui_params_ir.debug_camera_frustum_size_adjusted = true;
        }
        
        ImGui::SliderInt("Frustum Amount", &gui_params_ir.debug_frustum_amount, 1, dataset.camCount);

		if (gui_params_ir.parsedTestImages) {
			if (ImGui::Checkbox("Test Images", &gui_params_ir.cycleTestImages)) {
				setCurrentCam(dataset.currentCam, gui_params_ir.cycleTestImages);
			}
		}
		// camera frame selection
		ImGui::TextUnformatted("Current Camera");
		auto& cam_names = gui_params_ir.cycleTestImages ? dataset.cam_names_test : dataset.cam_names;
		if (ImGui::BeginCombo("##CurrentCam", cam_names[dataset.currentCam].c_str())) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < cam_names.size(); n++)
			{
				bool is_selected = (dataset.currentCam == n); // You can store your selection however you want, outside or inside your objects
				if (ImGui::Selectable(cam_names[n].c_str(), is_selected)) {
					dataset.currentCam = n;

					setCurrentCam(n, gui_params_ir.cycleTestImages);
					gui_params_ir.fov = current_camera()->fov_degree;
					gui_params_ir.campos = current_camera()->pos;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		/*if (ImGui::Checkbox("Optimize Poses", &gui_params_ir.use_darius_optimized_positions)) {
			if (dataset.cam_views[dataset.currentCam].darius_optimized_pose_present) {
				setCurrentCam(dataset.currentCam, gui_params_ir.cycleTestImages);
				gui_params_ir.fov = current_camera()->fov_degree;
				gui_params_ir.campos = current_camera()->pos;
			}
			else gui_params_ir.use_darius_optimized_positions = false; // no optimizatrion present anyways


		}*/

		ImGui::Checkbox("Log Nearest Views", &gui_params_ir.log_nearest_views);
		/*ImGui::Checkbox("Use Parsed Proj", &gui_params_ir.use_dataset_proj);
		if (gui_params_ir.use_dataset_proj) {
			ImGui::Checkbox("Use Parsed Proj Cropped", &gui_params_ir.use_dataset_proj_cropped);
		}*/
		ImGui::Checkbox("Use Timestamp", &gui_params_ir.use_timestamp);
		if (gui_params_ir.use_timestamp) {
			ImGui::Text("Timestamp Min,Max: %d : %d", gt_timestamp_min, gt_timestamp_max);
		}

		ImGui::Separator();

		ImGui::TextColored(ImVec4(1.0, 0.5, 1.0, 1.0), "Render Settings");
		glm::ivec2 res = Context::resolution();
		ImGui::Text("Context/Window res:\n %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("fbo_res0")->w,Framebuffer::find("fbo_res0")->h);
		ImGui::Text("FBO res0: %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("fbo_res1")->w, Framebuffer::find("fbo_res1")->h);
		ImGui::Text("FBO res1: %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("fbo_res2")->w, Framebuffer::find("fbo_res2")->h);
		ImGui::Text("FBO res2: %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("fbo_res3")->w, Framebuffer::find("fbo_res3")->h);
		ImGui::Text("FBO res3: %d,%d", res.x, res.y);
		res = glm::ivec2(Framebuffer::find("fbo_motion")->w, Framebuffer::find("fbo_motion")->h);
		ImGui::Text("FBO motion: %d,%d", res.x, res.y);
		//reset button to get beack to default image resolution
		/*if (ImGui::Button("Reset Res")) {		
			std::cout << "Reset Resolution to Initial Value" << std::endl;
			gui_params_ir.resolution_modifier = 1;
			int mod = (gui_params_ir.initial_resolution_default.y < 560) ? 4 : 2;
			Context::resize(gui_params_ir.initial_resolution_default.x * mod, gui_params_ir.initial_resolution_default.y * mod);
			ir_resize_callback(0,0); // dimesnions are not used anyways
			ir_resize_motion_buffer();
			
			
		}*/

		//combo for render content
		ImGui::Text("Display Mode");
		if (ImGui::Combo("##Display Mode", &gui_params_ir.displayMode, "Single\0Multi\0")) {
			if(gui_params_ir.displayMode == 0)
				gui_params_ir.currentRenderInfo = 17;
			else 
				gui_params_ir.currentRenderInfo = 8;
		}
		ImGui::Text("Render Content");
		if (gui_params_ir.displayMode == 0) {	// Single Input
			ImGui::Combo("##RenderContent", &gui_params_ir.currentRenderInfo, "Color Res 0\0Color Res 1\0Color Res 2\0Color Res 3\0Depth Res 0\0Depth Res 1\0Depth Res 2\0Depth Res 3\0Motion 1\0Motion 2\0Motion 3\0Groundtruth 1\0Groundtruth 2\0Groundtruth 3\0GT Depth 1\0GT Depth 2\0GT Depth 3\0Output\0");
			if(gui_params_ir.currentRenderInfo >= 4 && gui_params_ir.currentRenderInfo < 8 )
				ImGui::Checkbox("White Depth BG", &gui_params_ir.show_depth_bg_white);
		}
		else { // Multi Output
			ImGui::Combo("##RenderContent", &gui_params_ir.currentRenderInfo, "Color\0Depth\0Motion\0Motion 2\0Groundtruth\0Groundtruth 2\0GT Depth\0GT Depth 2\0Output\0");
			if (gui_params_ir.currentRenderInfo == 1)
				ImGui::Checkbox("White Depth BG", &gui_params_ir.show_depth_bg_white);
				
		}
		/*if(ImGui::Checkbox("MipMap Motion", &gui_params_ir.mipmap_motion)) ir_resize_motion_buffer();*/
		/*ImGui::Checkbox("Show Debug Diff", &gui_params_ir.show_diff);*/
		ImGui::Checkbox("Pre Init MV", &gui_params_ir.preInitMV);
		// network used for inference
		ImGui::Text("Network Type");
		if (ImGui::BeginCombo("##Network Type", gui_params_ir.network_filenames[gui_params_ir.network_id].c_str())) {
			for (int i = 0; i < gui_params_ir.network_filenames.size(); ++i) {
				const bool isSelected = (gui_params_ir.network_id == i);
				if (ImGui::Selectable(gui_params_ir.network_filenames[i].c_str(), isSelected)) {
					gui_params_ir.prev_network_id = gui_params_ir.network_id;
					gui_params_ir.network_id = i;
					gui_params_ir.preInitMV = gui_params_ir.network_prevInitMV[gui_params_ir.network_id];
					ir_resize_motion_buffer();
				}

				// Set the initial focus when opening the combo
				// (scrolling + keyboard navigation focus)
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}


		/*ImGui::Text("Resolution Modifier");
		//combo for resolution modifier
		if (ImGui::Combo("##resolution Modifier", &gui_params_ir.resolution_modifier, "2\0 1\0 0.5\0 0.25\0 0.125\0 0.0625")) {
			std::cout << "Resolution Modifier Switch to " << gui_params_ir.resolution_modifier << " -> Resize FBOs" << std::endl;
			
			int mod = (gui_params_ir.initial_resolution_default.y < 560) ? 4 : 2;
			Context::resize(gui_params_ir.initial_resolution_default.x* mod, gui_params_ir.initial_resolution_default.y* mod);
            ir_resize_callback(0,0); // dimesnions are not used anyways
			ir_resize_motion_buffer();
			

		}*/
		


		// enable culling
		/*ImGui::Checkbox("Culling", &gui_params_ir.enableCulling);*/

		ImGui::Checkbox("Skip Nearest Groundtruth", &gui_params_ir.skipNearest);
		ImGui::Checkbox("Use Prev as GT", &gui_params_ir.use_taa);
		if(gui_params_ir.use_taa)
			ImGui::Checkbox("Update on Move Only", &gui_params_ir.taa_update_on_move);

		// combo for LOD selection
		/*ImGui::Text("LOD");

		//ImGui::Combo("##network type", &gui_params_ir.network_id, "NAPR2\0Aliev\0");

		if (ImGui::BeginCombo("##LOD", pointCloud_filenames[gui_params_ir.lod].c_str())) {
			for (int i = 0; i < pointCloud_filenames.size(); ++i) {
				const bool isSelected = (gui_params_ir.lod == i);
				if (ImGui::Selectable(pointCloud_filenames[i].c_str(), isSelected)) {
					gui_params_ir.lod = i;
					glPointSize(std::max(1, gui_params_ir.pointSizeGL));
				}

				// Set the initial focus when opening the combo
				// (scrolling + keyboard navigation focus)
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}*/

		// display how big points are rendered
		// slider for point size modifier
		/*ImGui::Text("Point Size");
		if (ImGui::SliderInt("##PointSize", &gui_params_ir.pointSizeGL, 1, 5)) {
			// dont need point size for oriented quads
			glPointSize(std::max(1, gui_params_ir.pointSizeGL));// glPointSize(std::max(1, int(gui_params_ir.pointSizeGL * gui_params_ir.lodPointSizesGL[gui_params_ir.lod])));
		}*/
			



		ImGui::Separator();

		ImGui::TextColored(ImVec4(0.5, 1.0, 1.0, 1.0), "Camera Settings");

		/*if (ImGui::Button("Screenshot Cube")) {
			takeScreenshot = true;
			
		}
		if (ImGui::Button("Left")) {
			lastCubePos = "Left";
			current_camera()->dir = vec3(0, 1, 0);
			current_camera()->up = vec3(0, 0, 1);
		}
		if (ImGui::Button("Front")) {
			lastCubePos = "Front";
			current_camera()->dir = vec3(1, 0, 0);
			current_camera()->up = vec3(0, 0, 1);
		}
		if (ImGui::Button("Right")) {
			lastCubePos = "Right";
			current_camera()->dir = vec3(0, -1, 0);
			current_camera()->up = vec3(0, 0, 1);
		}
		if (ImGui::Button("Back")) {
			lastCubePos = "Back";
			current_camera()->dir = vec3(-1, 0, 0);
			current_camera()->up = vec3(0, 0, 1);
		}
		if (ImGui::Button("Top")) {
			lastCubePos = "Top";
			current_camera()->dir = vec3(0, 0, 1);
			current_camera()->up = vec3(-1, 0, 0);
		}
		if (ImGui::Button("Down")) {
			lastCubePos = "Down";
			current_camera()->dir = vec3(0, 0, -1);
			current_camera()->up = vec3(1, 0, 0);
		}*/

		/*ImGui::Text("FOV");
		if (ImGui::SliderFloat("##FOV", &gui_params_ir.fov, 25.f, 90.f)) {
			current_camera()->fov_degree = gui_params_ir.fov;
		}*/

		ImGui::Text("Cam Pos");
		if (ImGui::SliderFloat3("##Campos", &(gui_params_ir.campos.x), -50.f, 50.f)) {
			current_camera()->pos = gui_params_ir.campos;
		}

		// display camera direction
		ImGui::Text("Cam Dir");
		ImGui::Text("%.3f, %.3f, %.3f", current_camera()->dir.x, current_camera()->dir.y, current_camera()->dir.z);
		if (ImGui::Button("Print Cam")) {
			// push_node(vec3(-5.349366, -4.661439, 1.361215), glm::quat(0.329532, { -0.141117, 0.699628, 0.618074 }));
			std::cerr << "captureAnimation->push_node(" << current_camera()->pos << ", glm::" << glm::quat_cast(current_camera()->view) << ");" << std::endl;
			//std::cerr << "Current Camera at pos: " << current_camera()->pos << " and rot " << glm::quat_cast(current_camera()->view) << "; direction: " << glm::normalize(current_camera()->dir) << "; up: " << glm::normalize(current_camera()->up) << std::endl;
			//std::cerr << "word dir transformed with view matrix of nearest frame" <<  std::endl;
			glm::mat4 v = glm::mat4(1);
			if (gui_params_ir.use_darius_optimized_positions && dataset.cam_views[dataset.currentCam].darius_optimized_pose_present) {
				v = dataset.cam_views[dataset.currentCam].optimized_pose.view;
			}
			else {
				v = dataset.cam_views[dataset.currentCam].pose.view;
			}
			//std::cerr << glm::mat3(v) * current_camera()->dir << std::endl;
		}

		ImGui::TextColored(ImVec4(0.5, 0.5, 1.0, 1.0), "Capture Settings");
		ImGui::Text("Run Animation");
		ImGui::Checkbox("##AnimationRunning" , &gui_params_ir.animationRunning);
		ImGui::Text("Animation Speed");
		ImGui::SliderFloat("##AnimationSpeed", &gui_params_ir.animationSpeed, 0.01f, 10.f);
		ImGui::Text("Animation Forward");
		ImGui::Checkbox("##AnimationForward", &gui_params_ir.animationForward);
        ImGui::Text("Use Secondary Animation");
        if(ImGui::Checkbox("##use_secondary_anim", &gui_params_ir.animation_use_secondary)){
            if(gui_params_ir.animation_use_secondary && customAnimation->length() < 2) {
                gui_params_ir.animation_use_secondary=false;
            }
        }
        ImGui::Text("Animation show Capture Frustum");
        ImGui::Checkbox("##anim_show_frustum", &gui_params_ir.animation_show_capture_frustum);
        ImGui::Text("Visualize Primary");
        ImGui::Checkbox("##anim_vis_primary", &gui_params_ir.animation_visualize_primary_path);
        ImGui::Text("Visualize Secundary");
        ImGui::Checkbox("##anim_vis_secundary", &gui_params_ir.animation_visualize_secondary_path);
        if(ImGui::Button("Print Animation")){
            std::cout << "---------------------------" << std::endl << "Custom (Secondary) Animation: current animation trajectory:" << std::endl;
            customAnimation->print_path();
        }
        if(ImGui::Button("Add Pose")) {
            customAnimation->push_node(current_camera()->pos, glm::quat_cast(current_camera()->view));
            custom_to_standard_factor = float(standardAnimation->camera_path.size()) / float(customAnimation->camera_path.size());

            customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;
            std::cout << "---------------------------" << std::endl << "Custom (Secondary) Animation: added pose: " << current_camera()->pos << ", " << glm::quat_cast(current_camera()->view) << std::endl;
            std::cout << "standardAnimation length: " << standardAnimation->length() << ", customAnimation length: " << customAnimation->length() << ", factor: " << custom_to_standard_factor << std::endl;
                      std::cout << "current animation trajectory:" << std::endl;
            customAnimation->print_path();

        }
        if(ImGui::Button("Clear Animation")){
            std::cout << "---------------------------" << std::endl << "Custom (Secondary) Animation cleared" << std::endl;
            customAnimation->clear();
            custom_to_standard_factor = 1.f;
            customAnimation->ms_between_nodes = 1000.f * custom_to_standard_factor;
        }



	}
	else {
	ivec2 tmp_res = Context::resolution();
		ImGui::SetWindowPos(ImVec2(tmp_res.x - width, 0), ImGuiCond_Always);
		
	}

    ImGui::End();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);



}

void InferenceRenderer::createRandomAnimation(glm::vec3 endpos, glm::quat endrot) {
	myAnimation->clear();
	
	// create random start point.
	vec3 pos(1, 1, 1);
	while (length(pos) > 1.f) {
		pos = vec3(std::rand(), std::rand(), std::rand()) / float(RAND_MAX);
		pos = pos * 2.f - vec3(1.f, 1.f, 1.f);
	}
	pos = normalize(pos);
	// scale to range of 2.5 to 5 around the point
	float dist = 2.5f + (std::rand() / RAND_MAX) * 5.f;
	pos *= dist;
	// random rotation
	float x, y, z, u, v, w, s;
	do {
		x = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		y = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		z = x * x + y * y;
	} while (z > 1.f);
	do {
		u = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		v = (std::rand() / float(RAND_MAX)) * 2.f - 1.f;
		w = u * u + v * v;
	} while (w > 1.f);
	s = std::sqrt((1 - z) / w);
	glm::quat q(x,y,s*u,s*v);
	q = normalize(q);
	myAnimation->push_node(endpos+pos, q);
	myAnimation->push_node(endpos, endrot);

}