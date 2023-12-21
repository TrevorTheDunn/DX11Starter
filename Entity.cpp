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
    this->GetMaterial()->SetResources(transformPtr, camera);

    meshPtr->SetBuffersAndDraw(context);
}
