#version 460
//in vec2 tc;
//uniform sampler2D tex;
out vec4 out_col;

uniform int custom_col;
uniform vec3 col;
in vec4 pos_wc; 
void main() {
    if (custom_col == 1){ 
        out_col = vec4(col,1);
    } else {
        out_col = vec4(5*pos_wc.xyz + vec3(0.75),1);
    }
}
