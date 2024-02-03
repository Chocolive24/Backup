#version 300 es
precision highp float;

in vec2 texCoords;

layout(location = 0) out vec4 outColor;

uniform sampler2D outTexture;

uniform vec3 texture_color;

void main() {
    outColor = texture(outTexture, texCoords) * vec4(texture_color, 1.0);
}
