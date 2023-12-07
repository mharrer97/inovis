#pragma once
#include <cppgl.h>
#include <advancedcppgl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

#include <iostream>
#include <fstream>
#include <array>
#include <chrono>

#include "advcppglex.h"
#include "dataset.h"

#include <torch/script.h>
#include "texture_copy.h"


//#define LOD_LEVELS 6 //defines how many lod levels should be loaded


// structs for config etc
struct GUI_Parameters_IR {
	glm::ivec2 initial_resolution_default = ivec2(480, 272); // ivec2(640, 480);// ivec2(960, 544);// 540; // initial vertical res to go back to if wanted //768 // 0: 768; // 1: 384; // 2: 192; // 3: 96; // 4: 48;
	int draw_gui = 1; // contains which guis should be drawn 0: no gui, 1: custom gui, 2: cppl gui, 3: both  
	//int lodPointSizesGL[LOD_LEVELS] = { 1,2,5,7,15,30 };
	// Contains the preset point sizes for each lod in gl points
	int pointSizeGL = 1;//1.f				// Point Size used for rendering with gl_Points, is reset on switching lod
	// Contains the preset point sizes for each lod in oriented quad rendering
	int resolution_modifier = 1;		// modifier on which resolution is used for the fbo and screenshots compared to the context/window size (0:2, 1:1, 2:0.5, 3:0.25, 4:0.125, 5:0.0625)
	bool enableCulling = false;			// contains whether culling is used
	int lod = 0;						// contains the current lod level to be displayed -> set initial lod here
	int lod_amount = -1;				// number of loaded lods. filled on startup
	//bool alternatePointClouds = false;	// contains if pointclouds are alternated, i.e. different clouds of the same reolution are switched out each frame
	//int currentCloudIndex = 0;			// contains the index of the current cloud for alternating clouds
	//int pointCloudPartAmount = 4;		// contains number of different point cloud parts 
	float aabb_extend = 10.f;			// contains the extend of the point cloud, is computed after parsing and should normally not be adjusted. used for 
	float fov = 90.f;					// contains the current fov of the projective camera 
	vec3 campos = vec3(0);				// contains the current position of the camera
	
	bool use_darius_optimized_positions = true; // determine if optimized positions should be used

	bool animationRunning = false;	// for toggling if the currently set animation should run repeatetly
	float animationSpeed = 0.5f; // speed modifier of the animation
	bool animationForward = true; //if animation is played forward or backward
    bool animation_use_secondary = true; // if true, the secondary animation is used -> not the animation that follows the camera capture path
    bool animation_show_capture_frustum = true; // if true, shows the frustum of the capture animation
    bool animation_visualize_primary_path = false;
    bool animation_visualize_secondary_path = false;

	int displayMode = 1; // how renderings are displayed - 0: Single Output, 1: Multiple (4) Outputs
	int currentRenderInfo = 8;			// Render Detail containing wich information should be displayed
	bool show_depth_bg_white = false;
	bool show_diff = true;
	bool mipmap_motion = true;
	bool loadGroundtruth = true;
	bool skipNearest = true;			// skip the nearest groundtruth image?
	bool log_nearest_views = false;
	bool use_dataset_proj = true;
	bool use_dataset_proj_cropped = true;
	bool use_timestamp = true;
	bool preInitMV = true;
	bool use_taa = false;
	bool taa_update_on_move = true;

	int network_id = 0;					// 0: napr2, 1: aliev
	int prev_network_id = 0;				// previous network_id. for switching between the last two networks
	int network_amount = -1; // number of loaded networks. filled on startup
	glm::ivec2 res0 = initial_resolution_default;
	glm::ivec2 res1 = initial_resolution_default/2;
	glm::ivec2 res2 = initial_resolution_default/4;
	glm::ivec2 res3 = initial_resolution_default/8;

	std::vector<std::string> network_filenames{}; // filename of the cloud without the '.pt' after

	std::vector<int> network_feature_extraction_depth{};
	std::vector<int> network_movec_channels{};
	std::vector<int> network_groundtruth_amount{};
	std::vector<int> network_prevInitMV{};
	bool increment_image = false; // dummy to tell the renderer to use the next image in the dataset
	bool decrement_image = false; // dummy to tell the renderer to use the previous image in the dataset
	// capture stuff 
	bool startCapturing = false;
	bool capturing = false;
	bool captureDepthOnly = false;
	int captureGroundtruthAmount= 4;
	bool parsedTestImages = false;
	bool cycleTestImages = false;
    bool captureCustomAnimation = false;

