#version 460

struct ObjectData
{
	mat4 model;
};

layout( push_constant ) uniform constants
{
  mat4 viewProj;
  vec3 dirLight;
} PushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
	ObjectData objects[];
} objectBuffer;

const float AMBIENT = 0.02;

void main()
{
  mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;

  gl_Position = PushConstants.viewProj * modelMatrix * vec4(inPosition, 1.0);
  fragTexCoord = inTexCoord;
  fragNormal = inNormal;
  // fragDirLight = PushConstants.dirLight;

  // mat3 normalMatrix = transpose(inverse(mat3(PushConstants.model)));

  // vec3 normalWorldSpace = normalize(normalMatrix * inNormal);

  // float lightIntensity = AMBIENT + max(dot(normalWorldSpace, PushConstants.dirLight), 0);

  fragColor = inColor; // lightIntensity * inColor;
}