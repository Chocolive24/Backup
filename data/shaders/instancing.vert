#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 5) in mat4 aModel; // 5 because location 3 and 4 are for tangent and bitangent.

out vec3 fragPos;
out vec3 normal;
out vec2 texCoords;

uniform mat4 view;
uniform mat4 projection;

uniform mat4 normalMatrix;

void main() {
    gl_Position = projection * view * aModel * vec4(aPos, 1.0);
    fragPos = vec3(aModel * vec4(aPos, 1.0));
    texCoords = aTexCoords;
    normal = mat3(normalMatrix) * aNormal;
}