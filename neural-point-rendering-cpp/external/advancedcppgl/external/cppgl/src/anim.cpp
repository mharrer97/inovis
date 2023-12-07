#include "anim.h"
#include "camera.h"
#include <algorithm>

static Animation current_anim;

Animation current_animation() {
    static Animation default_anim("default");
    return current_anim ? current_anim : default_anim;
}

void make_animation_current(const Animation& anim) {
    current_anim = anim;
}

// -------------------------------------------
// spline (centripedal catmull rom) for float types, i.e. float, glm::vec2, glm::vec3, glm::vec4

template <typename T> struct CubicPolynomial {
    void init(const T& x0, const T& x1, const T& t0, const T& t1) {
        c0 = x0;
        c1 = t0;
        c2 = -3.f*x0 + 3.f*x1 - 2.f*t0 - t1;
        c3 = 2.f*x0 - 2.f*x1 + t0 + t1;
    }

    T eval(float t) const {
        const float t2 = t*t;
        const float t3 = t2*t;
        return c0 + c1*t + c2*t2 + c3*t3;
    }

    // data
    T c0, c1, c2, c3;
};

template <typename T> inline float dist_sqr(const T& x, const T& y) {
    const T d = x - y;
    return glm::dot(d, d);
}

template <typename T> struct CentripedalCR : public CubicPolynomial<T> {
    CentripedalCR(const T& p0, const T& p1, const T& p2, const T& p3) {
        float dt0 = powf(dist_sqr(p0, p1), 0.25f);
        float dt1 = powf(dist_sqr(p1, p2), 0.25f);
        float dt2 = powf(dist_sqr(p2, p3), 0.25f);
        // safety check for repeated points
        if (dt1 < 1e-4f)    dt1 = 1.0f;
        if (dt0 < 1e-4f)    dt0 = dt1;
        if (dt2 < 1e-4f)    dt2 = dt1;
        // compute tangents when parameterized in [t1,t2]
        T t1 = (p1 - p0) / dt0 - (p2 - p0) / (dt0 + dt1) + (p2 - p1) / dt1;
        T t2 = (p2 - p1) / dt1 - (p3 - p1) / (dt1 + dt2) + (p3 - p2) / dt2;
        // rescale tangents for parametrization in [0,1]
        t1 *= dt1;
        t2 *= dt1;
        // to cubic polynomial
        CubicPolynomial<T>::init(p1, p2, t1, t2);
    }
};

// -------------------------------------------
// Animation

AnimationImpl::AnimationImpl(const std::string& name) : name(name), time(0), ms_between_nodes(1000), running(false) {}

AnimationImpl::~AnimationImpl() {}

void AnimationImpl::update(float dt_ms) {
    if (running && time < camera_path.size() && !camera_path.empty()) {
        // update camera and advance time
        current_camera()->load(eval_pos(), eval_rot());
        time = glm::max(glm::min(time + dt_ms / ms_between_nodes, float(camera_path.size())), 0.f);
    } else
        running = false;
}

void AnimationImpl::clear() {
    camera_path.clear();
    data_int.clear();
    data_float.clear();
    data_vec2.clear();
    data_vec3.clear();
    data_vec4.clear();
}

size_t AnimationImpl::length() const {
    return camera_path.size();
}

void AnimationImpl::play() {
    time = 0;
    running = true;
}

void AnimationImpl::pause() {
    running = false;
}

void AnimationImpl::toggle_pause() {
    running = !running;
}

void AnimationImpl::stop() {
    time = 0;
    running = false;
}

void AnimationImpl::reset() {
    time = 0;
}

void AnimationImpl::print_path() {
    for(auto& node : camera_path) {
        std::cerr << "animation->push_node(vec3(" << node.first.x << ", " << node.first.y << ", " << node.first.z << "), glm::quat(" << node.second.w  << " , {" << node.second.x << ", " << node.second.y << ", " << node.second.z << "}));" << std::endl;
    }
}

size_t AnimationImpl::push_node(const glm::vec3& pos, const glm::quat& rot) {
    const size_t i = camera_path.size();
    camera_path.push_back(std::make_pair(pos, rot));
    return i;
}

void AnimationImpl::put_node(size_t i, const glm::vec3& pos, const glm::quat& rot) {
    if (i < camera_path.size())
        camera_path[i] = std::make_pair(pos, rot);
}

void AnimationImpl::put_data(const std::string& name, size_t i, int val) {
    if (data_int[name].size() <= i)
        data_int[name].resize(i + 1);
    data_int[name][i] = val;
}

void AnimationImpl::put_data(const std::string& name, size_t i, float val) {
    if (data_float[name].size() <= i)
        data_float[name].resize(i + 1);
    data_float[name][i] = val;
}

void AnimationImpl::put_data(const std::string& name, size_t i, const glm::vec2& val) {
    if (data_vec2[name].size() <= i)
        data_vec2[name].resize(i + 1);
    data_vec2[name][i] = val;
}

void AnimationImpl::put_data(const std::string& name, size_t i, const glm::vec3& val) {
    if (data_vec3[name].size() <= i)
        data_vec3[name].resize(i + 1);
    data_vec3[name][i] = val;
}

void AnimationImpl::put_data(const std::string& name, size_t i, const glm::vec4& val) {
    if (data_vec4[name].size() <= i)
        data_vec4[name].resize(i + 1);
    data_vec4[name][i] = val;
}

glm::vec3 AnimationImpl::eval_pos() const {
    // eval centripedal catmull rom spline
    const size_t at = size_t(glm::floor(time));
    // prevent underflow
    const size_t prev_at = size_t(std::max(int(at) - 1, 0));
    const auto& p0 = camera_path[std::max(prev_at, size_t(0))].first;
    const auto& p1 = camera_path[std::min(at + 0, camera_path.size()) % camera_path.size()].first;
    const auto& p2 = camera_path[std::min(at + 1, camera_path.size()) % camera_path.size()].first;
    const auto& p3 = camera_path[std::min(at + 2, camera_path.size()) % camera_path.size()].first;
    return CentripedalCR(p0, p1, p2, p3).eval(glm::fract(time));
}

glm::quat AnimationImpl::eval_rot() const {
    const size_t at = size_t(glm::floor(time));
    const auto& lower = camera_path[std::min(at, camera_path.size()) % camera_path.size()].second;
    const auto& upper = camera_path[std::min(at + 1, camera_path.size()) % camera_path.size()].second;
    return glm::slerp(lower, upper, time - at);
}

int AnimationImpl::eval_int(const std::string& name) const {
    const size_t i = std::min(size_t(glm::floor(time)), data_int.at(name).size() - 1);
    return data_int.at(name).at(i); // no lerp necessary
}

// lerp helper
template <typename T> inline T lerp_data(const std::vector<T>& data, float time) {
    const size_t at = size_t(glm::floor(time));
    const auto& lower = data[std::min(at, data.size()) % data.size()];
    const auto& upper = data[std::min(at + 1, data.size()) % data.size()];
    return glm::mix(lower, upper, time - at);
}

float AnimationImpl::eval_float(const std::string& name) const {
    return lerp_data<float>(data_float.at(name), time);
}

glm::vec2 AnimationImpl::eval_vec2(const std::string& name) const {
    return lerp_data<glm::vec2>(data_vec2.at(name), time);
}

glm::vec3 AnimationImpl::eval_vec3(const std::string& name) const {
    return lerp_data<glm::vec3>(data_vec3.at(name), time);
}

glm::vec4 AnimationImpl::eval_vec4(const std::string& name) const {
    return lerp_data<glm::vec4>(data_vec4.at(name), time);
}
