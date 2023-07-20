#include "DirectXFramework.h"

// DirectX libraries that are needed
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

DirectXFramework * _dxFramework = nullptr;

DirectXFramework::DirectXFramework() : DirectXFramework(800, 600)
{
}

DirectXFramework::DirectXFramework(unsigned int width, unsigned int height) : Framework(width, height)
{
	_dxFramework = this;

	// Set default background colour
	_backgroundColour[0] = 0.0f;
	_backgroundColour[1] = 0.0f;
	_backgroundColour[2] = 0.0f;
	_backgroundColour[3] = 0.0f;
}

DirectXFramework * DirectXFramework::GetDXFramework()
{
	return _dxFramework;
}

XMMATRIX DirectXFramework::GetProjectionTransformation()
{
	return XMLoadFloat4x4(&_projectionTransformation);
}

void DirectXFramework::SetBackgroundColour(XMFLOAT4 backgroundColour)
{
	_backgroundColour[0] = backgroundColour.x;
	_backgroundColour[1] = backgroundColour.y;
	_backgroundColour[2] = backgroundColour.z;
	_backgroundColour[3] = backgroundColour.w;
}

void DirectXFramework::CreateSceneGraph()
{
}

void DirectXFramework::UpdateSceneGraph()
{
}

bool DirectXFramework::Initialise()
{
	// The call to CoInitializeEx is needed if we are using
	// textures since the WIC library used requires it, so we
	// take care of initialising it here
	if FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
	{
		return false;
	}
	if (!GetDeviceAndSwapChain())
	{
		return false;
	}
	OnResize(SIZE_RESTORED);

	// Create camera and projection matrices (we will look at how the 
	// camera matrix is created from vectors later)
	XMStoreFloat4x4(&_projectionTransformation, XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)GetWindowWidth() / GetWindowHeight(), 1.0f, 10000.0f));
	_resourceManager = make_shared<ResourceManager>();
	_sceneGraph = make_shared<SceneGraph>();
	_camera = make_shared<Camera>();
	CreateSceneGraph();
	return _sceneGraph->Initialise();
}

void DirectXFramework::Shutdown()
{
	// Required because we called CoInitialize above
	_sceneGraph->Shutdown();
	CoUninitialize();
}

void DirectXFramework::Update()
{
	// Do any updates to the scene graph nodes
	UpdateSceneGraph();
	// Now apply any updates that have been made to world transformations
	// to all the nodes
	_sceneGraph->Update(XMMatrixIdentity());

	_camera->Update();
}

void DirectXFramework::Render()
{
	// Clear the render target and the depth stencil view
	_deviceContext->ClearRenderTargetView(_renderTargetView.Get(), _backgroundColour);
	_deviceContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	// Now recurse through the scene graph, rendering each object
	_sceneGraph->Render();
	// Now display the scene
	ThrowIfFailed(_swapChain->Present(0, 0));
}

void DirectXFramework::OnResize(WPARAM wParam)
{
	// Update view and projection matrices to allow for the window size change
	XMStoreFloat4x4(&_projectionTransformation, XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)GetWindowWidth() / GetWindowHeight(), 1.0f, 10000.0f));

	// This will free any existing render and depth views (which
	// would be the case if the window was being resized)
	_renderTargetView = nullptr;
	_depthStencilView = nullptr;
	_depthStencilBuffer = nullptr;

	ThrowIfFailed(_swapChain->ResizeBuffers(1, GetWindowWidth(), GetWindowHeight(), DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	// Create a drawing surface for DirectX to render to
	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
	ThrowIfFailed(_device->CreateRenderTargetView(backBuffer.Get(), NULL, _renderTargetView.GetAddressOf()));
	
	// The depth buffer is used by DirectX to ensure
	// that pixels of closer objects are drawn over pixels of more
	// distant objects.

	// First, we need to create a texture (bitmap) for the depth buffer
	D3D11_TEXTURE2D_DESC depthBufferTexture = { 0 };
	depthBufferTexture.Width = GetWindowWidth();
	depthBufferTexture.Height = GetWindowHeight();
	depthBufferTexture.ArraySize = 1;
	depthBufferTexture.MipLevels = 1;
	depthBufferTexture.SampleDesc.Count = 4;
	depthBufferTexture.Format = DXGI_FORMAT_D32_FLOAT;
	depthBufferTexture.Usage = D3D11_USAGE_DEFAULT;
	depthBufferTexture.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	// Create the depth buffer.  
	ComPtr<ID3D11Texture2D> depthBuffer;
	ThrowIfFailed(_device->CreateTexture2D(&depthBufferTexture, NULL, depthBuffer.GetAddressOf()));
	ThrowIfFailed(_device->CreateDepthStencilView(depthBuffer.Get(), 0, _depthStencilView.GetAddressOf()));

	// Bind the render target view buffer and the depth stencil view buffer to the output-merger stage
	// of the pipeline. 
	_deviceContext->OMSetRenderTargets(1, _renderTargetView.GetAddressOf(), _depthStencilView.Get());

	// Specify a viewport of the required size
	D3D11_VIEWPORT viewPort;
	viewPort.Width = static_cast<float>(GetWindowWidth());
	viewPort.Height = static_cast<float>(GetWindowHeight());
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	_deviceContext->RSSetViewports(1, &viewPort);
}

bool DirectXFramework::GetDeviceAndSwapChain()
{
	UINT createDeviceFlags = 0;

	// We are going to only accept a hardware driver or a WARP
	// driver
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP
	};
	unsigned int totalDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0
	};
	unsigned int totalFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = GetWindowWidth();
	swapChainDesc.BufferDesc.Height = GetWindowHeight();
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Set the refresh rate to 0 and let DXGI determine the best option (refer to DXGI best practices)
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetHWnd();
	// Start out windowed
	swapChainDesc.Windowed = true;
	// Enable multi-sampling to give smoother lines (set to 1 if performance becomes an issue)
	swapChainDesc.SampleDesc.Count = 4;
	swapChainDesc.SampleDesc.Quality = 0;

	// Loop through the driver types to determine which one is available to us
	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;

	for (unsigned int driver = 0; driver < totalDriverTypes && driverType == D3D_DRIVER_TYPE_UNKNOWN; driver++)
	{
		if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(0,
			driverTypes[driver],
			0,
			createDeviceFlags,
			featureLevels,
			totalFeatureLevels,
			D3D11_SDK_VERSION,
			&swapChainDesc,
			_swapChain.GetAddressOf(),
			_device.GetAddressOf(),
			0,
			_deviceContext.GetAddressOf()
		)))

		{
			driverType = driverTypes[driver];
		}
	}
	if (driverType == D3D_DRIVER_TYPE_UNKNOWN)
	{
		// Unable to find a suitable device driver
		return false;
	}
	return true;
}

