#ifndef __GGP_SHADER_INCLUDES__ // Each .hlsli file needs a unique identifier
#define __GGP_SHADER_INCLUDES__

#define LIGHT_TYPE_DIRECTIONAL		0
#define LIGHT_TYPE_POINT			1
#define LIGHT_TYPE_SPOT				2

#define MAX_SPECULAR_EXPONENT		256.0f

// All of your code pieces (structs, functions, etc.) go here!
struct VertexShaderInput
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float3 localPosition	: POSITION;     // XYZ position
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
};

// Struct representing the data we're sending down the pipeline
// - Should match our pixel shader's input (hence the name: Vertex to Pixel)
// - At a minimum, we need a piece of data defined tagged as SV_POSITION
// - The name of the struct itself is unimportant, but should be descriptive
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;	// XYZW position (System Value Position)
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
	float3 worldPosition	: POSITION;
};

struct Light
{
	int Type				: TYPE;
	float3 Direction		: DIRECTION;
	float Range				: RANGE;
	float3 Position			: L_POSITION;
	float Intensity			: INTENSITY;
	float3 Color			: COLOR;
	float SpotFalloff		: SPOTFALLOFF;
	float3 Padding			: PADDING;
};

float3 CalcDiffuse(float3 normal, float3 dirToLight) 
{
	return saturate(dot(normal, dirToLight));
}

float SpecRefl(float3 normal, float3 dirToLight, float3 toCam, float roughness)
{
	float3 r = reflect(dirToLight, normal);
	float3 v = toCam;

	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	//return pow(saturate(dot(r, v)), specExponent);

	return roughness == 1 ? 0.0f : pow(max(dot(v, r), 0), (1 - roughness) * MAX_SPECULAR_EXPONENT);
}

float3 DirLight(Light directLight, float3 normal, float3 cameraPos, float3 worldPos, float3 surfaceColor, float roughness, float specScale)
{
	float3 dirToLight = normalize(-directLight.Direction);
	float3 toCam = normalize(cameraPos - worldPos);

	float3 diffuse = CalcDiffuse(normal, dirToLight);
	float3 spec = SpecRefl(normal, directLight.Direction, toCam, roughness) * specScale;

	return (diffuse * surfaceColor + spec) * directLight.Intensity * directLight.Color;
}

float Attenuate(Light pointLight, float3 worldPos)
{
	float dist = distance(pointLight.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (pointLight.Range * pointLight.Range)));
	return att * att;
}

float3 PointLight(Light pointLight, float3 normal, float3 cameraPos, float3 worldPos, float3 surfaceColor, float roughness, float specScale)
{
	float3 dirToLight = normalize(pointLight.Position - worldPos);
	float3 toCam = normalize(cameraPos - worldPos);

	float3 diffuse = CalcDiffuse(normal, dirToLight);
	float spec = SpecRefl(normal, dirToLight, toCam, roughness) * specScale;
	float att = Attenuate(pointLight, worldPos);

	return (diffuse * surfaceColor + spec) * pointLight.Intensity * pointLight.Color * att;
}

#endif