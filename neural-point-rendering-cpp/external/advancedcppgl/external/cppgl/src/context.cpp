#include "context.h"
#include "debug.h"
#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "framebuffer.h"
#include "material.h"
#include "geometry.h"
#include "drawelement.h"
#include "anim.h"
#include "query.h"
#include "stb_image_write.h"
#include "gui.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>

// -------------------------------------------
// helper funcs

static void glfw_error_func(int error, const char *description) {
    fprintf(stderr, "GLFW: Error %i: %s\n", error, description);
}

static bool show_gui = false;

static void (*user_keyboard_callback)(int key, int scancode, int action, int mods) = 0;
static void (*user_mouse_callback)(double xpos, double ypos) = 0;
static void (*user_mouse_button_callback)(int button, int action, int mods) = 0;
static void (*user_mouse_scroll_callback)(double xoffset, double yoffset) = 0;
static void (*user_resize_callback)(int w, int h) = 0;

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
        show_gui = !show_gui;
    if (ImGui::GetIO().WantCaptureKeyboard) {
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        return;
    }
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, 1);
    if (user_keyboard_callback)
        user_keyboard_callback(key, scancode, action, mods);
}

static void glfw_mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    if (user_mouse_callback)
        user_mouse_callback(xpos, ypos);
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) {
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        return;
    }
    if (user_mouse_button_callback)
        user_mouse_button_callback(button, action, mods);
}

static void glfw_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) {
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        return;
    }
    CameraImpl::default_camera_movement_speed += CameraImpl::default_camera_movement_speed * float(yoffset * 0.1);
    CameraImpl::default_camera_movement_speed = std::max(0.00001f, CameraImpl::default_camera_movement_speed);
    if (user_mouse_scroll_callback)
        user_mouse_scroll_callback(xoffset, yoffset);
}

static void glfw_resize_callback(GLFWwindow* window, int w, int h) {
    int width = w;// - (w % 32);
    int height = h;// - (h % 32);
    std::cout << "Window got resized -> Resize Context and FBOs" << std::endl;
    std::cout << "\t[glfw_resize_callback( " << w << ", " << h << ")] Resize to (" << width << ", " << height << ")" << std::endl;
//    Context::resize(width, height);
    if (user_resize_callback)
        user_resize_callback(width, height);
}

static void glfw_char_callback(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
}

// -------------------------------------------
// Context

static ContextParameters parameters;

Context::Context() {
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed!");
    glfwSetErrorCallback(glfw_error_func);

    // some GL context settings
    if (parameters.gl_major > 0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, parameters.gl_major);
    if (parameters.gl_minor > 0)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, parameters.gl_minor);
    glfwWindowHint(GLFW_RESIZABLE, parameters.resizable);
    glfwWindowHint(GLFW_VISIBLE, parameters.visible);
    glfwWindowHint(GLFW_DECORATED, parameters.decorated);
    glfwWindowHint(GLFW_FLOATING, parameters.floating);
    glfwWindowHint(GLFW_MAXIMIZED, parameters.maximised);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, parameters.gl_debug_context);

    // create window and context
    glfw_window = glfwCreateWindow(parameters.width, parameters.height, parameters.title.c_str(), 0, 0);
    if (!glfw_window) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateContext failed!");
    }
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(parameters.swap_interval);

    glewExperimental = GL_TRUE;
    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        glfwDestroyWindow(glfw_window);
        glfwTerminate();
        throw std::runtime_error(std::string("GLEWInit failed: ") + (const char*)glewGetErrorString(err));
    }

    // output configuration
    std::cout << "GLFW: " << glfwGetVersionString() << std::endl;
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << ", " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // enable debugging output
    enable_strack_trace_on_crash();
    enable_gl_debug_output();

    // setup user ptr
    glfwSetWindowUserPointer(glfw_window, this);

    // install callbacks
    glfwSetKeyCallback(glfw_window, glfw_key_callback);
    glfwSetCursorPosCallback(glfw_window, glfw_mouse_callback);
    glfwSetMouseButtonCallback(glfw_window, glfw_mouse_button_callback);
    glfwSetScrollCallback(glfw_window, glfw_mouse_scroll_callback);
    glfwSetFramebufferSizeCallback(glfw_window, glfw_resize_callback);
    glfwSetCharCallback(glfw_window, glfw_char_callback);

    // set input mode
    glfwSetInputMode(glfw_window, GLFW_STICKY_KEYS, 1);
    glfwSetInputMode(glfw_window, GLFW_STICKY_MOUSE_BUTTONS, 1);

    // init imgui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(glfw_window, false);
    ImGui_ImplOpenGL3_Init("#version 130");
    // ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // load custom font?
    if (fs::exists(parameters.font_ttf_filename)) {
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 3;
        std::cout << "Loading: " << parameters.font_ttf_filename << "..." << std::endl;
        ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF(
                parameters.font_ttf_filename.string().c_str(), float(parameters.font_size_pixels), &config);
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // set some sane GL defaults
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClearDepth(1);

    // setup timer
    last_t = curr_t = glfwGetTime();
    cpu_timer = TimerQuery("CPU-time");
    frame_timer = TimerQuery("Frame-time");
    gpu_timer = TimerQueryGL("GPU-time");
    prim_count = PrimitiveQueryGL("#Primitives");
    frag_count = FragmentQueryGL("#Fragments");
    cpu_timer->begin();
    frame_timer->begin();
    gpu_timer->begin();
    prim_count->begin();
    frag_count->begin();
}

