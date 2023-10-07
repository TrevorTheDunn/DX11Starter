#pragma once
#include "Transform.h"
#include "Mesh.h"

#include <memory>

#include "BufferStructs.h"
#include "Camera.h"
#include "Material.h"

//#include <DirectXMath.h>

class Entity
{
public:
	//Constructor
	Entity(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

	//Destructor
	~Entity();
	
	//Getters
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();

	//Setters
	void SetMaterial(std::shared_ptr<Material> material);

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float deltaTime, DirectX::XMFLOAT2 screenRes);

private:
	std::shared_ptr<Mesh> meshPtr;
	std::shared_ptr<Transform> transformPtr;
	std::shared_ptr<Material> material;
};