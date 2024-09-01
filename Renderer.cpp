#include "Renderer.h"
#include "Logger.h"
#include "Utils.h"
#include "InputManager.h"

#include <dxgi1_3.h>
#include <combaseapi.h>
#include <ppltasks.h>
#include <array>
#include <fstream>

Renderer::Renderer(HWND windowHandle)
	: m_WindowHandle{ windowHandle }
	, m_pDevice{ nullptr }
	, m_pDeviceContext{ nullptr }
	, m_pSwapChain{}
	, m_pRenderTargetView{}
	, m_pDepthStencil{}
	, m_pDepthStencilView{}
	, m_pBackBuffer{}
	, m_BackBufferDescription{}
	, m_pInputLayout{}
	, m_pVertexShader{}
	, m_pPixelShader{}
	, m_pConstantBuffer{}
	, m_VertexConstantBuffer{}
	, m_pVertexBuffer{}
	, m_pIndexBuffer{}
	, m_IndexCount{}
	, m_Viewport{}
	, m_FeatureLevel{}
	, m_SuccesfullCreation{ false }
	, m_CameraPos{ 0.f, 0.f, -5.f }
{
	
	bool success =		   CreateDevice();
	if (success) success = CreateSwapChain();
	if (success) success = CreateRenderTarget();
	if (success) success = CreateDepthStencil();

	if (success) CreateViewport();
}

void Renderer::Temp_Update(float deltaTime)
{
	if (!m_SuccesfullCreation) return;

	// -----------------
	// DEBUG CAMERA MOVE
	// -----------------

	InputManager* pInput = InputManager::GetInstance();
	const float moveSpeed = 5.f;

	if (pInput->IsKeyPressed('A'))
	{
		m_CameraPos.x -= moveSpeed * deltaTime;
	}
	if (pInput->IsKeyPressed('D'))
	{
		m_CameraPos.x += moveSpeed * deltaTime;
	}

	if (pInput->IsKeyPressed('W'))
	{
		m_CameraPos.z += moveSpeed * deltaTime;
	}
	if (pInput->IsKeyPressed('S'))
	{
		m_CameraPos.z -= moveSpeed * deltaTime;
	}

	if (pInput->IsKeyPressed('Q'))
	{
		m_CameraPos.y += moveSpeed * deltaTime;
	}
	if (pInput->IsKeyPressed('E'))
	{
		m_CameraPos.y -= moveSpeed * deltaTime;
	}

	CreateViewProjectionMatrix();
}
void Renderer::Render()
{
	// Don't render if faulty init
	if (!m_SuccesfullCreation) return;

	// Update subresource
	m_pDeviceContext->UpdateSubresource
	(
		m_pConstantBuffer.Get(),	// Destination resource
		0,							// Resource index
		nullptr,					// Write data to destination resource without offset
		&m_VertexConstantBuffer,	// Source data
		0,							// Size of one row of source data
		0							// Size of one depth slice of source data
	);

	// Clear the renderTarget and the z-buffer
	const float backgroundColor[] = { 0.098f, 0.439f, 0.439f, 1.f };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), backgroundColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	// Set the renderTarget
	m_pDeviceContext->OMSetRenderTargets
	(
		1,									// Nr renderTargets
		m_pRenderTargetView.GetAddressOf(),	// RenderTargets
		m_pDepthStencilView.Get()			// DepthStencilView
	);

	// Set up the IA stage by setting the inputTopology and -layout
	UINT stride = sizeof(BaseVertexInput);
	UINT offset{};

	m_pDeviceContext->IASetVertexBuffers
	(
		0,								// StartSlot
		1,								// Nr VertexBuffers
		m_pVertexBuffer.GetAddressOf(),	// VertexBuffers
		&stride,						// Strides
		&offset							// Offsets
	);

	m_pDeviceContext->IASetIndexBuffer
	(
		m_pIndexBuffer.Get(),			// IndexBuffer
		DXGI_FORMAT_R16_UINT,			// Format
		0								// Offset
	);

	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());

	// Set up the vertexShader stage
	m_pDeviceContext->VSSetShader
	(
		m_pVertexShader.Get(),	// VertexShader
		nullptr,				// Class instances
		0						// Nr class instances
	);

	m_pDeviceContext->VSSetConstantBuffers
	(
		0,									// StartSlot
		1,									// Nr buffers
		m_pConstantBuffer.GetAddressOf()	// ConstantBuffers
	);

	// Set up the pixelShader stage
	m_pDeviceContext->PSSetShader
	(
		m_pPixelShader.Get(),	// PixelShader
		nullptr,				// Class instances
		0						// Nr class instances
	);

	// Draw
	m_pDeviceContext->DrawIndexed
	(
		m_IndexCount,	// IndexCount
		0,				// Start index
		0				// Base vertexLocation
	);

	// Present frame (do after every geometry is rendered)
	m_pSwapChain->Present(1, 0);
}

