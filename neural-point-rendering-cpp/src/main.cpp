#include "inferenceRenderer.h"
#include "pointCloudRenderer.h"

#include "texture_copy.h"
#include <torch/torch.h>

#include <iostream>

int main(int argc, char** argv) {


	bool do_inference = true;
	if (do_inference) {
		torch::NoGradGuard ngg;
		std::cerr << "[main] Create Point Cloud Renderer" << std::endl;
		InferenceRenderer ir;
		std::cerr << "[main] Start Rendering" << std::endl;
		ir.run(argc, argv);
		std::cerr << "[main] Finished" << std::endl;
	}
	else {
		std::cerr << "[main] Create Point Cloud Renderer" << std::endl;

		bool generate_point_clouds = false;

		PointCloudRenderer pcr;
		if (generate_point_clouds) {
			std::cerr << "[main] Start Creating Point Clouds" << std::endl;
			pcr.generatePointClouds();
		}
		else {
			std::cerr << "[main] Start Rendering" << std::endl;
			pcr.run(argc, argv);
		}
		std::cerr << "[main] Finished" << std::endl;
	}

}
