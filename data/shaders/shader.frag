#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D depthSampler;

void main()
{
  outColor = vec4( fragColor * texture(texSampler, fragTexCoord).rgb , 1.0);
  // outColor = vec4(0.0, 0.0, gl_FragCoord.z * 5, 1.0);
}