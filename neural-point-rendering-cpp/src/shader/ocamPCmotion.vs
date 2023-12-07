#version 460

//#extension GL_ARB_gpu_shader_fp64 : enable
//#pragma optionNV(fastmath off)
//#pragma optionNV(fastprecision off)

#include "__ocam_model_include.glsl"


layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in float in_curvature;
//in int point_id;

//uniform mat4 model_normal;
uniform mat4 view;
uniform mat4 proj;
// old view for motion vecs
uniform mat4 view_old;

//uniform mat4 debug_view;


uniform float aabb_extend;

layout(std430, binding = 0) buffer Ocam
{
	ocam_model ocam_mod;
};


out vec4 pos_vs;
out vec4 pos_view;
out vec3 norm_vs;
out vec3 color_vs;
out float curvature_vs;
out float depth_ocam;
out vec2 original_tc;
out vec2 novel_tc;
out vec4 pos_old;
//out int point_id_fs;

out vec4 debug;


void main() {
    color_vs = in_color;
     curvature_vs = in_curvature;
    //point_id_fs = point_id;

    vec4 pos = vec4(in_pos, 1.0);
    //norm_vs = normalize(mat3(model_normal) * in_norm);
    norm_vs = normalize(mat3(view)*in_normal);

  
  //  gl_Position = toOcam(fromOcam(toOcam(pos_vs, view, aabb_extend, ocam_mod, pos_view), view, aabb_extend, ocam_mod, pos_view), view, aabb_extend, ocam_mod, pos_view);
    vec4 ocam_pos_original = toOcam(pos, view, aabb_extend, ocam_mod, pos_view);
    original_tc = ocam_pos_original.xy*.5 +.5;
    gl_Position = ocam_pos_original;
    pos_vs = ocam_pos_original;
    // calculate old position for motion vecs
    vec4 pos_view_old;
    pos_old = toOcam(pos, view_old, aabb_extend, ocam_mod, pos_view_old);

    //debug = fromOcam(ocam_pos_original, view, aabb_extend, ocam_mod, pos_view);
    
    //gl_Position = fromOcam(ocam_pos_original, view, aabb_extend, ocam_mod, pos_view);

    /*mat4 view2 = inverse(view);
 //   view2[3][0]+=0.2;
 //   view2[3][1]+=0.2;
 //   view2[3][2]+=0.2;
    view2 = view;*/

    //gl_Position = toOcam(gl_Position, view, aabb_extend, ocam_mod, pos_view);
    novel_tc = gl_Position.xy*.5 +.5;
    depth_ocam = gl_Position.z; 

    //gl_Position = proj * view * pos_vs; // for debugging

    //color_vs = vec3(original_tc.xy,0);// for debugging
  
}
