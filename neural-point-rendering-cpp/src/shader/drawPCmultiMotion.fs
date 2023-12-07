#version 430
in vec4 pos_wc;
in vec3 col_wc;
in vec3 norm_wc;
in vec4 pos_proj;
in vec4 pos_old_1;
in vec4 pos_old_2;
in vec4 pos_old_3;
in vec4 pos_old_4;
in vec4 pos_old_5;
in vec4 pos_old_6;
flat in int timestamp;
//in vec2 tc;

layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_depth;
//layout (location = 2) out vec4 out_curvature;
//layout (location = 2) out vec4 out_normal;
layout (location = 2) out vec4 out_motion_1;
layout (location = 3) out vec4 out_motion_2;
layout (location = 4) out vec4 out_motion_3;
layout (location = 5) out vec4 out_motion_4;
layout (location = 6) out vec4 out_motion_5;
layout (location = 7) out vec4 out_motion_6;

uniform mat4 view;
uniform ivec2 resolution;
uniform int gt_timestamp_max; 
uniform int gt_timestamp_min; 
uniform bool use_timestamp;

void main() {
    if(use_timestamp){
        if (timestamp > gt_timestamp_max) discard;
        if (timestamp < gt_timestamp_min ) discard;
    }
    //-----------------------------------------------------------------
    // col
    out_col = vec4(col_wc,1);
    // if(timestamp==100) out_col = vec4(1,0,0,1);
    // if(timestamp==75) out_col = vec4(0,1,0,1);
    // if(timestamp==50) out_col = vec4(0,0,1,1);
    //out_col = vec4(vec3(1-(timestamp)/100.0),1);
    // depth
    out_depth = vec4(vec3(gl_FragCoord.z),1);
    
    //-----------------------------------------------------------------
    // curvature
    //out_curvature = vec4(vec3(curvature),1);
    
    //-----------------------------------------------------------------
    // norm
    //vec3 norm01 = (norm_wc+vec3(1.f)) * 0.5f;
    //out_normal = vec4(norm01, 1);
    //-----------------------------------------------------------------

    
    //vec2 tc_old_1 = pos_old_1.xy/pos_old_1.w; // [-1,1]^2
    //tc_old_1 = ((tc_old_1+vec2(1,1)) * 0.5); // [0,1]^2
    //tc_old_1 *= resolution; // [0,resolution]^2. the old tc is not pixel centered
    //vec2 movec_1 = gl_FragCoord.xy - tc_old_1; // glFrag coord is in [0,res]^2 and represents the pixel centered positions
    

    // movecs are unique for point cloud since glPoints does not interpolate anything -> the computed previous position etc. are not pixelcentered 
    // for warping, pixelcentered origin coordinastes are used as start point
    // -> to get best approximation, use pixelcentered glFragCoord and use the not centered computed old location
    // -> movecs are from center to specific location 
    // -> prevents from too large warping for same resolution and enables finer movement when used for zero upsampling etc.

    //out_motion_1 = vec4(movec_1,1, 1);

    /*vec2 tc_old_2 = pos_old_2.xy/pos_old_2.w; // [-1,1]^2
    tc_old_2 = ((tc_old_2+vec2(1,1)) * 0.5); // [0,1]^2
    tc_old_2 *= resolution; // [0,resolution]^2. the old tc is not pixel centered
    vec2 movec_2 = gl_FragCoord.xy - tc_old_2; // glFrag coord is in [0,res]^2 and represents the pixel centered positions
    */

    //out_motion_2 = vec4(movec_2,1, 1);

    /*
    vec2 tc_old_3 = pos_old_3.xy/pos_old_3.w; // [-1,1]^2
    tc_old_3 = ((tc_old_3+vec2(1,1)) * 0.5); // [0,1]^2
    tc_old_3 *= resolution; // [0,resolution]^2. the old tc is not pixel centered
    vec2 movec_3 = gl_FragCoord.xy - tc_old_3; // glFrag coord is in [0,res]^2 and represents the pixel centered positions
    */

    //out_motion_3 = vec4(movec_3,1, 1);   

    vec2 tc_old_1 = pos_old_1.xy/pos_old_1.w;
    out_motion_1 = vec4(tc_old_1,1, 1);
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