#version 130

uniform sampler2D fftwave; // frequency data and sound wave
uniform vec2 resolution; // viewport resolution
uniform float time; // playback time

// ##############################
// BEGIN	IQ methods
// ##############################

// Calculate matrix for camera looking at a specific target
mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
	vec3 cw = normalize(ta-ro);
	vec3 cp = vec3(sin(cr), cos(cr),0.0);
	vec3 cu = normalize( cross(cw,cp) );
	vec3 cv = normalize( cross(cu,cw) );
	return mat3( cu, cv, cw );
}
// ##############################
// END		IQ methods
// ##############################


#define PI 3.14159265
float vmax(vec3 v) {
	return max(max(v.x, v.y), v.z);
}
float fBox(vec3 p, vec3 b) {
	vec3 d = abs(p) - b;
	return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
}
float sdPlane( vec3 p )
{
	return p.y;
}

// Repeat around the origin by a fixed angle.
// For easier use, num of repetitions is use to specify the angle.
float pModPolar(inout vec2 p, float repetitions) {
	float angle = 2.*PI/repetitions;
	float a = atan(p.y, p.x) + angle/2.;
	float r = length(p);
	float c = floor(a/angle);
	a = mod(a,angle) - angle/2.;
	p = vec2(cos(a), sin(a))*r;
	// For an odd number of repetitions, fix cell index of the cell in -x direction
	// (cell index would be e.g. -5 and 5 in the two halves of the cell):
	if (abs(c) >= (repetitions/2.)) c = abs(c);
	return c;
}


// ##############################
// BEGIN	Camera helpers
// ##############################
uniform float iCamPosX;
uniform float iCamPosY;
uniform float iCamPosZ;
uniform float iCamRotX;
uniform float iCamRotY;
uniform float iCamRotZ;

vec3 calcCameraPos()
{
	return vec3(iCamPosX, iCamPosY, iCamPosZ);
}
void rotateAxis(inout vec2 p, float a)
{
	p = cos(a)*p + sin(a)*vec2(p.y, -p.x);
}
vec3 calcCameraRayDir(float fov, vec2 gl_FragCoord, vec2 resolution)
{
	float tanFov = tan(fov / 2.0 * 3.14159 / 180.0) / resolution.x;
	vec2 p = tanFov * (gl_FragCoord * 2.0 - resolution.xy);
	vec3 rayDir = normalize(vec3(p.x, p.y, 1.0));
	rotateAxis(rayDir.yz, iCamRotX);
	rotateAxis(rayDir.xz, iCamRotY);
	rotateAxis(rayDir.xy, iCamRotZ);
	return rayDir;
}
// ##############################
// END		Camera helpers
// ##############################

// Change this to change repetition interval
float afFrequencies[8];

float cubeCircle(vec3 point, float radius, int count, vec3 cubeSize)
{
	float c = pModPolar(point.xz, float(count));
	float index = floor(c + float(count)/2. -1.);

	return fBox( point-vec3(radius,0,0), cubeSize + vec3(0, 0.3 + 0.7*afFrequencies[int(mod(index,float(afFrequencies.length())))] , 0));
}

//	Calculates distance to nearest object given a point
float distFunc( vec3 point )
{
	//	parameters
	int circleTiles = 64;
	float radius = 4.;
	vec3 cubeSize = vec3(0.1);

	//	distance to floor plane
	float planeDist = sdPlane(point);

	//	distances to the cube circles
	float cubesCircleBigDist = cubeCircle(point, radius, circleTiles, cubeSize);
	float cubesCircleSmallDist = cubeCircle(point, radius/2., circleTiles/2, cubeSize);

	//	black cube in the center
	point.y -= 1.0;
	rotateAxis(point.yz, radians(45.0));
	rotateAxis(point.xy, PI/4.);
	float boxDist = fBox( point, 5.*cubeSize*afFrequencies[1] );

	//	return closest object
	return min(min(min(cubesCircleBigDist, boxDist), cubesCircleSmallDist), planeDist);
}

vec3 getNormal( in vec3 pos )
{
	// IQ
	vec2 e = vec2( 1.0,-1.0 ) * 0.001;
	return normalize( e.xyy*distFunc( pos + e.xyy ) +
	e.yyx*distFunc( pos + e.yyx ) +
	e.yxy*distFunc( pos + e.yxy ) +
	e.xxx*distFunc( pos + e.xxx ) );
}

vec3 getMaterialColor(vec3 point)
{
	//	floor color
	if(point.y < 0.0001)
	{
		return vec3( 0.5 );
	}

	//	color for center area
	if(length(point.xz) < 1.0)
	{
		return vec3( 0.0, 0.0, 0.0 );
	}

	//	otherwise determine color based on angle
	float count = 64.;

	//	Calculate ID for each segment of the circular rotation
	float c = pModPolar(point.xz, count);

	return vec3(0.45*sin(((c+time)/count)*2.*PI)+0.55);
}

float softshadow(const vec3 origin, in vec3 dir, in float mint, in float tmax, float k)
{
	float res = 1.0;
	float t = mint;
	for( int i=0; i<16; i++ )
	{
		float h = distFunc( origin + dir*t );
		res = min( res, k*h/t );
		t += clamp( h, 0.02, 0.10 );
		if( h<0.001 || t>tmax ) break;
	}
	return clamp( res, 0.0, 1.0 );
}

float ambientOcclusion(vec3 point, float delta, int samples)
{
	vec3 normal = getNormal(point);
	float occ = 0.;
	for(float i = 1.; i < float(samples); ++i)
	{
		occ += (2.0/i) * (i * delta - distFunc(point + i * delta * normal));
	}
	// occ = clamp(occ, 0, 1);
	return 1. - occ;
}


