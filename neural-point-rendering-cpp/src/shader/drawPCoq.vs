#version 430
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

out vec4 pos_vs;
out vec3 col_vs;
out vec3 norm_vs;
out float curvature_vs;
//out vec2 tc;

void main() {
    pos_vs = vec4(in_pos, 1.0);
    col_vs = in_color;
    norm_vs = normalize(mat3(view) * in_normal);
    curvature_vs = in_curvature;
    //tc = in_tc;
    gl_Position = proj * view * pos_vs;
}
