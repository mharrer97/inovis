/*
Adapted from Florian Guthleins parser
Who adapted from Saiga PLYLoader
*/

#pragma once
#include <fstream>
#include <iostream>
#include <vector>
#include "advcppglex.h"
#include "renderer_util.h"
#include "PointCloudData.h"
// ------------------------------------------
// PLYPointCloudParser

class PLYPointCloudParser {
public:
	struct Property {
		std::string name;
		std::string type;
	};

	bool log = false;
	std::string setType;
	bool cleared = true;
	// pairs for each element containing 1: offset/start of the property, 2: size of the property 
	std::vector<std::pair<int, int>> vertexOffsetType;
	std::vector<std::pair<int, int>> cameraOffsetType;

	size_t dataStart = 0; // should contain startindex of point cloud after the header -> 782
	// count of vertices in the pointcloud
	int vertexCount = -1; // should contain number of vertices of the pointcloud -> 72916178
	// size and properties(name,type pairs) of the elements
	int vertexSize = -1; // should contain size of vertex in bytes -> 31
	std::vector<Property> vertexProperties;
	int cameraSize = -1; // should contain size of camera in bytes -> 84
	std::vector<Property> cameraProperties;
	std::vector<char> data; // contains the whole file -> size 2260402384
	std::vector<char> header_data; // contains the first bytes describing the header -> size 782
	std::vector<char> camera_data; // contains the last bytes describing the camera -> size 84

	

	// store parsed data here
	std::vector<PointCloudPoint> points;
	PointCloudCameraData camera;

	std::vector<PointCloudVoxel> bounding_structure; // create strucutre with e.g. createBoundingStructureGrid before usage

	// list of parset poses to create timestamps
	std::vector<Capture_View> captured_views;
	/// <summary>
	/// default constructor -- do not use that
	/// </summary>
	PLYPointCloudParser() { std::cerr << "[PLYPointCloudParser] ERROR: Default constructor used -- use the consturctor that takes the file name!"; }
	/// <summary>
	/// Constructor to directly start parsing the pointcloud
	/// </summary>
	/// <param name="file">filename of the .ply file to parse</param>
	/// <param name="_log">flag if log should be generated to cerr. defaults to false</param>
	PLYPointCloudParser(const std::string& file, std::vector<Capture_View> _captured_views, std::string setType = "NavVis" , bool _log = false);
	~PLYPointCloudParser() {}


	// main functionalities
	void parseHeader();
	void parsePointCloud();

	void reducePointCloudByRadius(float radius);
	void reducePointCloudByBoundingBox(vec3& min, vec3& max);
	void reducePointCloudByFactor(int offset, int factor);
	void transformPointCloudToGL();
	void transformPointCloudToNavvis();

	void createBoundingStructureGrid(float cell_size);

	void savePointCloud(const std::string& name,
		const int count = -1);


	void clear();

	// helper functions
	int sizeoftype(std::string t);
	std::pair<vec3, vec3> getBoundingBox();


};