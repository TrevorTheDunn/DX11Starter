#include "ShaderInclude.hlsli"

Texture2D Albedo						:	register(t0);	//"t" registers for textures
Texture2D NormalMap						:	register(t1);
Texture2D RoughnessMap					:	register(t2);
Texture2D MetalnessMap					:	register(t3);
Texture2D ShadowMap						:	register(t4);	// Adjust index as necessary
SamplerState BasicSampler				:	register(s0);	//"s" registers for samplers
SamplerComparisonState ShadowSampler	:	register(s1);

cbuffer ExternalData : register(b0)
{
	float3 colorTint;
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
	/*float3 normDir = normalize(-directionalLight1.Direction);
	float3 diffuseAmt = calcDiffuse(input.normal, normDir);

	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;

	float3 V = normalize(cameraPos - input.worldPosition);

	float R = reflect(-normDir, input.normal);

	float spec = pow(saturate(dot(R, V)), specExponent);

	float3 light = colorTint * (diffuseAmt + spec);

	float3 finalColor = (light * directionalLight1.Intensity * directionalLight1.Color) + (ambient * colorTint);*/

	// Perform the perspective divide (divide by w) ourselves
	input.shadowMapPos /= input.shadowMapPos.w;

	// Convert the normalized device coordinates to UVs for sampling
	float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
	shadowUV.y = 1 - shadowUV.y; // Flip the Y

	float distToLight = input.shadowMapPos.z;

	// Get a ratio of comparison results using SampleCmpLevelZero()
	float shadowAmount = ShadowMap.SampleCmpLevelZero(
		ShadowSampler,
		shadowUV,
		distToLight).r;

	float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
	unpackedNormal = normalize(unpackedNormal);	//Don't forget to normalize

	//Feel free to adjust/simplify this code to fit with your existing shader(s)
	//Simplifications include not re-normalizing the same vector more than once!
	float3 N = normalize(input.normal); //Must be normalized here or before
	float3 T = normalize(input.tangent); //Must be normalized here or before
	T = normalize(T - N * dot(T, N)); //Gram-Schmidt assumes T&N are normalized
	float3 B = cross(T, N);
	float3x3 TBN = float3x3(T, B, N);

	//Assumes that input.normal is the normal later in the shader
	input.normal = mul(unpackedNormal, TBN); //Note the multiplication order

	float3 albedoColor = pow(Albedo.Sample(BasicSampler, input.uv).rgb, 2.2f);

	float roughnessPBR = RoughnessMap.Sample(BasicSampler, input.uv).r;

	float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;

	float3 specularColor = lerp(F0_NON_METAL, albedoColor.rgb, metalness);

	float3 finalColor = ambient;

	float3 lightResult;

	for (int i = 0; i < 5; i++)
	{
		switch (lights[i].Type)
		{
			case LIGHT_TYPE_DIRECTIONAL:
				//finalColor += DirLight(lights[i], input.normal, cameraPos, input.worldPosition, albedoColor, roughness, specScale);
				lightResult = DirLightPBR(lights[i], input.normal, cameraPos, input.worldPosition, albedoColor, specularColor, roughnessPBR, metalness);
				if (i == 0) lightResult *= shadowAmount;
				break;

			case LIGHT_TYPE_POINT:
				//finalColor += PointLight(lights[i], input.normal, cameraPos, input.worldPosition, albedoColor, roughness, specScale);
				lightResult = PointLightPBR(lights[i], input.normal, cameraPos, input.worldPosition, albedoColor, specularColor, roughnessPBR, metalness);
				break;
		}

		finalColor += lightResult;
	}

	return float4(pow(finalColor, 1.0f / 2.2f), 1);
}