

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

  stMesh Mesh;
  stTransform Transform;
  stEntity* Childrens[MAX_ENTITY_CILDRENS];
  stEntity* Parent = nullptr;
};

struct
stEntitySystem
{
  stEntity Entities[MAX_ENTITIES_COUNT];
};

namespace enity
{

stEntityBase
create_entity(stEntitySystem& entitySystem, stTransformSystem& transformSystem, const glm::vec3& startPos, const char* meshPath = nullptr)
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
    base.Entity->Mesh.LoadMesh(meshPath);
  } 

  return base;
}

}