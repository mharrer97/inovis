#include "debug.h"

#include <GL/glew.h>
#include <GL/gl.h>

// -----------------------------------------------------------
// GL debug output

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    // get source of error
    std::string src;
    switch (source){
    case GL_DEBUG_SOURCE_API:
        src = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        src = "WINDOW_SYSTEM"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        src = "SHADER_COMPILER"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        src = "THIRD_PARTY"; break;
    case GL_DEBUG_SOURCE_APPLICATION:
        src = "APPLICATION"; break;
    case GL_DEBUG_SOURCE_OTHER:
        src = "OTHER"; break;
    }
    // get type of error
    std::string typ;
    switch (type){
    case GL_DEBUG_TYPE_ERROR:
        typ = "ERROR"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typ = "DEPRECATED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typ = "UNDEFINED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typ = "PORTABILITY"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typ = "PERFORMANCE"; break;
    case GL_DEBUG_TYPE_OTHER:
        typ = "OTHER"; break;
    case GL_DEBUG_TYPE_MARKER:
        typ = "MARKER"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typ = "PUSH_GROUP"; break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typ = "POP_GROUP"; break;
    }
    // get severity
    std::string sev;
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        sev = "NOTIFICATION"; break;
    case GL_DEBUG_SEVERITY_LOW:
        sev = "LOW"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        sev = "MEDIUM"; break;
    case GL_DEBUG_SEVERITY_HIGH:
        sev = "HIGH"; break;
    }
    fprintf(stderr, "GL_DEBUG: Severity: %s, Source: %s, Type: %s.\nMessage: %s\n", sev.c_str(), src.c_str(), typ.c_str(), message);
}

void enable_gl_debug_output() {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debugCallback, 0);
    disable_gl_notifications();
}

void disable_gl_debug_output() {
    glDisable(GL_DEBUG_OUTPUT);
}

void enable_gl_notifications() {
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_TRUE);
}

void disable_gl_notifications() {
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
}


#ifdef _WIN32

void enable_strack_trace_on_crash(){ std::cerr <<"STACK TRACE NOT IMPLEMENTED ON WINDOWS" << std::endl; }
void disable_stack_trace_on_crash() { }

#else

#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>
// -----------------------------------------------------------
// Demangle and print current stack frame

using namespace std;

void print_stack_trace(FILE* out, unsigned int offset);

void signal_handler(int signum) {
    const char* name = NULL;
    switch (signum) {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV: name = "SIGSEGV";  break;
    case SIGBUS:  name = "SIGBUS";   break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    }
    if (name)
        fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
    else
        fprintf(stderr, "Caught signal %d\n", signum);
    print_stack_trace(stderr, 4);
    exit(signum);
}

void enable_strack_trace_on_crash() {
    signal(SIGABRT, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGFPE, signal_handler);
}

void disable_strack_trace_on_crash() {
    signal(SIGABRT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
}


#define MAX_FRAMES 63
void print_stack_trace(FILE *out, unsigned int offset) {
    fprintf(out, "Stack trace:\n");
    // storage array for stack trace address data
    void* addrlist[MAX_FRAMES+1];
    // retrieve current stack addresses
    unsigned int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
    if (addrlen == 0) {
        fprintf(out, "  \n");
        return;
    }
    // resolve addresses into strings containing "filename(function+address)",
    // Actually it will be ## program address function + offset
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);
    size_t funcnamesize = 1024;
    char funcname[1024];
    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (unsigned int i = offset; i < addrlen; i++) {
        char* begin_name   = NULL;
        char* begin_offset = NULL;
        char* end_offset   = NULL;
        // find parentheses and +address offset surrounding the mangled name
#ifdef DARWIN
        // OSX style stack trace
        for (char *p = symbollist[i]; *p; ++p) {
            if (( *p == '_' ) && ( *(p-1) == ' ' ))
                begin_name = p-1;
            else if ( *p == '+' )
                begin_offset = p-1;
        }
        if (begin_name && begin_offset && ( begin_name < begin_offset)) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply __cxa_demangle():
            int status;
            char* ret = abi::__cxa_demangle( begin_name, &funcname[0], &funcnamesize, &status );
            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string
                fprintf(out, "  %-30s %-40s %s\n", symbollist[i], funcname, begin_offset);
            } else {
                // demangling failed. Output function name as a C function with no arguments.
                fprintf(out, "  %-30s %-38s() %s\n", symbollist[i], begin_name, begin_offset);
            }
#else // !DARWIN - but is posix
        // not OSX style
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p) {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && (begin_offset || begin_name))
                end_offset = p;
        }
        if (begin_name && end_offset && (begin_name && end_offset)) {
            *begin_name++   = '\0';
            *end_offset++   = '\0';
            if (begin_offset)
                *begin_offset++ = '\0';
            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply __cxa_demangle():
            int status = 0;
            char* ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
            char* fname = begin_name;
            if (status == 0)
                fname = ret;
            if (begin_offset)
                fprintf(out, "  %-30s ( %-40s	+ %-6s)	%s\n", symbollist[i], fname, begin_offset, end_offset);
            else
                fprintf(out, "  %-30s ( %-40s	  %-6s)	%s\n", symbollist[i], fname, "", end_offset);
#endif  // !DARWIN - but is posix
        } else {
            // couldn't parse the line? print the whole line.
            fprintf(out, "  %-40s\n", symbollist[i]);
        }
    }
    fprintf(out, "\n");
    free(symbollist);
}

#endif