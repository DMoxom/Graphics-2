cbuffer ConstantBuffer
{
    float4x4 completeTransformation;
    float4x4 worldTransformation;    
	float4 cameraPosition;
    float4 lightVector;			// the light's vector
    float4 lightColor;			// the light's color
    float4 ambientColor;		// the ambient light's color
    float4 diffuseCoefficient;	// The diffuse reflection cooefficient
	float4 specularCoefficient;	// The specular reflection cooefficient
	float  shininess;			// The shininess factor
	float  opacity;				// The opacity (transparency) of the material. 0 = fully transparent, 1 = fully opaque
	float2 padding;
}

Texture2D Texture;
SamplerState ss;

struct VertexShaderInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD;
};

struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float4 PositionWS: TEXCOORD1;
	float4 NormalWS : TEXCOORD2;
	float2 TexCoord : TEXCOORD0;
};

PixelShaderInput VShader(VertexShaderInput vin)
{
    PixelShaderInput output;

	output.Position = mul(completeTransformation, float4(vin.Position, 1.0f));
	output.PositionWS = mul(worldTransformation, float4(vin.Position, 1.0f));
	output.NormalWS = float4(mul((float3x3)worldTransformation, vin.Normal), 1.0f);
	output.TexCoord = vin.TexCoord;
	return output;
}

float4 PShader(PixelShaderInput input) : SV_TARGET
{
	float4 viewDirection = normalize(cameraPosition - input.PositionWS);
	float4 directionToLight = normalize(-lightVector);

	// Calculate diffuse lighting
	float4 adjustedNormal = normalize(input.NormalWS);
	float NdotL = max(0, dot(adjustedNormal, directionToLight));
	float4 diffuse = saturate(lightColor * NdotL * diffuseCoefficient);

	// Calculate specular component
	float4 R = 2 * NdotL * adjustedNormal - directionToLight;
	float RdotV = max(0, dot(R, viewDirection));
	float4 specular = saturate(lightColor * pow(RdotV, shininess) * specularCoefficient);

	// Calculate ambient lighting
	float4 ambientLight = ambientColor * diffuseCoefficient;

	// Combine all components
	float4 color = saturate((ambientLight + diffuse + specular) * Texture.Sample(ss, input.TexCoord));
	if (opacity < 1.0f)
	{
		color.a = opacity;
	}
	return color;
}