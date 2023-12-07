#version 460
in vec2 tc;
uniform sampler2D tex;
out vec4 out_col;

void main() {
    out_col = vec4(texture(tex, tc).rgb,1);
}
