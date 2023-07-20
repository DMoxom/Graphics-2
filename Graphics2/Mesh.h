#pragma once
#include "core.h"
#include "DirectXCore.h"
#include <vector>

// Core material class.  Ideally, this should be extended to include more material attributes that can be
// recovered from Assimp, but this handles the basics.

class Material
{
public:
	Material(wstring materialName, XMFLOAT4 diffuseColour, XMFLOAT4 specularColour, float shininess, float opacity, ComPtr<ID3D11ShaderResourceView> texture );
	~Material();

	inline wstring							GetMaterialName() { return _materialName;  }
	inline XMFLOAT4							GetDiffuseColour() { return _diffuseColour; }
	inline XMFLOAT4							GetSpecularColour() { return _specularColour; }
	inline float							GetShininess() { return _shininess; }
	inline float							GetOpacity() { return _opacity; }
	inline ComPtr<ID3D11ShaderResourceView>	GetTexture() { return _texture; }

private:
	wstring									_materialName;
	XMFLOAT4								_diffuseColour;
	XMFLOAT4								_specularColour;
	float									_shininess;
	float									_opacity;
    ComPtr<ID3D11ShaderResourceView>		_texture;
};

// Basic SubMesh class.  A Mesh consists of one or more sub-meshes.  The submesh provides everything that is needed to
// draw the sub-mesh.

class SubMesh
{
public:
	SubMesh(ComPtr<ID3D11Buffer> vertexBuffer,
		ComPtr<ID3D11Buffer> indexBuffer,
		size_t vertexCount,
		size_t indexCount,
		shared_ptr<Material> material);
	~SubMesh();

	inline ComPtr<ID3D11Buffer>			GetVertexBuffer() { return _vertexBuffer; }
	inline ComPtr<ID3D11Buffer>			GetIndexBuffer() { return _indexBuffer; }
	inline shared_ptr<Material>			GetMaterial() { return _material; }
	inline size_t						GetVertexCount() { return _vertexCount; }
	inline size_t						GetIndexCount() { return _indexCount; }

private:
   	ComPtr<ID3D11Buffer>				_vertexBuffer;
	ComPtr<ID3D11Buffer>				_indexBuffer;
	shared_ptr<Material>				_material;
	size_t								_vertexCount;
	size_t								_indexCount;
};

// The core Mesh class.  A Mesh corresponds to a scene in ASSIMP. A mesh consists of one or more sub-meshes.

class Node
{
public:
	inline void						    SetName(wstring name) { _name = name; }
	inline wstring						GetName() { return _name; }
	inline size_t					    GetMeshCount() { return _meshIndices.size(); }
	inline unsigned int					GetMesh(unsigned int index) { return _meshIndices[index]; }
	inline void							AddMesh(unsigned int meshIndex) { _meshIndices.push_back(meshIndex); }
	inline size_t					    GetChildrenCount() { return _children.size(); }
	inline shared_ptr<Node>				GetChild(unsigned int index) { return _children[index]; }
	inline void							AddChild(shared_ptr<Node> node) { _children.push_back(node); }

private:
	wstring								_name;
	vector<unsigned int>				_meshIndices;
	vector<shared_ptr<Node>>			_children;
};

class Mesh
{
public:
	size_t								GetSubMeshCount();
	shared_ptr<SubMesh>					GetSubMesh(unsigned int i);
	void								AddSubMesh(shared_ptr<SubMesh> subMesh);
	shared_ptr<Node>				    GetRootNode();
	void								SetRootNode(shared_ptr<Node> node);

private:
	vector<shared_ptr<SubMesh>> 		_subMeshList;
	shared_ptr<Node>					_rootNode;
};