void Renderer::CreateDeviceDependentResources()
{
	// Compile shaders
	auto createShadersTask = Concurrency::create_task([this]()
	{
		CreateShaders();
	});

	// Load the geometry, after compiling shaders
	auto createTriangleTask = createShadersTask.then([this]()
	{
		CreateTriangle();
	});
}
void Renderer::CreateWindowSizeDependentResources()
{
	// Create view & projection matrix
	CreateViewProjectionMatrix();
}

// Privates
// --------
bool Renderer::CreateDevice()
{
	// Minimum level of hardware the application supports
	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	// Supports surface with different color-change than API default, required for Direct2D
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(DEBUG) || defined(_DEBUG)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Create device
	HRESULT result = D3D11CreateDevice
	(
		nullptr,							// Default adapter
		D3D_DRIVER_TYPE_HARDWARE,			// Create device using hardware graphics driver
		0,									// Not using D3D_DRIVER_TYPE_SOFTWARE
		deviceFlags,						// Direct2D and debug compatiblity
		featureLevels,						// Supported feature levels
		ARRAYSIZE(featureLevels),			// Size of levels array
		D3D11_SDK_VERSION,					// Version for Windows Store Apps
		m_pDevice.GetAddressOf(),			// Returns created Direct3D device
		&m_FeatureLevel,					// Returns feature level of created device
		m_pDeviceContext.GetAddressOf()		// Returns device immediate context
	);

	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error creating the DirectX Device");
		return false;
	}
	else
	{
		Logger::Log(L"Succeeded creating the DirectX Device");
		return true;
	}
}
bool Renderer::CreateSwapChain()
{
	// Create swapChain description
	DXGI_SWAP_CHAIN_DESC chainDescription;
	ZeroMemory(&chainDescription, sizeof(DXGI_SWAP_CHAIN_DESC));

	chainDescription.Windowed = TRUE;									// Not in full-screen mode
	chainDescription.BufferCount = 2;									// Double-buffer
	chainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	// 32-bit color
	chainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		// Used for drawing
	chainDescription.SampleDesc.Count = 1;								// Multisampling setting
	chainDescription.SampleDesc.Quality = 0;							// Vendor-specific flag
	chainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;		// How buffer swaps
	chainDescription.OutputWindow = m_WindowHandle;						// Window to render to

	// Cast to IDXGIDevice3
	Microsoft::WRL::ComPtr<IDXGIDevice3> pDxgiDevice;
	m_pDevice.As(&pDxgiDevice);

	// Create swapChain
	Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
	HRESULT result = pDxgiDevice->GetAdapter(&pAdapter);

	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error getting the DirectX Adapter");
		return false;
	}

	Logger::Log(L"Succeeded getting the DirectX Adapter");

	Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;
	pAdapter->GetParent(__uuidof(IDXGIFactory), &pFactory);

	result = pFactory->CreateSwapChain(m_pDevice.Get(), &chainDescription, m_pSwapChain.GetAddressOf());
	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error creating the DirectX SwapChain");
		return false;
	}
	else
	{
		Logger::Log(L"Succeeded creating the DirectX SwapChain");
		return true;
	}
}
bool Renderer::CreateRenderTarget()
{
	// Get backBuffer
	HRESULT result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_pBackBuffer.GetAddressOf()));
	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error getting the SwapChain backBuffer");
		return false;
	}

	// Create renderTargetView
	result = m_pDevice->CreateRenderTargetView
	(
		m_pBackBuffer.Get(),					// Give backBuffer for read
		nullptr,								// Default description
		m_pRenderTargetView.GetAddressOf()		// Returns renderTargetView
	);

	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error creating the RenderTargetView");
		return false;
	}

	m_pBackBuffer->GetDesc(&m_BackBufferDescription);

	return true;
}
bool Renderer::CreateDepthStencil()
{
	// Create description
	const CD3D11_TEXTURE2D_DESC depthStencilDesc
	{
		DXGI_FORMAT_D24_UNORM_S8_UINT,	// Texture Format
		m_BackBufferDescription.Width,	// Width
		m_BackBufferDescription.Height,	// Height
		1,								// Only 1 texture
		1,								// 1 mipmap level
		D3D11_BIND_DEPTH_STENCIL		// Used as depth stencil
	};

	// Create texture
	HRESULT result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, m_pDepthStencil.GetAddressOf());
	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error creating the DepthStencil texture");
		return false;
	}

	// Create view
	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{ D3D11_DSV_DIMENSION_TEXTURE2D };	// Used by texture2D
	result = m_pDevice->CreateDepthStencilView(m_pDepthStencil.Get(), &depthStencilViewDesc, m_pDepthStencilView.GetAddressOf());
	if (FAILED(result))
	{
		Logger::Log(L"FAILED - There was an error creating the DepthStencilView");
		return false;
	}

	return true;
}
void Renderer::CreateViewport()
{
	// Empty variable
	ZeroMemory(&m_Viewport, sizeof(D3D11_VIEWPORT));
	
	// Setup viewport
	m_Viewport.Width = static_cast<float>(m_BackBufferDescription.Width);
	m_Viewport.Height = static_cast<float>(m_BackBufferDescription.Height);
	m_Viewport.MinDepth = 0;
	m_Viewport.MaxDepth = 1;

	// Set viewport
	m_pDeviceContext->RSSetViewports(1, &m_Viewport);	// Only 1 viewport
}

