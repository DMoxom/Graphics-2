cbuffer ConstantBuffer
{
	float4x4 completeTransformation;
};
 
TextureCube cubeMap;

SamplerState samTriLinearSam;

struct VertexIn
{
	float3 Position : POSITION;
};

struct VertexOut
{
	float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};
 
VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.Position = mul(float4(vin.Position, 1.0f), completeTransformation).xyww;
	
	vout.TexCoord = vin.Position;
	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return cubeMap.Sample(samTriLinearSam, pin.TexCoord);
}

