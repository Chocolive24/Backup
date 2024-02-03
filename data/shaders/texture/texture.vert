#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoords;

out vec3 fragColor;
out vec2 texCoords;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    fragColor = aColor;
    texCoords = aTexCoords;
}
