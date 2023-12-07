#pragma once
#include <texture.h>
#include "advcppglex.h"
#include "rgbCameras.h"


class View {
private:

public:
	struct Pose
	{
		glm::vec3 position = glm::vec3(0, 0, 0);	   // capture pos
		glm::quat orientation = glm::quat(1, 0, 0, 0); // captured quaternion: contains the view to world quaternion
		glm::vec3 direction = glm::vec3(0, 0, 0);	   // capture direction
		glm::mat4 view = glm::mat4();				   // contains the world to view matrix
	} pose, optimized_pose;

	void loadFromFile_RGB_D(std::filesystem::path& file_path, std::filesystem::path& depth_path);

    Texture2D tex_gpu;
    
    bool darius_optimized_pose_present = false;

	View() {}
	~View() {}
};


class Dataset {
private:

public:

	// views parsed to be used as inpput data for rendering
	std::vector<std::string> cam_names;
	std::vector<View> cam_views;
	// views parsed to be used as test dataset without being used as inpuit data
	std::vector<std::string> cam_names_test;
	std::vector<View> cam_views_test;

	// opengl camera to be used:
	float camera_near = 0.1f;
	float camera_far = 35.f;
	float camera_fov_degree = 30.f;
	float camera_aspect_ratio = 1.f;
	// groundtruth projection matrix -> 90 degree fov
	glm::mat4 gt_proj = glm::perspective(90.f * float(M_PI / 180), 1.764711f, 0.1f, 35.f);
	glm::mat4 gt_proj_cropped = glm::perspective(90.f * float(M_PI / 180), 1.764711f, 0.1f, 35.f);

	glm::ivec2 targetResolution = glm::ivec2(0, 0);
	
	int currentCam = 0;			// contains the default camera position loaded from the dataset
	int camCount = 0;			// contains how many cam positions of the dataset are selectable. (first 95 are in the big office. 172 would be all)

	Dataset() {}
	~Dataset() {}

	std::vector<Capture_View> getCaptureViewList();
    // range is used for different purposed: Kitti: beginning and end index of relevant images; generic: sart index and step of which images to use as test images
	void load(int cur, int size, std::string type, std::string path, std::string info, std::pair<int, int> range, glm::ivec2 targetRes, std::pair<int, int> test_start_step) {
		currentCam = cur;
		camCount = size;
		targetResolution = targetRes;
		if (type == "NavVis")
			load_NavVis_dataset(path);
		else if (type == "TanksAndTemples")
			load_TT_dataset(path);
		else if (type == "KITTY-360")
			load_KITTY_dataset(path, info, range,test_start_step);
		else if (type == "L")
			load_L_dataset(path);
		else if (type == "Redwood")
			load_Redwood_dataset(path);
		else if (type == "ScanNet")
			load_ScanNet_dataset(path);
        else if (type == "Generic")
			load_generic_dataset(path, test_start_step);
		else
			std::cerr << "[Dataset::load] FATAL ERROR: Dataset Type not recognized: " << type << "." << std::endl;
	}

	void load_NavVis_dataset(const std::string& path);
	void load_TT_dataset(const std::string& path);
	void load_KITTY_dataset(const std::string& path, std::string info, std::pair<int,int> range, std::pair<int,int> test_start_step);
	void load_L_dataset(const std::string& path);
	void load_Redwood_dataset(const std::string& path);
	void load_ScanNet_dataset(const std::string& path);
	void load_generic_dataset(const std::string& path,std::pair<int,int> range);

};
