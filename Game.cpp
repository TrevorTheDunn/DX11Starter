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
		true)				// Show extra stats (fps) in title bar?
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
	directionalLight1.Direction = XMFLOAT3(1.0f, 0.0f, 0.0f); //points right
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
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red	= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green	= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 black = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 white = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4 purple = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);

	// Imports all of the different 3D meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device);
	std::shared_ptr<Mesh> cylinderMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str(), device);
	std::shared_ptr<Mesh> torusMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str(), device);
	std::shared_ptr<Mesh> quadMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str(), device);
	std::shared_ptr<Mesh> quadDSMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str(), device);

	//Loads Textures
	//HRESULT result1 = CreateWICTextureFromFile(*(device.GetAddressOf()), *(context.GetAddressOf()), FixPath(L"../../Assets/Textures/tiles.png").c_str(), nullptr, srvPtr1.GetAddressOf());
	//HRESULT result2 = CreateWICTextureFromFile(*(device.GetAddressOf()), *(context.GetAddressOf()), FixPath(L"../../Assets/Textures/brokentiles.png").c_str(), nullptr, srvPtr2.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/tiles.png").c_str(), 0, srvTile.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/brokentiles.png").c_str(), 0, srvBroken.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rustymetal.png").c_str(), 0, srvMetal.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/tiles_specular.png").c_str(), 0, srvTileSpec.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/brokentiles_specular.png").c_str(), 0, srvBrokenSpec.GetAddressOf());
	CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(L"../../Assets/Textures/rustymetal_specular.png").c_str(), 0, srvMetalSpec.GetAddressOf());

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());

	// Creates the different materials and adds them to a vector of materials 
	materials.push_back(std::make_shared<Material>(red, vertexShader, pixelShader, 0.0f));
	materials.push_back(std::make_shared<Material>(green, vertexShader, pixelShader, 0.25f));
	materials.push_back(std::make_shared<Material>(blue, vertexShader, pixelShader, 0.5f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.0f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.0f));
	materials.push_back(std::make_shared<Material>(white, vertexShader, pixelShader, 0.0f));

	//std::shared_ptr<Material> customPSMaterial = std::make_shared<Material>(white, vertexShader, customPixelShader, 1);

	materials[5]->AddTextureSRV("SurfaceTexture", srvTile);
	materials[5]->AddTextureSRV("SpecularTexture", srvTileSpec);
	materials[5]->AddSampler("BasicSampler", samplerState);
	materials[4]->AddTextureSRV("SurfaceTexture", srvBroken);
	materials[4]->AddTextureSRV("SpecularTexture", srvBrokenSpec);
	materials[4]->AddSampler("BasicSampler", samplerState);
	materials[3]->AddTextureSRV("SurfaceTexture", srvMetal);
	materials[3]->AddTextureSRV("SpecularTexture", srvMetalSpec);
	materials[3]->AddSampler("BasicSampler", samplerState);

	entities.push_back(std::make_shared<Entity>(sphereMesh, materials[4]));
	entities.push_back(std::make_shared<Entity>(cubeMesh, materials[3]));
	entities.push_back(std::make_shared<Entity>(helixMesh, materials[5]));
	entities.push_back(std::make_shared<Entity>(cylinderMesh, materials[4]));
	entities.push_back(std::make_shared<Entity>(torusMesh, materials[5]));
	entities.push_back(std::make_shared<Entity>(quadMesh, materials[4]));
	entities.push_back(std::make_shared<Entity>(quadDSMesh, materials[3]));

	float x = -9.0f;

	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->GetTransform()->MoveAbsolute(x, 0, 0);
		x += 3.0f;
	}
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
		ImGui::Begin("Assignment 7 Window");

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
				ImGui::ColorEdit3("Color##1", &lights[0].Color.x, 0.1f);
				ImGui::DragFloat("Intensity##1", &lights[0].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Directional Light 2"))
			{
				ImGui::DragFloat3("Direction##2", &lights[1].Direction.x, 0.1f);

				XMVECTOR normDirection = XMVector3Normalize(XMLoadFloat3(&lights[1].Direction));
				XMStoreFloat3(&lights[1].Direction, normDirection);
				ImGui::ColorEdit3("Color##2", &lights[1].Color.x, 0.1f);
				ImGui::DragFloat("Intensity##2", &lights[1].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Directional Light 3"))
			{
				ImGui::DragFloat3("Direction##3", &lights[2].Direction.x, 0.1f);

				XMVECTOR normDirection = XMVector3Normalize(XMLoadFloat3(&lights[2].Direction));
				XMStoreFloat3(&lights[2].Direction, normDirection);
				ImGui::ColorEdit3("Color##3", &lights[2].Color.x, 0.1f);
				ImGui::DragFloat("Intensity##3", &lights[2].Intensity, 0.01f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Point Light 1"))
			{
				ImGui::DragFloat3("Position##1", &lights[3].Position.x, 0.1f);
				ImGui::ColorEdit3("Color##4", &lights[3].Color.x, 0.1f);
				ImGui::DragFloat("Intensity##4", &lights[3].Intensity, 0.01f, 0.0f);
				ImGui::DragFloat("Range##1", &lights[3].Range, 0.1f, 0.0f);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Point Light 2"))
			{
				ImGui::DragFloat3("Position##2", &lights[4].Position.x, 0.1f);
				ImGui::ColorEdit3("Color##5", &lights[4].Color.x, 0.1f);
				ImGui::DragFloat("Intensity##5", &lights[4].Intensity, 0.01f, 0.0f);
				ImGui::DragFloat("Range##1", &lights[4].Range, 0.1f, 0.0f);
				ImGui::TreePop();
			}
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
	}

	XMFLOAT3 ambientColor = XMFLOAT3(0.1f, 0.1f, 0.25f);

	//Draws each of the entities
	for (auto& e : entities)
	{
		e->GetMaterial()->GetPixelShader()->SetData("lights", &lights[0], sizeof(Light) * (int)lights.size());
		e->Draw(context, activeCamera, deltaTime, XMFLOAT2((float)this->windowWidth, (float)this->windowHeight));
		//e->Draw(context, activeCamera, srvPtr1, samplerState);
	}

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
	}
}