	bool startCaptureByIndex = false;
	bool captureByIndex = false;
	int captureByIndexStart = 0;
	int captureByIndexStep = 1;
	int captureByIndexEnd = 0;
	int captureByIndexCurrent = 0;

	bool startCaptureVideo = false;
	bool captureVideo = false;
	int captureVideoIndex = 0;
	int captureVideoFrameTime = 100;
	int captureVideoSkipControlPoint = 1; // factor of how much less control points the capture animation has. 1 -> the same animation, 2 -> every second image, etc.
	int captureVideoStartIndex = 0;

    bool debug_camera_frustum = false;
	bool debug_camera_frustum_currentCam = false;
    float debug_camera_frustum_size = 0.1;
    float debug_camera_frustum_size_current = 0.2;
    bool debug_camera_frustum_size_adjusted = false;
    int debug_frustum_amount = 0;

};





// TODO ideally use a member variable, but interfers with keyboard kallback, since that changes gui params as well
static GUI_Parameters_IR gui_params_ir;

class InferenceRenderer {

private:
	// if false, the inference is not done. Used for e.g. displaying gt images in its native resolution or creating depth images from the point cloud for gt sizes that are not divisible by 16
	bool doInference = true;

	void setCurrentCam(int id, bool test = false);
	glm::mat4 getView(int id, bool test = false);
	void loadPointClouds(std::vector<PointCloud>& pcs, std::vector<std::string> files);
	//void loadPointCloudParts(std::vector<PointCloud>& pcm);
	void custom_gui_select_dataset();
	void custom_gui_draw();
	void createRandomAnimation(glm::vec3 endpos, glm::quat endrot);

	// data capturing sstuff for training
	void startCapture(); // init capturing data and set first pos
	void capture(); //advance counter and set next campos

	void startCaptureVideo();
	void captureVideoFrame();

	void startCaptureByIndex();
	void captureByIndex();
	//name of the set. used to load a correspoinding pointcloud for most datasets
	std::vector<std::string> setName{};
	//main folder of the dataset
	std::vector<std::string> setFolder{};
	//Type of set. different parsing operations are done for different types
	std::vector<std::string> setType{};
	//number of images to load
	std::vector<int>		 setSize{ };
	//std::vector<int>		 setSize{ 20,20,20, 20, 20};
	//additional info in a string. not used for navvis and TanksAndTemples
	std::vector<std::string> setInfo{ };
	//width of the gt images of the dataset
	std::vector<int> setWidth{ };
	//height of the gt images of the dataset
	std::vector<int> setHeight{ };
	std::pair<int, int> setKittyPCRange;
    std::pair<int, int> setGenericTestStartStep = {0,0};
	//std::vector<std::string> pointCloud_filenames{ "lod_0", "lod_2" , "lod_3" }; // filename of the cloud without the 'setX_' upfront and the '.ply' after
	std::vector<std::string> pointCloud_filenames{ "lod0", "lod1"};// , "lod_1"}; // filename of the cloud without the 'setX_' upfront and the '.ply' after
	int targetWidth = 0;
	int targetHeight = 0;
	bool enforceTargetResolution = false;
	int dataset_id = 0;

	int gt_timestamp_min = 0;
	int gt_timestamp_max = 0;

	

public:
	std::chrono::time_point<std::chrono::system_clock> time_start;

	Dataset dataset;
	

	// vector for sorting the nearest capture views. 
	// each entry consists of a pair of int,int that contains the camera id first and cmera num second
	// also contains a pair of camera positions and camera directions which should be used for sorting
	std::vector <Capture_View> nearest_views; 

	std::string lastCubePos = "";
	bool takeScreenshot = false;

	//TODO delete debug variable
	float debugFloat = 0.f;
	int debugInt = 1;
	bool debugBool = false;
	SSBO ocamModelShaderStorageBuffer;
	Animation myAnimation;
	Animation standardAnimation;
	Animation captureAnimation;
    Animation customAnimation;
    float custom_to_standard_factor = 1.f;
	
	//for motion vecs
	glm::mat4 view_old = glm::mat4(1);

	std::string cur_out_fbo;
	
	InferenceRenderer() { time_start = std::chrono::system_clock::now(); }
	~InferenceRenderer(){}

	void run(int argc, char** argv);

	
};