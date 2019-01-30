#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform int sample_size; // frequency data width

vec3 view(vec2 p)
{
    float r = resolution.y / resolution.x;
    float x = p.x-fract(p.x*80.)/80.;
    float h = texture(fftwave,vec2(x,0)).x;

    if (p.y <= h-fract(h*80.*r)/(80.*r)) {
        float py = p.y - fract(p.y*80.*r)/(80.*r);
        return vec3(h,h,py)*1.5;
    }
    return vec3(0.,0.,0.0);
}

void main()
{
    vec2 p = gl_FragCoord.xy / resolution.xy;
    gl_FragColor = vec4(view(p), 1.);
}