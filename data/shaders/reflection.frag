#version 300 es
precision highp float;

in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

layout(location = 0) out vec4 outColor;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
};

uniform vec3 camera_pos;
uniform samplerCube cube_map;
uniform Material material;
uniform float reflection_factor;

void main() {
    vec3 I = normalize(fragPos - camera_pos);
    vec3 R = reflect(I, normalize(normal));

    vec3 reflection_color = texture(cube_map, R).rgb;
    vec3 diffuse_color = texture(material.texture_diffuse1, texCoords).rgb;

    outColor = vec4(mix(diffuse_color, reflection_color, reflection_factor),1.0);
}
