#pragma once
#include "SceneNode.h"
#include "ResourceManager.h"
#include "DDSTextureLoader.h"
#include <fstream>

struct TerrainVertex
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT2 TexCoord;
	XMFLOAT2 BlendMapTexCoord;
};

class TerrainNode : public SceneNode
{
public:
	TerrainNode(wstring name, wstring heightMapFilename, int numberOfRows, int numberOfColumns, int worldHeight, int spacing);
	~TerrainNode();

	bool Initialise();
	void Render();
	void Shutdown() {}
	float GetHeightAtPoint(float x, float z);

private:
	unsigned int					_numberOfXPoints;
	unsigned int					_numberOfZPoints;
	unsigned int					_numberOfPolygons;
	unsigned int					_numberOfVertices;
	unsigned int					_numberOfIndices;

	int								_numberOfRows;
	int								_numberOfColumns;
	int								_worldHeight;
	int								_spacing;

	float							_terrainStartX;
	float							_terrainStartZ;
	float							_terrainEndX;
	float							_terrainEndZ;

	wstring							_heightMapFilename;

	vector<float>					_heightValues;

	vector<TerrainVertex>			_vertices;
	vector<UINT>					_indices;

	XMFLOAT4						_ambientLight;
	XMFLOAT4						_directionalLightVector;
	XMFLOAT4						_directionalLightColour;
	XMFLOAT4						_cameraPosition;

	ComPtr<ID3D11Device>			_device;
	ComPtr<ID3D11DeviceContext>		_deviceContext;

	ComPtr<ID3D11Buffer>			_vertexBuffer;
	ComPtr<ID3D11Buffer>			_indexBuffer;

	ComPtr<ID3DBlob>				_vertexShaderByteCode = nullptr;
	ComPtr<ID3DBlob>				_pixelShaderByteCode = nullptr;
	ComPtr<ID3D11VertexShader>		_vertexShader;
	ComPtr<ID3D11PixelShader>		_pixelShader;
	ComPtr<ID3D11InputLayout>		_layout;
	ComPtr<ID3D11Buffer>			_constantBuffer;

	ComPtr<ID3D11RasterizerState>	_defaultRasteriserState;
	ComPtr<ID3D11RasterizerState>	_wireframeRasteriserState;

	ComPtr<ID3D11ShaderResourceView> _texturesResourceView;
	ComPtr<ID3D11ShaderResourceView> _blendMapResourceView;

	void AddToVertexNormal(int z, int x, unsigned int vertexNumber, XMVECTOR normal);
	void GenerateVerticesAndIndices();
	void GenerateNormals();
	void GenerateBuffers();
	void BuildShaders();
	void BuildVertexLayout();
	void BuildConstantBuffer();
	void BuildRendererStates();
	void LoadTerrainTextures();
	void GenerateBlendMap();
	bool LoadHeightMap(wstring heightMapFilename);
};