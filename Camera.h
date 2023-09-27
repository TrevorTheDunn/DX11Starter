#pragma once
#include "Transform.h"
#include <DirectXMath.h>

class Camera
{
public:
	Camera(float x, float y, float z,
		float moveSpeed,
		float mouseLookSpeed,
		float fov,
		float aspectRatio);

	~Camera();

	//Update methods
	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float fov, float aspectRatio);

	Transform* GetTransform();

	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();

private:
	//Matrices
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	Transform transform;

	float moveSpeed;
	float mouseLookSpeed;
};