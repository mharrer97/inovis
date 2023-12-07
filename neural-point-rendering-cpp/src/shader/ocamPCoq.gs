#version 430

#include "__ocam_model_include.glsl"

layout(points) in;
layout(triangle_strip, max_vertices=4) out;
//uniform mat4 model;


in vec4 pos_vs[1];
in vec3 col_vs[1];
in vec3 norm_vs[1];
in float curvature_vs[1];

uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;
uniform float point_size;
uniform float aabb_extend;

layout(std430, binding = 0) buffer Ocam
{
	ocam_model ocam_mod;
};


out vec4 pos_fs;
out vec3 col_fs;
out vec3 norm_fs;
out float curvature_fs;
out vec4 pos_view;
//out vec4 center_fs;
out vec2 tc_fs;

void main() {
    pos_fs = pos_vs[0];
    col_fs = col_vs[0];
    norm_fs = norm_vs[0]; 
    curvature_fs = curvature_vs[0];  
    //center_fs = view * pos_vs[0];
    vec4 center = pos_vs[0];
    vec3 orth_up = normalize(cross(norm_fs,vec3(1,1,1)));
    vec4 orth_up4 = vec4(orth_up,0);
    vec3 orth_left = -normalize(cross(norm_fs,orth_up));
    vec4 orth_left4 = vec4(orth_left,0);
    norm_fs = mat3(view)*norm_vs[0]; // sewt ouptut normal to view space
    tc_fs = vec2(1,1);
    pos_fs = center + orth_left4 * point_size + orth_up4 * point_size;
    vec4 ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex();

    //col_fs = col_vs[0];
    //norm_fs = norm_vs[0]; 
    //curvature_fs = curvature_vs[0]; 
    tc_fs = vec2(-1,1);
    pos_fs = center + orth_left4 * (-point_size) + orth_up4 * point_size;
    ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex();

    //col_fs = col_vs[0];
    //norm_fs = norm_vs[0]; 
    //curvature_fs = curvature_vs[0]; 
    tc_fs = vec2(1,-1);
    pos_fs = center + orth_left4 * point_size + orth_up4 * (-point_size);
    ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex();
    
    /* EndPrimitive(); 

    col_fs = col_vs[0];
    norm_fs = norm_vs[0]; 
    curvature_fs = curvature_vs[0]; 
    tc_fs = vec2(-1,1);
    pos_fs = center + orth_left4 * (-point_size) + orth_up4 * point_size;
    ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex(); */

    //col_fs = col_vs[0];
    //norm_fs = norm_vs[0]; 
    //curvature_fs = curvature_vs[0]; 
    tc_fs = vec2(-1,-1);
    pos_fs = center + orth_left4 * (-point_size) + orth_up4 * (-point_size);
    ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex();

    /* col_fs = col_vs[0];
    norm_fs = norm_vs[0]; 
    curvature_fs = curvature_vs[0]; 
    tc_fs = vec2(1,-1);
    pos_fs = center + orth_left4 * point_size + orth_up4 * (-point_size);
    ocam_pos_original = toOcam(pos_fs, view, aabb_extend, ocam_mod, pos_view);
    gl_Position = ocam_pos_original;
    EmitVertex(); */
    
    EndPrimitive();
}
 
// #version 330
// layout(points) in;
// layout(points, max_vertices=1) out;
// //uniform mat4 model;
// uniform mat4 view_normal;
// uniform mat4 view;
// uniform mat4 proj;


// in vec4 pos_vs[1];
// in vec3 col_vs[1];
// in vec3 norm_vs[1];
// in float curvature_vs[1];

// out vec4 pos_fs;
// out vec3 col_fs;
// out vec3 norm_fs;
// out float curvature_fs;
// //out vec2 tc;

// void main() {
//     pos_fs = pos_vs[0];
//     col_fs = col_vs[0];
//     norm_fs = norm_vs[0]; 
//     curvature_fs = curvature_vs[0];  
//     vec4 pos = gl_in[0].gl_Position;
//     gl_Position = pos;
//     EmitVertex();    
//     EndPrimitive();
// }