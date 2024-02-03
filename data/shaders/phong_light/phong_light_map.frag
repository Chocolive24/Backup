#version 300 es
precision highp float;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

layout(location = 0) out vec4 outColor;

struct Material{
  sampler2D diffuse_map;
  sampler2D specular_map;
  float shininess;
};

uniform Material material;

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform Light light;

uniform vec3 viewPos;

void main() {
    // ambient
    vec4 ambient = texture(material.diffuse_map, texCoords) * vec4(light.ambient, 1.0);

    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - fragPos); // direction to the light.

    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = texture(material.diffuse_map, texCoords) * 
                   vec4(diff * light.diffuse, 1.0);

    // specluar
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm); // direction from the light.

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec4 specular = texture(material.specular_map, texCoords) * 
                            vec4(spec * light.specular, 1.0);

    outColor = vec4(ambient + diffuse + specular);
}
