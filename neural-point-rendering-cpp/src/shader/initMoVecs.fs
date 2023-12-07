#version 460
in vec2 tc;

in vec4 pos_proj;
in vec4 pos_old_1;
in vec4 pos_old_2;
in vec4 pos_old_3;
in vec4 pos_old_4;
in vec4 pos_old_5;
in vec4 pos_old_6;

layout (location = 2) out vec4 out_motion_1;
layout (location = 3) out vec4 out_motion_2;
layout (location = 4) out vec4 out_motion_3;
layout (location = 5) out vec4 out_motion_4;
layout (location = 6) out vec4 out_motion_5;
layout (location = 7) out vec4 out_motion_6;

uniform ivec2 resolution;

void main() {

    vec2 new_tc = tc * 2.f - vec2(1.f);
    out_motion_1 = vec4(new_tc, 1,1);
    out_motion_2 = vec4(new_tc, 1,1);
    out_motion_3 = vec4(new_tc, 1,1);


    vec2 tc_old_1 = pos_old_1.xy/pos_old_1.w;
    out_motion_1 = vec4(tc_old_1,1, 1);
    // out_motion_1 = vec4(new_tc,0, 1);
    vec2 tc_old_2 = pos_old_2.xy/pos_old_2.w;
    out_motion_2 = vec4(tc_old_2,1, 1);
    vec2 tc_old_3 = pos_old_3.xy/pos_old_3.w;
    out_motion_3 = vec4(tc_old_3,1, 1);
    vec2 tc_old_4 = pos_old_4.xy/pos_old_4.w;
    out_motion_4 = vec4(tc_old_4,1, 1);
    vec2 tc_old_5 = pos_old_5.xy/pos_old_5.w;
    out_motion_5 = vec4(tc_old_5,1, 1);
    vec2 tc_old_6 = pos_old_6.xy/pos_old_6.w;
    out_motion_6 = vec4(tc_old_6,1, 1);
      
}

