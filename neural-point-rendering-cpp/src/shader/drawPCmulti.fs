#version 330
in vec4 pos_wc;
in vec3 col_wc;
in vec3 norm_wc;
in float curvature;
in vec4 pos_proj;
in vec4 pos_old;
flat in int timestamp;

//in vec2 tc;

layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_depth;
layout (location = 2) out vec4 out_curvature;
layout (location = 3) out vec4 out_normal;
layout (location = 4) out vec4 out_motion;

uniform mat4 view;

uniform ivec2 resolution;

uniform int gt_timestamp; 
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

    // depth
    out_depth = vec4(vec3(gl_FragCoord.z),1);
    
    //-----------------------------------------------------------------
    // motion
    
    //-----------------------------------------------------------------
    // norm
    vec3 norm01 = (norm_wc+vec3(1.f)) * 0.5f;
    out_normal = vec4(norm01, 1);
    //-----------------------------------------------------------------

    vec2 tc_old = pos_old.xy/pos_old.w; // [-1,1]^2
    tc_old = ((tc_old+vec2(1,1)) * 0.5); // [0,1]^2
    tc_old *= resolution; // [0,resolution]^2. the old tc is not pixel centered
    vec2 movec = gl_FragCoord.xy - tc_old; // glFrag coord is in [0,res]^2 and represents the pixel centered positions
    // movecs are unique for point cloud since glPoints does not interpolate anything -> the computed previous position etc. are not pixelcentered 
    // for warping, pixelcentered origin coordinastes are used as start point
    // -> to get best approximation, use pixelcentered glFragCoord and use the not centered computed old location
    // -> movecs are from center to specific location 
    // -> prevents from too large warping for same resolution and enables finer movement when used for zero upsampling etc.
    out_motion = vec4(movec,0, 1);
    
}