#include "dataset.h"
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <glm/gtc/matrix_access.hpp>

#include "../util/xml/pugixml.hpp"
#include "../external/advancedcppgl/external/cppgl/src/stb_image.h"
#include "../external/advancedcppgl/external/cppgl/src/stb_image_write.h"
#include "../util/json/json11.hpp"


inline GLint channels_to_float_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA32F : channels == 3 ? GL_RGB32F : channels == 2 ? GL_RG32F : GL_R32F;
}
inline GLint channels_to_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA : channels == 3 ? GL_RGB : channels == 2 ? GL_RG : GL_RED;
}
inline GLint channels_to_ubyte_format(uint32_t channels) {
	return channels == 4 ? GL_RGBA8 : channels == 3 ? GL_RGB8 : channels == 2 ? GL_RG8 : GL_R8;
}

std::vector<Capture_View> Dataset::getCaptureViewList() {
	std::vector<Capture_View> views;

	for (int i = 0; i < camCount; ++i) {
		// add parsed cam positions to the nearest_view list
		if (cam_views[i].darius_optimized_pose_present) {
			views.emplace_back(i, cam_views[i].optimized_pose.position, cam_views[i].optimized_pose.direction);
		}
		else {
			views.emplace_back(i, cam_views[i].pose.position, cam_views[i].pose.direction);
		}
	}
	return views;
}


//quat is a cam_to_world quaternion -> columns are axes -> multiply transform matrix from right to switch axes
inline glm::quat ColMap_to_OpenGL(glm::quat q, bool verbose = false) {



	glm::mat4 mat_ColMap = glm::mat4(q);
	if (verbose) printMat("Transform COLMAP Orientation", mat_ColMap);
	glm::mat4 ColMap_to_OpenGL = glm::mat4(1);
	ColMap_to_OpenGL[1][1] = -1; 
	ColMap_to_OpenGL[2][2] = -1;
	if (verbose) printMat("Transform_Matrix", ColMap_to_OpenGL);
	glm::mat4 mat_OpenGL = mat_ColMap * ColMap_to_OpenGL;
	if (verbose) printMat("To OpenGL Orientation", mat_OpenGL);
	if (verbose) printMat("To OpenGL Orientation transformed twice", glm::mat4_cast(glm::quat_cast(mat_OpenGL)));
	return glm::quat_cast(mat_OpenGL);
}

//mat4 is a cam_to_world matrix -> columns are axes -> multiply transform matrix from right to switch axes 
inline glm::mat4 ColMap_to_OpenGL(glm::mat4 m) {


	//printMat("Transform COLMAP Orientation", mat_ColMap);
	glm::mat4 ColMap_to_OpenGL = glm::mat4(1);
	ColMap_to_OpenGL[1][1] = -1;
	ColMap_to_OpenGL[2][2] = -1;
	//printMat("Transform_Matrix", ColMap_to_OpenGL);
	glm::mat4 mat_OpenGL =  m * ColMap_to_OpenGL;
	//printMat("To OpenGL Orientation", mat_OpenGL);
	return mat_OpenGL;
}

/// <summary>
/// parse a json file containing the positions and orientations of the 4 cams + the head
/// </summary>
/// <param name="file">file path to the json file</param>
/// <returns>vector of poses from the info json file</returns>
static std::vector<View::Pose> parseJsonFile(std::filesystem::path file, bool isColmap = false)
{
	std::vector<View::Pose> poses;
	std::fstream in(file);
	if (!in) throw std::runtime_error(std::string("no such file") + file.string());
	std::string contents;
	in.seekg(0, std::ios::end);
	contents.resize(in.tellg());
	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	in.close();
	std::string err;
	auto json = json11::Json::parse(contents, err);
	if (json == json11::Json())
		throw std::runtime_error(std::string("cannot parse file") + file.string());

	if (!json.is_object()) throw std::runtime_error("unexpected type");
	std::vector<std::string> strings;
	for (int i = 0; i != NUMBER_OF_CAMERAS_PER_CP; ++i) {
		strings.push_back("cam" + std::to_string(i));
	}
	strings.push_back("cam_head");
	strings.push_back("footprint");
	int cntr = 0;
	for (auto s : strings) {
		auto& o = json.object_items().at(s);
		View::Pose c;
		auto pos = o.object_items().at("position");
		c.position.x = pos.array_items()[0].number_value();
		c.position.y = pos.array_items()[1].number_value();
		c.position.z = pos.array_items()[2].number_value();
		auto ori = o.object_items().at("quaternion");
		c.orientation.w = ori.array_items()[0].number_value();
		c.orientation.x = ori.array_items()[1].number_value();
		c.orientation.y = ori.array_items()[2].number_value();
		c.orientation.z = ori.array_items()[3].number_value();
		if (isColmap) {
			c.orientation = ColMap_to_OpenGL(c.orientation);
		}

		c.direction = glm::vec3(glm::mat4_cast(c.orientation)[2]);

		poses.push_back(c);
	}

	return poses;
}

static std::vector<View::Pose> getOptimizedPose(std::ifstream& opti_file, int cap_pos_num, int numOfCams, bool isColmap = false) {
	std::vector<View::Pose> result;
	//skip n lines
	std::string line;
	for (int i = 0; i < (cap_pos_num * numOfCams); ++i)
		std::getline(opti_file, line);

	//get correct line
	//std::getline(opti_file, line);
	for (int i = 0; i < numOfCams; ++i) {
		float q_w, q_x, q_y, q_z, p_x, p_y, p_z;
		opti_file >> q_w >> q_x >> q_y >> q_z >> p_x >> p_y >> p_z;
		//remove newline
		std::getline(opti_file, line);
		View::Pose pose_opti;
		pose_opti.position = glm::vec3(p_x, p_y, p_z);
		pose_opti.orientation = glm::quat(q_w, q_x, q_y, q_z);
		if (isColmap) {
			pose_opti.position = glm::vec3(p_x, p_y, p_z);
			//pose_opti.orientation = glm::quat(q_w, q_x, -q_y, -q_z);
			pose_opti.orientation = ColMap_to_OpenGL(pose_opti.orientation);
		}
		pose_opti.direction = glm::vec3(glm::mat4_cast(pose_opti.orientation)[2]);

		result.emplace_back(pose_opti);
	}


	return result;
}

static std::vector<View::Pose> getPoses(std::ifstream& opti_file, int numOfCams, std::string quat_order = "WXYZ", bool isColmap = false, bool verbose = false) {
	std::vector<View::Pose> result;
	//skip n lines
	std::string line;

	//all lines
	//std::getline(opti_file, line);
	for (int i = 0; i < numOfCams; ++i) {
		float q_w, q_x, q_y, q_z, p_x, p_y, p_z;
		if (quat_order == "WXYZ") {
			opti_file >> q_w >> q_x >> q_y >> q_z >> p_x >> p_y >> p_z;
		}
		else if (quat_order == "XYZW") {
			opti_file >> q_x >> q_y >> q_z >> q_w >> p_x >> p_y >> p_z;
		}
		else {
			std::cout << "[getPoses] FATAL ERROR: quaternion order not recognized: " << quat_order << std::endl;
		}
		
		//remove newline
		std::getline(opti_file, line);
		View::Pose pose_opti;
		pose_opti.position = glm::vec3(p_x, p_y, p_z);
		pose_opti.orientation = glm::quat(q_w, q_x, q_y, q_z);
		//std::cout << std::endl;
		//printMat("initial quat: ", glm::mat4(pose_opti.orientation));
		//if (!isInverse) {
		//	/*std::cout << "quat: " << pose_opti.orientation << std::endl;
		//	std::cout << "direct inverse: " << glm::inverse(pose_opti.orientation) << std::endl;
		//	std::cout << "transform inverse: " << glm::quat(glm::inverse(glm::mat4(pose_opti.orientation))) << std::endl;*/
		//	pose_opti.orientation = glm::inverse(pose_opti.orientation);

		//}
		if (isColmap) {
			//pose_opti.position = glm::vec3(p_y, -p_x, p_z);
			//pose_opti.orientation = glm::quat(q_w, q_y, -q_x, q_z);
			pose_opti.position = glm::vec3(p_x, p_y, p_z);
			//pose_opti.orientation = glm::quat(q_w, q_x, -q_y, -q_z);
			//std::cout << "before colmap transform: " << pose_opti.orientation << std::endl;
			//(std::cout << "inverse                : " << glm::inverse(pose_opti.orientation << std::endl;
			pose_opti.orientation = ColMap_to_OpenGL(pose_opti.orientation, verbose);
			//std::cout << "after  colmap transform: " << pose_opti.orientation << std::endl;
			/*auto mat = glm::mat4_cast(glm::quat(q_w, q_x, q_y, q_z));
			mat[1] = -mat[1];
			mat[2] = -mat[2];
			pose_opti.orientation = glm::quat_cast(mat);*/
		}
		pose_opti.direction = glm::vec3(glm::mat4_cast(pose_opti.orientation)[2]);
		//printMat("final quat: ", glm::mat4(pose_opti.orientation));
		result.emplace_back(pose_opti);
	}


	return result;
}

