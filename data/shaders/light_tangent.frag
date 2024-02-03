#version 300 es
precision highp float;

in vec2 texCoords;
in vec3 tangentLightPos;
in vec3 tangentViewPos;
in vec3 tangentFragPos;

layout(location = 0) out vec4 outColor;

struct Material{
  sampler2D texture_diffuse1;
  sampler2D texture_normal1;
  float shininess;
};

uniform Material material;

void main() {
    // Normal
    vec3 normal = texture(material.texture_normal1, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0); //[0,1] -> [-1,1]

    // Texture color.
    vec3 tex_diffuse = texture(material.texture_diffuse1, texCoords).rgb;

    // ambient
    vec3 ambient = tex_diffuse * 0.1;

    // diffuse
    vec3 lightDir = normalize(tangentLightPos - tangentFragPos); // direction to the light.

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = tex_diffuse * diff;

    // specluar
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    vec3 halfWayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(normal, halfWayDir), 0.0), material.shininess);
    vec3 specular = vec3(0.2f) * spec;

    outColor = vec4(ambient + diffuse + specular, 1.0);
}