Context::~Context() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
}

Context& Context::init(const ContextParameters& params) {
    parameters = params;
    return instance();
}

Context& Context::instance() {
    static Context ctx;
    return ctx;
}

bool Context::running() { return !glfwWindowShouldClose(instance().glfw_window); }

void Context::swap_buffers() {
    if (show_gui) gui_draw();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    instance().cpu_timer->end();
    instance().gpu_timer->end();
    instance().prim_count->end();
    instance().frag_count->end();
    glfwSwapBuffers(instance().glfw_window);
    instance().frame_timer->end();
    instance().frame_timer->begin();
    instance().cpu_timer->begin();
    instance().gpu_timer->begin();
    instance().prim_count->begin();
    instance().frag_count->begin();
    instance().last_t = instance().curr_t;
    instance().curr_t = glfwGetTime() * 1000; // s to ms
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

double Context::frame_time() { return instance().curr_t - instance().last_t; }

void Context::screenshot(const fs::path& path) {
    stbi_flip_vertically_on_write(1);
    const glm::ivec2 size = resolution();
    std::vector<uint8_t> pixels(size.x * size.y * 3);
    glReadPixels(0, 0, size.x, size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    // check file extension
    if (path.extension() == ".png") {
        stbi_write_png(path.string().c_str(), size.x, size.y, 3, pixels.data(), 0);
        std::cout << path << " written." << std::endl;
    } else if (path.extension() == ".jpg") {
        stbi_write_jpg(path.string().c_str(), size.x, size.y, 3, pixels.data(), 0);
        std::cout << path << " written." << std::endl;
    } else {
        std::cerr << "WARN: Context::screenshot: unknown extension, changing to .png!" << std::endl;
        stbi_write_png(fs::path(path).replace_extension(".png").string().c_str(), size.x, size.y, 3, pixels.data(), 0);
        std::cout << fs::path(path).replace_extension(".png") << " written." << std::endl;
    }
}


void Context::screenshotCustomResolution(const fs::path& path, const glm::ivec2 res) {
    stbi_flip_vertically_on_write(1);
    std::vector<uint8_t> pixels(res.x * res.y * 3);
    glReadPixels(0, 0, res.x, res.y, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    // check file extension
    if (path.extension() == ".png") {
        stbi_write_png(path.string().c_str(), res.x, res.y, 3, pixels.data(), 0);
        std::cout << path << " written." << std::endl;
    } else if (path.extension() == ".jpg") {
        stbi_write_jpg(path.string().c_str(), res.x, res.y, 3, pixels.data(), 0);
        std::cout << path << " written." << std::endl;
    } else {
        std::cerr << "WARN: Context::screenshot: unknown extension, changing to .png!" << std::endl;
        stbi_write_png(fs::path(path).replace_extension(".png").string().c_str(), res.x, res.y, 3, pixels.data(), 0);
        std::cout << fs::path(path).replace_extension(".png") << " written." << std::endl;
    }
}

void Context::screenshotCustomResolutionBinary(const fs::path& path, const glm::ivec2 res) {
    stbi_flip_vertically_on_write(1);

    std::vector<int> meta{ res.x,res.y,3,sizeof(float),0 };
    std::vector<float> pixels(res.x * res.y * 3);
    glReadPixels(0, 0, res.x, res.y, GL_RGB, GL_FLOAT, pixels.data()); // get RGB pixels as FLOAT
    // check file extension
    std::string path_str;
    if (path.extension() == ".bin") {
        path_str = path.string();
    } else {
        std::cerr << "WARN: Context::screenshot: unknown extension, adding .bin!" << std::endl;
        path_str = (path.string() + std::string(".bin")).c_str();
    }
    std::ofstream out_stream(path_str.c_str(), std::ios::binary);
    out_stream.write(reinterpret_cast<char*>(&meta[0]), sizeof(int) * meta.size()); // write out metadata as int

    //-------------------------------------------
    // Flip image vertically
    std::vector<float> pixels_flipped(res.x * res.y * 3);
    for (int row = res.y-1; row >= 0; --row) {
        for (int i = 0; i < res.x * 3; ++i) {
            pixels_flipped[(res.y - 1 - row) * res.x * 3 + i] = pixels[(row * res.x * 3 + i)];
        }
    }

    out_stream.write(reinterpret_cast<char*>(&pixels_flipped[0]), sizeof(float) * pixels_flipped.size()); // write out pixels as floats
    std::cout << path_str.c_str() << " written." << std::endl;
}

void Context::show() { glfwShowWindow(instance().glfw_window); }

void Context::hide() { glfwHideWindow(instance().glfw_window); }

glm::ivec2 Context::resolution() {
    int w, h;
    glfwGetFramebufferSize(instance().glfw_window, &w, &h);
    return glm::ivec2(w, h);
}

void Context::resize(int w, int h) {
    /*int width = w;
    if (width % 16 != 0) width += (16 - width % 16);
    int height = h;
    if (height % 16 != 0) height += (16 - height % 16);*/
    int width = w;// - (w % 32);
    int height = h;// - (h % 32);
    std::cout << "\t[Context::resize( " << w << ", " << h << ")] Resize to (" << width << ", " << height  << ")" << std::endl;
    glfwSetWindowSize(instance().glfw_window, width, height);
    glViewport(0, 0, width, height);
}

void Context::set_title(const std::string& name) { glfwSetWindowTitle(instance().glfw_window, name.c_str()); }

void Context::set_swap_interval(uint32_t interval) { glfwSwapInterval(interval); }

void Context::capture_mouse(bool on) { glfwSetInputMode(instance().glfw_window, GLFW_CURSOR, on ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); }

glm::vec2 Context::mouse_pos() {
    double xpos, ypos;
    glfwGetCursorPos(instance().glfw_window, &xpos, &ypos);
    return glm::vec2(xpos, ypos);
}

bool Context::mouse_button_pressed(int button) { return glfwGetMouseButton(instance().glfw_window, button) == GLFW_PRESS; }

bool Context::key_pressed(int key) { return glfwGetKey(instance().glfw_window, key) == GLFW_PRESS; }

void Context::set_keyboard_callback(void (*fn)(int key, int scancode, int action, int mods)) { user_keyboard_callback = fn; }

void Context::set_mouse_callback(void (*fn)(double xpos, double ypos)) { user_mouse_callback = fn; }

void Context::set_mouse_button_callback(void (*fn)(int button, int action, int mods)) { user_mouse_button_callback = fn; }

void Context::set_mouse_scroll_callback(void (*fn)(double xoffset, double yoffset)) { user_mouse_scroll_callback = fn; }

void Context::set_resize_callback(void (*fn)(int w, int h)) { user_resize_callback = fn; }
