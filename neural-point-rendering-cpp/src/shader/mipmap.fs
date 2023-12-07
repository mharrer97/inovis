#version 460
in vec2 tc;
uniform sampler2D tex_rgb;
uniform sampler2D tex_depth;
uniform ivec2 res_high;
uniform ivec2 res_low;
layout (location = 0) out vec4 out_col;
layout (location = 1) out vec4 out_depth;

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
    //out_col = vec4(vec3(minimum),1);
    
    /*if(minimum_i == 0) out_col = vec4(1,0,0,1);
    if(minimum_i == 1) out_col = vec4(0,1,0,1);
    if(minimum_i == 2) out_col = vec4(0,0,1,1);
    if(minimum_i == 3) out_col = vec4(0,0,0,1);*/
    //out_col = vec4(texture(tex_rgb, tc).rgb,1);
    
}
