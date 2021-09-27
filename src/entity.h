

struct stEntity;

struct
stEntityBase
{
  int64_t Id = -1;
  const char* Name = "entity";
  stEntity* Entity = nullptr;
  uint8_t Active = false;
};

struct
stEntity
{
  static const uint32_t MAX_ENTITY_CILDRENS = 10;

  stEntity() {}

  stMesh* Mesh[MAX_ENTITY_MESH_COUNT];
  uint32_t MeshCount = 0;
  stTransform Transform;
  stEntity* Childrens[MAX_ENTITY_CILDRENS];
  stEntity* Parent = nullptr;

  bool Dynamic = false;
};

struct
stEntitySystem
{
  std::unordered_map<uint64_t, stEntity> Entities;
};

namespace enity
{

stEntityBase
create_entity(stEntitySystem& entitySystem, stTransformSystem& transformSystem, const glm::vec3& startPos, const char* meshPath = nullptr, int index = -1)
{
  static int idCounter = 0;

  stEntityBase base = {};
  base.Id = idCounter++;
  base.Entity = &entitySystem.Entities[base.Id];

  // transformSystem.GetTransform(Transform);
  base.Entity->Transform = { &transformSystem.Tramsforms[base.Id], &transformSystem.Positions[base.Id] };
  *base.Entity->Transform.Tramsform = glm::translate(glm::mat4(1.0f), startPos);
  transformSystem.TransformCount++;

  if (meshPath)
  {
    std::vector<stMesh*> meshes = mesh::get_meshes(meshPath);
    for (size_t i = 0; i < meshes.size(); i++)
    {
      base.Entity->Mesh[i] = meshes[i];
      base.Entity->MeshCount++;
    }
  }
  else
  {
    base.Entity->Mesh[0] = mesh::get_mesh(index);
    base.Entity->MeshCount++;
  }

  return base;
}

}