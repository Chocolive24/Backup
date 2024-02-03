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

struct SpotLight {
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutoff;
    float outer_cutoff;
};

uniform Material material;
uniform SpotLight spot_light;

uniform vec3 viewPos;

uniform bool has_textures;
uniform bool use_blinn_phong;

void main() {
    vec3 lightDir = normalize(spot_light.position - fragPos);

    float theta     = dot(lightDir, normalize(-spot_light.direction));
    float epsilon   = spot_light.cutoff - spot_light.outer_cutoff;
    float intensity = clamp((theta - spot_light.outer_cutoff) / epsilon, 0.0, 1.0);

    // Ambient
    vec4 ambient;
    if (has_textures) {
        ambient = texture(material.diffuse_map, texCoords) * vec4(spot_light.ambient, 1.0);
    } else {
        ambient = vec4(material.diffuse * spot_light.ambient, 1.0);
    }

    // Diffuse
    vec3 norm = normalize(normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec4 diffuse;
    if (has_textures) {
        diffuse = texture(material.diffuse_map, texCoords) * vec4(diff * spot_light.diffuse, 1.0);
    } else {
        diffuse = vec4(material.diffuse * diff * spot_light.diffuse, 1.0);
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
        specular = texture(material.specular_map, texCoords) * vec4(spec * spot_light.specular, 1.0);
    } else {
        specular = vec4(material.specular * spec * spot_light.specular, 1.0);
    }

    ambient  *= intensity;
    diffuse  *= intensity;
    specular *= intensity;

    outColor = vec4(ambient + diffuse + specular);
}
