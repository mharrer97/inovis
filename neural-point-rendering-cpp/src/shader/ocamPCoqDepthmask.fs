#version 460
in vec4 pos_fs;
in vec4 pos_view;
in vec3 norm_fs;
in vec3 col_fs;
in float curvature_fs;
//in float depth_ocam;
//in vec2 original_tc;
//in vec2 novel_tc;
in vec2 tc_fs;

//in vec4 debug;

layout (location = 0) out vec4 out_col;

//uniform sampler2D diffuse;

void main() {
        if(tc_fs.x*tc_fs.x+tc_fs.y*tc_fs.y >1)discard;

    out_col = vec4(vec3(gl_FragCoord.z),1); 
}