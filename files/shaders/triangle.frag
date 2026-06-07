#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) flat in uint fragMaterialIndex;

layout(location = 0) out vec4 outColor;

struct Light {
  vec4 directionType; // xyz = direction, w = type
  vec4 colorForce;    // rgb = color, w = force
};

layout(std430, set = 0, binding = 0) readonly buffer Lights {
  uint count;
  vec3 ambient;
  Light items[];
} lights;

struct Material {
  vec4 color;
};

layout(std430, set = 1, binding = 0) readonly buffer Materials {
  Material items[];
} materials;

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 baseColor = materials.items[fragMaterialIndex].color.rgb;

  vec3 lit = baseColor * lights.ambient;
  for (uint i = 0u; i < lights.count; ++i) {
    vec3 lightDirection = normalize(-lights.items[i].directionType.xyz);
    float diffuse = max(dot(normal, lightDirection), 0.0) * lights.items[i].colorForce.w;
    lit += baseColor * lights.items[i].colorForce.rgb * diffuse;
  }

  outColor = vec4(lit, 1.0);
}
