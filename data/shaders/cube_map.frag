#version 300 es
precision highp float;

in vec3 texCoords;

out vec4 outColor;

uniform samplerCube cube_map;

void main() {
    outColor = texture(cube_map, texCoords);
}
