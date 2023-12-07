#version 330
in vec4 pos_wc;
in vec3 col_wc;
in vec3 norm_wc;
in float curvature;
//in vec2 tc;

layout (location = 0) out vec4 out_col;

//uniform sampler2D diffuse;

void main() {
    vec3 norm01 = (norm_wc+vec3(1.f)) * 0.5f;
    out_col = vec4(norm01, 1);
    //out_col = vec4(norm_wc,1);//vec4(texture(diffuse, tc).rgb, 1);
    //out_col = pos_wc;
}