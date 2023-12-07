#version 330
in vec4 pos_fs;
in vec3 col_fs;
in vec3 norm_fs;
in float curvature_fs;
//in vec4 center_fs;
in vec2 tc_fs;

layout (location = 0) out vec4 out_col;

//uniform sampler2D diffuse;
float sqlen(vec4 a, vec4 b){
    vec4 c = b-a;
    return c.x*c.x + c.y*c.y + c.z*c.z;
}

void main() {
    //if(sqlen(pos_fs,center_fs) > 0.0002) discard;
    if(tc_fs.x*tc_fs.x+tc_fs.y*tc_fs.y >1)discard;
    out_col = vec4(vec3(gl_FragCoord.z),1); 
    //out_col = vec4(norm_fs,1);
    //out_col = vec4(1); 
    //out_col = pos_wc;
}