#version 330
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in float in_curvature;
//layout (location = 2) in vec2 in_tc;
//layout (location = 3) in vec2 in_a;

//uniform mat4 model;
uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;
// old view for motion vecs
uniform mat4 view_old;

out vec4 pos_wc;
out vec3 col_wc;
out vec3 norm_wc;
out float curvature;
out vec4 pos_proj;
out vec4 pos_old;

//out vec2 tc;

void main() {
    pos_wc = vec4(in_pos, 1.0);
    col_wc = in_color;
    norm_wc = mat3(view)*in_normal;//normalize(mat3(view_normal) * in_normal);
    curvature = in_curvature;
    //tc = in_tc;
    pos_proj = proj * view * pos_wc;
    gl_Position = pos_proj;
    pos_old = proj * view_old * pos_wc; 
}