static std::vector<std::pair<int, View::Pose>> getPosesKITTY(std::ifstream& pose_file, bool isColmap = true) {
	std::vector<std::pair<int, View::Pose>> result;
	std::string line;
	int count = 0;
	while (!pose_file.eof()) {
		//int id;
		//View::Pose pose;
		//glm::mat4 cam0_to_world;
		//pose_file >> id >> cam0_to_world[0][0] >> cam0_to_world[1][0] >> cam0_to_world[2][0] >> cam0_to_world[3][0] // row 0 -> col 1 to 3 
		//				>> cam0_to_world[0][1] >> cam0_to_world[1][1] >> cam0_to_world[2][1] >> cam0_to_world[3][1] // row 1 -> col 1 to 3 
		//				>> cam0_to_world[0][2] >> cam0_to_world[1][2] >> cam0_to_world[2][2] >> cam0_to_world[3][2] // row 2 -> col 1 to 3 
		//				>> cam0_to_world[0][3] >> cam0_to_world[1][3] >> cam0_to_world[2][3] >> cam0_to_world[3][3];// row 3 -> col 1 to 3 
		////std::cout << id << " : " <<  cam0_to_world << std::endl;
		//cam0_to_world[0] = -cam0_to_world[0];
		//pose.view = glm::inverse(cam0_to_world);// glm::transpose(cam0_to_world);
		//glm::vec4 col0 = cam0_to_world[0];
		//cam0_to_world[0] = cam0_to_world[1];
		//cam0_to_world[1] = col0;

		//glm::vec4 pos4 = cam0_to_world * glm::vec4(0, 0, 0, 1);
		//pose.position = glm::vec3(pos4.x,pos4.y,pos4.z) / pos4.w;
		//glm::vec3 dir = glm::mat3(cam0_to_world) * glm::vec3(0, 0, 1);
		//pose.direction = dir;
		//pose.orientation = glm::quat_cast(cam0_to_world);
		////std::cout << pose.position << " : " << pose.view << std::endl;
		//result.emplace_back(std::pair<int, View::Pose>(id, pose));
		
		
		int id;
		View::Pose pose;
		glm::mat4 cam0_to_world;
		pose_file >> id >> cam0_to_world[0][0] >> cam0_to_world[1][0] >> cam0_to_world[2][0] >> cam0_to_world[3][0] // row 0 -> col 1 to 3 
						>> cam0_to_world[0][1] >> cam0_to_world[1][1] >> cam0_to_world[2][1] >> cam0_to_world[3][1] // row 1 -> col 1 to 3 
						>> cam0_to_world[0][2] >> cam0_to_world[1][2] >> cam0_to_world[2][2] >> cam0_to_world[3][2] // row 2 -> col 1 to 3 
						>> cam0_to_world[0][3] >> cam0_to_world[1][3] >> cam0_to_world[2][3] >> cam0_to_world[3][3];// row 3 -> col 1 to 3 
		
		glm::mat4 toGL = glm::mat4(1);
		toGL[1][1] = -1;
		toGL[2][2] = -1;

		
		cam0_to_world = cam0_to_world * toGL;
		pose.direction = -cam0_to_world[2];
		pose.position = glm::vec3(cam0_to_world[3]);

		// following weird code was the manual correction of the principal point offset
		/*glm::vec3 dir_target = glm::normalize(glm::vec3(0.039, 0.091, -0.995));
		glm::vec3 dir_source = glm::vec3(0,0,-1);
		vec3 corrected_dir = glm::mat3(cam0_to_world) * (dir_target);
		pose.direction = glm::normalize(corrected_dir);
		glm::vec3 right = glm::normalize(glm::cross(pose.direction, glm::normalize(glm::vec3(cam0_to_world[1]))));
		glm::vec3 up = glm::normalize(glm::cross(right, pose.direction));
		pose.view = glm::lookAt(pose.position, pose.position + pose.direction, up);*/
		
		
		//glm::vec3 cross_product = glm::cross(dir_source, dir_target);
		//// view_correction is in camera/view space
		//glm::quat view_correction;
		//view_correction.x = cross_product.x;
		//view_correction.y = cross_product.y;
		//view_correction.z = cross_product.z;
		//view_correction.w = 1.f + glm::dot(dir_source,dir_target);
		//view_correction = glm::normalize(view_correction); 

		// transform offsetted direction vector to world space with given quat

		
		//glm::vec3 dir = glm::normalize(glm::vec3(cam0_to_world[2])); // view direction in world space
		  //glm::normalize(glm::vec3(view_correction * cam0_to_world[2]));
		//glm::vec3 up = glm::normalize(glm::vec3(cam0_to_world[1]));  // up direction
		
		
		/*std::cout << "dir  " << cam0_to_world[2] << std::endl;
		std::cout << "dirc " << corrected_dir << std::endl;
		std::cout << "dirt " << dir_target<< std::endl;
		std::cout << std::endl;*/

		//glm::quat corrected = glm::quat(glm::transpose(cam0_to_world)) * view_correction;

		//glm::vec4 pos4 = cam0_to_world[3];
		//cam0_to_world = glm::transpose(glm::mat4(corrected));
		//cam0_to_world[3] = pos4;

		
		//glm::normalize(glm::vec3(view_correction * cam0_to_world[2]));
		
		//pose.direction =  glm::normalize(glm::vec3(cam0_to_world[2]));
		
		//pose.orientation = glm::quat(cam0_to_world);

		



		//if (id == 4931) printMat(std::to_string(id) + std::string(": toGL: "), toGL);
		//if (id == 4931) printMat(std::to_string(id) + std::string(": cam to world: "), cam0_to_world);


		//glm::mat4 world_to_cam0 = glm::inverse(cam0_to_world);
		//if (id == 4931) printMat(std::to_string(id) + std::string(": world to cam: "), world_to_cam0);


		
		pose.view = glm::inverse(cam0_to_world);
		pose.orientation = glm::quat(glm::transpose(pose.view));
		/*std::cout << "dir  " << pose.view * cam0_to_world[2] << std::endl;
		std::cout << "dirc " << glm::mat3(pose.view) * corrected_dir << std::endl;
		std::cout << "dirt " << dir_target << std::endl;*/
		//// cam coordinate system is: x = right, y = down, z = forward 
		//// OGL coordinate system is: x = right, y = up  , z = back (-z convention)
		//// -> negate y and z
		////cam0_to_world[1] = -cam0_to_world[1];
		////cam0_to_world[2] = -cam0_to_world[2];

		//if (id ==4931) std::cout << id << ": cam to world n: " << cam0_to_world << std::endl << " -> invert " << std::endl;
		//glm::mat3 world_to_cam0 = glm::transpose(glm::mat3(cam0_to_world));
		////world_to_cam0[1] = -world_to_cam0[1];
		////world_to_cam0[2] = -world_to_cam0[2];
		//if (id == 4931) std::cout << id << ": world to cam i: " << glm::transpose(glm::mat3(cam0_to_world)) << std::endl;

		////world_to_cam0[0] = glm::normalize(world_to_cam0[0]);
		////world_to_cam0[1] = glm::normalize(world_to_cam0[1]);
		////world_to_cam0[2] = glm::normalize(world_to_cam0[2]);
		//if (id == 4931) std::cout << id << ": world to cam: " << world_to_cam0 << std::endl;
		//pose.view = glm::mat4(world_to_cam0);
		//
		//pose.view[3] = cam0_to_world[3];
		//
		//if (id == 4931) std::cout << id << ": view: " << pose.view << std::endl;

		//pose.direction = glm::mat3(world_to_cam0) * vec3(0, 0, -1);
		//pose.direction = world_to_cam0[2];
		///*auto tmp = pose.direction[1];
		//pose.direction[1] = -pose.direction[2];
		//pose.direction[2] = -tmp;*/
		//if (id == 4931) std::cout << id << ": dir: " << glm::normalize(pose.direction) << std::endl;
		//if (count < 5) std::cout << "pos: " << pose.position << std::endl;

		
		/*cam0_to_world[0] = -cam0_to_world[0];
		pose.view = glm::inverse(cam0_to_world);// glm::transpose(cam0_to_world);
		glm::vec4 col0 = cam0_to_world[0];
		cam0_to_world[0] = cam0_to_world[1];
		cam0_to_world[1] = col0;*/

		//glm::vec4 pos4 = cam0_to_world * glm::vec4(0, 0, 0, 1);
		//pose.position = glm::vec3(pos4.x,pos4.y,pos4.z) / pos4.w;
		//glm::vec3 dir = glm::mat3(cam0_to_world) * glm::vec3(0, 0, 1);
		//if (id == 4931) std::cout << id << ": original parsed dir: " << glm::normalize(dir) << std::endl;
		////pose.direction = dir;
		//if (id == 4931) std::cout << id << ": cam to world n: " << cam0_to_world << std::endl << " -> invert " << std::endl;
		//pose.orientation = glm::quat_cast(cam0_to_world);
		//
		//glm::mat4 mat_v = (glm::mat4_cast(pose.orientation));
		//if (id == 4931) std::cout << id << ": cam to world n: " << mat_v << std::endl << " -> invert " << std::endl;
		////glm::mat4 mat_v = v;
		//glm::vec3 parsed_dir_from_quat = glm::normalize(glm::vec3(mat_v[2]));

		//if (id == 4931) std::cout << id << ": quat_dir: " << glm::normalize(parsed_dir_from_quat) << std::endl;

		//dir = glm::normalize(cam0_to_world[2]);
		//glm::vec3 up = -glm::normalize(cam0_to_world[0]);
		//pose.view = glm::lookAt(pose.position, pose.position + dir, up);

		result.emplace_back(std::pair<int, View::Pose>(id, pose));
		count++;
	}
	return result;
}

