
cbuffer ConstantBuffer 
{
	matrix worldViewProj;	// The complete transformation
	matrix world;			// The world transformation matrix
	float4 lightVector;     // The directional light's vector
	float4 lightColour;     // The directional light's colour
	float4 ambientColour;   // The ambient light's colour
};

Texture2D Texture;
SamplerState ss;

struct VertexIn
{
	float3 Position : POSITION;
	float3 Normal   : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 Position  : SV_POSITION;
    float4 Colour	 : COLOUR;
	float2 TexCoord	 : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.Position = mul(worldViewProj, float4(vin.Position, 1.0f));
	
	// calculate the diffuse light and add it to the ambient light
	float4 vectorBackToLight = -lightVector;
	float4 adjustedNormal = normalize(mul(world, float4(vin.Normal, 1.0f)));
	float diffusebrightness = saturate(dot(adjustedNormal, vectorBackToLight));
	vout.Colour = saturate(ambientColour + lightColour * diffusebrightness);

	vout.TexCoord = vin.TexCoord;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.Colour * Texture.Sample(ss, pin.TexCoord);
}


