#version 460
//in vec2 tc;
//uniform sampler2D tex;
out vec4 out_col;

uniform vec3 col;

in vec4 pos_wc; 
void main() {
    out_col = vec4(col,1);
}
