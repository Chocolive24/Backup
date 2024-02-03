#version 300 es
precision highp float;

layout(location = 0) out vec4 outColor;

uniform vec4 fragColor;

void main() {
    outColor = fragColor;
}