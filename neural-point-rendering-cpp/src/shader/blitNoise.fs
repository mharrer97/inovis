#version 460

#define M_PI 3.1415926535897932384626433832795

in vec2 tc;
uniform float time;
uniform ivec2 resolution;
out vec4 out_col;

float rand(float n){return fract(sin(n) * 43758.5453123);}

#define NOISE fbm
#define NUM_NOISE_OCTAVES 5

// Precision-adjusted variations of https://www.shadertoy.com/view/4djSRW
float hash(float p) { p = fract(p * 0.011); p *= p + 7.5; p *= p + p; return fract(p); }
float hash(vec2 p) {vec3 p3 = fract(vec3(p.xyx) * 0.13); p3 += dot(p3, p3.yzx + 3.333); return fract((p3.x + p3.y) * p3.z); }

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);

	// Four corners in 2D of a tile
	float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Simple 2D lerp using smoothstep envelope between the values.
	// return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
	//			mix(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps in smoothstep and common subexpressions
	// optimized away.
    vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 x) {
	float v = 0.0;
	float a = 0.5;
	vec2 shift = vec2(100);
	// Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
	for (int i = 0; i < NUM_NOISE_OCTAVES; ++i) {
		v += a * noise(x);
		x = rot * x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

void main() {
    vec2 rcoord = (tc.xy*resolution) * 0.5 - vec2(rand(time*30.0)*time * (-20.0), time*520.0*rand(time*200.0)*rand(resolution.y) );
    vec2 gcoord = (tc.xy*resolution) * 1.5 - vec2(rand(time*30.0)*time * 50.0, time*9087.0*rand(-time*13.0)*rand(resolution.x) );
    vec2 bcoord = (tc.xy*resolution) * 1 - vec2(rand(time*30.0)*time * 100.0, time*(-120.0)*rand(time*30.0)*rand(resolution.y*2) );
		float r = NOISE(rcoord);
    float g = NOISE(gcoord);
    float b = NOISE(bcoord);
    out_col = vec4(vec3(r,g,b),1);
}
