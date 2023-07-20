#pragma once
#include "DirectXFramework.h"
#include "TexturedCubeNode.h"
#include "MeshNode.h"
#include "TerrainNode.h"
#include "SkyNode.h"

class Graphics2 : public DirectXFramework
{
public:
	void CreateSceneGraph();
	void UpdateSceneGraph();
	void GetKeyInput();
private:
	float _angle = 0.0f;
	float _red = 0.0f;
	float _green = 0.0f;
	float _blue = 0.0f;

	shared_ptr<TerrainNode> _terrainNode;
};