void Renderer::CreateShaders()
{
	// -----------------------------------------------
	// TODO: WILL NEED TO BE REPLACED BY A FILE LOADER
	// -----------------------------------------------

	// VertexShader
	// ------------

	// Open file
	std::wstring fileName{ utils::GetFullResourcePath(L"Base_VS.cso") };
	std::ifstream vertexShaderFile{ fileName.c_str(), std::ifstream::binary | std::ifstream::ate };

	if (!vertexShaderFile)
	{
		Logger::Log(L"ERROR - Failed to open Base_VS.cso");
		return;
	}

	// Read file and create vertexShader
	std::streamsize fileSize{ vertexShaderFile.tellg() };
	vertexShaderFile.seekg(0, std::ios::beg);
	std::vector<char> readBytes(fileSize);
	
	if (!vertexShaderFile.read(readBytes.data(), fileSize))
	{
		Logger::Log(L"ERROR - Failed to read the shader file");
		return;
	}

	HRESULT result = m_pDevice->CreateVertexShader
	(
		readBytes.data(),				// Data
		readBytes.size(),				// Data size
		nullptr,						// Class linkage interface, for linking to the shader
		m_pVertexShader.GetAddressOf()	// VertexShader
	);

	if (FAILED(result))
	{
		Logger::Log(L"Error - Failed to create a vertexShader");
		return;
	}


	// Create inputLayout
	// ------------------

	const D3D11_INPUT_ELEMENT_DESC inputLayoutDescription[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	result = m_pDevice->CreateInputLayout
	(
		inputLayoutDescription,
		ARRAYSIZE(inputLayoutDescription),
		readBytes.data(),
		readBytes.size(),
		m_pInputLayout.GetAddressOf()
	);

	if (FAILED(result))
	{
		Logger::Log(L"Error - Failed to create an inputLayout");
		return;
	}


	// PixelShader
	// -----------

	// Open file
	fileName = utils::GetFullResourcePath(L"Color_PS.cso");
	std::ifstream pixelShaderFile{ fileName.c_str(), std::ifstream::binary | std::ifstream::ate };

	if (!pixelShaderFile)
	{
		Logger::Log(L"ERROR - Failed to open Color_PS.cso");
		return;
	}

	// Read file and create pixelShader
	fileSize = pixelShaderFile.tellg();
	pixelShaderFile.seekg(0, std::ios::beg);

	readBytes.clear();
	readBytes.resize(fileSize);

	if (!pixelShaderFile.read(readBytes.data(), fileSize))
	{
		Logger::Log(L"ERROR - Failed to read the shader file");
		return;
	}

	result = m_pDevice->CreatePixelShader
	(
		readBytes.data(),
		readBytes.size(),
		nullptr,
		m_pPixelShader.GetAddressOf()
	);

	if (FAILED(result))
	{
		Logger::Log(L"ERROR - Failed to create a pixelShader");
		return;
	}


	// ConstantBuffer
	// --------------

	const CD3D11_BUFFER_DESC constantBufferDescription
	{
		sizeof(CB_BaseVertex),
		D3D11_BIND_CONSTANT_BUFFER
	};

	result = m_pDevice->CreateBuffer
	(
		&constantBufferDescription,
		nullptr,
		m_pConstantBuffer.GetAddressOf()
	);

	if (FAILED(result))
	{
		Logger::Log(L"ERROR - Failed to create the constantBuffer");
		return;
	}
}
HRESULT Renderer::CreateTriangle()
{
	// Create triangle geometry
	const BaseVertexInput triangleVertices[] =
	{
		{ DirectX::XMFLOAT3{ -0.5f,-0.5f, 0.0f }, DirectX::XMFLOAT3{}, DirectX::XMFLOAT2{} },
		{ DirectX::XMFLOAT3{  0.5f,-0.5f, 0.0f }, DirectX::XMFLOAT3{}, DirectX::XMFLOAT2{} },
		{ DirectX::XMFLOAT3{  0.0f, 0.3f, 0.0f }, DirectX::XMFLOAT3{}, DirectX::XMFLOAT2{} }
	};

	// Create vertexBuffer
	const CD3D11_BUFFER_DESC vertexDescription{ sizeof(triangleVertices), D3D11_BIND_VERTEX_BUFFER};

	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));	// Stops writes being compiled away if it isn't being read immeadiatly
	vertexData.pSysMem = triangleVertices;						// Initialization data
	vertexData.SysMemPitch = 0;									// Distance from beginning line of texture to the next line (only for 2D & 3D texture)
	vertexData.SysMemSlicePitch = 0;							// Distance from beginning of one depth level to the next	(only for 3D texture)

	HRESULT result = m_pDevice->CreateBuffer
	(
		&vertexDescription,
		&vertexData,
		m_pVertexBuffer.GetAddressOf()
	);

	if (FAILED(result))
	{
		Logger::Log(L"ERROR - Failed to create a vertexBuffer");
		return result;
	}

	// Create indexBuffer
	const unsigned short triangleIndices[]
	{
		0,2,1
	};

	m_IndexCount = ARRAYSIZE(triangleIndices);

	const CD3D11_BUFFER_DESC indexDescription{ sizeof(triangleIndices), D3D11_BIND_INDEX_BUFFER };

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
	indexData.pSysMem = triangleIndices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = m_pDevice->CreateBuffer
	(
		&indexDescription,
		&indexData,
		m_pIndexBuffer.GetAddressOf()
	);

	if (FAILED(result))
	{
		Logger::Log(L"ERROR - Failed to create an indexBuffer");
		return result;
	}

	// Creation success
	m_SuccesfullCreation = true;

	return result;
}
void Renderer::CreateViewProjectionMatrix()
{
	using namespace DirectX;

	// Create world matrix
	const XMMATRIX translation = XMMatrixTranslation(0.f, 0.f, 0.f);
	const XMMATRIX rotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f);	// In radians
	const XMMATRIX scale = XMMatrixScaling(1.f, 1.f, 1.f);

	const XMMATRIX worldMatrix = scale * rotation * translation;
	XMStoreFloat4x4(&m_VertexConstantBuffer.worldMatrix, worldMatrix);

	// Create view matrix
	const XMFLOAT3 cameraForward{ 0.f, 0.f, 1.f };
	const XMFLOAT3 cameraUp{ 0.f, 1.f, 0.f };

	const XMVECTOR worldPos = XMLoadFloat3(&m_CameraPos);
	const XMVECTOR worldForward = XMLoadFloat3(&cameraForward);
	const XMVECTOR worldUp = XMLoadFloat3(&cameraUp);

	const XMMATRIX viewMatrix = XMMatrixLookToLH(worldPos, worldForward, worldUp);

	// Create projection matrix
	const float aspectRatioX = static_cast<float>(m_BackBufferDescription.Width) / m_BackBufferDescription.Height;
	const float FOV{ 45.f };
	const float nearZ{ 0.1f };
	const float farZ{ 100.f };
	const XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), aspectRatioX, nearZ, farZ);

	// Store WVP matrix
	const XMMATRIX WVPMatrix = worldMatrix * viewMatrix * projectionMatrix;
	XMStoreFloat4x4(&m_VertexConstantBuffer.worldViewProjection, WVPMatrix);
}