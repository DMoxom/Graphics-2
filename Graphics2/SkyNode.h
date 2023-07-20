#pragma once
#include "SceneNode.h"
#include "DirectXFramework.h"
#include "Core.h"
#include "DirectXCore.h"
#include "DDSTextureLoader.h"

struct Vertex
{
	XMFLOAT3 Position;
};

class SkyNode : public SceneNode
{
public:
	SkyNode(wstring name, wstring skyCubeFilename, float skySphereRadius);
	~SkyNode();

	bool Initialise();
	void Render();
	void Shutdown() {}
private:
	wstring							_skyCubeFilename;
	float							_SkySphereRadius;


	vector<Vertex>					_vertices;
	vector<UINT>					_indices;

	unsigned int					_numberOfVertices;
	unsigned int					_numberOfIndices;

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

	ComPtr<ID3D11RasterizerState>   _defaultRasteriserState;
	ComPtr<ID3D11RasterizerState>   _noCullRasteriserState;

	ComPtr<ID3D11ShaderResourceView> _skyBoxResourceView;

	ComPtr<ID3D11DepthStencilState>  _stencilState;

	void CreateSphere(float radius, size_t tessellation);
	void GenerateBuffers();
	void BuildShaders();
	void BuildVertexLayout();
	void BuildConstantBuffer();
	void BuildRendererStates();
	void BuildDepthStencilState();
	void LoadSkyBox();
};