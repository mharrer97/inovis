#pragma once

#include "parserConfig.h"
#include <glm/glm.hpp>
#include <vector>
#include <torch/script.h>
// struct for representation and sorting of groundtruth capture views
struct Capture_View {
	int id = 0;		// cam id
	int num = 0;
	glm::vec3 pos = glm::vec3(0, 0, 0);	// capture pos
	glm::vec3 dir = glm::vec3(0, 0, 0);	// capture direction
	float similarity_descriptor = std::numeric_limits<float>::infinity();

	Capture_View(int id, glm::vec3 pos, glm::vec3 dir) : id(id), num(0), pos(pos), dir(dir) {	}
	Capture_View(int id, int num, glm::vec3 pos, glm::vec3 dir) : id(id), num(num), pos(pos), dir(dir) {	}
};

struct ocam_model {
	glm::ivec2 image_size;
	float c;
	float d;
	float e;
	float cx;
	float cy;
	int world2cam_size;
	float world2cam[kMaxOcamParams]; //18 for the navis data set
	int cam2world_size;
	float cam2world[kMaxOcamParams]; //7 for the navis data set
};


// compute a descriptor to compare view overlap of different views
void get_capture_view_similarity(std::vector<Capture_View>& views, const glm::vec3& pos, const glm::vec3& dir);

void print_tensor(std::string name, torch::Tensor & ten);

void printMat(std::string name, glm::mat4 m);
void printMat(std::string name, glm::mat3 m);
