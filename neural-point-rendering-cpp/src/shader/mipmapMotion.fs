#version 460
in vec2 tc;
uniform sampler2D tex_rgb;
uniform sampler2D tex_depth;
uniform sampler2D tex_motion_1;
uniform sampler2D tex_motion_2;
uniform sampler2D tex_motion_3;
uniform sampler2D tex_motion_4;
uniform sampler2D tex_motion_5;
uniform sampler2D tex_motion_6;

uniform ivec2 res_high;
uniform ivec2 res_low;
layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_depth;
layout (location = 2) out vec4 out_motion_1;
layout (location = 3) out vec4 out_motion_2;
layout (location = 4) out vec4 out_motion_3;
layout (location = 5) out vec4 out_motion_4;
layout (location = 6) out vec4 out_motion_5;
layout (location = 7) out vec4 out_motion_6;

void main() {
    // tc is middle point of higher res -> on corner of 4 pixels
    // compute distance to each center
    // vec2 dist_high_to_low = 1.0/(res_high*2.0);
    // vec2 tcs[4];
    // tcs[0] = tc + dist_high_to_low;
    // tcs[1] = vec2(tc.x + dist_high_to_low.x, tc.y - dist_high_to_low.y);
    // tcs[2] = vec2(tc.x - dist_high_to_low.x, tc.y + dist_high_to_low.y);
    // tcs[3] = tc - dist_high_to_low;
    // float ds[4];
    // ds[0] = texture(tex_depth, tcs[0]).r;
    // ds[1] = texture(tex_depth, tcs[1]).r;
    // ds[2] = texture(tex_depth, tcs[2]).r;
    // ds[3] = texture(tex_depth, tcs[3]).r;
    // int minimum_i = 0;
    // float minimum = ds[0];
    // for(int i = 1; i<4; ++i){
    //     if(minimum > ds[i]){
    //          minimum = ds[i];
    //          minimum_i = i;
    //     }
    // }
    // gl_FragDepth = minimum;//-0.001;
    // out_col = vec4(texture(tex_rgb, tcs[minimum_i]).rgb,1);
     
    ivec2 coords[4];
    coords[0] = ivec2(gl_FragCoord.x, gl_FragCoord.y)*2;
    coords[1] = coords[0] + ivec2(0,1);
    coords[2] = coords[0] + ivec2(1,0);
    coords[3] = coords[0] + ivec2(1,1);
    float ds[4];
    ds[0] = texelFetch(tex_depth, coords[0],0).r;
    ds[1] = texelFetch(tex_depth, coords[1],0).r;
    ds[2] = texelFetch(tex_depth, coords[2],0).r;
    ds[3] = texelFetch(tex_depth, coords[3],0).r;
    
    int minimum_i = 0;
    float minimum = ds[0];
    for(int i = 1; i<4; ++i){
        if(minimum > ds[i]){
             minimum = ds[i];
             minimum_i = i;
        }
    }
    gl_FragDepth = minimum;//-0.001;
    out_col = vec4(texelFetch(tex_rgb, coords[minimum_i],0).rgb,1);
    out_depth = vec4(vec3(minimum),1);
    out_motion_1 = vec4(texelFetch(tex_motion_1, coords[minimum_i],0).rgb,1);
    out_motion_1 = vec4(out_motion_1.rg * 0.5, out_motion_1.b, 1);
    out_motion_2 = vec4(texelFetch(tex_motion_2, coords[minimum_i],0).rgb,1);
    out_motion_2 = vec4(out_motion_2.rg * 0.5, out_motion_2.b, 1);
    out_motion_3 = vec4(texelFetch(tex_motion_3, coords[minimum_i],0).rgb,1);
    out_motion_3 = vec4(out_motion_3.rg * 0.5, out_motion_3.b, 1);
    out_motion_4 = vec4(texelFetch(tex_motion_4, coords[minimum_i],0).rgb,1);
    out_motion_4 = vec4(out_motion_4.rg * 0.5, out_motion_4.b, 1);
    out_motion_5 = vec4(texelFetch(tex_motion_5, coords[minimum_i],0).rgb,1);
    out_motion_5 = vec4(out_motion_5.rg * 0.5, out_motion_5.b, 1);
    out_motion_6 = vec4(texelFetch(tex_motion_6, coords[minimum_i],0).rgb,1);
    out_motion_6 = vec4(out_motion_6.rg * 0.5, out_motion_6.b, 1);
    vec4 offset[4];
    for (int i=0; i<4; ++i) offset[i] = vec4(0);
    offset[0] = vec4( 0.25,  0.25,0,0);
    offset[1] = vec4( 0.25, -0.25,0,0);
    offset[2] = vec4(-0.25,  0.25,0,0);
    offset[3] = vec4(-0.25, -0.25,0,0);
    out_motion_1 += offset[minimum_i];
    out_motion_2 += offset[minimum_i];
    out_motion_3 += offset[minimum_i];
    out_motion_4 += offset[minimum_i];
    out_motion_5 += offset[minimum_i];
    out_motion_6 += offset[minimum_i];
    
    //if(minimum_i == 0) out_motion_1 = vec4(1);
    //out_motion_1 = vec4(coords[3],0,0);
    //out_col = vec4(vec3(minimum),1);
    
    /*if(minimum_i == 0) out_col = vec4(1,0,0,1);
    if(minimum_i == 1) out_col = vec4(0,1,0,1);
    if(minimum_i == 2) out_col = vec4(0,0,1,1);
    if(minimum_i == 3) out_col = vec4(0,0,0,1);*/
    //out_col = vec4(texture(tex_rgb, tc).rgb,1);
    
}
