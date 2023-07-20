#include "TerrainNode.h"
#include "DirectXFramework.h"

struct CBUFFER
{
	XMMATRIX	CompleteTransformation;
	XMMATRIX	WorldTransformation;
	XMFLOAT4	CameraPosition;
	XMVECTOR	LightVector;
	XMFLOAT4	LightColour;
	XMFLOAT4	AmbientColour;
	XMFLOAT4	DiffuseColour;
	XMFLOAT4	SpecularColour;
	float		Shininess;
	float		Opacity;
	float		Padding[2];
};

TerrainNode::TerrainNode(wstring name, wstring heightMapFilename, int numberOfRows, int numberOfColumns, int worldHeight, int spacing) : SceneNode(name)
{
	_heightMapFilename = heightMapFilename;
	_numberOfRows = numberOfRows;
	_numberOfColumns = numberOfColumns;
	_worldHeight = worldHeight;
	_spacing = spacing;
	
	_numberOfXPoints = _numberOfColumns + 1;
	_numberOfZPoints = _numberOfRows + 1;
	_numberOfPolygons = _numberOfColumns * _numberOfRows * 2;
	_numberOfVertices = _numberOfPolygons * 2;
}

TerrainNode::~TerrainNode()
{
}

bool TerrainNode::Initialise()
{
	_device = DirectXFramework::GetDXFramework()->GetDevice();
	_deviceContext = DirectXFramework::GetDXFramework()->GetDeviceContext();
	LoadHeightMap(_heightMapFilename);
	GenerateVerticesAndIndices();
	GenerateNormals();
	LoadTerrainTextures();
	GenerateBlendMap();
	GenerateBuffers();
	BuildShaders();
	BuildVertexLayout();
	BuildConstantBuffer();
	BuildRendererStates();
	return true;
}

void TerrainNode::Render()
{
	XMMATRIX projectionTransformation = DirectXFramework::GetDXFramework()->GetProjectionTransformation();
	XMMATRIX viewTransformation = DirectXFramework::GetDXFramework()->GetCamera()->GetViewMatrix();

	XMMATRIX completeTransformation = XMLoadFloat4x4(&_worldTransformation) * viewTransformation * projectionTransformation;

	CBUFFER cBuffer;
	cBuffer.CompleteTransformation = completeTransformation;
	cBuffer.WorldTransformation = XMLoadFloat4x4(&_worldTransformation);
	cBuffer.AmbientColour = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	cBuffer.LightVector = XMVector4Normalize(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
	cBuffer.LightColour = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cBuffer.DiffuseColour = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // Added these extra cBuffer things
	cBuffer.SpecularColour = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f);
	cBuffer.Shininess = 1.0f;
	cBuffer.Opacity = 1.0f;
	//XMStoreFloat4(&cBuffer.CameraPosition, DirectXFramework::GetDXFramework()->GetCamera()->GetCameraPosition());
	//cBuffer.CameraPosition = DirectXFramework::GetDXFramework()->GetCamera()->GetCameraPosition();

	_deviceContext->VSSetShader(_vertexShader.Get(), 0, 0);
	_deviceContext->PSSetShader(_pixelShader.Get(), 0, 0);
	_deviceContext->IASetInputLayout(_layout.Get());

	UINT stride = sizeof(TerrainVertex);
	UINT offset = 0;
	_deviceContext->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_deviceContext->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	
	// Update the constant buffer
	_deviceContext->UpdateSubresource(_constantBuffer.Get(), 0, 0, &cBuffer, 0, 0);
	_deviceContext->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceContext->PSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_deviceContext->PSSetShaderResources(0, 1, _blendMapResourceView.GetAddressOf());
	_deviceContext->PSSetShaderResources(1, 1, _texturesResourceView.GetAddressOf());
	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->DrawIndexed(_numberOfIndices, 0, 0);
}

