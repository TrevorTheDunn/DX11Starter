#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <memory> //needed for shared_ptr
#include "Mesh.h"
#include <vector>

//ImGui
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

#include "Entity.h"
#include "Camera.h"

#include "SimpleShader.h"

#include "Lights.h"

#include "Sky.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void LoadShaders(); 
	void CreateGeometry();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	// Buffers to hold actual geometry data
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

	std::shared_ptr<SimpleVertexShader> vertexShader;
	std::shared_ptr<SimplePixelShader> pixelShader;
	std::shared_ptr<SimplePixelShader> customPixelShader;

	//Pointers for the three meshes
	std::shared_ptr<Mesh> mesh1;
	std::shared_ptr<Mesh> mesh2;
	std::shared_ptr<Mesh> mesh3;

	std::vector<std::shared_ptr<Mesh>> meshes;

	std::vector<std::shared_ptr<Entity>> entities;

	std::shared_ptr<Camera> activeCamera;

	std::vector<std::shared_ptr<Camera>> cameras;

	std::vector<std::shared_ptr<Material>> materials;

	std::vector<Light> lights;

	void loadTextures(std::shared_ptr<Mesh> cubeMesh);
	void loadMaterials();
	void loadShadows();
	void renderShadows();
	void ppSetup();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBronzeAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBronzeMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBronzeNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvBronzeRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvCobbleAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvCobbleMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvCobbleNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvCobbleRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvFloorAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvFloorMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvFloorNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvFloorRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPaintAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPaintMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPaintNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvPaintRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRoughAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRoughMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRoughNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvRoughRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvScratchAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvScratchMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvScratchNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvScratchRough;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvWoodAlbedo;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvWoodMetal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvWoodNormal;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srvWoodRough;

	//D3D11_SAMPLER_DESC samplerDesc;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	std::shared_ptr<Sky> sky;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightProjectionMatrix;

	float shadowMapResolution;
	float lightProjectionSize;

	//Resources that are shared among all post processes
	Microsoft::WRL::ComPtr<ID3D11SamplerState> ppSampler;
	std::shared_ptr<SimpleVertexShader> ppVS;

	//Resources that are tied to a particular post process
	std::shared_ptr<SimplePixelShader> ppPS;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppRTV; //For rendering
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppSRV; //For sampling

	int blurAmt;
};

