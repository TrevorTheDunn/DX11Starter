struct VertexShaderInput
{
	float3 localPosition		: POSITION;
	float3 normal				: NORMAL;
	float2 uv					: TEXCOORD;
	float3 tangent				: TANGENT;
};

struct VertexToPixel
{
	float4 position				: SV_POSITION;
	float3 sampleDir			: DIRECTION;
};

cbuffer ExternalData : register(b0)
{
	matrix view;
	matrix proj;
}

VertexToPixel main( VertexShaderInput input )
{
	//return variable
	VertexToPixel output;

	//copy of view matrix, translation to all zeros
	matrix viewCopy = view;
	viewCopy._14 = 0;
	viewCopy._24 = 0;
	viewCopy._34 = 0;

	//apply projection and view to input position, saving to output
	matrix vp = mul(proj, viewCopy);
	output.position = mul(vp, float4(input.localPosition, 1.0f));
	
	//Ensure that the depth will be exactly 1.0
	output.position.z = output.position.w;

	//figure out sample direction of the vertex from the center of the object
	output.sampleDir = input.localPosition;

	//return output variable
	return output;
}