void TerrainNode::AddToVertexNormal(int z, int x, unsigned int vertexNumber, XMVECTOR normal)
{
	if (z > 0 && x > 0 && z < (int)_numberOfRows && x < (int)_numberOfColumns)
	{
		unsigned int vertexIndex = (z * _numberOfColumns + x) * 4 + vertexNumber;
		XMStoreFloat3(&_vertices[vertexIndex].Normal, XMVectorAdd(XMLoadFloat3(&_vertices[vertexIndex].Normal), normal));
	}
}

void TerrainNode::GenerateVerticesAndIndices()
{
	_numberOfIndices = _numberOfPolygons * 3;

	float width = (float)(_numberOfXPoints * _spacing);
	float depth = (float)(_numberOfZPoints * _spacing);

	float xOffset = width * -0.5f;
	float zOffset = depth * 0.5f;

	_terrainStartX = xOffset;
	_terrainStartZ = zOffset;
	_terrainEndX = xOffset + width - 1;
	_terrainEndZ = zOffset - depth + 1;

	float du = 1.0f / (_numberOfXPoints - 1);
	float dv = 1.0f / (_numberOfZPoints - 1);

	unsigned int vertexIndex = 0;

	for (int z = 0; z < _numberOfRows; z++)
	{
		for (int x = 0; x < _numberOfColumns; x++)
		{
			float startUV = (float)(rand() % 50) / 100.0f;
			float endUV = startUV + 0.2f + (float)(rand() % 30) / 100.0f;
			// Top left corner vertex
			TerrainVertex vertex;
			vertex.Position = XMFLOAT3(x * _spacing + xOffset, _heightValues[z * _numberOfXPoints + x] * _worldHeight, (-z + 1) * _spacing + zOffset);
			vertex.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = XMFLOAT2(startUV, endUV);
			vertex.BlendMapTexCoord = XMFLOAT2(du * x, dv * z);
			_vertices.push_back(vertex);

			// Top right corner vertex
			vertex.Position = XMFLOAT3((x + 1) * _spacing + xOffset, _heightValues[z * _numberOfXPoints + (x + 1)] * _worldHeight, (-z + 1) * _spacing + zOffset);
			vertex.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = XMFLOAT2(endUV, endUV);
			vertex.BlendMapTexCoord = XMFLOAT2(du * (x + 1), dv * z);
			_vertices.push_back(vertex);

			// Bottom left corner vertex
			vertex.Position = XMFLOAT3(x * _spacing + xOffset, _heightValues[(z + 1) * _numberOfXPoints + x] * _worldHeight, -z * _spacing + zOffset);
			vertex.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = XMFLOAT2(startUV, startUV);
			vertex.BlendMapTexCoord = XMFLOAT2(du * x, dv * (z + 1));
			_vertices.push_back(vertex);

			// Bottom right corner vertex
			vertex.Position = XMFLOAT3((x + 1) * _spacing + xOffset, _heightValues[(z + 1) * _numberOfXPoints + (x + 1)] * _worldHeight, -z * _spacing + zOffset);
			vertex.Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = XMFLOAT2(endUV, startUV);
			vertex.BlendMapTexCoord = XMFLOAT2(du * (x + 1), dv * (z + 1));
			_vertices.push_back(vertex);
			
			// First triangle
			_indices.push_back(vertexIndex);
			_indices.push_back(vertexIndex + 1);
			_indices.push_back(vertexIndex + 2);
			// Second triangle
			_indices.push_back(vertexIndex + 2);
			_indices.push_back(vertexIndex + 1);
			_indices.push_back(vertexIndex + 3);
			vertexIndex = vertexIndex + 4;
		}
	}
}

