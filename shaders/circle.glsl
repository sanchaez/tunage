#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform int sample_size; // frequency data width
uniform float time; // playback time

#define MUSICCHANNEL fftwave
#define MUSICTEXWIDTH sample_size
#define CONTACT 0.04

const int iDiskCount = 48; // switch to 64 for performance
const float fInnerDisk = 0.002;
const float fMinSamplingRate = 1.0 / 1024.;

const int iMaxSamples = 64;

const float fSplitY = 1.0 / float(iDiskCount);
const float TexelsPerDisk = 1.0 / float(iDiskCount + 1);

// utils
vec3 lerp(vec3 x, vec3 y, float s) { return s * (y - x) + x; }
vec4 lerp(vec4 x, vec4 y, float s) { return s * (y - x) + x; }
float saturate(float x) { return clamp(x, 0.0, 1.0); }
mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,-s,s,c);}

// iq
float sdTorus(vec3 p, vec2 t) {
    return length(vec2(length(p.xz) - t.x, p.y)) - t.y;
}

vec3 hsv2rgb(vec3 c) {
    const vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 hsv2rgb(float h, float s, float v) {
    return hsv2rgb(vec3(h,s,v));
}

vec3 LUT(float x) {
    return hsv2rgb(vec3((330.0 / 360.0) - x, 1.0, 1.0));
}

vec4 And(vec4 A, float MinB, vec3 BColor) {
    float NewMin = step(MinB, A.x);

    return A * (1.0 - NewMin) + NewMin * vec4(MinB, BColor);
}

vec4 map(vec3 pos) {
    // x = minDist
    // yzw = color
    vec4 Result = vec4(1e9, 0.0, 0.0, 0.0);

    for(int i = 0; i < iDiskCount; ++i) {
        float DiskWidth = textureGrad(MUSICCHANNEL, vec2(1.0 - float(i) * TexelsPerDisk, 0.0), vec2(0.0), vec2(0.0)).r;
        float AvoidDisk = (step(DiskWidth, 1e-5)) * 1e9;
        
        float DiskY = float(i) * (1.0 / float(iDiskCount)) - 0.5;
        Result = And(Result, AvoidDisk + sdTorus(pos - vec3(0.0, DiskY, 0.0), vec2(DiskWidth, fInnerDisk)), LUT(DiskWidth));
    }
   
    return Result;
}

bool HitCylinder(vec3 RayOri, vec3 RayDir) {
    const float cHalfHeight = 0.575;
    const float cRadius = 1.05;

    float a = 1.0 - (RayDir.y * RayDir.y);
    float b = dot(RayOri, RayDir) - (RayOri.y * RayDir.y);
    float c = dot(RayOri, RayOri) - (RayOri.y * RayOri.y) - (cRadius * cRadius);
    float h = b * b - a * c;
    
    if(h < 0.0) 
        return true;
    
    h = sqrt(h);
    float t1 = (-b - h) / a;
 
    float y = RayOri.y + t1 * RayDir.y;

    if(abs(y) < cHalfHeight) 
        return false;

    float sy = sign(y);
    float tp = (sy * cHalfHeight - RayOri.y) / RayDir.y;
    if(abs(a * tp + b) < h)
        return false;
    return true;
}

void main() {
    vec2 uv = (gl_FragCoord.xy / resolution.xy - 0.5);
    uv.x *= resolution.x / resolution.y;

    // ray   
    vec3 vRayDir = vec3(0, 0, 1);
    vec3 vRayPos = vec3(uv.xy * 2.0, -1.5); 
        
    vec3 vInter = vRayPos;

    // rotation
    vec2 um = vec2(3.5 + time * 0.05, 1.0);
    mat2 mx = mm2(um.x * -12.0);
    mat2 my = mm2(um.y * 12.0); 
    vInter.xz *= mx;
    vRayDir.xz *= mx;
    vInter.yz *= my;
    vRayDir.yz *= my;
    
    // early check hit worst cylinder
    if(HitCylinder(vInter, vRayDir)) {
        gl_FragColor = vec4(0.0);
        return;
    }

    //
    vec4 vDstColor = vec4(0.0);
    
    float r = 0.0;
    for(int iSample = 0; iSample < iMaxSamples; ++iSample)
    {
        if(!(vDstColor.a < 0.985 && r < 3.0))
           break;
        
        vec4 R = map(vInter);
        float l = R.x;
        
        const float h = .04;
        float Contact = step(l, h);
        float ld = (h - l) * Contact;
        
        vDstColor.rgb += ld * R.yzw;
        vDstColor.a += ld;
        
        l = max(l, fMinSamplingRate);

        r += l;

        vInter.xyz += l * vRayDir;
    }
    
    gl_FragColor = vec4(vDstColor.rgb, 0.0);
}