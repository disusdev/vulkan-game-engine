
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
  std::unordered_map<uint64_t, glm::mat4> Tramsforms;
  std::unordered_map<uint64_t, glm::vec3> Positions;

  void
  GetTransform(glm::mat4* transform)
  {
    transform = &Tramsforms[TransformCount++];
  }
};