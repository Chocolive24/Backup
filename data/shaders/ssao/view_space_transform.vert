#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoords;

struct Transform{
  mat4 model;
  mat4 view;
  mat4 projection;
};

uniform Transform transform;
uniform mat4 normalMatrix;

void main() {
    vec4 viewPos = transform.view * transform.model * vec4(aPos, 1.0);
    fragPos = viewPos.xyz;
    texCoords = aTexCoords;
    normal = mat3(normalMatrix) * aNormal;

    gl_Position = transform.projection * viewPos;
}