//NavVis Orientations are for OCam images that need to be rotated 90 degree to get a sensble view for a perspective camera.
inline glm::quat NavVisOCamOrientation_to_OpenGL(glm::quat q) {
	
	

	glm::mat4 mat_OCam = glm::mat4(q);
	//printMat("Transform OCam Orientation", mat_OCam); 
	glm::mat4 OCam_to_perspective = glm::mat4(1);
	OCam_to_perspective[0] = glm::vec4(0, 1, 0, 0);
	OCam_to_perspective[1] = glm::vec4(-1, 0, 0, 0);
	//printMat("Transform_Matrix", OCam_to_perspective);
	glm::mat4 mat_perspective = mat_OCam * OCam_to_perspective;
	//printMat("To Perspective Orientation", mat_perspective);
	return glm::quat(mat_perspective);
}

void Dataset::load_NavVis_dataset(const std::string& path) {
	// set camera
	camera_near = 0.1f;
	camera_far = 35.f;
	camera_fov_degree = 90.f;
	camera_aspect_ratio = 960.f / 544.f;
	gt_proj = glm::perspective(90.f * float(M_PI / 180), 1.764711f, 0.1f, 35.f);
	gt_proj_cropped = glm::perspective(90.f * float(M_PI / 180), 1.764711f, 0.1f, 35.f);
	 
	//contains the camera positions and orientations (and groundtruth textures if enabled) for all 4 camera nums for each camera id
	std::vector<RGBCameras> rgbCams;

	//contains the camera calibration parameters for all 4 camera nums
	ReferenceFrameRGBCamera refCam;

	// Parse OCAM Model
	std::cerr << "[Dataset::load_NavVis_dataset] Start Parsing " << std::endl;

	if (camCount % 4 != 0) {
		std::cout << "[Dataset::load_NavVis_dataset] FATAL ERROR: set size not divisibly by 4 for NavVis dataset: " << camCount << " % 4 = " << camCount % 4 << std::endl;
	}

	// parse the 4 ocam models as reference
	std::filesystem::path path_sensor_frame(path + "sensor_frame.xml");
	refCam = Parser::parseReferenceFile(path_sensor_frame);
	// parse the cam positions
	RGBCameras::clear();

	// parse all cams at once into rgbCams vector
	std::cerr << "[Dataset::load_NavVis_dataset] Cam Positions " << std::endl;
	
	int camCountParsed = 0;
	std::filesystem::path img_path = path + "/cam";
	for (auto& p : Helper::get_directory_entries_sorted(img_path)) ++camCountParsed;
	std::cout << "found " << camCountParsed << " images" << std::endl;
	for (int i = 0; i < std::min(camCount , camCountParsed ) / 4; ++i) {
		// create 4 Views for this camera pposition
		int cam_view_id_base = cam_views.size();
		cam_views.emplace_back();
		cam_views.emplace_back();
		cam_views.emplace_back();
		cam_views.emplace_back();

		std::string id = std::to_string(i);
		while (id.size() < 5) id = "0" + id;
		for (int num = 0; num < 4; ++num)
			cam_names.push_back(id + "cam" + std::to_string(num));
		std::cout << "Parse cam " << id << "\r";


		// parse 4 rgb cameras

		for (int num = 0; num < 4; ++num) {
			std::filesystem::path jpg_path = path;
			jpg_path /= "cam";
			jpg_path /= id + "-cam" + std::to_string(num) + ".jpg";
			std::filesystem::path jpg_depth_path = path;
			jpg_depth_path /= "cam_depth";
			jpg_depth_path /= id + "-cam" + std::to_string(num) + ".jpg";
			//if (!noTex) {
				// load images here
			cam_views[cam_view_id_base + num].loadFromFile_RGB_D(jpg_path, jpg_depth_path);

			//}
		}

		//Darius Camera Optimizations
		auto opti_path = std::filesystem::path(path) / "darius_optimization" / "poses_quat_wxyz_pos_xyz.txt";

		std::ifstream opti_file(opti_path);
		bool optimization_exists = opti_file.is_open();
		int cap_pos_num = stoi(id);

		std::filesystem::path json_path = path;
		json_path /= "info";
		json_path /= id + "-info.json";

		auto parsed_poses = parseJsonFile(json_path, true);
		std::vector<View::Pose> optimized_pose;
		if (optimization_exists) optimized_pose = getOptimizedPose(opti_file, cap_pos_num, 4, true);
		for (int num = 0; num < 4; ++num) {
			// Navvis Orientatioons are for Ocam images that are rotated. To crop a perspective image, the orientations are rotated by 90 degree
			//parsed_poses[num].orientation = NavVisOCamOrientation_to_OpenGL(parsed_poses[num].orientation);
			parsed_poses[num].orientation = NavVisOCamOrientation_to_OpenGL(parsed_poses[num].orientation);

			// load position and rotation from rgbcam
			glm::mat4 mat_v = glm::mat4_cast(parsed_poses[num].orientation);

			glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
			glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

			parsed_poses[num].position = navVis_pos_to_Opengl(parsed_poses[num].position);
			parsed_poses[num].direction = dir; //navVis_pos_to_Opengl(parsed_poses[num].direction);

			
			//glm::vec3 up = glm::normalize(-glm::vec3(mat_v[0]));


			parsed_poses[num].view = (glm::lookAt(parsed_poses[num].position, parsed_poses[num].position + dir, up));
			parsed_poses[num].orientation = glm::quat(glm::transpose(parsed_poses[num].view));
			cam_views[cam_view_id_base + num].pose = parsed_poses[num];
			/*std::cout << "Pose" << std::endl;
			std::cout << "pos " <<  parsed_poses[num].position << std::endl;
			std::cout << "dir " <<  parsed_poses[num].direction << std::endl;
			std::cout << "ori " <<  parsed_poses[num].orientation << std::endl;
			std::cout << "cast original " << glm::mat4_cast(parsed_poses[num].orientation) << std::endl;
			std::cout << "cast inverse  " << glm::inverse(glm::mat4_cast(parsed_poses[num].orientation)) << std::endl;*/
			/*auto tmp = mat_v[0];
			mat_v[0] = mat_v[1];
			mat_v[1] = tmp;
			std::cout << "cast swap col " << mat_v << std::endl;
			glm::quat q_swapped = glm::quat(parsed_poses[num].orientation.w, parsed_poses[num].orientation.y, parsed_poses[num].orientation.x, parsed_poses[num].orientation.z);
			std::cout << "q swap entry  " << glm::mat4_cast(q_swapped) << std::endl;
			std::cout << "q sqaw invers " << glm::inverse(glm::mat4_cast(q_swapped)) << std::endl;
			std::cout << "target " <<  parsed_poses[num].view << std::endl;*/
			

			if (optimization_exists) {
				cam_views[cam_view_id_base + num].darius_optimized_pose_present = true;
				// Navvis Orientatioons are for Ocam images that are rotated. To crop a perspective image, the orientations are rotated by 90 degree
				cam_views[cam_view_id_base + num].optimized_pose.orientation = NavVisOCamOrientation_to_OpenGL(optimized_pose[num].orientation);

				// load position and rotation from rgbcam
				glm::mat4 mat_v = glm::mat4_cast(cam_views[cam_view_id_base + num].optimized_pose.orientation);

				glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
				glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

				cam_views[cam_view_id_base + num].optimized_pose.position = navVis_pos_to_Opengl(optimized_pose[num].position);
				cam_views[cam_view_id_base + num].optimized_pose.direction = dir; //navVis_pos_to_Opengl(optimized_pose[num].direction);

				
				//glm::vec3 up = glm::normalize(-glm::vec3(mat_v[0]));
				cam_views[cam_view_id_base + num].optimized_pose.view = (glm::lookAt(optimized_pose[num].position, optimized_pose[num].position + dir, up));
				cam_views[cam_view_id_base + num].optimized_pose.orientation = glm::quat(glm::transpose(cam_views[cam_view_id_base + num].optimized_pose.view));
				//cam_views[cam_view_id_base + num].optimized_pose.view = optimized_pose[num].view;
				//printMat("view", optimized_pose[num].view);
				/*std::cout << "Optimized Pose" << std::endl;
				std::cout << "pos " << optimized_pose[num].position << std::endl;
				std::cout << "dir " << optimized_pose[num].direction << std::endl;
				std::cout << "ori " << optimized_pose[num].orientation << std::endl;
				
				std::cout << "view " << optimized_pose[num].view << std::endl;*/

			}

			

			//cam_views[cam_view_id_base + num].pose.orientation = convertNavvisToLookAt(parsed_poses[num].position, parsed_poses[num].orientation);
			//cam_views[cam_view_id_base + num].optimized_pose.orientation = convertNavvisToLookAt(cam_views[cam_view_id_base + num].optimized_pose.position, cam_views[cam_view_id_base + num].optimized_pose.orientation);

			//cam_views[cam_view_id_base + num].pose.view = glm::mat4_cast(cam_views[cam_view_id_base + num].pose.orientation);
			//cam_views[cam_view_id_base + num].optimized_pose.view = glm::mat4_cast(cam_views[cam_view_id_base + num].optimized_pose.orientation);



		}
		if (optimization_exists)     opti_file.close();

	}

	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	//for (int i = 0; i < camCount; ++i) {
	//	std::cout << i << ": \npos: " << cam_views[i].pose.position << " quat: " << cam_views[i].pose.orientation << " dir: " << cam_views[i].pose.direction << "\npos: " << cam_views[i].optimized_pose.position << " quat : " << cam_views[i].optimized_pose.orientation << " dir : " << cam_views[i].optimized_pose.direction << std::endl;
	//}




	std::cerr << "[Dataset::load_NavVis_dataset] Finished Parsing" << std::endl;

}

