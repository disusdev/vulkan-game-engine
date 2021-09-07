

struct
stSun
{
  void
  Update(float dt, stInputState& input)
  {
    // float sunAngle = 360.0f * DayTime;
    // Timer += dt * Speed;
    // DayTime = glm::cos(Timer);

    if (input.GetKeyDown(KEY_E))
    {
      DayTime += 0.1f;
    }
    if (input.GetKeyDown(KEY_Q))
    {
      DayTime -= 0.1f;
    }

    DayTime = glm::clamp(DayTime, 0.0f, 1.0f);

    float sunAngle = 180.0f * DayTime;
    // Position = 
    glm::vec3 pos = rotateX(StartPosition, sunAngle);

    float angle = 180.0f; // 0..360
    pos = glm::rotateY(pos, angle);

    LightDirection = -glm::normalize(pos);
  }

  glm::vec3 StartPosition = {0.0f, 1.0f, 0.0f};
  glm::vec3 LightDirection;
  float DayTime = 0.5f;
  float Speed = 0.1f;
  float Timer = 0.0f;
};