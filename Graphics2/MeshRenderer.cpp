#include "MeshRenderer.h"
#include "DirectXFramework.h"

struct CBUFFER
{
	XMMATRIX    CompleteTransformation;
	XMMATRIX	WorldTransformation;
	XMFLOAT4	CameraPosition;
	XMVECTOR    LightVector;
	XMFLOAT4    LightColor;
	XMFLOAT4    AmbientColor;
	XMFLOAT4    DiffuseCoefficient;
	XMFLOAT4	SpecularCoefficient;
	float		Shininess;
	float		Opacity;
	float       Padding[2];
};

CBUFFER cBuffer;

void MeshRenderer::SetMesh(shared_ptr<Mesh> mesh)
{
	_mesh = mesh;
}

void MeshRenderer::SetWorldTransformation(FXMMATRIX worldTransformation)
{
	XMStoreFloat4x4(&_worldTransformation, worldTransformation);
}

void MeshRenderer::SetAmbientLight(XMFLOAT4 ambientLight)
{
	_ambientLight = ambientLight;
}

void MeshRenderer::SetDirectionalLight(FXMVECTOR lightVector, XMFLOAT4 lightColour)
{
	_directionalLightColour = lightColour;
	XMStoreFloat4(&_directionalLightVector, lightVector);
}

void MeshRenderer::SetCameraPosition(XMFLOAT4 cameraPosition)
{
	_cameraPosition = cameraPosition;
}

bool MeshRenderer::Initialise()
{
	_device = DirectXFramework::GetDXFramework()->GetDevice();
	_deviceContext = DirectXFramework::GetDXFramework()->GetDeviceContext();
	BuildShaders();
	BuildVertexLayout();
	BuildConstantBuffer();
	BuildBlendState();
	BuildRendererState();
	return true;
}

void MeshRenderer::RenderNode(shared_ptr<Node> node, bool renderTransparent)
{
	unsigned int subMeshCount = (unsigned int)node->GetMeshCount();
	// Loop through all submeshes in the mesh, rendering them
	for (unsigned int i = 0; i < subMeshCount; i++)
	{
		unsigned int meshIndex = node->GetMesh(i);
		shared_ptr<SubMesh> subMesh = _mesh->GetSubMesh(meshIndex);
		shared_ptr<Material> material = subMesh->GetMaterial();
		float opacity = material->GetOpacity();
		if ((renderTransparent && opacity < 1.0f) ||
			(!renderTransparent && opacity == 1.0f))
		{
			UINT stride = sizeof(VERTEX);
			UINT offset = 0;
			_vertexBuffer = subMesh->GetVertexBuffer();
			_deviceContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
			_indexBuffer = subMesh->GetIndexBuffer();
			_deviceContext->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			cBuffer.DiffuseCoefficient = material->GetDiffuseColour();
			cBuffer.SpecularCoefficient = material->GetSpecularColour();
			cBuffer.Shininess = material->GetShininess();
			cBuffer.Opacity = opacity;
			// Update the constant buffer 
			_deviceContext->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
			_deviceContext->UpdateSubresource(_constantBuffer.Get(), 0, 0, &cBuffer, 0, 0);
			_texture = material->GetTexture();
			_deviceContext->PSSetShaderResources(0, 1, _texture.GetAddressOf());
			_deviceContext->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
			_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			_deviceContext->DrawIndexed(static_cast<UINT>(subMesh->GetIndexCount()), 0, 0);
		}
	}
	// Render the children
	unsigned int childrenCount = (unsigned int)node->GetChildrenCount();
	// Loop through all submeshes in the mesh, rendering them
	for (unsigned int i = 0; i < childrenCount; i++)
	{
		RenderNode(node->GetChild(i), renderTransparent);
	}
}