// Code from darglein/saiga
inline glm::mat4 CVCamera2GLProjectionMatrix(float fx, float fy, float cx, float cy, float width, float height, float znear = .01, float zfar = 1000.)
{
	/*glm::mat4 m = glm::mat4();
	m[0][0] = 2.0 * fx / width;
	m[0][1] = 0.0;
	m[0][2] = 1.0 - 2.0 * cx / width;
	m[0][3] = 0.0;

	m[1][0] = 0.0;
	m[1][1] = 2.0 * fy / height;
	m[1][2] = 2.0 * cy / height - 1.0;
	m[1][3] = 0.0;

	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = (zfar + znear) / (znear - zfar);
	m[2][3] = -1.0; 

	m[3][0] = 0.0;
	m[3][1] = 0.0;
	m[3][2] = 2.0 * zfar * znear / (znear - zfar);
	m[3][3] = 0.0;*/
	glm::mat4 m = glm::mat4();
	m[0][0] = 2.0 * fx / width;
	m[0][1] = 0.0;
	m[0][2] = 1.0 - 2.0 * cx / width;
	m[0][3] = 0.0;

	m[1][0] = 0.0;
	m[1][1] = 2.0 * fy / height;
	m[1][2] = 2.0 * cy / height - 1.0;
	m[1][3] = 0.0;

	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = (zfar + znear) / (znear - zfar);
	m[2][3] = 2.0 * zfar * znear / (znear - zfar);

	m[3][0] = 0.0;
	m[3][1] = 0.0;
	m[3][2] = -1.0;
	m[3][3] = 0.0;
	return glm::transpose(m);
}

void Dataset::load_TT_dataset(const std::string& path) {
	std::cerr << "[Dataset::load_TT_dataset] Start parsing Tanks and Temples dataset" << std::endl;
	
	// set camera

	float fx, fy, cx, cy, s;
	int w, h;
	//float k1, k2, k3, k4, k5, k6, p1, p2;
	auto cam_path = std::filesystem::path(path) / "camera0_undistorted.ini";
	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_TT_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;

	std::string dummy;
	cam_file >> dummy; // [SceneCameraParams]
	cam_file >> dummy >> dummy >> w; // w = 1920
	cam_file >> dummy >> dummy >> h; // h = 1080
	cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # fx fy cx cy s
	cam_file >> dummy >> dummy >> fx >> fy >> cx >> cy >> s; // K = ...
	//cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # 8 paramter distortion model. see distortion.h
	//cam_file >> dummy >> dummy >> k1 >> k2 >> k3 >> k4 >> k5 >> k6 >> p1 >> p2; // distortion = ...
	/*w = 2014;
	h = 1093;    
	fx = 2026.7877932808483;
	fy = 2102.0848276912375;
	cx = 1007;
	cy = 546.5;*/
	std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
	std::cout << "w,h " << w << "," << h << std::endl;
	//std::cout << "k1 k2 k3 k4 k5 k6 p1 p2 " << k1 << "," << k2 << "," << k3 << "," << k4 << "," << k5 << ","  << k6 << "," << p1 << "," << p2 << std::endl;

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);
	
	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);
	
	
	
	gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	//std::cout << "target " << gt_proj << std::endl;
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);
	//std::cout << "new    " << gt_proj << std::endl;
	
	// adjust proj for target image to accound for target resolutions that need to be divisible by 32
	if (w != targetResolution.x) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
		int diff = w - targetResolution.x; // difference in resolution
		int adjust_left = diff / 2; //number of pixels to subtract on the left side
		cx -= adjust_left;
		w = targetResolution.x;
	}
	if (h != targetResolution.y) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
		int diff = h - targetResolution.y; // difference in resolution
		int adjust_top = diff / 2; //number of pixels to subtract on the left side
		cy -= adjust_top;
		h = targetResolution.y;
	}
	gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);


	cam_file.close();

	//load camera poses
	auto opti_path = std::filesystem::path(path) / "poses.txt" ;


	std::ifstream opti_file(opti_path);
	if (!opti_file.is_open())
		std::cout << "[Dataset::load_TT_dataset] FATAL ERROR: Poses not found: " << opti_path << "." << std::endl;

	
	std::vector<View::Pose> poses = getPoses(opti_file, camCount, "XYZW",true);
	for (int i = 0; i < camCount; ++i) {
		std::string id = std::to_string(i + 1);
		while (id.size() < 5) id = "0" + id;
		cam_names.push_back(id);
		std::cout << "Parse cam " << id << "\r";
		cam_views.emplace_back();

		glm::mat4 mat_v = glm::mat4_cast(poses[i].orientation);
		//printMat("orientation pre axis flip", mat_v);
		//std::cout << "quat pre axis flip: " << poses[i].orientation << std::endl;


		//mat_v[1] = -mat_v[1];
		//mat_v[2] = -mat_v[2];

		glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
		//glm::vec3 up = glm::normalize(-glm::vec3(mat_v[0]));
		glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));
		//printMat("orientation post axis flip", mat_v);
		poses[i].orientation = glm::normalize(glm::quat(mat_v));
		//std::cout << "quat post axis flip: " << poses[i].orientation << std::endl;
		//printMat("orientation post axis flip, reconverted", glm::mat4(poses[i].orientation));
		poses[i].direction = dir;
		poses[i].view = glm::lookAt(poses[i].position, poses[i].position + dir, up);
		poses[i].orientation = glm::quat(glm::transpose(poses[i].view));
		//pose.orientation = glm::quat(glm::transpose(pose.view));
		cam_views[i].pose = poses[i];


		std::filesystem::path jpg_path = path;
		jpg_path /= "images_undistorted";
		jpg_path /= id + ".jpg";
		std::filesystem::path jpg_depth_path = path;
		jpg_depth_path /= "depth_undistorted";
		jpg_depth_path /= id + ".jpg";
		if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
		}
		else { // depth not present -> load rgb as depth
			std::cout << "[Dataset::load_TT_dataset] WARNING: No depth images found in " << jpg_depth_path << ". Use RGB images as depth. You should render the depths and put them into the dataset folder." << std::endl;
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_path);
		}
		//if (!noTex) {
			// load images here
		

		//}
	}
	opti_file.close();		


	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	//for (int i = 0; i < camCount; ++i) {
	//	std::cout << i << ": \npos: " << cam_views[i].pose.position << " quat: " << cam_views[i].pose.orientation << " dir: " << cam_views[i].pose.direction << "\npos: " << cam_views[i].optimized_pose.position << " quat : " << cam_views[i].optimized_pose.orientation << " dir : " << cam_views[i].optimized_pose.direction << std::endl;
	//}




	std::cerr << "[Dataset::load_TT_dataset] Finished Parsing" << std::endl;
}