//	Lighting settings
#define ENABLE_SHADOWS
//#define ENABLE_OCCLUSION

const float lightAttenuation = 0.00;

vec3 getShadedColor( vec3 hitPosition, vec3 normal, vec3 cameraPosition )
{
	//	light relative to camera position
	vec3 lightPosition = vec3(sin(time), 3.0, cos(time));

	//	Specular highlight factor
	float materialShininess = 64.0;
	vec3 materialSpecularColor = vec3( 1.0 );

	//	Output color
	vec3 outputColor = vec3( 0.0 );

	//	Calculate eye vector and its reflection
	vec3 surfaceToLight = normalize(lightPosition - hitPosition);
	vec3 surfaceToCamera = normalize(cameraPosition - hitPosition);

	//	surface color
	vec3 surfaceColor = getMaterialColor(hitPosition);

	//	ambient component
    vec3 lightColor = vec3(abs(sin(time*0.84)), abs(cos(time)), abs(sin(time*1.337)))*smoothstep(-0.3, 1.0, afFrequencies[0]);
	vec3 ambientColor = surfaceColor * lightColor * 0.0; // ambient factor

	//	diffuse component
	float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
	vec3 diffuseColor = diffuseCoefficient * surfaceColor * lightColor;

	//	specular component
	float specularCoefficient = 0.0;
	if(diffuseCoefficient > 0.0) {
		//specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflect(-surfaceToLight, normal))), materialShininess);
	}
	vec3 specularColor = specularCoefficient * materialSpecularColor * lightColor;

	//	light attenuation (falloff based on distance, fog)
	float distanceToLight = length(lightPosition - hitPosition);
	float attenuation = 1.0 / (1.0 + lightAttenuation * pow(distanceToLight, 2.0));

	//	soft shadows (optional)
	float shadow = 1.0;
	#ifdef ENABLE_SHADOWS
	shadow = max(0.2, softshadow(hitPosition, surfaceToLight, 0.01, 5.0, 8.0));
	#endif

	//	ambient occlusion (optional)
	float occlusionCoefficient = 1.0;
	#ifdef ENABLE_OCCLUSION
	occlusionCoefficient = ambientOcclusion(hitPosition, 0.01, 10);
	#endif

	//	calculate final color
	outputColor = ambientColor + occlusionCoefficient * shadow * attenuation*(diffuseColor + specularColor);

	//	gamma correction
	//vec3 gamma = vec3(1.0/2.2);
	//outputColor = vec3(pow(outputColor, gamma));

	//	return shading result
	return outputColor;
}

const float epsilon = 0.0001;
const int maxSteps = 256;
const float maxT = 20.0;
float trace(vec3 ro, vec3 rd, out vec3 point, out bool objectHit)
{
	float t = 0.0;
	point = ro;

	for(int steps = 0; steps < maxSteps; ++steps)
	{
		//check how far the point is from the nearest surface
		float dist = distFunc(point);
		//if we are very close
		if(epsilon > dist)
		{
			objectHit = true;
			break;
		}
		//not so close -> we can step at least dist without hitting anything
		t += dist;
		// return immediately if maximum t is reached
		if(t > maxT)
		{
			objectHit = false;
			return maxT;
		}
		//calculate new point
		point = ro + t * rd;
	}

	return t;
}

void populateSoundArray()
{
    // Get FFT values from texture
    for (int i = 0; i < afFrequencies.length(); i++)
    {
        afFrequencies[i] = texture( fftwave, vec2(float(i)/float(afFrequencies.length()), 0.25) ).x;
    }
}

const int reflectionBounces = 2;
void main()
{
    // Fill arrays for sound things
    populateSoundArray();
    
	//	Set up Camera
	vec3 camP = calcCameraPos(); // Camera position

	//	Move camera in a circle
	camP += vec3(5.0*cos(time*0.25), 1.5*cos(time*0.2)+2.5,  5.0*sin(time*0.25));

	//	Always look at center
	vec3 target = vec3(0.0);
	mat3 cameraMatrix = setCamera( camP, target, 0.0 );
	vec2 p = (-resolution.xy + 2.0*gl_FragCoord.xy)/resolution.y;
	vec3 camDir = cameraMatrix * normalize( vec3(p.xy, 2.0) );

	//	Set up ray
	vec3 point;		// Set in trace()
	bool objectHit;	// Set in trace()

	//	Initialize color
	vec3 color = vec3(0.0);

	float t = trace(camP, camDir, point, objectHit);
	if(objectHit)
	{
		//	Lighting calculations
		vec3 normal = getNormal(point);
		color = getShadedColor( point, normal, camP );

		//	Reflections
		for(int i = 0; i < reflectionBounces; i++)
		{
			vec3 pointRef;	// Set in trace()
			camDir = reflect(camDir, normal);
			trace(point + camDir*0.001, camDir, pointRef, objectHit);
			if(objectHit)
			{
				// Get color of reflection
				color += 0.1 * getShadedColor( pointRef, getNormal(pointRef), point );
			}
			point = pointRef;
		}
	}

	//	fog
	vec3 fogColor = vec3( 0.1, 0.3, 0.8);
	float FogDensity = 0.005;
	float fogFactor = 1.0 /exp(t * FogDensity);
	fogFactor = clamp( fogFactor, 0.0, 1.0 );
	color = mix(fogColor, color, fogFactor);

	gl_FragColor = vec4(color, clamp((t-6.0)/15.0, 0.0, 1.0));
}