#version 330
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in float in_curvature;
layout (location = 4) in int in_timestamp;
//layout (location = 2) in vec2 in_tc;
//layout (location = 3) in vec2 in_a;

//uniform mat4 model;
uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;
// old view for motion vecs
uniform mat4 view_old_1;
uniform mat4 view_old_2;
uniform mat4 view_old_3;
uniform mat4 view_old_4;
uniform mat4 view_old_5;
uniform mat4 view_old_6;

uniform mat4 proj_old;

uniform int gt_timestamp; 

uniform bool use_taa;

out vec4 pos_wc;
out vec3 col_wc;
out vec3 norm_wc;
out float curvature;
out vec4 pos_proj;
out vec4 pos_old_1;
out vec4 pos_old_2;
out vec4 pos_old_3;
out vec4 pos_old_4;
out vec4 pos_old_5;
out vec4 pos_old_6;
flat out int timestamp;

//out vec2 tc;

void main() {
    pos_wc = vec4(in_pos, 1.0);
    col_wc = in_color;
    norm_wc = mat3(view)*in_normal;//normalize(mat3(view_normal) * in_normal);
    curvature = in_curvature;
    //tc = in_tc;
    pos_proj = proj * view * pos_wc;
    gl_Position = pos_proj;
    if (use_taa) 
        pos_old_1 = proj * view_old_1 * pos_wc; 
    else
        pos_old_1 = proj_old * view_old_1 * pos_wc; 
    pos_old_2 = proj_old * view_old_2 * pos_wc; 
    pos_old_3 = proj_old * view_old_3 * pos_wc; 
    pos_old_4 = proj_old * view_old_4 * pos_wc; 
    pos_old_5 = proj_old * view_old_5 * pos_wc; 
    pos_old_6 = proj_old * view_old_6 * pos_wc;  

    timestamp = in_timestamp;
}