void Dataset::load_KITTY_dataset(const std::string& path, std::string info, std::pair<int, int> range, std::pair<int,int> test_start_step) {
	std::string set_name = info + "/"; 

	std::cerr << "[Dataset::load_KITTY_dataset] Start parsing KITTY-360 dataset" << std::endl;

	//load all camera poses of a scene for all ply files
	auto pose_path = std::filesystem::path(path) / "data_poses/" / set_name / "cam0_to_world.txt";

	std::ifstream pose_file(pose_path);
	if (!pose_file.is_open())
		std::cout << "[Dataset::load_KITTY_dataset] FATAL ERROR: Poses not found: " << pose_path << "." << std::endl;


	std::vector<std::pair<int, View::Pose>> kitty_poses = getPosesKITTY(pose_file);

	pose_file.close();

	// check ply files to get the range of image poses to load
	auto ply_path = std::filesystem::path(path) / "data_3d_semantics/" / "train/" / set_name / "static/";

	std::vector<std::pair<int, int>> pointcloud_ranges;
	for (const auto& pc : Helper::get_directory_entries_sorted(ply_path)) {
		std::string pc_name = pc.path().filename().string();
		
		//std::cout << pc_name.substr(0, 10) << " : " << pc_name.substr(11, 21) << std::endl;
		pointcloud_ranges.emplace_back(std::pair<int, int>(std::atoi(pc_name.substr(0, 10).c_str()), std::atoi(pc_name.substr(11, 21).c_str())));
		//std::cout << pointcloud_ranges[pointcloud_ranges.size()-1].first << " : " << pointcloud_ranges[pointcloud_ranges.size() - 1].second << std::endl;
	}
	
	// for the time, only load the images for the first pointcloud
	int start = pointcloud_ranges[range.first].first;
	int end   = pointcloud_ranges[range.second].second;



	// check image filenames to get a list of available ids. For each image, check the ids in the poses vector to fetch the correct one.
	auto img_path   = std::filesystem::path(path) / "data_2d_raw/"  / set_name / "image_00/" / "data_rect/"  ;
	auto depth_path = std::filesystem::path(path) / "data_2d_raw/"  / set_name / "image_00/" / "depth_rect/" ;

	std::vector<std::string> filenames;
	int cur_pose_index = -1;
	int cur_pose_num = -1;
	bool depthnotpresent = false;
	int count = 0;
    int int_id =0;
	for (const auto& img : Helper::get_directory_entries_sorted(img_path)) {
		int id = std::atoi(img.path().filename().string().c_str());
		if (id < start || id > end) { // skip invalid ids (ids out of wanted range)
			continue;
		}

		filenames.emplace_back(img.path().filename().string());
		
		//std::cout << filenames[filenames.size() - 1] << std::endl;
		/*cur_pose_index++;
		if (cur_pose_index >= kitty_poses.size()) break;
		cur_pose_num = kitty_poses[cur_pose_index].first;*/
		while (cur_pose_num < id) {
			cur_pose_index++;
			if (cur_pose_index >= kitty_poses.size()) break;
			cur_pose_num = kitty_poses[cur_pose_index].first;
		}
		if (cur_pose_num != id) {
			//std::cout << "[Dataset::load_KITTY_dataset] FATAL ERROR: Poses of image " << id << " not found." << std::endl;
			continue;
		}
		//std::cout << "img id: " << id << "; pose id: " << kitty_poses[cur_pose_index].first << std::endl;

        if (test_start_step.second > 0 && (int_id-test_start_step.first) % test_start_step.second == 0) { //create test set id step is greater 0
            cam_names_test.push_back(img.path().filename().string().substr(0, 10));

            cam_views_test.emplace_back();
            cam_views_test[cam_views_test.size()-1].pose = kitty_poses[cur_pose_index].second;
        } else {
            cam_names.push_back(img.path().filename().string().substr(0, 10));
            cam_views.emplace_back();
            cam_views[cam_views.size() - 1].pose = kitty_poses[cur_pose_index].second;


            std::filesystem::path jpg_path = img_path;
            jpg_path /= filenames[filenames.size() - 1];
            std::filesystem::path jpg_depth_path = depth_path;
            jpg_depth_path /=
                    filenames[filenames.size() - 1].substr(0, filenames[filenames.size() - 1].size() - 4) + ".jpg";
            if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
                cam_views[cam_views.size() - 1].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
            } else { // depth not present -> load rgb as depth
                std::cout << "[Dataset::load_KITTY_dataset] WARNING: Depth Image not found " << jpg_depth_path
                          << std::endl;
                depthnotpresent = true;
                cam_views[cam_views.size() - 1].loadFromFile_RGB_D(jpg_path, jpg_path);
            }
        }
        ++int_id;
		++count;
		if (camCount <= cam_views.size()) break;
		std::cout << "[Dataset::load_KITTY_dataset] Parse Image " << id << "\r";
	}
	if (depthnotpresent) {
		std::cout << "[Dataset::load_KITTY_dataset] WARNING: Not all depth images found in " << depth_path << ". Use RGB images as depth. You should render the depths and put them into the dataset folder." << std::endl;
	}
	camCount = cam_views.size();


	// parse projection matrix
	auto cam_path = std::filesystem::path(path) / "calibration/" / "perspective.txt";

	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_KITTY_dataset] FATAL ERROR: Poses not found: " << cam_path << "." << std::endl;

	std::string line;
	for (int i = 0; i < 9; ++i) {
		std::getline(cam_file, line);
	}

	std::string dummy;
	float fx, fy, cx, cy, s;
	int w = 1408, h = 376;

	cam_file >> dummy >> fx >> dummy >> cx >> dummy >> dummy >> fy >> cy;
	
	cam_file.close();

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);



	//gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	//gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h);
	//std::cout << "target " << gt_proj << std::endl;
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);

	// adjust proj for target image to accound for target resolutions that need to be divisible by 32
	if (w != targetResolution.x) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
		int diff = w - targetResolution.x; // difference in resolution
		int adjust_left = diff / 2; //number of pixels to subtract on the left side
		cx -= adjust_left;
		w = targetResolution.x;
	}
	if (h != targetResolution.y) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
		int diff = h - targetResolution.y; // difference in resolution
		int adjust_top = diff / 2; //number of pixels to subtract on the left side
		cy -= adjust_top;
		h = targetResolution.y;
	}
	gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);

	//std::cout << "new    " << gt_proj << std::endl;

	/*std::cout << "gt_proj: " << gt_proj << std::endl;
	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree =  2.0 * std::atan(1.0 / gt_proj[1][1]) * 180.0 / M_PI;
	camera_aspect_ratio = gt_proj[1][1] / gt_proj[0][0];*/
	//camera_aspect_ratio = 50.f;

	/*std::string tt_path = "../../../datasets/tt_m60/";
	
	// set camera

	float fx, fy, cx, cy, s;
	int w, h;
	//float k1, k2, k3, k4, k5, k6, p1, p2;
	auto cam_path = std::filesystem::path(tt_path) / "camera0_undistorted.ini";
	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_KITTY_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;

	std::string dummy;
	cam_file >> dummy; // [SceneCameraParams]
	cam_file >> dummy >> dummy >> w; // w = 1920
	cam_file >> dummy >> dummy >> h; // h = 1080
	cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # fx fy cx cy s
	cam_file >> dummy >> dummy >> fx >> fy >> cx >> cy >> s; // K = ...
	//cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # 8 paramter distortion model. see distortion.h
	//cam_file >> dummy >> dummy >> k1 >> k2 >> k3 >> k4 >> k5 >> k6 >> p1 >> p2; // distortion = ...
	
	std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
	std::cout << "w,h " << w << "," << h << std::endl;
	//std::cout << "k1 k2 k3 k4 k5 k6 p1 p2 " << k1 << "," << k2 << "," << k3 << "," << k4 << "," << k5 << ","  << k6 << "," << p1 << "," << p2 << std::endl;

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);



	gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	std::cout << "target " << gt_proj << std::endl;
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);
	std::cout << "new    " << gt_proj << std::endl;


	cam_file.close();*/

	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	//for (int i = 0; i < camCount; ++i) {
	//	std::cout << i << ": \npos: " << cam_views[i].pose.position << " quat: " << cam_views[i].pose.orientation << " dir: " << cam_views[i].pose.direction << "\npos: " << cam_views[i].optimized_pose.position << " quat : " << cam_views[i].optimized_pose.orientation << " dir : " << cam_views[i].optimized_pose.direction << std::endl;
	//}
	
	std::cerr << "[Dataset::load_KITTY_dataset] Finished Parsing KITTY-360 Set" << std::endl;
}

