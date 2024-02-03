#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;

out vec3 fragColor;

vec3 colors[3] = vec3[](
    vec3(1.0, 0.9, 0.9),
    vec3(0.5, 0.0, 1.0),
    vec3(1.0, 0.5, 0.5)
);

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    fragColor = colors[gl_VertexID];
}
