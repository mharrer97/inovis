#pragma once

#include "context.h"
#include "imgui/imgui.h"
#include "anim.h"
#include "mesh.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "material.h"
#include "geometry.h"
#include "drawelement.h"
#include "framebuffer.h"

// -------------------------------------------
// callbacks

void gui_add_callback(const std::string& name, void (*fn)(void));
void gui_remove_callback(const std::string& name);

// -------------------------------------------
// main draw call

void gui_draw();

// -------------------------------------------
// helper functions to display properties

void gui_display_mat4(glm::mat4& mat);
void gui_display_camera(Camera& cam);
void gui_display_texture(const Texture2D tex, const glm::ivec2& size = glm::ivec2(300, 300));
void gui_display_shader(Shader& shader);
void gui_display_framebuffer(const Framebuffer& fbo);
void gui_display_material(const Material& mat);
void gui_display_geometry(const Geometry& geom);
void gui_display_mesh(const Mesh& mesh);
void gui_display_drawelement(Drawelement& elem);
void gui_display_animation(const Animation& anim);
void gui_display_query_timer(const Query& query, const char* label="");
void gui_display_query_counter(const Query& query, const char* label="");