void Dataset::load_L_dataset(const std::string& path) {
	std::cerr << "[Dataset::load_L_dataset] Start parsing Tanks and Temples dataset" << std::endl;

	// set camera

	float fx, fy, cx, cy, s;
	int w, h;
	//float k1, k2, k3, k4, k5, k6, p1, p2;
	auto cam_path = std::filesystem::path(path) / "camera0.ini";
	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_TT_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;

	std::string dummy;
	cam_file >> dummy; // [SceneCameraParams]
	cam_file >> dummy >> dummy >> w; // w = 1920
	cam_file >> dummy >> dummy >> h; // h = 1080
	cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # fx fy cx cy s
	cam_file >> dummy >> dummy >> fx >> fy >> cx >> cy >> s; // K = ...
	//cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # 8 paramter distortion model. see distortion.h
	//cam_file >> dummy >> dummy >> k1 >> k2 >> k3 >> k4 >> k5 >> k6 >> p1 >> p2; // distortion = ...
	/*w = 2014;
	h = 1093;
	fx = 2026.7877932808483;
	fy = 2102.0848276912375;
	cx = 1007;
	cy = 546.5;*/
	std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
	std::cout << "w,h " << w << "," << h << std::endl;
	//std::cout << "k1 k2 k3 k4 k5 k6 p1 p2 " << k1 << "," << k2 << "," << k3 << "," << k4 << "," << k5 << ","  << k6 << "," << p1 << "," << p2 << std::endl;

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);



	gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	//std::cout << "target " << gt_proj << std::endl;
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);
	//std::cout << "new    " << gt_proj << std::endl;
	// adjust proj for target image to accound for target resolutions that need to be divisible by 32
	if (w != targetResolution.x) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
		int diff = w - targetResolution.x; // difference in resolution
		int adjust_left = diff / 2; //number of pixels to subtract on the left side
		cx -= adjust_left;
		w = targetResolution.x;
	}
	if (h != targetResolution.y) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
		int diff = h - targetResolution.y; // difference in resolution
		int adjust_top = diff / 2; //number of pixels to subtract on the left side
		cy -= adjust_top;
		h = targetResolution.y;
	}
	gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);


	cam_file.close();

	//load camera poses
	auto opti_path = std::filesystem::path(path) / "poses.txt";


	std::ifstream opti_file(opti_path);
	if (!opti_file.is_open())
		std::cout << "[Dataset::load_TT_dataset] FATAL ERROR: Poses not found: " << opti_path << "." << std::endl;


	std::filesystem::path img_path = path;
	img_path /= "images_scaled";
	std::vector<std::string> img_names;
	for (const auto& entry : Helper::get_directory_entries_sorted(img_path) ) {
		//std::cout << entry.path() << std::endl;
		img_names.emplace_back(entry.path().filename().string());
	}

	std::vector<View::Pose> poses = getPoses(opti_file, camCount, "XYZW", true);
	for (int i = 0; i < camCount; ++i) {
		std::string id = std::to_string(i + 1);
		while (id.size() < 5) id = "0" + id;
		cam_names.push_back(id);
		std::cout << "Parse cam " << id << "\r";
		cam_views.emplace_back();

		glm::mat4 mat_v = glm::mat4_cast(poses[i].orientation);
		//printMat("orientation pre axis flip", mat_v);
		//std::cout << "quat pre axis flip: " << poses[i].orientation << std::endl;


		//mat_v[1] = -mat_v[1];
		//mat_v[2] = -mat_v[2];

		glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
		//glm::vec3 up = glm::normalize(-glm::vec3(mat_v[0]));
		glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));
		//printMat("orientation post axis flip", mat_v);
		poses[i].orientation = glm::normalize(glm::quat(mat_v));
		//std::cout << "quat post axis flip: " << poses[i].orientation << std::endl;
		//printMat("orientation post axis flip, reconverted", glm::mat4(poses[i].orientation));
		poses[i].direction = dir;
		poses[i].view = glm::lookAt(poses[i].position, poses[i].position + dir, up);
		poses[i].orientation = glm::quat(glm::transpose(poses[i].view));
		//pose.orientation = glm::quat(glm::transpose(pose.view));
		cam_views[i].pose = poses[i];


		std::filesystem::path jpg_path = path;
		jpg_path /= "images_scaled";
		jpg_path /= img_names[i] ;
		std::filesystem::path jpg_depth_path = path;
		jpg_depth_path /= "images_scaled_depth";
		jpg_depth_path /= img_names[i];
		if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
		}
		else { // depth not present -> load rgb as depth
			std::cout << "[Dataset::load_TT_dataset] WARNING: No depth images found in " << jpg_depth_path << ". Use RGB images as depth. You should render the depths and put them into the dataset folder." << std::endl;
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_path);
		}
		//if (!noTex) {
			// load images here


		//}
	}
	opti_file.close();


	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	//for (int i = 0; i < camCount; ++i) {
	//	std::cout << i << ": \npos: " << cam_views[i].pose.position << " quat: " << cam_views[i].pose.orientation << " dir: " << cam_views[i].pose.direction << "\npos: " << cam_views[i].optimized_pose.position << " quat : " << cam_views[i].optimized_pose.orientation << " dir : " << cam_views[i].optimized_pose.direction << std::endl;
	//}


	camCount = cam_views.size();

	std::cerr << "[Dataset::load_TT_dataset] Finished Parsing" << std::endl;
}

void Dataset::load_Redwood_dataset(const std::string& path) {
	std::cerr << "[Dataset::load_Redwood_dataset] Start parsing ScanNet dataset" << std::endl;

	// set camera
	float fx, fy, cx, cy, s;
	int w, h;
	auto cam_path = std::filesystem::path(path) / "adop/camera0.ini";
	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;

	std::string dummy;
	cam_file >> dummy; // [SceneCameraParams]
	cam_file >> dummy >> dummy >> w; // w = 1920
	cam_file >> dummy >> dummy >> h; // h = 1080
	cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # fx fy cx cy s
	cam_file >> dummy >> dummy >> fx >> fy >> cx >> cy >> s; // K = ...

	std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
	std::cout << "w,h " << w << "," << h << std::endl;

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);

	//gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);
	// adjust proj for target image to accound for target resolutions that need to be divisible by 32
	if (w != targetResolution.x) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
		int diff = w - targetResolution.x; // difference in resolution
		int adjust_left = diff / 2; //number of pixels to subtract on the left side
		cx -= adjust_left;
		w = targetResolution.x;
	}
	if (h != targetResolution.y) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
		int diff = h - targetResolution.y; // difference in resolution
		int adjust_top = diff / 2; //number of pixels to subtract on the left side
		cy -= adjust_top;
		h = targetResolution.y;
	}
	gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);

	cam_file.close();


	// load image filenames from images.txt
	std::vector<std::string> image_filenames;
	auto img_path = std::filesystem::path(path) / "adop/images.txt";
	std::ifstream img_file(img_path);
	if (!img_file.is_open())
		std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Image List not found: " << img_path << "." << std::endl;
	std::string line;
	while (img_file >> line) {
		image_filenames.push_back(line.substr(0,line.size()));
		//std::cout << image_filenames[image_filenames.size() - 1] << std::endl;
	}
	img_file.close();

	// load image filenames from train_xx.txt
	auto train_path = std::filesystem::path(path) / "adop/train_keyframed.txt";// train_mod20.txt";//train_keyframed.txt";
	std::ifstream train_file(train_path);
	if(!train_file.is_open())
		std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Train List not found: " << train_path << "." << std::endl;
	int current_train = 0;
	std::vector<int> train_indices;
	while (train_file >> current_train) {
		train_indices.push_back(current_train);
		//std::cout << train_indices[train_indices.size() - 1] << std::endl;
	}

	//load camera poses
	auto opti_path = std::filesystem::path(path) / "adop/poses.txt";
	std::ifstream opti_file(opti_path);
	if (!opti_file.is_open())
		std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Poses not found: " << opti_path << "." << std::endl;
	std::vector<View::Pose> poses = getPoses(opti_file, image_filenames.size(), "XYZW", true);

	// accept pose for every index in train.txt referencing which image to load from images.txt
	for (int i = 0; i < std::min(train_indices.size(),size_t(camCount)); ++i) {
		std::cout << i<<": train_index: " << train_indices[i] << "; image_filename: " << image_filenames[train_indices[i]] << "\r";

		cam_names.push_back(image_filenames[train_indices[i]]);
		cam_views.emplace_back();

		glm::mat4 mat_v = glm::mat4_cast(poses[train_indices[i]].orientation);

		glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
		glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

		poses[train_indices[i]].orientation = glm::normalize(glm::quat(mat_v));
		poses[train_indices[i]].direction = dir;
		poses[train_indices[i]].view = glm::lookAt(poses[train_indices[i]].position, poses[train_indices[i]].position + dir, up);
		poses[train_indices[i]].orientation = glm::quat(glm::transpose(poses[train_indices[i]].view));
		cam_views[i].pose = poses[train_indices[i]];


		std::filesystem::path jpg_path = path;
		jpg_path /= "ass_color";
		jpg_path /= image_filenames[train_indices[i]];
		std::filesystem::path jpg_depth_path = path;
		jpg_depth_path /= "ass_depth_pr";
		jpg_depth_path /= image_filenames[train_indices[i]];


		if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
		}
		else { // depth not present -> load rgb as depth
			std::cout << "[Dataset::load_Redwood_dataset] WARNING: No depth images found in " << jpg_depth_path << ". Use RGB images as depth. You should render the depths and put them into the dataset folder." << std::endl;
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_path);
		}
	}
	
	// load test poses if test.txt is present
	// load image filenames from test.txt
	auto test_path = std::filesystem::path(path) / "adop/test.txt";
	if (std::filesystem::exists(test_path)) {
		std::cout << "[Dataset::load_Redwood_dataset] Test List found. Load Test Images." << std::endl;
		std::ifstream test_file(test_path);
		if (!test_file.is_open())
			std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Test List not found: " << test_path << "." << std::endl;
		int current_test = 0;
		std::vector<int> test_indices;
		while (test_file >> current_test) {
			test_indices.push_back(current_test);
		}

		// accept pose for every index in test.txt referencing which image to load from images.txt
		for (int i = 0; i < std::min(test_indices.size(), size_t(camCount)); ++i) {
			std::cout << i << ": test_index: " << test_indices[i] << "; image_filename: " << image_filenames[test_indices[i]] << "\r";

			cam_names_test.push_back(image_filenames[test_indices[i]]);
			cam_views_test.emplace_back();

			glm::mat4 mat_v = glm::mat4_cast(poses[test_indices[i]].orientation);

			glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
			glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

			poses[test_indices[i]].orientation = glm::normalize(glm::quat(mat_v));
			poses[test_indices[i]].direction = dir;
			poses[test_indices[i]].view = glm::lookAt(poses[test_indices[i]].position, poses[test_indices[i]].position + dir, up);
			poses[test_indices[i]].orientation = glm::quat(glm::transpose(poses[test_indices[i]].view));
			cam_views_test[i].pose = poses[test_indices[i]];
		}
		test_file.close();
	}
	else {
		std::cout << "[Dataset::load_Redwood_dataset] NOTE: No Test Image Indices found: " << test_path << "." << std::endl;
	}
	opti_file.close();


	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	camCount = cam_views.size();

	std::cerr << "[Dataset::load_Redwood_dataset] Finished Parsing" << std::endl;
}


