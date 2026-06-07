#version 450

// Per-vertex (binding 0)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

// Per-instance (binding 1): model matrix as 4 vec4 columns + material index
layout(location = 2) in vec4 inModel0;
layout(location = 3) in vec4 inModel1;
layout(location = 4) in vec4 inModel2;
layout(location = 5) in vec4 inModel3;
layout(location = 6) in uint inMaterialIndex;

layout(push_constant) uniform PushConstants {
  mat4 view;
  mat4 projection;
} pushConstants;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) flat out uint fragMaterialIndex;

void main() {
  mat4 model = mat4(inModel0, inModel1, inModel2, inModel3);

  // Normal transform. Correct for rotation and uniform scale.
  fragNormal = normalize(mat3(model) * inNormal);
  fragMaterialIndex = inMaterialIndex;

  gl_Position = pushConstants.projection * pushConstants.view * model * vec4(inPosition, 1.0);
}
