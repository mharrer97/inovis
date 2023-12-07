#ifndef __VERTEXLAYOUT_H__
#define __VERTEXLAYOUT_H__

#include <limits>

#include <glm/glm.hpp>

struct Vertex_lowP {
	glm::lowp_u16vec3 vPosition;
	glm::lowp_i16vec3 vNormal;
	glm::lowp_u16vec2 vTexCoord;

	static const GLenum pos_type = GL_UNSIGNED_SHORT;
	static const GLenum normal_type = GL_SHORT;
	static const GLenum tc_type = GL_UNSIGNED_SHORT;
	static const GLboolean pos_normalized = GL_TRUE;

	
	Vertex_lowP() :vPosition(0), vNormal(0), vTexCoord(0) {};
	Vertex_lowP(glm::vec3& p, glm::vec3& n, glm::vec2& tc) {
		float max = std::numeric_limits<unsigned short>::max();
		vPosition = glm::lowp_u16vec3(max * p);
		vTexCoord = glm::lowp_u16vec2(max * tc);
		max = std::numeric_limits<short>::max();
		vNormal = glm::lowp_i16vec3(max * n);
	}
	//Vertex_lowP(glm::vec3& p, glm::vec3& n, glm::vec2& t)
	//{
	//	vPosition = p;
	//	vNormal = n;
	//	vTexCoord = t;
	//};

};

struct Vertex_fullP {

	glm::vec3 vPosition;
	glm::vec3 vNormal;
	glm::vec2 vTexCoord;

	static const GLenum pos_type = GL_FLOAT;
	static const GLenum normal_type = GL_FLOAT;
	static const GLenum tc_type = GL_FLOAT;
	static const GLboolean pos_normalized = GL_FALSE;

	Vertex_fullP(glm::vec3& p, glm::vec3& n, glm::vec2& t)
	{
		vPosition = p;
		vNormal = n;
		vTexCoord = t;
	};
};

class VERTEX_ATTRIBUTES {
public: enum {
	A_POSITION = 0,
	A_NORMAL = 1,
	A_TEXCOORDS = 2,
	A_TANGENTS = 3,
	A_BITANGENTS = 4,

};
};

#endif // __VERTEXLAYOUT_H__

