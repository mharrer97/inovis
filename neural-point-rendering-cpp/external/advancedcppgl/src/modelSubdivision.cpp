#include "modelSubdivision.h"
#include <functional>
#include "glm/gtx/string_cast.hpp"


std::vector<SubdividedGeometry> ModelSubdivision::subdivideGeometry(Geometry& geo, unsigned int current_recursion_depth, unsigned int max_triangles_per_submodel) {


	//transfer to triangle data structure
	std::vector<Triangle> triangles;
	for (size_t indexbuffer_index = 0; indexbuffer_index < geo->indices.size(); indexbuffer_index += 3) {
		glm::vec3 p_tri[3];
		unsigned int ind_tri[3];
		for (int i = 0; i < 3; ++i) {
			unsigned int index = geo->indices[indexbuffer_index + i];
			p_tri[i] = geo->positions[index];
			ind_tri[i] = index;

		}
		triangles.emplace_back(p_tri[0], p_tri[1], p_tri[2], ind_tri[0], ind_tri[1], ind_tri[2]);
	}

	//vector of triangles will be sorted based on subdivision attribute, but stay in the same container.
	//for access, a list of iterators is stored, indicating the subdivided parts of the buffer
	std::vector<std::vector<Triangle>::iterator> list_of_subdivided_triangles;

	subdivideTriangleSoup(triangles.begin(), triangles.end(), list_of_subdivided_triangles, max_triangles_per_submodel);

	std::vector<SubdividedGeometry> result;
	//create new geometry based on subdivision
	for (unsigned int iter_index = 0; iter_index < list_of_subdivided_triangles.size() - 1; iter_index+=2) {
		std::vector<Triangle>::iterator begin_iterator = list_of_subdivided_triangles[iter_index];
		std::vector<Triangle>::iterator end_iterator = list_of_subdivided_triangles[iter_index + 1];

		std::vector<unsigned int> indices;

		for (auto triangle_iterator = begin_iterator; triangle_iterator < end_iterator; ++triangle_iterator) {
			auto& tri = *triangle_iterator;
			//use all three vertices of a triangle
			for (auto index_triangle : { tri.i0, tri.i1, tri.i2 }) {
				indices.push_back(index_triangle);
			}
		}
		result.push_back(SubdividedGeometry(geo->name + "_subd_" + std::to_string(iter_index), geo, indices));

	}

	return result;
}

glm::vec3 ModelSubdivision::getAABBdimensions(std::vector<Triangle>::iterator triangles_begin, std::vector<Triangle>::iterator triangles_end) {
	glm::vec3 bb_min = glm::vec3(FLT_MAX);
	glm::vec3 bb_max = glm::vec3(FLT_MIN);;
	for (auto tri = triangles_begin; tri != triangles_end; ++tri) {
		bb_min = glm::min(bb_min, tri->triangleCenter);
		bb_max = glm::max(bb_max, tri->triangleCenter);
	}
	return bb_max - bb_min;
}


void ModelSubdivision::subdivideTriangleSoup(std::vector<Triangle>::iterator triangles_begin, std::vector<Triangle>::iterator triangles_end, std::vector<std::vector<Triangle>::iterator>& iterators, unsigned int max_triangles_per_submodel) {
	//recursive end condition
	if (std::distance(triangles_begin, triangles_end) < max_triangles_per_submodel) {
		{
			iterators.push_back(triangles_begin);
			iterators.push_back(triangles_end);
		}
		return;
	}

	//find largest edge in aabb and set compare function to that dimension
	std::function<bool(Triangle&, Triangle&)> compare;
	glm::vec3 aabb_length = getAABBdimensions(triangles_begin, triangles_end);
	if (aabb_length.x > aabb_length.y &&
		aabb_length.x > aabb_length.z) {
		compare = [](Triangle& a, Triangle& b) {return a.triangleCenter.x > b.triangleCenter.x; };
	}
	else if (aabb_length.y > aabb_length.z) {
		compare = [](Triangle& a, Triangle& b) {return a.triangleCenter.y > b.triangleCenter.y; };
	}
	else {
		compare = [](Triangle& a, Triangle& b) {return a.triangleCenter.z > b.triangleCenter.z; };
	}

	//sort triangle centers by this compare function
	std::sort(triangles_begin, triangles_end, compare);

	std::vector<Triangle>::iterator triangles_middle = triangles_begin + std::distance(triangles_begin, triangles_end) / 2 + 1;

	//subdivideTriangleSoup(triangles_begin, triangles_middle, iterators, max_triangles_per_submodel);
	//subdivideTriangleSoup(triangles_middle, triangles_end, iterators, max_triangles_per_submodel);

	for (int i : {0, 1})
	{
		subdivideTriangleSoup((i==0)?triangles_begin: triangles_middle, (i==0)?triangles_middle: triangles_end, iterators, max_triangles_per_submodel);

	}

}

SubdividedGeometryImpl::SubdividedGeometryImpl(std::string name, Geometry geo, std::vector<unsigned int> indices)
	:name(name),original_geo(geo),indices(indices) 
{
	bb_min = glm::vec3(FLT_MAX);
	bb_max = glm::vec3(FLT_MIN);;
	for (auto& index: indices){
		auto& pos = geo->positions[index];
		bb_min = glm::min(bb_min, pos);
		bb_max = glm::max(bb_max, pos);
	}
}