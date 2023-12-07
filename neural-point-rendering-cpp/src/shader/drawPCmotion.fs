#version 330
in vec4 pos_wc;
in vec3 col_wc;
in vec3 norm_wc;
in float curvature;
in vec4 pos_proj;
in vec4 pos_old;
//in vec2 tc;

layout (location = 0) out vec4 out_col;
uniform mat4 view;

uniform ivec2 resolution;

void main() {
    vec2 tc_old = pos_old.xy/pos_old.w; // [-1,1]^2
    tc_old = ((tc_old+vec2(1,1)) * 0.5); // [0,1]^2
    tc_old *= resolution; // [0,resolution]^2. the old tc is not pixel centered
    vec2 movec = gl_FragCoord.xy - tc_old; // glFrag coord is in [0,res]^2 and represents the pixel centered positions

    // movecs are unique for point cloud since glPoints does not interpolate anything -> the computed previous position etc. are not pixelcentered 
    // for warping, pixelcentered origin coordinastes are used as start point
    // -> to get best approximation, use pixelcentered glFragCoord and use the not centered computed old location
    // -> movecs are from center to specific location 
    // -> prevents from too large warping for same resolution and enables finer movement when used for zero upsampling etc.

    // tc wise movecs?
    vec2 tc_old_1 = pos_old.xy/pos_old.w;
    out_col = vec4(tc_old_1,1, 1);
}