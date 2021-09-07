#version 450

layout( push_constant ) uniform constants
{
  mat4 model;
  mat4 view;
  mat4 proj;
  vec3 dirLight;
} PushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragDirLight;

const float AMBIENT = 0.02;

void main()
{
  gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * vec4(inPosition, 1.0);
  fragTexCoord = inTexCoord;
  fragNormal = inNormal;
  fragDirLight = PushConstants.dirLight;

  mat3 normalMatrix = transpose(inverse(mat3(PushConstants.model)));
  vec3 normalWorldSpace = normalize(normalMatrix * inNormal);

  float lightIntensity = AMBIENT + max(dot(normalWorldSpace, PushConstants.dirLight), 0);

  fragColor = lightIntensity * inColor;
}