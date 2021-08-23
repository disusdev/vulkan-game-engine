
struct
stTransform
{
  glm::mat4* Tramsform;
  glm::vec3* Position;
};

struct
stTransformSystem
{
  uint32_t TransformCount = 0;
  glm::mat4 Tramsforms[MAX_ENTITIES_COUNT];
  glm::vec3 Positions[MAX_ENTITIES_COUNT];

  void
  GetTransform(glm::mat4* transform)
  {
    transform = &Tramsforms[TransformCount++];
  }
};