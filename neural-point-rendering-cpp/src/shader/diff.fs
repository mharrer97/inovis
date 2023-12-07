#version 460
in vec2 tc;
uniform sampler2D tex_1;
uniform sampler2D tex_2;
layout (location = 0) out vec4 out_col;


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
     
    //vec4 val_1 = vec4(texelFetch(tex_1, ivec2(gl_FragCoord.x, gl_FragCoord.y),0).rgb,1);
    //vec4 val_2 = vec4(texelFetch(tex_2, ivec2(gl_FragCoord.x, gl_FragCoord.y),0).rgb,1);
    vec3 val_1 = texture(tex_1, tc).rgb;
    vec3 val_2 = texture(tex_2, tc).rgb;

    out_col = vec4(abs(val_1-val_2), 1);
    
}
