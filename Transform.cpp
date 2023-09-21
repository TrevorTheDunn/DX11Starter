#include "Transform.h"

#include <DirectXMath.h>

using namespace DirectX;

//CONSTRUCTOR
Transform::Transform() : 
	position(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1.0f, 1.0f, 1.0f)
{
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());
}

//SETTERS
void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	matrixDirty = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 position)
{
	this->position = position;
	matrixDirty = true;
}

void Transform::SetRotation(float p, float y, float r)
{
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;
	matrixDirty = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	this->pitchYawRoll = rotation;
	matrixDirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matrixDirty = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 scale)
{
	this->scale = scale;
	matrixDirty = true;
}

//GETTERS
DirectX::XMFLOAT3 Transform::GetPosition() {return position;}
DirectX::XMFLOAT3 Transform::GetPitchYawRoll() {return pitchYawRoll;}
DirectX::XMFLOAT3 Transform::GetScale() {return scale;}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix() 
{
	if (matrixDirty)
	{
		//Build individual transformation matrices
		XMMATRIX t = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX r = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
		XMMATRIX s = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

		//Combine into a single world matrix
		XMMATRIX wm = s * r * t;

		//Store it somewhere
		XMStoreFloat4x4(&worldMatrix, wm);
		XMStoreFloat4x4(&worldInverseTransposeMatrix,
			XMMatrixInverse(0, XMMatrixTranspose(wm)));
		matrixDirty = false;
	}

	return worldMatrix;
}
DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix() {return worldInverseTransposeMatrix;}

void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
	matrixDirty = true;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	position.x += offset.x;
	position.y += offset.y;
	position.z += offset.z;
	matrixDirty = true;
}

void Transform::Rotate(float p, float y, float r)
{
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
	matrixDirty = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	pitchYawRoll.x += rotation.x;
	pitchYawRoll.y += rotation.y;
	pitchYawRoll.z += rotation.z;
	matrixDirty = true;
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matrixDirty = true;
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	this->scale.x *= scale.x;
	this->scale.y *= scale.y;
	this->scale.z *= scale.z;
	matrixDirty = true;
}
