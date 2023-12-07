#include "gui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <map>

// -------------------------------------------
// callbacks

static std::map<std::string, void (*)(void)> gui_callbacks;

void gui_add_callback(const std::string& name, void (*fn)(void)) {
    gui_callbacks[name] = fn;
}

void gui_remove_callback(const std::string& name) {
    gui_callbacks.erase(name);
}

// -------------------------------------------
// main setup, draw and shutdown routines

void gui_draw() {
    // show timers/queries in top left corner
    ImGui::SetNextWindowPos(ImVec2(10, 20));
    ImGui::SetNextWindowSize(ImVec2(350, 500));
    if (ImGui::Begin("CPU/GPU Timer", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground)) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .9f);
        for (const auto& [name, query] : TimerQuery::map) {
            ImGui::Separator();
            gui_display_query_timer(*query, name.c_str());
        }
        for (const auto& [name, query] : TimerQueryGL::map) {
            ImGui::Separator();
            gui_display_query_timer(*query, name.c_str());
        }
        for (const auto& [name, query] : PrimitiveQueryGL::map) {
            ImGui::Separator();
            gui_display_query_counter(*query, name.c_str());
        }
        for (const auto& [name, query] : FragmentQueryGL::map) {
            ImGui::Separator();
            gui_display_query_counter(*query, name.c_str());
        }
        ImGui::PopStyleVar();
    }
    ImGui::End();

    static bool gui_show_cameras = false;
    static bool gui_show_textures = false;
    static bool gui_show_fbos = false;
    static bool gui_show_shaders = false;
    static bool gui_show_materials = false;
    static bool gui_show_geometries = false;
    static bool gui_show_drawelements = false;
    static bool gui_show_animations = false;

    if (ImGui::BeginMainMenuBar()) {
        // camera menu
        ImGui::Checkbox("cameras", &gui_show_cameras);
        ImGui::Separator();
        ImGui::Checkbox("textures", &gui_show_textures);
        ImGui::Separator();
        ImGui::Checkbox("fbos", &gui_show_fbos);
        ImGui::Separator();
        ImGui::Checkbox("shaders", &gui_show_shaders);
        ImGui::Separator();
        ImGui::Checkbox("materials", &gui_show_materials);
        ImGui::Separator();
        ImGui::Checkbox("geometries", &gui_show_geometries);
        ImGui::Separator();
        ImGui::Checkbox("drawelements", &gui_show_drawelements);
        ImGui::Separator();
        ImGui::Checkbox("animations", &gui_show_animations);
        ImGui::Separator();
        if (ImGui::Button("Screenshot"))
            Context::screenshot("screenshot.png");
        ImGui::EndMainMenuBar();
    }

    if (gui_show_cameras) {
        if (ImGui::Begin(std::string("Cameras (" + std::to_string(Camera::map.size()) + ")").c_str(), &gui_show_cameras)) {
            ImGui::Text("Current: %s", current_camera()->name.c_str());
            for (auto& [name, cam] : Camera::map) {
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_camera(cam);
            }
        }
        ImGui::End();
    }

    if (gui_show_textures) {
        if (ImGui::Begin(std::string("Textures (" + std::to_string(Texture2D::map.size()) + ")").c_str(), &gui_show_textures)) {
            for (const auto& [name, tex] : Texture2D::map) {
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_texture(tex, glm::ivec2(300, 300));
            }
        }
        ImGui::End();
    }

    if (gui_show_shaders) {
        if (ImGui::Begin(std::string("Shaders (" + std::to_string(Shader::map.size()) + ")").c_str(), &gui_show_shaders)) {
            for (auto& [name, shader] : Shader::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_shader(shader);
            if (ImGui::Button("Reload modified")) reload_modified_shaders();
        }
        ImGui::End();
    }

    if (gui_show_fbos) {
        if (ImGui::Begin(std::string("Framebuffers (" + std::to_string(Framebuffer::map.size()) + ")").c_str(), &gui_show_fbos)) {
            for (const auto& [name, fbo] : Framebuffer::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_framebuffer(fbo);
        }
        ImGui::End();
    }

    if (gui_show_materials) {
        if (ImGui::Begin(std::string("Materials (" + std::to_string(Material::map.size()) + ")").c_str(), &gui_show_materials)) {
            for (const auto& [name, mat] : Material::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_material(mat);
        }
        ImGui::End();
    }

    if (gui_show_geometries) {
        if (ImGui::Begin(std::string("Geometries (" + std::to_string(Geometry::map.size()) + ")").c_str(), &gui_show_geometries)) {
            for (const auto& [name, geom] : Geometry::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_geometry(geom);
        }
        ImGui::End();
    }

    if (gui_show_drawelements) {
        if (ImGui::Begin(std::string("Drawelements (" + std::to_string(Drawelement::map.size()) + ")").c_str(), &gui_show_drawelements)) {
            for (auto& [name, elem] : Drawelement::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_drawelement(elem);
        }
        ImGui::End();
    }

    if (gui_show_animations) {
        if (ImGui::Begin(std::string("Animation (" + std::to_string(Animation::map.size()) + ")").c_str(), &gui_show_animations)) {
            for (auto& [name, anim] : Animation::map)
                if (ImGui::CollapsingHeader(name.c_str()))
                    gui_display_animation(anim);
        }
        ImGui::End();
    }

    // call callbacks
    for (const auto& [name, fn] : gui_callbacks)
        fn();
}

// -------------------------------------------
// helpers

void gui_display_camera(Camera& cam) {
    ImGui::Indent();
    ImGui::Text("name: %s", cam->name.c_str());
    ImGui::DragFloat3(("pos##" + cam->name).c_str(), &cam->pos.x, 0.001f);
    ImGui::DragFloat3(("dir##" + cam->name).c_str(), &cam->dir.x, 0.001f);
    ImGui::DragFloat3(("up##" + cam->name).c_str(), &cam->up.x, 0.001f);
    ImGui::Checkbox(("fix_up_vector##" + cam->name).c_str(), &cam->fix_up_vector);
    ImGui::Checkbox(("perspective##" + cam->name).c_str(), &cam->perspective);
    if (cam->perspective) {
        ImGui::DragFloat(("fov##" + cam->name).c_str(), &cam->fov_degree, 0.01f);
        ImGui::DragFloat(("near##" + cam->name).c_str(), &cam->near, 0.001f);
        ImGui::DragFloat(("far##" + cam->name).c_str(), &cam->far, 0.001f);
    } else {
        ImGui::DragFloat(("left##" + cam->name).c_str(), &cam->left, 0.001f);
        ImGui::DragFloat(("right##" + cam->name).c_str(), &cam->right, 0.001f);
        ImGui::DragFloat(("top##" + cam->name).c_str(), &cam->top, 0.001f);
        ImGui::DragFloat(("bottom##" + cam->name).c_str(), &cam->bottom, 0.001f);
    }
    if (ImGui::Button(("Make current##" + cam->name).c_str())) make_camera_current(cam);
    ImGui::Unindent();
}

void gui_display_texture(const Texture2D tex, const glm::ivec2& size) {
    ImGui::Indent();
    ImGui::Text("name: %s", tex->name.c_str());
    ImGui::Text("ID: %u, size: %ux%u", tex->id, tex->w, tex->h);
    ImGui::Text("internal_format: %u, format %u, type: %u", tex->internal_format, tex->format, tex->type);
    ImGui::Image((ImTextureID)tex->id, ImVec2(size.x, size.y), ImVec2(0, 1), ImVec2(1, 0), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
    if (ImGui::Button(("Save PNG##" + tex->name).c_str())) tex->save_png(fs::path(tex->name).replace_extension(".png"));
    ImGui::SameLine();
    if (ImGui::Button(("Save JPEG##" + tex->name).c_str())) tex->save_jpg(fs::path(tex->name).replace_extension(".jpg"));
    ImGui::Unindent();
}

void gui_display_shader(Shader& shader) {
    ImGui::Indent();
    ImGui::Text("name: %s", shader->name.c_str());
    ImGui::Text("ID: %u", shader->id);
    if (shader->source_files.count(GL_VERTEX_SHADER))
        ImGui::Text("vertex source: %s", shader->source_files[GL_VERTEX_SHADER].string().c_str());
    if (shader->source_files.count(GL_TESS_CONTROL_SHADER))
        ImGui::Text("tess_control source: %s", shader->source_files[GL_TESS_CONTROL_SHADER].string().c_str());
    if (shader->source_files.count(GL_TESS_EVALUATION_SHADER))
        ImGui::Text("tess_eval source: %s", shader->source_files[GL_TESS_EVALUATION_SHADER].string().c_str());
    if (shader->source_files.count(GL_GEOMETRY_SHADER))
        ImGui::Text("geometry source: %s", shader->source_files[GL_GEOMETRY_SHADER].string().c_str());
    if (shader->source_files.count(GL_FRAGMENT_SHADER))
        ImGui::Text("fragment source: %s", shader->source_files[GL_FRAGMENT_SHADER].string().c_str());
    if (shader->source_files.count(GL_COMPUTE_SHADER))
        ImGui::Text("compute source: %s", shader->source_files[GL_COMPUTE_SHADER].string().c_str());
    if (ImGui::Button("Compile"))
        shader->compile();
    ImGui::Unindent();
}

void gui_display_framebuffer(const Framebuffer& fbo) {
    ImGui::Indent();
    ImGui::Text("name: %s", fbo->name.c_str());
    ImGui::Text("ID: %u", fbo->id);
    ImGui::Text("size: %ux%u", fbo->w, fbo->h);
    if (ImGui::CollapsingHeader(("depth attachment##" + fbo->name).c_str()) && fbo->depth_texture)
        gui_display_texture(fbo->depth_texture);
    for (uint32_t i = 0; i < fbo->color_textures.size(); ++i)
        if (ImGui::CollapsingHeader(std::string("color attachment " + std::to_string(i) + "##" + fbo->name).c_str()))
            gui_display_texture(fbo->color_textures[i]);
    ImGui::Unindent();
}

void gui_display_material(const Material& mat) {
    ImGui::Indent();
    ImGui::Text("name: %s", mat->name.c_str());

    ImGui::Text("int params: %lu", mat->int_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->int_map)
        ImGui::Text("%s: %i", entry.first.c_str(), entry.second);
    ImGui::Unindent();

    ImGui::Text("float params: %lu", mat->float_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->float_map)
        ImGui::Text("%s: %f", entry.first.c_str(), entry.second);
    ImGui::Unindent();

    ImGui::Text("vec2 params: %lu", mat->vec2_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->vec2_map)
        ImGui::Text("%s: (%f, %f)", entry.first.c_str(), entry.second.x, entry.second.y);
    ImGui::Unindent();

    ImGui::Text("vec3 params: %lu", mat->vec3_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->vec3_map)
        ImGui::Text("%s: (%f, %f, %f)", entry.first.c_str(), entry.second.x, entry.second.y, entry.second.z);
    ImGui::Unindent();

    ImGui::Text("vec4 params: %lu", mat->vec4_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->vec4_map)
        ImGui::Text("%s: (%f, %f, %f, %.f)", entry.first.c_str(), entry.second.x, entry.second.y, entry.second.z, entry.second.w);
    ImGui::Unindent();

    ImGui::Text("textures: %lu", mat->texture_map.size());
    ImGui::Indent();
    for (const auto& entry : mat->texture_map) {
        ImGui::Text("%s:", entry.first.c_str());
        gui_display_texture(entry.second);
    }
    ImGui::Unindent();

    ImGui::Unindent();
}

void gui_display_geometry(const Geometry& geom) {
    ImGui::Indent();
    ImGui::Text("name: %s", geom->name.c_str());
    ImGui::Text("AABB min: (%.3f, %.3f, %.3f)", geom->bb_min.x, geom->bb_min.y, geom->bb_min.z);
    ImGui::Text("AABB max: (%.3f, %.3f, %.3f)", geom->bb_max.x, geom->bb_max.y, geom->bb_max.z);
    ImGui::Text("#Vertices: %lu", geom->positions.size());
    ImGui::Text("#Indices: %lu", geom->indices.size());
    ImGui::Text("#Normals: %lu", geom->normals.size());
    ImGui::Text("#Texcoords: %lu", geom->texcoords.size());
    ImGui::Unindent();
}

void gui_display_mesh(const Mesh& mesh) {
    ImGui::Indent();
    ImGui::Text("name: %s", mesh->name.c_str());
    if (ImGui::CollapsingHeader(("geometry: " + mesh->geometry->name).c_str()))
        gui_display_geometry(mesh->geometry);
    if (ImGui::CollapsingHeader(("material: " + mesh->material->name).c_str()))
        gui_display_material(mesh->material);
    ImGui::Unindent();
}

void gui_display_mat4(glm::mat4& mat) {
    ImGui::Indent();
    glm::mat4 row_maj = glm::transpose(mat);
    bool modified = false;
    if (ImGui::DragFloat4("row0", &row_maj[0][0], .01f)) modified = true;
    if (ImGui::DragFloat4("row1", &row_maj[1][0], .01f)) modified = true;
    if (ImGui::DragFloat4("row2", &row_maj[2][0], .01f)) modified = true;
    if (ImGui::DragFloat4("row3", &row_maj[3][0], .01f)) modified = true;
    if (modified) mat = glm::transpose(row_maj);
    ImGui::Unindent();
}

void gui_display_drawelement(Drawelement& elem) {
    ImGui::Indent();
    ImGui::Text("name: %s", elem->name.c_str());
    if (ImGui::CollapsingHeader("modelmatrix"))
        gui_display_mat4(elem->model);
    if (ImGui::CollapsingHeader(("shader: " + elem->shader->name).c_str()))
        gui_display_shader(elem->shader);
    if (ImGui::CollapsingHeader(("mesh: " + elem->mesh->name).c_str()))
        gui_display_mesh(elem->mesh);
    ImGui::Unindent();
}

void gui_display_animation(const Animation& anim) {
    ImGui::Indent();
    ImGui::Text("name: %s", anim->name.c_str());
    ImGui::Text("curr time: %.3f / %lu, ms_per_node: %.3f", anim->time, anim->camera_path.size(), anim->ms_between_nodes);
    ImGui::Text("camera path:");
    ImGui::Indent();
    for (const auto& [pos, rot] : anim->camera_path)
        ImGui::Text("pos: (%.3f, %.3f, %.3f), rot: (%.3f, %.3f, %.3f, %.3f)", pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, rot.w);
    ImGui::Unindent();
    ImGui::Unindent();
}

void gui_display_query_timer(const Query& query, const char* label) {
    const float avg = query.exp_avg;
    const float lower = query.min();
    const float upper = query.max();
    ImGui::Text("avg: %.1fms, min: %.1fms, max: %.1fms", avg, lower, upper);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(.7, .7, 0, 1));
    ImGui::PlotHistogram(label, query.data.data(), query.data.size(), query.curr, 0, 0.f, std::max(upper, 17.f), ImVec2(0, 30));
    ImGui::PopStyleColor();
}

void gui_display_query_counter(const Query& query, const char* label) {
    const float avg = query.exp_avg;
    const float lower = query.min();
    const float upper = query.max();
    ImGui::Text("avg: %uK, min: %uK, max: %uK", uint32_t(avg / 1000), uint32_t(lower / 1000), uint32_t(upper / 1000));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, .7, .7, 1));
    ImGui::PlotHistogram(label, query.data.data(), query.data.size(), query.curr, 0, 0.f, std::max(upper, 17.f), ImVec2(0, 30));
    ImGui::PopStyleColor();
}
