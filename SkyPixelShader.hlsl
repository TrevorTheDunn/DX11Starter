
TextureCube CubeMap			:	register(t0);
SamplerState BasicSampler	:	register(s0);


struct VertexToPixel
{
	float4 position				: SV_POSITION;
	float3 sampleDir			: DIRECTION;
};

float4 main(VertexToPixel input) : SV_TARGET
{
	//sample the cube map in the correct direction and return the result
	return CubeMap.Sample(BasicSampler, input.sampleDir);
}