void Dataset::load_ScanNet_dataset(const std::string& path) {
	std::cerr << "[Dataset::load_ScanNet_dataset] Start parsing ScanNet dataset" << std::endl;

	// set camera
	float fx, fy, cx, cy, s;
	int w, h;
	auto cam_path = std::filesystem::path(path) / "adop/camera0.ini";
	std::ifstream cam_file(cam_path);
	if (!cam_file.is_open())
		std::cout << "[Dataset::load_ScanNet_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;

	std::string dummy;
	cam_file >> dummy; // [SceneCameraParams]
	cam_file >> dummy >> dummy >> w; // w = 1920
	cam_file >> dummy >> dummy >> h; // h = 1080
	cam_file >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy; // # fx fy cx cy s
	cam_file >> dummy >> dummy >> fx >> fy >> cx >> cy >> s; // K = ...

	std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
	std::cout << "w,h " << w << "," << h << std::endl;

	float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
	float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

	camera_near = 0.1f;
	camera_far = 1000.f;
	camera_fov_degree = fov_y;//31.6;// fov_y;
	camera_aspect_ratio = float(w) / float(h);

	//gt_proj = glm::perspective(camera_fov_degree * float(M_PI / 180), camera_aspect_ratio, camera_near, camera_far);
	gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);

	// adjust proj for target image to accound for target resolutions that need to be divisible by 32
	if (w != targetResolution.x) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
		int diff = w - targetResolution.x; // difference in resolution
		int adjust_left = diff / 2; //number of pixels to subtract on the left side
		cx -= adjust_left;
		w = targetResolution.x;
	}
	if (h != targetResolution.y) {
		std::cout << "[Dataset::load_ScanNet_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
		int diff = h - targetResolution.y; // difference in resolution
		int adjust_top = diff / 2; //number of pixels to subtract on the left side
		cy -= adjust_top;
		h = targetResolution.y;
	}
	gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);


	cam_file.close();


	// load image filenames from images.txt
	std::vector<std::string> image_filenames;
	auto img_path = std::filesystem::path(path) / "adop/images.txt";
	std::ifstream img_file(img_path);
	if (!img_file.is_open())
		std::cout << "[Dataset::load_ScanNet_dataset] FATAL ERROR: Image List not found: " << img_path << "." << std::endl;
	std::string line;
	while (img_file >> line) {
		image_filenames.push_back(line.substr(0, line.size()));
		//std::cout << image_filenames[image_filenames.size() - 1] << std::endl;
	}
	img_file.close();

	// load image filenames from train_xx.txt
	auto train_path = std::filesystem::path(path) / "adop/train_keyframed.txt";// train_mod20.txt";//train_keyframed.txt";
	std::ifstream train_file(train_path);
	if (!train_file.is_open())
		std::cout << "[Dataset::load_ScanNet_dataset] FATAL ERROR: Train List not found: " << train_path << "." << std::endl;
	int current_train = 0;
	std::vector<int> train_indices;
	while (train_file >> current_train) {
		train_indices.push_back(current_train);
		//std::cout << train_indices[train_indices.size() - 1] << std::endl;
	}

	//load camera poses
	auto opti_path = std::filesystem::path(path) / "adop/poses.txt";
	std::ifstream opti_file(opti_path);
	if (!opti_file.is_open())
		std::cout << "[Dataset::load_ScanNet_dataset] FATAL ERROR: Poses not found: " << opti_path << "." << std::endl;
	std::vector<View::Pose> poses = getPoses(opti_file, image_filenames.size(), "XYZW", true);

	// accept pose for every index in train.txt referencing which image to load from images.txt
	for (int i = 0; i < std::min(train_indices.size(), size_t(camCount)); ++i) {
		std::cout << i << ": train_index: " << train_indices[i] << "; image_filename: " << image_filenames[train_indices[i]] << "\r";

		cam_names.push_back(image_filenames[train_indices[i]]);
		cam_views.emplace_back();

		glm::mat4 mat_v = glm::mat4_cast(poses[train_indices[i]].orientation);

		glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
		glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

		poses[train_indices[i]].orientation = glm::normalize(glm::quat(mat_v));
		poses[train_indices[i]].direction = dir;
		poses[train_indices[i]].view = glm::lookAt(poses[train_indices[i]].position, poses[train_indices[i]].position + dir, up);
		poses[train_indices[i]].orientation = glm::quat(glm::transpose(poses[train_indices[i]].view));
		cam_views[i].pose = poses[train_indices[i]];


		std::filesystem::path jpg_path = path;
		jpg_path /= "color";
		jpg_path /= image_filenames[train_indices[i]];
		std::filesystem::path jpg_depth_path = path;
		jpg_depth_path /= "depth_pr";
		jpg_depth_path /= image_filenames[train_indices[i]];


		if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
		}
		else { // depth not present -> load rgb as depth
			std::cout << "[Dataset::load_ScanNet_dataset] WARNING: No depth images found in " << jpg_depth_path << ". Use RGB images as depth. You should render the depths and put them into the dataset folder." << std::endl;
			cam_views[i].loadFromFile_RGB_D(jpg_path, jpg_path);
		}
	}

	// load test poses if test.txt is present
	// load image filenames from test.txt
	auto test_path = std::filesystem::path(path) / "adop/test.txt";
	if (std::filesystem::exists(test_path)) {
		std::cout << "[Dataset::load_Redwood_dataset] Test List found. Load Test Images." << std::endl;
		std::ifstream test_file(test_path);
		if (!test_file.is_open())
			std::cout << "[Dataset::load_Redwood_dataset] FATAL ERROR: Test List not found: " << test_path << "." << std::endl;
		int current_test = 0;
		std::vector<int> test_indices;
		while (test_file >> current_test) {
			test_indices.push_back(current_test);
		}

		// accept pose for every index in test.txt referencing which image to load from images.txt
		for (int i = 0; i < std::min(test_indices.size(), size_t(camCount)); ++i) {
			std::cout << i << ": test_index: " << test_indices[i] << "; image_filename: " << image_filenames[test_indices[i]] << "\r";

			cam_names_test.push_back(image_filenames[test_indices[i]]);
			cam_views_test.emplace_back();

			glm::mat4 mat_v = glm::mat4_cast(poses[test_indices[i]].orientation);

			glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
			glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

			poses[test_indices[i]].orientation = glm::normalize(glm::quat(mat_v));
			poses[test_indices[i]].direction = dir;
			poses[test_indices[i]].view = glm::lookAt(poses[test_indices[i]].position, poses[test_indices[i]].position + dir, up);
			poses[test_indices[i]].orientation = glm::quat(glm::transpose(poses[test_indices[i]].view));
			cam_views_test[i].pose = poses[test_indices[i]];
		}
		test_file.close();
	}
	else {
		std::cout << "[Dataset::load_Redwood_dataset] NOTE: No Test Image Indices found: " << test_path << "." << std::endl;
	}

	opti_file.close();


	std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
	std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

	camCount = cam_views.size();

	std::cerr << "[Dataset::load_ScanNet_dataset] Finished Parsing" << std::endl;
}

