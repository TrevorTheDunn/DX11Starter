#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "SimpleShader.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

//For Constant Buffer(?)
#include "BufferStructs.h"

#include "Entity.h"

#include "Material.h"

#include <WICTextureLoader.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),				// Show extra stats (fps) in title bar?
	shadowMapResolution(1024),
	lightViewMatrix(),
	lightProjectionMatrix(),
	lightProjectionSize(15.0f),
	blurAmt(5)
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	//ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();

	CreateGeometry();

	Light directionalLight1 = {};
	directionalLight1.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight1.Direction = XMFLOAT3(1.0f, -1.0f, 0.0f); //points right
	directionalLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f); //red
	directionalLight1.Intensity = 0.5f;
	lights.push_back(directionalLight1);

	Light directionalLight2 = {};
	directionalLight2.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight2.Direction = XMFLOAT3(-1.0f, 0.0f, 0.0f); //points left
	directionalLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f); //blue
	directionalLight2.Intensity = 0.5f;
	lights.push_back(directionalLight2);

	Light directionalLight3 = {};
	directionalLight3.Type = LIGHT_TYPE_DIRECTIONAL;
	directionalLight3.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f); //points forwards
	directionalLight3.Color = XMFLOAT3(1.0f, 1.0f, 1.0f); //green
	directionalLight3.Intensity = 0.5f;
	lights.push_back(directionalLight3);

	Light pointLight1 = {};
	pointLight1.Type = LIGHT_TYPE_POINT;
	pointLight1.Range = 10.0f;
	pointLight1.Position = XMFLOAT3(-1.5f, -3.0f, 0.0f); 
	pointLight1.Color = XMFLOAT3(1.0f, 1.0f, 1.0f); 
	pointLight1.Intensity = 0.5f;
	lights.push_back(pointLight1);

	Light pointLight2 = {};
	pointLight2.Type = LIGHT_TYPE_POINT;
	pointLight2.Range = 10.0f;
	pointLight2.Position = XMFLOAT3(1.5f, 3.0f, 0.0f); 
	pointLight2.Color = XMFLOAT3(1.0f, 1.0f, 1.0f); 
	pointLight2.Intensity = 0.5f;
	lights.push_back(pointLight2);

	//Create the cameras and sets the active one to the first camera
	cameras.push_back(std::make_shared<Camera>(
		0.0f, 0.0f, -5.0f,
		5.0f,
		1.0f,
		XM_PIDIV4,
		(float)this->windowWidth / this->windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		-5.0f, 5.0f, -10.0f,
		5.0f,
		1.0f,
		XM_PIDIV2,
		(float)this->windowWidth / this->windowHeight));
	cameras.push_back(std::make_shared<Camera>(
		5.0f, -5.0f, -7.5f,
		5.0f,
		1.0f,
		XM_PIDIV4/2.0f,
		(float)this->windowWidth / this->windowHeight));
	activeCamera = cameras[0];
	
	// Set initial graphics API state
	// Tell the input assembler (IA) stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our vertices?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	//Pick a style (uncomment one of these 3)
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	//Get size as the next multiple of 16 (rather than hardcoding a size)
	unsigned int size = sizeof(VertexShaderExternalData);
	size = (size + 15) / 16 * 16; //Will work even if the struct size changes

	loadShadows();
	ppSetup();
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"VertexShader.cso").c_str());
	pixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"PixelShader.cso").c_str());
	customPixelShader = std::make_shared<SimplePixelShader>(device, context, FixPath(L"CustomPS.cso").c_str());
	ppVS = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"FullscreenVS.cso").c_str());
	ppPS = std::make_shared<SimplePixelShader>(device, context, FixPath(L"BoxBlurPPPS.cso").c_str());
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT3 red	= XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3 green	= XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 blue	= XMFLOAT3(0.0f, 0.0f, 1.0f);
	XMFLOAT3 black = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 white = XMFLOAT3(1.0f, 1.0f, 1.0f);
	XMFLOAT3 purple = XMFLOAT3(1.0f, 0.0f, 1.0f);

	// Imports all of the different 3D meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device);
	std::shared_ptr<Mesh> cylinderMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device);
	std::shared_ptr<Mesh> torusMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str(), device);
	std::shared_ptr<Mesh> quadMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str(), device);
	std::shared_ptr<Mesh> quadDSMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str(), device);

	loadTextures(cubeMesh);

	// Creates the different materials and adds them to a vector of materials 
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.0f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.17f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.34f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.51f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.68f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.85f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 1.0f));

	//std::shared_ptr<Material> customPSMaterial = std::make_shared<Material>(white, vertexShader, customPixelShader, 1);

	loadMaterials();

	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[0]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[1]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[2]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[3]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[4]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[5]));
	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[6]));

	float x = -9.0f;

	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->GetTransform()->MoveAbsolute(x, 0, 0);
		x += 3.0f;
	}

	entities.push_back(std::make_shared<Entity>(quadDSMesh, materials[6]));
	entities[7]->GetTransform()->MoveAbsolute(0, -1.5f, 0);
	entities[7]->GetTransform()->SetScale(10.0f, 10.0f, 10.0f);
}