void MeshRenderer::Render()
{
	// Turn off back face culling while we render a mesh. 
	// We do this since ASSIMP does not appear to be setting the
	// TWOSIDED property on materials correctly. Without turning off
	// back face culling, some materials do not render correctly.
	_deviceContext->RSSetState(_noCullRasteriserState.Get());

	XMMATRIX projectionTransformation = DirectXFramework::GetDXFramework()->GetProjectionTransformation();
	XMMATRIX viewTransformation = DirectXFramework::GetDXFramework()->GetCamera()->GetViewMatrix();

	XMMATRIX completeTransformation = XMLoadFloat4x4(&_worldTransformation) * viewTransformation * projectionTransformation;

	// Draw the first cube

	cBuffer.CompleteTransformation = completeTransformation;
	cBuffer.WorldTransformation = XMLoadFloat4x4(&_worldTransformation);
	cBuffer.AmbientColor = _ambientLight;
	cBuffer.LightVector = XMVector4Normalize(XMLoadFloat4(&_directionalLightVector)); 
	cBuffer.LightColor = _directionalLightColour;
	cBuffer.CameraPosition = _cameraPosition;

	_deviceContext->VSSetShader(_vertexShader.Get(), 0, 0);
	_deviceContext->PSSetShader(_pixelShader.Get(), 0, 0);
	_deviceContext->IASetInputLayout(_layout.Get());

	// Set the blend state correctly to handle opacity
	float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f }; 
	_deviceContext->OMSetBlendState(_transparentBlendState.Get(), blendFactors, 0xffffffff);

	// We do two passes through the nodes.  The first time we render nodes
	// that are not transparent (i.e. their opacity == 1.0f).
	RenderNode(_mesh->GetRootNode(), false);
	// Now we render any transparent nodes
	// We have to do this since blending always blends the submesh with
	// whatever is in the render target.  If we render a transparent node
	// first, it will be opaque.
	RenderNode(_mesh->GetRootNode(), true);

	// Turn back face culling back on in case another renderer 
	// relies on it
	_deviceContext->RSSetState(_defaultRasteriserState.Get());
}

void MeshRenderer::Shutdown(void)
{
}

void MeshRenderer::BuildShaders()
{
	DWORD shaderCompileFlags = 0;
#if defined( _DEBUG )
	shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> compilationMessages = nullptr;

	//Compile vertex shader
	HRESULT hr = D3DCompileFromFile(L"TexturedShaders.hlsl",
									nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									"VShader", "vs_5_0",
									shaderCompileFlags, 0,
									_vertexShaderByteCode.GetAddressOf(),
									compilationMessages.GetAddressOf());

	if (compilationMessages.Get() != nullptr)
	{
		// If there were any compilation messages, display them
		MessageBoxA(0, (char*)compilationMessages->GetBufferPointer(), 0, 0);
	}
	// Even if there are no compiler messages, check to make sure there were no other errors.
	ThrowIfFailed(hr);
	ThrowIfFailed(_device->CreateVertexShader(_vertexShaderByteCode->GetBufferPointer(), _vertexShaderByteCode->GetBufferSize(), NULL, _vertexShader.GetAddressOf()));

	// Compile pixel shader
	hr = D3DCompileFromFile(L"TexturedShaders.hlsl",
							nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
							"PShader", "ps_5_0",
							shaderCompileFlags, 0,
							_pixelShaderByteCode.GetAddressOf(),
							compilationMessages.GetAddressOf());

	if (compilationMessages.Get() != nullptr)
	{
		// If there were any compilation messages, display them
		MessageBoxA(0, (char*)compilationMessages->GetBufferPointer(), 0, 0);
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(_device->CreatePixelShader(_pixelShaderByteCode->GetBufferPointer(), _pixelShaderByteCode->GetBufferSize(), NULL, _pixelShader.GetAddressOf()));
}


void MeshRenderer::BuildVertexLayout()
{
	// Create the vertex input layout. This tells DirectX the format
	// of each of the vertices we are sending to it.

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ThrowIfFailed(_device->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), _vertexShaderByteCode->GetBufferPointer(), _vertexShaderByteCode->GetBufferSize(), _layout.GetAddressOf()));
}

void MeshRenderer::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBUFFER);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ThrowIfFailed(_device->CreateBuffer(&bufferDesc, NULL, _constantBuffer.GetAddressOf()));
}

void MeshRenderer::BuildBlendState()
{
	D3D11_BLEND_DESC transparentDesc = { 0 };
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;
	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	ThrowIfFailed(_device->CreateBlendState(&transparentDesc, _transparentBlendState.GetAddressOf()));
}

void MeshRenderer::BuildRendererState()
{
	// Set default and no cull rasteriser states
	D3D11_RASTERIZER_DESC rasteriserDesc;
	rasteriserDesc.FillMode = D3D11_FILL_SOLID;
	rasteriserDesc.CullMode = D3D11_CULL_BACK;
	rasteriserDesc.FrontCounterClockwise = false;
	rasteriserDesc.DepthBias = 0;
	rasteriserDesc.SlopeScaledDepthBias = 0.0f;
	rasteriserDesc.DepthBiasClamp = 0.0f;
	rasteriserDesc.DepthClipEnable = true;
	rasteriserDesc.ScissorEnable = false;
	rasteriserDesc.MultisampleEnable = false;
	rasteriserDesc.AntialiasedLineEnable = false;
	ThrowIfFailed(_device->CreateRasterizerState(&rasteriserDesc, _defaultRasteriserState.GetAddressOf()));
	rasteriserDesc.CullMode = D3D11_CULL_NONE;
	ThrowIfFailed(_device->CreateRasterizerState(&rasteriserDesc, _noCullRasteriserState.GetAddressOf()));
}

