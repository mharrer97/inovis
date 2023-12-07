#include "shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

// ----------------------------------------------------
// helper funcs

static std::string read_file(const fs::path& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return std::string();
}

static std::string get_log(GLuint object) {
    std::string error_string;
    GLint log_length = 0;
    if (glIsShader(object)) {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else if (glIsProgram(object)) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    } else {
        error_string += "Not a shader or a program";
        return error_string;
    }
    if (log_length <= 1)
        // ignore empty string
        return error_string;
    char *log = (char *)malloc(log_length);
    if (glIsShader(object))
        glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, log_length, NULL, log);
    error_string += log;
    free(log);
    return error_string;
}

static GLuint compile_shader(GLenum type, std::map<GLenum, fs::path>& source_files, std::map<GLenum, fs::file_time_type>& timestamps) {
    std::cout << "Loading: " << source_files[type] << "..." << std::endl;
    std::string source = read_file(source_files[type]);
    timestamps[type] = fs::last_write_time(source_files[type]);
    if (source.empty())
        throw std::runtime_error("ERROR: Trying to compile shader from empty source!");

    // handle single level of #include
    std::string::size_type inc_at;
    while ((inc_at = source.find("#include")) != std::string::npos) {
        auto inc_to = source.find("\n", inc_at);
        std::string inc_str = source.substr(inc_at, inc_to - inc_at);
        std::string inc_file;
        if (inc_str.find("\"") != std::string::npos) {
            // uses ""
            auto first = inc_str.find("\"") + 1;
            auto second = inc_str.find("\"", first);
            inc_file = inc_str.substr(first, second - first);
        } else if (inc_str.find("<") != std::string::npos) {
            // uses <>
            auto first = inc_str.find("<") + 1;
            auto second = inc_str.find(">", first);
            inc_file = inc_str.substr(first, second - first);
        } else
            throw std::runtime_error("ERROR: Failed to parse #include string: " + inc_str);

        // read #include-file
        fs::path p = source_files[type];
        p = p.remove_filename() / inc_file;
        std::ifstream f(p.c_str(), std::ios::in);
        if (f.is_open()) {
            std::string inc_src, line;
            while (getline(f, line))
                inc_src += line + "\n";
            // replace #include with file
            source.replace(inc_at, inc_to - inc_at, inc_src);
        } else
            throw std::runtime_error("ERROR: Failed to open #include file: " + inc_str);
    }

    // actually compile shader
    GLuint shader = glCreateShader(type);
    const char *src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    // print error msg if failed
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (shaderCompiled != GL_TRUE) {
        std::string log = get_log(shader);
        std::string error_msg = "ERROR: Failed to compile shader: " + source_files[type].string() + ".\n" + log + "\nSource:\n";
        // get relevant lines
        std::string out;
        std::stringstream logstream(log);
        std::vector<int> lines;
        while (!logstream.eof()) {
            getline(logstream, out);
            try {
                int line = stoi(out.substr(2, out.find(":") - 3));
                lines.push_back(line);
            }
            catch (const std::exception& e) { (void) e; }
        }
        // print relevant lines
        std::stringstream stream(source);
        int line = 1;
        while (!stream.eof()) {
            getline(stream, out);
            if (std::find(lines.begin(), lines.end(), line) != lines.end())
                error_msg += "(" + std::to_string(line) + ")\t" + out + "\n";
            line++;
        }
        glDeleteShader(shader);
        std::cerr << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
    return shader;
}

void reload_modified_shaders() {
    for (auto& pair : Shader::map)
        pair.second->reload_if_modified();
}

// ----------------------------------------------------
// ShaderImpl

ShaderImpl::ShaderImpl(const std::string& name) : name(name), id(0) {}

ShaderImpl::ShaderImpl(const std::string& name, const fs::path& compute_source) : name(name), id(0) {
    set_compute_source(compute_source);
    compile();
}

ShaderImpl::ShaderImpl(const std::string& name, const fs::path& vertex_source, const fs::path& fragment_source) : name(name), id(0)  {
    set_vertex_source(vertex_source);
    set_fragment_source(fragment_source);
    compile();
}

ShaderImpl::ShaderImpl(const std::string& name, const fs::path& vertex_source, const fs::path& geometry_source, const fs::path& fragment_source) : name(name), id(0)  {
    set_vertex_source(vertex_source);
    set_geometry_source(geometry_source);
    set_fragment_source(fragment_source);
    compile();
}

ShaderImpl::~ShaderImpl() {
    clear();
}

void ShaderImpl::clear() {
    if (glIsProgram(id))
        glDeleteProgram(id);
    id = 0;
    source_files.clear();
    timestamps.clear();
}

void ShaderImpl::bind() const { glUseProgram(id); }

void ShaderImpl::unbind() const { glUseProgram(0); }

void ShaderImpl::set_source(GLenum type, const fs::path& path) {
    source_files[type] = path;
}

void ShaderImpl::set_vertex_source(const fs::path& path) {
    set_source(GL_VERTEX_SHADER, path);
}

void ShaderImpl::set_tesselation_control_source(const fs::path& path) {
    set_source(GL_TESS_CONTROL_SHADER, path);
}

void ShaderImpl::set_tesselation_evaluation_source(const fs::path& path) {
    set_source(GL_TESS_EVALUATION_SHADER, path);
}

void ShaderImpl::set_geometry_source(const fs::path& path) {
    set_source(GL_GEOMETRY_SHADER, path);
}

void ShaderImpl::set_fragment_source(const fs::path& path) {
    set_source(GL_FRAGMENT_SHADER, path);
}

void ShaderImpl::set_compute_source(const fs::path& path) {
    set_source(GL_COMPUTE_SHADER, path);
}

void ShaderImpl::compile() {
    // compile shaders
    GLuint program = glCreateProgram();
    if (source_files.count(GL_COMPUTE_SHADER)) { // is compute shader
        GLuint shader = compile_shader(GL_COMPUTE_SHADER, source_files, timestamps);
        if (!shader) {
            glDeleteProgram(program);
            return;
        }
        glAttachShader(program, shader);
    } else { // is pipeline
        if (source_files.count(GL_VERTEX_SHADER)) {
            GLuint shader = compile_shader(GL_VERTEX_SHADER, source_files, timestamps);
            if (!shader) {
                glDeleteProgram(program);
                return;
            }
            glAttachShader(program, shader);
        }
        if (source_files.count(GL_TESS_CONTROL_SHADER)) {
            GLuint shader = compile_shader(GL_TESS_CONTROL_SHADER, source_files, timestamps);
            if (!shader) {
                glDeleteProgram(program);
                return;
            }
            glAttachShader(program, shader);
        }
        if (source_files.count(GL_TESS_EVALUATION_SHADER)) {
            GLuint shader = compile_shader(GL_TESS_EVALUATION_SHADER, source_files, timestamps);
            if (!shader) {
                glDeleteProgram(program);
                return;
            }
            glAttachShader(program, shader);
        }
        if (source_files.count(GL_GEOMETRY_SHADER)) {
            GLuint shader = compile_shader(GL_GEOMETRY_SHADER, source_files, timestamps);
            if (!shader) {
                glDeleteProgram(program);
                return;
            }
            glAttachShader(program, shader);
        }
        if (source_files.count(GL_FRAGMENT_SHADER)) {
            GLuint shader = compile_shader(GL_FRAGMENT_SHADER, source_files, timestamps);
            if (!shader) {
                glDeleteProgram(program);
                return;
            }
            glAttachShader(program, shader);
        }
    }
    // link program
    glLinkProgram(program);
    GLint link_ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (link_ok != GL_TRUE) {
        std::string error_msg = "ERROR: Failed to link shader from sources:\n";
        for (const auto& entry : source_files)
            error_msg += entry.second.string() + "\n";
        error_msg += "Log: " + get_log(program) + "\n";
        glDeleteProgram(program);
        std::cerr << error_msg << std::endl;
        throw std::runtime_error("Shader compilation failed, see full output in std::cerr");
    }
    // success, set new id
    if (glIsProgram(id))
        glDeleteProgram(id);
    id = program;
}

void ShaderImpl::dispatch_compute(uint32_t w, uint32_t h, uint32_t d) const {
    glm::ivec3 size;
    glGetProgramiv(id, GL_COMPUTE_WORK_GROUP_SIZE, &size.x);
    glDispatchCompute(int(ceil(w / float(size.x))), int(ceil(h / float(size.y))), int(ceil(d / float(size.z))));
}

void ShaderImpl::uniform(const std::string& name, int val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform1i(loc, val);
}

void ShaderImpl::uniform(const std::string& name, int *val, uint32_t count) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform1iv(loc, count, val);
}

