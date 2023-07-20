#include "Graphics2.h"

Graphics2 app;

void Graphics2::CreateSceneGraph()
{
	_red = 101.0f;
	_green = 149.0f;
	_blue = 252.0f;
	GetCamera()->SetCameraPosition(0.0f, 1000.0f, -1000.0f);

	SceneGraphPointer sceneGraph = GetSceneGraph();

	// Skybox
	shared_ptr<SkyNode> sky = make_shared<SkyNode>(L"Sky", L"skymap.dds", 6000.0f);
	sceneGraph->Add(sky);
	
	// Terrain
	_terrainNode = make_shared<TerrainNode>(L"Terrain1", L"Example_HeightMap.raw",
											1023, 1023, 1024, 10);
	sceneGraph->Add(_terrainNode);

	// Trees
	shared_ptr<MeshNode> tree = make_shared<MeshNode>(L"Tree1", L"Trees\\CL04_M.fbx");
	tree->SetWorldTransform(XMMatrixScaling(0.1f, 0.1f, 0.05f) * XMMatrixRotationAxis(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XM_PI) * XMMatrixTranslation(0, 435.0f, 0));
	sceneGraph->Add(tree);
	tree = make_shared<MeshNode>(L"Tree2", L"Trees\\CL04_M.fbx");
	tree->SetWorldTransform(XMMatrixScaling(0.1f, 0.1f, 0.05f) * XMMatrixRotationAxis(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XM_PI) * XMMatrixTranslation(100, 400.0f, 0));
	sceneGraph->Add(tree);
	tree = make_shared<MeshNode>(L"Tree3", L"Trees\\CL04_M.fbx");
	tree->SetWorldTransform(XMMatrixScaling(0.5, 0.5, 0.5) * XMMatrixRotationAxis(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XM_PI) * XMMatrixTranslation(600, 50.0f, 1300));
	sceneGraph->Add(tree);

	// Plane
	shared_ptr<MeshNode> plane = make_shared<MeshNode>(L"Plane1", L"Plane\\Bonanza.3DS");
	plane->SetWorldTransform(XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(0, 600.0f, 0.0f));
	sceneGraph->Add(plane);

	_angle = 0.0f;
}

void Graphics2::UpdateSceneGraph()
{
	SceneGraphPointer sceneGraph = GetSceneGraph();
	XMVECTOR cameraPosition = GetCamera()->GetCameraPosition();

	_angle += 1;
	sceneGraph->Find(L"Plane1")->SetWorldTransform(XMMatrixScaling(3, 3, 3) * XMMatrixTranslation(300.0f, 0.0f, -700.0f) * XMMatrixRotationAxis(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f), XM_PI) * XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), _angle * XM_PI / 180.0f));

	GetKeyInput();

	// Checks if camera position is at height of terrain to stop camera going through it
	GetCamera()->Update();
	XMVECTOR newCameraPosition = GetCamera()->GetCameraPosition();
	if (XMVectorGetY(newCameraPosition) < _terrainNode->GetHeightAtPoint(XMVectorGetX(cameraPosition), XMVectorGetZ(cameraPosition)) + 1)
	{
		GetCamera()->SetCameraPosition(XMVectorGetX(cameraPosition), XMVectorGetY(cameraPosition), XMVectorGetZ(cameraPosition));
	}
}

void Graphics2::GetKeyInput()
{
	// WASD movement
	if (GetAsyncKeyState(0x57) < 0)
	{
		GetCamera()->SetForwardBack(2);
	}
	if (GetAsyncKeyState(0x53) < 0)
	{
		GetCamera()->SetForwardBack(-2);
	}
	if (GetAsyncKeyState(0x44) < 0)
	{
		GetCamera()->SetLeftRight(2);
	}
	if (GetAsyncKeyState(0x41) < 0)
	{
		GetCamera()->SetLeftRight(-2);
	}

	// Hold shift then WASD for faster movement
	if (GetAsyncKeyState(VK_SHIFT) < 0)
	{
		if (GetAsyncKeyState(0x57) < 0)
		{
			GetCamera()->SetForwardBack(20);
		}
		if (GetAsyncKeyState(0x53) < 0)
		{
			GetCamera()->SetForwardBack(-20);
		}
		if (GetAsyncKeyState(0x44) < 0)
		{
			GetCamera()->SetLeftRight(20);
		}
		if (GetAsyncKeyState(0x41) < 0)
		{
			GetCamera()->SetLeftRight(-20);
		}
	}

	// Arrow keys for looking
	if (GetAsyncKeyState(VK_RIGHT) < 0)
	{
		GetCamera()->SetYaw(1);
	}
	if (GetAsyncKeyState(VK_LEFT) < 0)
	{
		GetCamera()->SetYaw(-1);
	}
	if (GetAsyncKeyState(VK_DOWN) < 0)
	{
		GetCamera()->SetPitch(1);
	}
	if (GetAsyncKeyState(VK_UP) < 0)
	{
		GetCamera()->SetPitch(-1);
	}
}
