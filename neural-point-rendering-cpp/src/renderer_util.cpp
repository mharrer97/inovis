#include "renderer_util.h"
#include <glm/gtc/matrix_access.hpp>

// compute a descriptor to compare view overlap of different views
void get_capture_view_similarity(std::vector<Capture_View>& views, const glm::vec3& pos, const glm::vec3& dir) {
	for (auto& v : views) {
		// squared distance -> penalize views far away even more
		// handle dist of sqrt(x) close enough and discard even closer positions to avoid high impact of extremely close position
		glm::vec3 diff = pos - v.pos;
		float positional_descriptor = (std::max<float>(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z, 0.5f) ) + 0.5f;

		// invert range -> same dir : 0, angle > 90: 1
		// add 0.1 -> same dir : 0.1, angle > 90: 1.1 -> same dir is not 0 -> positional still relevant
		float dot_angle = glm::dot(normalize(v.dir), normalize(dir)); // [-1:1] // represent angle between the two
		// invert to get lower = better
		dot_angle = 1.f - dot_angle; // [0:2] 0:same angle, 2: opposite direction
		dot_angle = dot_angle * 100.f; // scale to make 90 degree the same penalty as a distance of 10.f -> [0:200] 0:same angle, 100: 90 degree, 200: opposite
		dot_angle = dot_angle + 1.f; // [1:201] 1:same angle, 201: opposite direction

		float directional_descriptor = dot_angle;

		// final descriptor:
		// 1: perfect
		// 100: e.g. 0 deegree 10 dist
		// 100: e.g. 90 deegree 0 dist
		v.similarity_descriptor = positional_descriptor * directional_descriptor;
	}
}

void print_tensor(std::string name,  torch::Tensor& ten) {
	std::cout << name << ": \t" << " DataType " << ten.dtype() << std::endl;
	std::cout <<"\t\t mean " << ten.mean() << ", var " << ten.var() << ", is Cuda : " << ((ten.is_cuda()) ? "true" : "false") << "\n";
	//std::cout << name << ": \tis Cuda: " << ((ten.is_cuda()) ? "true" : "false") << "\n";
	std::cout << "\t\tshape" << ten.sizes() << "\n";
	std::cout << "\t\tstrides[";
	for (int i = 0; i < ten.sizes().size(); ++i) {
		std::cout << ten.stride(i);
		if (i != ten.sizes().size() - 1)
			std::cout << ", ";
	}
	std::cout << "]\n" << std::endl;
}

void printMat(std::string name, glm::mat4 m) {
	std::cout << name << std::endl;
	//std::cout << m << std::endl;
	std::cout << std::fixed;
	std::cout << std::setprecision(6);
	/*std::cout << "col1 " << m[1] << std::endl;
	std::cout << "col1 " << glm::column(m,1) << std::endl;
	std::cout << "col1 row2" << m[1][2] << std::endl;*/
	std::cout << "| " << m[0][0] << "\t" << m[1][0] << "\t" << m[2][0] << "\t" << m[3][0] << "\t|" << std::endl;
	std::cout << "| " << m[0][1] << "\t" << m[1][1] << "\t" << m[2][1] << "\t" << m[3][1] << "\t|" << std::endl;
	std::cout << "| " << m[0][2] << "\t" << m[1][2] << "\t" << m[2][2] << "\t" << m[3][2] << "\t|" << std::endl;
	std::cout << "| " << m[0][3] << "\t" << m[1][3] << "\t" << m[2][3] << "\t" << m[3][3] << "\t|" << std::endl;

}

void printMat(std::string name, glm::mat3 m) {
	std::cout << name << std::endl;
	//std::cout << m << std::endl;
	std::cout << std::fixed;
	std::cout << std::setprecision(6);
	/*std::cout << "col1 " << m[1] << std::endl;
	std::cout << "col1 " << glm::column(m,1) << std::endl;
	std::cout << "col1 row2" << m[1][2] << std::endl;*/
	std::cout << "| " << m[0][0] << "\t" << m[1][0] << "\t" << m[2][0]  << "\t|" << std::endl;
	std::cout << "| " << m[0][1] << "\t" << m[1][1] << "\t" << m[2][1]  << "\t|" << std::endl;
	std::cout << "| " << m[0][2] << "\t" << m[1][2] << "\t" << m[2][2]  << "\t|" << std::endl;

}