void TerrainNode::GenerateNormals()
{
	unsigned int vertexIndex = 0;

	for (int z = 0; z < _numberOfRows; z++)
	{
		for (int x = 0; x < _numberOfColumns; x++)
		{
			int index0 = vertexIndex;
			int index1 = vertexIndex + 1;
			int index2 = vertexIndex + 2;
			int index3 = vertexIndex + 3;
			vertexIndex = vertexIndex + 4;

			XMVECTOR u = XMVectorSet(_vertices[index1].Position.x - _vertices[index0].Position.x,
									 _vertices[index1].Position.y - _vertices[index0].Position.y,
									 _vertices[index1].Position.z - _vertices[index0].Position.z,
									 0.0f);
			XMVECTOR v = XMVectorSet(_vertices[index2].Position.x - _vertices[index0].Position.x,
									 _vertices[index2].Position.y - _vertices[index0].Position.y,
									 _vertices[index2].Position.z - _vertices[index0].Position.z,
									 0.0f);
			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(u, v));

			XMStoreFloat3(&_vertices[index0].Normal, XMVectorAdd(XMLoadFloat3(&_vertices[index0].Normal), normal));
			XMStoreFloat3(&_vertices[index1].Normal, XMVectorAdd(XMLoadFloat3(&_vertices[index1].Normal), normal));
			XMStoreFloat3(&_vertices[index2].Normal, XMVectorAdd(XMLoadFloat3(&_vertices[index2].Normal), normal));
			XMStoreFloat3(&_vertices[index3].Normal, XMVectorAdd(XMLoadFloat3(&_vertices[index3].Normal), normal));

			AddToVertexNormal(z - 1, x - 1, 3, normal);
			AddToVertexNormal(z - 1, x, 2, normal);
			AddToVertexNormal(z - 1, x, 3, normal);
			AddToVertexNormal(z - 1, x + 1, 2, normal);
			AddToVertexNormal(z, x - 1, 1, normal);
			AddToVertexNormal(z, x - 1, 3, normal);
			AddToVertexNormal(z, x + 1, 0, normal);
			AddToVertexNormal(z, x + 1, 2, normal);
			AddToVertexNormal(z + 1, x - 1, 1, normal);
			AddToVertexNormal(z + 1, x, 0, normal);
			AddToVertexNormal(z + 1, x, 1, normal);
			AddToVertexNormal(z + 1, x + 1, 0, normal);
		}
	}

	// Normalise vertex normals
	for (unsigned int i = 0; i < _numberOfVertices; i++)
	{
		XMVECTOR vertexNormal = XMLoadFloat3(&_vertices[i].Normal);
		XMStoreFloat3(&_vertices[i].Normal, XMVector3Normalize(vertexNormal));
	}
}

void TerrainNode::GenerateBuffers()
{
	D3D11_BUFFER_DESC vertexBufferDescriptor;
	vertexBufferDescriptor.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDescriptor.ByteWidth = sizeof(VERTEX) * _numberOfVertices; // Changed to variable
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
	indexBufferDescriptor.ByteWidth = sizeof(UINT) * _numberOfIndices; // Changed to variable
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

void TerrainNode::BuildShaders() // Taken from MeshRenderer
{
	DWORD shaderCompileFlags = 0;
#if defined( _DEBUG )
	shaderCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ComPtr<ID3DBlob> compilationMessages = nullptr;

	//Compile vertex shader
	HRESULT hr = D3DCompileFromFile(L"TerrainShaders.hlsl",
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
	hr = D3DCompileFromFile(L"TerrainShaders.hlsl",
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

void TerrainNode::BuildVertexLayout() // Taken from MeshRenderer
{
	// Create the vertex input layout. This tells DirectX the format
	// of each of the vertices we are sending to it.

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ThrowIfFailed(_device->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), _vertexShaderByteCode->GetBufferPointer(), _vertexShaderByteCode->GetBufferSize(), _layout.GetAddressOf()));
}

void TerrainNode::BuildConstantBuffer() // Taken from TexturedCubeNode
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(CBUFFER);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	ThrowIfFailed(_device->CreateBuffer(&bufferDesc, NULL, _constantBuffer.GetAddressOf()));
}

void TerrainNode::BuildRendererStates()
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
	rasteriserDesc.FillMode = D3D11_FILL_WIREFRAME;
	ThrowIfFailed(_device->CreateRasterizerState(&rasteriserDesc, _wireframeRasteriserState.GetAddressOf()));
}

