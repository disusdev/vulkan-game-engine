#version 450

layout( push_constant ) uniform constants
{
  mat4 model;
  mat4 view;
  mat4 proj;
} PushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main()
{
  gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * vec4(inPosition, 1.0);
  fragColor = vec3(1.0);
  fragTexCoord = inTexCoord;
  fragNormal = inNormal;
}