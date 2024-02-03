#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 texCoords;

struct Transform{
  mat4 model;
  mat4 view;
  mat4 projection;
};

uniform Transform transform;

void main()
{
    texCoords = aTexCoords;
    gl_Position = transform.projection * transform.view * transform.model * vec4(aPos, 1.0);
}