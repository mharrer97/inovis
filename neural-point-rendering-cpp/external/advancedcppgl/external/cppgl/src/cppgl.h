#pragma once

#include "anim.h"
#include "buffer.h"
#include "camera.h"
#include "context.h"
#include "debug.h"
#include "drawelement.h"
#include "framebuffer.h"
#include "geometry.h"
#include "gui.h"
#include "material.h"
#include "mesh.h"
#include "named_handle.h"
#include "platform.h"
#include "quad.h"
#include "query.h"
#include "shader.h"
#include "texture.h"

//glm to string with <<operators
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/type_trait.hpp>
#include <type_traits>
template<typename Mat, std::enable_if_t<glm::type<Mat>::is_vec || glm::type<Mat>::is_mat || glm::type<Mat>::is_quat, void*> = nullptr>
std::ostream& operator<<(std::ostream& os, const Mat& mat) {
    return os << glm::to_string(mat);
}