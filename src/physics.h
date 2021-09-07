
struct
stPhysicsSystem
{
  struct
  stConfigs
  {
    float Gravity = -9.8f; // per sec
  };

  stConfigs Configs;
  
  void
  Update(float dt)
  {
    // for all transforms translate on gravity
    for (size_t i = 0; i < TransformSystem->TransformCount; i++)
    {
      glm::vec3 force = { 0.0f, Configs.Gravity * 0.001, 0.0f };
      Velocities[i].y += force.y * dt;
      TransformSystem->Positions[i] += Velocities[i] * dt;
      TransformSystem->Tramsforms[i] = glm::translate(TransformSystem->Tramsforms[i], TransformSystem->Positions[i]);
    }
  }
  // control transforms and entity relates with each other

  // what we need: TransmormSystem
  std::unordered_map<uint64_t, glm::vec3> Velocities;
  
  stTransformSystem* TransformSystem;
};