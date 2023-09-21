#include "Entity.h"

//CONSTRUCTOR - Accepts a shared_ptr for a mesh and saves it, creates a new transform and saves that to a shared_ptr
Entity::Entity(std::shared_ptr<Mesh> mesh)
{
    meshPtr = mesh;
    transformPtr = std::make_shared<Transform>();
}

//DESTRUCTOR
Entity::~Entity()
{
}

//GETTERS - Returns shared_ptrs for mesh or transform
std::shared_ptr<Mesh> Entity::GetMesh() {return meshPtr;}
std::shared_ptr<Transform> Entity::GetTransform() {return transformPtr;}

//Draw Method - Accepts the device context and a constant buffer resource
void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer)
{
    //Set the correct Constant Buffer Resource for the Vertex Shader stage (done in assignment 3)

    
    //Collect data for the current entity in a C++ struct (done in assignment 3, but now updated to hold the world matrix from the entity you're about to draw)
    VertexShaderExternalData vsData;
    vsData.colorTint = DirectX::XMFLOAT4(1.0f, 0.5f, 0.5f, 1.0f);
    vsData.worldMatrix = this->transformPtr->GetWorldMatrix();

    //Map / memcpy / Unmap the Constant Buffer resource (done in assignment 3)
    D3D11_MAPPED_SUBRESOURCE mappedBuffer = {};
    context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);

    memcpy(mappedBuffer.pData, &vsData, sizeof(vsData));

    context->Unmap(constantBuffer.Get(), 0);

    context->VSSetConstantBuffers(
        0,
        1,
        constantBuffer.GetAddressOf());

    //Set the correct Vertex and Index Buffers (done in assignment 2)
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, meshPtr->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(meshPtr->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

    //Tell D3D to render using the currently bound resources (done in assignment 2)
    context->DrawIndexed(meshPtr->GetIndexCount(), 0, 0);
}
