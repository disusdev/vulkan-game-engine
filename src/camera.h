
struct
stCamera
{
  glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
  glm::vec3 Velocity = { 0.0f, 0.0f, 0.0f };

  glm::vec3 Forward = { 0.0f, 0.0f, 0.0f };
	glm::vec3 Right = { 0.0f, 0.0f, 0.0f };
	glm::vec3 Up = { 0.0f, 0.0f, 0.0f };

  float Pitch = 0.0f;
  float Yaw = 0.0f;

  bool Sprint = false;
  bool Locked = false;

  void
  update_camera(stInputState& input_state, double dt);

  glm::mat4
  get_view_matrix();

  glm::mat4
  get_projection_matrix(const glm::vec2& wondowSize, bool reverse = true);

  glm::mat4
  get_rotation_matrix();
};

void
stCamera::update_camera(stInputState& input_state, double dt)
{
  if (input_state.GetKeyDown(KEY_TAB))
  {
    Locked = !Locked;
  }

  const float cam_vel = 1.0f + Sprint * 0.01f;

  Forward = { 0, 0, cam_vel };
	Right = { cam_vel, 0, 0 };
	Up = { 0, cam_vel, 0 };
  
  Yaw += input_state.RotationDelta.x * (float)dt;// * 10.0f;
  Pitch += input_state.RotationDelta.y * (float)dt;// * 10.0f;

  glm::mat4 cam_rot = get_rotation_matrix();

  Forward = cam_rot * glm::vec4(Forward, 0.f);
	Right = cam_rot * glm::vec4(Right, 0.f);

  glm::vec3 inputAxis = {
    (float)input_state.GetKey(KEY_LEFT) - (float)input_state.GetKey(KEY_RIGHT),
    (float)input_state.GetKey(KEY_DOWN) - (float)input_state.GetKey(KEY_UP),
    (float)input_state.GetKey(KEY_FORWARD) - (float)input_state.GetKey(KEY_BACKWARD)
  };

  Velocity = inputAxis.z * Forward + inputAxis.x * Right + inputAxis.y * Up;

  Position -= Velocity * (float)dt;
}

glm::mat4
stCamera::get_view_matrix()
{
  glm::vec3 camPos = Position;

	glm::mat4 cam_rot = (get_rotation_matrix());

	glm::mat4 view = glm::translate(glm::mat4{ 1 }, camPos) * cam_rot;

	//we need to invert the camera matrix
	view = glm::inverse(view);

	return view;
}

glm::mat4
stCamera::get_projection_matrix(const glm::vec2& wondowSize, bool reverse)
{
  if (reverse)
	{
		glm::mat4 pro = glm::perspective(glm::radians(75.f), wondowSize.x / (float) wondowSize.y, 5000.0f, 0.1f);
		pro[1][1] *= -1;
		return pro;
	}
	else
  {
		glm::mat4 pro = glm::perspective(glm::radians(75.f), wondowSize.x / (float) wondowSize.y, 0.1f, 5000.0f);
		pro[1][1] *= -1;
		return pro;
	}
}

glm::mat4
stCamera::get_rotation_matrix()
{
  glm::mat4 yaw_rot = glm::rotate(glm::mat4{ 1 }, Yaw, {0.0f,-1.0f,0.0f});
	glm::mat4 pitch_rot = glm::rotate(glm::mat4{ yaw_rot }, Pitch, {-1.0f,0.0f,0.0f});

	return pitch_rot;
}