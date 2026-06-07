#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 sunDirectionForce;
  vec4 sunColorAmbient;
} pushConstants;

void main() {
  fragNormal = normalize(inNormal);
  fragColor = inColor;
  gl_Position = pushConstants.projection * pushConstants.view * pushConstants.model * vec4(inPosition, 1.0);
}
