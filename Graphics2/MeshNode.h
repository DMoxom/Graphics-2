#pragma once
#include "SceneNode.h"
#include "DirectXFramework.h"
#include "MeshRenderer.h"

class MeshNode : public SceneNode
{
public:
public:
	MeshNode(wstring name, wstring modelName) : SceneNode(name) { _modelName = modelName; }

	bool Initialise();
	void Render();
	void Shutdown();

private:
	shared_ptr<MeshRenderer>		_renderer;

	wstring							_modelName;
	shared_ptr<ResourceManager>		_resourceManager;
	shared_ptr<Mesh>				_mesh;
};

