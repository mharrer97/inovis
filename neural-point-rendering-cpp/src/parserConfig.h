#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#define NUMBER_OF_CAMERAS_PER_CP 4
#define MAX_NUMBER_OF_CAPTURE_POSITIONS 172
#define kMaxOcamParams 18




/*
* This is used to transform input NavVis format to common OpenGL formats and back
*/



inline glm::vec3 navVis_pos_to_Opengl(glm::vec3 pos) {
	return pos;
}

inline glm::quat navVis_orient_to_Opengl(glm::quat q) {

	return q;
}

inline glm::quat navVis_rgbCamOrient_to_Opengl(glm::quat q) {

	return q;
//	glm::mat4 rightHanded_to_LefthandedGL(
//		1.0f, 0.0f, 0.0f, 0.0f,
//		0.0f, -1.0f, 0.0f, 0.0f,
//		0.0f, 0.0f, -1.0f, 0.0f,
//		0.0f, 0.0f, 0.0f, 1.0f);
//	
//	glm::mat4 view = rightHanded_to_LefthandedGL * glm::mat4_cast(q);
//	return glm::quat_cast(view);


}

inline glm::quat convertNavvisToLookAt(glm::vec3 pos, glm::quat rot) {
	glm::mat3 m3 = glm::mat3(1, 0, 0, 0, -1, 0, 0, 0, 1);
	glm::mat4 v = glm::lookAt(pos, pos + m3 * glm::mat3_cast(glm::inverse(rot)) * glm::vec3(1, 0, 0),
		m3 * glm::mat3_cast(glm::inverse(rot)) * glm::vec3(0, 0, 1));
	return glm::quat_cast(v);
}

//inline glm::vec3 to_Navvis(glm::vec3 opengl) {
//	assert(0);
//	return opengl;
//}