void TerrainNode::LoadTerrainTextures()
{
	// Change the paths below as appropriate for your use
	wstring terrainTextureNames[5] = { L"terrain\\grass.dds", L"terrain\\darkdirt.dds", L"terrain\\stone.dds", L"terrain\\lightdirt.dds", L"terrain\\snow.dds" };

	// Load the textures from the files
	ComPtr<ID3D11Resource> terrainTextures[5];
	for (int i = 0; i < 5; i++)
	{
		ThrowIfFailed(CreateDDSTextureFromFileEx(_device.Get(),
												 _deviceContext.Get(),
												 terrainTextureNames[i].c_str(),
												 0,
												 D3D11_USAGE_IMMUTABLE,
												 D3D11_BIND_SHADER_RESOURCE,
												 0,
												 0,
												 false,
												 terrainTextures[i].GetAddressOf(),
												 nullptr
		));
	}
	// Now create the Texture2D arrary.  We assume all textures in the
	// array have the same format and dimensions

	D3D11_TEXTURE2D_DESC textureDescription;
	ComPtr<ID3D11Texture2D> textureInterface;
	terrainTextures[0].As<ID3D11Texture2D>(&textureInterface);
	textureInterface->GetDesc(&textureDescription);

	D3D11_TEXTURE2D_DESC textureArrayDescription;
	textureArrayDescription.Width = textureDescription.Width;
	textureArrayDescription.Height = textureDescription.Height;
	textureArrayDescription.MipLevels = textureDescription.MipLevels;
	textureArrayDescription.ArraySize = 5;
	textureArrayDescription.Format = textureDescription.Format;
	textureArrayDescription.SampleDesc.Count = 1;
	textureArrayDescription.SampleDesc.Quality = 0;
	textureArrayDescription.Usage = D3D11_USAGE_DEFAULT;
	textureArrayDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureArrayDescription.CPUAccessFlags = 0;
	textureArrayDescription.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> textureArray = 0;
	ThrowIfFailed(_device->CreateTexture2D(&textureArrayDescription, 0, textureArray.GetAddressOf()));

	// Copy individual texture elements into texture array.

	for (UINT i = 0; i < 5; i++)
	{
		// For each mipmap level...
		for (UINT mipLevel = 0; mipLevel < textureDescription.MipLevels; mipLevel++)
		{
			_deviceContext->CopySubresourceRegion(textureArray.Get(),
												  D3D11CalcSubresource(mipLevel, i, textureDescription.MipLevels),
												  NULL,
												  NULL,
												  NULL,
												  terrainTextures[i].Get(),
												  mipLevel,
												  nullptr
			);
		}
	}

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
	viewDescription.Format = textureArrayDescription.Format;
	viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDescription.Texture2DArray.MostDetailedMip = 0;
	viewDescription.Texture2DArray.MipLevels = textureArrayDescription.MipLevels;
	viewDescription.Texture2DArray.FirstArraySlice = 0;
	viewDescription.Texture2DArray.ArraySize = 5;

	ThrowIfFailed(_device->CreateShaderResourceView(textureArray.Get(), &viewDescription, _texturesResourceView.GetAddressOf()));
}

