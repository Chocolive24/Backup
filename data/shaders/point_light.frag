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

uniform bool has_textures;
uniform bool use_blinn_phong;

void main() {
    // Ambient
    vec4 ambient;
    if (has_textures) {
        ambient = texture(material.diffuse_map, texCoords) * vec4(point_light.ambient, 1.0);
    } else {
        ambient = vec4(material.diffuse * point_light.ambient, 1.0);
    }

    // Diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(point_light.position - fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse;
    if (has_textures) {
        diffuse = texture(material.diffuse_map, texCoords) * vec4(diff * point_light.diffuse, 1.0);
    } else {
        diffuse = vec4(material.diffuse * diff * point_light.diffuse, 1.0);
    }

    // Specular
    vec3 viewDir = normalize(viewPos - fragPos);
    float spec;
    if (!use_blinn_phong) {
        vec3 reflectDir = reflect(-lightDir, norm);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    } else {
        vec3 halfWayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(norm, halfWayDir), 0.0), material.shininess);
    }

    vec4 specular;
    if (has_textures) {
        specular = texture(material.specular_map, texCoords) * vec4(spec * point_light.specular, 1.0);
    } else {
        specular = vec4(material.specular * spec * point_light.specular, 1.0);
    }

    // Attenuation
    float distance = length(point_light.position - fragPos);
    float attenuation = 1.0 / (point_light.constant + point_light.linear * distance + point_light.quadratic * (distance * distance));

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    outColor = vec4(ambient + diffuse + specular);
}
