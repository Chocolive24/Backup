#version 300 es
precision highp float;

in vec3 fragPos;
in vec3 normal;

layout(location = 0) out vec4 outColor;

struct Material{
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
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
    vec3 ambient = material.ambient * light.ambient;

    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - fragPos); // direction to the light.

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = material.diffuse * diff * light.diffuse;

    // specluar
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm); // direction from the light.

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * light.specular;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
