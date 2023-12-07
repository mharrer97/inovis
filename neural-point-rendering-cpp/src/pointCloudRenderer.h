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
#include <fstream>
#include <iostream>

#include "advcppglex.h"
#include "renderer_util.h"

#define VERTICAL_RESOLUTION 544//768 // 0: 768; // 1: 384; // 2: 192; // 3: 96; // 4: 48;		 //1536;//990;
#define LOD_LEVELS 2//6 //defines how many lod levels should be loaded


// structs for config etc
struct GUI_Parameters_PCR {
	int draw_gui = 1; // contains which guis should be drawn 0: no gui, 1: custom gui, 2: cppl gui, 3: both  
	int lodPointSizesGL[LOD_LEVELS] = { 1,2 };//, 5, 7, 15, 30};
	// Contains the preset point sizes for each lod in gl points
	float pointSizeGL = 0.1f;//1.f				// Point Size used for rendering with gl_Points, is reset on switching lod
	float lodPointSizesOQ[LOD_LEVELS] = { 0.0048f,0.0072f };// , 0.012f, 0.024f, 0.06f, 0.12f};
	// Contains the preset point sizes for each lod in oriented quad rendering
	int resolution_modifier = 1;		// modifier on which resolution is used for the fbo and screenshots compared to the context/window size (0:2, 1:1, 2:0.5, 3:0.25, 4:0.125, 5:0.0625)
	float pointSizeOQ = 1.f;				// Point Size Multiplier used for rendering with oriented quads (1.f means preset radius, 0.5f half radius etc.)
	int currentRenderInfo = 0;			// Render Detail containing wich information should be displayed(color, normals, curvature, depthmask, motion vectors, etc)
	int renderMode = 0;					// Contains ho the pointcloud should be rendered (glPoints, smooth glPoints, oriented quads, etc)
	bool enableCulling = true;			// contains whether culling is used
	int lod = 0;						// contains the current lod level to be displayed -> set initial lod here
	bool alternatePointClouds = false;	// contains if pointclouds are alternated, i.e. different clouds of the same reolution are switched out each frame
	int currentCloudIndex = 0;			// contains the index of the current cloud for alternating clouds
	int pointCloudPartAmount = 4;		// contains number of different point cloud parts 
	int cameraMode = 0;					// contains the currently used camera mode (Projective or OCAM)
	float aabb_extend = 10.f;			// contains the extend of the point cloud, is computed after parsing and should normally not be adjusted. used for 
	float fov = 90.f;					// contains the current fov of the projective camera 
	vec3 campos = vec3(0);				// contains the current position of the camera
	int currentNavvisCam = 0;			// contains the default camera position loaded from the navvis dataset
	int navvisCamCount =  172;			// contains how many cam positions of the dataset are selectable. (first 95 are in the big office. 172 would be all)
	bool use_darius_optimized_positions = true; // determine if optimized positions should be used
	std::vector<std::string> navvisCameraSelection;
	// contains the strings used for selecting the navvis cam positions
	int num = 1;						// contains which of the 4 cameras should be loaded from the dataset

	int capture_startIndex = 0; // start index for capturing
	int capture_stepSize = 1; // step size for capturing, each x camposition is used for the captured dataset
	bool capturePreviousFramesAnimated = false; // for capturing the content of previous Frames as well
	int capturePreviousFramesAmount = 3; // amount of previous frames to be captured
	bool startCapturing = false; // set to true by keyboard callback, if capturing should be started by keypress of c
	bool captureGroundtruthAsPrevious = false;
	bool animationRunning = false;	// for toggling if the currently set animation should run repeatetly
	float animationSpeed = 0.5f; // speed modifier of the animation

	bool resized = false;				// contains whether the window should be resized after the render iteration

	bool showGroundtruth = true;
	bool loadGroundtruth = true;
};



static GUI_Parameters_PCR gui_params_pcr;

class PointCloudRenderer {
private:
	
	void setCurrentCam(ReferenceFrameRGBCamera refCam, RGBCameras rgbCam, int view_in_cp);
	void loadPointClouds(std::vector<PointCloud>& pcm);
	void loadPointCloudParts(std::vector<PointCloud>& pcm);
	void custom_gui_draw();
	void createRandomAnimation(glm::vec3 endpos, glm::quat endrot);

	// data capturing sstuff for training
	void startCaptureTrainingData(); // init capturing data and set first pos
	void captureTrainingData(); //advance counter and set next campos

	// information for data capturing, filled by captureTrainingData prior to capturing all data
	std::vector<int> capture_cameraIndices; // indices that indicate which cam postions should be used (see gui_params_pcr.currentNavvisCam)
	std::array<bool,4> capture_cameraNums = { true,true,true,true }; // which camNums should be ued onn each index (see gui_params_pcr.num)
	int capture_numCounter = 0; // counter to cycle through the nums
	std::array<bool, 5> capture_renderInfo = { true,false,false,false, false }; // containing wich information should be captured(color, normals, curvature, depth, motionetc)
	int capture_infoCounter = 0; // counter to cycle through the render infos
	int capture_currentFrameCounter = 0;

	std::array<std::string, 2> setFolder{ "../../../navvis_office_zip/2020-08-05_18.12.08_hq_5th_floor_whitebalance/", "../../../navvis_office_zip/2021-05-01_13.37.18/" };
	int dataset = 0;

	

public:
	std::chrono::time_point<std::chrono::system_clock> time_start;

	//contains the camera positions and orientations (and groundtruth textures if enabled) for all 4 camera nums for each camera id
	std::vector<RGBCameras> rgbCams;
	
	//contains the camera calibration parameters for all 4 camera nums
	ReferenceFrameRGBCamera refCam;

	// vector for sorting the nearest capture views. 
	// each entry consists of a pair of int,int that contains the camera id first and cmera num second
	// also contains a pair of camera positions and camera directions which should be used for sorting
	std::vector <Capture_View> nearest_views; 


	float debugFloat = 0.f;
	int debugInt = 1;
	bool createData = false; // true if a screenshot should be taken in the next frame with camera position set to the current config
	SSBO ocamModelShaderStorageBuffer;
	Animation myAnimation;
	//for motion vecs
	glm::mat4 view_old;
	PointCloudRenderer() { time_start = std::chrono::system_clock::now(); }
	~PointCloudRenderer(){}

	void generatePointClouds();
	void run(int argc, char** argv);

	
};