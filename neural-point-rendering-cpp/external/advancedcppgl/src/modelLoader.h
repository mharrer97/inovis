#pragma once
#include <cppgl.h>
#include <iostream>
#include <fstream>

namespace ModelLoader {
	
	std::vector<std::pair<Geometry, Material>> loadModel(const fs::path& path, bool normalized_model = false, bool use_binary_dump = false);
	std::vector<std::pair<Geometry, Material>> load_meshes(const fs::path& path, bool normalize);

	std::vector<std::pair<Geometry, Material>> loadBinary(std::ifstream& binary_file);
	void storeBinary(std::ofstream& binary_file, std::vector<std::pair<Geometry, Material>>& model);

}