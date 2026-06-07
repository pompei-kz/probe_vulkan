#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec4 sunDirectionForce;
  vec4 sunColorAmbient;
} pushConstants;

void main() {
  vec3 normal = normalize(fragNormal);
  vec3 lightDirection = normalize(-pushConstants.sunDirectionForce.xyz);
  float diffuse = max(dot(normal, lightDirection), 0.0) * pushConstants.sunDirectionForce.w;
  vec3 ambient = fragColor * pushConstants.sunColorAmbient.a;
  vec3 lit = ambient + fragColor * pushConstants.sunColorAmbient.rgb * diffuse;
  outColor = vec4(lit, 1.0);
}
