#include "Entity.h"

//CONSTRUCTOR - Accepts a shared_ptr for a mesh and saves it, creates a new transform and saves that to a shared_ptr
Entity::Entity(std::shared_ptr<Mesh> mesh, 
    std::shared_ptr<Material> material) : 
    material(material)
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
std::shared_ptr<Material> Entity::GetMaterial() { return material; }

void Entity::SetMaterial(std::shared_ptr<Material> material) { this->material = material; }

//Draw Method - Accepts the device context and a constant buffer resource
void Entity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> camera, float deltaTime, DirectX::XMFLOAT2 screenRes)
{   
    std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();

    vs->SetMatrix4x4("world", transformPtr->GetWorldMatrix());
    vs->SetMatrix4x4("view", camera->GetView());
    vs->SetMatrix4x4("proj", camera->GetProjection());
    vs->CopyAllBufferData();

    std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

    ps->SetFloat4("colorTint", material->GetColorTint());
    ps->SetFloat("deltaTime", deltaTime);
    ps->SetFloat2("viewportResolution", screenRes);
    ps->CopyAllBufferData();

    //Set the correct Vertex and Index Buffers (done in assignment 2)
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    context->IASetVertexBuffers(0, 1, meshPtr->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(meshPtr->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

    material->GetVertexShader()->SetShader();
    material->GetPixelShader()->SetShader();

    //Tell D3D to render using the currently bound resources (done in assignment 2)
    context->DrawIndexed(meshPtr->GetIndexCount(), 0, 0);
}
