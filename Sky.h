#pragma once

#include <wrl/client.h>
#include <memory> 
#include "DXCore.h"
#include "Mesh.h"
#include "SimpleShader.h"
#include "WICTextureLoader.h"
#include "Camera.h"

class Sky
{
public:

	Sky(Mesh skyMesh, Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState,
		Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, const wchar_t* right, const wchar_t* left,
		const wchar_t* up, const wchar_t* down, const wchar_t* front, const wchar_t* back,
		std::shared_ptr<SimpleVertexShader> vertexShader, std::shared_ptr<SimplePixelShader> pixelShader);

	~Sky();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Camera camera);

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetCubeMap();

private:
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvCubeMap;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthBufferType;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rastOptions;
	std::shared_ptr<Mesh> skyMesh;
	std::shared_ptr<SimplePixelShader> skyPixelShader;
	std::shared_ptr<SimpleVertexShader> skyVertexShader;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
};