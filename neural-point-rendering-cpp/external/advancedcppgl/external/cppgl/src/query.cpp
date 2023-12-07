#include "query.h"

// -------------------------------------------------------
// (CPU) TimerQuery (in ms)

TimerQueryImpl::TimerQueryImpl(const std::string& name, size_t samples) : Query(name, samples) {}

TimerQueryImpl::~TimerQueryImpl() {}

void TimerQueryImpl::begin() {
    timer.begin();
}

void TimerQueryImpl::end() {
    put(float(timer.look()));
}

// -------------------------------------------------------
// (GPU) TimerQueryGL (in ms)

TimerQueryGLImpl::TimerQueryGLImpl(const std::string& name, size_t samples) : Query(name, samples), start_time(0), stop_time(0){
    glGenQueries(2, query_ids[0]);
    glGenQueries(2, query_ids[1]);
    glQueryCounter(query_ids[1][0], GL_TIMESTAMP);
    glQueryCounter(query_ids[1][1], GL_TIMESTAMP);
}

TimerQueryGLImpl::~TimerQueryGLImpl() {
    glDeleteQueries(2, query_ids[0]);
    glDeleteQueries(2, query_ids[1]);
}

void TimerQueryGLImpl::begin() {
    glQueryCounter(query_ids[0][0], GL_TIMESTAMP);
}

void TimerQueryGLImpl::end() {
    glQueryCounter(query_ids[0][1], GL_TIMESTAMP);
    std::swap(query_ids[0], query_ids[1]); // switch front/back buffer
    glGetQueryObjectui64v(query_ids[0][0], GL_QUERY_RESULT, &start_time);
    glGetQueryObjectui64v(query_ids[0][1], GL_QUERY_RESULT, &stop_time);
    put(float((stop_time - start_time) / 1000000.0));
}

// -------------------------------------------------------
// (GPU) PrimitiveQueryGL

PrimitiveQueryGLImpl::PrimitiveQueryGLImpl(const std::string& name, size_t samples) : Query(name, samples), start_time(0), stop_time(0) {
    glGenQueries(2, query_ids);
    // avoid error on first run
    glBeginQuery(GL_PRIMITIVES_GENERATED, query_ids[0]);
    glEndQuery(GL_PRIMITIVES_GENERATED);
    glBeginQuery(GL_PRIMITIVES_GENERATED, query_ids[1]);
    glEndQuery(GL_PRIMITIVES_GENERATED);
}

PrimitiveQueryGLImpl::~PrimitiveQueryGLImpl() {
    glDeleteQueries(2, query_ids);
}

void PrimitiveQueryGLImpl::begin() {
    glBeginQuery(GL_PRIMITIVES_GENERATED, query_ids[0]);
}

void PrimitiveQueryGLImpl::end() {
    glEndQuery(GL_PRIMITIVES_GENERATED);
    std::swap(query_ids[0], query_ids[1]); // switch front/back buffer
    GLuint result;
    glGetQueryObjectuiv(query_ids[0], GL_QUERY_RESULT, &result);
    put(float(result));
}

// -------------------------------------------------------
// (GPU) FragmentQueryGL

FragmentQueryGLImpl::FragmentQueryGLImpl(const std::string& name, size_t samples) : Query(name, samples), start_time(0), stop_time(0) {
    glGenQueries(2, query_ids);
    // avoid error on first run
    glBeginQuery(GL_SAMPLES_PASSED, query_ids[0]);
    glEndQuery(GL_SAMPLES_PASSED);
    glBeginQuery(GL_SAMPLES_PASSED, query_ids[1]);
    glEndQuery(GL_SAMPLES_PASSED);
}

FragmentQueryGLImpl::~FragmentQueryGLImpl() {
    glDeleteQueries(2, query_ids);
}

void FragmentQueryGLImpl::begin() {
    glBeginQuery(GL_SAMPLES_PASSED, query_ids[0]);
}

void FragmentQueryGLImpl::end() {
    glEndQuery(GL_SAMPLES_PASSED);
    std::swap(query_ids[0], query_ids[1]); // switch front/back buffer
    GLuint result;
    glGetQueryObjectuiv(query_ids[0], GL_QUERY_RESULT, &result);
    put(float(result));
}
