#version 330
in vec3 in_pos;
in vec2 in_tc;
//layout (location = 2) in vec2 in_tc;
//layout (location = 3) in vec2 in_a;

//uniform mat4 model;
uniform mat4 view_normal;
uniform mat4 view;
uniform mat4 proj;
// old view for motion vecs
uniform mat4 view_old_1;
uniform mat4 view_old_2;
uniform mat4 view_old_3;
uniform mat4 view_old_4;
uniform mat4 view_old_5;
uniform mat4 view_old_6;

uniform mat4 proj_old;

uniform bool use_taa;

out vec4 pos_wc;

out vec4 pos_proj;
out vec4 pos_old_1;
out vec4 pos_old_2;
out vec4 pos_old_3;
out vec4 pos_old_4;
out vec4 pos_old_5;
out vec4 pos_old_6;

out vec2 tc;

void main() {
     vec4 pos= vec4(vec3(2.0)*in_pos - vec3(1.0), 1.0);
    //  vec4 pos= vec4(in_pos , 1.0);
    //pos.xyz /= 2;
    pos.z = 0.99999;
    gl_Position = pos; // clip space coordinate
    
    
    vec4 pos_view = inverse(proj) * pos;
    pos_view /=pos_view.w;
    // pos_view.z = -1000;
    vec4 pos_world = inverse(view) * pos_view;
    pos_world /=pos_world.w;

    tc = in_tc;
    
    
    pos_wc = vec4(in_pos, 1.0);

    // vec4 cur_pos_proj = vec4(in_pos, 1.0);
    // cur_pos_proj = inverse(view) * inverse(proj)  * cur_pos_proj;
    // pos_wc = cur_pos_proj;
    
    pos_proj = proj * view * pos_wc;
    //gl_Position = pos_proj;
    if (use_taa) 
        pos_old_1 = proj * view_old_1 * pos_world; 
    else
        pos_old_1 = proj_old * view_old_1 * pos_world; 
    
    pos_old_2 = proj_old * view_old_2 * pos_world; 
    pos_old_3 = proj_old * view_old_3 * pos_world; 
    pos_old_4 = proj_old * view_old_4 * pos_world; 
    pos_old_5 = proj_old * view_old_5 * pos_world; 
    pos_old_6 = proj_old * view_old_6 * pos_world; 

   
}