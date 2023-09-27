#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, 
    float moveSpeed, 
    float mouseLookSpeed, 
    float fov, 
    float aspectRatio) : 
    moveSpeed(moveSpeed),
    mouseLookSpeed(mouseLookSpeed)
{
    //Set our initial position
    transform.SetPosition(x, y, z);

    //Set up matrices
    UpdateViewMatrix();
    UpdateProjectionMatrix(fov, aspectRatio);
}

Camera::~Camera()
{
}

void Camera::Update(float dt)
{
    //Grab the input manager instance
    Input& input = Input::GetInstance();

    //Current speed
    float speed = dt * moveSpeed;

    if (input.KeyDown('W')) { transform.MoveRelative(0, 0, speed); }
    if (input.KeyDown('S')) { transform.MoveRelative(0, 0, -speed); }
    if (input.KeyDown('A')) { transform.MoveRelative(-speed, 0, 0); }
    if (input.KeyDown('D')) { transform.MoveRelative(speed, 0, 0); }
    if (input.KeyDown(' ')) { transform.MoveRelative(0, speed, 0); }
    if (input.KeyDown('X')) { transform.MoveRelative(0, -speed, 0); }

    if (input.MouseLeftDown())
    {
        float xDiff = 0.001f * mouseLookSpeed * input.GetMouseXDelta();
        float yDiff = 0.001f * mouseLookSpeed * input.GetMouseYDelta();
        transform.Rotate(yDiff, xDiff, 0);
    }

    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    //Grab the transform data we'll need
    XMFLOAT3 pos = transform.GetPosition();
    XMFLOAT3 fwd = transform.GetForward();

    //Build the view and store
    XMMATRIX view = XMMatrixLookToLH(
        XMLoadFloat3(&pos),
        XMLoadFloat3(&fwd),
        XMVectorSet(0, 1, 0, 0));
    XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix(float fov, float aspectRatio)
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        fov,
        aspectRatio,
        0.01f,      //Near clip distance
        1000.0f);   //Far clip distance
    XMStoreFloat4x4(&projectionMatrix, proj);
}

Transform* Camera::GetTransform() { return &transform; }

DirectX::XMFLOAT4X4 Camera::GetView() { return viewMatrix; }

DirectX::XMFLOAT4X4 Camera::GetProjection() { return projectionMatrix; }
