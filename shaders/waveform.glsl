#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform int sample_size; // frequency data width

void main()
{
    // create pixel coordinates
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // the sound texture is sample_size*2
    int tx = int(uv.x*sample_size);

    // first row is frequency data
    float fft  = texelFetch( fftwave, ivec2(tx,0), 0 ).x; 

    // second row is the sound wave, one texel is one mono sample
    float wave = texelFetch( fftwave, ivec2(tx,1), 0 ).x;

    // convert frequency to colors
    vec3 col = vec3( fft, 4.0*fft*(1.0-fft), 1.0-fft ) * fft ;

    // add wave form on top	
    col += 1.0 -  smoothstep( 0.0, 0.15, abs(wave - uv.y) );

    // output final color
    gl_FragColor = vec4(col,1.0);
}
