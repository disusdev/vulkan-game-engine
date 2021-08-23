#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
  vec3 lightDirection = vec3(1, 1, 1);
  float lightNormalAngle = max(dot(lightDirection, fragNormal), 0.0f);
  outColor = vec4(lightNormalAngle * fragColor * texture(texSampler, fragTexCoord).rgb , 1.0); // lightNormalAngle * // * texture(texSampler, fragTexCoord).rgb
  // outColor = vec4(0.0, 0.0, gl_FragCoord.z * 5, 1.0);
}