void Game::loadTextures(std::shared_ptr<Mesh> cubeMesh)
{
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/bronze_albedo.png").c_str(), 0, srvBronzeAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/bronze_metal.png").c_str(), 0, srvBronzeMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/bronze_normals.png").c_str(), 0, srvBronzeNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/bronze_roughness.png").c_str(), 0, srvBronzeRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/cobblestone_albedo.png").c_str(), 0, srvCobbleAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/cobblestone_metal.png").c_str(), 0, srvCobbleMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/cobblestone_normals.png").c_str(), 0, srvCobbleNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/cobblestone_roughness.png").c_str(), 0, srvCobbleRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/floor_albedo.png").c_str(), 0, srvFloorAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/floor_metal.png").c_str(), 0, srvFloorMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/floor_normals.png").c_str(), 0, srvFloorNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/floor_roughness.png").c_str(), 0, srvFloorRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/paint_albedo.png").c_str(), 0, srvPaintAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/paint_metal.png").c_str(), 0, srvPaintMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/paint_normals.png").c_str(), 0, srvPaintNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/paint_roughness.png").c_str(), 0, srvPaintRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rough_albedo.png").c_str(), 0, srvRoughAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rough_metal.png").c_str(), 0, srvRoughMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rough_normals.png").c_str(), 0, srvRoughNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rough_roughness.png").c_str(), 0, srvRoughRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/scratched_albedo.png").c_str(), 0, srvScratchAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/scratched_metal.png").c_str(), 0, srvScratchMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/scratched_normals.png").c_str(), 0, srvScratchNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/scratched_roughness.png").c_str(), 0, srvScratchRough.GetAddressOf());

	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/wood_albedo.png").c_str(), 0, srvWoodAlbedo.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/wood_metal.png").c_str(), 0, srvWoodMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/wood_normals.png").c_str(), 0, srvWoodNormal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/wood_roughness.png").c_str(), 0, srvWoodRough.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

	std::vector<const wchar_t*> skySidePaths;
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/right.png").c_str());
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/left.png").c_str());
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/up.png").c_str());
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/down.png").c_str());
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/front.png").c_str());
	skySidePaths.push_back(FixPath(L"../../Assets/Textures/back.png").c_str());

	sky = std::make_shared<Sky>(*cubeMesh, samplerState, device, context,
		FixPath(L"../../Assets/Textures/Skybox/right.png").c_str(), FixPath(L"../../Assets/Textures/Skybox/left.png").c_str(),
		FixPath(L"../../Assets/Textures/Skybox/up.png").c_str(), FixPath(L"../../Assets/Textures/Skybox/down.png").c_str(),
		FixPath(L"../../Assets/Textures/Skybox/front.png").c_str(), FixPath(L"../../Assets/Textures/Skybox/back.png").c_str(),
		std::make_shared<SimpleVertexShader>(device, context, FixPath(L"SkyVertexShader.cso").c_str()),
		std::make_shared<SimplePixelShader>(device, context, FixPath(L"SkyPixelShader.cso").c_str()));
}