void Dataset::load_generic_dataset(const std::string& path,std::pair<int,int> range) {
    std::cerr << "[Dataset::load_generic_dataset] Start parsing generic dataset" << std::endl;

    // set camera

    float fx, fy, cx, cy, s;
    int w, h;
    //float k1, k2, k3, k4, k5, k6, p1, p2;
    auto res_path = std::filesystem::path(path) / "intrinsic/resolution.txt";
    std::ifstream res_file(res_path);
    if (!res_file.is_open())
        std::cout << "[Dataset::load_generic_dataset] FATAL ERROR: Res file not found: " << res_path << "." << std::endl;
    std::string dummy;
    res_file >> w >> h;
    auto cam_path = std::filesystem::path(path) / "intrinsic/intrinsic_color.txt";
    std::ifstream cam_file(cam_path);
    if (!cam_file.is_open())
        std::cout << "[Dataset::load_generic_dataset] FATAL ERROR: Cam file not found: " << cam_path << "." << std::endl;


    cam_file >> fx >> dummy >> cx >> dummy; // fx 0 cx 0
    cam_file >> dummy >> fy >> cy >> dummy ; // 0 fy cy 0

    std::cout << "fx fy cx cy s " << fx << "," << fy << "," << cx << "," << cy << "," << s << std::endl;
    std::cout << "w,h " << w << "," << h << std::endl;
    //std::cout << "k1 k2 k3 k4 k5 k6 p1 p2 " << k1 << "," << k2 << "," << k3 << "," << k4 << "," << k5 << ","  << k6 << "," << p1 << "," << p2 << std::endl;

    float fov_y = 2.f * std::atan2(h, 2.f * fy) * float(180.f / M_PI);
    float fov_x = 2.f * std::atan2(w, 2.f * fx) * float(180.f / M_PI);

    camera_near = 0.1f;
    camera_far = 1000.f;
    camera_fov_degree = fov_y;//31.6;// fov_y;
    camera_aspect_ratio = float(w) / float(h);

    gt_proj = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);

    // adjust proj for target image to accound for target resolutions that need to be divisible by 32
    if (w != targetResolution.x) {
        std::cout << "[Dataset::load_generic_dataset] Target and GT Width does not match -> create projection matrix for cropped image." << std::endl;
        int diff = w - targetResolution.x; // difference in resolution
        int adjust_left = diff / 2; //number of pixels to subtract on the left side
        cx -= adjust_left;
        w = targetResolution.x;
    }
    if (h != targetResolution.y) {
        std::cout << "[Dataset::load_generic_dataset] Target and GT Height does not match -> create projection matrix for cropped image." << std::endl;
        int diff = h - targetResolution.y; // difference in resolution
        int adjust_top = diff / 2; //number of pixels to subtract on the left side
        cy -= adjust_top;
        h = targetResolution.y;
    }
    gt_proj_cropped = CVCamera2GLProjectionMatrix(fx, fy, cx, cy, w, h, camera_near, camera_far);


    cam_file.close();


    // load associations file
    auto ass_path = std::filesystem::path(path) / "associations.txt";
    std::ifstream ass_file(ass_path);
    std::vector<std::string> associations;
    if (!ass_file.is_open())
        std::cout << "[Dataset::load_generic_dataset] FATAL ERROR: Associations file not found: " << res_path << "." << std::endl;
    std::string line;
    while (ass_file >> dummy >> line) {
        std::cout << "original line " << line << std::endl;
        double timestamp = std::stod(line);
        std::cout << "as float " << timestamp << std::endl;
        std::stringstream stream;
        stream.precision(5);
        stream << std::fixed;
        stream << timestamp;
        associations.push_back(stream.str());
        std::cout << "after round " << associations[associations.size()-1] << std::endl;
    }
    ass_file.close();
    for (int i=0; i< associations.size(); ++i){
        std::cout << i << " " << associations[i] << std::endl;
    }
    // load image filenames from train_xx.txt
    auto train_path = std::filesystem::path(path) / "KeyframeTrajectory.txt";
    std::ifstream train_file(train_path);
    if (!train_file.is_open())
        std::cout << "[Dataset::load_generic_dataset] FATAL ERROR: Train List not found: " << train_path << "." << std::endl;
    std::vector<View::Pose> poses;
    std::vector<std::string> used_ids;
    // format is "id p_x p_y p_z q_x q_y q_z q_w"
    float q_w, q_x, q_y, q_z, p_x, p_y, p_z;
    std::string cur_id;
    int int_id =0;
    while (train_file >> cur_id >> p_x >> p_y >> p_z >> q_x >> q_y >> q_z >> q_w ) {
        //remove newline
        std::getline(train_file, line);
        View::Pose pose;
        pose.position = glm::vec3(p_x, p_y, p_z);
        pose.orientation = ColMap_to_OpenGL(glm::quat(q_w, q_x, q_y, q_z));

        pose.direction = glm::vec3(glm::mat4_cast(pose.orientation)[2]);
        //printMat("final quat: ", glm::mat4(pose_opti.orientation));
        poses.emplace_back(pose);
        used_ids.emplace_back(cur_id);

        glm::mat4 mat_v = glm::mat4_cast(pose.orientation);

        glm::vec3 dir = -glm::normalize(glm::vec3(mat_v[2]));
        glm::vec3 up = glm::normalize(glm::vec3(mat_v[1]));

        //pose.orientation = glm::normalize(glm::quat(mat_v));
        pose.direction = dir;
        pose.view = glm::lookAt(pose.position, pose.position + dir, up);
        pose.orientation = glm::quat(glm::transpose(pose.view));

        //std::cout << cur_id << std::endl;
        // get id from string id/timestamp
        double timestamp = std::stod(cur_id);
        std::stringstream stream;
        stream.precision(5);
        stream << std::fixed;
        stream << timestamp;
        cur_id = stream.str();
        std::vector<std::string>::iterator itr = std::find(associations.begin(), associations.end(), cur_id);
        int num_id = std::distance(associations.begin(), itr);
        //std::cout << num_id << std::endl;
        if (num_id >= associations.size()) {
            std::cout << "did not find " << cur_id << std::endl;
            continue;
        }
        std::string num_id_str = std::to_string(num_id);
        while (num_id_str.size() < 4)
            num_id_str = "0" + num_id_str;
        num_id_str = num_id_str + ".png";

        if (range.second >0 && (int_id-range.first) % range.second == 0) { //create test set if step is present
            cam_names_test.emplace_back(num_id_str.substr(0, -4));

            cam_views_test.emplace_back();
            cam_views_test[cam_views_test.size() - 1].pose = pose;
        } else {
            cam_names.emplace_back(num_id_str.substr(0, -4));

            cam_views.emplace_back();
            cam_views[cam_views.size() - 1].pose = pose;

            std::filesystem::path jpg_path = path;
            jpg_path /= "color";
            jpg_path /= num_id_str;
            std::filesystem::path jpg_depth_path = path;
            jpg_depth_path /= "depth";
            jpg_depth_path /= num_id_str;


            if (std::filesystem::exists(jpg_depth_path)) { // depth present -> load depth path
                cam_views[cam_views.size() - 1].loadFromFile_RGB_D(jpg_path, jpg_depth_path);
            } else { // depth not present -> load rgb as depth
                std::cout << "[Dataset::load_generic_dataset] WARNING: No depth images found in " << jpg_depth_path
                          << ". Use RGB images as depth. You should render the depths and put them into the dataset folder."
                          << std::endl;
                cam_views[cam_views.size() - 1].loadFromFile_RGB_D(jpg_path, jpg_path);
            }
            if (camCount <= cam_views.size()) break;
        }
        ++int_id;
    }



    std::cout << "Parsed cam_names: " << cam_names.size() << std::endl;
    std::cout << "Parsed cam_views: " << cam_views.size() << std::endl;

    camCount = cam_views.size();

    std::cerr << "[Dataset::load_generic_dataset] Finished Parsing" << std::endl;
}

void View::loadFromFile_RGB_D(std::filesystem::path& file_path, std::filesystem::path& depth_path) {
	//std::cout << "load rgb" << std::endl;
	// load image from disk
	stbi_set_flip_vertically_on_load(1);
	int channels;
	int tex_channels = 4;
	GLint internal_format;
	GLenum format;
	GLenum type;
	uint8_t* data = 0;

	int w = 0;
	int h = 0;
	data = stbi_load(file_path.string().c_str(), &w, &h, &channels, 0);
    //std::cout << "file_channels " << channels << " w " << w << " h " <<std::endl;
	internal_format = channels_to_float_format(tex_channels);
	format = channels_to_format(tex_channels);
	type = GL_FLOAT;

	if (!data) {
		std::cerr << "[View::loadFromFile_RGB_D] FATAL ERROR: File not found: " << file_path << std::endl;
		throw std::runtime_error("Failed to load image file: " + file_path.string());
		return;
	}
	if (channels < 1 || channels>4)
		throw std::runtime_error("Image " + file_path.string() +
			" has unexpected number of channels: " + std::to_string(channels));

	std::vector<float> flipped_data;
	//write tex to cpu buffer

	flipped_data.reserve(w * h * tex_channels);
	//std::cout << "read rgb" << std::endl;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int index = (x + y * w) * channels;
			for (int i = 0; i < 3; ++i) {
				flipped_data.push_back(float(data[index + i]) / 255.f);
			}
			flipped_data.push_back(1);
		}
	}



	//opengl by default needs 4 byte alignment after every row
	//stbi loaded data is not aligned that way -> pixelStore attributes need to be set
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);



	// free data
	stbi_image_free(data);
	//std::cout << "load depth" << std::endl;
	data = stbi_load(depth_path.string().c_str(), &w, &h, &channels, 0);

	if (!data) {
		throw std::runtime_error("Failed to load image file: " + file_path.string());
		return;
	}
	if (channels < 1 || channels>4)
		throw std::runtime_error("Image " + file_path.string() +
			" has unexpected number of channels: " + std::to_string(channels));
	//write tex to cpu buffer
	//std::cout << "read depth" << std::endl;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			int index = (x + y * w) * channels;
			int tex_index = (x + y * w) * tex_channels;

			flipped_data[tex_index + 3] = (float(data[index]) / 255.f);

		}
	}



	//opengl by default needs 4 byte alignment after every row
	//stbi loaded data is not aligned that way -> pixelStore attributes need to be set
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);



	// free data
	stbi_image_free(data);

	tex_gpu = Texture2D(file_path.string(), w, h, internal_format, format, type, flipped_data.data(), true);
	//std::cout << "finished" << std::endl;
}
