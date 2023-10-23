#include "ShaderInclude.hlsli"

cbuffer ExternalData : register(b0)
{
	float4 colorTint;
	float roughness;
	float3 cameraPos;
	float3 ambient;
	Light directionalLight1;
	Light directionalLight2;
	Light directionalLight3;
	Light pointLight1;
	Light pointLight2;

	Light lights[5];	//Array of exactly 5 lights
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
//struct VertexToPixel
//{
//	// Data type
//	//  |
//	//  |   Name          Semantic
//	//  |    |                |
//	//  v    v                v
//	float4 screenPosition	: SV_POSITION;
//	//float4 color			: COLOR;
//	float3 normal			: NORMAL;
//	float2 uv				: TEXCOORD;
//};

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering

	//return colorTint * ambient;

	input.normal = normalize(input.normal);

	/*input.normal = normalize(input.normal);
	float3 normDir = normalize(-directionalLight1.Direction);
	float3 diffuseAmt = calcDiffuse(input.normal, normDir);

	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	float3 V = normalize(cameraPos - input.worldPosition);

	float R = reflect(-normDir, input.normal);

	float spec = pow(saturate(dot(R, V)), specExponent);

	float3 light = colorTint * (diffuseAmt + spec);

	float3 finalColor = (light * directionalLight1.Intensity * directionalLight1.Color) + (ambient * colorTint);*/

	float3 finalColor = ambient * colorTint;

	for (int i = 0; i < 5; i++)
	{
		switch (lights[i].Type)
		{
			case LIGHT_TYPE_DIRECTIONAL:
				finalColor += DirLight(lights[i], input.normal, cameraPos, input.worldPosition, colorTint, roughness);
				break;

			case LIGHT_TYPE_POINT:
				finalColor += PointLight(lights[i], input.normal, cameraPos, input.worldPosition, colorTint, roughness);
				break;
		}
	}

	//float3 dirLight1Final = DirLight(directionalLight1, input.normal, cameraPos, input.worldPosition, colorTint, roughness);
	//float3 dirLight2Final = DirLight(directionalLight2, input.normal, cameraPos, input.worldPosition, colorTint, roughness);
	//float3 dirLight3Final = DirLight(directionalLight3, input.normal, cameraPos, input.worldPosition, colorTint, roughness);
	//float pointLight1Final = PointLight(pointLight1, input.normal, cameraPos, input.worldPosition, colorTint, roughness);
	//float pointLight2Final = PointLight(pointLight2, input.normal, cameraPos, input.worldPosition, colorTint, roughness);

	//float3 finalColor = dirLight1Final + dirLight2Final + dirLight3Final + pointLight1Final + pointLight2Final;

	//return colorTint * float4(ambient, 1.0);
	//return float4(input.normal, 1);
	//return float4(directionalLight1.Color, 1);
	return float4(finalColor, 1);
}