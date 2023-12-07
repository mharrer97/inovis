#version 330
in vec4 pos_wc;
in vec3 col_wc;
in vec3 norm_wc;
in float curvature;
//in vec2 tc;

layout (location = 0) out vec4 out_col;
uniform mat4 view;
//uniform sampler2D diffuse;

void main() {
    out_col = vec4(col_wc,1);//vec4(texture(diffuse, tc).rgb, 1);
    //out_col = vec4(-(view*pos_wc).z);
}