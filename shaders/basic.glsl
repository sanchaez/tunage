#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform int sample_size; // frequency data width

const float nBands = 128;

vec3 mixc(vec3 col1, vec3 col2, float v)
{
    v = clamp(v,0.0,1.0);
    return col1+v*(col2-col1);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 p = uv*2.0-1.0;
    p.x*=resolution.x/resolution.y;
    p.y+=0.5;
    
    vec3 col = vec3(0.0);
    vec3 ref = vec3(0.0);
   
    float i = floor(uv.x*nBands);
    float f = fract(uv.x*nBands);
    float band = i/nBands;
    band *= band*band;
    band = band*0.995;
    band += 0.005;
    float s = texture( fftwave, vec2(band,0.25) ).x;

    /* Gradient colors and amount here */
    const int nColors = 4;
    vec3 colors[nColors];  
    colors[0] = vec3(0.0,0.0,1.0);
    colors[1] = vec3(0.0,1.0,1.0);
    colors[2] = vec3(1.0,1.0,0.0);
    colors[3] = vec3(1.0,0.0,0.0);
    
    vec3 gradCol = colors[0];
    float n = float(nColors)-1.0;
    for(int i = 1; i < nColors; i++)
    {
        gradCol = mixc(gradCol,colors[i],(s-float(i-1)/n)*n);
    }
      
    col += vec3(1.0-smoothstep(0.0,0.01,p.y-s*1.5));
    col *= gradCol;

    ref += vec3(1.0-smoothstep(0.0,-0.01,p.y+s*1.5));
    ref*= gradCol*smoothstep(-0.5,0.5,p.y);
    
    col = mix(ref,col,smoothstep(-0.01,0.01,p.y));

    col *= smoothstep(0.125,0.375,f);
    col *= smoothstep(0.875,0.625,f);

    col = clamp(col, 0.0, 1.0);

    //float dither = noise3D(vec3(p,time))*2.0/256.0;
    //col += dither;
    
    gl_FragColor = vec4(col,1.0);
}
