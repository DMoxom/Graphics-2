#include "SkyNode.h"

struct CBUFFER
{
	XMMATRIX CompleteTransformation;
};

SkyNode::SkyNode(wstring name, wstring skyCubeFilename, float skySphereRadius) : SceneNode(name)
{
	_skyCubeFilename = skyCubeFilename;
	_SkySphereRadius = skySphereRadius;
}

SkyNode::~SkyNode()
{
}

bool SkyNode::Initialise()
{
	_device = DirectXFramework::GetDXFramework()->GetDevice();
	_deviceContext = DirectXFramework::GetDXFramework()->GetDeviceContext();
	CreateSphere(_SkySphereRadius, 30);
	GenerateBuffers();
	BuildShaders();
	BuildVertexLayout();
	BuildConstantBuffer();
	BuildRendererStates();
	BuildDepthStencilState();
	LoadSkyBox();
	return true;
}

void SkyNode::Render()
{
	XMMATRIX projectionTransformation = DirectXFramework::GetDXFramework()->GetProjectionTransformation();
	XMMATRIX viewTransformation = DirectXFramework::GetDXFramework()->GetCamera()->GetViewMatrix();

	XMFLOAT4 cameraPosition;
	XMStoreFloat4(&cameraPosition, DirectXFramework::GetDXFramework()->GetCamera()->GetCameraPosition());
	XMMATRIX skyWorldTransformation = XMMatrixTranslation(cameraPosition.x, cameraPosition.y, cameraPosition.z);

	XMMATRIX completeTransformation = skyWorldTransformation * viewTransformation * projectionTransformation;

	CBUFFER cBuffer;
	cBuffer.CompleteTransformation = completeTransformation;

	_deviceContext->VSSetShader(_vertexShader.Get(), 0, 0);
	_deviceContext->PSSetShader(_pixelShader.Get(), 0, 0);
	_deviceContext->IASetInputLayout(_layout.Get());

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	_deviceContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_deviceContext->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	_deviceContext->UpdateSubresource(_constantBuffer.Get(), 0, 0, &cBuffer, 0, 0);
	_deviceContext->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceContext->PSSetShaderResources(0, 1, _skyBoxResourceView.GetAddressOf());
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->RSSetState(_noCullRasteriserState.Get());
	_deviceContext->OMSetDepthStencilState(_stencilState.Get(), 1);
	_deviceContext->DrawIndexed(_numberOfIndices, 0, 0);
	_deviceContext->OMSetDepthStencilState(nullptr, 1);
	_deviceContext->RSSetState(_defaultRasteriserState.Get());
}

void SkyNode::CreateSphere(float radius, size_t tessellation)
{
	_vertices.clear();
	_indices.clear();

	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	// Create rings of vertices at progressively higher latitudes.
	for (size_t i = 0; i <= verticalSegments; i++)
	{
		float v = 1 - (float)i / verticalSegments;

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			float u = (float)j / horizontalSegments;

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			Vertex vertex;
			vertex.Position.x = dx * radius;
			vertex.Position.y = dy * radius;
			vertex.Position.z = dz * radius;
			_vertices.push_back(vertex);
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;

	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			_indices.push_back((UINT)(i * stride + j));
			_indices.push_back((UINT)(nextI * stride + j));
			_indices.push_back((UINT)(i * stride + nextJ));

			_indices.push_back((UINT)(i * stride + nextJ));
			_indices.push_back((UINT)(nextI * stride + j));
			_indices.push_back((UINT)(nextI * stride + nextJ));
		}
	}
	_numberOfVertices = (unsigned int)_vertices.size();
	_numberOfIndices = (unsigned int)_indices.size();
}

void SkyNode::GenerateBuffers()
{
	D3D11_BUFFER_DESC vertexBufferDescriptor;
	vertexBufferDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDescriptor.ByteWidth = sizeof(VERTEX) * _numberOfVertices;
	vertexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDescriptor.CPUAccessFlags = 0;
	vertexBufferDescriptor.MiscFlags = 0;
	vertexBufferDescriptor.StructureByteStride = 0;

	// Now set up a structure that tells DirectX where to get the
	// data for the vertices from
	D3D11_SUBRESOURCE_DATA vertexInitialisationData;
	vertexInitialisationData.pSysMem = &_vertices[0]; // From vector, values set in GenerateVerticesAndIndices

	// and create the vertex buffer
	ThrowIfFailed(_device->CreateBuffer(&vertexBufferDescriptor, &vertexInitialisationData, _vertexBuffer.GetAddressOf()));

	D3D11_BUFFER_DESC indexBufferDescriptor;
	indexBufferDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDescriptor.ByteWidth = sizeof(UINT) * _numberOfIndices;
	indexBufferDescriptor.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	indexBufferDescriptor.CPUAccessFlags = 0;
	indexBufferDescriptor.MiscFlags = 0;
	indexBufferDescriptor.StructureByteStride = 0;

	// Now set up a structure that tells DirectX where to get the
	// data for the vertices from
	D3D11_SUBRESOURCE_DATA indexInitialisationData;
	indexInitialisationData.pSysMem = &_indices[0]; // From vector, values set in GenerateVerticesAndIndices

	// and create the vertex buffer
	ThrowIfFailed(_device->CreateBuffer(&indexBufferDescriptor, &indexInitialisationData, _indexBuffer.GetAddressOf()));
}

void SkyNode::BuildShaders()
{
	DWORD shaderCompileFlags = 0;
#if defined( _DEBUG )
	shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> compilationMessages = nullptr;

	//Compile vertex shader
	HRESULT hr = D3DCompileFromFile(L"SkyShader.hlsl",
									nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
									"VS", "vs_5_0",
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
	hr = D3DCompileFromFile(L"SkyShader.hlsl",
							nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
							"PS", "ps_5_0",
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

void SkyNode::BuildVertexLayout()
{
	// Create the vertex input layout. This tells DirectX the format
// of each of the vertices we are sending to it.

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ThrowIfFailed(_device->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), _vertexShaderByteCode->GetBufferPointer(), _vertexShaderByteCode->GetBufferSize(), _layout.GetAddressOf()));
}

void SkyNode::BuildConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBUFFER);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	ThrowIfFailed(_device->CreateBuffer(&bufferDesc, NULL, _constantBuffer.GetAddressOf()));
}

void SkyNode::BuildRendererStates()
{
	// Set default and wireframe rasteriser states
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

void SkyNode::BuildDepthStencilState()
{
	D3D11_DEPTH_STENCIL_DESC stencilDesc;
	stencilDesc.DepthEnable = true;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	stencilDesc.StencilEnable = false;
	stencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	stencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	ThrowIfFailed(_device->CreateDepthStencilState(&stencilDesc, _stencilState.GetAddressOf()));
}

void SkyNode::LoadSkyBox()
{
	ThrowIfFailed(CreateDDSTextureFromFile(_device.Get(),
										   _skyCubeFilename.c_str(),
										   nullptr,
										   _skyBoxResourceView.GetAddressOf()
	));
}
