#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;

out vec3 texCoords;

struct Transform{
  mat4 view;
  mat4 projection;
};

uniform Transform transform;

void main() {
    texCoords = aPos;
    vec4 pos = transform.projection * transform.view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
