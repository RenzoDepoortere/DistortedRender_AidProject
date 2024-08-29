#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl.h>
#include <DirectXMath.h>

class Renderer final
{
public:
	// Rule of five
	Renderer(HWND windowHandle);
	~Renderer() = default;

	Renderer(const Renderer& other) = delete;
	Renderer(Renderer&& other) = delete;
	Renderer& operator= (const Renderer& other) = delete;
	Renderer& operator= (Renderer&& other) = delete;

	// Publics
	void Temp_Update(float deltaTime);
	void Render();

	void CreateDeviceDependentResources();		// Called whenever the scene must be intialized or restarted
	void CreateWindowSizeDependentResources();	// Called whenever the window state changes (buffers also need to be changed, see the DirectX manual)

private:
	// Structs
	struct CB_BaseVertex
	{
		DirectX::XMFLOAT4X4 worldViewProjection;
		DirectX::XMFLOAT4X4 worldMatrix;
	};

	static_assert((sizeof(CB_BaseVertex) % 16) == 0, "Constant Buffer size must be 16-byte aligned");

	struct BaseVertexInput
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 uv;
	};

	// Member variables
	HWND m_WindowHandle;

	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pDepthStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_pBackBuffer;
	D3D11_TEXTURE2D_DESC m_BackBufferDescription;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPixelShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pConstantBuffer;
	CB_BaseVertex m_VertexConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pVertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_pIndexBuffer;
	int m_IndexCount;

	D3D11_VIEWPORT m_Viewport;
	D3D_FEATURE_LEVEL m_FeatureLevel;
	bool m_SuccesfullCreation;

	DirectX::XMFLOAT3 m_CameraPos;

	// Member functions
	bool CreateDevice();
	bool CreateSwapChain();
	bool CreateRenderTarget();
	bool CreateDepthStencil();
	void CreateViewport();

	void CreateShaders();
	HRESULT CreateCube();
	void CreateViewProjectionMatrix();
};

