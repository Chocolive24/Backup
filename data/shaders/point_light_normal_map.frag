#version 300 es
precision highp float;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoords;

layout(location = 0) out vec4 outColor;

struct Material {
    vec3 diffuse;
    vec3 specular;
    sampler2D diffuse_map;
    sampler2D specular_map;
    sampler2D normal_map;
    float shininess;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform Material material;
uniform PointLight point_light;

uniform vec3 viewPos;

void main() {
    // Ambient
    vec4 ambient = texture(material.diffuse_map, texCoords) * vec4(point_light.ambient, 1.0);

    // Diffuse
    // obtain normal from normal map in range [0,1]
    vec3 norm = texture(material.normal_map, texCoords).rgb;
    // transform normal vector to range [-1,1]
    norm = normalize(norm * 2.0 - 1.0);
    vec3 lightDir = normalize(point_light.position - fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse = texture(material.diffuse_map, texCoords) * vec4(diff * point_light.diffuse, 1.0);

    // Specular
    vec3 viewDir = normalize(viewPos - fragPos);

    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfWayDir), 0.0), material.shininess);

    vec4 specular = texture(material.specular_map, texCoords) * vec4(spec * point_light.specular, 1.0);

    // Attenuation
    float distance = length(point_light.position - fragPos);
    float attenuation = 1.0 / (point_light.constant + point_light.linear * distance + 
                        point_light.quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    outColor = vec4(ambient + diffuse + specular);
}
