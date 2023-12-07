#version 460
in vec2 tc;
uniform vec3 col;
out vec4 out_col;
void main() {
    out_col = vec4(-col,1);
}
