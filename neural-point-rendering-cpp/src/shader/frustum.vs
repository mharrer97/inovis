#version 460
layout (location = 0) in vec3 in_pos;
// in vec2 in_tc;
// out vec2 tc;
uniform mat4 view;
uniform mat4 view_frustum;
uniform mat4 proj;

out vec4 pos_wc;


void main() {
    vec4 pos= proj * view  * inverse(view_frustum) * vec4(in_pos, 1.0);
    //pos.z = -0.8;
    gl_Position = pos;
    pos_wc = vec4(in_pos, 1.0);
    
}
