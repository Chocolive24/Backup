#version 300 es
precision highp float;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

layout(location = 0) out vec4 outColor;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_specular1;
  float shininess;
};

uniform Material material;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    // Texture color.
    vec3 diffuse_map_col = texture(material.texture_diffuse1, texCoords).rgb;
    vec3 specular_map_col = texture(material.texture_specular1, texCoords).rgb;

    // ambient
    vec3 ambient = diffuse_map_col * 0.1;

    // diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos); // direction to the light.

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuse_map_col * diff;

    // specluar
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfWayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(norm, halfWayDir), 0.0), material.shininess);
    vec3 specular = specular_map_col * spec;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}