void ShaderImpl::uniform(const std::string& name, float val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform1f(loc, val);
}

void ShaderImpl::uniform(const std::string& name, float *val, uint32_t count) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform1fv(loc, count, val);
}

void ShaderImpl::uniform(const std::string& name, const glm::vec2& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform2f(loc, val.x, val.y);
}

void ShaderImpl::uniform(const std::string& name, const glm::vec3& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform3f(loc, val.x, val.y, val.z);
}

void ShaderImpl::uniform(const std::string& name, const glm::vec4& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform4f(loc, val.x, val.y, val.z, val.w);
}

void ShaderImpl::uniform(const std::string& name, const glm::ivec2& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform2i(loc, val.x, val.y);
}

void ShaderImpl::uniform(const std::string& name, const glm::ivec3& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform3i(loc, val.x, val.y, val.z);
}

void ShaderImpl::uniform(const std::string& name, const glm::ivec4& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform4i(loc, val.x, val.y, val.z, val.w);
}

void ShaderImpl::uniform(const std::string& name, const glm::uvec2& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform2ui(loc, val.x, val.y);
}

void ShaderImpl::uniform(const std::string& name, const glm::uvec3& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform3ui(loc, val.x, val.y, val.z);
}

void ShaderImpl::uniform(const std::string& name, const glm::uvec4& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniform4ui(loc, val.x, val.y, val.z, val.w);
}

void ShaderImpl::uniform(const std::string& name, const glm::mat3& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderImpl::uniform(const std::string& name, const glm::mat4& val) const {
    int loc = glGetUniformLocation(id, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderImpl::uniform(const std::string& name, const Texture2D tex, uint32_t unit) const {
    int loc = glGetUniformLocation(id, name.c_str());
    tex->bind(unit);
    glUniform1i(loc, unit);
}

void ShaderImpl::uniform(const std::string& name, const Texture3D& tex, uint32_t unit) const {
    int loc = glGetUniformLocation(id, name.c_str());
    tex->bind(unit);
    glUniform1i(loc, unit);
}

void ShaderImpl::reload_if_modified() {
    for (const auto& entry : source_files) {
        try {
            if (fs::last_write_time(entry.second) != timestamps[entry.first]) {
                compile();
                return;
            }
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
