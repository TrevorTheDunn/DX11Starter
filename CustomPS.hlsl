cbuffer ExternalData : register(b0)
{
	float4 colorTint;
	float deltaTime;
	float2 viewportResolution;
}

// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 screenPosition	: SV_POSITION;
	//float4 color			: COLOR;
	float3 normal			: NORMAL;
	float2 uv				: TEXCOORD;
};

float3 hash23(float2 p) {
	float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yxz + 33.33);
	return frac((p3.xxy + p3.yzz) * p3.zyx);
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
	const float PI = acos(-1.0);

	float2 uv = (2.0 * input.screenPosition - viewportResolution.xy) / viewportResolution.y;
	uv *= 5.0;
	float e = 35.0 / viewportResolution.y;

	float t = deltaTime * 6.0;
	float a = PI / 3.0 * floor(t / (2.0 * PI));
	matrix<float, 2, 2> mat2 = { cos(a), -sin(a), sin(a), cos(a) };
	float2 huv = mul(uv, mat2);

	huv.y /= 0.866;
	huv -= 0.5;
	float v = ceil(huv.y);
	huv.x += v * 0.5;
	huv.x += (1.0 - cos(t / 2.0)) * (fmod(v,2.0) - 0.5);

	float2 hid = floor(huv);
	huv = 2.0 * frac(huv) - 1.0;
	huv.y *= 0.866;

	float d = smoothstep(e, -e, length(huv) - 0.6);
	float3 col = lerp(float3(1.0,1.0,1.0), hash23(hid), d);

	col = pow(col, float3(0.4545, 0.4545, 0.4545));

	return float4(col, 1.0);
}