void Game::loadMaterials()
{
	materials[0]->AddTextureSRV("Albedo", srvBronzeAlbedo);
	materials[0]->AddTextureSRV("NormalMap", srvBronzeNormal);
	materials[0]->AddTextureSRV("RoughnessMap", srvBronzeRough);
	materials[0]->AddTextureSRV("MetalnessMap", srvBronzeMetal);
	materials[0]->AddSampler("BasicSampler", samplerState);

	materials[1]->AddTextureSRV("Albedo", srvCobbleAlbedo);
	materials[1]->AddTextureSRV("NormalMap", srvCobbleNormal);
	materials[1]->AddTextureSRV("RoughnessMap", srvCobbleRough);
	materials[1]->AddTextureSRV("MetalnessMap", srvCobbleMetal);
	materials[1]->AddSampler("BasicSampler", samplerState);

	materials[2]->AddTextureSRV("Albedo", srvFloorAlbedo);
	materials[2]->AddTextureSRV("NormalMap", srvFloorNormal);
	materials[2]->AddTextureSRV("RoughnessMap", srvFloorRough);
	materials[2]->AddTextureSRV("MetalnessMap", srvFloorMetal);
	materials[2]->AddSampler("BasicSampler", samplerState);

	materials[3]->AddTextureSRV("Albedo", srvPaintAlbedo);
	materials[3]->AddTextureSRV("NormalMap", srvPaintNormal);
	materials[3]->AddTextureSRV("RoughnessMap", srvPaintRough);
	materials[3]->AddTextureSRV("MetalnessMap", srvPaintMetal);
	materials[3]->AddSampler("BasicSampler", samplerState);

	materials[4]->AddTextureSRV("Albedo", srvRoughAlbedo);
	materials[4]->AddTextureSRV("NormalMap", srvRoughNormal);
	materials[4]->AddTextureSRV("RoughnessMap", srvRoughRough);
	materials[4]->AddTextureSRV("MetalnessMap", srvRoughMetal);
	materials[4]->AddSampler("BasicSampler", samplerState);

	materials[5]->AddTextureSRV("Albedo", srvScratchAlbedo);
	materials[5]->AddTextureSRV("NormalMap", srvScratchNormal);
	materials[5]->AddTextureSRV("RoughnessMap", srvScratchRough);
	materials[5]->AddTextureSRV("MetalnessMap", srvScratchMetal);
	materials[5]->AddSampler("BasicSampler", samplerState);

	materials[6]->AddTextureSRV("Albedo", srvWoodAlbedo);
	materials[6]->AddTextureSRV("NormalMap", srvWoodNormal);
	materials[6]->AddTextureSRV("RoughnessMap", srvWoodRough);
	materials[6]->AddTextureSRV("MetalnessMap", srvWoodMetal);
	materials[6]->AddSampler("BasicSampler", samplerState);
}

