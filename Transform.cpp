#include "Transform.h"

#include <DirectXMath.h>

using namespace DirectX;

//CONSTRUCTOR
Transform::Transform() : 
	position(0, 0, 0),
	pitchYawRoll(0, 0, 0),
	scale(1.0f, 1.0f, 1.0f),
	matrixDirty(false),
	forward(0,0,1),
	right(1,0,0),
	up(0,1,0),
	vectorsDirty(false)
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
	SetPosition(position.x, position.y, position.z);
}

void Transform::SetRotation(float p, float y, float r)
{
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;
	matrixDirty = true;
	vectorsDirty = true;
}

void Transform::SetRotation(DirectX::XMFLOAT3 rotation)
{
	SetRotation(rotation.x, rotation.y, rotation.z);
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
	SetScale(scale.x, scale.y, scale.z);
}

//GETTERS
DirectX::XMFLOAT3 Transform::GetPosition() {return position;}
DirectX::XMFLOAT3 Transform::GetPitchYawRoll() {return pitchYawRoll;}
DirectX::XMFLOAT3 Transform::GetScale() {return scale;}

DirectX::XMFLOAT3 Transform::GetForward() { UpdateVectors(); return forward; }
DirectX::XMFLOAT3 Transform::GetRight() { UpdateVectors(); return right; }
DirectX::XMFLOAT3 Transform::GetUp() { UpdateVectors(); return up; }

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
	MoveAbsolute(offset.x, offset.y, offset.z);
}

void Transform::MoveRelative(float x, float y, float z)
{
	//Create a direction vector from the input and
	//rotate to match our current orientation
	XMVECTOR movement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	//The now-relative direction for our movement
	XMVECTOR relativeDir = XMVector3Rotate(movement, rotQuat);

	//Add and store the results
	XMStoreFloat3(&position, XMLoadFloat3(&position) + relativeDir);
	matrixDirty = true;
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	MoveRelative(offset.x, offset.y, offset.z);
}

void Transform::Rotate(float p, float y, float r)
{
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
	matrixDirty = true;
	vectorsDirty = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
	Rotate(rotation.x, rotation.y, rotation.z);
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
	Scale(scale.x, scale.y, scale.z);
}

void Transform::UpdateVectors()
{
	//Leave if there's no work
	if (!vectorsDirty)
		return;

	//Update all three vectors
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
	XMStoreFloat3(&forward, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotQuat));
	XMStoreFloat3(&right, XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotQuat));
	XMStoreFloat3(&up, XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotQuat));

	//We're clean
	vectorsDirty = false;
}
