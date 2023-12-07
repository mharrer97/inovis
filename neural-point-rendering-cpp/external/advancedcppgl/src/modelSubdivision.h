#pragma once
#include <cppgl.h>

//subdivided geometry only has an index buffer, referencing the original geometry's vertex buffer
//and a bounding box
class SubdividedGeometryImpl {
public:
	std::string name;
	Geometry original_geo;
	std::vector<unsigned int> indices;
	glm::vec3 bb_min, bb_max;

public:
	SubdividedGeometryImpl(std::string name, Geometry geo, std::vector<unsigned int> indices);
};

//not named_handled atm, as used only for internal use. Can be changed if necessary
using SubdividedGeometry = SubdividedGeometryImpl;


namespace ModelSubdivision {

	struct Triangle {
		//vertices
		//glm::vec3 v0, v1, v2;
		glm::vec3 triangleCenter;
		//indices
		unsigned int i0, i1, i2;
		Triangle():triangleCenter(0), i0(0), i1(0), i2(0) {};
		Triangle(glm::vec3 V0, glm::vec3 V1, glm::vec3 V2,
			unsigned int I0, unsigned int I1, unsigned int I2)
			:i0(I0), i1(I1), i2(I2), triangleCenter((V0 + V1 + V2) / glm::vec3(3.0)) {}
		//glm::vec3 center() { return (v0 + v1 + v2) / glm::vec3(3.0); }
	};

	//subdivide geometry by the largest edge in the aabb
	std::vector<SubdividedGeometry> subdivideGeometry(Geometry& geo, unsigned int current_recursion_depth = 0, unsigned int max_triangles_per_submodel = 1000);

	void subdivideTriangleSoup(std::vector<Triangle>::iterator triangles_begin, std::vector<Triangle>::iterator triangles_end, std::vector<std::vector<Triangle>::iterator>& iterators, unsigned int max_triangles_per_submodel);

	glm::vec3 getAABBdimensions(std::vector<Triangle>::iterator triangles_begin, std::vector<Triangle>::iterator triangles_end);

}