void Game::loadShadows()
{
	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = (UINT)shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.Height = (UINT)shadowMapResolution; // Ideally a power of 2 (like 1024)
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

	// Create the depth/stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	XMVECTOR newPosition = XMVectorSet(-(lights[0].Direction.x * 20), -(lights[0].Direction.y * 20), -(lights[0].Direction.z * 20), 0);
	XMVECTOR lightDirection = XMVectorSet(lights[0].Direction.x, lights[0].Direction.y, lights[0].Direction.z, 0);
	XMVECTOR upVec = XMVectorSet(0, 1, 0, 0);

	XMMATRIX lightView = XMMatrixLookToLH(
		newPosition,					// Position: "Backing up" 20 units from origin
		lightDirection,					// Direction: light's direction
		upVec);							// Up: World up vector (Y axis)
	XMStoreFloat4x4(&lightViewMatrix, lightView);

	lightProjectionSize = 15.0f;
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);
	XMStoreFloat4x4(&lightProjectionMatrix, lightProjection);

	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	activeCamera->UpdateProjectionMatrix(XM_PIDIV4,
		(float)this->windowWidth / this->windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	//Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;

	//Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//Determine new input capture
	Input& input = Input::GetInstance();
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);

	//Show the demo window
	//ImGui::ShowDemoWindow();

	int value = 0;

	std::vector<XMFLOAT3> ePos;
	std::vector<XMFLOAT3> eRot;
	std::vector<XMFLOAT3> eSca;

	for (int i = 0; i < entities.size(); i++)
	{
		ePos.push_back(entities[i]->GetTransform()->GetPosition());
		eRot.push_back(entities[i]->GetTransform()->GetPitchYawRoll());
		eSca.push_back(entities[i]->GetTransform()->GetScale());
	}

	//Creation of custom ImGui Window
	{
		ImGui::SetNextWindowSize(ImVec2(500, 400));

		//Header Text for the Window
		ImGui::Begin("Assignment 12 Window");

		//Dropdown menu for app details
		if (ImGui::TreeNode("App Details"))
		{
			//Tracker for the framerate
			ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

			//Tracker for the Window Dimensions
			ImGui::Text("Window Dimensions: %i x %i", this->windowWidth, this->windowHeight);

			ImGui::TreePop();
		}

		//TASK 7 - OFFSET AND COLOR CHANGE
		//You'll probably need variables for these pieces of data, as they need to persist across frames

		/*if (ImGui::TreeNode("Meshes"))
		{
			//OFFSET - To edit a 3 component vector with ImGui, use the DragFloat3() function
			ImGui::DragFloat3("Offset", &offset.x);

			//COLOR - To edit a 4-component color with ImGui, use the ColorEdit4() function
			ImGui::ColorEdit4("4 component RGBA Color Editor", &tint.x);

			ImGui::TreePop();
		}*/

		if (ImGui::TreeNode("Scene Entities"))
		{
			if (ImGui::TreeNode("Entity 1"))
			{
				ImGui::DragFloat3("Position##1", &ePos[0].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##1", &eRot[0].x, 0.1f);
				ImGui::DragFloat3("Scale##1", &eSca[0].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 2"))
			{
				ImGui::DragFloat3("Position##2", &ePos[1].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##2", &eRot[1].x, 0.1f);
				ImGui::DragFloat3("Scale##2", &eSca[1].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 3"))
			{
				ImGui::DragFloat3("Position##3", &ePos[2].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##3", &eRot[2].x, 0.1f);
				ImGui::DragFloat3("Scale##3", &eSca[2].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 4"))
			{
				ImGui::DragFloat3("Position##4", &ePos[3].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##4", &eRot[3].x, 0.1f);
				ImGui::DragFloat3("Scale##4", &eSca[3].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 5"))
			{
				ImGui::DragFloat3("Position##5", &ePos[4].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##5", &eRot[4].x, 0.1f);
				ImGui::DragFloat3("Scale##5", &eSca[4].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 6"))
			{
				ImGui::DragFloat3("Position##6", &ePos[4].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##6", &eRot[4].x, 0.1f);
				ImGui::DragFloat3("Scale##6", &eSca[4].x, 0.1f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Entity 7"))
			{
				ImGui::DragFloat3("Position##7", &ePos[4].x, 0.1f);
				ImGui::DragFloat3("Rotation (Radians)##7", &eRot[4].x, 0.1f);
				ImGui::DragFloat3("Scale##7", &eSca[4].x, 0.1f);
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Cameras"))
		{
			if (ImGui::TreeNode("Camera 1"))
			{
				XMFLOAT3 c1Pos = cameras[0]->GetTransform()->GetPosition();
				ImGui::Text("Camera 1 Position: X - %f Y - %f Z - %f", c1Pos.x, c1Pos.y, c1Pos.z);
				ImGui::Text("Camera 1 FOV: %f", XM_PIDIV2);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[0]) return;
					activeCamera = cameras[0];
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Camera 2"))
			{
				XMFLOAT3 c2Pos = cameras[1]->GetTransform()->GetPosition();
				ImGui::Text("Camera 2 Position: X - %f Y - %f Z - %f", c2Pos.x, c2Pos.y, c2Pos.z);
				ImGui::Text("Camera 2 FOV: %f", XM_PIDIV4);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[1]) return;
					activeCamera = cameras[1];
				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Camera 3"))
			{
				XMFLOAT3 c3Pos = cameras[2]->GetTransform()->GetPosition();
				ImGui::Text("Camera 3 Position: X - %f Y - %f Z - %f", c3Pos.x, c3Pos.y, c3Pos.z);
				ImGui::Text("Camera 3 FOV: %f", XM_PIDIV4/2.0f);
				if (ImGui::Button("Set As Current Camera"))
				{
					if (activeCamera == cameras[2]) return;
					activeCamera = cameras[2];
				}
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Lights"))
		{
			if (ImGui::TreeNode("Directional Light 1"))
			{
				ImGui::DragFloat3("Direction##1", &lights[0].Direction.x, 0.1f);

				XMVECTOR normDirection = XMVector3Normalize(XMLoadFloat3(&lights[0].Direction));
				XMStoreFloat3(&lights[0].Direction, normDirection);
				ImGui::ColorEdit3("Color##1", &lights[0].Color.x);
				ImGui::DragFloat("Intensity##1", &lights[0].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Directional Light 2"))
			{
				ImGui::DragFloat3("Direction##2", &lights[1].Direction.x, 0.1f);

				XMVECTOR normDirection = XMVector3Normalize(XMLoadFloat3(&lights[1].Direction));
				XMStoreFloat3(&lights[1].Direction, normDirection);
				ImGui::ColorEdit3("Color##2", &lights[1].Color.x);
				ImGui::DragFloat("Intensity##2", &lights[1].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Directional Light 3"))
			{
				ImGui::DragFloat3("Direction##3", &lights[2].Direction.x, 0.1f);

				XMVECTOR normDirection = XMVector3Normalize(XMLoadFloat3(&lights[2].Direction));
				XMStoreFloat3(&lights[2].Direction, normDirection);
				ImGui::ColorEdit3("Color##3", &lights[2].Color.x);
				ImGui::DragFloat("Intensity##3", &lights[2].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Point Light 1"))
			{
				ImGui::DragFloat3("Position##1", &lights[3].Position.x, 0.1f);
				ImGui::ColorEdit3("Color##4", &lights[3].Color.x);
				ImGui::DragFloat("Intensity##4", &lights[3].Intensity, 0.01f, 0.0f);
				ImGui::DragFloat("Range##1", &lights[3].Range, 0.1f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Point Light 2"))
			{
				ImGui::DragFloat3("Position##2", &lights[4].Position.x, 0.1f);
				ImGui::ColorEdit3("Color##5", &lights[4].Color.x);
				ImGui::DragFloat("Intensity##5", &lights[4].Intensity, 0.01f, 0.0f);
				ImGui::DragFloat("Range##1", &lights[4].Range, 0.1f, 0.0f);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Shadows"))
		{
			ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Blur"))
		{
			ImGui::DragInt("Blur Amount", &blurAmt, 0.05f, 0, 10);
			ImGui::TreePop();
		}

		ImGui::End();

		for (int i = 0; i < entities.size(); i++)
		{
			entities[i]->GetTransform()->SetPosition(ePos[i]);
			entities[i]->GetTransform()->SetRotation(eRot[i]);
			entities[i]->GetTransform()->SetScale(eSca[i]);
		}
	}

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	activeCamera->Update(deltaTime);

	entities[0]->GetTransform()->MoveAbsolute(0, 0, 0.001f);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; //Clear color
		context->ClearRenderTargetView(ppRTV.Get(), clearColor);
	}

	renderShadows();

	XMFLOAT3 ambientColor = XMFLOAT3(0.1f, 0.2f, 0.35f);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyCube = sky->GetCubeMap();

	context->OMSetRenderTargets(1, ppRTV.GetAddressOf(), depthBufferDSV.Get());

	//Draws each of the entities
	for (auto& e : entities)
	{
		std::shared_ptr<SimpleVertexShader> entityVS = e->GetMaterial()->GetVertexShader();
		entityVS->SetMatrix4x4("lightView", lightViewMatrix);
		entityVS->SetMatrix4x4("lightProjection", lightProjectionMatrix);

		e->GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());

		std::shared_ptr<SimplePixelShader> entityPS = e->GetMaterial()->GetPixelShader();
		entityPS->SetShaderResourceView("ShadowMap", shadowSRV);
		entityPS->SetSamplerState("ShadowSampler", shadowSampler);

		e->Draw(context, activeCamera, deltaTime, XMFLOAT2((float)this->windowWidth, (float)this->windowHeight));
		//e->Draw(context, activeCamera, srvPtr1, samplerState);
	}

	sky->Draw(context, *activeCamera);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);

	// Activate shaders and bind resources
	// Also set any required cbuffer data (not shown)
	ppVS->SetShader();
	ppPS->SetShader();
	ppPS->SetInt("blurRadius", blurAmt);
	ppPS->SetFloat("pixelWidth", (1.0f / windowWidth));
	ppPS->SetFloat("pixelHeight", (1.0f / windowHeight));
	ppPS->SetShaderResourceView("Pixels", ppSRV.Get());
	ppPS->SetSamplerState("ClampSampler", ppSampler.Get());
	ppPS->CopyAllBufferData();

	context->Draw(3, 0); // Draw exactly 3 vertices (one triangle)

	//ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

		ID3D11ShaderResourceView* nullSRVs[128] = {};
		context->PSSetShaderResources(0, 128, nullSRVs);
	}
}

void Game::renderShadows()
{
	context->RSSetState(shadowRasterizer.Get());

	// Clear the shadow map - resets depth values to 1.0
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set up the output merger state - Set shadow map as current depth buffer and unbind back buffer
	// as we don't need any color output
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

	// Deactivate pixel shader - Unbind to prevent pixel processing entirely
	context->PSSetShader(0, 0, 0);

	// Change viewport - We're about to render into the shadow map, so 
	// the viewport needs to perfectly match the shadow map's resolution
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)shadowMapResolution;
	viewport.Height = (float)shadowMapResolution;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);

	// Entity render loop - Loop through all scene entities and draw them using the specialized
	// shadow map vertex shader described above. 
	// AVOID the entity's material entirely (as that might activate a different set of shaders
	// and we need no material data at all)
	std::shared_ptr<SimpleVertexShader> shadowVS = std::make_shared<SimpleVertexShader>(device, context, FixPath(L"ShadowVS.cso").c_str());
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", lightViewMatrix);
	shadowVS->SetMatrix4x4("projection", lightProjectionMatrix);

	// Loop and draw all entities
	for (auto& e : entities)
	{
		shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();

		// Draw the mesh directly to avoid the entity's material
		// Note: Your code may differ significantly here!
		e->GetMesh()->SetBuffersAndDraw(context);
	}

	// Reset the pipeline - Change pipeline settings back tot prepare to render to the screen once again
	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->OMSetRenderTargets(
		1,
		backBufferRTV.GetAddressOf(),
		depthBufferDSV.Get());

	context->RSSetState(0);
}

void Game::ppSetup()
{
	// Sampler state for post processing
	D3D11_SAMPLER_DESC ppSampDesc = {};
	ppSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	ppSampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	ppSampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&ppSampDesc, ppSampler.GetAddressOf());

	// Describe the texture we're creating
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = windowWidth;
	textureDesc.Height = windowHeight;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	// Create the resource (no need to track it after the views are created below)
	Microsoft::WRL::ComPtr<ID3D11Texture2D> ppTexture;
	device->CreateTexture2D(&textureDesc, 0, ppTexture.GetAddressOf());

	// Create the Render Target View
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(
		ppTexture.Get(),
		&rtvDesc,
		ppRTV.ReleaseAndGetAddressOf());

	// Create the Shader Resource View
	// By passing it a null description for the SRV, we
	// get a "default" SRV that has access to the entire resource
	device->CreateShaderResourceView(
		ppTexture.Get(),
		0,
		ppSRV.ReleaseAndGetAddressOf());
}
