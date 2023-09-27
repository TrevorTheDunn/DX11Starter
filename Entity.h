#pragma once
#include "Transform.h"
#include "Mesh.h"

#include <memory>

#include "BufferStructs.h"
#include "Camera.h"

//#include <DirectXMath.h>

class Entity
{
public:
	//Constructor
	Entity(std::shared_ptr<Mesh> mesh);

	//Destructor
	~Entity();
	
	//Getters
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer, std::shared_ptr<Camera> camera);

private:
	std::shared_ptr<Mesh> meshPtr;
	std::shared_ptr<Transform> transformPtr;
};