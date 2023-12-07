#version 460
in vec4 pos_vs;
in vec4 pos_view;
in vec3 norm_vs;
in vec3 color_vs;
in float curvature_vs;
in float depth_ocam;
in vec2 original_tc;
in vec2 novel_tc;
//in vec2 tc;

in vec4 debug;

layout (location = 0) out vec4 out_col;

//uniform sampler2D diffuse;

void main() {
    out_col = vec4(color_vs, 1); 
}