#version 460

#include "__ocam_model_include.glsl"



in vec2 tc;
uniform sampler2D tex;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 view_normal;
uniform float aabb_extend;
uniform ivec2 resolution;

layout(std430, binding = 0) buffer Ocam
{
	ocam_model ocam_mod;
};

out vec4 out_col;

void main() {
    ocam_model ocamtest = ocam_mod;
    //vec3 res = toWorld(tc, ocamtest);

    vec4 pos = inverse(view) * inverse(proj) * vec4(((gl_FragCoord.xy) / resolution) * 2.f-1.f, gl_FragCoord.z,1);
    
    vec4 pos_view;
    vec4 ocam_pos = toOcam(pos, view, aabb_extend, ocamtest, pos_view);
    vec2 original_tc = ocam_pos.xy*.5 +.5;



    out_col = vec4(texture(tex, original_tc).rgb,1);
    //out_col = vec4(ocamtest.cx,ocamtest.cy,ocamtest.c,1);
}
