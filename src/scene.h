
struct
stScene
{
  void
  Load(stEntitySystem& entitySystem,
       stTransformSystem& transformSystem)
  {
    stEntityBase base = enity::create_entity(entitySystem, transformSystem, glm::vec3(0.0f,0.0f,0.0f), "./data/models/lost-empire/loast-empire.gltf");
    *base.Entity->Transform.Tramsform = glm::rotate(*base.Entity->Transform.Tramsform, glm::radians(90.0f),glm::vec3(1.0f, 0.0f, 0.0f));
    Entities.push_back(base);
  }

  // TODO: unload scene that were closed
  void
  Unload()
  {
  }

  struct
  scene_header
  {
    uint32_t EntitiesCount = 0;
  };

  struct
  scene_entity
  {
    // int64_t Id = -1; -
    // const char* Name = "entity";
    // stEntity* Entity = nullptr;
    // uint8_t Active = false;
    // name buffer
    uint32_t NameLength;
    uint32_t Active;
  };

  // TODO: save current active scene
  void
  Save()
  {
    // stFile file = stFileSystem.CreateFile(name, FILE_BINARY);
    // file.write<scene_header>(scene_header);
    // file.write<scene_entity>(scene_entity, count);
  }

  void
  CombineMeshes()
  {
  }

  std::vector<stEntityBase> Entities;

  std::string Name;
};