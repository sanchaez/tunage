#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform int sample_size; // frequency data width
uniform float time; // playback time

// #define ONE_SABER

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    uv = -1.0 + 2.0 * uv;
    uv.x *= resolution.x / resolution.y;
    float freqs[4];
    freqs[0] = texture( fftwave, vec2( 0.01, 0.25 ) ).x;
    freqs[1] = texture( fftwave, vec2( 0.07, 0.25 ) ).x;
    freqs[2] = texture( fftwave, vec2( 0.15, 0.25 ) ).x;
    freqs[3] = texture( fftwave, vec2( 0.30, 0.25 ) ).x;
    
    uv.x += sin(uv.y * uv.y + time * 1.5) * freqs[3];
    
    float d = 0.0;
    
    #ifdef ONE_SABER
    d += abs(10.0 / uv.x) * freqs[3] * 0.01;
    #else
    for (float i = 0.0; i < 13.0; i++) {
        uv.x += sin(i * cos(time * 0.1) * 20.0 + time * 0.5 + uv.y * 4.0) * freqs[1] * 0.1;
        d += abs(1.0 / uv.x) * freqs[3] * 0.01;  
    }
    #endif
    
    vec3 colour = vec3(freqs[0], freqs[1], freqs[2] * 2.0) * d;
    if (length(vec4(freqs[0], freqs[1], freqs[2], freqs[3])) > 1.5) {
        colour = 1.0 - colour;
    }

    gl_FragColor = vec4(colour, 1.0);
}