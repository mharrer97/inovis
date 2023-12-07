#version 460
in vec2 tc;
uniform sampler2D tex;
out vec4 out_col;

void main() {
    out_col = vec4(vec3(texture(tex, tc).a),1);
}
