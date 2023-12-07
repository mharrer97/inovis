#version 460
in vec2 tc;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
out vec4 out_col;

void main() {
    
    //out_col = vec4(texture(tex1, tc).rgb,1);
    if(tc.x < 0.5 && tc.y >= 0.5){ // top left
        vec2 newtc = vec2(tc.x*2, (tc.y-0.5)*2);
        out_col = vec4(texture(tex0, newtc).rgb,1);
    } else if(tc.x >= 0.5 && tc.y >= 0.5){ // top right
        vec2 newtc = vec2((tc.x-0.5)*2, (tc.y-0.5)*2)*0.5;
        out_col = vec4(texture(tex1, newtc).rgb,1);
    } else if(tc.x < 0.5 && tc.y < 0.5){ // bottom left
        vec2 newtc = tc*2*0.25;
        out_col = vec4(texture(tex2, newtc).rgb,1);
    } else if(tc.x >= 0.5 && tc.y < 0.5){ // bottom right
        vec2 newtc = vec2((tc.x-0.5)*2, tc.y*2)*0.125;
        out_col = vec4(texture(tex3, newtc).rgb,1);
    } 
    out_col = vec4(texture(tex1, tc).rgb,1);
    //out_col = vec4(tc,0,1);
}