void TerrainNode::GenerateBlendMap()
{
	// Note that _numberOfRows and _numberOfColumns need to be setup
	// to the number of rows and columns in your grid in order for this
	// to work.
	DWORD* blendMap = new DWORD[_numberOfRows * _numberOfColumns];
	DWORD* blendMapPtr = blendMap;
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;

	DWORD index = 0;
	for (DWORD i = 0; i < _numberOfColumns; i++)
	{
		for (DWORD j = 0; j < _numberOfRows; j++)
		{
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			
			// Calculates the appropriate blend colour for the current location in the blend map.
			float y = 0.0f;
			for (int k = 0; k < 6; k++)
			{
				y += _vertices[_indices[index]].Position.y;
				index++;
			}		
			y = y / 6.0f;

			if (y < 200.0f)
			{
				b = (BYTE)200.0f - y;
				r = 200 - b;
			}
			if (y >= 200.0f && y <= 400.0f)
			{
				r = (BYTE)(-y + 400.0f);
			}
			if (y >= 650.0f)
			{
				if (y < 900.0f)
				{
					g = (BYTE)(y - 650);
				}
			}

			DWORD mapValue = (a << 24) + (b << 16) + (g << 8) + r;
			*blendMapPtr++ = mapValue;
		}
	}
	D3D11_TEXTURE2D_DESC blendMapDescription;
	blendMapDescription.Width = _numberOfRows;
	blendMapDescription.Height = _numberOfColumns;
	blendMapDescription.MipLevels = 1;
	blendMapDescription.ArraySize = 1;
	blendMapDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	blendMapDescription.SampleDesc.Count = 1;
	blendMapDescription.SampleDesc.Quality = 0;
	blendMapDescription.Usage = D3D11_USAGE_DEFAULT;
	blendMapDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	blendMapDescription.CPUAccessFlags = 0;
	blendMapDescription.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA blendMapInitialisationData;
	blendMapInitialisationData.pSysMem = blendMap;
	blendMapInitialisationData.SysMemPitch = 4 * _numberOfColumns;

	ComPtr<ID3D11Texture2D> blendMapTexture;
	ThrowIfFailed(_device->CreateTexture2D(&blendMapDescription, &blendMapInitialisationData, blendMapTexture.GetAddressOf()));

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
	viewDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDescription.Texture2D.MostDetailedMip = 0;
	viewDescription.Texture2D.MipLevels = 1;

	ThrowIfFailed(_device->CreateShaderResourceView(blendMapTexture.Get(), &viewDescription, _blendMapResourceView.GetAddressOf()));
	delete[] blendMap;
}

bool TerrainNode::LoadHeightMap(wstring heightMapFilename)
{
	unsigned int mapSize = _numberOfXPoints * _numberOfZPoints;
	USHORT* rawFileValues = new USHORT[mapSize];

	std::ifstream inputHeightMap;
	inputHeightMap.open(heightMapFilename.c_str(), std::ios_base::binary);
	if (!inputHeightMap)
	{
		return false;
	}

	inputHeightMap.read((char*)rawFileValues, mapSize * 2);
	inputHeightMap.close();

	// Normalise BYTE values to the range 0.0f - 1.0f;
	for (unsigned int i = 0; i < mapSize; i++)
	{
		_heightValues.push_back((float)rawFileValues[i] / 65536);
	}
	delete[] rawFileValues;
	return true;
}

float TerrainNode::GetHeightAtPoint(float x, float z)
{
	int indexBase;

	int cellX = (int)((x - _terrainStartX) / _spacing);
	int cellZ = (int)((_terrainStartZ - z) / _spacing);

	int verticesIndex = (cellZ * _numberOfColumns * 4) + (cellX * 4);
	float dx = x - _vertices[verticesIndex].Position.x;
	float dz = z - _vertices[verticesIndex].Position.z;

	if (dz > dx)
	{
		indexBase = (cellZ * _numberOfColumns + cellX) * 6;
	}
	else
	{
		indexBase = (cellZ * _numberOfColumns + cellX) * 6 + 3;
	}
	int index0 = _indices[indexBase];
	int index1 = _indices[indexBase + 1];
	int index2 = _indices[indexBase + 2];

	// Taken from normals calculations method
	XMVECTOR u = XMVectorSet(_vertices[index1].Position.x - _vertices[index0].Position.x,
							 _vertices[index1].Position.y - _vertices[index0].Position.y,
							 _vertices[index1].Position.z - _vertices[index0].Position.z,
							 0.0f);
	XMVECTOR v = XMVectorSet(_vertices[index2].Position.x - _vertices[index0].Position.x,
							 _vertices[index2].Position.y - _vertices[index0].Position.y,
							 _vertices[index2].Position.z - _vertices[index0].Position.z,
							 0.0f);
	XMFLOAT4 normal;
	XMStoreFloat4(&normal, XMVector3Normalize(XMVector3Cross(u, v)));

	float y = _vertices[verticesIndex].Position.y + (normal.x * dx + normal.y * dz) / -normal.y;
	return y;
}
